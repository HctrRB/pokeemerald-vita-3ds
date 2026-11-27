// "Choose a POKéMON" opened by game scripts (party_menu.c specials) instead of the GBA party
// screen. Each kind is a row of sKinds: what it shows (header, help, tags, which POKéMON can be
// chosen) and what choosing sets for the script that waits after `waitstate`. A new kind is a new
// row, not another branch. Optional parts of a row: an action menu on the chosen POKéMON (the
// Battle Pyramid's TAKE / TOSS), a CONFIRM for a choice of several (the Battle Frontier's half
// party) and a question before leaving (its "Cancel the challenge?"); their rules and texts are
// party_menu.c's own (PartyMenu_*ForUi).

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "contest.h"
#include "event_data.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"

extern u8 PartyMenu_TutorStatusForUi(u8 slot, u8 tutor);
extern u8 PartyMenu_TakeHeldItemForUi(u8 slot);
extern bool8 PartyMenu_AskTossHeldItemForUi(u8 slot);
extern void PartyMenu_TossHeldItemForUi(u8 slot);
extern void PartyMenu_BeginHalfPartyForUi(void);
extern u8 PartyMenu_HalfPartyMaxForUi(void);
extern u8 PartyMenu_HalfPartyStatusForUi(u8 slot);
extern u8 PartyMenu_HalfPartyPlaceForUi(u8 slot);
extern const u8 *PartyMenu_HalfPartyPlaceNameForUi(u8 place);
extern bool8 PartyMenu_HalfPartyToggleForUi(u8 slot);
extern const u8 *PartyMenu_HalfPartyCheckForUi(void);
extern const u8 *PartyMenu_HalfPartyCancelTextForUi(void);

#define CARD_X 22
#define CARD_Y 66
#define CARD_W 308
#define CARD_H 70

typedef struct
{
    const char *header;                          // right of the title (NULL: none)
    const char *help;
    const char *button;                          // the choose button
    const char *foot;                            // its footer guide
    bool eggOk;                                  // an EGG can be chosen (the script answers it)
    const char *(*tag)(int32_t slot, bool *ok);  // chip by the POKéMON and whether it can be chosen
    const char *refused;                         // said when a POKéMON can't be chosen
    void (*prepare)(void);                       // once when the screen opens (NULL: nothing)
    void (*defaults)(void);                      // the results if nothing is chosen
    bool (*chosen)(int32_t slot);                // sets the results; true: the screen closes
    void (*action)(int32_t slot, int32_t id);    // a choice of the action menu chosen() opened
    bool (*confirm)(void);                       // the button confirms a choice of several (true: closes)
    const u8 *(*cancelAsk)(void);                // asked before leaving with nothing (YES: defaults again)
} ChooseKind;

enum { ASK_NONE, ASK_TOSS, ASK_CANCEL };

static int sKind;
static UiList sList;
static UiMenu sForget, sAction, sYesNo;
static int32_t sAsk;
static char sAskText[128];
static uint32_t sFrame;

// The game's text (gStringVar4 and the like) as a toast
static void SayGame(const u8 *text, bool ok)
{
    char buf[128];
    UiGame_Text(text, buf, sizeof(buf));
    PlaySE(ok ? SE_SELECT : SE_FAILURE);
    UiKit_Toast("%s", buf);
}

static void Ask(int32_t kind, const u8 *text)
{
    UiGame_Text(text, sAskText, sizeof(sAskText));
    sAsk = kind;
    UiMenu_Open(&sYesNo, UI_TR("POKéMON"), 700, 330, 220);
    UiMenu_Add(&sYesNo, UI_TR("YES"), 1);
    UiMenu_Add(&sYesNo, UI_TR("NO"), UI_MENU_CANCEL);
    sYesNo.list.sel = 0;                             // YES first, as PartyMenuDisplayYesNoMenu
}

static void Resume(void)
{
    // The scripts fade to black before opening the party menu, which fades back in on its way out
    // (CB2_FadeFromPartyMenu): do the same, or the field stays black
    if (gPaletteFade.y != 0 && !gPaletteFade.active)
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    ScriptContext_Enable();
}

// ---------- Move tutor (ChooseMonForMoveTutor): VAR_RESULT ----------

static uint16_t TutorMove(void)
{
    return gTutorMoves[gSpecialVar_0x8005];
}

