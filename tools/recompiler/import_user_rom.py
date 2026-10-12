"""Build a native module locally from the user's compatible ROM. No downloads."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile
import sys

# Standalone Python builds may isolate sys.path; only our sibling tools are
# added, not the caller's current working directory.
sys.path.insert(0,str(Path(__file__).resolve().parent))

from emit_blocks import emit_image as emit_program
from emit_world_maps import emit_image as emit_maps
from opcodes_6502 import OPCODES, SIZES

HERE = Path(__file__).resolve().parent

def digest(data):
    return hashlib.sha256(data).hexdigest()

def native_compiler(headers):
    bundled=headers/'toolchain/zig'/('zig.exe' if os.name=='nt' else 'zig')
    target=headers/'toolchain/target.json'
    if bundled.is_file() and target.is_file():
        triple=json.loads(target.read_text())['target']
        # Modules call only the engine ABI; no Darwin system SDK or libc is
        # needed for the generated C. stdint comes from Zig's builtin headers.
        flags=['-ffreestanding','-nostdlib'] if triple.endswith('-macos') else []
        return [str(bundled),'cc','-target',triple,*flags]
    compiler = os.environ.get('NES_NATIVE_CC') or (shutil.which('gcc') if os.name=='nt' else shutil.which('clang')) or shutil.which('gcc')
    if not compiler:raise RuntimeError('No C compiler found. See the local import instructions.')
    return [compiler]

def validate_rom(path, profile):
    # Check size before allocating, then authenticate the whole image before
    # decoding, invoking a compiler, or replacing an existing local import.
    if path.stat().st_size != profile['rom_size']:
        raise ValueError('Incompatible ROM. This version requires the supported European PAL release (En/Fr/De).')
    image = path.read_bytes()
    if digest(image) != profile['rom_sha256']:
        raise ValueError('Incompatible ROM: SHA-256 mismatch. The ROM has not been modified or patched.')
    if image[:4] != b'NES\x1a' or image[4:6] != bytes([8,16]) or (image[6] >> 4 | image[7] & 0xf0) != 1:
        raise ValueError('Incompatible iNES/MMC1 header.')
    return image

def emit_resources(image, target):
    def values(data):
        return '\n'.join(','.join(str(b) for b in data[i:i+32])+',' for i in range(0,len(data),32))
    prg, chr_data = image[16:131088], image[131088:]
    text = '#include "game/recovered_resources.h"\nconst uint8_t game_prg_data[8][16384]={\n'
    text += '\n'.join('{'+values(prg[b*16384:(b+1)*16384])+'},' for b in range(8))
    text += '\n};\nconst uint8_t game_chr_data[131072]={'+values(chr_data)+'};\n'
    text += 'const uint16_t game_reset_vector='+str(int.from_bytes(prg[-4:-2],'little'))+';\n'
    text += 'const char game_source_sha256[65]="'+digest(image)+'";\n'
    target.write_text(text)

def module_abi_version(headers):
    text=(headers/'include/recomp/module_abi.h').read_text()
    match=re.search(r'^\s*#define\s+NES_RECOMP_ABI_VERSION\s+(\d+)u?\s*$',text,re.MULTILINE)
    if not match:raise ValueError('Native module ABI version missing from headers.')
    return int(match.group(1))

def cache_identity(headers, profile_bytes):
    include_paths = [headers/'include', headers/'game/include']
    fingerprint = hashlib.sha256(profile_bytes)
    for file in sorted(HERE.glob('*.py')):
        fingerprint.update(file.name.encode()); fingerprint.update(file.read_bytes())
    for directory in include_paths:
        for file in sorted(directory.rglob('*.h')):
            fingerprint.update(file.relative_to(headers).as_posix().encode()); fingerprint.update(file.read_bytes())
    fingerprint.update((platform.machine()+platform.system()).encode())
    target_file=headers/'toolchain/target.json'
    if target_file.is_file():fingerprint.update(target_file.read_bytes())
    return fingerprint.hexdigest()

def import_rom(rom, cache, include_root=None):
    profile_bytes = (HERE/'compatible_profile.json').read_bytes()
    profile = json.loads(profile_bytes)
    image = validate_rom(rom, profile)
    headers = include_root or HERE.parents[1]
    include_paths = [headers/'include', headers/'game/include']
    abi_version = module_abi_version(headers)
    identity = cache_identity(headers, profile_bytes)
    suffix = '.dll' if os.name == 'nt' else '.dylib' if platform.system() == 'Darwin' else '.so'
    cache.mkdir(parents=True, exist_ok=True)
    module = cache/('user_game'+suffix)
    receipt = cache/'import.json'
    if module.is_file() and receipt.is_file():
        prior = json.loads(receipt.read_text())
        if (prior.get('abi_version') == abi_version and prior.get('generator_sha256') == identity
                and prior.get('module_sha256') == digest(module.read_bytes())):
            print('Existing local game import verified.', flush=True)
            return module
    compiler=native_compiler(headers)
    # Only temporary generated files and the user's own cache contain ROM data.
    with tempfile.TemporaryDirectory(prefix='native-import-', dir=cache) as directory:
        work = Path(directory)
        prg = image[16:131088]
        banks = []
        for bank, starts in enumerate(profile['instruction_starts']):
            instructions = []
            for pc in starts:
                offset = bank*16384 + pc-(0xc000 if bank==7 else 0x8000)
                opcode = prg[offset]
                if opcode not in OPCODES: raise ValueError('Incompatible opcode in the supported instruction profile.')
                instructions.append(dict(pc=pc,size=SIZES[OPCODES[opcode][1]],tier='profile_location'))
            banks.append(dict(instruction_evidence=instructions))
        print('Preparing your game: generating native C from your ROM...',flush=True)
        emit_program(image,dict(banks=banks),work/'program',digest(profile_bytes))
        emit_maps(image,work/'world')
        emit_resources(image,work/'resources.c')
        sources = sorted((work/'program').glob('*.c'))+[work/'world/world_maps.c',work/'resources.c']
        includes = [arg for path in include_paths for arg in ('-I',str(path))]
        print('Compiling game code. This may take a few minutes...',flush=True)
        def compile_file(source):
            obj = work/(source.stem+'.o')
            subprocess.run([*compiler,'-std=c99','-O1','-fPIC',*includes,'-c',str(source),'-o',str(obj)],check=True,stdout=sys.stdout,stderr=sys.stderr)
            return obj
        with ThreadPoolExecutor(max_workers=min(4,os.cpu_count() or 1)) as pool:
            objects = list(pool.map(compile_file,sources))
        output = work/module.name
        mode = '-dynamiclib' if platform.system()=='Darwin' else '-shared'
        export = ['-Wl,--export-all-symbols'] if os.name=='nt' else []
        subprocess.run([*compiler,mode,*export,*map(str,objects),'-o',str(output)],check=True,stdout=sys.stdout,stderr=sys.stderr)
        if platform.system()=='Darwin':
            subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(output)],check=True,stdout=sys.stdout,stderr=sys.stderr)
        local_receipt = dict(schema=2,abi_version=abi_version,rom_sha256=digest(image),generator_sha256=identity,
                            module_sha256=digest(output.read_bytes()),machine=platform.machine(),
                            generated_locally=True)
        os.replace(output,module)
        # The receipt is committed last. Interrupted imports are regenerated.
        next_receipt = work/'import.json'
        next_receipt.write_text(json.dumps(local_receipt,indent=2)+'\n')
        os.replace(next_receipt,receipt)
    print('Local import complete: '+str(module),flush=True)
    return module

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--cache',type=Path,required=True)
    parser.add_argument('--include-root',type=Path)
    parser.add_argument('--validate-only',action='store_true')
    parser.add_argument('--log',type=Path)
    args=parser.parse_args()
    if args.log:
        args.log.parent.mkdir(parents=True,exist_ok=True)
        stream=args.log.open('w',buffering=1,encoding='utf-8')
        sys.stdout=stream;sys.stderr=stream
    try:
        if args.validate_only:
            validate_rom(args.rom,json.loads((HERE/'compatible_profile.json').read_text()))
            print('Compatible ROM verified; no game content was generated or copied.')
        else: import_rom(args.rom,args.cache,args.include_root)
    except (OSError,ValueError,RuntimeError,subprocess.CalledProcessError) as error:
        parser.exit(1,str(error)+'\n')

if __name__=='__main__':main()
