"""Official branding, negative format checks and synthetic package layouts."""
import json
from pathlib import Path
import plistlib
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/public'))
import icon_assets as icons
import package_platform as packaging
import build_identity


def synthetic_pe(plane=None):
    """Own PE resource fixture only; not an executable or a game module."""
    images = icons.ico_images((icons.ASSETS/'nes-native-lab.ico').read_bytes())
    group = bytearray(struct.pack('<HHH', 0, 1, len(images)))
    for i, (w, h, colors, planes, depth, payload) in enumerate(images, 1):
        group.extend(struct.pack('<BBBBHHIH', w, h, colors, 0, planes if plane is None else plane, depth, len(payload), i))
    tree = {3: {i:{1033:image[5]} for i,image in enumerate(images,1)}, 14:{1:{1033:bytes(group)}}}
    blob = bytearray()
    def reserve(length):
        while len(blob)%4: blob.append(0)
        at = len(blob); blob.extend(bytes(length)); return at
    def directory(values):
        at = reserve(16+8*len(values)); struct.pack_into('<HH', blob, at+12, 0, len(values))
        for i, (key, value) in enumerate(sorted(values.items())):
            if isinstance(value, dict): child = directory(value)|0x80000000
            else:
                child = reserve(16); data = reserve(len(value)); blob[data:data+len(value)] = value
                struct.pack_into('<IIII', blob, child, 0x1000+data, len(value), 0, 0)
            struct.pack_into('<II', blob, at+16+i*8, key, child)
        return at
    assert directory(tree) == 0
    header = bytearray(512); header[:2] = b'MZ'; struct.pack_into('<I', header, 60, 128)
    header[128:132] = b'PE\0\0'; struct.pack_into('<HH', header, 132, 0x8664, 1)
    struct.pack_into('<H', header, 148, 240); optional = 152
    struct.pack_into('<H', header, optional, 0x20b)
    struct.pack_into('<I', header, optional+108, 16)
    struct.pack_into('<II', header, optional+128, 0x1000, len(blob))
    section = optional+240; header[section:section+8] = b'.rsrc\0\0\0'
    struct.pack_into('<IIII', header, section+8, len(blob), 0x1000, len(blob), 512)
    return bytes(header+blob)


class IconChecks(unittest.TestCase):
    def test_independent_png_filter_vectors(self):
        # Hand-calculated two-row vectors, not the encoder's output.
        raw = bytes(range(10, 170, 10))
        rows = {
            0: (raw[:8], raw[8:]),
            1: (bytes([10,20,30,40,40,40,40,40]), bytes([90,100,110,120,40,40,40,40])),
            2: (raw[:8], bytes([80]*8)),
            3: (bytes([10,20,30,40,45,50,55,60]), bytes([85,90,95,100,60,60,60,60])),
            4: (bytes([10,20,30,40,40,40,40,40]), bytes([80,80,80,80,40,40,40,40])),
        }
        def chunk(kind, data):
            return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data)&0xffffffff)
        for mode, (one, two) in rows.items():
            with self.subTest(mode=mode):
                png = b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB',2,2,8,6,0,0,0))
                png += chunk(b'IDAT', zlib.compress(bytes([mode])+one+bytes([mode])+two))+chunk(b'IEND',b'')
                self.assertEqual(icons.png_rgba(png), (2,2,raw))

    def test_official_formats_and_hashes(self):
        icons.verify_assets()

    def test_png_corruption_is_rejected(self):
        data = bytearray((icons.ASSETS/'nes-native-lab-64.png').read_bytes()); data[-5] ^= 1
        with self.assertRaises(ValueError): icons.png_rgba(bytes(data))
        with self.assertRaises(ValueError): icons.png_rgba(bytes(data[:20]))

    def test_container_bounds_are_checked(self):
        data = bytearray((icons.ASSETS/'nes-native-lab.ico').read_bytes())
        struct.pack_into('<I', data, 18, len(data)+1)
        with self.assertRaises(ValueError): icons.ico_images(bytes(data))
        data = (icons.ASSETS/'nes-native-lab.icns').read_bytes()
        with self.assertRaises(ValueError): icons.icns_images(data[:-1])

    def test_windows_resources_and_changed_payload(self):
        with tempfile.TemporaryDirectory(prefix='icon Windows with spaces ') as name:
            root = Path(name); (root/'assets/icons').mkdir(parents=True)
            (root/'assets/icons/nes-native-lab.ico').write_bytes((icons.ASSETS/'nes-native-lab.ico').read_bytes())
            data = synthetic_pe(); (root/'nes_native.exe').write_bytes(data)
            icons.verify_windows(root)
            (root/'nes_native.exe').write_bytes(synthetic_pe(plane=1))
            icons.verify_windows(root)  # Verified real RC normalization.
            (root/'nes_native.exe').write_bytes(synthetic_pe(plane=2))
            with self.assertRaises(ValueError): icons.verify_windows(root)
            changed = bytearray(data); changed[-1] ^= 1; (root/'nes_native.exe').write_bytes(changed)
            with self.assertRaises(ValueError): icons.verify_windows(root)
            (root/'nes_native.exe').write_bytes(b'Not an executable')
            with self.assertRaises(ValueError): icons.verify_windows(root)

    def test_mac_bundle_declaration_and_bytes(self):
        with tempfile.TemporaryDirectory(prefix='icon mac bundle ') as name:
            app = Path(name)/'Official.app'; resources = app/'Contents/Resources'; resources.mkdir(parents=True)
            (resources/'nes-native-lab.icns').write_bytes((icons.ASSETS/'nes-native-lab.icns').read_bytes())
            info = app/'Contents/Info.plist'; info.write_bytes(plistlib.dumps({'CFBundleIconFile':'nes-native-lab.icns'}))
            icons.verify_macos(app)
            info.write_bytes(plistlib.dumps({'CFBundleIconFile':'old.icns'}))
            with self.assertRaises(ValueError): icons.verify_macos(app)

    def test_linux_packager_and_desktop_registration(self):
        with tempfile.TemporaryDirectory(prefix='icon Linux package ') as name:
            root = Path(name); build = root/'build'; build.mkdir(); (build/'nes_native').write_bytes(b'synthetic fixture')
            tools = root/'toolchain'; tools.mkdir(); (tools/'target.json').write_text(json.dumps({'system':'linux-x86_64'}))
            build_identity.generate(build,'0.2.1','Linux','x86_64')
            appdir = packaging.package(build, root/'dist', tools, 'Linux', 'x86_64')
            icons.verify_linux(appdir)
            with self.assertRaisesRegex(ValueError,'already exists'):
                packaging.package(build, root/'dist', tools, 'Linux', 'x86_64')
            (appdir/'nes-native-lab.desktop').write_text('[Desktop Entry]\nIcon=old-icon\n')
            with self.assertRaises(ValueError): icons.verify_linux(appdir)


if __name__ == '__main__':
    unittest.main()
