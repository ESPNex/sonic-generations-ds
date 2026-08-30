#!/bin/bash
# Re-bake di tutti i model_*.h con bake3d (linear) + gfx_mesh (ETC1 bit-exatto),
# rebuild e cattura. Da eseguire sul box dopo git pull.
set -e
cd /home/lardx/sonic-generations-ds
git pull origin main
A=/home/lardx/sgnds/extracted/pak/out/Modelli_BCRES
for m in "p_sonc:sonic" "p_sonm:sonm" "g_cmn_ring:ring" "g_cmn_dash_ring_a:dashring" "e_bat:ebat" "g_cmn_item_box:itembox" "g_cmn_spr_c:spr_c" "g_cmn_spr_m:spr_m"; do
  src="${m%%:*}"; name="${m##*:}"
  echo "== bake $src -> model_$name.h"
  python3 tools/bake3d.py --model "$A/$src.bcres" --name "$name" --out engine/source
done
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITARM/bin:$PATH
make -C engine
ls -la engine/sgds.nds
