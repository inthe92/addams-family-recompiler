"""Authored PAL/CPU IRQ regression programs; no ROM input or opcode interpreter.

PAL contracts: blargg PAL 07/08 at nes-test-roms commit
95d8f621ae55cee0d09b91519a8989ae0e64753b. CPU controls derive from
https://www.nesdev.org/wiki/Interrupts. See docs/CPU_IRQ_POLL_REGRESSIONS.md.
Only generated C executes instructions. Delays below use independent
6502 cycle arithmetic; the production decoder verifies the authored bytes.
"""
import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools/recompiler"))
import emit_blocks as E
import opcodes_6502 as O
reverse = {v: k for k, v in O.OPCODES.items()}

class Program:
 def __init__(self,name,kind): self.name=name;self.kind=kind;self.pc=0x8000;self.ins=[];self.labels={};self.fix=[];self.meta={}
 def label(self,name):self.labels[name]=self.pc;return self.pc
 def org(self,pc): self.pc=pc
 def op(self,name,mode='imp',value=0,label=None):
  opcode=reverse[name,mode];size=O.SIZES[mode];pc=self.pc;self.pc+=size
  self.ins.append(dict(pc=pc,name=name,mode=mode,size=size,value=value,opcode=opcode,tier='authored IRQ regression',raw=''))
  if label:self.fix.append((self.ins[-1],label))
  return pc
 def bne(self,label):return self.op('BNE','rel',label=label)
 def write(self,v,label=None):self.op('LDA','imm',v);pc=self.op('STA','abs',0x4017);self.meta[label or f'write_{pc:04x}']=pc;return pc
 def delay(self,n):
  original=n;begin=self.pc;pad=0
  if n>150:
   while self.pc&255>235:self.op('NOP');pad+=2;n-=2
   outer=(n-1)//131;assert 1<=outer<=255
   if n-(1+outer*131)==1:outer-=1
   self.op('LDY','imm',outer);top=self.label(f'outer{begin:x}');self.op('LDX','imm',25);inner=self.label(f'inner{begin:x}');self.op('DEX');self.bne(f'inner{begin:x}');self.op('DEY');self.bne(f'outer{begin:x}');n-=1+outer*131
  assert n!=1 and n>=0
  if n&1:self.op('BIT','zp',0xf0);n-=3
  for i in range(n//2):self.op('NOP')
  self.meta.setdefault('delays',[]).append({'pc':begin,'cycles':original,'padding_cycles':pad,'formula':'1+outer*(5*25+6), plus BIT zp=3 / NOP=2'})
 def preamble(self):self.op('SEI');self.op('LDA','zp',0x10);self.op('BNE','rel',0)
 def sync_call(self):self.op('JSR','abs',label='sync');self.write(0x40)
 def finish(self):self.meta['stop']=self.pc;self.op('NOP');self.org(0x9000);self.label('irq');self.op('STA','zp',0x21);self.op('BIT','abs',0x4015);self.op('RTI');self.meta['irq']=0x9000
 def sync_sub(self):
  self.org(0xa000);self.label('sync');self.op('SEI');self.write(0x40,'sync_inhibit');self.write(0,'sync_start');self.delay(33251);self.meta['sync_read']=self.op('LDA','abs',0x4015);self.op('AND','imm',0x40);self.op('BNE','rel',0);self.write(0x40,'sync_end');self.op('RTS')
 def resolve(self):
  for i,label in self.fix:
   target=self.labels[label];i['value']=(target-(i['pc']+2))&255 if i['mode']=='rel' else target
   if i['mode']=='rel':assert -128<=target-(i['pc']+2)<=127;assert (i['pc']+2)&0xff00==target&0xff00, 'No branch page-crossing allowed in counted loops'
  self.ins.sort(key=lambda i:i['pc']);prg=bytearray(32768);used=set()
  for i in self.ins:
   raw=bytes([i['opcode']])+i['value'].to_bytes(i['size']-1,'little');i['raw']=raw.hex()
   for j,b in enumerate(raw):a=i['pc']-0x8000+j;assert a not in used;used.add(a);prg[a]=b
   recovered={'pc':i['pc'],'size':i['size'],'tier':i['tier']};d=E.decode(prg,recovered,0,0x8000,O.OPCODES,O.SIZES)
   assert all(d[k]==i[k] for k in ('pc','name','mode','size','value','opcode','raw'))
  # Authored native IRQ vector; no commercial ROM.
  prg[0x7ffe]=0;prg[0x7fff]=0x90
  self.prg=prg;return E.emit_bank(self.ins,0,self.name)[0]

def generate(output):
    programs, cases = [], []

    def add(p, group, configs, contract):
        p.finish()
        p.sync_sub()
        pid = len(programs)
        programs.append(p)
        for seed, phase in configs:
            cases.append((pid, seed, phase, group, contract(phase)))

    # 07: W is the observed write, not the fresh frame-counter origin.
    for k, (rel, bits) in enumerate([(33254, [0]), (33255, [1]),
                                   (33256, [1, 1]), (33257, [1, 0])]):
        p = Program("poll_flag07_" + str(k), "flag07")
        p.preamble(); p.sync_call(); p.write(0, "origin"); p.delay(rel - 4)
        p.meta["reads"] = [p.op("LDA", "abs", 0x4015)]
        if len(bits) == 2:
            p.meta["reads"].append(p.op("LDA", "abs", 0x4015))
        add(p, "pal07", [(s, 0) for s in (0, 1)],
            lambda phase, rel=rel, bits=bits: dict(bits=bits, read_relative=rel,
                irq_count=0, total=rel+1+4*(len(bits)-1), final_a=bits[-1]*0x40,
                final_p=0x24 if bits[-1] else 0x26, final_x=0))

    # 08: exact instruction window; original nested loop takes 33247 cycles.
    p = Program("poll_irq08", "irq08")
    p.preamble(); p.op("LDA", "imm", 4); p.op("STA", "zp", 0x21)
    p.sync_call(); p.op("LDA", "imm", 2); p.op("CLI"); p.op("LDX", "imm", 0)
    p.meta["origin"] = p.op("STX", "abs", 0x4017)
    p.op("LDY", "imm", 254); p.op("LDX", "imm", 19); p.label("dly08")
    p.op("DEX"); p.bne("dly08"); p.op("LDX", "imm", 25); p.op("DEY"); p.bne("dly08")
    p.op("LDX", "zp", 0x20); p.op("BNE", "rel", 0); p.op("NOP")
    p.meta["critical"] = p.op("LDA", "imm", 0)
    p.meta["successor"] = p.pc
    p.op("LDA", "imm", 3); p.delay(40); p.op("SEI")
    add(p, "pal08", [(s, ph) for s in (0, 1) for ph in (0, 1)],
        lambda ph: dict(bits=[], irq_count=1, entry=33257+ph+7,
            total=33321+ph, entry_a=0, entry_p=0x22, final_a=3, final_p=0x24,
            final_x=ph, critical_cycles=2))

    # General CPU contracts, not additional PAL hardware measurements.
    for mode in ("clear", "no_clear", "masked"):
        p = Program("poll_held_" + mode, "held")
        p.preamble(); p.op("LDA", "imm", 0xa5); p.op("STA", "zp", 0x21)
        p.sync_call(); p.write(0, "origin"); p.delay(33260)
        p.op("NOP" if mode == "masked" else "CLI")
        p.meta["critical"] = p.op("LDA", "imm", 0x40) if mode == "no_clear" else p.op("LDA", "abs", 0x4015)
        p.meta["successor"] = p.pc
        if mode != "no_clear": p.meta["reads"] = [p.meta["critical"]]
        p.delay(12); p.op("SEI")
        dur = 2 if mode == "no_clear" else 4
        irq = int(mode != "masked")
        add(p, "held_" + mode, [(s, 0) for s in (0, 1)],
            lambda ph, dur=dur, irq=irq, mode=mode: dict(bits=[] if mode == "no_clear" else [1],
                read_relative=33266, irq_count=irq, entry=33263+dur+7,
                total=33263+dur+12+2+irq*20, entry_a=0x40, entry_p=0x20,
                final_a=0x40, final_p=0x24, final_x=0, critical_cycles=dur))

    # Stable-source branch controls exercise 2/3/4 cycles and return PC.
    # They do NOT certify an IRQ assertion/acknowledgement inside a branch.
    for mode, dur in [("not_taken", 2), ("taken", 3), ("cross_page", 4)]:
        p = Program("poll_branch_" + mode, "branch")
        p.preamble(); p.sync_call(); p.write(0, "origin"); p.delay(33260)
        a = int(mode != "not_taken")
        p.op("LDA", "imm", a)
        cli = 0x80fc if mode == "cross_page" else 0x80d0
        p.op("JMP", "abs", cli); p.org(cli); p.op("CLI")
        p.meta["critical"] = p.op("BNE", "rel", 3 if mode == "cross_page" else 0)
        if mode == "cross_page": p.org(0x8102)
        p.meta["successor"] = p.pc
        add(p, "branches", [(s, 0) for s in (0, 1)],
            lambda ph, a=a, dur=dur: dict(bits=[], irq_count=1,
                entry=33268+dur+7, total=33268+dur+20, entry_a=a,
                entry_p=0x20 if a else 0x22, final_a=a,
                final_p=0x20 if a else 0x22, final_x=0, critical_cycles=dur))

    output.mkdir(parents=True, exist_ok=True)
    code = [p.resolve() for p in programs]
    (output / "irq_poll_fixtures.c").write_text("\n".join(code))
    fields = ["byte_count", "stop", "origin", "sync_start", "sync_read", "sync_end",
              "read1", "read2", "critical", "successor", "seed", "phase",
              "bit_count", "bit1", "bit2", "read_relative", "irq_count", "entry",
              "total", "entry_a", "entry_p", "final_a", "final_p", "final_x", "critical_cycles"]
    header = ["#pragma once", '#include "recomp/runtime_services.h"',
              "struct IRQByte { uint16_t address; uint8_t value; };",
              "struct IRQFixture { const char* name; const char* group;",
              "uint8_t (*execute)(CPUState*, const RuntimeServices*, uint32_t*); const IRQByte* bytes;",
              "unsigned " + ", ".join(fields) + "; };"]
    for i, p in enumerate(programs):
        header.append('extern "C" uint8_t game_bank0_'+p.name+'(CPUState*, const RuntimeServices*, uint32_t*);')
        data = [(0x8000+j, b) for j, b in enumerate(p.prg) if b]
        header.append("static const IRQByte irq_bytes"+str(i)+"[]={" + ",".join("{%d,%d}" % b for b in data) + "};")
    header.append("static const IRQFixture irq_fixtures[]={")
    for pid, seed, phase, group, expected in cases:
        p = programs[pid]; meta = p.meta; reads = meta.get("reads", []); bits = expected.pop("bits")
        values = dict(expected, byte_count=sum(bool(b) for b in p.prg), stop=meta["stop"],
            origin=meta["origin"], sync_start=meta["sync_start"], sync_read=meta["sync_read"],
            sync_end=meta["sync_end"], read1=reads[0] if reads else 0,
            read2=reads[1] if len(reads)>1 else 0, critical=meta.get("critical", 0),
            successor=meta.get("successor", 0), seed=seed, phase=phase,
            bit_count=len(bits), bit1=bits[0] if bits else 0, bit2=bits[1] if len(bits)>1 else 0)
        header.append('{"%s","%s",game_bank0_%s,irq_bytes%d,%s},' %
            (p.name, group, p.name, pid, ",".join(str(values.get(k, 0)) for k in fields)))
    header.append("};")
    (output / "irq_poll_fixtures.hpp").write_text("\n".join(header)+"\n")
    print(f"Generated {len(programs)} authored IRQ programs / {len(cases)} cases; no ROM")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    generate(parser.parse_args().output)
