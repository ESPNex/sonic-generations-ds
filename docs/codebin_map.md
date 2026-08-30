# code.bin — mappa del codice di gioco (ExeFS, ARM11, 4.2 MB)

sha256: 8e2d421114988f61a9a54fc3ed736528dc73229526d154e86cc27322bd7d11c2
Caricato a 0x00100000, ARM (branch table in testa), misto ARM/Thumb.
Analisi: Ghidra 12 headless sul box (vedi /tmp/gproj).

## Cosa sappiamo dal binario
- Il gioco e' DATA-DRIVEN: logica in code.bin, valori nei .bprm,
  livelli in Dati_BIN {coll,evt,map}. Il porting diretto = questi 3 pezzi.
- **210 file sorgente** leaked via stringhe di assert/log (percorsi
  .\\src\\*.cpp e .\\library\\CTRFramework\\): engine interno Sega
  (prefissi gm/dm/Gm + CTRFramework Nintendo).
- 99 oggetti GMK (gameplay objects), 21 scene SCENE_*, ~20 nemici gmEne*,
  3 boss (BioLizard, EggEmperor, TimeEater gmFB*).

## Fisica (GAME/PLAYER/sonic_{c,m}.bprm — float puri, little endian)
Classico (216 B) / Modern (272 B), indici float:
| idx | Classic | Modern | ipotesi |
|-----|---------|--------|---------|
| 0 | 0.015 | 0.020 | accel |
| 2 | 2.25 | 3.40 | max run speed |
| 8 | 5.0 | 5.5 | jump velocity |
| 16 | -0.0593 | -0.05 | gravity/frame |
| 17 | -5.0 | -3.5 | terminal velocity |
| 25 | 50 | 65 | ? (boost gauge?) |
| 34 | 20 | 20 | ? |
(semantiche da confermare con xref Ghidra ai load dei bprm)

## Oggetti GMK con parametri (G_PARAM_GMK_*.bprm)
- SPRING: 2.25 (spinta), 0.25, sin/cos 0.707 = 45 gradi (2 direzioni)
- DASHRING: 2.2, 1.0, 45 gradi | N_SPRING: 5.0 | CHARGE_SPRING: 2.0
- R1..R3 (rival/boss), S1..S7 (stage 1-7), SYSTEM (tabelle tempo/score u32)

