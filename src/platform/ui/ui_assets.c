// Atlas of the new UI (assets/ui2/ui_atlas.bin, built by tools/ui2/build_atlas.py) and text.
// Loaded once on Core 0 before the game thread starts; read-only afterwards, so Core 1 can use
// the glyph tables without locks.

#include <stdlib.h>
#include <string.h>
#include "ui_internal.h"
#include "../vita/vita_platform.h"

#define UI_MAX_FONTS 40
#define UI_MAX_SPRITES 256   // must cover UI_SPR_COUNT (ui_sprite_ids.h); a bigger atlas is rejected and the new UI turns off

typedef struct
{
    uint8_t face, px, ascent, lineH;
    uint16_t extFirst, extCount;   // this font's code points above 255: a slice of sExtGlyphs
    int16_t map[256];              // Latin-1 code point -> glyph index, -1 when missing
} UiFont;

// Glyphs above Latin-1 (a language's extra letters, L2 of docs/plan/I18N_PLAN.md), sorted by code
// point within each font's slice. Kept apart from map[] so the common Latin-1 lookup stays one load.
typedef struct { uint16_t cp; int16_t gi; } UiExtGlyph;

static UiFont sFonts[UI_MAX_FONTS];
static uint32_t sFontCount;
static UiGlyph *sGlyphs;
static UiExtGlyph *sExtGlyphs;
static uint16_t *sPixels;            // RGBA5551 fixed part of the texture, uploaded once by ui_render.c
typedef struct { uint16_t id, x, y, w, h; } UiSprite;
static UiSprite sSprites[UI_MAX_SPRITES];
static uint32_t sSpriteCount;
static uint16_t sTexW, sTexH, sWhiteX, sWhiteY;
static bool sReady;

static uint16_t Rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

bool UiAssets_Load(void)
{
    if (sReady)
        return true;
    size_t size = 0;
    uint8_t *data = VitaPlatform_LoadAsset("ui2/ui_atlas.bin", &size);
    if (data == NULL || size < 26 || memcmp(data, "UI2A", 4) != 0 || Rd16(data + 4) != 3)
    {
        VITA_LOG("[UI2] ui2/ui_atlas.bin missing or not version 3\n");
        free(data);
        return false;
    }
    sTexW = Rd16(data + 6);
    sTexH = Rd16(data + 8);
    sWhiteX = Rd16(data + 10);
    sWhiteY = Rd16(data + 12);
    const uint16_t fontCount = Rd16(data + 18);
    const uint16_t glyphCount = Rd16(data + 20);
    const uint16_t spriteCount = Rd16(data + 22);
    const size_t fontsAt = 24, glyphsAt = fontsAt + (size_t)fontCount * 8;
    const size_t spritesAt = glyphsAt + (size_t)glyphCount * 12, pixelsAt = spritesAt + (size_t)spriteCount * 10;
    if (fontCount > UI_MAX_FONTS || spriteCount > UI_MAX_SPRITES || sTexW != UI_TEX_W || sTexH > UI_DYN_Y
        || pixelsAt + (size_t)sTexW * sTexH * 2 > size)
    {
        VITA_LOG("[UI2] ui_atlas.bin truncated or does not fit the texture\n");
        free(data);
        return false;
    }
    uint32_t extTotal = 0;
    for (uint32_t i = 0; i < glyphCount; i++)
        if (Rd16(data + glyphsAt + i * 12) > 0xFF)
            extTotal++;
    sGlyphs = malloc(sizeof(UiGlyph) * glyphCount);
    sPixels = malloc((size_t)sTexW * sTexH * 2);
    sExtGlyphs = extTotal ? malloc(sizeof(UiExtGlyph) * extTotal) : NULL;
    if (sGlyphs == NULL || sPixels == NULL || (extTotal && sExtGlyphs == NULL))
    {
        free(sExtGlyphs);
        free(sPixels);
        free(sGlyphs);
        sExtGlyphs = NULL;
        sPixels = NULL;
        sGlyphs = NULL;
        free(data);
        return false;
    }
    uint32_t extUsed = 0;
    for (uint32_t i = 0; i < glyphCount; i++)
    {
        const uint8_t *g = data + glyphsAt + i * 12;
        sGlyphs[i] = (UiGlyph){ Rd16(g + 2), Rd16(g + 4), g[6], g[7], (int8_t)g[8], (int8_t)g[9], g[10] };
    }
    sFontCount = fontCount;
    for (uint32_t f = 0; f < fontCount; f++)
    {
        const uint8_t *r = data + fontsAt + f * 8;
        UiFont *font = &sFonts[f];
        font->face = r[0];
        font->px = r[1];
        font->ascent = r[2];
        font->lineH = r[3];
        for (uint32_t c = 0; c < 256; c++)
            font->map[c] = -1;
        font->extFirst = (uint16_t)extUsed;
        const uint16_t first = Rd16(r + 4), count = Rd16(r + 6);
        for (uint32_t i = first; i < (uint32_t)first + count && i < glyphCount; i++)
        {
            const uint16_t cp = Rd16(data + glyphsAt + i * 12);
            if (cp < 256)
            {
                font->map[cp] = (int16_t)i;
                continue;
            }
            // Insertion sort: a font has a few dozen extra letters at most, sorted once at boot.
            // extUsed < extTotal holds: extTotal counted every glyph above 255 and each glyph
            // belongs to at most one font slice of the atlas.
            if (extUsed >= extTotal)
                break;
            uint32_t at = extUsed++;
            while (at > font->extFirst && sExtGlyphs[at - 1].cp > cp)
            {
                sExtGlyphs[at] = sExtGlyphs[at - 1];
                at--;
            }
            sExtGlyphs[at] = (UiExtGlyph){ cp, (int16_t)i };
        }
        font->extCount = (uint16_t)(extUsed - font->extFirst);
    }
    sSpriteCount = spriteCount;
    for (uint32_t i = 0; i < spriteCount; i++)
    {
        const uint8_t *p = data + spritesAt + i * 10;
        sSprites[i] = (UiSprite){ Rd16(p), Rd16(p + 2), Rd16(p + 4), Rd16(p + 6), Rd16(p + 8) };
    }
    for (uint32_t i = 0; i < (uint32_t)sTexW * sTexH; i++)
        sPixels[i] = Rd16(data + pixelsAt + i * 2);
    free(data);
    sReady = true;
    VITA_LOG("[UI2] atlas %ux%u, %u fonts, %u glyphs, %u sprites\n", sTexW, sTexH, (unsigned)fontCount, (unsigned)glyphCount, (unsigned)spriteCount);
    return true;
}

