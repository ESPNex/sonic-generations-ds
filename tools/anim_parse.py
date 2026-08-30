#!/usr/bin/env python3
"""anim_parse.py — animazioni SKELETAL dai .amb / .bcres (RE in corso, vedi docs/ANIM_RE.md).

CONFERMATO (P_SONC in PLAYER_CLS.amb):
- .amb "#AMB": header {magic, u32 0x20, u32 0x40000, 0, u32 n, u32 0x20, u32 0x80}
  poi n entry {u32 off, u32 size, -1, 0} + nome file in coda; entry = file CGFX interi
- CGFX root dict 'skel_anims': 84 anim in P_SONC (idle/run/push/sqat/brake/...)
- CANM (typeid nel dict root) layout:
    +0x00 "CANM"  +0x04 rev(7)  +0x08 name ptr  +0x0c ?
    +0x10 u32 loop  +0x14 f32 DURATION  +0x18 u32 flags(0xf)
    +0x1c u32 ntracks  +0x20 0  +0x24 "DICT" (read_dict a +0x28)
  bone track dict: nomi = NOMI OSSA REALI (Hips, Neck, Hair_center, UpperArm_L, ...)
- BONE ANIM (dal dict):
    +0x00 u32 flags(0x20)  +0x04 name ptr  +0x08 c1(8?)  +0x0c c2(12?)
    +0x10 c3(0x9a0?)  ... +0x20/+0x28: KEY = quat unit stride 20B (4B extra/key)
    (verificato: 20/20 norme ≈1 su Hips idle)

Uso:
  python3 anim_parse.py sgds-data/amb/PLAYER_CLS.amb          # lista anim + track
  python3 anim_parse.py --bcres /tmp/psonc_fromamb.bcres      # diretto su CGFX
"""
import struct, sys, re


def selfrel(pos, val):
    return (pos + val) & 0xFFFFFFFF


def cstr(r, a):
    e = r.find(b'\0', a)
    return r[a:e].decode('ascii', 'replace') if 0 < e - a < 64 else '?'


def read_dict(d, a):
    if d[a:a + 4] != b'DICT':
        return None
    nvals = struct.unpack_from('<I', d, a + 8)[0]
    names, vals = [], []
    p = a + 12
    for i in range(nvals + 1):
        # nodo: {u32 refBit, u16 left, u16 right, name ptr, value ptr}
        name_ptr = selfrel(p + 8, struct.unpack_from('<I', d, p + 8)[0])
        val_ptr = selfrel(p + 12, struct.unpack_from('<I', d, p + 12)[0])
        if i > 0:
            names.append(cstr(d, name_ptr)); vals.append(val_ptr)
        p += 16
    return names, vals


def parse_root(d):
    assert d[:4] == b'CGFX'
    hlen = struct.unpack_from('<H', d, 6)[0]
    p = max(hlen, 0x1c)
    ROOT = ['models', 'textures', 'luts', 'materials', 'shaders', 'cameras', 'lights',
            'fogs', 'scenes', 'skel_anims', 'mat_anims', 'vis_anims', 'cam_anims',
            'light_anims', 'fog_anims', 'emitters']
    out = {}
    for k in ROOT:
        cnt = struct.unpack_from('<i', d, p)[0]
        ptr = selfrel(p + 4, struct.unpack_from('<I', d, p + 4)[0])
        p += 8
        out[k] = (cnt, read_dict(d, ptr) if cnt > 0 else None)
    return out


def parse_canm(d, a, name):
    rev = struct.unpack_from('<I', d, a + 4)[0]
    loop = struct.unpack_from('<I', d, a + 0x10)[0]
    dur = struct.unpack_from('<f', d, a + 0x14)[0]
    flags = struct.unpack_from('<I', d, a + 0x18)[0]
    ntr = struct.unpack_from('<I', d, a + 0x1c)[0]
    trk = read_dict(d, a + 0x28)
    tracks = []
    if trk:
        for bn, ba in zip(*trk):
            t_flags, t_namep, c1, c2, c3 = struct.unpack_from('<5I', d, ba)
            # chiavi: quat stride 20B da +0x28 (finché finite)
            import math
            keys = []
            p = ba + 0x28
            while p + 20 <= len(d):
                v = struct.unpack_from('<4f', d, p + 4)  # quat nei float 1..4
                if not all(math.isfinite(x) and -2 < x < 2 for x in v):
                    break
                keys.append(v)
                p += 20
            tracks.append({'bone': bn, 'flags': hex(t_flags), 'c1': c1, 'c2': c2,
                           'c3': hex(c3), 'nkeys': len(keys), 'keys': keys})
    return {'name': name, 'rev': rev, 'loop': loop, 'duration': dur,
            'flags': hex(flags), 'ntracks': ntr, 'tracks': tracks}


def amb_entries(d):
    n = struct.unpack_from('<I', d, 0x10)[0]
    ents = []
    p = 0x18
    for i in range(n):
        off, size = struct.unpack_from('<2I', d, p)
        ents.append((off, size))
        p += 16
    return ents


if __name__ == '__main__':
    if '--bcres' in sys.argv:
        d = open(sys.argv[sys.argv.index('--bcres') + 1], 'rb').read()
        roots = [('direct', d)]
    else:
        d = open(sys.argv[1], 'rb').read()
        roots = [(f'entry{i}', d[o:o + s]) for i, (o, s) in enumerate(amb_entries(d))
                 if d[o:o + 4] == b'CGFX']
    for nm, cg in roots:
        root = parse_root(cg)
        cnt, sa = root['skel_anims']
        if not sa:
            continue
        names, addrs = sa
        print(f'== {nm}: {cnt} skel_anims')
        for an, aa in list(zip(names, addrs))[:6]:
            info = parse_canm(cg, aa, an)
            print(f"  {an}: dur={info['duration']} loop={info['loop']} "
                  f"tracks={info['ntracks']} → "
                  f"{[(t['bone'], t['nkeys'], t['c1'], t['c2']) for t in info['tracks'][:4]]}")
        print(f'  ... ({len(names)} totali: {names[:8]} ...)')
