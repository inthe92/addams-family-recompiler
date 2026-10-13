"""Create an authored, content-free API v1 example mod ZIP."""
from pathlib import Path
import sys
import zipfile
source = Path(__file__).resolve().parents[2] / 'examples/mods/hello'
destination = Path(sys.argv[1]); destination.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as z:
    for name in ('mod.json', 'main.lua'): z.write(source / name, name)
