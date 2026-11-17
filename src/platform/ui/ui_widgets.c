// Components of the new UI: the C side of design/stitch/_shared/ui.css (same tokens, sizes and
// finish). Everything is rectangles from the atlas white block (with vertical two-colour
// gradients), glyphs and sprites, so a whole screen stays one draw call.
// Bevels: outer edge with the corner pixel cut, inner highlight (top-left), inner shade
// (bottom-right) and a hard drop shadow, like the reference's double-bevel boxes.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ui_internal.h"

#define HEADER_H 48
#define FOOTER_H 40

#define T gUiTheme
#define C_SHADOW      UI_RGBA(0, 0, 0, 102)
#ifndef C_WHITE
#define C_WHITE       UI_RGBA(255, 255, 255, 255)
#endif
#define C_GLOSS       UI_RGBA(255, 255, 255, 46)
#define GBA_CHARCOAL  UI_RGBA(40, 48, 48, 255)
#define GBA_MINT      UI_RGBA(115, 205, 164, 255)
#define GBA_EMERALD   UI_RGBA(56, 152, 120, 255)
#define GBA_DARK_SH   UI_RGBA(16, 26, 30, 255)
#define GBA_YELLOW    UI_RGBA(255, 230, 57, 255)
#define GBA_TOPBAR_H  52
#define GBA_INK       UI_RGBA(48, 48, 48, 255)

// ---------- Shape primitives (rounded or chamfered corners, from the theme) ----------

// Edge inset of row k of a corner of size r
static int32_t CornerInset(int32_t r, int32_t k, bool chamfer)
{
    if (chamfer)
        return r - k;
    const float d = (float)r - (float)k - 0.5f;
    const float in = (float)r - sqrtf((float)(r * r) - d * d);
    return (int32_t)(in + 0.5f);
}

static void ShapeFill(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, bool chamfer, uint32_t top, uint32_t bottom)
{
    if (w <= 0 || h <= 0)
        return;
    for (int32_t k = 0; k < r; k++)
    {
        const int32_t in = CornerInset(r, k, chamfer);
        UiDraw_Rect(x + in, y + k, w - 2 * in, 1, top);
        UiDraw_Rect(x + in, y + h - 1 - k, w - 2 * in, 1, bottom);
    }
    if (top == bottom)
        UiDraw_Rect(x, y + r, w, h - 2 * r, top);
    else
        UiDraw_RectV(x, y + r, w, h - 2 * r, top, bottom);
}

// Box of corner size r: hard shadow (sx, sy), edge of thickness t, vertical gradient fill, inner
// highlight / shade of thickness b and an optional glass highlight on the upper half
static void BevelBoxR(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, bool chamfer, int32_t t, uint32_t edge,
                      uint32_t top, uint32_t bottom, int32_t b, uint32_t hi, uint32_t shade, int32_t sx, int32_t sy,
                      uint32_t shadowCol, bool gloss)
{
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    if (sx > 0 || sy > 0)
        ShapeFill(x + sx, y + sy, w, h, r, chamfer, shadowCol, shadowCol);
    if (t > 0)
        ShapeFill(x, y, w, h, r, chamfer, edge, edge);
    const int32_t fx = x + t, fy = y + t, fw = w - 2 * t, fh = h - 2 * t;
    const int32_t fr = r > t ? r - t : 0;
    ShapeFill(fx, fy, fw, fh, fr, chamfer, top, bottom);
    if (b > 0)
    {
        UiDraw_Rect(fx + fr, fy, fw - 2 * fr, b, hi);
        UiDraw_Rect(fx, fy + fr, b, fh - 2 * fr, hi);
        UiDraw_Rect(fx + fr, fy + fh - b, fw - 2 * fr, b, shade);
        UiDraw_Rect(fx + fw - b, fy + fr, b, fh - 2 * fr, shade);
    }
    if (gloss && fh > 6)
        UiDraw_Rect(fx + fr, fy + 1, fw - 2 * fr, fh / 2 - 1, C_GLOSS);
}

// Box in the shape of the current theme. shadow: the depth of the default design (4 px panels,
// 6 px windows, 2 px capsules), scaled to the theme's shadow.
static void BevelBox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t edge, uint32_t top, uint32_t bottom,
                     int32_t b, uint32_t hi, uint32_t shade, int32_t shadow)
{
    int32_t r = T->radius;
    if (h < 12) r = 0;
    else if (h < 24 && r > 3) r = T->chamfer ? 4 : 3;
    const int32_t sy = shadow > 0 ? (shadow * T->shadow + 3) / 4 : 0;
    const int32_t sx = shadow > 0 ? (shadow * T->shadowR + 3) / 4 : 0;
    BevelBoxR(x, y, w, h, r, T->chamfer, t, edge, top, bottom, T->bevel ? b : 0, hi, shade, sx, sy, T->shadowCol, false);
}

// Glass highlight of the theme (CRYSTAL) on a box already drawn
static void Gloss(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t)
{
    if (!T->gloss || h <= 2 * t + 6)
        return;
    const int32_t r = T->radius > t ? T->radius - t : 0;
    const int32_t rr = r > (h - 2 * t) / 2 ? (h - 2 * t) / 2 : r;
    UiDraw_Rect(x + t + rr, y + t + 1, w - 2 * t - 2 * rr, (h - 2 * t) / 2 - 1, C_GLOSS);
}

// ---------- Bevel primitives ----------

// Public bevelled box (battle HUD plates, command buttons): always the original rounded shape,
// the battle HUD keeps its own colours whatever the menu theme
void UiW_Bevel(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t edge, uint32_t top, uint32_t bottom,
               int32_t b, uint32_t hi, uint32_t shade, int32_t shadow)
{
    const int32_t r = h >= 24 ? 6 : h >= 12 ? 3 : 0;
    BevelBoxR(x, y, w, h, r, false, t, edge, top, bottom, b, hi, shade, 0, shadow, C_SHADOW, false);
}

void UiW_ThemeBox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t edge, uint32_t top, uint32_t bottom,
                  uint32_t hi, uint32_t shade, int32_t shadow, bool pill, uint32_t ring)
{
    int32_t r = T->radius;
    if (pill) r = h / 2;
    else if (h < 12) r = 0;
    else if (h < 24 && r > 3) r = T->chamfer ? 4 : 3;
    const bool chamfer = T->chamfer && !pill;
    const int32_t sy = shadow > 0 ? (shadow * T->shadow + 3) / 4 : 0;
    const int32_t sx = shadow > 0 ? (shadow * T->shadowR + 3) / 4 : 0;
    if (ring)
        BevelBoxR(x - 3, y - 3, w + 6, h + 6, r + 3, chamfer, 0, 0, ring, ring, 0, 0, 0, sx, sy, T->shadowCol, false);
    BevelBoxR(x, y, w, h, r, chamfer, t, edge, top, bottom, T->bevel ? 2 : 0, hi, shade, ring ? 0 : sx, ring ? 0 : sy,
              T->shadowCol, false);
    if (T->gloss && h > 2 * t + 6)
    {
        const int32_t fr = r > t ? r - t : 0, rr = fr > (h - 2 * t) / 2 ? (h - 2 * t) / 2 : fr;
        UiDraw_Rect(x + t + rr, y + t + 1, w - 2 * t - 2 * rr, (h - 2 * t) / 2 - 1, C_GLOSS);
    }
}

// GBA Gen 3 stepped 9-slice frame: 6 px total border + 2 px inner bevel ring (matching .gba-win in index.html)
static void GbaFrameBoxPal(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t outerEdge, uint32_t midRing, uint32_t innerRing,
                           uint32_t innerEdge, uint32_t top, uint32_t bot, uint32_t shade, int32_t shadow)
{
    if (w <= 0 || h <= 0)
        return;
    int32_t r = 6;
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    if (shadow > 0)
        ShapeFill(x, y + shadow, w, h, r, false, UI_RGBA(0, 0, 0, 92), UI_RGBA(0, 0, 0, 92));
    ShapeFill(x, y, w, h, r, false, outerEdge, outerEdge);
    if (w > 4 && h > 4)
    {
        int32_t r2 = r > 1 ? r - 1 : 0;
        if (r2 > (h - 4) / 2) r2 = (h - 4) / 2;
        if (r2 > (w - 4) / 2) r2 = (w - 4) / 2;
        ShapeFill(x + 2, y + 2, w - 4, h - 4, r2, false, midRing, midRing);
    }
    if (w > 8 && h > 8)
    {
        int32_t r1 = r > 2 ? r - 2 : 0;
        if (r1 > (h - 8) / 2) r1 = (h - 8) / 2;
        if (r1 > (w - 8) / 2) r1 = (w - 8) / 2;
        ShapeFill(x + 4, y + 4, w - 8, h - 8, r1, false, innerRing, innerRing);
    }
    if (w > 10 && h > 10)
    {
        int32_t r0 = r > 3 ? r - 3 : 0;
        if (r0 > (h - 10) / 2) r0 = (h - 10) / 2;
        if (r0 > (w - 10) / 2) r0 = (w - 10) / 2;
        ShapeFill(x + 5, y + 5, w - 10, h - 10, r0, false, innerEdge, innerEdge);
    }
    const int32_t ix = x + 6, iy = y + 6, iw = w - 12, ih = h - 12;
    if (iw <= 0 || ih <= 0)
        return;
    int32_t ri = r > 4 ? r - 4 : 0;
    if (ri > ih / 2) ri = ih / 2;
    if (ri > iw / 2) ri = iw / 2;
    if (iw > 6 && ih > 6 && shade != 0)
    {
        ShapeFill(ix, iy, iw, ih, ri, false, shade, shade);
        ShapeFill(ix + 2, iy + 2, iw - 4, ih - 4, ri > 1 ? ri - 1 : 0, false, top, bot);
    }
    else
    {
        ShapeFill(ix, iy, iw, ih, ri, false, top, bot);
    }
}

