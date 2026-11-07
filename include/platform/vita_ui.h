#ifndef GUARD_VITA_UI_H
#define GUARD_VITA_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VITA_UI_WIDTH  320
#define VITA_UI_HEIGHT 240

enum VitaUiSection
{
    VITA_UI_SEC_MAP = 0,
    VITA_UI_SEC_POKEDEX,
    VITA_UI_SEC_POKEMON,
    VITA_UI_SEC_BAG,
    VITA_UI_SEC_POKENAV,
    VITA_UI_SEC_CARD,
    VITA_UI_SEC_SAVE,
    VITA_UI_SEC_OPTIONS,
    VITA_UI_SEC_NUZLOCKE,
    VITA_UI_SEC_COUNT
};

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

void VitaUI_Init(void);
void VitaUI_Prewarm(void);
bool VitaUI_IsOpen(void);
void VitaUI_Open(enum VitaUiSection section);
void VitaUI_Close(void);
void VitaUI_Toggle(void);
enum VitaUiSection VitaUI_GetSection(void);

// Standalone options mode (invoked from Main Menu before entering CB2_Overworld)
void VitaUI_OpenStandaloneOptions(void);
bool VitaUI_IsStandaloneOptions(void);

// Opens Map tab directly in HM02 Fly target selection mode
void VitaUI_OpenFlyMap(uint8_t partySlot);

// Opens Party tab directly in Bag item target selection mode
void VitaUI_OpenPartyForBagItem(uint16_t itemId);

// Opens Bag tab directly in Poké Mart sell mode
void VitaUI_OpenShopSell(uint8_t shopTaskId);
bool VitaUI_IsShopSellActive(void);

// Called when START is pressed in overworld
bool VitaUI_OnStartMenu(void);

// Updates logic and inputs; returns true if state or visual changed
bool VitaUI_Update(uint32_t buttons);

// Renders the UI into the provided RGB565 buffer (320x240 pixels)
void VitaUI_Render(uint16_t *dest, size_t pitchPixels);

// Snapshot + DirtyRect incremental renderer: returns true when [outY0, outY1) changed on *outCanvas.
bool VitaUI_RenderDirty(const uint16_t **outCanvas, int32_t *outY0, int32_t *outY1);
void VitaUI_Invalidate(void);

// Returns true if running always (RUN toggle in header)
bool VitaUI_IsRunAlwaysActive(void);

// Input & Save path helpers (replaces legacy VitaMenu_*)
uint32_t VitaUI_ActionButton(int32_t action);
uint16_t VitaUI_MapButtons(uint32_t buttons);
bool VitaUI_FastForwardActive(uint32_t buttons);
void VitaUI_GetSavePath(char *dest, size_t size);

// Platform-side reset/quit hooks implemented in sdl2.c
void VitaPlatform_RequestReset(void);
void VitaPlatform_RequestQuit(void);

// Game-side field/map bridge helpers
bool VitaGame_CanFlyToMapSec(uint16_t mapSec);
bool VitaGame_ExecuteFlyToMapSec(uint8_t partySlot, uint16_t mapSec, uint8_t posWithinMapSec);
bool VitaGame_TryUseFieldMove(uint8_t partySlot, uint16_t moveId, uint8_t *outFailReason);

#endif // GUARD_VITA_UI_H
