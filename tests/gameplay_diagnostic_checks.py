"""Run native observer tests and independently parse their synthetic JSONL."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/verification'))
from review_gameplay_log import review

with tempfile.TemporaryDirectory(prefix='nes-gameplay-diagnostic-') as directory:
    log=Path(directory)/'synthetic.jsonl'
    subprocess.run([sys.argv[1],str(log)],check=True)
    events=[json.loads(line) for line in log.read_text(encoding='utf-8').splitlines()]
    sessions=[]
    for event in events:
        assert event['schema']==1
        assert isinstance(event['cycle'],int) and isinstance(event['frame'],int)
        if event['event']=='session_start':sessions.append([])
        assert sessions
        sessions[-1].append(event)
        assert event['seq']==len(sessions[-1])-1
    assert len(sessions)==11
    assert all(session[-1]['event']=='session_end' for session in sessions)
    kinds=[event['event'] for event in events]
    for kind in ['room_enter','room_exit','transition_request','death','respawn_observed',
                 'item_pickup','progress_changed','bank_mapping_changed','uncovered_native_address',
                 'ending_screen_observed','error','no_cpu_progress']:
        assert kind in kinds,kind
    pickups=[event for event in events if event['event']=='item_pickup']
    assert len(pickups)==1 and pickups[0]['item_flag']==0x0633
    assert pickups[0]['pc']==0xA80A and pickups[0]['bank']==1
    assert any(event.get('code')=='synthetic"error\n' for event in events)
    assert sum(session[-1]['assisted'] for session in sessions)==1
    assert all('room_name' not in event and 'rom_bytes' not in event for event in events)
    assert any(event['event']=='session_end' and event['ending_screen_observed']==1 for event in events)
    candidates=0
    for session in sessions:
        result=review(json.dumps(event)+'\n' for event in session)
        assert result['session_ended']
        if result['normal_completion_candidate']:
            candidates+=1
            assert not result['assisted'] and result['ending_screen_observed'] and not result['problem_events']
        assert result['manual_gameplay_review_required']
    assert candidates==1
    truncated=review(json.dumps(event)+'\n' for event in sessions[0][:-1])
    assert not truncated['session_ended'] and not truncated['normal_completion_candidate']
    malformed=[dict(event) for event in sessions[0]];malformed[-1]['seq']=999
    try:review(json.dumps(event)+'\n' for event in malformed)
    except ValueError:pass
    else:raise AssertionError('Corrupt sequence accepted')
print('Synthetic JSONL: 11 complete sessions, event schema/order, numeric identities and assistance status: PASS')
print('Diagnostic reviewer: assisted/incomplete/error evidence cannot certify a normal completion: PASS')
