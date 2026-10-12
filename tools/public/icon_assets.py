"""Validate the supplied branding files and embed their unmodified RGBA pixels.

Uses only the Python standard library; no image library is shipped or linked.
The PNG reader intentionally accepts the official non-interlaced RGBA8 format.
"""
import argparse
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import plistlib
import struct
import zlib

ASSETS = Path(__file__).resolve().parents[2] / 'assets/icons'
NAME = 'nes-native-lab'


def require(ok, message):
    if not ok:
        raise ValueError(message)


@lru_cache(maxsize=32)
def png_rgba(data):
    require(data[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid PNG signature')
    at = 8; compressed = bytearray(); size = None; ended = False
    while at + 12 <= len(data):
        length, = struct.unpack_from('>I', data, at)
        kind = data[at+4:at+8]; body = data[at+8:at+8+length]
        require(at + length + 12 <= len(data), 'Truncated PNG chunk')
        crc, = struct.unpack_from('>I', data, at+8+length)
        require(zlib.crc32(kind+body) & 0xffffffff == crc, 'PNG CRC mismatch')
        if kind == b'IHDR':
            require(size is None and at == 8 and length == 13, 'Invalid PNG header')
            w, h, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', body)
            require(0 < w <= 1024 and 0 < h <= 1024, 'Unexpected PNG dimensions')
            require((depth, color, compression, filtering, interlace) == (8, 6, 0, 0, 0),
                    'Expected non-interlaced RGBA8 PNG')
            size = (w, h)
        elif kind == b'IDAT':
            compressed.extend(body)
        elif kind == b'IEND':
            require(length == 0 and at+12 == len(data), 'Invalid PNG end')
            ended = True; break
        at += length+12
    require(size is not None and ended, 'Incomplete PNG')
    w, h = size; stride = w*4; raw = zlib.decompress(compressed)
    require(len(raw) == h*(stride+1), 'PNG scanline length mismatch')
    pixels = bytearray(); previous = bytearray(stride)
    for row in range(h):
        start = row*(stride+1); mode = raw[start]; scan = bytearray(raw[start+1:start+1+stride])
        require(mode <= 4, 'Invalid PNG filter')
        for x in range(stride):
            a = scan[x-4] if x >= 4 else 0; b = previous[x]; c = previous[x-4] if x >= 4 else 0
            predictor = 0
            if mode == 1: predictor = a
            elif mode == 2: predictor = b
            elif mode == 3: predictor = (a+b)//2
            elif mode == 4:
                p = a+b-c; distances = (abs(p-a), abs(p-b), abs(p-c))
                predictor = (a, b, c)[distances.index(min(distances))]
            scan[x] = (scan[x]+predictor) & 255
        pixels.extend(scan); previous = scan
    return w, h, bytes(pixels)


def ico_images(data):
    require(len(data) >= 6, 'Truncated ICO')
    reserved, kind, count = struct.unpack_from('<HHH', data)
    require((reserved, kind) == (0, 1) and 0 < count <= 32, 'Invalid ICO header')
    require(6+count*16 <= len(data), 'Truncated ICO directory')
    images = []
    for i in range(count):
        w, h, colors, reserved, planes, depth, length, offset = struct.unpack_from('<BBBBHHII', data, 6+i*16)
        require(reserved == 0 and planes == 0 and depth == 32, 'Unexpected ICO entry')
        require(offset >= 6+count*16 and length > 0 and offset+length <= len(data), 'Invalid ICO bounds')
        payload = data[offset:offset+length]; width, height, _ = png_rgba(payload)
        require((width, height) == (w or 256, h or 256), 'ICO image size mismatch')
        images.append((w, h, colors, planes, depth, payload))
    return images


def icns_images(data):
    require(data[:4] == b'icns' and len(data) >= 8, 'Invalid ICNS header')
    require(int.from_bytes(data[4:8], 'big') == len(data), 'ICNS length mismatch')
    sizes = {b'ic07':128, b'ic08':256, b'ic09':512, b'ic10':1024,
             b'ic11':32, b'ic12':64, b'ic13':256, b'ic14':512}
    at = 8; found = set()
    while at < len(data):
        require(at+8 <= len(data), 'Truncated ICNS chunk')
        kind, length = struct.unpack_from('>4sI', data, at)
        require(length >= 8 and at+length <= len(data), 'Invalid ICNS bounds')
        if kind in sizes:
            w, h, _ = png_rgba(data[at+8:at+length])
            require(w == h == sizes[kind] and kind not in found, 'Invalid ICNS image')
            found.add(kind)
        else:
            require(kind == b'TOC ', 'Unexpected ICNS chunk')
        at += length
    require(set(sizes) == found, 'Missing ICNS resolutions')
    return found


def official(path, filename):
    expected = (ASSETS/filename).read_bytes()
    require(path.is_file() and path.read_bytes() == expected, 'Missing/changed official icon: '+str(path))


def verify_assets():
    files = json.loads((ASSETS/'SHA256SUMS.json').read_text())['files']
    for name, sha in files.items():
        data = (ASSETS/name).read_bytes()
        require(hashlib.sha256(data).hexdigest() == sha, 'Official asset hash mismatch: '+name)
        if name.endswith('.png'):
            size = int(Path(name).stem.rsplit('-', 1)[1]); w, h, _ = png_rgba(data)
            require(w == h == size, 'Incorrect PNG size')
    require(len(ico_images((ASSETS/(NAME+'.ico')).read_bytes())) == 7, 'Missing ICO sizes')
    icns_images((ASSETS/(NAME+'.icns')).read_bytes())


def pe_icon_resources(data):
    """Read named-by-ID RT_GROUP_ICON and RT_ICON from an x64 PE, without executing it."""
    require(data[:2] == b'MZ' and len(data) >= 64, 'Invalid PE header')
    pe, = struct.unpack_from('<I', data, 60)
    require(data[pe:pe+4] == b'PE\0\0', 'Invalid PE signature')
    machine, count = struct.unpack_from('<HH', data, pe+4)
    optional_size, = struct.unpack_from('<H', data, pe+20); optional = pe+24
    require(machine == 0x8664 and struct.unpack_from('<H', data, optional)[0] == 0x20b, 'Expected x64 PE')
    resource_rva, resource_size = struct.unpack_from('<II', data, optional+112+2*8)
    require(resource_rva and resource_size, 'Missing executable icon resources')
    sections = optional+optional_size
    def offset(rva):
        for i in range(count):
            virtual, base, length, raw = struct.unpack_from('<IIII', data, sections+i*40+8)
            if base <= rva < base+max(virtual, length):
                at = raw+rva-base
                require(rva-base < length and at < len(data), 'PE RVA outside file')
                return at
        raise ValueError('Unmapped resource RVA')
    root = offset(resource_rva)
    def entries(relative):
        at = root+relative
        require(0 <= relative and relative+16 <= resource_size, 'Invalid resource directory')
        named, ids = struct.unpack_from('<HH', data, at+12)
        require(relative+16+8*(named+ids) <= resource_size, 'Truncated resource directory')
        result = {}
        for i in range(named+ids):
            name, child = struct.unpack_from('<II', data, at+16+i*8)
            if not name & 0x80000000: result[name] = child
        return result
    def directory(child):
        require(child & 0x80000000, 'Expected resource subdirectory')
        return child & 0x7fffffff
    types = entries(0); result = {}
    for kind in (3, 14):
        require(kind in types, 'Missing PE icon resource type')
        for ident, languages in entries(directory(types[kind])).items():
            variants = entries(directory(languages)); require(variants, 'Missing resource language')
            require(len(variants) == 1, 'Unexpected multiple icon languages')
            child = next(iter(variants.values()))
            require(not child & 0x80000000 and child+16 <= resource_size, 'Invalid resource data entry')
            rva, length = struct.unpack_from('<II', data, root+child); at = offset(rva)
            require(length > 0 and at+length <= len(data), 'Truncated icon payload')
            result[kind, ident] = data[at:at+length]
    return result


def verify_windows(folder):
    official(folder/'assets/icons'/ (NAME+'.ico'), NAME+'.ico')
    images = ico_images((ASSETS/(NAME+'.ico')).read_bytes())
    resources = pe_icon_resources((folder/'nes_native.exe').read_bytes())
    group = resources.get((14, 1), b'')
    require(len(group) == 6+14*len(images), 'Invalid primary icon group')
    require(struct.unpack_from('<HHH', group) == (0, 1, len(images)), 'Wrong icon group header')
    for i, (w, h, colors, planes, depth, payload) in enumerate(images):
        actual = struct.unpack_from('<BBBBHHIH', group, 6+14*i)
        # RC normalizes PNG ICO's unspecified plane count (0) to one plane.
        require(actual[:4] == (w, h, colors, 0) and actual[4] in (planes, 1)
                and actual[5:7] == (depth, len(payload)), 'PE icon descriptor mismatch')
        require(resources.get((3, actual[7])) == payload, 'PE embedded icon differs from official ICO')


def verify_macos(app):
    info = plistlib.loads((app/'Contents/Info.plist').read_bytes())
    require(info.get('CFBundleIconFile') == NAME+'.icns', 'Bundle icon is not declared')
    official(app/'Contents/Resources'/(NAME+'.icns'), NAME+'.icns')
    icns_images((app/'Contents/Resources'/(NAME+'.icns')).read_bytes())


def verify_linux(appdir):
    filename = NAME+'-256.png'
    official(appdir/(NAME+'.png'), filename); official(appdir/'.DirIcon', filename)
    for size in (16, 32, 48, 64, 128, 256, 512, 1024):
        official(appdir/f'usr/share/icons/hicolor/{size}x{size}/apps/{NAME}.png', f'{NAME}-{size}.png')
    desktop = (appdir/(NAME+'.desktop')).read_text()
    require('\nIcon='+NAME+'\n' in desktop and '\nExec=nes_native\n' in desktop, 'Wrong desktop icon/launcher')
    require((appdir/f'usr/share/applications/{NAME}.desktop').read_text() == desktop, 'Desktop entries differ')


def emit_header(png, destination):
    w, h, pixels = png_rgba(png.read_bytes())
    require(w == h == 64, 'Window icon must use official 64px image')
    destination.parent.mkdir(parents=True, exist_ok=True)
    rows = [','.join(str(b) for b in pixels[i:i+32]) for i in range(0, len(pixels), 32)]
    destination.write_text('// Generated losslessly from the official PNG; no ROM content.\n'
        '#pragma once\nnamespace nes::runtime::icon_data {\n'
        f'constexpr int width={w}, height={h};\nconstexpr unsigned char rgba[]={{\n'+',\n'.join(rows)+'\n};\n}\n')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--png', type=Path); p.add_argument('--emit-header', type=Path)
    p.add_argument('--appdir', type=Path, help='Verify Linux branding after linuxdeploy')
    args = p.parse_args()
    if args.emit_header:
        require(args.png is not None, '--png is required'); emit_header(args.png, args.emit_header)
    else:
        verify_assets(); print('Official PNG/ICO/ICNS formats and hashes: PASS')
        if args.appdir:
            verify_linux(args.appdir); print('Linux AppDir icon registration: PASS')
