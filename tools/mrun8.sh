#!/bin/bash
# mrun8.sh — cattura AD ALTA FREQUENZA (0.3s) attorno al JUMP per capire il
# pattern nero/scena: quando il game va nero, dopo quanto torna, e se la
# schermata eccezione (text su MAIN) appare mai.
set -u
ROM="$1"; OUT="${2:-/tmp/shots8}"
mkdir -p "$OUT" /tmp/mdscfg8/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg8/melonDS/melonDS.toml
elif [ -f /tmp/melonDS.toml ]; then cp /tmp/melonDS.toml /tmp/mdscfg8/melonDS/melonDS.toml
fi
export XDG_CONFIG_HOME=/tmp/mdscfg8
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :94 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:94 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run8.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 48
# serie rapida: PRE, poi A, poi 0.3s x 20
import -window root "$OUT/S00.png" 2>/dev/null; echo S00
xdotool key --clearmodifiers A; echo A_AT_$(date +%s)
k=1
while [ $k -le 20 ]; do
  sleep 0.3
  printf -v n "%02d" $k
  import -window root "$OUT/S$n.png" 2>/dev/null
  echo "S$n"
  k=$((k+1))
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
echo "=== log resets ==="; grep -n -E 'Resetting JIT|unmapping|booting' /tmp/m_run8.log
