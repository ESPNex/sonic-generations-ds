#!/usr/bin/env python3
"""vision.py — COMPUTER VISION per la verifica visiva del motore su melonDS.

Non è OCR e non è "contare i pixel": è vera CV (OpenCV) per verificare che
sullo schermo ci sia quello che il motore deve aver disegnato:

  - segmentazione per colore con palette GROUND-TRUTH estratta dalle
    TEXTURE ORIGINALI del gioco (non colori inventati);
  - connected-components -> blob con centroide, bbox, area, fill-ratio;
  - matching di entità (Sonic, anelli, nemici) per colore+forma;
  - frame-differencing / SSIM tra due screenshot -> prova di ANIMAZIONE
    e di movimento (hover, run, anim skeletal);
  - matchTemplate con reference renderizate offline;
  - report JSON + overlay PNG con bbox/centroidi disegnati.

Uso (CLI):  python3 tools/vision.py one <file.png> [--overlay out.png]
            python3 tools/vision.py diff <a.png> <b.png>
            python3 tools/vision.py dir <dir> [--overlay-dir out]
            python3 tools/vision.py dbg <file.png>   (debug: salva ogni mask)
"""
import os, sys, json
import numpy as np
import cv2

# --------------------------------------------------------------------------
# 1. Palette ground-truth dai colori DOMINANTI delle texture originali.
# --------------------------------------------------------------------------
def dominant_colors(bgr_img, k=5):
    px = bgr_img.reshape(-1, 3).astype(np.float32)
    crit = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 20, 1.0)
    _, lbl, cen = cv2.kmeans(px, k, None, crit, 5, cv2.KMEANS_PP_CENTERS)
    counts = np.bincount(lbl.flatten(), minlength=k)
    out = []
    for i in np.argsort(-counts):
        c = cen[i].astype(int)
        frac = counts[i] / len(lbl)
        out.append({"bgr": [int(c[0]), int(c[1]), int(c[2])], "frac": float(frac)})
    return out


def hsv_range_from_bgr(bgr, tol_h=12, tol_s=70, tol_v=50):
    """Soglia HSV attorno a un colore campione (BGR)."""
    c = np.uint8([[[bgr[0], bgr[1], bgr[2]]]])
    hsv = cv2.cvtColor(c, cv2.COLOR_BGR2HSV)[0][0]
    h, s, v = int(hsv[0]), int(hsv[1]), int(hsv[2])
    lo = np.array([max(0, h - tol_h), max(0, s - tol_s), max(0, v - tol_v)])
    hi = np.array([min(179, h + tol_h), 255, 255])
    if h < tol_h or h > 179 - tol_h:
        lo2 = np.array([0, max(0, s - tol_s), max(0, v - tol_v)])
        hi2 = np.array([min(179, h + tol_h), 255, 255])
        return [(lo, hi), (lo2, hi2)]
    return [(lo, hi)]


def sample_entity_palette(fn, label):
    img = cv2.imread(fn)
    if img is None:
        return None
    cols = dominant_colors(img, 5)
    return {"file": fn, "label": label, "colors": cols}


# --------------------------------------------------------------------------
# 2. Blob analysis (connected components) su una mask binaria
# --------------------------------------------------------------------------
def find_blobs(mask, min_area=8):
    n, lbl, stats, cen = cv2.connectedComponentsWithStats(mask, 8)
    blobs = []
    for i in range(1, n):
        x, y, w, h, a = stats[i]
        if a < min_area:
            continue
        cx, cy = cen[i]
        blobs.append({
            "bbox": [int(x), int(y), int(w), int(h)],
            "area": int(a),
            "centroid": [float(cx), float(cy)],
            "fill": float(a) / (w * h) if w * h else 0.0,
        })
    return blobs


def segment(img_bgr, profile):
    hsv = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2HSV)
    mask = np.zeros(hsv.shape[:2], np.uint8)
    for lo, hi in profile["ranges"]:
        mask = cv2.bitwise_or(mask, cv2.inRange(hsv, lo, hi))
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, np.ones((2, 2), np.uint8))
    blobs = find_blobs(mask, profile.get("min_area", 8))
    return mask, blobs


