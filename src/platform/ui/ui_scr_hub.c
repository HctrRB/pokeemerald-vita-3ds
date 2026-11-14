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
#include "safari_zone.h"
#include "sound.h"
#include "constants/battle_frontier.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/species.h"

enum HubEntry { HUB_DEX, HUB_MON, HUB_BAG, HUB_NAV, HUB_CARD, HUB_SAVE, HUB_OPTION, HUB_NUZLOCKE, HUB_RETIRE, HUB_EXIT, HUB_COUNT };

// Informative only: short, at most two lines in the small hint box
static const char *const sHints[HUB_COUNT] = {
    "POKéMON seen and caught.",
    "Check and organize your party.",
    "Items you bought or found.",
    "Map, calls, condition, ribbons.",
    "Your money and game data.",
    "Save your progress.",
    "Game settings.",
    "Areas, fallen allies, level cap.",
    "Leave the SAFARI GAME.",
    "Close this menu.",
};

#define MENU_X 672
#define MENU_W 272
#define ROW_H 40
#define MENU_Y 20

static uint8_t sEntries[HUB_COUNT];
static int32_t sCount, sSel;
static uint32_t sFrame;
static int32_t sPopup;                 // 0 none, 1 save confirm, 2 save result, 3 retire confirm
static int32_t sPopupSel;
static bool sSaveOk;
static int32_t sRunX, sRunW;
static const char *sToast;
static int32_t sToastTicks;


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
    if (!inSafari && !inPike) sEntries[sCount++] = HUB_SAVE;
    sEntries[sCount++] = HUB_OPTION;
    if (VitaExtras_GetFast(FAST_EXTRA_NUZLOCKE) != 0) sEntries[sCount++] = HUB_NUZLOCKE;
    if (inSafari) sEntries[sCount++] = HUB_RETIRE;
    sEntries[sCount++] = HUB_EXIT;
    if (sSel >= sCount) sSel = sCount - 1;
}

static void Enter(void)
{
    sPopup = 0;
    sToast = NULL;
    BuildEntries();
}

static void Resume(void)
{
    BuildEntries();
}

static void RetireSafari(void)
{
    // Same as the game's START menu "RETIRE": the script asks and warps to the counter
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
    case HUB_SAVE:     sPopup = 1; sPopupSel = 0; break;
    case HUB_OPTION:   UiNav_Push(&gUiOptionsScreen); break;
    case HUB_NUZLOCKE: UiNav_Push(&gUiNuzlockeScreen); break;
    case HUB_RETIRE:   sPopup = 3; sPopupSel = 1; break;
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

    if (sPopup == 1)
    {
        const int32_t r = UpdateYesNo(in);
        if (r == 1)
        {
            sSaveOk = UiGame_Save();
            PlaySE(sSaveOk ? SE_SAVE : SE_FAILURE);
            sPopup = 2;
        }
        else if (r == 0)
            sPopup = 0;
        return true;
    }
    if (sPopup == 2)
    {
        if (in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE) || in->tap)
        {
            PlaySE(SE_SELECT);
            sPopup = 0;
        }
        return true;
    }
    if (sPopup == 3)
    {
        const int32_t r = UpdateYesNo(in);
        if (r == 1)
            UiNav_Close(RetireSafari);
        else if (r == 0)
            sPopup = 0;
        return true;
    }

    if (in->pressed & (VITA_BTN_CIRCLE | VITA_BTN_START))
    {
        PlaySE(SE_SELECT);
        return false;
    }
    if (sRunW > 0 && UiHit(in, sRunX, 116, sRunW, 32))
    {
        VitaExtras_Set("auto_run", VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) ? 0 : 1);
        VitaExtras_Save();
        PlaySE(SE_SELECT);
        return true;
    }
    const int32_t before = sSel;
    if (in->repeat & VITA_BTN_UP) sSel = (sSel + sCount - 1) % sCount;
    if (in->repeat & VITA_BTN_DOWN) sSel = (sSel + 1) % sCount;
    bool activate = (in->pressed & VITA_BTN_CROSS) != 0;
    for (int32_t i = 0; i < sCount; i++)
        if (UiHit(in, MENU_X + 8, MENU_Y + 10 + i * ROW_H, MENU_W - 16, ROW_H))
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
    case HUB_SAVE:     UiSpr_ItemIcon(ITEM_ORANGE_MAIL, x + 4, y + 4, 1); break;
    case HUB_OPTION:   UiSpr_ItemIcon(ITEM_CARD_KEY, x + 4, y + 4, 1); break;
    case HUB_NUZLOCKE: UiDraw_Sprite(UI_SPR_BALL_PREMIER, x + 4, y + 4, 2); break;
    case HUB_RETIRE:   UiDraw_Sprite(UI_SPR_BALL_SAFARI, x + 4, y + 4, 2); break;
    default: break;
    }
}

static void EntryLabel(uint8_t e, char *out, size_t cap)
{
    static const char *const labels[HUB_COUNT] = { "POKéDEX", "POKéMON", "BAG", "POKéNAV", NULL, "SAVE", "OPTION", "NUZLOCKE", "RETIRE", "EXIT" };
    if (e == HUB_CARD)
        UiGame_PlayerName(out, cap);
    else
        snprintf(out, cap, "%s", labels[e]);
}

static int32_t StatusCapsule(int32_t x, int32_t y, const char *text, uint32_t colour)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, text) + 20;
    UiW_Capsule(x, y, w, 32);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 10, y, text, colour);
    return w;
}

