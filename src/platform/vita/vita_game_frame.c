// The game frame (include/platform/game_frame.h): captured on Core 1, pinned and read on Core 0.
#include <string.h>

#include "global.h"
#include "main.h"
#include "palette.h"
#include "window.h"
#include "constants/field_effects.h"
#include "platform/game_frame.h"
#include "platform/ui.h"
#include "overworld.h"
#include "platform.h"

// The game's own answers (Core 1 only).
extern bool8 BattleTransition_IsActiveForUi(void);
extern bool8 Shop_IsBuyMenuActive(void);
extern bool8 Decoration_IsFieldModeActive(void);
extern bool8 Overworld_IsReturningToField(void);
extern bool8 FieldEffectActiveListContains(u8 id);
extern u8 GetFieldMessageBoxMode(void);
extern u8 GetMapNamePopUpWindowId(void);
extern bool16 IsTextPrinterActive(u8 id);

/*
 * Four slots: the one Core 0 has pinned, the latest published (which Core 0 may be about to
 * pin), the one being written, and a spare. Core 1 never writes the latest nor the pinned slot.
 * Pinning stores the index and checks that it is still the latest: once it is, Core 1 sees the
 * pin before it chooses its next slot.
 */
// Cortex-A9 / PL310 cache lines are 32 bytes: what Core 1 writes (a slot, sLatest) and what
// Core 0 writes (sPinned) never share one, so neither core's writes evict the other's reads.
#define CACHE_LINE 32

GameFrame gGameFrameSlots[GAME_FRAME_SLOTS] __attribute__((aligned(CACHE_LINE)));
const GameFrame *gGameFrameCore0 = &gGameFrameSlots[0];                 // Core 0's pinned frame
static int32_t sLatest __attribute__((aligned(CACHE_LINE)));            // written by Core 1
static uint32_t sMapEpoch;                                              // map mutations begun (Core 1)
static int32_t sPinned __attribute__((aligned(CACHE_LINE))) = -1;       // written by Core 0

void GameFrame_BeginMapMutation(void)
{
    __atomic_store_n(&sMapMutationLock, 1u, __ATOMIC_SEQ_CST);
    __atomic_store_n(&sMapEpoch, sMapEpoch + 1, __ATOMIC_SEQ_CST);
}

// `voxel`: the 3D field is on; without it Core 0 draws the GBA frame and needs none of the
// sprite palettes (nor the entities, the weather or the fade, VoxelWorld / VoxelEntities).
// Full-screen GBA scenes shown in the GBA's own format (UI plan, principle 6): mini-games, the
// contest, the Pokéblock eating scene, the Battle Dome tree and Factory screens, the berry tag,
// the Union Room chat and the system screens. Each scene file answers for its own callbacks.
bool8 BerryBlender_IsGbaFormatForUi(void);
bool8 BerryCrush_IsGbaFormatForUi(void);
bool8 PokemonJump_IsGbaFormatForUi(void);
bool8 DodrioBerryPicking_IsGbaFormatForUi(void);
bool8 SlotMachine_IsGbaFormatForUi(void);
bool8 Roulette_IsGbaFormatForUi(void);
bool8 Contest_IsGbaFormatForUi(void);
bool8 ContestResults_IsGbaFormatForUi(void);
bool8 ContestPainting_IsGbaFormatForUi(void);
bool8 PokeblockFeed_IsGbaFormatForUi(void);
bool8 DomeTourneyTree_IsGbaFormatForUi(void);
bool8 FactoryScreen_IsGbaFormatForUi(void);
bool8 BerryTag_IsGbaFormatForUi(void);
bool8 UnionRoomChat_IsGbaFormatForUi(void);
bool8 SaveFailed_IsGbaFormatForUi(void);
bool8 ResetRtc_IsGbaFormatForUi(void);
bool8 ClearSaveData_IsGbaFormatForUi(void);
bool8 BerryFix_IsGbaFormatForUi(void);
bool8 HallOfFame_IsGbaFormatForUi(void);
bool8 Diploma_IsGbaFormatForUi(void);
bool8 WallClock_IsGbaFormatForUi(void);
bool8 StarterChoose_IsGbaFormatForUi(void);
bool8 MysteryGift_IsGbaFormatForUi(void);
bool8 MysteryEvent_IsGbaFormatForUi(void);
bool8 TrainerCard_IsGbaFormatForUi(void);
bool8 TradeMenu_IsGbaFormatForUi(void);

