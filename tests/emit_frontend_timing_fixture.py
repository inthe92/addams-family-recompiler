"""One authored native integration fixture; reuses existing emitter helpers."""
from pathlib import Path
import sys
from emit_irq_poll_fixtures import Program

def generate(output):
    p=Program('frontend_timing','synthetic frontend contract')
    p.op('SEI');p.op('LDA','imm',0x40);p.op('STA','abs',0x4017)
    for address,value in [(0x4015,1),(0x4000,0x3f),(0x4002,50),(0x4003,0)]:
        p.op('LDA','imm',value);p.op('STA','abs',address)
    p.label('loop')
    for address,value in [(0x2000,0x80),(0x2001,0x18),(0x4016,1),(0x4016,0)]:
        p.op('LDA','imm',value);p.op('STA','abs',address)
    for address,target in [(0x4016,0x20),(0x2002,0x41),(0x2007,0x42)]:
        p.op('LDA','abs',address);p.op('STA','zp',target)
    p.op('INC','zp',0x40);p.op('LDA','imm',2);p.op('STA','abs',0x4014);p.op('JMP','abs',label='loop')
    p.org(0x9000);p.op('INC','zp',0x43);p.op('RTI')
    code=p.resolve();p.prg[0x7ffa:0x7ffc]=bytes([0,0x90]);output.mkdir(parents=True,exist_ok=True)
    (output/'frontend_timing_fixture.c').write_text(code)
    pairs=','.join('{%d,%d}'%(i,v) for i,v in enumerate(p.prg) if v)
    (output/'frontend_timing_fixture.hpp').write_text('#pragma once\n#include "recomp/runtime_services.h"\nextern "C" uint8_t game_bank0_frontend_timing(CPUState*,const RuntimeServices*,uint32_t*);\nstruct TimingByte {uint16_t offset;uint8_t value;};\nstatic const TimingByte timing_bytes[]={'+pairs+'};\n')
    print('Generated one authored native frontend timing fixture; no ROM')

if __name__=='__main__':generate(Path(sys.argv[1]))
