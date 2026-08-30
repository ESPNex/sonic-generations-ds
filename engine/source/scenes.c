#include <nds.h>
#include <stdio.h>
#include "game.h"
#include "world.h"

// ---------------- dati stage (dal RomFS) ----------------
// zone dai nomi dei file audio/oggetti originali: 41OBJ_{GHZ,CNZ,MHZ,RHW...}.acb
// Dati_BIN: {r01,z11,z12,z21,z22,z31,z32,z41,z42,boss1,boss2,boss3}{coll,evt,map}.bin
const StageInfo stage_list[] = {
    { "z11", "GHZ", "GREEN HILL  AT 1", 1 },
    { "z12", "GHZ", "GREEN HILL  AT 2", 1 },
    { "z21", "CNZ", "CASINO NIGHT AT 1", 1 },
    { "z22", "CNZ", "CASINO NIGHT AT 2", 1 },
    { "z31", "MHZ", "MUSHROOM HILL AT 1", 1 },
    { "z32", "MHZ", "MUSHROOM HILL AT 2", 1 },
    { "z41", "RHW", "ZONE 4  AT 1", 1 },
    { "z42", "RHW", "ZONE 4  AT 2", 1 },
    { "r01", "RIVAL", "RIVAL BATTLE 1", 1 },
    { "boss1", "BOSS", "BOSS ATTACK 1", 1 },
    { "boss2", "BOSS", "BOSS ATTACK 2", 1 },
    { "boss3", "BOSS", "BOSS ATTACK 3", 1 },
};
const int stage_count = sizeof(stage_list) / sizeof(stage_list[0]);

Game g;
static Scene scene = SCENE_TITLE;
static int loading_tick = 0;
static int loading_target = 60;   // ~1s di loading (con tip)
static int result_rank = 0;

static void title_draw(void);

void scene_init(void) {
    g.rings = 0; g.score = 0; g.lives = 3;
    g.time_frames = 0; g.stage = 0; g.act_clear = 0;
    consoleClear();
    title_draw();          // il titolo si vede SUBITO
}
Scene scene_current(void) { return scene; }
void scene_set(Scene s) {
    scene = s;
    loading_tick = 0;
    consoleClear();
}

const char* stage_display_name(int i) {
    return stage_list[i].name;
}

// ---------------- disegno console (schermo inferiore) ----------------
static void title_draw(void) {
    iprintf("\x1b[2J");
    iprintf("\x1b[6;5H@@@@  @   @  @@@@@   @@@@@");
    iprintf("\x1b[7;5H@     @@  @  @       @   @");
    iprintf("\x1b[8;5H@@@@  @ @ @  @@@@@   @@@@@");
    iprintf("\x1b[9;5H   @  @  @@  @       @ @");
    iprintf("\x1b[10;5H@@@@  @   @  @@@@@   @  @");
    iprintf("\x1b[13;3H G E N E R A T I O N S   D S");
    iprintf("\x1b[18;7H--- PORTING DIRETTO 3DS ---");
    iprintf("\x1b[21;9HPREMI  START");
}

static void map_draw(int cursor) {
    iprintf("\x1b[2J");
    iprintf("\x1b[1;8HSONIC GENERATIONS DS");
    iprintf("\x1b[2;8H - SELEZIONE STAGE -");
    iprintf("\x1b[4;2H%-17s %-4s %s", "STAGE", "DATI", "STATO");
    for (int i = 0; i < stage_count; i++) {
        int row = 6 + i;
        if (row > 22) break;
        const char* sel = (i == cursor) ? ">" : " ";
        iprintf("\x1b[%d;1H%s%-14s %-4s %s", row, sel,
                stage_list[i].name,
                stage_list[i].playable ? "OK" : "--",
                stage_list[i].playable ? "" : "[locked]");
    }
    iprintf("\x1b[23;1H<-> muovi   A scegli   stage: %s",
            stage_list[cursor].code);
}

static const char* TIPS[] = {
    "TIP: 100 anelli = 1UP",
    "TIP: tieni A per saltare piu' in alto",
    "TIP: giu' mentre corri = rotolata",
    "TIP: i dati livello vengono da Dati_BIN",
    "TIP: la fisica e' quella di sonic_c.bprm",
};

