// Components of the new UI: the C side of design/stitch/_shared/ui.css (same tokens, sizes and
// finish). Everything is rectangles from the atlas white block (with vertical two-colour
// gradients), glyphs and sprites, so a whole screen stays one draw call.
// Bevels: outer edge with the corner pixel cut, inner highlight (top-left), inner shade
// (bottom-right) and a hard drop shadow, like the reference's double-bevel boxes.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"

#define HEADER_H 48
#define FOOTER_H 40

// Finish colours (ui.css)
#define C_FRAME_TOP   UI_RGBA(14, 43, 37, 255)
#define C_FRAME_BOT   UI_RGBA(6, 26, 22, 255)
#define C_EDGE_DARK   UI_RGBA(4, 18, 16, 255)
#define C_HI_DARK     UI_RGBA(27, 84, 75, 255)
#define C_SHADE_DARK  UI_RGBA(5, 22, 20, 255)
#define C_CAP_TOP     UI_RGBA(18, 58, 49, 255)
#define C_CAP_BOT     UI_RGBA(10, 36, 30, 255)
#define C_CAP_HI      UI_RGBA(46, 107, 91, 255)
#define C_MINT        UI_RGBA(153, 246, 228, 255)
#define C_MINT2       UI_RGBA(110, 231, 183, 255)
#define C_PAPER_EDGE  UI_RGBA(26, 48, 36, 255)
#define C_PAPER_SHADE UI_RGBA(209, 200, 175, 255)
#define C_PAPER_STRIP UI_RGBA(231, 223, 198, 255)
#define C_ROW_A       UI_RGBA(235, 228, 207, 255)
#define C_ROW_B       UI_RGBA(241, 235, 216, 255)
#define C_ROW_LINE    UI_RGBA(216, 207, 183, 255)
#define C_SEL_TOP     UI_RGBA(255, 247, 214, 255)
#define C_SEL_BOT     UI_RGBA(253, 230, 138, 255)
#define C_SEL_EDGE    UI_RGBA(217, 119, 6, 255)
#define C_TAB_TOP     UI_RGBA(252, 237, 166, 255)
#define C_TAB_BOT     UI_RGBA(226, 171, 36, 255)
#define C_TAB_EDGE    UI_RGBA(140, 96, 6, 255)
#define C_TAB_HI      UI_RGBA(255, 251, 224, 255)
#define C_RAIL        UI_RGBA(4, 22, 18, 255)
#define C_RAIL_EDGE   UI_RGBA(28, 85, 71, 255)
#define C_KEY_TOP     UI_RGBA(27, 58, 50, 255)
#define C_KEY_BOT     UI_RGBA(12, 32, 26, 255)
#define C_KEY_LIGHT   UI_RGBA(54, 109, 95, 255)
#define C_KEY_DARK    UI_RGBA(6, 21, 17, 255)
#define C_SHADOW      UI_RGBA(0, 0, 0, 102)
#define C_WHITE       UI_RGBA(255, 255, 255, 255)
#define C_GOLD        UI_RGBA(252, 211, 77, 255)

// ---------- Bevel primitives ----------

