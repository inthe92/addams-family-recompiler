"""Local mod ZIP transactions. No script execution, ROM, network or third-party modules."""
import hashlib
import math
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
import importlib.util
_RESOURCE_SPEC=importlib.util.spec_from_file_location("_nes_resource_tools",Path(__file__).with_name("resources.py"))
resource_tools=importlib.util.module_from_spec(_RESOURCE_SPEC)
_RESOURCE_SPEC.loader.exec_module(resource_tools)
_IMAGE_SPEC=importlib.util.spec_from_file_location("_nes_image_tools",Path(__file__).with_name("images.py"))
image_tools=importlib.util.module_from_spec(_IMAGE_SPEC);_IMAGE_SPEC.loader.exec_module(image_tools)

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

PERMISSIONS = {'observe.player': 1, 'overlay.text': 2, 'memory.read': 4, 'memory.observe': 8, 'resources.text':16, 'gameplay.jump':32, 'graphics.layers':64, 'ui.draw':128}

def version(value):
    if not isinstance(value,str) or not re.fullmatch(r'[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}',value):
        raise ValueError('Expected a numeric major.minor.patch version')
    return tuple(int(x) for x in value.split('.'))

def manifest(data):
    if not isinstance(data, dict): raise ValueError('Invalid manifest')
    api=data.get('api_version')
    if type(api) is not int or api not in (1,2): raise ValueError('Unsupported mod API; supported versions: 1 and 2')
    if api==2 and not isinstance(data.get('permissions'),list):raise ValueError('Permissions must be a list')
    resource_only=api==2 and data.get('entry') is None
    expected=FIELDS if api==1 else FIELDS|{'permissions','target'}|({'translation'} if resource_only else set())|({'images'} if 'graphics.layers' in data.get('permissions',[]) else set())
    optional={'category','dependencies','conflicts','settings'} if api==2 else set()
    if set(data)-optional != expected: raise ValueError('Invalid manifest fields for API version')
    metadata(data)
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
    if api==2:
        permissions=data['permissions']
        if not isinstance(permissions,list) or len(permissions)>len(PERMISSIONS) or any(not isinstance(p,str) or p not in PERMISSIONS for p in permissions) or len(set(permissions))!=len(permissions):
            raise ValueError('Unknown, duplicate or invalid mod permission')
        if 'memory.observe' in permissions and 'memory.read' not in permissions: raise ValueError('memory.observe requires memory.read')
        if resource_only:
            t=data['translation']
            if permissions!=['resources.text'] or not isinstance(t,dict) or set(t)!={'profile_id','file'} or not resource_tools.ident(t['profile_id']) or not isinstance(t['file'],str) or not re.fullmatch(r'translations/[a-z]{2}-[A-Z]{2}\.json',t['file']): raise ValueError('Invalid data-only translation declaration')
        elif 'resources.text' in permissions: raise ValueError('Text replacements require a data-only package')
        target=data['target']
        if not isinstance(target,dict) or set(target)!={'game_id','engine_min','engine_max_exclusive'}: raise ValueError('Invalid target requirements')
        if not isinstance(target['game_id'],str) or not ID.fullmatch(target['game_id']): raise ValueError('Invalid game profile ID')
        if version(target['engine_min'])>=version(target['engine_max_exclusive']): raise ValueError('Empty engine version range')
    if api==2 and 'graphics.layers' in data['permissions']:
        images=data['images']
        if resource_only or not isinstance(images,dict) or not 1<=len(images)<=2:raise ValueError('Declare one or two own images')
        used=set()
        for name,file in images.items():
            if not isinstance(name,str) or not ID.fullmatch(name) or not isinstance(file,str) or not re.fullmatch(r'assets/[a-z0-9][a-z0-9-]{0,47}\.png',file) or file in used:raise ValueError('Invalid/duplicate declared image')
            used.add(file)
    if not resource_only and (not isinstance(data['entry'], str) or safe_name(data['entry']) != 'main.lua'): raise ValueError('Entry must be main.lua')
    return data

