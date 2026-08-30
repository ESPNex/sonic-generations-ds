/* world3d.c — mondo: istanze evt ORIGINALI + collisioni ORIGINALI per act */
#include "world3d.h"
#include "render3d.h"
#include "stage_coll_z11.h"
#include "stage_coll_z12.h"

static const W3DWorld *g_w;
static const R3DModel *g_ring;
static int g_act;

void w3d_set_world(const W3DWorld *w) { g_w = w; }
void w3d_set_ring_model(const R3DModel *m) { g_ring = m; }
void w3d_set_act(int a) { g_act = a; }

void w3d_draw(int camx, int camy) {
    if (!g_w) return;
    r3d_camera(camx, camy);
    if (g_ring && g_w->insts) {
        r3d_load_model(g_ring);
        for (int i = 0; i < g_w->ninsts; i++) {
            const R3DInstance *in = &g_w->insts[i];
            int dx = (in->x - camx) >> 16;
            if (dx < -24 || dx > 24) continue;
            for (int m = 0; m < g_ring->nmeshes; m++)
                r3d_draw_mesh(g_ring, m, in->x, in->y, 0);
        }
    }
}

#define CH 9
static int ground_below(const CollBox *tab, int n, int x, int feet_y) {
    int best = 100 << 16;
    for (int i = 0; i < n; i++) {
        const CollBox *b = &tab[i];
        int dx = x - b->cx;
        int g;
        if (b->cos == 0 && b->sin == 0) {
            if (dx < -b->hw || dx > b->hw) continue;
            g = -(b->cy + (CH << 16));
        } else {
            long long num = dx + (((long long)b->sin * CH) >> 16);
            long long t = (num << 16) / b->cos;
            if (t < -b->hw || t > b->hw) continue;
            int y3 = b->cy
                   + (int)((t * b->sin) >> 16)
                   + (int)((((long long)CH << 16) * b->cos) >> 16);
            g = -y3;
        }
        if (g >= feet_y - 0x4000 && g < best) best = g;
    }
    return best;
}

int w3d_ground_below(int x, int feet_y) {
    if (g_act == 1)
        return ground_below(z12_coll, Z12_NCOLL, x, feet_y);
    return ground_below(z11_coll, Z11_NCOLL, x, feet_y);
}

int w3d_ground_y(int x) { return w3d_ground_below(x, -100 << 16); }
