"""Validate the extracted Windows ZIP using bundled tools and synthetic data only."""
import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import platform
import struct
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FOLDER = 'NES-Native-Lab-Windows-x86_64'


def pe_info(data):
    """Read COFF architecture and normal/delay DLL imports (Microsoft PE format)."""
    def unpack(fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise ValueError('Truncated PE structure')
        return struct.unpack_from(fmt, data, offset)
    if data[:2] != b'MZ':
        raise ValueError('Not a PE image')
    pe, = unpack('<I', 60)
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Missing PE signature')
    machine, sections = unpack('<HH', pe + 4)
    size, = unpack('<H', pe + 20)
    optional = pe + 24
    magic, = unpack('<H', optional)
    if magic not in (0x10b, 0x20b) or not 0 < sections <= 96:
        raise ValueError('Invalid PE optional header/section count')
    directories = optional + (112 if magic == 0x20b else 96)
    count, = unpack('<I', directories - 4)
    headers, = unpack('<I', optional + 60)
    section_table = optional + size
    def offset(rva):
        if rva < headers and rva < len(data):
            return rva
        for index in range(sections):
            virtual_size, address, raw_size, raw = unpack('<IIII', section_table + index * 40 + 8)
            if address <= rva < address + max(virtual_size, raw_size):
                delta = rva - address
                if delta < raw_size and raw + delta < len(data):
                    return raw + delta
        raise ValueError('Unmapped PE RVA')
    imports = []
    for index, length, name_index in [(1, 20, 3), (13, 32, 1)]:
        if index >= count:
            continue
        rva, table_size = unpack('<II', directories + index * 8)
        if not rva:
            continue
        table = offset(rva)
        for at in range(table, table + table_size, length):
            fields = unpack('<' + 'I' * (length // 4), at)
            if not any(fields):
                break
            if index == 13 and not fields[0] & 1:
                raise ValueError('Unsupported VA-based delay import')
            start = offset(fields[name_index])
            end = data.find(b'\0', start, start + 512)
            if end < 0:
                raise ValueError('Invalid PE import name')
            name = data[start:end].decode('ascii')
            if not name or Path(name).name != name or '/' in name or '\\' in name:
                raise ValueError('Unsafe PE import name')
            imports.append(name)
        else:
            raise ValueError('Unterminated PE import table')
    return dict(machine=hex(machine), magic=hex(magic), imports=sorted(set(imports)))


def archive_members(archive):
    seen = set()
    for entry in archive.infolist():
        path = PurePosixPath(entry.filename)
        if (path.is_absolute() or '..' in path.parts or '\\' in entry.filename
                or ':' in entry.filename or not path.parts or path.parts[0] != FOLDER
                or ((entry.external_attr >> 16) & 0o170000) == 0o120000):
            raise ValueError('Unsafe ZIP member: ' + entry.filename)
        key = entry.filename.casefold()
        if key in seen:
            raise ValueError('Case-colliding ZIP member: ' + entry.filename)
        seen.add(key)
    return sorted(seen)


def windows_api():
    from ctypes import wintypes as w
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    user = ctypes.WinDLL('user32', use_last_error=True)
    signatures = [
        (kernel.LoadLibraryExW, w.HMODULE, [w.LPCWSTR, w.HANDLE, w.DWORD]),
        (kernel.GetModuleFileNameW, w.DWORD, [w.HMODULE, w.LPWSTR, w.DWORD]),
        (kernel.FreeLibrary, w.BOOL, [w.HMODULE]),
        (user.GetWindowTextW, ctypes.c_int, [w.HWND, w.LPWSTR, ctypes.c_int]),
        (user.GetWindowThreadProcessId, w.DWORD, [w.HWND, ctypes.POINTER(w.DWORD)]),
        (user.PostMessageW, w.BOOL, [w.HWND, w.UINT, w.WPARAM, w.LPARAM]),
    ]
    for function, result, args in signatures:
        function.restype, function.argtypes = result, args
    return w, kernel, user


def dependency_inventory(resources):
    w, kernel, _ = windows_api()
    inventory = []
    resolved = {}
    python = resources / 'toolchain/python'
    # Upstream pip/setuptools contain launcher *templates* for other targets.
    # They are recorded, not treated as executable importer dependencies.
    templates = {
        'pip/_vendor/distlib/t32.exe', 'pip/_vendor/distlib/w32.exe',
        'pip/_vendor/distlib/t64-arm.exe', 'pip/_vendor/distlib/w64-arm.exe',
        'setuptools/cli-32.exe', 'setuptools/gui-32.exe',
        'setuptools/cli-arm64.exe', 'setuptools/gui-arm64.exe',
    }
    for file in sorted(resources.rglob('*')):
        if not file.is_file() or file.suffix.lower() not in ('.exe', '.dll', '.pyd'):
            continue
        info = pe_info(file.read_bytes())
        relative = file.relative_to(resources).as_posix()
        template = any(relative.endswith('/site-packages/' + item) for item in templates)
        if not template and (info['machine'] != '0x8664' or info['magic'] != '0x20b'):
            raise ValueError('Not Windows x64: ' + relative)
        record = dict(path=relative, **info, upstream_launcher_template=template, dependencies={})
        if not template:
            for name in info['imports']:
                # Frontend DLLs must be beside its EXE; Python's runtime also
                # owns extension dependencies. Never search the MSYS2 PATH.
                directories = [file.parent]
                if file.is_relative_to(python):
                    directories.extend([python, python / 'DLLs'])
                local = next((candidate for directory in directories if directory.is_dir()
                              for candidate in directory.iterdir()
                              if candidate.is_file() and candidate.name.casefold() == name.casefold()), None)
                if local:
                    record['dependencies'][name] = local.relative_to(resources).as_posix()
                else:
                    if name not in resolved:
                        handle = kernel.LoadLibraryExW(name, None, 0x800)  # SYSTEM32 only, including API sets
                        if not handle:
                            raise OSError(ctypes.get_last_error(), 'Missing package/system DLL: ' + relative + ': ' + name)
                        try:
                            text = ctypes.create_unicode_buffer(32768)
                            if not kernel.GetModuleFileNameW(handle, text, len(text)):
                                raise ctypes.WinError(ctypes.get_last_error())
                            location = Path(text.value).resolve()
                            if not location.is_relative_to(Path(os.environ['SystemRoot']).resolve()):
                                raise ValueError('DLL resolved outside Windows: ' + str(location))
                            resolved[name] = str(location)
                        finally:
                            kernel.FreeLibrary(handle)
                    record['dependencies'][name] = resolved[name]
        inventory.append(record)
    return inventory


def windows_for_pid(pid):
    w, _, user = windows_api()
    callback_type = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    user.EnumWindows.argtypes = [callback_type, w.LPARAM]
    user.EnumChildWindows.argtypes = [w.HWND, callback_type, w.LPARAM]
    found = []
    def text(hwnd):
        buffer = ctypes.create_unicode_buffer(16384)
        user.GetWindowTextW(hwnd, buffer, len(buffer))
        return buffer.value
    def visit(hwnd, unused):
        owner = w.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid:
            children = []
            def child(handle, ignored):
                children.append(text(handle))
                return True
            user.EnumChildWindows(hwnd, callback_type(child), 0)
            found.append(dict(handle=hwnd, title=text(hwnd), text='\n'.join(children)))
        return True
    user.EnumWindows(callback_type(visit), 0)
    return found


def package_processes(resources):
    """Inspect only processes whose executable lies in our unique extraction."""
    from ctypes import wintypes as w
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    class Entry(ctypes.Structure):
        _fields_ = [('dwSize', w.DWORD), ('cntUsage', w.DWORD), ('pid', w.DWORD),
                    ('heap', ctypes.c_size_t), ('module', w.DWORD), ('threads', w.DWORD),
                    ('parent', w.DWORD), ('priority', w.LONG), ('flags', w.DWORD), ('exe', w.WCHAR * 260)]
    kernel.CreateToolhelp32Snapshot.restype = w.HANDLE
    kernel.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
    kernel.Process32FirstW.argtypes = kernel.Process32NextW.argtypes = [w.HANDLE, ctypes.POINTER(Entry)]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)]
    kernel.CloseHandle.argtypes = [w.HANDLE]
    snapshot = kernel.CreateToolhelp32Snapshot(2, 0)
    if snapshot == w.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    processes = []
    try:
        entry = Entry(); entry.dwSize = ctypes.sizeof(entry)
        more = kernel.Process32FirstW(snapshot, ctypes.byref(entry))
        while more:
            handle = kernel.OpenProcess(0x1000, False, entry.pid)
            if handle:
                try:
                    buffer = ctypes.create_unicode_buffer(32768); size = w.DWORD(len(buffer))
                    if kernel.QueryFullProcessImageNameW(handle, 0, buffer, ctypes.byref(size)):
                        if Path(buffer.value).resolve().is_relative_to(resources):
                            processes.append(dict(pid=entry.pid, exe=buffer.value))
                finally:
                    kernel.CloseHandle(handle)
            more = kernel.Process32NextW(snapshot, ctypes.byref(entry))
    finally:
        kernel.CloseHandle(snapshot)
    return processes


def frontend_check(resources, directory, invalid=False):
    cache = directory / ('invalid import cache' if invalid else 'picker cache')
    args = [str(resources / 'nes_native.exe'), '--no-god', '--import-cache', str(cache)]
    if invalid:
        profile = json.loads((resources / 'tools/recompiler/compatible_profile.json').read_text())
        fixture = directory / 'synthetic invalid image.bin'
        fixture.write_bytes(b'NES\x1a' + bytes(profile['rom_size'] - 4))
        args.extend(['--import-only', '--rom', str(fixture)])
    process = subprocess.Popen(args, cwd=directory, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    observed = []
    try:
        deadline = time.monotonic() + 40
        while process.poll() is None and time.monotonic() < deadline:
            observed = windows_for_pid(process.pid)
            match = next((item for item in observed if ('SHA-256 distinto' in item['text'] if invalid
                         else 'Importa tu ROM compatible' in item['title'])), None)
            if match:
                _, _, user = windows_api()
                # OK for the real error box; WM_CLOSE cancels the native picker.
                if not user.PostMessageW(match['handle'], 0x111 if invalid else 0x10, 1 if invalid else 0, 0):
                    raise ctypes.WinError(ctypes.get_last_error())
                output, error = process.communicate(timeout=20)
                expected = 1 if invalid else 0
                if process.returncode != expected:
                    raise AssertionError('Frontend exit: ' + str(process.returncode) + ': ' + repr(error))
                if invalid:
                    log = cache / 'import.log'
                    if not log.is_file() or 'SHA-256 distinto' not in log.read_text(encoding='utf-8'):
                        raise AssertionError('Missing real frontend import diagnostic')
                    if list(cache.glob('user_game.*')):
                        raise AssertionError('Invalid image produced a game module')
                time.sleep(0.2)
                remaining = package_processes(resources)
                if remaining:
                    raise AssertionError('Package process left running: ' + repr(remaining))
                return dict(command=args, returncode=process.returncode, dialog=match,
                            stdout=output.decode('utf-8', errors='replace'), stderr=error.decode('utf-8', errors='replace'),
                            package_processes_after_exit=remaining)
            time.sleep(0.1)
        raise AssertionError('Frontend did not show expected dialog: ' + repr(observed) + '; exit=' + str(process.poll()))
    finally:
        if process.poll() is None:
            process.kill()  # Only our test process; this path is a FAIL, never normal shutdown.
        process.communicate(timeout=20)


def extracted_checks(resources, directory):
    commands = []
    def run(args, timeout=180):
        result = subprocess.run(list(map(str, args)), cwd=directory, capture_output=True, text=True,
                                encoding='utf-8', errors='replace', timeout=timeout)
        record = dict(command=list(map(str, args)), returncode=result.returncode,
                      stdout=result.stdout, stderr=result.stderr)
        commands.append(record)
        print(json.dumps(record), flush=True)
        result.check_returncode()
    report = dict(commands=commands)
    try:
        report['dependencies'] = dependency_inventory(resources)
        python = resources / 'toolchain/python/python.exe'
        zig = resources / 'toolchain/zig/zig.exe'
        run([python, '-I', '--version']); run([zig, 'version'])
        run([python, '-B', ROOT / 'tests/public_toolchain_smoke.py', resources])
        run([python, '-B', ROOT / 'tests/public_import_process_checks.py', resources,
             Path(os.environ['NES_PACKAGE_TEST_BUILD']) / 'import_probe.exe'])
        # Reuse the four existing cache decision tests, overriding only their
        # resource root to the extracted package. ROM validation/compiler are
        # mocked by that existing harness; this is NOT a real ROM cache import.
        cache_code = '''import importlib.util,sys,unittest
from pathlib import Path
resources=Path(sys.argv[1]); sys.path.insert(0,str(resources/'tools/recompiler'))
import import_user_rom
spec=importlib.util.spec_from_file_location('cache_checks',sys.argv[2])
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
module.ROOT=resources
assert module.importer.HERE == resources/'tools/recompiler'
result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromModule(module))
assert result.testsRun==4 and not result.skipped
sys.exit(0 if result.wasSuccessful() else 1)
'''
        run([python, '-B', '-c', cache_code, resources, ROOT / 'tests/import_cache_checks.py'])
        report['frontend_picker_cancel'] = frontend_check(resources, directory)
        report['frontend_invalid_import'] = frontend_check(resources, directory, invalid=True)
        report['status'] = 'PASS'
    except Exception as error:
        report.update(status='FAIL', error=repr(error))
        raise
    finally:
        Path(os.environ['NES_PACKAGE_CHILD_REPORT']).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


def verify(build, output, report_path):
    report = dict(host=platform.platform(), machine=platform.machine(), checks={}, status='FAIL')
    try:
        if os.name != 'nt' or platform.machine().lower() not in ('amd64', 'x86_64'):
            raise ValueError('Requires native Windows x64; cross-compilation is not verification')
        archive_path = output / (FOLDER + '.zip')
        report['zip_sha256'] = hashlib.sha256(archive_path.read_bytes()).hexdigest()
        report['zip_bytes'] = archive_path.stat().st_size
        archive_path.with_suffix('.zip.sha256').write_text(report['zip_sha256'] + '  ' + archive_path.name + '\n')
        with tempfile.TemporaryDirectory(prefix='nes extracted Windows & spaces ') as name:
            temp = Path(name).resolve(); working = temp / 'external working directory'; working.mkdir()
            if temp.is_relative_to(ROOT) or temp.is_relative_to(build):
                raise ValueError('Extraction must be outside checkout and build')
            with zipfile.ZipFile(archive_path) as archive:
                report['zip_members'] = archive_members(archive)
                if archive.testzip() is not None:
                    raise ValueError('ZIP CRC failure')
                archive.extractall(temp)
            resources = temp / FOLDER
            required = ['nes_native.exe', 'WINDOWS_README.txt', 'LICENSE', 'THIRD_PARTY.md', 'README.md',
                        'README.es.md', 'licenses/SDL2-LICENSE.txt', 'licenses/Lua-readme.html',
                        'toolchain/python/python.exe', 'toolchain/zig/zig.exe', 'toolchain/target.json',
                        'toolchain/runtime_tools.json', 'include/recomp/runtime_services.h',
                        'include/recomp/module_abi.h', 'game/include/game/recovered_resources.h',
                        'tools/recompiler/import_user_rom.py', 'tools/recompiler/emit_blocks.py',
                        'tools/recompiler/opcodes_6502.py', 'tools/recompiler/compatible_profile.json']
            for relative in required:
                if not (resources / relative).is_file():
                    raise ValueError('Missing required file: ' + relative)
            if json.loads((resources / 'toolchain/target.json').read_text())['system'] != 'windows-x86_64':
                raise ValueError('Wrong toolchain target')
            spec = importlib.util.spec_from_file_location('release_audit', ROOT / 'tools/public/audit_release.py')
            audit = importlib.util.module_from_spec(spec); spec.loader.exec_module(audit)
            report['release_audit'] = audit.audit(resources)
            report['checks'].update(required_files='PASS', zip_crc='PASS', public_content_audit='PASS')
            # Exclude all installed compilers/interpreters/MSYS2 from child PATH.
            env = {key: value for key, value in os.environ.items() if key.upper() not in ('PYTHONPATH', 'PYTHONHOME', 'NES_NATIVE_CC')}
            env.update(PATH=str(Path(env['SystemRoot']) / 'System32'), PYTHONDONTWRITEBYTECODE='1',
                       ZIG_GLOBAL_CACHE_DIR=str(temp / 'synthetic zig cache'), NES_PACKAGE_TEST_BUILD=str(build),
                       NES_PACKAGE_CHILD_REPORT=str(temp / 'child-report.json'))
            report['child_path'] = env['PATH']; report['extraction_outside_build'] = True
            python = resources / 'toolchain/python/python.exe'
            result = subprocess.run([str(python), '-B', str(Path(__file__).resolve()), '--extracted', str(resources),
                                     '--working', str(working)], cwd=working, env=env, capture_output=True,
                                    text=True, encoding='utf-8', errors='replace', timeout=600)
            report['child_returncode'] = result.returncode
            report['child_stdout'] = result.stdout; report['child_stderr'] = result.stderr
            child_report = temp / 'child-report.json'
            if child_report.exists():
                report['extracted_checks'] = json.loads(child_report.read_text(encoding='utf-8'))
            print(result.stdout, result.stderr, flush=True)
            result.check_returncode()
            report['checks']['bundled_tools_import_cache_frontend_shutdown'] = 'PASS'
        report['status'] = 'PASS'
    except Exception as error:
        report['error'] = repr(error)
        raise
    finally:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


class FormatChecks(unittest.TestCase):
    def image(self, machine=0x8664):
        data = bytearray(512); data[:2] = b'MZ'; struct.pack_into('<I', data, 60, 64)
        data[64:68] = b'PE\0\0'; struct.pack_into('<HH', data, 68, machine, 1)
        struct.pack_into('<H', data, 84, 240); struct.pack_into('<H', data, 88, 0x20b)
        struct.pack_into('<I', data, 148, 512)
        return data
    def test_x64(self): self.assertEqual(pe_info(self.image())['machine'], '0x8664')
    def test_other_arch_recorded(self): self.assertEqual(pe_info(self.image(0xaa64))['machine'], '0xaa64')
    def imported_image(self, delay=False):
        data = self.image() + bytearray(512)
        struct.pack_into('<I', data, 148, 1024)
        struct.pack_into('<I', data, 196, 16)
        struct.pack_into('<II', data, 200 + (13 if delay else 1) * 8, 600, 64 if delay else 40)
        struct.pack_into('<' + ('IIIIIIII' if delay else 'IIIII'), data, 600,
                         *([1, 700, 0, 0, 0, 0, 0, 0] if delay else [1, 0, 0, 700, 1]))
        data[700:713] = b'kernel32.dll\0'
        return data
    def test_dll_import(self): self.assertEqual(pe_info(self.imported_image())['imports'], ['kernel32.dll'])
    def test_delay_import(self): self.assertEqual(pe_info(self.imported_image(True))['imports'], ['kernel32.dll'])
    def test_bad_import_rva(self):
        data = self.imported_image(); struct.pack_into('<I', data, 612, 999999)
        with self.assertRaises(ValueError): pe_info(data)
    def test_invalid(self):
        with self.assertRaises(ValueError): pe_info(b'not an executable')
    def test_truncated(self):
        with self.assertRaises(ValueError): pe_info(b'MZ')
    def check_zip(self, names):
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / 'test.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                for entry in names: archive.writestr(entry, 'synthetic')
            with zipfile.ZipFile(path) as archive: return archive_members(archive)
    def test_archive_valid(self): self.assertEqual(len(self.check_zip([FOLDER + '/a.txt'])), 1)
    def test_archive_traversal(self):
        with self.assertRaises(ValueError): self.check_zip([FOLDER + '/../a.txt'])
    def test_archive_collision(self):
        with self.assertRaises(ValueError): self.check_zip([FOLDER + '/A.txt', FOLDER + '/a.txt'])
    def test_archive_wrong_root(self):
        with self.assertRaises(ValueError): self.check_zip(['other/a.txt'])


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test', action='store_true'); p.add_argument('--build', type=Path)
    p.add_argument('--output', type=Path); p.add_argument('--report', type=Path)
    p.add_argument('--extracted', type=Path); p.add_argument('--working', type=Path)
    args = p.parse_args()
    if args.self_test:
        result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(FormatChecks))
        sys.exit(0 if result.wasSuccessful() else 1)
    elif args.extracted:
        extracted_checks(args.extracted.resolve(), args.working.resolve())
    else:
        if not all((args.build, args.output, args.report)): p.error('--build, --output and --report are required')
        verify(args.build.resolve(), args.output.resolve(), args.report.resolve())