static const char *TutorTag(int32_t slot, bool *ok)
{
    const u8 st = PartyMenu_TutorStatusForUi((u8)slot, (u8)gSpecialVar_0x8005);
    *ok = st == 0;
    return st == 0 ? UI_TR("ABLE") : st == 2 ? UI_TR("LEARNED") : UI_TR("NOT ABLE");
}

static void TutorDefaults(void)
{
    gSpecialVar_Result = FALSE;
}

static void Learned(int32_t slot)
{
    UiMon m;
    char mv[16];
    UiGame_Mon(slot, &m);
    UiGame_MoveName(TutorMove(), mv, sizeof(mv));
    PlaySE(SE_SUCCESS);
    UiKit_Toast(UI_TR("%s learned %s!"), m.name, mv);
    gSpecialVar_Result = TRUE;
    UiNav_Pop();
}

static bool TutorChosen(int32_t slot)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GiveMoveToMon(mon, TutorMove()) != MON_HAS_MAX_MOVES)
    {
        Learned(slot);
        return false;                                // Learned pops
    }
    // Four moves already: which one to forget
    UiMenu_Open(&sForget, UI_TR("FORGET WHICH MOVE?"), 348, 150, 420);
    for (int32_t i = 0; i < 4; i++)
    {
        UiMove mv;
        UiGame_Move(slot, i, &mv);
        if (mv.move)
            UiMenu_Add(&sForget, mv.name, i);
    }
    UiMenu_Add(&sForget, UI_TR("DON'T LEARN"), 9);
    PlaySE(SE_SELECT);
    return false;
}

// ---------- The others: VAR_0x8004 ----------

static void SlotDefaults(void)
{
    gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;
}

// BufferMonSelection: the slot in VAR_0x8004 and the party cursor (GetCursorSelectionMonId)
static bool SlotChosen(int32_t slot)
{
    gSpecialVar_0x8004 = (u16)slot;
    gPartyMenu.slotId = (s8)slot;
    return true;
}

static const char *DaycareTag(int32_t slot, bool *ok)
{
    UiMon m;
    UiGame_Mon(slot, &m);
    *ok = !m.egg;
    return m.egg ? UI_TR("EGG") : NULL;
}

// Contest entry (CB2_ChooseContestMon): the entrant too
static void ContestDefaults(void)
{
    gContestMonPartyIndex = PARTY_NOTHING_CHOSEN;
    gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;
}

static bool ContestChosen(int32_t slot)
{
    gContestMonPartyIndex = (u8)slot;
    return SlotChosen(slot);
}

// Move Relearner (CB2_ChooseMonForMoveRelearner): how many moves it can remember, in VAR_0x8005
// (counted once when the screen opens: it walks the learnset)
static u8 sRelearnable[PARTY_SIZE];

static void RelearnerPrepare(void)
{
    for (int32_t i = 0; i < UiGame_PartyCount() && i < PARTY_SIZE; i++)
        sRelearnable[i] = GetNumberOfRelearnableMoves(&gPlayerParty[i]);
}

static const char *RelearnerTag(int32_t slot, bool *ok)
{
    static char sBuf[16];
    UiMon m;
    UiGame_Mon(slot, &m);
    *ok = true;                                      // the script answers an EGG or no moves itself
    if (m.egg)
        return NULL;
    const u8 n = sRelearnable[slot];
    snprintf(sBuf, sizeof(sBuf), n == 1 ? UI_TR("1 MOVE") : UI_TR("%u MOVES"), (unsigned)n);
    return sBuf;
}

static bool RelearnerChosen(int32_t slot)
{
    gSpecialVar_0x8005 = sRelearnable[slot];
    return SlotChosen(slot);
}

// Battle Pyramid, a full bag on leaving (BattlePyramidChooseMonHeldItems): the held items, TAKE
// ITEM or TOSS each (PARTY_MENU_TYPE_STORE_PYRAMID_HELD_ITEMS); nothing is chosen for the script
enum { PYR_TAKE = 1, PYR_TOSS };

static const char *HeldItemTag(int32_t slot, bool *ok)
{
    static char sBuf[24];
    UiMon m;
    UiGame_Mon(slot, &m);
    *ok = !m.egg;
    if (m.egg || m.item == ITEM_NONE)
        return m.egg ? NULL : UI_TR("NO ITEM");
    UiGame_ItemName(m.item, sBuf, sizeof(sBuf));
    return sBuf;
}

