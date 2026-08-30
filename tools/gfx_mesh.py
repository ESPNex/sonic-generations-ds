#!/usr/bin/env python3
"""gfx_mesh.py v2 — decoder DIRETTO dei .bcres (CTR-GFX rev vecchia) di SGDS.

Layout empiricamente validato su z11_mdl.bcres (incano: bbox 38.44/26.73/17.83,
vertici float 12.18/9.01/15.89, stride 48 = pos3f+norm3f+col4f+uv2f, indici u8):

  container: 'CGFX' BOM u16 HdrLen u16 Rev u32 FileLen i32 NSec i32 ['DATA' len]
             16 GfxDict inline {count i32, ptr selfrel}
             GfxDict@ptr: 'DICT' u32, treelen u32, nvals u32,
                          (nvals+1) x {refbit u32, l u16, r u16, name ptr, val ptr}
  ogni oggetto: [typeid u32][magic4][rev u32][name ptr][meta{c,p}]
  puntatori: u32 SELF-RELATIVI con wrap ((fieldpos + val) & 0xffffffff)

  CMDL 0x40000092 (GfxModelSkeletal, 0xe4 + liste):
     +0x18 unk, +0x1c unk, +0x20 childs{c,p}, +0x28 anim{c,p},
     +0x30 t_scale v3, +0x3c t_rot v3, +0x48 t_trans v3,
     +0x54 local m3x4, +0x84 world m3x4,
     +0xb4 meshes{c,p}, +0xbc materials dict{c,p}, +0xc4 shapes{c,p},
     +0xcc meshnode_vis{c,p}, +0xd4 flags, +0xd8 culling, +0xdc layer, +0xe0 skel ref
  MESH 0x01000000 (0x80): +0x18 shape_idx i32, +0x1c mat_idx i32, +0x20 parent ref,
     +0x24 vis u8+prio u8+node i16, +0x28 prim_idx, +0x2c flags, +0x30 cmds[12],
     +0x60 4*u32, +0x70 nodename ref, +0x74 u64, +0x7c u32
  SHAPE 0x10000001 (0x8c): +0x18 sflags, +0x1c unk, +0x20 pos_off v3,
     +0x2c submeshes{c,p}, +0x34 base, +0x38 vbs{c,p}, +0x40 blendref, +0x4c unk,
     +0x50 bbox{center v3, orient m3x3, size v3}
  SUBMESH: {bone_idx{c,p}, skinning u32, faces{c,p}}
  FACE: {descs{c,p}, bufobjs{c,p}, flags u32, cmdalloc u32}
  DESC: {format u32(GL), prim u8, vis u8, pad2, raw{c,p}, 7*u32}
  VB-INTERLEAVED 0x40000002: {attr u32(21), vtype, bufobj, locflag, raw{c,p},
     locptr, mem, stride i32, attrs{c,p}}
  VB-ATTR 0x40000001 (0x34): {attr u32, vtype, bufobj, locflag, raw{c,p},
     locptr, mem, format u32, elements i32, scale f32, offset i32}
  TXOB-img 0x20000011: +0x18 h, +0x1c w, +0x34 hwfmt, +0x38 unk,
     +0x3c IMAGEDATA inline {h,w, raw{c,p}, dyn, bpp, locptr, mem}
  TXOB-ref 0x20000004: header + path ref + texptr
  MTOB 0x08000000: texture risolte scansionando ref -> oggetti 0x20000004

Uso: python3 gfx_mesh.py FILE.bcres OUTDIR [--obj]
"""
import os, struct, sys, json

class R:
    def __init__(s, d): s.d = d; s.p = 0; s.objs = {}
    def u8(s):  v = s.d[s.p]; s.p += 1; return v
    def u16(s): v = struct.unpack_from('<H', s.d, s.p)[0]; s.p += 2; return v
    def i16(s): v = struct.unpack_from('<h', s.d, s.p)[0]; s.p += 2; return v
    def u32(s): v = struct.unpack_from('<I', s.d, s.p)[0]; s.p += 4; return v
    def i32(s): v = struct.unpack_from('<i', s.d, s.p)[0]; s.p += 4; return v
    def f32(s): v = struct.unpack_from('<f', s.d, s.p)[0]; s.p += 4; return v
    def u64(s): v = struct.unpack_from('<Q', s.d, s.p)[0]; s.p += 8; return v
    def f32n(s, n): return list(struct.unpack_from(f'<{n}f', s.d, s.p)) or None

