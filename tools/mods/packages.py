"""Local mod ZIP transactions. No script execution, ROM, network or third-party modules."""
import argparse
import contextlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tempfile
import uuid
import zipfile
import zlib

MAX_FILES = 128
MAX_TOTAL = 16 * 1024 * 1024
MAX_ARCHIVE = 8 * 1024 * 1024
MAX_SCRIPT = 128 * 1024
FIELDS = {'id', 'name', 'version', 'api_version', 'description', 'author', 'entry'}
ID = re.compile(r'[a-z0-9][a-z0-9.-]{0,63}\Z')
SUFFIXES = {'.json', '.lua', '.png', '.jpg', '.jpeg', '.txt', '.wav', '.ogg'}

def unique(pairs):
    result = {}
    for k, v in pairs:
        if k in result: raise ValueError('Duplicate JSON key')
        result[k] = v
    return result

def read_json(data):
    return json.loads(data, object_pairs_hook=unique)

def safe_name(name):
    if len(name) > 180 or not name or '\\' in name or ':' in name:
        raise ValueError('Unsafe ZIP path')
    if any(ord(c) < 32 or ord(c) > 126 for c in name): raise ValueError('ZIP paths must be printable ASCII')
    p = PurePosixPath(name)
    if p.is_absolute() or any(x in ('', '.', '..') for x in name.rstrip('/').split('/')):
        raise ValueError('Unsafe ZIP path')
    for part in p.parts:
        stem = part.split('.')[0].upper()
        if part.endswith((' ', '.')) or stem in {'CON', 'PRN', 'AUX', 'NUL'} or re.fullmatch(r'(COM|LPT)[0-9]', stem):
            raise ValueError('Reserved ZIP path')
    return p.as_posix().rstrip('/')

def manifest(data):
    if not isinstance(data, dict) or set(data) != FIELDS: raise ValueError('Invalid manifest fields')
    if not isinstance(data['id'], str) or not ID.fullmatch(data['id']) or '..' in data['id']:
        raise ValueError('Invalid mod ID')
    stem = data['id'].split('.')[0].upper()
    if data['id'].endswith('.') or stem in {'CON', 'PRN', 'AUX', 'NUL'} or re.fullmatch(r'(COM|LPT)[0-9]', stem): raise ValueError('Reserved mod ID')
    for key, limit in [('name', 60), ('author', 60), ('description', 300)]:
        value = data[key]
        if not isinstance(value, str) or not 1 <= len(value) <= limit or any(ord(c) < 32 or ord(c) > 126 for c in value):
            raise ValueError('Manifest text must be printable ASCII within limits')
    if not isinstance(data['version'], str) or not re.fullmatch(r'[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}', data['version']):
        raise ValueError('Invalid version')
    if type(data['api_version']) is not int or data['api_version'] != 1: raise ValueError('Incompatible mod API; version 1 required')
    if not isinstance(data['entry'], str) or safe_name(data['entry']) != 'main.lua': raise ValueError('Entry must be main.lua')
    return data