static bool HeldItemChosen(int32_t slot)
{
    UiMon m;
    UiGame_Mon(slot, &m);
    UiMenu_Open(&sAction, m.name, 348, 150, 300);
    UiMenu_Add(&sAction, UI_TR("TAKE ITEM"), PYR_TAKE);
    UiMenu_Add(&sAction, UI_TR("TOSS"), PYR_TOSS);
    UiMenu_Add(&sAction, UI_TR("CANCEL"), UI_MENU_CANCEL);
    return false;
}

static void HeldItemAction(int32_t slot, int32_t id)
{
    if (id == PYR_TAKE)
        SayGame(gStringVar4, PartyMenu_TakeHeldItemForUi((u8)slot) == 2);
    else if (id == PYR_TOSS)
    {
        if (PartyMenu_AskTossHeldItemForUi((u8)slot))
            Ask(ASK_TOSS, gStringVar4);
        else
            SayGame(gStringVar4, false);
    }
}

// Battle Frontier and Tents (ChoosePartyForBattleFrontier): the POKéMON that enter, in order;
// VAR_RESULT TRUE once CONFIRM passes the game's checks (CB2_ReturnFromChooseBattleFrontierParty)
static void HalfPartyDefaults(void)
{
    PartyMenu_BeginHalfPartyForUi();
    gSpecialVar_Result = FALSE;
}

static const char *HalfPartyTag(int32_t slot, bool *ok)
{
    static char sBuf[16];
    const u8 status = PartyMenu_HalfPartyStatusForUi((u8)slot);
    *ok = status != 2;
    if (status == 2)
        return UI_TR("NOT ABLE");
    const u8 place = PartyMenu_HalfPartyPlaceForUi((u8)slot);
    if (place == 0)
        return UI_TR("ABLE");
    UiGame_Text(PartyMenu_HalfPartyPlaceNameForUi(place), sBuf, sizeof(sBuf));
    return sBuf;
}

static bool HalfPartyChosen(int32_t slot)
{
    if (!PartyMenu_HalfPartyToggleForUi((u8)slot))
        SayGame(gStringVar4, false);                 // "No more than N POKéMON may enter."
    return false;
}

static bool HalfPartyConfirm(void)
{
    const u8 *why = PartyMenu_HalfPartyCheckForUi();
    if (why != NULL)
    {
        SayGame(why, false);
        return false;
    }
    gSpecialVar_Result = TRUE;
    return true;
}

static const ChooseKind sKinds[UI_CHOOSE_COUNT] = {
    [UI_CHOOSE_TUTOR] = {
        .help = UI_N("Choose who will learn the move. Those marked ABLE can learn it."), .button = UI_N("TEACH"), .foot = UI_N("Teach"),
        .tag = TutorTag, .refused = UI_N("It can't learn that move."), .defaults = TutorDefaults, .chosen = TutorChosen },
    [UI_CHOOSE_DAYCARE] = {
        .header = UI_N("DAY CARE"), .help = UI_N("Choose the POKéMON to leave at the DAY CARE."), .button = UI_N("CHOOSE"), .foot = UI_N("Choose"),
        .tag = DaycareTag, .refused = UI_N("An EGG can't be chosen."), .defaults = SlotDefaults, .chosen = SlotChosen },
    // ChoosePartyMon (PARTY_ACTION_CHOOSE_AND_CLOSE) takes an EGG: the scripts (Name Rater, Move
    // Deleter, trades) answer it themselves
    [UI_CHOOSE_ANY] = {
        .help = UI_N("Choose a POKéMON."), .button = UI_N("CHOOSE"), .foot = UI_N("Choose"), .eggOk = true,
        .defaults = SlotDefaults, .chosen = SlotChosen },
    [UI_CHOOSE_CONTEST] = {
        .header = UI_N("CONTEST"), .help = UI_N("Choose the POKéMON that will enter the CONTEST."), .button = UI_N("ENTER"), .foot = UI_N("Enter"),
        .eggOk = true, .defaults = ContestDefaults, .chosen = ContestChosen },
    [UI_CHOOSE_RELEARNER] = {
        .header = UI_N("MOVE RELEARNER"), .help = UI_N("Choose the POKéMON that will remember a move."), .button = UI_N("CHOOSE"), .foot = UI_N("Choose"),
        .eggOk = true, .tag = RelearnerTag, .prepare = RelearnerPrepare, .defaults = SlotDefaults, .chosen = RelearnerChosen },
    [UI_CHOOSE_PYRAMID_ITEMS] = {
        .header = UI_N("BATTLE PYRAMID"),
        .help = UI_N("The PYRAMID BAG is full. Choose a POKéMON to take or toss the item it holds."),
        .button = UI_N("CHOOSE"), .foot = UI_N("Choose"), .tag = HeldItemTag, .refused = UI_N("An EGG holds no item."),
        .defaults = SlotDefaults, .chosen = HeldItemChosen, .action = HeldItemAction },
    [UI_CHOOSE_FRONTIER_PARTY] = {
        .header = UI_N("BATTLE FRONTIER"),
        .help = UI_N("Choose the POKéMON that will enter, in order: choose one again to take it out. Then CONFIRM."),
        .button = UI_N("CONFIRM"), .foot = UI_N("Enter / out"), .tag = HalfPartyTag, .refused = UI_N("That POKéMON can't enter."),
        .defaults = HalfPartyDefaults, .chosen = HalfPartyChosen, .confirm = HalfPartyConfirm,
        .cancelAsk = PartyMenu_HalfPartyCancelTextForUi },
};

