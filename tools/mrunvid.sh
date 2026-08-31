#!/bin/bash
# Registrazione VIDEO della build su melonDS headless (Xvfb + ffmpeg statico).
# Regola del progetto: la verifica e' SEMPRE un video, mai screenshot.
# Uso: mrunvid.sh ROM.nds OUT.mp4 [SECONDI]
ROM="$1"; OUT="$2"; SECS="${3:-30}"
FFMPEG=$(python3 -c 'import imageio_ffmpeg; print(imageio_ffmpeg.get_ffmpeg_exe())')
pkill -f melonDS/build  2>/dev/null; sleep 1
pkill Xvfb 2>/dev/null; pkill openbox 2>/dev/null; sleep 1
Xvfb :95 +extension GLX +render -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 2
export DISPLAY=:95 QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
openbox >/dev/null 2>&1 &
sleep 2
# NOTA: usa il profilo melonDS di default (~/.config/melonDS) con BIOS/firmware già configurati
/home/lardx/melonDS/build/melonDS "$ROM" >/tmp/m_runvid.log 2>&1 &
sleep 4
for i in 1 2 3 4 5 6 7 8; do
  sleep 1.5
  xdotool key --clearmodifiers Return 2>/dev/null   # DS START: salta loghi, entra nei menu
  xdotool mousemove 400 310 click 1 2>/dev/null     # prova anche touch centrale
done
sleep 22
"$FFMPEG" -y -f x11grab -video_size 800x600 -framerate 15 -i :95 \
  -c:v libx264 -pix_fmt yuv420p -crf 23 -preset fast -t "$SECS" "$OUT" 2>/tmp/ffmpeg_vid.log
pkill -f melonDS/build; sleep 1; pkill openbox; pkill Xvfb
ls -la "$OUT"
# controllo interno (NON e' verifica visiva): luminanza media di 3 frame per scartare video neri
"$FFMPEG" -i "$OUT" -vf "select='eq(n\,60)+eq(n\,150)',signalstats,metadata=mode=print" -f null - 2>&1 | grep -o 'y average:[0-9.]*' || true