static bool8 GbaFormatScene(void)
{
    return BerryBlender_IsGbaFormatForUi()
        || BerryCrush_IsGbaFormatForUi()
        || PokemonJump_IsGbaFormatForUi()
        || DodrioBerryPicking_IsGbaFormatForUi()
        || SlotMachine_IsGbaFormatForUi()
        || Roulette_IsGbaFormatForUi()
        || Contest_IsGbaFormatForUi()
        || ContestResults_IsGbaFormatForUi()
        || ContestPainting_IsGbaFormatForUi()
        || PokeblockFeed_IsGbaFormatForUi()
        || DomeTourneyTree_IsGbaFormatForUi()
        || FactoryScreen_IsGbaFormatForUi()
        || BerryTag_IsGbaFormatForUi()
        || UnionRoomChat_IsGbaFormatForUi()
        || SaveFailed_IsGbaFormatForUi()
        || ResetRtc_IsGbaFormatForUi()
        || ClearSaveData_IsGbaFormatForUi()
        || BerryFix_IsGbaFormatForUi()
        || HallOfFame_IsGbaFormatForUi()
        || Diploma_IsGbaFormatForUi()
        || WallClock_IsGbaFormatForUi()
        || StarterChoose_IsGbaFormatForUi()
        || MysteryGift_IsGbaFormatForUi()
        || MysteryEvent_IsGbaFormatForUi()
        || TrainerCard_IsGbaFormatForUi()
        || TradeMenu_IsGbaFormatForUi();
}

// The 16:9 contexts: the field, the battle, the stage scenes and the 2D scenes with mirrors
extern bool8 EvolutionScene_IsActiveForUi(void);
extern bool8 EggHatch_IsActiveForUi(void);
extern bool8 Trade_IsAnimActiveForUi(void);
extern void BattleMainCB2(void);
static bool8 WideContext(bool8 stageActive)
{
    return gMain.callback2 == CB2_Overworld || gMain.callback2 == CB2_OverworldBasic
        || (gMain.inBattle && gMain.callback2 == BattleMainCB2) || stageActive
        || EvolutionScene_IsActiveForUi() || EggHatch_IsActiveForUi() || Trade_IsAnimActiveForUi();
}

// Latched: once a GBA-format scene starts, the format stays until a 16:9 context is back, so the
// loading and closing steps between them never switch it for a frame (smooth transitions)
static bool8 sGbaFormatLatched;
static bool8 GbaFormatLatched(bool8 stageActive)
{
    if (GbaFormatScene())
        sGbaFormatLatched = TRUE;
    else if (WideContext(stageActive))
        sGbaFormatLatched = FALSE;
    return sGbaFormatLatched;
}

