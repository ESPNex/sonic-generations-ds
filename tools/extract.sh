#!/bin/bash
# M0 — Estrazione ROM Sonic Colors DS con ndstool (strumento Linux nativo, NON python)
# Uso: extract.sh [BASE_DIR]   (default /home/lardx/sgds2)
set -e
BASE="${1:-/home/lardx/sgds2}"
ROM="$BASE/colors_zip/Sonic Colors (USA) (En,Ja,Fr,De,Es,It).nds"
OUT="$BASE/rom"
export PATH=/opt/devkitpro/tools/bin:$PATH
[ -f "$ROM" ] || unzip -o "$BASE/colors.zip" -d "$BASE/colors_zip"
mkdir -p "$OUT"; cd "$OUT"
ndstool -x "$ROM" -9 arm9.bin -7 arm7.bin -y9 y9.bin -y7 y7.bin \
  -d data -y overlay -t banner.bin -h header.bin
echo "OK: estratto in $OUT"
