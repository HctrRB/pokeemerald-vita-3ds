"""Builds assets/ui2/ui_atlas.bin and src/platform/ui/ui_sprite_ids.h.

The atlas is the fixed top part of the new UI's 1024x1024 RGBA5551 texture: the fonts baked by
design/stitch/_tools/font_bake.html (tools/ui2/fonts/ui_fonts.png + .json), the white block for
solid fills and the fixed original GBA sprites of the menus (decision D7): Poké Balls, START menu
icons, player heads, gym badges, Battle Frontier symbols, status icons, held-item icons, type and
contest icons. Sprites that depend on the species or the item (Pokémon icons and front sprites,
item icons, footprints) are decoded at run time from the game's own tables into the cells below
this part (src/platform/ui/ui_sprites.c).

RGBA5551 keeps the GBA's 5-bit colour exactly; fonts and sprites only use full or no alpha.
Only the type sizes chosen on the console are kept (decision D2): Press Start 2P 16 / 24 and
VT323 32. Run from the repository root:

    python tools/ui2/build_atlas.py

Format v3 (little endian), read by src/platform/ui/ui_assets.c:
    char  magic[4] = "UI2A"
    u16   version = 3
    u16   texW, texH                           fixed part only (texW = 1024)
    u16   whiteX, whiteY, whiteW, whiteH       opaque white block used for solid fills
    u16   fontCount, glyphCount, spriteCount
    fontCount   x { u8 face, u8 px, u8 ascent, u8 lineH, u16 firstGlyph, u16 glyphCount }
    glyphCount  x { u16 codepoint, u16 x, u16 y, u8 w, u8 h, s8 xoff, s8 yoff, u8 adv, u8 pad }
    spriteCount x { u16 id, u16 x, u16 y, u16 w, u16 h }
    texW * texH x u16 RGBA5551 (as GL_UNSIGNED_SHORT_5_5_5_1: R in bits 15-11, alpha in bit 0)
"""
import json
import os
import struct

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC_PNG = os.path.join(HERE, "fonts", "ui_fonts.png")
SRC_JSON = os.path.join(HERE, "fonts", "ui_fonts.json")
OUT = os.path.join(ROOT, "assets", "ui2", "ui_atlas.bin")
OUT_H = os.path.join(ROOT, "src", "platform", "ui", "ui_sprite_ids.h")
KEEP = {(0, 16), (0, 24), (1, 32), (1, 40)}                   # (face, px): D2
TEX_W = 1024
MAX_FIXED_H = 384                                    # = UI_DYN_Y in ui_internal.h                                    # the dynamic cells start below (ui_sprites.c)
ASSETS = os.path.join(ROOT, "design", "stitch", "_assets")   # GBA art exported by export_assets.py
GFX = os.path.join(ROOT, "graphics")

BALLS = ["poke", "great", "ultra", "master", "safari", "net", "dive", "nest", "repeat", "timer", "luxury", "premier"]
STATUS = ["PSN", "PAR", "SLP", "FRZ", "BRN", "PKRS", "FNT"]          # rows of status_icons.png
# Type icons in TYPE_* order, then the contest categories; palette = sMoveTypeToOamPaletteNum
# (pokemon_summary_screen.c): 13 -> move_types_1.pal, 14 -> _2, 15 -> _3
TYPES = [("normal", 1), ("fight", 1), ("flying", 2), ("poison", 2), ("ground", 1), ("rock", 1), ("bug", 3),
         ("ghost", 2), ("steel", 1), ("mystery", 3), ("fire", 1), ("water", 2), ("grass", 3), ("electric", 1),
         ("psychic", 2), ("ice", 2), ("dragon", 3), ("dark", 1)]
CONTEST = [("contest_cool", 1), ("contest_beauty", 2), ("contest_cute", 2), ("contest_smart", 3), ("contest_tough", 1)]


GLYPH_ART = [
    ("GENDER_MALE", (47, 125, 194, 255), [
        "......########",
        "..........####",
        ".........#####",
        "........###.##",
        "..####.###..##",
        ".#########....",
        "###....###....",
        "##......##....",
        "##......##....",
        "##......##....",
        "###....###....",
        ".########.....",
        "..######......",
        ".............."]),
    ("GENDER_FEMALE", (217, 72, 59, 255), [
        "....######....",
        "..##########..",
        ".###......###.",
        ".##........##.",
        ".##........##.",
        ".###......###.",
        "..##########..",
        "....######....",
        "......##......",
        "....######....",
        "....######....",
        "......##......",
        "......##......",
        ".............."]),
    ("SHINY_STAR", (246, 207, 87, 255), [
        "......##......",
        "......##......",
        ".....####.....",
        ".....####.....",
        "##############",
        ".############.",
        "..##########..",
        "...########...",
        "...########...",
        "..####..####..",
        "..###....###..",
        ".###......###.",
        ".##........##.",
        ".............."]),
]


