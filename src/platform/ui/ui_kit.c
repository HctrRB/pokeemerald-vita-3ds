// Shared behaviour of the new UI's screens (design/stitch/_shared/nav.js on the console): lists
// that scroll with the d-pad and by dragging, where the first tap selects and a second tap (or X)
// activates; the action popup; the quantity popup with - / +; the toast. Core 1 only.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "../vita/vita_platform.h"
#include "global.h"
#include "sound.h"
#include "constants/songs.h"

// ---------- Toast (drawn by ui_core.c over every screen) ----------
static char sToast[96];
static int32_t sToastTicks;

void UiKit_Toast(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sToast, sizeof(sToast), fmt, ap);
    va_end(ap);
    sToastTicks = 120;
}

void UiKit_DrawToast(void)
{
    if (sToastTicks <= 0)
        return;
    sToastTicks--;
    UiW_Toast(sToast);
}

void UiKit_ClearToast(void)
{
    sToastTicks = 0;
}

// ---------- List ----------
void UiList_Reset(UiList *l, int32_t count, int32_t rows)
{
    l->count = count;
    l->rows = rows;
    if (l->sel >= count) l->sel = count > 0 ? count - 1 : 0;
    if (l->sel < 0) l->sel = 0;
    if (l->first > l->sel) l->first = l->sel;
    if (l->sel >= l->first + rows) l->first = l->sel - rows + 1;
    if (l->first > count - rows) l->first = count - rows;
    if (l->first < 0) l->first = 0;
    l->dragPx = 0;
}

int32_t UiList_Update(UiList *l, const UiInput *in, int32_t x, int32_t y, int32_t w, int32_t rowH, bool wrap)
{
    if (l->count <= 0)
        return (in->pressed & VITA_BTN_CROSS) ? -2 : -1;
    const int32_t before = l->sel;
    if (in->repeat & VITA_BTN_UP) l->sel = (l->sel > 0) ? l->sel - 1 : (wrap ? l->count - 1 : 0);
    if (in->repeat & VITA_BTN_DOWN) l->sel = (l->sel < l->count - 1) ? l->sel + 1 : (wrap ? 0 : l->sel);
    const int32_t h = l->rows * rowH;
    // Drag scrolls the rows under the finger; the selection stays visible
    if (in->dragging && in->downX >= x && in->downX < x + w && in->downY >= y && in->downY < y + h)
    {
        l->dragPx -= in->dragDy;
        while (l->dragPx >= rowH && l->first < l->count - l->rows) { l->first++; l->dragPx -= rowH; }
        while (l->dragPx <= -rowH && l->first > 0) { l->first--; l->dragPx += rowH; }
        if (l->sel < l->first) l->sel = l->first;
        if (l->sel >= l->first + l->rows) l->sel = l->first + l->rows - 1;
    }
    else if (!in->touching)
        l->dragPx = 0;
    int32_t activated = (in->pressed & VITA_BTN_CROSS) ? l->sel : -1;
    for (int32_t r = 0; r < l->rows && l->first + r < l->count; r++)
        if (UiHit(in, x, y + r * rowH, w, rowH))
        {
            if (l->first + r == l->sel) activated = l->sel;
            else l->sel = l->first + r;
        }
    if (l->sel < l->first) l->first = l->sel;
    if (l->sel >= l->first + l->rows) l->first = l->sel - l->rows + 1;
    if (l->sel != before)
        PlaySE(SE_SELECT);
    return activated;
}

// ---------- Action popup ----------
#define MENU_ROW 40

void UiMenu_Open(UiMenu *m, const char *title, int32_t x, int32_t y, int32_t w)
{
    m->open = true;
    m->count = 0;
    m->x = x;
    m->y = y;
    m->w = w;
    m->list.sel = 0;
    m->list.first = 0;
    snprintf(m->title, sizeof(m->title), "%s", title);
}

void UiMenu_Add(UiMenu *m, const char *label, int32_t id)
{
    if (m->count >= UI_MENU_MAX)
        return;
    snprintf(m->labels[m->count], sizeof(m->labels[0]), "%s", label);
    m->ids[m->count++] = id;
}

// Returns the id of the chosen entry, UI_MENU_CANCEL on circle, UI_MENU_NONE otherwise
int32_t UiMenu_Update(UiMenu *m, const UiInput *in)
{
    // Keep it on screen
    const int32_t h = 44 + m->count * MENU_ROW + 12;
    if (m->y + h > UI_SCREEN_H - 48) m->y = UI_SCREEN_H - 48 - h;
    if (m->y < 56) m->y = 56;
    UiList_Reset(&m->list, m->count, m->count);
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        m->open = false;
        return UI_MENU_CANCEL;
    }
    const int32_t a = UiList_Update(&m->list, in, m->x + 8, m->y + 44, m->w - 16, MENU_ROW, true);
    if (a >= 0)
    {
        PlaySE(SE_SELECT);
        m->open = false;
        return m->ids[a];
    }
    // A tap outside the window closes it
    if (in->tap && !(in->tx >= m->x && in->tx < m->x + m->w && in->ty >= m->y && in->ty < m->y + h))
    {
        m->open = false;
        return UI_MENU_CANCEL;
    }
    return UI_MENU_NONE;
}