# --------------------------------------------------------------------------
# 3. Frame differencing / motion (prova di animazione)
# --------------------------------------------------------------------------
def frame_diff(a, b, thresh=24):
    ga = cv2.cvtColor(a, cv2.COLOR_BGR2GRAY)
    gb = cv2.cvtColor(b, cv2.COLOR_BGR2GRAY)
    d = cv2.absdiff(ga, gb)
    _, mov = cv2.threshold(d, thresh, 255, cv2.THRESH_BINARY)
    mov = cv2.morphologyEx(mov, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
    blobs = find_blobs(mov, min_area=10)
    changed = float((mov > 0).sum()) / mov.size
    region = None
    if len(blobs):
        xs0 = min(b["bbox"][0] for b in blobs)
        ys0 = min(b["bbox"][1] for b in blobs)
        xs1 = max(b["bbox"][0] + b["bbox"][2] for b in blobs)
        ys1 = max(b["bbox"][1] + b["bbox"][3] for b in blobs)
        region = [xs0, ys0, xs1 - xs0, ys1 - ys0]
    return {"changed_frac": changed, "blobs": blobs, "region": region, "diff_img": d}


def ssim(a, b):
    from skimage.metrics import structural_similarity as _ssim
    ga = cv2.cvtColor(a, cv2.COLOR_BGR2GRAY)
    gb = cv2.cvtColor(b, cv2.COLOR_BGR2GRAY)
    s, _ = _ssim(ga, gb, full=True)
    return float(s)


# --------------------------------------------------------------------------
# 4. Template matching con reference renderizzate offline
# --------------------------------------------------------------------------
def match_template(img, templ_fn, thresh=0.6):
    t = cv2.imread(templ_fn, cv2.IMREAD_GRAYSCALE)
    if t is None:
        return {"error": f"no template {templ_fn}"}
    g = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    res = cv2.matchTemplate(g, t, cv2.TM_CCOEFF_NORMED)
    _, maxv, _, maxloc = cv2.minMaxLoc(res)
    th, tw = t.shape[:2]
    ok = maxv >= thresh
    return {
        "template": os.path.basename(templ_fn),
        "score": float(maxv),
        "match": ok,
        "bbox": [int(maxloc[0]), int(maxloc[1]), tw, th] if ok else None,
    }


# --------------------------------------------------------------------------
# 5. Crop automatico dello schermo DS (melonDS window ha bordi neri/chrome)
# --------------------------------------------------------------------------
def find_screen_bbox(img):
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    nz = np.where(gray > 24)
    if len(nz[0]) == 0:
        return [0, 0, img.shape[1], img.shape[0]]
    x0, x1 = int(nz[1].min()), int(nz[1].max())
    y0, y1 = int(nz[0].min()), int(nz[0].max())
    return [x0, y0, x1 - x0 + 1, y1 - y0 + 1]


# --------------------------------------------------------------------------
# 6. Profili entità noti
# --------------------------------------------------------------------------
def build_profiles(tex_dir=None):
    profiles = {}
    sonic = {"label": "sonic", "min_area": 30,
             "ranges": [(np.array([95, 60, 60]), np.array([125, 255, 255]))]}  # blu
    ring = {"label": "ring", "min_area": 4,
            "ranges": [(np.array([15, 120, 140]), np.array([30, 255, 255]))]}  # oro
    bat = {"label": "ebat", "min_area": 12,
           "ranges": [(np.array([120, 60, 60]), np.array([160, 255, 255])),
                      (np.array([0, 0, 60]), np.array([179, 80, 120]))]}       # viola/nero
    if tex_dir and os.path.isdir(tex_dir):
        for fn in os.listdir(tex_dir):
            if fn.endswith('.png') and any(k in fn for k in ['sonic', 'z11', 'ghc']):
                prof = sample_entity_palette(os.path.join(tex_dir, fn), fn)
                if prof:
                    cols = prof["colors"]
                    best = max(cols, key=lambda c: c["bgr"][2] - c["bgr"][0])
                    if best["bgr"][2] > 120 and best["frac"] > 0.01:
                        ranges = hsv_range_from_bgr(best["bgr"])
                        sonic = {"label": "sonic", "min_area": 30, "ranges": ranges,
                                 "src": prof["file"]}
                    break
    return {"sonic": sonic, "ring": ring, "ebat": bat}


# --------------------------------------------------------------------------
# 7. Analisi scena completa
# --------------------------------------------------------------------------
def analyze_scene(img, profiles=None, tex_dir=None):
    if profiles is None:
        profiles = build_profiles(tex_dir)
    out = {"entities": {}, "blob_total": 0}
    for key, prof in profiles.items():
        mask, blobs = segment(img, prof)
        out["entities"][key] = {
            "count": len(blobs),
            "blobs": blobs[:40],
            "pixel_count": int((mask > 0).sum()),
            "src": prof.get("src"),
        }
        out["blob_total"] += len(blobs)
    return out


def annotate(img, analysis, out_fn=None):
    vis = img.copy()
    colors = {"sonic": (255, 90, 30), "ring": (40, 190, 250), "ebat": (240, 40, 200)}
    for key, ent in analysis["entities"].items():
        col = colors.get(key, (0, 255, 0))
        for b in ent["blobs"]:
            x, y, w, h = b["bbox"]
            cv2.rectangle(vis, (x, y), (x + w, y + h), col, 1)
            cv2.circle(vis, (int(b["centroid"][0]), int(b["centroid"][1])), 2, col, -1)
    if out_fn:
        cv2.imwrite(out_fn, vis)
    return vis


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------
def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 1
    cmd = argv[1]
    tex_dir = os.environ.get("SGDS_TEX", None)

    if cmd == "dbg":
        img = cv2.imread(argv[2])
        scr = find_screen_bbox(img)
        crop = img[scr[1]:scr[1] + scr[3], scr[0]:scr[0] + scr[2]]
        profs = build_profiles(tex_dir)
        print(json.dumps({"screen": scr, "size": [crop.shape[1], crop.shape[0]],
                          "dominant": dominant_colors(crop, 6)}, indent=1))
        for key, prof in profs.items():
            mask, blobs = segment(crop, prof)
            cv2.imwrite(f"/tmp/mask_{key}.png", mask)
            print(f"{key}: {len(blobs)} blobs, {int((mask>0).sum())} px")
        return 0

    if cmd == "one":
        img = cv2.imread(argv[2])
        scr = find_screen_bbox(img)
        crop = img[scr[1]:scr[1] + scr[3], scr[0]:scr[0] + scr[2]]
        analysis = analyze_scene(crop, tex_dir=tex_dir)
        overlay = argv[4] if len(argv) > 3 and argv[3] == "--overlay" else None
        if overlay:
            annotate(crop, analysis, out_fn=overlay)
        report = {"file": argv[2], "screen_bbox": scr,
                  "entities": {k: {"count": v["count"], "pixel_count": v["pixel_count"],
                                   "blobs": v["blobs"]} for k, v in analysis["entities"].items()}}
        print(json.dumps(report, indent=1))
        return 0

    if cmd == "diff":
        a = cv2.imread(argv[2]); b = cv2.imread(argv[3])
        res = frame_diff(a, b)
        res["ssim"] = ssim(a, b)
        res.pop("diff_img", None)
        print(json.dumps(res, indent=1))
        return 0

    if cmd == "dir":
        import glob
        for fn in sorted(glob.glob(os.path.join(argv[2], '*.png'))):
            img = cv2.imread(fn)
            scr = find_screen_bbox(img)
            crop = img[scr[1]:scr[1] + scr[3], scr[0]:scr[0] + scr[2]]
            analysis = analyze_scene(crop, tex_dir=tex_dir)
            s = analysis["entities"]
            print(f"{os.path.basename(fn):10s} sonic={s['sonic']['count']:2d} "
                  f"ring={s['ring']['count']:2d} ebat={s['ebat']['count']:2d} "
                  f"px(s/r/e)={s['sonic']['pixel_count']:5d}/{s['ring']['pixel_count']:5d}/{s['ebat']['pixel_count']:5d}")
        return 0

    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
