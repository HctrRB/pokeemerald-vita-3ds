// Zone sign of the new UI (design/stitch/_tools/_sign_badge.html): replaces the GBA map name popup
// (map_name_popup.c) when the new UI is on. It is an overlay, not a screen: the game keeps its
// input. The name sits on the GBA popup's own plate of that zone (wood, marble, stone, brick, sea),
// at x2; towns and cities add their sign's motto (the game's own text, ui_town_signs.h) in a box
// below, in the theme's regular-weight text face. Slides in, stays, slides out.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "ui_town_signs.h"
#include "platform/ui.h"

#include "global.h"
#include "constants/region_map_sections.h"

extern u8 MapNamePopUp_ThemeForUi(u16 mapSec, bool8 *underwater);

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
    const UiTheme *t = gUiTheme;
    bool8 underwater = FALSE;
    const uint8_t plateTheme = MapNamePopUp_ThemeForUi(sSec, &underwater);

    // The name: EMERALD keeps its title face (Press Start 2P 16), the others the sign face
    const bool pixel = t->faceSign == UI_FACE_VT323;
    const uint8_t nameFace = pixel ? UI_FACE_PS2P : t->faceSign, namePx = pixel ? 16 : UI_BODY_PX;
    const int32_t nameW = UiText_Width(nameFace, namePx, name);
    int32_t w = nameW + 96;
    if (w < 240) w = 240;
    if (w > 440) w = 440;
    const int32_t x = 16, y = 12 - off, plateH = 80;
    UiSpr_ZonePlate(plateTheme, underwater, x, y, w);
    UiText_DrawMid(nameFace, namePx, x + (w - nameW) / 2, y + 16, plateH - 32, name, UI_RGBA(43, 43, 51, 255));

    if (motto)
    {
        // The motto drops down under the plate once it has slid in: the theme's dark colour at
        // 67 %, the text face at regular weight, white
        const int32_t bw = w - 24 < 376 ? 376 : w - 24, bx = x + 12, by = y + plateH - 2, lineH = 30;
        const int32_t lines = UiW_TextWrappedF(t->faceSign, UI_BODY_PX, 0, 0, bw - 32, lineH, motto, 0, 4);
        const int32_t full = 22 + lines * lineH;
        int32_t drop = age <= SLIDE ? 0 : (age - SLIDE) * full / 10;
        if (drop > full) drop = full;
        if (sTicks < SLIDE) drop = full * sTicks / SLIDE;
        if (drop > 0)
        {
            UiDraw_PushClip(bx, by, bw, drop);
            UiDraw_Rect(bx, by, bw, full, (t->darkBot & 0x00FFFFFFu) | (171u << 24));
            UiDraw_Rect(bx, by + full - 2, bw, 2, UI_RGBA(0, 0, 0, 102));
            UiW_TextWrappedF(t->faceSign, UI_BODY_PX, bx + 16, by + 10, bw - 32, lineH, motto, UI_RGBA(255, 255, 255, 255), 4);
            UiDraw_PopClip();
        }
    }
    return true;
}
