#!/bin/bash
# mrun10.sh — sequenza ALTA FREQUENZA (0.15s) per osservare la transizione nero<->scena
set -u
ROM="$1"; OUT="${2:-/tmp/shots10}"
mkdir -p "$OUT" /tmp/mdscfg10/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg10/melonDS/melonDS.toml; fi
export XDG_CONFIG_HOME=/tmp/mdscfg10
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :91 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:91 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run10.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 48
# 130 frame a 0.15s = ~19.5s
k=0
while [ $k -lt 130 ]; do
  printf -v n "%03d" $k
  import -window root "$OUT/H$n.png" 2>/dev/null
  k=$((k+1))
  sleep 0.15
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
grep -n -E 'booting' /tmp/m_run10.log
