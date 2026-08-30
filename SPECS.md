# Nintendo DS/DSi — vincoli hardware di target

## DSi (target primario)
| Voce | Valore |
|---|---|
| CPU | ARM9 @ 133 MHz + ARM7TDMI @ 33 MHz — **no FPU: solo fixed-point** (1.19.12 / 16.16 / 12.4) |
| RAM | 16 MB (DSi mode) / **4 MB @ 67 MHz in DS-compat** (fallback: DS phat/lite via flashcard) |
| VRAM | 656 KB in 9 banche: A–D 128KB (texture/BG/OBJ), E 64KB, F/G 16KB (palette), H 32KB, I 16KB |
| Schermi | 2× 256×192, 15-bit RGB out (18-bit interno 3D); 3D visibile solo su schermo A (superiore) |
| Storage | 256 MB NAND + SD (FAT32); homebrew: .nds via Unlaunch/hiyaCFW/TWiLight |
| Audio | 16 canali HW, PCM16/IMA-ADPCM, ~32.7 kHz; DSi codec migliorato |

## Motore 3D hardware (il motivo per cui il porting è viable)
- Fixed-function, integrato come **BG0 del 2D engine A**; comandi via GXFIFO (256 entry) / glCallList
- **Limiti per frame: 2048 poligoni / 6144 vertici** (dopo clipping; vertex RAM 144 KB, polygon RAM 104 KB)
- Budget pratico: ~2000 poli visibili a 60 fps; **modelli ≤ ~400 triangoli**
- Primitive: tri/quad/tri-strip/quad-strip; vertex cache riusa i vertici trasformati tra strip
- Texture: PoT 8×8→1024×1024; **max 512 KB** (banche A–D); palette in E/F/G; formati A3I5, A5I3, 4/16/256 colori, texcompr; texcoord 12.4; **niente filtering** (nearest → stile pixel-art)
- Feature: prospettiva corretta, alpha test/blend, fog, **toon shading**, edge marking, luci per-vertice, stack matrici 32, doppio buffer geometria/rendering
- Uscita catturabile in framebuffer (display capture) se servisse il 3D anche sul touch screen

## Pipeline asset (dal dump 3DS su LARDEX-PC → non committati nel repo)
| Asset 3DS | Formato | Conversione DS |
|---|---|---|
| Modelli | **CGFX/BCRES** (magic `CGFX`, SOBJ/TOBJ) | parser Python → display list libnds (fixed-point), decimazione ≤400 tri, texture → 256-color/A5I3 |
| Texture | CTPK | → VRAM texture (PoT, palettizzate) |
| Audio | ACB/AWB (CRI) → vgmstream | → PCM16/ADPCM (mm7 / maxmod) o stream |
| Fisica | `GAME/PARAM/*.bprm` | → header C (valori autentici) |
| Stage | archivi AMB (amb/), `mission_z*.bmis` | → binario livello + layout oggetti |
| Codice | `ExeFS/code.bin` (ARM11) | solo consultazione/RTTI per decifrare i formati — mai eseguito |

## Piano motore
- Schermo superiore: 3D hardware (Green Hill 2.5D classico); inferiore: 2D engine B (HUD touch/pausa)
- Fisica fixed-point a step fisso 60 fps; costanti Sonic dai .bprm (autenticità 3DS)
- Un solo .nds: usa RAM/clock DSi quando disponibile (Unlaunch/nds-bootstrap), degrada su DS

## Nota legale
Il repo contiene SOLO codice e tool di conversione. Gli asset Nintendo/SEGA convertiti
restano sulla macchina locale e non vengono mai committati.

## UPDATE TARGET (deciso dall'utente): DSi-ONLY
- Il porting NON deve funzionare su DS: target esclusivo Nintendo DSi.
- Sblocca: ARM9 133 MHz (vs 67), RAM 16 MB (vs 4), NAND/SD.
- ROM finale: unit code TWL (3) + header esteso; boot via Unlaunch/nds-bootstrap.
- Nessun vincolo di compatibilita' NTR: si puo' allocare liberamente e
  usare la CPU extra per geometria/FX.