bool UiAssets_Ready(void)
{
    return sReady;
}

const uint16_t *UiAssets_Pixels(uint16_t *outW, uint16_t *outH)
{
    *outW = sTexW;
    *outH = sTexH;
    return sPixels;
}

void UiAssets_TexSize(uint16_t *outW, uint16_t *outH)
{
    *outW = UI_TEX_W;
    *outH = UI_TEX_H;
}

bool UiAssets_Sprite(uint16_t id, uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h)
{
    for (uint32_t i = 0; i < sSpriteCount; i++)
    {
        if (sSprites[i].id == id)
        {
            *x = sSprites[i].x;
            *y = sSprites[i].y;
            *w = sSprites[i].w;
            *h = sSprites[i].h;
            return true;
        }
    }
    return false;
}

void UiAssets_White(uint16_t *x, uint16_t *y)
{
    *x = sWhiteX;
    *y = sWhiteY;
}

// The two faces the screens ask for are roles: Press Start 2P = titles, VT323 = text. Each menu
// theme maps them to its own faces (ui_theme.c, fonts baked by tools/ui2/bake_theme_fonts.py).
static uint8_t ThemeFace(uint8_t face)
{
    return face == UI_FACE_PS2P ? gUiTheme->faceTitle : face == UI_FACE_VT323 ? gUiTheme->faceBody : face;
}

static const UiFont *FindFont(uint8_t face, uint8_t px)
{
    face = ThemeFace(face);
    const UiFont *best = NULL;
    for (uint32_t i = 0; i < sFontCount; i++)
    {
        if (sFonts[i].face != face)
            continue;
        if (sFonts[i].px == px)
            return &sFonts[i];
        if (best == NULL || abs((int)sFonts[i].px - (int)px) < abs((int)best->px - (int)px))
            best = &sFonts[i];
    }
    return best;
}

