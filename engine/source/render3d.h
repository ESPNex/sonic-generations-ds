/* render3d.h — API motore 3D hardware DS/DSi (asset originali bake-ati) */
#ifndef RENDER3D_H
#define RENDER3D_H

#include <nds.h>
#include "model3d.h"

void r3d_init(void);
void r3d_load_model(const R3DModel *m);   /* carica texture del modello in VRAM */
void r3d_camera(int wx, int wy);          /* s11.16 */
void r3d_begin_frame(void);
void r3d_draw_mesh(const R3DModel *m, int mi, int wx, int wy, int wz);
void r3d_end_frame(void);

#endif
void r3d_draw_mesh_anim(const R3DModel *m, int mi, int wx, int wy, int wz,
                        const s16 *rots, const s16 *pivots, const u8 *remap);
