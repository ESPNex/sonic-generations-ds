#!/usr/bin/env python3
"""z11/zXX coll: estrattore libreria forme di collisione (spazio locale).
Uso: python3 z11_coll_lib.py FILE.coll [OUT.json]
Metodo: run consecutivi >=2 di record 6-float (x,y,z,sin,cos,k) con
(sin,cos) unitario. Segmenti = punti consecutivi, stesso z, dist < 26.
"""
import struct, sys, math, json

def unit(a, c): return 0.985 < a*a + c*c < 1.015

def parse(path):
    b = open(path, 'rb').read()
    n = (len(b) - 4) // 4
    f = struct.unpack_from('<%df' % n, b, 4)
    chains, off = [], 0
    while off + 6 <= n:
        x, y, z, sn, cs, kk = f[off:off+6]
        if unit(sn, cs) and abs(x) < 400 and abs(y) < 400 and abs(z) < 200:
            run, p = [], off
            while p + 6 <= n:
                x2, y2, z2, s2, c2, k2 = f[p:p+6]
                if unit(s2, c2) and abs(x2) < 400 and abs(y2) < 400 and abs(z2) < 200:
                    run.append([x2, y2, z2, math.degrees(math.atan2(s2, c2)), k2]); p += 6
                else: break
            if len(run) >= 2:
                chains.append({'offset': off, 'points': run}); off = p; continue
        off += 1
    segs = []
    for ch in chains:
        pts = ch['points']
        for i in range(len(pts) - 1):
            x1, y1, z1 = pts[i][0], pts[i][1], pts[i][2]
            x2, y2, z2 = pts[i+1][0], pts[i+1][1], pts[i+1][2]
            d = math.hypot(x2 - x1, y2 - y1)
            if abs(z1 - z2) > 0.5 or not (0.05 < d < 26): continue
            segs.append({'p1': [round(x1,3), round(y1,3), round(z1,3)],
                         'p2': [round(x2,3), round(y2,3), round(z2,3)],
                         'deg': round(pts[i][3], 2)})
    return chains, segs

if __name__ == '__main__':
    src = sys.argv[1]
    chains, segs = parse(src)
    print('%s: %d catene, %d segmenti' % (src, len(chains), len(segs)))
    for ch in chains[:6]:
        print('  offset %d: %d punti, da %s a %s' % (
            ch['offset'], len(ch['points']),
            [round(v,1) for v in ch['points'][0][:3]],
            [round(v,1) for v in ch['points'][-1][:3]]))
    if len(sys.argv) > 2:
        json.dump({'chains': chains, 'segments': segs}, open(sys.argv[2], 'w'), indent=1)
        print('scritto', sys.argv[2])
