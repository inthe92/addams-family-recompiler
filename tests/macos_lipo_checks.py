"""Real Mach-O/lipo deployment checks using two tiny synthetic C libraries."""
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/public'))
from package_platform import validate_macos_deployment


@unittest.skipUnless(platform.system() == 'Darwin', 'Requires actual Apple tools')
class MacOSLipoChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='nes-lipo-synthetic-', dir='/private/tmp')
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        source = cls.root / 'fixture.c'
        source.write_text('int synthetic_contract(void) { return 42; }\n')
        for arch in ['arm64', 'x86_64']:
            folder = cls.root / arch
            folder.mkdir()
            subprocess.run(['/usr/bin/xcrun', 'clang', '-target', arch + '-apple-macos15.0',
                            '-dynamiclib', str(source), '-o', str(folder / 'fixture.dylib')],
                           check=True, capture_output=True, text=True)
        # Capture the old invocation on a valid synthetic file using the actual
        # host tool. Newer local tools accept it; legacy runner tools reject it.
        old = subprocess.run(['/usr/bin/lipo', '-verify_arch', 'arm64',
                              str(cls.root / 'arm64/fixture.dylib')], capture_output=True, text=True)
        print('Pre-fix ordering probe: exit=' + str(old.returncode) + '; stderr=' + (old.stderr.strip() or '(empty)'), flush=True)

    def test_arm64_valid(self):
        validate_macos_deployment(self.root / 'arm64', '15.0', 'arm64')

    def test_intel_valid(self):
        validate_macos_deployment(self.root / 'x86_64', '15.0', 'x86_64')

    def test_wrong_architecture_rejected_with_diagnostic(self):
        with self.assertRaises(ValueError) as caught:
            validate_macos_deployment(self.root / 'arm64', '15.0', 'x86_64')
        message = str(caught.exception)
        self.assertIn('lipo architecture verification failed', message)
        self.assertIn('fixture.dylib; expected x86_64; stderr:', message)
        # The emitted diagnostic includes exactly the actual tool's stderr.
        self.assertIsInstance(caught.exception.__cause__, subprocess.CalledProcessError)
        error = caught.exception.__cause__
        self.assertNotEqual(error.returncode, 0)
        self.assertIn(error.stderr.strip() or '(empty)', message)

    def test_invalid_file_rejected(self):
        invalid = self.root / 'invalid.txt'
        invalid.write_text('synthetic invalid input\n')
        result = subprocess.run(['/usr/bin/lipo', str(invalid), '-verify_arch', 'arm64'],
                                capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)

    def test_missing_native_file_rejected(self):
        folder = self.root / 'empty-app'
        folder.mkdir()
        (folder / 'invalid.txt').write_text('not Mach-O\n')
        with self.assertRaisesRegex(ValueError, 'No native macOS binaries'):
            validate_macos_deployment(folder, '15.0', 'arm64')

    def test_deployment_floor_still_enforced(self):
        with self.assertRaisesRegex(ValueError, r'macOS 15\.0 required above package floor 14\.0'):
            validate_macos_deployment(self.root / 'arm64', '14.0', 'arm64')


if __name__ == '__main__':
    unittest.main(verbosity=2)
