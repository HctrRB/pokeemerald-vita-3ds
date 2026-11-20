// Core of the new UI on Core 1 (the game thread). Ui_Tick runs once per game frame from
// VBlankIntrWait, between two game frames, so it can read and change game state directly:
// no locks, no snapshots (docs/reference/THREADS.md).
//
// START in the field opens the hub (the options button, OPTIONS). The world keeps running
// and rendering behind it; the player and the NPCs are frozen while it is open (decision D4).

#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_extras.h"
#include "platform/vita_controls.h"
#include "../vita/vita_platform.h"
#include "../vita/vita_perf.h"

#include "global.h"
#include "main.h"
#include "overworld.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "field_specials.h"
#include "link.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "sprite.h"
#include "task.h"
#include "field_screen_effect.h"
#include "scanline_effect.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define REPEAT_DELAY 18
#define REPEAT_RATE 5
#define DRAG_THRESHOLD 6

static bool sOpen;                    // written by Core 1, read anywhere
#define UI_NAV_DEPTH 8
static const UiScreen *sStack[UI_NAV_DEPTH];   // screen stack: the top one is updated and drawn
static int32_t sDepth;
static bool sCloseRequested;
static bool sFromScript;              // opened by a game script (mart): it owns the freeze and the controls
static void (*sAfterClose)(void);     // field action run once the UI is closed (bike, FLY...)
static UiInput sInput;
static uint32_t sPrevHeld = ~0u;      // ~0: ignore buttons held when the UI opens
static uint32_t sRepeatTicks;
static bool sPrevTouch;
static bool sIgnoreTouch;
static uint32_t sCircleTicks;
static int32_t sLastTy;
static float sAvgTickMs;
static bool sHosted;                  // the UI owns the game loop (naming screen...): see UiCore_OpenHosted
static uint32_t sRenderMsBits;        // float bits, written by Core 0

void Ui_Init(void)
{
    UiAssets_Load();
    // Publish one empty list so Core 0 always has a valid one to read
    UiDraw_Begin();
    UiDraw_Publish(0.0f);
}

bool Ui_IsEnabled(void)
{
    return UiAssets_Ready();
}

bool Ui_IsOpen(void)
{
    return __atomic_load_n(&sOpen, __ATOMIC_RELAXED);
}

bool Ui_ConsumesInput(void)
{
    return Ui_IsOpen();
}

float UiCore_LastRenderMs(void)
{
    float ms;
    const uint32_t bits = __atomic_load_n(&sRenderMsBits, __ATOMIC_RELAXED);
    memcpy(&ms, &bits, sizeof(ms));
    return ms;
}

void UiCore_SetLastRenderMs(float ms)
{
    uint32_t bits;
    memcpy(&bits, &ms, sizeof(bits));
    __atomic_store_n(&sRenderMsBits, bits, __ATOMIC_RELAXED);
}

float UiCore_AvgTickMs(void)
{
    return sAvgTickMs;
}

static void ReadInput(void)
{
    UiInput *in = &sInput;
    const uint32_t held = VitaPlatform_ReadButtons();
    const uint32_t prev = (sPrevHeld == ~0u) ? held : sPrevHeld;
    in->held = held;
    in->pressed = held & ~prev;
    in->repeat = in->pressed;
    const uint32_t dpad = VITA_BTN_UP | VITA_BTN_DOWN | VITA_BTN_LEFT | VITA_BTN_RIGHT;
    if (held & dpad)
    {
        if (++sRepeatTicks >= REPEAT_DELAY && (sRepeatTicks - REPEAT_DELAY) % REPEAT_RATE == 0)
            in->repeat |= held & dpad;
    }
    else
        sRepeatTicks = 0;
    sPrevHeld = held;

    int32_t tx = 0, ty = 0;
    bool touch = VitaPlatform_ReadTouch(&tx, &ty);
    if (sIgnoreTouch)
    {
        // Until that finger is lifted the touch screen is ignored (no tap, no drag)
        if (!touch)
            sIgnoreTouch = false;
        in->touching = in->touchDown = in->tap = in->dragging = false;
        in->dragDy = 0;
        sPrevTouch = false;
        return;
    }
    in->touchDown = touch && !sPrevTouch;
    in->tap = false;
    in->dragDy = 0;
    if (in->touchDown)
    {
        in->downX = tx;
        in->downY = ty;
        in->dragging = false;
        sLastTy = ty;
    }
    if (touch)
    {
        if (!in->dragging && (tx - in->downX > DRAG_THRESHOLD || in->downX - tx > DRAG_THRESHOLD ||
                              ty - in->downY > DRAG_THRESHOLD || in->downY - ty > DRAG_THRESHOLD))
            in->dragging = true;
        if (in->dragging)
            in->dragDy = ty - sLastTy;
        sLastTy = ty;
        in->tx = tx;
        in->ty = ty;
    }
    else if (sPrevTouch && !in->dragging)
        in->tap = true;   // released without dragging: tx/ty keep the last point
    in->touching = touch;
    sPrevTouch = touch;
}

