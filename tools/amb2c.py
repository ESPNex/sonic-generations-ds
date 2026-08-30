#!/usr/bin/env python3
"""amb2c.py — genera engine/source/amb_data.c: embed .amb originali come
array const (il decoder runtime gfxrt.c li decodifica in ARM9).
Uso: python3 tools/amb2c.py engine/source/amb_data.c PLAYER_CLS [PLAYER_MDN ...]
I file sorgente sono in sgds-data/amb/ (copiati dal box GAME/PLAYER/).
NOTA binario ARM9: il loader BIOS wrappa >4MB — max ~2 .amb embedded;
il resto arrivera' via card-reader (M6.2)."""
import sys
def esc(d):
    out=[]
    for i in range(0,len(d),800):
        out.append('"'+''.join('\\x%02x'%b for b in d[i:i+800])+'"')
    return '\n'.join(out)
names=[('PLAYER_CLS','cls'),('PLAYER_MDN','mdn')]
out=sys.argv[1]
srcs=sys.argv[2:]
with open(out,'w') as f:
    f.write('/* generato da tools/amb2c.py — .amb ORIGINALI del gioco */\n#include <nds.h>\n#include "amb_data.h"\n\n')
    for s in srcs:
        base=s.split('/')[-1].replace('.amb','')
        short='cls' if 'CLS' in base else ('mdn' if 'MDN' in base else base.lower())
        d=open(s,'rb').read()
        f.write('const unsigned char amb_%s[] =\n%s;\nconst unsigned int amb_%s_len = %d;\n\n'%(short,esc(d),short,len(d)))
