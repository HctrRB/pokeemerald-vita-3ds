// Zone sign of the new UI (design/stitch/cartel_zona): replaces the GBA map name popup
// (map_name_popup.c) when the new UI is on. It is an overlay, not a screen: the game keeps its
// input. Name strip tinted by the kind of area; towns and cities add their sign's motto (the game's
// own text, ui_town_signs.h) in a box below. Slides in, stays, slides out.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "ui_town_signs.h"
#include "platform/ui.h"

#include "global.h"
#include "constants/region_map_sections.h"

#define SLIDE 12
#define HOLD 150

static uint16_t sSec = MAPSEC_NONE;
static int32_t sTicks;           // > 0 while shown

void Ui_ShowZoneSign(uint16_t mapSec)
{
    sSec = mapSec;
    sTicks = SLIDE + HOLD + SLIDE;
}

void Ui_HideZoneSign(void)
{
    if (sTicks > SLIDE)
        sTicks = SLIDE;
}

static const char *Motto(uint16_t sec)
{
    for (size_t i = 0; i < sizeof(sTownSigns) / sizeof(sTownSigns[0]); i++)
        if (sTownSigns[i].mapSec == sec)
            return sTownSigns[i].motto;
    return NULL;
}

// Core 1, from Ui_Tick while no screen is open: draws into the list being built
bool UiSign_Draw(void)
{
    if (sTicks <= 0)
        return false;
    sTicks--;
    const int32_t total = SLIDE + HOLD + SLIDE;
    const int32_t age = total - sTicks;
    int32_t off = 0;                               // slide offset (px above the screen)
    if (age < SLIDE) off = (SLIDE - age) * 200 / SLIDE;
    else if (sTicks < SLIDE) off = (SLIDE - sTicks) * 200 / SLIDE;

    char name[32];
    UiGame_MapSecName(sSec, name, sizeof(name));
    const char *motto = Motto(sSec);
    const uint32_t tint = sSec <= MAPSEC_EVER_GRANDE_CITY ? UI_RGBA(79, 178, 122, 255)
                        : (sSec >= MAPSEC_ROUTE_101 && sSec <= MAPSEC_ROUTE_134) ? UI_RGBA(201, 165, 90, 255)
                                                                                : UI_RGBA(88, 167, 201, 255);
    const int32_t x = 16, y = 16 - off, w = 400;
    // Name strip
    UiDraw_Rect(x + 3, y + 4, w, 48, UI_RGBA(0, 0, 0, 110));
    UiDraw_Rect(x, y, w, 48, UI_RGBA(4, 18, 16, 255));
    UiDraw_RectV(x + 2, y + 2, w - 4, 44, UI_RGBA(18, 58, 49, 255), UI_RGBA(10, 36, 30, 255));
    UiDraw_Rect(x, y, 8, 48, tint);
    UiText_Draw(UI_FACE_PS2P, 16, x + 24, y + 16, name, UI_COL_ON_FRAME);
    if (motto)
    {
        // The motto drops down under the name once it has slid in: half-transparent box,
        // Press Start 2P 16 (one step smaller than the body text)
        const int32_t lines = UiW_TextWrappedF(UI_FACE_PS2P, 16, 0, 0, w - 40, 24, motto, 0, 4);
        const int32_t full = 16 + lines * 24;
        int32_t drop = age <= SLIDE ? 0 : (age - SLIDE) * full / 10;
        if (drop > full) drop = full;
        if (sTicks < SLIDE) drop = full * sTicks / SLIDE;
        if (drop > 0)
        {
            UiDraw_PushClip(x + 8, y + 48, w - 8, drop);
            UiDraw_Rect(x + 8, y + 48, w - 8, full, UI_RGBA(9, 33, 29, 150));
            UiDraw_Rect(x + 8, y + 48 + full - 2, w - 8, 2, UI_RGBA(43, 88, 73, 200));
            UiW_TextWrappedF(UI_FACE_PS2P, 16, x + 24, y + 48 + 10, w - 40, 24, motto, UI_COL_ON_SOFT, 4);
            UiDraw_PopClip();
        }
    }
    return true;
}
