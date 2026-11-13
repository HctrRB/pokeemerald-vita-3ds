// Mirrors of the game's field windows (design/stitch/dialogo,
// si_no). The game keeps running its text printer and menus as always (scripts untouched, the input
// still goes to the game); Core 1 reads their state and draws them in the new style, and pass 6
// skips the GBA frame while every visible window has a mirror (Ui_MirrorCovers). Windows without a
// mirror yet (multichoice, rare boxes) fall back to the GBA overlay.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "data.h"
#include "field_message_box.h"
#include "main.h"
#include "overworld.h"
#include "platform.h"
#include "string_util.h"
#include "text.h"
#include "window.h"
#include "constants/characters.h"

extern const u8 *gUiPrinterStart[WINDOWS_MAX];
extern u32 gUiPrinterSerial;
extern const struct TextPrinter *GetTextPrinterForUi(u8 windowId);
extern bool8 Menu_GetYesNoForUi(s8 *cursor, u8 *windowId);
extern struct Window gWindows[];
extern bool8 gUiDialogFrameShown;
extern u8 gUiChoiceWindow, gUiChoiceCount;
extern const u8 *gUiChoiceTexts[8];
extern bool8 Menu_GetCursorForUi(u8 windowId, s8 *cursor);
extern void Menu_SetCursorForUi(s8 pos);
extern u8 GetMoneyBoxWindowForUi(void);
extern bool gUiVoxelFrame;
extern bool8 MatchCall_GetUiState(u8 *windowId, u8 *step, u16 *trainerIdx);
extern bool32 IsMatchCallTaskActive(void);
extern const u8 *gUiFieldMessageSrc;
extern bool8 EvolutionScene_IsActiveForUi(void);
extern bool8 EggHatch_IsActiveForUi(void);
extern bool8 Trade_IsAnimActiveForUi(void);
static bool sSceneText;        // a 2D scene (evolution, EGG hatch, trade) has printed its first message
extern u16 gSpecialVar_0x8000, gSpecialVar_0x8004;
extern const u8 gText_ObtainedTheItem[], gText_PlayerFoundOneItem[], gText_PlayerFoundOneTMHM[], gText_PutItemInPocket[];
static uint16_t sShownItem;      // item of the "obtained / found" messages (design/stitch/objeto_obtenido)

static bool sCovers;           // written by Core 1, read by Core 0
// The last dialogue box drawn, held a few frames when the mirror would drop out for a moment: the
// windows of a closing YES/NO, a finished call or the next message are torn down and created over
// several frames, and in between the GBA box (or nothing) showed for a frame or two (flicker)
#define HOLD_FRAMES 6
static char sHeldLines[2][160];
static int32_t sHold;
static uint16_t sInjected;     // GBA keys asked by a tap (Core 1 only)
static bool sTouchPrev;
static int32_t sTouchX, sTouchY, sTouchDownX, sTouchDownY;

uint16_t Ui_TakeInjectedKeys(void)
{
    const uint16_t k = sInjected;
    sInjected = 0;
    return k;
}

void UiMirror_Inject(uint16_t keys)
{
    sInjected |= keys;
}

// A tap (press and release without moving) this tick, in logical coordinates. One caller per tick
// (the field mirrors or the battle HUD).
bool UiMirror_Tap(int32_t *x, int32_t *y);
static bool Tap(int32_t *x, int32_t *y)
{
    return UiMirror_Tap(x, y);
}

bool UiMirror_Tap(int32_t *x, int32_t *y)
{
    int32_t tx = 0, ty = 0;
    const bool touch = VitaPlatform_ReadTouch(&tx, &ty);
    bool tap = false;
    if (touch && !sTouchPrev) { sTouchDownX = tx; sTouchDownY = ty; }
    if (touch) { sTouchX = tx; sTouchY = ty; }
    else if (sTouchPrev && sTouchX - sTouchDownX < 8 && sTouchDownX - sTouchX < 8 && sTouchY - sTouchDownY < 8 && sTouchDownY - sTouchY < 8)
        tap = true;
    sTouchPrev = touch;
    *x = sTouchX;
    *y = sTouchY;
    return tap;
}

// Tap on option i of a mirrored menu: the first tap moves the cursor, a tap on the selected one is A
static void TapMenu(int32_t tx, int32_t ty, int32_t x, int32_t y, int32_t w, int32_t count, int32_t cursor)
{
    for (int32_t i = 0; i < count; i++)
        if (tx >= x + 8 && tx < x + w - 8 && ty >= y + 8 + i * 42 && ty < y + 50 + i * 42)
        {
            if (i == cursor) sInjected |= A_BUTTON;
            else Menu_SetCursorForUi((s8)i);
        }
}
static uint32_t sFrame;