def jasc(path):
    lines = open(path, encoding="ascii").read().split()
    n = int(lines[2])
    vals = list(map(int, lines[3:3 + n * 3]))
    return [tuple(vals[i * 3:i * 3 + 3]) for i in range(n)]


def indexed(path, palette=None, crop=None):
    """Indexed GBA PNG -> RGBA, index 0 transparent, optional replacement palette."""
    im = Image.open(path)
    if crop:
        im = im.crop(crop)
    pal = palette or [tuple(im.getpalette()[i * 3:i * 3 + 3]) for i in range(16)]
    out = Image.new("RGBA", im.size, (0, 0, 0, 0))
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            i = px[x, y]
            if i:
                out.putpixel((x, y), pal[i] + (255,))
    return out


def sprite_list():
    """(C name, RGBA image) in id order."""
    out = []
    for b in BALLS:
        out.append(("BALL_" + b.upper(), Image.open(os.path.join(ASSETS, "balls", b + ".png")).convert("RGBA")))
    out.append(("START_BAG", Image.open(os.path.join(ASSETS, "start", "bag.png")).convert("RGBA")))
    out.append(("START_POKENAV", Image.open(os.path.join(ASSETS, "start", "pokenav.png")).convert("RGBA")))
    # Bag art by pocket (graphics/bag/bag_*.png: frame 0 closed, 1..5 = ITEMS..KEY ITEMS), 64x64
    for g in ("male", "female"):
        sheet = Image.open(os.path.join(ASSETS, "bag", f"bag_{g}.png")).convert("RGBA")
        for f in range(1, 6):
            out.append((f"BAG_{g.upper()}_{f}", sheet.crop((0, f * 64, 64, f * 64 + 64))))
    # PokéNav icon of the field call window: 8 animation frames of 32x32
    for f in range(8):
        out.append((f"PHONE_{f}", indexed(os.path.join(GFX, "pokenav", "match_call", "nav_icon.png"), crop=(0, f * 32, 32, f * 32 + 32))))
    # Gender symbols and the shiny star (the UI fonts have no glyph for them): 14x14 pixel art
    for name, colour, rows in GLYPH_ART:
        img = Image.new("RGBA", (len(rows[0]), len(rows)), (0, 0, 0, 0))
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch == "#":
                    img.putpixel((x, y), colour)
                elif ch == "o":
                    img.putpixel((x, y), (40, 30, 20, 255))
        out.append((name, img))
    out.append(("HOENN_MAP", Image.open(os.path.join(ASSETS, "map", "hoenn_224x120.png")).convert("RGBA")))
    for r in range(32):
        out.append((f"RIBBON_{r}", Image.open(os.path.join(ASSETS, "ribbons", f"ribbon_{r}.png")).convert("RGBA")))
    out.append(("HEAD_BRENDAN", Image.open(os.path.join(ASSETS, "map", "brendan_head.png")).convert("RGBA")))
    out.append(("HEAD_MAY", Image.open(os.path.join(ASSETS, "map", "may_head.png")).convert("RGBA")))
    for i in range(1, 9):
        out.append((f"BADGE_{i}", Image.open(os.path.join(ASSETS, "badges", f"badge_{i}.png")).convert("RGBA")))
    for metal in ("silver", "gold"):
        for i in range(1, 8):
            out.append((f"FRONTIER_{metal.upper()}_{i}", Image.open(os.path.join(ASSETS, "medals", f"{metal}_{i}.png")).convert("RGBA")))
    for row, name in enumerate(STATUS):
        out.append(("STATUS_" + name, indexed(os.path.join(GFX, "interface", "status_icons.png"), crop=(0, row * 8, 32, row * 8 + 8))))
    out.append(("HOLD_ITEM", indexed(os.path.join(GFX, "party_menu", "hold_icons.png"), crop=(0, 0, 8, 8))))
    out.append(("HOLD_MAIL", indexed(os.path.join(GFX, "party_menu", "hold_icons.png"), crop=(0, 8, 8, 16))))
    pals = {n: jasc(os.path.join(GFX, "types", f"move_types_{n}.pal")) for n in (1, 2, 3)}
    for name, p in TYPES + CONTEST:
        out.append(("TYPE_" + name.upper(), indexed(os.path.join(GFX, "types", name + ".png"), palette=pals[p])))
    # Battle HUD: party status balls (normal, empty, status, fainted: the order of ball_display.png)
    # and the "already caught" ball of the wild foe's healthbox
    bi = os.path.join(GFX, "battle_interface")
    for f, name in enumerate(("OK", "EMPTY", "STATUS", "FAINTED")):
        out.append((f"BATTLE_BALL_{name}", indexed(os.path.join(bi, "ball_display.png"), crop=(f * 8, 0, f * 8 + 8, 8))))
    out.append(("BATTLE_CAUGHT", indexed(os.path.join(bi, "ball_caught_indicator.png"))))
    return out


def rgba5551(r, g, b, a):
    if a < 128:
        return 0
    return ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | 1


