/* gfxrt.c — RUNTIME decoder #AMB/CGFX/CANM in ARM9 (M6).
 * Port C fedele di tools/gfx_mesh.py + tools/anim_bake.py + tools/bake3d.py:
 *   - container #AMB {n@0x10, entry 16B @0x18 {off,size,-1,0}}
 *   - CGFX old-rev: header (hlen@6), 16 dict root @max(hlen,0x1c), ptr self-rel
 *   - CMDL 0x40000092/12 - MESH 0x01000000 - SHAPE 0x10000001 - VB 0x40000002
 *     ATTR 0x40000001 - TXOB-img 0x20000011 - texref 0x20000004 - skel 0x02000000
 *   - texture ETC1(0x0C)/ETC1A4(0x0D)/RGB565(0x3)/L8(0x7)/A8(0x8)/RGBA8(0x0)
 *     -> median cut 256 -> 8bpp tiled + palette RGB15 (identico a bake3d)
 *   - CANM: dur f32@0x14, dict tracce@0x28, quat xyzw stride 20B da bone+0x28
 *   - bake Q14: W(f,b)=W(f,par)*q ; D=W(f,b)*conj(W(0,b)) ; pivot giunzione
 * Arena statica in .bss. Float software: solo a caricamento.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gfxrt.h"

#define ARENA_SZ (768u << 10)
static u8  g_arena[ARENA_SZ] __attribute__((aligned(16)));
static u32 g_used = 0;

static void *ga(int n) {
    n = (n + 15) & ~15;
    if (n <= 0 || g_used + (u32)n > ARENA_SZ) return NULL;
    void *p = &g_arena[g_used]; g_used += n; return p;
}
void gfxrt_reset(void) { g_used = 0; }
u32   gfxrt_used(void) { return g_used; }

/* ---------------- reader LE con guard ---------------- */
static const u8 *D; static u32 LEN;
static u32 rd_u32(u32 a){ if(a+4>LEN) return 0; return (u32)D[a]|((u32)D[a+1]<<8)|((u32)D[a+2]<<16)|((u32)D[a+3]<<24); }
static int rd_i32(u32 a){ return (int)rd_u32(a); }
static u16 rd_u16(u32 a){ if(a+2>LEN) return 0; return (u16)(D[a]|(D[a+1]<<8)); }
static float rd_f32(u32 a){ u32 v=rd_u32(a); float f; memcpy(&f,&v,4); return f; }
static u32 selfrel(u32 a, u32 v){ return a + v; }

static int name_len(u32 a){ if(!a||a>=LEN) return 0; u32 e=a; while(e<LEN&&D[e]) e++; return (int)(e-a); }
static int name_ends(u32 a, const char *sfx){
    int l = name_len(a), sl = (int)strlen(sfx);
    if (l < sl || sl == 0) return 0;
    return memcmp(&D[a+l-sl], sfx, sl) == 0;
}
static int name_eq(u32 a, const char *s){
    int l = name_len(a); if ((int)strlen(s) != l) return 0;
    return l ? (memcmp(&D[a], s, l) == 0) : 1;
}
static void name_copy(u32 a, char *out, int max){
    int l = name_len(a); if (l > max-1) l = max-1;
    if (l > 0) memcpy(out, &D[a], l); out[l] = 0;
}
static int name_eq2(u32 a, u32 b){
    int la=name_len(a), lb=name_len(b);
    if (la!=lb||la==0) return 0;
    return memcmp(&D[a],&D[b],la)==0;
}

static int listref(u32 a, u32 *p){
    int c = rd_i32(a); u32 q = selfrel(a+4, rd_u32(a+4));
    if (c < 0 || q == 0 || q >= LEN) { *p = 0; return 0; }
    *p = q; return c;
}
static u32 obj_at(u32 lst, int i){
    if (!lst || lst + 4u*(i+1) > LEN) return 0;
    return selfrel(lst + 4*i, rd_u32(lst + 4*i));
}
static int read_dict(u32 a, u32 *names, u32 *vals, int max){
    if (a+12 > LEN || D[a]!='D'||D[a+1]!='I'||D[a+2]!='C'||D[a+3]!='T') return -1;
    int n = rd_i32(a+8); if (n < 0 || n > max) return -1;
    u32 p = a+12; int c = 0;
    for (int i = 0; i <= n; i++){
        if (p+16 > LEN) break;
        if (i > 0 && c < n){ names[c]=selfrel(p+8,rd_u32(p+8)); vals[c]=selfrel(p+12,rd_u32(p+12)); c++; }
        p += 16;
    }
    return c;
}

/* ---------------- ETC1 ---------------- */
static const int ETCT[8][4] = {{2,8,-2,-8},{5,17,-5,-17},{9,29,-9,-29},{13,42,-13,-42},
                               {18,60,-18,-60},{24,80,-24,-80},{33,106,-33,-106},{47,183,-47,-183}};