static void GbaFrameBox(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bot, uint32_t shade, int32_t shadow)
{
    GbaFrameBoxPal(x, y, w, h, GBA_CHARCOAL, GBA_MINT, GBA_EMERALD, GBA_CHARCOAL, top, bot, shade, shadow);
}

// Dialogue / choice box (design/stitch .dialog): cream gradient, white highlight, warm shade,
// dark rounded edge and a soft drop shadow
void UiW_DialogBox(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (T->gbaFrame)
    {
        UiW_GbaMsg(x, y, w, h, false, false);
        return;
    }
    BevelBox(x, y, w, h, T->edge > 3 ? T->edge : 3, T->dlgEdge, T->dlgTop, T->dlgBot, 2, C_WHITE, T->dlgShade, 6);
    if (T->accent)
        UiDraw_Rect(x, y + (T->chamfer ? T->radius : 0), 8, h - (T->chamfer ? 2 * T->radius : 0), T->accent);
}

// Sunken info box inside a panel (labels and values of the summaries)
void UiW_Inset(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (T->gbaFrame)
    {
        UiW_GbaBox(x, y, w, h, 3, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        return;
    }
    BevelBox(x, y, w, h, 2, T->paperShade, T->paper1, T->paper1, 2, T->paperShade, C_WHITE, 0);
}

// Key cap: bevelled (light top-left, dark bottom-right) or, in flat themes, a shaped box
static void KeyCap(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (T->bevel || T->gloss)
    {
        const int32_t r = T->radius >= 10 ? (h >= 40 ? 10 : 6) : 0;
        if (r > 0)
        {
            BevelBoxR(x, y, w, h, r, T->chamfer, 2, T->keyLight, T->keyTop, T->keyBot, 0, 0, 0, 0, 2, C_SHADOW, T->gloss);
            return;
        }
        UiDraw_Rect(x + 1, y + h, w - 2, 2, C_SHADOW);
        UiDraw_RectV(x, y, w, h, T->keyTop, T->keyBot);
        UiDraw_Rect(x, y, w, 2, T->keyLight);
        UiDraw_Rect(x, y, 2, h, T->keyLight);
        UiDraw_Rect(x, y + h - 2, w, 2, T->keyDark);
        UiDraw_Rect(x + w - 2, y, 2, h, T->keyDark);
        return;
    }
    const int32_t r = T->chamfer ? 4 : (T->radius >= 10 ? h / 2 : 3);
    const int32_t t = T->edge >= 4 ? 3 : 0;
    BevelBoxR(x, y, w, h, r, T->chamfer, t, T->keyDark, T->keyTop, T->keyBot, 0, 0, 0, 0, t ? 2 : 0, T->keyDark, false);
}

// Bevelled key; the footer one uses the footer type (Press Start 2P 16)
static int32_t KeyEx(int32_t x, int32_t y, const char *label, bool small)
{
    const int32_t w = (small ? UiText_Width(UI_FACE_PS2P, UI_FOOT_PX, label) + 12 : UiText_Width(UI_FACE_VT323, UI_BODY_PX, label) + 14);
    KeyCap(x, y, w, 24);
    if (small)
        UiText_DrawMid(UI_FACE_PS2P, UI_FOOT_PX, x + 6, y, 24, label, T->keyInk);
    else
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, x + 7, y, 24, label, T->keyInk);
    return w;
}

static int32_t Key(int32_t x, int32_t y, const char *label)
{
    return KeyEx(x, y, label, false);
}

// Pixel circle (spans merged by equal half-width), radius r, centred at (cx, cy)
static void Disc(int32_t cx, int32_t cy, int32_t r, uint32_t colour)
{
    int32_t runStart = 0, runHw = -1;
    for (int32_t dy = -r; dy <= r; dy++)
    {
        int32_t hw = 0;
        while ((hw + 1) * (hw + 1) + dy * dy <= r * r + r)
            hw++;
        if (hw != runHw)
        {
            if (runHw >= 0)
                UiDraw_Rect(cx - runHw, cy + runStart, runHw * 2 + 1, dy - runStart, colour);
            runStart = dy;
            runHw = hw;
        }
    }
    UiDraw_Rect(cx - runHw, cy + runStart, runHw * 2 + 1, r + 1 - runStart, colour);
}

void UiW_Glyph(int32_t x, int32_t y, uint8_t kind)
{
    static const uint32_t ring[4] = { UI_RGBA(91, 127, 214, 255), UI_RGBA(214, 81, 81, 255), UI_RGBA(47, 167, 126, 255), UI_RGBA(194, 102, 168, 255) };
    static const uint32_t ink[4] = { UI_RGBA(143, 180, 255, 255), UI_RGBA(255, 122, 122, 255), UI_RGBA(95, 224, 176, 255), UI_RGBA(244, 155, 216, 255) };
    const int32_t cx = x + 12, cy = y + 12;
    Disc(cx, cy + 2, 12, C_SHADOW);
    Disc(cx, cy, 12, ring[kind & 3]);
    Disc(cx, cy, 10, UI_RGBA(27, 30, 36, 255));
    const uint32_t c = ink[kind & 3];
    switch (kind & 3)
    {
    case UI_GLYPH_CROSS:
        for (int32_t i = -4; i <= 3; i++)
        {
            UiDraw_Rect(cx + i, cy + i, 2, 2, c);
            UiDraw_Rect(cx + i, cy - i - 1, 2, 2, c);
        }
        break;
    case UI_GLYPH_CIRCLE:
        Disc(cx, cy, 5, c);
        Disc(cx, cy, 3, UI_RGBA(27, 30, 36, 255));
        break;
    case UI_GLYPH_TRIANGLE:
        for (int32_t i = 0; i < 8; i++)
        {
            const int32_t half = i / 2 + 1;
            if (i == 7 || i == 0)
                UiDraw_Rect(cx - half, cy - 4 + i, half * 2, 1, c);
            else
            {
                UiDraw_Rect(cx - half, cy - 4 + i, 2, 1, c);
                UiDraw_Rect(cx + half - 2, cy - 4 + i, 2, 1, c);
            }
        }
        break;
    default:
        UiDraw_Frame(cx - 4, cy - 4, 9, 9, 2, c);
        break;
    }
}

// ---------- Frame, header, footer ----------

void UiW_FullFrame(void)
{
    // Translucent frame over the 3D world (decision D3), then the bars with their shadows
    UiDraw_Rect(0, HEADER_H, UI_SCREEN_W, UI_SCREEN_H - HEADER_H - FOOTER_H, UI_COL_FRAME0_A);
    if (T->gbaFrame)
    {
        for (int32_t gy = HEADER_H; gy < UI_SCREEN_H - FOOTER_H; gy += 16)
            UiDraw_Rect(0, gy, UI_SCREEN_W, 1, UI_RGBA(115, 205, 164, 18));
        for (int32_t gx = 0; gx < UI_SCREEN_W; gx += 16)
            UiDraw_Rect(gx, HEADER_H, 1, UI_SCREEN_H - HEADER_H - FOOTER_H, UI_RGBA(115, 205, 164, 14));
    }
    else if (T->stripes)
    {
        // Diagonal stripes: 14 px bands every 28 px, clipped to the body
        const float top = HEADER_H, bot = UI_SCREEN_H - FOOTER_H, hgt = bot - top;
        UiDraw_PushClip(0, HEADER_H, UI_SCREEN_W, UI_SCREEN_H - HEADER_H - FOOTER_H);
        for (int32_t sx = -(int32_t)hgt; sx < UI_SCREEN_W; sx += 28)
        {
            const float xs[4] = { (float)sx + hgt, (float)sx + hgt + 14, (float)sx + 14, (float)sx };
            const float ys[4] = { top, top, bot, bot };
            UiDraw_Poly4(xs, ys, UI_RGBA(255, 255, 255, 10));
        }
        UiDraw_PopClip();
    }
    UiDraw_RectV(0, 0, UI_SCREEN_W, HEADER_H - 2, T->barTop, T->barBot);
    if (T->gloss)
        UiDraw_Rect(0, 1, UI_SCREEN_W, HEADER_H / 2 - 2, C_GLOSS);
    UiDraw_Rect(0, HEADER_H - 2, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    if (T->gbaFrame)
    {
        UiDraw_Rect(0, HEADER_H, UI_SCREEN_W, 1, GBA_CHARCOAL);
        UiDraw_Rect(0, HEADER_H + 1, UI_SCREEN_W, 2, C_SHADOW);
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 3, UI_SCREEN_W, 2, C_SHADOW);
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 1, UI_SCREEN_W, 1, GBA_CHARCOAL);
    }
    else
    {
        UiDraw_Rect(0, HEADER_H, UI_SCREEN_W, 2, C_SHADOW);
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 2, UI_SCREEN_W, 2, C_SHADOW);
    }
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_RectV(0, UI_SCREEN_H - FOOTER_H + 2, UI_SCREEN_W, FOOTER_H - 2, T->barBot, T->barTop);
}

void UiW_FooterBar(void)
{
    if (T->gbaFrame)
    {
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 3, UI_SCREEN_W, 2, C_SHADOW);
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 1, UI_SCREEN_W, 1, GBA_CHARCOAL);
    }
    else
        UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 2, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_RectV(0, UI_SCREEN_H - FOOTER_H + 2, UI_SCREEN_W, FOOTER_H - 2, T->barBot, T->barTop);
}

int32_t UiW_Capsule(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 2, T->capEdge, T->capTop, T->capBot, 1, T->capHi, T->capBot, 2);
    Gloss(x, y, w, h, 2);
    return w;
}

