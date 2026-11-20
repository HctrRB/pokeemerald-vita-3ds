#ifndef GUARD_PLATFORM_GAME_FRAME_H
#define GUARD_PLATFORM_GAME_FRAME_H

/*
 * The game frame: everything the other cores may know about the game, as it stood at the end of
 * one game frame (docs/reference/THREADS.md, "The game frame").
 *
 * The game runs on Core 1 alone. At each VBlank, Core 1 captures what Core 0 needs (each module
 * fills its own part: VoxelWorld_CaptureField, VoxelBattle_Capture, VoxelEntities_Capture...)
 * and publishes it. Core 0 pins the latest frame once per rendered frame (GameFrame_Pin) and
 * reads only that pinned copy, so every pass of a rendered frame sees the same game frame and
 * Core 1 never writes the slot Core 0 reads.
 *
 * The only other ways the game is seen from Core 0 / Core 2 are read-only and declared in
 * tools/core_boundary/allow.txt: the emulated GBA display (VRAM, OAM, palettes, registers) and
 * the current map's grid under sMapMutationLock. tools/core_boundary checks it on every build.
 */

#include "global.h"
#include "palette.h"
#include "platform/voxel_snapshot.h"

#define GAME_FRAME_BATTLERS 4

// The field: which map, and whether the field is what the screen shows.
typedef struct
{
    bool8 saveLoaded;            // gSaveBlock1Ptr is set
    bool8 fieldShown;            // the overworld is on screen (or the shop / a return to it)
    bool8 overworldCallback;     // CB2_Overworld / CB2_OverworldBasic
    bool8 battleSceneShown;      // in a battle, on BattleMainCB2 (not its 2D menus)
    s16 mapGroup;                // -1 without a save
    s16 mapNum;
    u16 mapLayoutId;             // the save block's (setmaplayoutindex may swap it)
    u32 mapEpoch;                // map mutations begun before this frame (GameFrame_BeginMapMutation)
    u32 layoutSwapMask;          // VoxelWorld: bit i set when its layout swap i applies
    float playerTileX;           // the player's map-local tile (object event coordinates)
    float playerTileZ;
    u8 playerFacing;             // DIR_*
    bool8 playerSurfing;
    const void *callback2;       // gMain.callback2, compared, never called (widescreen.c)
} GameFrameField;

// Weather, as the voxel world shows it.
typedef struct
{
    u8 weatherClass;             // VoxelWeatherClass
    float amount;                // 0..1, the sprites coming in and out
    float lightning;             // 0..1
    float fogDensity;            // 0..1
} GameFrameWeather;

// The screen fade the world follows (VoxelWorld_ScreenFade).
typedef struct
{
    bool8 fading;
    float amount;
    float rgb[3];
} GameFrameFade;

// What the 2D layer over the world shows (Pass_Overlay2D, RenderGameTexture).
typedef struct
{
    bool8 battleTransition;      // BattleTransition_IsActiveForUi
    bool8 buyMenu;               // Shop_IsBuyMenuActive
    bool8 decorationMode;        // Decoration_IsFieldModeActive
    bool8 returningToField;      // Overworld_IsReturningToField
    bool8 fieldMoveShowMon;      // FLDEFF_FIELD_MOVE_SHOW_MON(_INIT)
    bool8 flyAnim;               // FLDEFF_USE_FLY / FLDEFF_FLY_IN
    bool8 messageBox;            // GetFieldMessageBoxMode() != hidden
    bool8 mapNamePopUp;          // GetMapNamePopUpWindowId() != WINDOW_NONE
    bool8 textPrinter0;          // IsTextPrinterActive(0)
    bool8 bg0Window;             // a window other than 0 on BG0 holds tiles
    bool8 stageActive;           // a Vita stage (the intro's scenes) owns the VBlank callback
    bool8 leavesScene;           // that stage is the intro's falling leaves
    bool8 gbaFormat;             // a full-screen GBA scene shown 816x544 with black bars (*_IsGbaFormatForUi)
    bool8 gbaWindow;             // that scene in the UI's window (UiCore_SceneInWindow, UI_SCENE_WIN_*)
    u16 bgPaletteFaded[256];     // gPlttBufferFaded's BG half (the leaves scene's colours)
    u16 objPaletteUnfaded[256];  // gPlttBufferUnfaded's OBJ half (sprite cards keep their colours)
    u16 objPaletteExtUnfaded[PLTT_EXT_COUNT * 16];  // the extended sprite palettes, unfaded
} GameFrameOverlay;

// The battle.
typedef struct
{
    bool8 inBattle;              // gMain.inBattle
    bool8 isLink;                // BATTLE_TYPE_LINK
    bool8 isWild;                // not a trainer, link, recorded link... battle (VoxelBattle_BeginStage)
    bool8 critical;              // gCritMultiplier > 1
    u8 moveType;                 // the type of the move being used (TYPE_NORMAL when unknown)
    u16 hp[GAME_FRAME_BATTLERS];
    u16 maxHp[GAME_FRAME_BATTLERS];
    u32 shadowCount;
    struct { float x, y, rx, ry; } shadows[GAME_FRAME_BATTLERS];
} GameFrameBattle;

typedef struct
{
    u32 sequence;                // counts the published frames
    bool8 voxel;                 // captured with the 3D field on: entities, weather, fade, sprite palettes
    GameFrameField field;
    GameFrameWeather weather;
    GameFrameFade fade;
    GameFrameOverlay overlay;
    GameFrameBattle battle;
    VoxelEntitySnapshot entities;
} __attribute__((aligned(32))) GameFrame;

// ── Core 1 (the game thread) ─────────────────────────────────────────────────
// Each part is filled by the module that owns it (VoxelEntities_Capture: voxel_snapshot.h).
// `voxel`: the 3D field is on; without it only what the 2D frame needs is captured.
void VoxelWorld_CaptureField(GameFrameField *field, GameFrameWeather *weather, GameFrameFade *fade, bool8 voxel);
void VoxelBattle_Capture(GameFrameBattle *battle, bool8 voxel);
void VitaStage_CaptureFrame(bool8 *stageActive, bool8 *leavesScene);

// The map loader is about to change the current map (overworld.c): Core 0 keeps drawing the
// instances it has until a frame captured after the change is published (GameFrame_MapSettled).
void GameFrame_BeginMapMutation(void);
// Captures the current game state and publishes it (VBlankIntrWait).
void GameFrame_CaptureAndPublish(void);
// The last frame Core 1 published (to carry part of it over to the next).
const GameFrame *GameFrame_LastPublished(void);

// ── Core 0 ───────────────────────────────────────────────────────────────────
// Pins the latest published frame for this rendered frame; until the next pin, GameFrame_Get()
// returns it and Core 1 leaves its slot alone.
void GameFrame_Pin(void);
void GameFrame_Unpin(void);
// The current map's grid (gMapHeader, gBackupMapLayout) is the pinned frame's map and no change
// of it is under way: Core 0 may read it.
bool8 GameFrame_MapSettled(void);
// The pinned frame. Inline: Core 0 asks it per sprite. Never from the game thread, which reads
// the game itself (tools/core_boundary fails the build on any path from DoMain to
// gGameFrameCore0).
#define GAME_FRAME_SLOTS 4
extern GameFrame gGameFrameSlots[GAME_FRAME_SLOTS];
extern const GameFrame *gGameFrameCore0;

static inline const GameFrame *GameFrame_Get(void)
{
    return gGameFrameCore0;
}

#endif // GUARD_PLATFORM_GAME_FRAME_H
