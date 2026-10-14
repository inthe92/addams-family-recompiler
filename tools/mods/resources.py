"""Trusted declarative text profiles and authored translations. No execution/ROM patch."""
import json,re
from pathlib import Path
MAX_TRANSLATION=16384

def unique(pairs):
    d={}
    for k,v in pairs:
        if k in d:raise ValueError('Duplicate resource JSON key')
        d[k]=v
    return d

def read(data):return json.loads(data,object_pairs_hook=unique)
def integer(v,lo,hi):return type(v) is int and lo<=v<=hi
def ident(v):return isinstance(v,str) and re.fullmatch(r'[a-z0-9][a-z0-9.-]{0,63}',v) is not None

def validate_profile(p,instructions=None):
    if not isinstance(p,dict) or set(p)!={'profile_schema','profile_id','game_id','rom_sha256','rom_size','prg_size','codec','resources'}|({'visual_contexts'} if p.get('profile_schema')==2 else set()):raise ValueError('Invalid resource profile fields')
    if type(p['profile_schema']) is not int or p['profile_schema'] not in (1,2) or not ident(p['profile_id']) or not ident(p['game_id']):raise ValueError('Invalid profile identity/schema')
    if not isinstance(p['rom_sha256'],str) or not re.fullmatch('[a-f0-9]{64}',p['rom_sha256']) or not integer(p['rom_size'],16400,1048576) or not integer(p['prg_size'],32768,262144) or p['prg_size']%16384:raise ValueError('Invalid profile ROM revision/layout')
    c=p['codec']
    if not isinstance(c,dict) or set(c)!={'id','glyphs','padding','display'} or c['id']!='fixed-glyph-field-v1' or c['display']!='uppercase' or not integer(c['padding'],0,127) or not isinstance(c['glyphs'],dict) or not 1<=len(c['glyphs'])<=128:raise ValueError('Unsupported text codec')
    if any(not isinstance(ch,str) or len(ch)!=1 or not integer(code,0,127) for ch,code in c['glyphs'].items()) or c['glyphs'].get(' ')!=c['padding']:raise ValueError('Invalid glyph mapping')
    if not isinstance(p['resources'],dict) or not 1<=len(p['resources'])<=16:raise ValueError('Invalid resource count')
    spans=[]
    for name,r in p['resources'].items():
        if not ident(name) or not isinstance(r,dict) or set(r)!={'type','bank','offset','expected_length','replacement_length','max_chars','codec','original_sha256','original_crc32','reader_bank','reader_pc','controls'}:raise ValueError('Invalid resource descriptor')
        if r['type']!='text' or r['codec']!=c['id'] or not integer(r['bank'],0,p['prg_size']//16384-1) or not integer(r['offset'],0,16383) or not integer(r['expected_length'],1,64) or r['offset']+r['expected_length']>16384:raise ValueError('Unsupported/invalid resource range')
        if r['offset']+r['expected_length']>0x3ffa:raise ValueError('Interrupt vector bytes are not text resources')
        if type(r['replacement_length']) is not int or r['replacement_length']!=r['expected_length'] or type(r['max_chars']) is not int or r['max_chars']!=r['expected_length'] or r['controls']!=[]:raise ValueError('Only fixed glyph fields excluding controls are supported')
        if not isinstance(r['original_sha256'],str) or not re.fullmatch('[a-f0-9]{64}',r['original_sha256']) or not integer(r['original_crc32'],0,0xffffffff) or not integer(r['reader_bank'],0,p['prg_size']//16384-1) or not integer(r['reader_pc'],0x8000,0xffff):raise ValueError('Invalid resource fingerprint/provenance')
        lo=r['bank']*16384+r['offset'];hi=lo+r['expected_length']
        if any(lo<b and a<hi for a,b in spans):raise ValueError('Overlapping resource descriptors')
        spans.append((lo,hi))
        if instructions is not None:
            pc=(0xc000 if r['bank']==7 else 0x8000)+r['offset']
            if any(start<pc+r['expected_length'] and start+3>pc for start in instructions[r['bank']]):raise ValueError('Resource overlaps known instruction/operand bytes')
            if r['reader_pc'] not in instructions[r['reader_bank']]:raise ValueError('Unverified resource reader')
    if p['profile_schema']==2:
        contexts=p['visual_contexts']
        if not isinstance(contexts,dict) or not 1<=len(contexts)<=16:raise ValueError('Invalid visual consumer count')
        spans=[]
        for name,c in contexts.items():
            if name not in p['resources'] or not isinstance(c,dict) or set(c)!={'backend','chr_bank','nametable','mirroring'} or c['backend']!='nes-background-cell-v1':raise ValueError('Invalid visual consumer')
            if not integer(c['chr_bank'],0,31) or not integer(c['nametable'],0x2000,0x2fff) or not integer(c['mirroring'],0,3):raise ValueError('Invalid visual consumer bounds')
            lo=c['nametable']&0x3ff;length=p['resources'][name]['expected_length']
            if lo>=960 or (lo&31)+length>32:raise ValueError('Visual consumer crosses a nametable row')
            if any(c['chr_bank']==bank and c['mirroring']==mirror and lo<b and a<lo+length for bank,mirror,a,b in spans):raise ValueError('Overlapping visual consumers')
            spans.append((c['chr_bank'],c['mirroring'],lo,lo+length))
    return p

def catalog(path):
    path=Path(path)
    if path.is_symlink() or path.stat().st_size>65536:raise ValueError('Invalid trusted resource catalog')
    data=read(path.read_bytes())
    if not isinstance(data,dict) or set(data)!={'catalog_schema','profiles'} or type(data['catalog_schema']) is not int or data['catalog_schema']!=1 or not isinstance(data['profiles'],list) or not 1<=len(data['profiles'])<=8:raise ValueError('Invalid resource catalog')
    result={}
    instruction_file=path.parent.parent/'recompiler/compatible_profile.json'
    instruction_map=None
    if instruction_file.exists():instruction_map=read(instruction_file.read_bytes())
    for p in data['profiles']:
        ins=instruction_map['instruction_starts'] if instruction_map and instruction_map['rom_sha256']==p.get('rom_sha256') else None
        if ins is None:raise ValueError('Resource profile requires a matching verified instruction map')
        if not isinstance(ins,list) or len(ins)!=p.get('prg_size',0)//16384 or any(not isinstance(bank,list) or any(not integer(pc,0x8000,0xffff) for pc in bank) for bank in ins):raise ValueError('Invalid verified instruction map')
        validate_profile(p,ins)
        if p['profile_id'] in result:raise ValueError('Duplicate resource profile')
        for old in result.values():
            if old['rom_sha256']!=p['rom_sha256']:continue
            for a in old['resources'].values():
                for b in p['resources'].values():
                    if a['bank']==b['bank'] and a['offset']<b['offset']+b['expected_length'] and b['offset']<a['offset']+a['expected_length']:raise ValueError('Overlapping profiles for same ROM')
        result[p['profile_id']]=p
    return result

def encode(profile,name,text):
    if name not in profile['resources']:raise ValueError('Unknown resource ID: '+str(name))
    if not isinstance(text,str):raise ValueError('Translation text must be a Unicode string')
    r=profile['resources'][name];c=profile['codec']
    if not text or len(text)>r['max_chars']:raise ValueError('Text overflow/empty for '+name+' (maximum '+str(r['max_chars'])+' glyphs)')
    codes=[]
    for ch in text:
        if ch not in c['glyphs']:raise ValueError('Unsupported glyph U+%04X for %s'%(ord(ch),name))
        codes.append(c['glyphs'][ch])
    return bytes(codes+[c['padding']]*(r['replacement_length']-len(codes)))

def decode(profile,data):
    inverse={}
    for ch,code in profile['codec']['glyphs'].items():
        if code not in inverse or ch.isupper():inverse[code]=ch
    if any(v not in inverse for v in data):raise ValueError('Unmapped glyph code')
    return ''.join(inverse[v] for v in data)

def compile_translation(data,profile,glyph_files=None):
    if len(data)>MAX_TRANSLATION:raise ValueError('Translation JSON exceeds limit')
    d=read(data)
    if not isinstance(d,dict) or set(d)!={'schema','profile_id','locale','texts'}|({'glyphs'} if d.get('schema')==2 else set()) or type(d['schema']) is not int or d['schema'] not in (1,2) or d['profile_id']!=profile['profile_id'] or not isinstance(d['locale'],str) or not re.fullmatch('[a-z]{2}-[A-Z]{2}',d['locale']) or not isinstance(d['texts'],dict) or not 1<=len(d['texts'])<=16:raise ValueError('Invalid translation schema/profile/locale')
    if d['schema']==1:return [(name,encode(profile,name,text)) for name,text in sorted(d['texts'].items())]
    declarations=glyph_declarations(d,profile)
    if glyph_files is None or set(glyph_files)!=set(v['file'] for v in declarations.values()):raise ValueError('Missing or undeclared own glyph file')
    for value in glyph_files.values():
        if not isinstance(value,bytes) or len(value)!=16:raise ValueError('Own glyph must be exactly one 8x8 NES 2bpp tile')
    result=[];used=set();cells=0
    for name,text in sorted(d['texts'].items()):
        if name not in profile.get('visual_contexts',{}):raise ValueError('No approved visual consumer for '+str(name))
        if not isinstance(text,str):raise ValueError('Translation text must be Unicode')
        encoded=''
        for char in text:
            if char in declarations:used.add(char);cells+=1;encoded+=declarations[char]['fallback']
            else:encoded+=char
        result.append((name,encode(profile,name,encoded)))
    if used!=set(declarations) or cells>8:raise ValueError('Unused own glyph or more than eight authored glyph cells')
    return result

def glyph_declarations(d,profile):
    declarations=d.get('glyphs')
    if not isinstance(declarations,dict) or not 1<=len(declarations)<=8:raise ValueError('Invalid own glyph count')
    files=set()
    for char,g in declarations.items():
        if not isinstance(char,str) or len(char)!=1 or 0xd800<=ord(char)<=0xdfff or char in profile['codec']['glyphs'] or ord(char)<0xa0 or not isinstance(g,dict) or set(g)!={'file','fallback'}:raise ValueError('Invalid own Unicode glyph')
        if not isinstance(g['file'],str) or not re.fullmatch(r'assets/[a-z0-9][a-z0-9-]{0,47}\.chr',g['file']) or g['file'] in files:raise ValueError('Invalid or repeated own glyph path')
        if not isinstance(g['fallback'],str) or len(g['fallback'])!=1 or g['fallback'] not in profile['codec']['glyphs']:raise ValueError('Unapproved glyph fallback')
        files.add(g['file'])
    return declarations