bool Ui_OpenChooseMon(int kind)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || kind < 0 || kind >= UI_CHOOSE_COUNT || !__atomic_load_n(&gUiVoxelFrame, __ATOMIC_RELAXED))
        return false;
    sKind = kind;
    sKinds[kind].defaults();                          // if the screen is left with circle
    UiCore_OpenFromScript(&gUiChooseScreen, Resume);
    return true;
}

// Chip and whether the POKéMON can be chosen
static const char *Tag(int32_t slot, bool *ok)
{
    const ChooseKind *k = &sKinds[sKind];
    if (k->tag != NULL)
        return k->tag(slot, ok);
    UiMon m;
    UiGame_Mon(slot, &m);
    *ok = k->eggOk || !m.egg;
    return NULL;
}

static void Enter(void)
{
    sList.sel = sList.first = 0;
    sForget.open = false;
    sAction.open = false;
    sYesNo.open = false;
    sAsk = ASK_NONE;
    UiList_Reset(&sList, UiGame_PartyCount(), 6);
    if (sKinds[sKind].prepare != NULL)
        sKinds[sKind].prepare();
}

// The YES / NO of a question (TOSS, leaving); false: the screen closes
static bool UpdateAsk(const UiInput *in)
{
    const int32_t id = UiMenu_Update(&sYesNo, in);
    if (id == UI_MENU_NONE)
        return true;
    const int32_t kind = sAsk;
    sAsk = ASK_NONE;
    if (id != 1)
        return true;
    if (kind == ASK_TOSS)
    {
        PartyMenu_TossHeldItemForUi((u8)sList.sel);
        SayGame(gStringVar4, true);
        return true;
    }
    sKinds[sKind].defaults();                         // ASK_CANCEL: nothing chosen
    return false;
}

static bool Leave(void)
{
    const ChooseKind *k = &sKinds[sKind];
    PlaySE(SE_SELECT);
    if (k->cancelAsk != NULL)
    {
        Ask(ASK_CANCEL, k->cancelAsk());
        return true;
    }
    return false;                                     // nothing chosen: the defaults stay
}

