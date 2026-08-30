# EVT RE — DEEP REVERSE ENGINEING COMPLETO (code.bin 3DS)

Base Ghidra gproj2 = 0 (indirizzi file); runtime = file + 0x100000.
Qui sotto: **runtime** salvo indicato. (`file` = runtime - 0x100000)

## Catena loader (tutta verificata su disasm)

```
manager 0x128a54   (update per-frame, array layer +4/+8, count +0x14/+0x18)
  └─ layer_cull_load 0x30dbfc (file 0x20dbfc)
       ├─ carica stream idx 0/1/2 → bl 0x2fd598(stream_state, pos, idx)
       ├─ stream state stride 0x54: +0x14 idx0, +0x68 idx1, +0xbc idx2
       └─ patcha layer-instance globale @0x4d44e0 (file 0x3d44e0)
            {f32 bounds min[3] @0, max[3] @0xc, fn table @0x18}
  └─ stream processor 0x2fd598 (file 0x1fd598)
       ├─ chunk runtime: {u32 next_off@4 (0=fine), f32 X@8, f32 Y@0xc,
       │   u16 angle@0x10, u16 span@0x12, s16 z@0x14, u16 ngroups@0x16,
       │   groups@0x18}
       ├─ group runtime: {u16 count@0, s16 yanchor@2, u32 size@4, objs@8}
       ├─ handler table @0x4d4504 (file 0x3d4504):
       │   [0]=0x1340ec terreno, [1]=0x13dc64 oggetti-16B, [2]=0x146ca0 oggetti-8B
       └─ per group nel clip Y: blx handler[idx](group, chX, chY, angle)
```

## !! MODELLO OBJ v2 (CORRETTO — 2026-08-30, disasm handler + verifica incrociata)

Il modello s8.8 (`xy = y lo16 | x hi16`) qui sotto-era' **SBAGLIATO**: e' un artefatto.
L'obj FILE nei record B/C e' GIA' in formato runtime (il loader li usa in-place):

