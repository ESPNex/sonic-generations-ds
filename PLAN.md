# PIANO FULL-CONVERSION — SONIC GENERATIONS DS
## Da Sonic Colors DS a Sonic Generations 3DS

- **Tipo di progetto**: full-conversion ROM hack — **NON** più port diretto con motore custom.
- **Base**: la ROM retail di **Sonic Colors DS** (sul box: `/home/lardx/colors.zip`).
- **Contenuto iniettato**: asset originali di **Sonic Generations 3DS** (già scompattato sul box,
  cartella TARGET.cpk_unpacked) e della ROM Colors stessa.
- **Obiettivo**: la ROM finale deve sembrare, suonare e giocare **il più possibile identica a
  Sonic Generations 3DS**, stando viva dentro il motore di Sonic Colors DS.
- **Console target**: Nintendo DSi (verifica su melonDS headless, modalità DSi).

---

## 0. Cambio di strategia (perché)

Il vecchio approccio ("port diretto": motore 3D nostro + decoder CGFX runtime su DS) è
archiviato nel branch **`archive/direct-port`** (ultimo commit `e77d6cf`) — resta riferimento
per decoder CGFX/AMB, RE evt/collisioni e tool Python.

Con la full-conversion partiamo da un gioco DS **completo e funzionante**: motore, fisica,
rendering, audio, salvataggi, atti. Sonic Colors DS (Dimps) è il cugino stretto di Generations
3DS: stessa formula 2.5D/boost, atti in stile classico. **Invece di riscrivere un motore da
zero, sostituiamo TUTTO il contenuto e la presentazione**: loghi, title, menu, musiche, SFX,
texture, e progressivamente zone/nemici/gimmick.

## 1. Regole vincolanti

1. **Asset solo originali**: ogni logo/texture/musica/menu deriva dagli asset di Sonic
   Generations 3DS o dalla ROM Colors. Vietato ridisegnare o inventare.
2. **Verifica = VIDEO**: ogni milestone si chiude con un **video della build** su melonDS
   (Xvfb + ffmpeg, vedi `tools/mrunvid.sh`). Mai screenshot come consegna.
3. **Giudice visivo = l'utente** (niente Gemma / niente CV / niente OCR per decidere).
4. **Unpack/repack ROM con strumenti nativi Linux** (ndstool su tutti), NON con script Python.
5. Report/commit/messaggi in italiano. Ogni passo: commit + video + report.

## 2. Specifiche DSi (ricerca) — riferimento

