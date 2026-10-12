"""Synthetic reports test the CI gate; these are not CPU/device executions."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

import verify_ctest_suite as gate


class GateChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='ctest gate checks ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.inventory = self.root / 'inventory.json'
        self.junit = self.root / 'results.xml'
        self.tests = [{'name': n, 'command': [sys.executable], 'properties': []}
                      for n in sorted(gate.REQUIRED)]
        # Authored report examples, independent of the verifier's regexes.
        self.outputs = {
            'package_identity_contract': 'Ran 7 tests',
            'public_import_contract': 'Ran 3 tests',
            'diagnostic_harness_contract': 'Ran 5 tests',
            'native_opcode_contract': '4736 native instruction cases:',
            'native_stack_programs': '2 synthetic native programs:',
            'ppustatus_read_timing': '2 PPUSTATUS timing regressions: PASS',
            'native_bus_side_effects': '14 native timed side-effect cases: PASS',
            'import_cache_abi_contract': 'Ran 5 tests',
            'module_abi_contract': '4 real dynamic module ABI cases: PASS',
            'synthetic_ram_contract': '3 synthetic device cases;',
            'synthetic_mmc1_contract': '4 synthetic device cases;',
            'synthetic_bus_contract': '5 synthetic device cases;',
            'synthetic_ppu_contract': '5 synthetic device cases;',
            'gameplay_diagnostic_contract': '8 synthetic gameplay diagnostic cases: PASS',
            'ctest_verification_contract': 'Ran 6 tests',
            'apu_audio_contract': 'APU cases executed=17 passed=17 failed=0',
        }
        for n in ('status_open_bus', 'frame_mode_delay'):
            self.outputs['apu_'+n+'_reference'] = 'APU cases executed=1 passed=1 failed=0'
        self.outputs['apu_frame_mode_edges_contract'] = 'APU cases executed=1 passed=1 failed=0'
        for n, count in [('pal08_alignment0', 2), ('pal08_alignment1', 2), ('pal07', 8),
                         ('held_clear', 2), ('held_no_clear', 2), ('held_masked', 2), ('branches', 6)]:
            self.outputs['cpu_irq_poll_'+n] = f'IRQ contracts executed={count} passed={count} failed=0 harness_errors=0'
        for n, count in [('masks', 10), ('edges', 36), ('priority_dma', 7), ('adapters', 5)]:
            self.outputs['cpu_irq_edge_'+n] = f'IRQ edge controls executed={count} passed={count} failed=0'

    def write_inventory(self):
        self.inventory.write_text(json.dumps({'tests': self.tests}))

    def results(self, failure=None, skip=None, missing=None):
        root = ET.Element('testsuite')
        for t in self.tests:
            n = t['name']
            if n == missing:
                continue
            case = ET.SubElement(root, 'testcase', name=n, status='run')
            ET.SubElement(case, 'system-out').text = self.outputs.get(n, 'Synthetic report fixture')
            if n == failure:
                case.set('status', 'fail')
                ET.SubElement(case, 'failure').text = 'Unresolved A02'
            if n == skip:
                ET.SubElement(case, 'skipped')
        ET.ElementTree(root).write(self.junit, encoding='utf-8')

    def test_complete_desktop_and_headless_inventory(self):
        for headless in (False, True):
            with self.subTest(headless=headless), contextlib.redirect_stdout(io.StringIO()):
                if headless:
                    self.tests = [t for t in self.tests if t['name'] not in gate.DESKTOP_ONLY]
                self.write_inventory()
                names = gate.check_inventory(self.inventory, headless)
                self.results()
                gate.check_results(self.junit, names)

    def test_missing_irq_is_rejected(self):
        self.tests = [t for t in self.tests if t['name'] != 'cpu_irq_edge_adapters']
        self.write_inventory()
        with self.assertRaisesRegex(ValueError, 'Missing required tests: cpu_irq_edge_adapters'):
            gate.check_inventory(self.inventory)

    def test_exemptions_are_rejected(self):
        for key, value in [('DISABLED', True), ('WILL_FAIL', True), ('SKIP_RETURN_CODE', 77),
                           ('SKIP_REGULAR_EXPRESSION', 'A02')]:
            with self.subTest(key=key):
                self.tests[0]['properties'] = [{'name': key, 'value': value}]
                self.write_inventory()
                with self.assertRaises(ValueError):
                    gate.check_inventory(self.inventory)

    def test_old_or_incomplete_execution_counts_are_rejected(self):
        for name, bad in [('import_cache_abi_contract', 'Ran 4 tests'),
                          ('module_abi_contract', '3 real dynamic module ABI cases: PASS'),
                          ('cpu_irq_edge_adapters', 'IRQ edge controls executed=4 passed=4 failed=0'),
                          ('cpu_irq_poll_pal07', '[test output removed at 1024 bytes]')]:
            with self.subTest(name=name):
                original = self.outputs[name]
                self.outputs[name] = bad
                self.results()
                with self.assertRaisesRegex(ValueError, 'Missing execution count/diagnostic: '+name):
                    gate.check_results(self.junit, gate.REQUIRED)
                self.outputs[name] = original

    def test_a02_failure_is_reported_and_rejected(self):
        self.results(failure='apu_status_irq_collision_reference')
        output = io.StringIO()
        with contextlib.redirect_stdout(output), self.assertRaisesRegex(ValueError, 'Failed tests: apu_status_irq_collision_reference'):
            gate.check_results(self.junit, gate.REQUIRED)
        self.assertIn(f'{len(gate.REQUIRED)} executed; {len(gate.REQUIRED)-1} passed; 1 failed; zero skips', output.getvalue())

    def test_missing_or_skipped_execution_is_rejected(self):
        self.results(missing='cpu_irq_poll_pal07')
        with self.assertRaisesRegex(ValueError, 'execution set differs'):
            gate.check_results(self.junit, gate.REQUIRED)
        self.results(skip='apu_status_irq_collision_reference')
        with self.assertRaisesRegex(ValueError, 'Skipped test: apu_status_irq_collision_reference'):
            gate.check_results(self.junit, gate.REQUIRED)


if __name__ == '__main__':
    unittest.main()
