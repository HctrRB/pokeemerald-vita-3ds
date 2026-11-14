// Button mapping, fast forward, always-run and the save path. Kept from the old 3DS-style Vita
// menu (vita_ui.c) when the new UI (src/platform/ui/) replaced it.

#include <stdint.h>
#include <stdio.h>
#include "vita_platform.h"
#include "platform/vita_controls.h"
#include "platform/vita_extras.h"
#include "platform/ui.h"

#include "global.h"
#include "event_data.h"
#include "platform.h"
#include "constants/flags.h"

uint32_t VitaControls_ActionButton(const int32_t action)
{
    if (action < 0 || action >= VITA_ACTION_COUNT)
        return 0U;
    const u8 bit = Platform_GetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + action));
    return bit < 16 ? (1U << bit) : 0U;   // VITA_BTN_UNBOUND: no button
}

uint16_t VitaControls_MapButtons(const uint32_t buttons)
{
    static const uint16_t sGbaKeys[VITA_ACTION_COUNT] = {
        A_BUTTON, B_BUTTON, L_BUTTON, R_BUTTON, START_BUTTON, SELECT_BUTTON, 0, 0
    };
    uint16_t keys = 0;
    if (buttons & VITA_BTN_UP)    keys |= DPAD_UP;
    if (buttons & VITA_BTN_DOWN)  keys |= DPAD_DOWN;
    if (buttons & VITA_BTN_LEFT)  keys |= DPAD_LEFT;
    if (buttons & VITA_BTN_RIGHT) keys |= DPAD_RIGHT;

    for (int32_t a = 0; a < VITA_ACTION_COUNT; a++)
    {
        if ((buttons & VitaControls_ActionButton(a)) != 0U)
            keys |= sGbaKeys[a];
    }
    return keys;
}

bool VitaControls_FastForwardActive(const uint32_t buttons)
{
    static bool sToggled = false;
    static bool sWasPressed = false;
    if (Ui_IsOpen())
        return false;

    const bool pressed = (buttons & VitaControls_ActionButton(VITA_ACTION_FAST_FORWARD)) != 0U;
    if (!Platform_GetSetting(PLATFORM_SETTING_FAST_FORWARD_TOGGLE))
    {
        sToggled = false;
        sWasPressed = pressed;
        return pressed;
    }
    if (pressed && !sWasPressed)
        sToggled = !sToggled;
    sWasPressed = pressed;
    return sToggled;
}

bool VitaControls_IsRunAlwaysActive(void)
{
    return (VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0) && (FlagGet(FLAG_SYS_B_DASH) == TRUE);
}

void VitaControls_GetSavePath(char *dest, const size_t size)
{
    const int32_t file = (int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE);
    char rel[32];
    if (file <= 1)
        snprintf(rel, sizeof(rel), "saves/pokeemerald.sav");
    else
        snprintf(rel, sizeof(rel), "saves/pokeemerald_%d.sav", (int32_t)file);
    VitaPlatform_BuildPath(dest, size, rel);
}
