# PIANO VERDE — Porting COMPLETO diretto (nessuna consegna intermedia)

Obiettivo: il gioco COMPLETO, tutto da asset originali decodificati direttamente.
Vietato: ricostruzioni, procedurali, fallback, sprite disegnati a mano, posizioni inventate,
**modelli statici** (ogni oggetto animato usa le sue animazioni skeletal originali).
Target: **DSi-only** (unitcode 3, SCFG_CLK, 16MB RAM / 133MHz). Verificatore: melonDS headless DSI.

## Stato fatto (M0–M5) — commit …cfa30e8 → a217a0d → 6884e25 → dc3145c → f8a0b54
- [x] M0 RE: decoder CGFX old-rev (gfx_mesh.py), AMB container, evt z11/z12/z21/z31/r01/boss1,
      collisioni MAP (24B record), bprm, skeleton 25 ossa, CANM/BoneAnim (**docs/EVT_RE_DEEP.md, ANIM_RE.md**).
- [x] M1 Motore 3D HW: render3d.c glTexImage2D+palette, v16/t16, camera 2.5D.
- [x] M2/M3-base: fisica bprm (gravità/salto/clamp), ground s15.16 per-act, SELECT act-switch.
- [x] M4 Skeletal: bake Q14 + `r3d_draw_mesh_anim` skinning 2-infl ARM9, verificato su HW (AB-test).
      P_SONC 84 anim / P_SONM 96 anim disponibili nei .amb (tutte decodificabili).
- [x] M5 2-act: z11 CLASSIC (model_sonic 11 mesh + anim_sonic 6) e z12 MODERN
      (model_sonm 9 mesh + anim_sonm 8) entrambi verificati su HW; fix bake3d mesh-table.
- [x] Nemici: estratti tutti i CGFX da ENE.amb → `sgds-data/enemies/` (v. inventario).

