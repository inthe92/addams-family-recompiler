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
    'public_import_contract', 'diagnostic_harness_contract',
    'native_opcode_contract', 'native_stack_programs', 'ppustatus_read_timing',
    'native_bus_side_effects', 'import_cache_abi_contract', 'module_abi_contract',
    'synthetic_ram_contract', 'synthetic_mmc1_contract',
    'synthetic_bus_contract', 'synthetic_ppu_contract',
    'gameplay_diagnostic_contract',
}
OUTPUT = {
    'public_import_contract': r'Ran 3 tests\b',
    'diagnostic_harness_contract': r'Ran 5 tests\b',
    'native_opcode_contract': r'\b4736 native instruction cases:',
    'native_stack_programs': r'\b2 synthetic native programs:',
    'ppustatus_read_timing': r'\b2 PPUSTATUS timing regressions: PASS',
    'native_bus_side_effects': r'\b14 native timed side-effect cases: PASS',
    'import_cache_abi_contract': r'Ran 4 tests\b',
    'module_abi_contract': r'\b3 real dynamic module ABI cases: PASS',
    'synthetic_ram_contract': r'\b3 synthetic device cases;',
    'synthetic_mmc1_contract': r'\b4 synthetic device cases;',
    'synthetic_bus_contract': r'\b5 synthetic device cases;',
    'synthetic_ppu_contract': r'\b5 synthetic device cases;',
    'gameplay_diagnostic_contract': r'\b8 synthetic gameplay diagnostic cases: PASS',
}

def require(condition, message):
    if not condition:
        raise ValueError(message)

def enabled(value):
    return value is True or str(value).lower() in {'1', 'true', 'on', 'yes'}

def check_inventory(path):
    data=json.loads(path.read_text(encoding='utf-8'))
    tests=data['tests'];names=[t['name'] for t in tests]
    require(len(names)==len(set(names)), 'Duplicate CTest names')
    require(REQUIRED<=set(names), 'Missing required tests: '+', '.join(sorted(REQUIRED-set(names))))
    for test in tests:
        name=test['name'];props={p['name']:p['value'] for p in test.get('properties',[])}
        require(not enabled(props.get('DISABLED',False)), 'Disabled test: '+name)
        require(not enabled(props.get('WILL_FAIL',False)), 'Expected-failure exemption: '+name)
        require(not any(p in props for p in ['SKIP_RETURN_CODE','SKIP_REGULAR_EXPRESSION']), 'Skip exemption: '+name)
        command=test.get('command',[])
        require(command and Path(command[0]).is_file(), 'Missing test executable: '+name)
    print('CTest inventory: '+str(len(tests))+' registered tests; all '+str(len(REQUIRED))+' required entries present, executable and mandatory.')
    return set(names)

def check_results(path, names):
    root=ET.parse(path).getroot();cases=list(root.iter('testcase'))
    executed=[c.attrib['name'] for c in cases]
    require(len(executed)==len(set(executed)), 'Duplicate JUnit test cases')
    require(set(executed)==names, 'JUnit execution set differs from CTest inventory')
    for case in cases:
        name=case.attrib['name']
        require(case.attrib.get('status')=='run', 'Test did not run: '+name)
        require(not any(case.find(tag) is not None for tag in ['skipped','failure','error']), 'Failed/skipped test: '+name)
        output=case.findtext('system-out',default='')
        if name in OUTPUT:
            require(re.search(OUTPUT[name],output) is not None, 'Missing execution count/diagnostic: '+name)
    print('CTest results: '+str(len(cases))+' executed and passed; zero skips; CPU/device/PPUSTATUS/ABI/importer counts confirmed.')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--junit',type=Path)
    args=parser.parse_args()
    try:
        names=check_inventory(args.inventory)
        if args.junit:check_results(args.junit,names)
    except (OSError,ValueError,KeyError,ET.ParseError) as error:
        print('CTest verification FAILED: '+str(error),file=sys.stderr)
        sys.exit(1)
