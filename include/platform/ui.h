#ifndef PLATFORM_UI_H
#define PLATFORM_UI_H

// New UI system (docs/VITA_UI.md, design in design/stitch/).
// Logic runs on Core 1 (the game thread), drawing on Core 0. Core 1 builds a list of quads once
// per game frame and publishes it through a lock-free triple buffer; Core 0 draws the latest
// published list over the frame (3D or 2D) in one textured draw call.
// Behind the "ui_v2" extra ("NEW UI (TEST)", OFF by default): OFF keeps the current Vita UI.

#include <stdbool.h>
#include <stdint.h>

// Core 0, at boot, before the game thread starts: loads the atlas (app0:ui2/ui_atlas.bin).
void Ui_Init(void);

// True when the "ui_v2" extra is on. Any thread.
bool Ui_IsEnabled(void);

// Core 1, once per game frame from VBlankIntrWait: input, screens, actions, publishes the list.
void Ui_Tick(void);

// True while a UI screen is open. Any thread (relaxed read).
bool Ui_IsOpen(void);

// Core 1 (Platform_GetKeyInput): the open UI takes the buttons, the game gets none.
bool Ui_ConsumesInput(void);

// Core 0, after the 3D passes or the 2D frame, before the swap: draws the latest list.
void Ui_Render(int32_t viewWidth, int32_t viewHeight);

// Core 1, from the shop script (shop.c): opens the new mart screen (BUY or SELL tab) over the
// field; when it closes the shop task gets Shop_ReturnFrom3DsSell. False if the new UI is off.
bool Ui_OpenMart(uint8_t shopTaskId, bool sell, const uint16_t *items, uint16_t count);
// Core 1 (map_name_popup.c): the new UI's zone sign instead of the GBA popup when the UI is on
void Ui_ShowZoneSign(uint16_t mapSec);
// Core 0 (pass 6): true while every visible game window has a new-UI mirror (no GBA frame needed)
bool Ui_MirrorCovers(void);
// Core 1 (Platform_GetKeyInput): GBA keys a tap on a mirrored menu asks for (A to choose), once
uint16_t Ui_TakeInjectedKeys(void);
void Ui_HideZoneSign(void);
// Core 1, from the script specials of party_menu.c: the new "choose a POKéMON" screen. Sets the
// same results as the GBA screen (VAR_RESULT for the tutor, VAR_0x8004 for the others) and
// resumes the script. False if the new UI is off (the GBA screen opens).
enum { UI_CHOOSE_TUTOR, UI_CHOOSE_DAYCARE, UI_CHOOSE_ANY };
bool Ui_OpenChooseMon(int kind);

// Battle controller: the new BAG / POKEMON instead of the old Vita UI (false: not taken)
// Naming screen (naming_screen.c DoNamingScreen): true when the new UI took it; it writes the name
// to destBuffer as the GBA screen does and then sets returnCallback.
bool Ui_OpenNaming(uint8_t templateNum, uint8_t *destBuffer, uint16_t species, uint16_t gender, uint32_t personality, void (*returnCallback)(void));
// PC boxes (pokemon_storage_system.c, PC menu): option = OPTION_WITHDRAW / DEPOSIT / MOVE_MONS /
// MOVE_ITEMS. True when the new UI took it; it returns to the PC menu when closed.
bool Ui_OpenPc(uint8_t option);
// Berry for the BERRY BLENDER / BERRY CRUSH (item_menu.c ChooseBerryForMachine)
bool Ui_OpenBerryChoice(void (*exitCallback)(void));
// Easy Chat (easy_chat.c DoEasyChatScreen): mail, profile, interviews, trendy saying...
bool Ui_OpenEasyChat(uint8_t type, uint16_t *words, void (*exitCallback)(void), uint8_t personType);
bool Ui_OpenBattleBag(void);
bool Ui_OpenBattleParty(void);

// Battle HUD of the new UI: GBA layers to leave out of the frame (Core 0; DISPCNT_BG*_ON bits)
uint16_t Ui_BattleDispcntClear(void);

#endif // PLATFORM_UI_H
