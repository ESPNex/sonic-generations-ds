/* gfxrt.h — RUNTIME asset loader (M6): decodifica DIRETTAMENTE in ARM9 i
 * file originali del gioco (#AMB container -> CGFX -> modelli/texture/CANM),
 * producendo le stesse strutture del motore (R3DModel + rig animato).
 * Nessun dato inventato: tutto viene dagli asset originali.
 */
#ifndef GFXRT_H
#define GFXRT_H

#include <nds.h>
#include "model3d.h"

#define GFXRT_MAX_ANIM   16
#define GFXRT_MAX_BONES  64
#define GFXRT_MAX_MESH   40
#define GFXRT_MAX_TEX    16

typedef struct GfxRigAnim {
    char       name[40];
    int        nframes;
    const s16 *rots;      /* [nframes][nbones*9] Q14 */
} GfxRigAnim;

typedef struct GfxRig {
    R3DModel model;                   /* renderizzabile con render3d */
    u8       remap[GFXRT_MAX_BONES];  /* bone originale -> boneset (0xFF = id) */
    int      nbones;                  /* boneset */
    int      nbones_total;
    s16      pivots[GFXRT_MAX_BONES * 3];  /* x512 */
    s16      mesh_scale;                   /* Q14, 0x4000 = 1.0 (scala per-modello) */
    int      nanim;
    GfxRigAnim anims[GFXRT_MAX_ANIM];
} GfxRig;

/* carica un .amb (entry 0 = CGFX) o un .bcres diretto dal filesystem
 * (nitro:/...). anim_names = suffissi delle anim richieste (es. "runR00").
 * ritorna 0 su errore. */
int  gfx_load_rig(const u8 *src, u32 slen, const char **anim_names, int nanim, GfxRig *rig);
void gfxrt_scale_rig(GfxRig *rig, int f);
const GfxRigAnim *gfx_anim(const GfxRig *rig, const char *name);
void gfxrt_reset(void);   /* libera l'arena (cambio act) */
u32   gfxrt_used(void);

#endif
