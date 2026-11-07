#ifndef GUARD_VITA_UI_PLATE_H
#define GUARD_VITA_UI_PLATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} VitaUiRgb;

typedef struct
{
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
} VitaUiClip;

enum VitaPlateState
{
    VITA_PLATE_NORMAL   = 0,
    VITA_PLATE_FOCUS    = 1,
    VITA_PLATE_PRESSED  = 2,
    VITA_PLATE_DISABLED = 3
};

enum VitaBtaElementId
{
    VITA_BTA_BACKDROP        = 0,
    VITA_BTA_FIGHT_WIDE      = 16,
    VITA_BTA_FIGHT_FULL      = 19,
    VITA_BTA_BALL            = 22,
    VITA_BTA_BOTTOM          = 25,
    VITA_BTA_MOVE            = 28,
    VITA_BTA_CANCEL          = 31,
    VITA_BTA_TARGET          = 34,
    VITA_BTA_CLIP_FIGHT_WIDE = 40,
    VITA_BTA_CLIP_FIGHT_FULL = 41,
    VITA_BTA_ICON_BAG        = 48,
    VITA_BTA_ICON_RUN        = 49,
    VITA_BTA_ICON_NEAR       = 50,
    VITA_BTA_ICON_BLOCK      = 51,
    VITA_BTA_BALL_OK         = 56,
    VITA_BTA_BALL_STATUS     = 57,
    VITA_BTA_BALL_FAINT      = 58,
    VITA_BTA_BALL_EMPTY      = 59,
    VITA_BTA_COUNT           = 64
};

enum
{
    VITA_PLATE_SINK_PX = 2
};

static inline uint16_t VitaUi_PackRgb565(const int32_t r, const int32_t g, const int32_t b)
{
    const int32_t rc = r < 0 ? 0 : (r > 255 ? 255 : r);
    const int32_t gc = g < 0 ? 0 : (g > 255 ? 255 : g);
    const int32_t bc = b < 0 ? 0 : (b > 255 ? 255 : b);
    return (uint16_t)((((rc * 31 + 127) / 255) << 11)
                    | (((gc * 63 + 127) / 255) << 5)
                    | ((bc * 31 + 127) / 255));
}

static inline VitaUiRgb VitaUi_UnpackRgb565(const uint16_t c)
{
    const VitaUiRgb out = {
        (uint8_t)(((c >> 11) & 31U) * 255U / 31U),
        (uint8_t)(((c >> 5) & 63U) * 255U / 63U),
        (uint8_t)((c & 31U) * 255U / 31U)
    };
    return out;
}

static inline uint16_t VitaUi_Bgr555ToRgb565(const uint16_t bgr)
{
    const uint32_t r5 = bgr & 31U;
    const uint32_t g5 = (bgr >> 5) & 31U;
    const uint32_t b5 = (bgr >> 10) & 31U;
    const uint32_t g6 = (g5 << 1) | (g5 >> 4);
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static inline VitaUiRgb VitaUi_ShadeRgb(const VitaUiRgb c, const int32_t keepPct)
{
    const VitaUiRgb out = {
        (uint8_t)((int32_t)c.r * keepPct / 100),
        (uint8_t)((int32_t)c.g * keepPct / 100),
        (uint8_t)((int32_t)c.b * keepPct / 100)
    };
    return out;
}

void VitaUiPlate_Init(void);
void VitaUiPlate_Shutdown(void);
void VitaUiPlate_Prewarm(void);

bool VitaUiPlate_HasMenuBackdrop(void);
bool VitaUiPlate_HasBattleBackdrop(void);

void VitaUiPlate_BlitMenuBackdrop(uint16_t *dst, size_t pitch, const VitaUiClip *clip);
void VitaUiPlate_BlitBattleBackdrop(uint16_t *dst, size_t pitch, const VitaUiClip *clip);

// Draws a procedural Emerald 3DS chamfered bevelled plate (cached by geometry + colour + state).
// Returns vertical content Y offset (y + VITA_PLATE_SINK_PX when pressed, y otherwise).
int32_t VitaUiPlate_Draw(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    int32_t chamfer,
    VitaUiRgb base,
    uint8_t state,
    uint8_t blinkPhase,
    int32_t headBandH);

// Draws a decomposed EM3DBTA1 battle plate/icon/ball element using row-major RLE Tint cache.
int32_t VitaUiPlate_DrawBtaPlate(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint8_t baseId,
    int32_t x,
    int32_t y,
    int32_t fallbackW,
    int32_t fallbackH,
    VitaUiRgb colour,
    uint8_t state,
    uint8_t blinkPhase);

void VitaUiPlate_DrawBtaElem(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint8_t elemId,
    int32_t x,
    int32_t y,
    VitaUiRgb colour);

VitaUiRgb VitaUiPlate_FocusRingRgb(uint8_t blinkPhase);

#endif // GUARD_VITA_UI_PLATE_H
