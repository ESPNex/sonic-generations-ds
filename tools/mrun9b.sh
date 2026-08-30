#!/bin/bash
# mrun9b.sh — stesso di mrun9 ma con Renderer=1 (GL) + Threaded=false:
# il loop nero è artefatto del renderer soft threadato o del gioco?
set -u
ROM="$1"; OUT="${2:-/tmp/shots9b}"
mkdir -p "$OUT" /tmp/mdscfg9b/melonDS
SRC="$(dirname "$0")/melonDS.toml"
if [ -f "$SRC" ]; then cp "$SRC" /tmp/mdscfg9b/melonDS/melonDS.toml; fi
# override: renderer GL, non threadato
python3 - << 'PY'
p="/tmp/mdscfg9b/melonDS/melonDS.toml"
s=open(p).read()
import re
s=re.sub(r'\[3D\]\nRenderer = \d+', '[3D]\nRenderer = 1', s)
s=re.sub(r'\[3D\.Soft\]\nThreaded = \w+', '[3D.Soft]\nThreaded = false', s)
open(p,"w").write(s)
PY
export XDG_CONFIG_HOME=/tmp/mdscfg9b
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :92 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:92 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$ROM" >/tmp/m_run9b.log 2>&1 &
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 48
k=0
while [ $k -lt 40 ]; do
  printf -v n "%02d" $k
  import -window root "$OUT/G$n.png" 2>/dev/null
  k=$((k+1))
  sleep 0.5
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox; pkill Xvfb
grep -n -E 'booting' /tmp/m_run9b.log
