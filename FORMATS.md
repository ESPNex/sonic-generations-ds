# Formati dati — Sonic Generations 3DS (fonte RomFS)

Percorso estratto: `~/sgnds/extracted` → `TARGET.cpk_unpacked`, poi
`~/nds/pak/out/Dati_BIN/{stage}{coll,evt,map}.bin` per
`r01,z11,z12,z21,z22,z31,z32,z41,z42,boss1..3`.

## *.coll — libreria di forme di collisione (spazio LOCALE)
- Header: u32 = 1 (versione, NON un contatore).
- Corpo: sezioni di record concatenati da 6 float: `x, y, z, sin, cos, k`.
  - (sin,cos) è sempre una coppia unitaria (terreno: (0,±1)).
  - **Le coordinate sono locali all'oggetto-collisa** (colline rampe si
    ripetono con x che riparte da 0): il file è una LIBRERIA di forme,
    il posizionamento nel mondo sta in `*.evt`.
  - Gap tra sezioni: 10/6/9 float (record con campi extra o padding).
- Estrazione affidabile: scorrere il file, allineare record a 6 float,
  accettare solo **run consecutivi >= 2 di record validi** (punto+angolo
  unitario). Run singoli isolati = falsi positivi della testata.
- Segmento = punto[i]→punto[i+1], stesso z, distanza < 26.
  Ground truth: catena collina `(2.75,20)→(8.5,18.5)→(13.7,16.2)→(18.2,14.1)`;
  primo record `(0,40,-19)` pendenza 26.565° (sin .4472 cos .8944);
  angolo con segno opposto ad atan2(dy,dx) → confrontare moduli (tol 2.5°).
- z11: ~268 punti, 223 segmenti validi; piani a z −19 (e −12/−10); mondo ≈ x 0..160, y −29..130.

## *.evt — placement oggetti (WIP)
- Testata tipo TOC: u32 `(12, 2860, 8156, 0, 24, ...)` → sezioni/offset.
- Misto interi e float; **tabelle di rotazione s16** (23170 = 32767·sin45°,
  stride 12 byte, valori che crescono di 20) → matrici/quaternioni fixed-point.
- Coordinate mondo probabilmente a punto fisso (bounds −100..100 visibili
  come interi; pochissimi float in range mondo).

## *.map — tabella tile/chunk
- Tabella u16: primo valore 114, poi interi piccoli 1..35 → id chunk/tile
  della griglia mappa.

## Pipeline DS
mondo = forme(coll) × placement(evt) → bake polilinee collisione →
engine DS (terreno 2.5D, camera GHZ-style). Map = gfx tile, non collisione.

## Mesh BCRES (revisione Generations) — note RE
- Gli offset CMDL di 3dbrew NON calzano: header con flags 0x40000092,
  lista SOBJ non a +0xB8. In z11_mdl: 1 CMDL contenitore, **112 oggetti
  nel dict modelli** (stage a blocchi), **670 SOBJ** totali.
- Scan diretto `TXOB` (flags a magic-4) funziona su TUTTI i file provati
  (z11: 7 tex, dm_ghill_res_c: 14, sonic_pose: 5) — vedi cgfx_extract.py v3.
- Prossimo: mappare a mano header CMDL di questa revisione partendo dal
  dump a 0xba0 (z11_mdl) e dal dump del dict modelli.

## MESH: la chiave e' SPICA (CtrGfx) — mappa completa
Il formato BCRES di Generations = SPICA "CtrGfx" (gdkchan/SPICA, MIT).
Riferimento banco in sandbox: spica-ref/*.cs (persistance workspace).
- TypeChoice: 0x01000005 GfxMesh, 0x10000001 GfxShape (mesh+vertici!),
  0x40000012 GfxModel, 0x40000092 GfxModelSkeletal (il "CMDL"),
  0x40000002 GfxVertexBufferInterleaved, 0x40000001 GfxAttribute,
  0x08000000 GfxMaterial, 0x20000011 GfxTextureImage.
- OGNI GfxObject: GfxRevHeader{flags u32, 'SOBJ'} + Name(ptr) + MetaData(dict ptr).
- PUNTATORI: u32 SELF-RELATIVE (addr += field_pos - 4) — spiega tutti gli
  offset "piccoli" visti nei dump.
- GfxShape: Flags, BBox(6f), PositionOffset(3f), SubMeshes(list), BaseAddress,
  VertexBuffers(list), BlendShape(ptr).
- GfxVertexBufferInterleaved: BufferObj, LocationFlag, RawBuffer(ptr->section),
  LocationPtr, MemoryArea, VertexStride, Attributes(list di GfxAttribute).
- GfxAttribute: ... RawBuffer, ... Format(GL enum), Elements, Scale(f), Offset.
- GfxFaceDescriptor: Format(u8 GL enum: 0x1401=U8? 0x1403=USHORT), PrimitiveMode,
  pad4, RawBuffer(ptr) = INDICI u8/u16, PrimitiveMode = triangoli/quads/strip.
=> prossimo: port del BinaryDeserializer in Python (tools/gfx_mesh.py) e
   estrazione mesh DIRETTE (pos+uv+indici per blocco) da tutti i {zone}_mdl.
=> TARGET DIRETTO: niente piu' ricolor/procedural — rendering dai mesh veri.