## INVENTARIO COMPLETO dei dati (TARGET.cpk_unpacked, riscansionione integrale)
| Cartella | Contenuto | Formato | Stato RE |
|---|---|---|---|
| GAME/PLAYER (10f) | P_CLS/P_MDN + BUR/LAS/GHO/FIN/SPE/RES_SPE + sonic_c.bprm + sonic_m.bprm | AMB→CGFX | ✓ CLS/MDN; 6 da fare |
| GAME/GIMMICK (21f) | COMMON_GMK{,_CLS,_MDN} + ZONE{1-7}{1,2}_GMK + RACE{1-3}_GMK + SS1_GMK | AMB→CGFX+CANM | da fare |
| GAME/ENEMY (21f) | ZONE*{E_HIDM,E_HIDC,E_KIK,E_BLE,E_EGD,E_SPI,E_BAT,E_GAD} + BOSS1-3/FINAL + RACE1-3 | AMB→CGFX | ✓ estratti tutti |
| GAME/BG (66f) | {Z,R}{n}_MAP.amb + Block/ (z11…z72, r01-r03 MDL interi!) + Z*_D_ALL/FAR_BG.bcres + SS1-7_BG + WS_BG + BOSS*_BG | AMB + CGFX | coll ✓ z11/z12; resto da fare |
| GAME/EFFECT (24f) | ZONE*_EFF + BOSS*_EFF + RACE*_EFF + SS1_EFF + WHITEMAP/MENU_EFF | AMB→? (particelle) | da fare |
| GAME/ENV (16f) | LT_S1-7, LT_Z1-7, LT_Z31/32, LT_ZF + *.bcenv | CGFX light/env | da fare |
| GAME/PARAM (68f) | G_PARAM: 4 ENE_* (EGGPAWN+WELCOME, GUARDON, GUNHUNTER, KUREAGEN), 3 BOSS_* (BIOLIZARD, EGGEMPEROR, B_BIG), **31 GMK_*** (airfan, air_ship, balloon, bar, break_land, bumper, charge_spring, dashring, dolphin, drop_target, elevator, flipper, land, mush_jump, n_spring, orca, pole_ud, spk_iron_ball, spring, timehole, vine_t, wall_run, water_land, water_land_z6, z7_sp1, z7_sp_meteor), R1-R3, S1-S7, RESULT_*(×22), SYSTEM | bprm | ✓ reader |
| GAME/MISSION (25f) | mission_z11…z72, s1-s7, boss1-3, final (.bmis) | binario `8c0a…` | da fare |
| MENU (2f) | MNCMN_PL.amb + MNCMN_BG.amb (modelli 3D menu) | AMB→CGFX | da fare |
| MSG (6f) | MSG_{E,F,G,I,J,S}.amb — tutti i testi localizzati | AMB→? | da fare |
| LAYOUT_{E,F,G,I,S} (29f×5) | TITLE/LOGO/ACT(HUD)/RESULT{,02,03,04}/OPTION/LOAD/MSNLOAD/CMNLOAD/COMMON/MENU_COMMON/MATCH/TIMEATK/CREDIT/THANK/BOSS/MISSION{,GAME,BOSS}/RACE/SS/SSMSN/SSRESULT/WHITEMAP/COLLECT/TEST/DEMO_LYT | AMB→BCLYT+tex | da fare |
| COLLECTION (1f) | COLLECTION.amb 8.8MB — galleria museo | AMB | da fare |
| DEMO (16f) | DEMO_CHAR1/2/3 + DEMO_CHAR_RES_{CLS,MDN,FIN} + DEMO_BG_{CSN,MHILL,GHILL,GHILL2,FGATE,BGATE,EPILOGE} + tkdm_story_demo.ayk + tkdm_credit.ayk + dm_credit.mg | AMB→CGFX + **AYK** + **#MSG** | da fare |
| SOUND (54f) | acb CRI @UTF: BGM_3DSanniv(+awb **CPK** 238+199MB!), BGM2_32k, 10SYS_{CMN,MNU,WMP,LOG,RSL}, 21/22PLY_SNC, 41/42OBJ_{CMN,BTL,SPS,GHZ,CNZ,EMC,MHZ,TPR,RHW,WTP}, 51/52ENM_{CMN,GHZ,EMC,MHZ,TPR,RHW,WTP,BLZ,BEE}, 51/52BOS_{BGA,BLZ,BEE,BSD,BLB,BSV,TPR}, 40AMB_CMN, 40OBJ_SPS, 60DMO_EVT, 80MIS_CMN, Anniv3DS.acf | CRI acb/awb→HCA | da fare |
| SOUND_{E,F,G,I,S} (7f) | 32VCE_{ACT,BEE,BLB,BSD,BSV,EVT_ALL}_{lang} + awb — **voci localizzate** | CRI | da fare |
| System (12f) | DebugFontTerminal.bcfnt, SkySphere.bcmdl, StencilShadow/ShadowObjects.bcres, RingData.bcres (anello!), Mii_Body, ICON_CEC.amb, DSCF*Tex.ctpk ×4 | CGFX/CTPK/BCFNT | da fare |
| pak/Modelli_BCRES (689f) | dump già scalzato dei modelli | CGFX | riferimento |
| RomFS/ExeFS | 3DS exec (code.bin RE continuo) | — | in corso |

