#ifndef PLATFORM_UI_H
#define PLATFORM_UI_H

// New UI system (docs/reference/VITA_UI.md, design in design/stitch/).
// Logic runs on Core 1 (the game thread), drawing on Core 0. Core 1 builds a list of quads once
// per game frame and publishes it through a lock-free triple buffer; Core 0 draws the latest
// published list over the frame (3D or 2D) in one textured draw call.
// The only menu system of the port (the old 3DS-style Vita menu was removed on 2026-11-13). If
// its atlas cannot be loaded, the game's own GBA screens are used.

#include <stdbool.h>
#include <stdint.h>

// Core 0, at boot, before the game thread starts: loads the atlas (app0:ui2/ui_atlas.bin).
void Ui_Init(void);

// True when the UI's atlas is loaded (the UI can be shown). Any thread.
bool Ui_IsEnabled(void);

// GBA window frame (0-19) that goes with the menu theme (ui_theme.c), for text_window.c
uint8_t Ui_GbaWindowFrame(void);

// A GBA scene shown in a window over the theme's frame (UiCore_CloseToSceneInWindow): Core 1
// publishes it in the game frame (GameFrameOverlay.gbaWindow), Core 0 draws the picture here (3:2)
#define UI_SCENE_WIN_X 180
#define UI_SCENE_WIN_Y 104
#define UI_SCENE_WIN_W 600
#define UI_SCENE_WIN_H 400
bool UiCore_SceneInWindow(void);

// Core 1, once per game frame from VBlankIntrWait: input, screens, actions, publishes the list.
void Ui_Tick(void);

// Core 1, during SoftReset(): clears open screens, zone signs and draw lists before reboot.
void Ui_OnReset(void);

// Core 1: START in the field opens the hub (else the game's START menu: link and Union rooms,
// the Battle Pike and Pyramid, or no UI atlas).
bool Ui_OwnsFieldStart(void);

// True while a UI screen is open. Any thread (relaxed read).
bool Ui_IsOpen(void);

// Core 1 (Platform_GetKeyInput): the open UI takes the buttons, the game gets none.
bool Ui_ConsumesInput(void);

// Core 0, after the 3D passes or the 2D frame, before the swap: draws the latest list.
void Ui_Render(int32_t viewWidth, int32_t viewHeight);

// Core 1, from the shop script (shop.c): opens the new mart screen (BUY or SELL tab) over the
// field; when it closes the shop task gets Shop_ReturnFromNewUiMart. False if it cannot open.
bool Ui_OpenMart(uint8_t shopTaskId, bool sell, uint8_t martType, const uint16_t *items, uint16_t count);   // martType: shop.c MART_TYPE_*
// Core 1, OPTION on the title menu (option_menu.c): the options screen, hosted; `after` takes the
// game back to the main menu. False if it cannot open (the GBA option menu is used).
bool Ui_OpenOptionsHosted(void (*after)(void));
// Core 1, from ShowSelectMovePokemonSummaryScreen (pokemon_summary_screen.c): "which move to
// forget?" for mons[monIndex] learning newMove; `callback` takes over when it closes. False if it
// cannot open (the summary screen is used).
// Core 1, from the player's PC (player_pc.c): ITEM STORAGE (tab 0) or MAILBOX (tab 1); the PC's
// list comes back when it closes (PlayerPC_ReturnFromUi). False if it cannot open.
enum { UI_PC_TAB_ITEMS, UI_PC_TAB_MAIL, UI_PC_TAB_DECORATION };
enum { UI_PC_CENTER, UI_PC_BEDROOM, UI_PC_SECRET_BASE };   // POKéMON CENTER: ITEMS, MAILBOX; bedroom: + DECORATION; secret base: DECORATION
bool Ui_OpenPlayerPc(uint8_t taskId, uint8_t tab, uint8_t kind);
struct Pokemon;
bool Ui_OpenForgetMove(struct Pokemon *mons, uint8_t monIndex, uint16_t newMove, void (*callback)(void));
// The MOVE RELEARNER (move_relearner.c, POKéMON in VAR_0x8004): its move list, hosted. False: the
// GBA scene. Results as the scene leaves them (VAR_0x8004 TRUE when a move was learned).
bool Ui_OpenMoveRelearner(void);
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
enum { UI_CHOOSE_TUTOR, UI_CHOOSE_DAYCARE, UI_CHOOSE_ANY, UI_CHOOSE_CONTEST, UI_CHOOSE_RELEARNER, UI_CHOOSE_COUNT };
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
// A BAG a script opens to ask for an item (item_menu.c GoToBagMenu: berry tree, Favor and Quiz
// Ladies, Apprentice): the BAG in "choose" mode, hosted; pocket < POCKETS_COUNT keeps it to one
// pocket. The game side (ItemMenu_*ForUi) holds each location's rules.
bool Ui_OpenBagChoice(uint8_t pocket);
// Easy Chat (easy_chat.c DoEasyChatScreen): mail, profile, interviews, trendy saying...
bool Ui_OpenEasyChat(uint8_t type, uint16_t *words, void (*exitCallback)(void), uint8_t personType);
bool Ui_OpenBattleBag(void);
bool Ui_OpenBattleParty(void);
// POKéBLOCK CASE (ui_scr_pokeblock.c), Core 1. False: the GBA case is used.
// From pokeblock.c's OpenPokeblockCase: the Safari feeder and the Contest Lady (PBLOCK_CASE_*),
// replacing the case scene; it sets what the GBA case sets and then `callback`.
bool Ui_OpenPokeblockCase(uint8_t caseId, void (*callback)(void));
// SELECT on the field (item_use.c): the case over the field; closing it unfreezes the field.
bool Ui_OpenPokeblockCaseField(void);
// Safari battle (battle_controller_safari.c): the case as a battle menu; gSpecialVar_ItemId as the
// GBA case leaves it. The controller waits for !Ui_IsOpen().
bool Ui_OpenPokeblockCaseBattle(void);
// After a catch (battle_script_commands.c Cmd_displaydexinfo): the new species' POKéDEX page over
// the battle; the command waits for !Ui_IsOpen(). False: the GBA caught page.
bool Ui_OpenCaughtDexPage(uint16_t nationalNum);

// Battle HUD of the new UI: GBA layers to leave out of the frame (Core 0; DISPCNT_BG*_ON bits)
uint16_t Ui_BattleDispcntClear(void);

#endif // PLATFORM_UI_H
