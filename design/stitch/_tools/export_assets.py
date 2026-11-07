"""Export the original GBA sprites used by the design mocks into design/stitch/_assets/.

Run from the repository root:  python design/stitch/_tools/export_assets.py

Every sprite keeps its original pixel size (the mocks scale them by an integer factor with
`image-rendering: pixelated`). Colour index 0 becomes transparent, as on the GBA. Palettes come
from the same files the game uses (item icon palettes, the three Pokemon icon palettes,
normal.pal for front pictures), not from the template palette stored inside each PNG.

It also composes the Hoenn region map exactly as src/platform/vita/vita_ui_map.c does
(map.8bpp + map.bin + map.gbapal, 28x15 cells of 8 px) and writes the zone grid of
src/data/region_map/region_map_layout.h with the zone rectangles and names of
region_map_sections.json to hoenn_zones.json.
"""
import json
import os
import re
import sys

from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(ROOT, "design", "stitch", "_assets")
G = os.path.join(ROOT, "graphics")


def read_jasc(path):
    with open(path, encoding="ascii") as f:
        lines = f.read().split()
    count = int(lines[2])
    vals = [int(v) for v in lines[3:3 + count * 3]]
    return [tuple(vals[i:i + 3]) for i in range(0, len(vals), 3)]


def read_gbapal(path):
    data = open(path, "rb").read()
    out = []
    for i in range(0, len(data), 2):
        c = data[i] | (data[i + 1] << 8)
        out.append(((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31))
    return out


def to_rgba(png_path, palette=None, transparent0=True, crop=None):
    im = Image.open(png_path)
    if im.mode != "P":
        return im.convert("RGBA")
    pal = palette
    if pal is None:
        raw = im.getpalette()
        pal = [tuple(raw[i:i + 3]) for i in range(0, len(raw), 3)]
    w, h = im.size
    src = im.load()
    out = Image.new("RGBA", (w, h))
    dst = out.load()
    for y in range(h):
        for x in range(w):
            v = src[x, y]
            if transparent0 and v == 0:
                dst[x, y] = (0, 0, 0, 0)
            else:
                r, g, b = pal[v % len(pal)]
                dst[x, y] = (r, g, b, 255)
    if crop:
        out = out.crop(crop)
    return out


def save(img, rel):
    path = os.path.join(OUT, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path)


ITEMS = ["POTION", "SUPER_POTION", "HYPER_POTION", "ANTIDOTE", "PARALYZE_HEAL", "REVIVE",
         "ESCAPE_ROPE", "REPEL", "POKE_BALL", "GREAT_BALL", "ULTRA_BALL", "NET_BALL", "DIVE_BALL",
         "NUGGET", "ORAN_BERRY", "PECHA_BERRY", "RARE_CANDY", "MACH_BIKE", "POKEBLOCK_CASE",
         "TM01", "TM28", "TM39", "HM01", "OLD_ROD", "SOOTHE_BELL", "TIMER_BALL", "REPEAT_BALL",
         "NEST_BALL", "LUXURY_BALL", "PREMIER_BALL", "MASTER_BALL", "AWAKENING", "BURN_HEAL",
         "ICE_HEAL", "FULL_HEAL", "ETHER", "X_SPEED", "LETTER", "CARD_KEY"]
PARTY = ["mudkip", "torchic", "ralts", "taillow", "zigzagoon", "wurmple", "treecko", "grovyle",
         "sceptile", "combusken", "marshtomp", "poochyena", "seedot", "lotad", "wingull",
         "blaziken", "swampert", "mightyena", "linoone", "silcoon", "beautifly", "cascoon",
         "dustox", "lombre", "ludicolo", "nuzleaf", "shiftry", "whismur", "magikarp", "tentacool", "goldeen", "corphish", "marill", "surskit", "geodude", "nosepass", "zubat"]
TYPES = ["normal", "fight", "flying", "poison", "ground", "rock", "bug", "ghost", "steel",
         "mystery", "fire", "water", "grass", "electric", "psychic", "ice", "dragon", "dark",
         "contest_cool", "contest_beauty", "contest_cute", "contest_smart", "contest_tough"]
BALLS = ["poke", "great", "ultra", "master", "safari", "net", "dive", "nest", "repeat", "timer",
         "luxury", "premier"]


def icon_palette_indices():
    src = open(os.path.join(ROOT, "src", "pokemon_icon.c"), encoding="utf-8").read()
    body = src[src.index("gMonIconPaletteIndices[]"):]
    body = body[:body.index("};")]
    return {m.group(1).lower(): int(m.group(2))
            for m in re.finditer(r"\[SPECIES_(\w+)\]\s*=\s*(\d+)", body)}


def item_icon_files():
    """ITEM_X -> (icon png, palette pal), resolved through the game's own icon table."""
    table = open(os.path.join(ROOT, "src", "data", "item_icon_table.h"), encoding="utf-8").read()
    gfx = open(os.path.join(ROOT, "src", "data", "graphics", "items.h"), encoding="utf-8").read()
    paths = dict(re.findall(r"const u32 (\w+)\[\] = INCBIN_U32\(\"(graphics/items/[^\"]+)\"\)", gfx))
    out = {}
    for item, icon, pal in re.findall(r"\[ITEM_(\w+)\] = \{(\w+), (\w+)\}", table):
        if icon in paths and pal in paths:
            out[item] = (paths[icon].split(".")[0] + ".png", paths[pal].split(".")[0] + ".pal")
    return out


def export_sprites():
    files = item_icon_files()
    for name in ITEMS:
        if name not in files:
            print("missing item icon", name)
            continue
        png, pal = (os.path.join(ROOT, p) for p in files[name])
        save(to_rgba(png, read_jasc(pal) if os.path.exists(pal) else None), f"items/{name.lower()}.png")

    idx = icon_palette_indices()
    icon_pals = [read_jasc(os.path.join(G, "pokemon", "icon_palettes", f"icon_palette_{i}.pal"))
                 for i in range(3)]
    for mon in PARTY:
        base = os.path.join(G, "pokemon", mon)
        save(to_rgba(os.path.join(base, "icon.png"), icon_pals[idx.get(mon, 0)]),
             f"pokemon/{mon}_icon.png")
        save(to_rgba(os.path.join(base, "front.png"), read_jasc(os.path.join(base, "normal.pal"))),
             f"pokemon/{mon}_front.png")
        save(to_rgba(os.path.join(base, "footprint.png"), None, transparent0=False),
             f"pokemon/{mon}_footprint.png")
    egg = os.path.join(G, "pokemon", "egg")
    save(to_rgba(os.path.join(egg, "icon.png"), icon_pals[1]), "pokemon/egg_icon.png")
    save(to_rgba(os.path.join(egg, "front.png"), read_jasc(os.path.join(egg, "normal.pal")) if os.path.exists(os.path.join(egg, "normal.pal")) else None), "pokemon/egg_front.png")

    for t in TYPES:
        save(to_rgba(os.path.join(G, "types", t + ".png")), f"types/{t}.png")
    for b in BALLS:
        save(to_rgba(os.path.join(G, "balls", b + ".png"), crop=(0, 0, 16, 16)), f"balls/{b}.png")

    badges = to_rgba(os.path.join(G, "trainer_card", "badges.png"))
    for i in range(8):
        save(badges.crop((i * 16, 0, i * 16 + 16, 16)), f"badges/badge_{i + 1}.png")
    for metal in ("silver", "gold"):
        medals = to_rgba(os.path.join(G, "frontier_pass", "medals.png"),
                         read_jasc(os.path.join(G, "frontier_pass", metal + ".pal")))
        for i in range(medals.height // 16):
            save(medals.crop((0, i * 16, 16, i * 16 + 16)), f"medals/{metal}_{i + 1}.png")

    save(to_rgba(os.path.join(G, "interface", "status_icons.png")), "ui_gba/status_icons.png")
    save(to_rgba(os.path.join(G, "party_menu", "hold_icons.png")), "ui_gba/hold_icons.png")
    save(to_rgba(os.path.join(G, "interface", "mon_markings.png")), "ui_gba/mon_markings.png")
    save(to_rgba(os.path.join(G, "pokeblock", "pokeblock.png")), "ui_gba/pokeblock.png")
    # Ribbons as the PokéNav draws them (sRibbonGfxData, pokenav_ribbons_summary.c): a 32x32 frame of
    # icons_big.png and one of the five icons*.pal palettes, saved by ribbon id (constants/pokemon.h)
    ribbon_gfx = [(0, 1)] + [(1 + r, 1 + c) for c in range(5) for r in range(4)] + [
        (5, 1), (6, 1), (7, 2), (8, 3), (9, 2), (9, 4), (9, 5), (10, 4), (10, 5), (11, 1), (11, 2)]
    for rid, (frame, pal) in enumerate(ribbon_gfx):
        img = to_rgba(os.path.join(G, "pokenav", "ribbons", "icons_big.png"),
                      read_jasc(os.path.join(G, "pokenav", "ribbons", f"icons{pal}.pal")),
                      crop=(0, frame * 32, 32, frame * 32 + 32))
        save(img, f"ribbons/ribbon_{rid}.png")
    for g in ("male", "female"):
        save(to_rgba(os.path.join(G, "bag", f"bag_{g}.png")), f"bag/bag_{g}.png")
    for tr in ("brendan", "may"):
        save(to_rgba(os.path.join(G, "trainers", "front_pics", tr + ".png")), f"trainers/{tr}.png")
        save(to_rgba(os.path.join(G, "pokenav", "region_map", tr + "_icon.png")),
             f"map/{tr}_head.png")
    for leader in ("roxanne", "brawly", "wattson", "flannery"):
        save(to_rgba(os.path.join(G, "trainers", "front_pics", "leader_" + leader + ".png")), f"trainers/{leader}.png")
    # START menu icons (decision D11): original sprites reused, 32x32
    save(to_rgba(os.path.join(G, "pokenav", "nav_icon.png"), crop=(0, 0, 32, 32)), "start/pokenav.png")
    save(to_rgba(os.path.join(G, "bag", "bag_male.png"), crop=(0, 0, 64, 64)).resize((32, 32), Image.NEAREST), "start/bag.png")
    save(to_rgba(os.path.join(G, "pokenav", "region_map", "fly_target_icons.png")),
         "map/fly_target_icons.png")
    save(to_rgba(os.path.join(G, "pokenav", "region_map", "cursor_small.png")), "map/cursor_small.png")
    save(to_rgba(os.path.join(G, "pokenav", "region_map", "cursor_large.png")), "map/cursor_large.png")


def export_region_map():
    # Same composition as EnsureHoennMapDecoded (vita_ui_map.c), from tracked sources only:
    # map.png holds the 8bpp tiles (16 per row), map.bin the 64x64 byte tilemap, map.pal the
    # 32 colours used by pixel values 112..143.
    base = os.path.join(G, "pokenav", "region_map")
    sheet = Image.open(os.path.join(base, "map.png"))
    sw = sheet.size[0] // 8
    spx = sheet.load()
    tilemap = open(os.path.join(base, "map.bin"), "rb").read()
    pal = read_jasc(os.path.join(base, "map.pal"))
    img = Image.new("RGB", (224, 120))
    px = img.load()
    for y in range(120):
        sy = y + 16
        for x in range(224):
            sx = x + 8
            t = tilemap[(sy >> 3) * 64 + (sx >> 3)]
            tx, ty = (t % sw) * 8 + (sx & 7), (t // sw) * 8 + (sy & 7)
            v = spx[tx, ty] if ty < sheet.size[1] else 0
            px[x, y] = pal[v - 112] if 112 <= v < 144 else pal[0]
    save(img, "map/hoenn_224x120.png")

    layout = open(os.path.join(ROOT, "src", "data", "region_map", "region_map_layout.h"),
                  encoding="utf-8").read()
    rows = [re.findall(r"MAPSEC_\w+", row) for row in re.findall(r"\{(MAPSEC[^}]*)\}", layout)]
    sections = json.load(open(os.path.join(ROOT, "src", "data", "region_map",
                                           "region_map_sections.json"), encoding="utf-8"))
    zones = {s["id"]: {"name": s["name"], "x": s["x"], "y": s["y"],
                       "w": s["width"], "h": s["height"]} for s in sections["map_sections"] if "x" in s}
    with open(os.path.join(OUT, "map", "hoenn_zones.json"), "w", encoding="utf-8") as f:
        json.dump({"cell": 8, "width": 28, "height": 15, "grid": rows, "zones": zones}, f)
    print("grid", len(rows), "x", len(rows[0]), "zones", len(zones))


def export_battle_and_extras():
    """Assets of the later mocks (battle HUD, call, gender glyphs), all from tracked sources."""
    for sp in ("mudkip", "treecko", "torchic", "marshtomp", "ralts", "zigzagoon", "poochyena", "taillow"):
        d = os.path.join(G, "pokemon", sp)
        pal = read_jasc(os.path.join(d, "normal.pal"))
        save(to_rgba(os.path.join(d, "back.png"), pal), f"pokemon/{sp}_back.png")
    bi = os.path.join(G, "battle_interface")
    save(to_rgba(os.path.join(bi, "ball_display.png")), "ui_gba/ball_display.png")
    save(to_rgba(os.path.join(bi, "ball_status_bar.png")), "ui_gba/ball_status_bar.png")
    save(to_rgba(os.path.join(bi, "ball_caught_indicator.png")), "ui_gba/ball_caught.png")
    save(to_rgba(os.path.join(G, "pokenav", "ribbons", "icons.png"), read_jasc(os.path.join(G, "pokenav", "ribbons", "icons1.pal"))), "ui_gba/ribbons.png")
    # The POKENAV call icon: 8 frames of 32x32 in one strip
    icon = to_rgba(os.path.join(G, "pokenav", "match_call", "nav_icon.png"))
    strip = Image.new("RGBA", (32 * 8, 32))
    for f in range(8):
        strip.paste(icon.crop((0, f * 32, 32, f * 32 + 32)), (f * 32, 0))
    save(strip, "ui_gba/phone_strip.png")
    # Gender / shiny pixel art of the new UI (tools/ui2/build_atlas.py GLYPH_ART, our own art)
    src = open(os.path.join(ROOT, "tools", "ui2", "build_atlas.py"), encoding="utf-8").read()
    i = src.index("GLYPH_ART = [")
    j = src.index(chr(10) + "]" + chr(10), i) + 3
    ns = {}
    exec(src[i:j], ns)
    names = {"GENDER_MALE": "gender_male", "GENDER_FEMALE": "gender_female", "SHINY_STAR": "shiny_star"}
    for n, col, rows in ns["GLYPH_ART"]:
        im = Image.new("RGBA", (14, 14), (0, 0, 0, 0))
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c == "#":
                    im.putpixel((x, y), col)
        save(im, f"ui_gba/{names[n]}.png")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    export_sprites()
    export_region_map()
    export_battle_and_extras()
    print("done ->", OUT)
    sys.exit(0)
