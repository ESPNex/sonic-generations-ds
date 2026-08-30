#!/usr/bin/env python3
"""evt_parse4.py — walk DEFINITIVO stream B/C (deep-RE, vedi docs/EVT_RE_DEEP.md).

RECORD {u32 prev, u32 this}, stride = this; payload = [rec+8, rec+this):
  {u32 chk, 0x80000000, 0x00640000, u32 w3, poi GROUPS fino a fine payload}
  GROUP = {u16 count (lo16 w4), s16 anchor (hi16 w4), u32 size (0 = fino a fine), objs}
  stream B: obj 16B {u32 xy (y s8.8 lo16 | x s8.8 hi16), u32 p1, u32 p2, u32 p3}
  stream C: obj  8B {u32 xy, u32 p1}
  (handler runtime 0x13dc64 / 0x146ca0: x_rel = byte@2 ×0.5, y_rel = s15@4 ×0.5,
   y_mondo += anchor×100 — il loader espande; qui estraggo le coordinate FILE
   s8.8 che sono ASSOLUTE e combaciano col terreno z11 X[−43,110])
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
        if this < 16 or p + this > end or this == 0:
            break
        yield p, d[p + 8:p + this]
        p += this


def parse_payload(pay, objsize):
    if len(pay) < 0x18:
        return None, None, 0
    w3 = struct.unpack_from('<I', pay, 0xC)[0]
    typ = (w3 >> 16) & 0xFFFF
    q = 0x10
    groups = []
    while q + 8 <= len(pay):
        count, anchor, size = struct.unpack_from('<HHi', pay, q)
        if count == 0 and anchor == 0 and size == 0:
            break
        if count > 0x400:
            break
        if size == 0:
            size = len(pay) - q          # ultimo group: fino a fine payload
        if size < 8 or q + size > len(pay):
            break
        objs = []
        for i in range(count):
            off = q + 8 + i * objsize
            if off + 8 > len(pay):
                break
            w0, p1 = struct.unpack_from('<2I', pay, off)
            o = {'x': round(s88(w0 >> 16), 3), 'y': round(s88(w0 & 0xFFFF), 3), 'p1': p1}
            if objsize == 16 and off + 16 <= len(pay):
                o['p2'], o['p3'] = struct.unpack_from('<2I', pay, off + 8)
            objs.append(o)
        groups.append({'count': count, 'anchor': anchor, 'size': size, 'objs': objs})
        q += size
    return typ, groups, q


def parse_file(path):
    d = open(path, 'rb').read()
    off_b, off_c = struct.unpack_from('<II', d, 4)
    out = {}
    for nm, a, b, osz in (('B', off_b, off_c, 16), ('C', off_c, len(d), 8)):
        recs = []
        ok = bad = 0
        for p, pay in walk_records(d, a, b):
            typ, groups, used = parse_payload(pay, osz)
            if groups and used >= len(pay) - 8:
                ok += 1
                recs.append({'at': p, 'type': typ, 'groups': groups})
            else:
                bad += 1
        out[nm] = {'ok': ok, 'bad': bad, 'recs': recs, 'objsize': osz}
    return out


if __name__ == '__main__':
    path = sys.argv[1]
    r = parse_file(path)
    for nm in ('B', 'C'):
        s = r[nm]
        tc = Counter()
        for rec in s['recs']:
            for g in rec['groups']:
                tc[rec['type']] += g['count']
        print(f"stream {nm} (obj {s['objsize']}B): {s['ok']} chiusi / {s['bad']} parziali; "
              f"tipi {dict(sorted(tc.items()))} tot={sum(tc.values())}")
    if '--objs' in sys.argv:
        for nm in ('B', 'C'):
            for rec in r[nm]['recs']:
                for g in rec['groups']:
                    print(f"{nm}@{rec['at']:#07x} type={rec['type']} anchor={g['anchor']} "
                          f"n={g['count']}: {[(o['x'], o['y']) for o in g['objs']]}")
    if '--json' in sys.argv:
        json.dump(r, open(sys.argv[sys.argv.index('--json') + 1], 'w'), indent=1)
