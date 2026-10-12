"""CI gate: require the complete bus-timing suite, real executions and no skips.

This validates CTest's JSON inventory before execution and its JUnit report
following execution. It never supplies expected CPU/device values or runs a ROM.
"""
import argparse
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

REQUIRED = {
    'package_identity_contract',
    'official_icon_assets', 'window_icon_contract',
    'public_import_contract', 'diagnostic_harness_contract',
    'native_opcode_contract', 'native_stack_programs', 'ppustatus_read_timing',
    'native_bus_side_effects', 'import_cache_abi_contract', 'module_abi_contract',
    'synthetic_ram_contract', 'synthetic_mmc1_contract',
    'synthetic_bus_contract', 'synthetic_ppu_contract',
    'gameplay_diagnostic_contract',
    'ctest_verification_contract',
    'cpu_irq_poll_pal08_alignment0', 'cpu_irq_poll_pal08_alignment1',
    'cpu_irq_poll_pal07', 'cpu_irq_poll_held_clear', 'cpu_irq_poll_held_no_clear',
    'cpu_irq_poll_held_masked', 'cpu_irq_poll_branches',
    'cpu_irq_edge_masks', 'cpu_irq_edge_edges', 'cpu_irq_edge_priority_dma',
    'cpu_irq_edge_adapters',
    'apu_audio_contract', 'apu_status_open_bus_reference',
    'apu_status_irq_collision_reference', 'apu_frame_mode_delay_reference',
    'apu_frame_mode_edges_contract', 'apu_sdl_queue_contract',
}
DESKTOP_ONLY = {'module_abi_contract', 'apu_sdl_queue_contract', 'window_icon_contract'}
OUTPUT = {
    'package_identity_contract': r'Ran 7 tests\b',
    'public_import_contract': r'Ran 3 tests\b',
    'diagnostic_harness_contract': r'Ran 5 tests\b',
    'native_opcode_contract': r'\b4736 native instruction cases:',
    'native_stack_programs': r'\b2 synthetic native programs:',
    'ppustatus_read_timing': r'\b2 PPUSTATUS timing regressions: PASS',
    'native_bus_side_effects': r'\b14 native timed side-effect cases: PASS',
    'import_cache_abi_contract': r'Ran 5 tests\b',
    'module_abi_contract': r'\b4 real dynamic module ABI cases: PASS',
    'synthetic_ram_contract': r'\b3 synthetic device cases;',
    'synthetic_mmc1_contract': r'\b4 synthetic device cases;',
    'synthetic_bus_contract': r'\b5 synthetic device cases;',
    'synthetic_ppu_contract': r'\b5 synthetic device cases;',
    'gameplay_diagnostic_contract': r'\b8 synthetic gameplay diagnostic cases: PASS',
    'ctest_verification_contract': r'Ran 6 tests\b',
    'apu_audio_contract': r'APU cases executed=17 passed=17 failed=0\b',
    'apu_status_open_bus_reference': r'APU cases executed=1 passed=1 failed=0\b',
    'apu_frame_mode_delay_reference': r'APU cases executed=1 passed=1 failed=0\b',
    'apu_frame_mode_edges_contract': r'APU cases executed=1 passed=1 failed=0\b',
}
for name, count in {'pal08_alignment0': 2, 'pal08_alignment1': 2, 'pal07': 8,
                    'held_clear': 2, 'held_no_clear': 2, 'held_masked': 2,
                    'branches': 6}.items():
    OUTPUT['cpu_irq_poll_'+name] = rf'IRQ contracts executed={count} passed={count} failed=0 harness_errors=0\b'
for name, count in {'masks': 10, 'edges': 36, 'priority_dma': 7, 'adapters': 5}.items():
    OUTPUT['cpu_irq_edge_'+name] = rf'IRQ edge controls executed={count} passed={count} failed=0\b'

def require(condition, message):
    if not condition:
        raise ValueError(message)

def enabled(value):
    return value is True or str(value).lower() in {'1', 'true', 'on', 'yes'}

def check_inventory(path, headless=False):
    data=json.loads(path.read_text(encoding='utf-8'))
    tests=data['tests'];names=[t['name'] for t in tests]
    require(len(names)==len(set(names)), 'Duplicate CTest names')
    required = REQUIRED-DESKTOP_ONLY if headless else REQUIRED
    require(required<=set(names), 'Missing required tests: '+', '.join(sorted(required-set(names))))
    for test in tests:
        name=test['name'];props={p['name']:p['value'] for p in test.get('properties',[])}
        require(not enabled(props.get('DISABLED',False)), 'Disabled test: '+name)
        require(not enabled(props.get('WILL_FAIL',False)), 'Expected-failure exemption: '+name)
        require(not any(p in props for p in ['SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION']), 'Skip exemption: '+name)
        command=test.get('command',[])
        require(command and Path(command[0]).is_file(), 'Missing test executable: '+name)
    print('CTest inventory: '+str(len(tests))+' registered tests; all '+str(len(required))+' required entries present, executable and mandatory.')
    return set(names)

def check_results(path, names):
    root=ET.parse(path).getroot();cases=list(root.iter('testcase'))
    executed=[c.attrib['name'] for c in cases]
    require(len(executed)==len(set(executed)), 'Duplicate JUnit test cases')
    require(set(executed)==names, 'JUnit execution set differs from CTest inventory')
    failed=[]
    for case in cases:
        name=case.attrib['name']
        status=case.attrib.get('status')
        failed_case=any(case.find(tag) is not None for tag in ['failure','error'])
        # CTest uses status="fail" for an executed failure, not an omission.
        require(status in {'run','fail'}, 'Test did not run: '+name)
        require(status!='fail' or failed_case, 'Failed status without failure evidence: '+name)
        require(case.find('skipped') is None, 'Skipped test: '+name)
        if failed_case:
            failed.append(name)
            continue
        output=case.findtext('system-out',default='')
        if name in OUTPUT:
            require(re.search(OUTPUT[name],output) is not None, 'Missing execution count/diagnostic: '+name)
    print(f'CTest results: {len(cases)} executed; {len(cases)-len(failed)} passed; {len(failed)} failed; zero skips.')
    require(not failed, 'Failed tests: '+', '.join(failed))
    print('CPU/IRQ/device/PPUSTATUS/ABI/importer counts confirmed.')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--junit',type=Path)
    parser.add_argument('--headless',action='store_true',help='Require the headless suite; desktop loader/SDL entries are not built')
    args=parser.parse_args()
    try:
        names=check_inventory(args.inventory,args.headless)
        if args.junit:check_results(args.junit,names)
    except (OSError,ValueError,KeyError,ET.ParseError) as error:
        print('CTest verification FAILED: '+str(error),file=sys.stderr)
        sys.exit(1)