## Inventario (da stringhe)
### File sorgente (210)
```
Boss/BioLizard/gmBioStageManager.h
Boss/EggEmperor/gmEmpStageManager.h
CCecControl.cpp
CGlobalNetwork.cpp
CNetwork.cpp
GmGmkBalloon.cpp
GmGmkZ7SpecialMeteor.cpp
dmObjScnMgr.cpp
dmObject.cpp
dmScriptAyks.cpp
dmScriptCredit.cpp
dmScriptMsg.cpp
dmScriptVM.cpp
gmBioLizardEggManager.cpp
gmBioStageManager.cpp
gmBoss01Stage.cpp
gmBossBioLizard.cpp
gmBossStartup.cpp
gmCamera.cpp
gmCamera.h
gmCannon.cpp
gmDeco.cpp
gmDecoEffect.cpp
gmEffect.cpp
gmEggEmperor.cpp
gmEmpStageManager.cpp
gmEne.h
gmEneBatabata.cpp
gmEneBeeton.cpp
gmEneBlueeagle.cpp
gmEneEggpawn.cpp
gmEneEggpawnWelcome.cpp
gmEneEggpawnY.cpp
gmEneFlapper.cpp
gmEneGanigani.cpp
gmEneGuardon.cpp
gmEneGunhunter.cpp
gmEneHidun.cpp
gmEneKiki.cpp
gmEneKureagen.cpp
gmEneMeleon.cpp
gmEneMogumogu.cpp
gmEneMonobeetle.cpp
gmEneMotora.cpp
gmEneRenotank.cpp
gmEneSpinner.cpp
gmEneSweep.cpp
gmEneTefutefu.cpp
gmEneTonber.cpp
gmFBBg.cpp
gmFBCamera.cpp
gmFBDebrisMng.cpp
gmFBEffect.cpp
gmFBMain.cpp
gmFBObstacle.cpp
gmFBPly.cpp
gmFBPlyModel.cpp
gmFBTimeEater.cpp
gmFBTimeEaterArm.cpp
gmFarBg.cpp
gmFarBgZ1.cpp
gmFarBgZ2.cpp
gmFarBgZ3.cpp
gmFarBgZ3_2.cpp
gmFarBgZ4.cpp
gmFarBgZ5.cpp
gmFarBgZ6.cpp
gmFarBgZ7.cpp
gmGmkAirFan.cpp
gmGmkAirShip.cpp
gmGmkArrivePoint.cpp
gmGmkBEDBlockWall.cpp
gmGmkBEDBreakWall.cpp
gmGmkBEDSearch.cpp
gmGmkBar.cpp
gmGmkBeltConveyor.cpp
gmGmkBigEggDiver.cpp
gmGmkBombBalloon.cpp
gmGmkBreakLand.cpp
gmGmkBreakPier.cpp
gmGmkBreakPillar.cpp
gmGmkBridge.cpp
gmGmkBubble.cpp
gmGmkBumper.cpp
gmGmkBurnWall.cpp
gmGmkBurningCatapult.cpp
gmGmkChargeSpring.cpp
gmGmkClpsTotem.cpp
gmGmkCorkScrew.cpp
gmGmkDTarget.cpp
gmGmkDashPanel.cpp
gmGmkDashRing.cpp
gmGmkDashRingLayer.cpp
gmGmkDepthCtrl.cpp
gmGmkDolphin.cpp
gmGmkEggTotem.cpp
gmGmkElevator.cpp
gmGmkElevatorBody.cpp
gmGmkFireTotem.cpp
gmGmkFish.cpp
gmGmkFlap.cpp
gmGmkFlipper.cpp
gmGmkGoal.cpp
gmGmkGull.cpp
gmGmkItemBox.cpp
gmGmkJack.cpp
gmGmkJumpStand.cpp
gmGmkKeepOut.cpp
gmGmkLand.cpp
gmGmkLaserTotem.cpp
gmGmkLensFlare.cpp
gmGmkMissionWall.cpp
gmGmkMushJump.cpp
gmGmkMushWall.cpp
gmGmkMushWeight.cpp
gmGmkNeedle.cpp
gmGmkNeonLand.cpp
gmGmkNumberSpring.cpp
gmGmkOrcaJump.cpp
gmGmkOrcaMove.cpp
gmGmkPointCage.cpp
gmGmkPointMarker.cpp
gmGmkPoleMoveLR.cpp
gmGmkPoleMoveUD.cpp
gmGmkPrism.cpp
gmGmkPropeller.cpp
gmGmkPsychoSmash.cpp
gmGmkPulley.cpp
gmGmkPushFloor.cpp
gmGmkRock.cpp
gmGmkRocket.cpp
gmGmkRoller.cpp
gmGmkSPipe.cpp
gmGmkSign.cpp
gmGmkSliding.cpp
gmGmkSlot.cpp
gmGmkSpecifyArea.cpp
gmGmkSpecifyAreaManager.cpp
gmGmkSpecifyAreaManager.h
gmGmkSpikeIronBall.cpp
gmGmkSpring.cpp
gmGmkSpringLayer.cpp
gmGmkSpringPole.cpp
gmGmkSwitch.cpp
gmGmkTerrain.cpp
gmGmkTimeHole.cpp
gmGmkUpReel.cpp
gmGmkVineStretch.cpp
gmGmkVineTwisted.cpp
gmGmkWallRun.cpp
gmGmkWaterArea.cpp
gmGmkWaterFlow.cpp
gmGmkWaterGenerator.cpp
gmGmkWaterGun.cpp
gmGmkWaterJet.cpp
gmGmkWaterLand.cpp
gmGmkWaterSlider.cpp
gmGmkWaterStream.cpp
gmGmkWaterWheel.cpp
gmGmkWispBox.cpp
gmGmkWoodBox.cpp
gmGmkZ1Special1.cpp
gmGmkZ2Special1.cpp
gmGmkZ3Special1.cpp
gmGmkZ4Special1.cpp
gmGmkZ7Special1.cpp
gmGmkZ7Special2.cpp
gmMap.cpp
gmMissionParam.cpp
gmPlayerControllerBase.cpp
gmPly.cpp
gmPlyCtrlManager.h
gmPlyEffect.cpp
gmRaceManager.cpp
gmResult.cpp
gmSceneEnvironment.cpp
gsGameData.cpp
gsLayout.cpp
gsMiiRender.cpp
gsMiiUtility.cpp
gsNetMatch.cpp
gsSaveData.cpp
gsSound.cpp
lytCollectLower.cpp
lytCtr.cpp
lytEndCredit.cpp
lytHelp.cpp
lytPauseManual.cpp
lytRes.cpp
mnCommonLayout.cpp
objCollision.cpp
objObject.cpp
objRes.cpp
objTask.cpp
objTerrainObj.cpp
scCecLoglistMenu.cpp
scCollectModelMenu.cpp
scGlobalMatch.cpp
scProfCardListMenu.cpp
scWhiteMapMenu.cpp
scWhiteMapResource.cpp
ssBg.cpp
ssEmerald.cpp
ssGimmick.cpp
ssGmkMng.cpp
ssMain.cpp
ssMap.cpp
ssPly.cpp
tdsRes.cpp
tdsRes.h
```
### Oggetti GMK (99)
```
GMK AIR SHIP
GMK BALLOON
GMK BED BLOCK WALL
GMK BED BREAK WALL
GMK BED SEARCH
GMK BELT CNVR
GMK BIG BUBBLE
GMK BIG EGG DIVER
GMK BOMB BALLOON
GMK BREAK LAND
GMK BREAK PIER
GMK BREAK PILLAR
GMK BRIDGE
GMK BUBBLE
GMK BUMPER
GMK BURN CATA
GMK BURN WALL
GMK C SPRING
GMK CLPS TOTEM
GMK DASH PANEL
GMK DASH RING
GMK DOLPHIN
GMK DROP TARGET
GMK EGG TOTEM
GMK ELEVATOR
GMK ELEVATOR BODY
GMK ELEVATOR MOVE
GMK FIRE TOTEM
GMK FISH
GMK FLAP PATH_COL
GMK FLIPPER
GMK GOALPANEL
GMK GOALRING
GMK GRAIL_CTRL
GMK GRAIL_LANE
GMK GULL
GMK ITEM_BOX
GMK JACK
GMK JUMP STAND
GMK KEEP OUT
GMK LAND
GMK LASER TOTEM
GMK LAYER SPRING
GMK LENS FLARE
GMK METEOR
GMK MISSION WALL
GMK MUSH_JUMP
GMK MUSH_WALL
GMK NEEDLE
GMK NEON LAND
GMK NUMBER SPRING
GMK ORCA JUMP
GMK ORCA JUMP STAND
GMK ORCA MOVE
GMK PF ONE STEP
GMK PIPE S
GMK PLY CTL MANAGER
GMK POINT CAGE
GMK POINT MARKER
GMK PRISM
GMK PROPELLER
GMK PULLEY
GMK PUSH FLOOR
GMK ROCK
GMK ROCKET
GMK ROCKET BASE
GMK ROCKET HANDLE
GMK ROLLER
GMK SIGN
GMK SLIDING
GMK SLOT
GMK SOUP BUBBLE
GMK SP ZONE7 NO1
GMK SPECIFY AREA MANAGER
GMK SPK IRON BALL
GMK SPRING
GMK SPRING POLE
GMK SWITCH
GMK TERRAIN
GMK TERRAIN ZONE2
GMK TIME HOLE
GMK UP REEL
GMK VINE_STRETCH
GMK VINE_TWISTED
GMK WALL RUN
GMK WATER FLOW
GMK WATER GENERATOR
GMK WATER GUN
GMK WATER JET
GMK WATER LAND
GMK WATER PLANE
GMK WATER SLIDER
GMK WATER STREAM
GMK WATER WHEEL
GMK WATER_AREA
GMK WISP BOX
GMK WOOD_BOX
GMK Z2 SPECIAL
GMK Z7SP1_ROCKET
```
### Parametri tb_* (UI/menu, 149)
```
tb_01
tb_02
tb_1
tb_10
tb_1_1
tb_1_2
tb_1_3
tb_1_4
tb_2
tb_20
tb_3
tb_30
tb_3_1
tb_3_2
tb_3_3
tb_3_4
tb_4
tb_40
tb_5
tb_50
tb_6
tb_art_1
tb_art_2
tb_art_3
tb_art_4
tb_art_5
tb_btl_b
tb_c
tb_card
tb_chara
tb_coin_num
tb_coll
tb_count
tb_cs
tb_data_1
tb_data_2
tb_day
tb_f_b_00
tb_haji
tb_height
tb_help
tb_help_c
tb_help_m
tb_his
tb_homing
tb_ivent_1
tb_ivent_2
tb_ivent_3
tb_ivent_4
tb_ivent_5
tb_jump
tb_jump_dush
tb_kiroku
tb_kounyu
tb_kousin_1
tb_kousin_2
tb_l
tb_local
tb_lv
tb_m
tb_men_01
tb_men_02
tb_men_03
tb_men_04
tb_men_05
tb_menu_u
tb_mis
tb_model_1
tb_model_2
tb_model_3
tb_model_4
tb_model_5
tb_ms
tb_msn
tb_msn_haji
tb_msn_title
tb_msn_txt
tb_name
tb_name1
tb_name2
tb_name_1
tb_name_2
tb_name_aaa
tb_name_your
tb_net
tb_no
tb_no_art_1
tb_no_art_2
tb_no_art_3
tb_no_art_4
tb_no_art_5
tb_no_ivent_1
tb_no_ivent_2
tb_no_ivent_3
tb_no_ivent_4
tb_no_ivent_5
tb_no_model_1
tb_no_model_2
tb_no_model_3
tb_no_model_4
tb_no_model_5
tb_no_msn
tb_no_sound_1
tb_no_sound_2
tb_no_sound_3
tb_no_sound_4
tb_no_sound_5
tb_no_sund
tb_now
tb_r
tb_race_b
tb_rating
tb_rc_name_1
tb_rc_name_2
tb_rc_name_3
tb_rc_name_4
tb_rc_name_5
tb_ring
tb_s
tb_sound_1_1
tb_sound_1_2
tb_sound_2_1
tb_sound_2_2
tb_sound_3_1
tb_sound_3_2
tb_sound_4_1
tb_sound_4_2
tb_sound_5_1
tb_sound_5_2
tb_spst
tb_sund_1_1
tb_sund_1_2
tb_syoji
tb_syuhen
tb_syuhen_00
tb_syuhen_01
tb_syuhen_02
tb_syuhen_03
tb_time
tb_tit
tb_title_c
tb_title_m
tb_txt
tb_update
tb_victories
tb_vs_1
tb_vs_2
tb_year
tb_yes
```