static void DrawGbaSectionPill(int32_t x, int32_t y, int32_t w, int32_t h, bool lens, uint16_t iconSpr,
                               const char *title, uint32_t border, uint32_t bgTop, uint32_t bgBot,
                               uint32_t ink, bool textShadow, bool barAccent)
{
    BevelBoxR(x, y, w, h, 5, false, 2, border, bgTop, bgBot, 1, UI_RGBA(255, 255, 255, 96),
              UI_RGBA(0, 0, 0, 56), 0, 2, UI_RGBA(0, 0, 0, 120), false);
    if (barAccent)
        UiDraw_Rect(x + 8, y + (h - 18) / 2, 4, 18, GBA_YELLOW);
    if (lens)
    {
        const int32_t lcx = x + 26, lcy = y + h / 2;
        Disc(lcx, lcy, 10, UI_RGBA(40, 56, 64, 255));
        Disc(lcx, lcy, 8, UI_RGBA(0, 88, 168, 255));
        Disc(lcx - 1, lcy - 1, 5, UI_RGBA(56, 184, 248, 255));
        UiDraw_Rect(lcx - 3, lcy - 4, 3, 3, C_WHITE);
    }
    else if (iconSpr != 0)
    {
        uint16_t sx, sy, sw, sh;
        if (UiAssets_Sprite(iconSpr, &sx, &sy, &sw, &sh))
        {
            const int32_t drawW = sw > 24 ? 22 : (int32_t)sw;
            const int32_t drawH = sh > 24 ? 22 : (int32_t)sh;
            UiDraw_Quad(x + 16, y + (h - drawH) / 2, drawW, drawH, sx, sy, sx + sw, sy + sh, C_WHITE);
        }
    }
    const int32_t tx = x + 42;
    UiDraw_PushClip(tx, y, w - 50, h);
    if (textShadow)
        UiText_DrawMid(UI_FACE_PS2P, 16, tx + 1, y + 1, h, title, UI_RGBA(24, 32, 32, 200));
    UiText_DrawMid(UI_FACE_PS2P, 16, tx, y, h, title, ink);
    UiDraw_PopClip();
}

void UiW_HeaderTitle(const char *title)
{
    if (T->gbaFrame)
    {
        DrawGbaSectionPill(12, 8, 244, 32, false, UI_SPR_BALL, title, GBA_CHARCOAL,
                           UI_RGBA(44, 106, 136, 255), UI_RGBA(32, 82, 108, 255), C_WHITE, true, true);
        return;
    }
    const int32_t w = UiText_Width(UI_FACE_PS2P, 16, title) + 48;
    UiW_Capsule(16, 8, w, 32);
    UiDraw_Sprite(UI_SPR_BALL, 24, 16, 1);
    UiText_DrawMid(UI_FACE_PS2P, 16, 48, 8, 32, title, T->title);
}

void UiW_HeaderValue(const char *text, uint32_t colour)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, text) + 24;
    UiW_Capsule(UI_SCREEN_W - 16 - w, 8, w, 32);
    UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, UI_SCREEN_W - 16 - w + 12, 8, 32, text, colour);
}

int32_t UiW_TabsPad(int32_t x, const char *const *labels, int32_t count, int32_t current, int32_t *tabX, int32_t *tabW, int32_t pad)
{
    int32_t total = 4;
    total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, "L") + 14 + 4;
    for (int32_t i = 0; i < count; i++)
        total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, labels[i]) + 2 * pad + 2;
    total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, "R") + 14 + 4;
    // Sunken rail
    if (T->gbaFrame)
    {
        BevelBoxR(x, 6, total, 36, 4, false, 2, UI_RGBA(15, 20, 22, 255), UI_RGBA(23, 30, 33, 255), UI_RGBA(23, 30, 33, 255), 0, 0, 0, 0, 0, 0, false);
    }
    else
    {
        const int32_t r = T->chamfer ? 6 : T->radius >= 10 ? 12 : 1;
        BevelBoxR(x, 6, total, 36, r, T->chamfer, 2, T->railEdge, T->rail, T->rail, 0, 0, 0, 0, 0, 0, false);
        if (T->bevel)
            UiDraw_Rect(x + 2 + r, 8, total - 4 - 2 * r, 2, C_SHADOW);
    }
    int32_t cx = x + 4;
    cx += Key(cx, 12, "L") + 4;
    for (int32_t i = 0; i < count; i++)
    {
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, labels[i]) + 2 * pad;
        if (i == current)
        {
            if (T->gbaFrame)
            {
                BevelBoxR(cx, 10, w, 28, 3, false, 2, GBA_YELLOW, UI_RGBA(255, 246, 205, 255), UI_RGBA(248, 228, 152, 255), 0, 0, 0, 0, 0, 0, false);
            }
            else
            {
                const int32_t r = T->chamfer ? 5 : T->radius >= 10 ? 10 : 3;
                BevelBoxR(cx, 10, w, 28, r, T->chamfer, 2, T->tabEdge, T->tabTop, T->tabBot, T->bevel ? 1 : 0, T->tabHi, T->tabBot, 0, 0, 0, T->gloss);
            }
            UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, cx + pad, 10, 28, labels[i], T->gbaFrame ? UI_RGBA(41, 49, 49, 255) : T->tabInk);
        }
        else
            UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, cx + pad, 10, 28, labels[i], UI_COL_ON_SOFT);
        if (tabX) tabX[i] = cx;
        if (tabW) tabW[i] = w;
        cx += w + 2;
    }
    cx += 2;
    cx += Key(cx, 12, "R");
    return x + total;
}

int32_t UiW_Tabs(int32_t x, const char *const *labels, int32_t count, int32_t current, int32_t *tabX, int32_t *tabW)
{
    return UiW_TabsPad(x, labels, count, current, tabX, tabW, 12);
}

// Word-wrapped VT323 text in a box of width w; returns the number of lines drawn
int32_t UiW_TextWrapped(int32_t x, int32_t y, int32_t w, int32_t lineH, const char *text, uint32_t colour, int32_t maxLines)
{
    return UiW_TextWrappedF(UI_FACE_VT323, UI_BODY_PX, x, y, w, lineH, text, colour, maxLines);
}

int32_t UiW_TextWrappedF(uint8_t face, uint8_t px, int32_t x, int32_t y, int32_t w, int32_t lineH, const char *text, uint32_t colour, int32_t maxLines)
{
    char line[160];
    int32_t lines = 0;
    // Translate the complete help text before splitting it into visual lines.
    // Translating each wrapped fragment cannot match dictionary entries.
    const char *p = UiLocale_Translate(text);
    if (p == NULL)
        return 0;
    while (*p && lines < maxLines)
    {
        size_t len = 0, lastSpace = 0;
        while (p[len] && len < sizeof(line) - 1)
        {
            memcpy(line, p, len + 1);
            line[len + 1] = 0;
            if (UiText_Width(face, px, line) > w)
                break;
            if (p[len] == ' ')
                lastSpace = len;
            len++;
        }
        if (p[len] && lastSpace > 0)
            len = lastSpace;
        memcpy(line, p, len);
        line[len] = 0;
        if (colour != 0)                           // colour 0: only count the lines
            UiText_Draw(face, px, x, y + lines * lineH, line, colour);
        lines++;
        p += len;
        while (*p == ' ')
            p++;
    }
    return lines;
}

// Scrolling marquee for all fixed-width translated labels. Pause at either
// edge so the beginning/end remains readable. Clip only during the transition.
void UiW_TextMarquee(uint8_t face, uint8_t px, int32_t x, int32_t y,
                     int32_t w, int32_t h, const char *label, uint32_t color,
                     uint32_t tick)
{
    const char *text = UiLocale_Translate(label);
    if (text == NULL || w <= 8)
        return;
    const int32_t pad = 6;
    const int32_t available = w - 2 * pad;
    const int32_t width = UiText_Width(face, px, text);
    int32_t drawX = x + pad;
    if (width <= available)
        drawX = x + (w - width) / 2;
    else
    {
        const int32_t overflow = width - available;
        // 40 frames of rest at each end; about one pixel per 2 frames.
        const int32_t travel = overflow * 2;
        const int32_t cycle = 80 + 2 * travel;
        const int32_t t = (int32_t)(tick % (uint32_t)cycle);
        const int32_t offset = t < 40 ? 0 :
                               t < 40 + travel ? (t - 40) / 2 :
                               t < 80 + travel ? overflow :
                               overflow - (t - 80 - travel) / 2;
        drawX -= offset;
    }
    UiDraw_PushClip(x + pad, y, available, h);
    UiText_DrawMid(face, px, drawX, y, h, text, color);
    UiDraw_PopClip();
}

static void DrawGbaFooterItem(int32_t *x, int32_t y, const char *key, const char *label, bool gold)
{
    const int32_t capW = UiW_GbaKeyCapWidth(key, 24);
    UiW_GbaKeyCap(*x, y, 24, key, gold);
    *x += capW + 6;
    if (label && label[0])
    {
        UiText_DrawMid(UI_FACE_VT323, 24, *x + 1, y + 1, 24, label, UI_RGBA(16, 20, 22, 220));
        *x += UiText_DrawMid(UI_FACE_VT323, 24, *x, y, 24, label, C_WHITE) + 18;
    }
}

void UiW_FooterGlyph(int32_t *x, uint8_t glyph, const char *label)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 8;
    if (T->gbaFrame)
    {
        static const char *const kGlyphKeys[4] = { "X", "O", "TRI", "SQ" };
        DrawGbaFooterItem(x, y, kGlyphKeys[glyph & 3u], label, (glyph & 3u) == UI_GLYPH_CIRCLE);
        return;
    }
    UiW_Glyph(*x, y, glyph);
    *x += 30;
    *x += UiText_DrawMid(UI_FACE_PS2P, UI_FOOT_PX, *x, y, 24, label, UI_COL_ON_FRAME) + 18;
}

