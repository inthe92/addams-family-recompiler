"""Collect same-build evidence without rerunning CPU tests or hiding A02.

The native contract has a virtual schedule and conversion-only presentations.
GL probe measurements are separate host draw/swap measurements, never scanout.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/public'))
from build_identity import validate, source_identity
import verify_ctest_suite as gate

KNOWN = 'apu_status_irq_collision_reference'
FOCUS = {'frontend_presentation_schedule','frontend_presentation_snapshot','frontend_presentation_native',
         'frontend_timing_metrics','frontend_timing_clock_contract','frontend_timing_source_contract','frontend_timing_native_contract',
         'original_presentation_geometry','original_presentation_pixels','original_presentation_pacing',
         'frontend_sdl_events','frontend_audio_pause'}

def require(ok,message):
    if not ok:raise ValueError(message)

def finite_nonnegative(value):
    return type(value) in (int,float) and math.isfinite(value) and value>=0

def native_measurements(output):
    pending=None;rows=[]
    for line in output.splitlines():
        match=re.fullmatch(r'Native measurement target=(60|90|120) disturbance=([012]) schedule=virtual graphics=conversion_only',line)
        if match:
            require(pending is None,'Native measurement has no timing summary')
            pending=(int(match[1]),int(match[2]))
        elif line.startswith('Timing diagnostic: '):
            require(pending is not None,'Unpaired native timing summary')
            data=json.loads(line.removeprefix('Timing diagnostic: '));counts=data['counts']
            require(data['schema']==1 and data['clock_frequency']==1000000000 and data['legacy_frame_units']==12,'Invalid native timing identity')
            for key in ('nes_frames_executed','ppu_frames_produced','audio_batches','god_hook_calls','diagnostic_frame_calls'):
                require(counts[key]==12,'Native count differs: '+key)
            require(counts['pcm_samples_collected']>0 and counts['native_slice_calls']>0,'Native fixture did not generate PCM/instructions')
            require(counts['buffer_swaps_completed']==0,'Conversion-only probe claims GL swaps')
            require(finite_nonnegative(data['wall_seconds']) and data['invalid_clock_samples']==0,'Invalid native timing clock')
            for stage in data['stages'].values():require(finite_nonnegative(stage['seconds']),'Invalid stage duration')
            rows.append(dict(target_hz=pending[0],disturbance=pending[1],schedule='virtual',
                             presentation_calls_are_synthetic=True,opengl=False,physical_refresh_verified=False,timing=data));pending=None
    require(pending is None,'Incomplete native observation')
    require(len(rows)==9 and {(r['target_hz'],r['disturbance']) for r in rows}=={(h,d) for h in (60,90,120) for d in (0,1,2)},'Missing/duplicate native rate or disturbance')
    return rows

def suite_evidence(inventory_path,junit_path,headless=False):
    names=gate.check_inventory(inventory_path,headless)
    cases=list(ET.parse(junit_path).getroot().iter('testcase'))
    require(len(cases)==len(names) and {c.attrib['name'] for c in cases}==names,'CTest execution set differs from inventory')
    failed=[];by_name={}
    for c in cases:
        name=c.attrib['name'];by_name[name]=c
        require(c.find('skipped') is None and c.attrib.get('status') not in ('notrun','disabled'),'Skipped/disabled test: '+name)
        failure=c.attrib.get('status')=='fail' or c.find('failure') is not None or c.find('error') is not None
        if failure:failed.append(name)
        elif name in FOCUS:
            require(re.search(gate.OUTPUT[name],c.findtext('system-out','')),'Missing execution count: '+name)
    needed=FOCUS-gate.DESKTOP_ONLY if headless else FOCUS
    require(needed<=by_name.keys(),'Missing presentation/UI contract')
    rows=native_measurements(by_name['frontend_presentation_native'].findtext('system-out',''))
    return dict(executed=len(cases),passed=len(cases)-len(failed),failed=len(failed),omitted=0,
                failed_names=failed,a02_state='failed' if KNOWN in failed else 'not_failed_requires_review',
                fully_passed=not failed,native_measurements=rows)

def gl_evidence(output):
    def kv(line):return dict(re.findall(r'(\w+)=([^\s]+)',line))
    rates={};stages={};bench={};display=None;case_count=None
    for line in output.splitlines():
        if line.startswith('Display diagnostic '):display=kv(line)
        elif line.startswith('GL stage '):s=kv(line);key=int(s.pop('target'));require(key not in stages,'Duplicate GL stage');stages[key]={k:float(v) for k,v in s.items()}
        elif line.startswith('Scheduled GL '):s=kv(line);key=int(s.pop('target'));require(key not in rates,'Duplicate GL rate');rates[key]={k:float(v) for k,v in s.items()}
        elif line.startswith('Repeat benchmark '):s=kv(line);key=s.pop('path');require(key not in bench,'Duplicate repeat benchmark');bench[key]={k:float(v) for k,v in s.items()}
        else:
            m=re.match(r'(\d+) real SDL/OpenGL presentation cases: PASS;',line)
            if m:case_count=int(m[1])
    require(case_count==23 and display is not None,'GL correctness cases/context not completed')
    require(set(rates)==set(stages)=={60,90,120} and set(bench)=={'raw','cached'},'GL measurements incomplete')
    for sample in bench.values():
        require(sample['batches']==5 and sample['draws_per_batch']==200 and finite_nonnegative(sample['median_batch_ms']),'Invalid repeat benchmark')
    for hz in rates:
        require(all(finite_nonnegative(x) for x in list(rates[hz].values())+list(stages[hz].values())),'Invalid GL metric')
        require(rates[hz]['draws']>0 and rates[hz]['elapsed']>0,'Empty GL sample')
        require(stages[hz]['conversions']==stages[hz]['uploads']<=rates[hz]['own_snapshot_updates'],'Cache uploaded more than published images')
        require(rates[hz]['draws']>=stages[hz]['uploads'],'Cache count exceeds drawings')
    return dict(status='passed',cases_passed=case_count,display_reported=display,rate_measurements=rates,
                stage_measurements=stages,repeat_benchmark=bench,physical_refresh_verified=False,
                nes_execution=False,images_are_authored_synthetic=True)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--expected-commit');p.add_argument('--headless',action='store_true')
    p.add_argument('--graphics',choices=['run','not-executed'],default='not-executed')
    p.add_argument('--graphics-label',default='display_not_provisioned')
    args=p.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True);report=dict(schema=1,process_platform=platform.system(),process_machine=platform.machine(),
                                  physical_refresh_verified=False,errors=[],graphics=dict(status='not_executed',environment=args.graphics_label))
    try:
        identity=args.build/'native-build-info.json'
        report['build_identity']=validate(json.loads(identity.read_text()),args.expected_commit) if identity.exists() else None
        require(not args.expected_commit or report['build_identity'] is not None,'Missing exact-commit build identity')
        if report['build_identity']:
            commit,dirty,digest=source_identity(ROOT)
            require((commit,dirty,digest)==(report['build_identity']['commit'],report['build_identity']['dirty'],report['build_identity']['source_sha256']),'Build source identity is stale; rebuild before collecting evidence')
        report['suite']=suite_evidence(args.build/'ctest-inventory.json',args.build/'ctest-results.xml',args.headless)
        require(report['suite']['failed_names']==[KNOWN],'Suite has a new failure or changed A02 result; review required')
        if args.graphics=='run':
            binary=args.build/('presentation_gl_checks.exe' if platform.system()=='Windows' else 'presentation_gl_checks')
            run=subprocess.run([str(binary.resolve()),'--benchmark'],capture_output=True,text=True,timeout=90)
            (args.output.parent/'high-refresh-gl.log').write_text(run.stdout+run.stderr)
            report['graphics']=dict(status='failed',exit_code=run.returncode,environment=args.graphics_label,physical_refresh_verified=False)
            require(run.returncode==0,'GL probe failed; see high-refresh-gl.log (not certified)')
            report['graphics']=gl_evidence(run.stdout);report['graphics'].update(exit_code=0,environment=args.graphics_label,
                probe_file=binary.name,probe_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
        report['experimental_contracts_verified']=True
    except (OSError,ValueError,KeyError,ET.ParseError,subprocess.SubprocessError) as e:
        report['errors'].append(str(e));report['experimental_contracts_verified']=False
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('High refresh evidence:', 'ERROR' if report['errors'] else 'collected; complete suite remains failed by A02')
    for error in report['errors']:print(error,file=sys.stderr)
    return int(bool(report['errors']))

if __name__=='__main__':sys.exit(main())
