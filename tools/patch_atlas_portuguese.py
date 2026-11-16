from pathlib import Path

root=Path(r'C:\Emerald\pokeemerald-vita-ptbr')
p=root/'tools/ui2/build_atlas.py'
s=p.read_text('utf8')
if 'def portuguese_accents(' not in s:
    s=s.replace('import struct\n', 'import struct\nimport unicodedata\nfrom PIL import ImageDraw\n',1)
    marker='\ndef main():\n'
    assert marker in s
    code=r'''
# Generate missing Portuguese glyphs directly from the original theme font masks.
# This does not change any of the style's source images or modify GBA graphics.
# Added glyphs are alphabetic variants of each face's Latin letters.
PORTUGUESE_CHARS = "ÀÁÂÃÇÊÍÓÕÚàáâãçêíóôõú"


def portuguese_accents(font, source):
    lookup = {g[0]: g for g in font["glyphs"]}
    for char in PORTUGUESE_CHARS:
        codepoint = ord(char)
        if codepoint in lookup:
            continue
        decomp = unicodedata.normalize("NFD", char)
        if len(decomp) != 2 or ord(decomp[0]) not in lookup:
            continue
        mark = decomp[1]
        if mark not in ("\u0301", "\u0300", "\u0302", "\u0303", "\u0327"):
            continue
        base = lookup[ord(decomp[0])]
        cp, sx, sy, w, h, xoff, yoff, advance = base
        pad_x = max(3, font["px"] // 7)
        pad_up = max(5, font["px"] // 5)
        pad_down = max(4, font["px"] // 7)
        canvas = Image.new("RGBA", (w + pad_x * 2, h + pad_up + pad_down), (0, 0, 0, 0))
        src_img = source.crop((sx, sy, sx + w, sy + h))
        canvas.paste(src_img, (pad_x, pad_up))
        draw = ImageDraw.Draw(canvas)
        mx = pad_x + (w + 1) // 2
        thick = max(1, font["px"] // 19)
        span = max(3, font["px"] // 7)
        yy = pad_up - 2
        white = (255, 255, 255, 255)
        if mark == "\u0301":  # acute
            draw.line([(mx - span // 2, yy), (mx + span // 2, yy - span)], fill=white, width=thick)
        elif mark == "\u0300":  # grave
            draw.line([(mx + span // 2, yy), (mx - span // 2, yy - span)], fill=white, width=thick)
        elif mark == "\u0302":  # circumflex
            draw.line([(mx - span, yy), (mx, yy - span)], fill=white, width=thick)
            draw.line([(mx, yy - span), (mx + span, yy)], fill=white, width=thick)
        elif mark == "\u0303":  # tilde
            draw.line([(mx - span, yy - 1), (mx - span // 2, yy - 2),
                       (mx, yy - 1), (mx + span // 2, yy - 2),
                       (mx + span, yy - 1)], fill=white, width=thick)
        elif mark == "\u0327":  # cedilla
            bottom = pad_up + h
            draw.line([(mx, bottom - 2), (mx + thick, bottom),
                       (mx + thick, bottom + max(2, span - 1)),
                       (mx - thick, bottom + span)], fill=white, width=thick)
        box = canvas.getbbox()
        if box:
            glyph = canvas.crop(box)
            yield codepoint, glyph, xoff - pad_x + box[0], yoff - pad_up + box[1], advance

'''
    s=s.replace(marker,'\n'+code+marker,1)
    old='        fonts.append((f["face"], f["px"], f["ascent"], f["lineH"], first, len(glyphs) - first))'
    assert s.count(old)==1
    s=s.replace(old,'''        # Put Portuguese glyphs directly after the original font glyphs, preserving each face.
        for cp, mask, xoff, yoff, adv in portuguese_accents(f, src):
            w, h = mask.size
            x, y = place(w, h)
            atlas.paste(mask, (x, y))
            glyphs.append((cp, x, y, w, h, max(-128, min(127, xoff)), max(-128, min(127, yoff)), adv))
'''+old,1)
    s=s.replace('MAX_FIXED_H = 1024', 'MAX_FIXED_H = 1280',1)
    p.write_text(s,'utf8',newline='\n')
    print('Font atlas patched')
else: print('Atlas already patched')

p=root/'src/platform/ui/ui_internal.h'
s=p.read_text('utf8')
assert '#define UI_TEX_H 2048' in s
if '#define UI_DYN_Y 1024' in s:
    s=s.replace('#define UI_DYN_Y 1024','#define UI_DYN_Y 1280',1)
    p.write_text(s,'utf8',newline='\n')
print('UI dynamic texture begins at row 1280, with 192 runtime cells preserved')