void UiW_FooterClock(const char *clock)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 5;
    const int32_t w = UiText_InkWidth(UI_FACE_PS2P, UI_FOOT_PX, clock) + 26;
    const int32_t x = UI_SCREEN_W - 8 - w;
    UiW_Capsule(x, y, w, 30);
    UiText_DrawCentred(UI_FACE_PS2P, UI_FOOT_PX, x, y, w, 30, clock, UI_COL_ON_FRAME);
}

void UiW_FooterKey(int32_t *x, const char *key, const char *label)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 8;
    if (T->gbaFrame)
    {
        const bool gold = strcmp(key, "SELECT") == 0 || strcmp(key, "START") == 0;
        DrawGbaFooterItem(x, y, key, label, gold);
        return;
    }
    *x += KeyEx(*x, y, key, true) + 6;
    *x += UiText_DrawMid(UI_FACE_PS2P, UI_FOOT_PX, *x, y, 24, label, UI_COL_ON_FRAME) + 18;
}

// ---------- Boxes ----------

// Left accent bar of the theme (RED), between the cut corners
static void AccentBar(int32_t x, int32_t y, int32_t h)
{
    if (!T->accent)
        return;
    const int32_t r = T->chamfer && h >= 24 ? T->radius : 0;
    UiDraw_Rect(x, y + r, 6, h - 2 * r, T->accent);
}

void UiW_Panel(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (T->gbaFrame)
    {
        GbaFrameBox(x, y, w, h, T->paper0, T->paper0, T->paperShade, 4);
        return;
    }
    BevelBox(x, y, w, h, T->edge, T->paperEdge, T->paper0, T->paper0, 2, C_WHITE, T->paperShade, 4);
    AccentBar(x, y, h);
}

void UiW_PanelDark(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (T->gbaFrame)
    {
        GbaFrameBox(x, y, w, h, T->darkTop, T->darkBot, GBA_DARK_SH, 4);
        return;
    }
    BevelBox(x, y, w, h, T->edge > 3 ? T->edge : 3, T->darkEdge, T->darkTop, T->darkBot, 1, T->darkHi, T->darkBot, 4);
    Gloss(x, y, w, h, 3);
    AccentBar(x, y, h);
}

// Head strip inside a paper panel ("LEDGER: 05 / 12 ITEMS")
void UiW_PanelHead(int32_t x, int32_t y, int32_t w, const char *label, const char *right)
{
    if (T->gbaFrame)
    {
        UiDraw_Rect(x + 6, y + 6, w - 12, 33, T->paperStrip);
        UiDraw_Rect(x + 6, y + 39, w - 12, 2, GBA_CHARCOAL);
        int32_t tagW = UiText_Width(UI_FACE_PS2P, 16, label) + 28;
        if (tagW > w - 24)
            tagW = w - 24;
        if (tagW > 0)
        {
            UiDraw_RectV(x + 6, y + 6, tagW, 33, UI_RGBA(62, 144, 112, 255), UI_RGBA(40, 112, 84, 255));
            const float rx0 = (float)(x + 6 + tagW), rx1 = (float)(x + 6 + tagW + 12);
            const float xs[4] = { rx0, rx1, rx0, rx0 };
            const float ys[4] = { (float)(y + 6), (float)(y + 39), (float)(y + 39), (float)(y + 39) };
            UiDraw_Poly4(xs, ys, UI_RGBA(40, 112, 84, 255));
            for (int32_t k = 0; k < 3; k++)
                UiDraw_Rect(x + 9, y + 11 + k * 8, 2, 5, UI_RGBA(168, 230, 207, 255));
        }
        UiText_DrawMid(UI_FACE_PS2P, 16, x + 16, y + 6, 33, label, T->stripInk);
        if (right)
            UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, x + w - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, right), y + 6, 33, right, T->stripSoft);
        return;
    }
    // Square strip under the panel edge, kept clear of the rounded / cut top corners
    const int32_t e = T->edge, r = T->radius > e ? T->radius - e : 0;
    ShapeFill(x + e, y + e, w - 2 * e, 2 * r + 2, r, T->chamfer, T->paperStrip, T->paperStrip);
    UiDraw_Rect(x + e, y + e + r, w - 2 * e, 39 - e - r, T->paperStrip);
    if (T->bevel)
        UiDraw_Rect(x + e + r, y + e, w - 2 * e - 2 * r, 2, C_WHITE);
    UiDraw_Rect(x + e, y + 39, w - 2 * e, 2, T->edge >= 4 ? T->paperEdge : T->paperShade);
    AccentBar(x, y, 41);
    UiText_DrawMid(UI_FACE_PS2P, 16, x + 16, y + T->edge, 39 - T->edge, label, T->stripInk);
    if (right)
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, x + w - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, right), y + T->edge, 39 - T->edge, right, T->stripSoft);
}

void UiW_Window(int32_t x, int32_t y, int32_t w, int32_t h, const char *title)
{
    if (T->gbaFrame)
    {
        GbaFrameBox(x, y, w, h, T->paper0, T->paper0, T->paperShade, 6);
        UiDraw_RectV(x + 6, y + 6, w - 12, 35, T->winTop, T->winBot);
        UiDraw_Rect(x + 6, y + 6, w - 12, 1, T->capHi);
        UiDraw_Rect(x + 6, y + 41, w - 12, 2, GBA_CHARCOAL);
        UiText_DrawMid(UI_FACE_PS2P, 16, x + 16, y + 6, 35, title, T->winInk);
        return;
    }
    const int32_t e = T->edge;
    BevelBox(x, y, w, h, e, T->paperEdge, T->paper0, T->paper0, 2, C_WHITE, T->paperShade, 6);
    // Title bar: same corner shape as the window on top, square below
    const int32_t r = T->radius > e ? T->radius - e : 0;
    ShapeFill(x + e, y + e, w - 2 * e, 2 * r + 2, r, T->chamfer, T->winTop, T->winTop);
    UiDraw_RectV(x + e, y + e + r, w - 2 * e, 41 - e - r, T->winTop, T->winBot);
    if (T->bevel)
        UiDraw_Rect(x + e + r, y + e, w - 2 * e - 2 * r, 1, T->capHi);
    if (T->gloss)
        UiDraw_Rect(x + e + r, y + e + 1, w - 2 * e - 2 * r, 18, C_GLOSS);
    UiDraw_Rect(x + e, y + 41, w - 2 * e, 2, T->edge >= 4 ? T->paperEdge : T->darkEdge);
    if (T->accent)
        UiDraw_Rect(x + e, y + 41, w - 2 * e, 3, T->accent);
    UiText_DrawMid(UI_FACE_PS2P, 16, x + 16, y + T->edge, 41 - T->edge, title, T->winInk);
}

// ---------- Rows, chips, bars, buttons ----------

void UiW_Arrow(int32_t x, int32_t y, uint32_t colour)
{
    // ▶ in pixel steps: 12 px tall, 8 px wide
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t len = (i + 1) * 4 / 3 + 1;
        UiDraw_Rect(x, y + i, len, 1, colour);
        UiDraw_Rect(x, y + 11 - i, len, 1, colour);
    }
}

void UiW_Row(int32_t x, int32_t y, int32_t w, int32_t h, bool selected, int32_t index)
{
    if (selected)
    {
        if (T->gbaFrame)
        {
            UiDraw_Rect(x, y, w, h - 2, (index & 1) ? T->rowB : T->rowA);
            const int32_t r = 4 < (h - 2) / 2 ? 4 : (h - 2) / 2;
            BevelBoxR(x, y, w, h - 2, r, false, 2, T->selEdge, T->selTop, T->selBot, 1, C_WHITE, 0, 0, 0, 0, false);
            UiW_Arrow(x + 9, y + (h - 12) / 2 + 1, UI_GBA_SHAD_LIGHT);
            UiW_Arrow(x + 8, y + (h - 12) / 2, T->selArrow);
        }
        else if (T->selRadius > 0)
        {
            // Pill (CRYSTAL, TROPICAL): outlined and rounded, over the row background
            UiDraw_Rect(x, y, w, h - 2, (index & 1) ? T->rowB : T->rowA);
            const int32_t r = T->selRadius < (h - 2) / 2 ? T->selRadius : (h - 2) / 2;
            BevelBoxR(x, y, w, h - 2, r, false, T->edge >= 4 ? 3 : 2, T->selEdge, T->selTop, T->selBot, 0, 0, 0, 0, 0, 0, T->gloss);
            UiW_Arrow(x + 10, y + (h - 14) / 2, T->selArrow);
        }
        else if (T->chamfer)
        {
            // Block with a cut right end (RED, ROYAL)
            const int32_t c = h - 2 < 20 ? 6 : 12;
            UiDraw_RectV(x, y, w - c, h - 2, T->selTop, T->selBot);
            const float xs[4] = { (float)(x + w - c), (float)(x + w), (float)(x + w - c), (float)(x + w - c) };
            const float ys[4] = { (float)y, (float)y, (float)(y + h - 2), (float)(y + h - 2) };
            UiDraw_Poly4(xs, ys, T->selBot);
            UiDraw_Rect(x, y + h - 2, w - c, 2, T->selEdge);
            UiW_Arrow(x + 8, y + (h - 12) / 2, T->selArrow);
        }
        else
        {
            UiDraw_RectV(x, y, w, h - 2, T->selTop, T->selBot);
            UiDraw_Rect(x, y, w, 1, C_WHITE);
            UiDraw_Rect(x, y + h - 2, w, 2, T->selEdge);
            UiW_Arrow(x + 8, y + (h - 12) / 2, T->selArrow);
        }
    }
    else
    {
        UiDraw_Rect(x, y, w, h - 2, (index & 1) ? T->rowB : T->rowA);
        UiDraw_Rect(x, y + h - 2, w, 2, T->rowLine);
    }
}

