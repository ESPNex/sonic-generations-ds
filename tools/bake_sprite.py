#!/usr/bin/env python3
"""bake_sprite.py — Sonic dalla TEXTURE ORIGINALE (sonic_pose.bcres).
Cerca la finestra 16x16 piu' "sonic" (max blu corpo + pelle muso) nella
texture e la quantizza a 15 colori + trasparente -> array C 4bpp.
Uso: python3 bake_sprite.py tex.bmp out.h
"""
import sys, struct
from collections import Counter

def bmp_read(path):
    b = open(path, 'rb').read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    rowsz = (w*3+3)&~3
    return w, h, [[(b[off+(h-1-y)*rowsz+x*3+2], b[off+(h-1-y)*rowsz+x*3+1], b[off+(h-1-y)*rowsz+x*3])
                  for x in range(w)] for y in range(h)]

def is_blue(p):  r,g,b = p; return b > 90 and b > r + 30 and g < b
def is_skin(p):  r,g,b = p; return r > 140 and r > b + 50 and 60 < g < 200 and abs(r-g) < 90

def main(src, dst):
    w, h, px = bmp_read(src)
    best = None; best_score = -1
    for y in range(0, h-16, 8):
        for x in range(0, w-16, 8):
            blu = skin = 0
            for yy in range(16):
                for xx in range(16):
                    p = px[y+yy][x+xx]
                    if is_blue(p): blu += 1
                    elif is_skin(p): skin += 1
            score = skin*3 + blu*2
            if score > best_score:
                best_score = score; best = (x, y, blu, skin)
    x, y, blu, skin = best
    print('finestra ottima @(%d,%d): %d blu, %d pelle (score %d)' % (x, y, blu, skin, best_score))
    # quantizza: i pixel chiari-bordi -> trasparente; il resto a popolarita'
    cnt = Counter()
    for yy in range(16):
        for xx in range(16):
            p = px[y+yy][x+xx]
            if not (is_blue(p) or is_skin(p)):
                cnt[p] += 1
    palette = [p for p, _ in cnt.most_common(14)]
    def near(p, q): return abs(p[0]-q[0])+abs(p[1]-q[1])+abs(p[2]-q[2]) < 90
    def idx(p):
        for i, q in enumerate(palette):
            if near(p, q): return i + 1
        return 0  # trasparente
    rows = []
    for yy in range(16):
        row = []
        for xx in range(16):
            p = px[y+yy][x+xx]
            if is_blue(p): row.append(1)
            elif is_skin(p): row.append(2)
            else: row.append(idx(p))
        rows.append(row)
    pal = [(0,0,0)] + palette
    pal += [(0,0,0)] * (16 - len(pal))
    out = ['// generato da tools/bake_sprite.py — non committare', '#pragma once',
           'static const unsigned short spr_pal[16] = {']
    out.append('    ' + ', '.join('0x%04X' % (0x8000 | ((b>>3)<<10) | ((g>>3)<<5) | (r>>3)) for r,g,b in pal) + ',')
    out.append('};')
    out.append('static const unsigned char spr_px[16][16] = {')
    for r in rows:
        out.append('    {' + ','.join(str(v) for v in r) + '},')
    out.append('};')
    open(dst, 'w').write('\n'.join(out) + '\n')
    print('-> %s (palette %d colori)' % (dst, len(palette)))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
