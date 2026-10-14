from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None) as t:
 p=Path(t);subprocess.run([sys.executable,'-B',str(root/'tools/mods/make_sdk_examples.py'),str(p/'zip')],check=True)
 subprocess.run([sys.argv[1],str(root/'examples/mods/ram-change-logger/main.lua'),str(root/'examples/mods/ram-change-counter/main.lua'),str(p/'mod store & spaces'),sys.executable,str(root/'tools/mods/packages.py'),str(p/'zip')],check=True)
