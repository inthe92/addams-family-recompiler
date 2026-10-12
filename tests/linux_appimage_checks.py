"""Verify the Linux x86_64 AppImage and its public ABI-v3 payload."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/public'))
from build_identity import packaged
from icon_assets import verify_linux
from audit_release import audit


def checked_rejection(result, cache):
    log=cache/'import.log'
    if result.returncode != 1 or 'ROM incompatible:' not in result.stderr or not log.is_file():
        raise RuntimeError('Expected real incompatible-input rejection (exit 1 and import.log), not a crash/loader failure')
    if 'ROM incompatible:' not in log.read_text() or list(cache.glob('user_game.*')):
        raise RuntimeError('Unexpected synthetic import log/module output')
    return {'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr,
            'import_log':log.read_text(),'mode':'synthetic incompatible input; no real game'}


def run(command, cwd=None, timeout=180):
    result = subprocess.run([str(item) for item in command], cwd=cwd,
                            capture_output=True, text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(map(str, command))}\n"
                           f"{result.stdout}\n{result.stderr}")
    return result


def verify(args, report):
    args.build=args.build.resolve();args.output=args.output.resolve()
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'amd64'):
        raise RuntimeError('Requires a native Linux x86_64 runner')
    archives = sorted(args.output.glob('*.AppImage'))
    if len(archives) != 1:
        raise RuntimeError(f'Expected exactly one AppImage, found {len(archives)}')
    archive = archives[0].resolve()
    report['appimage'] = str(archive)
    report['appimage_sha256'] = hashlib.sha256(archive.read_bytes()).hexdigest()
    checksum=archive.with_suffix('.AppImage.sha256').read_text().split()[0]
    if checksum != report['appimage_sha256']:
        raise RuntimeError('AppImage checksum sidecar differs')
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
        verify_linux(extracted)
        report['official_icons']='PASS (PNG bytes, hicolor, desktop entry and DirIcon)'
        report['release_audit']=audit(extracted)
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
        resources=extracted/'usr/bin'
        report['package_identity']=packaged(resources,binary,args.expected_commit,
            lambda command:run(command,cwd=clean).stdout)
        direct_info=run([copied,'--appimage-extract-and-run','--build-info'],cwd=clean).stdout
        if json.loads(direct_info) != report['package_identity']:
            raise RuntimeError('Direct AppImage identity differs from extracted frontend')
        report['direct_identity']='PASS'
        report['readelf'] = run(['readelf', '-h', binary]).stdout
        dynamic = run(['readelf', '-d', binary]).stdout
        report['dynamic_dependencies'] = dynamic
        report['ldd'] = run(['ldd', binary]).stdout
        if 'not found' in report['ldd']:
            raise RuntimeError('Unresolved dynamic dependency')
        report['dependencies'] = 'PASS'

        python = extracted / 'usr/bin/toolchain/python/bin/python3'
        zig = extracted / 'usr/bin/toolchain/zig/zig'
        report['tool_dependencies']={}
        for name,executable in [('Python',python),('Zig',zig)]:
            result=subprocess.run(['ldd',str(executable)],capture_output=True,text=True)
            text=result.stdout+result.stderr
            if 'not found' in text or (result.returncode and not any(s in text for s in ['not a dynamic executable','statically linked'])):
                raise RuntimeError('Unresolved '+name+' dependency: '+text)
            report['tool_dependencies'][name]=text
        report['python_version'] = run([python, '--version']).stdout.strip() or run([python, '--version']).stderr.strip()
        report['zig_version'] = run([zig, 'version']).stdout.strip()
        smoke_root = Path(name) / 'smoke'
        shutil.copytree(extracted / 'usr/bin', smoke_root)
        run([python, '-B', ROOT / 'tests/public_toolchain_smoke.py', smoke_root,
             '--guard-inventory',args.build/'ctest-inventory.json'])
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
        report['direct_execution']=checked_rejection(direct,direct_cache)
        extracted_cache=clean/'extracted cache'
        unpackaged_cache=clean/'unpackaged cache'
        for label,executable,cache in [('extracted',extracted/'AppRun',extracted_cache),
                                        ('unpackaged',args.build/'nes_native',unpackaged_cache)]:
            result=subprocess.run([str(executable),'--import-only','--rom',str(invalid_rom),
                '--import-cache',str(cache),'--import-tools',str(resources/'tools/recompiler')],
                cwd=clean,env={**env,'SDL_VIDEODRIVER':'dummy'},capture_output=True,text=True,timeout=60)
            report[label+'_execution']=checked_rejection(result,cache)
        run([python,'-B',ROOT/'tests/public_import_process_checks.py',resources,args.build/'import_probe'])
        report['import_process_scenarios']='PASS (3 synthetic scenarios)'
        report['clean_path_spaces'] = 'PASS'
    report['result'] = 'PASS'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
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
        print('Linux AppImage verification: ' + report['result'], flush=True)
