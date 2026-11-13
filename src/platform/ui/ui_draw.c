#include <math.h>
// Draw list of the new UI and its triple buffer (docs/RENDERING.md).
//
// Core 1 writes into the "back" list and publishes it by exchanging it with the "middle" slot,
// marked fresh. Core 0 takes the middle slot only when it is fresh, exchanging it with its
// "front" list. The three indices are always a permutation of 0..2, so the writer never touches
// the list Core 0 is drawing and Core 0 always sees a complete list: no locks, no waits.
//
// Core 1 emits the final vertices (position, normalised texels, colour; two triangles per quad),
// so Core 0 only binds the array and draws (it measured 0.79 ms converting quads on Core 0).

#include <string.h>
#include "ui_internal.h"
#include "platform/ui.h"

#define UI_FRESH 4u
#define UI_MAX_CLIP 8

static UiDrawList sLists[3];
static uint32_t sMiddle = 1u;     // index | UI_FRESH, shared
static uint32_t sBack = 0u;       // Core 1 only
static uint32_t sFront = 2u;      // Core 0 only
static uint32_t sSeq;             // Core 1 only

typedef struct { int32_t x0, y0, x1, y1; } UiClip;
static UiClip sClip[UI_MAX_CLIP];
static int32_t sClipDepth;

static uint16_t sWhiteU, sWhiteV;
static float sInvW = 1.0f, sInvH = 1.0f;

void UiDraw_Begin(void)
{
    UiDrawList *list = &sLists[sBack];
    list->vcount = 0;
    UiTheme_Refresh();
    sClipDepth = 0;
    sClip[0] = (UiClip){ 0, 0, UI_SCREEN_W, UI_SCREEN_H };
    uint16_t wx = 0, wy = 0, tw = 1, th = 1;
    UiAssets_White(&wx, &wy);
    UiAssets_TexSize(&tw, &th);
    sWhiteU = (uint16_t)(wx + 1);   // inside the white block, safe with nearest filtering
    sWhiteV = (uint16_t)(wy + 1);
    sInvW = 1.0f / (float)(tw ? tw : 1);
    sInvH = 1.0f / (float)(th ? th : 1);
}

void UiDraw_PushClip(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (sClipDepth + 1 >= UI_MAX_CLIP)
        return;
    const UiClip *p = &sClip[sClipDepth];
    UiClip c = { x, y, x + w, y + h };
    if (c.x0 < p->x0) c.x0 = p->x0;
    if (c.y0 < p->y0) c.y0 = p->y0;
    if (c.x1 > p->x1) c.x1 = p->x1;
    if (c.y1 > p->y1) c.y1 = p->y1;
    sClip[++sClipDepth] = c;
}

void UiDraw_PopClip(void)
{
    if (sClipDepth > 0)
        sClipDepth--;
}

static uint32_t LerpColour(uint32_t a, uint32_t b, int32_t num, int32_t den)
{
    if (a == b || den <= 0)
        return a;
    uint32_t out = 0;
    for (int32_t s = 0; s < 32; s += 8)
    {
        const int32_t ca = (int32_t)((a >> s) & 0xFF), cb = (int32_t)((b >> s) & 0xFF);
        out |= (uint32_t)(ca + (cb - ca) * num / den) << s;
    }
    return out;
}

// Clips against the current clip rect (texels and the gradient follow proportionally) and emits
// two triangles.
void UiDraw_Quad2(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t u0, uint32_t v0, uint32_t u1, uint32_t v1, uint32_t top, uint32_t bottom)
{
    UiDrawList *list = &sLists[sBack];
    if (w <= 0 || h <= 0 || list->vcount + 6 > UI_MAX_QUADS * 6)
        return;
    const UiClip *c = &sClip[sClipDepth];
    int32_t x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    if (x1 <= c->x0 || y1 <= c->y0 || x0 >= c->x1 || y0 >= c->y1)
        return;
    const int32_t du = (int32_t)u1 - (int32_t)u0, dv = (int32_t)v1 - (int32_t)v0;
    uint32_t ct = top, cb = bottom;
    if (x0 < c->x0) { u0 = (uint32_t)((int32_t)u0 + du * (c->x0 - x0) / w); x0 = c->x0; }
    if (x1 > c->x1) { u1 = (uint32_t)((int32_t)u1 - du * (x1 - c->x1) / w); x1 = c->x1; }
    if (y0 < c->y0) { v0 = (uint32_t)((int32_t)v0 + dv * (c->y0 - y0) / h); ct = LerpColour(top, bottom, c->y0 - y, h); y0 = c->y0; }
    if (y1 > c->y1) { v1 = (uint32_t)((int32_t)v1 - dv * (y1 - c->y1) / h); cb = LerpColour(top, bottom, c->y1 - y, h); y1 = c->y1; }

    const float fx0 = (float)x0, fy0 = (float)y0, fx1 = (float)x1, fy1 = (float)y1;
    const float fu0 = (float)u0 * sInvW, fv0 = (float)v0 * sInvH, fu1 = (float)u1 * sInvW, fv1 = (float)v1 * sInvH;
    UiGlVertex *v = &list->verts[list->vcount];
    v[0] = (UiGlVertex){ fx0, fy0, fu0, fv0, ct };
    v[1] = (UiGlVertex){ fx1, fy0, fu1, fv0, ct };
    v[2] = (UiGlVertex){ fx1, fy1, fu1, fv1, cb };
    v[3] = (UiGlVertex){ fx0, fy0, fu0, fv0, ct };
    v[4] = (UiGlVertex){ fx1, fy1, fu1, fv1, cb };
    v[5] = (UiGlVertex){ fx0, fy1, fu0, fv1, cb };
    list->vcount += 6;
}