// Places whose START menu is the game's own (link and Union rooms, the Battle Pike): their entries
// (link cards...) and rules are not the hub's. The Battle Pyramid's (REST, RETIRE) are the hub's.
bool Ui_OwnsFieldStart(void)
{
    return Ui_IsEnabled() && !IsOverworldLinkActive() && !InUnionRoom() && !InMultiPartnerRoom() && !InBattlePike();
}

static bool CanOpenFromField(void)
{
    return gMain.callback2 == CB2_Overworld && !ArePlayerFieldControlsLocked() && !gPaletteFade.active;
}

static void EnterTop(void)
{
    sPrevHeld = ~0u;                  // buttons held across a screen change are not new presses
    sPrevTouch = true;
    sIgnoreTouch = true;              // a finger still down from the last screen is not a tap here
    if (sStack[sDepth - 1]->enter)
        sStack[sDepth - 1]->enter();
}

void UiNav_Push(const UiScreen *screen)
{
    if (sDepth >= UI_NAV_DEPTH)
        return;
    sStack[sDepth++] = screen;
    EnterTop();
}

void UiNav_Replace(const UiScreen *screen)
{
    if (sDepth == 0)
        return;
    sStack[sDepth - 1] = screen;
    EnterTop();
}

void UiNav_Pop(void)
{
    if (sDepth > 0)
        sDepth--;
    if (sDepth == 0)
        sCloseRequested = true;
    else
    {
        sPrevHeld = ~0u;
        sPrevTouch = true;
    sIgnoreTouch = true;              // a finger still down from the last screen is not a tap here
        if (sStack[sDepth - 1]->resume)
            sStack[sDepth - 1]->resume();
    }
}

void UiNav_Close(void (*afterClose)(void))
{
    sCloseRequested = true;
    sAfterClose = afterClose;
}

static void Open(const UiScreen *screen)
{
    // Same freeze as the game's START menu (ShowStartMenu): NPCs and the player stop, the world
    // (water, weather, day/night, particles) keeps animating.
    if (!sFromScript)
    {
        FreezeObjectEvents();
        PlayerFreeze();
        StopPlayerAvatar();
        LockPlayerFieldControls();
    }
    sDepth = 0;
    sCloseRequested = false;
    sAfterClose = NULL;
    UiKit_ClearToast();
    UiNav_Push(screen);
    __atomic_store_n(&sOpen, true, __ATOMIC_RELAXED);
    PlaySE(SE_WIN_OPEN);
}

void UiCore_OpenFromScript(const UiScreen *screen, void (*afterClose)(void))
{
    if (Ui_IsOpen())
        return;
    sFromScript = true;
    Open(screen);
    sAfterClose = afterClose;
}

// Hosted screens replace a whole GBA scene (naming screen, ...) outside the field and the battle
// menus: the game loop runs this callback (tasks, sprites, fades) while the UI is open; the screen
// ends with UiNav_Close(after), where `after` puts the game's return callback back. The GBA picture
// left behind is whatever the caller faded to (black in every caller).
static void CB2_UiHost(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

bool UiCore_IsHostCallback(void)
{
    return gMain.callback2 == CB2_UiHost;
}

void UiCore_OpenHosted(const UiScreen *screen, void (*afterClose)(void))
{
    if (Ui_IsOpen())
        return;
    SetMainCallback2(CB2_UiHost);
    sHosted = true;
    sFromScript = true;
    Open(screen);
    sAfterClose = afterClose;
}

// ---------- A GBA scene started from screens on the field, and the same screens afterwards ----------
// (the Pokéblock eating scene, the berry tag...). The whole stack of screens is kept, the UI closes,
// the field fades to black and is stopped as the GBA menus that open those scenes stop it, then
// `start` runs the scene. Its exit callback calls UiCore_ReopenAfterField: back on the field the
// same stack opens again (each screen keeps its state; `resume` refreshes its data). In a window:
// the scene is drawn in UI_SCENE_WIN_* inside the theme's frame instead of full screen.
static void (*sSceneStart)(void);
static void (*sReopenPrepare)(void);
static const UiScreen *sSavedStack[UI_NAV_DEPTH];
static int32_t sSavedDepth;
static bool sSceneWindow;
static const char *sSceneTitle;

static void Task_FadeToScene(u8 taskId)
{
    if (gTasks[taskId].data[0] == 0)
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].data[0] = 1;
        return;
    }
    if (gPaletteFade.active)
        return;
    DestroyTask(taskId);
    void (*start)(void) = sSceneStart;
    sSceneStart = NULL;
    CleanupOverworldWindowsAndTilemaps();
    ScanlineEffect_Stop();
    ResetTasks();
    gMain.state = 0;                   // the scenes' loaders are gMain.state switches
    if (start != NULL)
        start();
}

