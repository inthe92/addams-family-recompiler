"""Own three-glyph resource, read by the real emitter through timed PRG reads."""
from pathlib import Path
import sys
from emit_irq_poll_fixtures import Program
p=Program('resource','authored data resource')
p.op('SEI');p.op('LDA','imm',0x40);p.op('STA','abs',0x4017)
for a,v in [(0x4015,1),(0x4000,0x3f),(0x4002,50),(0x4003,0),(0x2000,0x80),(0x2001,0x0a)]:p.op('LDA','imm',v);p.op('STA','abs',a)
p.label('loop')
p.op('BIT','abs',0x2002);p.op('BPL','rel',label='loop')
for a,v in [(0x2000,0x80),(0x2001,0x0a)]:p.op('LDA','imm',v);p.op('STA','abs',a)
for a,v in [(0x2006,0x3f),(0x2006,0),(0x2007,0),(0x2007,0x30),(0x2006,0x20),(0x2006,0)]:p.op('LDA','imm',v);p.op('STA','abs',a)
for a in range(0x8800,0x8803):p.op('LDA','abs',a);p.op('STA','abs',0x2007)
p.op('LDA','imm',0);p.op('STA','abs',0x2005);p.op('STA','abs',0x2005)
p.op('STA','abs',0x4016);p.op('LDA','abs',0x4016);p.op('STA','zp',0x20)
p.op('INC','zp',0x40);p.op('LDA','imm',2);p.op('STA','abs',0x4014);p.op('JMP','abs',label='loop')
p.org(0x9000);p.op('INC','zp',0x43);p.op('RTI');code=p.resolve();p.prg[0x7ffa:0x7ffc]=bytes([0,0x90]);p.prg[0x800:0x803]=bytes([1,2,3])
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);(out/'resource_fixture.c').write_text(code)
pairs=','.join('{%d,%d}'%(i,v) for i,v in enumerate(p.prg) if v)
(out/'resource_fixture.hpp').write_text('#pragma once\n#include "recomp/runtime_services.h"\nextern "C" uint8_t game_bank0_resource(CPUState*,const RuntimeServices*,uint32_t*);\nstruct ResourceByte{uint16_t offset;uint8_t value;};static const ResourceByte resource_bytes[]={'+pairs+'};\n')
