// START menu of the new UI (design/stitch/hub): over the running world, without the full frame.
// Zone, status capsules (clock, money, registered item, repel / Safari, run), the menu with the
// GBA icons (D11), the party strip and the description of the selected entry. SAVE asks first
// and shows the result. Every entry opens a screen of the new UI.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_extras.h"
#include "platform/vita_controls.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "pokedex.h"
#include "event_scripts.h"
#include "safari_zone.h"
#include "script.h"
#include "strings.h"
#include "sound.h"
#include "constants/battle_frontier.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/species.h"

// HUB_REST and HUB_PYRAMID_RETIRE: the Battle Pyramid's START entries (BuildBattlePyramidStartMenu)
enum HubEntry { HUB_DEX, HUB_MON, HUB_BAG, HUB_NAV, HUB_CARD, HUB_SAVE, HUB_REST, HUB_PYRAMID_RETIRE, HUB_OPTION, HUB_NUZLOCKE,
                HUB_RETIRE, HUB_EXIT, HUB_COUNT };

extern const u8 *StartMenu_PyramidFloorForUi(void);

// Informative only: short, at most two lines in the small hint box
static const char *const sHints[HUB_COUNT] = {
    UI_N("POKéMON seen and caught in the HOENN POKéDEX."),
    UI_N("Check and organize your party POKéMON."),
    UI_N("Open your BAG pockets and use items."),
    UI_N("Check the HOENN map, Match Call, and ribbons."),
    UI_N("Inspect your Trainer Card, Money, and Badges."),
    UI_N("Save your progress to the memory card."),
    UI_N("Save and quit the BATTLE PYRAMID for now."),
    UI_N("Give up the BATTLE PYRAMID challenge."),
    UI_N("Configure game settings, controls, and Menu Skin."),
    UI_N("Inspect Nuzlocke routes, fallen allies, and cap."),
    UI_N("Leave the SAFARI GAME."),
    UI_N("Close this menu and return to the adventure."),
};

#define MENU_X 672
#define MENU_W 272
#define ROW_H 40
#define MENU_Y 20
#define GBA_MENU_Y 14
#define GBA_MENU_W 274

static uint8_t sEntries[HUB_COUNT];
static int32_t sCount, sSel;
static uint32_t sFrame;
enum { POP_NONE, POP_SAVE, POP_SAVED, POP_PYRAMID_RETIRE };
static int32_t sPopup;                 // POP_*
static bool sRest;                     // the save is the Pyramid's REST: a soft reset follows it
static int32_t sPopupSel;
static bool sSaveOk;
static int32_t sRunX, sRunY, sRunW, sRunH;
static const char *sToast;
static int32_t sToastTicks;

static int32_t GbaRowH(void)
{
    return (sCount * 45 + 28 <= 416) ? 45 : 40;
}


static void BuildEntries(void)
{
    const bool inSafari = UiGame_InSafari();
    const bool inPike = InBattlePike() == TRUE;
    const bool inPyr = CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE;
    sCount = 0;
    if (FlagGet(FLAG_SYS_POKEDEX_GET) && !inPyr) sEntries[sCount++] = HUB_DEX;
    if (FlagGet(FLAG_SYS_POKEMON_GET) || UiGame_PartyCount() > 0) sEntries[sCount++] = HUB_MON;
    if (!inPike) sEntries[sCount++] = HUB_BAG;
    if (FlagGet(FLAG_SYS_POKENAV_GET) && !inSafari && !inPike && !inPyr) sEntries[sCount++] = HUB_NAV;
    sEntries[sCount++] = HUB_CARD;
    if (!inSafari && !inPike && !inPyr) sEntries[sCount++] = HUB_SAVE;
    if (inPyr) sEntries[sCount++] = HUB_REST;
    if (inPyr) sEntries[sCount++] = HUB_PYRAMID_RETIRE;
    sEntries[sCount++] = HUB_OPTION;
    if (VitaExtras_GetFast(FAST_EXTRA_NUZLOCKE) != 0) sEntries[sCount++] = HUB_NUZLOCKE;
    if (inSafari) sEntries[sCount++] = HUB_RETIRE;
    sEntries[sCount++] = HUB_EXIT;
    if (sSel >= sCount) sSel = sCount - 1;
}

static void Enter(void)
{
    sPopup = POP_NONE;
    sRest = false;
    sToast = NULL;
    BuildEntries();
}

static void Resume(void)
{
    BuildEntries();
}

// The Pyramid's RETIRE after its YES (BattlePyramidRetireCallback): the game's retire script
static void RetirePyramid(void)
{
    ScriptContext_SetupScript(BattlePyramid_Retire);
}

// The Pyramid's REST once saved (SaveCallback): the game's soft reset in the Pyramid
static void RestInPyramid(void)
{
    SoftResetInBattlePyramid();
}

static void RetireSafari(void)
{
    // Same as the game's START menu "RETIRE": no question here, the script asks once
    // (SafariZone_EventScript_RetirePrompt) and warps to the counter
    SafariZoneRetirePrompt();
}