def atomic_json(path, data):
    fd, name = tempfile.mkstemp(prefix='.state-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            json.dump(data, f, sort_keys=True); f.write('\n'); f.flush(); os.fsync(f.fileno())
        os.replace(name, path)
    finally:
        if os.path.exists(name): os.unlink(name)

class Store:
    def __init__(self, root):
        self.root = Path(root).absolute()
        # Refuse linked ancestors, not just links inside the package root.
        for p in (self.root, *self.root.parents):
            if p.is_symlink(): raise ValueError('Mod storage cannot use symbolic links')
        self.root.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.packages = self.root / 'packages'
        if self.packages.is_symlink(): raise ValueError('Linked package directory')
        self.packages.mkdir(exist_ok=True, mode=0o700)

    @contextlib.contextmanager
    def lock(self):
        lock = self.root / '.transaction-lock'
        try: lock.mkdir(mode=0o700)
        except FileExistsError: raise ValueError('Another mod operation is running; stale locks require manual review')
        try: yield
        finally: lock.rmdir()

    def state(self):
        p = self.root / 'state.json'
        if p.is_symlink(): raise ValueError('Linked state file')
        if not p.exists(): return {'enabled': [], 'errors': {}}
        if p.stat().st_size > 65536: raise ValueError('State file too large')
        d = read_json(p.read_text())
        if not isinstance(d, dict) or set(d) != {'enabled', 'errors'} or not isinstance(d['enabled'], list) or not isinstance(d['errors'], dict):
            raise ValueError('Invalid mod state')
        if len(d['enabled']) > 32 or len(d['errors']) > 32: raise ValueError('Too many mods')
        for k in list(d['enabled']) + list(d['errors']):
            if not isinstance(k, str) or not ID.fullmatch(k): raise ValueError('Invalid state ID')
        if any(not isinstance(v, str) or len(v)>300 for v in d['errors'].values()): raise ValueError('Invalid persisted error')
        return d

    def installed(self):
        result = []
        paths = sorted(self.packages.iterdir())
        if len(paths) > 32: raise ValueError('Too many installed mods')
        state = self.state()
        for p in paths:
            if p.is_symlink() or not p.is_dir(): raise ValueError('Unexpected mod storage member')
            f = p / 'mod.json'
            if f.is_symlink() or f.stat().st_size > 8192: raise ValueError('Invalid installed manifest')
            m = manifest(read_json(f.read_bytes()))
            if p.name != m['id']: raise ValueError('Manifest ID does not match directory')
            script = p / m['entry']
            if script.is_symlink() or not script.is_file() or script.stat().st_size > MAX_SCRIPT: raise ValueError('Invalid installed entry')
            m = dict(m, enabled=m['id'] in state['enabled'], error=state['errors'].get(m['id'], ''))
            result.append(m)
        return result

    def install(self, archive):
        with self.lock():
            if Path(archive).stat().st_size > MAX_ARCHIVE: raise ValueError('ZIP too large')
            if len(list(self.packages.iterdir())) >= 32: raise ValueError('At most 32 mods may be installed')
            with zipfile.ZipFile(archive) as z:
                infos = z.infolist()
                if not 2 <= len(infos) <= MAX_FILES: raise ValueError('Invalid ZIP file count')
                names = {}; total = 0
                for i in infos:
                    n = safe_name(i.filename); key = n.lower()
                    if key in names: raise ValueError('Duplicate ZIP path')
                    names[key] = i
                    kind = stat.S_IFMT(i.external_attr >> 16)
                    if kind not in (0, stat.S_IFREG, stat.S_IFDIR): raise ValueError('ZIP links or special files forbidden')
                    if i.flag_bits & 1 or i.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED): raise ValueError('Unsupported ZIP encoding')
                    if i.is_dir() and i.file_size: raise ValueError('Nonempty ZIP directory payload')
                    if not i.is_dir():
                        if n not in ('mod.json', 'main.lua') and not (n.startswith('assets/') and Path(n).suffix.lower() in SUFFIXES - {'.lua', '.json'}):
                            raise ValueError('Unexpected file or executable in mod')
                        total += i.file_size
                        if i.file_size > MAX_TOTAL or i.file_size > max(1024 * 1024, i.compress_size * 100): raise ValueError('ZIP expansion limit')
                if total > MAX_TOTAL: raise ValueError('Expanded ZIP too large')
                for n, i in names.items():
                    for parent in PurePosixPath(n).parents:
                        if str(parent) in names and not names[str(parent)].is_dir(): raise ValueError('ZIP path collision')
                if 'mod.json' not in names or 'main.lua' not in names: raise ValueError('Missing manifest or main.lua')
                if names['mod.json'].file_size > 8192 or names['main.lua'].file_size > MAX_SCRIPT: raise ValueError('Script or manifest too large')
                m = manifest(read_json(z.read(names['mod.json'])))
                target = self.packages / m['id']
                if target.exists() or target.is_symlink(): raise ValueError('Mod already installed; uninstall before replacing it')
                stage = Path(tempfile.mkdtemp(prefix='.install-', dir=self.root))
                try:
                    for i in infos:
                        n = safe_name(i.filename); dest = stage / n
                        if i.is_dir(): dest.mkdir(parents=True, exist_ok=True); continue
                        dest.parent.mkdir(parents=True, exist_ok=True)
                        with z.open(i) as src, dest.open('xb') as out:
                            count = 0
                            while True:
                                chunk = src.read(65536)
                                if not chunk: break
                                count += len(chunk)
                                if count > i.file_size or count > MAX_TOTAL: raise ValueError('ZIP exceeded declared size')
                                if count == len(chunk) and chunk.startswith((b'MZ', b'\x7fELF', b'\xcf\xfa\xed\xfe', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xfe\xed\xfa\xce', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca', b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca', b'#!', b'\x1bLua')):
                                    raise ValueError('Executable or binary Lua payload forbidden')
                                out.write(chunk)
                            if count != i.file_size: raise ValueError('Truncated ZIP entry')
                    script = (stage / 'main.lua').read_bytes()
                    script.decode('utf-8')
                    if b'\0' in script: raise ValueError('Entry must be text Lua')
                    # A stale/manual state entry must never auto-enable a new package.
                    state = self.state()
                    state['enabled'] = [x for x in state['enabled'] if x != m['id']]
                    state['errors'].pop(m['id'], None)
                    atomic_json(self.root / 'state.json', state)
                    os.rename(stage, target)
                finally:
                    if stage.exists(): shutil.rmtree(stage)
                return m

    def change(self, action, mod_id, error=''):
        with self.lock():
            records = {m['id']: m for m in self.installed()}
            if mod_id not in records: raise ValueError('Mod not installed')
            state = self.state(); state['enabled'] = [x for x in state['enabled'] if x != mod_id]
            if action == 'enable': state['enabled'].append(mod_id); state['errors'].pop(mod_id, None)
            elif action == 'disable':
                if error: state['errors'][mod_id] = ''.join(c for c in error[:300] if 32 <= ord(c) < 127)
            elif action == 'uninstall': state['errors'].pop(mod_id, None)
            else: raise ValueError('Unknown action')
            # Deactivation persists before deletion, so an interrupted uninstall never auto-loads.
            atomic_json(self.root / 'state.json', state)
            if action == 'uninstall':
                source = self.packages / mod_id; trash = self.root / ('.removed-' + uuid.uuid4().hex)
                os.rename(source, trash); shutil.rmtree(trash)

def main():
    p = argparse.ArgumentParser(); p.add_argument('--root', required=True); p.add_argument('action', choices=['list', 'install', 'enable', 'disable', 'uninstall']); p.add_argument('value', nargs='?', default=''); p.add_argument('--error', default='')
    a = p.parse_args()
    try:
        s = Store(a.root)
        if a.action == 'install': s.install(a.value)
        elif a.action != 'list': s.change(a.action, a.value, a.error)
        print('OK')
        for m in s.installed():
            fields = [m['id'], m['name'], m['version'], m['author'], m['description'], str(int(m['enabled'])), m['error']]
            print('\t'.join(x.encode('utf-8').hex() for x in fields))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError, KeyError, TypeError, zlib.error) as e:
        print('ERROR: ' + str(e)); return 1

if __name__ == '__main__': raise SystemExit(main())