def u32at(r, a): return struct.unpack_from('<I', r.d, a)[0]
def i32at(r, a): return struct.unpack_from('<i', r.d, a)[0]
def f32at(r, a): return struct.unpack_from('<f', r.d, a)[0]
def selfrel(a, val): return (a + val) & 0xFFFFFFFF

def cstr(r, a):
    if not a or a >= len(r.d): return None
    try:
        e = r.d.index(b'\0', a)
    except ValueError:
        return None
    s = r.d[a:e]
    return s.decode('utf-8', 'replace') if s else None

def hdr(r, a):
    """header comune oggetto -> dict"""
    o = {'_addr': a, 'tid': u32at(r, a), 'magic': r.d[a+4:a+8].decode('latin1'),
         'rev': u32at(r, a+8),
         'name': cstr(r, selfrel(a+0x0c, u32at(r, a+0x0c)))}
    return o

def listref(r, a):
    """{count i32, ptr} all'indirizzo a -> (count, lista_addr)"""
    c = i32at(r, a)
    p = selfrel(a+4, u32at(r, a+4))
    return c, p

def read_dict(r, a):
    """DICT@ -> (names, value_addrs)"""
    if r.d[a:a+4] != b'DICT': return None
    _save = r.p
    nvals = u32at(r, a+8)
    names, vals = [], []
    p = a + 12
    for i in range(nvals+1):
        r.p = p
        r.u32(); r.u16(); r.u16()
        np = selfrel(r.p, r.u32()); vp = selfrel(r.p, r.u32())
        if i > 0:
            names.append(cstr(r, np)); vals.append(vp)
        p += 16
    r.p = _save
    return names, vals

def read_objs_list(r, lst, n):
    out = []
    for i in range(n):
        e = u32at(r, lst+4*i)
        out.append(selfrel(lst+4*i, e))
    return out

# --------------------------------------------------------------- oggetti mesh
def parse_shape(r, a):
    sh = hdr(r, a)
    sh['pos_offset'] = r.f32n(3, a+0x20) if False else list(struct.unpack_from('<3f', r.d, a+0x20))
    c, l = listref(r, a+0x2c)
    sh['submeshes'] = read_objs_list(r, l, c) if c > 0 else []
    c, l = listref(r, a+0x38)
    sh['vbs'] = read_objs_list(r, l, c) if c > 0 else []
    sh['bbox'] = {'center': list(struct.unpack_from('<3f', r.d, a+0x50)),
                  'size':   list(struct.unpack_from('<3f', r.d, a+0x78))}
    return sh

def parse_submesh(r, a):
    c, l = listref(r, a)
    bi = [i32at(r, l+4*i) for i in range(c)] if c > 0 else []
    sk = u32at(r, a+8)
    c, l = listref(r, a+12)
    return {'bones': bi, 'skinning': sk, 'faces': read_objs_list(r, l, c) if c > 0 else []}

def parse_face(r, a):
    c, l = listref(r, a)
    return {'descs': read_objs_list(r, l, c) if c > 0 else []}

def parse_desc(r, a):
    fmt = u32at(r, a)
    prim = r.d[a+4]; vis = r.d[a+5]
    c, l = listref(r, a+8)
    data = r.d[l:l+c] if c > 0 and l < len(r.d) else b''
    if fmt == 0x1403:
        idx = list(struct.unpack_from(f'<{len(data)//2}H', data)) if data else []
    else:
        idx = list(data)
    return {'format': fmt, 'prim': prim, 'vis': vis, 'indices': idx}

GL_NAMES = {0:'Position',1:'Normal',2:'Tangent',3:'Color',4:'TexCoord0',5:'TexCoord1',
            6:'TexCoord2',7:'BoneIndex',8:'BoneWeight',21:'Interleave'}
GLFMT = {0x1400:('b',1),0x1401:('B',1),0x1402:('h',2),0x1403:('H',2),0x1406:('f',4),0x140C:('h',2)}

def parse_vb(r, a):
    """buffer vertici -> dict con attributi decodificabili"""
    tid = u32at(r, a)
    if tid == 0x40000002:      # interleaved
        c, l = listref(r, a+0x14)
        raw = r.d[l:l+c] if c > 0 else b''
        stride = i32at(r, a+0x24)
        c, l = listref(r, a+0x28)
        attrs = []
        for aa in read_objs_list(r, l, c):
            at = parse_attr(r, aa)
            if at: attrs.append(at)
        return {'kind': 'interleaved', 'stride': stride, 'raw': raw, 'attrs': attrs}
    raise ValueError(f'vb tid 0x{tid:08x}')

