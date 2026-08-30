#!/usr/bin/env python3
"""evt_parse3.py — parser DEFINITIVO dei evt SGDS (deep-RE completato, vedi docs/EVT_RE_DEEP.md).

FORMATO CONFERMATO:
- header {u32 ?, u32 offB, u32 offC}
- stream A @0x18: chunk {0x80000000, 0x00640000, 0, inner=0x18, sizeA, f32 X}
  stride = 0x18 + sizeA (verifica geometrica); sub-chunk/groups in area sizeA.
- stream B/C: record chain {u32 prev, u32 this}; payload
  {chk_prev, 0x80000000, 0x00640000, yspan, ENTRY...}
  ENTRY = {u16 type, u16 n, u32 param, n × 8B {u32 xy (y s8.8 lo16 | x s8.8 hi16), u32 p2}}
  (il loader 3DS espande in obj runtime {type u16, x, y, done} — handler 0x1340ec/0x13dc64/0x146ca0)

Uso: python3 evt_parse3.py FILE [--objs] [--json out]
"""
import struct, sys, json
from collections import Counter


def s88(v):
    v &= 0xFFFF
    return (v - 0x10000 if v >= 0x8000 else v) / 256.0


def walk_records(d, base, end):
    p = base
    while p + 8 <= end:
        prev, this = struct.unpack_from('<II', d, p)
        if this < 8 or p + this > end or this == 0:
            break
        yield p, d[p + 8:p + this]
        p += this


def parse_entries_strict(pay):
    """payload dopo i 16 byte di intestazione fissa {chk, 80000000, 00640000, yspan}"""
    out = []
    q = 0x10
    while q + 8 <= len(pay):
        t, n, param = struct.unpack_from('<HHI', pay, q)
        if t == 0 and n == 0:
            break
        if n > 0x400 or q + 8 + n * 8 > len(pay):
            break
        objs = []
        for i in range(n):
            xy, p2 = struct.unpack_from('<2I', pay, q + 8 + i * 8)
            objs.append({'x': round(s88(xy >> 16), 3), 'y': round(s88(xy & 0xFFFF), 3),
                         'p2': p2})
        out.append({'type': t, 'n': n, 'param': param, 'objs': objs})
        q += 8 + n * 8
    return out, q


def parse_stream_a(d):
    """chunk top {flags, span, 0, inner, sizeA, X f32}; stride = 0x18+sizeA"""
    chunks = []
    p = 0x18
    end = struct.unpack_from('<I', d, 4)[0]
    while p + 0x18 <= end:
        w0, w1, w2, inner, sizeA = struct.unpack_from('<5I', d, p)
        if w0 != 0x80000000 or w1 != 0x00640000:
            p += 4
            continue
        if sizeA == 0 or p + 0x18 + sizeA > end:
            p += 4
            continue
        xf = struct.unpack_from('<f', d, p + 0x14)[0]
        if not (0 < xf < 100000):
            p += 4
            continue
        chunks.append({'at': p, 'sizeA': sizeA, 'X': xf})
        p = p + 0x18 + sizeA
    return chunks


def parse_file(path):
    d = open(path, 'rb').read()
    off_b, off_c = struct.unpack_from('<II', d, 4)
    A = parse_stream_a(d)
    B, C = [], []
    for p, pay in walk_records(d, off_b, off_c):
        ents, _ = parse_entries_strict(pay)
        B.append({'at': p, 'size': len(pay) + 8, 'entries': ents})
    for p, pay in walk_records(d, off_c, len(d)):
        ents, _ = parse_entries_strict(pay)
        C.append({'at': p, 'size': len(pay) + 8, 'entries': ents})
    return d, A, B, C


def all_objs(streams):
    h = Counter()
    for s in streams:
        for e in s['entries']:
            h[e['type']] += e['n']
    return h


if __name__ == '__main__':
    path = sys.argv[1]
    d, A, B, C = parse_file(path)
    print(f'{path}: stream A {len(A)} chunk (X={sorted(set(c["X"] for c in A))}), '
          f'B {len(B)} rec, C {len(C)} rec')
    hb, cb = all_objs(B), all_objs(C)
    print('B tipi:', dict(sorted(hb.items())), 'tot', sum(hb.values()))
    print('C tipi:', dict(sorted(cb.items())), 'tot', sum(cb.values()))
    if '--objs' in sys.argv:
        for nm, S in (('B', B), ('C', C)):
            for s in S:
                for e in s['entries']:
                    print(f"{nm}@{s['at']:#x} type={e['type']} n={e['n']} param={e['param']:#x} "
                          f"objs={[(o['x'], o['y']) for o in e['objs']][:8]}")
    if '--json' in sys.argv:
        out = sys.argv[sys.argv.index('--json') + 1]
        json.dump({'A': A, 'B': B, 'C': C}, open(out, 'w'), indent=1)
        print('->', out)
