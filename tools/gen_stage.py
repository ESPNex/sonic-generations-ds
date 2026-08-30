#!/usr/bin/env python3
"""gen_stage.py — genera stage_zXX.h dal placement evt ORIGINALE (evt_parse4).

Uso: python3 tools/gen_stage.py z11 [stream]   (stream: C = piano di gioco, B = livelli alti, BC = entrambi)
Ogni oggetto: {x, y} s8.8 evt (= unità mondo ground-compatibili), type = type record.
Nel world3d: world_x = x (u), world_y = y (u).
"""
import sys, struct

sys.path.insert(0, 'tools')
from evt_parse4 import parse_file


def gen(zone, streams='C'):
    r = parse_file(f'sgds-data/evt/{zone}evt.bin')
    objs = []
    for nm in streams:
        for rec in r[nm]['recs']:
            for g in rec['groups']:
                for o in g['objs']:
                    objs.append((o['x'], o['y'], rec['type'], nm, o.get('p1', 0)))
    n = len(objs)
    hdr = [f'/* stage_{zone}.h — placement ORIGINALE da {zone}evt.bin (evt_parse4 deep-RE) */',
           f'/* {n} oggetti, stream {"+".join(streams)}; type = tipo record evt */',
           '#include "model3d.h"', '',
           f'#define {zone.upper()}_NOBJ {n}',
           f'const R3DInstance {zone}_objs[{zone.upper()}_NOBJ] = {{']
    for x, y, t, s, p1 in objs:
        ix, iy = int(round(x * 65536)), int(round(y * 65536))
        hdr.append(f'  {{ NULL, {ix}, {iy}, 0, {t}, {(s == "B") << 8} }},  /* x={x} y={y} type={t} {s} p1={p1:#x} */')
    hdr += ['};', '']
    out = f'engine/source/stage_{zone}.h'
    open(out, 'w').write('\n'.join(hdr))
    print(f'{out}: {n} oggetti')
    return n


if __name__ == '__main__':
    zone = sys.argv[1]
    streams = sys.argv[2] if len(sys.argv) > 2 else 'C'
    gen(zone, streams)
