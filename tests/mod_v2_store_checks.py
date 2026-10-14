"""Install actual authored HUD ZIP through the native manager; no game ROM."""
from pathlib import Path
import sys,tempfile,subprocess
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None) as tmp:
    tmp=Path(tmp)
    subprocess.run([sys.executable,'-B',str(root/'tools/mods/make_debug_hud.py'),str(tmp/'hud.zip')],check=True)
    subprocess.run([sys.argv[1],str(tmp/'store & spaces'),sys.executable,str(root/'tools/mods/packages.py'),str(tmp/'hud.zip')],check=True)
