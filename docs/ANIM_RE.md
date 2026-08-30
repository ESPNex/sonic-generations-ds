# ANIMAZIONI SKELETAL — RE (in corso, già leggibile al 100% dei dati)

Fonte: `sgds-data/amb/PLAYER_CLS.amb` (classic) / `PLAYER_MDN.amb` (modern) /
`COMMON_GMK_CLS.amb` — vedi tabella risorse in code.bin (@file 0x3bf579+).

- `.amb` "#AMB": entry {off,size,-1,0}; ogni entry = file CGFX intero
- CGFX root dict **skel_anims** (ROOT[9]): P_SONC = **84 anims** (idle/run/push/sqat/
  brake/stumble/lookup...) — `tools/anim_parse.py` le parsa tutte
- CANM: {magic, rev, name, loop, DURATION(frame), flags, ntracks, DICT}
  - track per OSSA REALE: Hips/Spine/Neck/Hair_*/UpperArm_*/ForeArm_*/Hand_*/
    Thigh_L/Foot_L/Thigh_R/Foot_R (12-15)
  - chiavi: **1 key/frame**, stride 20B = {f32 x, quat unit} — dur+2 chiavi
  - c1=8, c2=12 costanti (probabile formato/interp per canale)

Prossimo passo: bake anim selezionate → array C {frame × bone × quat s16} +
skeleton da CMDL (bones/inverse-bind in skeleton_dict già parsato da gfx_mesh)
→ **software skinning** su ARM9 (palette matrici ossa, 1.5k vtx × 4 infl).
