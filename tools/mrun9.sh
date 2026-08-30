#!/bin/bash
# mrun9.sh — 30s SENZA input dopo il load: il loop nero/scena è innescato
# dall'input o è endogeno? (shot ogni 0.5s = 60 frame)
set -u
ROM="$1"; OUT="${2:-/tmp/shots9}"
mkdir -p "$OUT" /tmp/mdscfg9/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg9/melonDS/melonDS.toml
elif [ -f /tmp/melonDS.toml ]; then cp /tmp/melonDS.toml /tmp/mdscfg9/melonDS/melonDS.toml
fi
export XDG_CONFIG_HOME=/tmp/mdscfg9
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :93 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:93 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run9.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 48
# 60 shot a 0.5s, NESSUN input
k=0
while [ $k -lt 60 ]; do
  printf -v n "%02d" $k
  import -window root "$OUT/N$n.png" 2>/dev/null
  k=$((k+1))
  sleep 0.5
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
echo "=== boot events ==="; grep -n -E 'booting' /tmp/m_run9.log
