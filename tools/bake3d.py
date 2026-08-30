#!/usr/bin/env python3
"""bake3d.py — converte le mesh CTR-GFX decodificate (gfx_mesh.py) in array C
per il motore 3D DS (render3d). TUTTO da asset originali:
  - vertici -> v16 4.12 (scala 1/16 in unita' bake, unita' mondo mantenute a runtime)
  - uv      -> t16 12.4 (per texture size)
  - colori  -> RGB15 dai vertex color originali
  - indici  -> u16 triangoli (strip/fan -> tri)
  - texture -> 8bpp RGB256 lineare row-major + palette 256 (median cut dai pixel RGBA originali)

Uso:
  python3 bake3d.py --zone z11 --out engine/data
  python3 bake3d.py --model sgds-data/objects/g_cmn_ring.bcres --name ring --out engine/data
"""
import os, sys, struct, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gfx_mesh as G

# ---------------- quantizzazione colore (median cut) ----------------
def median_cut(pixels, n=256):
    """pixels: list[(r,g,b,a)] -> palette, index map"""
    # considera solo alpha>0 per la palette (trasparenza gestita a parte)
    opaque = [p for p in pixels if p[3] >= 8]
    if not opaque:
        opaque = pixels
    buckets = [opaque]
    while len(buckets) < n:
        # split del bucket con maggiore range
        best, br = None, -1
        for i, b in enumerate(buckets):
            if len(b) < 2: continue
            for ch in range(3):
                lo = min(p[ch] for p in b); hi = max(p[ch] for p in b)
                r = (hi - lo) #- len(b)*0.0
                if r > br: br, best = r, (i, ch)
        if best is None: break
        i, ch = best
        b = sorted(buckets[i], key=lambda p: p[ch])
        mid = len(b)//2
        buckets[i] = b[:mid]; buckets.append(b[mid:])
    pal = []
    for b in buckets:
        if not b: continue
        r = sum(p[0] for p in b)//len(b)
        g = sum(p[1] for p in b)//len(b)
        bl = sum(p[2] for p in b)//len(b)
        pal.append((r, g, bl))
    return pal

def pal_index(pal, c):
    best, bd = 0, 1<<30
    for i, (r, g, b) in enumerate(pal):
        d = (r-c[0])**2 + (g-c[1])**2 + (b-c[2])**2
        if d < bd: bd, best = d, i
    return best

def rgb15(c):
    return (c[0]>>3) | ((c[1]>>3) << 5) | ((c[2]>>3) << 10)

# ---------------- texture bake ----------------
def bake_texture(t, max_pal=256):
    dec = G.texture_rgba(t)
    if not dec: return None
    w, h, px = dec
    flat = [px[y][x] for y in range(h) for x in range(w)]
    pal = median_cut(flat, max_pal)
    # row-major lineare -> u8 indici (NON tile 8x8: melonDS, nostro verificatore,
    # campiona la VRAM texture linearmente; tile 8x8 rendeva i poligoni a blocchi)
    data = bytearray()
    for yy in range(h):
        for xx in range(w):
            data.append(pal_index(pal, px[yy][xx]))
    pal16 = [rgb15(c) for c in pal]
    return {'w': w, 'h': h, 'data': bytes(data), 'pal': pal16}

def log2sz(n):
    s = 0
    while (1 << s) < n: s += 1
    return s

# ---------------- mesh bake ----------------
def tri_list(prims):
    tris = []
    for pr in prims:
        idx, mode = pr['indices'], pr['prim']
        if mode == 0:
            for k in range(0, len(idx)-2, 3):
                tris += [idx[k], idx[k+1], idx[k+2]]
        elif mode == 1:
            for k in range(1, len(idx)-1):
                a, b, c = idx[k-1], idx[k], idx[k+1]
                if k % 2 == 0: b, c = c, b
                tris += [a, b, c]
        elif mode == 2:
            for k in range(1, len(idx)-1):
                tris += [idx[0], idx[k], idx[k+1]]
    return tris

SCALE = 512  # v16 = world << 9 (4.12: world/8, scala 8 a runtime)

def bake_mesh(g, textures, tex_names):
    """g = mesh geom da gfx_mesh.extract -> dict arrays"""
    pos, uv, col = g['pos'], g['uv'] or [], g['col'] or []
    if not pos: return None
    po = g['pos_offset'] or [0, 0, 0]
    nv = len(pos)
    vtx = bytearray()
    uvs = bytearray()
    cols = bytearray()
    for i in range(nv):
        x = round((pos[i][0] + po[0]) * SCALE)
        y = round((pos[i][1] + po[1]) * SCALE)
        z = round((pos[i][2] + po[2]) * SCALE)
        for v in (x, y, z):
            vtx += struct.pack('<h', max(-32768, min(32767, v)))
        if i < len(uv):
            u = round(uv[i][0] * 4096)   # t16 12.4 relativo (scala tex a runtime)
            v = round(uv[i][1] * 4096)
            uvs += struct.pack('<hh', max(-32768, min(32767, u)), max(-32768, min(32767, v)))
        else:
            uvs += struct.pack('<hh', 0, 0)
        if i < len(col):
            r = max(0, min(31, round(col[i][0] * 31)))
            g_ = max(0, min(31, round(col[i][1] * 31)))
            b = max(0, min(31, round(col[i][2] * 31)))
            cols += struct.pack('<H', r | (g_ << 5) | (b << 10))
        else:
            cols += struct.pack('<H', 0x7FFF)
    tris = []
    for part in g['parts']:
        tris += tri_list(part['prims'])
    tris = [t for t in tris if t < nv]
    tex = -1
    mats = g.get('mats')
    return {'nv': nv, 'ntri': len(tris)//3,
            'vtx': bytes(vtx), 'uv': bytes(uvs), 'col': bytes(cols),
            'idx': struct.pack(f'<{len(tris)}H', *tris),
            'tex': g.get('tex_index', -1),
            'bbox': g.get('bbox')}