static void FadeToScene(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    CreateTask(Task_FadeToScene, 80);
}

static void CloseToScene(void (*start)(void), bool window, const char *title)
{
    // Only screens over the field come back on the field (a hosted screen belongs to its scene)
    for (int32_t i = 0; i < sDepth && i < UI_NAV_DEPTH; i++)
        sSavedStack[i] = sStack[i];
    sSavedDepth = (sHosted || gMain.callback2 != CB2_Overworld) ? 0 : sDepth;
    sSceneStart = start;
    sSceneWindow = window;
    sSceneTitle = title;
    UiNav_Close(FadeToScene);
}

void UiCore_CloseToScene(void (*start)(void))
{
    CloseToScene(start, false, NULL);
}

void UiCore_CloseToSceneInWindow(void (*start)(void), const char *title)
{
    CloseToScene(start, true, title);
}

// The scene runs in its window: from the start of the scene until the field is back
bool UiCore_SceneInWindow(void)
{
    return sSceneWindow && gMain.callback2 != CB2_Overworld;
}

// The kept stack, open again as the START menu opens (it unfreezes the field when it closes)
static void RestoreStack(void)
{
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();
    LockPlayerFieldControls();
    sFromScript = false;
    sHosted = false;
    sCloseRequested = false;
    sAfterClose = NULL;
    UiKit_ClearToast();
    for (int32_t i = 0; i < sSavedDepth; i++)
        sStack[i] = sSavedStack[i];
    sDepth = sSavedDepth;
    sSavedDepth = 0;
    sPrevHeld = ~0u;
    sPrevTouch = true;
    sIgnoreTouch = true;
    // Bottom to top: the data under each screen may have changed during the scene
    for (int32_t i = 0; i < sDepth; i++)
        if (sStack[i]->resume)
            sStack[i]->resume();
    __atomic_store_n(&sOpen, true, __ATOMIC_RELAXED);
}

static void Task_ReopenAfterField(u8 taskId)
{
    if (gPaletteFade.active)
        return;
    DestroyTask(taskId);
    sSceneWindow = false;
    if (sSavedDepth > 0 && Ui_IsEnabled() && !Ui_IsOpen())
    {
        RestoreStack();
        if (sReopenPrepare != NULL)
            sReopenPrepare();               // after: the screens' state is back (a toast survives)
    }
    else
    {
        sSavedDepth = 0;
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
    }
    sReopenPrepare = NULL;
}

static void FieldCB_ReopenAfterField(void)
{
    LockPlayerFieldControls();
    FadeInFromBlack();
    CreateTask(Task_ReopenAfterField, 80);
}

void UiCore_ReopenAfterField(void (*prepare)(void))
{
    sReopenPrepare = prepare;
    gFieldCallback = FieldCB_ReopenAfterField;
    SetMainCallback2(CB2_ReturnToField);
}

// While a scene runs in its window and no screen is open: the theme's frame around it (the scene
// itself is drawn by Core 0 in UI_SCENE_WIN_*, under this list; nothing here covers it)
static bool DrawSceneWindow(void)
{
    if (!UiCore_SceneInWindow())
        return false;
    const int32_t x = UI_SCENE_WIN_X, y = UI_SCENE_WIN_Y, w = UI_SCENE_WIN_W, h = UI_SCENE_WIN_H, e = 6;
    UiDraw_Rect(0, 0, UI_SCREEN_W, y - e, UI_COL_FRAME1);
    UiDraw_Rect(0, y + h + e, UI_SCREEN_W, UI_SCREEN_H - y - h - e, UI_COL_FRAME1);
    UiDraw_Rect(0, y - e, x - e, h + 2 * e, UI_COL_FRAME1);
    UiDraw_Rect(x + w + e, y - e, UI_SCREEN_W - x - w - e, h + 2 * e, UI_COL_FRAME1);
    UiDraw_Frame(x - e, y - e, w + 2 * e, h + 2 * e, e, gUiTheme->paperEdge);
    UiDraw_Frame(x - 2, y - 2, w + 4, h + 4, 2, gUiTheme->selEdge);
    if (sSceneTitle != NULL)
    {
        const int32_t tw = UiText_Width(UI_FACE_PS2P, 16, sSceneTitle) + 32;
        UiW_Capsule((UI_SCREEN_W - tw) / 2, y - e - 44, tw, 34);
        UiText_DrawMid(UI_FACE_PS2P, 16, (UI_SCREEN_W - tw) / 2 + 16, y - e - 44, 34, sSceneTitle, UI_COL_TITLE);
    }
    return true;
}

