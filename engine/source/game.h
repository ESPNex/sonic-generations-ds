#pragma once

typedef enum {
    SCENE_TITLE,
    SCENE_MAP,
    SCENE_LOADING,
    SCENE_GAME,
    SCENE_RESULTS,
    SCENE_GAMEOVER
} Scene;

typedef struct {
    int rings;
    int score;
    int lives;
    int time_frames;     // 60 = 1s
    int stage;           // indice in stage_list
    int act_clear;
} Game;

// stage reali del RomFS (Dati_BIN / G_PARAM / 41OBJ_*.acb)
typedef struct {
    const char* code;    // z11, z12, ...
    const char* zone;    // GHZ, CNZ, ... (dai nomi file originali)
    const char* name;
    int playable;        // 1 = dati collisione gia' convertiti
} StageInfo;

extern const StageInfo stage_list[];
extern const int stage_count;

extern Game g;

void scene_init(void);
Scene scene_current(void);
void scene_set(Scene s);
const char* stage_display_name(int i);