static void etc1_block(u32 o, u8 out[4][4][3]){
    /* ETC1 3DS bit-exatto (riferimento EveryFileExplorer): diff=33, flip=32,
       cw1@37..35, cw2@34..32; diff: R@63..59 G@55..51 B@47..43, delta@58..56/50..48/42..40
       (estensione segno 3-bit: v>=4 -> v-8); indiv: R1@63..60 G1@55..52 B1@47..44
       R2@59..56 G2@51..48 B2@43..40; indice pixel (y3,x3) = bit(4*x3+y3) LSB, +16 MSB. */
    u64 bits = ((u64)rd_u32(o+4) << 32) | rd_u32(o);
    int diff=(int)((bits>>33)&1), flip=(int)((bits>>32)&1);
    int cw1=(int)((bits>>37)&7), cw2=(int)((bits>>34)&7);
    int r1,g1,b1,r2,g2,b2;
    if (diff){
        int r=(int)((bits>>59)&31), g=(int)((bits>>51)&31), b=(int)((bits>>43)&31);
        r1=(r<<3)|((r&28)>>2); g1=(g<<3)|((g&28)>>2); b1=(b<<3)|((b&28)>>2);
        int dr=(int)((bits>>56)&7), dg=(int)((bits>>48)&7), db=(int)((bits>>40)&7);
        dr = dr>=4 ? dr-8 : dr; dg = dg>=4 ? dg-8 : dg; db = db>=4 ? db-8 : db;
        r+=dr; g+=dg; b+=db;
        if(r<0)r=0; if(r>31)r=31; if(g<0)g=0; if(g>31)g=31; if(b<0)b=0; if(b>31)b=31;
        r2=(r<<3)|((r&28)>>2); g2=(g<<3)|((g&28)>>2); b2=(b<<3)|((b&28)>>2);
    } else {
        r1=(int)(((bits>>60)&15)*17); g1=(int)(((bits>>52)&15)*17); b1=(int)(((bits>>44)&15)*17);
        r2=(int)(((bits>>56)&15)*17); g2=(int)(((bits>>48)&15)*17); b2=(int)(((bits>>40)&15)*17);
    }
    for (int y3=0; y3<4; y3++)
    for (int x3=0; x3<4; x3++){
        int val=(int)((bits>>(x3*4+y3))&1);
        int neg=(int)((bits>>(x3*4+y3+16))&1);
        int add, c0,c1,c2;
        if ((flip && y3<2) || (!flip && x3<2)){
            add = ETCT[cw1][(neg<<1)|val];
            c0=r1+add; c1=g1+add; c2=b1+add;
        } else {
            add = ETCT[cw2][(neg<<1)|val];
            c0=r2+add; c1=g2+add; c2=b2+add;
        }
        out[y3][x3][0]=c0<0?0:(c0>255?255:c0);
        out[y3][x3][1]=c1<0?0:(c1>255?255:c1);
        out[y3][x3][2]=c2<0?0:(c2>255?255:c2);
    }
}

/* ---------------- texture -> RGBA ---------------- */
static u8 *tex_rgba(u32 data, u32 dlen, int w, int h, u32 hw){
    if (!data || data >= LEN || w<=0 || h<=0 || w>512 || h>512) return NULL;
    u8 *px = ga(w*h*4); if (!px) return NULL;
    if (hw==0x0C || hw==0x0D){
        int a4 = hw==0x0D, bsz = a4?16:8, tsize = 4*bsz, tw8=(w+7)/8;
        for (int ty8=0; ty8<(h+7)/8; ty8++)
        for (int tx8=0; tx8<tw8; tx8++){
            u32 base = data + (u32)(ty8*tw8+tx8)*tsize;
            for (int sub=0; sub<4; sub++){
                if (base + (u32)sub*bsz + 8 > LEN) continue;
                /* ETC1A4: alpha PRIMA (8B), colore DOPO (+8) */
                u8 blk[4][4][3]; etc1_block(base+(u32)sub*bsz + (a4?8:0), blk);
                int ox=(sub&1)*4, oy=(sub>>1)*4;
                for (int yy=0; yy<4; yy++)
                for (int xx=0; xx<4; xx++){
                    int X=tx8*8+ox+xx, Y=ty8*8+oy+yy;
                    if (X>=w||Y>=h) continue;
                    u8 *p=&px[(Y*w+X)*4];
                    p[0]=blk[yy][xx][0]; p[1]=blk[yy][xx][1]; p[2]=blk[yy][xx][2]; p[3]=255;
                    if (a4){
                        u32 ab = base + (u32)sub*16;   /* alpha nel primo u64 del blocco */
                        if (ab + 8 > data + dlen || ab + 8 > LEN) continue;
                        int q = xx*4+yy;              /* nibble (col*4+row) nel flusso LE */
                        int ai = (D[ab + (q>>1)] >> ((q&1)*4)) & 0xF;
                        p[3] = (u8)(ai*17);
                    }
                }
            }
        }
        return px;
    }
    int bpp = hw==0x3?2 : (hw==0x7||hw==0x8)?1 : (hw==0x0?4:0);
    if (!bpp) return NULL;
    int tw8=(w+7)/8;
    static const int rows4[8]={0,2,4,6,1,3,5,7};
    for (int ty8=0; ty8<(h+7)/8; ty8++)
    for (int tx8=0; tx8<tw8; tx8++){
        u32 base = data + (u32)(ty8*tw8+tx8)*64*bpp;
        for (int ri=0; ri<8; ri++){
            int ry = (bpp==4)?rows4[ri]:ri;
            for (int xx=0; xx<8; xx++){
                int X=tx8*8+xx, Y=ty8*8+ry;
                u32 o = base + (u32)(ri*8+xx)*bpp;
                if (X>=w||Y>=h||o<bpp||o+bpp>LEN||o+bpp>data+dlen) continue;
                u8 *p=&px[(Y*w+X)*4];
                if (hw==0x3){ u16 v=rd_u16(o);
                    p[0]=(u8)(((v>>11)&31)*255/31); p[1]=(u8)(((v>>5)&63)*255/63); p[2]=(u8)((v&31)*255/31); p[3]=255; }
                else if (hw==0x7){ p[0]=p[1]=p[2]=D[o]; p[3]=255; }
                else if (hw==0x8){ p[0]=p[1]=p[2]=255; p[3]=D[o]; }
                else { p[0]=D[o]; p[1]=D[o+1]; p[2]=D[o+2]; p[3]=D[o+3]; }
            }
        }
    }
    return px;
}