static bool Activate(void)
{
    PlaySE(SE_SELECT);
    switch (sEntries[sSel])
    {
    case HUB_DEX:      UiNav_Push(&gUiDexScreen); break;
    case HUB_MON:      UiNav_Push(&gUiPartyScreen); break;
    case HUB_BAG:      UiBag_SetGiveTo(-1); UiNav_Push(&gUiBagScreen); break;
    case HUB_NAV:      UiNav_Push(&gUiNavScreen); break;
    case HUB_CARD:     UiNav_Push(&gUiCardScreen); break;
    case HUB_SAVE:     sPopup = POP_SAVE; sPopupSel = 0; sRest = false; break;
    case HUB_REST:     sPopup = POP_SAVE; sPopupSel = 0; sRest = true; break;
    case HUB_PYRAMID_RETIRE: sPopup = POP_PYRAMID_RETIRE; sPopupSel = 1; break;   // NO first, as the game
    case HUB_OPTION:   UiNav_Push(&gUiOptionsScreen); break;
    case HUB_NUZLOCKE: UiNav_Push(&gUiNuzlockeScreen); break;
    case HUB_RETIRE:   UiNav_Close(RetireSafari); break;
    case HUB_EXIT:     return false;
    }
    return true;
}

// Yes / No window: d-pad or tap selects, X or a second tap confirms. Returns 1 yes, 0 no, -1 open.
#define POP_X 260
#define POP_Y 150
#define POP_W 440
static int32_t UpdateYesNo(const UiInput *in)
{
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return 0; }
    if (gUiTheme->gbaFrame && (sPopup == POP_SAVE || sPopup == POP_PYRAMID_RETIRE))
    {
        if (in->repeat & (VITA_BTN_UP | VITA_BTN_DOWN | VITA_BTN_LEFT | VITA_BTN_RIGHT))
        {
            sPopupSel ^= 1;
            PlaySE(SE_SELECT);
        }
        bool confirm = (in->pressed & VITA_BTN_CROSS) != 0;
        for (int32_t i = 0; i < 2; i++)
            if (UiHit(in, 732, 322 + i * 46, 198, 42))
            {
                if (sPopupSel == i) confirm = true;
                else { sPopupSel = i; PlaySE(SE_SELECT); }
            }
        if (!confirm)
            return -1;
        PlaySE(SE_SELECT);
        return sPopupSel == 0 ? 1 : 0;
    }
    if (in->repeat & (VITA_BTN_LEFT | VITA_BTN_RIGHT)) { sPopupSel ^= 1; PlaySE(SE_SELECT); }
    bool confirm = (in->pressed & VITA_BTN_CROSS) != 0;
    for (int32_t i = 0; i < 2; i++)
        if (UiHit(in, POP_X + 16 + i * 208, POP_Y + 168, 200, 40))
        {
            if (sPopupSel == i) confirm = true;
            else { sPopupSel = i; PlaySE(SE_SELECT); }
        }
    if (!confirm)
        return -1;
    PlaySE(SE_SELECT);
    return sPopupSel == 0 ? 1 : 0;
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sToastTicks > 0 && --sToastTicks == 0)
        sToast = NULL;

    if (sPopup == POP_SAVE)
    {
        const int32_t r = UpdateYesNo(in);
        if (r == 1)
        {
            sSaveOk = UiGame_Save();
            PlaySE(sSaveOk ? SE_SAVE : SE_FAILURE);
            sPopup = POP_SAVED;
        }
        else if (r == 0)
            sPopup = POP_NONE;
        return true;
    }
    if (sPopup == POP_SAVED)
    {
        if (in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE) || in->tap)
        {
            PlaySE(SE_SELECT);
            sPopup = POP_NONE;
            // REST: saved, then out of the Pyramid by its soft reset (only once the save is safe)
            if (sRest && sSaveOk)
                UiNav_Close(RestInPyramid);
        }
        return true;
    }
    if (sPopup == POP_PYRAMID_RETIRE)
    {
        const int32_t r = UpdateYesNo(in);
        if (r >= 0)
            sPopup = POP_NONE;
        if (r == 1)
            UiNav_Close(RetirePyramid);
        return true;
    }

    if (in->pressed & (VITA_BTN_CIRCLE | VITA_BTN_START))
    {
        PlaySE(SE_SELECT);
        return false;
    }
    const bool gba = gUiTheme->gbaFrame;
    if (sRunW > 0 && ((in->pressed & VITA_BTN_TRIANGLE) || UiHit(in, sRunX, sRunY, sRunW, sRunH)))
    {
        VitaExtras_Set("auto_run", VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) ? 0 : 1);
        VitaExtras_Save();
        PlaySE(SE_SELECT);
        return true;
    }
    if (gba)
    {
        const int32_t pCount = UiGame_PartyCount();
        for (int32_t i = 0; i < pCount; i++)
        {
            const int32_t mx = 24 + (i & 1) * 212;
            const int32_t my = 220 + (i >> 1) * 46;
            if (UiHit(in, mx, my, 204, 42))
            {
                PlaySE(SE_SELECT);
                UiNav_Push(&gUiPartyScreen);
                return true;
            }
        }
    }
    const int32_t before = sSel;
    if (in->repeat & VITA_BTN_UP) sSel = (sSel + sCount - 1) % sCount;
    if (in->repeat & VITA_BTN_DOWN) sSel = (sSel + 1) % sCount;
    bool activate = (in->pressed & VITA_BTN_CROSS) != 0;
    const int32_t menuY0 = gba ? GBA_MENU_Y + 15 : MENU_Y + 10;
    const int32_t rowH = gba ? GbaRowH() : ROW_H;
    const int32_t menuW = gba ? GBA_MENU_W : MENU_W;
    for (int32_t i = 0; i < sCount; i++)
        if (UiHit(in, MENU_X + (gba ? 15 : 8), menuY0 + i * rowH, menuW - (gba ? 30 : 16), rowH))
        {
            if (sSel == i) activate = true;
            else sSel = i;
        }
    if (sSel != before)
        PlaySE(SE_SELECT);
    if (activate)
        return Activate();

    return true;
}

