# Copertura code.bin — porting integrale (tracker)

Obiettivo: **tutto** ciò che fa il gioco 3DS entra nel porting DSi.
Fonte: docs/codebin_map.md (210 sorgenti, 99 GMK, 21 scene da code.bin).

| Sistema (da code.bin) | Sorgenti originali | Stato port | Note |
|---|---|---|---|
| Fisica player Classic/Modern | gmPly.cpp + sonic_{c,m}.bprm | ✅ valori originali | slope/air da confermare con Ghidra |
| Collisioni terreno | objCollision.cpp + {stage}coll.bin | ✅ bake 12 stage | 2.5D heightmap |
| Placement oggetti | dmObjScnMgr/dmObject + {stage}evt.bin | 🔶 RE in corso | firme record + coord s8.8 decodificate |
| Anelli | GMK (ring) + evt | ✅ raccogli/1UP | placement evt da innestare |
| Camera | gmCamera.cpp | ✅ base | lerp 2 assi |
| Selettore stage/scene | SCENE_* (21 scene) | ✅ title/map/loading/results/gameover | layout UI originali in LAYOUT_*.amb |
| Zone/atti (12) | Dati_BIN + G_PARAM_S* | ✅ tutti giocabili | palette per zona |
| Traguardo/results/rank | GMK GOAL* + G_PARAM_RESULT | ✅ | soglie tempo come G_PARAM |
| Molle/dash panel | gmGmk{Spring,DashPanel,DashRing…}.cpp | ⏳ prossimi | bprm già decodificati |
| Nemici (20) | gmEne*.cpp | ⏳ | modelli e_* già estratti |
| Boss (3+Final) | gmBoss*/gmFB* | ⏳ | modelli b_* già estratti |
| Audio | CRI acb/awb + 21PLY_SNC | ⏳ | conversione maxmod |
| Musica stream | BGM awb 437MB | ⏳ | DSi: stream da NAND/SD |
| Missioni | mission_*.bmis + SCENE_MISSION* | ⏳ | |
| Collezioni/ring road | COLLECTION.amb | ⏳ | |
| Save | nn::cekfs?/profilo | ⏳ | extdata DSi |
| Grafica player | PLAYER_CLS.amb | ✅ texture→sprite | da refinare con mesh CMDL |
| Grafica stage | {stage}_mdl.bcres | ✅ texture→sfondo+palette | mesh blocchi in RE |
| Effetti/PFD | gmEffect/gmFB* | ⏳ | |
| Rete/match | CNetwork/CGlobalNetwork | ❌ fuori scope DSi (no WiFi match) | documentato in SPECS |

DSi-only: ROM sigillata unitcode 3 + SCFG_CLK boost 133 MHz (tools/seal_twl.py).
