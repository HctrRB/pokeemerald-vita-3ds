// Zone sign of the new UI (design/stitch/_tools/_sign_badge.html): replaces the GBA map name popup
// (map_name_popup.c) when the new UI is on. It is an overlay, not a screen: the game keeps its
// input. The name sits on the GBA popup's own plate of that zone (wood, marble, stone, brick, sea),
// at x2. Slides in, stays, slides out.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
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

void UiSign_Reset(void)
{
    sTicks = 0;
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
    const UiTheme *t = gUiTheme;
    bool8 underwater = FALSE;
    const uint8_t plateTheme = MapNamePopUp_ThemeForUi(sSec, &underwater);

    // The name: EMERALD keeps its title face (Press Start 2P 16), the others the sign face
    const bool pixel = t->faceSign == UI_FACE_VT323;
    const uint8_t nameFace = pixel ? UI_FACE_PS2P : t->faceSign;
    const uint8_t namePx = pixel ? 16 : (t->faceSign == 15 ? 32 : UI_BODY_PX);
    const int32_t nameW = UiText_InkWidth(nameFace, namePx, name);
    int32_t w = nameW + 96;
    if (w < 260) w = 260;
    if (w > 460) w = 460;
    const int32_t x = 16, y = 12 - off;
    UiSpr_ZonePlate(plateTheme, underwater, x, y, w);
    UiDraw_PushClip(x + 12, y + 6, w - 24, 58);
    UiText_DrawCentred(nameFace, namePx, x + 12, y + 6, w - 24, 58, name, UI_RGBA(43, 43, 51, 255));
    UiDraw_PopClip();
    return true;
}
