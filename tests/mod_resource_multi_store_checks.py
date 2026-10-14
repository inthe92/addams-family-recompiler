"""Run the native Manager with two fields and Unicode/space host paths."""
from pathlib import Path
import json, subprocess, sys, tempfile, zipfile
import mod_resource_checks as base
with tempfile.TemporaryDirectory(prefix='SDK ñ Ω 東京 ') as tmp:
    root=Path(tmp).resolve();profile,catalog=base.fixture(root);profile['resources']['second']=dict(profile['resources']['label'],offset=2064)
    catalog.write_text(json.dumps({'catalog_schema':1,'profiles':[profile]}))
    for ident,texts in [('translate.multi',{'label':'CAB','second':'BAC'}),('translate.single',{'label':'ABC'})]:
        t=base.translation();t['texts']=texts
        with zipfile.ZipFile(root/(ident+'.zip'),'w') as z:
            z.writestr('mod.json',json.dumps(base.manifest(ident)));z.writestr('translations/es-ES.json',json.dumps(t))
    subprocess.run([sys.argv[1],str(root/'store ñ Ω'),sys.executable,str(base.ROOT/'tools/mods/packages.py'),str(catalog),str(root/'translate.multi.zip'),str(root/'translate.single.zip')],check=True)