// Next code point of a UTF-8 string. A malformed or truncated sequence gives '?' and consumes only
// the bytes that belong to it, so a string cut in the middle of a letter never reads past its end.
static uint32_t NextCodepoint(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    const uint32_t c = p[0];
    if (c < 0x80)
    {
        *s += 1;
        return c;
    }
    uint32_t len, cp;
    if ((c & 0xE0) == 0xC0)      { len = 2; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07; }
    else                         { *s += 1; return '?'; }   // stray continuation byte or invalid lead
    for (uint32_t i = 1; i < len; i++)
    {
        if ((p[i] & 0xC0) != 0x80)                       // includes the terminating 0
        {
            *s += i;
            return '?';
        }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    *s += len;
    return cp;
}

// Letter drawn in place of a Latin-1 one the font lacks (0xC0-0xFF): the same letter without its
// accent, so a name stays readable before a language's glyphs are baked (L2 of the i18n plan).
static const char sLatin1Fold[64 + 1] =
    "AAAAAAAC" "EEEEIIII" "DNOOOOOx" "OUUUUY?s"
    "aaaaaaac" "eeeeiiii" "dnooooo?" "ouuuuy?y";
_Static_assert(sizeof(sLatin1Fold) == 65, "sLatin1Fold covers 0xC0-0xFF");

static uint32_t FoldCodepoint(uint32_t cp)
{
    if (cp >= 0xC0 && cp <= 0xFF)
        return (uint8_t)sLatin1Fold[cp - 0xC0];
    switch (cp)
    {
    case 0xA1: return '!';
    case 0xBF: return '?';
    case 0xAA: return 'a';
    case 0xBA: return 'o';
    case 0x152: return 'O';                 // Œ
    case 0x153: return 'o';                 // œ
    case 0x2018: case 0x2019: return '\'';
    case 0x201C: case 0x201D: return '"';
    case 0x2026: return '.';                // …
    case 0x2715: return 'X';                // ✕, the cross button in hints
    default: return '?';
    }
}

static int16_t ExactGlyph(const UiFont *font, uint32_t cp)
{
    if (cp < 256)
        return font->map[cp];
    if (cp > 0xFFFF)                        // the atlas stores 16-bit code points
        return -1;
    uint32_t lo = font->extFirst, hi = (uint32_t)font->extFirst + font->extCount;
    while (lo < hi)
    {
        const uint32_t mid = (lo + hi) / 2;
        if (sExtGlyphs[mid].cp < cp)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (lo < (uint32_t)font->extFirst + font->extCount && sExtGlyphs[lo].cp == cp) ? sExtGlyphs[lo].gi : -1;
}

// Glyph of a code point: exact, else its fallback letter, else '?'; -1 when the font has none of
// them, and for missing ASCII (control codes): the caller then leaves a gap of half the size.
static int16_t GlyphOf(const UiFont *font, uint32_t cp)
{
    int16_t gi = ExactGlyph(font, cp);
    if (gi >= 0 || cp < 0x80)
        return gi;
    gi = font->map[FoldCodepoint(cp)];
    return gi >= 0 ? gi : font->map['?'];
}

int32_t UiText_LineHeight(uint8_t face, uint8_t px)
{
    const UiFont *font = FindFont(face, px);
    return font ? font->lineH : px;
}

// Offset from the top of the line (UiText_Draw's y) to the middle of the capitals: what is
// centred in a box, since each theme font has its own ascent and cap height
int32_t UiText_CapMid(uint8_t face, uint8_t px)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL)
        return px / 2;
    const int16_t gi = font->map['H'];
    if (gi < 0)
        return font->ascent - px / 3;
    const UiGlyph *g = &sGlyphs[gi];
    return font->ascent + g->yoff + g->h / 2;
}

// Left and right ends of the ink of `s` drawn at x = 0 (the advance widths leave a side bearing)
static void InkSpan(const UiFont *font, const char *s, int32_t *left, int32_t *right)
{
    int32_t x = 0, lo = 0, hi = 0;
    bool any = false;
    while (*s)
    {
        const int16_t gi = GlyphOf(font, NextCodepoint(&s));
        if (gi < 0) { x += font->px / 2; continue; }
        const UiGlyph *g = &sGlyphs[gi];
        if (g->w)
        {
            if (!any || x + g->xoff < lo) lo = x + g->xoff;
            if (!any || x + g->xoff + g->w > hi) hi = x + g->xoff + g->w;
            any = true;
        }
        x += g->adv;
    }
    *left = lo;
    *right = any ? hi : x;
}

// Text centred on both axes in the box: its ink across, its capitals down
int32_t UiText_DrawCentred(uint8_t face, uint8_t px, int32_t boxX, int32_t boxY, int32_t boxW, int32_t boxH, const char *s, uint32_t rgba)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL || s == NULL)
        return 0;
    int32_t lo, hi;
    InkSpan(font, s, &lo, &hi);
    return UiText_DrawMid(face, px, boxX + (boxW - (hi - lo)) / 2 - lo, boxY, boxH, s, rgba);
}

// Width of the ink of `s` (no side bearings)
int32_t UiText_InkWidth(uint8_t face, uint8_t px, const char *s)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL || s == NULL)
        return 0;
    int32_t lo, hi;
    InkSpan(font, s, &lo, &hi);
    return hi - lo;
}

// Text whose capitals are centred vertically in the box (boxY, boxH)
int32_t UiText_DrawMid(uint8_t face, uint8_t px, int32_t x, int32_t boxY, int32_t boxH, const char *s, uint32_t rgba)
{
    return UiText_Draw(face, px, x, boxY + boxH / 2 - UiText_CapMid(face, px), s, rgba);
}

int32_t UiText_Width(uint8_t face, uint8_t px, const char *s)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL || s == NULL)
        return 0;
    int32_t w = 0;
    while (*s)
    {
        const int16_t gi = GlyphOf(font, NextCodepoint(&s));
        w += gi >= 0 ? sGlyphs[gi].adv : font->px / 2;
    }
    return w;
}

int32_t UiText_Draw(uint8_t face, uint8_t px, int32_t x, int32_t y, const char *s, uint32_t rgba)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL || s == NULL)
        return 0;
    const int32_t x0 = x, base = y + font->ascent;
    while (*s)
    {
        const int16_t gi = GlyphOf(font, NextCodepoint(&s));
        if (gi < 0)
        {
            x += font->px / 2;
            continue;
        }
        const UiGlyph *g = &sGlyphs[gi];
        if (g->w && g->h)
            UiDraw_Quad(x + g->xoff, base + g->yoff, g->w, g->h, g->x, g->y, g->x + g->w, g->y + g->h, rgba);
        x += g->adv;
    }
    return x - x0;
}

_Static_assert(UI_SPR_COUNT <= UI_MAX_SPRITES, "ui_atlas.bin has more sprites than UI_MAX_SPRITES: the new UI would not load");