// Box with a cut-corner edge of thickness t, a vertical gradient fill, an inner highlight and an
// inner shade of thickness b, and an optional hard shadow of `shadow` px below.
static void BevelBox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t edge, uint32_t top, uint32_t bottom,
                     int32_t b, uint32_t hi, uint32_t shade, int32_t shadow)
{
    // Rounded corners: radius 6 (smaller on short boxes), drawn as stepped rows
    static const int8_t kInset[6] = { 4, 2, 1, 1, 0, 0 };   // edge inset of the first rows of a radius-6 corner
    int32_t r = h >= 24 ? 6 : h >= 12 ? 3 : 0;
    if (w < 2 * r) r = 0;
    if (shadow > 0)
        UiDraw_Rect(x + r, y + h, w - 2 * r, shadow, C_SHADOW);
    // Edge shape
    for (int32_t k = 0; k < r; k++)
    {
        const int32_t in = r == 6 ? kInset[k] : (k == 0 ? 2 : k == 1 ? 1 : 0);
        UiDraw_Rect(x + in, y + k, w - 2 * in, 1, edge);
        UiDraw_Rect(x + in, y + h - 1 - k, w - 2 * in, 1, edge);
    }
    UiDraw_Rect(x, y + r, w, h - 2 * r, edge);
    // Fill (gradient), inset by the edge thickness, with the same rounding
    const int32_t fx = x + t, fy = y + t, fw = w - 2 * t, fh = h - 2 * t;
    const int32_t fr = r > t ? r - t : 0;
    for (int32_t k = 0; k < fr; k++)
    {
        const int32_t in = fr - k > 2 ? (fr - k - 1) : (fr - k == 2 ? 1 : 0);
        UiDraw_Rect(fx + in, fy + k, fw - 2 * in, 1, top);
        UiDraw_Rect(fx + in, fy + fh - 1 - k, fw - 2 * in, 1, bottom);
    }
    UiDraw_RectV(fx, fy + fr, fw, fh - 2 * fr, top, bottom);
    if (b > 0)
    {
        // Inner highlight (top, left) and shade (bottom, right): the bevel
        UiDraw_Rect(fx + fr, fy, fw - 2 * fr, b, hi);
        UiDraw_Rect(fx, fy + fr, b, fh - 2 * fr, hi);
        UiDraw_Rect(fx + fr, fy + fh - b, fw - 2 * fr, b, shade);
        UiDraw_Rect(fx + fw - b, fy + fr, b, fh - 2 * fr, shade);
    }
}

// Public bevelled box (battle HUD plates, command buttons)
void UiW_Bevel(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t edge, uint32_t top, uint32_t bottom,
               int32_t b, uint32_t hi, uint32_t shade, int32_t shadow)
{
    BevelBox(x, y, w, h, t, edge, top, bottom, b, hi, shade, shadow);
}

// Dialogue / choice box (design/stitch .dialog): cream gradient, white highlight, warm shade,
// dark rounded edge and a soft drop shadow
void UiW_DialogBox(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 3, UI_RGBA(38, 39, 45, 255), UI_RGBA(251, 250, 242, 255), UI_RGBA(235, 231, 211, 255),
             2, C_WHITE, UI_RGBA(158, 151, 132, 255), 6);
}

// Sunken info box inside a panel (labels and values of the summaries)
void UiW_Inset(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 2, UI_RGBA(209, 200, 175, 255), UI_COL_PAPER1, UI_RGBA(229, 221, 197, 255),
             2, UI_RGBA(205, 196, 170, 255), C_WHITE, 0);
}

// Bevelled key; the footer one uses the footer type (Press Start 2P 16)
static int32_t KeyEx(int32_t x, int32_t y, const char *label, bool small)
{
    const int32_t w = (small ? UiText_Width(UI_FACE_PS2P, UI_FOOT_PX, label) + 12 : UiText_Width(UI_FACE_VT323, UI_BODY_PX, label) + 14);
    UiDraw_Rect(x + 1, y + 24, w - 2, 2, C_SHADOW);
    UiDraw_RectV(x, y, w, 24, C_KEY_TOP, C_KEY_BOT);
    UiDraw_Rect(x, y, w, 2, C_KEY_LIGHT);
    UiDraw_Rect(x, y, 2, 24, C_KEY_LIGHT);
    UiDraw_Rect(x, y + 22, w, 2, C_KEY_DARK);
    UiDraw_Rect(x + w - 2, y, 2, 24, C_KEY_DARK);
    if (small)
        UiText_Draw(UI_FACE_PS2P, UI_FOOT_PX, x + 6, y + 5, label, C_MINT2);
    else
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 7, y - 4, label, C_MINT2);
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
    UiDraw_RectV(0, 0, UI_SCREEN_W, HEADER_H - 2, C_FRAME_TOP, C_FRAME_BOT);
    UiDraw_Rect(0, HEADER_H - 2, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_Rect(0, HEADER_H, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 2, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_RectV(0, UI_SCREEN_H - FOOTER_H + 2, UI_SCREEN_W, FOOTER_H - 2, C_FRAME_BOT, C_FRAME_TOP);
}

void UiW_FooterBar(void)
{
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H - 2, UI_SCREEN_W, 2, C_SHADOW);
    UiDraw_Rect(0, UI_SCREEN_H - FOOTER_H, UI_SCREEN_W, 2, UI_COL_LINE_DARK);
    UiDraw_RectV(0, UI_SCREEN_H - FOOTER_H + 2, UI_SCREEN_W, FOOTER_H - 2, C_FRAME_BOT, C_FRAME_TOP);
}

