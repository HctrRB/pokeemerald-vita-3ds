#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "platform/vita_ui_plate.h"
#include "platform/vita_ui.h"
#include "vita_platform.h"

#include "global.h"
#include "data.h"
#include "decompress.h"
#include "constants/species.h"

enum
{
    PLATE_PAD_L       = 5,
    PLATE_PAD_T       = 5,
    PLATE_PAD_R       = 5,
    PLATE_PAD_B       = 7,
    PLATE_CACHE_SLOTS = 48,
    TINT_CACHE_SLOTS  = 32,
    RAY_SIZE          = 72,
    BTA_ELEM_HDR_SIZE = 10
};

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    uint8_t flags;
    const uint8_t *alpha;
    const uint8_t *a;
    const uint8_t *r;
    const uint8_t *c;
} BtaElem;

typedef struct
{
    uint32_t age;
    uint16_t w;
    uint16_t h;
    uint8_t chamfer;
    uint8_t state;
    uint8_t blink;
    uint8_t headBandH;
    VitaUiRgb base;
    uint16_t boxW;
    uint16_t boxH;
    uint16_t *p;    // Premultiplied RGB565, row-major (boxH rows of boxW)
    uint8_t *a;     // Coverage 0..255, row-major
    uint16_t *run;  // Per-row [run0, run1) opaque span (boxH * 2)
} PlateSlot;

typedef struct
{
    uint32_t age;
    uint8_t id;
    VitaUiRgb colour;
    VitaUiRgb ring;
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t *p;    // Premultiplied RGB565, row-major (h rows of w)
    uint8_t *a;     // Coverage 0..255, row-major
    uint16_t *run;  // Per-row [run0, run1) opaque span (h * 2)
} TintSlot;

static bool sInitialized = false;
static uint8_t *sMenuFile = NULL;
static const uint16_t *sMenuBackdrop = NULL;
static uint8_t *sBtaFile = NULL;
static BtaElem sBtaElems[VITA_BTA_COUNT];
static const uint16_t *sBattleBackdrop = NULL;

static uint8_t sRayBody[RAY_SIZE * RAY_SIZE];
static uint8_t sRayLines[RAY_SIZE * RAY_SIZE];
static bool sRayWatermarkBuilt = false;

static PlateSlot sPlateCache[PLATE_CACHE_SLOTS];
static TintSlot sTintCache[TINT_CACHE_SLOTS];
static uint32_t sCacheClock = 0U;

static const VitaUiRgb sFocusRing[2] = {
    {255, 120, 40},
    {255, 206, 72}
};
static const VitaUiRgb sHueFight = {236, 70, 56};

static inline int32_t Div255(const int32_t x)
{
    return (x + 1 + (x >> 8)) >> 8;
}