Zone-id → nomi interni: GHZ(z1) CNZ(z2) EMC? no: EMC=MHZ? — mappa nomi da confermare in
OBJ/ENM acb: {GHZ, CNZ, EMC, MHZ, TPR, RHW, WTP} = zona 1-7 (da verificare l'ordine in MAP.amb di code.bin).

## Architettura runtime (decisione vincolante per M6+)
1. **Niente .h statici per i contenuti**: 14 act + 7 SS + 4 boss + race + menu superano ogni ROM.
   → decoder CGFX/CANM **runtime in ARM9** (porting C di gfx_mesh+anim_bake) che carica da
   filesystem (nitrofs nella .nds per i asset essenziali + streaming da SD per il grosso).
   La pipeline bake/.h resta solo come strumento di verifica.
2. Per-act bundle: MAP(stage+coll) + GMK + ENE + BG/D_ALL/FAR + LT_* + EFF caricati/scaricati.
3. Lingua selezionabile E/F/G/I/S (default **I**): MSG_* + LAYOUT_* + 32VCE_*.
4. Audio: HCA non esiste su DS → conversione lossless-di-origine → stream ADPCM (DSi) o
   maxmod SFX; la conversione di formato NON è rielaborazione (pixel/waveform originali).
5. UI 3DS 400×240 → DS 256×192: stessi asset, viewport/scala a runtime.

## M6 — Runtime asset system (sblocca tutto)
- [x] Porting ARM9: parser AMB + CGFX (modelli/texture ETC1/A4/RGB565/L8→texel DS) + CANM/bake Q14 a caldo
      (**gfxrt.c**: verificato nativamente IDENTICO alla pipeline bake: 11/11 mesh vtx/uv/idx/col,
      anim Q14 ±1 rounding; pivot ±152/512 solo Hair_top tie-break). SU HW: z11 classic player
      decodificato in tempo reale da PLAYER_CLS.amb (5670px vs baked 5362, fase idle diversa).
      Bug chiusi: bones-list = inti grezzi (non ptr), quat CANM = (k3,k0,k1,k2), tri mode0 step3,
      desc u16 = c/2 entry, ordine dict ossa = skel+0x2c, rounding half-even lrint.
- [x] M6.2 CARD READER (**romcard.c**): cmd 0xB7 polled (flag cardReadHeader libnds),
      FNT/FAT nitrofs (ndstool -d, NITRO_FILES in Makefile). VERIFICATO SU HW: entrambi gli
      act caricano PLAYER_CLS/MDN.amb DIRETTAMENTE dalla ROM (6.6MB di dati fuori dal binario
      ARM9). ROM 4.77MB. Lookup FNT validato offline byte-perfect in python prima del port.
- [x] M6.3-base: ZONE1_1_ENE.amb in nitrofs; E_BAT caricato via card + renderizzato
      con anim attack00-02 ORIGINALI (debug spawn, posizioni evt = TODO mappa type-id).
      Fix: scratch dict separati (an/av) per skel_anims (il player funzionava solo per
      coda stale!), cache texture per-modelo in render3d (niente churn glDelete/Gen),
      ordine rombuf (player decodificato PRIMA che l'ENE sovrascriva il buffer).
      LOAD ~50s su melonDS → ottimizzare: BLK_SIZE(4) 0x1000B/op + via CLK_SLOW/DELAY.
- [x] M6.5 ASSET LOADING SU HW REALE (SD/TWLMenu++): romcard v2 con backend FILE via
      DLDI (argv[0] di nds-bootstrap + scansione cartelle, identita verificata dal
      contenuto nitrofs), card bus solo emulatori (con TIMEOUT anti-hang slot vuoto),
      fallback baked. LED stato su subscreen: verde=file, blu=card, ciano=card+fat,
      giallo/rosso=nessuno. VERIFICATO in melonDS con DLDI image+foldersync: LED verde,
      load ~20s (3x piu veloce del card bus). Bug fixato: probe file con g_mode=0
      (non leggeva mai) + path "fat://" doppio slash.
- [ ] M6.4: speedup card read (BLK_SIZE grande) — solo per emulatore; su HW passa da DLDI.
- [ ] VRAM management: texture LRU per-act, palette bank.
- [ ] Verifica: z11 caricato da file (non .h) identico al bake. [x per PLAYER_CLS embedded]
- NOTA melonDS: config nel profilo si corrompe (pkill) → tools/mrun2.sh ora usa
  XDG_CONFIG_HOME=/tmp/mdscfg + tools/melonDS.toml deterministico (BIOS DSi esterni).
  "Secure area decryption failed" nel log = warning innocuo (homebrew). Il loader BIOS
  wrappa binario ARM9 >4MB (PU region 7 = 4MB): i dati vivono nel nitrofs, non nel binario.

## M7 — Nemici & IA (TUTTI e 8)
- [ ] Spawn dallo stream C evt → tipo nemico per zona (E_BAT z1, E_GAD z2, E_HIDM/HIDC z3,
      E_KIK z4, E_BLE z5, E_EGD z6, E_SPI z7) con modelli+anim ENE originali (mai statici).
- [ ] IA da bprm ENE_* (EGGPAWN/GUARDON/GUNHUNTER/KUREAGEN — mappare nome↔tipo E_*) + pattern evt.
- [ ] Contact damage, invuln, perdita anelli, distruzione con anim+EFF originali.

## M8 — Gimmick TUTTI (31 tipi + comuni)
- [ ] COMMON_GMK_CLS/MDN: ring, item box, molle c/m, dash pad/dash ring, bar, pulley…
- [ ] Per-zona ZONE*_GMK: airfan, air_ship, balloon, bumper, charge_spring, dolphin,
      drop_target, elevator, flipper, mush_jump, n_spring, orca, pole_ud, spk_iron_ball,
      vine_t, wall_run, water_land(+z6), z7_sp1, z7_sp_meteor, break_land, timehole, land…
      ciascuno con G_PARAM_GMK_* + modelli+anim originali.
- [ ] Verifica headless per ogni gimmick usato in z11/z12.

## M9 — Zone rimanenti z21→z72 (tutti e 14 gli act)
- [ ] Block/*.bcres + {Z}_MAP.amb + evt + coll + LT_* + FAR_BG + D_ALL per ogni zona.
- [ ] Runtime evt loader (architettura già in docs/EVT_RE_DEEP.md): stream C=piano di gioco.
- [ ] Goal/sign-post, transit act, result (G_PARAM_RESULT_*).
- [ ] Verifica headless: ogni act percorribile dall'inizio alla fine.

## M10 — Boss & Rivali
- [ ] BOSS1 B_BIG (67 anim) + BOSS2 B_BIO (78) + BOSS3 B_EMP (43) + FINAL B_TET (32):
      modelli/anim/BG/EFF + G_PARAM_BOSS_{BIOLIZARD,EGGEMPEROR} + bprm B_BIG + boss evt (boss1 ✓ RE).
- [ ] RACE r01-r03: rivali B_MTA/B_SHA/B_SIL, RACE*_GMK/EFF/ENE + RACE_LYT + G_PARAM_R1-3.
- [ ] Boss-select door / missioni boss.

## M11 — Special Stages SS1-7
- [ ] SS1-7_BG + SS1_GMK + SS1_EFF + G_PARAM_S1-7 + SS_LYT/SSMSN_LYT/SSRESULT_LYT.
- [ ] Chaos Emerald → sblocco finale (FINAL_ENE/BG/EFF + LT_ZF + DEMO_CHAR_RES_FIN).

## M12 — White Space / mappa (hub)
- [ ] WS_BG + WHITEMAP_LYT/EFF: world map con gate zona, progressione salvata.

## M13 — Player COMPLETO (niente pose congelate)
- [ ] sonic_m.bprm: homing, wall stick/run, grind, boost gauge, water run, sliding, airboost…
      tutte le 96 anim P_SONM cablate sugli stati.
- [ ] Classic: spin dash, roll, tutte le 84 anim P_SONC.
- [ ] PLAYER_BUR/LAS/GHO/FIN/SPE + RES_SPE: capire ruolo (form speciali/multiplayer) e cablare.
- [ ] StencilShadow/ShadowObjects + SkySphere + RingData (anello 3D originale).

## M14 — UI completa (LAYOUT + MSG + font)
- [ ] RE LYT.amb (BCLYT+texture) → renderer UI DS; TITLE/LOGO/ACT HUD/RESULT/OPTION/LOAD/
      MATCH/TIMEATK/CREDIT/THANK/BOSS/MISSION*/RACE/SS*/WHITEMAP/COLLECT/COMMON/MENU_COMMON.
- [ ] MNCMN_PL/BG (menu 3D) + BCFNT→font DS + MSG_*.amb testi (default I).

## M15 — Missioni & Collezione
- [ ] RE .bmis (26 file) → missioni per act/boss/SS + MISSION_LYT/MISSIONGAME/MISSIONBOSS/SSMSN.
- [ ] COLLECTION.amb + COLLECT_LYT + ICON_CEC: galleria completa.

## M16 — Audio COMPLETO
- [ ] RE acb @UTF (cue→wave) + awb (CPK→HCA) + acf; convertitore → DSi (stream ADPCM + SFX).
- [ ] BGM per zona/menu/boss/SS + SFX SYS/PLY/OBJ/ENM/BOS/AMB/DMO/MIS + VCE voci localizzate.
- [ ] Runtime: cue system evento→suono (maxmod).

## M17 — Cutscene & storia
- [ ] RE AYK (timeline cutscene) + dm_credit.mg (#MSG script) + DEMO_CHAR1-3/BG_* modelli+anim.
- [ ] Player cutscene: storyboard+camere+anim originali; crediti滚动 (CREDIT_LYT + tkdm_credit).

## M18 — Sistema & sigillo finale
- [ ] Save (progressi/record/collezione/opzioni), Time Attack (G_PARAM_RESULT_*), lingua.
- [ ] code.bin coverage finale (docs/codebin_coverage.md), sigillo TWL+CRC.
- [ ] Verifica headless TUTTI gli act + boss + SS + menu. Nessuna texture rielaborata.

## Regola di completamento
Il gioco è "completo" quando: ogni act di ogni zona (z11→z72, r01-03, s1-07, boss1-3, final)
è giocabile dall'inizio alla fine con placement originale, oggetti/nemici/boss originali con
TUTTE le loro animazioni skeletal originali, audio originale (musiche+sfx+voci), UI/menu/
testi/missioni/collezione/cutscene originali, salvataggi — su melonDS DSi headless.
SEGA non l'ha mai fatto: noi sì.
