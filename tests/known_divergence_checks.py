"""Retain the five harness cases; resolved PPUSTATUS failures cannot be skipped."""
import contextlib
import io
import subprocess
import unittest
from run_known_divergence import outcome


class KnownDivergenceChecks(unittest.TestCase):
    def classify(self, code, output):
        result = subprocess.CompletedProcess([], code, output, '')
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return outcome(result)

    def test_former_documented_failure_now_fails(self):
        self.assertEqual(self.classify(1, 'KNOWN_DIVERGENCE: PPUSTATUS_READ_OFFSET'), 1)

    def test_unrelated_failure_fails(self):
        self.assertEqual(self.classify(1, 'other failure'), 1)

    def test_invariant_error_with_marker_still_fails(self):
        self.assertEqual(self.classify(2, 'KNOWN_DIVERGENCE: PPUSTATUS_READ_OFFSET'), 1)

    def test_crash_still_fails(self):
        self.assertEqual(self.classify(-11, 'KNOWN_DIVERGENCE: PPUSTATUS_READ_OFFSET'), 1)

    def test_normal_pass_is_accepted(self):
        self.assertEqual(self.classify(0, 'resolved'), 0)


if __name__ == '__main__':
    unittest.main()