| Componente | DS (NTR) | DSi (TWL) |
|---|---|---|
| CPU principale | ARM9E 67 MHz | ARM9E fino a **133 MHz** (SCFG_CLK) |
| Co-CPU | ARM7 33 MHz | ARM7 66 MHz |
| RAM principale | 4 MB | **16 MB** (4×) |
| VRAM | 656 KB (bank A–I) | 656 KB (+ nuovi modi via SCFG/MBK) |
| Schermi | 2 × 256×192 TFT | 2 × 256×192 TFT (più grandi 3.25") |
| Storage gioco | cartuccia | cartuccia + **SD/SDHC** + NAND 256 MB |
| Audio | 16 ch PCM/ADPCM | 16 ch PCM/ADPCM (migliore uscita) |
| Extra | — | 2 camere, registri **SCFG_* (0x4004xxx)**, MBK1–9 |

Header ROM: `unitcode` @0x012 = 0 (NTR), 2 (DSi-enhanced ibrido), 3 (DSi-only).

### 2b. Decisione NTR vs TWL
Colors DS è ROM **NTR**: su DSi gira in compatibilità NTR (67 MHz / 4 MB / no SD). Sbloccare
DSi-mode (133 MHz / 16 MB / SD) richiede conversione binaria TWL **e firme RSA Nintendo** per
l'hardware reale; su melonDS è aggirabile (direct boot). **Strategia: M0–M5 in NTR-mode**
(la ROM gira su qualsiasi DS/DSi e su melonDS semplice); ibrido TWL valutato a **M6** con
documento dedicato (`docs/DSI_TWL.md`). Nessuna decisione bloccante ora.

## 3. Toolchain Linux (ricerca — tutti strumenti nativi/CLI, non-Python per l'unpack)

| Compito | Strumento | Note |
|---|---|---|
| Estrarre/ricostruire ROM NDS | **ndstool** (devkitPro tools o build 2.1.2) | `-x rom.nds -9 arm9.bin -7 arm7.bin -y9 y9.bin -y7 y7.bin -d data -y overlay -t banner.bin -h header.bin`; repack `-c out.nds …` |
| Patch arm9/overlays (asm) | NitroPacker (dotnet CLI), devkitARM/Ghidra | BLZ decompress ARM9 inclusa (`-d`) |
| Decompressione BLZ/LZ10/LZ11 | dsdecmp (jar), CUE DS compressors | per bin compressi nel data FS |
| Audio SDAT (estrarre/sostituire) | **Nitro Studio 2**, VGMTrans, NDS Sound Extractor / sdatxtract (C, CLI) | SSEQ/SBNK/SWAR/STRM/SWAV |
| HCA 3DS → WAV | **vgmstream** CLI | acb/awb CRI dei SYS/BGM/voci 3DS |
| WAV → STRM/SWAV | wav2swav / loveemu tools / writer documentato | loop point dai cue originali |
| 2D Nitro (NCLR/NCGR/NSCR/NFTR) | Tinke, NitroPaint, CrystalTile | texture/palette/tilemap/font |
| 3D Nitro (NSBMD/NSBTX/NSBCA) | MKDS Course Modifier, apicula | modelli/texture 3D |
| Banner (icona+titolo) | ndstool `-b` / banner editor | 32×32 16 colori + titolo 6 lingue + CRC |
| RE codice | Ghidra (+loader NDS), arm-none-eabi-objdump | arm9.bin + y9 overlay table |
| Video build | Xvfb + melonDS + **ffmpeg statico** | `tools/mrunvid.sh` → mp4 |

## 4. Milestones — ognuna chiude con commit + VIDEO

### M0 — Fondamenta & inventario  *(in corso)*
- [x] Svuota repo (main) + archivio vecchio progetto su branch + questo piano.
- [ ] `unzip colors.zip` → ROM base; **ndstool -x** → dump completo (arm9/arm7/y9/y7/banner/header + data FS).
- [ ] **Inventario Colors DS**: mappa del filesystem (SDAT, NARC/archivi Dimps .bb/.bac, layout boot, title/menu) → `docs/COLORS_LAYOUT.md`.
- [ ] `tools/extract.sh` + `tools/repack.sh` riproducibili (bash + ndstool).
- [ ] Repack 1:1 **non modificato** → **video di boot** su melonDS (pipeline end-to-end provata).

### M1 — Identità di boot (logo SEGA + loghi d'avvio Generations)
- Banner ROM: titolo "SONIC GENERATIONS" + icona dagli asset 3DS (6 lingue).
- Sequenza di avvio: individuare gli sponsor/logo screens nel FS (SEGA, Dimps/Sonic Team) e
  sostituirli con i logo di **Generations 3DS** (da LAYOUT/LOGO 3DS → formato 2D DS).
- Risultato: boot → **logo SEGA (Generations)** → Sonic Team → title Colors (placeholder).
- **Video: boot completo della build.**

### M2 — Title screen + menu iniziale di Generations
- Title 3DS (sfondo, logo del gioco, "PRESS START") ricostruito sullo stato title di Colors
  (conversione nei formati 2D usati dal gioco).
- **Menu principale Generations** (voci in italiano da `MSG_I` 3DS): atti/Green Hill, opzioni
  base — wiring sulle entry esistenti del menu Colors (target temporanei = livelli Colors).
- **Video: navigazione title → menu.**

### M3 — Musica boot/menu (HCA → SDAT)
- Tracce menu/boot 3DS (`10SYS_MNU`, `10SYS_LOG`, BGM menu): **vgmstream** acb/awb → WAV.
- WAV → **STRM** (o SSEQ+SWAR) e sostituzione nel `sound_data.sdat` di Colors (cue title/menu),
  con loop point.
- **Video CON AUDIO: menu Generations con la sua musica.**

### M4 — Audio completo
- Mappa cue↔cue completa: musiche di zona (BGM GHZ classic/modern…), SFX (SYS/PLY/OBJ/ENM),
  voci dove fattibile → `docs/AUDIO_MAP.md`.
- Sostituzione progressiva in SDAT mantenendo i nomi cue del motore.

### M5 — Texture/interfaccia complete
- **Tutte** le texture 2D di menu/HUD/risultati/opzioni sostituite con le controparti 3DS dai
  `LAYOUT_*` (TITLE/LOGO/ACT-HUD/RESULT/OPTION/LOAD/CMNLOAD/WHITEMAP…), font (BCFNT→NFTR).
- **Loading screen Generations** (con tips) al posto dei caricamenti Colors.
- **Video: walkthrough di tutte le schermate.**

### M6 — Zone (Green Hill per prima)
- Atti GHZ classic+modern dai dati RE 3DS (evt z11/z12, già decodificati nel vecchio progetto)
  adattati agli atti del motore Colors.
- Nemici 3DS (E_BAT…) come reskin/comportamenti sui tipi nemici Colors; gimmick (spring,
  dash ring, item box) in stile Generations.
- **Qui la decisione TWL-hybrid** (16 MB aiutano gli stage grandi) → `docs/DSI_TWL.md`.
- **Video: playthrough GHZ.**

### M7+ — Resto del gioco (ordine da definire dopo M6)
Zone 2–7 (z21→z72 dai dati 3DS), boss (B_BIG/B_BIO/B_EMP) come conversione dei boss Colors,
rivali/race, Special Stage → Chaos Emerald, hub White Space sulla mappa Colors, cutscene DEMO
semplificate, COLLECTION/galleria, crediti.

### MX — Sigillo finale
Testi completi (default IT, poi E/F/G/J/S), salvataggi, opzioni, verifica video end-to-end
dell'intero gioco, eventuale upgrade TWL + label, release `sgds-colors.nds`.

## 5. Definizione di "identico a Generations 3DS"
Per OGNI schermata/stato: logo uguale, palette uguale, font uguale, **stessa musica**, stessi
SFX/voci di menu, stesso flusso (boot → title → menu → atto → result). Checklist validata
dall'utente sui **video**.

## 6. Riferimenti
- Vecchio progetto (decoder CGFX/AMB, RE evt/coll, note): branch `archive/direct-port`.
- GBATEK (specs DSi/NDS, SCFG/MBK), devkitPro ndstool, haroohie-club/NitroPacker,
  loveemu/vgmstream, VGMTrans, Oreo639/sdatxtract, Nitro Studio, MKDS Course Modifier, Tinke.
