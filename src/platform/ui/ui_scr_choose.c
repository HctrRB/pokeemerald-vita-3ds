// "Choose a POKéMON" opened by game scripts (party_menu.c specials) instead of the GBA party
// screen: the move tutor (teach a move, forget one if needed), the day care (which POKéMON to
// leave) and the generic ChoosePartyMon. Only the buttons each case needs, no tabs. The results
// are the ones the scripts read after `waitstate`: VAR_RESULT (tutor) or VAR_0x8004 (others).

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "script.h"
#include "sound.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

extern u8 PartyMenu_TutorStatusForUi(u8 slot, u8 tutor);

#define CARD_X 22
#define CARD_Y 66
#define CARD_W 308
#define CARD_H 70

static int sKind;
static UiList sList;
static UiMenu sForget;
static uint32_t sFrame;

static void Resume(void)
{
    // The scripts fade to black before opening the party menu, which fades back in on its way out
    // (CB2_FadeFromPartyMenu): do the same, or the field stays black
    if (gPaletteFade.y != 0 && !gPaletteFade.active)
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    ScriptContext_Enable();
}

bool Ui_OpenChooseMon(int kind)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || !__atomic_load_n(&gUiVoxelFrame, __ATOMIC_RELAXED))
        return false;
    sKind = kind;
    // Defaults if the screen is left with circle
    if (kind == UI_CHOOSE_TUTOR) gSpecialVar_Result = FALSE;
    else gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;
    UiCore_OpenFromScript(&gUiChooseScreen, Resume);
    return true;
}

static uint16_t TutorMove(void)
{
    return gTutorMoves[gSpecialVar_0x8005];
}

// Tag and whether the POKéMON can be chosen
static const char *Tag(int32_t slot, bool *ok)
{
    UiMon m;
    UiGame_Mon(slot, &m);
    // ChoosePartyMon (PARTY_ACTION_CHOOSE_AND_CLOSE) takes an EGG too: the scripts (Name Rater,
    // Move Deleter, trades) answer it themselves. The tutor and the day care refuse it.
    *ok = sKind == UI_CHOOSE_ANY || !m.egg;
    if (sKind == UI_CHOOSE_TUTOR)
    {
        const u8 st = PartyMenu_TutorStatusForUi((u8)slot, (u8)gSpecialVar_0x8005);
        *ok = st == 0;
        return st == 0 ? "ABLE" : st == 2 ? "LEARNED" : "NOT ABLE";
    }
    if (sKind == UI_CHOOSE_DAYCARE)
        return m.egg ? "EGG" : NULL;
    return NULL;
}

static void Enter(void)
{
    sList.sel = sList.first = 0;
    sForget.open = false;
    UiList_Reset(&sList, UiGame_PartyCount(), 6);
}

