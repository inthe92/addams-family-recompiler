"""Emit native C basic blocks from the recovered official 6502 instruction CFG."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

BRANCHES = {'BCC':('CPU_C',False),'BCS':('CPU_C',True),'BEQ':('CPU_Z',True),'BNE':('CPU_Z',False),
            'BMI':('CPU_N',True),'BPL':('CPU_N',False),'BVC':('CPU_V',False),'BVS':('CPU_V',True)}
READS = {'ADC','SBC','AND','ORA','EOR','LDA','LDX','LDY','CMP','CPX','CPY','BIT'}
STORES = {'STA':'A','STX':'X','STY':'Y'}
RMW = {'ASL','LSR','ROL','ROR','INC','DEC'}
CONTROL = set(BRANCHES) | {'JMP','JSR','RTS','RTI','BRK'}
EXPECTED_SHA256 = '6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880'

def base_cycles(name,mode):
    if name in BRANCHES:return 2
    if name in RMW:return {'acc':2,'zp':5,'zpx':6,'abs':6,'abx':7}[mode]
    if name in STORES:return {'zp':3,'zpx':4,'zpy':4,'abs':4,'abx':5,'aby':5,'izx':6,'izy':6}[mode]
    if name=='BIT':return {'zp':3,'abs':4}[mode]
    if name in READS:return {'imm':2,'zp':3,'zpx':4,'zpy':4,'abs':4,'abx':4,'aby':4,'izx':6,'izy':5}[mode]
    return {'JSR':6,'RTS':6,'RTI':6,'BRK':7,'JMP':5 if mode=='ind' else 3,
            'PHA':3,'PHP':3,'PLA':4,'PLP':4}.get(name,2)

def decode(prg,instruction,bank,base,opcodes,sizes):
    offset=bank*16384+instruction['pc']-(0xc000 if bank==7 else 0x8000)
    opcode=prg[offset];name,mode=opcodes[opcode];size=sizes[mode]
    if size!=instruction['size']:raise ValueError('Source instruction width differs')
    raw=prg[offset:offset+size]
    return dict(pc=base+offset%16384,name=name,mode=mode,size=size,value=int.from_bytes(raw[1:],'little'),
                opcode=opcode,tier=instruction['tier'],raw=raw.hex())

def direct_target(i):
    if i['name'] in BRANCHES:return (i['pc']+2+(i['value']-256 if i['value']>=128 else i['value']))&65535
    if i['name'] in ['JSR','JMP'] and i['mode']=='abs':return i['value']
    return None

def split_blocks(instructions):
    by_pc={i['pc']:i for i in instructions};leaders={instructions[0]['pc']} if instructions else set()
    previous=None
    for i in instructions:
        pc=i['pc']
        if previous is None or previous['pc']+previous['size']!=pc or previous['name'] in CONTROL:leaders.add(pc)
        target=direct_target(i)
        if target in by_pc:leaders.add(target)
        previous=i
    groups=[];group=[]
    for i in instructions:
        if group and (i['pc'] in leaders or len(group)>=64):groups.append(group);group=[]
        group.append(i)
    if group:groups.append(group)
    return groups

def emit_address(name,mode,value):
    """Address arithmetic is baked into each native block, not decoded at runtime."""
    result=[];literal=f'0x{value:04X}'
    if mode=='imm':return result,f'0x{value:02X}'
    if mode in ['abs','zp']:result.append(f'uint16_t address = {literal};')
    elif mode in ['zpx','zpy']:
        register='X' if mode=='zpx' else 'Y'
        result += [f'(void)recomp_read8(s, {literal}, 2);',f'uint16_t address = (uint8_t)({literal} + cpu->{register});']
    elif mode=='izx':
        result += [f'(void)recomp_read8(s, {literal}, 2);',f'uint16_t address = recomp_read16_zp(s, (uint8_t)({literal} + cpu->X), 3);']
    elif mode in ['abx','aby','izy']:
        register='X' if mode=='abx' else 'Y'
        base=f'recomp_read16_zp(s, (uint8_t){literal}, 2)' if mode=='izy' else literal
        result += [f'uint16_t base = {base};',f'uint16_t address = (uint16_t)(base + cpu->{register});']
        dummy='(uint16_t)((base & 0xFF00u) | (address & 0x00FFu))'
        dummy_offset=4 if mode=='izy' else 3
        if name in STORES or name in RMW:result.append(f'(void)recomp_read8(s, {dummy}, {dummy_offset});')
        else:result.append(f'if ((base & 0xFF00u) != (address & 0xFF00u)) {{ ++cycles; (void)recomp_read8(s, {dummy}, {dummy_offset}); }}')
    else:raise ValueError('Unimplemented address mode '+mode)
    offset='cycles - 3u' if name in RMW else 'cycles - 1u'
    return result,f'recomp_read8(s, address, (uint8_t)({offset}))'

def emit_semantics(i):
    name,mode,value,pc=i['name'],i['mode'],i['value'],i['pc'];following=(pc+i['size'])&65535
    lines=[f'uint8_t cycles = {base_cycles(name,mode)};'];next_pc=f'0x{following:04X}'
    if name in ['CLI','SEI','PLP']:
        lines.append('uint8_t irq_mask = (uint8_t)(cpu->P & CPU_I);')
    if name in READS or name in STORES or name in RMW and mode!='acc':
        address,operand=emit_address(name,mode,value);lines+=address
    if name in READS:
        lines.append(f'uint8_t operand = (uint8_t)({operand});')
        if name in ['ADC','SBC']:lines.append(f'cpu_{name.lower()}(cpu, operand);')
        elif name in ['LDA','LDX','LDY']:
            reg=name[-1];lines += [f'cpu->{reg} = operand;',f'cpu_set_nz(cpu, cpu->{reg});']
        elif name in ['AND','ORA','EOR']:
            op={'AND':'&','ORA':'|','EOR':'^'}[name];lines += [f'cpu->A = (uint8_t)(cpu->A {op} operand);','cpu_set_nz(cpu, cpu->A);']
        elif name in ['CMP','CPX','CPY']:lines.append(f'recomp_compare(cpu, cpu->{dict(CMP="A",CPX="X",CPY="Y")[name]}, operand);')
        elif name=='BIT':lines += ['cpu_set_flag(cpu, CPU_Z, (uint8_t)((cpu->A & operand) == 0));','cpu_set_flag(cpu, CPU_N, (uint8_t)((operand & 0x80u) != 0));','cpu_set_flag(cpu, CPU_V, (uint8_t)((operand & 0x40u) != 0));']
    elif name in STORES:lines.append(f'recomp_write8(s, address, cpu->{STORES[name]}, (uint8_t)(cycles - 1u));')
    elif name in RMW:
        lines.append('uint8_t before = cpu->A;' if mode=='acc' else 'uint8_t before = recomp_read8(s, address, (uint8_t)(cycles - 3u));')
        if mode!='acc':lines.append('recomp_write8(s, address, before, (uint8_t)(cycles - 2u));')
        if name in ['ROL','ROR']:lines.append('uint8_t carry = cpu_has_flag(cpu, CPU_C);')
        if name in ['ASL','ROL']:
            lines.append('cpu_set_flag(cpu, CPU_C, (uint8_t)((before & 0x80u) != 0));')
            expression='(uint8_t)(before << 1)' if name=='ASL' else '(uint8_t)((uint8_t)(before << 1) | carry)'
        elif name in ['LSR','ROR']:
            lines.append('cpu_set_flag(cpu, CPU_C, (uint8_t)(before & 1u));')
            expression='(uint8_t)(before >> 1)' if name=='LSR' else '(uint8_t)((before >> 1) | (carry ? 0x80u : 0))'
        else:expression=f'(uint8_t)(before {"+" if name=="INC" else "-"} 1u)'
        lines += [f'uint8_t result = {expression};','cpu_set_nz(cpu, result);']
        lines.append('cpu->A = result;' if mode=='acc' else 'recomp_write8(s, address, result, (uint8_t)(cycles - 1u));')
    elif name in BRANCHES:
        flag,wanted=BRANCHES[name];target=direct_target(i);page=int((following&0xff00)!=(target&0xff00))
        lines += [f'uint8_t taken = (uint8_t)({"" if wanted else "!"}cpu_has_flag(cpu, {flag}));',
                  f'if (taken) cycles = (uint8_t)(cycles + {1+page}u);',f'uint16_t next_pc = taken ? 0x{target:04X} : 0x{following:04X};'];next_pc='next_pc'
    elif name=='JMP':
        if mode=='abs':next_pc=f'0x{value:04X}'
        else:
            high=(value&0xff00)|((value+1)&255)
            lines += [f'uint8_t low = recomp_read8(s, 0x{value:04X}, 3);',f'uint8_t high = recomp_read8(s, 0x{high:04X}, 4);','uint16_t next_pc = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));'];next_pc='next_pc'
    elif name=='JSR':
        ret=(following-1)&65535;lines += [f'recomp_push(cpu, s, 0x{ret>>8:02X}, 3);',f'recomp_push(cpu, s, 0x{ret&255:02X}, 4);'];next_pc=f'0x{value:04X}'
    elif name in ['RTS','RTI']:
        if name=='RTI':lines.append('cpu_status_from_pull(cpu, recomp_pull(cpu, s, 3));')
        offset=4 if name=='RTI' else 3
        lines += [f'uint8_t low = recomp_pull(cpu, s, {offset});',f'uint8_t high = recomp_pull(cpu, s, {offset+1});','uint16_t next_pc = (uint16_t)((uint16_t)low | ((uint16_t)high << 8));']
        if name=='RTS':lines.append('next_pc = (uint16_t)(next_pc + 1u);')
        next_pc='next_pc'
    elif name=='BRK':
        ret=(pc+2)&65535;lines += [f'recomp_push(cpu, s, 0x{ret>>8:02X}, 2);',f'recomp_push(cpu, s, 0x{ret&255:02X}, 3);','recomp_push(cpu, s, cpu_status_for_push(cpu, 1), 4);','cpu_set_flag(cpu, CPU_I, 1);','uint16_t next_pc = recomp_read16(s, 0xFFFE, 5);'];next_pc='next_pc'
    elif name in ['PHA','PHP']:lines.append(f'recomp_push(cpu, s, {"cpu->A" if name=="PHA" else "cpu_status_for_push(cpu, 1)"}, 2);')
    elif name=='PLA':lines += ['cpu->A = recomp_pull(cpu, s, 3);','cpu_set_nz(cpu, cpu->A);']
    elif name=='PLP':lines.append('cpu_status_from_pull(cpu, recomp_pull(cpu, s, 3));')
    elif name in ['CLC','SEC','CLI','SEI','CLD','SED','CLV']:
        flag,setting={'CLC':('C',0),'SEC':('C',1),'CLI':('I',0),'SEI':('I',1),'CLD':('D',0),'SED':('D',1),'CLV':('V',0)}[name];lines.append(f'cpu_set_flag(cpu, CPU_{flag}, {setting});')
    elif name in ['INX','DEX','INY','DEY']:
        reg=name[-1];op='+' if name.startswith('IN') else '-';lines += [f'cpu->{reg} = (uint8_t)(cpu->{reg} {op} 1u);',f'cpu_set_nz(cpu, cpu->{reg});']
    elif name in ['TAX','TAY','TXA','TYA','TSX','TXS']:
        source,destination={'TAX':('A','X'),'TAY':('A','Y'),'TXA':('X','A'),'TYA':('Y','A'),'TSX':('SP','X'),'TXS':('X','SP')}[name]
        lines.append(f'cpu->{destination} = cpu->{source};')
        if name!='TXS':lines.append(f'cpu_set_nz(cpu, cpu->{destination});')
    elif name!='NOP':raise ValueError('Unimplemented mnemonic '+name)
    return lines,next_pc

def emit_bank(instructions,bank,window):
    chunks=['#include "game/compiled_program.h"\n'];mapping={};blocks=split_blocks(instructions)
    for block in blocks:
        symbol=f'block_b{bank}_{window}_{block[0]["pc"]:04X}';pcs={i['pc'] for i in block}
        chunks.append(f'static uint8_t {symbol}(CPUState* cpu, const RuntimeServices* s, uint32_t* left) {{\n  switch (cpu->PC) {{')
        for i in block:chunks.append(f'    case 0x{i["pc"]:04X}: goto L{i["pc"]:04X};');mapping[i['pc']]=symbol
        chunks.append('    default: return 0;\n  }')
        for i in block:
            pc=i['pc'];lines,next_pc=emit_semantics(i);chunks.append(f'L{pc:04X}: {{ /* {i["name"]} {i["mode"]}; {i["tier"]}; {i["raw"]} */')
            chunks.extend('  '+line for line in lines)
            finish = (f'recomp_finish_polled(cpu, s, {bank}, {next_pc}, cycles, left, irq_mask)'
                      if i['name'] in ['CLI','SEI','PLP']
                      else f'recomp_finish(cpu, s, {bank}, {next_pc}, cycles, left)')
            chunks.append(f'  if ({finish}) return 1;')
            following=pc+i['size'];target=direct_target(i)
            if i['name'] in BRANCHES:
                chunks.append('  if (taken) { '+(f'goto L{target:04X};' if target in pcs else 'return 1;')+' }')
                chunks.append('  '+(f'goto L{following:04X};' if following in pcs else 'return 1;'))
            elif i['name']=='JMP' and i['mode']=='abs' or i['name']=='JSR':chunks.append('  '+(f'goto L{target:04X};' if target in pcs else 'return 1;'))
            elif i['name'] in ['RTS','RTI','BRK'] or i['name']=='JMP':chunks.append('  return 1;')
            else:chunks.append('  '+(f'goto L{following:04X};' if following in pcs else 'return 1;'))
            chunks.append('}')
        chunks.append('}\n')
    name=f'game_bank{bank}_{window}'
    chunks.append(f'uint8_t {name}(CPUState* cpu, const RuntimeServices* s, uint32_t* left) {{\n  switch (cpu->PC) {{')
    for pc,symbol in sorted(mapping.items()):chunks.append(f'    case 0x{pc:04X}: return {symbol}(cpu, s, left);')
    chunks.append('    default: return 0;\n  }\n}\n')
    chunks.append(f'uint8_t {name}_has(uint16_t pc) {{\n  switch (pc) {{')
    chunks.extend(f'    case 0x{pc:04X}:' for pc in sorted(mapping));chunks.append('      return 1;\n    default: return 0;\n  }\n}\n')
    return '\n'.join(chunks),len(blocks)


def emit_image(image,m,output,profile_sha256):
    """Translate only bytes supplied locally by the user; m holds code locations."""
    from opcodes_6502 import OPCODES,SIZES
    if hashlib.sha256(image).hexdigest()!=EXPECTED_SHA256:raise ValueError('Incompatible ROM')
    prg=image[16:131088];output.mkdir(parents=True,exist_ok=True);stats=[];supported=set()
    for bank in range(8):
        text=[]
        for window,base in [('low',0x8000),('high',0xc000)]:
            instructions=[decode(prg,i,bank,base,OPCODES,SIZES) for i in sorted(m['banks'][bank]['instruction_evidence'],key=lambda i:i['pc'])]
            if len({i['pc'] for i in instructions})!=len(instructions):raise ValueError('Duplicate recovered instruction start')
            code,blocks=emit_bank(instructions,bank,window);text.append(code);supported.update(i['opcode'] for i in instructions)
            stats.append(dict(bank=bank,window=window,instructions=len(instructions),basic_blocks=blocks,tiers=dict(Counter(i['tier'] for i in instructions))))
        (output/f'bank{bank}.c').write_text('\n'.join(text))
    dispatcher=['#include "game/compiled_program.h"',
                'uint32_t nes_recomp_abi_version(void) { return NES_RECOMP_ABI_VERSION; }']
    for bank in range(8):
        for window in ['low','high']:
            dispatcher.append(f'uint8_t game_bank{bank}_{window}(CPUState*, const RuntimeServices*, uint32_t*);')
            dispatcher.append(f'uint8_t game_bank{bank}_{window}_has(uint16_t);')
    # Known self-installed interrupt jumps have an explicit native translation.
    # The opcode is guarded; the writable JMP operand remains a data read.
    dispatcher += ['uint8_t game_execute_slice(CPUState* cpu, const RuntimeServices* s, uint32_t* left) {','  if (!*left) return 1;',
        '  if (cpu->PC == 0x0104 || cpu->PC == 0x0314) {',
        '    if (recomp_read8(s, cpu->PC, 0) != 0x4C) return 0;',
        '    const uint16_t next = recomp_read16(s, (uint16_t)(cpu->PC + 1u), 1);',
        '    (void)recomp_finish(cpu,s,0xFF,next,3,left); return 1;','  }',
        '  const uint8_t bank = s->bank_for_pc(s->context, cpu->PC);','  const uint8_t high = (uint8_t)(cpu->PC >= 0xC000);','  if (cpu->PC < 0x8000) return 0;','  switch (bank) {']
    for bank in range(8):dispatcher.append(f'    case {bank}: return high ? game_bank{bank}_high(cpu,s,left) : game_bank{bank}_low(cpu,s,left);')
    dispatcher += ['    default: return 0;','  }','}','uint8_t game_has_compiled_address(uint8_t bank, uint16_t pc) {',
        '  if (pc == 0x0104 || pc == 0x0314) return 1;','  if (pc < 0x8000) return 0;','  switch (bank) {']
    for bank in range(8):dispatcher.append(f'    case {bank}: return pc >= 0xC000 ? game_bank{bank}_high_has(pc) : game_bank{bank}_low_has(pc);')
    dispatcher += ['    default: return 0;','  }','}'];(output/'dispatch.c').write_text('\n'.join(dispatcher)+'\n')
    result=dict(rom_sha256=EXPECTED_SHA256,source_manifest_sha256=profile_sha256,banks=stats,
        physical_instruction_starts=sum(r['instructions'] for r in stats if r['window']=='low'),native_window_instruction_entries=sum(r['instructions'] for r in stats),
        native_basic_blocks=sum(r['basic_blocks'] for r in stats),encountered_official_opcodes=sorted(supported),
        ram_trampolines=[dict(pc=0x0104,original_target=0xc21f),dict(pc=0x0314,original_target=0xc511)],
        limitations=['Candidates remain candidates: emitting native C does not prove original reachability or complete program behavior.',
            'Only recovered instruction starts and two guarded RAM interrupt JMPs are compiled. Other missing addresses have no instruction-interpreter fallback.',
            'Both physical CPU windows are emitted; absolute operands are preserved and relative branches use the selected CPU window.',
            'Service ABI v2 preserves timed reads/writes, RMW double writes and existing indexed dummy accesses. Baked instruction/operand fetches and omitted dummy accesses remain unrepresented.',
            'CLI/SEI/PLP use the pre-instruction I flag, but interrupt-line sampling within instructions and instruction-fetch bus side effects remain unverified.'])
    (output/'compiled_program.json').write_text(json.dumps(result,indent=2)+'\n')
    print({k:result[k] for k in ['physical_instruction_starts','native_window_instruction_entries','native_basic_blocks']})

