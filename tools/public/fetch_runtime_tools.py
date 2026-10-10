"""Bundle pinned, verified Python/Zig for end-user imports; no game downloads."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import tempfile
import urllib.request
import zipfile

HERE=Path(__file__).resolve().parent

def fetch_tools(destination,system,cache):
    manifest=json.loads((HERE/'runtime_tools.json').read_text())
    config=manifest[system];destination.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    for name in ['python','zig']:
        item=config[name];archive=cache/item['filename']
        if not archive.is_file() or hashlib.sha256(archive.read_bytes()).hexdigest()!=item['sha256']:
            with urllib.request.urlopen(item['url']) as response: data=response.read()
            if hashlib.sha256(data).hexdigest()!=item['sha256']:raise ValueError('Runtime tool checksum mismatch: '+name)
            archive.write_bytes(data)
        output=destination/name
        if output.exists():raise ValueError('Runtime tool destination already exists: '+str(output))
        with tempfile.TemporaryDirectory(prefix='runtime-extract-',dir=cache) as temp:
            base=Path(temp).resolve()
            if zipfile.is_zipfile(archive):
                with zipfile.ZipFile(archive) as source:
                    for entry in source.infolist():
                        if not (base/entry.filename).resolve().is_relative_to(base):raise ValueError('Unsafe zip member')
                    source.extractall(base)
            else:
                with tarfile.open(archive) as source:
                    for entry in source.getmembers():
                        target=(base/entry.name).resolve()
                        if not target.is_relative_to(base):raise ValueError('Unsafe tar member')
                        if entry.issym() or entry.islnk():
                            link=(target.parent/entry.linkname if entry.issym() else base/entry.linkname).resolve()
                            if not link.is_relative_to(base):raise ValueError('Unsafe tar link')
                    # The importer never uses curses. Omit its optional terminfo
                    # database, whose case aliases collide on default macOS.
                    members=[e for e in source.getmembers() if '/share/terminfo/' not in e.name]
                    source.extractall(base,members=members)
            roots=list(base.iterdir())
            if len(roots)!=1 or not roots[0].is_dir():raise ValueError('Unexpected runtime archive root')
            shutil.copytree(roots[0],output,symlinks=True)
    (destination/'target.json').write_text(json.dumps(dict(target=config['target'],system=system,
        python_sha256=config['python']['sha256'],zig_sha256=config['zig']['sha256']),indent=2)+'\n')
    (destination/'runtime_tools.json').write_text(json.dumps(config,indent=2)+'\n')
    print('Verified runtime tools:',system,destination)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--destination',type=Path,required=True)
    p.add_argument('--system',choices=['linux-x86_64','windows-x86_64','macos-arm64','macos-x86_64'],required=True)
    p.add_argument('--cache',type=Path,required=True)
    args=p.parse_args();fetch_tools(args.destination,args.system,args.cache)
