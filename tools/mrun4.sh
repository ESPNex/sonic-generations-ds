#!/bin/bash
# mrun4.sh — melonDS DSi headless: dismiss solo ai primi 6s, shot a +20/40/60/90s
# (il load via card bus richiede ~50s in emulazione: la scena appare dallo shot +60s)
mkdir -p /tmp/shots /tmp/mdscfg/melonDS
cp "$(dirname "$0")/melonDS.toml" /tmp/mdscfg/melonDS/melonDS.toml 2>/dev/null || cp /tmp/melonDS.toml /tmp/mdscfg/melonDS/melonDS.toml
export XDG_CONFIG_HOME=/tmp/mdscfg
pkill -f 'melonDS/build' 2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :99 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:99 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
/tmp/melonDS/build/melonDS "$1" >/tmp/m_run.log 2>&1 &
for i in 1 2 3 4; do sleep 1.5; xdotool key Return >/dev/null 2>&1; xdotool mousemove 400 310 click 1 >/dev/null 2>&1; done
for s in 20 40 60 90; do
  sleep $s
  import -window root /tmp/shots/L$s.png 2>/dev/null
done
pkill -f 'melonDS/build'; sleep 1; pkill openbox
