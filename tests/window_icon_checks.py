"""Compare the actual compiled SDL surface to the source PNG, no game/ROM."""
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/public'))
from icon_assets import png_rgba
with tempfile.TemporaryDirectory(prefix='official icon pixels ') as name:
    pixels=Path(name)/'surface.rgba'
    subprocess.run([sys.argv[1],str(pixels)],check=True)
    w,h,expected=png_rgba((ROOT/'assets/icons/nes-native-lab-64.png').read_bytes())
    assert w==h==64 and pixels.read_bytes()==expected, 'SDL pixels differ from official PNG'
print('Embedded SDL pixels match official RGBA including alpha: PASS')
