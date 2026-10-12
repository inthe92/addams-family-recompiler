"""Own frontend/import messages only; never the original game text or OS chrome."""
import ast
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/recompiler'))
from import_user_rom import validate_rom

class EnglishChecks(unittest.TestCase):
    def test_owned_literals_have_no_legacy_spanish(self):
        paths=['app/desktop.cpp','runtime/src/user_rom.cpp','runtime/src/user_rom_mac.mm','runtime/include/runtime/ui_text.hpp','tools/recompiler/import_user_rom.py']
        pattern=r'Importa tu|Importando|No se (?:ha|puede)|Falta (?:el|un|una)|Flechas|pantalla completa|GOD activo|Registro:|SHA-256 distinto|revision europea|[áéíóúñ]'
        for f in paths:
            source=(ROOT/f).read_text()
            strings=[n.value for n in ast.walk(ast.parse(source)) if isinstance(n,ast.Constant) and isinstance(n.value,str)] if f.endswith('.py') else re.findall(r'"((?:\\.|[^"\\])*)"',source)
            for s in strings:self.assertNotRegex(s,pattern,f)
    def test_rom_prompt_and_exit_contract(self):
        text=(ROOT/'runtime/include/runtime/ui_text.hpp').read_text()
        for message in ['Select your NES ROM','Preparing your game...','Compiling game code. This may take a few minutes...','Exit NES Native Lab?','Are you sure you want to quit the game?','Cancel','Exit Game']:
            self.assertIn('"'+message+'"',text)
    def test_incompatible_size_and_hash_english(self):
        profile={'rom_size':32,'rom_sha256':'0'*64}
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'synthetic.nes';p.write_bytes(b'x')
            with self.assertRaisesRegex(ValueError,'Incompatible ROM.*supported European PAL'):validate_rom(p,profile)
            p.write_bytes(bytes(32))
            with self.assertRaisesRegex(ValueError,'Incompatible ROM: SHA-256 mismatch'):validate_rom(p,profile)
    def test_invalid_header_english(self):
        data=bytes(32);profile={'rom_size':32,'rom_sha256':hashlib.sha256(data).hexdigest()}
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'synthetic.nes';p.write_bytes(data)
            with self.assertRaisesRegex(ValueError,'Incompatible iNES/MMC1 header'):validate_rom(p,profile)
    def test_real_process_rejection_without_module(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'synthetic.nes';p.write_bytes(bytes(4))
            result=subprocess.run([sys.executable,'-B',str(ROOT/'tools/recompiler/import_user_rom.py'),'--rom',str(p),'--cache',d,'--validate-only'],capture_output=True,text=True)
            self.assertEqual(result.returncode,1);self.assertIn('supported European PAL',result.stderr);self.assertFalse(list(Path(d).glob('user_game.*')))
    def test_no_permanent_control_title_or_game_translation(self):
        source=(ROOT/'app/desktop.cpp').read_text()
        self.assertIn('caption=title+',source);self.assertNotIn('F11: pantalla',source)
        self.assertIn('NES_RECOMP_ABI_VERSION',(ROOT/'game/src/gameplay_diagnostic.cpp').read_text())
        self.assertIn('VERSION 0.9.0',(ROOT/'CMakeLists.txt').read_text())

if __name__=='__main__':unittest.main()