static void BuildStatus(void)
{
    char buf[48], zone[32];
    UiGame_ZoneName(zone, sizeof(zone));
    UiW_PanelDark(16, 16, 360, 52);
    UiText_Draw(UI_FACE_PS2P, 16, 34, 34, zone, UI_COL_TITLE);

    // Row 1: clock and money
    char clock[8];
    UiGame_Clock(clock, sizeof(clock));
    const unsigned rate = UiGame_ClockRate();
    if (rate > 1)
        snprintf(buf, sizeof(buf), "%s x%u", clock, rate);
    else
        snprintf(buf, sizeof(buf), "%s", clock);
    int32_t x = 16;
    x += StatusCapsule(x, 76, buf, UI_COL_ON_FRAME) + 8;
    snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
    StatusCapsule(x, 76, buf, UI_COL_GOLD);

    // Row 2: registered item, repel or Safari counter
    x = 16;
    const uint16_t reg = UiGame_RegisteredItem();
    if (reg != ITEM_NONE)
    {
        char name[24];
        UiGame_ItemName(reg, name, sizeof(name));
        snprintf(buf, sizeof(buf), "SEL: %s", name);
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf) + 48;
        UiW_Capsule(x, 116, w, 32);
        UiSpr_ItemIcon(reg, x + 6, 120, 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 36, 116, buf, UI_COL_ON_FRAME);
        x += w + 8;
    }
    if (UiGame_InSafari())
    {
        snprintf(buf, sizeof(buf), "SAFARI BALLS %u", (unsigned)UiGame_SafariBalls());
        x += StatusCapsule(x, 116, buf, UI_COL_ON_FRAME) + 8;
    }
    else if (UiGame_RepelSteps() > 0)
    {
        snprintf(buf, sizeof(buf), "REPEL %u", (unsigned)UiGame_RepelSteps());
        x += StatusCapsule(x, 116, buf, UI_COL_ON_FRAME) + 8;
    }
    // AUTO RUN toggle (touch), only with the RUNNING SHOES
    if (UiGame_HasFlag(FLAG_SYS_B_DASH))
    {
        const bool on = VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0;
        sRunX = x;
        sRunW = StatusCapsule(x, 116, on ? "AUTO RUN: ON" : "AUTO RUN: OFF", on ? UI_RGBA(110, 219, 151, 255) : UI_COL_ON_SOFT);
    }
    else
        sRunW = 0;
}

static void BuildParty(void)
{
    const int32_t n = UiGame_PartyCount();
    if (n == 0)
        return;
    const int32_t y0 = UI_SCREEN_H - 40 - 16 - n * 36 - 12;
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
        snprintf(lv, sizeof(lv), "Lv%u", (unsigned)m.level);
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
    const int32_t hy = UI_SCREEN_H - 40 - 16 - 76;
    UiW_PanelDark(UI_SCREEN_W - 16 - 300, hy, 300, 76);
    UiW_TextWrapped(UI_SCREEN_W - 16 - 300 + 14, hy + 8, 272, 30, sHints[sEntries[sSel]], UI_COL_ON_FRAME, 2);
}

static void BuildPopup(void)
{
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    char buf[64];
    if (sPopup == 2)
    {
        UiW_Window(POP_X, POP_Y + 40, POP_W, 136, "SAVE");
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 92, sSaveOk ? "GAME SAVED SAFELY!" : "SAVE FAILED!", UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 124, sSaveOk ? "Saved to ux0:data/POKEVITA/" : "Try again.", UI_COL_INK_SOFT);
        return;
    }
    UiW_Window(POP_X, POP_Y, POP_W, 224, sPopup == 1 ? "SAVE" : "RETIRE");
    if (sPopup == 1)
    {
        char name[16], zone[32], time[16];
        UiGame_PlayerName(name, sizeof(name));
        UiGame_ZoneName(zone, sizeof(zone));
        UiGame_PlayTime(time, sizeof(time));
        snprintf(buf, sizeof(buf), "SLOT %u · %s · %s", (unsigned)UiGame_SaveSlot(), name, zone);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 50, buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), "BADGES %u · POKéDEX %u · TIME %s", (unsigned)UiGame_BadgeCount(), (unsigned)UiGame_DexOwned(), time);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 82, buf, UI_COL_INK_SOFT);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 118,
                    UiGame_SaveOverwritesOtherGame() ? "This overwrites the other game!" : "Would you like to save the game?",
                    UiGame_SaveOverwritesOtherGame() ? UI_COL_HP_LOW : UI_COL_INK);
    }
    else
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 20, POP_Y + 70, "Would you like to retire?", UI_COL_INK);
    static const char *const opt[2] = { "YES", "NO" };
    for (int32_t i = 0; i < 2; i++)
    {
        const int32_t bx = POP_X + 16 + i * 208;
        UiW_Row(bx, POP_Y + 168, 200, 40, sPopupSel == i, 0);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, bx + 84, POP_Y + 172, opt[i], sPopupSel == i ? UI_COL_SELECT_INK : UI_COL_INK);
    }
}

static void Build(void)
{
    BuildStatus();
    BuildParty();
    BuildMenu();
    UiW_FooterBar();
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Open");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Close");
    char clock[8];
    UiGame_Clock(clock, sizeof(clock));
    UiW_FooterClock(clock);
    if (sPopup)
        BuildPopup();
    if (sToast)
        UiW_Toast(sToast);
}

const UiScreen gUiHubScreen = { Enter, Update, Build, Resume };
