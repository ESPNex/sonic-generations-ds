#!/bin/bash
# mrun6.sh — melonDS DSi headless: GAMEPLAY CON INPUT per la verifica CV.
# Boot, dismiss dialoghi, poi a t>=60s tiene premuto RIGHT (Sonic corre) e
# preme A (salto); cattura frame ravvicinati per frame-diff (motion di
# camera + animazione skeletal). Uso: mrun6.sh <rom.nds> [outdir=/tmp/shots6]
set -u
ROM="$1"
OUT="${2:-/tmp/shots6}"
mkdir -p "$OUT" /tmp/mdscfg6/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg6/melonDS/melonDS.toml
elif [ -f /tmp/melonDS.toml ]; then cp /tmp/melonDS.toml /tmp/mdscfg6/melonDS/melonDS.toml
fi
export XDG_CONFIG_HOME=/tmp/mdscfg6
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :96 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:96 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run6.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
# attesa load (card bus ~50s)
sleep 40
# frame di base
import -window root "$OUT/M60.png" 2>/dev/null; echo "M60"
sleep 3
# --- input: corri (RIGHT per 2s), poi salta (A) ---
xdotool keydown Right; sleep 2; xdotool keyup Right
import -window root "$OUT/M65.png" 2>/dev/null; echo "M65(ran)"
xdotool keydown Right; xdotool keydown A; sleep 1; xdotool keyup A; xdotool keyup Right
import -window root "$OUT/M66.png" 2>/dev/null; echo "M66(jump)"
sleep 2
xdotool keydown Right; sleep 1; xdotool keyup Right
import -window root "$OUT/M69.png" 2>/dev/null; echo "M69"
sleep 2
import -window root "$OUT/M71.png" 2>/dev/null; echo "M71"
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
ls -la "$OUT"/M*.png 2>/dev/null
