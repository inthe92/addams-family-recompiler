"""Build/source identity and stale-package rejection; no ROM or hardware oracle."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/public'))
import build_identity as identity
sys.path.insert(0,str(ROOT/'tests'))
from linux_appimage_checks import checked_rejection


class IdentityChecks(unittest.TestCase):
    def test_linux_crash_cannot_pass_as_import_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            cache=Path(directory);log=cache/'import.log';log.write_text('ROM incompatible: synthetic fixture\n')
            ok=subprocess.CompletedProcess([],1,'','ROM incompatible: synthetic fixture')
            checked_rejection(ok,cache)
            for status in [-11,0,2,139]:
                with self.subTest(status=status),self.assertRaises(RuntimeError):
                    checked_rejection(subprocess.CompletedProcess([],status,'','import failure'),cache)
            log.unlink()
            with self.assertRaises(RuntimeError):checked_rejection(ok,cache)

    def sample(self):
        return dict(schema=1,project='NES Native Lab',version='0.2.1',commit='a'*40,
                    dirty=False,source_sha256='b'*64,abi=3,platform='Linux',arch='x86_64')

    def test_exact_clean_commit_required(self):
        data=self.sample();self.assertEqual(identity.validate(data,'a'*40),data)
        for key,value in [('commit','c'*40),('dirty',True),('commit',None)]:
            changed={**data,key:value}
            with self.assertRaises(ValueError):identity.validate(changed,'a'*40)

    def test_invalid_identity_is_rejected(self):
        for key,value in [('abi',0),('abi',True),('version','RC'),('arch','i386'),
                          ('source_sha256','invalid'),('schema',2)]:
            with self.subTest(key=key),self.assertRaises(ValueError):
                identity.validate({**self.sample(),key:value})

    def test_stale_build_and_foreign_architecture_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            build=Path(directory);data=self.sample();(build/'native-build-info.json').write_text(json.dumps(data))
            with patch.object(identity,'current',return_value=data):identity.prepared(build,'Linux','x86_64')
            for key,value in [('source_sha256','c'*64),('commit','c'*40),('abi',2),('arch','arm64')]:
                with self.subTest(key=key),patch.object(identity,'current',return_value={**data,key:value}),self.assertRaises(ValueError):
                    identity.prepared(build,'Linux','x86_64')

    def test_binary_and_header_must_match_package(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'include/recomp').mkdir(parents=True);data=self.sample()
            (root/'PACKAGE_INFO.json').write_text(json.dumps(data));header=root/'include/recomp/module_abi.h'
            header.write_text('#define NES_RECOMP_ABI_VERSION 3u\n')
            run=lambda command:json.dumps(data)
            identity.packaged(root,root/'frontend','a'*40,run)
            with self.assertRaises(ValueError):
                identity.packaged(root,root/'frontend',run=lambda command:json.dumps({**data,'commit':'c'*40}))
            header.write_text('#define NES_RECOMP_ABI_VERSION 2u\n')
            with self.assertRaises(ValueError):identity.packaged(root,root/'frontend',run=run)

    def test_native_guard_inventory_uses_real_existing_targets(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);path=root/'ctest-inventory.json'
            path.write_text(json.dumps({'tests':[{'name':'module_abi_contract','command':['guard','missing','old','v2','current']}]}))
            self.assertEqual(identity.guard_inventory(root),['guard','missing','old','v2'])
            path.write_text(json.dumps({'tests':[]}))
            with self.assertRaises(ValueError):identity.guard_inventory(root)

    def test_text_fingerprint_is_independent_of_crlf(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'app').mkdir();file=root/'app/example.cpp';file.write_bytes(b'one\ntwo\n')
            with patch.object(identity,'git',side_effect=OSError):first=identity.source_identity(root)
            file.write_bytes(b'one\r\ntwo\r\n')
            with patch.object(identity,'git',side_effect=OSError):second=identity.source_identity(root)
            self.assertEqual(first,second)
            file.write_bytes(b'changed\n')
            with patch.object(identity,'git',side_effect=OSError):self.assertNotEqual(first[2],identity.source_identity(root)[2])


if __name__=='__main__':unittest.main()
