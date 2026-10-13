"""Run native manager transactions with bundled-style host tooling and own ZIPs."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None) as name:
    p=Path(name)
    subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_example.py'),str(p/'hello.zip')],check=True)
    with zipfile.ZipFile(p/'bad.zip','w') as z:
        z.writestr('mod.json',json.dumps(dict(id='test.bad',name='Bad test',version='1.0.0',api_version=1,description='Own failing test',author='Tests',entry='main.lua')))
        z.writestr('main.lua','function on_update(dt) while true do end end')
    with zipfile.ZipFile(p/'order.zip','w') as z:
        z.writestr('mod.json',json.dumps(dict(id='aaa.first',name='Order test',version='1.0.0',api_version=1,description='Own order test',author='Tests',entry='main.lua')))
        z.writestr('main.lua',"function on_update(dt) log.info('FIRST') end")
    source=(ROOT/'app/desktop.cpp').read_text()
    assert source.count('managed_mods.update(')==1
    body=source.split('if(simulate) {',1)[1].split('if(wav)wav->finish()',1)[0]
    assert body.index('managed_mods.update(')<body.index('god.begin_frame()')<body.index('game_execute_slice(')
    paused=source.split('if(ui.paused()) {',1)[1].split('const bool simulate=',1)[0]
    assert 'managed_mods.update' not in paused
    assert 'if(no_mods)mod_path.clear()' in source
    subprocess.run([sys.argv[1],str(p/'native store & spaces'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(p/'hello.zip'),str(p/'bad.zip'),str(p/'order.zip')],check=True)
