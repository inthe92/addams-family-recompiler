"""Profiles/codecs/data-only ZIP transactions; all original bytes are authored fixtures."""
from pathlib import Path
import copy, hashlib, importlib.util, json, tempfile, unittest, zipfile, zlib
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('packages',ROOT/'tools/mods/packages.py');pkg=importlib.util.module_from_spec(spec);spec.loader.exec_module(pkg);r=pkg.resource_tools

def fixture(path):
    data=b'ABC';p={'profile_schema':1,'profile_id':'fixture-text-v1','game_id':'own-fixture','rom_sha256':'a'*64,'rom_size':32784,'prg_size':32768,'codec':{'id':'fixed-glyph-field-v1','glyphs':{' ':0,'A':65,'B':66,'C':67},'padding':0,'display':'uppercase'},'resources':{'label':{'type':'text','bank':0,'offset':2048,'expected_length':3,'replacement_length':3,'max_chars':3,'codec':'fixed-glyph-field-v1','original_sha256':hashlib.sha256(data).hexdigest(),'original_crc32':zlib.crc32(data),'reader_bank':0,'reader_pc':32768,'controls':[]}}}
    catalog=path/'tools/mods/catalog.json';catalog.parent.mkdir(parents=True);catalog.write_text(json.dumps({'catalog_schema':1,'profiles':[p]}))
    ins=path/'tools/recompiler/compatible_profile.json';ins.parent.mkdir(parents=True);ins.write_text(json.dumps({'rom_sha256':'a'*64,'instruction_starts':[[32768],[]]}));return p,catalog

def manifest(id='translate.one'):
    return {'id':id,'name':'Own fixture','version':'1.0.0','api_version':2,'description':'Authored data only','author':'Tests','entry':None,'permissions':['resources.text'],'target':{'game_id':'own-fixture','engine_min':'0.9.0','engine_max_exclusive':'0.10.0'},'translation':{'profile_id':'fixture-text-v1','file':'translations/es-ES.json'}}
def translation(text='CAB'):
    return {'schema':1,'profile_id':'fixture-text-v1','locale':'es-ES','texts':{'label':text}}
