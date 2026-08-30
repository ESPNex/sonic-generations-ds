#!/bin/bash
# mrun2.sh — melonDS DSi headless, dismiss robusto dei dialoghi secure-area
mkdir -p /tmp/shots /tmp/mdscfg
mkdir -p /tmp/mdscfg/melonDS && cp /tmp/melonDS.toml /tmp/mdscfg/melonDS/melonDS.toml
export XDG_CONFIG_HOME=/tmp/mdscfg
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :99 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:99 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$1" >/tmp/m_run.log 2>&1 &
for i in $(seq 1 14); do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null
  xdotool key --clearmodifiers space 2>/dev/null
  xdotool key --clearmodifiers Escape 2>/dev/null
  # click sul bottone del dialogo (centrato ~400,310)
  xdotool mousemove 400 310 click 1 2>/dev/null
done
sleep 3
import -window root /tmp/shots/t.png 2>/dev/null
pkill -f 'melonDS/build'; sleep 1; pkill openbox