/* ---------------- median cut 256 ---------------- */
static void mc_sort(u16 *pi, int st, int ln, int ch, const u8 *px, u16 *tmp){
    int cnt[256]; memset(cnt,0,sizeof cnt);
    for (int i=0;i<ln;i++) cnt[px[(size_t)pi[st+i]*4+ch]]++;
    int pos[256]; int o=0;
    for (int c=0;c<256;c++){ pos[c]=o; o+=cnt[c]; }
    for (int i=0;i<ln;i++){ u16 id=pi[st+i]; tmp[pos[px[(size_t)id*4+ch]]++]=id; }
    memcpy(pi+st, tmp, (size_t)ln*2);
}
static int median_cut(const u8 *px, int npix, u8 pal[256][3]){
    u16 *pi = ga(npix*2); u16 *tmp = ga(npix*2);
    if (!pi||!tmp) return 0;
    int n=0;
    for (int i=0;i<npix;i++) if (px[i*4+3]>=8) pi[n++]=(u16)i;
    if (!n) for (int i=0;i<npix;i++) pi[n++]=(u16)i;
    typedef struct { int st,ln; } Bkt;
    Bkt *bk = ga(256*sizeof(Bkt)); if (!bk) return 0;
    int nb=1; bk[0].st=0; bk[0].ln=n;
    while (nb<256){
        int best=-1, br=-1, bch=0;
        for (int i=0;i<nb;i++){
            if (bk[i].ln<2) continue;
            for (int ch=0;ch<3;ch++){
                int lo=255,hi=0;
                for (int k=0;k<bk[i].ln;k++){
                    int v=px[(size_t)pi[bk[i].st+k]*4+ch];
                    if(v<lo)lo=v; if(v>hi)hi=v;
                }
                if (hi-lo>br){ br=hi-lo; best=i; bch=ch; }
            }
        }
        if (best<0) break;
        int mid=bk[best].ln/2;
        mc_sort(pi, bk[best].st, bk[best].ln, bch, px, tmp);
        bk[nb].st=bk[best].st+mid; bk[nb].ln=bk[best].ln-mid; nb++;
        bk[best].ln=mid;
    }
    for (int i=0;i<nb;i++){
        long r=0,g=0,b=0;
        for (int k=0;k<bk[i].ln;k++){ const u8*p=&px[(size_t)pi[bk[i].st+k]*4]; r+=p[0]; g+=p[1]; b+=p[2]; }
        if (bk[i].ln){ pal[i][0]=(u8)(r/bk[i].ln); pal[i][1]=(u8)(g/bk[i].ln); pal[i][2]=(u8)(b/bk[i].ln); }
        else { pal[i][0]=pal[i][1]=pal[i][2]=0; }
    }
    return nb;
}
static int pal_index(const u8 pal[256][3], int np, const u8 *p){
    int best=0, bd=1<<30;
    for (int i=0;i<np;i++){
        int dr=pal[i][0]-p[0], dg=pal[i][1]-p[1], db=pal[i][2]-p[2];
        int d=dr*dr+dg*dg+db*db;
        if (d<bd){ bd=d; best=i; }
    }
    return best;
}
static int log2sz(int n){ int s=0; while((1<<s)<n) s++; return s; }

static int bake_texture(u32 ta, R3DTexture *out){
    u32 hw = rd_u32(ta+0x34);
    u32 ia = ta+0x3c;
    int ih = rd_i32(ia), iw = rd_i32(ia+4);
    u32 lst; int c = listref(ia+8, &lst);
    if (c<=0) return 0;
    u8 *px = tex_rgba(lst, (u32)c, iw, ih, hw);
    if (!px) return 0;
    u8 pal[256][3];
    int np = median_cut(px, iw*ih, pal);
    if (np<=0) return 0;
    u16 *pal16 = ga(256*2); if (!pal16) return 0;
    for (int i=0;i<256;i++)
        pal16[i]=(i<np)?(u16)((pal[i][0]>>3)|((pal[i][1]>>3)<<5)|((pal[i][2]>>3)<<10)):0;
    int npx = ((iw+7)/8)*8*((ih+7)/8)*8;
    u8 *tiles = ga(npx); if (!tiles) return 0;
    u32 w8=0;
    for (int ty=0; ty<ih; ty+=8)
    for (int tx=0; tx<iw; tx+=8)
    for (int yy=0; yy<8; yy++)
    for (int xx=0; xx<8; xx++){
        const u8 *p = (ty+yy<ih && tx+xx<iw) ? &px[((ty+yy)*iw+tx+xx)*4] : NULL;
        u8 rgb[3]={0,0,0};
        if (p){ rgb[0]=p[0]; rgb[1]=p[1]; rgb[2]=p[2]; }
        tiles[w8++]=(u8)pal_index(pal,np,rgb);
    }
    out->pal=pal16; out->pixels=tiles;
    out->log2w=(u8)log2sz(iw); out->log2h=(u8)log2sz(ih);
    return 1;
}

/* ---------------- mesh decode ---------------- */
typedef struct MeshDec {
    float *pos, *uv, *col, *bidx, *bwgt;
    int nv, uv_el, col_el, bidx_el, bwgt_el;
    int has_skin;
    float pos_off[3];
    int rigid_bone;
    int mat_index, tex_slot;
    u16 *tri; int ntri;
} MeshDec;

static float *decode_attr(u32 raw, u32 rawlen, int stride, u32 name,
                          u32 fmt, int el, float scale, int off, int *n_out){
    (void)name;
    if (!raw || rawlen==0 || el<=0 || el>8) return NULL;
    int sz = (fmt==0x1406)?4 : ((fmt==0x1400||fmt==0x1401)?1:2);
    int n = (stride>0) ? (int)(rawlen/(u32)stride) : (int)(rawlen/(u32)(el*sz));
    if (n<=0||n>20000) return NULL;
    float *out = ga(n*el*4); if (!out) return NULL;
    for (int i=0;i<n;i++){
        u32 base = (stride>0) ? raw+(u32)(off+i*stride) : raw+(u32)(i*el*sz);
        if (base+(u32)(el*sz) > raw+rawlen || base+(u32)(el*sz) > LEN){ n=i; break; }
        for (int k=0;k<el;k++){
            float v; u32 a = base + (u32)k*sz;
            if (fmt==0x1406){ u32 u=rd_u32(a); memcpy(&v,&u,4); }
            else if (fmt==0x1400) v=(float)(s8)D[a];
            else if (fmt==0x1401) v=(float)D[a];
            else v=(float)(s16)rd_u16(a);
            out[i*el+k]=v*scale;
        }
    }
    *n_out=n;
    return out;
}

