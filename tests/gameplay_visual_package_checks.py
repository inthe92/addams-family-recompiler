"""Own PNG/ZIP adversarial checks and actual native Manager integration."""
from pathlib import Path
import sys,tempfile,subprocess,importlib.util,json,zipfile,unittest,struct,zlib
ROOT=Path(__file__).resolve().parents[1]
def module(name,file):
 s=importlib.util.spec_from_file_location(name,ROOT/file);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
P=module('p','tools/mods/packages.py');M=module('m','tools/mods/make_phase5_examples.py')
class Packages(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name).resolve();M.build(self.root);self.s=P.Store(self.root/'store',engine_version='0.9.0',game_id='addams-family-pal-eu-enfrde')
 def tearDown(self):self.tmp.cleanup()
 def test_png(self):
  png=M.mansion();w,h,raw=P.image_tools.decode(png);self.assertEqual((w,h,len(raw)),(64,32,8192))
  for bad in (b'bad',png[:-1],png+b'x',png[:25]+bytes([0])+png[26:],png[:16]+struct.pack('>I',129)+png[20:]):
   with self.assertRaises(ValueError):P.image_tools.decode(bad)
 def test_install_never_executes(self):
  self.s.install(self.root/'Mansion-Parallax-Demo-v1.0.zip');self.assertFalse(self.s.installed()[0]['enabled']);self.assertEqual(len(self.s.images(self.s.installed()[0])),1)
 def test_conflict_and_persistence(self):
  z=self.root/'Super-Gomez-Gameplay-v1.0.zip';self.s.install(z);self.s.change('enable','phase5.super-gomez-gameplay','')
  with zipfile.ZipFile(z) as src,zipfile.ZipFile(self.root/'second.zip','w') as dest:
   for name in src.namelist():
    data=src.read(name)
    if name=='mod.json':m=json.loads(data);m['id']='phase5.second';data=json.dumps(m).encode()
    dest.writestr(name,data)
  self.s.install(self.root/'second.zip')
  with self.assertRaises(ValueError):self.s.change('enable','phase5.second','')
  self.assertTrue(self.s.installed()[0]['enabled'] or self.s.installed()[1]['enabled'])
  self.s.change('disable','phase5.super-gomez-gameplay','');self.s.change('enable','phase5.second','')
 def test_invalid_manifest(self):
  m=json.loads((ROOT/'examples/mods/mansion-parallax-demo/mod.json').read_text())
  for replace in ({'permissions':None},{'images':{'a':'../bad.png'}},{'images':{'a':'assets/a.png','b':'assets/a.png'}},{'permissions':['unknown']}):
   with self.assertRaises(ValueError):P.manifest({**m,**replace})
 def test_malformed_zip(self):
  for i,name in enumerate(('../bad.png','assets/missing.png','assets/MANSION.PNG')):
   with zipfile.ZipFile(self.root/'Mansion-Parallax-Demo-v1.0.zip') as src,zipfile.ZipFile(self.root/f'bad{i}.zip','w') as dst:
    for p in src.namelist():dst.writestr(p,src.read(p))
    dst.writestr(name,b'invalid')
   with self.assertRaises(ValueError):self.s.install(self.root/f'bad{i}.zip')
 def test_filters(self):
  # Constant four-byte pixels, encoded with all five legal filters.
  for filt in range(5):
   raw=bytes([30,40,50,255]*2);encoded=bytearray()
   for row in range(2):
    encoded.append(filt)
    for i,v in enumerate(raw):
     a=raw[i-4] if i>=4 else 0;b=raw[i] if row else 0;c=raw[i-4] if row and i>=4 else 0
     pred=0 if filt==0 else a if filt==1 else b if filt==2 else (a+b)//2 if filt==3 else (a if abs(b-c)<=abs(a-c) and abs(b-c)<=abs(a+b-2*c) else b if abs(a-c)<=abs(a+b-2*c) else c)
     encoded.append((v-pred)&255)
   def chunk(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d))
   png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',2,2,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(encoded))+chunk(b'IEND',b'')
   self.assertEqual(P.image_tools.decode(png)[2],raw*2)
 def test_diagnostic_wiring(self):
  source=(ROOT/'app/desktop.cpp').read_text()
  self.assertEqual(source.count('diagnostic.frame(cpu,god.enabled(),god.all_items(),gameplay_assistance())'),3)
  self.assertIn('managed_assistance.observe(!mod_path.empty(),managed_mods.jump_extension())',source)
 def test_visual_quota(self):
  for i in range(9):
   with zipfile.ZipFile(self.root/'Speedrun-HUD-v1.0.zip') as src,zipfile.ZipFile(self.root/f'visual-{i}.zip','w') as dest:
    for name in src.namelist():
     data=src.read(name)
     if name=='mod.json':m=json.loads(data);m['id']=f'phase5.visual-{i}';data=json.dumps(m).encode()
     dest.writestr(name,data)
   self.s.install(self.root/f'visual-{i}.zip')
   if i<8:self.s.change('enable',f'phase5.visual-{i}','')
   else:
    with self.assertRaises(ValueError):self.s.change('enable',f'phase5.visual-{i}','')
 def test_native_store(self):
  for filename,ident in (('conflict.zip','phase5.conflict'),('failed.zip','phase5.failed')):
   with zipfile.ZipFile(self.root/'Super-Gomez-Gameplay-v1.0.zip') as src,zipfile.ZipFile(self.root/filename,'w') as dest:
    for name in src.namelist():
     data=src.read(name)
     if name=='mod.json':m=json.loads(data);m['id']=ident;data=json.dumps(m).encode()
     if name=='main.lua' and ident=='phase5.failed':data=b"function on_load()game.player.set_modifier('jump_extension',12)end;function on_update()error('intentional own fixture')end"
     dest.writestr(name,data)
  if len(sys.argv)>1 and Path(sys.argv[1]).is_file():subprocess.run([sys.argv[1],str(self.root/'native store'),sys.executable,str(ROOT/'tools/mods/packages.py'),str(self.root)],check=True)
if __name__=='__main__':unittest.main(argv=[sys.argv[0]])