int32_t UiW_Chip(int32_t x, int32_t y, const char *label, bool amber)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, label) + 16;
    const int32_t r = T->chamfer ? 4 : T->radius >= 10 ? 12 : 3;
    if (amber)
        BevelBoxR(x, y, w, 26, r, T->chamfer, 2, T->tabEdge, T->tabTop, T->tabBot, T->bevel ? 1 : 0, T->tabHi, T->tabBot, 0, 0, 0, false);
    else
        BevelBoxR(x, y, w, 26, r, T->chamfer, 2, T->chipEdge, T->chipTop, T->chipBot, T->bevel ? 1 : 0, T->capHi, T->chipBot, 0, 0, 0, false);
    UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, x + 8, y, 26, label, amber ? T->tabInk : T->chipInk);
    return w;
}

void UiW_Bar(int32_t x, int32_t y, int32_t w, int32_t h, int32_t percent, uint32_t top, uint32_t bottom)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    UiDraw_Rect(x + 1, y, w - 2, h, UI_RGBA(26, 38, 32, 255));
    UiDraw_Rect(x, y + 1, w, h - 2, UI_RGBA(26, 38, 32, 255));
    UiDraw_Rect(x + 2, y + 2, w - 4, h - 4, UI_COL_BAR_BG);
    UiDraw_Rect(x + 2, y + 2, w - 4, 1, UI_RGBA(255, 255, 255, 26));
    UiDraw_RectV(x + 2, y + 2, (w - 4) * percent / 100, h - 4, top, bottom);
}

void UiW_Button(int32_t x, int32_t y, int32_t w, const char *label, uint8_t colour, bool pressed)
{
    const int32_t dy = pressed ? 2 : 0;
    const int32_t r = T->chamfer ? 8 : T->radius >= 10 ? 20 : 6;
    const int32_t sh = pressed ? 0 : (T->shadowR ? 4 : 2);
    BevelBoxR(x, y + dy, w, 40, r, T->chamfer, T->edge >= 4 ? 3 : 2, T->btnEdge, T->btnTop[colour % 3], T->btnBot[colour % 3],
              T->bevel ? 1 : 0, UI_RGBA(255, 255, 255, 102), UI_RGBA(0, 0, 0, 102), T->shadowR ? sh : 0, sh,
              T->shadowR ? T->shadowCol : C_SHADOW, T->gloss);
    UiText_DrawMid(UI_FACE_PS2P, 16, x + (w - UiText_Width(UI_FACE_PS2P, 16, label)) / 2, y + dy, 40, label, T->btnInk);
}

void UiW_Strip3(int32_t x, int32_t y, int32_t w, const char *const labels[3], const char *const values[3])
{
    UiDraw_Rect(x, y, w, 64, T->rowLine);
    UiDraw_Rect(x + 2, y + 2, w - 4, 60, T->paper1);
    const int32_t cw = (w - 4) / 3;
    for (int32_t i = 0; i < 3; i++)
    {
        const int32_t cx = x + 2 + i * cw;
        if (i > 0)
            UiDraw_Rect(cx - 1, y + 2, 2, 60, T->rowLine);
        UiText_Draw(UI_FACE_PS2P, 16, cx + (cw - UiText_Width(UI_FACE_PS2P, 16, labels[i])) / 2, y + 8, labels[i], UI_COL_INK_SOFT);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx + (cw - UiText_Width(UI_FACE_VT323, UI_BODY_PX, values[i])) / 2, y + 26, values[i], UI_COL_INK);
    }
}

void UiW_Stepper(int32_t x, int32_t y, int32_t value, int32_t *minusX, int32_t *plusX)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "x%02d", (int)value);
    for (int32_t k = 0; k < 2; k++)
    {
        const int32_t kx = x + k * 136;
        KeyCap(kx, y, 40, 40);
        UiText_DrawMid(UI_FACE_VT323, 32, kx + (40 - UiText_Width(UI_FACE_VT323, 32, k ? "+" : "-")) / 2, y, 40, k ? "+" : "-", T->keyInk);
    }
    UiDraw_Rect(x + 48, y, 80, 40, T->paperShade);
    UiDraw_Rect(x + 50, y + 2, 76, 36, T->paper1);
    UiDraw_Rect(x + 50, y + 2, 76, 2, UI_RGBA(0, 0, 0, 31));
    UiText_DrawMid(UI_FACE_VT323, 32, x + 48 + (80 - UiText_Width(UI_FACE_VT323, 32, buf)) / 2, y, 40, buf, UI_COL_INK);
    if (minusX) *minusX = x;
    if (plusX) *plusX = x + 136;
}

void UiW_ScrollBar(int32_t x, int32_t y, int32_t h, int32_t first, int32_t visible, int32_t total)
{
    if (total <= visible || total <= 0)
        return;
    int32_t barH = h * visible / total;
    if (barH < 24) barH = 24;
    const int32_t top = y + (h - barH) * first / (total - visible);
    UiDraw_Rect(x, top, 7, barH, UI_RGBA(62, 74, 67, 255));
    UiDraw_RectV(x + 1, top + 1, 5, barH - 2, UI_RGBA(148, 163, 154, 255), UI_RGBA(94, 110, 100, 255));
}

void UiW_Toast(const char *text)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, text) + 28;
    const int32_t x = (UI_SCREEN_W - w) / 2, y = 446;
    UiW_PanelDark(x, y, w, 42);
    UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, x + 14, y, 42, text, UI_COL_ON_FRAME);
}

// ---------- GBA Gen 3 screen chrome and widgets (design/stitch/gba_vita/index.html) ----------

static void DrawBits(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *rows, int32_t scale, uint32_t col)
{
    for (int32_t r = 0; r < h; r++)
    {
        const uint16_t bits = rows[r];
        int32_t c = 0;
        while (c < w)
        {
            while (c < w && !(bits & (1u << (w - 1 - c))))
                c++;
            if (c >= w)
                break;
            const int32_t c0 = c;
            while (c < w && (bits & (1u << (w - 1 - c))))
                c++;
            UiDraw_Rect(x + c0 * scale, y + r * scale, (c - c0) * scale, scale, col);
        }
    }
}

void UiW_GbaArrowH(int32_t x, int32_t y, bool right, uint32_t colour)
{
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t len = (i + 1) * 4 / 3 + 1;
        const int32_t rx = right ? x : (x + 8 - len);
        UiDraw_Rect(rx, y + i, len, 1, colour);
        UiDraw_Rect(rx, y + 11 - i, len, 1, colour);
    }
}

void UiW_GbaArrowV(int32_t x, int32_t y, bool down, uint32_t colour)
{
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t ry = down ? (y + i) : (y + 5 - i);
        const int32_t w = (5 - i) * 2 + 2;
        UiDraw_Rect(x + i, ry, w, 1, colour);
    }
}

void UiW_GbaStar(int32_t x, int32_t y, uint32_t colour)
{
    static const uint16_t kStar[9] = {
        0x010, 0x038, 0x038, 0x1FF, 0x0FE, 0x07C, 0x0EE, 0x183, 0x000
    };
    DrawBits(x, y, 9, 8, kStar, 2, colour);
}

void UiW_GbaMark(int32_t x, int32_t y, uint8_t shape, uint32_t colour)
{
    static const uint16_t kCircle[7] = { 0x1C, 0x3E, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C };
    static const uint16_t kSquare[7] = { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F };
    static const uint16_t kTri[7]    = { 0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x7F, 0x7F };
    static const uint16_t kHeart[7]  = { 0x36, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C, 0x08 };
    const uint16_t *const kShapes[4] = { kCircle, kSquare, kTri, kHeart };
    DrawBits(x, y, 7, 7, kShapes[shape & 3u], 2, colour);
}

int32_t UiW_GbaKeyCapWidth(const char *key, int32_t h)
{
    if (strcmp(key, "X") == 0 || strcmp(key, "O") == 0 || strcmp(key, "TRI") == 0
        || strcmp(key, "SQ") == 0 || strcmp(key, "LR") == 0 || strcmp(key, "UD") == 0)
        return h;
    return UiText_Width(UI_FACE_VT323, 24, key) + 14;
}

