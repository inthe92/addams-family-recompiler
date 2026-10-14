"""Completion02 portable data-only profiles and bounded persistence; no Lua execution."""
import copy,hashlib,json,subprocess,sys,tempfile,unittest,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/mods'));import packages
BASE=dict(id='test.first',name='Test mod',version='1.0.0',api_version=2,author='Tests',description='Owned fixture',entry='main.lua',permissions=[],target=dict(game_id='addams-family-pal-eu-enfrde',engine_min='0.9.0',engine_max_exclusive='1.0.0'))
SETTINGS=json.loads((ROOT/'examples/mods/configurable-jump/mod.json').read_text())['settings']
class Checks(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None);self.path=Path(self.temp.name).resolve();self.s=packages.Store(self.path/'store','0.9.0','addams-family-pal-eu-enfrde');self.add(settings=SETTINGS);self.s.change('enable','test.first');self.s.configure(json.dumps(dict(id='test.first',values=dict(extension=9))));self.s.profile('profile-save','Portable');self.file=self.path/'profile.json';self.s.export_profile(json.dumps(dict(name='Portable',file=str(self.file))));self.s.profile('profile-delete','Portable')
 def tearDown(self):self.temp.cleanup()
 def add(self,**fields):
  m=dict(copy.deepcopy(BASE),**fields);file=self.path/(m['id']+'.zip')
  with zipfile.ZipFile(file,'w') as z:z.writestr('mod.json',json.dumps(m));z.writestr('main.lua',"error('Inspection must never execute this')")
  self.s.install(file);return m['id']
 def data(self):return json.loads(self.file.read_text())
 def edit(self,func):
  d=self.data();func(d);self.file.write_text(json.dumps(d))
 def inspect(self):return self.s.inspect_profile(self.file)
 def import_file(self):
  _,_,sha=self.inspect();return self.s.import_profile(json.dumps(dict(file=str(self.file),sha256=sha)))
 def test_profile_defaults_snapshot(self):
  self.s.configure(json.dumps(dict(id='test.first',values={})));self.s.profile('profile-save','Default snapshot');self.s.configure(json.dumps(dict(id='test.first',values=dict(extension=10))));self.s.profile('profile-apply','Default snapshot');self.assertEqual(self.s.installed()[0]['configuration']['extension'],6)
 def test_unmentioned_disabled_config_preserved(self):
  self.add(id='test.other',settings=SETTINGS);self.s.configure(json.dumps(dict(id='test.other',values=dict(extension=11))));self.import_file();self.s.profile('profile-apply','Portable');other=next(m for m in self.s.installed() if m['id']=='test.other');self.assertEqual(other['configuration']['extension'],11)
 def test_export_only_portable_fields(self):
  d=self.data();self.assertEqual(set(d),{'schema','name','context','mods'});self.assertEqual(d['mods'][0]['settings']['extension'],dict(type='integer',value=9));self.assertNotIn(str(self.s.root),self.file.read_text());self.assertNotIn('main.lua',self.file.read_text())
 def test_import_inactive(self):
  before=copy.deepcopy(self.s.state());self.import_file();after=self.s.state();self.assertEqual(after['enabled'],before['enabled']);self.assertEqual(after['management']['active'],before['management']['active']);self.assertIn('Portable',after['management']['profiles'])
 def test_import_then_apply_restores_settings(self):
  self.import_file();self.s.configure(json.dumps(dict(id='test.first',values=dict(extension=2))));self.s.profile('profile-apply','Portable');self.assertEqual(self.s.installed()[0]['configuration']['extension'],9)
 def test_delete_preserves_packages_and_other_profiles(self):
  self.s.profile('profile-save','Other');self.import_file();before=self.s.state()['enabled'][:];self.s.profile('profile-delete','Portable');self.assertEqual(self.s.state()['enabled'],before);self.assertIn('Other',self.s.state()['management']['profiles']);self.assertEqual(len(self.s.installed()),1)
 def test_original_delete_forbidden(self):
  with self.assertRaises(ValueError):self.s.profile('profile-delete','Original')
 def test_export_refuses_overwrite(self):
  old=self.file.read_bytes();self.s.profile('profile-save','Portable')
  with self.assertRaises(ValueError):self.s.export_profile(json.dumps(dict(name='Portable',file=str(self.file))))
  self.assertEqual(old,self.file.read_bytes())
 def test_inspection_no_state_change(self):
  before=(self.s.root/'state.json').read_bytes();self.inspect();self.assertEqual(before,(self.s.root/'state.json').read_bytes())
 def test_duplicate_import_atomic(self):
  self.import_file();before=(self.s.root/'state.json').read_bytes()
  with self.assertRaises(ValueError):self.import_file()
  self.assertEqual(before,(self.s.root/'state.json').read_bytes())
 def test_changed_after_review_rejected(self):
  _,_,sha=self.inspect();self.edit(lambda d:d.update(name='Changed'));before=(self.s.root/'state.json').read_bytes()
  with self.assertRaises(ValueError):self.s.import_profile(json.dumps(dict(file=str(self.file),sha256=sha)))
  self.assertEqual(before,(self.s.root/'state.json').read_bytes())
 def test_missing_mod(self):
  self.edit(lambda d:d['mods'][0].update(id='test.absent'))
  with self.assertRaises(ValueError):self.inspect()
 def test_version_mismatch(self):
  self.edit(lambda d:d['mods'][0].update(version='2.0.0'))
  with self.assertRaises(ValueError):self.inspect()
 def test_api_mismatch(self):
  self.edit(lambda d:d['mods'][0].update(api_version=1))
  with self.assertRaises(ValueError):self.inspect()
 def test_persisted_version_lock(self):
  self.import_file();path=self.s.packages/'test.first/mod.json';d=json.loads(path.read_text());d['version']='1.1.0';path.write_text(json.dumps(d));before=self.s.state()
  with self.assertRaises(ValueError):self.s.profile('profile-apply','Portable')
  self.assertEqual(before,self.s.state())
 def test_engine_mismatch(self):
  self.edit(lambda d:d['context'].update(engine_version='0.9.1'))
  with self.assertRaises(ValueError):self.inspect()
 def test_game_mismatch(self):
  self.edit(lambda d:d['context'].update(game_id='another.game'))
  with self.assertRaises(ValueError):self.inspect()
 def test_schema_boolean_or_future_rejected(self):
  for v in (True,2,'1'):
   self.edit(lambda d:d.update(schema=v))
   with self.assertRaises(ValueError):self.inspect()
 def test_paths_not_profile_names(self):
  self.edit(lambda d:d.update(name='../../personal'))
  with self.assertRaises(ValueError):self.inspect()
 def test_duplicate_mod_ids(self):
  self.edit(lambda d:d['mods'].append(copy.deepcopy(d['mods'][0])))
  with self.assertRaises(ValueError):self.inspect()
 def test_unknown_fields(self):
  self.edit(lambda d:d.update(lua='execute me'))
  with self.assertRaises(ValueError):self.inspect()
 def test_unknown_setting(self):
  self.edit(lambda d:d['mods'][0]['settings'].update(unknown=dict(type='boolean',value=True)))
  with self.assertRaises(ValueError):self.inspect()
 def test_changed_type(self):
  self.edit(lambda d:d['mods'][0]['settings']['extension'].update(type='float'))
  with self.assertRaises(ValueError):self.inspect()
 def test_overflow_and_nan(self):
  for value in (13,float('nan'),float('inf'),True):
   self.edit(lambda d:d['mods'][0]['settings']['extension'].update(value=value))
   with self.assertRaises(ValueError):self.inspect()
 def test_invalid_enum(self):
  self.edit(lambda d:d['mods'][0]['settings']['position'].update(value='../file'))
  with self.assertRaises(ValueError):self.inspect()
 def test_linked_or_oversized_file(self):
  link=self.path/'linked.json';link.symlink_to(self.file)
  with self.assertRaises(ValueError):self.s.inspect_profile(link)
  self.file.write_bytes(b'x'*65537)
  with self.assertRaises(ValueError):self.inspect()
 def test_duplicate_json_key(self):
  self.file.write_text('{"schema":1,"schema":1}')
  with self.assertRaises(ValueError):self.inspect()
 def test_defaults_then_unchanged(self):
  self.assertTrue(self.s.configure(json.dumps(dict(id='test.first',values={}))));before=(self.s.root/'state.json').read_bytes();self.assertFalse(self.s.configure(json.dumps(dict(id='test.first',values={}))));self.assertEqual(before,(self.s.root/'state.json').read_bytes());self.assertEqual(self.s.installed()[0]['configuration']['extension'],6)
 def test_profile_name_case_ambiguity(self):
  self.s.profile('profile-save','Sample')
  with self.assertRaises(ValueError):self.s.profile('profile-save','sample')
  with self.assertRaises(ValueError):self.s.profile('profile-save','ORIGINAL')
 def test_global_corrupt_state_preserved(self):
  path=self.s.root/'state.json';path.write_text('{bad');old=path.read_bytes()
  with self.assertRaises(ValueError):self.s.installed()
  self.assertEqual(path.read_bytes(),old)
 def test_invalid_saved_configuration(self):
  state=self.s.state();state['management']['configs']['test.first']={'show':{'nested':'danger'}};packages.atomic_json(self.s.root/'state.json',state)
  with self.assertRaises(ValueError):self.s.state()
 def test_missing_dependency(self):
  self.add(id='test.dependency',dependencies={'test.absent':dict(min='1.0.0',max_exclusive='2.0.0')});d=self.data();d['mods'].append(dict(id='test.dependency',version='1.0.0',api_version=2,enabled=True,settings={}));self.file.write_text(json.dumps(d))
  with self.assertRaises(ValueError):self.inspect()
 def test_declared_conflict(self):
  self.add(id='test.conflicting',conflicts=['test.first']);d=self.data();d['mods'].append(dict(id='test.conflicting',version='1.0.0',api_version=2,enabled=True,settings={}));self.file.write_text(json.dumps(d))
  with self.assertRaises(ValueError):self.inspect()
 def test_import_quota_atomic(self):
  for i in range(16):self.s.profile('profile-save',f'Other {i}')
  before=(self.s.root/'state.json').read_bytes()
  with self.assertRaises(ValueError):self.import_file()
  self.assertEqual(before,(self.s.root/'state.json').read_bytes())
if __name__=='__main__':
 native=sys.argv.pop(1) if len(sys.argv)>1 else None
 result=unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(Checks))
 if not result.wasSuccessful():raise SystemExit(1)
 if native:
  with tempfile.TemporaryDirectory(dir='/private/tmp' if sys.platform=='darwin' else None) as tmp:
   p=Path(tmp).resolve();subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_phase5_examples.py'),str(p/'zips')],check=True);subprocess.run([sys.executable,'-B',str(ROOT/'tools/mods/make_completion_example.py'),str(p/'zips/Configurable-Jump-Example-v1.0.zip')],check=True)
   fixture=dict(copy.deepcopy(BASE),id='test.noop')
   with zipfile.ZipFile(p/'zips/noop-fixture.zip','w') as z:z.writestr('mod.json',json.dumps(fixture));z.writestr('main.lua',"function on_load() log.info('NOOP_LOAD') end; function on_unload() log.info('NOOP_UNLOAD') end")
   subprocess.run([native,str(p/'store & spaces'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(p/'zips')],check=True)
