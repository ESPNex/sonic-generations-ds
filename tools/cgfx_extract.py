#!/usr/bin/env python3
"""cgfx_extract.py v3 — BCRES/CGFX: TUTTE le texture e le mesh.
Metodo: scan diretto dei magic TXOB/CMDL nel chunk DATA (robusto).
Export: tex_*.bmp + mdl_*.obj (+ report). Uso:
  python3 cgfx_extract.py FILE.bcres OUTDIR [--obj]
Spec: 3dbrew/wiki/CGFX
"""
import os, struct, sys, re

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

def decode(fmt, w, h, d):
    img=[[(0,0,0)]*w for _ in range(h)]
    tw8=(w+7)//8
    if fmt in (0xC,0xD):
        bsz = 16 if fmt==0xD else 8
        for ty8 in range((h+7)//8):
            for tx8 in range(tw8):
                base=(ty8*tw8+tx8)*4*bsz
                for sub in range(4):
                    blk=etc1_block(d, base+sub*bsz)
                    ox,oy=(sub&1)*4,(sub>>1)*4
                    for yy in range(4):
                        for xx in range(4):
                            X,Y=tx8*8+ox+xx,ty8*8+oy+yy
                            if X<w and Y<h:
                                if fmt==0xD:
                                    idx=xx*4+yy
                                    ai=(d[base+sub*16+8+(idx>>1)] >> ((idx&1)*4)) & 0xF
                                    img[Y][X]=blk[yy][xx]+(ai*0x11,)
                                else:
                                    img[Y][X]=blk[yy][xx]
    elif fmt==0x0:
        for ty8 in range((h+7)//8):
            for tx8 in range(tw8):
                base=(ty8*tw8+tx8)*256
                for yy in range(8):
                    for xx in range(8):
                        X,Y=tx8*8+xx,ty8*8+yy
                        if X<w and Y<h:
                            o=base+yy*8+xx
                            img[Y][X]=(d[o],d[o+64],d[o+128])
    elif fmt in (0x2,0x3,0x4):
        for ty8 in range((h+7)//8):
            for tx8 in range(tw8):
                for yy in range(8):
                    for xx in range(8):
                        X,Y=tx8*8+xx,ty8*8+yy
                        if X<w and Y<h:
                            o=(ty8*8+yy)*tw8*16+tx8*16+xx*2
                            v=struct.unpack_from('<H',d,o)[0]
                            if fmt==0x2: img[Y][X]=((v&31)*8,((v>>5)&31)*8,((v>>10)&31)*8)
                            elif fmt==0x3: img[Y][X]=((v&31)*8,((v>>6)&31)*4,((v>>11)&31)*8)
                            else: img[Y][X]=(((v>>4)&15)*17,((v>>8)&15)*17,((v>>12)&15)*17)
    elif fmt==0x7:
        for ty8 in range((h+7)//8):
            for tx8 in range(tw8):
                for yy in range(8):
                    for xx in range(8):
                        X,Y=tx8*8+xx,ty8*8+yy
                        if X<w and Y<h:
                            v=d[(ty8*8+yy)*tw8*8+tx8*8+xx]; img[Y][X]=(v,v,v)
    else:
        return None
    return img

def write_bmp(path, img):
    h=len(img); w=len(img[0]); rowsz=(w*3+3)&~3
    data=bytearray()
    for y in range(h-1,-1,-1):
        row=bytearray()
        for x in range(w):
            r,g,b=img[y][x][:3]; row+=bytes((b,g,r))
        row+=b'\0'*(rowsz-len(row)); data+=row
    hd=struct.pack('<2sIHHI',b'BM',54+len(data),0,0,54)+\
       struct.pack('<IiiHHIIiiII',40,w,h,1,24,0,len(data),2835,2835,0,0)
    open(path,'wb').write(hd+bytes(data))

FN={0:'RGBA8',2:'RGBA5551',3:'RGB565',4:'RGBA4',7:'L8',0xC:'ETC1',0xD:'ETC1A4'}

def name_at(b, addr):
    if addr+4 >= len(b): return None
    ln=struct.unpack_from('<I',b,addr)[0]
    if 0 < ln < 64:
        s=b[addr+4:addr+4+ln]
        if all(32<=c<127 for c in s): return s.decode()
    return None

def extract(path, outdir, want_obj=False):
    os.makedirs(outdir, exist_ok=True)
    b=open(path,'rb').read()
    assert b[:4]==b'CGFX', 'non CGFX'
    dsize=struct.unpack_from('<I',b,0x18)[0]
    lo, hi = 0x1C, min(0x14+dsize, len(b))
    report=[]
    seen=set()
    ntex=0
    for m in re.finditer(b'TXOB', b[lo:hi]):
        o = lo + m.start() - 4          # flags prima del magic
        if o in seen: continue
        try:
            h_=struct.unpack_from('<I',b,o+0x18)[0]
            w_=struct.unpack_from('<I',b,o+0x1C)[0]
            fmt=struct.unpack_from('<I',b,o+0x34)[0]
            dsz=struct.unpack_from('<I',b,o+0x44)[0]
            doff=struct.unpack_from('<I',b,o+0x48)[0]
            if not (0<w_<=2048 and 0<h_<=2048 and fmt in FN and 0<dsz<len(b)): continue
            nm = name_at(b, o+struct.unpack_from('<I',b,o+0xC)[0]) or ('tex%02d'%ntex)
            dd = b[o+doff:o+doff+dsz]
            if len(dd)!=dsz: continue
            img = decode(fmt,w_,h_,dd)
            if not img: continue
            seen.add(o); ntex+=1
            safe=''.join(ch if ch.isalnum() or ch in '_-' else '_' for ch in nm)
            write_bmp(os.path.join(outdir,'tex_%s.bmp'%safe), img)
            report.append('TXOB %-28s %4dx%-4d %s'%(nm,w_,h_,FN[fmt]))
        except Exception:
            continue
    nobj=0; nv_tot=0; nf_tot=0
    if want_obj:
        for m in re.finditer(b'CMDL', b[lo:hi]):
            o = lo + m.start() - 4
            try:
                nm = name_at(b, o+struct.unpack_from('<I',b,o+0xC)[0]) or ('mdl%d'%nobj)
                n_sobj=struct.unpack_from('<I',b,o+0xB8)[0]
                lst=o+struct.unpack_from('<I',b,o+0xBC)[0]
                if not (0<n_sobj<512): continue
                verts=[]; faces=[]; vtot=0
                for si in range(n_sobj):
                    so=o+lst+struct.unpack_from('<I',b,o+lst+si*4)[0]
                    if b[so+4:so+8]!=b'SOBJ': continue
                    pos=struct.unpack_from('<3f',b,so+0x20)
                    ng=struct.unpack_from('<I',b,so+0x2C)[0]
                    go=struct.unpack_from('<I',b,so+0x30)[0]
                    nv=struct.unpack_from('<I',b,so+0x38)[0]
                    vo=struct.unpack_from('<I',b,so+0x3C)[0]
                    vg=so+struct.unpack_from('<I',b,vo)[0] if nv else 0
                    if not vg or struct.unpack_from('<I',b,vg)[0]!=0x40000002: continue
                    vsz=struct.unpack_from('<I',b,vg+0x14)[0]
                    voff=vg+struct.unpack_from('<I',b,vg+0x18)[0]
                    stride=struct.unpack_from('<I',b,vg+0x24)[0]
                    ncomp=struct.unpack_from('<I',b,vg+0x28)[0]
                    co=vg+struct.unpack_from('<I',b,vg+0x2C)[0]
                    comps=[]
                    for ci in range(ncomp):
                        c=co+struct.unpack_from('<I',b,co+ci*4)[0]
                        comps.append((struct.unpack_from('<I',b,c+4)[0], b[c+0x24],
                                      struct.unpack_from('<I',b,c+0x28)[0],
                                      struct.unpack_from('<f',b,c+0x2C)[0],
                                      struct.unpack_from('<I',b,c+0x30)[0]))
                    nvtx=vsz//stride if stride else 0
                    for vi in range(nvtx):
                        base=voff+vi*stride; vv=[]
                        for ctype,dtype,nvals,mul,ins in comps:
                            if ctype==0:
                                for kk in range(3):
                                    if dtype==6: vv.append(struct.unpack_from('<f',b,base+ins+4*kk)[0])
                                    elif dtype==2: vv.append(struct.unpack_from('<h',b,base+ins+2*kk)[0]*mul)
                                    elif dtype in (0,1): vv.append(b[base+ins+kk]*mul*(1 if dtype else -1))
                            elif ctype==4:
                                for kk in range(2):
                                    if dtype==6: vv.append(struct.unpack_from('<f',b,base+ins+4*kk)[0])
                                    elif dtype==2: vv.append(struct.unpack_from('<h',b,base+ins+2*kk)[0]*mul)
                        if len(vv)>=3:
                            verts.append((vv[0]+pos[0],vv[1]+pos[1],vv[2]+pos[2],vv[3] if len(vv)>3 else 0,vv[4] if len(vv)>4 else 0))
                    for gi in range(ng):
                        fg=so+go+struct.unpack_from('<I',b,so+go+gi*4)[0]
                        n2=struct.unpack_from('<I',b,fg+0xC)[0]
                        o2=fg+struct.unpack_from('<I',b,fg+0x10)[0]
                        for kj in range(n2):
                            u2=o2+struct.unpack_from('<I',b,o2+kj*4)[0]
                            nf=struct.unpack_from('<I',b,u2)[0]
                            fl=u2+struct.unpack_from('<I',b,u2+4)[0]
                            for fi in range(nf):
                                fd=fl+struct.unpack_from('<I',b,fl+fi*4)[0]
                                flg=struct.unpack_from('<I',b,fd)[0]
                                isz=struct.unpack_from('<I',b,fd+8)[0]
                                io=fd+struct.unpack_from('<I',b,fd+0xC)[0]
                                short=(flg&2)!=0
                                cnt_i=isz//(2 if short else 1)
                                idx=[struct.unpack_from('<H' if short else '<B',b,io+ii*(2 if short else 1))[0] for ii in range(cnt_i)]
                                for t in range(0,len(idx)-2,3):
                                    faces.append((idx[t]+vtot,idx[t+1]+vtot,idx[t+2]+vtot))
                    vtot=len(verts)
                if verts:
                    safe=''.join(ch if ch.isalnum() or ch in '_-' else '_' for ch in nm)
                    with open(os.path.join(outdir,'mdl_%s.obj'%safe),'w') as f:
                        f.write('# %s\n'%os.path.basename(path))
                        for v in verts: f.write('v %f %f %f\n'%(v[0],v[1],v[2]))
                        for v in verts: f.write('vt %f %f\n'%(v[3],v[4]))
                        for t in faces: f.write('f %d/%d %d/%d %d/%d\n'%(t[0]+1,t[0]+1,t[1]+1,t[1]+1,t[2]+1,t[2]+1))
                    nobj+=1; nv_tot+=len(verts); nf_tot+=len(faces)
                    report.append('CMDL %-28s %d vertici, %d tri'%(nm,len(verts),len(faces)))
            except Exception:
                continue
    for r in report: print(r)
    print('%s: %d texture, %d modelli'%(os.path.basename(path),ntex,nobj))
    return ntex

if __name__=='__main__':
    extract(sys.argv[1], sys.argv[2], '--obj' in sys.argv)
