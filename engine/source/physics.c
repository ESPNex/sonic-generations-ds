#include "physics.h"
#include "world.h"
#include <nds.h>
#include <math.h>

// ---- ORIGINAL sonic_c.bprm values (Classic Sonic) ----
static const float P_ACCEL  = 0.015f;   // idx 0
static const float P_DEC    = 0.1f;     // idx 4  (skid/frenata)
static const float P_FRIC   = 0.15f;    // idx 12 (attrito)
static const float P_TOP    = 2.25f;    // idx 2  (velocita' max corsa)
static const float P_JUMP   = 5.0f;     // idx 8
static const float P_GRAV   = 0.0593f;  // idx 16
static const float P_TERM   = 5.0f;     // idx 17 (velocita' di caduta max)
static const float P_SLOPE  = 0.08f;    // TBD: slope factor
static const float P_AIR    = 0.03f;    // aria = 2x accel (classico)

#define SIGNF(x) ((x) > 0.0001f ? 1.f : ((x) < -0.0001f ? -1.f : 0.f))

void phys_reset(Player* p, float x, float y) {
    p->x = x; p->y = y;
    p->vx = 0; p->vy = 0;
    p->angle = 0; p->grounded = 1; p->facing = 1;
    p->rolling = 0; p->jumped = 0;
}

static float slope_at(float x) {
    float dh = worldHeight(x + 0.5f) - worldHeight(x - 0.5f);
    return atan2f(dh, 1.0f);
}

void phys_step(Player* p, int held, int down) {
    int in = 0;
    if (held & KEY_RIGHT) in += 1;
    if (held & KEY_LEFT)  in -= 1;
    if (in) p->facing = in;

    if (p->grounded) {
        p->angle = slope_at(p->x);
        // slope factor (rotola giu' per le pendenze)
        p->vx -= P_SLOPE * sinf(p->angle);
        // input
        if (in != 0) {
            if (SIGNF(p->vx) != 0 && SIGNF(p->vx) != in)
                p->vx += in * P_DEC;              // skid
            else {
                p->vx += in * P_ACCEL;
                float s = SIGNF(p->vx);
                if (s != 0 && in == s && p->vx * s > P_TOP) p->vx = s * P_TOP;
            }
        } else {
            float s = SIGNF(p->vx);
            float f = P_FRIC;
            if (p->vx * s < f) p->vx = 0; else p->vx -= s * f;
        }
        // roll (giu' + velocita')
        if ((held & KEY_DOWN) && (p->vx * p->vx) > 0.04f) p->rolling = 1;
        if (p->rolling && (p->vx * p->vx) < 0.01f && !(held & KEY_DOWN)) p->rolling = 0;
        // salto lungo la normale del terreno
        if (down & KEY_A) {
            p->vx -= P_JUMP * sinf(p->angle);
            p->vy  = P_JUMP * cosf(p->angle);
            p->grounded = 0; p->jumped = 1;
            p->y += 0.02f;
        }
    } else {
        // controllo aereo
        if (in != 0) {
            p->vx += in * P_AIR;
            float s = SIGNF(p->vx);
            if (s != 0 && in == s && p->vx * s > P_TOP * 1.1f) p->vx = s * P_TOP * 1.1f;
        }
        // gravita' originale
        p->vy -= P_GRAV;
        if (p->vy < -P_TERM) p->vy = -P_TERM;
        if (!(held & KEY_A) && p->vy > 0 && p->jumped) p->vy -= P_GRAV * 1.4f; // salto variabile
    }

    p->x += p->vx;
    if (p->x < 0.5f)  { p->x = 0.5f;  p->vx = 0; }
    if (p->x > worldX1() - 0.5f) { p->x = worldX1() - 0.5f; p->vx = 0; }

    if (p->grounded) {
        float g = worldHeight(p->x);
        float drop = p->y - g;
        if (drop > 1.2f) {           // il terreno e' scappato via: decolla
            p->grounded = 0;
            p->vy = p->vx * sinf(p->angle);   // lancio da rampa
        } else {
            p->y = g;                // incollato al terreno
        }
    } else {
        p->y += p->vy;
        float g = worldHeight(p->x);
        if (p->vy <= 0 && p->y <= g) {   // atterraggio
            p->y = g; p->vy = 0;
            p->grounded = 1; p->jumped = 0;
            p->angle = slope_at(p->x);
        }
    }
}