def metadata(m):
    category=m.get('category','Uncategorized')
    if not isinstance(category,str) or not re.fullmatch(r'[A-Za-z0-9 -]{1,32}',category):raise ValueError('Invalid mod category')
    deps=m.get('dependencies',{})
    if not isinstance(deps,dict) or len(deps)>16:raise ValueError('Invalid dependencies')
    for key,r in deps.items():
        if not isinstance(key,str) or not ID.fullmatch(key) or key==m['id'] or not isinstance(r,dict) or set(r)!={'min','max_exclusive'} or version(r['min'])>=version(r['max_exclusive']):raise ValueError('Invalid dependency version range')
    conflicts=m.get('conflicts',[])
    if not isinstance(conflicts,list) or len(conflicts)>16 or any(not isinstance(k,str) or not ID.fullmatch(k) or k==m['id'] for k in conflicts) or len(set(conflicts))!=len(conflicts):raise ValueError('Invalid conflicts')
    settings=m.get('settings',{})
    if not isinstance(settings,dict) or len(settings)>8:raise ValueError('At most eight settings per mod')
    for key,d in settings.items():
        if not isinstance(key,str) or not re.fullmatch(r'[a-z][a-z0-9_]{0,31}',key) or not isinstance(d,dict):raise ValueError('Invalid setting ID')
        t=d.get('type');required={'type','label','default'}|({'min','max','step'} if t in ('integer','float') else {'choices'} if t=='enum' else set())
        if t not in ('boolean','integer','float','enum') or set(d)!=required:raise ValueError('Invalid setting schema')
        if not isinstance(d['label'],str) or not re.fullmatch(r'[A-Za-z0-9 -]{1,32}',d['label']):raise ValueError('Invalid setting label')
        if t in ('integer','float'):
            for k in ('min','max','step'):
                n=d[k]
                if type(n) not in (int,float) or not math.isfinite(n) or abs(n)>1000000 or (t=='integer' and type(n)!=int):raise ValueError('Invalid numeric bounds')
            if d['min']>d['max'] or d['step']<=0 or d['step']>max(1,d['max']-d['min']):raise ValueError('Invalid numeric range')
        if t=='enum' and (not isinstance(d['choices'],list) or not 1<=len(d['choices'])<=8 or any(not isinstance(x,str) or not re.fullmatch(r'[a-z0-9-]{1,24}',x) for x in d['choices']) or len(set(d['choices']))!=len(d['choices'])):raise ValueError('Invalid enum choices')
        setting_value(d,d['default'])

def setting_value(d,v):
    t=d['type']
    if t=='boolean':ok=type(v)==bool
    elif t=='enum':ok=isinstance(v,str) and v in d['choices']
    else:ok=type(v) in ((int,) if t=='integer' else (int,float)) and math.isfinite(v) and d['min']<=v<=d['max']
    if not ok:raise ValueError('Invalid value for setting '+d['label'])
    return v

def config_values(m,values):
    if not isinstance(values,dict) or set(values)-set(m.get('settings',{})):raise ValueError('Unknown mod setting')
    return {k:setting_value(d,values.get(k,d['default'])) for k,d in m.get('settings',{}).items()}

def category(m):
    if 'category' in m:return m['category']
    p=set(m.get('permissions',[]))
    if 'gameplay.jump' in p:return 'Gameplay'
    if 'graphics.layers' in p:return 'Visual'
    if p&{'ui.draw','overlay.text'}:return 'HUD'
    if 'resources.text' in p:return 'Resources'
    return 'Utility'

def config_wire(m):
    rows=[]
    for k,d in sorted(m.get('settings',{}).items()):
        v=m['configuration'][k];v=str(v).lower() if type(v)==bool else str(v)
        rows.append('|'.join([k,d['label'],d['type'],str(d.get('min',0)),str(d.get('max',0)),str(d.get('step',1)),','.join(d.get('choices',[])),v,str(d['default']).lower() if type(d['default'])==bool else str(d['default'])]))
    return '\n'.join(rows)

def zip_name(info):
    # CPython keeps the archive name in orig_filename before Windows separator
    # normalization/NUL truncation. Validate it before trusting filename.
    name = safe_name(info.orig_filename)
    if info.filename != info.orig_filename:
        raise ValueError('Normalized or alternate ZIP path forbidden')
    return name

