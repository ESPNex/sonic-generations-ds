# COLORS_LAYOUT — inventario del contenuto di Sonic Colors DS (M0)

Fonte: `/home/lardx/colors_zip/Sonic Colors (USA) (En,Ja,Fr,De,Es,It).nds`
(64 MB, dump 2016, USA multi-lingua En/Ja/Fr/De/Es/**It** — perfetto per testi in italiano).
Estratta con `tools/extract.sh` (ndstool, strumento Linux nativo — niente Python) in
`/home/lardx/sgds2/rom/`.

## Struttura dump
- `arm9.bin` (binario principale, compresso BLZ), `arm7.bin` (156 KB)
- `y9.bin` / `y7.bin` (overlay table ARM9/ARM7), `banner.bin`, `header.bin`
- `overlay/` (overlay ARM9)
- `data/` (NitroFS del gioco)

## data/ — directory top-level
| Dir | Ipotesi contenuto | Note |
|---|---|---|
| `act` | dati atti/livelli | da mappare per M6 (zone) |
| `ayk` | timeline AYK (stesso formato cutscene di Generations 3DS!) | interessante per cutscene |
| `banner` | banner/icone interne | 8 file `.bnr` |
| `bb` / `.bbg` | formati custom Dimps (grafica) | 22 `.bb` + 67 `.bbg` |
| `bg` | background/screen 2D | **probabile sede dei logo di boot** → M1 |
| `dwc` | Nintendo WFC | ignorabile |
| `fnt` | font (2 `.fnt`, 2 `.nbfp`, 2 `.nbfc`) | per testi menu |
| `mb` | multiboot | ignorabile |
| `mod` / `.mods` | moduli/model streaming (6) | |
| `movie` | FMV | |
| `narc` | 355 archivi NARC | bulk di modelli/texture/sprite |
| `snd` | **12 SDAT** | vedi sotto |
| `ss` | special stage | |
| extra | 1 `.srl` (inner ROM), 1 `.ssd`, 1 `.bin`, 2 `.ayk` | |

## Conteggio per estensione
355 `.narc` · 67 `.bbg` · 22 `.nsbmd` (modelli Nitro 3D) · 22 `.bb` · 12 `.sdat` · 8 `.bnr` ·
7 `.sst` · 6 `.mods` · 2 `.nbfp`/`.nbfc`/`.fnt`/`.ayk` · 1 `.srl`/`.ssd`/`.bin`

## Audio — i 12 SDAT
`snd/sound_data.sdat` (main) + per sezione: `gmmain`, `gmcom`, `gmcomex`, `vsmenu`,
`spst` (special stage), `rz1`, `rz2`, `rz3`, `rz6` (+1 altra).
→ M3: la musica del title/menu di Generations 3DS andrà nel SDAT usato da title/menu
(da individuare quale dei 12 serve la schermata).

## Repack 1:1 (prova pipeline)
`tools/repack.sh` → `rebuild.nds` (59.9 MB — il padding è rimosso, normale).
**Risultato: boota e renderizza su melonDS headless.** Video: `vid_boot_repack.mp4`
(registrato con `tools/mrunvid.sh`, Xvfb + ffmpeg statico).

## TODO M1
- [ ] Identificare i file dei logo screens (SEGA / Sonic Team / Dimps) in `data/bg` o NARC boot.
- [ ] Game code / unitcode esatti da header.bin (atteso NTR, USA "B*S E").
- [ ] Individuare quale SDAT suona a title/menu (probe: sostituzione silenzio → ascolto).
- [ ] Dove sta la title screen (BG+palette+tilemap) e il menu principale (testi in `fnt`+script).
