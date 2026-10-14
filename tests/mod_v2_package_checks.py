"""Version/permission/profile negotiation with own packages; never executes Lua."""
from pathlib import Path
import importlib.util,json,tempfile,unittest,zipfile,sys
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('packages',root/'tools/mods/packages.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
class V2(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None);self.addCleanup(self.tmp.cleanup);self.base=Path(self.tmp.name)
        self.store=p.Store(self.base/'store','0.9.0','addams-family-pal-eu-enfrde')
        self.manifest=json.loads((root/'examples/mods/gomez-debug-hud/mod.json').read_text())
    def zip(self,m):
        path=self.base/'input.zip'
        with zipfile.ZipFile(path,'w') as z:z.writestr('mod.json',json.dumps(m));z.writestr('main.lua',"error('must not run at install')")
        return path
    def test_v1_and_v2(self):
        v1={k:v for k,v in self.manifest.items() if k not in ('permissions','target')};v1['api_version']=1;v1['id']='own.v1'
        self.store.install(self.zip(v1));self.store.install(self.zip(self.manifest));self.assertEqual([m['enabled'] for m in self.store.installed()],[False,False])
    def test_engine_min_max(self):
        for current,allowed in [('0.8.9',False),('0.9.0',True),('0.9.99',True),('0.10.0',False),('1.0.0',False)]:
            s=p.Store(self.base/current,current,self.manifest['target']['game_id'])
            if allowed:s.compatible(p.manifest(self.manifest))
            else:
                with self.assertRaises(ValueError):s.compatible(self.manifest)
    def test_game_and_context(self):
        for engine,game in [(None,None),('0.9.0','other'),('wrong','addams-family-pal-eu-enfrde')]:
            with self.assertRaises(ValueError):p.Store(self.base/'other',engine,game).install(self.zip(self.manifest))
    def test_permissions(self):
        for permissions in [['write_ram'],['overlay.text','overlay.text'],'overlay.text',{},[True]]:
            with self.assertRaises(ValueError):self.store.install(self.zip(dict(self.manifest,permissions=permissions)))
    def test_api_schema_versions(self):
        for m in [dict(self.manifest,api_version=True),dict(self.manifest,api_version=3),dict(self.manifest,api_version=1),dict(self.manifest,dependencies=[]),dict(self.manifest,target=dict(self.manifest['target'],engine_min='0.10.0')),dict(self.manifest,target=dict(self.manifest['target'],engine_min='0.9.0-beta'))]:
            with self.assertRaises(ValueError):p.manifest(m)
    def test_incompatible_recovery(self):
        self.store.install(self.zip(self.manifest));self.store.change('enable',self.manifest['id'])
        other=p.Store(self.base/'store','1.0.0','other-game')
        self.assertEqual(len(other.installed()),1)
        with self.assertRaises(ValueError):other.change('enable',self.manifest['id'])
        other.change('disable',self.manifest['id']);other.change('uninstall',self.manifest['id']);self.assertEqual(other.installed(),[])
if __name__=='__main__':unittest.main()
