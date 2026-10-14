"""Transactional settings/profiles and unchanged prior packages, no commercial content."""
import copy,hashlib,json,math,subprocess,sys,tempfile,unittest,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/mods'));import packages
BASE=dict(id='test.first',name='Test mod',version='1.0.0',api_version=2,author='Tests',description='Synthetic fixture',entry='main.lua',permissions=[],target=dict(game_id='addams-family-pal-eu-enfrde',engine_min='0.9.0',engine_max_exclusive='1.0.0'))
SETTINGS=json.loads((ROOT/'examples/mods/configurable-jump/mod.json').read_text())['settings']
class Checks(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None);self.path=Path(self.temp.name).resolve();self.store=packages.Store(self.path/'store','0.9.0','addams-family-pal-eu-enfrde')
 def tearDown(self):self.temp.cleanup()
 def install(self,**fields):
  m=dict(copy.deepcopy(BASE),**fields)
  if m['api_version']==1:m.pop('permissions',None);m.pop('target',None)
  z=self.path/(m['id']+'.zip')
  with zipfile.ZipFile(z,'w') as f:f.writestr('mod.json',json.dumps(m));f.writestr('main.lua',"error('Never execute during inspection')")
  self.store.install(z);return m['id']
 def test_legacy_state_migration(self):
  packages.atomic_json(self.store.root/'state.json',{'enabled':[],'errors':{}});self.install();self.store.profile('profile-apply','Original');self.assertEqual(self.store.state()['management']['schema'],1)
 def test_install_disabled_no_execution(self):
  self.install(settings=SETTINGS);self.assertFalse(self.store.installed()[0]['enabled'])
 def test_four_typed_settings_defaults(self):
  self.install(settings=SETTINGS);self.assertEqual(self.store.installed()[0]['configuration'],dict(extension=6,show=True,opacity=.7,position='bottom'))
 def test_config_persistence(self):
  key=self.install(settings=SETTINGS);self.store.configure(json.dumps(dict(id=key,values=dict(extension=9,show=False,opacity=.3,position='top'))));again=packages.Store(self.store.root,'0.9.0','addams-family-pal-eu-enfrde');self.assertEqual(again.installed()[0]['configuration']['extension'],9)
 def test_numeric_out_of_bounds_atomic(self):
  key=self.install(settings=SETTINGS);before=self.store.state()
  for v in (-1,13,True,1.5,float('nan'),float('inf')):
   with self.assertRaises(ValueError):self.store.configure(json.dumps(dict(id=key,values=dict(extension=v))))
   self.assertEqual(self.store.state(),before)
 def test_boolean_type(self):
  key=self.install(settings=SETTINGS)
  with self.assertRaises(ValueError):self.store.configure(json.dumps(dict(id=key,values=dict(show=1))))
 def test_unknown_setting(self):
  key=self.install(settings=SETTINGS)
  with self.assertRaises(ValueError):self.store.configure(json.dumps(dict(id=key,values={'../../file':0})))
 def test_enum_invalid(self):
  key=self.install(settings=SETTINGS)
  with self.assertRaises(ValueError):self.store.configure(json.dumps(dict(id=key,values=dict(position='secret'))))
 def test_schema_unknown_fields(self):
  d=copy.deepcopy(BASE);d['settings']=copy.deepcopy(SETTINGS);d['settings']['extension']['path']='/tmp/no'
  with self.assertRaises(ValueError):packages.manifest(d)
 def test_settings_quota(self):
  d=copy.deepcopy(BASE);d['settings']={f'field{i}':SETTINGS['show'] for i in range(9)}
  with self.assertRaises(ValueError):packages.manifest(d)
 def test_missing_dependency_atomic(self):
  key=self.install(dependencies={'test.absent':{'min':'1.0.0','max_exclusive':'2.0.0'}});before=self.store.state()
  with self.assertRaises(ValueError):self.store.change('enable',key)
  self.assertEqual(self.store.state(),before)
 def test_version_dependency(self):
  first=self.install();second=self.install(id='test.second',dependencies={first:{'min':'2.0.0','max_exclusive':'3.0.0'}});self.store.change('enable',first)
  with self.assertRaises(ValueError):self.store.change('enable',second)
 def test_disable_dependency_rejected(self):
  first=self.install();second=self.install(id='test.second',dependencies={first:{'min':'1.0.0','max_exclusive':'2.0.0'}});self.store.change('enable',first);self.store.change('enable',second)
  with self.assertRaises(ValueError):self.store.change('disable',first)
 def test_dependency_failure_cascade(self):
  first=self.install();second=self.install(id='test.second',dependencies={first:{'min':'1.0.0','max_exclusive':'2.0.0'}});self.store.change('enable',first);self.store.change('enable',second);self.store.change('disable',first,'Lua failed');self.assertEqual(self.store.state()['enabled'],[]);self.assertIn(second,self.store.state()['errors'])
 def test_conflict_both_directions(self):
  first=self.install(conflicts=['test.second']);second=self.install(id='test.second');self.store.change('enable',second)
  with self.assertRaises(ValueError):self.store.change('enable',first)
 def test_profile_missing_package_no_partial_write(self):
  key=self.install();self.store.change('enable',key);self.store.profile('profile-save','Test Profile');self.store.change('uninstall',key);before=self.store.state()
  with self.assertRaises(ValueError):self.store.profile('profile-apply','Test Profile')
  self.assertEqual(self.store.state(),before)
 def test_original_keeps_installations(self):
  key=self.install();self.store.change('enable',key);self.store.profile('profile-apply','Original');self.assertEqual(self.store.state()['enabled'],[]);self.assertEqual(len(self.store.installed()),1)
 def test_profile_configs_restored(self):
  key=self.install(settings=SETTINGS);self.store.change('enable',key);self.store.configure(json.dumps(dict(id=key,values=dict(extension=9))));self.store.profile('profile-save','Gameplay');self.store.configure(json.dumps(dict(id=key,values=dict(extension=2))));self.store.profile('profile-apply','Gameplay');self.assertEqual(self.store.installed()[0]['configuration']['extension'],9)
 def test_profile_rename(self):
  self.store.profile('profile-save','Old');self.store.profile('profile-rename',json.dumps(dict(old='Old',name='New')));self.assertEqual(self.store.state()['management']['active'],'New')
 def test_original_name_reserved(self):
  with self.assertRaises(ValueError):self.store.profile('profile-save','Original')
 def test_profile_quota(self):
  for i in range(16):self.store.profile('profile-save',f'Profile {i}')
  with self.assertRaises(ValueError):self.store.profile('profile-save','Overflow')
 def test_profile_paths_forbidden(self):
  with self.assertRaises(ValueError):self.store.profile('profile-save','../../private')
 def test_v1_compatibility(self):
  m={k:v for k,v in BASE.items() if k not in ('permissions','target')};m['api_version']=1;self.install(**m);self.assertEqual(self.store.installed()[0]['api_version'],1)
 def test_damaged_package_does_not_block_other(self):
  first=self.install();second=self.install(id='test.second');(self.store.packages/first/'mod.json').write_text('{bad')
  self.store.change('enable',second);self.assertTrue(next(m for m in self.store.installed() if m['id']==second)['enabled']);self.assertTrue(next(m for m in self.store.installed() if m['id']==first)['_invalid'])
 def test_damaged_package_can_be_uninstalled(self):
  key=self.install();(self.store.packages/key/'mod.json').write_text('{bad');self.store.change('uninstall',key);self.assertEqual(self.store.installed(),[])
 def test_damaged_package_cannot_enable(self):
  key=self.install();(self.store.packages/key/'main.lua').unlink()
  with self.assertRaises(ValueError):self.store.change('enable',key)
 def test_dependency_cycle_rejected(self):
  a=self.install(dependencies={'test.second':dict(min='1.0.0',max_exclusive='2.0.0')});b=self.install(id='test.second',dependencies={a:dict(min='1.0.0',max_exclusive='2.0.0')});records={m['id']:m for m in self.store.installed()}
  with self.assertRaisesRegex(ValueError,'Cyclic'):self.store.validate_selection(records,{a,b})
 def test_profile_conflict_atomic(self):
  a=self.install(conflicts=['test.second']);b=self.install(id='test.second');s=self.store.state();mg=self.store.management(s);mg['profiles']['Bad']={'enabled':[a,b],'configs':{}};packages.atomic_json(self.store.root/'state.json',s);before=(self.store.root/'state.json').read_bytes()
  with self.assertRaises(ValueError):self.store.profile('profile-apply','Bad')
  self.assertEqual((self.store.root/'state.json').read_bytes(),before)
 def test_rename_collision(self):
  self.store.profile('profile-save','One');self.store.profile('profile-save','Two')
  with self.assertRaises(ValueError):self.store.profile('profile-rename',json.dumps(dict(old='One',name='Two')))
 def test_state_quota_does_not_overwrite(self):
  self.store.profile('profile-apply','Original');before=(self.store.root/'state.json').read_bytes()
  with self.assertRaises(ValueError):packages.atomic_json(self.store.root/'state.json',{'large':'x'*65536})
  self.assertEqual((self.store.root/'state.json').read_bytes(),before)
 def test_unchanged_three_zips(self):
  subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_phase5_examples.py'),str(self.path/'zips')],check=True,stdout=subprocess.DEVNULL)
  hashes={'Super-Gomez-Gameplay-v1.0.zip':'d1114dbd01484f782455d8f5b218cd01a13546164a2fbd04bd190329bb35f3e4','Speedrun-HUD-v1.0.zip':'ef3ff4587ae92dc15c11433b3f1bebc5d3acc0f78cddb9740302dc81c455397d','Mansion-Parallax-Demo-v1.0.zip':'88ebf414b1cfdc8d94d7dfc7a4985e409f65a924b48291b882622712e7428571'}
  for name,sha in hashes.items():
   self.assertEqual(hashlib.sha256((self.path/'zips'/name).read_bytes()).hexdigest(),sha);self.store.install(self.path/'zips'/name)
  for m in self.store.installed():self.store.change('enable',m['id'])
  self.assertEqual(len(self.store.state()['enabled']),3)
if __name__=='__main__':
 native=sys.argv.pop(1) if len(sys.argv)>1 else None
 result=unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(Checks))
 if not result.wasSuccessful():raise SystemExit(1)
 if native:
  with tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None) as tmp:
   p=Path(tmp).resolve();subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_phase5_examples.py'),str(p/'zips')],check=True)
   subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_completion_example.py'),str(p/'config.zip')],check=True)
   bad=dict(copy.deepcopy(BASE),id='test.bad-unload',settings={'flag':dict(type='boolean',label='Flag',default=False)})
   with zipfile.ZipFile(p/'zips/bad-unload.zip','w') as z:z.writestr('mod.json',json.dumps(bad));z.writestr('main.lua',"function on_unload() error('Synthetic unload failure') end")
   subprocess.run([native,str(p/'native store with spaces'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(p/'zips'),str(p/'config.zip')],check=True)