bool Ui_MirrorCovers(void)
{
    return Ui_IsEnabled() && !Ui_IsOpen() && __atomic_load_n(&sCovers, __ATOMIC_RELAXED);
}

// The two visible lines of a GBA message: walks the string up to the printer's position as the
// printer does (\n second line, \l scroll, \p new page); control codes are skipped.
void UiMirror_VisibleLines(const u8 *start, const u8 *end, char out[2][160])
{
    // A page break (\p) or a scroll (\l) waits for A with the old text still on screen: it only
    // takes effect when the next character is printed
    u8 lines[2][96];
    int32_t len[2] = { 0, 0 }, cur = 0;
    bool pendingClear = false, pendingScroll = false;
    for (const u8 *p = start; p != NULL && *p != EOS && (end == NULL || p < end); p++)
    {
        const u8 c = *p;
        if (c == CHAR_PROMPT_CLEAR) { pendingClear = true; pendingScroll = false; continue; }
        if (c == CHAR_PROMPT_SCROLL) { pendingScroll = true; continue; }
        if (pendingClear) { len[0] = len[1] = 0; cur = 0; pendingClear = false; }
        if (pendingScroll) { memcpy(lines[0], lines[1], (size_t)len[1]); len[0] = len[1]; len[1] = 0; cur = 1; pendingScroll = false; }
        if (c == CHAR_NEWLINE) { if (cur == 0) cur = 1; else { memcpy(lines[0], lines[1], (size_t)len[1]); len[0] = len[1]; len[1] = 0; } continue; }
        if (c == PLACEHOLDER_BEGIN || c == CHAR_KEYPAD_ICON || c == CHAR_EXTRA_SYMBOL)
        {
            if (p[1] == EOS) break;
            p++;                                           // one argument byte
            continue;
        }
        if (c == EXT_CTRL_CODE_BEGIN)
        {
            if (p[1] == EOS) break;
            p += GetExtCtrlCodeLength(p[1]);
            continue;
        }
        if (len[cur] < 94)
            lines[cur][len[cur]++] = c;
    }
    for (int32_t i = 0; i < 2; i++)
    {
        lines[i][len[i]] = EOS;
        UiGame_Text(lines[i], out[i], 160);
    }
}

static bool OtherBg0Windows(u8 except, u8 except2)
{
    for (int32_t w = 1; w < WINDOWS_MAX; w++)
        if (w != except && w != except2 && gWindows[w].window.bg == 0 && gWindows[w].tileData != NULL)
            return true;
    return false;
}

// The mirror has nothing to show this frame: the held box while the hold lasts, else nothing (the
// GBA overlay shows its windows)
static bool HeldBox(void)
{
    if (sHold <= 0)
    {
        __atomic_store_n(&sCovers, false, __ATOMIC_RELAXED);
        return false;
    }
    sHold--;
    const int32_t x = 48, y = UI_SCREEN_H - 24 - 124, w = UI_SCREEN_W - 96, h = 124;
    UiW_DialogBox(x, y, w, h);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 12, sHeldLines[0], UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 58, sHeldLines[1], UI_COL_INK);
    __atomic_store_n(&sCovers, true, __ATOMIC_RELAXED);
    return true;
}