static void Learned(int32_t slot)
{
    UiMon m;
    char mv[16];
    UiGame_Mon(slot, &m);
    UiGame_MoveName(TutorMove(), mv, sizeof(mv));
    PlaySE(SE_SUCCESS);
    UiKit_Toast("%s learned %s!", m.name, mv);
    gSpecialVar_Result = TRUE;
    UiNav_Pop();
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sForget.open)
    {
        const int32_t id = UiMenu_Update(&sForget, in);
        if (id == UI_MENU_NONE)
            return true;
        if (id == UI_MENU_CANCEL || id == 9)
        {
            UiKit_Toast("It did not learn the move.");
            return true;
        }
        struct Pokemon *mon = &gPlayerParty[sList.sel];
        if (IsMoveHm((u16)GetMonData(mon, MON_DATA_MOVE1 + id)))
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast("HM moves can't be forgotten now.");
            return true;
        }
        RemoveMonPPBonus(mon, (u8)id);
        SetMonMoveSlot(mon, TutorMove(), (u8)id);
        Learned(sList.sel);
        return true;
    }
    if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 364, 436, 200, 40))
    {
        PlaySE(SE_SELECT);
        return false;                                   // nothing chosen: the defaults stay
    }
    const int32_t a = UiList_Update(&sList, in, CARD_X, CARD_Y, CARD_W, CARD_H, true);
    if (a < 0 && !UiHit(in, 580, 436, 200, 40))
        return true;
    const int32_t slot = a >= 0 ? a : sList.sel;
    bool ok;
    Tag(slot, &ok);
    if (!ok)
    {
        PlaySE(SE_FAILURE);
        UiKit_Toast(sKind == UI_CHOOSE_TUTOR ? "It can't learn that move." : "An EGG can't be chosen.");
        return true;
    }
    if (sKind != UI_CHOOSE_TUTOR)
    {
        PlaySE(SE_SELECT);
        gSpecialVar_0x8004 = (u16)slot;
        gPartyMenu.slotId = (s8)slot;                // GetCursorSelectionMonId (StoreSelectedPokemonInDaycare...)
        return false;
    }
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GiveMoveToMon(mon, TutorMove()) != MON_HAS_MAX_MOVES)
    {
        Learned(slot);
        return true;
    }
    // Four moves already: which one to forget
    UiMenu_Open(&sForget, "FORGET WHICH MOVE?", 348, 150, 420);
    for (int32_t i = 0; i < 4; i++)
    {
        UiMove mv;
        UiGame_Move(slot, i, &mv);
        if (mv.move)
            UiMenu_Add(&sForget, mv.name, i);
    }
    UiMenu_Add(&sForget, "DON'T LEARN", 9);
    PlaySE(SE_SELECT);
    return true;
}

static void Build(void)
{
    static const char *const tab[1] = { "CHOOSE A POKéMON" };
    int32_t tx[1], tw[1];
    char buf[64], mv[16];
    UiKit_Header("POKéMON", tab, 1, 0, tx, tw, 12);
    if (sKind == UI_CHOOSE_TUTOR)
    {
        UiGame_MoveName(TutorMove(), mv, sizeof(mv));
        snprintf(buf, sizeof(buf), "TEACH · %s", mv);
        UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    }
    else if (sKind == UI_CHOOSE_DAYCARE)
        UiW_HeaderValue("DAY CARE", UI_COL_ON_FRAME);
    UiW_Panel(16, 60, 320, 432);
    for (int32_t i = 0; i < UiGame_PartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = CARD_Y + i * CARD_H;
        const bool on = i == sList.sel;
        UiW_Row(CARD_X, y, CARD_W, CARD_H - 2, on, i);
        const uint8_t f = (uint8_t)((sFrame / (on ? 8 : 16)) & 1);
        UiSpr_MonIcon(m.species, f, CARD_X + 8, y + 14 - (f ? 3 : 0), 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, CARD_X + 52, y, m.name, on ? UI_COL_SELECT_INK : UI_COL_INK);
        if (!m.egg)
        {
            snprintf(buf, sizeof(buf), "Lv%u", (unsigned)m.level);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, CARD_X + 52, y + 32, buf, on ? UI_COL_SELECT_INK : UI_COL_INK_SOFT);
        }
        bool ok;
        const char *tag = Tag(i, &ok);
        if (tag)
            UiW_Chip(CARD_X + CARD_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, tag) - 16, y + 36, tag, ok);
    }
    UiW_Panel(348, 60, 596, 432);
    const char *help = sKind == UI_CHOOSE_TUTOR ? "Choose who will learn the move. Those marked ABLE can learn it."
                     : sKind == UI_CHOOSE_DAYCARE ? "Choose the POKéMON to leave at the DAY CARE."
                                                  : "Choose a POKéMON.";
    UiW_TextWrapped(364, 80, 564, 30, help, UI_COL_INK, 4);
    UiW_Button(364, 436, 200, "BACK", UI_BTN_RED, false);
    UiW_Button(580, 436, 200, sKind == UI_CHOOSE_TUTOR ? "TEACH" : "CHOOSE", UI_BTN_GREEN, false);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sKind == UI_CHOOSE_TUTOR ? "Teach" : "Choose");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Cancel");
    UiKit_FooterClock();
    if (sForget.open)
        UiMenu_Draw(&sForget);
}

const UiScreen gUiChooseScreen = { Enter, Update, Build, NULL };
