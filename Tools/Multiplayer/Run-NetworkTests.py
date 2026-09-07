"""Separate-process local integration tests. Never counts as WAN acceptance.
Editor runtime is the default; pass matching packaged ClientExe/ServerExe to test products.
The smoke scenario deliberately invokes a server-owned fixture (BRNetworkSmoke).
"""
from pathlib import Path
import argparse, subprocess, time, json, sys, re

P=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser()
ap.add_argument('--engine',default=r'D:\Unreal5.8\UE_5.8')
ap.add_argument('--client-exe');ap.add_argument('--server-exe')
ap.add_argument('--output',required=True);ap.add_argument('--suite',choices=['smoke','faults'],default='smoke')
ap.add_argument('--outcome',choices=['timer','extract','down'],default='extract')
ap.add_argument('--game-port',type=int,default=7777);ap.add_argument('--beacon-port',type=int,default=15000)
a=ap.parse_args();OUT=Path(a.output).resolve();OUT.mkdir(parents=True,exist_ok=True)
EDITOR=Path(a.engine)/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
procs=[];records=[];checks=[]
def read(log):return log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
def start(role,server=False,extra=()):
    exe=Path(a.server_exe if server else a.client_exe) if (a.server_exe if server else a.client_exe) else EDITOR
    map='/Game/UI/Menu/Maps/'+('L_Lobby' if server else 'L_MainMenu')
    cmd=[str(exe)]
    if exe==EDITOR:cmd += [str(P/'backrooms.uproject'),map,'-server' if server else '-game']
    else:cmd += [map]
    if server:cmd+=['-port='+str(a.game_port)]
    else:cmd+=['-BRServerHost=127.0.0.1']
    cmd += [f'-BRGamePort={a.game_port}',f'-BRBeaconPort={a.beacon_port}','-BRTestTimeout=360']
    cmd += list(extra)+['-NullRHI','-unattended','-nop4','-nosound','-NoSplash','-abslog='+(OUT/(role+'.log')).as_posix()]
    si=subprocess.STARTUPINFO();si.dwFlags|=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
    f=(OUT/(role+'_stdout.log')).open('wb')
    proc=subprocess.Popen(cmd,stdout=f,stderr=subprocess.STDOUT,startupinfo=si,creationflags=subprocess.CREATE_NO_WINDOW)
    record={'role':role,'pid':proc.pid,'command':cmd};records.append(record)
    item=(proc,OUT/(role+'.log'),f,record);procs.append(item)
    print('NET_START role='+role+' pid='+str(proc.pid),flush=True);return item

def wait_log(item,marker,timeout=100):
    proc,log,_,_=item;end=time.monotonic()+timeout
    while marker not in read(log):
        if proc.poll() is not None or time.monotonic()>end:raise AssertionError(f'{log.name}: missing {marker}, exit={proc.poll()}')
        time.sleep(.5)
    return read(log)

def completed(item,marker,timeout=100):
    wait_log(item,marker,timeout);item[0].wait(timeout=90)
    assert item[0].returncode==0,(item[1].name,item[0].returncode)
    checks.append({'case':marker,'log':str(item[1]),'result':'PASS','exit':0})

def stop(item,reason):
    proc,_,_,record=item
    if proc.poll() is None:record['stop_reason']=reason;proc.terminate();proc.wait(timeout=120)

def probe(name,expected):
    item=start(name,extra=['-BRTestRole=probe:'+expected]);completed(item,f'BR_PROBE case={expected} result=PASS');return item

