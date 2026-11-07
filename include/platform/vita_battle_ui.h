#ifndef GUARD_VITA_BATTLE_UI_H
#define GUARD_VITA_BATTLE_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/vita_ui_plate.h"

#ifdef __cplusplus
extern "C" {
#endif

enum VitaBattleUiMode
{
    VITA_BATTLE_MODE_NONE = 0,
    VITA_BATTLE_MODE_ACTION,    // FIGHT, BAG, POKEMON, RUN / Safari
    VITA_BATTLE_MODE_MOVES,     // 4 Move plates with PP & effectiveness + CANCEL
    VITA_BATTLE_MODE_TARGET,    // Doubles target selector
    VITA_BATTLE_MODE_LEVEL_UP,  // +delta stat gain card
    VITA_BATTLE_MODE_LEARN_MOVE // 5th move replace prompt
};

void VitaBattleUi_Init(void);
void VitaBattleUi_Shutdown(void);

// Returns true if the battle UI deck is currently active
bool VitaBattleUi_IsActive(void);

// Updates touch and button input for the battle UI deck
bool VitaBattleUi_Update(uint32_t buttons);

// Renders the battle bottom action deck onto a 320x240 RGB565 canvas
void VitaBattleUi_Render(uint16_t *dst, size_t pitch, const VitaUiClip *clip);

// Sets the current UI deck mode
void VitaBattleUi_SetMode(enum VitaBattleUiMode mode);
enum VitaBattleUiMode VitaBattleUi_GetMode(void);

// Battle speed toggle (1x..4x)
uint8_t VitaBattleUi_GetSpeed(void);
void VitaBattleUi_CycleSpeed(void);

#ifdef __cplusplus
}
#endif

#endif // GUARD_VITA_BATTLE_UI_H