int32_t UiW_Capsule(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 2, C_EDGE_DARK, C_CAP_TOP, C_CAP_BOT, 1, C_CAP_HI, C_SHADE_DARK, 2);
    return w;
}

void UiW_HeaderTitle(const char *title)
{
    const int32_t w = UiText_Width(UI_FACE_PS2P, 16, title) + 48;
    UiW_Capsule(16, 8, w, 32);
    UiDraw_Sprite(UI_SPR_BALL, 24, 16, 1);
    UiText_Draw(UI_FACE_PS2P, 16, 48, 16, title, C_MINT);
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
    UiDraw_Rect(x + 1, 6, total - 2, 36, C_RAIL_EDGE);
    UiDraw_Rect(x, 7, total, 34, C_RAIL_EDGE);
    UiDraw_Rect(x + 2, 8, total - 4, 32, C_RAIL);
    UiDraw_Rect(x + 2, 8, total - 4, 2, C_SHADOW);
    int32_t cx = x + 4;
    cx += Key(cx, 12, "L") + 4;
    for (int32_t i = 0; i < count; i++)
    {
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, labels[i]) + 2 * pad;
        if (i == current)
        {
            BevelBox(cx, 10, w, 28, 2, C_TAB_EDGE, C_TAB_TOP, C_TAB_BOT, 1, C_TAB_HI, C_TAB_BOT, 0);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx + pad, 8, labels[i], UI_COL_INK);
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

void UiW_Panel(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 3, C_PAPER_EDGE, UI_COL_PAPER0, UI_COL_PAPER0, 2, C_WHITE, C_PAPER_SHADE, 4);
}

void UiW_PanelDark(int32_t x, int32_t y, int32_t w, int32_t h)
{
    BevelBox(x, y, w, h, 3, C_EDGE_DARK, UI_RGBA(12, 47, 39, 255), UI_RGBA(7, 29, 24, 255), 1, C_HI_DARK, C_SHADE_DARK, 4);
}

// Head strip inside a paper panel ("LEDGER: 05 / 12 ITEMS")
void UiW_PanelHead(int32_t x, int32_t y, int32_t w, const char *label, const char *right)
{
    UiDraw_Rect(x + 3, y + 3, w - 6, 36, C_PAPER_STRIP);
    UiDraw_Rect(x + 3, y + 3, w - 6, 2, C_WHITE);
    UiDraw_Rect(x + 3, y + 39, w - 6, 2, C_PAPER_SHADE);
    UiText_Draw(UI_FACE_PS2P, 16, x + 16, y + 14, label, UI_COL_INK);
    if (right)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, right), y + 5, right, UI_COL_INK_SOFT);
}

