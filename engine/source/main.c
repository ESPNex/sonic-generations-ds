/* main.c — Sonic Generations DS: 2 ACT (z11 classic / z12 modern),
 * animazioni skeletal originali, collisioni originali. */
#include <nds.h>
#include <stdio.h>
#include <math.h>
#include "render3d.h"
#include "model3d.h"
#include "world3d.h"
#include "stage_z11.h"
#include "stage_z12.h"
#include "model_sonic.h"
#include "model_sonm.h"
#include "anim_sonic.h"
#include "anim_sonm.h"
#include "model_ring.h"
#include "gfxrt.h"
#include <string.h>
#include "romcard.h"
#include "enemy_z11.h"
#include "enemy_z12.h"

static int px, py, vx;
static int act;          /* 0 = z11 classic, 1 = z12 modern */

static W3DWorld w_z11 = { NULL, NULL, 0, z11_objs, Z11_NOBJ, 0, 3353 << 16 };
static W3DWorld w_z12 = { NULL, NULL, 0, z12_objs, Z12_NOBJ, 0, 3353 << 16 };

typedef struct PlayerDef {
    const R3DModel *model;
    const u8 *remap;
    const s16 *pivots;
    int nbones;
    const s16 *idle;  int idle_n;
    const s16 *run;   int run_n;
    const s16 *jump;  int jump_n;
    const s16 *damage; int damage_n;
} PlayerDef;

static const PlayerDef players[2] = {
    { &sonic, sonic_bone_remap, sonic_pivots, SONIC_NBONES,
      sonic_anim_idleR00[0], 120, sonic_anim_runR00[0], 80,
      sonic_anim_jumpR00[0], 8, sonic_anim_damageR00[0], 50 },
    { &sonm, sonm_bone_remap, sonm_pivots, SONM_NBONES,
      sonm_anim_idle00[0], 160, sonm_anim_walk00[0], 80,
      sonm_anim_jump00[0], 7, sonm_anim_damageF00[0], 10 },
};

/* ---- M6: runtime asset loader (decoder AMB/CGFX/CANM in ARM9) ----
 * M6.2: i file arrivano DIRETTAMENTE dalla ROM via bus card (NitroFS) */
static GfxRig rig;
static PlayerDef rt_pd;
static const PlayerDef *P;
static int rom_ok;
static u8 rombuf[2u << 20];     /* .amb entry0 max ~2.3MB */

static const char *cls_anims[] = { "idleR00", "runR00", "jumpR00", "damageR00" };
static const char *mdn_anims[] = { "idle00", "walk00", "jump00", "damageF00" };
static const char *bat_anims[] = { "attack00", "attack01", "attack02" };

/* ---- M7: entita' nemico (E_BAT) con anim ORIGINALI dal ZONE1_1_ENE.amb ----
 * Spawn E_BAT: posizioni ORIGINALI dallo stream C evt (type 14) - vedi
 * tools/gen_enemies.py e docs/EVT_RE_DEEP.md "MODELLO OBJ v2". */
typedef struct Enemy {
    const GfxRig *rig;
    int x, y, vx, vy, t, anim, anim_t, hp;
    int home_x, home_y;
} Enemy;
#define NEN 18
static Enemy en[NEN];
static int nen;          /* nemici attivi per l'act corrente */
static GfxRig bat_rig;
static int bat_ok;
static void en_spawn(Enemy *e, const GfxRig *r, int x, int y);

/* spawn ORIGINALI dallo stream C evt (type 14 = E_BAT; docs/EVT_RE_DEEP.md):
 * z11: 3 bat (evt X 230/1292/2187), z12: 18 — mappati sul corso del motore */
static void spawn_enemies(void) {
    if (!bat_ok) return;
    const s32 (*tab)[3] = act ? z12_enemies : z11_enemies;
    int n = act ? Z12_NENE : Z11_NENE;
    if (n > NEN) n = NEN;
    memset(en, 0, sizeof en);      /* azzera slot stale dell'altro act */
    nen = n;
    for (int i = 0; i < n; i++) {
        int gx = tab[i][0];
        int gy = w3d_ground_below(gx, py);              /* terreno sotto lo spawn */
        en_spawn(&en[i], &bat_rig, gx, gy - tab[i][1]); /* hover sopra */
    }
}

