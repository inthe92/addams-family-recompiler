"""Diagnostic package eligibility; does not alter CTest status or its mandatory gate."""
import argparse,json
from pathlib import Path
import verify_ctest_suite as gate

def assess(inventory,junit,headless=False):
 names=gate.check_inventory(inventory,headless)
 failures=gate.result_failures(junit,names)
 unexpected=sorted(set(failures)-{'apu_status_irq_collision_reference'})
 return dict(schema=1,suite_pass=not failures,failures=failures,new_failures=unexpected,omitted=0,package_eligible=not unexpected,scope='All required tests executed; A02 remains a real CI failure')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--inventory',type=Path,required=True);p.add_argument('--junit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--headless',action='store_true');a=p.parse_args()
 try:r=assess(a.inventory,a.junit,a.headless);a.output.write_text(json.dumps(r,indent=2)+'\n');print('Diagnostic package eligible:',r['package_eligible'],'suite PASS:',r['suite_pass']);raise SystemExit(0 if r['package_eligible'] else 1)
 except (OSError,ValueError,KeyError,gate.ET.ParseError) as e:print('Diagnostic package BLOCKED:',e);raise SystemExit(1)
