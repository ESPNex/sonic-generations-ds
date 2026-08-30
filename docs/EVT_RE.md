# RE evt — stato dell'arte (aggiornato)

## Struttura file evt (CONFERMATA su z11/z12/z21/z31/r01)
- header: [0]=0xc, [1]=offset stream B, [2]=offset stream C; +0x10: {0x18, 0}
- TRE stream di "chunk" indicizzati dal float X (ancora da 100 in 100):
  A: 0x18..[1]  (terreno!), B: [1]..[2], C: [2]..EOF
- chunk (streams B/C confermati, A da validare):
  {u32 prev_size, u32 this_size,          <- catena bidirezionale
   u32 0, f32 X, u32 0x80000000, u32 0x00640000 (span (0,100)),
   s8.8 (y_top=1, y_bottom=-100) come 0x0001ff9c,
   poi ENTRY...}
- ENTRY: {u16 type, u16 count, count x u32 (x s8.8 << 16 | y s8.8)} fino a
  terminatore 0xcccccccc (fill Codewarrior) o fine payload.
- stream A: chunk SENZA coppia prev/this (start fisso 0x18), header
  {0x80000000, 0x00640000, 0, 0x18, size, 0, f32 X, ...} — stesso formato
  entry. Contiene il terreno (gtype 0x17 = gmGmkTerrain).

## Tabella tipo -> classe Gmk (DECODIFICATA)
- Codice: `*(ushort*)rec & 0x1ff` = indice; dispatcher a 0x152da4:
  `ldr r1,=0x495098; ldr r0,[r1,r0]; bx r0`
- TABELLA @ 0x495098 (61 slot, 0x00..0x3c): vedi sgds-data/gmk_table.txt
- NOTE: 0x17 e 0x1b = gmGmkTerrain; 0x0c ArrivePoint; 0x0d SPipe; 0x0e Sliding;
  0x12 CorkScrew; 0x21 KeepOut; 0x28=40 WaterFlow/Stream/Slider (0x2b/2c/2d);
  0x33 Z7Special2. Nomini completi da ghidra_out.csv (solo ctor con assert).
- Seconda tabella @ 0x4948f8 (104 slot) = ene_table.txt (probabile nemici).
- Aggiornare i nomi mancanti decompilando i ctor (Ghidra su box: progetto
  /tmp/gproj, script /tmp/DumpFuncs.java e DumpCallers.java).

## Coordinate
- u32 = (x s8.8 << 16) | (y s8.8 & 0xffff), s8.8 = s16/256
- Unità = unità modelli (Sonic alto 1.35); X stage z11 = 0..3353

## Prossimo passaggio (parser v2)
1. Parsing stream A per X-float + entry con terminatore cccc
2. Dump completo placement z11: per ogni X, lista (type, count, punti)
3. Mappa type->nome da gmk_table.txt + decompile ctor mancanti (ring? spring?)
4. Correlazione con i modelli z11_mdl.bcres (dict 112 nomi) per il terreno