static void en_spawn(Enemy *e, const GfxRig *r, int x, int y) {
    e->rig = r; e->x = e->home_x = x; e->y = e->home_y = y;
    e->vx = e->vy = 0; e->t = 0; e->anim = 0; e->anim_t = 0; e->hp = 1;
}

static void load_act(int a) {
    act = a;
    w3d_set_act(a);
    w3d_set_world(a ? &w_z12 : &w_z11);
    px = 0; py = 10 << 16; vx = 0;

    /* arena UNICA: reset una sola volta, poi player e nemici convivono */
    gfxrt_reset();
    u32 len = rom_ok ? romcard_read_file(a ? "amb/PLAYER_MDN.amb" : "amb/PLAYER_CLS.amb",
                                         rombuf, sizeof rombuf) : 0;
    int rt_ok = (len && gfx_load_rig(rombuf, len, a ? mdn_anims : cls_anims, 4, &rig));
    if (a == 0 && !bat_ok) {
        /* E_BAT dal ZONE1_1_ENE.amb originale (card bus), stessa arena,
         * letto DOPO aver decodificato il player (rombuf condiviso) */
        u32 bl = romcard_read_file("amb/ZONE1_1_ENE.amb", rombuf, sizeof rombuf);
        if (bl && gfx_load_rig(rombuf, bl, bat_anims, 3, &bat_rig)) {
            gfxrt_scale_rig(&bat_rig, 0x2000);   /* E_BAT 2.3x player: riporta a ~1.1x (fix blackout) */
            bat_ok = 1;
        }
    }
    if (rt_ok) {
        rt_pd.model  = &rig.model;
        rt_pd.remap  = rig.remap;
        rt_pd.pivots = rig.pivots;
        rt_pd.nbones = rig.nbones;
        rt_pd.idle   = rig.anims[0].rots;  rt_pd.idle_n  = rig.anims[0].nframes;
        rt_pd.run    = rig.anims[1].rots;  rt_pd.run_n   = rig.anims[1].nframes;
        rt_pd.jump   = rig.anims[2].rots;  rt_pd.jump_n  = rig.anims[2].nframes;
        rt_pd.damage = rig.anims[3].rots;  rt_pd.damage_n= rig.anims[3].nframes;
        P = &rt_pd;
    } else {
        P = &players[a];   /* fallback: header bake-ati */
    }
}

