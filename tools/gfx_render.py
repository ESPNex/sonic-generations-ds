#!/usr/bin/env python3
"""gfx_render.py — verifica visiva OFFLINE dei mesh decodificati (render ortografico).
Per ogni modello: proietta XY (Z=depth), rasterizza i triangoli con UV -> texture.
Se i blocchi appaiono corretti, la decodifica è giusta e si porta al DS.
Uso: python3 gfx_render.py FILE.bcres OUTDIR [N]
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gfx_mesh as G

W, H = 256, 192

def tex_pixels(texcache, name):
    if name in texcache: return texcache[name]
    t = TEXTURES.get(name)
    if not t: return None
    dec = G.texture_rgba(t)
    if not dec: return None
    w, h, px = dec
    texcache[name] = (w, h, px)
    return texcache[name]

def render_model(m, path_out):
    fb = [[(10, 14, 40)]*W for _ in range(H)]
    zb = [[1e9]*W for _ in range(H)]
    texcache = {}
    allp = []
    for g in m['meshes']:
        pos = g['pos'] or []
        if not pos: continue
        po = g['pos_offset'] or [0,0,0]
        allp += [(p[0]+po[0], p[1]+po[1], p[2]+po[2]) for p in pos]
    if not allp: return False
    xs = [p[0] for p in allp]; ys = [p[1] for p in allp]
    minx, maxx = min(xs), max(xs); miny, maxy = min(ys), max(ys)
    sx = (W-16) / max(maxx-minx, 1e-6); sy = (H-16) / max(maxy-miny, 1e-6)
    s = min(sx, sy)
    ox = 8 - minx*s; oy = 8 - miny*s

    def tri(v0, v1, v2, uv0, uv1, uv2, tex):
        pts = [(v[0]*s+ox, v[1]*s+oy, v[2]) for v in (v0, v1, v2)]
        minx_ = max(0, int(min(p[0] for p in pts))); maxx_ = min(W-1, int(max(p[0] for p in pts))+1)
        miny_ = max(0, int(min(p[1] for p in pts))); maxy_ = min(H-1, int(max(p[1] for p in pts))+1)
        x0, y0, z0 = pts[0]; x1, y1, z1 = pts[1]; x2, y2, z2 = pts[2]
        d = (y1-y2)*(x0-x2) + (x2-x1)*(y0-y2)
        if abs(d) < 1e-9: return
        for Y in range(miny_, maxy_+1):
            for X in range(minx_, maxx_+1):
                l0 = ((y1-y2)*(X-x2) + (x2-x1)*(Y-y2)) / d
                l1 = ((y2-y0)*(X-x2) + (x0-x2)*(Y-y2)) / d
                l2 = 1 - l0 - l1
                if l0 < 0 or l1 < 0 or l2 < 0: continue
                Z = l0*z0 + l1*z1 + l2*z2
                if Z >= zb[Y][X]: continue
                if tex:
                    tw, th, px = tex
                    u = l0*uv0[0] + l1*uv1[0] + l2*uv2[0]
                    v = l1*0 + l0*0
                    v = l0*uv0[1] + l1*uv1[1] + l2*uv2[1]
                    tx = int(u*tw) % tw; ty = int(v*th) % th
                    c = px[ty][tx]
                    if len(c) == 4 and c[3] < 32: continue
                else:
                    c = (255, 0, 255)
                zb[Y][X] = Z; fb[Y][X] = c[:3]
    for g in m['meshes']:
        pos = g['pos'] or []; uv = g['uv'] or []
        if not pos: continue
        po = g['pos_offset'] or [0,0,0]
        V = [(p[0]+po[0], p[1]+po[1], p[2]+po[2]) for p in pos]
        mi = g['material']
        mats = m['materials']
        tname = None
        if 0 <= mi < len(mats):
            cands = [t for t in mats[mi]['textures'] if not t.endswith('_sp')]
            if cands: tname = cands[0]
        tex = tex_pixels(texcache, tname) if tname else None
        def UV(i):
            if i < len(uv) and len(uv[i]) >= 2: return (uv[i][0], uv[i][1])
            return (0.0, 0.0)
        for part in g['parts']:
            for pr in part['prims']:
                idx = pr['indices']; mode = pr['prim']
                tris = []
                if mode == 0:
                    for k in range(0, len(idx)-2, 3): tris.append((idx[k], idx[k+1], idx[k+2]))
                elif mode == 1:
                    for k in range(1, len(idx)-1):
                        a, b, c = idx[k-1], idx[k], idx[k+1]
                        if k % 2 == 0: b, c = c, b
                        tris.append((a, b, c))
                elif mode == 2:
                    for k in range(1, len(idx)-1): tris.append((idx[0], idx[k], idx[k+1]))
                for a, b, c in tris:
                    if max(a, b, c) >= len(V): continue
                    tri(V[a], V[b], V[c], UV(a), UV(b), UV(c), tex)
    with open(path_out, 'wb') as f:
        f.write(f'P6\n{W} {H}\n255\n'.encode())
        f.write(b''.join(bytes(c) for row in fb for c in row))
    return True

if __name__ == '__main__':
    src, outdir = sys.argv[1], sys.argv[2]
    N = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    os.makedirs(outdir, exist_ok=True)
    global TEXTURES
    r, root, TEXTURES, models = G.extract(src)
    n = 0
    for m in models:
        if not any(g['pos'] for g in m['meshes']): continue
        nm = m['name'].replace('/', '_')
        if render_model(m, os.path.join(outdir, f'{nm}.ppm')):
            n += 1
        if n >= N: break
    print(f'{n} render in {outdir}')
