"""Synthetic Store -> Manager -> Cartridge contracts; no commercial data."""
from pathlib import Path
import json,subprocess,sys,tempfile,zipfile
from mod_resource_checks import fixture,manifest,ROOT
with tempfile.TemporaryDirectory(prefix='Composition ñ spaces ') as tmp:
    p=Path(tmp).resolve();profile,catalog=fixture(p)
    profile['profile_schema']=2
    profile['visual_contexts']={'label':{'backend':'nes-background-cell-v1','chr_bank':1,'nametable':0x20a0,'mirroring':0}}
    catalog.write_text(json.dumps({'catalog_schema':1,'profiles':[profile]}))
    for name,text in [('one','CÑA'),('two','AÑC')]:
        with zipfile.ZipFile(p/(name+'.zip'),'w') as z:
            z.writestr('mod.json',json.dumps(manifest('compose.'+name)))
            z.writestr('translations/es-ES.json',json.dumps({'schema':2,'profile_id':profile['profile_id'],'locale':'es-ES','texts':{'label':text},'glyphs':{'Ñ':{'file':'assets/enye.chr','fallback':'B'}}},ensure_ascii=False))
            z.writestr('assets/enye.chr',(ROOT/'examples/resources/chr-research/own-enye-8x8.chr').read_bytes())
    subprocess.run([sys.argv[1],str(p/'store'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(catalog),str(p/'one.zip'),str(p/'two.zip')],check=True)