// Core 1, from Ui_Tick while no screen is open: draws into the list being built
bool UiMirror_Draw(void)
{
    sFrame++;
    bool drew = false;
    // Only over the 3D world: in 2D maps the GBA frame (with its windows) is the whole picture
    const bool field = Ui_IsEnabled() && gMain.callback2 == CB2_Overworld && __atomic_load_n(&gUiVoxelFrame, __ATOMIC_RELAXED);
    // The box stays while the game shows the message window: the text has finished printing
    // (field message mode back to HIDDEN) but the window waits for A until closemessage
    // Full-screen 2D scenes with a message window 0 (evolution, EGG hatch, trade): the box from the
    // first message to the end of the scene, over a band that hides the GBA box under it
    const bool scene = Ui_IsEnabled() && !gMain.inBattle
                    && (EvolutionScene_IsActiveForUi() || EggHatch_IsActiveForUi() || Trade_IsAnimActiveForUi());
    // Latched on the first printer started after the scene began (instant prints never look active)
    static u32 sSceneSerial;
    if (!scene) { sSceneText = false; sSceneSerial = gUiPrinterSerial; }
    else if (GetTextPrinterForUi(0)->active || gUiPrinterSerial != sSceneSerial) sSceneText = true;
    const bool sceneBox = scene && sSceneText;
    const bool box = sceneBox || (field && ((gUiDialogFrameShown && gWindows[0].tileData != NULL)
                            || GetFieldMessageBoxMode() != FIELD_MESSAGE_BOX_HIDDEN || GetTextPrinterForUi(0)->active));
    // PokéNav call in the field (design/stitch/llamada): its own window, from the intro to the end
    u8 callWin = 0, callStep = 0;
    u16 caller = 0xFFFF;
    const bool call = field && MatchCall_GetUiState(&callWin, &callStep, &caller) && callStep >= 3 && callStep <= 5 && callWin < WINDOWS_MAX;
    s8 yesCursor = 0;
    u8 yesWin = 0;
    const bool yesNo = (field || sceneBox) && Menu_GetYesNoForUi(&yesCursor, &yesWin);
    // Script multichoice (PC access, NPC choices): window, options and the cursor of the game
    s8 choiceCursor = 0;
    const bool choice = field && !yesNo && gUiChoiceWindow < WINDOWS_MAX && Menu_GetCursorForUi(gUiChoiceWindow, &choiceCursor);
    const u8 menuWin = yesNo ? yesWin : choice ? gUiChoiceWindow : 0;
    const u8 moneyWin = field ? GetMoneyBoxWindowForUi() : WINDOW_NONE;
    int32_t tapX = 0, tapY = 0;
    const bool tap = Tap(&tapX, &tapY);
    // A window without a mirror on BG0 (multichoice...): the GBA overlay shows everything
    if (call)
    {
        if (OtherBg0Windows(callWin, 0))
            return HeldBox();
        const struct TextPrinter *tp = GetTextPrinterForUi(callWin);
        const u8 *start = gUiPrinterStart[callWin];
        const u8 *end = tp->active ? tp->printerTemplate.currentChar : NULL;
        if (end != NULL && (start == NULL || end < start || end - start > 1024))
            end = NULL;
        char lines[2][160];
        UiMirror_VisibleLines(start, end, lines);
        const int32_t x = 48, y = UI_SCREEN_H - 24 - 124, w = UI_SCREEN_W - 96, h = 124;
        // Caller: the trainer's picture in a box and the POKéNAV · MATCH CALL tag
        UiW_DialogBox(x, y - 112, 100, 100);
        if (caller != 0xFFFF)
            UiSpr_TrainerSized(gTrainers[caller].trainerPic, x + 18, y - 94, 64, 64);
        else
            UiDraw_Sprite(UI_SPR_START_POKENAV, x + 34, y - 78, 1);
        UiW_Capsule(x + 112, y - 52, 300, 34);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 124, y - 51, "POKéNAV · MATCH CALL", UI_COL_ON_FRAME);
        memcpy(sHeldLines, lines, sizeof(sHeldLines));
        sHold = HOLD_FRAMES;
        UiW_DialogBox(x, y, w, h);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 12, lines[0], UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 58, lines[1], UI_COL_INK);
        if ((!tp->active || tp->state == RENDER_STATE_WAIT) && ((sFrame / 16) & 1))
            UiDraw_Tri(x + w - 40, y + h - 30, x + w - 24, y + h - 30, x + w - 32, y + h - 20, UI_COL_MONEY);
        // The ringing PokéNav, right of the text (the call window's animated icon, x2)
        UiDraw_Sprite((uint16_t)(UI_SPR_PHONE_FIRST + (sFrame / 6) % 8), x + w - 96, y + 22, 2);
        __atomic_store_n(&sCovers, true, __ATOMIC_RELAXED);
        return true;
    }
    if ((!box && !yesNo && !choice) || (!sceneBox && OtherBg0Windows(menuWin, moneyWin == WINDOW_NONE ? 0 : moneyWin)))
    {
        if (!field)
            sHold = 0;                                     // a battle, a menu, a 2D map: no hold
        return HeldBox();
    }
    if (box)
    {
        const struct TextPrinter *tp = GetTextPrinterForUi(0);
        const u8 *start = gUiPrinterStart[0];
        const u8 *end = tp->active ? tp->printerTemplate.currentChar : NULL;
        if (end != NULL && (start == NULL || end < start || end - start > 1024))
            end = NULL;                                    // a stale printer: show the whole string
        char lines[2][160];
        UiMirror_VisibleLines(start, end, lines);
        // Dialogue box (design/stitch/dialogo): paper box at the bottom, VT323 32
        const int32_t x = 48, y = UI_SCREEN_H - 24 - 124, w = UI_SCREEN_W - 96, h = 124;
        if (sceneBox)
        {
            // The GBA box of these scenes is GBA y 112..160 (x3.4 = 381..544): an opaque band hides it
            UiDraw_Rect(0, UI_SCREEN_H - 172, UI_SCREEN_W, 172, UI_COL_FRAME1);
            UiDraw_Rect(0, UI_SCREEN_H - 172, UI_SCREEN_W, 3, UI_COL_LINE_DARK);
        }
        memcpy(sHeldLines, lines, sizeof(sHeldLines));
        sHold = sceneBox ? 0 : HOLD_FRAMES;
        UiW_DialogBox(x, y, w, h);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 12, lines[0], UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + 28, y + 58, lines[1], UI_COL_INK);
        // Waiting for A: the blinking arrow
        const bool waiting = !tp->active || tp->state == RENDER_STATE_WAIT;
        if (waiting && !yesNo && !choice && ((sFrame / 16) & 1))
            UiDraw_Tri(x + w - 40, y + h - 30, x + w - 24, y + h - 30, x + w - 32, y + h - 20, UI_COL_MONEY);
        // Item obtained / found: its icon and the pocket it goes to, above the box
        const u8 *src = sceneBox ? NULL : gUiFieldMessageSrc;
        if (src == NULL) sShownItem = 0;
        else if (src == gText_ObtainedTheItem) sShownItem = gSpecialVar_0x8000;
        else if (src == gText_PlayerFoundOneItem || src == gText_PlayerFoundOneTMHM) sShownItem = gSpecialVar_0x8004;
        else if (src != gText_PutItemInPocket) sShownItem = 0;
        if (sShownItem != 0)
        {
            static const char *const pockets[6] = { "", "ITEMS", "POKé BALLS", "TMs & HMs", "BERRIES", "KEY ITEMS" };
            UiW_DialogBox(x, y - 112, 100, 100);
            UiSpr_ItemIcon(sShownItem, x + 26, y - 86, 2);
            char buf[48];
            const int32_t pk = UiGame_ItemPocket(sShownItem) + 1;
            snprintf(buf, sizeof(buf), "Put away in the %s POCKET", pk >= 1 && pk <= 5 ? pockets[pk] : "");
            const int32_t cw = UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf) + 24;
            UiW_Capsule(x + 112, y - 52, cw, 34);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 124, y - 51, buf, UI_COL_ON_FRAME);
        }
        drew = true;
    }
    else
        sShownItem = 0;
    if (moneyWin != WINDOW_NONE)
    {
        // Money box (shops, games): top right
        char money[24];
        snprintf(money, sizeof(money), "MONEY $%u", (unsigned)UiGame_Money());
        const int32_t mw = UiText_Width(UI_FACE_VT323, UI_BODY_PX, money) + 28;
        UiW_DialogBox(UI_SCREEN_W - 48 - mw, 16, mw, 44);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, UI_SCREEN_W - 48 - mw + 14, 20, money, UI_COL_INK);
    }
    // A tap on the dialogue box is A (next page), as long as no menu is open
    if (box && tap && !yesNo && !choice && tapY >= UI_SCREEN_H - 24 - 124 && tapY < UI_SCREEN_H - 24)
        sInjected |= A_BUTTON;
    if (choice)
    {
        // Choice list (design/stitch/multichoice): right side, above the dialogue box
        char opt[40];
        int32_t w = 200;
        for (int32_t i = 0; i < gUiChoiceCount; i++)
        {
            UiGame_Text(gUiChoiceTexts[i], opt, sizeof(opt));
            const int32_t tw = UiText_Width(UI_FACE_VT323, UI_BODY_PX, opt) + 64;
            if (tw > w) w = tw;
        }
        const int32_t h = 16 + gUiChoiceCount * 42, x = UI_SCREEN_W - 48 - w, y = UI_SCREEN_H - 24 - 124 - 12 - h;
        if (tap) TapMenu(tapX, tapY, x, y, w, gUiChoiceCount, choiceCursor);
        UiW_DialogBox(x, y, w, h);
        for (int32_t i = 0; i < gUiChoiceCount; i++)
        {
            UiGame_Text(gUiChoiceTexts[i], opt, sizeof(opt));
            UiW_Row(x + 8, y + 8 + i * 42, w - 16, 42, choiceCursor == i, i);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 36, y + 12 + i * 42, opt, choiceCursor == i ? UI_COL_SELECT_INK : UI_COL_INK);
        }
        drew = true;
    }
    if (yesNo)
    {
        // YES / NO (design/stitch/si_no): right side, above the dialogue box
        const int32_t x = UI_SCREEN_W - 48 - 180, y = UI_SCREEN_H - 24 - 124 - 112, w = 180;
        if (tap) TapMenu(tapX, tapY, x, y, w, 2, yesCursor);
        UiW_DialogBox(x, y, w, 100);
        static const char *const opt[2] = { "YES", "NO" };
        for (int32_t i = 0; i < 2; i++)
        {
            UiW_Row(x + 8, y + 8 + i * 42, w - 16, 42, yesCursor == i, i);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 36, y + 12 + i * 42, opt[i], yesCursor == i ? UI_COL_SELECT_INK : UI_COL_INK);
        }
        drew = true;
    }
    // pass 6 can skip the GBA frame (never in the 2D scenes: their picture is the GBA frame)
    __atomic_store_n(&sCovers, drew && !sceneBox, __ATOMIC_RELAXED);
    return drew;
}
