/* render3d.c — motore di rendering 3D hardware (DS/DSi) su asset originali.
 * Pipeline: modelli bake-ati (model3d.h) + placement evt originale.
 */
#include "render3d.h"
#include "model3d.h"
#include <stdio.h>

/* cache texture per-modelo (niente churn glGen/glDelete tra player e nemici) */
#define R3D_NSLOTS 4
static struct { const R3DModel *m; int names[64]; int n; } g_slots[R3D_NSLOTS];
static int g_curslot = -1;
static u32 cur_texname(int ti) {
    if (g_curslot < 0 || ti < 0 || ti >= g_slots[g_curslot].n) return 0;
    return (u32)g_slots[g_curslot].names[ti];
}

static int tex_size_enum(int log2) {
    /* libnds GL_TEXTURE_SIZE_ENUM: 64=0,32=1,16=2,8=3,128=4,256=5,512=6,1024=7 */
    switch (log2) {
        case 3: return 3;   /* 8   */
        case 4: return 2;   /* 16  */
        case 5: return 1;   /* 32  */
        case 6: return 0;   /* 64  */
        case 7: return 4;   /* 128 */
        case 8: return 5;   /* 256 */
        case 9: return 6;   /* 512 */
        case 10: return 7;  /* 1024 */
    }
    return 5;
}

void r3d_init(void) {
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);
    /* subschermo nero */
    vramSetBankH(VRAM_H_SUB_BG);
    BG_PALETTE_SUB[0] = RGB15(0, 0, 0);
    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_TEXTURE);
    vramSetBankD(VRAM_D_TEXTURE);
    /* FIX sagome nere: senza un bank TEX_PALETTE (E/F/G) glColorTableEXT scrive nel
     * vuoto -> palette[0]=0 -> tutte le texture campionano nero. E=64KB copre i
     * palette slot 0-3 usati dai modelli. */
    vramSetBankE(VRAM_E_TEX_PALETTE);
    glInit();
    glEnable(GL_TEXTURE_2D | GL_ANTIALIAS);
    glClearColor(10, 14, 31, 31);   /* cielo: componenti 0..31 */
    glClearPolyID(63);
    glViewport(0, 0, 255, 191);
    /* fill: nessun toon/shading strano, colore dal vertex */
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
}

void r3d_load_model(const R3DModel *m) {
    if (g_curslot >= 0 && g_slots[g_curslot].m == m) return;
    int s = -1;
    for (int i = 0; i < R3D_NSLOTS; i++) if (g_slots[i].m == m) { s = i; break; }
    if (s < 0) {
        for (int i = 0; i < R3D_NSLOTS; i++) if (!g_slots[i].m) { s = i; break; }
        if (s < 0) {   /* evict: slot dopo il primo */
            s = 1;
            for (int i = 0; i < g_slots[s].n && i < 64; i++)
                if (g_slots[s].names[i]) glDeleteTextures(1, (u32*)&g_slots[s].names[i]);
            g_slots[s].m = NULL; g_slots[s].n = 0;
        }
        g_slots[s].m = m;
        g_slots[s].n = 0;
        for (int i = 0; i < m->ntexs && i < 64; i++) {
            const R3DTexture *t = &m->texs[i];
            glGenTextures(1, (u32*)&g_slots[s].names[i]);
            glBindTexture(0, g_slots[s].names[i]);
            int sz = tex_size_enum(t->log2w > t->log2h ? t->log2w : t->log2h);
            glTexImage2D(0, 0, GL_RGB256, sz, sz, 0,
                         TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T,
                         t->pixels);
            glColorTableEXT(0, 0, 256, 0, 0, t->pal);
            g_slots[s].n = i + 1;
        }
    }
    g_curslot = s;
}

/* camera 2.5D: unita' mondo -> pixel con scala 8px/unita' */
static int g_camx, g_camy;       /* s11.16 */
#define PX_PER_UNIT 8

void r3d_camera(int wx, int wy) { g_camx = wx; g_camy = wy; }

/* budget GE: 2048 poligoni/frame max */
#define POLY_BUDGET 2000
static int g_poly;