static int fmt_is_u16(u32 da){ return rd_u32(da)==0x1403; }
static int tri_from_desc(u32 da, int nv, u16 *dst, int cap){
    if (da+12 > LEN) return 0;
    u32 fmt = rd_u32(da);
    int prim = D[da+4];
    u32 data; int c = listref(da+8, &data);
    if (c<=0) return 0;
    int ni = (fmt==0x1403) ? c/2 : c;   /* c = bytes */
    u16 *idx = ga(ni*2); if(!idx) return 0;
    for (int i=0;i<ni;i++){
        idx[i] = (fmt==0x1403) ? rd_u16(data+(u32)i*2) : (u16)D[data+(u32)i];
        if (idx[i]>=nv) idx[i]=0xFFFF;
    }
    int t=0;
    if (prim==0){ for(int k=0;k+2<ni;k+=3) if(t+3<=cap && idx[k]!=0xFFFF && idx[k+1]!=0xFFFF && idx[k+2]!=0xFFFF){dst[t++]=idx[k];dst[t++]=idx[k+1];dst[t++]=idx[k+2];} }
    else if (prim==1){ for(int k=1;k+1<ni;k++){ int a=idx[k-1],b=idx[k],cc=idx[k+1]; if(k%2==0){int tm=b;b=cc;cc=tm;} if(t+3<=cap && a!=0xFFFF && b!=0xFFFF && cc!=0xFFFF){dst[t++]=a;dst[t++]=b;dst[t++]=cc;} } }
    else if (prim==2){ for(int k=1;k+1<ni;k++){ int a=idx[0],b=idx[k],cc=idx[k+1]; if(t+3<=cap && a!=0xFFFF && b!=0xFFFF && cc!=0xFFFF){dst[t++]=a;dst[t++]=b;dst[t++]=cc;} } }
    return t;
}

/* texref: oggetti 0x20000004 'TXOB' -> path */
#define MAX_TEXREF 256
static u32 g_tr_addr[MAX_TEXREF], g_tr_path[MAX_TEXREF]; static int g_ntr;
static void build_texrefs(void){
    g_ntr=0;
    if (LEN<8) return;
    for (u32 i=4; i+4<=LEN && g_ntr<MAX_TEXREF; i++){
        if (D[i]=='T'&&D[i+1]=='X'&&D[i+2]=='O'&&D[i+3]=='B'){
            u32 a=i-4;
            if (rd_u32(a)!=0x20000004) continue;
            u32 p=selfrel(a+0x18, rd_u32(a+0x18));
            if (p && p<LEN && D[p]){ g_tr_addr[g_ntr]=a; g_tr_path[g_ntr]=p; g_ntr++; }
        }
    }
}
static int mat_texture(u32 mat, u32 *out_path, int max_out){
    int n_out=0;
    u32 end = mat+0x600; if (end>LEN-4) end=LEN-4;
    for (u32 a=mat+0x18; a<end && n_out<max_out; a+=4){
        u32 t=selfrel(a, rd_u32(a));
        if (t==mat||t>=LEN) continue;
        for (int k=0;k<g_ntr;k++)
            if (g_tr_addr[k]==t){
                u32 pth=g_tr_path[k];
                int dup=0;
                for (int q=0;q<n_out;q++) if (out_path[q]==pth) dup=1;
                if (!dup) out_path[n_out++]=pth;
            }
    }
    return n_out;
}

/* arrotondamento half-even (parita' con python round()) */
static long rnd_even(double x){ return lrint(x); }

/* ---------------- quat ---------------- */
typedef float Q4[4];
static void qmul(const Q4 a, const Q4 b, Q4 r){
    float ax=a[0],ay=a[1],az=a[2],aw=a[3], bx=b[0],by=b[1],bz=b[2],bw=b[3];
    r[0]=aw*bx+ax*bw+ay*bz-az*by;
    r[1]=aw*by-ax*bz+ay*bw+az*bx;
    r[2]=aw*bz+ax*by-ay*bx+az*bw;
    r[3]=aw*bw-ax*bx-ay*by-az*bz;
}

#define MAXN 128
static u32 dn[MAXN], dv[MAXN];    /* dict corrente (tracce/materiali) */
static u32 an[MAXN], av[MAXN];    /* dict skel_anims (MAI sovrascritto) */

