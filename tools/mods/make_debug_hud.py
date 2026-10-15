"""Package authored API v2 text only; no ROM or private data."""
from pathlib import Path
import sys,zipfile
root=Path(__file__).resolve().parents[2]/'examples/mods/gomez-debug-hud'
output=Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
    for name in ('mod.json','main.lua','README.txt'):
        info=zipfile.ZipInfo('assets/README.txt' if name=='README.txt' else name,date_time=(2026,10,13,0,0,0))
        info.compress_type=zipfile.ZIP_DEFLATED;info.create_system=3;info.external_attr=0o100644<<16;z.writestr(info,(root/name).read_bytes())