static void loading_draw(void) {
    iprintf("\x1b[2J");
    iprintf("\x1b[6;1H%s", stage_display_name(g.stage));
    iprintf("\x1b[8;1Hcaricamento dati di gioco...");
    int w = loading_tick * 24 / loading_target;
    iprintf("\x1b[10;1H[");
    for (int i = 0; i < 24; i++) iprintf("%c", i < w ? '=' : ' ');
    iprintf("]");
    iprintf("\x1b[13;3H%s", TIPS[g.stage % 5]);
}

static int results_calc(void) {
    // rank sui tempi (soglie in secondi) — come le tabelle G_PARAM_RESULT
    int t = g.time_frames / 60;
    if (t < 45) return 0;      // S
    if (t < 60) return 1;      // A
    if (t < 80) return 2;      // B
    if (t < 120) return 3;     // C
    return 4;                  // D
}

static void results_draw(void) {
    const char RC[] = "SABCD";
    int rb = g.rings * 100;
    int tb = (g.time_frames / 60) < 60 ? 5000 : 1000;
    iprintf("\x1b[2J");
    iprintf("\x1b[3;8H== RESULTS ==");
    iprintf("\x1b[6;3HSTAGE   %s", stage_display_name(g.stage));
    iprintf("\x1b[8;3HTIME     %d'%02d\"%02d", g.time_frames / 3600,
            (g.time_frames / 60) % 60, (g.time_frames % 60) * 50 / 30);
    iprintf("\x1b[10;3HRINGS    %d   bonus %d", g.rings, rb);
    iprintf("\x1b[12;3HTIME BONUS  %d", tb);
    iprintf("\x1b[14;3HSCORE    %d + %d + %d = %d", g.score, rb, tb, g.score + rb + tb);
    g.score += rb + tb;
    iprintf("\x1b[17;6H     RANK  %c", RC[result_rank]);
    iprintf("\x1b[21;7HA = mappa    START = rifai");
}

static void gameover_draw(void) {
    iprintf("\x1b[2J");
    iprintf("\x1b[10;6HG A M E   O V E R");
    iprintf("\x1b[16;8HSTART = mappa");
}

// ---------------- update per scena (ritorna scena nuova o -1) ----------------
#define NOCHANGE (-1)

int scene_update(int held, int down) {
    static int cursor = 0;
    static int loaded_stage = 0;
    switch (scene) {
    case SCENE_TITLE:
        if (down & KEY_START) { scene_set(SCENE_MAP); map_draw(cursor); }
        break;
    case SCENE_MAP:
        if ((down & KEY_RIGHT) || (down & KEY_DOWN)) { cursor = (cursor + 1) % stage_count; map_draw(cursor); }
        if ((down & KEY_LEFT)  || (down & KEY_UP))   { cursor = (cursor + stage_count - 1) % stage_count; map_draw(cursor); }
        if (down & KEY_A) {
            if (stage_list[cursor].playable) {
                g.stage = cursor; g.rings = 0; g.score = 0;
                g.time_frames = 0; g.act_clear = 0;
                loaded_stage = cursor;
                scene_set(SCENE_LOADING); loading_draw();
            } else {
                iprintf("\x1b[23;1H%-32s", "dati stage non ancora convertiti");
            }
        }
        break;
    case SCENE_LOADING:
        loading_tick++;
        if (loading_tick >= loading_target) { scene_set(SCENE_GAME); }
        else if ((loading_tick & 15) == 0) loading_draw();
        break;
    case SCENE_RESULTS:
        if (down & KEY_A)      { scene_set(SCENE_MAP); map_draw(cursor); }
        if (down & KEY_START)  {
            g.stage = loaded_stage; g.rings = 0; g.time_frames = 0;
            scene_set(SCENE_LOADING); loading_draw();
        }
        break;
    case SCENE_GAMEOVER:
        if (down & KEY_START) { g.lives = 3; scene_set(SCENE_MAP); map_draw(cursor); }
        break;
    case SCENE_GAME:
        break;
    }
    return NOCHANGE;
}

void scene_notify_goal(void) {
    result_rank = results_calc();
    scene_set(SCENE_RESULTS);
    results_draw();
}
void scene_notify_death(void) {
    g.lives--;
    if (g.lives <= 0) { scene_set(SCENE_GAMEOVER); gameover_draw(); }
}
