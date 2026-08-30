#!/usr/bin/env python3
"""evt_re.py — RE dei file evt di SGDS (placement originale).

Struttura confermata:
  header: [0]=nsezioni(12), [1]=off blob1, [2]=off blob2, ...
  record: {u32 prev_size, u32 this_size, payload[this_size-8]}
          catena: next = cur + this_size, prev control == prev_size.
  coordinate: u32 = (x_s8.8 << 16) | (y_s8.8 & 0xffff)

Uso: python3 evt_re.py FILE.evt.bin [--all]
"""
import struct, sys

def s88(w):
    x = (w >> 16)
    y = (w & 0xffff)
    if x >= 0x8000: x -= 0x10000
    if y >= 0x8000: y -= 0x10000
    return x / 256.0, y / 256.0

def walk_records(d, base, end):
    recs = []
    p = base
    while p + 8 <= end:
        prev, this = struct.unpack_from('<II', d, p)
        if this < 8 or this > 0x2000 or p + this > end:
            break
        recs.append((p, prev, this, d[p+8:p+this]))
        if this == 0:
            break
        p += this
    return recs

def words_of(payload):
    n = len(payload) // 4
    return list(struct.unpack_from(f'<{n}I', payload, 0)) if n else []

def fmt_word(w):
    x, y = s88(w)
    looks_coord = abs(x) < 2000 and abs(y) < 2000 and (x, y) != (0, 0)
    f = struct.unpack('<f', struct.pack('<I', w))[0]
    fs = f'{f:.2f}' if 0.01 < abs(f) < 1e5 else ''
    tag = ''
    if looks_coord and abs(x) > 0.1 and abs(y) > 0.1:
        tag = f'≈({x:.2f},{y:.2f})'
    elif fs:
        tag = f'f={fs}'
    return f'{w:08x}{(" "+tag) if tag else ""}'

def analyze(path):
    d = open(path, 'rb').read()
    n = struct.unpack_from('<I', d, 0)[0]
    offs = struct.unpack_from('<6I', d, 4)
    print(f'{path}: len={len(d)} sezioni={n} offs={[hex(o) for o in offs[:3]]}')
    blobs = []
    for i, o in enumerate(offs[:3]):
        if o == 0: continue
        end = offs[i+1] if i < 2 and offs[i+1] > o else len(d)
        # il blob vero inizia quando compaiono i record: prova da o
        recs = walk_records(d, o, end)
        if recs:
            blobs.append((o, end, recs))
    for base, end, recs in blobs:
        print(f'-- blob @{base:#x}..{end:#x}: {len(recs)} record, payload medio '
              f'{sum(len(r[3]) for r in recs)//max(len(recs),1)}B')
        # istogramma size
        from collections import Counter
        print('   sizes:', dict(Counter(r[2] for r in recs)))
    return d, blobs

if __name__ == '__main__':
    for path in sys.argv[1:]:
        if path.startswith('--'): continue
        analyze(path)
