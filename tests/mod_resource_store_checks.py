from pathlib import Path
import sys,tempfile,subprocess,zipfile,json
from mod_resource_checks import fixture,manifest,translation,ROOT
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp).resolve();_,catalog=fixture(p)
    for name,text in [('one','CAB'),('two','BAC')]:
        with zipfile.ZipFile(p/(name+'.zip'),'w') as z:
            z.writestr('mod.json',json.dumps(manifest('translate.'+name)));z.writestr('translations/es-ES.json',json.dumps(translation(text)))
    subprocess.run([sys.argv[1],str(p/'store & spaces'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(catalog),str(p/'one.zip'),str(p/'two.zip')],check=True)
