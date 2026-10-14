"""Exercise the extracted package's mod helper/Python using authored data only."""
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile


def verify_mod_distribution(resources, python, working):
    resources, working = Path(resources), Path(working)
    archive = resources / 'examples/mods/Hello-NES-Native-Lab.zip'
    helper = resources / 'tools/mods/packages.py'
    source = Path(__file__).resolve().parents[1] / 'examples/mods/hello'
    with zipfile.ZipFile(archive) as z:
        if sorted(z.namelist()) != ['main.lua', 'mod.json'] or z.testzip() is not None:
            raise ValueError('Invalid authored example ZIP')
        for name in ('main.lua', 'mod.json'):
            if z.read(name) != (source / name).read_bytes():
                raise ValueError('Packaged example differs from reviewed source')
    hello_sha256=hashlib.sha256(archive.read_bytes()).hexdigest()
    storage = working / 'mod store & spaces'
    if storage.exists():
        raise ValueError('Mod package probe requires new private storage')
    commands = []

    def action(name, value=None):
        argv = [str(python), '-I', '-B', str(helper), '--root', str(storage),
                '--engine-version','0.9.0','--game-id','addams-family-pal-eu-enfrde',
                '--rom-sha256','6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880',name]
        if value is not None:
            argv.append(str(value))
        result = subprocess.run(argv, cwd=working, capture_output=True, text=True,
                                encoding='utf-8', errors='replace', timeout=30)
        commands.append(dict(action=name, returncode=result.returncode,
                             stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        if not result.stdout.splitlines() or result.stdout.splitlines()[0] != 'OK':
            raise ValueError('Packaged mod helper did not confirm transaction')
        if name == 'resources':
            rows = [line.split('\t')[1:] for line in result.stdout.splitlines()[1:]
                    if line.startswith('RESOURCE\t')]
            if any(len(row) != 6 for row in rows):
                raise ValueError('Malformed packaged resource transport')
            return [[bytes.fromhex(field).decode('utf-8') for field in row] for row in rows]
        return json.loads((storage / 'state.json').read_text()) if (storage / 'state.json').exists() else None

    ident = 'example.hello-native-lab'
    action('list')
    if list((storage / 'packages').iterdir()):
        raise ValueError('New storage not empty')
    if action('install', archive)['enabled']:
        raise ValueError('Installed example enabled automatically')
    if action('enable', ident)['enabled'] != [ident]:
        raise ValueError('Enable did not persist')
    if action('list')['enabled'] != [ident]:
        raise ValueError('State lost in a new helper process')
    if action('disable', ident)['enabled']:
        raise ValueError('Disable did not persist')
    if action('uninstall', ident)['enabled'] or list((storage / 'packages').iterdir()):
        raise ValueError('Uninstall left an installed example')
    hud_archive=resources/'examples/mods/Gomez-Debug-HUD-v1.0.zip'
    authored=Path(__file__).resolve().parents[1]/'examples/mods/gomez-debug-hud'
    with zipfile.ZipFile(hud_archive) as z:
        if sorted(z.namelist())!=['assets/README.txt','main.lua','mod.json'] or z.testzip() is not None:
            raise ValueError('Invalid authored HUD ZIP')
        for name in ('main.lua','mod.json','README.txt'):
            if z.read('assets/README.txt' if name=='README.txt' else name)!=(authored/name).read_bytes():
                raise ValueError('HUD package differs from reviewed source')
    hud_id='nes-native-lab.gomez-debug-hud'
    if action('install',hud_archive)['enabled']: raise ValueError('HUD automatically enabled')
    if action('enable',hud_id)['enabled']!=[hud_id]: raise ValueError('HUD enable failed')
    action('check',hud_id)
    if action('disable',hud_id)['enabled']: raise ValueError('HUD disable failed')
    action('uninstall',hud_id)
    sdk_hashes={}
    for folder,filename,mod_id in [('ram-change-logger','RAM-Change-Logger-v1.0.zip','sdk.ram-change-logger'),('ram-change-counter','RAM-Change-Counter-v1.0.zip','sdk.ram-change-counter')]:
        archive=resources/'examples/mods'/filename
        authored=Path(__file__).resolve().parents[1]/'examples/mods'/folder
        with zipfile.ZipFile(archive) as z:
            if sorted(z.namelist())!=['assets/README.txt','main.lua','mod.json'] or z.testzip() is not None: raise ValueError('Invalid SDK example ZIP')
            for name in ('main.lua','mod.json','README.txt'):
                if z.read('assets/README.txt' if name=='README.txt' else name)!=(authored/name).read_bytes(): raise ValueError('SDK source differs')
        if action('install',archive)['enabled']: raise ValueError('SDK auto enabled')
        if action('enable',mod_id)['enabled']!=[mod_id]: raise ValueError('SDK enable failed')
        action('check',mod_id)
        if action('disable',mod_id)['enabled']: raise ValueError('SDK disable failed')
        action('uninstall',mod_id)
        sdk_hashes[filename]=hashlib.sha256(archive.read_bytes()).hexdigest()
    translation_hashes={}
    for folder,filename,mod_id in [('addams-spanish-inventory','Addams-Spanish-Inventory-v1.0.zip','translation.addams-spanish-inventory'),('addams-spanish-equipment','Addams-Spanish-Equipment-v1.0.zip','translation.addams-spanish-equipment'),('addams-spanish-multi','Addams-Spanish-Multi-v1.0.zip','translation.addams-spanish-multi')]:
        archive=resources/'examples/mods'/filename
        authored=Path(__file__).resolve().parents[1]/'examples/mods'/folder
        with zipfile.ZipFile(archive) as z:
            if sorted(z.namelist())!=['assets/README.txt','mod.json','translations/es-ES.json'] or z.testzip() is not None: raise ValueError('Invalid data-only ZIP')
            for name in ('mod.json','translations/es-ES.json','README.txt'):
                if z.read('assets/README.txt' if name=='README.txt' else name)!=(authored/name).read_bytes():raise ValueError('Translation differs from authored source')
        if action('install',archive)['enabled']:raise ValueError('Translation automatically enabled')
        if action('enable',mod_id)['enabled']!=[mod_id]:raise ValueError('Translation enable failed')
        action('check',mod_id)
        expected = set(json.loads((authored/'translations/es-ES.json').read_text())['texts'])
        rows = action('resources')
        if {row[1] for row in rows} != expected or any(row[0] != mod_id for row in rows):
            raise ValueError('Packaged translation lost a field or owner')
        if action('list')['enabled'] != [mod_id]:raise ValueError('Translation preference lost across helper processes')
        if action('disable',mod_id)['enabled']:raise ValueError('Translation disable failed')
        if action('resources'):raise ValueError('Disabled translation still owns resources')
        action('uninstall',mod_id);translation_hashes[filename]=hashlib.sha256(archive.read_bytes()).hexdigest()
    completion_hashes={}
    definitions=[('Super-Gomez-Gameplay-v1.0.zip','phase5.super-gomez-gameplay','d1114dbd01484f782455d8f5b218cd01a13546164a2fbd04bd190329bb35f3e4'),('Speedrun-HUD-v1.0.zip','phase5.speedrun-hud','ef3ff4587ae92dc15c11433b3f1bebc5d3acc0f78cddb9740302dc81c455397d'),('Mansion-Parallax-Demo-v1.0.zip','phase5.mansion-parallax-demo','88ebf414b1cfdc8d94d7dfc7a4985e409f65a924b48291b882622712e7428571'),('Configurable-Jump-Example-v1.0.zip','nes-native-lab.configurable-jump','7e4bdc048493bd1bd6b63abe322fa6340465cf344ff23833fc7df7c4e55e783b')]
    for filename,ident,expected in definitions:
        archive=resources/'examples/mods'/filename
        if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected:raise ValueError('Completion example changed: '+filename)
        if action('install',archive)['enabled']:raise ValueError('Completion mod enabled during installation')
        action('enable',ident);action('check',ident)
        if ident=='phase5.mansion-parallax-demo':action('assets',ident)
        if ident=='nes-native-lab.configurable-jump':
            action('configure',json.dumps(dict(id=ident,values=dict(extension=9))))
            action('profile-save','Packaged Profile')
            exported=working/'exported-mod-profile.json'
            action('profile-export',json.dumps(dict(name='Packaged Profile',file=str(exported))))
            sha=hashlib.sha256(exported.read_bytes()).hexdigest();portable=json.loads(exported.read_text())
            if portable['schema']!=1 or str(storage) in exported.read_text():raise ValueError('Private or malformed profile export')
            action('profile-delete','Packaged Profile');action('profile-apply','Original')
            action('profile-inspect',exported);action('profile-import',json.dumps(dict(file=str(exported),sha256=sha)))
            if action('list')['enabled']:raise ValueError('Import activated packaged profile without confirmation')
            action('profile-apply','Packaged Profile')
            if action('list')['enabled']!=[ident]:raise ValueError('Packaged profile did not restore owner')
            action('configure',json.dumps(dict(id=ident,values={})));action('profile-apply','Original');action('profile-delete','Packaged Profile')
        action('disable',ident);action('uninstall',ident);completion_hashes[filename]=expected
    return dict(status='PASS', transactions=commands,
                completion_sha256=completion_hashes,
                translation_sha256=translation_hashes,
                sdk_sha256=sdk_hashes,
                hud_sha256=hashlib.sha256(hud_archive.read_bytes()).hexdigest(),
                example_sha256=hello_sha256,
                scope='bundled Python/helper ZIP transactions; Lua/native execution tested separately')
