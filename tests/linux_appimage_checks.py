"""Verify the Linux x86_64 AppImage and its public ABI-v3 payload."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd=None, timeout=180):
    result = subprocess.run([str(item) for item in command], cwd=cwd,
                            capture_output=True, text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(map(str, command))}\n"
                           f"{result.stdout}\n{result.stderr}")
    return result


def verify(args, report):
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'amd64'):
        raise RuntimeError('Requires a native Linux x86_64 runner')
    archives = sorted(args.output.glob('*.AppImage'))
    if len(archives) != 1:
        raise RuntimeError(f'Expected exactly one AppImage, found {len(archives)}')
    archive = archives[0].resolve()
    report['appimage'] = str(archive)
    report['appimage_sha256'] = hashlib.sha256(archive.read_bytes()).hexdigest()
    report['size_bytes'] = archive.stat().st_size
    if archive.read_bytes()[:4] != b'\x7fELF':
        raise RuntimeError('AppImage does not start with an ELF runtime')
    file_info = run(['file', '--brief', str(archive)]).stdout.strip()
    report['file'] = file_info
    if 'x86-64' not in file_info:
        raise RuntimeError('AppImage is not x86-64: ' + file_info)

    with tempfile.TemporaryDirectory(prefix='nes-appimage-check-', dir='/tmp') as name:
        clean = Path(name) / 'path with spaces'
        clean.mkdir()
        copied = clean / archive.name
        shutil.copy2(archive, copied)
        extracted = clean / 'squashfs-root'
        env = os.environ.copy()
        env['APPIMAGE_EXTRACT_AND_RUN'] = '1'
        subprocess.run([str(copied), '--appimage-extract'], cwd=clean, env=env,
                       check=True, capture_output=True, text=True, timeout=180)
        # appimage-extract creates squashfs-root in the working directory.
        if not extracted.is_dir():
            raise RuntimeError('AppImage extraction did not create squashfs-root')
        report['extraction'] = 'PASS (appimage-extract; direct mode recorded separately)'
        members = [p.relative_to(extracted).as_posix() for p in extracted.rglob('*') if p.is_file()]
        report['member_count'] = len(members)
        required = ['AppRun', 'usr/bin/nes_native', 'usr/bin/include/recomp/module_abi.h',
                    'usr/bin/toolchain/python/bin/python3', 'usr/bin/toolchain/zig/zig',
                    'usr/bin/LICENSE', 'usr/bin/THIRD_PARTY.md']
        for item in required:
            if item not in members:
                raise RuntimeError('Missing AppImage member: ' + item)
        for item in members:
            path = PurePosixPath(item)
            if path.suffix.lower() in {'.nes', '.rom', '.sav', '.n2s', '.n2l'}:
                raise RuntimeError('Private/game resource included: ' + item)
            if any(part in {'user-import', 'recovery', 'runtime_ram'} for part in path.parts):
                raise RuntimeError('Private cache included: ' + item)
        report['content_audit'] = 'PASS (no ROMs/private caches; public notices present)'

        binary = extracted / 'usr/bin/nes_native'
        report['readelf'] = run(['readelf', '-h', binary]).stdout
        dynamic = run(['readelf', '-d', binary]).stdout
        report['dynamic_dependencies'] = dynamic
        report['ldd'] = run(['ldd', binary]).stdout
        if 'not found' in report['ldd']:
            raise RuntimeError('Unresolved dynamic dependency')
        report['dependencies'] = 'PASS'

        python = extracted / 'usr/bin/toolchain/python/bin/python3'
        zig = extracted / 'usr/bin/toolchain/zig/zig'
        report['python_version'] = run([python, '--version']).stdout.strip() or run([python, '--version']).stderr.strip()
        report['zig_version'] = run([zig, 'version']).stdout.strip()
        smoke_root = Path(name) / 'smoke'
        shutil.copytree(extracted / 'usr/bin', smoke_root)
        run([python, '-B', ROOT / 'tests/public_toolchain_smoke.py', smoke_root])
        report['synthetic_c_module'] = 'PASS (bundled compiler, ctypes execution, ABI v3 and poll order)'
        report['abi_v3'] = 'PASS (module ABI v3 and ordered services asserted by smoke test)'

        # This is a real execution of the packaged frontend from a clean path.
        # --import-only avoids requiring a ROM or a graphical display, while still
        # loading AppRun and the packaged executable through the AppImage mount.
        direct_cache = clean / 'runtime cache'
        invalid_rom = clean / 'synthetic invalid image.bin'
        invalid_rom.write_bytes(b'not a private ROM')
        direct = subprocess.run([str(copied), '--import-only', '--rom', str(invalid_rom),
                                 '--import-cache', str(direct_cache)],
                                cwd=clean, env={**env, 'SDL_VIDEODRIVER': 'dummy'},
                                capture_output=True, text=True, timeout=60)
        report['direct_execution'] = {'returncode': direct.returncode,
                                      'stdout': direct.stdout, 'stderr': direct.stderr,
                                      'mode': 'direct AppImage; expected rejection of synthetic invalid input'}
        if direct.returncode == 0 or 'import' not in (direct.stdout + direct.stderr).lower():
            raise RuntimeError('Direct AppImage did not execute its import error path')
        report['clean_path_spaces'] = 'PASS'
    report['result'] = 'PASS'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
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
        print('Linux AppImage verification: ' + report['result'], flush=True)
