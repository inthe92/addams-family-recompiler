"""Own declarative configuration example; no ROM/assets or script execution."""
from pathlib import Path
import json,sys,zipfile
sys.path.insert(0,str(Path(__file__).parent))
import packages
source=Path(__file__).resolve().parents[2]/'examples/mods/configurable-jump'
packages.manifest(packages.read_json((source/'mod.json').read_bytes()))
output=Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED) as z:
 for name in ('mod.json','main.lua'):
  info=zipfile.ZipInfo(name,(2026,10,14,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,(source/name).read_bytes())