def atomic_json(path, data):
    if len(json.dumps(data,sort_keys=True).encode('utf-8'))>65000:raise ValueError('Mod state quota exceeded')
    fd, name = tempfile.mkstemp(prefix='.state-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            json.dump(data, f, sort_keys=True); f.write('\n'); f.flush(); os.fsync(f.fileno())
        os.replace(name, path)
    finally:
        if os.path.exists(name): os.unlink(name)

class Store:
    def __init__(self, root, engine_version=None, game_id=None, rom_sha256=None, profiles=None):
        self.engine_version,self.game_id=engine_version,game_id
        self.rom_sha256=rom_sha256
        self.profile_path=Path(profiles) if profiles else Path(__file__).with_name("resource_profiles.json")
        self.root = Path(root).absolute()
        # Refuse linked ancestors, not just links inside the package root.
        for p in (self.root, *self.root.parents):
            if p.is_symlink(): raise ValueError('Mod storage cannot use symbolic links')
        self.root.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.packages = self.root / 'packages'
        if self.packages.is_symlink(): raise ValueError('Linked package directory')
        self.packages.mkdir(exist_ok=True, mode=0o700)

    def compatible(self, m):
        if m['api_version']==1: return
        if self.engine_version is None or self.game_id is None: raise ValueError('API v2 requires explicit engine and game profile context')
        target=m['target']
        if target['game_id']!=self.game_id: raise ValueError('Mod requires a different game profile')
        if not version(target['engine_min'])<=version(self.engine_version)<version(target['engine_max_exclusive']):
            raise ValueError('Mod requires a different engine version')

    def translation(self,m,data,glyph_files=None):
        profiles=resource_tools.catalog(self.profile_path)
        if m['translation']['profile_id'] not in profiles: raise ValueError('Unknown resource profile')
        p=profiles[m['translation']['profile_id']]
        if p['game_id']!=self.game_id or not self.rom_sha256 or p['rom_sha256']!=self.rom_sha256: raise ValueError('Translation requires the exact supported ROM revision')
        parsed=resource_tools.read(data)
        if not isinstance(parsed,dict):raise ValueError('Translation must be an object')
        if parsed.get('locale')!=Path(m['translation']['file']).stem:raise ValueError('Translation locale does not match declared file')
        return p,resource_tools.compile_translation(data,p,glyph_files)

    def replacements(self,m):
        if m['entry'] is not None:return []
        path=self.packages/m['id']/m['translation']['file']
        if path.is_symlink() or path.stat().st_size>resource_tools.MAX_TRANSLATION:raise ValueError('Invalid installed translation')
        raw=path.read_bytes();d=resource_tools.read(raw);glyphs=None
        if not isinstance(d,dict):raise ValueError('Translation must be an object')
        if d.get('schema')==2:
            # Paths are checked before any filesystem access.
            profile=resource_tools.catalog(self.profile_path).get(m['translation']['profile_id'])
            if profile is None:raise ValueError('Unknown resource profile')
            declarations=resource_tools.glyph_declarations(d,profile);glyphs={}
            for g in declarations.values():
                file=self.packages/m['id']/g['file']
                if file.is_symlink() or file.parent.is_symlink() or file.stat().st_size!=16:raise ValueError('Invalid installed own glyph')
                glyphs[g['file']]=file.read_bytes()
        p,items=self.translation(m,raw,glyphs);return [(p,name,data) for name,data in items]

    def images(self,m):
        if 'graphics.layers' not in m.get('permissions',[]):return []
        result=[];total=0
        for name,file in sorted(m['images'].items()):
            path=self.packages/m['id']/file
            if path.is_symlink() or path.parent.is_symlink() or path.stat().st_size>image_tools.MAX_PNG:raise ValueError('Invalid installed image')
            w,h,rgba=image_tools.decode(path.read_bytes());total+=len(rgba)
            if total>131072:raise ValueError('Mod image memory quota exceeded')
            result.append((name,w,h,rgba))
        return result

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
        if not isinstance(d, dict) or not {'enabled','errors'}<=set(d) or set(d)-{'enabled','errors','management'} or not isinstance(d['enabled'], list) or not isinstance(d['errors'], dict):
            raise ValueError('Invalid mod state')
        if len(d['enabled']) > 32 or len(d['errors']) > 32: raise ValueError('Too many mods')
        for k in list(d['enabled']) + list(d['errors']):
            if not isinstance(k, str) or not ID.fullmatch(k): raise ValueError('Invalid state ID')
        if any(not isinstance(v, str) or len(v)>300 for v in d['errors'].values()): raise ValueError('Invalid persisted error')
        self.management(d)
        return d

    def management(self,state):
        mg=state.setdefault('management',{'schema':1,'configs':{},'profiles':{},'active':''})
        if not isinstance(mg,dict) or set(mg)!={'schema','configs','profiles','active'} or type(mg['schema']) is not int or mg['schema']!=1 or not isinstance(mg['configs'],dict) or not isinstance(mg['profiles'],dict) or len(mg['profiles'])>16 or len(mg['configs'])>32 or not isinstance(mg['active'],str):raise ValueError('Invalid management state')
        for k,v in mg['configs'].items():
            if not isinstance(k,str) or not ID.fullmatch(k) or not isinstance(v,dict):raise ValueError('Invalid saved settings')
        for name,profile in mg['profiles'].items():
            self.profile_name(name)
            if not isinstance(profile,dict) or not {'enabled','configs'}<=set(profile) or set(profile)-{'enabled','configs','requirements','context'} or not isinstance(profile['enabled'],list) or len(profile['enabled'])>32 or len(set(profile['enabled']))!=len(profile['enabled']) or any(not isinstance(k,str) or not ID.fullmatch(k) for k in profile['enabled']) or not isinstance(profile['configs'],dict) or len(profile['configs'])>32:raise ValueError('Invalid saved profile')
            self.saved_configs(profile['configs'])
            if 'requirements' in profile:self.requirements(profile['requirements'])
            if 'context' in profile:self.profile_context(profile['context'])
        self.saved_configs(mg['configs'])
        if len({name.casefold() for name in mg['profiles']})!=len(mg['profiles']):raise ValueError('Ambiguous profile names')
        if mg['active'] and mg['active']!='Original' and mg['active'] not in mg['profiles']:raise ValueError('Unknown active profile')
        return mg

    @staticmethod
    def profile_name(name):
        if not isinstance(name,str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 -]{0,23}',name) or name.casefold()=='original':raise ValueError('Use a unique profile name (1-24 letters or digits)')

    @staticmethod
    def saved_configs(configs):
        for key,values in configs.items():
            if not isinstance(key,str) or not ID.fullmatch(key) or not isinstance(values,dict) or len(values)>8:raise ValueError('Invalid saved configuration identity')
            for option,value in values.items():
                if not isinstance(option,str) or not re.fullmatch(r'[a-z][a-z0-9_]{0,31}',option):raise ValueError('Invalid saved setting ID')
                if type(value) not in (bool,int,float,str):raise ValueError('Invalid saved setting type')
                if type(value) in (int,float) and (not math.isfinite(value) or abs(value)>1000000):raise ValueError('Invalid saved numeric setting')
                if isinstance(value,str) and not re.fullmatch(r'[a-z0-9-]{1,24}',value):raise ValueError('Invalid saved enum')

    @staticmethod
    def requirements(values):
        if not isinstance(values,dict) or len(values)>32:raise ValueError('Invalid profile requirements')
        for key,r in values.items():
            if not isinstance(key,str) or not ID.fullmatch(key) or not isinstance(r,dict) or set(r)!={'version','api_version'} or type(r['api_version']) is not int or r['api_version'] not in (1,2):raise ValueError('Invalid profile mod identity')
            version(r['version'])

    @staticmethod
    def profile_context(value):
        if not isinstance(value,dict) or set(value)!={'game_id','engine_version'} or not isinstance(value['game_id'],str) or not ID.fullmatch(value['game_id']):raise ValueError('Invalid profile context')
        version(value['engine_version'])

    def validate_profile(self,records,target):
        self.validate_selection(records,set(target['enabled']))
        for key,values in target['configs'].items():
            if key not in records:raise ValueError('Profile has settings for missing mod '+key)
            if records[key].get('_invalid'):raise ValueError('Profile requires damaged package '+key)
            config_values(records[key],values)
        if 'context' in target and target['context']!={'game_id':self.game_id,'engine_version':self.engine_version}:raise ValueError('Profile requires a different engine or game')
        for key,r in target.get('requirements',{}).items():
            if key not in records:raise ValueError('Profile requires missing mod '+key)
            if records[key]['version']!=r['version'] or records[key]['api_version']!=r['api_version']:raise ValueError('Profile requires '+key+' version '+r['version']+' / API '+str(r['api_version']))

    def installed(self):
        result = []
        paths = sorted(self.packages.iterdir())
        if len(paths) > 32: raise ValueError('Too many installed mods')
        state = self.state()
        for p in paths:
            if p.is_symlink() or not p.is_dir(): raise ValueError('Unexpected mod storage member')
            if not ID.fullmatch(p.name) or '..' in p.name:raise ValueError('Unsafe installed package identity; manual review required')
            try:
                f = p / 'mod.json'
                if f.is_symlink() or f.stat().st_size > 8192: raise ValueError('Invalid installed manifest')
                m = manifest(read_json(f.read_bytes()))
                if p.name != m['id']: raise ValueError('Manifest ID does not match directory')
                if m['entry'] is not None:
                    script=p/m['entry']
                    if script.is_symlink() or not script.is_file() or script.stat().st_size>MAX_SCRIPT:raise ValueError('Invalid installed entry')
                else:
                    t=p/m['translation']['file']
                    if t.is_symlink() or not t.is_file() or t.stat().st_size>resource_tools.MAX_TRANSLATION:raise ValueError('Invalid installed translation')
                m = dict(m, enabled=m['id'] in state['enabled'], error=state['errors'].get(m['id'], ''))
                m['configuration']=config_values(m,self.management(state)['configs'].get(m['id'],{}))
            except (OSError,ValueError,KeyError,TypeError) as error:
                m=dict(id=p.name,name=p.name,version='0.0.0',api_version=1,author='Unknown',description='Damaged installed package. Uninstall or review its files.',entry='main.lua',enabled=False,error=('Invalid package: '+str(error))[:300],configuration={},_invalid=True)
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
                    n = zip_name(i); key = n.lower()
                    if key in names: raise ValueError('Duplicate ZIP path')
                    names[key] = i
                    kind = stat.S_IFMT(i.external_attr >> 16)
                    if kind not in (0, stat.S_IFREG, stat.S_IFDIR): raise ValueError('ZIP links or special files forbidden')
                    if i.flag_bits & 1 or i.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED): raise ValueError('Unsupported ZIP encoding')
                    if i.is_dir() and i.file_size: raise ValueError('Nonempty ZIP directory payload')
                    if not i.is_dir():
                        if n not in ('mod.json', 'main.lua') and not re.fullmatch(r'translations/[a-z]{2}-[A-Z]{2}\.json',n) and not (n.startswith('assets/') and Path(n).suffix.lower() in (SUFFIXES - {'.lua', '.json'}) | {'.chr'}):
                            raise ValueError('Unexpected file or executable in mod')
                        total += i.file_size
                        if i.file_size > MAX_TOTAL or i.file_size > max(1024 * 1024, i.compress_size * 100): raise ValueError('ZIP expansion limit')
                if total > MAX_TOTAL: raise ValueError('Expanded ZIP too large')
                for n, i in names.items():
                    for parent in PurePosixPath(n).parents:
                        if str(parent) in names and not names[str(parent)].is_dir(): raise ValueError('ZIP path collision')
                if 'mod.json' not in names:raise ValueError('Missing manifest')
                if names['mod.json'].file_size>8192 or ('main.lua' in names and names['main.lua'].file_size>MAX_SCRIPT):raise ValueError('Script or manifest too large')
                m = manifest(read_json(z.read(names['mod.json'])))
                self.compatible(m)
                translation_paths=[n for n in names if n.startswith('translations/') and not names[n].is_dir()]
                if m['entry'] is None:
                    file=m['translation']['file']
                    if 'main.lua' in names or translation_paths!=[file.lower()] or names[file.lower()].orig_filename!=file or names[file.lower()].file_size>resource_tools.MAX_TRANSLATION:raise ValueError('Data-only package requires exactly its declared translation and no Lua')
                    raw=z.read(names[file.lower()]);d=resource_tools.read(raw);glyphs=None
                    if not isinstance(d,dict):raise ValueError('Translation must be an object')
                    declared=set()
                    if d.get('schema')==2:
                        profile=resource_tools.catalog(self.profile_path).get(m['translation']['profile_id'])
                        if profile is None:raise ValueError('Unknown resource profile')
                        declarations=resource_tools.glyph_declarations(d,profile)
                        declared={g['file'] for g in declarations.values()};glyphs={}
                        for f in declared:
                            if f not in names or names[f].orig_filename!=f or names[f].file_size!=16 or names[f].is_dir():raise ValueError('Missing/alternate/oversized own glyph file')
                            glyphs[f]=z.read(names[f])
                    if {n for n in names if n.endswith('.chr')}!=declared:raise ValueError('Undeclared own glyph file')
                    self.translation(m,raw,glyphs)
                elif 'main.lua' not in names or translation_paths or any(n.endswith('.chr') for n in names):raise ValueError('Script package requires main.lua and no translation JSON')
                if 'graphics.layers' in m.get('permissions',[]):
                    declared=set(m['images'].values());actual={n for n in names if Path(n).suffix.lower()=='.png'}
                    if actual!=declared:raise ValueError('Missing/undeclared own image')
                    for file in declared:
                        info=names[file]
                        if info.orig_filename!=file or info.is_dir() or info.file_size>image_tools.MAX_PNG:raise ValueError('Invalid/alternate own image file')
                        image_tools.decode(z.read(info))
                target = self.packages / m['id']
                if target.exists() or target.is_symlink(): raise ValueError('Mod already installed; uninstall before replacing it')
                stage = Path(tempfile.mkdtemp(prefix='.install-', dir=self.root))
                try:
                    for i in infos:
                        n = zip_name(i); dest = stage / n
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
                    if m['entry'] is not None:
                        script=(stage/'main.lua').read_bytes();script.decode('utf-8')
                        if b'\0' in script:raise ValueError('Entry must be text Lua')
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
            enabled={k for k,m in records.items() if m['enabled']}
            if action=='enable':self.validate_selection(records,enabled|{mod_id})
            if action=='disable' and not error:self.validate_dependencies(records,enabled-{mod_id})
            if action=='uninstall':self.validate_dependencies(records,enabled-{mod_id})
            state = self.state(); state['enabled'] = [x for x in state['enabled'] if x != mod_id]
            if action == 'enable': state['enabled'].append(mod_id); state['errors'].pop(mod_id, None)
            elif action == 'disable':
                if error:
                    state['errors'][mod_id] = ''.join(c for c in error[:300] if 32 <= ord(c) < 127)
                    removed={mod_id}
                    while True:
                        dependents={k for k in state['enabled'] if set(records[k].get('dependencies',{}))&removed}
                        if not dependents:break
                        removed|=dependents;state['enabled']=[k for k in state['enabled'] if k not in dependents]
                        for k in dependents:state['errors'][k]='Required mod failed; review dependencies before re-enabling'

            elif action == 'uninstall': state['errors'].pop(mod_id, None)
            else: raise ValueError('Unknown action')
            # Deactivation persists before deletion, so an interrupted uninstall never auto-loads.
            mg=self.management(state);mg['active']=''
            if action=='uninstall':mg['configs'].pop(mod_id,None)
            atomic_json(self.root / 'state.json', state)
            if action == 'uninstall':
                source = self.packages / mod_id; trash = self.root / ('.removed-' + uuid.uuid4().hex)
                os.rename(source, trash); shutil.rmtree(trash)

    def validate_dependencies(self,records,enabled):
        visiting=set();done=set()
        def visit(key):
            if key in visiting:raise ValueError('Cyclic mod dependencies')
            if key in done:return
            if key not in records:raise ValueError('Profile requires missing mod '+key)
            visiting.add(key)
            for dep,r in records[key].get('dependencies',{}).items():
                if dep not in enabled or dep not in records:raise ValueError(key+' requires enabled mod '+dep)
                if not version(r['min'])<=version(records[dep]['version'])<version(r['max_exclusive']):raise ValueError('Incompatible dependency '+dep)
                visit(dep)
            visiting.remove(key);done.add(key)
        for key in sorted(enabled):visit(key)

    def validate_selection(self,records,enabled):
        self.validate_dependencies(records,enabled)
        jump=None;visual=0;owned=set()
        for key in sorted(enabled):
            m=records[key]
            if m.get('_invalid'):raise ValueError('Repair or uninstall invalid package '+key)
            self.compatible(m);self.images(m)
            for other in m.get('conflicts',[]):
                if other in enabled:raise ValueError('Conflict: '+key+' and '+other)
            if 'gameplay.jump' in m.get('permissions',[]):
                if jump:raise ValueError('Gameplay conflict: jump_extension owned by '+jump+'; disable it first')
                jump=key
            visual+=bool(set(m.get('permissions',[]))&{'graphics.layers','ui.draw'})
            if visual>8:raise ValueError('At most 8 active UI/visual mods; disable one first')
            for p,n,d in self.replacements(m):
                pair=(p['profile_id'],n)
                if pair in owned:raise ValueError('Resource conflict: '+n+' already owned; disable it first')
                owned.add(pair)
            if len(owned)>16:raise ValueError('At most 16 active text resources')

    def configure(self,payload):
        d=read_json(payload)
        if not isinstance(d,dict) or set(d)!={'id','values'}:raise ValueError('Invalid configuration request')
        with self.lock():
            records={m['id']:m for m in self.installed()}
            if d['id'] not in records:raise ValueError('Mod not installed')
            if records[d['id']].get('_invalid'):raise ValueError('Repair or uninstall invalid package')
            values=config_values(records[d['id']],d['values']);state=self.state();mg=self.management(state)
            if values==records[d['id']]['configuration']:return False
            mg['configs'][d['id']]=values;mg['active']='';atomic_json(self.root/'state.json',state)
            return True

    def profile(self,action,payload):
        with self.lock():
            state=self.state();mg=self.management(state)
            if action=='profile-save':
                self.profile_name(payload)
                if any(n.casefold()==payload.casefold() and n!=payload for n in mg['profiles']):raise ValueError('Profile name already used (case insensitive)')
                if payload not in mg['profiles'] and len(mg['profiles'])==16:raise ValueError('At most 16 profiles')
                records={m['id']:m for m in self.installed()};configs=mg['configs'].copy()
                for key in state['enabled']:
                    if key not in records or records[key].get('_invalid'):raise ValueError('Cannot save a profile with an invalid mod '+key)
                    if records[key].get('settings'):configs[key]=records[key]['configuration'].copy()
                mg['profiles'][payload]={'enabled':sorted(state['enabled']),'configs':configs};mg['active']=payload
            elif action=='profile-rename':
                d=read_json(payload)
                if not isinstance(d,dict) or set(d)!={'old','name'} or d['old'] not in mg['profiles']:raise ValueError('Unknown profile')
                self.profile_name(d['name'])
                if any(n.casefold()==d['name'].casefold() and n!=d['old'] for n in mg['profiles']):raise ValueError('Profile name already used')
                mg['profiles'][d['name']]=mg['profiles'].pop(d['old'])
                if mg['active']==d['old']:mg['active']=d['name']
            elif action=='profile-delete':
                self.profile_name(payload)
                if payload not in mg['profiles']:raise ValueError('Unknown profile')
                del mg['profiles'][payload]
                if mg['active']==payload:mg['active']=''
            elif action=='profile-apply':
                target={'enabled':[],'configs':mg['configs']} if payload=='Original' else mg['profiles'].get(payload)
                if target is None:raise ValueError('Unknown profile')
                records={m['id']:m for m in self.installed()};self.validate_profile(records,target)
                configs=mg['configs'].copy();configs.update(target['configs'])
                for key in target['enabled']:
                    if records[key].get('settings'):configs[key]=config_values(records[key],target['configs'].get(key,{}))
                same_selection=set(state['enabled'])==set(target['enabled'])
                unchanged=same_selection and all(config_values(records[k],mg['configs'].get(k,{}))==config_values(records[k],configs.get(k,{})) for k in target['enabled'])
                state['enabled']=target['enabled'].copy();mg['configs']=configs;mg['active']=payload
                for key in state['enabled']:state['errors'].pop(key,None)
            else:raise ValueError('Unknown profile operation')
            atomic_json(self.root/'state.json',state)
            return action!='profile-apply' or not unchanged

    def portable_profile(self,name):
        state=self.state();mg=self.management(state)
        target={'enabled':[],'configs':{}} if name=='Original' else mg['profiles'].get(name)
        if target is None:raise ValueError('Unknown profile')
        records={m['id']:m for m in self.installed()};self.validate_profile(records,target)
        context={'game_id':self.game_id,'engine_version':self.engine_version};self.profile_context(context)
        mods=[]
        for key in sorted(set(target['enabled'])|set(target['configs'])):
            m=records[key];values=config_values(m,target['configs'].get(key,{}))
            mods.append(dict(id=key,version=m['version'],api_version=m['api_version'],enabled=key in target['enabled'],settings={k:dict(type=m['settings'][k]['type'],value=v) for k,v in values.items()}))
        return dict(schema=1,name=name,context=context,mods=mods)

    def inspect_profile(self,file,expected=None):
        file=Path(file)
        if file.is_symlink() or not file.is_file() or file.stat().st_size>65536:raise ValueError('Profile must be a regular JSON file <=64 KiB')
        with file.open('rb') as f:raw=f.read(65537)
        if len(raw)>65536:raise ValueError('Profile exceeds64 KiB')
        digest=hashlib.sha256(raw).hexdigest()
        if expected is not None and digest!=expected:raise ValueError('Profile changed after review; inspect it again')
        d=read_json(raw)
        if not isinstance(d,dict) or set(d)!={'schema','name','context','mods'} or type(d['schema']) is not int or d['schema']!=1:raise ValueError('Unsupported portable profile schema')
        self.profile_name(d['name']);self.profile_context(d['context'])
        if not isinstance(d['mods'],list) or len(d['mods'])>32:raise ValueError('Invalid profile mod list')
        records={m['id']:m for m in self.installed()};target={'enabled':[],'configs':{},'requirements':{},'context':d['context']}
        for m in d['mods']:
            if not isinstance(m,dict) or set(m)!={'id','version','api_version','enabled','settings'} or type(m['enabled']) is not bool:raise ValueError('Invalid portable mod declaration')
            key=m['id'];self.requirements({key:{'version':m['version'],'api_version':m['api_version']}})
            if key in target['requirements']:raise ValueError('Duplicate profile mod ID')
            if key not in records:raise ValueError('Install required mod first: '+key)
            if records[key].get('_invalid'):raise ValueError('Repair required mod first: '+key)
            self.compatible(records[key])
            defs=records[key].get('settings',{})
            if not isinstance(m['settings'],dict) or set(m['settings'])!=set(defs):raise ValueError('Profile settings do not match installed schema for '+key)
            values={}
            for option,v in m['settings'].items():
                if not isinstance(v,dict) or set(v)!={'type','value'} or v['type']!=defs[option]['type']:raise ValueError('Profile setting type mismatch: '+option)
                values[option]=setting_value(defs[option],v['value'])
            target['configs'][key]=values
            target['requirements'][key]={'version':m['version'],'api_version':m['api_version']}
            if m['enabled']:target['enabled'].append(key)
        self.validate_profile(records,target)
        return d['name'],target,digest

    def import_profile(self,payload):
        d=read_json(payload)
        if not isinstance(d,dict) or set(d)!={'file','sha256'} or not isinstance(d['file'],str) or not isinstance(d['sha256'],str) or not re.fullmatch(r'[a-f0-9]{64}',d['sha256']):raise ValueError('Invalid profile import request')
        with self.lock():
            name,target,digest=self.inspect_profile(d['file'],d['sha256']);state=self.state();mg=self.management(state)
            if any(n.casefold()==name.casefold() for n in mg['profiles']):raise ValueError('Profile name already used; rename the existing profile first')
            if len(mg['profiles'])==16:raise ValueError('At most 16 profiles')
            mg['profiles'][name]=target;atomic_json(self.root/'state.json',state)
            return name

    def export_profile(self,payload):
        d=read_json(payload)
        if not isinstance(d,dict) or set(d)!={'name','file'} or not isinstance(d['file'],str):raise ValueError('Invalid profile export request')
        with self.lock():
            data=self.portable_profile(d['name']);file=Path(d['file'])
            if file.exists() or file.is_symlink():raise ValueError('Export file already exists; choose a new filename')
            if not file.parent.is_dir():raise ValueError('Export destination directory is missing')
            fd,temp=tempfile.mkstemp(prefix='.profile-',dir=file.parent)
            try:
                with os.fdopen(fd,'w',encoding='utf-8') as f:json.dump(data,f,sort_keys=True);f.write('\n');f.flush();os.fsync(f.fileno())
                # Publish atomically without replacing a pre-existing user file.
                os.link(temp,file)
            finally:os.unlink(temp)