static void EntryIcon(uint8_t e, int32_t x, int32_t y)
{
    UiMon lead;
    switch (e)
    {
    case HUB_DEX:      UiDraw_Sprite(UI_SPR_BALL_POKE, x + 4, y + 4, 2); break;
    case HUB_MON:      if (UiGame_Mon(0, &lead)) UiSpr_MonIcon(lead.species, (uint8_t)((sFrame / 16) & 1), x, y, 1); break;
    case HUB_BAG:      UiDraw_Sprite(UI_SPR_START_BAG, x, y, 1); break;
    case HUB_NAV:      UiDraw_Sprite(UI_SPR_START_POKENAV, x, y, 1); break;
    case HUB_CARD:     UiDraw_Sprite(UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY, x + 4, y + 4, 2); break;
    case HUB_SAVE:
    case HUB_REST:     UiSpr_ItemIcon(ITEM_LETTER, x + 4, y + 4, 1); break;
    case HUB_PYRAMID_RETIRE: UiSpr_ItemIcon(ITEM_ESCAPE_ROPE, x + 4, y + 4, 1); break;
    case HUB_OPTION:   UiSpr_ItemIcon(ITEM_CARD_KEY, x + 4, y + 4, 1); break;
    case HUB_NUZLOCKE: UiDraw_Sprite(UI_SPR_BALL_PREMIER, x + 4, y + 4, 2); break;
    case HUB_RETIRE:   UiDraw_Sprite(UI_SPR_BALL_SAFARI, x + 4, y + 4, 2); break;
    default: break;
    }
}

static void EntryLabel(uint8_t e, char *out, size_t cap)
{
    static const char *const labels[HUB_COUNT] = { UI_N("POKéDEX"), UI_N("POKéMON"), UI_N("BAG"), UI_N("POKéNAV"), NULL, UI_N("SAVE"), UI_N("REST"), UI_N("RETIRE"), UI_N("OPTION"),
                                                   UI_N("NUZLOCKE"), UI_N("RETIRE"), UI_N("EXIT") };
    if (e == HUB_CARD)
        UiGame_PlayerName(out, cap);
    else
        snprintf(out, cap, "%s", UiLocale_Lookup(labels[e]));
}

static void BuildStatus(void)
{
    char buf[48], zone[32];
    UiGame_ZoneName(zone, sizeof(zone));
    UiW_PanelDark(16, 16, 332, 48);
    UiDraw_PushClip(24, 16, 316, 48);
    UiText_DrawCentred(UI_FACE_PS2P, 16, 24, 16, 316, 48, zone, UI_COL_TITLE);
    UiDraw_PopClip();

    // 2x2 status grid (left column x=16, right column x=186, width=162, height=34)
    // Top-left: registered item (SEL)
    UiW_Capsule(16, 70, 162, 34);
    const uint16_t reg = UiGame_RegisteredItem();
    if (reg != ITEM_NONE)
    {
        char name[24];
        UiGame_ItemName(reg, name, sizeof(name));
        UiSpr_ItemIcon(reg, 22, 75, 1);
        UiDraw_PushClip(50, 70, 122, 34);
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 50, 70, 34, name, UI_COL_ON_FRAME);
        UiDraw_PopClip();
    }
    else
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 26, 70, 34, UI_TR("SEL: NONE"), UI_COL_ON_SOFT);

    // Top-right: auto-run toggle (touch when Running Shoes are unlocked)
    UiW_Capsule(186, 70, 162, 34);
    if (UiGame_HasFlag(FLAG_SYS_B_DASH))
    {
        const bool on = VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0;
        sRunX = 186;
        sRunY = 70;
        sRunW = 162;
        sRunH = 34;
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 196, 70, 34, on ? UI_TR("AUTO-RUN: ON") : UI_TR("AUTO-RUN: OFF"),
                       on ? UI_RGBA(115, 205, 164, 255) : UI_COL_ON_SOFT);
    }
    else
    {
        sRunW = 0;
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 196, 70, 34, UI_TR("RUN: WALK"), UI_COL_ON_SOFT);
    }

    // Bottom-left: Safari balls and steps, or Repel steps
    UiW_Capsule(16, 110, 162, 34);
    if (UiGame_InSafari())
    {
        // Safari Ball, then balls · steps left (at most "30 · 500": 104 px in the widest theme face)
        UiDraw_Sprite(UI_SPR_BALL_SAFARI, 24, 119, 1);
        snprintf(buf, sizeof(buf), "%u · %u", (unsigned)UiGame_SafariBalls(), (unsigned)UiGame_SafariSteps());
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 46, 110, 34, buf, UI_COL_ON_FRAME);
    }
    else if (UiGame_InPyramid())
    {
        // The Pyramid's floor, as its START menu shows it (ShowPyramidFloorWindow)
        UiGame_Text(StartMenu_PyramidFloorForUi(), buf, sizeof(buf));
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 26, 110, 34, buf, UI_COL_GOLD);
    }
    else if (UiGame_RepelSteps() > 0)
    {
        snprintf(buf, sizeof(buf), UI_TR("REPEL: %u"), (unsigned)UiGame_RepelSteps());
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 26, 110, 34, buf, UI_COL_GOLD);
    }
    else
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 26, 110, 34, UI_TR("REPEL: OFF"), UI_COL_ON_SOFT);

    // Bottom-right: money and clock
    UiW_Capsule(186, 110, 162, 34);
    snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
    UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 194, 110, 34, buf, UI_COL_GOLD);
    char clock[8];
    UiGame_Clock(clock, sizeof(clock));
    const int32_t clockW = UiText_Width(UI_FACE_VT323, UI_BODY_PX, clock);
    UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 186 + 154 - clockW, 110, 34, clock, UI_COL_ON_FRAME);
}

