"""Bakes the extra letters of every language into tools/ui2/fonts/locale_glyphs.png + .json.

The letters are those listed in tools/locales/languages.json. They are rasterised from the real
fonts, by the same pages and with the same rules as the base glyphs, in a headless browser:
design/stitch/_tools/font_bake.html for the pixel fonts (1-bit, alpha >= 50 %) and
theme_font_bake.html for the menu themes' fonts (8-bit alpha). Both pages need the internet (Google
Fonts). A letter a face lacks is reported and left out: the runtime then draws it without its
accent (ui_assets.c, GlyphOf).

The base sheets (ui_fonts.*, theme_fonts.*) are not touched, so the glyphs every build already has
never change; tools/ui2/build_atlas.py --locale <code> adds only that language's letters to its
atlas. Run again after adding letters to languages.json, then commit both output files:

    python tools/ui2/bake_locale_glyphs.py [path-to-chromium-browser]
"""
import html
import json
import os
import pathlib
import subprocess
import sys
import urllib.parse

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
FONTS = os.path.join(HERE, "fonts")
LANGUAGES = os.path.join(ROOT, "tools", "locales", "languages.json")
PAGES = os.path.join(ROOT, "design", "stitch", "_tools")
OUT_PNG = os.path.join(FONTS, "locale_glyphs.png")
OUT_JSON = os.path.join(FONTS, "locale_glyphs.json")
BROWSERS = [r"C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe",
            r"C:\Program Files\Google\Chrome\Application\chrome.exe",
            r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"]
SHEET_W = 1024


def all_letters():
    langs = json.load(open(LANGUAGES, encoding="utf-8"))["languages"]
    return "".join(sorted({c for lang in langs for c in lang["letters"]}))


def bake(browser, page, letters):
    url = pathlib.Path(os.path.join(PAGES, page)).resolve().as_uri() + "?chars=" + urllib.parse.quote(letters)
    dom = subprocess.run([browser, "--headless=new", "--disable-gpu", "--virtual-time-budget=20000", "--dump-dom", url],
                         capture_output=True, text=True, encoding="utf-8").stdout
    if '<pre id="out">' not in dom:
        raise SystemExit(f"{page}: no output (no internet for Google Fonts, or the page failed)")
    return json.loads(html.unescape(dom.split('<pre id="out">')[-1].split("</pre>")[0]))


def main():
    browser = sys.argv[1] if len(sys.argv) > 1 else next((b for b in BROWSERS if os.path.exists(b)), None)
    if browser is None:
        raise SystemExit("no Chromium browser found: pass its path")
    letters = all_letters()
    # Rows of '0'/'1' for the pixel fonts, two hex digits of alpha per pixel for the theme fonts
    baked = [(f, lambda r: [255 if ch == "1" else 0 for ch in r]) for f in bake(browser, "font_bake.html", letters)]
    baked += [(f, lambda r: list(bytes.fromhex(r))) for f in bake(browser, "theme_font_bake.html", letters)]

    sheet = Image.new("RGBA", (SHEET_W, 4096), (0, 0, 0, 0))
    cx = cy = row_h = 0
    fonts, missing = [], {}
    for f, alpha_of in baked:
        glyphs = []
        for g in f["glyphs"]:
            if g.get("missing"):
                missing.setdefault(chr(g["cp"]), []).append(f"{f['face']}/{f['px']}")
                continue
            rows = [alpha_of(r) for r in g["rows"]]
            h = len(rows)
            w = len(rows[0]) if h else 0
            if cx + w + 1 > SHEET_W:
                cx, cy, row_h = 0, cy + row_h + 1, 0
            for j, row in enumerate(rows):
                for i, a in enumerate(row):
                    if a:
                        sheet.putpixel((cx + i, cy + j), (255, 255, 255, a))
            glyphs.append([g["cp"], cx, cy, w, h, g["xoff"], g["yoff"], g["adv"]])
            cx += w + 1
            row_h = max(row_h, h)
        fonts.append({"face": f["face"], "px": f["px"], "glyphs": glyphs})
    used = cy + row_h + 1
    sheet.crop((0, 0, SHEET_W, used)).save(OUT_PNG)
    with open(OUT_JSON, "w", encoding="utf-8", newline="\n") as fh:
        json.dump({"letters": letters, "w": SHEET_W, "h": used, "fonts": fonts}, fh, ensure_ascii=False)
    print(f"{OUT_PNG}: {SHEET_W}x{used}, {len(fonts)} fonts, {len(letters)} letters")
    for ch, where in sorted(missing.items()):
        print(f"  missing {ch!r} (U+{ord(ch):04X}) in face/px {', '.join(where)}: drawn without its accent")


if __name__ == "__main__":
    main()