void r3d_begin_frame(void) {
    g_poly = POLY_BUDGET;
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* ortho: 32x24 unita' centrate sulla camera (256x192 px @ 8px/unita') */
    glOrthof32(-floattof32(16.0f), floattof32(16.0f),
               floattof32(-12.0f), floattof32(12.0f),
               floattof32(-100.0f), floattof32(100.0f));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void r3d_draw_mesh(const R3DModel *m, int mi, int wx, int wy, int wz) {
    if (mi < 0 || mi >= m->nmeshes) return;
    const R3DMesh *g = &m->meshes[mi];
    if (g->ntri > g_poly) return;   /* budget poligoni GE */
    g_poly -= g->ntri;
    if (g->tex >= 0 && g->tex < m->ntexs) {
        const R3DTexture *tt = &m->texs[g->tex];
        glBindTexture(tex_size_enum(tt->log2w > tt->log2h ? tt->log2w : tt->log2h),
                      cur_texname(g->tex));
        const R3DTexture *t = &m->texs[g->tex];
        int shift = t->log2w;   /* t16 = (uv12.4 * W) >> 8 = uv << (log2w - 8+12-? ) */
        /* uv memorizzati come <<12; t16 texel*16 => uv<<(log2+4); da <<12: >>(8-log2) */
        (void)shift;
    }
    /* posizione relativa alla camera calcolata su CPU (s11.16 -> f32 20.12) */
    glPushMatrix();
    glTranslate3f32((wx - g_camx) >> 4, (wy - g_camy) >> 4, wz >> 4);
    /* NOTA scala (verificata empiricamente): glVertex3v16 divide per 4096
     * (fixed 1.12), quindi v16 = world*512 -> GL = world/8; glScalef32(0x8000)
     * (8.0) -> GL = world. Ortho ±16 = 32 GL = 255px -> ~8px per unita mondo:
     * Sonic (11u) ~88px, ring (6u) ~48px. Calibrazione con 0x0080/0x0100
     * rendeva tutto invisibile (Sonic 3px): la scala 0x8000 e' quella giusta. */
    glScalef32(0x8000, 0x8000, 0x8000);
    glBegin(GL_TRIANGLES);
    const u16 *V = g->vtx, *U = g->uv, *C = g->col, *I = g->idx;
    int tw = 1 << (g->tex >= 0 && g->tex < m->ntexs ? m->texs[g->tex].log2w : 3);
    for (int t = 0; t < g->ntri; t++) {
        for (int k = 0; k < 3; k++) {
            u16 vi = I[3*t + k];
            if (C) glColor(C[vi]);
            if (U) {
                int u = U[2*vi], v = U[2*vi+1];
                /* u e' <<12 (0..4096 = 0..1); t16 = texel<<4 = u*W>>8 */
                glTexCoord2t16((u * tw) >> 8, (v * tw) >> 8);
            }
            glVertex3v16(V[3*vi], V[3*vi+1], V[3*vi+2]);
        }
    }
    glEnd();
    glPopMatrix(1);
}

void r3d_end_frame(void) {
    glFlush(0);
    /* swiWaitForVBlank gestito dal main */
}

/* ---- SKINNING (animazioni skeletal originali, rotazioni Q14 + pivot v16) ---- */

static void skin_point(const s16 *R, const s16 *piv, int x, int y, int z,
                       int *ox, int *oy, int *oz) {
    int dx = x - piv[0], dy = y - piv[1], dz = z - piv[2];
    *ox = piv[0] + (int)(((long long)dx*R[0] + (long long)dy*R[1] + (long long)dz*R[2]) >> 14);
    *oy = piv[1] + (int)(((long long)dx*R[3] + (long long)dy*R[4] + (long long)dz*R[5]) >> 14);
    *oz = piv[2] + (int)(((long long)dx*R[6] + (long long)dy*R[7] + (long long)dz*R[8]) >> 14);
}

void r3d_draw_mesh_anim(const R3DModel *m, int mi, int wx, int wy, int wz,
                        const s16 *rots, const s16 *pivots, const u8 *remap) {
    if (mi < 0 || mi >= m->nmeshes) return;
    const R3DMesh *g = &m->meshes[mi];
    if (g->ntri > g_poly) return;
    g_poly -= g->ntri;
    if (g->tex >= 0 && g->tex < m->ntexs) {
        const R3DTexture *tt = &m->texs[g->tex];
        glBindTexture(tex_size_enum(tt->log2w > tt->log2h ? tt->log2w : tt->log2h),
                      cur_texname(g->tex));
    }
    glPushMatrix();
    glTranslate3f32((wx - g_camx) >> 4, (wy - g_camy) >> 4, wz >> 4);
    glScalef32(0x8000, 0x8000, 0x8000);
    glBegin(GL_TRIANGLES);
    const u16 *V = g->vtx, *U = g->uv, *C = g->col, *I = g->idx;
    const u8 *BI = g->bidx, *BW = g->bwgt;
    int tw = 1 << (g->tex >= 0 && g->tex < m->ntexs ? m->texs[g->tex].log2w : 3);
    for (int t = 0; t < g->ntri; t++) {
        for (int k = 0; k < 3; k++) {
            u16 vi = I[3*t + k];
            int sx = V[3*vi], sy = V[3*vi+1], sz = V[3*vi+2];
            if (BI && BW) {
                long ax = 0, ay = 0, az = 0;
                for (int infl = 0; infl < 2; infl++) {
                    u8 eb = remap[BI[2*vi + infl]];
                    int w = BW[2*vi + infl];
                    if (w == 0) continue;
                    if (eb != 0xFF) {
                        int px, py, pz;
                        skin_point(&rots[eb*9], &pivots[eb*3], sx, sy, sz, &px, &py, &pz);
                        ax += (long)w * px; ay += (long)w * py; az += (long)w * pz;
                    } else {
                        ax += (long)w * sx; ay += (long)w * sy; az += (long)w * sz;
                    }
                }
                sx = (int)(ax >> 8); sy = (int)(ay >> 8); sz = (int)(az >> 8);
            } else if (g->rigid_bone >= 0 && remap[g->rigid_bone] != 0xFF) {
                int px, py, pz;
                skin_point(&rots[remap[g->rigid_bone]*9], &pivots[remap[g->rigid_bone]*3],
                           sx, sy, sz, &px, &py, &pz);
                sx = px; sy = py; sz = pz;
            }
            if (C) glColor(C[vi]);
            if (U) {
                int u = U[2*vi], v = U[2*vi+1];
                glTexCoord2t16((u * tw) >> 8, (v * tw) >> 8);
            }
            glVertex3v16((u16)sx, (u16)sy, (u16)sz);
        }
    }
    glEnd();
    glPopMatrix(1);
}
