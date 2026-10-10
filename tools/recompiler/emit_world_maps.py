"""Decode horizontal base maps from verified source; keep extent provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from emit_blocks import EXPECTED_SHA256


def emit_image(image, output):
    """Decode the user's own image at import time, never from bundled assets."""
    if hashlib.sha256(image).hexdigest()!=EXPECTED_SHA256:raise ValueError('Incompatible ROM')
    prg = image[16:131088]

    def byte(bank, address):
        if not 0x8000 <= address <= 0xFFFF:
            raise ValueError(f'Map source requires unresolved RAM ${address:04X}')
        physical = 7 if address >= 0xC000 else bank
        return prg[physical * 16384 + (address & 0x3FFF)]

    def word(bank, address):
        return byte(bank, address) | byte(bank, address + 1) << 8

    code = ['#include "game/world_maps.h"']
    records = []
    for level in range(37):
        layout = byte(7, 0xD7DA + level)
        bank = 3 if level == 36 or (level < 13 and level != 1) else 5 if level < 29 or level == 35 else 4
        count = (byte(7, 0xF222 + level) or 256) if layout == 0 else 0
        if count == 255:
            count = 256
        record = dict(level=level, layout=layout, data_bank=bank, column_count=count,
                      extent_basis='F222 nominal index limit; natural traversal remains unverified' if not layout else 'Vertical geometry pending')
        if count:
            pointers = [word(7, base + level * 2) for base in (0xF0B0, 0xF0FA, 0xF144, 0xF18E, 0xF1D8)]
            columns = []
            for column in range(count):
                upper = word(bank, pointers[2] + ((byte(bank, pointers[0] + column) * 2) & 255))
                lower = word(bank, pointers[3] + ((byte(bank, pointers[1] + column) * 2) & 255))
                tiles = [byte(bank, upper + row) for row in range(20)] + [byte(bank, lower + row) for row in range(4)]
                packed = [byte(bank, word(bank, pointers[4] + group * 2) + (column >> 2)) for group in range(6)]
                slots = [(packed[row // 4] >> ((2 if column & 2 else 0) + (4 if row & 2 else 0))) & 3 for row in range(24)]
                columns.append((tiles, slots))
            code.append(f'static const GameWorldColumn map_{level}[{count}] = {{')
            code.extend('  {{' + ','.join(f'0x{value:02X}' for value in tiles) + '},{' + ','.join(map(str, slots)) + '}},' for tiles, slots in columns)
            code.append('};')
            record['decoded_base_sha256'] = hashlib.sha256(bytes(value for pair in columns for data in pair for value in data)).hexdigest()
        records.append(record)
    edits = [dict(column=byte(7, 0xDAAE + index), row=11 if byte(7, 0xDAD6 + index // 2) else 7,
                  tile=byte(7, 0xDAC2 + index), flag_index=index // 2) for index in range(20)]
    code.append('const GameWorldEdit game_level13_edits[20] = {')
    code.extend('  {' + ','.join(str(edit[key]) for key in ('column', 'row', 'tile', 'flag_index')) + '},' for edit in edits)
    code.append('};')
    code.append('const GameWorldMap game_world_maps[37] = {')
    code.extend(f'  {{{item["column_count"]},{item["layout"]},{item["data_bank"]},{"map_" + str(item["level"]) if item["column_count"] else "0"}}},' for item in records)
    code.append('};')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'world_maps.c').write_text('\n'.join(code) + '\n')
    (output / 'world_maps.json').write_text(json.dumps(dict(rom_sha256=EXPECTED_SHA256, levels=records,
        level13_edits=dict(flag_ram_base=0x04B0, ordered_edits=edits, source='D9DB/DA7C..DAAB, tablesDAAE/DAC2/DAD6'),
        limitations=['Base columns are immutable; level13 flag overlays are emitted separately. Other live game edits remain unverified.',
                     'Nominal limits are data index extents, not a proof of playable scene bounds.',
                     'Vertical layouts and out-of-view actors remain pending.']), indent=2) + '\n')
    print(f'Generated{sum(bool(item["column_count"]) for item in records)}horizontal base maps with{sum(item["column_count"] for item in records)}columns')

