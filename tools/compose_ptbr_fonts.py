from pathlib import Path
import re

p=Path(r'C:\Emerald\pokeemerald-vita-ptbr\src\text.c')
s=p.read_text('utf-8')
marker='/* PT-BR synthesized tilde glyphs: base Latin letter plus the tilde of n-tilde. */'
helper=r'''
/* PT-BR synthesized tilde glyphs: base Latin letter plus the tilde of n-tilde. */
/* The original 2-bpp fonts do not include glyphs for A/O/a/o with a tilde.
 * Draw the base glyph in the current palette and copy only the accent rows
 * from N-tilde/n-tilde where the corresponding N/n row is blank. */
static void ComposePortugueseGlyph(u16 glyphId, const u16 *font, const u8 *widths)
{
    static const struct { u8 slot, letter, tilde, plain; } sLetters[] =
    {
        { CHAR_A_TILDE, CHAR_A, CHAR_N_TILDE, CHAR_N },
        { CHAR_O_TILDE, CHAR_O, CHAR_N_TILDE, CHAR_N },
        { CHAR_a_TILDE, CHAR_a, CHAR_n_TILDE, CHAR_n },
        { CHAR_o_TILDE, CHAR_o, CHAR_n_TILDE, CHAR_n },
    };
    u32 tildeTop[8], tildeBottom[8], plainTop[8], plainBottom[8];
    u32 emptyTop[8], emptyBottom[8];
    u32 row, old, blank;
    unsigned y, x;
    int shift;
    const u16 *glyphs;
    const unsigned idx = (unsigned)(glyphId - CHAR_A_TILDE);
    if (glyphId < CHAR_A_TILDE || glyphId > CHAR_o_TILDE)
        return;
    glyphs = font + 0x20 * sLetters[idx].letter;
    DecompressGlyphTile(glyphs, gCurGlyph.gfxBufferTop);
    DecompressGlyphTile(glyphs + 0x10, gCurGlyph.gfxBufferBottom);

    glyphs = font + 0x20 * sLetters[idx].tilde;
    DecompressGlyphTile(glyphs, tildeTop);
    DecompressGlyphTile(glyphs + 0x10, tildeBottom);
    glyphs = font + 0x20 * sLetters[idx].plain;
    DecompressGlyphTile(glyphs, plainTop);
    DecompressGlyphTile(glyphs + 0x10, plainBottom);
    glyphs = font + 0x20 * CHAR_SPACE;
    DecompressGlyphTile(glyphs, emptyTop);
    DecompressGlyphTile(glyphs + 0x10, emptyBottom);

    shift = ((int)widths[sLetters[idx].letter] - (int)widths[sLetters[idx].tilde]) / 2;
    for (y = 0; y < 16; ++y)
    {
        u32 *dest = y < 8 ? gCurGlyph.gfxBufferTop + y : gCurGlyph.gfxBufferBottom + (y - 8);
        row = y < 8 ? tildeTop[y] : tildeBottom[y - 8];
        old = y < 8 ? plainTop[y] : plainBottom[y - 8];
        blank = y < 8 ? emptyTop[y] : emptyBottom[y - 8];
        if (old != blank)
            continue;
        for (x = 0; x < 8; ++x)
        {
            unsigned dx;
            const u32 pixel = (row >> (x * 4)) & 15;
            const u32 bg = (blank >> (x * 4)) & 15;
            if (pixel == bg || (int)x + shift < 0 || (int)x + shift >= 8)
                continue;
            dx = (unsigned)((int)x + shift);
            *dest = (*dest & ~((u32)15 << (dx * 4))) | (pixel << (dx * 4));
        }
    }
}

'''
if marker not in s:
    insert_at=s.find('static void DecompressGlyph_Small(u16 glyphId, bool32 isJapanese)\n{')
    assert insert_at>30000,insert_at
    s=s[:insert_at]+helper+s[insert_at:]
    mapping={
        'Small':'gFontSmall',
        'SmallNarrow':'gFontSmallNarrow',
        'Narrow':'gFontNarrow',
        'Short':'gFontShort',
        'Normal':'gFontNormal',
    }
    for name, glyph in mapping.items():
        pattern=r'(static void DecompressGlyph_'+name+r'\(u16 glyphId, bool32 isJapanese\)\n\{.*?)(\n\}\n\nstatic u32 GetGlyphWidth_'+name+r'\()'
        match=re.search(pattern,s,re.S)
        assert match is not None,name
        body=match.group(1)
        found=list(re.finditer(r'(gCurGlyph.height\s*=\s*\d+;)',body))
        assert len(found)==2,(name,len(found))
        # Inject once into the Latin branch, after glyph decompression.
        pos=found[-1].end()
        body=body[:pos]+f'\n        if (glyphId >= CHAR_A_TILDE && glyphId <= CHAR_o_TILDE)\n            ComposePortugueseGlyph(glyphId, {glyph}LatinGlyphs, {glyph}LatinGlyphWidths);'+body[pos:]
        s=s[:match.start(1)]+body+s[match.end(1):]
    p.write_text(s,'utf-8')
    print('Patched accent composition into five GBA fonts')
else:
    print('Already patched')
