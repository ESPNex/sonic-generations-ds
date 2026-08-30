/* world3d.h — mondo di gioco: istanze originali evt + blocchi terreno bake-ati */
#ifndef WORLD3D_H
#define WORLD3D_H

#include "model3d.h"

#define W3D_MAX_INST 512

typedef struct W3DWorld {
    const R3DModel *terrain;            /* blocchi zona (model_z11 ecc.) */
    const char *const *terr_names;      /* manifest nomi, per il loader */
    int terr_count;
    const R3DInstance *insts;           /* placement evt originale */
    int ninsts;
    int x0, x1;                         /* bound mondo s11.16 */
} W3DWorld;

void w3d_set_world(const W3DWorld *w);
void w3d_set_ring_model(const R3DModel *m);
void w3d_draw(int camx, int camy);      /* s11.16 */
/* profilo collisione terreno: cerca blocco che copre x (s11.16) */
int  w3d_ground_y(int x);

#endif