void UiMenu_Draw(const UiMenu *m)
{
    const int32_t h = 44 + m->count * MENU_ROW + 12;
    UiW_Window(m->x, m->y, m->w, h, m->title);
    for (int32_t i = 0; i < m->count; i++)
    {
        const int32_t y = m->y + 44 + i * MENU_ROW;
        UiW_Row(m->x + 8, y, m->w - 16, MENU_ROW, i == m->list.sel, i);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, m->x + 36, y + 4, m->labels[i], i == m->list.sel ? UI_COL_SELECT_INK : UI_COL_INK);
    }
}

// ---------- Quantity popup ----------
void UiQty_Open(UiQty *q, const char *title, int32_t max)
{
    q->open = true;
    q->value = 1;
    q->max = max < 1 ? 1 : max;
    snprintf(q->title, sizeof(q->title), "%s", title);
}

// Returns the chosen quantity, 0 on cancel, -1 while open. Up/down +-1, left/right +-10.
int32_t UiQty_Update(UiQty *q, const UiInput *in)
{
    const int32_t before = q->value;
    if (in->repeat & VITA_BTN_UP) q->value++;
    if (in->repeat & VITA_BTN_DOWN) q->value--;
    if (in->repeat & VITA_BTN_RIGHT) q->value += 10;
    if (in->repeat & VITA_BTN_LEFT) q->value -= 10;
    if (UiHit(in, q->minusX, q->stepY, 40, 40)) q->value--;
    if (UiHit(in, q->plusX, q->stepY, 40, 40)) q->value++;
    if (q->value < 1) q->value = q->max;          // wraps like the game's quantity box
    if (q->value > q->max) q->value = (before == q->max) ? 1 : q->max;
    if (q->value != before)
        PlaySE(SE_SELECT);
    if (in->pressed & VITA_BTN_CIRCLE) { q->open = false; PlaySE(SE_SELECT); return 0; }
    if ((in->pressed & VITA_BTN_CROSS) || UiHit(in, q->okX, q->stepY, 120, 40)) { q->open = false; PlaySE(SE_SELECT); return q->value; }
    return -1;
}

void UiQty_Draw(UiQty *q, uint16_t itemId)
{
    const int32_t x = 280, y = 190, w = 400, h = 190;   // room under the stepper for a total line
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    UiW_Window(x, y, w, h, q->title);
    if (itemId)
        UiSpr_ItemIcon(itemId, x + 20, y + 72, 2);
    q->stepY = y + 76;
    UiW_Stepper(x + 80, q->stepY, q->value, &q->minusX, &q->plusX);
    q->okX = x + w - 136;
    UiW_Button(q->okX, q->stepY, 120, "OK", UI_BTN_GREEN, false);
}

// ---------- Full-screen header ----------
int32_t UiKit_Header(const char *title, const char *const *tabs, int32_t n, int32_t cur, int32_t *tabX, int32_t *tabW, int32_t pad)
{
    UiW_FullFrame();
    UiW_HeaderTitle(title);
    if (n <= 0)
        return 0;
    const int32_t x = 16 + UiText_Width(UI_FACE_PS2P, 16, title) + 48 + 12;
    return UiW_TabsPad(x, tabs, n, cur, tabX, tabW, pad);
}

int32_t UiKit_TabHit(const UiInput *in, const int32_t *tabX, const int32_t *tabW, int32_t n)
{
    for (int32_t i = 0; i < n; i++)
        if (UiHit(in, tabX[i], 8, tabW[i], 32))
            return i;
    return -1;
}

void UiKit_FooterClock(void)
{
    char clock[8];
    UiGame_Clock(clock, sizeof(clock));
    UiW_FooterClock(clock);
}

void UiKit_HeaderMoney(void)
{
    char money[16];
    snprintf(money, sizeof(money), "$%u", (unsigned)UiGame_Money());
    UiW_HeaderValue(money, UI_COL_GOLD);
}

// Gender symbol and shiny star after a name (x2 sprites, 28 px); returns the width used
int32_t UiKit_GenderShiny(int32_t x, int32_t y, uint8_t gender, bool shiny)
{
    int32_t w = 0;
    if (gender == 0x00) { UiDraw_Sprite(UI_SPR_GENDER_MALE, x, y, 2); w += 32; }
    else if (gender == 0xFE) { UiDraw_Sprite(UI_SPR_GENDER_FEMALE, x, y, 2); w += 32; }
    if (shiny) { UiDraw_Sprite(UI_SPR_SHINY_STAR, x + w, y, 2); w += 32; }
    return w;
}