class Checks(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.path=Path(self.tmp.name).resolve();self.p,self.catalog=fixture(self.path);self.store=pkg.Store(self.path/'store','0.9.0','own-fixture','a'*64,self.catalog)
    def zip(self,m=None,t=None,extra=None):
        path=self.path/'package.zip'
        with zipfile.ZipFile(path,'w') as z:
            z.writestr('mod.json',json.dumps(m or manifest()));z.writestr('translations/es-ES.json',json.dumps(t or translation(),ensure_ascii=False));
            if extra:
                for n,data in extra:z.writestr(n,data)
        return path
    def bad_profile(self,key,value):
        p=copy.deepcopy(self.p);p[key]=value
        with self.assertRaises(ValueError):r.validate_profile(p)
    def test_profile_valid(self):self.assertEqual(r.catalog(self.catalog)['fixture-text-v1'],self.p)
    def test_schema(self):self.bad_profile('profile_schema',True)
    def test_unknown_field(self):self.bad_profile('execute','native')
    def test_fingerprint(self):self.bad_profile('rom_sha256','bad')
    def test_range(self):
        p=copy.deepcopy(self.p);p['resources']['label']['offset']=16383
        with self.assertRaises(ValueError):r.validate_profile(p)
    def test_vectors(self):
        p=copy.deepcopy(self.p);p['resources']['label']['offset']=0x3ffa
        with self.assertRaisesRegex(ValueError,'vector'):r.validate_profile(p)
    def test_controls(self):
        p=copy.deepcopy(self.p);p['resources']['label']['controls']=[255]
        with self.assertRaises(ValueError):r.validate_profile(p)
    def test_instruction_overlap(self):
        with self.assertRaises(ValueError):r.validate_profile(self.p,[[0x8000,0x8800],[]])
    def test_operand_overlap(self):
        with self.assertRaises(ValueError):r.validate_profile(self.p,[[0x8000,0x87fe],[]])
    def test_reader(self):
        with self.assertRaises(ValueError):r.validate_profile(self.p,[[],[]])
    def test_missing_map(self):
        (self.path/'tools/recompiler/compatible_profile.json').unlink()
        with self.assertRaises(ValueError):r.catalog(self.catalog)
    def test_encode_decode(self):self.assertEqual(r.decode(self.p,r.encode(self.p,'label','CA')),'CA ')
    def test_unknown_resource(self):
        with self.assertRaises(ValueError):r.encode(self.p,'address.8800','A')
    def test_overflow(self):
        with self.assertRaises(ValueError):r.encode(self.p,'label','ABCA')
    def test_empty(self):
        with self.assertRaises(ValueError):r.encode(self.p,'label','')
    def test_unicode_rejected(self):
        for text in ('Ñ','ñ','Á','É','Í','Ó','Ú','á','é','í','ó','ú','Ü','ü','¿','¡','\n','\0','A\u0301'):
            with self.subTest(text=text),self.assertRaises(ValueError):r.encode(self.p,'label',text)
    def test_duplicate_json(self):
        with self.assertRaises(ValueError):r.read('{"schema":1,"schema":1}')
    def test_oversized_json(self):
        with self.assertRaises(ValueError):r.compile_translation(b' '*16385,self.p)
    def test_utf8(self):
        with self.assertRaises(ValueError):r.compile_translation(b'\xff',self.p)
    def test_profile_unknown(self):
        m=manifest();m['translation']['profile_id']='unknown'
        with self.assertRaises(ValueError):self.store.install(self.zip(m))
    def test_revision(self):
        self.store.rom_sha256='b'*64
        with self.assertRaises(ValueError):self.store.install(self.zip())
    def test_permission(self):
        m=manifest();m['permissions']=[]
        with self.assertRaises(ValueError):self.store.install(self.zip(m))
    def test_no_lua(self):
        with self.assertRaises(ValueError):self.store.install(self.zip(extra=[('main.lua',"error('must never run')")]))
        self.assertEqual(self.store.installed(),[])
    def test_paths(self):
        with self.assertRaises(ValueError):self.store.install(self.zip(extra=[('../escape.txt','x')]))
    def test_duplicates(self):
        with self.assertRaises(ValueError):self.store.install(self.zip(extra=[('translations/es-ES.json','{}')]))
    def test_unknown_json(self):
        with self.assertRaises(ValueError):self.store.install(self.zip(extra=[('translations/en-US.json','{}')]))
    def test_locale(self):
        t=translation();t['locale']='en-US'
        with self.assertRaises(ValueError):self.store.install(self.zip(t=t))
    def test_original_disabled(self):
        self.store.install(self.zip());self.assertFalse(self.store.installed()[0]['enabled']);self.assertEqual(self.store.state()['enabled'],[])
    def test_conflict_atomic(self):
        self.store.install(self.zip());self.store.change('enable','translate.one');self.store.install(self.zip(manifest('translate.two')));before=self.store.state()
        with self.assertRaisesRegex(ValueError,'disable it first'):self.store.change('enable','translate.two')
        self.assertEqual(self.store.state(),before)
    def test_disable_reactivate(self):
        self.store.install(self.zip());self.store.change('enable','translate.one');self.assertEqual(self.store.replacements(self.store.installed()[0])[0][2],b'CAB');self.store.change('disable','translate.one');self.assertFalse(self.store.installed()[0]['enabled']);self.store.change('enable','translate.one');self.store.change('uninstall','translate.one');self.assertEqual(self.store.installed(),[])
    def test_revalidate_installed(self):
        self.store.install(self.zip());(self.store.packages/'translate.one/translations/es-ES.json').write_text(json.dumps(translation('Ñ')))
        with self.assertRaises(ValueError):self.store.change('enable','translate.one')
    def test_v1_compatible(self):
        m=manifest();m['api_version']=1;m['entry']='main.lua'
        for k in ('permissions','target','translation'):del m[k]
        self.assertEqual(pkg.manifest(m)['api_version'],1)
    def test_real_catalog_no_original_assets(self):
        p=r.catalog(ROOT/'tools/mods/resource_profiles.json')['addams-pal-inventory-text-v1'];self.assertEqual(r.decode(p,r.encode(p,'inventory.heading','OBJETOS')),'OBJETOS  ')
if __name__=='__main__':unittest.main()
