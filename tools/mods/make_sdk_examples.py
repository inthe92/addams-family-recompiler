"""Build two own-text ZIP mods using the same SDK; no game content."""
import sys,zipfile
from pathlib import Path
base=Path(__file__).resolve().parents[2]/'examples/mods'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
for folder,name in [('ram-change-logger','RAM-Change-Logger-v1.0.zip'),('ram-change-counter','RAM-Change-Counter-v1.0.zip')]:
    with zipfile.ZipFile(out/name,'w',zipfile.ZIP_DEFLATED) as z:
        for file in ('mod.json','main.lua','README.txt'):
            i=zipfile.ZipInfo('assets/README.txt' if file=='README.txt' else file,(2026,10,13,0,0,0));i.compress_type=zipfile.ZIP_DEFLATED;i.external_attr=0o100644<<16;z.writestr(i,(base/folder/file).read_bytes())