def parse_attr(r, a):
    if u32at(r, a) != 0x40000001: return None
    name = u32at(r, a+4)
    c, l = listref(r, a+0x14)
    raw = r.d[l:l+c] if c > 0 and l < len(r.d) else b''
    return {'name': GL_NAMES.get(name, f'attr{name}'), 'format': u32at(r, a+0x24),
            'elements': i32at(r, a+0x28), 'scale': f32at(r, a+0x2c),
            'offset': i32at(r, a+0x30), 'raw': raw}

def decode_attr(at, stride, raw):
    code, sz = GLFMT[at['format']]
    n = (len(raw) // stride) if stride > 0 else (len(raw) // (at['elements']*sz))
    out = []
    el = at['elements']; sc = at['scale']; off = at['offset']
    for i in range(n):
        base = off + i*stride if stride > 0 else i*el*sz
        if base + el*sz > len(raw): break
        v = list(struct.unpack_from(f'<{el}{code}', raw, base))
        out.append([x*sc for x in v])
    return out

# --------------------------------------------------------------- modelli
def parse_model(r, a):
    m = hdr(r, a)
    m['t_scale'] = list(struct.unpack_from('<3f', r.d, a+0x30))
    m['t_trans'] = list(struct.unpack_from('<3f', r.d, a+0x48))
    m['local_m'] = list(struct.unpack_from('<12f', r.d, a+0x54))
    m['world_m'] = list(struct.unpack_from('<12f', r.d, a+0x84))
    c, l = listref(r, a+0xb4); m['meshes'] = read_objs_list(r, l, c) if c > 0 else []
    c, l = listref(r, a+0xbc)
    dd = read_dict(r, l) if c > 0 and l < len(r.d) else None
    m['materials'] = {'names': dd[0], 'addrs': dd[1]} if dd else {'names': [], 'addrs': []}
    c, l = listref(r, a+0xc4); m['shapes'] = read_objs_list(r, l, c) if c > 0 else []
    sk = selfrel(a+0xe0, u32at(r, a+0xe0))
    m['skeleton'] = sk if sk and sk < len(r.d) and u32at(r, sk) == 0x02000000 else None
    return m

def parse_mesh(r, a):
    m = hdr(r, a)
    m['shape_index'] = i32at(r, a+0x18)
    m['material_index'] = i32at(r, a+0x1c)
    return m

# --------------------------------------------------------------- texture
_T = [[2,8,-2,-8],[5,17,-5,-17],[9,29,-9,-29],[13,42,-13,-42],
      [18,60,-18,-60],[24,80,-24,-80],[33,106,-33,-106],[47,183,-47,-183]]
def etc1_block(d, o):
    """ETC1 3DS: bit esatti (diff=33, flip=32, cw1@37..35, cw2@34..32;
    diff: R@63..59 G@55..51 B@47..43 + dR/dG/dB@58..56/50..48/42..40;
    indiv: R1@63..60 G1@55..52 B1@47..44 R2@59..56 G2@51..48 B2@43..40;
    indice pixel (y3,x3) = bit (4*x3+y3) LSB, (4*x3+y3+16) MSB)."""
    b = d[o:o+8]
    if len(b) < 8: return [[(0,0,0)]*4 for _ in range(4)]
    data = struct.unpack_from('<Q', b, 0)[0]
    diff = ((data >> 33) & 1) == 1
    flip = ((data >> 32) & 1) == 1
    if diff:
        r=(data>>59)&31; g=(data>>51)&31; b=(data>>43)&31
        r1=(r<<3)|((r&28)>>2); g1=(g<<3)|((g&28)>>2); b1=(b<<3)|((b&28)>>2)
        dr=(data>>56)&7; dg=(data>>48)&7; db=(data>>40)&7
        r += dr-8 if dr>=4 else dr
        g += dg-8 if dg>=4 else dg
        b += db-8 if db>=4 else db
        r2=(r<<3)|((r&28)>>2); g2=(g<<3)|((g&28)>>2); b2=(b<<3)|((b&28)>>2)
    else:
        r1=((data>>60)&15)*17; g1=((data>>52)&15)*17; b1=((data>>44)&15)*17
        r2=((data>>56)&15)*17; g2=((data>>48)&15)*17; b2=((data>>40)&15)*17
    T1=(data>>37)&7; T2=(data>>34)&7
    out=[[(0,0,0)]*4 for _ in range(4)]
    for y3 in range(4):
        for x3 in range(4):
            val=(data>>(x3*4+y3))&1
            neg=((data>>(x3*4+y3+16))&1)==1
            if (flip and y3<2) or (not flip and x3<2):
                add=_T[T1][(2 if neg else 0)+val]
                c=(max(0,min(255,r1+add)),max(0,min(255,g1+add)),max(0,min(255,b1+add)))
            else:
                add=_T[T2][(2 if neg else 0)+val]
                c=(max(0,min(255,r2+add)),max(0,min(255,g2+add)),max(0,min(255,b2+add)))
            out[y3][x3]=c
    return out

def decode_etc1(d, w, h, a4=False):
    px = [[(0,0,0,255)]*w for _ in range(h)]
    tw8 = (w+7)//8
    bsz = 16 if a4 else 8
    tsize = 4*bsz
    for ty8 in range((h+7)//8):
        for tx8 in range(tw8):
            base = (ty8*tw8+tx8)*tsize
            for sub in range(4):
                # ETC1A4: alpha PRIMA (8 byte), colore DOPO (+8)
                blk = etc1_block(d, base+sub*bsz + (8 if a4 else 0))
                ox, oy = (sub&1)*4, (sub>>1)*4
                for yy in range(4):
                    for xx in range(4):
                        X, Y = tx8*8+ox+xx, ty8*8+oy+yy
                        if X < w and Y < h:
                            c = blk[yy][xx]; a = 255
                            if a4:
                                ab = d[base+sub*16:base+sub*16+8]
                                idx = xx*4+yy
                                ai = (ab[idx>>1] >> ((idx&1)*4)) & 0xF
                                a = ai*0x11
                            px[Y][X] = (c[0], c[1], c[2], a)
    return px

def parse_texture(r, a):
    t = hdr(r, a)
    t['height'] = i32at(r, a+0x18)
    t['width'] = i32at(r, a+0x1c)
    t['hw_format'] = u32at(r, a+0x34)
    # imagedata inline @+0x3c
    ia = a + 0x3c
    t['img_h'] = i32at(r, ia); t['img_w'] = i32at(r, ia+4)
    c, l = listref(r, ia+8)
    t['data'] = r.d[l:l+c] if c > 0 and l < len(r.d) else b''
    return t

def tile_px(d, w, h, bpp):
    # decomponi il layout a tile 8x8 -> elenco (X,Y,offset)
    out = []
    tw8 = (w+7)//8
    for ty8 in range((h+7)//8):
        for tx8 in range(tw8):
            base = (ty8*tw8+tx8)*64*bpp
            rows = [0,2,4,6,1,3,5,7] if bpp == 4 else list(range(8))
            for ri, ry in enumerate(rows):
                for xx in range(8):
                    out.append((tx8*8+xx, ty8*8+ry, base+(ri*8+xx)*bpp))
    return out

def texture_rgba(t):
    w, h = t['img_w'], t['img_h']
    dta = t['data']
    if not dta or w <= 0 or h <= 0: return None
    hw = t['hw_format']
    if hw == 0x0C: return w, h, decode_etc1(dta, w, h)
    if hw == 0x0D: return w, h, decode_etc1(dta, w, h, a4=True)
    if hw == 0x3:      # RGB565
        px = [[(0,0,0,255)]*w for _ in range(h)]
        for X, Y, o in tile_px(dta, w, h, 2):
            if X < w and Y < h and o+2 <= len(dta):
                v = struct.unpack_from('<H', dta, o)[0]
                px[Y][X] = (((v>>11)&31)*255//31, ((v>>5)&63)*255//63, (v&31)*255//31, 255)
        return w, h, px
    if hw == 0x7:      # L8
        px = [[(0,0,0,255)]*w for _ in range(h)]
        for X, Y, o in tile_px(dta, w, h, 1):
            if X < w and Y < h and o < len(dta):
                v = dta[o]; px[Y][X] = (v, v, v, 255)
        return w, h, px
    if hw == 0x8:      # A8
        px = [[(0,0,0,0)]*w for _ in range(h)]
        for X, Y, o in tile_px(dta, w, h, 1):
            if X < w and Y < h and o < len(dta):
                px[Y][X] = (255, 255, 255, dta[o])
        return w, h, px
    if hw == 0x0:      # RGBA8
        px = [[(0,0,0,255)]*w for _ in range(h)]
        for X, Y, o in tile_px(dta, w, h, 4):
            if X < w and Y < h and o+4 <= len(dta):
                px[Y][X] = (dta[o], dta[o+1], dta[o+2], dta[o+3])
        return w, h, px
    return None

# --------------------------------------------------------------- texref scan
def build_texrefs(r):
    """tutti gli oggetti 0x20000004 'TXOB' -> {addr: path}"""
    out = {}
    i = 0
    d = r.d
    while True:
        i = d.find(b'TXOB', i+1)
        if i < 0: break
        a = i-4
        if a < 0: continue
        if u32at(r, a) != 0x20000004: continue
        p = selfrel(a+0x18, u32at(r, a+0x18))  # +0x18: path ref dopo header
        nm = cstr(r, selfrel(a+0x0c, u32at(r, a+0x0c)))
        path = cstr(r, p)
        if path: out[a] = path
    return out

def mat_textures(r, mat_addr, texrefs, limit=3):
    """MTOB: trova i ref a oggetti texref tra i campi"""
    out = []
    end = min(mat_addr + 0x600, len(r.d)-4)
    for a in range(mat_addr+0x18, end, 4):
        v = u32at(r, a)
        t = selfrel(a, v)
        if t in texrefs and t != mat_addr:
            p = texrefs[t]
            if p not in out:
                out.append(p)
                if len(out) >= limit: break
    return out

# --------------------------------------------------------------- container
ROOT = ['models','textures','luts','materials','shaders','cameras','lights',
        'fogs','scenes','skel_anims','mat_anims','vis_anims','cam_anims',
        'light_anims','fog_anims','emitters']

def parse(path):
    d = open(path, 'rb').read()
    r = R(d)
    assert d[:4] == b'CGFX'
    hlen = struct.unpack_from('<H', d, 6)[0]
    r.p = max(hlen, 0x1c)
    root = {}
    for k in ROOT:
        c = i32at(r, r.p)
        p = selfrel(r.p+4, u32at(r, r.p+4))
        r.p += 8
        root[k] = {'count': c, 'dict': read_dict(r, p) if c > 0 and p < len(d) else None}
    return r, root

# --------------------------------------------------------------- assembla modelli
def extract(path):
    r, root = parse(path)
    texrefs = build_texrefs(r)
    textures = {}
    if root['textures']['dict']:
        for nm, a in zip(*root['textures']['dict']):
            if a and u32at(r, a) == 0x20000011:
                textures[nm] = parse_texture(r, a)
    models = []
    if root['models']['dict']:
        for nm, a in zip(*root['models']['dict']):
            if a and u32at(r, a) in (0x40000092, 0x40000012):
                m = parse_model(r, a); m['name'] = nm
                models.append(m)
    out_models = []
    for m in models:
        mm = {'name': m['name'], 'world_m': m['world_m'], 'local_m': m['local_m'],
              't_trans': m['t_trans'], 'meshes': []}
        mats = []
        for i, (mn, ma) in enumerate(zip(m['materials']['names'], m['materials']['addrs'])):
            mats.append({'name': mn, 'textures': mat_textures(r, ma, texrefs)})
        mm['materials'] = mats
        for mi_addr in m['meshes']:
            if u32at(r, mi_addr) != 0x01000000: continue
            mesh = parse_mesh(r, mi_addr)
            si = mesh['shape_index']
            if si < 0 or si >= len(m['shapes']): continue
            sh = parse_shape(r, m['shapes'][si])
            g = {'shape': sh['name'], 'material': mesh['material_index'],
                 'bbox': sh['bbox'], 'pos_offset': sh['pos_offset'],
                 'pos': None, 'uv': None, 'nrm': None, 'col': None,
                 'bidx': None, 'bwgt': None, 'parts': []}
            for vb_addr in sh['vbs']:
                try:
                    vb = parse_vb(r, vb_addr)
                except ValueError:
                    continue
                for at in vb['attrs']:
                    vecs = decode_attr(at, vb['stride'], vb['raw'])
                    if at['name'] == 'Position':  g['pos'] = vecs
                    elif at['name'] == 'TexCoord0': g['uv'] = vecs
                    elif at['name'] == 'Normal':  g['nrm'] = vecs
                    elif at['name'] == 'Color':   g['col'] = vecs
                    elif at['name'] == 'BoneIndex': g['bidx'] = vecs
                    elif at['name'] == 'BoneWeight': g['bwgt'] = vecs
            for sm_addr in sh['submeshes']:
                sm = parse_submesh(r, sm_addr)
                part = {'skinning': sm['skinning'], 'bones': sm['bones'], 'prims': []}
                for f_addr in sm['faces']:
                    f = parse_face(r, f_addr)
                    for d_addr in f['descs']:
                        dd = parse_desc(r, d_addr)
                        part['prims'].append({'prim': dd['prim'], 'indices': dd['indices']})
                g['parts'].append(part)
            mm['meshes'].append(g)
        # skeleton
        if m['skeleton']:
            sk = {'bones': []}
            c, l = listref(r, m['skeleton']+0x18)
            dd = read_dict(r, l) if c > 0 and l < len(r.d) else None
            if dd:
                for bn, ba in zip(*dd):
                    if ba and u32at(r, ba) == 0x02000000: continue
                    # GfxBone: name ref, flags, index, parent, 4 ref, SRT, 3 mat, bb, meta
                    sk['bones'].append({'name': bn, 'addr': ba})
            mm['skeleton_dict'] = sk
        out_models.append(mm)
    return r, root, textures, out_models

def main():
    src = sys.argv[1]; outdir = sys.argv[2]
    want_obj = '--obj' in sys.argv
    os.makedirs(outdir, exist_ok=True)
    r, root, textures, models = extract(src)
    print(f'{src}:')
    print(f'  texture: {list(textures)}')
    tot_v = tot_i = 0
    for m in models:
        nv = sum(len(g['pos'] or []) for g in m['meshes'])
        ni = sum(len(pr['indices']) for g in m['meshes'] for p in g['parts'] for pr in p['prims'])
        tot_v += nv; tot_i += ni
        tx = set()
        for mat in m['materials']:
            tx.update(mat['textures'])
        sk = m.get('skeleton_dict', {}).get('bones', [])
        print(f"  {m['name']}: {len(m['meshes'])} mesh, {nv} vertici, {ni} indici, "
              f"tex={sorted(tx)}, bones={len(sk)}")
    print(f'  TOTALE: {tot_v} vertici, {tot_i} indici, {len(models)} modelli')
    # dump JSON (senza raw)
    rep = {'file': src, 'models': models, 'texture_names': list(textures)}
    def clean(o):
        if isinstance(o, dict): return {k: clean(v) for k, v in o.items() if k != 'raw_buffer'}
        if isinstance(o, list): return [clean(x) for x in o]
        return o
    out = os.path.join(outdir, os.path.basename(src) + '.geom.json')
    with open(out, 'w') as f: json.dump(clean(rep), f)
    print(f'  -> {out}')
    # texture -> ppm
    for nm, t in textures.items():
        rgba = texture_rgba(t)
        if not rgba: 
            print(f'  tex {nm}: formato 0x{t["hw_format"]:x} non supportato'); continue
        w, h, px = rgba
        p = os.path.join(outdir, f'tex_{nm}.ppm')
        with open(p, 'wb') as f:
            f.write(f'P6\n{w} {h}\n255\n'.encode())
            f.write(b''.join(bytes(c[:3]) for row in px for c in row))
        print(f'  tex {nm}: {w}x{h} fmt 0x{t["hw_format"]:x} -> ok')
    if want_obj:
        for m in models:
            write_obj(m, os.path.join(outdir, m['name'].replace('/', '_') + '.obj'))
        print('  OBJ scritti')

def write_obj(m, path):
    with open(path, 'w') as f:
        f.write(f'# {m["name"]}\n'); vo = 1
        for g in m['meshes']:
            pos = g['pos'] or []; uv = g['uv'] or []
            po = g['pos_offset'] or [0,0,0]
            for i, p in enumerate(pos):
                f.write(f'v {p[0]+po[0]:.4f} {p[1]+po[1]:.4f} {p[2]+po[2]:.4f}\n')
                if i < len(uv): f.write(f'vt {uv[i][0]:.4f} {uv[i][1]:.4f}\n')
            for part in g['parts']:
                for pr in part['prims']:
                    idx = pr['indices']; mode = pr['prim']
                    def emit(a, b, c):
                        f.write('f ' + ' '.join(f'{x+vo}/{x+vo}' for x in (a,b,c)) + '\n')
                    if mode == 0:
                        for k in range(0, len(idx)-2, 3): emit(idx[k], idx[k+1], idx[k+2])
                    elif mode == 1:  # strip
                        for k in range(1, len(idx)-1):
                            a, b, c = idx[k-1], idx[k], idx[k+1]
                            if k % 2 == 0: b, c = c, b
                            emit(a, b, c)
                    elif mode == 2:  # fan
                        for k in range(1, len(idx)-1): emit(idx[0], idx[k], idx[k+1])
            vo += len(pos)

if __name__ == '__main__':
    main()
