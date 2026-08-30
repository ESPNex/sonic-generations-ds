#!/bin/bash
# mrun7.sh — riproduzione crash su JUMP: boot, dismiss, scena (~60s), poi A
# con shot ogni 1s per catturare il momento dell'eccezione (defaultExceptionHandler
# disegna schermata con indirizzo su MAIN). Uso: mrun7.sh <rom.nds> [outdir]
set -u
ROM="$1"; OUT="${2:-/tmp/shots7}"
mkdir -p "$OUT" /tmp/mdscfg7/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg7/melonDS/melonDS.toml
elif [ -f /tmp/melonDS.toml ]; then cp /tmp/melonDS.toml /tmp/mdscfg7/melonDS/melonDS.toml
fi
export XDG_CONFIG_HOME=/tmp/mdscfg7
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :95 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:95 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run7.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 45          # attesa load (card bus ~50s)
import -window root "$OUT/PRE.png" 2>/dev/null; echo PRE
sleep 3
xdotool key --clearmodifiers A   # JUMP
echo "A pressed"
for s in 1 2 3 4 5 6 7 8 9 10; do
  sleep 1
  import -window root "$OUT/J$s.png" 2>/dev/null; echo "J$s"
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
ls -la "$OUT"/*.png 2>/dev/null | tail -12
