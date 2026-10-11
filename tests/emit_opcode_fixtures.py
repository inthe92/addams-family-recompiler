"""Generate isolated native instruction fixtures, used only by compiler tests."""
import argparse
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools/recompiler"))
from emit_blocks import emit_bank
from opcodes_6502 import OPCODES, SIZES

def emit(output):
    instructions=[]
    for opcode,(name,mode) in sorted(OPCODES.items()):
        value=0xa5 if mode=='imm' else 0xf8 if mode in ['zp','zpx','zpy','izx','izy'] else 0x30ff if mode=='ind' else 0x7f if mode=='rel' else 0x2fff if mode in ['abs','abx','aby'] else 0
        size=SIZES[mode];raw=bytes([opcode])+value.to_bytes(size-1,'little') if size>1 else bytes([opcode])
        instructions.append(dict(pc=0x8000+opcode*16,opcode=opcode,name=name,mode=mode,size=size,value=value,raw=raw.hex(),tier='synthetic_ISA_fixture',legacy=True))
    branches=[(op,n) for op,(n,m) in sorted(OPCODES.items()) if m=='rel']
    for index,(opcode,name) in enumerate(branches):
        for base,value in [(0x90f0,0x7f),(0xa010,0x80)]:
            instructions.append(dict(pc=base+index*256,opcode=opcode,name=name,mode='rel',size=2,value=value,raw=bytes([opcode,value]).hex(),tier='synthetic_ISA_fixture',legacy=True))
    # Explicit boundary encodings protect zero-page pointer high-byte wrap,
    # 16-bit effective-address wrap, JMP's NMOS bug, and relative PC wrap.
    extra=[(0xB000,0xB1,0xFF),(0xB010,0x91,0xFF),
           (0xB020,0xBD,0xFFFF),(0xB030,0xB9,0xFFFF),
           (0xB040,0x9D,0xFFFF),(0xB050,0xFE,0xFFFF),
           (0xB060,0x6C,0xFFFF),(0xFFFD,0xD0,0x7F),(0x0000,0xF0,0x80)]
    for pc,opcode,value in extra:
        name,mode=OPCODES[opcode];size=SIZES[mode]
        raw=bytes([opcode])+value.to_bytes(size-1,'little')
        instructions.append(dict(pc=pc,opcode=opcode,name=name,mode=mode,size=size,
            value=value,raw=raw.hex(),tier='synthetic_boundary_fixture',legacy=False))
    instructions.sort(key=lambda i:i['pc']);code,_=emit_bank(instructions,0,'fixture');output.mkdir(parents=True,exist_ok=True)
    (output/'opcode_fixtures.c').write_text(code)
    header='#pragma once\n#include <stdint.h>\nstruct OpcodeFixture { uint16_t pc; uint8_t opcode; uint8_t size; uint16_t operand; const char* name; const char* mode; bool legacy; };\nstatic const OpcodeFixture opcode_fixtures[] = {\n'
    header+='\n'.join(f'  {{0x{i["pc"]:04X},0x{i["opcode"]:02X},{i["size"]},0x{i["value"]:04X},"{i["name"]}","{i["mode"]}",{str(i["legacy"]).lower()}}},' for i in instructions)
    (output/'opcode_fixtures.hpp').write_text(header+'\n};\n')
    timing=dict(pc=0x8000,opcode=0xAD,name='LDA',mode='abs',size=3,value=0x2002,
                raw='ad0220',tier='synthetic_PPUSTATUS_fixture')
    code,_=emit_bank([timing],0,'timing')
    (output/'timing_fixture.c').write_text(code)
    program=[]
    # Authored test programs: JSR/PHA/PLA/RTS and BRK/RTI across SP=$00.
    for pc,opcode,value in [(0x8000,0x20,0x8010),(0x8010,0x48,0),
                            (0x8011,0x68,0),(0x8012,0x60,0),
                            (0x8100,0x00,0),(0x8300,0x40,0)]:
        name,mode=OPCODES[opcode];size=SIZES[mode]
        raw=bytes([opcode])+value.to_bytes(size-1,'little')
        program.append(dict(pc=pc,opcode=opcode,name=name,mode=mode,size=size,
            value=value,raw=raw.hex(),tier='synthetic_stack_program'))
    code,_=emit_bank(program,0,'program')
    (output/'program_fixture.c').write_text(code)
    effects=[]
    # Authored register-access programs, independent of any game ROM.
    encodings=[(0xAD,0x2002),(0xBD,0x2001),(0xBD,0x20FF),(0xB9,0x20FF),
        (0xA1,0xF8),(0xB1,0xFF),(0x2C,0x2002),(0xEE,0x2002),
        (0xFE,0x20FF),(0x9D,0x20FF),(0xAD,0x2007),(0xBD,0x2006),
        (0xBD,0x20FF),(0xB1,0xFF)]
    for index,(opcode,value) in enumerate(encodings):
        name,mode=OPCODES[opcode];size=SIZES[mode]
        raw=bytes([opcode])+value.to_bytes(size-1,'little')
        effects.append(dict(pc=0xB100+index*16,opcode=opcode,name=name,mode=mode,
            size=size,value=value,raw=raw.hex(),tier='synthetic_bus_effects'))
    code,_=emit_bank(effects,0,'effects')
    (output/'bus_timing_fixture.c').write_text(code)
    print(f'Generated {len(instructions)} CPU fixtures and one PPUSTATUS fixture; no ROM input')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args();emit(a.output)
