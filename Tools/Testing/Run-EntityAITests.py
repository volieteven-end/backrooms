"""Real garage navigation, patrol, closed-door pursuit and attack presentation.

Only explicitly launched local processes are owned/stopped. Fixtures never save maps.
"""
import argparse
import json
import re
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=['standalone', 'network'])
parser.add_argument('--output', required=True)
parser.add_argument('--engine', default='D:/Unreal5.8/UE_5.8')
parser.add_argument('--render', action='store_true')
parser.add_argument('--doors-only', action='store_true', help='Focused door diagnostic; does not verify patrol coverage')
parser.add_argument('--rounds', type=int, default=1)
parser.add_argument('--game-port', type=int, default=19777)
parser.add_argument('--beacon-port', type=int, default=19750)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
exe = Path(args.engine) / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
owned = []
result = {'result': 'RUNNING', 'mode': args.mode, 'rendered': args.render, 'scope': 'door diagnostics' if args.doors_only else 'full patrol and door integration', 'processes': []}
failure_pattern = re.compile(r'result=FAIL|Fatal error:|Assertion failed:')


def read(item):
    return item['log'].read_text(encoding='utf-8', errors='replace') if item['log'].exists() else ''


def launch(role, map_name, flags):
    capture = out / role
    capture.mkdir(exist_ok=True)
    log = out / (role + '.log')
    command = [str(exe), str(root/'backrooms.uproject'), map_name, '-unattended', '-nop4', '-nosplash',
               '-stdout', '-abslog='+str(log), '-BREntityAISmoke', '-BREntityAILog', '-BRAudioLog', *flags]
    if args.doors_only: command.append('-BRAIDoorsOnly')
    if role != 'server' and args.render:
        command += ['-RenderOffscreen', '-windowed', '-ForceRes', '-ResX=1280', '-ResY=720',
                    '-ExecCmds=t.MaxFPS 30', '-BRAICapture='+str(capture)]
    else:
        command += ['-NullRHI', '-nosound', '-ExecCmds=t.MaxFPS 60']
    stream = (out / (role + '.console.log')).open('wb')
    process = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
    record = {'role': role, 'pid': process.pid, 'command': command}
    result['processes'].append(record)
    item = {'process': process, 'log': log, 'stream': stream, 'record': record}
    owned.append(item)
    print('START', role, process.pid, flush=True)
    return item


def wait_for(item, marker, timeout):
    deadline = time.monotonic() + timeout
    while True:
        text = read(item)
        if re.search(r'Fatal error:|Assertion failed:', text):
            raise AssertionError(item['record']['role']+' reported failure; see log')
        if marker in text:
            return
        if item['process'].poll() is not None:
            raise RuntimeError(item['record']['role']+' exited before '+marker)
        if time.monotonic() >= deadline:
            raise TimeoutError(item['record']['role']+': '+marker)
        time.sleep(.5)


def finish(item):
    item['process'].wait(timeout=30)
    assert item['process'].returncode == 0
    assert not failure_pattern.search(read(item))


try:
    if args.mode == 'standalone':
        authority = launch('standalone', '/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark', ['-game'])
        clients = [authority]
        wait_for(authority, 'BR_AI_TEST result=', 270)
        finish(authority)
        rounds = 1
    else:
        common = ['-BRGamePort='+str(args.game_port), '-BRBeaconPort='+str(args.beacon_port)]
        authority = launch('server', '/Game/UI/Menu/Maps/L_Lobby', ['-server', '-port='+str(args.game_port), *common])
        wait_for(authority, 'BR_BEACON result=LISTENING', 60)
        clients = [launch(role, '/Game/UI/Menu/Maps/L_MainMenu', ['-game', '-BRServerHost=127.0.0.1',
                         '-BRTestRole='+role, '-BRExpectedPlayers=2', '-BRTestRounds='+str(args.rounds),
                         '-BRTestTimeout=600', *common]) for role in ['host', 'guest']]
        rounds = args.rounds
        for client in clients:
            wait_for(client, 'result=ROUND_TRIP_PASS round='+str(rounds), 270*rounds+60)
            finish(client)
        assert read(authority).count('BR_AI_TEST result=PASS') == rounds
    required = ['WORLD_COLLISION_SIZE', 'DYNAMIC_NAVIGATION', 'PATROL_COVERS_GARAGE', 'DISTANT_PATROL_GOALS',
                'REAL_SIGHT_STARTS_CHASE', 'PLAYER_CLOSES_DOOR_DURING_CHASE', 'CLOSED_DOOR_SIGHT_GRACE',
                'CHASE_EXPIRES_AFTER_LOST_SIGHT', 'CHASE_STAMINA_RELEASED', 'WALKS_AWAY_FROM_CLOSED_DOOR',
                'REOPENED_DOOR_NAV_READY', 'ATTACK_ONCE_ON_SERVER', 'WALKS_THROUGH_REOPENED_DOOR', 'NO_REPEAT_ATTACK_ON_DOWNED_PLAYER']
    required += ['DOOR_'+str(i)+'_'+case for i in range(6) for case in ['NAV_POINTS', 'CLOSED_BLOCKS_PATH', 'OPEN_RESTORES_PATH']]
    if args.doors_only:
        required = [c for c in required if c not in ['PATROL_COVERS_GARAGE', 'DISTANT_PATROL_GOALS']]
    for case in required:
        assert read(authority).count('BR_AI_TEST case='+case+' result=PASS') >= rounds, case
    for client in clients:
        assert read(client).count('BR_AI_CLIENT case=ATTACK_ONCE result=PASS') == rounds
        if args.render:
            assert 'BR_ATTACK_AUDIO ' in read(client)
            assert (out/client['record']['role']/'attack.wav').exists()
    result['cases'] = required
    result['result'] = 'PASS'
except Exception as error:
    result['result'] = 'FAIL'
    result['error'] = repr(error)
finally:
    for item in owned:
        process = item['process']
        item['record']['natural_exit'] = process.poll()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=10)
            item['record']['stopped_owned_process'] = True
        item['stream'].close()
        item['record']['checks'] = re.findall(r'BR_AI_TEST case=(\w+) result=(\w+)', read(item))
    (out/'RESULT.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(result['result'], result.get('error', ''), flush=True)
raise SystemExit(0 if result['result'] == 'PASS' else 1)
