#include "world.h"
#include <string.h>

static const WorldDef* cur = &worlds[0];

void worldSet(int i) { if (i >= 0 && i < WORLD_COUNT) cur = &worlds[i]; }
const char* worldCode(void) { return cur->code; }

float worldHeight(float x) {
    float fi = (x - WORLD_X0) / WORLD_STEP;
    int i = (int)fi;
    if (i < 0) return cur->h[0];
    if (i >= cur->n - 1) return cur->h[cur->n - 1];
    float t = fi - i;
    return cur->h[i] * (1.0f - t) + cur->h[i + 1] * t;
}

float worldSpawnX(void) { return 2.0f; }

float worldX1(void) { return WORLD_X0 + (cur->n - 1) * WORLD_STEP; }
