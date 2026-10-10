"""Inspect an unpacked frontend release without treating generic tool binaries as game modules."""
import argparse
import hashlib
import json
from pathlib import Path
import re


def audit(root,reference_rom=None):
    markers=[]
    if reference_rom:
        image=reference_rom.read_bytes()
        markers=[image[n:n+64] for n in range(16,len(image)-64,1024)]
        markers=[m for m in markers if len(set(m))>8]
    pattern=re.compile(b'|'.join(map(re.escape,markers))) if markers else None
    files=0;scanned=0
    for p in root.rglob('*'):
        if not p.is_file():continue
        files+=1;rel=p.relative_to(root)
        if 'toolchain' in rel.parts:continue  # verified upstream distributions
        if p.name.startswith('user_game.') or p.suffix.lower() in {'.nes','.rom','.asm','.snapshot','.sav','.n2s','.n2l'} or re.fullmatch(r'bank[0-7]\.c',p.name):
            raise ValueError('Private game file in package: '+str(rel))
        if any(x in rel.parts for x in ['user-import','generated','recovery','evidence']):raise ValueError('Private cache in package: '+str(rel))
        data=p.read_bytes();scanned+=len(data)
        if pattern and pattern.search(data):raise ValueError('ROM byte marker in release: '+str(rel))
    return dict(files=files,owned_bytes_scanned=scanned,game_modules_included=False,rom_included=False,reference_byte_scan=bool(pattern))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('--reference-rom',type=Path);p.add_argument('--output',type=Path)
    args=p.parse_args();result=audit(args.root,args.reference_rom)
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Release audit OK:',json.dumps(result))
