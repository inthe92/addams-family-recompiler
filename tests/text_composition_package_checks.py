"""Authored schema2 visual resource transactions; no game/ROM fixtures."""
import copy,json,unittest,zipfile
from mod_resource_checks import fixture,manifest,translation,ROOT,pkg,r
class Checks(unittest.TestCase):
    def setUp(self):
        import tempfile
        from pathlib import Path
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.path=Path(self.tmp.name).resolve();self.p,self.catalog=fixture(self.path)
        self.p['profile_schema']=2;self.p['visual_contexts']={'label':{'backend':'nes-background-cell-v1','chr_bank':1,'nametable':0x20a0,'mirroring':0}}
        self.catalog.write_text(json.dumps({'catalog_schema':1,'profiles':[self.p]}))
        self.store=pkg.Store(self.path/'store','0.9.0','own-fixture','a'*64,self.catalog)
        self.t=translation('CÑA');self.t.update(schema=2,glyphs={'Ñ':{'file':'assets/enye.chr','fallback':'B'}})
        self.tile=(ROOT/'examples/resources/chr-research/own-enye-8x8.chr').read_bytes()
    def archive(self,t=None,tile=None,m=None,extra=()):
        f=self.path/'mod.zip'
        with zipfile.ZipFile(f,'w') as z:
            z.writestr('mod.json',json.dumps(m or manifest()));z.writestr('translations/es-ES.json',json.dumps(t or self.t,ensure_ascii=False))
            z.writestr('assets/enye.chr',self.tile if tile is None else tile)
            for n,v in extra:z.writestr(n,v)
        return f
    def bad(self,t=None,tile=None,m=None,extra=()):
        with self.assertRaises(ValueError):self.store.install(self.archive(t,tile,m,extra))
        self.assertEqual(self.store.installed(),[])
    def test_install_disabled_persist_restore(self):
        self.store.install(self.archive());self.assertEqual(self.store.state()['enabled'],[]);self.store.change('enable','translate.one')
        fresh=pkg.Store(self.path/'store','0.9.0','own-fixture','a'*64,self.catalog);self.assertTrue(fresh.installed()[0]['enabled']);self.assertEqual(fresh.replacements(fresh.installed()[0])[0][2],b'CBA');fresh.change('disable','translate.one');self.assertEqual(fresh.state()['enabled'],[])
    def test_bad_sizes(self):
        for n in (0,15,17,1024):
            with self.subTest(n=n):self.bad(tile=b'x'*n)
    def test_missing(self):
        t=copy.deepcopy(self.t);t['glyphs']['Ñ']['file']='assets/missing.chr';self.bad(t)
    def test_undeclared(self):self.bad(extra=[('assets/extra.chr',self.tile)])
    def test_duplicate(self):self.bad(extra=[('assets/ENYE.chr',self.tile)])
    def test_alternate_case(self):
        t=copy.deepcopy(self.t);t['glyphs']['Ñ']['file']='assets/Enye.chr';self.bad(t)
    def test_path_traversal(self):
        t=copy.deepcopy(self.t);t['glyphs']['Ñ']['file']='../enye.chr';self.bad(t)
    def test_unknown_permission(self):
        m=manifest();m['permissions'].append('resources.chr');self.bad(m=m)
    def test_bad_fallback(self):
        t=copy.deepcopy(self.t);t['glyphs']['Ñ']['fallback']='D';self.bad(t)
    def test_invalid_unicode(self):
        t=copy.deepcopy(self.t);t['glyphs']={'\ud800':t['glyphs']['Ñ']};self.bad(t)
    def test_unrepresentable_text(self):
        t=copy.deepcopy(self.t);t['texts']['label']='CÁA';self.bad(t)
    def test_unknown_field(self):
        t=copy.deepcopy(self.t);t['chr_address']=0x1070;self.bad(t)
    def test_wrong_resource(self):
        t=copy.deepcopy(self.t);t['texts']={'unknown':'Ñ'};self.bad(t)
    def test_unused(self):
        t=copy.deepcopy(self.t);t['texts']['label']='ABC';self.bad(t)
    def test_no_lua_execution(self):self.bad(extra=[('main.lua',"error('must not execute')")])
    def test_wrong_context(self):
        for key,value in [('nametable',0x23ff),('chr_bank',32),('mirroring',True),('backend','arbitrary')]:
            p=copy.deepcopy(self.p);p['visual_contexts']['label'][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):r.validate_profile(p)
    def test_no_approved_context(self):
        p=copy.deepcopy(self.p);p['profile_schema']=1;p.pop('visual_contexts')
        with self.assertRaises(ValueError):r.compile_translation(json.dumps(self.t,ensure_ascii=False).encode(),p,{'assets/enye.chr':self.tile})
    def test_v1_and_prg_still_valid(self):
        self.assertEqual(r.encode(self.p,'label','ABC'),b'ABC');self.assertEqual(r.compile_translation(json.dumps(translation()).encode(),self.p),[('label',b'CAB')])
if __name__=='__main__':unittest.main()
