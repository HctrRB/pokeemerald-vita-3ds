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
#define C_WHITE       UI_RGBA(255, 255, 255, 255)
#define C_GLOSS       UI_RGBA(255, 255, 255, 46)

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

// Dialogue / choice box (design/stitch .dialog): cream gradient, white highlight, warm shade,
// dark rounded edge and a soft drop shadow
void UiW_DialogBox(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, T->edge > 3 ? T->edge : 3, T->dlgEdge, T->dlgTop, T->dlgBot, 2, C_WHITE, T->dlgShade, 6);
    if (T->accent)
        UiDraw_Rect(x, y + (T->chamfer ? T->radius : 0), 8, h - (T->chamfer ? 2 * T->radius : 0), T->accent);
}

// Sunken info box inside a panel (labels and values of the summaries)
void UiW_Inset(int32_t x, int32_t y, int32_t w, int32_t h)
{
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
        UiText_Draw(UI_FACE_PS2P, UI_FOOT_PX, x + 6, y + 5, label, T->keyInk);
    else
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 7, y - 4, label, T->keyInk);
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
    if (T->stripes)
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
    UiDraw_Rect(0, HEADER_H, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 2, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_RectV(0, UI_SCREEN_H - FOOTER_H + 2, UI_SCREEN_W, FOOTER_H - 2, T->barBot, T->barTop);
}

void UiW_FooterBar(void)
{
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

void UiW_HeaderTitle(const char *title)
{
    const int32_t w = UiText_Width(UI_FACE_PS2P, 16, title) + 48;
    UiW_Capsule(16, 8, w, 32);
    UiDraw_Sprite(UI_SPR_BALL, 24, 16, 1);
    UiText_Draw(UI_FACE_PS2P, 16, 48, 16, title, T->title);
}

void UiW_HeaderValue(const char *text, uint32_t colour)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, text) + 24;
    UiW_Capsule(UI_SCREEN_W - 16 - w, 8, w, 32);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, UI_SCREEN_W - 16 - w + 12, 8, text, colour);
}

int32_t UiW_TabsPad(int32_t x, const char *const *labels, int32_t count, int32_t current, int32_t *tabX, int32_t *tabW, int32_t pad)
{
    int32_t total = 4;
    total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, "L") + 14 + 4;
    for (int32_t i = 0; i < count; i++)
        total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, labels[i]) + 2 * pad + 2;
    total += UiText_Width(UI_FACE_VT323, UI_BODY_PX, "R") + 14 + 4;
    // Sunken rail
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
            const int32_t r = T->chamfer ? 5 : T->radius >= 10 ? 10 : 3;
            BevelBoxR(cx, 10, w, 28, r, T->chamfer, 2, T->tabEdge, T->tabTop, T->tabBot, T->bevel ? 1 : 0, T->tabHi, T->tabBot, 0, 0, 0, T->gloss);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx + pad, 8, labels[i], T->tabInk);
        }
        else
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx + pad, 8, labels[i], UI_COL_ON_SOFT);
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
    const char *p = text;
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

void UiW_FooterGlyph(int32_t *x, uint8_t glyph, const char *label)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 8;
    UiW_Glyph(*x, y, glyph);
    *x += 30;
    *x += UiText_Draw(UI_FACE_PS2P, UI_FOOT_PX, *x, y + 5, label, UI_COL_ON_FRAME) + 18;
}

void UiW_FooterClock(const char *clock)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 5;
    const int32_t w = UiText_Width(UI_FACE_PS2P, UI_FOOT_PX, clock) + 24;
    const int32_t x = UI_SCREEN_W - 8 - w;
    UiW_Capsule(x, y, w, 30);
    UiText_Draw(UI_FACE_PS2P, UI_FOOT_PX, x + 12, y + 8, clock, UI_COL_ON_FRAME);
}

void UiW_FooterKey(int32_t *x, const char *key, const char *label)
{
    const int32_t y = UI_SCREEN_H - FOOTER_H + 8;
    *x += KeyEx(*x, y, key, true) + 6;
    *x += UiText_Draw(UI_FACE_PS2P, UI_FOOT_PX, *x, y + 5, label, UI_COL_ON_FRAME) + 18;
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
    BevelBox(x, y, w, h, T->edge, T->paperEdge, T->paper0, T->paper0, 2, C_WHITE, T->paperShade, 4);
    AccentBar(x, y, h);
}

void UiW_PanelDark(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, T->edge > 3 ? T->edge : 3, T->darkEdge, T->darkTop, T->darkBot, 1, T->darkHi, T->darkBot, 4);
    Gloss(x, y, w, h, 3);
    AccentBar(x, y, h);
}

// Head strip inside a paper panel ("LEDGER: 05 / 12 ITEMS")
void UiW_PanelHead(int32_t x, int32_t y, int32_t w, const char *label, const char *right)
{
    // Square strip under the panel edge, kept clear of the rounded / cut top corners
    const int32_t e = T->edge, r = T->radius > e ? T->radius - e : 0;
    ShapeFill(x + e, y + e, w - 2 * e, 2 * r + 2, r, T->chamfer, T->paperStrip, T->paperStrip);
    UiDraw_Rect(x + e, y + e + r, w - 2 * e, 39 - e - r, T->paperStrip);
    if (T->bevel)
        UiDraw_Rect(x + e + r, y + e, w - 2 * e - 2 * r, 2, C_WHITE);
    UiDraw_Rect(x + e, y + 39, w - 2 * e, 2, T->edge >= 4 ? T->paperEdge : T->paperShade);
    AccentBar(x, y, 41);
    UiText_Draw(UI_FACE_PS2P, 16, x + 16, y + 14, label, T->stripInk);
    if (right)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, right), y + 5, right, T->stripSoft);
}

void UiW_Window(int32_t x, int32_t y, int32_t w, int32_t h, const char *title)
{
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
    UiText_Draw(UI_FACE_PS2P, 16, x + 16, y + 15, title, T->winInk);
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
        if (T->selRadius > 0)
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
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 8, y - 4, label, amber ? T->tabInk : T->chipInk);
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
    UiText_Draw(UI_FACE_PS2P, 16, x + (w - UiText_Width(UI_FACE_PS2P, 16, label)) / 2, y + 12 + dy, label, T->btnInk);
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
        UiText_Draw(UI_FACE_VT323, 32, kx + 13, y + 4, k ? "+" : "-", T->keyInk);
    }
    UiDraw_Rect(x + 48, y, 80, 40, T->paperShade);
    UiDraw_Rect(x + 50, y + 2, 76, 36, T->paper1);
    UiDraw_Rect(x + 50, y + 2, 76, 2, UI_RGBA(0, 0, 0, 31));
    UiText_Draw(UI_FACE_VT323, 32, x + 48 + (80 - UiText_Width(UI_FACE_VT323, 32, buf)) / 2, y + 4, buf, UI_COL_INK);
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
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 14, y + 3, text, UI_COL_ON_FRAME);
}