static void CaptureOverlay(GameFrameOverlay *o, bool8 voxel)
{
    o->battleTransition = BattleTransition_IsActiveForUi();
    o->buyMenu = Shop_IsBuyMenuActive();
    o->decorationMode = Decoration_IsFieldModeActive();
    o->returningToField = Overworld_IsReturningToField();
    o->fieldMoveShowMon = FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON)
                       || FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    o->flyAnim = FieldEffectActiveListContains(FLDEFF_USE_FLY)
              || FieldEffectActiveListContains(FLDEFF_FLY_IN);
    o->messageBox = GetFieldMessageBoxMode() != 0;      // FIELD_MESSAGE_BOX_HIDDEN
    o->mapNamePopUp = GetMapNamePopUpWindowId() != WINDOW_NONE;
    o->textPrinter0 = IsTextPrinterActive(0);
    o->bg0Window = FALSE;
    for (int32_t w = 1; w < WINDOWS_MAX; w++)
    {
        if (gWindows[w].window.bg == 0 && gWindows[w].tileData != NULL)
        {
            o->bg0Window = TRUE;
            break;
        }
    }
    VitaStage_CaptureFrame(&o->stageActive, &o->leavesScene);
    o->gbaWindow = UiCore_SceneInWindow();
    o->gbaFormat = o->gbaWindow || GbaFormatLatched(o->stageActive);
    if (o->stageActive)
        memcpy(o->bgPaletteFaded, gPlttBufferFaded, sizeof(o->bgPaletteFaded));
    if (voxel)
    {
        memcpy(o->objPaletteUnfaded, &gPlttBufferUnfaded[256], sizeof(o->objPaletteUnfaded));
        memcpy(o->objPaletteExtUnfaded, &gPlttBufferUnfaded[PLTT_EXT_ID(0)], sizeof(o->objPaletteExtUnfaded));
    }
}

void GameFrame_CaptureAndPublish(void)
{
    const int32_t latest = __atomic_load_n(&sLatest, __ATOMIC_RELAXED);
    const int32_t pinned = __atomic_load_n(&sPinned, __ATOMIC_SEQ_CST);
    int32_t slot = (latest + 1) % GAME_FRAME_SLOTS;

    while (slot == latest || slot == pinned)
        slot = (slot + 1) % GAME_FRAME_SLOTS;

    GameFrame *const frame = &gGameFrameSlots[slot];
    const GameFrame *const previous = &gGameFrameSlots[latest];
#ifdef VOXEL_SUPPORTED
    const bool8 voxel = Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D) != 0;
#else
    const bool8 voxel = FALSE;
#endif

    frame->sequence = previous->sequence + 1;
    frame->voxel = voxel;
    frame->field.mapEpoch = __atomic_load_n(&sMapEpoch, __ATOMIC_RELAXED);
    CaptureOverlay(&frame->overlay, voxel);
#ifdef VOXEL_SUPPORTED
    VoxelWorld_CaptureField(&frame->field, &frame->weather, &frame->fade, voxel);
    VoxelBattle_Capture(&frame->battle, voxel);
    if (voxel)
        VoxelEntities_Capture(&frame->entities, &previous->entities, frame->overlay.buyMenu);
#endif
    __atomic_store_n(&sLatest, slot, __ATOMIC_SEQ_CST);

    // The map loader holds Core 0 off the layout until a frame of the new map is published.
    __atomic_store_n(&sMapMutationLock, 0u, __ATOMIC_RELEASE);
}

const GameFrame *GameFrame_LastPublished(void)
{
    return &gGameFrameSlots[__atomic_load_n(&sLatest, __ATOMIC_RELAXED)];
}

void GameFrame_Pin(void)
{
    int32_t idx = __atomic_load_n(&sLatest, __ATOMIC_SEQ_CST);

    for (;;)
    {
        __atomic_store_n(&sPinned, idx, __ATOMIC_SEQ_CST);
        const int32_t check = __atomic_load_n(&sLatest, __ATOMIC_SEQ_CST);
        if (check == idx)
            break;
        idx = check;
    }
    gGameFrameCore0 = &gGameFrameSlots[idx];
}

void GameFrame_Unpin(void)
{
    // Core 0 reads no frame until it pins again (MainThread_ParkForReset)
    __atomic_store_n(&sPinned, -1, __ATOMIC_RELEASE);
}

bool8 GameFrame_MapSettled(void)
{
    if (__atomic_load_n(&sMapMutationLock, __ATOMIC_ACQUIRE) != 0u)
        return FALSE;
    return GameFrame_Get()->field.mapEpoch == __atomic_load_n(&sMapEpoch, __ATOMIC_ACQUIRE);
}
