"""Current SDK must fail closed: own CHR fixtures are NOT installed mod resources."""
from pathlib import Path
import json, tempfile, unittest
import mod_resource_checks as base

class Checks(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='CHR ñ research ');self.addCleanup(self.tmp.cleanup)
        self.path=Path(self.tmp.name).resolve();self.profile,self.catalog=base.fixture(self.path)
        self.store=base.pkg.Store(self.path/'store','0.9.0','own-fixture','a'*64,self.catalog)
    def archive(self,manifest=None,translation=None,extra=()):
        import zipfile
        target=self.path/'research.zip'
        with zipfile.ZipFile(target,'w') as archive:
            archive.writestr('mod.json',json.dumps(manifest or base.manifest()))
            archive.writestr('translations/es-ES.json',json.dumps(translation or base.translation(),ensure_ascii=False))
            for name,data in extra:archive.writestr(name,data)
        return target
    def test_original_fixture_encoding(self):
        root=base.ROOT/'examples/resources/chr-research';meta=json.loads((root/'own-enye-8x8.json').read_text());data=(root/'own-enye-8x8.chr').read_bytes()
        self.assertEqual((meta['character'],meta['width'],meta['height'],len(data)),('Ñ',8,8,16))
        for y in range(8):
            for x in range(8):self.assertEqual(((data[y]>>(7-x))&1)|(((data[y+8]>>(7-x))&1)<<1),meta['pixels'][y][x])
    def test_undeclared_chr_file_rejected_atomically(self):
        tile=(base.ROOT/'examples/resources/chr-research/own-enye-8x8.chr').read_bytes()
        with self.assertRaises(ValueError):self.store.install(self.archive(extra=[('assets/own-enye.chr',tile)]))
        self.assertEqual(self.store.installed(),[])
    def test_unimplemented_permission_rejected(self):
        manifest=base.manifest();manifest['permissions'].append('resources.chr')
        with self.assertRaises(ValueError):self.store.install(self.archive(manifest))
        self.assertEqual(self.store.state()['enabled'],[])
    def test_untrusted_chr_target_rejected(self):
        self.profile['resources']['glyph']={'type':'chr','bank':14,'tile':51}
        with self.assertRaises(ValueError):base.r.validate_profile(self.profile)
    def test_spanish_mapping_still_fails_closed(self):
        for text in ('Ñ','Á','É','Í','Ó','Ú','ñ','á','é','í','ó','ú','Ü','ü','¿','¡'):
            with self.subTest(text=text),self.assertRaisesRegex(ValueError,'Unsupported glyph'):base.r.encode(self.profile,'label',text)
    def test_glyph_and_address_declarations_rejected(self):
        for key,value in [('glyphs',{'Ñ':'assets/own-enye.chr'}),('chr_patches',{'14:51':'00'*16})]:
            data=base.translation();data[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):self.store.install(self.archive(translation=data))
        self.assertEqual(self.store.installed(),[])

if __name__=='__main__':unittest.main()
