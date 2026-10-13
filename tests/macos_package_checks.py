"""Inspect and exercise the content-free macOS ZIP outside the build tree."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import plistlib
import re
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/public'))
from package_platform import macos_deployment_target, validate_macos_deployment
from icon_assets import verify_macos
from build_identity import packaged
from mod_distribution_checks import verify_mod_distribution


def verify(args, report):
    build, output = args.build.resolve(), args.output.resolve()
    report.update(host_arch=platform.machine(), host_version=platform.mac_ver()[0],
                  package_arch=args.arch, commands=[], binaries=[])
    assert platform.system() == 'Darwin', 'Requires actual macOS tools'
    if args.require_native_macos15:
        assert report['host_arch'] == args.arch, 'Requires matching native host architecture'
        assert report['host_version'].split('.')[0] == '15', 'Requires execution on macOS 15'
    assert macos_deployment_target(build) == '15.0', 'Unexpected deployment cache'
    archive = output / ('NES-Native-Lab-macOS-' + args.arch + '.zip')
    report['zip_sha256'] = hashlib.sha256(archive.read_bytes()).hexdigest()
    assert archive.with_suffix('.zip.sha256').read_text().split()[0]==report['zip_sha256'], 'ZIP checksum sidecar differs'
    with zipfile.ZipFile(archive) as source:
        assert source.testzip() is None, 'ZIP CRC failure'
        for entry in source.infolist():
            path = Path(entry.filename)
            assert not path.is_absolute() and '..' not in path.parts, 'Unsafe ZIP path'
            assert path.suffix.lower() not in {'.nes', '.rom', '.sav'}, 'Unexpected game data'
            assert not any(p in {'user-import', '.git'} for p in path.parts), 'Unexpected private cache'
            # The pinned Python distribution contains legitimate pip/build
            # source and Python bytecode; these are not game/build-tree caches.
            if 'toolchain' not in path.parts:
                assert not any(p in {'__pycache__', 'build'} for p in path.parts), 'Unexpected build cache'
    report['zip_integrity'] = 'PASS'

    with tempfile.TemporaryDirectory(prefix='nes-native-package-check-', dir='/private/tmp') as name:
        external = Path(name)

        def run(command):
            result = subprocess.run([str(p) for p in command], cwd=external,
                                    capture_output=True, text=True, timeout=180)
            report['commands'].append(dict(command=[str(p) for p in command],
                                           cwd=str(external), returncode=result.returncode,
                                           stdout=result.stdout, stderr=result.stderr))
            if result.returncode:
                raise RuntimeError('Command failed: ' + str(command[0]) + '\n' + result.stderr)
            return result.stdout

        run(['/usr/bin/sw_vers'])
        run(['/usr/bin/codesign', '--verify', '--deep', '--strict', output / 'NES Native Lab.app'])
        run(['/usr/bin/ditto', '-x', '-k', archive, external])
        app = external / 'NES Native Lab.app'
        verify_macos(app)
        report['official_icon'] = 'PASS (ICNS bytes and CFBundleIconFile)'
        run(['/usr/bin/codesign', '--verify', '--deep', '--strict', app])
        report['signature_before_after_extraction'] = 'PASS (ad hoc, not notarized)'
        info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
        assert info['LSMinimumSystemVersion'] == '15.0', 'Unexpected bundle minimum'
        validate_macos_deployment(app, '15.0', args.arch)
        frontend = app / 'Contents/MacOS' / info['CFBundleExecutable']
        report['package_identity']=packaged(app/'Contents/Resources',frontend,args.expected_commit,run)
        assert info['CFBundleShortVersionString']==report['package_identity']['version'], 'Bundle/version mismatch'
        for binary in sorted(app.rglob('*')):
            if not binary.is_file() or binary.is_symlink():
                continue
            with binary.open('rb') as handle:
                header = handle.read(16)
            native = (header[:4] in [b'\xcf\xfa\xed\xfe', b'\xce\xfa\xed\xfe']
                      and int.from_bytes(header[12:16], 'little') in [2, 6, 8])
            fat = header[:4] in [b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca']
            if not (native or fat):
                continue
            load = run(['/usr/bin/xcrun', 'vtool', '-arch', args.arch, '-show-build', binary])
            minimum = re.search(r'^\s+(?:minos|version)\s+(\d+\.\d+(?:\.\d+)?)\s*$', load, re.MULTILINE)
            assert minimum, 'Missing minimum load command'
            if binary == frontend:
                assert minimum.group(1) == '15.0', 'Frontend must declare 15.0'
            dependencies = run(['/usr/bin/otool', '-L', binary])
            report['binaries'].append(dict(path=str(binary.relative_to(app)),
                                           minimum=minimum.group(1), load_commands=load,
                                           dependencies=dependencies))
        resources = app / 'Contents/Resources'
        assert (resources/'tools/mods/packages.py').is_file(), 'Missing mod package helper'
        assert (resources/'examples/mods/Hello-NES-Native-Lab.zip').is_file(), 'Missing authored example mod'
        python = resources / 'toolchain/python/bin/python3'
        zig = resources / 'toolchain/zig/zig'
        report['python_version'] = run([python, '--version']).strip()
        report['zig_version'] = run([zig, 'version']).strip()
        report['mod_distribution'] = verify_mod_distribution(resources, python, external)
        run([python, '-B', ROOT / 'tests/public_toolchain_smoke.py', resources,
             '--guard-inventory',build/'ctest-inventory.json'])
        report['synthetic_compile_load'] = 'PASS (return 42)'
        run([python, '-B', ROOT / 'tests/public_import_process_checks.py', resources, build / 'import_probe'])
        report['packaged_import_process_checks'] = 'PASS (3 scenarios)'
        # Ensure exercising the extracted tools did not corrupt the app seal.
        run(['/usr/bin/codesign', '--verify', '--deep', '--strict', app])
        report['signature_after_tool_execution'] = 'PASS'
    report['result'] = 'PASS'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--arch', choices=['arm64', 'x86_64'], required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--require-native-macos15', action='store_true')
    parser.add_argument('--expected-commit')
    args = parser.parse_args()
    report = {'result': 'FAIL'}
    try:
        verify(args, report)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        print('macOS package verification: ' + report['result'], flush=True)
