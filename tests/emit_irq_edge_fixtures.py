"""Small authored instructions for poll/mask/DMA controls, never a ROM input."""
import pathlib
import sys
from emit_irq_poll_fixtures import Program


def generate(output):
    definitions = [
        ("nop", "NOP", "imp", 0, 0x8000, 0x8001, 2),
        ("cli", "CLI", "imp", 0, 0x8000, 0x8001, 2),
        ("sei", "SEI", "imp", 0, 0x8000, 0x8001, 2),
        ("plp", "PLP", "imp", 0, 0x8000, 0x8001, 4),
        ("rti", "RTI", "imp", 0, 0x8000, 0x8200, 6),
        ("clear", "LDA", "abs", 0x4015, 0x8000, 0x8003, 4),
        ("inhibit", "STA", "abs", 0x4017, 0x8000, 0x8003, 4),
        ("rmw_clear", "INC", "abs", 0x4015, 0x8000, 0x8003, 6),
        ("branch", "BNE", "rel", 0, 0x8000, 0x8002, 3),
        ("cross", "BNE", "rel", 3, 0x80fd, 0x8102, 4),
        ("jmp", "JMP", "abs", 0x8200, 0x8000, 0x8200, 3),
        ("oam", "STA", "abs", 0x4014, 0x8000, 0x8003, 4),
    ]
    output.mkdir(parents=True, exist_ok=True)
    code, header = [], ["#pragma once", '#include "recomp/runtime_services.h"',
        "struct EdgeByte { uint16_t address; uint8_t value; };",
        "struct EdgeFixture { const char* name; uint8_t (*execute)(CPUState*, const RuntimeServices*, uint32_t*);",
        "uint16_t pc, next; unsigned cycles, count; const EdgeByte* bytes; };"]
    fixtures = []
    for index, (name, op, mode, value, pc, next_pc, cycles) in enumerate(definitions):
        p = Program("edge_" + name, "cpu-control")
        p.org(pc); p.op(op, mode, value)
        # Fall-through for a not-taken crossing branch is distinct from target.
        if name == "cross": p.org(0x80ff); p.op("NOP")
        p.org(next_pc); p.op("NOP")
        p.org(next_pc + 1); p.op("NOP")
        p.org(0x9000); p.op("RTI")
        p.org(0x9100); p.op("RTI")
        code.append(p.resolve())
        p.prg[0x7ffa:0x7ffc] = bytes([0, 0x91])
        data = [(0x8000 + j, b) for j, b in enumerate(p.prg) if b]
        symbol = "game_bank0_edge_" + name
        header.append('extern "C" uint8_t ' + symbol + '(CPUState*, const RuntimeServices*, uint32_t*);')
        header.append("static const EdgeByte edge_bytes%d[]={%s};" %
                      (index, ",".join("{%d,%d}" % b for b in data)))
        fixtures.append('{"%s",%s,%d,%d,%d,%d,edge_bytes%d}' %
                        (name, symbol, pc, next_pc, cycles, len(data), index))
    header.append("static const EdgeFixture edge_fixtures[]={" + ",".join(fixtures) + "};")
    (output / "irq_edge_fixtures.c").write_text("\n".join(code))
    (output / "irq_edge_fixtures.hpp").write_text("\n".join(header) + "\n")
    print("Generated 12 authored IRQ edge fixtures; no ROM")


if __name__ == "__main__":
    generate(pathlib.Path(sys.argv[1]))
