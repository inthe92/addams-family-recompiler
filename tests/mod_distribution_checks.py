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
    storage = working / 'mod store & spaces'
    if storage.exists():
        raise ValueError('Mod package probe requires new private storage')
    commands = []

    def action(name, value=None):
        argv = [str(python), '-I', '-B', str(helper), '--root', str(storage), name]
        if value is not None:
            argv.append(str(value))
        result = subprocess.run(argv, cwd=working, capture_output=True, text=True,
                                encoding='utf-8', errors='replace', timeout=30)
        commands.append(dict(action=name, returncode=result.returncode,
                             stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        if not result.stdout.splitlines() or result.stdout.splitlines()[0] != 'OK':
            raise ValueError('Packaged mod helper did not confirm transaction')
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
    return dict(status='PASS', transactions=commands,
                example_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),
                scope='bundled Python/helper ZIP transactions; Lua/native execution tested separately')