static void BuildParty(void)
{
    const int32_t n = UiGame_PartyCount();
    if (n == 0)
        return;
    const int32_t y0 = 152;
    UiW_Panel(16, y0, 332, n * 36 + 12);
    for (int32_t i = 0; i < n; i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = y0 + 6 + i * 36;
        UiW_Row(22, y, 320, 36, i == 0, i);
        UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 16) & 1), 26, y + 2 - 4, 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 62, y + 2, m.name, i == 0 ? UI_COL_SELECT_INK : UI_COL_INK);
        if (m.egg)
            continue;
        char lv[8];
        snprintf(lv, sizeof(lv), UI_TR("Lv%u"), (unsigned)m.level);
        // Level right-aligned in its column, ending 6 px before the HP bar
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 270 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, lv), y + 2, lv, i == 0 ? UI_COL_SELECT_INK : UI_COL_INK);
        const int32_t pct = m.maxHp ? (int32_t)m.hp * 100 / m.maxHp : 0;
        const uint32_t top = pct > 50 ? UI_RGBA(110, 219, 151, 255) : pct > 20 ? UI_RGBA(245, 209, 98, 255) : UI_RGBA(240, 120, 104, 255);
        const uint32_t bot = pct > 50 ? UI_COL_HP_HIGH : pct > 20 ? UI_COL_HP_MID : UI_COL_HP_LOW;
        UiW_Bar(276, y + 14, 56, 8, pct, top, bot);
    }
}

static void BuildMenu(void)
{
    UiW_Panel(MENU_X, MENU_Y, MENU_W, sCount * ROW_H + 20);
    for (int32_t i = 0; i < sCount; i++)
    {
        const int32_t y = MENU_Y + 10 + i * ROW_H;
        UiW_Row(MENU_X + 8, y, MENU_W - 16, ROW_H, i == sSel, i);
        EntryIcon(sEntries[i], MENU_X + 32, y + 4);
        char label[24];
        EntryLabel(sEntries[i], label, sizeof(label));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, MENU_X + 76, y + 4, label, i == sSel ? UI_COL_SELECT_INK : UI_COL_INK);
    }
    // Description of the selected entry
    const int32_t hy = UI_SCREEN_H - 16 - 76;
    UiW_PanelDark(MENU_X, hy, MENU_W, 76);
    UiW_TextWrapped(MENU_X + 14, hy + 8, MENU_W - 28, 30, UiLocale_Lookup(sHints[sEntries[sSel]]), UI_COL_ON_FRAME, 2);
}

// YES / NO of the popups (save, the Pyramid's RETIRE), as UpdateYesNo reads them
static void DrawYesNoRows(void)
{
    static const char *const opt[2] = { UI_N("YES"), UI_N("NO") };
    for (int32_t i = 0; i < 2; i++)
    {
        const int32_t bx = POP_X + 16 + i * 208;
        UiW_Row(bx, POP_Y + 168, 200, 40, sPopupSel == i, 0);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, bx + 84, POP_Y + 172, UiLocale_Lookup(opt[i]), sPopupSel == i ? UI_COL_SELECT_INK : UI_COL_INK);
    }
}