# ---------------- emitter C ----------------
def c_arr(name, typ, data):
    out = [f'const {typ} {name}[] = {{']
    if typ == 'u8':
        vals = [f'0x{b:02x}' for b in data]
    elif typ == 'u16':
        vals = [f'0x{struct.unpack_from("<H", data, i)[0]:04x}' for i in range(0, len(data), 2)]
    else:
        vals = []
    for i in range(0, len(vals), 16):
        out.append('  ' + ','.join(vals[i:i+16]) + ',')
    out.append('};')
    return '\n'.join(out)

def bake_model_file(bcres, cname, outdir, texfmt='RGB256'):
    r, root, textures, models = G.extract(bcres)
    os.makedirs(outdir, exist_ok=True)
    tex_arrs, baked = [], []
    manifest = {'file': bcres, 'models': []}
    for mi, m in enumerate(models):
        mstart = len(baked)
        for g in m['meshes']:
            bm = bake_mesh(g, textures, list(textures))
            if bm and bm['ntri'] > 0:
                # texture del materiale
                tnames = []
                try:
                    mat = m['materials'][g['material']]
                    tnames = [t for t in mat['textures'] if not t.endswith('_sp')]
                except Exception:
                    pass
                bm['tex'] = tnames[0] if tnames else None
                baked.append(bm)
        if any(b for b in baked[mstart:] if b['ntri'] > 0):
            manifest['models'].append({'name': m['name'], 'mesh_start': mstart,
                                       'mesh_count': len(baked) - mstart,
                                       't_trans': m.get('t_trans')})
    # texture usate
    used = []
    for bm in baked:
        if bm['tex'] and bm['tex'] not in [u['name'] for u in used]:
            t = textures.get(bm['tex'])
            if t:
                bt = bake_texture(t)
                if bt:
                    bt['name'] = bm['tex']
                    used.append(bt)
    tmap = {u['name']: i for i, u in enumerate(used)}
    hdr = [f'/* generato da bake3d.py da {os.path.basename(bcres)} — asset originali */',
           '#include "model3d.h"', '']
    for i, u in enumerate(used):
        hdr.append(c_arr(f'{cname}_tex{i}_px', 'u8', u['data']))
        pal = list(u['pal']) + [0] * (256 - len(u['pal']))
        hdr.append(c_arr(f'{cname}_tex{i}_pal', 'u16', struct.pack('<256H', *pal)))
    mesh_tbl = []
    for j, bm in enumerate(baked):
        hdr.append(c_arr(f'{cname}_m{j}_vtx', 'u16', bm['vtx']))
        hdr.append(c_arr(f'{cname}_m{j}_uv', 'u16', bm['uv']))
        hdr.append(c_arr(f'{cname}_m{j}_col', 'u16', bm['col']))
        hdr.append(c_arr(f'{cname}_m{j}_idx', 'u16', bm['idx']))
        ti = tmap.get(bm['tex'], -1)
        mesh_tbl.append((j, bm['nv'], bm['ntri'], ti))
    hdr.append(f'const R3DMesh {cname}_meshes[{len(baked)}] = {{')
    for j, nv, ntri, ti in mesh_tbl:
        hdr.append(f'  {{ {cname}_m{j}_vtx, {cname}_m{j}_uv, {cname}_m{j}_idx, {ntri}, {ti}, {cname}_m{j}_col }},')
    hdr.append('};')
    hdr.append(f'const R3DTexture {cname}_texs[{len(used)}] = {{')
    for i, u in enumerate(used):
        hdr.append(f'  {{ {cname}_tex{i}_pal, {cname}_tex{i}_px, '
                   f'{log2sz(u["w"])}, {log2sz(u["h"])} }},')
    hdr.append('};')
    hdr.append(f'const R3DModel {cname} = {{ {cname}_meshes, {len(baked)}, '
               f'{cname}_texs, {len(used)} }};')
    out = os.path.join(outdir, f'model_{cname}.h')
    open(out, 'w').write('\n'.join(hdr) + '\n')
    json.dump(manifest, open(os.path.join(outdir, f'manifest_{cname}.json'), 'w'), indent=1)
    nv = sum(b['nv'] for b in baked); nt = sum(b['ntri'] for b in baked)
    print(f'{bcres}: {len(baked)} mesh, {nv} vtx, {nt} tri, {len(used)} tex -> {out}')
    return out

if __name__ == '__main__':
    args = sys.argv[1:]
    outdir = 'engine/data'
    if '--out' in args: outdir = args[args.index('--out')+1]
    if '--model' in args:
        src = args[args.index('--model')+1]
        name = args[args.index('--name')+1] if '--name' in args else \
            os.path.basename(src).replace('.bcres', '')
        bake_model_file(src, name, outdir)