def main():
    meta = json.load(open(SRC_JSON, encoding="utf-8"))
    src = Image.open(SRC_PNG).convert("RGBA")
    atlas = Image.new("RGBA", (TEX_W, 1024), (0, 0, 0, 0))
    for y in range(4):                                # white block for solid fills
        for x in range(4):
            atlas.putpixel((x, y), (255, 255, 255, 255))
    cx, cy, row_h = 6, 0, 4

    def place(w, h):
        nonlocal cx, cy, row_h
        if cx + w + 1 > TEX_W:
            cx, cy, row_h = 0, cy + row_h + 1, 0
        x, y = cx, cy
        cx += w + 1
        row_h = max(row_h, h)
        return x, y

    fonts, glyphs = [], []
    for f in meta["fonts"]:
        if (f["face"], f["px"]) not in KEEP:
            continue
        first = len(glyphs)
        for cp, gx, gy, w, h, xoff, yoff, adv in f["glyphs"]:
            x, y = place(w, h)
            for j in range(h):
                for i in range(w):
                    if src.getpixel((gx + i, gy + j))[3] >= 128:
                        atlas.putpixel((x + i, y + j), (255, 255, 255, 255))
            glyphs.append((cp, x, y, w, h, max(-128, min(127, xoff)), max(-128, min(127, yoff)), adv))
        fonts.append((f["face"], f["px"], f["ascent"], f["lineH"], first, len(glyphs) - first))

    # Sprites, tallest first so they pack in few rows
    named = sprite_list()
    order = sorted(range(len(named)), key=lambda i: -named[i][1].height)
    cx, cy, row_h = 0, cy + row_h + 2, 0
    sprites = [None] * len(named)
    for i in order:
        im = named[i][1]
        x, y = place(im.width, im.height)
        atlas.paste(im, (x, y))
        sprites[i] = (i, x, y, im.width, im.height)

    tex_h = (cy + row_h + 8) & ~7
    if tex_h > MAX_FIXED_H:
        raise SystemExit(f"fixed atlas is {tex_h} px tall, more than {MAX_FIXED_H}")
    out = bytearray(b"UI2A")
    out += struct.pack("<HHHHHHHHHH", 3, TEX_W, tex_h, 0, 0, 4, 4, len(fonts), len(glyphs), len(sprites))
    for face, px, asc, lh, first, count in fonts:
        out += struct.pack("<BBBBHH", face, px, asc, lh, first, count)
    for cp, x, y, w, h, xoff, yoff, adv in glyphs:
        out += struct.pack("<HHHBBbbBB", cp, x, y, w, h, xoff, yoff, adv, 0)
    for sid, x, y, w, h in sprites:
        out += struct.pack("<HHHHH", sid, x, y, w, h)
    for y in range(tex_h):
        for x in range(TEX_W):
            out += struct.pack("<H", rgba5551(*atlas.getpixel((x, y))))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "wb") as fh:
        fh.write(out)

    lines = ["// Generated by tools/ui2/build_atlas.py: ids of the fixed GBA sprites of assets/ui2/ui_atlas.bin.",
             "// Do not edit; run the script again after changing its sprite list.", "",
             "#ifndef UI_SPRITE_IDS_H", "#define UI_SPRITE_IDS_H", "", "enum UiSpriteId", "{"]
    lines += [f"    UI_SPR_{name} = {i}," for i, (name, _) in enumerate(named)]
    lines += [f"    UI_SPR_COUNT = {len(named)}", "};", "",
              "// First ids of the runs indexed by game constants",
              "#define UI_SPR_STATUS_FIRST UI_SPR_STATUS_PSN       // + STATUS row (PSN, PAR, SLP, FRZ, BRN, PKRS, FNT)",
              "#define UI_SPR_TYPE_FIRST UI_SPR_TYPE_NORMAL        // + TYPE_* (pokemon.h)",
              "#define UI_SPR_CONTEST_FIRST UI_SPR_TYPE_CONTEST_COOL   // + CONTEST_CATEGORY_*",
              "#define UI_SPR_BADGE_FIRST UI_SPR_BADGE_1",
              "#define UI_SPR_BAG_MALE_FIRST UI_SPR_BAG_MALE_1     // + pocket (ITEMS_POCKET..KEYITEMS_POCKET)",
              "#define UI_SPR_BAG_FEMALE_FIRST UI_SPR_BAG_FEMALE_1",
              "#define UI_SPR_PHONE_FIRST UI_SPR_PHONE_0           // + frame (0..7)",
              "#define UI_SPR_RIBBON_FIRST UI_SPR_RIBBON_0         // + ribbon id (constants/pokemon.h)",
              "#define UI_SPR_BATTLE_BALL_FIRST UI_SPR_BATTLE_BALL_OK   // + OK, EMPTY, STATUS, FAINTED",
              "", "#endif", ""]
    with open(OUT_H, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines))
    print(f"{OUT}: {TEX_W}x{tex_h}, {len(fonts)} fonts, {len(glyphs)} glyphs, {len(sprites)} sprites, {len(out)} bytes")


if __name__ == "__main__":
    main()
