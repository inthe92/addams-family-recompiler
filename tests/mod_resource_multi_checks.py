"""Multi-field security and transactions; original data is an authored fixture."""
import copy, json, tempfile, unittest, zipfile
from pathlib import Path
import mod_resource_checks as base
pkg, r = base.pkg, base.r

class MultiChecks(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='SDK ñ Ω 東京 ');self.addCleanup(self.tmp.cleanup)
        self.path=Path(self.tmp.name).resolve();self.p,self.catalog=base.fixture(self.path)
        self.p['resources']['second']=dict(self.p['resources']['label'],offset=2064)
        self.save();self.store=pkg.Store(self.path/'store ñ Ω','0.9.0','own-fixture','a'*64,self.catalog)
    def save(self):self.catalog.write_text(json.dumps({'catalog_schema':1,'profiles':[self.p]}))
    def archive(self,ident='translate.multi',texts=None,extra=()):
        path=self.path/(ident+'.zip');t=base.translation();t['texts']=texts if texts is not None else {'label':'CAB','second':'BAC'}
        with zipfile.ZipFile(path,'w') as z:
            z.writestr('mod.json',json.dumps(base.manifest(ident)));z.writestr('translations/es-ES.json',json.dumps(t,ensure_ascii=False))
            for name,data in extra:z.writestr(name,data)
        return path
    def enable(self,ident='translate.multi',texts=None):self.store.install(self.archive(ident,texts));self.store.change('enable',ident)
    def test_two_valid_fields(self):
        self.enable();items=self.store.replacements(self.store.installed()[0]);self.assertEqual([(x[1],x[2]) for x in items],[('label',b'CAB'),('second',b'BAC')])
    def test_invalid_second_install_atomic(self):
        with self.assertRaises(ValueError):self.store.install(self.archive(texts={'label':'CAB','second':'Ñ'}))
        self.assertEqual(self.store.installed(),[]);self.assertEqual(self.store.state()['enabled'],[])
    def test_invalid_second_enable_atomic(self):
        self.store.install(self.archive());f=self.store.packages/'translate.multi/translations/es-ES.json';t=base.translation();t['texts']={'label':'CAB','second':'ABCA'};f.write_text(json.dumps(t));before=self.store.state()
        with self.assertRaises(ValueError):self.store.change('enable','translate.multi')
        self.assertEqual(self.store.state(),before)
    def test_partial_conflict_atomic(self):
        self.enable('translate.single',{'label':'CAB'});self.store.install(self.archive());before=self.store.state()
        with self.assertRaisesRegex(ValueError,'disable it first'):self.store.change('enable','translate.multi')
        self.assertEqual(self.store.state(),before)
    def test_disjoint_owners(self):
        self.enable('translate.first',{'label':'CAB'});self.enable('translate.second',{'second':'BAC'});self.assertEqual(len(self.store.state()['enabled']),2)
    def test_disable_all_fields(self):
        self.enable();self.store.change('disable','translate.multi');self.assertEqual(self.store.state()['enabled'],[])
    def test_uninstall_active(self):
        self.enable();self.store.change('uninstall','translate.multi');self.assertEqual(self.store.installed(),[]);self.assertEqual(self.store.state()['enabled'],[])
    def test_restart_persistence_unicode_host_path(self):
        self.enable();again=pkg.Store(self.store.root,'0.9.0','own-fixture','a'*64,self.catalog);self.assertEqual(len(again.replacements(again.installed()[0])),2);again.change('disable','translate.multi');third=pkg.Store(self.store.root,'0.9.0','own-fixture','a'*64,self.catalog);self.assertFalse(third.installed()[0]['enabled'])
    def test_overlap_profile(self):
        self.p['resources']['second']['offset']=2049;self.save()
        with self.assertRaisesRegex(ValueError,'Overlapping'):r.catalog(self.catalog)
    def test_bank_out_of_range(self):
        self.p['resources']['second']['bank']=2;self.save()
        with self.assertRaises(ValueError):r.catalog(self.catalog)
    def test_instruction_second(self):
        with self.assertRaisesRegex(ValueError,'instruction'):r.validate_profile(self.p,[[32768,0x8810],[]])
    def test_operand_second(self):
        with self.assertRaisesRegex(ValueError,'operand'):r.validate_profile(self.p,[[32768,0x880e],[]])
    def test_wrong_revision_atomic(self):
        self.store.rom_sha256='b'*64
        with self.assertRaises(ValueError):self.store.install(self.archive())
        self.assertEqual(self.store.installed(),[])
    def test_lengths_and_unicode(self):
        for value in ('ABCA','Ñ','Á','¿','A\u0301',''):
            with self.subTest(value=value),self.assertRaises(ValueError):self.store.install(self.archive(texts={'label':'CAB','second':value}))
        self.assertEqual(self.store.installed(),[])
    def test_unknown_second(self):
        with self.assertRaises(ValueError):self.store.install(self.archive(texts={'label':'CAB','unknown':'ABC'}))
    def test_malformed_duplicate_json(self):
        for data in ('{', '{"schema":1,"schema":1}'):
            with self.assertRaises(ValueError):r.compile_translation(data.encode(),self.p)
    def test_duplicate_zip(self):
        with self.assertWarns(UserWarning):path=self.archive(extra=[('translations/es-ES.json','{}')])
        with self.assertRaises(ValueError):self.store.install(path)
    def test_windows_paths_and_zip_slip(self):
        for name in ('../escape','..\\escape','C:/escape','translations\\es-ES.json','/absolute','assets/test:stream','assets/CON'):
            with self.subTest(name=name),self.assertRaises(ValueError):self.store.install(self.archive(extra=[(name,'x')]))
        self.assertEqual(self.store.installed(),[])
    def test_install_no_execution(self):
        with self.assertRaises(ValueError):self.store.install(self.archive(extra=[('main.lua',"error('must not run')")]))
        self.assertEqual(self.store.installed(),[])
    def test_reviewed_catalog_additive(self):
        profile=r.catalog(base.ROOT/'tools/mods/resource_profiles.json')['addams-pal-inventory-text-v1']
        self.assertEqual(set(profile['resources']),{'inventory.heading','hud.life'})
        self.assertEqual(r.encode(profile,'hud.life','VIDA'),bytes([22,9,4,1]))
        self.assertEqual(r.encode(profile,'inventory.heading','OBJETOS'),bytes([15,2,10,5,20,15,19,0,0]))
        with self.assertRaises(ValueError):r.encode(profile,'hud.life','VIDAS')
    def test_source_and_v1_unchanged(self):
        before=self.catalog.read_bytes();self.enable();self.store.change('disable','translate.multi');self.assertEqual(self.catalog.read_bytes(),before)
        m=base.manifest();m['api_version']=1;m['entry']='main.lua'
        for key in ('permissions','target','translation'):del m[key]
        self.assertEqual(pkg.manifest(m)['api_version'],1)

if __name__=='__main__':unittest.main()