```
obj 8B  (stream C): { u16 tv@0, u8 x@2 (x0.5), u8 f3@3 (heading 0-255), s15 y@4 (x0.5), u16 par@6 }
obj 16B (stream B): idem + p2@8, p3@12
tv: type = tv & 0x1FF  (bic 0xFE00 = 11 bit ma la tabella ne usa 9), variant = (tv>>7)&0x3C,
    bit13/14 = flag, bit15 = done
RECORD payload = chunk runtime: { f32 X_MONDO@0 (!! es. 0x43960000=300.0), 0x80000000@4 (fine),
  0x00640000@8, w3@0xc, groups@0x10 }
GROUP: { u16 count@0, s16 anchor@2 (Y = anchor*100 + y*0.5), u32 size@4, objs@8 }
handler scelto da INDICE STREAM (0=terreno 6B, 1=B 16B, 2=C 8B) - tabella @0x4d4504
dispatcher 0x322ef4: ctor = slots[variant/4 - 1] (5 class-slot @0x4948f8[0..4]);
  accessor `ldr r1,[pc]; ldr r0,[r1, r0, lsl#2]` -> table2[type] = ctor
slot4 (variant 0x10) = thunk `mov r1,#K; b 0x303860` -> 0x212a14 ctor entita' generica
  (0xB00B, kind@0xa8) -> K = KIND Globale
TABELLA NOMI KIND @file 0x399494: {name\0, FFFF, 0,0, fn[]}: gmBoostEnergy, **E_BAT**,
  ..., "EneBlue"+"eagle.cpp" ...
```

### TYPE-ID verificati
- **type 14 = E_BAT** (kind 0xF; z11 3 es.: X=230/1292/2187 Y=100.5/100.5/0.5 f3=164/140/164
  p1=00010001, stream C; z12 18 dedup/21 grezzi X=1016..7587)
- type 13 = kind 0xE (gmBoostEnergy? n z11=9/z12=11), 15=kind 0x10, 17=kind 0x14,
  20=kind 0x17 (z12=150: probabile ring/pickup), 24=kind 0x13
- conteggi z11: 349 obj (58 tipi), z12 897 (68), z21 1031 (74), z31 817 (59), r01 259, boss1 3

### Mappatura corso motore (M7)
x_engine = (X_evt - 100) / (Xmax+128 - 100) * LEN_corso (z11 LEN=120, z12 LEN=170);
y = w3d_ground_below(x) + hover (scala Y evt<->mondo da risolvere col terrain 1:1, M6.6)

## Handler (semantica obj runtime — dopo espansione loader) [modello v1: vedere v2 sopra]

- **handler[0] 0x1340ec (terreno)**: obj 6B `{u8 x(×0.5), u8 y(×0.5), s15 depth(−v−15)}`
  → `spawn_terrain(0x1202fc)(obj, chX+x, yanchor×100+y, chY+z)`; done: `[o+2]|=0x8000`
- **handler[1] 0x13dc64 (oggetti 16B)**: obj `{u16 type|variant@0, s8 x@2(×0.5), u8 @3,
  s15 y@4(×0.5, bit15=done), +10B param}` → dispatcher(x+chX, y+yanchor×100, z+chY)
- **handler[2] 0x146ca0 (oggetti 8B)**: obj `{u16 type@0, u8 x@2(×0.5), u8 @3, s15 y@4}` → idem
- rotazione atan: se chunk angle ≠ 0 → ruota (x,depth) di angle×0.7111111 (0x3f360b61)

## Dispatcher evt 0x322ef4 (file 0x222ef4)

`{u16 v = [obj] ; type = v & 0x1FF (9 bit = indice TABELLA TIPI 549 slot @0x4948f8,
file 0x3948f8, slot = u32 ctor); variant = (v>>7)&0x3C} → ctor(type, x,y,z f32)`

## FORMATO FILE evt (z11evt.bin ecc.) — confermato

```
header: {u32 ?, u32 offB, u32 offC}   (z11: 0xb2c / 0x1fdc)
stream A @0x18: chunk top:
  {0x80000000, 0x00640000, 0, u32 inner(0x18), u32 sizeA, f32 X, [sub-chunks/groups]}
  stride chunk = 0x18 + sizeA (verificato geometricamente su tutti i chunk z11)
  X f32 @chunk+0x14 (100.0, 300.0, 800.0, 2300.0 in z11)
stream B @offB, C @offC: record chain {u32 prev_size, u32 this_size}
  payload = {u32 chk_prev, 0x80000000, 0x00640000, u32 yspan,
             ENTRY..., pad}
  ENTRY = {u16 type, u16 count, u32 param, count × 8B {u32 xy, u32 param2}}
  xy    = y s8.8 (lo16) | x s8.8 (hi16)   ← VERIFICATO: colononne/righe di anelli
          coerenti (5 anelli x≈40..44 y≈10 = colonna; righe a y≈10 passo 0.5)
  IL LOADER ESPANDE: entry → obj runtime {type (u16 da entry!), x, y, done-flag}
```

### Tabella tipi 549 @file 0x3948f8 (slot u32 = ctor)
Mappatura nomi (da CSV Ghidra + stringhe): vedi session memory
(265-288 nemici, 289-296 Spring, 299-300 Goal, 332-336 ItemBox, ...).
Entry-type dei record B/C = categoria locale; il tipo GMK vero nell'obj espanso.

## Scale runtime
- x,y obj: ×0.5 (mezzo-unità)
- yanchor group: ×100
- angle chunk: ×0.7111111 rad? (0x3f360b61), span@0x12
- depth terreno: −s15 −15.0

## z11map.bin (placement terreno)
{u32 n(114), poi n × {u16 a, u16 b}} + area dati: chunk-evt-like con f32
(100.0 × 177 occorrenze @0x1890+):布局 blocchi con coordinate f32 + z11coll.bin
box {x, y, cos(0.45/0.89 = 26.57°), sin, 0, h} (piano inclinato standard).
