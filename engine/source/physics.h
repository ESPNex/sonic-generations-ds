#pragma once

// Player state (world units, y-up, per-frame velocities @60fps)
typedef struct Player {
    float x, y;
    float vx, vy;
    float angle;      // ground slope (rad, >0 = uphill toward +x)
    int   grounded;
    int   facing;     // +1 / -1
    int   rolling;
    int   jumped;
} Player;

// Parametri fisici — valori ORIGINALI estratti da GAME/PLAYER/sonic_c.bprm
// (float puri, little endian). Semantica in revisione via Ghidra (gmPly).
//   accel .015 = idx0 | dec .1 = idx4 | fric .15 = idx12 | top 2.25 = idx2
//   jump 5.0 = idx8   | grav .0593 = idx16 | terminal 5.0 = idx17
// slope/air: da confermare (idx9/10 = .03) — default classici al momento.
void phys_reset(Player* p, float x, float y);
void phys_step(Player* p, int keys_held, int keys_down);