static void DrawGbaYesNo(void)
{
    UiW_GbaWin(724, 310, 214, 110, UI_GBA_WIN_DEFAULT);
    static const char *const kOpt[2] = { UI_N("YES"), UI_N("NO") };
    for (int32_t i = 0; i < 2; i++)
    {
        const int32_t ry = 324 + i * 44;
        if (sPopupSel == i)
        {
            UiW_GbaBox(738, ry, 186, 40, 3, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(749, ry + 15, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(748, ry + 14, true, UI_RGBA(230, 8, 8, 255));
        }
        UiW_GbaTextMid(UI_FACE_VT323, 32, 770, ry, 40, UiLocale_Lookup(kOpt[i]), UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }
}

// The Pyramid's RETIRE question (BattlePyramidConfirmRetireCallback), in each theme
static void BuildRetirePopup(bool gba)
{
    char q[96];
    UiGame_Text(gText_BattlePyramidConfirmRetire, q, sizeof(q));
    if (gba)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_RGBA(16, 28, 28, 175));
        DrawGbaYesNo();
        UiW_GbaMsg(14, 438, 932, 94, false, true);
        UiW_GbaTextWrapped(UI_FACE_VT323, 32, 38, 448, 880, 38, q, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
        return;
    }
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    UiW_Window(POP_X, POP_Y, POP_W, 224, UI_TR("RETIRE"));
    UiW_TextWrapped(POP_X + 20, POP_Y + 56, POP_W - 40, 34, q, UI_COL_INK, 3);
    DrawYesNoRows();
}

static void BuildPopup(void)
{
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    char buf[64];
    if (sPopup == POP_SAVED)
    {
        UiW_Window(POP_X, POP_Y + 40, POP_W, 136, UI_TR("SAVE"));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 92, sSaveOk ? UI_TR("GAME SAVED SAFELY!") : UI_TR("SAVE FAILED!"), UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 124, sSaveOk ? "Saved to ux0:data/POKEVITA/" : UI_TR("Try again."), UI_COL_INK_SOFT);
        return;
    }
    UiW_Window(POP_X, POP_Y, POP_W, 224, UI_TR("SAVE"));
    {
        char name[16], zone[32], time[16];
        UiGame_PlayerName(name, sizeof(name));
        UiGame_ZoneName(zone, sizeof(zone));
        UiGame_PlayTime(time, sizeof(time));
        snprintf(buf, sizeof(buf), UI_TR("SLOT %u · %s · %s"), (unsigned)UiGame_SaveSlot(), name, zone);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 50, buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), UI_TR("BADGES %u · POKéDEX %u · TIME %s"), (unsigned)UiGame_BadgeCount(), (unsigned)UiGame_DexOwned(), time);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 82, buf, UI_COL_INK_SOFT);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 118,
                    UiGame_SaveOverwritesOtherGame() ? UI_TR("This overwrites the other game!") : UI_TR("Would you like to save the game?"),
                    UiGame_SaveOverwritesOtherGame() ? UI_COL_HP_LOW : UI_COL_INK);
    }
    DrawYesNoRows();
}

static void GbaStatCell(int32_t x, int32_t y)
{
    UiW_Bevel(x, y, 204, 40, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(27, 44, 50, 255), UI_RGBA(27, 44, 50, 255),
              1, UI_RGBA(72, 120, 136, 255), UI_RGBA(16, 24, 28, 255), 0);
}

