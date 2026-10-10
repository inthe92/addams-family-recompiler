"""Package the content-free frontend and importer, never the local game cache."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import shutil
import subprocess
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[2]

def copy_resources(destination,toolchain):
    for relative in ['tools/recompiler','include','game/include','docs']:
        shutil.copytree(ROOT/relative,destination/relative,ignore=shutil.ignore_patterns('__pycache__','*.pyc'),dirs_exist_ok=True)
    for name in ['README.md','README.es.md','LICENSE','THIRD_PARTY.md']:
        shutil.copyfile(ROOT/name,destination/name)
    shutil.copytree(toolchain,destination/'toolchain',symlinks=True)
    licenses=destination/'licenses';licenses.mkdir(exist_ok=True)
    for file in (ROOT/'third_party/licenses').iterdir():
        if file.is_file():shutil.copyfile(file,licenses/file.name)

def make_archive(folder,target):
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for file in sorted(folder.rglob('*')):
            if file.is_file():archive.write(file,folder.name+'/'+file.relative_to(folder).as_posix())

def package(build,output,toolchain,system=None,arch=None):
    system=system or platform.system()
    arch=arch or ('x86_64' if platform.machine().lower() in ['amd64','x86_64'] else 'arm64')
    target=json.loads((toolchain/'target.json').read_text())['system']
    expected={'Darwin':'macos','Windows':'windows','Linux':'linux'}[system]+'-'+arch
    if target!=expected:raise ValueError('Bundled runtime architecture mismatch')
    output.mkdir(parents=True,exist_ok=True)
    if system=='Darwin':
        with tempfile.TemporaryDirectory(prefix='nes-public-',dir='/private/tmp') as temp:
            app=Path(temp)/'NES Native Lab.app'
            subprocess.run(['/usr/bin/ditto','--norsrc','--noextattr','--noqtn','--noacl',str(build/'nes_native.app'),str(app)],check=True)
            copy_resources(app/'Contents/Resources',toolchain)
            info=app/'Contents/Info.plist';data=plistlib.loads(info.read_bytes())
            data.update(CFBundleName='NES Native Lab',CFBundleDisplayName='NES Native Lab',NSHighResolutionCapable=True)
            info.write_bytes(plistlib.dumps(data))
            subprocess.run(['/usr/bin/xattr','-cr',str(app)],check=True)
            # Sign nested Python libraries and compiler executables before the app.
            for binary in sorted(app.rglob('*'),key=lambda p:len(p.parts),reverse=True):
                if not binary.is_file() or binary.is_symlink():continue
                with binary.open('rb') as handle:head=handle.read(16)
                native=(head[:4] in [b'\xcf\xfa\xed\xfe',b'\xce\xfa\xed\xfe'] and int.from_bytes(head[12:16],'little') in [2,6,8])
                fat=head[:4] in [b'\xca\xfe\xba\xbe',b'\xbe\xba\xfe\xca']
                if native or fat:subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(binary)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(app)],check=True)
            subprocess.run(['/usr/bin/codesign','--verify','--deep','--strict',str(app)],check=True)
            target=output/('NES-Native-Lab-macOS-'+arch+'.zip')
            subprocess.run(['/usr/bin/ditto','-c','-k','--norsrc','--keepParent',str(app),str(target)],check=True)
            check=Path(temp)/'verify';check.mkdir()
            subprocess.run(['/usr/bin/ditto','-x','-k',str(target),str(check)],check=True)
            subprocess.run(['/usr/bin/codesign','--verify','--deep','--strict',str(check/app.name)],check=True)
            subprocess.run(['/usr/bin/ditto','--norsrc','--noextattr','--noqtn','--noacl',str(app),str(output/app.name)],check=True)
    elif system=='Windows':
        folder=output/'NES-Native-Lab-Windows-x86_64';folder.mkdir(exist_ok=True)
        shutil.copyfile(build/'nes_native.exe',folder/'nes_native.exe')
        copy_resources(folder,toolchain)
        target=output/(folder.name+'.zip');make_archive(folder,target)
    else:
        # ELF dependencies and AppImage runtime are handled by linuxdeploy.
        folder=output/'NES-Native-Lab.AppDir';(folder/'usr/bin').mkdir(parents=True,exist_ok=True)
        shutil.copyfile(build/'nes_native',folder/'usr/bin/nes_native')
        (folder/'usr/bin/nes_native').chmod(0o755)
        copy_resources(folder/'usr/bin',toolchain)
        shutil.copyfile(ROOT/'assets/nes-native-lab.svg',folder/'nes-native-lab.svg')
        (folder/'nes-native-lab.desktop').write_text('[Desktop Entry]\nType=Application\nName=NES Native Lab\nExec=nes_native\nIcon=nes-native-lab\nCategories=Game;\n')
        # Executable location, not the caller's current directory, owns tools.
        (folder/'AppRun').write_text('#!/bin/sh\nTASK_APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$TASK_APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$TASK_APPDIR/usr/bin/nes_native" "$@"\n')
        (folder/'AppRun').chmod(0o755);target=folder
    print(target)
    return target

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,default=ROOT/'build');p.add_argument('--output',type=Path,default=ROOT/'dist')
    p.add_argument('--toolchain',type=Path,required=True);p.add_argument('--system',choices=['Darwin','Windows','Linux']);p.add_argument('--arch',choices=['arm64','x86_64'])
    args=p.parse_args();package(args.build.resolve(),args.output.resolve(),args.toolchain.resolve(),args.system,args.arch)