void UiCore_OpenScreen(const UiScreen *screen)
{
    if (!Ui_IsOpen())
        Open(screen);
}

static void Close(void)
{
    __atomic_store_n(&sOpen, false, __ATOMIC_RELAXED);
    sDepth = 0;
    sCloseRequested = false;
    if (!sFromScript)
    {
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
    }
    sFromScript = false;
    sHosted = false;
    UiGame_BattleMenuEnd();
    UiGame_ViewParty();
    UiGame_UsePyramidBag(false);
    UiStall_Closed();
    UiDraw_Begin();
    UiDraw_Publish(0.0f);
}

void Ui_OnReset(void)
{
    __atomic_store_n(&sOpen, false, __ATOMIC_RELAXED);
    UiGame_ViewParty();
    UiGame_UsePyramidBag(false);
    sDepth = 0;
    sCloseRequested = false;
    sFromScript = false;
    sHosted = false;
    sAfterClose = NULL;
    // A GBA scene started from the UI and its way back: forgotten (the scene is gone)
    sSceneStart = NULL;
    sReopenPrepare = NULL;
    sSavedDepth = 0;
    sSceneWindow = false;
    sPrevHeld = ~0u;
    (void)Ui_TakeInjectedKeys();
    UiKit_ClearToast();
    UiSign_Reset();
    UiStall_Closed();
    UiDraw_Begin();
    UiDraw_Publish(0.0f);
}

void Ui_Tick(void)
{
    UiBattle_Tick();
    if (!Ui_IsOpen())
    {
        if (!Ui_IsEnabled())
            return;
        // Overlays while no screen is open: zone sign and the mirrors of the game's windows
        static bool sOverlayShown;
        UiDraw_Begin();
        UiSpr_BeginTick();
        bool any = DrawSceneWindow();
        any |= UiSign_Draw();
        any |= UiBattle_Active() ? UiBattle_Draw() : UiMirror_Draw();
        if (any || sOverlayShown)
            UiDraw_Publish(0.0f);
        sOverlayShown = any;
        const uint32_t held = VitaPlatform_ReadButtons();
        const uint16_t mappedHeld = VitaControls_MapButtons(held);
        const bool resetPrimer = (mappedHeld & (A_BUTTON | B_BUTTON | SELECT_BUTTON)) == (A_BUTTON | B_BUTTON | SELECT_BUTTON);
        const bool startNew = !resetPrimer && (held & VITA_BTN_START) && !(sPrevHeld & VITA_BTN_START) && sPrevHeld != ~0u;
        // The options button (OPTIONS > remap, square by default) goes straight to OPTIONS
        const uint32_t menuButton = VitaControls_ActionButton(VITA_ACTION_MENU);
        const bool menuNew = menuButton && (held & menuButton) && !(sPrevHeld & menuButton) && sPrevHeld != ~0u;
        sPrevHeld = held;
        if (startNew && CanOpenFromField() && Ui_OwnsFieldStart())
            Open(&gUiHubScreen);
        else if (menuNew && CanOpenFromField())
            Open(&gUiOptionsScreen);
        if (!Ui_IsOpen())
            return;
    }

    const double t0 = VitaPerf_NowMs();
    ReadInput();
    if (gMain.callback2 != CB2_Overworld && !(gMain.inBattle && UiGame_InBattleMenu()) && !(sHosted && gMain.callback2 == CB2_UiHost))
    {
        Close();
        return;
    }
    // Escape hatch: circle held for a second closes the whole UI (never stuck in a menu)
    sCircleTicks = (sInput.held & VITA_BTN_CIRCLE) ? sCircleTicks + 1 : 0;
    if (sCircleTicks == 60 && !sFromScript)
    {
        Close();
        return;
    }
    const UiScreen *top = sStack[sDepth - 1];
    UiStall_Tick(top, sAvgTickMs);
    if (!top->update(&sInput))
        UiNav_Pop();
    if (sCloseRequested || sDepth == 0)
    {
        void (*after)(void) = sAfterClose;
        sAfterClose = NULL;
        Close();
        if (after != NULL)
            after();
        return;
    }
    UiDraw_Begin();
    UiSpr_BeginTick();
    UiW_GbaPageReset();
    sStack[sDepth - 1]->build();
    UiW_GbaPageFlush();                // a GBA page whose screen drew no footer clock
    UiKit_DrawToast();
    const float ms = (float)(VitaPerf_NowMs() - t0);
    sAvgTickMs = sAvgTickMs * 0.9f + ms * 0.1f;
    UiDraw_Publish(ms);
}