static void BuildGbaHub(void)
{
    char buf[64], zone[32], clock[8];
    UiGame_ZoneName(zone, sizeof(zone));
    UiGame_Clock(clock, sizeof(clock));

    // 1. Wood & Emerald Zone Sign (.hub-sign: 14, 14, 438, 74)
    UiW_GbaBox(14, 14, 438, 74, 5, UI_RGBA(24, 32, 32, 255), UI_RGBA(216, 184, 136, 255), UI_RGBA(196, 160, 112, 255), 0, 4);
    UiW_GbaBox(18, 18, 430, 34, 3, UI_RGBA(24, 32, 32, 255), UI_RGBA(44, 134, 104, 255), UI_RGBA(23, 94, 71, 255),
               UI_RGBA(115, 205, 164, 160), 0);
    const int32_t zw = UiText_Width(UI_FACE_PS2P, 16, zone);
    const int32_t zx = 18 + (430 - (22 + zw)) / 2;
    UiDraw_Sprite(UI_SPR_BALL_POKE, zx > 24 ? zx : 24, 27, 1);
    UiDraw_PushClip(44, 18, 396, 34);
    UiW_GbaTextCentred(UI_FACE_PS2P, 16, 30, 18, 406, 34, zone, C_WHITE, UI_RGBA(16, 32, 24, 230));
    UiDraw_PopClip();
    UiW_GbaBox(18, 50, 430, 34, 3, UI_RGBA(24, 32, 32, 255), UI_RGBA(232, 207, 166, 255), UI_RGBA(212, 180, 132, 255),
               UI_RGBA(255, 244, 218, 255), 0);
    snprintf(buf, sizeof(buf), UI_TR("HOENN REGION · $%u"), (unsigned)UiGame_Money());
    UiDraw_PushClip(22, 50, 422, 34);
    UiW_GbaTextCentred(UI_FACE_VT323, 24, 18, 50, 430, 34, buf, UI_RGBA(62, 28, 0, 255), UI_RGBA(255, 244, 218, 255));
    UiDraw_PopClip();

    // 2. Dark-Well Status Card (.hub-status-card: 14, 96, 438, 104) -> 2x2 cells
    UiW_GbaWin(14, 96, 438, 104, UI_GBA_WIN_DARK);
    GbaStatCell(24, 106);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 32, 106, 40, UI_TR("TIME"), UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
    snprintf(buf, sizeof(buf), "%s %s", clock, UiGame_IsNight() ? UI_TR("NIGHT") : UI_TR("DAY"));
    UiW_GbaTextMid(UI_FACE_VT323, 28, 220 - UiText_Width(UI_FACE_VT323, 28, buf), 106, 40, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);

    GbaStatCell(236, 106);
    if (UiGame_InSafari())
    {
        UiDraw_Sprite(UI_SPR_BALL_SAFARI, 244, 118, 1);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 266, 106, 40, UI_TR("SAFARI"), UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
        // Balls · steps left, as the REPEL cell's value
        snprintf(buf, sizeof(buf), "%u · %u", (unsigned)UiGame_SafariBalls(), (unsigned)UiGame_SafariSteps());
        UiW_GbaTextMid(UI_FACE_VT323, 28, 432 - UiText_Width(UI_FACE_VT323, 28, buf), 106, 40, buf, UI_RGBA(115, 255, 172, 255), UI_GBA_SHAD_DARK);
    }
    else if (UiGame_InPyramid())
    {
        // The Pyramid's floor (ShowPyramidFloorWindow) where the REPEL goes
        UiW_GbaTextMid(UI_FACE_VT323, 24, 248, 106, 40, UI_TR("PYRAMID"), UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
        UiGame_Text(StartMenu_PyramidFloorForUi(), buf, sizeof(buf));
        UiW_GbaTextMid(UI_FACE_VT323, 28, 432 - UiText_Width(UI_FACE_VT323, 28, buf), 106, 40, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
    }
    else
    {
        const uint16_t repel = UiGame_RepelSteps();
        UiSpr_ItemIcon(ITEM_SUPER_REPEL, 242, 114, 1);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 270, 106, 40, UI_TR("REPEL"), UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
        if (repel > 0)
            snprintf(buf, sizeof(buf), UI_TR("%u ST"), (unsigned)repel);
        else
            snprintf(buf, sizeof(buf), UI_TR("OFF"));
        UiW_GbaTextMid(UI_FACE_VT323, 28, 432 - UiText_Width(UI_FACE_VT323, 28, buf), 106, 40, buf,
                       repel > 0 ? UI_RGBA(115, 255, 172, 255) : UI_RGBA(144, 164, 174, 255), UI_GBA_SHAD_DARK);
    }

    GbaStatCell(24, 150);
    UiW_GbaKeyCap(30, 158, 24, "SELECT", true);
    const int32_t selW = UiW_GbaKeyCapWidth("SELECT", 24);
    const uint16_t reg = UiGame_RegisteredItem();
    if (reg != ITEM_NONE)
    {
        char itemName[24];
        UiGame_ItemName(reg, itemName, sizeof(itemName));
        UiSpr_ItemIcon(reg, 34 + selW, 158, 1);
        UiDraw_PushClip(60 + selW, 150, 162 - selW, 40);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 60 + selW, 150, 40, itemName, C_WHITE, UI_GBA_SHAD_DARK);
        UiDraw_PopClip();
    }
    else
        UiW_GbaTextMid(UI_FACE_VT323, 24, 38 + selW, 150, 40, UI_TR("NONE"), UI_RGBA(144, 164, 174, 255), UI_GBA_SHAD_DARK);

    GbaStatCell(236, 150);
    UiW_GbaKeyCap(242, 158, 24, "TRI", false);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 272, 150, 40, UI_TR("AUTO-RUN"), UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
    if (UiGame_HasFlag(FLAG_SYS_B_DASH))
    {
        const bool on = VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0;
        sRunX = 236;
        sRunY = 150;
        sRunW = 204;
        sRunH = 40;
        const char *rt = on ? UI_TR("ON") : UI_TR("OFF");
        UiW_GbaTextMid(UI_FACE_VT323, 28, 432 - UiText_Width(UI_FACE_VT323, 28, rt), 150, 40, rt,
                       on ? UI_RGBA(115, 255, 172, 255) : UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
    }
    else
    {
        sRunW = 0;
        UiW_GbaTextMid(UI_FACE_VT323, 28, 432 - UiText_Width(UI_FACE_VT323, 28, UI_TR("WALK")), 150, 40, UI_TR("WALK"),
                       UI_RGBA(144, 164, 174, 255), UI_GBA_SHAD_DARK);
    }

    // 3. 2x3 Mini Party Grid (.hub-party: 14, 210, 438, 156)
    UiW_GbaWin(14, 210, 438, 156, UI_GBA_WIN_DARK);
    const int32_t pCount = UiGame_PartyCount();
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t mx = 24 + (i & 1) * 212;
        const int32_t my = 220 + (i >> 1) * 46;
        if (i >= pCount)
        {
            UiW_Bevel(mx, my, 204, 42, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(24, 38, 44, 255), UI_RGBA(20, 32, 36, 255),
                      0, 0, 0, 0);
            continue;
        }
        UiMon m;
        UiGame_Mon(i, &m);
        if (i == 0)
            UiW_GbaRingBox(mx, my, 204, 42, 4, UI_RGBA(255, 90, 32, 255), UI_RGBA(255, 230, 57, 255),
                           UI_RGBA(56, 136, 156, 255), UI_RGBA(56, 136, 156, 255), UI_RGBA(124, 224, 244, 255), 0);
        else
            UiW_Bevel(mx, my, 204, 42, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(48, 104, 128, 255), UI_RGBA(48, 104, 128, 255),
                      2, UI_RGBA(110, 192, 216, 255), UI_RGBA(20, 48, 64, 255), 0);
        UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 16) & 1), mx + 4, my + 2, 1);
        UiDraw_PushClip(mx + 38, my + 2, 114, 26);
        UiW_GbaText(UI_FACE_VT323, 24, mx + 38, my + 3, m.name, C_WHITE, UI_GBA_SHAD_DARK);
        UiDraw_PopClip();
        if (!m.egg)
        {
            snprintf(buf, sizeof(buf), UI_TR("Lv%u"), (unsigned)m.level);
            UiW_GbaText(UI_FACE_VT323, 24, mx + 196 - UiText_Width(UI_FACE_VT323, 24, buf), my + 3, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
            const int32_t pct = m.maxHp ? (int32_t)m.hp * 100 / m.maxHp : 0;
            const uint32_t top = pct > 50 ? UI_RGBA(115, 255, 172, 255) : pct > 20 ? UI_RGBA(255, 246, 139, 255) : UI_RGBA(255, 180, 123, 255);
            const uint32_t bot = pct > 50 ? UI_RGBA(90, 213, 131, 255) : pct > 20 ? UI_RGBA(255, 230, 57, 255) : UI_RGBA(222, 106, 90, 255);
            UiW_Bar(mx + 40, my + 29, 156, 7, pct, top, bot);
        }
    }

    // 4. Right Start Menu Panel (.hub-start: 672, 14, 274)
    const int32_t rowH = GbaRowH();
    const int32_t menuH = sCount * rowH + 28;
    UiW_GbaWin(MENU_X, GBA_MENU_Y, GBA_MENU_W, menuH, UI_GBA_WIN_DEFAULT);
    for (int32_t i = 0; i < sCount; i++)
    {
        const int32_t ry = GBA_MENU_Y + 15 + i * rowH;
        const int32_t rh = rowH - 2;
        const bool sel = (i == sSel);
        if (sel)
        {
            UiW_GbaBox(MENU_X + 15, ry, GBA_MENU_W - 30, rh, 3, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(MENU_X + 24, ry + (rh - 12) / 2 + 1, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(MENU_X + 23, ry + (rh - 12) / 2, true, UI_RGBA(230, 8, 8, 255));
        }
        EntryIcon(sEntries[i], MENU_X + 38, ry + (rh - 32) / 2);
        char label[24];
        EntryLabel(sEntries[i], label, sizeof(label));
        UiW_GbaTextMid(UI_FACE_VT323, 32, MENU_X + 78, ry, rh, label, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }

    // 5. Full-width Bottom GBA Dialogue Box (.gba-msg: 14, 438, 932, 94)
    UiW_GbaMsg(14, 438, 932, 94, false, true);
    UiW_GbaTextMid(UI_FACE_VT323, 32, 38, 438, 94, UiLocale_Lookup(sHints[sEntries[sSel]]), UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
}

static void BuildGbaSavePopup(void)
{
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_RGBA(16, 28, 28, 175));
    char buf[96], name[16], zone[32], time[16];
    UiGame_PlayerName(name, sizeof(name));
    UiGame_ZoneName(zone, sizeof(zone));
    UiGame_PlayTime(time, sizeof(time));

    // Left Save Journal Card (.save-card: 22, 22, 500, 396) matching 10_save
    UiW_GbaWin(22, 22, 500, 396, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(34, 34, 476, 30, UI_GBA_TAG_TEAL, UI_TR("SAVE JOURNAL"), zone);

    const uint16_t tid = (uint16_t)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8));
    const uint16_t dexOwned = IsNationalPokedexEnabled() ? GetNationalPokedexCount(FLAG_GET_CAUGHT) : GetHoennPokedexCount(FLAG_GET_CAUGHT);
    const uint16_t dexSeen = IsNationalPokedexEnabled() ? GetNationalPokedexCount(FLAG_GET_SEEN) : GetHoennPokedexCount(FLAG_GET_SEEN);
    char rowVals[5][48];
    snprintf(rowVals[0], sizeof(rowVals[0]), UI_TR("%s · IDNo. %05u"), name, (unsigned)tid);
    snprintf(rowVals[1], sizeof(rowVals[1]), UI_TR("%u HOENN BADGES"), (unsigned)UiGame_BadgeCount());
    snprintf(rowVals[2], sizeof(rowVals[2]), UI_TR("%03u CAUGHT / %03u SEEN"), (unsigned)dexOwned, (unsigned)dexSeen);
    snprintf(rowVals[3], sizeof(rowVals[3]), "%s", time);
    snprintf(rowVals[4], sizeof(rowVals[4]), "SLOT %u · ux0:data/POKEVITA/", (unsigned)UiGame_SaveSlot());
    static const char *const kRowKeys[5] = { UI_N("PLAYER"), UI_N("BADGES"), UI_N("POKéDEX"), UI_N("PLAY TIME"), UI_N("SAVE TARGET") };
    const uint32_t kValCols[5] = {
        UI_RGBA(32, 104, 168, 255),
        UI_RGBA(184, 128, 12, 255),
        UI_RGBA(32, 156, 8, 255),
        UI_RGBA(41, 49, 49, 255),
        UI_RGBA(41, 49, 49, 255),
    };

    UiW_GbaBox(34, 72, 476, 228, 4, UI_RGBA(40, 48, 48, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 0);
    for (int32_t r = 0; r < 5; r++)
    {
        const int32_t ry = 74 + r * 45;
        UiDraw_Rect(36, ry, 472, 45, (r & 1) ? UI_RGBA(244, 245, 232, 255) : C_WHITE);
        if (r < 4)
            UiDraw_Rect(36, ry + 44, 472, 1, UI_RGBA(213, 213, 205, 255));
        UiW_GbaTextMid(UI_FACE_VT323, 24, 48, ry, 45, UiLocale_Lookup(kRowKeys[r]), UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaTextMid(UI_FACE_VT323, 28, 496 - UiText_Width(UI_FACE_VT323, 28, rowVals[r]), ry, 45, rowVals[r], kValCols[r], UI_GBA_SHAD_LIGHT);
    }

    // Bottom Party Strip inside Save Journal (.save-party-strip: 34, 308, 476, 96)
    UiW_Bevel(34, 308, 476, 96, 4, UI_RGBA(24, 32, 32, 255), UI_RGBA(27, 44, 50, 255), UI_RGBA(27, 44, 50, 255),
              2, UI_RGBA(72, 120, 136, 255), UI_RGBA(16, 24, 28, 255), 0);
    const int32_t pCount = UiGame_PartyCount();
    UiW_GbaText(UI_FACE_VT323, 24, 46, 314, UI_TR("CURRENT PARTY"), UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
    snprintf(buf, sizeof(buf), UI_TR("%d / 6 ACTIVE"), (int)pCount);
    UiW_GbaText(UI_FACE_VT323, 24, 498 - UiText_Width(UI_FACE_VT323, 24, buf), 314, buf, UI_RGBA(168, 230, 207, 255), UI_GBA_SHAD_DARK);
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t sx = 44 + i * 76;
        const int32_t sy = 342;
        if (i < pCount)
        {
            UiMon m;
            UiGame_Mon(i, &m);
            UiW_Bevel(sx, sy, 70, 54, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(48, 104, 128, 255), UI_RGBA(48, 104, 128, 255),
                      1, UI_RGBA(110, 192, 216, 255), UI_RGBA(20, 48, 64, 255), 0);
            UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 16) & 1), sx + 19, sy + 2, 1);
            if (!m.egg)
            {
                snprintf(buf, sizeof(buf), UI_TR("Lv%u"), (unsigned)m.level);
                UiW_GbaTextCentred(UI_FACE_VT323, 20, sx, sy + 32, 70, 20, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
            }
        }
        else
            UiW_Bevel(sx, sy, 70, 54, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(24, 38, 44, 255), UI_RGBA(20, 32, 36, 255), 0, 0, 0, 0);
    }

    // Right Confirm Box (.save-confirm: 724, 310, 214, 110) when confirming
    if (sPopup == POP_SAVE)
        DrawGbaYesNo();

    // Bottom Dialogue Box (.gba-msg: 14, 438, 932, 94)
    UiW_GbaMsg(14, 438, 932, 94, false, true);
    if (sPopup == POP_SAVED)
    {
        UiW_GbaText(UI_FACE_VT323, 36, 38, 448, sSaveOk ? UI_TR("GAME SAVED SAFELY!") : UI_TR("SAVE FAILED!"),
                    sSaveOk ? UI_RGBA(32, 156, 8, 255) : UI_RGBA(217, 72, 59, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaText(UI_FACE_VT323, 28, 38, 488, sSaveOk ? "Saved to ux0:data/POKEVITA/" : UI_TR("Try again."),
                    UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }
    else
    {
        UiW_GbaText(UI_FACE_VT323, 32, 38, 450,
                    UiGame_SaveOverwritesOtherGame() ? UI_TR("Overwrite existing save file?") : UI_TR("Would you like to save the game?"),
                    UiGame_SaveOverwritesOtherGame() ? UI_RGBA(217, 72, 59, 255) : UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaText(UI_FACE_VT323, 24, 38, 488, "Existing save data in ux0:data/POKEVITA/ will be updated.",
                    UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGbaHub();
        if (sPopup == POP_PYRAMID_RETIRE)
            BuildRetirePopup(true);
        else if (sPopup != POP_NONE)
            BuildGbaSavePopup();
        if (sToast)
            UiW_Toast(sToast);
        return;
    }
    BuildStatus();
    BuildParty();
    BuildMenu();
    if (sPopup == POP_PYRAMID_RETIRE)
        BuildRetirePopup(false);
    else if (sPopup != POP_NONE)
        BuildPopup();
    if (sToast)
        UiW_Toast(sToast);
}

const UiScreen gUiHubScreen = { Enter, Update, Build, Resume };
