#ifndef GUARD_VITA_CONTROLS_H
#define GUARD_VITA_CONTROLS_H

// Button mapping, fast forward, always-run and the save path (vita_controls.c), plus the game-side
// bridges the new UI uses for Fly and field moves. What was left of the old 3DS-style Vita menu,
// replaced by the new UI (src/platform/ui/).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Remappable physical actions (stored in PLATFORM_SETTING_BUTTON_FIRST + action)
enum VitaAction
{
    VITA_ACTION_A = 0,
    VITA_ACTION_B,
    VITA_ACTION_L,
    VITA_ACTION_R,
    VITA_ACTION_START,
    VITA_ACTION_SELECT,
    VITA_ACTION_FAST_FORWARD,
    VITA_ACTION_MENU,
    VITA_ACTION_COUNT
};

enum VitaFieldMoveFail
{
    VITA_FMOVE_OK = 0,
    VITA_FMOVE_NEED_BADGE,
    VITA_FMOVE_CANT_HERE,
    VITA_FMOVE_LINK_ACTIVE
};

// The Vita button bound to an action (0 when unbound)
uint32_t VitaControls_ActionButton(int32_t action);
// Vita buttons -> GBA keys, through the bindings
uint16_t VitaControls_MapButtons(uint32_t buttons);
// Fast forward held (or toggled, per OPTIONS); off while a menu of the new UI is open
bool VitaControls_FastForwardActive(uint32_t buttons);
// Running without holding B (AUTO RUN extra, once the RUNNING SHOES are owned)
bool VitaControls_IsRunAlwaysActive(void);
// Path of the save file chosen in OPTIONS
void VitaControls_GetSavePath(char *dest, size_t size);

// Platform-side reset/quit hooks implemented in sdl2.c
void VitaPlatform_RequestReset(void);
void VitaPlatform_RequestQuit(void);

// Game-side field/map bridge helpers
bool VitaGame_CanFlyToMapSec(uint16_t mapSec);
bool VitaGame_ExecuteFlyToMapSec(uint8_t partySlot, uint16_t mapSec, uint8_t posWithinMapSec);
bool VitaGame_TryUseFieldMove(uint8_t partySlot, uint16_t moveId, uint8_t *outFailReason);

#endif // GUARD_VITA_CONTROLS_H
