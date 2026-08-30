#!/usr/bin/env python3
"""evt_parse.py v2 — parser DEFINITIVO dei evt SGDS (placement originale).

CONFERMATO:
- stream B (da word1) e C (da word2): catena record {u32 prev_size, u32 this_size}
  payload = {u32 chk_prev, u32 0x80000000, u32 0x00640000, u32 yspan(0x0001ff9c),
             ENTRY..., padding}
  ENTRY   = {u16 type, u16 count, count x 16B oggetto}
  oggetto = {u32 packed_xy (x s8.8 << 16 | y s8.8), u32 z?, u32 angle?, u32 param?}
  (le file di anelli mostrano x = 0x280a, 0x280c... passo 0.5 unita')
- stream A (0x18..word1): chunk terreno {flags 0x80000000, span 0x00640000, 0,
  sizeA, sizeTotal, 0, f32 X, flags2, span, yspan, entry...} + padding 0xcccc.

Uso:
  python3 evt_parse.py z11evt.bin --dump out.json   # placement completo
  python3 evt_parse.py z11evt.bin --hist            # istogramma tipi
"""
import struct, sys, json
from collections import Counter

def s88(v):
    v &= 0xFFFF
    if v >= 0x8000: v -= 0x10000
    return v / 256.0

def unpack_xy(w):
    return (s88(w >> 16), s88(w & 0xFFFF))

def walk_chain(d, base, end):
    """record chain {prev,this}"""
    recs = []
    p = base
    while p + 8 <= end:
        prev, this = struct.unpack_from('<II', d, p)
        if this < 8 or p + this > end:
            break
        recs.append((p, d[p+8:p+this]))
        p += this
        if this == 0:
            break
    return recs

def parse_entries(pay, q=16, resync=True):
    """entry: {u16 type, u16 count, u32 param, count x 8B {u32 xy, u32 w2}}
    con resync: se l'entry non torna, avanza di 4 e riprova (fino a fine payload)"""
    out = []
    while q + 8 <= len(pay):
        t, n = struct.unpack_from('<HH', pay, q)
        param = struct.unpack_from('<I', pay, q+4)[0]
        if (t == 0 and n == 0) or n > 0x200 or q + 8 + n*8 > len(pay):
            if not resync:
                break
            q += 4
            continue
        objs = []
        for i in range(n):
            xy, w2 = struct.unpack_from('<2I', pay, q + 8 + i*8)
            x, y = unpack_xy(xy)
            objs.append({'x': round(x, 3), 'y': round(y, 3), 'w2': w2})
        q += 8 + n * 8
        out.append({'type': t, 'n': n, 'param': param, 'objs': objs})
    return out, q

def parse_stream_a(d, end):
    """chunk terreno trovati col pattern {0x80000000, 0x00640000} + f32 X@+0x18"""
    chunks = []
    p = 0x18
    while p + 0x28 <= end:
        w0, w1 = struct.unpack_from('<II', d, p)
        if w0 != 0x80000000 or w1 != 0x00640000:
            p += 4
            continue
        x = struct.unpack_from('<f', d, p+0x18)[0]
        if not (0 < x < 100000 and abs(x/100 - round(x/100)) < 0.01):
            p += 4
            continue
        sizeA   = struct.unpack_from('<I', d, p+0x0c)[0]
        sizeTot = struct.unpack_from('<I', d, p+0x10)[0]
        yspan   = struct.unpack_from('<I', d, p+0x24)[0]
        entries, _ = parse_entries(d, p+0x24)
        # le entry vivono in [p+0x28, p+sizeA+0x10?]: riprova con boundary
        chunks.append({'at': p, 'x': x, 'sizeA': sizeA, 'sizeTot': sizeTot,
                       'yspan': hex(yspan), 'entries': entries})
        nxt = p + 4 + sizeTot if 0 < sizeTot < 0x8000 else p + 4
        p = nxt if nxt > p else p + 4
    return chunks

def parse_file(path):
    d = open(path, 'rb').read()
    o_b, o_c = struct.unpack_from('<II', d, 4)
    a = parse_stream_a(d, o_b)
    b = []
    for p, pay in walk_chain(d, o_b, o_c):
        ents, _ = parse_entries(pay)
        b.append({'at': p, 'entries': ents})
    c = []
    for p, pay in walk_chain(d, o_c, len(d)):
        ents, _ = parse_entries(pay)
        c.append({'at': p, 'entries': ents})
    return d, a, b, c

def hist(streams):
    h = Counter(); pts = 0
    for s in streams:
        for ch in s:
            for e in ch['entries']:
                h[e['type']] += e['n']; pts += e['n']
    return h, pts

if __name__ == '__main__':
    path = sys.argv[1]
    d, A, B, C = parse_file(path)
    print(f'{path}: A={len(A)} chunk terreno, B={len(B)} rec, C={len(C)} rec')
    for label, s in (('A', A), ('B', B), ('C', C)):
        h, pts = hist([s])
        print(f'  stream {label}: {sum(len(c["entries"]) for c in s)} entry, '
              f'{pts} oggetti, tipi: {dict(h.most_common(20))}')
    if '--dump' in sys.argv:
        out = sys.argv[sys.argv.index('--dump')+1]
        json.dump({'A': A, 'B': B, 'C': C}, open(out, 'w'), indent=1)
        print(f'-> {out}')
    if '--sample' in sys.argv:
        for ch in A[:3] + B[:3]:
            print(f"chunk/rec X={ch.get('x', ch.get('at'))}:")
            for e in ch['entries'][:6]:
                print(f"   type={e['type']:3d} n={e['n']}: "
                      f"{[(o['x'], o['y']) for o in e['objs'][:6]]}")
