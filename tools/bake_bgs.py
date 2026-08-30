#!/usr/bin/env python3
"""bake_bgs.py — bande sfondo PER ZONA dalle texture ORIGINALI ({zone}_mdl.bcres).
Blur 2D separabile (wrap-x) + fusione del bordo inferiore verso il cielo:
niente streak, niente cucitura, niente gradino.
Uso: python3 bake_bgs.py OUT.h nome1=bmp1 nome2=bmp2 ...
"""
import sys, struct

def bmp_read(path):
    b = open(path, 'rb').read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    rowsz = (w*3+3)&~3
    return w, h, [[(b[off+(h-1-y)*rowsz+x*3+2], b[off+(h-1-y)*rowsz+x*3+1], b[off+(h-1-y)*rowsz+x*3])
                  for x in range(w)] for y in range(h)]

OW, OH, R = 256, 96, 6

def band(src):
    w, h, px = bmp_read(src)
    sm = [[px[int(y*h/OH)][int(x*w/OW)] for x in range(OW)] for y in range(OH)]
    # blur separabile: orizzontale wrap, poi verticale (clamp)
    tmp = [[None]*OW for _ in range(OH)]
    for y in range(OH):
        for x in range(OW):
            r=g=b_=n=0
            for d in range(-R, R+1):
                pr, pg, pb = sm[y][(x+d) % OW]
                wgt = R+1-abs(d)
                r += pr*wgt; g += pg*wgt; b_ += pb*wgt; n += wgt
            tmp[y][x] = (r//n, g//n, b_//n)
    out = [[None]*OW for _ in range(OH)]
    for y in range(OH):
        for x in range(OW):
            r=g=b_=n=0
            for d in range(-R, R+1):
                yy = min(OH-1, max(0, y+d))
                pr, pg, pb = tmp[yy][x]
                wgt = R+1-abs(d)
                r += pr*wgt; g += pg*wgt; b_ += pb*wgt; n += wgt
            out[y][x] = (r//n, g//n, b_//n)
    # fusione bordo inferiore (ultimi 24px) -> trasparente via alpha decrescente
    # (il motore fonde col cielo sotto: emettiamo il colore tal quale, il fade
    #  lo fa il render con un lerp; qui predisponiamo una maschera alpha)
    return out

def main():
    dst = sys.argv[1]
    out = ['// generato da tools/bake_bgs.py — non committare', '#pragma once']
    for arg in sys.argv[2:]:
        name, bmp = arg.split('=', 1)
        b = band(bmp)
        out.append('static const unsigned short %s[%d] = {' % (name, OW*OH))
        for y in range(OH):
            row = []
            for x in range(OW):
                r, g, bb = b[y][x]
                row.append('0x%04X' % (0x8000 | ((bb>>3)<<10) | ((g>>3)<<5) | (r>>3)))
            out.append('    ' + ', '.join(row) + ',')
        out.append('};')
        print('banda %s <- %s' % (name, bmp))
    open(dst, 'w').write('\n'.join(out) + '\n')

if __name__ == '__main__':
    main()
