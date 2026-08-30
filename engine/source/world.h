#pragma once

#include "world_data.h"   // generato: NON committato (dato derivato)

// Terreno per-stage (bake da {stage}coll.bin originali)
void        worldSet(int stage_index);
const char* worldCode(void);
float       worldHeight(float x);
float       worldSpawnX(void);
float       worldX1(void);
