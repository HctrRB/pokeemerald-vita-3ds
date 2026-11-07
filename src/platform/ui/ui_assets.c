// Atlas of the new UI (assets/ui2/ui_atlas.bin, built by tools/ui2/build_atlas.py) and text.
// Loaded once on Core 0 before the game thread starts; read-only afterwards, so Core 1 can use
// the glyph tables without locks.

#include <stdlib.h>
#include <string.h>
#include "ui_internal.h"
#include "../vita/vita_platform.h"

#define UI_MAX_FONTS 8
#define UI_MAX_SPRITES 256   // must cover UI_SPR_COUNT (ui_sprite_ids.h); a bigger atlas is rejected and the new UI turns off

typedef struct
{
    uint8_t face, px, ascent, lineH;
    int16_t map[256];          // Latin-1 code point -> glyph index, -1 when missing
} UiFont;

static UiFont sFonts[UI_MAX_FONTS];
static uint32_t sFontCount;
static UiGlyph *sGlyphs;
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
    sGlyphs = malloc(sizeof(UiGlyph) * glyphCount);
    sPixels = malloc((size_t)sTexW * sTexH * 2);
    if (sGlyphs == NULL || sPixels == NULL)
    {
        free(data);
        return false;
    }
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
        const uint16_t first = Rd16(r + 4), count = Rd16(r + 6);
        for (uint32_t i = first; i < (uint32_t)first + count && i < glyphCount; i++)
        {
            const uint16_t cp = Rd16(data + glyphsAt + i * 12);
            if (cp < 256)
                font->map[cp] = (int16_t)i;
        }
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

static const UiFont *FindFont(uint8_t face, uint8_t px)
{
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

// Next Latin-1 code point of a UTF-8 string (other code points become '?').
static uint32_t NextCodepoint(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t c = p[0];
    if (c < 0x80)
    {
        *s += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && p[1] != 0)
    {
        *s += 2;
        return ((c & 0x1F) << 6) | (p[1] & 0x3F);
    }
    // 3- and 4-byte sequences: skip them
    uint32_t len = ((c & 0xF0) == 0xE0) ? 3 : ((c & 0xF8) == 0xF0) ? 4 : 1;
    for (uint32_t i = 1; i < len; i++)
        if (p[i] == 0) { len = i; break; }
    *s += len;
    return '?';
}

int32_t UiText_LineHeight(uint8_t face, uint8_t px)
{
    const UiFont *font = FindFont(face, px);
    return font ? font->lineH : px;
}

int32_t UiText_Width(uint8_t face, uint8_t px, const char *s)
{
    const UiFont *font = FindFont(face, px);
    if (font == NULL || s == NULL)
        return 0;
    int32_t w = 0;
    while (*s)
    {
        const uint32_t c = NextCodepoint(&s);
        const int16_t gi = c < 256 ? font->map[c] : -1;
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
        const uint32_t c = NextCodepoint(&s);
        const int16_t gi = c < 256 ? font->map[c] : -1;
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
