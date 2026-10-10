"""Public tests use synthetic bytes only; a commercial ROM is never a fixture."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/recompiler'))
from import_user_rom import validate_rom
from opcodes_6502 import OPCODES, SIZES
from emit_blocks import emit_semantics

class PublicImportChecks(unittest.TestCase):
    def test_profile_contains_only_identity_and_addresses(self):
        p=json.loads((ROOT/'tools/recompiler/compatible_profile.json').read_text())
        self.assertEqual(set(p),{'schema','rom_sha256','rom_size','instruction_starts'})
        self.assertEqual(len(p['instruction_starts']),8)
        for bank,addresses in enumerate(p['instruction_starts']):
            self.assertEqual(addresses,sorted(set(addresses)))
            base=0xc000 if bank==7 else 0x8000
            self.assertTrue(all(base<=pc<base+16384 for pc in addresses))
    def test_wrong_size_and_wrong_hash_rejected_without_output(self):
        profile=json.loads((ROOT/'tools/recompiler/compatible_profile.json').read_text())
        with tempfile.TemporaryDirectory() as name:
            f=Path(name)/'fake.nes';f.write_bytes(b'NES\x1a')
            with self.assertRaises(ValueError):validate_rom(f,profile)
            f.write_bytes(b'NES\x1a'+b'\0'*(profile['rom_size']-4))
            with self.assertRaises(ValueError):validate_rom(f,profile)
            self.assertEqual([p.name for p in Path(name).iterdir()],['fake.nes'])
    def test_generic_opcode_codegen(self):
        self.assertEqual(len(OPCODES),151)
        for opcode,(name,mode) in OPCODES.items():
            lines,next_pc=emit_semantics(dict(pc=0x8000,name=name,mode=mode,value=0,size=SIZES[mode]))
            self.assertTrue(lines);self.assertTrue(next_pc)

if __name__=='__main__':unittest.main()