static bool Update(const UiInput *in)
{
    sFrame++;
    const ChooseKind *k = &sKinds[sKind];
    if (sAsk != ASK_NONE)
        return UpdateAsk(in);
    if (sAction.open)
    {
        const int32_t id = UiMenu_Update(&sAction, in);
        if (id != UI_MENU_NONE && id != UI_MENU_CANCEL && k->action != NULL)
            k->action(sList.sel, id);
        return true;
    }
    if (sForget.open)
    {
        const int32_t id = UiMenu_Update(&sForget, in);
        if (id == UI_MENU_NONE)
            return true;
        if (id == UI_MENU_CANCEL || id == 9)
        {
            UiKit_Toast(UI_TR("It did not learn the move."));
            return true;
        }
        struct Pokemon *mon = &gPlayerParty[sList.sel];
        if (IsMoveHm((u16)GetMonData(mon, MON_DATA_MOVE1 + id)))
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast(UI_TR("HM moves can't be forgotten now."));
            return true;
        }
        RemoveMonPPBonus(mon, (u8)id);
        SetMonMoveSlot(mon, TutorMove(), (u8)id);
        Learned(sList.sel);
        return true;
    }
    if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 364, 436, 200, 40))
        return Leave();
    // A choice of several: the button (or START, the GBA's way to CONFIRM) checks it
    if (k->confirm != NULL && ((in->pressed & VITA_BTN_START) || UiHit(in, 580, 436, 200, 40)))
    {
        if (!k->confirm())
            return true;
        PlaySE(SE_SELECT);
        return false;
    }
    const int32_t a = UiList_Update(&sList, in, CARD_X, CARD_Y, CARD_W, CARD_H, true);
    if (a < 0 && (k->confirm != NULL || !UiHit(in, 580, 436, 200, 40)))
        return true;
    const int32_t slot = a >= 0 ? a : sList.sel;
    bool ok;
    Tag(slot, &ok);
    if (!ok)
    {
        PlaySE(SE_FAILURE);
        UiKit_Toast("%s", k->refused != NULL ? UiLocale_Lookup(k->refused) : UI_TR("That POKéMON can't be chosen."));
        return true;
    }
    PlaySE(SE_SELECT);
    return !k->chosen(slot);
}

static void Build(void)
{
    static const char *const tab[1] = { UI_N("CHOOSE A POKéMON") };
    const ChooseKind *k = &sKinds[sKind];
    int32_t tx[1], tw[1];
    char buf[64], mv[16];
    UiKit_HeaderAs(UI_GBA_BG_PARTY, UI_SPR_BALL_POKE, UI_TR("POKéMON"), tab, 1, 0, tx, tw, 12);
    if (sKind == UI_CHOOSE_TUTOR)
    {
        UiGame_MoveName(TutorMove(), mv, sizeof(mv));
        snprintf(buf, sizeof(buf), UI_TR("TEACH · %s"), mv);
        UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    }
    else if (k->header != NULL)
        UiW_HeaderValue(UiLocale_Lookup(k->header), UI_COL_ON_FRAME);
    UiW_Panel(16, 60, 320, 432);
    for (int32_t i = 0; i < UiGame_PartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = CARD_Y + i * CARD_H;
        const bool on = i == sList.sel;
        UiW_Row(CARD_X, y, CARD_W, CARD_H - 2, on, i);
        const uint8_t f = (uint8_t)((sFrame / (on ? 8 : 16)) & 1);
        UiSpr_MonIconOf(m.species, m.personality, f, CARD_X + 8, y + 14 - (f ? 3 : 0), 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, CARD_X + 52, y, m.name, on ? UI_COL_SELECT_INK : UI_COL_INK);
        if (!m.egg)
        {
            snprintf(buf, sizeof(buf), UI_TR("Lv%u"), (unsigned)m.level);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, CARD_X + 52, y + 32, buf, on ? UI_COL_SELECT_INK : UI_COL_INK_SOFT);
        }
        bool ok;
        const char *tag = Tag(i, &ok);
        if (tag)
            UiW_Chip(CARD_X + CARD_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, tag) - 16, y + 36, tag, ok);
    }
    UiW_Panel(348, 60, 596, 432);
    UiW_TextWrapped(364, 80, 564, 30, UiLocale_Lookup(k->help), UI_COL_INK, 4);
    UiW_Button(364, 436, 200, UI_TR("BACK"), UI_BTN_RED, false);
    UiW_Button(580, 436, 200, UiLocale_Lookup(k->button), UI_BTN_GREEN, false);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UiLocale_Lookup(k->foot));
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Cancel"));
    if (k->confirm != NULL)
        UiW_FooterKey(&fx, "START", UiLocale_Lookup(k->button));
    UiKit_FooterClock();
    if (sForget.open)
        UiMenu_Draw(&sForget);
    if (sAction.open)
        UiMenu_Draw(&sAction);
    if (sAsk != ASK_NONE)
    {
        UiDraw_Darken(1, 3);
        UiW_DialogBox(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124);
        UiW_TextWrapped(76, UI_SCREEN_H - 24 - 112, UI_SCREEN_W - 152, 40, sAskText, UI_COL_INK, 2);
        UiMenu_Draw(&sYesNo);
    }
}

const UiScreen gUiChooseScreen = { Enter, Update, Build, NULL };
