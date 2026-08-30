#!/usr/bin/env python3
"""Bake v2 — corso giocabile ASSEMBLATO dalla libreria reale di z11coll.

Il file coll e' una LIBRERIA di forme in spazio locale: le catene in
ORDINE DI FILE vengono disposte in sequenza su x (fino alla decodifica
dei placement evt). Filtri: layer z principale, banda camminabile,
avanzamento in x. Nessun dato inventato: solo geometria originale.
Uso: python3 tools/bake_world.py FILE.coll OUT.h
"""
import sys, math

sys.path.insert(0, 'tools')
from z11_coll_lib import parse

src, dst = sys.argv[1], sys.argv[2]
chains, segs = parse(src)

fl = []
for ch in chains:
    pts = ch['points']
    if len(pts) < 4:
        continue
    zs = [p[2] for p in pts]
    zm = sum(zs) / len(zs)
    ys = [p[1] for p in pts]
    xs = [p[0] for p in pts]
    if abs(zm + 19.0) > 3.0:            # layer di gioco principale
        continue
    if min(ys) < 6.0 or max(ys) > 60.0:  # soffitti/sfondo fuori banda
        continue
    if max(xs) - min(xs) < 2.0:          # la catena deve avanzare
        continue
    fl.append(pts)

X0, STEP = 0.0, 0.25
cur = 0.0
spans = []
for pts in fl:
    x0 = min(p[0] for p in pts)
    x1 = max(p[0] for p in pts)
    spans.append((cur - x0, pts))
    cur += (x1 - x0) + 1.0
X1 = cur + 12.0
n = int((X1 - X0) / STEP) + 2
h = [-29.0] * n
for off, pts in spans:
    for i in range(len(pts) - 1):
        ax, ay = pts[i][0] + off, pts[i][1]
        bx, by = pts[i + 1][0] + off, pts[i + 1][1]
        lo, hi = min(ax, bx), max(ax, bx)
        i0 = max(0, int(math.ceil((lo - X0) / STEP)))
        i1 = min(n - 1, int((hi - X0) / STEP))
        for k in range(i0, i1 + 1):
            x = X0 + k * STEP
            t = 0.0 if bx == ax else max(0.0, min(1.0, (x - ax) / (bx - ax)))
            y = ay + (by - ay) * t
            if 5.0 <= y <= 62.0 and y > h[k]:
                h[k] = y

# ponticelli: isole di bedrock (<3 unita') tra due terreni reali → interpola
BRIDGE = int(3.0 / STEP)
i = 0
while i < n:
    if h[i] < -20.0:
        j = i
        while j < n and h[j] < -20.0:
            j += 1
        if i > 0 and j < n and (j - i) <= BRIDGE:
            y0, y1 = h[i - 1], h[j]
            for k in range(i, j):
                t = (k - i + 1) / (j - i + 1)
                h[k] = y0 + (y1 - y0) * t
        i = j
    else:
        i += 1

SPAWN = 2.0
with open(dst, 'w') as f:
    f.write('// generato da tools/bake_world.py — non committare\n')
    f.write('#pragma once\n')
    f.write('#define WORLD_X0 %s\n' % repr(X0))
    f.write('#define WORLD_STEP %s\n' % repr(STEP))
    f.write('#define WORLD_N %d\n' % n)
    f.write('#define WORLD_SPAWN_X %s\n' % repr(SPAWN))
    f.write('static const float world_h[WORLD_N] = {\n')
    for i in range(0, n, 8):
        f.write('    ' + ', '.join('%.3ff' % v for v in h[i:i + 8]) + ',\n')
    f.write('};\n')
print('bake v2: %d catene reali, lunghezza corso %.1f unita, %d campioni, h %.1f..%.1f'
      % (len(spans), cur, n, max(h), min(h)))
