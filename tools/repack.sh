#!/bin/bash
# M0 — Ricostruzione ROM dai file estratti (ndstool). La ROM ripaccata e' la base dei patch.
# Uso: repack.sh [BASE_DIR] [OUT_NDS]
set -e
BASE="${1:-/home/lardx/sgds2}"
OUT_NDS="${2:-$BASE/rebuild.nds}"
export PATH=/opt/devkitpro/tools/bin:$PATH
cd "$BASE/rom"
ndstool -c "$OUT_NDS" -9 arm9.bin -7 arm7.bin -y9 y9.bin -y7 y7.bin \
  -d data -y overlay -t banner.bin -h header.bin
echo "OK: scritta $OUT_NDS"
