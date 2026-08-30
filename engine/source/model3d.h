/* model3d.h — strutture dati per modelli bake-ati da asset originali (CTR-GFX)
 * generati da tools/bake3d.py (niente dati inventati).
 */
#ifndef MODEL3D_H
#define MODEL3D_H

#include <nds.h>

typedef struct R3DTexture {
    const u16 *pal;      /* palette RGB15 x256 (originale, quantizzata) */
    const u8  *pixels;   /* 8bpp tiled 8x8 */
    u8 log2w, log2h;
} R3DTexture;

typedef struct R3DMesh {
    const u16 *vtx;      /* v16 x,y,z (4.12, unita' mondo x16) */
    const u16 *uv;       /* t16 12.4 relativo (0..1 = 0..4096) */
    const u16 *idx;      /* triangoli */
    int ntri;
    int tex;             /* indice texture nel modello, -1 = nessuna */
    const u16 *col;      /* vertex color RGB15 (originale), NULL = bianco */
    int nv;              /* numero vertici (per skinning) */
    const u8 *bidx;      /* coppie bone-index originali (2 infl), NULL = rigido */
    const u8 *bwgt;      /* coppie pesi 0..255 (somma 255) */
    int rigid_bone;      /* bone originale se rigido, -1 se smooth/nessuno */
} R3DMesh;

typedef struct R3DModel {
    const R3DMesh   *meshes;
    int nmeshes;
    const R3DTexture *texs;
    int ntexs;
} R3DModel;

/* istanza oggetto piazzata dall'evt originale */
typedef struct R3DInstance {
    const R3DModel *model;
    int x, y, z;         /* s11.16 world (dal placement evt) */
    u16 type;            /* tipo evt originale */
    u16 flags;
} R3DInstance;

#endif