static inline float Clamp01(const float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline VitaUiRgb MixRgb(const VitaUiRgb a, const VitaUiRgb b, const float t)
{
    const float u = Clamp01(t);
    const VitaUiRgb out = {
        (uint8_t)((float)a.r + ((float)b.r - (float)a.r) * u + 0.5f),
        (uint8_t)((float)a.g + ((float)b.g - (float)a.g) * u + 0.5f),
        (uint8_t)((float)a.b + ((float)b.b - (float)a.b) * u + 0.5f)
    };
    return out;
}

static inline bool SameRgb(const VitaUiRgb a, const VitaUiRgb b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

VitaUiRgb VitaUiPlate_FocusRingRgb(const uint8_t blinkPhase)
{
    return sFocusRing[blinkPhase & 1U];
}

// Signed distance to a chamfered rectangle centered at (cx, cy) with half-extents (hw, hh)
// and corner chamfer `c`. Negative inside, positive outside.
static float ChamferSdf(const float px, const float py, const float cx, const float cy,
                        const float hw, const float hh, const float c)
{
    const float dx = fabsf(px - cx) - hw;
    const float dy = fabsf(py - cy) - hh;
    const float rectDist = fmaxf(dx, dy);
    const float diagDist = (dx + dy + c) * 0.70710678f;
    return fmaxf(rectDist, diagDist);
}

static uint8_t BtaAlphaAt(const uint8_t id, const int32_t dx, const int32_t dy)
{
    if (id >= VITA_BTA_COUNT)
    {
        return 0U;
    }
    const BtaElem *const e = &sBtaElems[id];
    const int32_t lx = dx - e->x;
    const int32_t ly = dy - e->y;
    if (e->alpha == NULL || lx < 0 || ly < 0 || lx >= (int32_t)e->w || ly >= (int32_t)e->h)
    {
        return 0U;
    }
    return e->alpha[(size_t)ly * e->w + (size_t)lx];
}

static float RaySample64(const float *const m, const float u, const float v)
{
    const int32_t x0 = (int32_t)floorf(u);
    const int32_t y0 = (int32_t)floorf(v);
    const float fx = u - (float)x0;
    const float fy = v - (float)y0;
    float sum = 0.0f;

    for (int32_t dy = 0; dy < 2; ++dy)
    {
        for (int32_t dx = 0; dx < 2; ++dx)
        {
            const int32_t x = x0 + dx;
            const int32_t y = y0 + dy;
            const float wgt = (dx != 0 ? fx : (1.0f - fx)) * (dy != 0 ? fy : (1.0f - fy));
            if (x >= 0 && y >= 0 && x < 64 && y < 64)
            {
                sum += wgt * m[y * 64 + x];
            }
        }
    }
    return sum;
}

static void BuildRayquazaWatermark(void)
{
    static uint8_t sRayDecomp[8192];
    static uint16_t sPalDecomp[256];
    static uint8_t sRayTiles[64 * 64 / 2];
    static uint16_t sRayPal[16];
    static float sBody[64 * 64];
    static float sLines[64 * 64];
    static float sSoft[64 * 64];

    LZ77UnCompWram(gMonStillFrontPicTable[SPECIES_RAYQUAZA].data, sRayDecomp);
    memcpy(sRayTiles, sRayDecomp, sizeof(sRayTiles));

    LZ77UnCompWram(gMonPaletteTable[SPECIES_RAYQUAZA].data, sPalDecomp);
    memcpy(sRayPal, sPalDecomp, sizeof(sRayPal));

    for (int32_t t = 0; t < 64; ++t)
    {
        for (int32_t py = 0; py < 8; ++py)
        {
            for (int32_t px = 0; px < 8; ++px)
            {
                const uint8_t v = (uint8_t)((sRayTiles[t * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15U);
                const int32_t x = (t % 8) * 8 + px;
                const int32_t y = (t / 8) * 8 + py;
                const uint16_t c = sRayPal[v];
                const int32_t lum = (((c & 31U) + ((c >> 5) & 31U) + ((c >> 10) & 31U)) * 255) / 31;
                sBody[y * 64 + x] = (v != 0U) ? 1.0f : 0.0f;
                sLines[y * 64 + x] = (v != 0U && lum < 200) ? 1.0f : 0.0f;
            }
        }
    }

    for (int32_t y = 0; y < 64; ++y)
    {
        for (int32_t x = 0; x < 64; ++x)
        {
            float sum = 0.0f;
            float norm = 0.0f;
            for (int32_t dy = -1; dy <= 1; ++dy)
            {
                for (int32_t dx = -1; dx <= 1; ++dx)
                {
                    const float wgt = (dx != 0 ? 1.0f : 2.0f) * (dy != 0 ? 1.0f : 2.0f);
                    if (x + dx >= 0 && y + dy >= 0 && x + dx < 64 && y + dy < 64)
                    {
                        sum += wgt * sBody[(y + dy) * 64 + (x + dx)];
                    }
                    norm += wgt;
                }
            }
            sSoft[y * 64 + x] = sum / norm;
        }
    }

    for (int32_t ry = 0; ry < RAY_SIZE; ++ry)
    {
        for (int32_t rx = 0; rx < RAY_SIZE; ++rx)
        {
            int32_t inBody = 0;
            int32_t inLines = 0;
            for (int32_t sy = 0; sy < 4; ++sy)
            {
                for (int32_t sx = 0; sx < 4; ++sx)
                {
                    const float u = ((float)rx + ((float)sx + 0.5f) * 0.25f) * (64.0f / 70.4f) - 0.5f;
                    const float v = ((float)ry + ((float)sy + 0.5f) * 0.25f) * (64.0f / 70.4f) - 0.5f;
                    if (RaySample64(sSoft, u, v) >= 0.47f) inBody++;
                    if (RaySample64(sLines, u, v) >= 0.43f) inLines++;
                }
            }
            sRayBody[ry * RAY_SIZE + rx] = (uint8_t)(inBody * 255 / 16);
            sRayLines[ry * RAY_SIZE + rx] = (uint8_t)(inLines * 255 / 16);
        }
    }
}

static void ParseMenuBin(void)
{
    size_t size = 0U;
    sMenuFile = VitaPlatform_LoadAsset("ui/menu.bin", &size);
    if (sMenuFile == NULL || size < 10U + BTA_ELEM_HDR_SIZE || memcmp(sMenuFile, "EM3DMNU1", 8U) != 0)
    {
        free(sMenuFile);
        sMenuFile = NULL;
        VITA_LOG("[UI] ui/menu.bin missing or invalid; using fallback backdrop\n");
        return;
    }

    const uint8_t *const p = sMenuFile + 10U;
    const uint8_t flags = p[1];
    const uint16_t w = (uint16_t)(p[6] | (p[7] << 8));
    const uint16_t h = (uint16_t)(p[8] | (p[9] << 8));
    const size_t pixelBytes = (size_t)w * (size_t)h * sizeof(uint16_t);

    if ((flags & 8U) != 0U && w == VITA_UI_WIDTH && h == VITA_UI_HEIGHT && 10U + BTA_ELEM_HDR_SIZE + pixelBytes <= size)
    {
        sMenuBackdrop = (const uint16_t *)(p + BTA_ELEM_HDR_SIZE);
    }
}

static void ParseBattleBin(void)
{
    size_t size = 0U;
    sBtaFile = VitaPlatform_LoadAsset("ui/battle.bin", &size);
    if (sBtaFile == NULL || size < 10U || memcmp(sBtaFile, "EM3DBTA1", 8U) != 0)
    {
        free(sBtaFile);
        sBtaFile = NULL;
        VITA_LOG("[UI] ui/battle.bin missing or invalid\n");
        return;
    }

    const uint8_t *p = sBtaFile + 8U;
    const uint8_t *const end = sBtaFile + size;
    const uint16_t count = (uint16_t)(p[0] | (p[1] << 8));
    p += 2U;

    for (uint16_t n = 0U; n < count && p + BTA_ELEM_HDR_SIZE <= end; ++n)
    {
        const uint8_t id = p[0];
        BtaElem elem;
        memset(&elem, 0, sizeof(elem));
        elem.flags = p[1];
        elem.x = (int16_t)(p[2] | (p[3] << 8));
        elem.y = (int16_t)(p[4] | (p[5] << 8));
        elem.w = (uint16_t)(p[6] | (p[7] << 8));
        elem.h = (uint16_t)(p[8] | (p[9] << 8));

        const size_t area = (size_t)elem.w * (size_t)elem.h;
        p += BTA_ELEM_HDR_SIZE;

        if ((elem.flags & 8U) != 0U)
        {
            elem.c = p;
            p += area * 2U;
        }
        else
        {
            elem.alpha = p;
            p += area;
            if ((elem.flags & 4U) == 0U)
            {
                elem.a = p;
                elem.c = p + area;
                p += area * 3U;
                if ((elem.flags & 2U) != 0U)
                {
                    elem.r = p;
                    p += area;
                }
            }
        }
        if (p > end)
        {
            break;
        }
        if (id < VITA_BTA_COUNT)
        {
            sBtaElems[id] = elem;
        }
    }

    if (sBtaElems[VITA_BTA_BACKDROP].c != NULL
        && sBtaElems[VITA_BTA_BACKDROP].w == VITA_UI_WIDTH
        && sBtaElems[VITA_BTA_BACKDROP].h == VITA_UI_HEIGHT)
    {
        sBattleBackdrop = (const uint16_t *)sBtaElems[VITA_BTA_BACKDROP].c;
    }
}

void VitaUiPlate_Init(void)
{
    if (sInitialized)
    {
        return;
    }
    sInitialized = true;
    memset(sBtaElems, 0, sizeof(sBtaElems));
    memset(sPlateCache, 0, sizeof(sPlateCache));
    memset(sTintCache, 0, sizeof(sTintCache));

    ParseMenuBin();
    ParseBattleBin();
}

void VitaUiPlate_Shutdown(void)
{
    for (int32_t i = 0; i < PLATE_CACHE_SLOTS; ++i)
    {
        free(sPlateCache[i].p);
        sPlateCache[i].p = NULL;
    }
    for (int32_t i = 0; i < TINT_CACHE_SLOTS; ++i)
    {
        free(sTintCache[i].p);
        sTintCache[i].p = NULL;
    }
    free(sMenuFile);
    sMenuFile = NULL;
    sMenuBackdrop = NULL;
    free(sBtaFile);
    sBtaFile = NULL;
    sBattleBackdrop = NULL;
    sInitialized = false;
}

bool VitaUiPlate_HasMenuBackdrop(void)
{
    if (!sInitialized) VitaUiPlate_Init();
    return sMenuBackdrop != NULL;
}

bool VitaUiPlate_HasBattleBackdrop(void)
{
    if (!sInitialized) VitaUiPlate_Init();
    return sBattleBackdrop != NULL;
}

static void BlitBackdropRows(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip,
                             const uint16_t *const src, const uint16_t fallbackCol)
{
    const int32_t x0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t y0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t x1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t y1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;
    if (x0 >= x1 || y0 >= y1)
    {
        return;
    }

    const size_t spanW = (size_t)(x1 - x0);
    if (src != NULL)
    {
        for (int32_t y = y0; y < y1; ++y)
        {
            memcpy(&dst[(size_t)y * pitch + (size_t)x0],
                   &src[(size_t)y * VITA_UI_WIDTH + (size_t)x0],
                   spanW * sizeof(uint16_t));
        }
        return;
    }

    for (int32_t y = y0; y < y1; ++y)
    {
        uint16_t *const row = &dst[(size_t)y * pitch + (size_t)x0];
        for (size_t i = 0U; i < spanW; ++i)
        {
            row[i] = fallbackCol;
        }
    }
}

void VitaUiPlate_BlitMenuBackdrop(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    if (!sInitialized) VitaUiPlate_Init();
    BlitBackdropRows(dst, pitch, clip, sMenuBackdrop, VitaUi_PackRgb565(146, 212, 172));
}

void VitaUiPlate_BlitBattleBackdrop(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    if (!sInitialized) VitaUiPlate_Init();
    BlitBackdropRows(dst, pitch, clip, sBattleBackdrop, VitaUi_PackRgb565(58, 156, 134));
}

static void BlitPremulSpanBox(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const int32_t boxX,
    const int32_t boxY,
    const int32_t boxW,
    const int32_t boxH,
    const uint16_t *const pBuf,
    const uint8_t *const aBuf,
    const uint16_t *const runBuf)
{
    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    const int32_t i0 = (cx0 - boxX > 0) ? (cx0 - boxX) : 0;
    const int32_t j0 = (cy0 - boxY > 0) ? (cy0 - boxY) : 0;
    const int32_t i1 = (cx1 - boxX < boxW) ? (cx1 - boxX) : boxW;
    const int32_t j1 = (cy1 - boxY < boxH) ? (cy1 - boxY) : boxH;
    if (i0 >= i1 || j0 >= j1)
    {
        return;
    }

    for (int32_t j = j0; j < j1; ++j)
    {
        uint16_t *const dstRow = &dst[(size_t)(boxY + j) * pitch + (size_t)boxX];
        const uint16_t *const pRow = &pBuf[(size_t)j * (size_t)boxW];
        const uint8_t *const aRow = &aBuf[(size_t)j * (size_t)boxW];
        int32_t r0 = (int32_t)runBuf[j * 2];
        int32_t r1 = (int32_t)runBuf[j * 2 + 1];

        if (r0 < i0) r0 = i0;
        if (r1 > i1) r1 = i1;
        if (r0 < r1)
        {
            memcpy(&dstRow[r0], &pRow[r0], (size_t)(r1 - r0) * sizeof(uint16_t));
        }
        else
        {
            r0 = i1;
            r1 = i1;
        }

        for (int32_t i = i0; i < i1; ++i)
        {
            if (i == r0)
            {
                i = r1 - 1;
                continue;
            }
            const int32_t al = aRow[i];
            const uint16_t pm = pRow[i];
            if (al == 0 && pm == 0U)
            {
                continue;
            }
            if (al == 255)
            {
                dstRow[i] = pm;
                continue;
            }
            const VitaUiRgb under = VitaUi_UnpackRgb565(dstRow[i]);
            const VitaUiRgb add = VitaUi_UnpackRgb565(pm);
            dstRow[i] = VitaUi_PackRgb565(
                Div255((int32_t)under.r * (255 - al)) + (int32_t)add.r,
                Div255((int32_t)under.g * (255 - al)) + (int32_t)add.g,
                Div255((int32_t)under.b * (255 - al)) + (int32_t)add.b);
        }
    }
}

static bool BuildProceduralPlate(PlateSlot *const slot, const uint16_t w, const uint16_t h,
                                 const uint8_t chamfer, const VitaUiRgb base, const uint8_t state,
                                 const uint8_t blink, const uint8_t headBandH)
{
    const uint16_t boxW = (uint16_t)(w + PLATE_PAD_L + PLATE_PAD_R);
    const uint16_t boxH = (uint16_t)(h + PLATE_PAD_T + PLATE_PAD_B);
    const size_t area = (size_t)boxW * (size_t)boxH;
    const size_t totalBytes = area * sizeof(uint16_t) + area + (size_t)boxH * 2U * sizeof(uint16_t);

    free(slot->p);
    slot->p = (uint16_t *)malloc(totalBytes);
    if (slot->p == NULL)
    {
        return false;
    }
    slot->a = (uint8_t *)(slot->p + area);
    slot->run = (uint16_t *)((void *)(slot->a + area));
    slot->w = w;
    slot->h = h;
    slot->chamfer = chamfer;
    slot->state = state;
    slot->blink = (uint8_t)(blink & 1U);
    slot->headBandH = headBandH;
    slot->base = base;
    slot->boxW = boxW;
    slot->boxH = boxH;

    const bool pressed = (state == VITA_PLATE_PRESSED);
    const bool focused = (state == VITA_PLATE_FOCUS);
    const bool disabled = (state == VITA_PLATE_DISABLED);
    const VitaUiRgb white = {255, 255, 255};
    const VitaUiRgb black = {0, 0, 0};
    const VitaUiRgb ink = {22, 28, 38};
    const VitaUiRgb ringCol = sFocusRing[blink & 1U];

    VitaUiRgb effBase = disabled ? MixRgb(base, (VitaUiRgb){140, 152, 146}, 0.55f) : base;
    VitaUiRgb cTop = MixRgb(effBase, white, 0.22f);
    VitaUiRgb cMid = effBase;
    VitaUiRgb cLow = MixRgb(effBase, black, 0.16f);
    const VitaUiRgb cDeep = MixRgb(effBase, black, 0.30f);

    if (focused)
    {
        cTop = MixRgb(cTop, white, 0.15f);
        cMid = MixRgb(cMid, white, 0.12f);
    }
    if (pressed)
    {
        cTop = cMid;
        cMid = cLow;
        cLow = cDeep;
    }

    const float sink = pressed ? (float)VITA_PLATE_SINK_PX : 0.0f;
    const float hh = (float)h - sink;
    const float ox = (float)PLATE_PAD_L;
    const float oy = (float)PLATE_PAD_T + sink;
    const float cx = ox + (float)w * 0.5f;
    const float cy = oy + hh * 0.5f;
    const float hw = (float)w * 0.5f;
    const float hhh = hh * 0.5f;
    const float ch = (float)chamfer;
    const float shCy = (float)PLATE_PAD_T + (pressed ? 1.5f : 3.2f) + hhh;

    for (uint16_t j = 0U; j < boxH; ++j)
    {
        int32_t best0 = 0;
        int32_t best1 = 0;
        int32_t start = -1;
        const float py = (float)j + 0.5f;

        for (uint16_t i = 0U; i <= boxW; ++i)
        {
            int32_t outAlpha = 0;
            if (i < boxW)
            {
                const float px = (float)i + 0.5f;
                const float dOuter = ChamferSdf(px, py, cx, cy, hw, hhh, ch);
                const float dInner = ChamferSdf(px, py, cx, cy, hw - 1.2f, hhh - 1.2f, fmaxf(1.0f, ch - 0.5f));
                const float dShadow = ChamferSdf(px, py, cx, shCy, hw, hhh, ch);

                float rAcc = 0.0f;
                float gAcc = 0.0f;
                float bAcc = 0.0f;
                float aAcc = 0.0f;

                // 1. Soft drop shadow (10, 50, 44)
                const float covSh = Clamp01(1.0f - (dShadow + 0.6f) * 0.45f) * 0.50f;
                if (covSh > 0.004f)
                {
                    rAcc = 10.0f * covSh;
                    gAcc = 50.0f * covSh;
                    bAcc = 44.0f * covSh;
                    aAcc = covSh;
                }

                // 2. Focus outer glow ring
                if (focused)
                {
                    const float dRing = ChamferSdf(px, py, cx, cy, hw + 2.4f, hhh + 2.4f, ch + 1.0f);
                    const float covRing = Clamp01(0.8f - dRing * 0.55f) * 0.92f;
                    if (covRing > 0.004f)
                    {
                        const float inv = 1.0f - covRing;
                        rAcc = rAcc * inv + (float)ringCol.r * covRing;
                        gAcc = gAcc * inv + (float)ringCol.g * covRing;
                        bAcc = bAcc * inv + (float)ringCol.b * covRing;
                        aAcc = aAcc * inv + covRing;
                    }
                }

                // 3. Outer ink border (22, 28, 38)
                const float covOuter = Clamp01(0.5f - dOuter);
                if (covOuter > 0.004f)
                {
                    const float inv = 1.0f - covOuter;
                    rAcc = rAcc * inv + (float)ink.r * covOuter;
                    gAcc = gAcc * inv + (float)ink.g * covOuter;
                    bAcc = bAcc * inv + (float)ink.b * covOuter;
                    aAcc = aAcc * inv + covOuter;
                }

                // 4. Inner bevelled face
                const float covInner = Clamp01(0.5f - dInner);
                if (covInner > 0.004f)
                {
                    const float vy = Clamp01((py - oy) / fmaxf(1.0f, hh));
                    VitaUiRgb face = (vy <= 0.45f)
                        ? MixRgb(cTop, cMid, vy / 0.45f)
                        : MixRgb(cMid, cLow, (vy - 0.45f) / 0.55f);

                    float fr = (float)face.r;
                    float fg = (float)face.g;
                    float fb = (float)face.b;

                    // Lower oblique deep band (0.64 -> 0.58 across width)
                    const float ux = Clamp01((px - ox) / fmaxf(1.0f, (float)w));
                    const float bandY = oy + hh * (0.64f - 0.06f * ux);
                    if (py >= bandY)
                    {
                        fr = fr * 0.55f + (float)cDeep.r * 0.45f;
                        fg = fg * 0.55f + (float)cDeep.g * 0.45f;
                        fb = fb * 0.55f + (float)cDeep.b * 0.45f;
                    }

                    // Optional split body below headBandH (e.g., move / info card body)
                    if (headBandH > 0 && py >= oy + (float)headBandH)
                    {
                        const float bodyT = Clamp01((py - (oy + (float)headBandH)) / fmaxf(1.0f, hh - (float)headBandH));
                        const VitaUiRgb bodyTop = MixRgb(effBase, white, 0.88f);
                        const VitaUiRgb bodyLow = MixRgb(effBase, white, 0.74f);
                        const VitaUiRgb bodyCol = MixRgb(bodyTop, bodyLow, bodyT);
                        if (fabsf(py - (oy + (float)headBandH)) < 0.7f)
                        {
                            fr = (float)ink.r;
                            fg = (float)ink.g;
                            fb = (float)ink.b;
                        }
                        else
                        {
                            fr = (float)bodyCol.r;
                            fg = (float)bodyCol.g;
                            fb = (float)bodyCol.b;
                        }
                    }

                    // Upper glass gloss (0..0.5h, alpha 0.16)
                    if (vy < 0.5f)
                    {
                        const float gl = (1.0f - vy * 2.0f) * 0.16f;
                        fr = fr * (1.0f - gl) + 255.0f * gl;
                        fg = fg * (1.0f - gl) + 255.0f * gl;
                        fb = fb * (1.0f - gl) + 255.0f * gl;
                    }

                    // Inner highlight rim around inset = 1.9
                    const float rimDist = fabsf(dInner + 0.7f);
                    if (rimDist < 0.75f)
                    {
                        const float fade = (vy < 0.55f) ? (1.0f - vy * 1.15f) : ((vy > 0.8f) ? 0.55f : 0.18f);
                        const float rimA = (1.0f - rimDist / 0.75f) * 0.65f * fade;
                        fr = fr * (1.0f - rimA) + 255.0f * rimA;
                        fg = fg * (1.0f - rimA) + 255.0f * rimA;
                        fb = fb * (1.0f - rimA) + 255.0f * rimA;
                    }

                    // Subtle 2px scanlines (0.045)
                    if (((j - PLATE_PAD_T) & 1U) == 0U)
                    {
                        fr *= 0.955f;
                        fg *= 0.955f;
                        fb *= 0.955f;
                    }

                    const float inv = 1.0f - covInner;
                    rAcc = rAcc * inv + fr * covInner;
                    gAcc = gAcc * inv + fg * covInner;
                    bAcc = bAcc * inv + fb * covInner;
                    aAcc = aAcc * inv + covInner;
                }

                outAlpha = (int32_t)(Clamp01(aAcc) * 255.0f + 0.5f);
                const size_t idx = (size_t)j * (size_t)boxW + (size_t)i;
                slot->p[idx] = (outAlpha > 0)
                    ? VitaUi_PackRgb565((int32_t)(rAcc + 0.5f), (int32_t)(gAcc + 0.5f), (int32_t)(bAcc + 0.5f))
                    : 0U;
                slot->a[idx] = (uint8_t)outAlpha;
            }

            if (i < boxW && outAlpha == 255)
            {
                if (start < 0) start = (int32_t)i;
            }
            else if (start >= 0)
            {
                if ((int32_t)i - start > best1 - best0)
                {
                    best0 = start;
                    best1 = (int32_t)i;
                }
                start = -1;
            }
        }
        slot->run[j * 2U] = (uint16_t)best0;
        slot->run[j * 2U + 1U] = (uint16_t)best1;
    }
    return true;
}

int32_t VitaUiPlate_Draw(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const int32_t x,
    const int32_t y,
    const int32_t w,
    const int32_t h,
    const int32_t chamfer,
    const VitaUiRgb base,
    const uint8_t state,
    const uint8_t blinkPhase,
    const int32_t headBandH)
{
    if (!sInitialized) VitaUiPlate_Init();
    if (w <= 2 || h <= 2)
    {
        return y;
    }

    const int32_t boxX = x - PLATE_PAD_L;
    const int32_t boxY = y - PLATE_PAD_T;
    const int32_t boxW = w + PLATE_PAD_L + PLATE_PAD_R;
    const int32_t boxH = h + PLATE_PAD_T + PLATE_PAD_B;
    if (clip != NULL && (boxX >= clip->x1 || boxY >= clip->y1 || boxX + boxW <= clip->x0 || boxY + boxH <= clip->y0))
    {
        return y + (state == VITA_PLATE_PRESSED ? VITA_PLATE_SINK_PX : 0);
    }

    const uint8_t ch = (uint8_t)(chamfer < 1 ? 1 : (chamfer > 64 ? 64 : chamfer));
    const uint8_t effBlink = (state == VITA_PLATE_FOCUS) ? (uint8_t)(blinkPhase & 1U) : 0U;
    const uint8_t hb = (uint8_t)(headBandH > 0 ? headBandH : 0);

    PlateSlot *found = NULL;
    PlateSlot *victim = &sPlateCache[0];
    for (int32_t i = 0; i < PLATE_CACHE_SLOTS; ++i)
    {
        PlateSlot *const s = &sPlateCache[i];
        if (s->p != NULL && s->w == (uint16_t)w && s->h == (uint16_t)h && s->chamfer == ch
            && s->state == state && s->blink == effBlink && s->headBandH == hb && SameRgb(s->base, base))
        {
            found = s;
            break;
        }
        if (s->age < victim->age)
        {
            victim = s;
        }
    }

    if (found == NULL)
    {
        if (!BuildProceduralPlate(victim, (uint16_t)w, (uint16_t)h, ch, base, state, effBlink, hb))
        {
            return y;
        }
        found = victim;
    }
    found->age = ++sCacheClock;

    if (dst != NULL)
    {
        BlitPremulSpanBox(dst, pitch, clip, boxX, boxY, found->boxW, found->boxH, found->p, found->a, found->run);
    }
    return y + (state == VITA_PLATE_PRESSED ? VITA_PLATE_SINK_PX : 0);
}

static const TintSlot *GetBtaTint(const uint8_t id, const VitaUiRgb colour, const VitaUiRgb ring)
{
    if (id >= VITA_BTA_COUNT || sBtaElems[id].alpha == NULL || sBtaElems[id].a == NULL)
    {
        return NULL;
    }

    TintSlot *victim = &sTintCache[0];
    for (int32_t i = 0; i < TINT_CACHE_SLOTS; ++i)
    {
        TintSlot *const s = &sTintCache[i];
        if (s->p != NULL && s->id == id && SameRgb(s->colour, colour) && SameRgb(s->ring, ring))
        {
            s->age = ++sCacheClock;
            return s;
        }
        if (s->age < victim->age)
        {
            victim = s;
        }
    }

    const BtaElem *const e = &sBtaElems[id];
    const size_t area = (size_t)e->w * (size_t)e->h;
    const size_t totalBytes = area * sizeof(uint16_t) + area + (size_t)e->h * 2U * sizeof(uint16_t);
    free(victim->p);
    victim->p = (uint16_t *)malloc(totalBytes);
    if (victim->p == NULL)
    {
        return NULL;
    }
    victim->a = (uint8_t *)(victim->p + area);
    victim->run = (uint16_t *)((void *)(victim->a + area));
    victim->id = id;
    victim->colour = colour;
    victim->ring = ring;
    victim->x = e->x;
    victim->y = e->y;
    victim->w = e->w;
    victim->h = e->h;

    const bool wide = (id >= VITA_BTA_FIGHT_WIDE && id < VITA_BTA_FIGHT_WIDE + 3U);
    const bool full = (id >= VITA_BTA_FIGHT_FULL && id < VITA_BTA_FIGHT_FULL + 3U);
    if ((wide || full) && !sRayWatermarkBuilt)
    {
        BuildRayquazaWatermark();
        sRayWatermarkBuilt = true;
    }
    const uint8_t clipId = wide ? VITA_BTA_CLIP_FIGHT_WIDE : VITA_BTA_CLIP_FIGHT_FULL;
    const int32_t sink = ((wide && id - VITA_BTA_FIGHT_WIDE == VITA_PLATE_PRESSED)
                       || (full && id - VITA_BTA_FIGHT_FULL == VITA_PLATE_PRESSED)) ? VITA_PLATE_SINK_PX : 0;
    const int32_t plateW = wide ? 224 : 284;
    const VitaUiRgb rayBodyRgb = VitaUi_ShadeRgb(sHueFight, 82);
    const VitaUiRgb rayLineRgb = VitaUi_ShadeRgb(sHueFight, 66);

    for (uint16_t j = 0U; j < e->h; ++j)
    {
        int32_t best0 = 0;
        int32_t best1 = 0;
        int32_t start = -1;

        for (uint16_t i = 0U; i <= e->w; ++i)
        {
            int32_t al = 0;
            if (i < e->w)
            {
                const size_t q = (size_t)j * (size_t)e->w + (size_t)i;
                const int32_t aCoef = e->a[q];
                const int32_t rCoef = (e->r != NULL) ? e->r[q] : 0;
                const uint16_t cRaw = (uint16_t)(e->c[q * 2U] | (e->c[q * 2U + 1U] << 8));
                const VitaUiRgb add = VitaUi_UnpackRgb565(cRaw);

                int32_t r = Div255((int32_t)colour.r * aCoef + (int32_t)ring.r * rCoef) + (int32_t)add.r;
                int32_t g = Div255((int32_t)colour.g * aCoef + (int32_t)ring.g * rCoef) + (int32_t)add.g;
                int32_t b = Div255((int32_t)colour.b * aCoef + (int32_t)ring.b * rCoef) + (int32_t)add.b;
                al = e->alpha[q];

                if (wide || full)
                {
                    const int32_t px = (int32_t)e->x + (int32_t)i;
                    const int32_t py = (int32_t)e->y + (int32_t)j;
                    const int32_t rx = px - (plateW - 84);
                    const int32_t ry = py - (sink - 1);
                    if (rx >= 0 && ry >= 0 && rx < RAY_SIZE && ry < RAY_SIZE)
                    {
                        const int32_t cMask = BtaAlphaAt(clipId, px, py - sink);
                        const int32_t cb = Div255(Div255((int32_t)sRayBody[ry * RAY_SIZE + rx] * cMask) * 217);
                        const int32_t cl = Div255(Div255((int32_t)sRayLines[ry * RAY_SIZE + rx] * cMask) * 153);
                        r = Div255(r * (255 - cb) + (int32_t)rayBodyRgb.r * cb);
                        g = Div255(g * (255 - cb) + (int32_t)rayBodyRgb.g * cb);
                        b = Div255(b * (255 - cb) + (int32_t)rayBodyRgb.b * cb);
                        r = Div255(r * (255 - cl) + (int32_t)rayLineRgb.r * cl);
                        g = Div255(g * (255 - cl) + (int32_t)rayLineRgb.g * cl);
                        b = Div255(b * (255 - cl) + (int32_t)rayLineRgb.b * cl);
                    }
                }
                victim->p[q] = VitaUi_PackRgb565(r, g, b);
                victim->a[q] = (uint8_t)al;
            }

            if (i < e->w && al == 255)
            {
                if (start < 0) start = (int32_t)i;
            }
            else if (start >= 0)
            {
                if ((int32_t)i - start > best1 - best0)
                {
                    best0 = start;
                    best1 = (int32_t)i;
                }
                start = -1;
            }
        }
        victim->run[j * 2U] = (uint16_t)best0;
        victim->run[j * 2U + 1U] = (uint16_t)best1;
    }

    victim->age = ++sCacheClock;
    return victim;
}

int32_t VitaUiPlate_DrawBtaPlate(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t baseId,
    const int32_t x,
    const int32_t y,
    const int32_t fallbackW,
    const int32_t fallbackH,
    const VitaUiRgb colour,
    const uint8_t state,
    const uint8_t blinkPhase)
{
    if (!sInitialized) VitaUiPlate_Init();
    const uint8_t effState = (state > VITA_PLATE_PRESSED) ? VITA_PLATE_NORMAL : state;
    const VitaUiRgb ring = (effState == VITA_PLATE_FOCUS) ? sFocusRing[blinkPhase & 1U] : (VitaUiRgb){0, 0, 0};
    const TintSlot *const tint = GetBtaTint((uint8_t)(baseId + effState), colour, ring);

    if (tint != NULL)
    {
        BlitPremulSpanBox(dst, pitch, clip, x + tint->x, y + tint->y, tint->w, tint->h, tint->p, tint->a, tint->run);
        return y + (effState == VITA_PLATE_PRESSED ? VITA_PLATE_SINK_PX : 0);
    }

    return VitaUiPlate_Draw(dst, pitch, clip, x, y, fallbackW, fallbackH, 7, colour, state, blinkPhase, 0);
}

void VitaUiPlate_DrawBtaElem(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t elemId,
    const int32_t x,
    const int32_t y,
    const VitaUiRgb colour)
{
    if (!sInitialized) VitaUiPlate_Init();
    const TintSlot *const tint = GetBtaTint(elemId, colour, (VitaUiRgb){0, 0, 0});
    if (tint != NULL)
    {
        BlitPremulSpanBox(dst, pitch, clip, x + tint->x, y + tint->y, tint->w, tint->h, tint->p, tint->a, tint->run);
    }
}

void VitaUiPlate_Prewarm(void)
{
    if (!sInitialized)
        VitaUiPlate_Init();

    const VitaUiRgb kPrewarmHeader   = { 28,  58,  52 };
    const VitaUiRgb kPrewarmDock     = { 24,  50,  46 };
    const VitaUiRgb kPrewarmSage     = { 88, 146, 126 };
    const VitaUiRgb kPrewarmSageDark = { 52,  94,  82 };
    const VitaUiRgb kPrewarmCream    = { 242, 246, 241 };

    // 1. Header bar and Dock bar base
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 320, 22, 0, kPrewarmHeader, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 320, 28, 0, kPrewarmDock, VITA_PLATE_NORMAL, 0, 0);

    // 2. Dock button sizes (8-slot: 37x22, 9-slot: 32x22)
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 37, 22, 3, kPrewarmSage, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 37, 22, 3, kPrewarmSage, VITA_PLATE_FOCUS, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 37, 22, 3, kPrewarmSageDark, VITA_PLATE_NORMAL, 0, 0);

    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 32, 22, 3, kPrewarmSage, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 32, 22, 3, kPrewarmSage, VITA_PLATE_FOCUS, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 32, 22, 3, kPrewarmSageDark, VITA_PLATE_NORMAL, 0, 0);

    // 3. Header buttons (RUN / Quick item / Time)
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 78, 18, 3, kPrewarmSageDark, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 59, 18, 3, kPrewarmSage, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 59, 18, 3, kPrewarmSage, VITA_PLATE_FOCUS, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 59, 18, 3, kPrewarmSageDark, VITA_PLATE_NORMAL, 0, 0);

    // 4. Action pills (Exit / Back / Cancel)
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 84, 20, 2, kPrewarmSage, VITA_PLATE_NORMAL, 0, 0);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 94, 20, 2, kPrewarmSage, VITA_PLATE_NORMAL, 0, 0);

    // 5. Common cream card panels (PokéNav / Bag / Summary)
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 106, 163, 4, kPrewarmCream, VITA_PLATE_NORMAL, 0, 15);
    VitaUiPlate_Draw(NULL, 0, NULL, 0, 0, 198, 163, 4, kPrewarmCream, VITA_PLATE_NORMAL, 0, 0);
}