void UiW_GbaKeyCap(int32_t x, int32_t y, int32_t h, const char *key, bool gold)
{
    const int32_t w = UiW_GbaKeyCapWidth(key, h);
    const uint32_t top = gold ? UI_RGBA(240, 184, 40, 255) : UI_RGBA(232, 32, 76, 255);
    const uint32_t bot = gold ? UI_RGBA(196, 136, 16, 255) : UI_RGBA(184, 16, 52, 255);
    const uint32_t hi  = gold ? UI_RGBA(255, 244, 160, 255) : UI_RGBA(255, 136, 164, 255);
    const uint32_t sh  = gold ? UI_RGBA(120, 72, 0, 140) : UI_RGBA(72, 0, 16, 150);
    BevelBoxR(x, y, w, h, 4, false, 2, UI_RGBA(24, 32, 32, 255), top, bot, 0, 0, 0, 0, 2, UI_RGBA(16, 20, 22, 200), false);
    UiDraw_Rect(x + 3, y + 2, w - 6, 2, hi);
    UiDraw_Rect(x + 3, y + h - 4, w - 6, 2, sh);

    static const uint16_t kCross[7]  = { 0x41, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x41 };
    static const uint16_t kCircle[7] = { 0x1C, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1C };
    static const uint16_t kTri[7]    = { 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x7F };
    static const uint16_t kSq[7]     = { 0x7F, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7F };
    static const uint16_t kLR[7]     = { 0x00, 0x24, 0x66, 0xE7, 0x66, 0x24, 0x00 };
    static const uint16_t kUD[7]     = { 0x08, 0x1C, 0x3E, 0x00, 0x3E, 0x1C, 0x08 };

    const uint32_t ink = gold ? UI_RGBA(48, 32, 0, 255) : C_WHITE;
    const int32_t ix = x + (w - 14) / 2, iy = y + (h - 14) / 2;
    if (strcmp(key, "X") == 0) DrawBits(ix, iy, 7, 7, kCross, 2, ink);
    else if (strcmp(key, "O") == 0) DrawBits(ix, iy, 7, 7, kCircle, 2, ink);
    else if (strcmp(key, "TRI") == 0) DrawBits(ix, iy, 7, 7, kTri, 2, ink);
    else if (strcmp(key, "SQ") == 0) DrawBits(ix, iy, 7, 7, kSq, 2, ink);
    else if (strcmp(key, "LR") == 0) DrawBits(x + (w - 16) / 2, iy, 8, 7, kLR, 2, ink);
    else if (strcmp(key, "UD") == 0) DrawBits(ix, iy, 7, 7, kUD, 2, ink);
    else UiText_DrawCentred(UI_FACE_VT323, 24, x, y, w, h, key, ink);
}

void UiW_GbaHelpBadgeY(int32_t *rx, int32_t y, const char *key, const char *action, bool gold)
{
    const int32_t capW = UiW_GbaKeyCapWidth(key, 24);
    const int32_t actW = (action && action[0]) ? UiText_Width(UI_FACE_VT323, 24, action) : 0;
    const int32_t totalW = capW + (actW > 0 ? 5 + actW : 0);
    const int32_t x = *rx - totalW;
    UiW_GbaKeyCap(x, y, 24, key, gold);
    if (actW > 0)
    {
        UiText_DrawMid(UI_FACE_VT323, 24, x + capW + 6, y + 1, 24, action, UI_RGBA(16, 20, 22, 220));
        UiText_DrawMid(UI_FACE_VT323, 24, x + capW + 5, y, 24, action, C_WHITE);
    }
    *rx = x - 12;
}

void UiW_GbaHelpBadge(int32_t *rx, const char *key, const char *action, bool gold)
{
    UiW_GbaHelpBadgeY(rx, 14, key, action, gold);
}

typedef struct
{
    uint32_t body, scan, grid, barAccent, pillBg, pillBorder, pillInk;
    bool stripes;
} GbaBgSpec;

