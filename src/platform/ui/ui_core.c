// Core of the new UI on Core 1 (the game thread). Ui_Tick runs once per game frame from
// VBlankIntrWait, between two game frames, so it can read and change game state directly:
// no locks, no snapshots (docs/THREADS.md).
//
// Phase B: START opens the test screen when the "ui_v2" extra is on. The world keeps running
// and rendering behind it; the player and the NPCs are frozen while it is open (decision D4).

#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_extras.h"
#include "platform/vita_ui.h"
#include "../vita/vita_platform.h"
#include "../vita/vita_perf.h"

#include "global.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
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
    return UiAssets_Ready() && VitaExtras_GetFast(FAST_EXTRA_UI_V2) != 0;
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

static bool CanOpenFromField(void)
{
    return gMain.callback2 == CB2_Overworld && !VitaUI_IsOpen() && !ArePlayerFieldControlsLocked() && !gPaletteFade.active;
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
    UiGame_BattleMenuEnd();
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
        bool any = UiSign_Draw();
        any |= UiBattle_Active() ? UiBattle_Draw() : UiMirror_Draw();
        if (any || sOverlayShown)
            UiDraw_Publish(0.0f);
        sOverlayShown = any;
        const uint32_t held = VitaPlatform_ReadButtons();
        const bool startNew = (held & VITA_BTN_START) && !(sPrevHeld & VITA_BTN_START) && sPrevHeld != ~0u;
        sPrevHeld = held;
        if (startNew && CanOpenFromField())
            Open(&gUiHubScreen);
        if (!Ui_IsOpen())
            return;
    }

    const double t0 = VitaPerf_NowMs();
    ReadInput();
    if (gMain.callback2 != CB2_Overworld && !(gMain.inBattle && UiGame_InBattleMenu()))
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
    sStack[sDepth - 1]->build();
    UiKit_DrawToast();
    const float ms = (float)(VitaPerf_NowMs() - t0);
    sAvgTickMs = sAvgTickMs * 0.9f + ms * 0.1f;
    UiDraw_Publish(ms);
}
