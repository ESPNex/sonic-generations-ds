#!/usr/bin/env python3
"""gemini_vision.py — analisi computer-vision dei frame melonDS via Gemini API.
Uso:
  gemini_vision.py "prompt" img1.png [img2.png ...]
  gemini_vision.py --batch "prompt" img1.png img2.png ...   (un invio per immagine)
Legge la chiave da creds/gemini.key (o env GEMINI_KEY).
Modello di default: gemini-3.7-flash (richiesto dall'utente); override con GEMINI_MODEL.
"""
import base64, json, os, sys, urllib.request, mimetypes

def _find_key():
    cands = [os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "creds", "gemini.key"),
             "/home/user/creds/gemini.key", "creds/gemini.key"]
    for c in cands:
        if os.path.exists(c):
            return open(c).read().strip()
    return ""

KEY = os.environ.get("GEMINI_KEY") or _find_key()
MODEL = os.environ.get("GEMINI_MODEL", "gemini-3.7-flash")
URL = f"https://generativelanguage.googleapis.com/v1beta/models/{MODEL}:generateContent?key={KEY}"

def encode(path):
    mime = mimetypes.guess_type(path)[0] or "image/png"
    return {"mime_type": mime, "data": base64.b64encode(open(path, "rb").read()).decode()}

def ask(prompt, images, max_tokens=4096):
    parts = [{"text": prompt}] + [{"inline_data": encode(p)} for p in images]
    body = {"contents": [{"parts": parts}],
            "generationConfig": {"maxOutputTokens": max_tokens, "temperature": 0.2}}
    req = urllib.request.Request(URL, data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=180) as r:
            d = json.load(r)
        return d["candidates"][0]["content"]["parts"][0]["text"]
    except urllib.error.HTTPError as e:
        return f"HTTP {e.code}: {e.read().decode()[:500]}"

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    if sys.argv[1] == "--batch":
        prompt = sys.argv[2]
        for p in sys.argv[3:]:
            print(f"===== {p} =====")
            print(ask(prompt, [p]))
            print()
    else:
        prompt = sys.argv[1]
        imgs = sys.argv[2:]
        print(ask(prompt, imgs))

if __name__ == "__main__":
    main()
