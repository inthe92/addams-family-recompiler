"""Synthetic ZIP, transaction and persistent-state regressions. No ROM."""
import importlib.util
import json
from pathlib import Path
import stat
import tempfile
import unittest
from unittest import mock
import zipfile
import zlib
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('mod_packages', ROOT/'tools/mods/packages.py')
packages = importlib.util.module_from_spec(spec); spec.loader.exec_module(packages)

class Packages(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir="/private/tmp" if __import__("sys").platform=="darwin" else None); self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name); self.store = packages.Store(self.base/'store')
        self.manifest = dict(id='test.hello',name='Test',version='1.0.0',api_version=1,description='Own test data',author='Tests',entry='main.lua')
        self.counter = 0
    def archive(self, extras=(), data=None):
        self.counter += 1; p = self.base/('input%d.zip'%self.counter)
        with zipfile.ZipFile(p,'w',zipfile.ZIP_DEFLATED) as z:
            z.writestr('mod.json',json.dumps(data or self.manifest)); z.writestr('main.lua','function on_update(dt) end')
            for name, contents in extras:
                if isinstance(name, str):
                    # Preserve malformed raw archive names on Windows too;
                    # ZipInfo normally sanitizes separators/NUL at construction.
                    info = zipfile.ZipInfo(name)
                    info.filename = info.orig_filename = name
                    info.compress_type = zipfile.ZIP_DEFLATED
                    name = info
                z.writestr(name, contents)
        return p
    def reject(self, extras=(), data=None):
        with self.assertRaises((ValueError,zipfile.BadZipFile)): self.store.install(self.archive(extras,data))
        self.assertEqual(self.store.installed(),[])
        self.assertEqual(list(self.store.root.glob('.install-*')),[])
    def test_install_disabled_by_default(self):
        packages.atomic_json(self.store.root/'state.json',{'enabled':['test.hello'],'errors':{}})
        self.store.install(self.archive()); self.assertFalse(self.store.installed()[0]['enabled'])
    def test_enable_persists_and_disable(self):
        self.store.install(self.archive()); self.store.change('enable','test.hello')
        self.assertTrue(packages.Store(self.store.root).installed()[0]['enabled'])
        self.store.change('disable','test.hello'); self.assertFalse(self.store.installed()[0]['enabled'])
    def test_uninstall_keeps_other_files(self):
        marker=self.store.root/'personal.txt'; marker.write_text('keep')
        self.store.install(self.archive()); self.store.change('uninstall','test.hello')
        self.assertEqual(marker.read_text(),'keep'); self.assertEqual(self.store.installed(),[])
    def test_previous_install_not_overwritten(self):
        self.store.install(self.archive()); before=(self.store.packages/'test.hello/main.lua').read_bytes()
        with self.assertRaises(ValueError): self.store.install(self.archive())
        self.assertEqual((self.store.packages/'test.hello/main.lua').read_bytes(),before)
    def test_traversal(self):
        for name in ('../outside.txt','assets/../../bad.txt','/bad.txt','assets//bad.txt','assets/./bad.txt','assets/good.txt\0hidden'):
            self.reject([(name,'bad')])
    def test_windows_paths(self):
        for name in ('C:/bad.txt','assets\\bad.txt','assets/CON.txt','assets/LPT1.txt','assets/file.txt:stream','assets/end.','assets/end '): self.reject([(name,'bad')])
        archive = self.archive([('assets\\bad.txt', 'bad')])
        base_info = zipfile.ZipInfo
        class WindowsNormalizedInfo(base_info):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.filename = self.filename.replace('\\', '/')
        # Reproduce the reader's Windows normalization on every host. Keep the
        # raw ZIP and rejection contract, rather than accepting a sanitized name.
        with mock.patch.object(zipfile, 'ZipInfo', WindowsNormalizedInfo):
            with self.assertRaises(ValueError): self.store.install(archive)
        self.assertEqual(self.store.installed(), [])
    def test_case_duplicate(self): self.reject([('assets/test.txt','a'),('assets/TEST.txt','b')])
    def test_exact_duplicate(self): self.reject([('main.lua','replace')])
    def test_links(self):
        info=zipfile.ZipInfo('assets/link.txt');info.create_system=3;info.external_attr=(stat.S_IFLNK|0o777)<<16
        self.reject([(info,'../../bad')])
    def test_directory_collision(self): self.reject([('assets/a.txt','a'),('assets/a.txt/sub.txt','b')])
    def test_executables(self):
        for name in ('evil.exe','assets/evil.dll','assets/a.py','assets/x.so','assets/run.sh'): self.reject([(name,b'MZ')])
    def test_bad_api(self): self.reject(data=dict(self.manifest,api_version=2))
    def test_bad_fields(self):
        for data in (dict(self.manifest,id='../bad'),dict(self.manifest,id='com1.mod'),dict(self.manifest,id='ending.'),dict(self.manifest,version='latest'),dict(self.manifest,entry='../main.lua'),dict(self.manifest,api_version=True),dict(self.manifest,unknown=1)):
            self.reject(data=data)
    def test_malformed_zip(self):
        f=self.base/'bad.zip';f.write_bytes(b'not a ZIP')
        with self.assertRaises(zipfile.BadZipFile): self.store.install(f)
    def test_script_limit(self):
        self.reject([('assets/big.txt',b'0'*(packages.MAX_TOTAL+1))])
        p=self.base/'large-script.zip'
        with zipfile.ZipFile(p,'w',zipfile.ZIP_DEFLATED) as z:
            z.writestr('mod.json',json.dumps(self.manifest));z.writestr('main.lua',' '*(packages.MAX_SCRIPT+1))
        with self.assertRaises(ValueError):self.store.install(p)
        self.assertEqual(self.store.installed(),[])
    def test_file_count(self): self.reject([('assets/%d.txt'%n,'x') for n in range(packages.MAX_FILES)])
    def test_crc_rollback(self):
        info=zipfile.ZipInfo('assets/check.txt');info.compress_type=zipfile.ZIP_STORED
        p=self.archive([(info,'checksum marker')]);data=bytearray(p.read_bytes());data[data.index(b'checksum marker')]^=0xff;p.write_bytes(data)
        with self.assertRaises((ValueError,zipfile.BadZipFile,UnicodeDecodeError,zlib.error)):self.store.install(p)
        self.assertEqual(self.store.installed(),[])
        self.assertEqual(list(self.store.root.glob('.install-*')),[])
    def test_duplicate_json(self):
        with self.assertRaises(ValueError):packages.read_json('{"id":1,"id":2}')
    def test_root_link(self):
        link=self.base/'link';link.symlink_to(self.store.root,target_is_directory=True)
        with self.assertRaises(ValueError):packages.Store(link)
    def test_busy_lock(self):
        (self.store.root/'.transaction-lock').mkdir()
        with self.assertRaises(ValueError):self.store.install(self.archive())
    def test_failure_deactivation_persists(self):
        self.store.install(self.archive());self.store.change('enable','test.hello');self.store.change('disable','test.hello','budget exceeded')
        row=self.store.installed()[0];self.assertFalse(row['enabled']);self.assertEqual(row['error'],'budget exceeded')
    def test_assets_are_passive(self):
        self.store.install(self.archive([('assets/textures/own.png',b'own synthetic asset')]))
        self.assertEqual((self.store.packages/'test.hello/assets/textures/own.png').read_bytes(),b'own synthetic asset')
    def test_disguised_native_files(self):
        for data in (b'MZpretend PNG',b'\x7fELFpretend text',b'\xcf\xfa\xed\xfeMach-O',b'#!/bin/sh\necho bad'):
            self.reject([('assets/fake.png',data)])
    def test_binary_lua(self):
        p=self.base/'bytecode.zip'
        with zipfile.ZipFile(p,'w') as z:
            z.writestr('mod.json',json.dumps(self.manifest));z.writestr('main.lua',b'\x1bLua')
        with self.assertRaises(ValueError):self.store.install(p)
        self.assertEqual(self.store.installed(),[])
    def test_expansion_ratio(self): self.reject([('assets/bomb.txt',b'A'*(2*1024*1024))])

if __name__=='__main__': unittest.main()
