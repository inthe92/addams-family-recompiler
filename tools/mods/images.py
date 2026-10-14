"""Strict PNG RGBA8 decoder for declared own mod art; stdlib only, no execution."""
import struct,zlib
MAX_PNG=65536
MAX_RGBA=65536
def decode(data):
    if not isinstance(data,bytes) or not 45<=len(data)<=MAX_PNG or data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('Invalid/oversized PNG')
    at=8;header=None;compressed=bytearray();ended=False;seen_data=False;closed_data=False
    while at<len(data):
        if at+12>len(data):raise ValueError('Truncated PNG chunk')
        size=struct.unpack('>I',data[at:at+4])[0];kind=data[at+4:at+8];end=at+12+size
        if end>len(data) or size>MAX_PNG:raise ValueError('Invalid PNG chunk size')
        if any(not (65<=c<=90 or 97<=c<=122) for c in kind) or not 65<=kind[2]<=90:raise ValueError("Invalid PNG chunk name")
        if kind in (b'acTL',b'fcTL',b'fdAT'):raise ValueError("Animated PNG is not supported")
        payload=data[at+8:at+8+size]
        if zlib.crc32(kind+payload)!=struct.unpack('>I',data[at+8+size:end])[0]:raise ValueError('PNG CRC mismatch')
        if kind==b'IHDR':
            if header is not None or at!=8 or size!=13:raise ValueError('Duplicate/misplaced PNG header')
            w,h,depth,color,comp,filter_,interlace=struct.unpack('>IIBBBBB',payload)
            if not 1<=w<=128 or not 1<=h<=128 or (depth,color,comp,filter_,interlace)!=(8,6,0,0,0):raise ValueError('PNG requires 1..128 dimensions, RGBA8, no interlace')
            header=(w,h)
        elif kind==b'IDAT':
            if header is None or closed_data:raise ValueError('Invalid PNG data order')
            compressed.extend(payload);seen_data=True
        elif kind==b'IEND':
            if size or not seen_data or end!=len(data):raise ValueError('Invalid PNG end/trailing content')
            ended=True;at=end;break
        else:
            if not(kind[0]&32):raise ValueError('Unsupported critical PNG chunk')
            if seen_data:closed_data=True
        at=end
    if not ended or header is None:raise ValueError('Missing PNG data/end')
    w,h=header;stride=w*4;expected=(stride+1)*h
    decoder=zlib.decompressobj();raw=decoder.decompress(compressed,expected+1)
    if len(raw)!=expected or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:raise ValueError('PNG decoded size/stream mismatch')
    pixels=bytearray();before=bytearray(stride)
    for y in range(h):
        filter_=raw[y*(stride+1)];row=bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        if filter_>4:raise ValueError('Unsupported PNG filter')
        for i in range(stride):
            a=row[i-4] if i>=4 else 0;b=before[i];c=before[i-4] if i>=4 else 0
            if filter_==1:pred=a
            elif filter_==2:pred=b
            elif filter_==3:pred=(a+b)//2
            elif filter_==4:
                p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c);pred=a if pa<=pb and pa<=pc else b if pb<=pc else c
            else:pred=0
            row[i]=(row[i]+pred)&255
        pixels.extend(row);before=row
    return w,h,bytes(pixels)
