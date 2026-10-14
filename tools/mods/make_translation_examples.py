"""Data-only authored Spanish labels; no source ROM/resource bytes or Lua."""
from pathlib import Path
import sys,zipfile
root=Path(__file__).resolve().parents[2]/'examples/mods';out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
for folder,name in [('addams-spanish-inventory','Addams-Spanish-Inventory-v1.0.zip'),('addams-spanish-equipment','Addams-Spanish-Equipment-v1.0.zip'),('addams-spanish-multi','Addams-Spanish-Multi-v1.0.zip')]:
    with zipfile.ZipFile(out/name,'w',zipfile.ZIP_DEFLATED) as z:
        for file in ('mod.json','translations/es-ES.json','README.txt'):
            info=zipfile.ZipInfo('assets/README.txt' if file=='README.txt' else file,(2026,10,13,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,(root/folder/file).read_bytes())
