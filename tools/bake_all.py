#!/usr/bin/env python3
"""bake_all.py — bake di TUTTI gli stage Dati_BIN + palette zona + sfondo texture.
Genera engine/source/world_data.h (NON committare: dati derivati).
Uso: python3 tools/bake_all.py SRCDIR(out/Dati_BIN) engine/source/world_data.h texdir
"""
import sys, os, math, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from z11_coll_lib import parse

STAGES = [
    ("z11", "GHZ", 1), ("z12", "GHZ", 0),
    ("z21", "CNZ", 1), ("z22", "CNZ", 0),
    ("z31", "MHZ", 1), ("z32", "MHZ", 0),
    ("z41", "RHW", 1), ("z42", "RHW", 0),
    ("r01", "RVL", 1), ("boss1", "BOS", 1), ("boss2", "BOS", 1), ("boss3", "BOS", 1),
]

def bake(src, code):
    chains, segs = parse(os.path.join(src, code + 'coll.bin'))
    X0, STEP = 0.0, 0.25
    fl = []
    for ch in chains:
        pts = ch['points']
        if len(pts) < 4: continue
        zs = [p[2] for p in pts]; zm = sum(zs)/len(zs)
        ys = [p[1] for p in pts]; xs = [p[0] for p in pts]
        if abs(zm + 19.0) > 3.0: continue
        if min(ys) < 6.0 or max(ys) > 60.0: continue
        if max(xs) - min(xs) < 2.0: continue
        fl.append(pts)
    cur = 0.0; spans = []
    for pts in fl:
        x0 = min(p[0] for p in pts); x1 = max(p[0] for p in pts)
        spans.append((cur - x0, pts)); cur += (x1 - x0) + 1.0
    n = int((cur + 12.0 - X0)/STEP) + 2
    h = [-29.0]*n
    for off, pts in spans:
        for i in range(len(pts)-1):
            ax, ay = pts[i][0]+off, pts[i][1]
            bx, by = pts[i+1][0]+off, pts[i+1][1]
            lo, hi = min(ax,bx), max(ax,bx)
            i0 = max(0, int(math.ceil((lo-X0)/STEP))); i1 = min(n-1, int((hi-X0)/STEP))
            for k in range(i0, i1+1):
                x = X0 + k*STEP
                t = 0.0 if bx == ax else max(0.0, min(1.0, (x-ax)/(bx-ax)))
                y = ay + (by-ay)*t
                if 5.0 <= y <= 62.0 and y > h[k]: h[k] = y
    BR = int(3.0/STEP); i = 0
    while i < n:
        if h[i] < -20.0:
            j = i
            while j < n and h[j] < -20.0: j += 1
            if i > 0 and j < n and (j-i) <= BR:
                y0, y1 = h[i-1], h[j]
                for k in range(i, j): h[k] = y0 + (y1-y0)*((k-i+1)/(j-i+1))
            i = j
        else: i += 1
    return h, X0, STEP

def main(srcdir, dst, texdir):
    out = ['// generato da tools/bake_all.py — non committare', '#pragma once',
       '#define WORLD_X0 0.0f', '#define WORLD_STEP 0.25f', '']
    total = 0
    names = []
    for code, zone, _ in STAGES:
        p = os.path.join(srcdir, code + 'coll.bin')
        if not os.path.exists(p):
            print('MANCA', p); continue
        h, X0, STEP = bake(srcdir, code)
        nm = 'world_%s' % code
        names.append((code, nm, len(h)))
        out.append('#define %s_N %d' % (nm.upper(), len(h)))
        out.append('static const float %s[%d] = {' % (nm, len(h)))
        for i in range(0, len(h), 8):
            out.append('    ' + ', '.join('%.3ff' % v for v in h[i:i+8]) + ',')
        out.append('};')
        total += len(h)
        print('%s: %d campioni, len %.0f' % (code, len(h), len(h)*STEP))
    out.append('')
    out.append('typedef struct { const char* code; const float* h; int n; } WorldDef;')
    out.append('static const WorldDef worlds[] = {')
    for code, nm, n in names:
        out.append('    { "%s", %s, %s_N },' % (code, nm, nm.upper()))
    out.append('};')
    out.append('#define WORLD_COUNT %d' % len(names))
    open(dst, 'w').write('\n'.join(out) + '\n')
    print('totale campioni: %d -> %s' % (total, dst))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None)
