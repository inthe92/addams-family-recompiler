"""Verify a bundled native compiler with synthetic C; never needs a ROM."""
import argparse
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
p=argparse.ArgumentParser();p.add_argument('resources',type=Path);a=p.parse_args()
sys.path.insert(0,str(a.resources/'tools/recompiler'))
from import_user_rom import native_compiler
with tempfile.TemporaryDirectory(prefix='synthetic-native-') as name:
    source=Path(name)/'fixture.c';source.write_text('int public_toolchain_contract(void){return 42;}\n')
    output=Path(name)/('fixture.dll' if platform.system()=='Windows' else 'fixture.dylib' if platform.system()=='Darwin' else 'fixture.so')
    mode='-dynamiclib' if platform.system()=='Darwin' else '-shared'
    exports=['-Wl,--export-all-symbols'] if platform.system()=='Windows' else []
    subprocess.run([*native_compiler(a.resources),mode,'-fPIC',*exports,str(source),'-o',str(output)],check=True)
    if platform.system()=='Darwin':subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(output)],check=True)
    # A separate process releases the DLL before Windows removes the fixture.
    subprocess.run([sys.executable,'-c',
        'import ctypes,sys; library=ctypes.CDLL(sys.argv[1]); assert library.public_toolchain_contract()==42',
        str(output)],check=True)
print('Bundled Python + native compiler contract: OK')