error=''
try:
    if a.suite=='smoke':
        server_args=['-BRNetworkSmoke']
        if a.outcome!='timer':server_args+=['-BRRealRoundSmoke']
        if a.outcome=='down':server_args+=['-BRSmokeAllDown']
        server=start('server',True,server_args);wait_log(server,'BR_BEACON result=LISTENING')
        late=start('late_probe',extra=['-BRTestRole=probe:IN_PROGRESS'])
        clients=[]
        for role in ['host','guest','guest2','guest3']:
            clients.append((role,start(role,extra=['-BRTestRole='+role,'-BRExpectedPlayers=4','-BRTestRounds=2'])));time.sleep(1)
        for role,item in clients:
            completed(item,f'BR_NETTEST role={role} result=ROUND_TRIP_PASS round=2',timeout=170)
            log=read(item[1])
            for case in ['KEY_COUNT','DOOR_OPEN','OWNER_HIDDEN_ON','OWNER_HIDDEN_OFF']:
                assert log.count(f'BR_CLIENT_SYNC role={role} case={case} result=PASS')>=2,(role,case)
        for case in ['HIDDEN_ON','HIDDEN_OFF']:
            assert read(clients[0][1][1]).count(f'BR_CLIENT_SYNC role=host case={case} result=PASS')>=2,case
        server_text=read(server[1])
        for case in ['HIDE_ENTER','HIDE_EXIT','KEY_ONCE','DOOR_OPEN','AI_MOVED']:
            assert server_text.count(f'BR_GAMEPLAY_SMOKE case={case} result=PASS')>=2,case
        if a.outcome!='timer':
            case='ALL_DOWN_END' if a.outcome=='down' else 'EXTRACTION_OVERLAP_END'
            assert server_text.count(f'BR_GAMEPLAY_SMOKE case={case} result=PASS')>=2,case
        locations=re.findall(r'BR_PLAYER_START result=ASSIGNED name=(\S+) location=(.*)',server_text)
        assert len(locations)==8 and len(set(name for name,_ in locations))==4,'unique starts per round'
        assert 'BR_ENTITY_NAVIGATION result=READY nav_data=present' in server_text
        completed(late,'BR_PROBE case=IN_PROGRESS result=PASS')
        wait_log(server,'BR_LOGOUT players=0');probe('empty_after_rounds','EMPTY')
        checks.append({'case':'four clients, two rounds, four unique starts, hidden/key/door replication, server AI moved','result':'PASS'})
    else:
        server=start('server',True);wait_log(server,'BR_BEACON result=LISTENING')
        probe('initial_empty','EMPTY');probe('wrong_version','VERSION')
        host=start('host',extra=['-BRTestRole=host','-BRTestNoStart'])
        wait_log(server,'BR_LOGIN result=ADMITTED players=1')
        clients=[]
        for role in ['guest','guest2','guest3']:
            clients.append(start(role,extra=['-BRTestRole='+role,'-BRTestNoStart','-BRTestIllegalStart']));time.sleep(1)
        wait_log(server,'BR_LOGIN result=ADMITTED players=4')
        for item in clients:wait_log(item,'BR_PROBE case=NON_OWNER_START result=PASS')
        checks.append({'case':'non-owner Start RPC rejected on server','result':'PASS'})
        probe('full','FULL');probe('duplicate_create','ROOM_EXISTS')
        stop(clients[0],'fault injection: ordinary client process loss')
        wait_log(server,'BR_LOGOUT players=3')
        checks.append({'case':'ordinary client process loss releases slot','result':'PASS'})
        for item in [host,*clients]:stop(item,'end capacity fixture')
        wait_log(server,'BR_LOGOUT players=0');probe('empty_after_disconnects','EMPTY')
        # Owner leaves intentionally. Earliest remaining member becomes owner, then exits normally.
        host2=start('migration_host',extra=['-BRTestRole=host','-BRTestNoStart','-BRTestLeaveAfter=12'])
        wait_log(host2,'BR_PRESENTATION map=L_Lobby')
        guest=start('migration_guest',extra=['-BRTestRole=guest','-BRTestNoStart','-BRExpectOwnerTransfer'])
        completed(host2,'BR_FAULT case=LEAVE result=PASS')
        completed(guest,'BR_FAULT case=OWNER_TRANSFER result=PASS')
        probe('empty_after_migration','EMPTY')
        # Stop the known server, not any unrelated UE process. Client must return to menu.
        host3=start('offline_client',extra=['-BRTestRole=host','-BRTestNoStart','-BRExpectDisconnect'])
        wait_log(host3,'BR_PRESENTATION map=L_Lobby');time.sleep(2)
        stop(server,'fault injection: server process loss')
        completed(host3,'BR_FAULT case=DISCONNECT result=PASS',timeout=100)
except Exception as e:
    error=str(e)
finally:
    for item in procs:
        proc,log,f,record=item;record['natural_exit']=proc.poll()
        stop(item,'harness cleanup');f.close();record['exit_after_cleanup']=proc.returncode
    report={'result':'FAIL' if error else 'PASS','suite':a.suite,'error':error,'checks':checks,'processes':records,
        'server_runtime':'packaged' if a.server_exe else 'editor executable in dedicated-server mode',
        'client_runtime':'packaged' if a.client_exe else 'editor executable in game mode',
        'network':'127.0.0.1 only; no WAN acceptance','smoke_outcome':a.outcome,
        'smoke_fixture':'server-controlled actor interactions/teleports or downed state; exercises real outcome logic when outcome != timer; not a human playthrough'}
    (OUT/'RESULT.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
    print('NETWORK_SUITE suite='+a.suite+' result='+report['result']+(' error='+error if error else ''),flush=True)
    if error:
        for _,log,_,_ in procs:
            lines=[s for s in read(log).splitlines() if any(k in s for k in ['BR_','Error:','Failure'])]
            print(log.name+'\n'+'\n'.join(lines[-15:]),flush=True)
sys.exit(1 if error else 0)