void UiW_Window(int32_t x, int32_t y, int32_t w, int32_t h, const char *title)
{
    BevelBox(x, y, w, h, 3, C_PAPER_EDGE, UI_COL_PAPER0, UI_COL_PAPER0, 2, C_WHITE, C_PAPER_SHADE, 6);
    UiDraw_RectV(x + 3, y + 3, w - 6, 38, UI_RGBA(28, 56, 47, 255), UI_RGBA(11, 35, 29, 255));
    UiDraw_Rect(x + 3, y + 3, w - 6, 1, C_CAP_HI);
    UiDraw_Rect(x + 3, y + 41, w - 6, 2, C_EDGE_DARK);
    UiText_Draw(UI_FACE_PS2P, 16, x + 16, y + 15, title, C_MINT);
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
        UiDraw_RectV(x, y, w, h - 2, C_SEL_TOP, C_SEL_BOT);
        UiDraw_Rect(x, y, w, 1, C_WHITE);
        UiDraw_Rect(x, y + h - 2, w, 2, C_SEL_EDGE);
        UiW_Arrow(x + 8, y + (h - 12) / 2, C_SEL_EDGE);
    }
    else
    {
        UiDraw_Rect(x, y, w, h - 2, (index & 1) ? C_ROW_B : C_ROW_A);
        UiDraw_Rect(x, y + h - 2, w, 2, C_ROW_LINE);
    }
}

int32_t UiW_Chip(int32_t x, int32_t y, const char *label, bool amber)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, label) + 16;
    if (amber)
        BevelBox(x, y, w, 26, 2, C_TAB_EDGE, C_TAB_TOP, C_TAB_BOT, 1, C_TAB_HI, C_TAB_BOT, 0);
    else
        BevelBox(x, y, w, 26, 2, UI_RGBA(11, 46, 38, 255), UI_RGBA(36, 82, 68, 255), UI_RGBA(22, 58, 48, 255), 1, UI_RGBA(60, 122, 104, 255), UI_RGBA(22, 58, 48, 255), 0);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 8, y - 4, label, amber ? UI_COL_INK : UI_RGBA(209, 250, 229, 255));
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
    static const uint32_t tops[3] = { UI_RGBA(47, 122, 92, 255), UI_RGBA(224, 138, 42, 255), UI_RGBA(168, 58, 58, 255) };
    static const uint32_t bots[3] = { UI_RGBA(29, 90, 67, 255), UI_RGBA(184, 102, 26, 255), UI_RGBA(122, 38, 38, 255) };
    const int32_t dy = pressed ? 2 : 0;
    BevelBox(x, y + dy, w, 40, 2, UI_RGBA(23, 42, 34, 255), tops[colour % 3], bots[colour % 3], 1,
             UI_RGBA(255, 255, 255, 102), UI_RGBA(0, 0, 0, 102), pressed ? 0 : 2);
    UiText_Draw(UI_FACE_PS2P, 16, x + (w - UiText_Width(UI_FACE_PS2P, 16, label)) / 2, y + 12 + dy, label, C_WHITE);
}

void UiW_Strip3(int32_t x, int32_t y, int32_t w, const char *const labels[3], const char *const values[3])
{
    UiDraw_Rect(x, y, w, 64, C_ROW_LINE);
    UiDraw_Rect(x + 2, y + 2, w - 4, 60, C_ROW_A);
    const int32_t cw = (w - 4) / 3;
    for (int32_t i = 0; i < 3; i++)
    {
        const int32_t cx = x + 2 + i * cw;
        if (i > 0)
            UiDraw_Rect(cx - 1, y + 2, 2, 60, C_ROW_LINE);
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
        UiDraw_Rect(kx + 2, y + 40, 36, 2, C_SHADOW);
        UiDraw_RectV(kx, y, 40, 40, C_KEY_TOP, C_KEY_BOT);
        UiDraw_Rect(kx, y, 40, 2, C_KEY_LIGHT);
        UiDraw_Rect(kx, y, 2, 40, C_KEY_LIGHT);
        UiDraw_Rect(kx, y + 38, 40, 2, C_KEY_DARK);
        UiDraw_Rect(kx + 38, y, 2, 40, C_KEY_DARK);
        UiText_Draw(UI_FACE_VT323, 32, kx + 13, y + 4, k ? "+" : "-", C_MINT2);
    }
    UiDraw_Rect(x + 48, y, 80, 40, C_PAPER_SHADE);
    UiDraw_Rect(x + 50, y + 2, 76, 36, C_ROW_A);
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
