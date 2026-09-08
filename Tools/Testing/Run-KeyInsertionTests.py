"""Exercise the saved parking map with real E input; runs only owned local processes."""
import argparse
import json
import re
import subprocess
import time
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('mode', choices=['views', 'network'])
p.add_argument('--output', required=True)
p.add_argument('--engine', default='D:/Unreal5.8/UE_5.8')
p.add_argument('--game-port', type=int, default=19677)
p.add_argument('--beacon-port', type=int, default=19650)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
out = Path(a.output).resolve()
out.mkdir(parents=True, exist_ok=True)
engine = Path(a.engine)/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
running = []
report = {'mode': a.mode, 'result': 'RUNNING', 'processes': []}


def read(item):
    return item[1].read_text(encoding='utf-8', errors='replace') if item[1].exists() else ''


def launch(role, map_name, flags):
    capture = out/role
    capture.mkdir(exist_ok=True)
    log = out/(role+'.log')
    command = [str(engine), str(root/'backrooms.uproject'), map_name, '-unattended', '-nop4',
               '-nosound', '-stdout', '-abslog='+str(log), '-BRKeyInsertionSmoke', *flags]
    if role != 'server':
        command += ['-RenderOffscreen', '-windowed', '-ForceRes', '-ResX=1280', '-ResY=720',
                    '-ExecCmds=t.MaxFPS 30', '-BRKeyCapture='+str(capture)]
    stream = (out/(role+'.console.log')).open('wb')
    proc = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
    record = {'role': role, 'pid': proc.pid, 'command': command}
    report['processes'].append(record)
    item = (proc, log, stream, record)
    running.append(item)
    print('START', role, proc.pid, flush=True)
    return item


def wait_for(item, marker, timeout=240):
    end = time.monotonic()+timeout
    while marker not in read(item):
        log = read(item)
        if re.search(r'result=FAIL|Fatal error:|Assertion failed:', log):
            raise AssertionError(item[3]['role']+' reported a failure')
        if item[0].poll() is not None:
            raise RuntimeError(item[3]['role']+' exited before '+marker)
        if time.monotonic() > end:
            raise TimeoutError(item[3]['role']+': '+marker)
        time.sleep(.5)
    assert 'result=FAIL' not in read(item), item[3]['role']


def finish(item):
    item[0].wait(timeout=40)
    assert item[0].returncode == 0, item[0].returncode
    assert not re.search(r'result=FAIL|Fatal error:|Assertion failed:', read(item))


try:
    if a.mode == 'views':
        game = launch('standalone', '/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark', ['-game', '-BRKeyResultReturn'])
        wait_for(game, 'BR_KEY_CLIENT case=RETURN_MAIN_MENU result=PASS')
        finish(game)
        assert 'BR_KEY_TEST result=PASS' in read(game)
        assert 'BR_KEY_CLIENT case=SUCCESS_PAGE result=PASS' in read(game)
    else:
        common = ['-BRGamePort='+str(a.game_port), '-BRBeaconPort='+str(a.beacon_port)]
        server = launch('server', '/Game/UI/Menu/Maps/L_Lobby', ['-server', '-NullRHI', '-port='+str(a.game_port), *common])
        wait_for(server, 'BR_BEACON result=LISTENING')
        clients = []
        for role in ['host', 'guest']:
            clients.append(launch(role, '/Game/UI/Menu/Maps/L_MainMenu', ['-game', '-BRServerHost=127.0.0.1',
                                  '-BRTestRole='+role, '-BRExpectedPlayers=2', '-BRTestRounds=2', '-BRTestTimeout=300', *common]))
        for item in clients:
            wait_for(item, 'result=ROUND_TRIP_PASS round=2', 330)
            finish(item)
            for case in ['FRESH_ROUND_EMPTY', 'FOUR_COLLECTED_STILL_CLOSED', 'FIRST_INSERT_REPLICATED_ONCE',
                         'THREE_INSERTED_STILL_CLOSED', 'FOUR_INSERTED_DOOR_OPEN', 'SUCCESS_PAGE']:
                assert read(item).count('BR_KEY_CLIENT case='+case+' result=PASS') == 2, (item[3]['role'], case)
        assert read(server).count('BR_KEY_TEST result=PASS') == 2
    for item in running:
        report[item[3]['role']+'_checks'] = re.findall(r'BR_KEY_(?:TEST|CLIENT) case=(\w+) result=(\w+)', read(item))
    report['result'] = 'PASS'
except Exception as error:
    report['result'] = 'FAIL'
    report['error'] = repr(error)
finally:
    for proc, log, stream, record in running:
        record['natural_exit'] = proc.poll()
        if proc.poll() is None:
            proc.terminate()
            try: proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=10)
            record['stopped_owned_process'] = True
        stream.close()
    report['captures'] = [str(x.relative_to(out)) for x in out.glob('*/*.png')]
    (out/'RESULT.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(report['result'], report.get('error', ''), flush=True)
raise SystemExit(0 if report['result']=='PASS' else 1)
