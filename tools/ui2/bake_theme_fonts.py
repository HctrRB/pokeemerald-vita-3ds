"""Bakes the fonts of the menu themes into tools/ui2/fonts/theme_fonts.png + .json.

The glyphs are rasterised by design/stitch/_tools/theme_font_bake.html in a headless browser (the
design server must be running: python -m http.server 8765 --directory design/stitch). Each theme
face is baked for the four type roles of the UI (16 / 24 = Press Start 2P titles, 32 / 40 = VT323
text and dialogue); the role is stored as "px", so the runtime finds a theme font with the same
size key the screens already use. Ascent and line height are those of the pixel font of the same
role, with the baseline moved so the capitals sit on the same centre line. Then run
tools/ui2/build_atlas.py.

    python tools/ui2/bake_theme_fonts.py [path-to-chromium-browser]
"""
import html
import json
import os
import subprocess
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, "fonts")
URL = "http://localhost:8765/_tools/theme_font_bake.html"
BROWSERS = [r"C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe",
            r"C:\Program Files\Google\Chrome\Application\chrome.exe",
            r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"]
# Pixel font of each role in ui_fonts.json: (face, px)
ROLE_REF = {16: (0, 16), 24: (0, 24), 32: (1, 32), 40: (1, 40)}


def main():
    browser = sys.argv[1] if len(sys.argv) > 1 else next((b for b in BROWSERS if os.path.exists(b)), None)
    if browser is None:
        raise SystemExit("no Chromium browser found: pass its path")
    dom = subprocess.run([browser, "--headless=new", "--disable-gpu", "--virtual-time-budget=15000", "--dump-dom", URL],
                         capture_output=True, text=True, encoding="utf-8").stdout
    data = json.loads(html.unescape(dom.split('<pre id="out">')[-1].split("</pre>")[0]))

    ref = json.load(open(os.path.join(FONTS, "ui_fonts.json"), encoding="utf-8"))
    ref_fonts = {(f["face"], f["px"]): f for f in ref["fonts"]}

    def cap_height(glyphs, rows_of):
        for g in glyphs:
            if g[0] == 72:
                return rows_of(g)
        return 0

    W = 1024
    sheet = Image.new("RGBA", (W, 2048), (0, 0, 0, 0))
    cx = cy = row_h = 0
    fonts = []
    for f in data:
        rf = ref_fonts[ROLE_REF[f["px"]]]
        ref_cap = cap_height(rf["glyphs"], lambda g: g[4])
        cap = next(len(g["rows"]) for g in f["glyphs"] if g["cp"] == 72)
        ascent = rf["ascent"] - (ref_cap - cap) // 2
        glyphs = []
        for g in f["glyphs"]:
            h = len(g["rows"])
            w = len(g["rows"][0]) if h else 0
            if cx + w + 1 > W:
                cx, cy, row_h = 0, cy + row_h + 1, 0
            for j, row in enumerate(g["rows"]):
                for i, bit in enumerate(row):
                    if bit == "1":
                        sheet.putpixel((cx + i, cy + j), (255, 255, 255, 255))
            glyphs.append([g["cp"], cx, cy, w, h, g["xoff"], g["yoff"], g["adv"]])
            cx += w + 1
            row_h = max(row_h, h)
        fonts.append({"face": f["face"], "px": f["px"], "size": f["size"], "ascent": ascent, "lineH": rf["lineH"], "glyphs": glyphs})
    used = cy + row_h + 1
    sheet.crop((0, 0, W, used)).save(os.path.join(FONTS, "theme_fonts.png"))
    json.dump({"w": W, "h": used, "fonts": fonts}, open(os.path.join(FONTS, "theme_fonts.json"), "w", encoding="utf-8"))
    print(f"theme_fonts.png: {W}x{used}, {len(fonts)} fonts")


if __name__ == "__main__":
    main()
