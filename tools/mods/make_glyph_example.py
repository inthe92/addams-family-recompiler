"""Build deterministic authored data-only example; no ROM or private module."""
from pathlib import Path
import sys,zipfile
source=Path(__file__).resolve().parents[2]/'examples/mods/addams-enye-inventory'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(out/'Addams-Spanish-Enye-v1.0.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for name in ('mod.json','translations/es-ES.json','assets/enye.chr','assets/README.txt'):
        info=zipfile.ZipInfo(name,(2026,10,14,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.create_system=3;info.external_attr=0o100644<<16
        archive.writestr(info,(source/name).read_bytes())