def main():
    p = argparse.ArgumentParser(); p.add_argument('--root', required=True); p.add_argument('--engine-version'); p.add_argument('--game-id'); p.add_argument('--rom-sha256'); p.add_argument('--profiles'); p.add_argument('action', choices=['list', 'check', 'resources', 'assets', 'install', 'enable', 'disable', 'uninstall','configure','profile-save','profile-rename','profile-apply','profile-delete','profile-inspect','profile-import','profile-export']); p.add_argument('value', nargs='?', default=''); p.add_argument('--error', default='')
    a = p.parse_args()
    try:
        s = Store(a.root,a.engine_version,a.game_id,a.rom_sha256,a.profiles)
        unchanged=False;preview=None
        if a.action == 'install': s.install(a.value)
        elif a.action=='configure':unchanged=not s.configure(a.value)
        elif a.action=='profile-inspect':preview=s.inspect_profile(a.value)
        elif a.action=='profile-import':s.import_profile(a.value)
        elif a.action=='profile-export':s.export_profile(a.value)
        elif a.action.startswith('profile-'):unchanged=not s.profile(a.action,a.value)
        elif a.action=='check':
            records={m['id']:m for m in s.installed()}
            if a.value not in records: raise ValueError('Mod not installed')
            s.validate_dependencies(records,{k for k,m in records.items() if m['enabled']});s.compatible(records[a.value]);s.replacements(records[a.value]);s.images(records[a.value])
        elif a.action not in ('list','resources','assets'): s.change(a.action, a.value, a.error)
        print('OK')
        if unchanged:print('UNCHANGED')
        if preview:print('PREVIEW\t'+'\t'.join(v.encode().hex() for v in (preview[0],str(len(preview[1]['enabled']))+' enabled mods; review Details before Apply',preview[2])))
        if a.action=='assets':
            records={m['id']:m for m in s.installed()}
            if a.value not in records:raise ValueError('Mod not installed')
            for name,w,h,rgba in s.images(records[a.value]):
                print('IMAGE\t'+'\t'.join(x.encode('utf-8').hex() for x in (name,str(w),str(h),rgba.hex())))
            return 0
        if a.action=='resources':
            owned=set()
            for m in s.installed():
                if not m['enabled']:continue
                for profile,name,data in s.replacements(m):
                    key=(profile['profile_id'],name)
                    if key in owned:raise ValueError('Conflicting active resources')
                    owned.add(key);r=profile['resources'][name]
                    fields=[m['id'],name,str(r['bank']),str(r['offset']),str(r['original_crc32']),data.hex()]
                    translation=resource_tools.read((s.packages/m['id']/m['translation']['file']).read_bytes())
                    prefix='RESOURCE'
                    if translation['schema']==2:
                        context=profile['visual_contexts'][name];packed=bytearray()
                        for index,char in enumerate(translation['texts'][name]):
                            if char in translation['glyphs']:
                                packed.append(index);packed.extend((s.packages/m['id']/translation['glyphs'][char]['file']).read_bytes())
                        fields.extend([str(context['chr_bank']),str(context['nametable']),str(context['mirroring']),packed.hex()]);prefix='COMPOSE'
                    print(prefix+'\t'+'\t'.join(x.encode('utf-8').hex() for x in fields))
            return 0
        mg=s.management(s.state())
        for name in ['Original']+sorted(mg['profiles']):
            profile=mg['profiles'].get(name,{'enabled':[]})
            print('PROFILE\t'+'\t'.join(v.encode().hex() for v in [name,str(int(name==mg['active'])),', '.join(profile['enabled']) or 'No mods']))
        for m in s.installed():
            fields = [m['id'], m['name'], m['version'], m['author'], m['description'], str(int(m['enabled'])), m['error'], str(m['api_version']), str(sum(PERMISSIONS[p] for p in m.get('permissions',[]))),str(int(m['entry'] is None)),category(m),', '.join(k+' '+r['min']+' to <'+r['max_exclusive'] for k,r in sorted(m.get('dependencies',{}).items())) or 'None',', '.join(m.get('conflicts',[])) or 'None',config_wire(m)]
            print('\t'.join(x.encode('utf-8').hex() for x in fields))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError, KeyError, TypeError, zlib.error) as e:
        print('ERROR: ' + str(e)); return 1

if __name__ == '__main__': raise SystemExit(main())
