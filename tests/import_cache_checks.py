"""Cache ABI decisions with synthetic bytes; never load or read a ROM."""
import json
import os
from pathlib import Path
import platform
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/recompiler'))
import import_user_rom as importer

class CacheABI(unittest.TestCase):
    def check_receipt(self,version,valid_identity=True):
        with tempfile.TemporaryDirectory() as directory:
            cache=Path(directory)
            suffix='.dll' if os.name=='nt' else '.dylib' if platform.system()=='Darwin' else '.so'
            module=cache/('user_game'+suffix)
            module.write_bytes(b'authored synthetic cache marker, not a native game')
            identity=importer.cache_identity(ROOT,(importer.HERE/'compatible_profile.json').read_bytes())
            receipt=dict(generator_sha256=identity if valid_identity else 'outdated',module_sha256=importer.digest(module.read_bytes()))
            if version is not None:receipt['abi_version']=version
            (cache/'import.json').write_text(json.dumps(receipt))
            with patch.object(importer,'validate_rom',return_value=b'synthetic validation result'),patch.object(importer,'native_compiler',side_effect=RuntimeError('regeneration required')) as compile_probe:
                if version==importer.module_abi_version(ROOT) and valid_identity:
                    self.assertEqual(importer.import_rom(cache/'unused.nes',cache,ROOT),module)
                    compile_probe.assert_not_called()
                else:
                    with self.assertRaisesRegex(RuntimeError,'regeneration required'):
                        importer.import_rom(cache/'unused.nes',cache,ROOT)
                    compile_probe.assert_called_once_with(ROOT)
    def test_current_abi_reuses_cache(self):self.check_receipt(importer.module_abi_version(ROOT))
    def test_old_abi_regenerates(self):self.check_receipt(1)
    def test_missing_abi_regenerates(self):self.check_receipt(None)
    def test_changed_generator_regenerates(self):self.check_receipt(importer.module_abi_version(ROOT),False)

if __name__=='__main__':unittest.main()
