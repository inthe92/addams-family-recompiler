"""Verify a bundled native compiler with synthetic C; never needs a ROM."""
import argparse
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
p=argparse.ArgumentParser();p.add_argument('resources',type=Path);a=p.parse_args()
sys.path.insert(0,str(a.resources/'tools/recompiler'))
from import_user_rom import native_compiler, module_abi_version
from emit_blocks import decode, emit_bank
from opcodes_6502 import OPCODES, SIZES
assert module_abi_version(a.resources)==3, 'Package must provide ABI v3 headers'
# Own BNE +0 bytes. A taken three-cycle branch must retain its early poll.
prg=bytes([0xd0,0])+bytes(16382)
instruction=decode(prg,dict(pc=0x8000,size=2,tier='synthetic'),0,0x8000,OPCODES,SIZES)
code,_=emit_bank([instruction],0,'package_smoke')
code+='''
int public_toolchain_contract(void){return 42;}
uint32_t nes_recomp_abi_version(void){return NES_RECOMP_ABI_VERSION;}
struct Trace {unsigned stage, errors, polls;};
static uint8_t bank(void* p,uint16_t pc){(void)p;(void)pc;return 0;}
static void advance(void* p,uint8_t total){struct Trace* t=p;
    if(t->stage!=0 || total!=3)++t->errors;t->stage=1;}
static void poll(void* p,uint8_t total,uint8_t offsets){struct Trace* t=p;
    if(t->stage!=1 || total!=3 || offsets!=2)++t->errors;
    t->stage=2;++t->polls;}
static uint8_t boundary(void* p,CPUState* c,uint8_t mask){struct Trace* t=p;
    if(t->stage!=2 || c->PC!=0x8002 || mask!=CPU_I)++t->errors;
    t->stage=3;return 1;}
int public_abi_poll_contract(void){
    struct Trace trace={0,0,0};CPUState cpu=cpu_initial_state(0x8000);
    uint32_t budget=1;
    RuntimeServices services={.context=&trace,.bank_for_pc=bank,
        .advance=advance,.boundary=boundary,.poll_irq=poll};
    uint8_t result=game_bank0_package_smoke(&cpu,&services,&budget);
    return result==1 && budget==0 && cpu.PC==0x8002 && cpu.SP==0xfd
        && cpu.P==(CPU_I|CPU_U) && cpu.A==0 && cpu.X==0 && cpu.Y==0
        && trace.stage==3 && trace.polls==1 && trace.errors==0;
}
'''
with tempfile.TemporaryDirectory(prefix='synthetic-native-') as name:
    source=Path(name)/'fixture.c';source.write_text(code)
    output=Path(name)/('fixture.dll' if platform.system()=='Windows' else 'fixture.dylib' if platform.system()=='Darwin' else 'fixture.so')
    mode='-dynamiclib' if platform.system()=='Darwin' else '-shared'
    exports=['-Wl,--export-all-symbols'] if platform.system()=='Windows' else []
    # Match import_user_rom.compile_file: optimized, freestanding C module.
    subprocess.run([*native_compiler(a.resources),mode,'-std=c99','-O1','-fPIC',*exports,
        '-I',str(a.resources/'include'),'-I',str(a.resources/'game/include'),
        str(source),'-o',str(output)],check=True)
    if platform.system()=='Darwin':subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(output)],check=True)
    # A separate process releases the DLL before Windows removes the fixture.
    subprocess.run([sys.executable,'-c',
        'import ctypes,sys; library=ctypes.CDLL(sys.argv[1]); '
        'assert library.public_toolchain_contract()==42; '
        'assert library.nes_recomp_abi_version()==3; '
        'assert library.public_abi_poll_contract()==1',
        str(output)],check=True)
print('Bundled Python + native compiler contract: OK')
print('Packaged recompiler + ABI v3 + ordered branch poll execution: OK')
