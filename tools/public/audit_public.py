"""Reject private recovery/generated assets before publishing. No ROM required."""
import argparse
import hashlib
import json
from pathlib import Path
import re

FORBIDDEN_DIRS={'build','dist','outputs','evidence','recovery','runtime_ram','__pycache__','user-import','generated'}
FORBIDDEN_SUFFIXES={'.nes','.rom','.n2s','.n2l','.snapshot','.ppm','.wav','.sav','.dylib','.dll','.so','.o','.a'}

def audit(root,reference_rom=None):
    root=root.resolve();errors=[];files=[]
    reference=reference_rom.read_bytes() if reference_rom else None
    # Match spaced raw resource blocks as well as complete input images in
    # binaries. This supplements the exact export allowlist; it is not a legal
    # assessment or proof that arbitrary source has no derivative content.
    markers=[]
    if reference:
        markers=[reference[offset:offset+64] for offset in range(16,len(reference)-64,1024)]
        markers=[m for m in markers if len(set(m))>8]
    for path in sorted(root.rglob('*')):
        if not path.is_file():continue
        rel=path.relative_to(root)
        dependency=rel.parts[0]=='third_party'
        if not dependency and any(part in FORBIDDEN_DIRS or part.startswith('build-') for part in rel.parts):errors.append(str(rel))
        if path.suffix.lower() in FORBIDDEN_SUFFIXES and not (dependency and path.suffix.lower()=='.wav'):errors.append(str(rel))
        if rel.parts[0]!='third_party':
            if path.suffix.lower()=='.asm' or re.fullmatch(r'bank[0-7]\.(c|asm)',path.name):errors.append(str(rel))
            if path.name in {'recovered_resources.c','world_maps.c','vertical_world_maps.c','manifest.json'}:errors.append(str(rel))
        data=path.read_bytes()
        if reference and (reference in data or any(marker in data for marker in markers)):
            errors.append('ROM byte content: '+str(rel))
        files.append(dict(path=rel.as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
    profile=json.loads((root/'tools/recompiler/compatible_profile.json').read_text())
    if set(profile)!={'schema','rom_sha256','rom_size','instruction_starts'}:errors.append('Profile includes unexpected fields')
    dependencies=json.loads((root/'third_party/manifest.json').read_text())['dependencies']
    expected_archives={}
    for item in dependencies:
        archive=root/'third_party'/(item['license_file'].split('/')[0]+'.tar.gz')
        expected_archives[archive]=item['sha256']
        if not archive.is_file() or hashlib.sha256(archive.read_bytes()).hexdigest()!=item['sha256']:
            errors.append('Dependency archive checksum: '+archive.name)
    for archive in root.rglob('*.tar.gz'):
        if archive not in expected_archives:errors.append('Unexpected source archive: '+str(archive.relative_to(root)))
    if errors:raise ValueError('Excluded/private content found:\n'+'\n'.join(sorted(set(errors))))
    return dict(schema=1,file_count=len(files),total_bytes=sum(f['bytes'] for f in files),
        recovery_assets_included=False,rom_included=False,reference_byte_scan=bool(reference),files=files)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('--reference-rom',type=Path);p.add_argument('--output',type=Path)
    args=p.parse_args();result=audit(args.root,args.reference_rom)
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Public export audit OK:',result['file_count'],'files;',result['total_bytes'],'bytes')