static GbaBgSpec BgSpecFor(uint8_t bgKind)
{
    static const GbaBgSpec kSpecs[7] = {
        [UI_GBA_BG_PARTY]   = { UI_RGBA(43, 107, 100, 152), UI_RGBA(24, 66, 61, 140),  UI_RGBA(115, 205, 164, 22), UI_RGBA(115, 205, 164, 255), UI_RGBA(43, 122, 107, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
        [UI_GBA_BG_BAG]     = { UI_RGBA(40, 91, 117, 152),  UI_RGBA(22, 56, 75, 140),  UI_RGBA(130, 210, 235, 22), UI_RGBA(92, 180, 208, 255),  UI_RGBA(44, 106, 136, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
        [UI_GBA_BG_SUMMARY] = { UI_RGBA(84, 70, 134, 152),  UI_RGBA(52, 40, 98, 140),  UI_RGBA(190, 175, 230, 22), UI_RGBA(164, 148, 222, 255), UI_RGBA(100, 82, 164, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
        [UI_GBA_BG_POKEDEX] = { UI_RGBA(24, 128, 32, 152),  UI_RGBA(40, 184, 56, 20),  UI_RGBA(0, 0, 0, 28),       UI_RGBA(48, 208, 72, 255),   UI_RGBA(248, 248, 248, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(40, 56, 64, 255),   true  },
        [UI_GBA_BG_POKENAV] = { UI_RGBA(24, 48, 76, 152),   UI_RGBA(12, 28, 48, 140),  UI_RGBA(88, 184, 248, 26),  UI_RGBA(88, 184, 248, 255),  UI_RGBA(34, 84, 136, 255),  UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
        [UI_GBA_BG_CARD]    = { UI_RGBA(22, 62, 74, 152),   UI_RGBA(12, 38, 48, 140),  UI_RGBA(232, 192, 80, 22),  UI_RGBA(232, 192, 80, 255),  UI_RGBA(142, 106, 16, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
        [UI_GBA_BG_OPTIONS] = { UI_RGBA(32, 56, 72, 152),   UI_RGBA(16, 32, 44, 140),  UI_RGBA(104, 168, 224, 22), UI_RGBA(104, 168, 224, 255), UI_RGBA(54, 92, 122, 255),  UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 255, 255, 255), false },
    };
    return kSpecs[bgKind < 7 ? bgKind : 0];
}

void UiW_GbaTopBar(uint8_t bgKind, uint16_t iconSpr, const char *secTitle, int32_t dotCount, int32_t dotActive, bool dotAllGreen, const char *pagerText)
{
    const GbaBgSpec sp = BgSpecFor(bgKind);

    // Translucent GBA backdrop below the 52 px top bar over the 3D world
    UiDraw_Rect(0, GBA_TOPBAR_H, UI_SCREEN_W, UI_SCREEN_H - GBA_TOPBAR_H, sp.body);
    if (sp.stripes)
    {
        for (int32_t sy = GBA_TOPBAR_H; sy < UI_SCREEN_H; sy += 16)
            UiDraw_Rect(0, sy + 8, UI_SCREEN_W, 8, sp.scan);
    }
    else
    {
        for (int32_t sy = GBA_TOPBAR_H; sy < UI_SCREEN_H; sy += 8)
        {
            UiDraw_Rect(0, sy, UI_SCREEN_W, 4, UI_RGBA(255, 255, 255, 14));
            UiDraw_Rect(0, sy + 4, UI_SCREEN_W, 4, UI_RGBA(0, 0, 0, 22));
        }
    }
    for (int32_t gy = GBA_TOPBAR_H; gy < UI_SCREEN_H; gy += 16)
        UiDraw_Rect(0, gy, UI_SCREEN_W, 1, sp.grid);
    for (int32_t gx = 0; gx < UI_SCREEN_W; gx += 16)
        UiDraw_Rect(gx, GBA_TOPBAR_H, 1, UI_SCREEN_H - GBA_TOPBAR_H, sp.grid);

    // Top header bar (52 px)
    UiDraw_RectV(0, 0, UI_SCREEN_W, GBA_TOPBAR_H - 6, UI_RGBA(46, 59, 64, 255), UI_RGBA(32, 41, 45, 255));
    UiDraw_Rect(0, 0, UI_SCREEN_W, 1, UI_RGBA(255, 255, 255, 36));
    UiDraw_Rect(0, GBA_TOPBAR_H - 6, UI_SCREEN_W, 3, sp.barAccent);
    UiDraw_Rect(0, GBA_TOPBAR_H - 3, UI_SCREEN_W, 3, UI_RGBA(24, 32, 32, 255));
    UiDraw_Rect(0, GBA_TOPBAR_H, UI_SCREEN_W, 3, UI_RGBA(0, 0, 0, 105));

    // Fixed-width left section badge (.gba-sec-pill), omitted when secTitle is empty (e.g. Bag)
    const bool hasSec = (secTitle && secTitle[0] != '\0');
    if (hasSec)
    {
        DrawGbaSectionPill(12, 8, 244, 34, bgKind == UI_GBA_BG_POKEDEX, iconSpr, secTitle,
                           sp.pillBorder, sp.pillBg, sp.pillBg, sp.pillInk, !sp.stripes, !sp.stripes);
    }

    // Fixed-width pager (.gba-pager) with [L], centered dots + label, [R]
    if (dotCount > 0 || (pagerText && pagerText[0]))
    {
        const int32_t pagerW = 304;
        const int32_t lW = 24, rW = 24;
        const int32_t px = hasSec ? 268 : 14;
        BevelBoxR(px, 8, pagerW, 34, 5, false, 2, UI_RGBA(15, 20, 22, 255), UI_RGBA(23, 30, 33, 255), UI_RGBA(23, 30, 33, 255),
                  1, UI_RGBA(62, 77, 82, 255), UI_RGBA(8, 12, 14, 200), 0, 0, 0, false);

        const int32_t lx = px + 6;
        BevelBoxR(lx, 13, lW, 24, 3, false, 1, UI_RGBA(16, 20, 22, 255), UI_RGBA(64, 76, 84, 255), UI_RGBA(48, 58, 64, 255),
                  1, UI_RGBA(112, 128, 140, 255), UI_RGBA(28, 36, 40, 255), 0, 0, 0, false);
        UiText_DrawCentred(UI_FACE_PS2P, 16, lx, 13, lW, 24, "L", GBA_YELLOW);

        const int32_t rxBtn = px + pagerW - 6 - rW;
        BevelBoxR(rxBtn, 13, rW, 24, 3, false, 1, UI_RGBA(16, 20, 22, 255), UI_RGBA(64, 76, 84, 255), UI_RGBA(48, 58, 64, 255),
                  1, UI_RGBA(112, 128, 140, 255), UI_RGBA(28, 36, 40, 255), 0, 0, 0, false);
        UiText_DrawCentred(UI_FACE_PS2P, 16, rxBtn, 13, rW, 24, "R", GBA_YELLOW);

        const int32_t dotsW = dotCount > 0 ? dotCount * 15 - 3 : 0;
        const int32_t txtW = (pagerText && pagerText[0]) ? UiText_Width(UI_FACE_PS2P, 16, pagerText) : 0;
        const int32_t gap = (dotsW > 0 && txtW > 0) ? 8 : 0;
        const int32_t innerW = pagerW - 68;
        int32_t contentW = dotsW + gap + txtW;
        if (contentW > innerW) contentW = innerW;
        int32_t cx = px + 34 + (innerW - contentW) / 2;

        UiDraw_PushClip(px + 32, 8, innerW + 4, 34);
        for (int32_t i = 0; i < dotCount; i++)
        {
            const int32_t dx = cx + 6, dy = 25;
            if (dotAllGreen)
            {
                Disc(dx, dy, 6, UI_RGBA(24, 32, 32, 255));
                Disc(dx, dy, 4, UI_RGBA(90, 213, 131, 255));
            }
            else if (i == dotActive)
            {
                Disc(dx, dy, 6, GBA_YELLOW);
                Disc(dx, dy, 4, C_WHITE);
            }
            else
            {
                Disc(dx, dy, 5, UI_RGBA(16, 20, 22, 255));
                Disc(dx, dy, 3, UI_RGBA(59, 72, 78, 255));
            }
            cx += 15;
        }
        if (dotCount > 0)
            cx += 5;
        if (pagerText && pagerText[0])
            UiText_DrawMid(UI_FACE_PS2P, 16, cx, 8, 34, pagerText, GBA_YELLOW);
        UiDraw_PopClip();
    }
}

void UiW_GbaBox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t border, uint32_t top, uint32_t bot,
                uint32_t innerTop, int32_t shadowY)
{
    if (w <= 0 || h <= 0)
        return;
    int32_t rr = r;
    if (rr > h / 2) rr = h / 2;
    if (rr > w / 2) rr = w / 2;
    BevelBoxR(x, y, w, h, rr, false, 2, border, top, bot, innerTop ? 2 : 0, innerTop,
              innerTop ? UI_RGBA(0, 0, 0, 32) : 0, 0, shadowY, UI_RGBA(0, 0, 0, 100), false);
}

void UiW_GbaRingBox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t outerRing, uint32_t innerRing,
                    uint32_t top, uint32_t bot, uint32_t innerHi, int32_t shadowY)
{
    if (w <= 0 || h <= 0)
        return;
    int32_t rr = r;
    if (rr > h / 2) rr = h / 2;
    if (rr > w / 2) rr = w / 2;
    if (shadowY > 0)
        ShapeFill(x - 2, y + shadowY, w + 4, h + 2, rr + 2, false, UI_RGBA(0, 0, 0, 100), UI_RGBA(0, 0, 0, 100));
    ShapeFill(x - 2, y - 2, w + 4, h + 4, rr + 2, false, outerRing, outerRing);
    ShapeFill(x, y, w, h, rr, false, innerRing, innerRing);
    if (w > 4 && h > 4)
        ShapeFill(x + 2, y + 2, w - 4, h - 4, rr > 2 ? rr - 2 : 0, false, top, bot);
    if (innerHi != 0 && w > 8 && h > 8)
    {
        const int32_t hiR = rr > 3 ? rr - 3 : 0;
        UiDraw_Rect(x + 3 + hiR, y + 2, w - 6 - 2 * hiR, 2, innerHi);
        UiDraw_Rect(x + 3 + hiR, y + h - 4, w - 6 - 2 * hiR, 2, UI_RGBA(0, 0, 0, 36));
    }
}

int32_t UiW_GbaText(uint8_t face, uint8_t px, int32_t x, int32_t y, const char *text, uint32_t ink, uint32_t shadow)
{
    if (!text || !text[0])
        return 0;
    const int32_t d = (face == UI_FACE_VT323 && px >= 22) ? 2 : 1;
    if (shadow != 0)
        UiText_Draw(face, px, x + d, y + d, text, shadow);
    return UiText_Draw(face, px, x, y, text, ink);
}

int32_t UiW_GbaTextMid(uint8_t face, uint8_t px, int32_t x, int32_t y, int32_t h, const char *text, uint32_t ink, uint32_t shadow)
{
    if (!text || !text[0])
        return 0;
    const int32_t d = (face == UI_FACE_VT323 && px >= 22) ? 2 : 1;
    if (shadow != 0)
        UiText_DrawMid(face, px, x + d, y + d, h, text, shadow);
    return UiText_DrawMid(face, px, x, y, h, text, ink);
}

void UiW_GbaTextCentred(uint8_t face, uint8_t px, int32_t x, int32_t y, int32_t w, int32_t h, const char *text, uint32_t ink, uint32_t shadow)
{
    if (!text || !text[0])
        return;
    const int32_t d = (face == UI_FACE_VT323 && px >= 22) ? 2 : 1;
    if (shadow != 0)
        UiText_DrawCentred(face, px, x + d, y + d, w, h, text, shadow);
    UiText_DrawCentred(face, px, x, y, w, h, text, ink);
}

int32_t UiW_GbaTextWrapped(uint8_t face, uint8_t px, int32_t x, int32_t y, int32_t w, int32_t lineH,
                           const char *text, uint32_t ink, uint32_t shadow, int32_t maxLines)
{
    if (!text || !text[0])
        return 0;
    const int32_t d = (face == UI_FACE_VT323 && px >= 22) ? 2 : 1;
    if (shadow != 0)
        UiW_TextWrappedF(face, px, x + d, y + d, w, lineH, text, shadow, maxLines);
    return UiW_TextWrappedF(face, px, x, y, w, lineH, text, ink, maxLines);
}

int32_t UiW_GbaPagerHit(const UiInput *in, bool hasSecPill, int32_t dotCount)
{
    const int32_t px = hasSecPill ? 268 : 14;
    if (UiHit(in, px, 6, 44, 38))
        return -1;
    if (UiHit(in, px + 260, 6, 44, 38))
        return 1;
    if (dotCount > 0 && UiHit(in, px + 44, 6, 216, 38))
        return 1;
    return 0;
}

void UiW_GbaWin(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t kind)
{
    switch (kind)
    {
    case UI_GBA_WIN_GOLD:
        GbaFrameBoxPal(x, y, w, h, UI_RGBA(40, 48, 48, 255), UI_RGBA(248, 224, 136, 255), UI_RGBA(216, 168, 56, 255),
                       UI_RGBA(24, 32, 32, 255), UI_RGBA(255, 253, 240, 255), UI_RGBA(255, 248, 220, 255), UI_RGBA(232, 220, 172, 255), 4);
        break;
    case UI_GBA_WIN_LAVENDER:
        GbaFrameBoxPal(x, y, w, h, UI_RGBA(40, 48, 48, 255), UI_RGBA(144, 148, 200, 255), UI_RGBA(82, 82, 136, 255),
                       UI_RGBA(24, 32, 32, 255), C_WHITE, UI_RGBA(246, 246, 252, 255), UI_RGBA(216, 216, 236, 255), 4);
        break;
    case UI_GBA_WIN_DARK:
        GbaFrameBoxPal(x, y, w, h, UI_RGBA(20, 26, 28, 255), GBA_MINT, UI_RGBA(23, 94, 71, 255),
                       UI_RGBA(20, 26, 28, 255), UI_RGBA(38, 59, 64, 255), UI_RGBA(28, 45, 50, 255), GBA_DARK_SH, 4);
        if (w > 24 && h > 18)
            UiDraw_Rect(x + 9, y + 8, w - 18, 1, UI_RGBA(115, 205, 164, 55));
        break;
    default:
        GbaFrameBoxPal(x, y, w, h, GBA_CHARCOAL, GBA_MINT, UI_RGBA(32, 156, 8, 255),
                       GBA_CHARCOAL, C_WHITE, UI_RGBA(248, 250, 246, 255), UI_GBA_SHAD_LIGHT, 4);
        break;
    }
}

void UiW_GbaMsg(int32_t x, int32_t y, int32_t w, int32_t h, bool battleDark, bool redArrow)
{
    const uint32_t innerEdge = battleDark ? UI_RGBA(20, 26, 28, 255) : GBA_CHARCOAL;
    if (battleDark)
    {
        GbaFrameBoxPal(x, y, w, h, UI_RGBA(20, 26, 28, 255), UI_RGBA(255, 164, 98, 255), UI_RGBA(220, 90, 65, 255),
                       innerEdge, UI_RGBA(41, 57, 65, 255), UI_RGBA(33, 47, 54, 255), UI_RGBA(20, 28, 34, 255), 4);
        if (w > 36 && h > 24)
        {
            UiDraw_Rect(x + 8, y + 6, w - 16, 3, UI_RGBA(92, 114, 128, 255));
            UiDraw_Rect(x + 7, y + 6, 8, h - 12, UI_RGBA(220, 90, 65, 255));
            UiDraw_Rect(x + 11, y + 6, 4, h - 12, UI_RGBA(255, 164, 98, 255));
            UiDraw_Rect(x + 15, y + 6, 2, h - 12, UI_RGBA(216, 228, 236, 255));
            UiDraw_Rect(x + w - 15, y + 6, 8, h - 12, UI_RGBA(255, 164, 98, 255));
            UiDraw_Rect(x + w - 11, y + 6, 4, h - 12, UI_RGBA(220, 90, 65, 255));
            UiDraw_Rect(x + w - 17, y + 6, 2, h - 12, UI_RGBA(216, 228, 236, 255));
        }
    }
    else
    {
        GbaFrameBoxPal(x, y, w, h, GBA_CHARCOAL, UI_RGBA(24, 232, 120, 255), UI_RGBA(0, 168, 136, 255),
                       innerEdge, C_WHITE, UI_RGBA(248, 250, 246, 255), UI_GBA_SHAD_LIGHT, 4);
        if (w > 36 && h > 24)
        {
            UiDraw_Rect(x + 8, y + 6, w - 16, 3, UI_GBA_SHAD_LIGHT);
            UiDraw_Rect(x + 8, y + h - 9, w - 16, 3, UI_GBA_SHAD_LIGHT);
            UiDraw_Rect(x + 6, y + 6, 9, h - 12, UI_RGBA(24, 232, 120, 255));
            UiDraw_Rect(x + 10, y + 6, 5, h - 12, UI_RGBA(0, 168, 136, 255));
            UiDraw_Rect(x + 15, y + 6, 2, h - 12, GBA_CHARCOAL);
            UiDraw_Rect(x + w - 15, y + 6, 9, h - 12, UI_RGBA(0, 168, 136, 255));
            UiDraw_Rect(x + w - 11, y + 6, 5, h - 12, UI_RGBA(24, 232, 120, 255));
            UiDraw_Rect(x + w - 17, y + 6, 2, h - 12, GBA_CHARCOAL);
        }
    }
    if (redArrow && w > 50 && h > 28)
    {
        UiW_GbaArrowV(x + w - 34, y + h - 18, true, UI_RGBA(24, 32, 32, 180));
        UiW_GbaArrowV(x + w - 35, y + h - 19, true, UI_RGBA(230, 8, 8, 255));
    }
}

void UiW_GbaSectionStrip(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t tagKind, const char *label, const char *rightSub)
{
    static const uint32_t kTop[7] = {
        UI_RGBA(72, 136, 208, 255),  // BLUE
        UI_RGBA(62, 144, 112, 255),  // TEAL
        UI_RGBA(200, 148, 24, 255),  // GOLD
        UI_RGBA(74, 168, 64, 255),   // GREEN
        UI_RGBA(184, 64, 168, 255),  // MAGENTA
        UI_RGBA(200, 40, 48, 255),   // CRIMSON
        UI_RGBA(88, 88, 152, 255),   // SLATE
    };
    static const uint32_t kBot[7] = {
        UI_RGBA(44, 100, 168, 255),
        UI_RGBA(40, 112, 84, 255),
        UI_RGBA(158, 112, 12, 255),
        UI_RGBA(52, 128, 44, 255),
        UI_RGBA(140, 40, 128, 255),
        UI_RGBA(152, 24, 32, 255),
        UI_RGBA(62, 62, 120, 255),
    };
    const uint8_t idx = tagKind < 7 ? tagKind : 0;
    BevelBoxR(x, y, w, h, 3, false, 2, GBA_CHARCOAL, UI_RGBA(53, 66, 71, 255), UI_RGBA(44, 56, 61, 255),
              1, UI_RGBA(255, 255, 255, 24), UI_RGBA(0, 0, 0, 64), 0, 0, 0, false);

    const int32_t rw = (rightSub && rightSub[0]) ? UiText_Width(UI_FACE_VT323, 24, rightSub) : 0;
    const int32_t maxTagW = rw > 0 ? (w - rw - 26) : (w - 18);
    int32_t tagW = UiText_Width(UI_FACE_PS2P, 16, label) + 22;
    if (tagW > maxTagW) tagW = maxTagW;
    if (tagW < 0) tagW = 0;
    if (tagW > 0)
    {
        UiDraw_RectV(x + 2, y + 2, tagW, h - 4, kTop[idx], kBot[idx]);
        UiDraw_Rect(x + 2, y + 2, tagW, 1, UI_RGBA(255, 255, 255, 64));
        const float rx0 = (float)(x + 2 + tagW), rx1 = (float)(x + 2 + tagW + 10);
        const float xs[4] = { rx0, rx1, rx0, rx0 };
        const float ys[4] = { (float)(y + 2), (float)(y + h - 2), (float)(y + h - 2), (float)(y + h - 2) };
        UiDraw_Poly4(xs, ys, kBot[idx]);
        for (int32_t g = 4; g + 4 < h - 4; g += 6)
            UiDraw_Rect(x + 5, y + 2 + g, 3, 4, UI_RGBA(255, 255, 255, 160));
    }
    if (tagW > 14)
    {
        UiDraw_PushClip(x + 10, y + 2, tagW - 12, h - 4);
        UiText_DrawMid(UI_FACE_PS2P, 16, x + 13, y + 1, h, label, UI_RGBA(0, 0, 0, 180));
        UiText_DrawMid(UI_FACE_PS2P, 16, x + 12, y, h, label, C_WHITE);
        UiDraw_PopClip();
    }
    if (rw > 0)
    {
        UiDraw_PushClip(x + 10, y + 2, w - 14, h - 4);
        UiW_GbaTextMid(UI_FACE_VT323, 24, x + w - 10 - rw, y, h, rightSub, GBA_YELLOW, UI_GBA_SHAD_DARK);
        UiDraw_PopClip();
    }
}

void UiW_GbaHpBar(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t hp, uint16_t maxHp)
{
    if (h < 14) h = 14;
    BevelBoxR(x, y, w, h, h / 2, false, 2, UI_RGBA(24, 32, 32, 255), UI_RGBA(57, 64, 64, 255), UI_RGBA(57, 64, 64, 255), 0, 0, 0, 0, 0, 0, false);
    UiDraw_Rect(x + 3, y + 2, 24, h - 4, UI_RGBA(36, 42, 42, 255));

    // Pixel "HP" tag (11x7 at 1x inside the dark left capsule)
    static const uint16_t kHP[7] = {
        0x45E, // #...#.####.
        0x451, // #...#.#...#
        0x451, // #...#.#...#
        0x7DE, // #####.####.
        0x450, // #...#.#....
        0x450, // #...#.#....
        0x450, // #...#.#....
    };
    DrawBits(x + 10, y + (h - 7) / 2 + 1, 11, 7, kHP, 1, UI_RGBA(168, 100, 0, 255));
    DrawBits(x + 9, y + (h - 7) / 2, 11, 7, kHP, 1, UI_RGBA(255, 230, 0, 255));

    const int32_t tx = x + 28, ty = y + 3, tw = w - 32, th = h - 6;
    if (tw <= 2 || th <= 2)
        return;
    UiDraw_Rect(tx - 1, ty - 1, tw + 2, th + 2, UI_RGBA(24, 32, 32, 255));
    UiDraw_Rect(tx, ty, tw, th, UI_RGBA(32, 57, 0, 255));
    if (maxHp == 0 || hp == 0)
        return;
    int32_t fw = (int32_t)((int64_t)tw * hp / maxHp);
    if (fw < 2) fw = 2;
    if (fw > tw) fw = tw;
    const int32_t pct = (int32_t)((int32_t)hp * 100 / maxHp);
    uint32_t top = UI_RGBA(115, 255, 172, 255), bot = UI_RGBA(90, 213, 131, 255);
    if (pct <= 20)
        top = UI_RGBA(255, 180, 123, 255), bot = UI_RGBA(222, 106, 90, 255);
    else if (pct <= 50)
        top = UI_RGBA(255, 246, 139, 255), bot = UI_RGBA(255, 230, 57, 255);
    const int32_t hiH = (th * 42 + 50) / 100;
    if (hiH > 0)
        UiDraw_Rect(tx, ty, fw, hiH, top);
    if (th - hiH > 0)
        UiDraw_Rect(tx, ty + hiH, fw, th - hiH, bot);
}

void UiW_GbaExpBar(int32_t x, int32_t y, int32_t w, int32_t pct, const char *nextText)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    const int32_t h = 20;
    const int32_t nextW = (nextText && nextText[0]) ? UiText_Width(UI_FACE_VT323, 20, nextText) + 12 : 0;
    const int32_t barW = w - nextW;
    // Full-width dark rounded .gba-exp-wrap container so NEXT ... sits inside the box in #ACEEFF
    BevelBoxR(x, y, w, h, 3, false, 2, UI_RGBA(24, 32, 32, 255), UI_RGBA(62, 68, 82, 255), UI_RGBA(62, 68, 82, 255), 0, 0, 0, 0, 0, 0, false);
    UiDraw_Rect(x + 2, y + 2, 30, h - 4, UI_RGBA(36, 40, 48, 255));

    static const uint16_t kExp[7] = {
        0x7A2E, // ####.#...#.###.
        0x4229, // #....#...#.#..#
        0x4149, // #.....#.#..#..#
        0x708E, // ###....#...###.
        0x4148, // #.....#.#..#...
        0x4228, // #....#...#.#...
        0x7A28, // ####.#...#.#...
    };
    DrawBits(x + 9, y + (h - 7) / 2, 15, 7, kExp, 1, GBA_YELLOW);

    const int32_t tx = x + 34, ty = y + 5, tw = barW - 38, th = h - 10;
    if (tw > 2 && th > 2)
    {
        UiDraw_Rect(tx - 1, ty - 1, tw + 2, th + 2, UI_RGBA(16, 20, 24, 255));
        UiDraw_Rect(tx, ty, tw, th, UI_RGBA(24, 32, 42, 255));
        const int32_t fw = tw * pct / 100;
        if (fw > 0)
        {
            UiDraw_RectV(tx, ty, fw, th, UI_RGBA(106, 230, 255, 255), UI_RGBA(36, 136, 184, 255));
            UiDraw_Rect(tx, ty, fw, 2, UI_RGBA(170, 244, 255, 255));
        }
        for (int32_t sx = tx + 12; sx < tx + tw; sx += 12)
            UiDraw_Rect(sx, ty, 2, th, UI_RGBA(24, 32, 42, 210));
    }
    if (nextW > 0)
        UiW_GbaTextMid(UI_FACE_VT323, 20, x + barW + 4, y, h, nextText, UI_RGBA(172, 238, 255, 255), UI_GBA_SHAD_DARK);
}
