"""Summarize a local gameplay JSONL; evidence checks do not certify gameplay."""
import argparse
from collections import Counter
import json
from pathlib import Path


def review(lines):
    counts=Counter();rooms=set();transitions=[];progress={}
    previous_room=None;ended=False;assisted=False;ending=False;sequence=0;last_cycle=0
    for line in lines:
        event=json.loads(line)
        if event.get('schema')!=1 or event.get('seq')!=sequence:
            raise ValueError('Invalid schema or sequence at event '+str(sequence))
        if ended or (sequence==0 and event.get('event')!='session_start'):
            raise ValueError('Invalid session lifecycle')
        cycle=event.get('cycle');frame=event.get('frame')
        if not isinstance(cycle,int) or not isinstance(frame,int) or cycle<last_cycle or frame<0:
            raise ValueError('Invalid diagnostic timestamp')
        kind=event['event']
        if kind=='session_start' and sequence!=0:raise ValueError('Multiple sessions in one file')
        counts[kind]+=1;sequence+=1;last_cycle=cycle
        if kind=='assistance_state':
            assisted=assisted or any(event.get(k,0) for k in ['god','all_items','mods','assisted'])
        if kind=='room_enter':
            room=event['room_id'];rooms.add(room)
            if previous_room is not None:
                pair=[previous_room,room]
                if pair not in transitions:transitions.append(pair)
            previous_room=room
        if kind in ['progress_initial','progress_changed']:
            progress[str(event['field'])]=event.get('to',event.get('value'))
        if kind=='ending_screen_observed':ending=True
        if kind=='session_end':
            ended=True;assisted=assisted or bool(event.get('assisted'))
            if bool(event.get('ending_screen_observed'))!=ending:raise ValueError('Ending summary disagrees with events')
    if not sequence:raise ValueError('Empty diagnostic file')
    problems=sum(counts[k] for k in ['error','uncovered_native_address','no_cpu_progress','no_video_frame_progress'])
    return {'schema':1,'events':sequence,'session_ended':ended,'assisted':bool(assisted),
        'ending_screen_observed':ending,'problem_events':problems,
        'normal_completion_candidate':bool(ended and ending and not assisted and not problems and
            counts['assistance_state'] and counts['runtime_attached'] and counts['room_enter']),
        'manual_gameplay_review_required':True,'room_ids':sorted(rooms),'observed_room_pairs':transitions,
        'event_counts':dict(counts),'last_progress_values':progress}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path)
    args=parser.parse_args()
    try:
        with args.log.open(encoding='utf-8') as stream:result=review(stream)
        print(json.dumps(result,indent=2))
    except (OSError,ValueError,KeyError,TypeError) as error:
        parser.exit(1,'Invalid gameplay diagnostic: '+str(error)+'\n')
