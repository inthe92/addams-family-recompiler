"""Evidence cannot turn synthetic draws into physical FPS or hide A02."""
import json
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

import high_refresh_report as report
import verify_ctest_suite_checks as examples


def native_output():
    lines=[]
    for hz in (60,90,120):
        for disturbance in (0,1,2):
            timing=dict(schema=1,clock_frequency=1000000000,legacy_frame_units=12,wall_seconds=0.1,invalid_clock_samples=0,
                        counts={k:12 for k in ('nes_frames_executed','ppu_frames_produced','audio_batches','god_hook_calls','diagnostic_frame_calls')},
                        stages={k:dict(seconds=0.01) for k in ('simulation','audio','render','swap','wait','capture')})
            timing['counts'].update(pcm_samples_collected=1000,native_slice_calls=400,buffer_swaps_completed=0)
            lines+=['Native measurement target=%s disturbance=%s schedule=virtual graphics=conversion_only'%(hz,disturbance),
                    'Timing diagnostic: '+json.dumps(timing)]
    lines+=['Frontend scheduled native cases=9 PASS']
    return '\n'.join(lines)


def gl_output():
    lines=['Display diagnostic driver=synthetic mode_result=0 reported_hz=60 swap_request_result=0 swap_interval=0']
    for hz,draws in ((60,120),(90,180),(120,240)):
        lines+=['GL stage target=%s conversions=100 uploads=100 conversion_ms=1 upload_host_ms=2 render_host_ms=4 swap_ms=5 wait_ms=1900'%hz,
                'Scheduled GL target=%s draws=%s elapsed=2 host_draws_per_second=%s own_snapshot_updates=100'%(hz,draws,hz)]
    lines+=['Repeat benchmark path=raw batches=5 draws_per_batch=200 median_batch_ms=10',
            'Repeat benchmark path=cached batches=5 draws_per_batch=200 median_batch_ms=9',
            '23 real SDL/OpenGL presentation cases: PASS; OpenGL synthetic']
    return '\n'.join(lines)


class EvidenceChecks(unittest.TestCase):
    def test_native_virtual_schedule_is_explicit(self):
        rows=report.native_measurements(native_output())
        self.assertEqual(len(rows),9)
        for row in rows:
            self.assertEqual(row['schedule'],'virtual');self.assertFalse(row['opengl']);self.assertFalse(row['physical_refresh_verified'])
            self.assertTrue(row['presentation_calls_are_synthetic'])

    def test_missing_or_duplicate_native_rates_rejected(self):
        output=native_output()
        for bad in (output.replace('target=90','target=60'),output.split('Native measurement target=120')[0]):
            with self.assertRaises(ValueError):report.native_measurements(bad)

    def test_native_count_or_fake_swap_rejected(self):
        output=native_output()
        for bad in (output.replace('"audio_batches": 12','"audio_batches": 24'),output.replace('"buffer_swaps_completed": 0','"buffer_swaps_completed": 1')):
            with self.assertRaises(ValueError):report.native_measurements(bad)

    def test_gl_partial_or_invalid_results_rejected(self):
        output=gl_output()
        for bad in (output.replace('23 real','22 real'),output.replace('conversion_ms=1','conversion_ms=nan')):
            with self.assertRaises(ValueError):report.gl_evidence(bad)

    def test_cached_uploads_and_physical_refresh_not_conflated(self):
        output=gl_output();result=report.gl_evidence(output)
        self.assertEqual(result['cases_passed'],23);self.assertFalse(result['physical_refresh_verified']);self.assertFalse(result['nes_execution'])
        with self.assertRaises(ValueError):report.gl_evidence(output.replace('uploads=100','uploads=240'))

    def test_a02_stays_a_failed_suite_and_skips_rejected(self):
        legacy=examples.GateChecks('test_a02_failure_is_reported_and_rejected');legacy.setUp()
        try:
            legacy.outputs['frontend_presentation_native']=native_output()
            legacy.write_inventory();legacy.results(failure=report.KNOWN)
            data=report.suite_evidence(legacy.inventory,legacy.junit)
            self.assertEqual(data['failed_names'],[report.KNOWN]);self.assertFalse(data['fully_passed']);self.assertEqual(data['a02_state'],'failed')
            legacy.results(skip=report.KNOWN)
            with self.assertRaises(ValueError):report.suite_evidence(legacy.inventory,legacy.junit)
        finally:legacy.doCleanups()

    def test_four_runner_ci_evidence_is_prepared_and_not_published(self):
        source=(report.ROOT/'.github/workflows/build.yml').read_text()
        self.assertIn('os: macos-15\n            arch: arm64',source);self.assertIn('os: macos-15-intel\n            arch: x86_64',source)
        self.assertIn('runs-on: windows-latest',source);self.assertIn('runs-on: ubuntu-22.04',source)
        self.assertEqual(source.count('tests/high_refresh_report.py'),3)
        self.assertEqual(source.count('name: Collect high refresh evidence'),3)
        self.assertEqual(source.count('build/high-refresh-evidence.json'),6)
        self.assertIn('--graphics not-executed --graphics-label windows-hosted-display-unverified',source)
        self.assertIn('xvfb-run -a',source);self.assertNotIn('continue-on-error',source)
        self.assertNotIn('git push',source);self.assertNotIn('gh release',source)

if __name__=='__main__':unittest.main()
