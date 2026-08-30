#!/bin/bash
# gitpush.sh — commit+push robusti. Il .git NON persiste tra i turni
# (escluso dallo snapshot del workspace): se manca, lo ricrea allineandosi
# a origin/main PRIMA di qualsiasi add (gli untracked sopravvivono).
# Uso:  gitpush.sh [-m "msg"] <files...>
set -u
cd "$(dirname "$0")/.."
TOKEN="$(cat /home/user/creds/gh.token 2>/dev/null || echo '')"
if [ -z "$TOKEN" ]; then echo "no token in /home/user/creds/gh.token"; exit 1; fi
URL="https://x-access-token:${TOKEN}@github.com/ESPNex/sonic-generations-ds.git"
MSG="update"
FILES=()
while [ $# -gt 0 ]; do
  case "$1" in
    -m) MSG="$2"; shift 2 ;;
    *) FILES+=("$1"); shift ;;
  esac
done
if [ ! -d .git ]; then
  echo "[gitpush] .git mancante: reinizializzo..."
  git init -q
  git remote add origin "$URL"
  git fetch -q origin || git fetch -q "$URL"
  git reset --hard origin/main -q
  git branch -M main 2>/dev/null
fi
git remote set-url origin "$URL" 2>/dev/null || git remote add origin "$URL"
git add "${FILES[@]}"
git -c user.name=lardx -c user.email=lardx@local commit -q -m "$MSG"
git push -q origin main 2>&1 | tail -1
git log --oneline -1
