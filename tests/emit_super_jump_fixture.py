"""Two independently authored instructions, not an extraction from a game."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/recompiler'))
import emit_blocks as E
import opcodes_6502 as O
reverse={v:k for k,v in O.OPCODES.items()}
prg=bytearray(131072);instructions=[]
for pc,name in [(0xecfe,'STA'),(0xee2a,'INC')]:
    prg[7*16384+pc-0xc000:7*16384+pc-0xc000+2]=bytes([reverse[name,'zp'],0x15])
    instructions.append(E.decode(prg,dict(pc=pc,size=2,tier='own synthetic adapter test'),7,0xc000,O.OPCODES,O.SIZES))
out=Path(sys.argv[1]);out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(E.emit_bank(instructions,7,'super_jump')[0])