/* sorgente: buffer #AMB (o CGFX puro) gia' in memoria (ROM .rodata) */
int gfx_load_rig(const u8 *src, u32 slen, const char **anim_names, int nanim, GfxRig *rig){
    memset(rig,0,sizeof(*rig));
    for(int i=0;i<GFXRT_MAX_BONES;i++) rig->remap[i]=0xFF;
    if (!src||slen<0x40) return 0;
    u32 off=0, size=slen;
    if (src[0]=='#'&&src[1]=='A'&&src[2]=='M'&&src[3]=='B'){
        D=src; LEN=slen;
        u32 n=rd_u32(0x10);
        if (n==0||n>64) return 0;
        off=rd_u32(0x18); size=rd_u32(0x18+4);
        if (off==0xFFFFFFFF||size==0||off+size>slen) return 0;
    }
    if (size<=0x40) return 0;
    D=src+off; LEN=size;
    if (!(D[0]=='C'&&D[1]=='G'&&D[2]=='F'&&D[3]=='X')) return 0;

    build_texrefs();

    /* root: 16 dict @ max(hlen,0x1c) */
    u32 hlen=rd_u16(6);
    u32 p=(hlen>0x1c)?hlen:0x1c;
    u32 model0=0, texs_a=0, skel_a=0; int texs_n=0, skel_n=0;
    for (int k=0;k<16;k++){
        u32 lst; int c=listref(p+(u32)k*8,&lst);
        int nc=(c>0&&lst)?read_dict(lst,dn,dv,MAXN):-1;
        if (nc>0){
            if (k==0){ for(int i=0;i<nc;i++) if (dv[i]){ model0=dv[i]; break; } }
            else if (k==1){ texs_a=lst; texs_n=nc; }
            else if (k==9){ skel_a=lst; skel_n=nc; }
        }
    }
    if (!model0) return 0;

    u32 ma=model0;
    u32 meshes_l=0; int nmesh=listref(ma+0xb4,&meshes_l);
    u32 mats_l=0;  int nmats =listref(ma+0xbc,&mats_l);
    u32 shapes_l=0; int nshapes=listref(ma+0xc4,&shapes_l);
    u32 skel=selfrel(ma+0xe0, rd_u32(ma+0xe0));
    if (skel>=LEN||rd_u32(skel)!=0x02000000) skel=0;
    if (nmesh<=0) return 0;
    if (nmesh>GFXRT_MAX_MESH) nmesh=GFXRT_MAX_MESH;

    MeshDec *md=ga(nmesh*sizeof(MeshDec));
    if (!md) return 0;
    int nmd=0;

    for (int mi=0; mi<nmesh; mi++){
        u32 maddr=obj_at(meshes_l,mi);
        if (!maddr||rd_u32(maddr)!=0x01000000) continue;
        int sidx=rd_i32(maddr+0x18);
        int matidx=rd_i32(maddr+0x1c);
        if (sidx<0||sidx>=nshapes) continue;
        u32 sh=obj_at(shapes_l,sidx);
        if (!sh) continue;
        MeshDec *g=&md[nmd];
        memset(g,0,sizeof(*g));
        g->mat_index=matidx; g->tex_slot=-1; g->rigid_bone=-1;
        g->pos_off[0]=rd_f32(sh+0x20); g->pos_off[1]=rd_f32(sh+0x24); g->pos_off[2]=rd_f32(sh+0x28);

        u32 vbs_l=0; int nvbs=listref(sh+0x38,&vbs_l);
        for (int vi=0; vi<nvbs; vi++){
            u32 vb=obj_at(vbs_l,vi);
            if (!vb||rd_u32(vb)!=0x40000002) continue;
            u32 rawl=0; int rawc=listref(vb+0x14,&rawl);
            if (rawc<=0) continue;
            int stride=rd_i32(vb+0x24);
            u32 atl=0; int nat=listref(vb+0x28,&atl);
            for (int ai=0; ai<nat; ai++){
                u32 aa=obj_at(atl,ai);
                if (!aa||rd_u32(aa)!=0x40000001) continue;
                u32 name=rd_u32(aa+4);
                u32 fmt=rd_u32(aa+0x24);
                int el=rd_i32(aa+0x28);
                float sc=rd_f32(aa+0x2c);
                int off=rd_i32(aa+0x30);
                int nvv=0;
                float *arr=decode_attr(rawl,(u32)rawc,stride,name,fmt,el,sc,off,&nvv);
                if (!arr) continue;
                if (name==0){ if(!g->pos){ g->pos=arr; g->nv=nvv; } }
                else if (name==4){ if(!g->uv){ g->uv=arr; g->uv_el=el; } }
                else if (name==3){ if(!g->col){ g->col=arr; g->col_el=el; } }
                else if (name==7){ if(!g->bidx){ g->bidx=arr; g->bidx_el=el; } }
                else if (name==8){ if(!g->bwgt){ g->bwgt=arr; g->bwgt_el=el; } }
            }
        }
        if (!g->pos||g->nv<=0) continue;
        g->has_skin=(g->bidx&&g->bwgt)?1:0;

        u32 subs_l=0; int nsub=listref(sh+0x2c,&subs_l);
        /* pre-pass: conteggio indici totale (niente cap inventati) */
        int tot_idx=0;
        for (int si=0; si<nsub; si++){
            u32 sm=obj_at(subs_l,si); if(!sm) continue;
            u32 faces_l=0; int nf=listref(sm+12,&faces_l);
            for (int fi=0; fi<nf; fi++){
                u32 fa=obj_at(faces_l,fi); if(!fa) continue;
                u32 dl=0; int ndd=listref(fa,&dl);
                for (int di=0; di<ndd; di++){
                    u32 da=obj_at(dl,di); if(!da||da+12>LEN) continue;
                    u32 rl=0; int rc=listref(da+8,&rl);
                    if (rc>0) tot_idx += (fmt_is_u16(da)) ? rc/2 : rc;
                }
            }
        }
        u16 *tri=ga((tot_idx+16)*2); if (!tri) continue;
        int nt=0, tri_cap=tot_idx;
        for (int si=0; si<nsub; si++){
            u32 sm=obj_at(subs_l,si);
            if (!sm) continue;
            if (!g->has_skin && g->rigid_bone<0){
                /* bones list = interi grezzi (indici osso), non puntatori */
                u32 bl=0; int nb=listref(sm,&bl);
                if (nb>0 && bl+4<=LEN) g->rigid_bone=rd_i32(bl);
            }
            u32 faces_l=0; int nf=listref(sm+12,&faces_l);
            for (int fi=0; fi<nf; fi++){
                u32 fa=obj_at(faces_l,fi);
                if (!fa) continue;
                u32 dl=0; int ndd=listref(fa,&dl);
                for (int di=0; di<ndd; di++){
                    u32 da=obj_at(dl,di);
                    if (da) nt+=tri_from_desc(da,g->nv,tri+nt,tri_cap-nt);
                }
            }
        }
        g->tri=tri; g->ntri=nt/3;
        if (g->ntri>0) nmd++;
    }
    if (!nmd) return 0;

    /* texture per materiale (come bake3d: prima non-_sp, ordine d'uso) */
    R3DTexture *texs=ga(GFXRT_MAX_TEX*sizeof(R3DTexture));
    u32 used_path[GFXRT_MAX_TEX]; int nused=0;
    if (!texs) return 0;
    u32 mnames[MAXN], mvals[MAXN];
    int nm=(nmats>0)?read_dict(mats_l,mnames,mvals,MAXN):0;
    for (int mi=0; mi<nmd; mi++){
        MeshDec *g=&md[mi];
        if (g->mat_index<0||g->mat_index>=nm) continue;
        u32 paths[3]; int np=mat_texture(mvals[g->mat_index],paths,3);
        u32 want=0;
        for (int k=0;k<np;k++) if (!name_ends(paths[k],"_sp")){ want=paths[k]; break; }
        if (!want) continue;
        int slot=-1;
        for (int k=0;k<nused;k++) if (used_path[k]==want){ slot=k; break; }
        if (slot<0 && nused<GFXRT_MAX_TEX && texs_n>0){
            int nc=read_dict(texs_a,dn,dv,MAXN);
            for (int k=0;k<nc;k++){
                if (name_eq2(dn[k],want)){
                    if (rd_u32(dv[k])==0x20000011 && bake_texture(dv[k],&texs[nused])){
                        used_path[nused]=want; slot=nused; nused++;
                    }
                    break;
                }
            }
        }
        g->tex_slot=slot;
    }

    /* skeleton */
    u32 bnames[MAXN], bvals[MAXN];
    int nbn=0;
    if (skel){
        /* ordine osse = dict @skel+0x2c (coerente con i bidx nel file e con
         * i parent idx @bone+0xc) — il dict via listref(+0x18) ha altro ordine */
        nbn=read_dict(skel+0x2c,bnames,bvals,MAXN);
        if (nbn<=0){ u32 bl=0; int c=listref(skel+0x18,&bl);
            if (c>0&&bl) nbn=read_dict(bl,bnames,bvals,MAXN); }
    }
    if (nbn<0) nbn=0;
    if (nbn>GFXRT_MAX_BONES) nbn=GFXRT_MAX_BONES;
    int parent[GFXRT_MAX_BONES];
    for (int i=0;i<nbn;i++){
        u32 pv=rd_u32(bvals[i]+0xc);
        parent[i]=(pv==0xFFFFFFFF||pv>=(u32)nbn)?i:(int)pv;
    }
    rig->nbones_total=nbn;

    /* boneset dalle influenze reali */
    char in_bs[GFXRT_MAX_BONES]; memset(in_bs,0,sizeof in_bs);
    for (int mi=0; mi<nmd; mi++){
        MeshDec *g=&md[mi];
        if (g->has_skin){
            for (int vi=0; vi<g->nv; vi++)
            for (int k=0; k<g->bidx_el; k++){
                int b=(int)g->bidx[vi*g->bidx_el+k];
                float w=(k<g->bwgt_el)?g->bwgt[vi*g->bwgt_el+k]:0.0f;
                if (w>0.0f&&b>=0&&b<nbn) in_bs[b]=1;
            }
        } else if (g->rigid_bone>=0&&g->rigid_bone<nbn) in_bs[g->rigid_bone]=1;
    }
    int boneset[GFXRT_MAX_BONES], nb=0;
    for (int b=0;b<nbn;b++) if (in_bs[b]) boneset[nb++]=b;
    rig->nbones=nb;
    for (int e=0;e<nb;e++) rig->remap[boneset[e]]=(u8)e;

    /* vertici per osso -> pivot giunzione */
    int tot_inf=0;
    for (int mi=0; mi<nmd; mi++){
        MeshDec *g=&md[mi];
        if (g->has_skin){
            for (int vi=0; vi<g->nv; vi++)
            for (int k=0;k<g->bidx_el;k++){
                float w=(k<g->bwgt_el)?g->bwgt[vi*g->bwgt_el+k]:0.0f;
                if (w>0.15f) tot_inf++;
            }
        } else if (g->rigid_bone>=0) tot_inf+=g->nv;
    }
    float *bverts=(tot_inf>0)?ga(tot_inf*3*4):NULL;
    int *bhead=ga(nbn*4), *bnext=(tot_inf>0)?ga(tot_inf*4):NULL, *bcnt=ga(nbn*4);
    if (bhead&&bcnt){
        for(int i=0;i<nbn;i++){ bhead[i]=-1; bcnt[i]=0; }
        if (bverts&&bnext){
            int cur=0;
            for (int mi=0; mi<nmd; mi++){
                MeshDec *g=&md[mi];
                if (g->has_skin){
                    for (int vi=0; vi<g->nv; vi++)
                    for (int k=0;k<g->bidx_el;k++){
                        int b=(int)g->bidx[vi*g->bidx_el+k];
                        float w=(k<g->bwgt_el)?g->bwgt[vi*g->bwgt_el+k]:0.0f;
                        if (w<=0.15f||b<0||b>=nbn) continue;
                        float *v=&bverts[cur*3];
                        v[0]=g->pos[vi*3]+g->pos_off[0];
                        v[1]=g->pos[vi*3+1]+g->pos_off[1];
                        v[2]=g->pos[vi*3+2]+g->pos_off[2];
                        bnext[cur]=bhead[b]; bhead[b]=cur; bcnt[b]++; cur++;
                    }
                } else if (g->rigid_bone>=0&&g->rigid_bone<nbn){
                    int b=g->rigid_bone;
                    for (int vi=0; vi<g->nv; vi++){
                        float *v=&bverts[cur*3];
                        v[0]=g->pos[vi*3]+g->pos_off[0];
                        v[1]=g->pos[vi*3+1]+g->pos_off[1];
                        v[2]=g->pos[vi*3+2]+g->pos_off[2];
                        bnext[cur]=bhead[b]; bhead[b]=cur; bcnt[b]++; cur++;
                    }
                }
            }
        }
    }
    float piv3[GFXRT_MAX_BONES][3];
    char have_piv[GFXRT_MAX_BONES]; memset(have_piv,0,sizeof have_piv);
    if (bverts&&bnext&&bcnt){
        for (int e=0;e<nb;e++){
            int b=boneset[e];
            if (!bcnt[b]) continue;
            int par=parent[b];
            if (par==b||!bcnt[par]){
                float s[3]={0,0,0};
                for (int i=bhead[b];i>=0;i=bnext[i]){ s[0]+=bverts[i*3]; s[1]+=bverts[i*3+1]; s[2]+=bverts[i*3+2]; }
                piv3[b][0]=s[0]/bcnt[b]; piv3[b][1]=s[1]/bcnt[b]; piv3[b][2]=s[2]/bcnt[b];
                have_piv[b]=1;
            } else {
                int bi[6]; float bd[6];
                for(int q=0;q<6;q++){ bi[q]=-1; bd[q]=1e30f; }
                for (int i=bhead[par];i>=0;i=bnext[i])
                for (int j=bhead[b];j>=0;j=bnext[j]){
                    float dx=bverts[i*3]-bverts[j*3], dy=bverts[i*3+1]-bverts[j*3+1], dz=bverts[i*3+2]-bverts[j*3+2];
                    float d=dx*dx+dy*dy+dz*dz;
                    for(int q=0;q<6;q++) if (d<bd[q]){
                        for(int r=5;r>q;r--){ bd[r]=bd[r-1]; bi[r]=bi[r-1]; }
                        bd[q]=d; bi[q]=(i<<16)|(j&0xFFFF); break;
                    }
                }
                float s[3]={0,0,0}; int cnt=0;
                for(int q=0;q<6;q++){
                    if (bi[q]<0) continue;
                    int i=bi[q]>>16, j=bi[q]&0xFFFF;
                    s[0]+=(bverts[i*3]+bverts[j*3])*0.5f;
                    s[1]+=(bverts[i*3+1]+bverts[j*3+1])*0.5f;
                    s[2]+=(bverts[i*3+2]+bverts[j*3+2])*0.5f;
                    cnt++;
                }
                if (cnt){ piv3[b][0]=s[0]/cnt; piv3[b][1]=s[1]/cnt; piv3[b][2]=s[2]/cnt; have_piv[b]=1; }
            }
        }
        for (int pass=0;pass<nbn;pass++)
        for (int e=0;e<nb;e++){
            int b=boneset[e];
            if (have_piv[b]) continue;
            int par=parent[b];
            if (par!=b&&have_piv[par]){ piv3[b][0]=piv3[par][0]; piv3[b][1]=piv3[par][1]; piv3[b][2]=piv3[par][2]; have_piv[b]=1; }
        }
    }
#ifdef GFXRT_DEBUG
    for (int b=0;b<nbn;b++)
        fprintf(stderr,"bone %d parent %d cnt %d pivpass %d\n",b,parent[b],bcnt?bcnt[b]:0,have_piv[b]);
    for (int mi=0; mi<nmd; mi++)
        fprintf(stderr,"mesh %d bidx_el %d bwgt_el %d col_el %d uv_el %d\n",mi,md[mi].bidx_el,md[mi].bwgt_el,md[mi].col_el,md[mi].uv_el);
#endif
    for (int e=0;e<nb;e++){
        int b=boneset[e];
        rig->pivots[e*3+0]=have_piv[b]?(s16)rnd_even((double)piv3[b][0]*512.0):0;
        rig->pivots[e*3+1]=have_piv[b]?(s16)rnd_even((double)piv3[b][1]*512.0):0;
        rig->pivots[e*3+2]=have_piv[b]?(s16)rnd_even((double)piv3[b][2]*512.0):0;
    }

    /* animazioni CANM */
    if (skel_n>0&&nanim>0){
        int nc=read_dict(skel_a,an,av,MAXN);
#ifdef GFXRT_DEBUG
        fprintf(stderr,"skel dict nc=%d skel_a=%x skel_n=%d\n",nc,skel_a,skel_n);
        for (int k=0;k<nc&&k<5;k++){ char nm[40]; name_copy(an[k],nm,40); fprintf(stderr,"  anim[%d]=%s @%x\n",k,nm,av[k]); }
#endif
        for (int ai=0; ai<nanim&&ai<GFXRT_MAX_ANIM; ai++){
            u32 ca=0;
            char want[48]; snprintf(want,sizeof(want),"_%s",anim_names[ai]);
            for (int k=0;k<nc;k++) if (name_ends(an[k],want)||name_eq(an[k],anim_names[ai])){ ca=av[k]; break; }
            if (!ca||ca+0x2c>LEN) continue;
            int nfr=(int)rd_f32(ca+0x14);
            if (nfr<=0||nfr>2048) continue;
            int nt2=read_dict(ca+0x28,dn,dv,MAXN);
            if (nt2<=0) continue;
            int tb[MAXN]; const float *keys[MAXN]; int nk[MAXN];
            for (int k=0;k<nt2&&k<MAXN;k++){
                tb[k]=-1; keys[k]=NULL; nk[k]=0;
                for (int b=0;b<nbn;b++) if (name_eq2(bnames[b],dn[k])){ tb[k]=b; break; }
                if (tb[k]<0) continue;
                u32 ba=dv[k];
                if (ba+0x28>LEN) continue;
                int cnt=0; u32 q=ba+0x28;
                while (q+20<=LEN){
                    float v0=rd_f32(q+4),v1=rd_f32(q+8),v2=rd_f32(q+12),v3=rd_f32(q+16);
                    if (!(v0>-2&&v0<2&&v1>-2&&v1<2&&v2>-2&&v2<2&&v3>-2&&v3<2)) break;
                    cnt++; q+=20;
                }
                nk[k]=cnt;
                if (cnt>0){
                    float *kk=ga(cnt*4*4);
                    if (!kk){ nk[k]=0; continue; }
                    for (int i2=0;i2<cnt;i2++){
                        u32 o=ba+0x28+(u32)i2*20;
                        kk[i2*4]=rd_f32(o+4); kk[i2*4+1]=rd_f32(o+8);
                        kk[i2*4+2]=rd_f32(o+12); kk[i2*4+3]=rd_f32(o+16);
                    }
                    keys[k]=kk;
                }
            }
            int tr_of_b[GFXRT_MAX_BONES];
            for (int b=0;b<nbn;b++) tr_of_b[b]=-1;
            for (int k=0;k<nt2;k++) if (tb[k]>=0&&tb[k]<nbn) tr_of_b[tb[k]]=k;

            s16 *rots=ga(nfr*nb*9*2);
            Q4 *W=ga(nbn*sizeof(Q4)), *W0=ga(nbn*sizeof(Q4));
            if (!rots||!W||!W0) continue;
            for (int f=-1; f<nfr; f++){
                int frame=(f<0)?0:f;
                for (int b=0;b<nbn;b++){
                    Q4 lq={0,0,0,1};
                    int tk=tr_of_b[b];
                    if (tk>=0&&keys[tk]&&frame<nk[tk]){
                        /* float CANM (k0,k1,k2,k3) -> quat (x,y,z,w)=(k3,k0,k1,k2)
                         * (interpretazione verificata in anim_bake: ordine 'wxyz') */
                        lq[0]=keys[tk][frame*4+3]; lq[1]=keys[tk][frame*4];
                        lq[2]=keys[tk][frame*4+1]; lq[3]=keys[tk][frame*4+2];
                        float n2=lq[0]*lq[0]+lq[1]*lq[1]+lq[2]*lq[2]+lq[3]*lq[3];
                        float inv=1.0f/sqrtf(n2>0?n2:1.0f);
                        lq[0]*=inv; lq[1]*=inv; lq[2]*=inv; lq[3]*=inv;
                    }
                    int par=parent[b];
                    if (par==b) memcpy(W[b],lq,sizeof(Q4));
                    else { Q4 tmp; qmul(W[par],lq,tmp); memcpy(W[b],tmp,sizeof(Q4)); }
                }
                if (f<0){ memcpy(W0,W,(size_t)nbn*sizeof(Q4)); continue; }
                for (int e=0;e<nb;e++){
                    int b=boneset[e];
                    Q4 cj,Dq;
                    cj[0]=-W0[b][0]; cj[1]=-W0[b][1]; cj[2]=-W0[b][2]; cj[3]=W0[b][3];
                    qmul(W[b],cj,Dq);
                    float x=Dq[0],y=Dq[1],z=Dq[2],w=Dq[3];
                    float R[9]={1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),
                                2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),
                                2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)};
                    s16 *dst=&rots[(f*nb+e)*9];
                    for (int r2=0;r2<9;r2++){
                        int v=(int)rnd_even((double)R[r2]*16384.0);
                        if (v>32767)v=32767; if(v<-32768)v=-32768;
                        dst[r2]=(s16)v;
                    }
                }
            }
            GfxRigAnim *an=&rig->anims[rig->nanim++];
            snprintf(an->name,sizeof(an->name),"%s",anim_names[ai]);
            an->nframes=nfr; an->rots=rots;
        }
    }

    /* finalizza R3DMesh */
    R3DMesh *rm=ga(nmd*sizeof(R3DMesh));
    if (!rm) return 0;
    for (int mi=0; mi<nmd; mi++){
        MeshDec *g=&md[mi];
        u16 *vtx=ga(g->nv*3*2), *uv=ga(g->nv*2*2), *col=ga(g->nv*2*2);
        u8 *bidx=NULL,*bwgt=NULL;
        if (!vtx||!uv||!col) return 0;
        if (g->has_skin){ bidx=ga(g->nv*2); bwgt=ga(g->nv*2); if(!bidx||!bwgt) return 0; }
        for (int vi=0; vi<g->nv; vi++){
            for (int c2=0;c2<3;c2++){
                double v=((double)g->pos[vi*3+c2]+(double)g->pos_off[c2])*512.0;
                long iv=rnd_even(v); if(iv>32767)iv=32767; if(iv<-32768)iv=-32768;
                vtx[vi*3+c2]=(s16)iv;
            }
            if (g->uv){
                for (int c2=0;c2<2;c2++){
                    double v=((double)(g->uv_el>=2?g->uv[vi*g->uv_el+c2]:0.0f))*4096.0;
                    long iv=rnd_even(v); if(iv>32767)iv=32767; if(iv<-32768)iv=-32768;
                    uv[vi*2+c2]=(s16)iv;
                }
            } else { uv[vi*2]=0; uv[vi*2+1]=0; }
            if (g->col&&g->col_el>=3){
                int r=(int)rnd_even((double)g->col[vi*g->col_el]*31.0),
                    gg=(int)rnd_even((double)g->col[vi*g->col_el+1]*31.0),
                    b2=(int)rnd_even((double)g->col[vi*g->col_el+2]*31.0);
                if(r<0)r=0;if(r>31)r=31; if(gg<0)gg=0;if(gg>31)gg=31; if(b2<0)b2=0;if(b2>31)b2=31;
                col[vi]=(u16)(r|(gg<<5)|(b2<<10));
            } else col[vi]=0x7FFF;
            if (g->has_skin){
                double w0f=(g->bwgt_el>0)?(double)g->bwgt[vi*g->bwgt_el]:1.0;
                int w0=(int)rnd_even(w0f*255.0); if(w0<0)w0=0; if(w0>255)w0=255;
                int b0=(g->bidx_el>0)?(int)g->bidx[vi*g->bidx_el]:0;
                int b1=(g->bidx_el>1)?(int)g->bidx[vi*g->bidx_el+1]:0;
                if(b0<0)b0=0; if(b0>255)b0=255; if(b1<0)b1=0; if(b1>255)b1=255;
                bidx[2*vi]=(u8)b0; bwgt[2*vi]=(u8)w0;
                bidx[2*vi+1]=(u8)b1; bwgt[2*vi+1]=(u8)(255-w0);
            }
        }
        rm[mi].vtx=vtx; rm[mi].uv=uv; rm[mi].col=col;
        rm[mi].nv=g->nv; rm[mi].ntri=g->ntri;
        rm[mi].idx=g->tri; rm[mi].tex=g->tex_slot;
        rm[mi].bidx=bidx; rm[mi].bwgt=bwgt;
        rm[mi].rigid_bone=g->has_skin?-1:g->rigid_bone;
    }
    rig->model.meshes=rm; rig->model.nmeshes=nmd;
    rig->model.texs=texs; rig->model.ntexs=nused;
    return 1;
}

/* scala post-bake un rig (mesh vtx + pivots) di un fattore Q14 f (0x4000=1.0).
 * usato per correggere unita' dei modelli nemici (es. E_BAT 2.3x player). */
void gfxrt_scale_rig(GfxRig *rig, int f){
    if (!rig || f == 0x4000 || f <= 0) return;
    for (int mi=0; mi<rig->model.nmeshes; mi++){
        const R3DMesh *g=&rig->model.meshes[mi];
        u16 *v=(u16*)g->vtx;
        for (int i=0; i<g->nv*3; i++){
            int x=(int)(s16)v[i]*f>>14;
            if (x>32767)x=32767; if (x<-32768)x=-32768;
            v[i]=(u16)(s16)x;
        }
    }
    for (int e=0;e<rig->nbones;e++)
        for (int c=0;c<3;c++){
            int x=(int)rig->pivots[e*3+c]*f>>14;
            if (x>32767)x=32767; if (x<-32768)x=-32768;
            rig->pivots[e*3+c]=(s16)x;
        }
}

const GfxRigAnim *gfx_anim(const GfxRig *rig, const char *name){
    for (int i=0;i<rig->nanim;i++) if (strcmp(rig->anims[i].name,name)==0) return &rig->anims[i];
    return NULL;
}