int main(void) {
    REG_SCFG_CLK |= 1;              /* ARM9 133 MHz DSi */
    defaultExceptionHandler();
    rom_ok = (romcard_init() == 0);
    r3d_init();
    /* subscreen = LED stato asset (backdrop, zero VRAM GL):
       verde = file DLDI (SD), blu = card bus (emu), rosso = fallback baked */
    videoSetModeSub(MODE_0_2D);
    BG_PALETTE_SUB[0] =
        rom_ok ? (romcard_mode() == 2 ? RGB15(2,31,2)              /* verde: file DLDI (SD) */
                     : romcard_fat_ok() ? RGB15(2,31,31)           /* ciano: card, ma FAT montato */
                                        : RGB15(2,2,31))           /* blu: card, niente FAT */
               : romcard_fat_ok() ? RGB15(31,31,2)                 /* giallo: solo FAT, ROM non trovata */
                                  : RGB15(31,2,2);                 /* rosso: nessun backend */
    r3d_load_model(&ring);
    w3d_set_ring_model(&ring);
    load_act(0);

    spawn_enemies();
    int invuln = 0;
    int vbl = 0;
    while (1) {
        scanKeys();
        int keys = keysHeld();
        if (keysDown() & KEY_SELECT) {
            load_act(act ^ 1);
            spawn_enemies();
        }


        if (keys & KEY_RIGHT) vx += 0x480;
        if (keys & KEY_LEFT)  vx -= 0x480;
        vx -= (vx >> 6);
        if (vx > 0x2C000) vx = 0x2C000;
        if (vx < -0x2C000) vx = -0x2C000;
        px += vx;
        if (px < 0) { px = 0; vx = 0; }

        /* fisica */
        {
            int g = w3d_ground_below(px, py);
            static int vy, on_ground;
            if ((keys & KEY_A) && on_ground) { vy = -(5 << 16) / 4; on_ground = 0; }
            vy += 0xC000 / 8;
            if (vy > (5 << 16)) vy = (5 << 16);
            py += vy >> 4;
            if (py >= g) { py = g; vy = 0; on_ground = 1; }
            else if (py < g - (12 << 16)) on_ground = 0;

            /* nemici: hover sinus attorno all'home, anim attack ciclica */
            for (int e2 = 0; e2 < nen; e2++) {
                if (!en[e2].rig) continue;
                en[e2].t++;
                en[e2].x = en[e2].home_x + (int)(0.6 * sinf(en[e2].t * 0.03f + e2) * (1 << 18));
                en[e2].y = en[e2].home_y + (int)(0.3 * cosf(en[e2].t * 0.045f + e2 * 2) * (1 << 16));
                /* contatto: danno al player */
                if (invuln <= 0) {
                    int dx = px - en[e2].x, dy = py - en[e2].y;
                    if (dx > -(5 << 16) && dx < (5 << 16) &&
                        dy > -(6 << 16) && dy < (6 << 16)) {
                        /* colpito: knockback + invuln (anelli/danno da G_PARAM) */
                        static int vy2;
                        vy2 = -(5 << 16) / 3;
                        invuln = 90;
                        (void)vy2;
                    }
                }
            }
            if (invuln > 0) invuln--;

            int camx = px + (vx << 2);
            int camy = py + (4 << 16);
            r3d_camera(camx, camy);
            r3d_begin_frame();
#if 1
            w3d_draw(camx, camy);
#endif
            r3d_load_model(P->model);

            /* disegno nemici (anim attack originali) — BISECT: draw OFF */
#if 0
            for (int e2 = 0; e2 < nen; e2++) {
                if (!en[e2].rig) continue;
                const GfxRigAnim *ba = &en[e2].rig->anims[en[e2].anim];
                int f = (en[e2].anim_t >> 2) % ba->nframes;
                const s16 *rots = ba->rots + f * en[e2].rig->nbones * 9;
                r3d_load_model(&en[e2].rig->model);
                for (int i2 = 0; i2 < en[e2].rig->model.nmeshes; i2++)
                    r3d_draw_mesh_anim(&en[e2].rig->model, i2,
                                       en[e2].x, en[e2].y, 0,
                                       rots, en[e2].rig->pivots, en[e2].rig->remap);
                en[e2].anim_t++;
            }
#endif

            /* stato: jump / run / idle */
            const s16 *fr; int nfr;
            if (!on_ground)              { fr = P->jump;  nfr = P->jump_n; }
            else if (vx > 0x9000 || vx < -0x9000) { fr = P->run; nfr = P->run_n; }
            else                         { fr = P->idle; nfr = P->idle_n; }
            vbl++;
            int f = (vbl >> 1) % nfr;
            const s16 *rots = fr + f * P->nbones * 9;
#if 1   /* BISECT4: player draw ON */
#if 0   /* TEST-BISECT skinning: mesh RIGIDA senza skinning NON e' informativa
         * (le mesh sono in bone-space, tutte centrate: esce un ammasso).
         * Test valido = skinning con rotazioni IDENTITA' (posa di riposo). */
            for (int i = 0; i < P->model->nmeshes; i++)
                r3d_draw_mesh(P->model, i, px, py, 0);
#else
            static s16 idrots[64 * 9];   /* TEST posa di riposo (rotazioni unitarie) */
            static int id_ok;
            if (!id_ok) {
                for (int b = 0; b < 64; b++) {
                    idrots[b*9 + 0] = 0x4000; idrots[b*9 + 4] = 0x4000; idrots[b*9 + 8] = 0x4000;
                }
                id_ok = 1;
            }
            const s16 *use_rots = idrots;  /* TEST: posa di riposo */

            /* TEST: solo mesh 0, vertici GREZZI senza skinning (bone-space?) */
            r3d_draw_mesh(P->model, 0, px, py, 0);
#endif
#endif
        }
        r3d_end_frame();
        swiWaitForVBlank();
    }
    return 0;
}