void UiDraw_Quad(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t u0, uint32_t v0, uint32_t u1, uint32_t v1, uint32_t rgba)
{
    UiDraw_Quad2(x, y, w, h, u0, v0, u1, v1, rgba, rgba);
}

void UiDraw_Rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t rgba)
{
    UiDraw_Quad2(x, y, w, h, sWhiteU, sWhiteV, sWhiteU, sWhiteV, rgba, rgba);
}

void UiDraw_RectV(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bottom)
{
    UiDraw_Quad2(x, y, w, h, sWhiteU, sWhiteV, sWhiteU, sWhiteV, top, bottom);
}

void UiDraw_Frame(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint32_t rgba)
{
    UiDraw_Rect(x, y, w, t, rgba);
    UiDraw_Rect(x, y + h - t, w, t, rgba);
    UiDraw_Rect(x, y + t, t, h - 2 * t, rgba);
    UiDraw_Rect(x + w - t, y + t, t, h - 2 * t, rgba);
}

void UiDraw_Sprite(uint16_t id, int32_t x, int32_t y, int32_t scale)
{
    uint16_t sx, sy, sw, sh;
    if (!UiAssets_Sprite(id, &sx, &sy, &sw, &sh))
        return;
    if (scale < 1)
        scale = 1;
    UiDraw_Quad(x, y, sw * scale, sh * scale, sx, sy, sx + sw, sy + sh, 0xFFFFFFFFu);
}

uint32_t UiDraw_Count(void)
{
    return sLists[sBack].vcount / 6;
}

// Darkens every vertex of the list being built (the battle HUD follows the game's fades to black)
void UiDraw_Darken(int32_t num, int32_t den)
{
    UiDrawList *list = &sLists[sBack];
    if (num <= 0 || den <= 0)
        return;
    if (num > den)
        num = den;
    for (uint32_t i = 0; i < list->vcount; i++)
    {
        const uint32_t c = list->verts[i].rgba;
        uint32_t out = c & 0xFF000000u;
        for (int32_t s = 0; s < 24; s += 8)
            out |= (uint32_t)((int32_t)((c >> s) & 0xFF) * (den - num) / den) << s;
        list->verts[i].rgba = out;
    }
}

void UiDraw_Publish(float tickMs)
{
    UiDrawList *list = &sLists[sBack];
    list->seq = ++sSeq;
    list->tickMs = tickMs;
    UiStall_Published(list->seq);
    sBack = __atomic_exchange_n(&sMiddle, sBack | UI_FRESH, __ATOMIC_ACQ_REL) & 3u;
}

const UiDrawList *UiDraw_Acquire(void)
{
    if (__atomic_load_n(&sMiddle, __ATOMIC_ACQUIRE) & UI_FRESH)
        sFront = __atomic_exchange_n(&sMiddle, sFront, __ATOMIC_ACQ_REL) & 3u;
    return &sLists[sFront];
}

void UiDraw_SpriteTint(uint16_t id, int32_t x, int32_t y, int32_t scale, uint32_t rgba)
{
    uint16_t sx, sy, sw, sh;
    if (!UiAssets_Sprite(id, &sx, &sy, &sw, &sh))
        return;
    if (scale < 1)
        scale = 1;
    UiDraw_Quad(x, y, sw * scale, sh * scale, sx, sy, sx + sw, sy + sh, rgba);
}

// Arbitrary quad (no clipping: used inside panels); 0-1-2 and 0-2-3
void UiDraw_Poly4(const float *xs, const float *ys, uint32_t rgba)
{
    UiDrawList *list = &sLists[sBack];
    if (list->vcount + 6 > UI_MAX_QUADS * 6)
        return;
    const float u = (float)sWhiteU * sInvW, v = (float)sWhiteV * sInvH;
    static const int idx[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; i++)
        list->verts[list->vcount++] = (UiGlVertex){ xs[idx[i]], ys[idx[i]], u, v, rgba };
}

void UiDraw_Tri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t rgba)
{
    const float xs[4] = { x0, x1, x2, x2 }, ys[4] = { y0, y1, y2, y2 };
    UiDraw_Poly4(xs, ys, rgba);
}

void UiDraw_Line(float x0, float y0, float x1, float y1, float width, uint32_t rgba)
{
    float dx = x1 - x0, dy = y1 - y0;
    const float len = sqrtf(dx * dx + dy * dy);
    if (len <= 0.0f)
        return;
    const float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
    const float xs[4] = { x0 + nx, x1 + nx, x1 - nx, x0 - nx }, ys[4] = { y0 + ny, y1 + ny, y1 - ny, y0 - ny };
    UiDraw_Poly4(xs, ys, rgba);
}
