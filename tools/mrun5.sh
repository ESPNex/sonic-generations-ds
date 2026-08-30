#!/bin/bash
# mrun5.sh — melonDS DSi headless: capture GAMEPLAY per la verifica CV.
# Dismiss dei dialoghi per i primi ~6s, poi 8 shot ravvicinati (60..95s)
# per il frame-diff (prova di animazione: player idle/run + bat hover).
# Uso: mrun5.sh <rom.nds> [outdir=/tmp/shots]
set -u
ROM="$1"
OUT="${2:-/tmp/shots}"
mkdir -p "$OUT" /tmp/mdscfg5/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg5/melonDS/melonDS.toml
elif [ -f /tmp/melonDS.toml ]; then cp /tmp/melonDS.toml /tmp/mdscfg5/melonDS/melonDS.toml
fi
# DLDI attivo per caricamento veloce dai file (SD simulata)
export XDG_CONFIG_HOME=/tmp/mdscfg5
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :97 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:97 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run5.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
# shot ravvicinati della scena (attesa load ~50-60s card bus / ~20s DLDI)
for t in 55 60 65 70 75 80 85 90 95; do
  sleep 5
  import -window root "$OUT/G$t.png" 2>/dev/null
  echo "shot $t"
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
ls -la "$OUT"/G*.png 2>/dev/null
