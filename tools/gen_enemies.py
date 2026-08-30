#!/usr/bin/env python3
"""gen_enemies.py — spawn NEMICI reali dallo stream evt (modello runtime CORRETTO).

FORMATO OBJ RUNTIME (verificato su disasm handler[1] 0x13dc64 / handler[2] 0x146ca0
— ARM, code.bin; vedi docs/EVT_RE_DEEP.md "MODELLO OBJ v2"):
  obj 8B  (stream C): {u16 tv@0, u8 x@2 (×0.5), u8 f3@3 (heading), s15 y@4 (×0.5), u16 par@6}
  obj 16B (stream B): idem + p2@8, p3@12
  tv: type = tv & 0x1FF (bic 0xFE00), variant = (tv>>7)&0x3C, bit13/14 flag, bit15 done
  (il vecchio modello s8.8 {y lo16 | x hi16} era SBAGLIATO: era type|variant + x + f3!)

RECORD stream B/C = chunk runtime: {f32 X@0 (mondo!), 0x80000000@4 (fine catena),
  0x00640000@8, w3@0xc, groups@0x10}
GROUP: {u16 count@0, s16 anchor@2 (×100 = Y mondo), u32 size@4, objs@8}

TYPE-ID VERIFICATI (incrocio zone + kind-table slot4 thunks -> 0x303860/0x212a14):
  type 14 = E_BAT  (kind 0xF; tabella nomi @file 0x399494: gmBoostEnergy, E_BAT, ...)
  z11: 3 bat  X=230/1292/2187 Y=100.5/100.5/0.5 f3=164/140/164
  z12: 21 bat (X=1016..7587)

Mappatura sul corso di test del motore (coll assemblate 0..LEN):
  x_engine = (X_evt - X0) / (X1 - X0) * LEN   (X0/X1 = min/max chunk del livello)
  y = terreno sotto px + hover (l'Y evt va risolto col terrain 1:1, M6.6)
Uso: python3 tools/gen_enemies.py z11 [LEN]
"""
import struct, sys, json

E_BAT = 14


def parse_zone(fn):
    d = open(fn, 'rb').read()
    off_b, off_c = struct.unpack_from('<II', d, 4)
    objs = []
    chunks = []

    def walk(base, end, osz, strm):
        p = base
        while p + 8 <= end:
            prev, this = struct.unpack_from('<II', d, p)
            if this < 16 or p + this > end:
                break
            pay = d[p + 8:p + this]
            if len(pay) >= 0x18:
                chx = struct.unpack_from('<f', pay, 0)[0]
                if 0.0 < chx < 100000.0:
                    chunks.append(chx)
                q = 0x10
                while q + 8 <= len(pay):
                    count, anchor, size = struct.unpack_from('<HHi', pay, q)
                    if count == 0 and size == 0:
                        break
                    if count > 0x400:
                        break
                    if size == 0:
                        size = len(pay) - q
                    if size < 8 or q + size > len(pay):
                        break
                    for i in range(count):
                        o = q + 8 + i * osz
                        if o + osz > len(pay):
                            break
                        w0, p1 = struct.unpack_from('<2I', pay, o)
                        tv = w0 & 0x7FFF
                        t = tv & 0x1FF
                        y15 = p1 & 0x7FFF
                        if y15 & 0x4000:
                            y15 -= 0x8000
                        objs.append({
                            'type': t, 'variant': (tv >> 7) & 0x3C,
                            'x': chx + ((w0 >> 16) & 0xFF) * 0.5,
                            'y': anchor * 100 + y15 * 0.5,
                            'anchor': anchor, 'f3': (w0 >> 24) & 0xFF,
                            'par': p1 >> 16, 'strm': strm,
                        })
                    q += size
            p += this

    walk(off_b, off_c, 16, 'B')
    walk(off_c, len(d), 8, 'C')
    return objs, chunks


def main():
    zone = sys.argv[1]
    length = float(sys.argv[2]) if len(sys.argv) > 2 else 120.0
    objs, chunks = parse_zone(f'sgds-data/evt/{zone}evt.bin')
    x0, x1 = min(chunks), max(chunks) + 128.0
    bats = [o for o in objs if o['type'] == E_BAT]
    # dedup (chunk overlap nello streaming) per (x arrotondato, f3)
    seen, uniq = set(), []
    for o in sorted(bats, key=lambda o: o['x']):
        k = (round(o['x']), o['f3'])
        if k in seen:
            continue
        seen.add(k)
        uniq.append(o)
    json.dump({'x0': x0, 'x1': x1, 'objs': objs},
              open(f'sgds-data/enemies/{zone}_objs.json', 'w'), indent=1)
    hdr = [f'/* enemy_{zone}.h — spawn E_BAT ORIGINALI da {zone}evt.bin (type 14, stream C) */',
           f'/* {len(uniq)} nemici; x evt [{x0:.0f}..{x1:.0f}] -> corso [0..{length:.0f}] */',
           '#include <nds.h>', '',
           f'#define {zone.upper()}_NENE {len(uniq)}',
           f'const s32 {zone}_enemies[{zone.upper()}_NENE][3] = {{  /* x(f16), hover, f3 */']
    for o in uniq:
        ex = (o['x'] - x0) / (x1 - x0) * length
        hdr.append(f'  {{ {int(round(ex * 65536))}, {10 + (o["anchor"] & 1) * 4} << 16, {o["f3"]} }},'
                   f'  /* evt X={o["x"]:.0f} Y={o["y"]:.1f} f3={o["f3"]} {o["strm"]} */')
    hdr += ['};', '']
    out = f'engine/source/enemy_{zone}.h'
    open(out, 'w').write('\n'.join(hdr))
    print(f'{out}: {len(uniq)} E_BAT; livello evt x[{x0:.0f}..{x1:.0f}]')
    for o in uniq:
        print(f"   X={o['x']:7.1f} Y={o['y']:6.1f} f3={o['f3']:3d} -> engine x={(o['x']-x0)/(x1-x0)*length:6.1f}")


if __name__ == '__main__':
    main()
