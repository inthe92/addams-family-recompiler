"""Deterministically package own scripts/art; no ROM or extracted content."""
from pathlib import Path
import struct,zlib,zipfile,sys
ROOT=Path(__file__).resolve().parents[2]
NAMES={'super-gomez-gameplay':'Super-Gomez-Gameplay-v1.0.zip','speedrun-hud':'Speedrun-HUD-v1.0.zip','mansion-parallax-demo':'Mansion-Parallax-Demo-v1.0.zip'}
def mansion():
    # Original procedural silhouette, not a game asset.
    w,h=64,32;pixels=bytearray()
    for y in range(h):
        pixels.append(0)
        for x in range(w):
            roof=6+abs(x-32)//4
            wall=roof<=y<31 and 5<x<59
            window=wall and y in range(18,22) and x%12 in (5,6,7)
            star=(x*13+y*19)%157==0 and y<roof
            pixels.extend((240,215,130,255) if window or star else (58,70,95,235) if wall else (0,0,0,0))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(pixels))+chunk(b'IEND',b'')
def build(output):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    for folder,name in NAMES.items():
        source=ROOT/'examples/mods'/folder
        files={'mod.json':(source/'mod.json').read_bytes(),'main.lua':(source/'main.lua').read_bytes(),'assets/README.txt':(source/'README.txt').read_bytes()}
        if folder=='mansion-parallax-demo':files['assets/mansion.png']=mansion()
        with zipfile.ZipFile(output/name,'w') as z:
            for path,data in sorted(files.items()):
                info=zipfile.ZipInfo(path,(2026,10,14,0,0,0));info.create_system=3;info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
if __name__=='__main__':build(sys.argv[1])
