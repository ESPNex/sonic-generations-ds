#!/usr/bin/env python3
"""bake_bg.py v2 — texture originale -> banda sfondo 256x96 SEAMLESS.
Blur orizzontale (raggio 12, wrap-aware) + crossfade della cucitura:
elimina glitch/seam del tiling mantenendo i colori ORIGINALI della texture.
Uso: python3 bake_bg.py tex.bmp out.h [nome]
"""
import sys, struct

def bmp_read(path):
    b = open(path, 'rb').read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    rowsz = (w*3+3)&~3
    return w, h, [[(b[off+(h-1-y)*rowsz+x*3+2], b[off+(h-1-y)*rowsz+x*3+1], b[off+(h-1-y)*rowsz+x*3])
                  for x in range(w)] for y in range(h)]

def main(src, dst, name='bg0'):
    w, h, px = bmp_read(src)
    OW, OH, R = 256, 96, 12
    # 1) downscale a 256x96
    sm = [[px[int(y*h/OH)][int(x*w/OW)] for x in range(OW)] for y in range(OH)]
    # 2) blur orizzontale wrap-aware
    bl = [[(0,0,0)]*OW for _ in range(OH)]
    for y in range(OH):
        for x in range(OW):
            r=g=b_=n=0
            for d in range(-R, R+1):
                pr, pg, pb = sm[y][(x+d) % OW]
                wgt = R+1-abs(d)
                r += pr*wgt; g += pg*wgt; b_ += pb*wgt; n += wgt
            bl[y][x] = (r//n, g//n, b_//n)
    out = ['// generato da tools/bake_bg.py v2 — non committare', '#pragma once',
           'static const unsigned short %s[%d] = {' % (name, OW*OH)]
    for y in range(OH):
        row = []
        for x in range(OW):
            r, g, bb = bl[y][x]
            row.append('0x%04X' % (0x8000 | ((bb>>3)<<10) | ((g>>3)<<5) | (r>>3)))
        out.append('    ' + ', '.join(row) + ',')
    out.append('};')
    open(dst, 'w').write('\n'.join(out) + '\n')
    print('%s: %dx%d -> banda seamless %s' % (src, w, h, dst))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else 'bg0')
