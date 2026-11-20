// "Which move should be forgotten?" of the new UI (design/stitch/olvidar_movimiento), replacing the
// summary screen's SELECT_MOVE mode (ShowSelectMovePokemonSummaryScreen): a level up in battle or
// outside it, an evolution, the Move Relearner. Hosted, as that screen is a scene of its own: it
// starts and ends as the summary screen does (sprites and palettes freed, gLastViewedMonIndex) and
// leaves its choice where the callers read it (GetMoveSlotToReplace, VAR_0x8005): the slot of the
// move to forget, or MAX_MON_MOVES (the new move chosen, or circle) to stop learning it. HM moves
// can't be chosen (the game's "HM moves can't be forgotten now."). The cursor goes round the
// known moves and the new one, skipping empty slots, as ChangeSelectedMove. With no new move
// (MOVE_NONE: the Move Deleter) there is no "new move" row: circle stops, as in the game.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "battle_main.h"
#include "data.h"
#include "event_data.h"
#include "main.h"
#include "menu_helpers.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "constants/moves.h"
#include "contest_effect.h"
#include "move_relearner.h"
#include "string_util.h"
#include "constants/songs.h"

extern void SummaryScreen_SetMoveSlotForUi(u8 slot);
extern u8 gLastViewedMonIndex;

#define ROW_X 348
#define ROW_W 580
#define ROW_Y 106
#define ROW_H 40
#define NEW_Y (ROW_Y + MAX_MON_MOVES * ROW_H + 30)

typedef struct
{
    uint16_t move, power;
    uint8_t type, accuracy, pp, maxPp;
    char name[16], desc[96];
} ForgetMove;

static struct Pokemon *sMon;
static uint8_t sMonIndex;
static uint16_t sNewMove;
static void (*sCallback)(void);
static ForgetMove sMoves[MAX_MON_MOVES + 1];   // [MAX_MON_MOVES]: the new move
static int32_t sSel;
static uint32_t sFrame;

static void Fill(ForgetMove *m, uint16_t move, int32_t slot)
{
    memset(m, 0, sizeof(*m));
    move = UiGame_SafeMove(move);
    m->move = move;
    if (move == MOVE_NONE)
        return;
    const struct BattleMove *bm = &gBattleMoves[move];
    UiGame_Text(gMoveNames[move], m->name, sizeof(m->name));
    UiGame_Text(gMoveDescriptionPointers[move - 1], m->desc, sizeof(m->desc));
    m->type = bm->type;
    m->power = bm->power;
    m->accuracy = bm->accuracy;
    if (slot < 0)
        m->pp = m->maxPp = bm->pp;
    else
    {
        m->pp = (uint8_t)GetMonData(sMon, MON_DATA_PP1 + slot);
        m->maxPp = (uint8_t)CalculatePPWithBonus(move, (u8)GetMonData(sMon, MON_DATA_PP_BONUSES), (u8)slot);
    }
}

// As CloseSummaryScreen: back to the caller's callback with the sprites and palettes freed
static void Finish(void)
{
    gLastViewedMonIndex = sMonIndex;
    ResetSpriteData();
    FreeAllSpritePalettes();
    SetMainCallback2(sCallback);
}

bool Ui_OpenForgetMove(struct Pokemon *mons, uint8_t monIndex, uint16_t newMove, void (*callback)(void))
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || mons == NULL || callback == NULL)
        return false;
    sMon = &mons[monIndex];
    sMonIndex = monIndex;
    sNewMove = newMove;
    sCallback = callback;
    // As the summary screen's LoadGraphics: the caller's picture (already faded to black) is let go
    SetVBlankHBlankCallbacksToNull();
    ScanlineEffect_Stop();
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();
    SummaryScreen_SetMoveSlotForUi(MAX_MON_MOVES);
    UiCore_OpenHosted(&gUiForgetMoveScreen, NULL);
    return true;
}

// A move is being learned (else a move is only forgotten: the Move Deleter)
static bool Learning(void)
{
    return sNewMove != MOVE_NONE;
}

// The rows the cursor can be on: the known moves, and the new one when learning
static bool Selectable(int32_t i)
{
    return i == MAX_MON_MOVES ? Learning() : sMoves[i].move != MOVE_NONE;
}

static void Enter(void)
{
    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
        Fill(&sMoves[i], (uint16_t)GetMonData(sMon, MON_DATA_MOVE1 + i), i);
    Fill(&sMoves[MAX_MON_MOVES], sNewMove, -1);
    sSel = 0;
    while (sSel < MAX_MON_MOVES && sMoves[sSel].move == MOVE_NONE)
        sSel++;
}

static void Choose(int32_t slot)
{
    PlaySE(SE_SELECT);
    SummaryScreen_SetMoveSlotForUi((u8)slot);
    UiNav_Close(Finish);
}

// Up / down round the list, skipping empty move slots (ChangeSelectedMove)
static void Step(int32_t dir)
{
    int32_t s = sSel;
    for (int32_t i = 0; i <= MAX_MON_MOVES; i++)
    {
        s += dir;
        if (s > MAX_MON_MOVES) s = 0;
        else if (s < 0) s = MAX_MON_MOVES;
        if (Selectable(s))
            break;
    }
    if (s != sSel)
    {
        sSel = s;
        PlaySE(SE_SELECT);
    }
}

static int32_t RowY(int32_t i)
{
    return i == MAX_MON_MOVES ? NEW_Y : ROW_Y + i * ROW_H;
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        Choose(MAX_MON_MOVES);
        return true;
    }
    if (in->repeat & VITA_BTN_UP) Step(-1);
    if (in->repeat & VITA_BTN_DOWN) Step(1);
    bool activate = (in->pressed & VITA_BTN_CROSS) != 0;
    for (int32_t i = 0; i <= MAX_MON_MOVES; i++)
        if (Selectable(i) && UiHit(in, ROW_X, RowY(i), ROW_W, ROW_H))
        {
            if (i == sSel) activate = true;
            else { sSel = i; PlaySE(SE_SELECT); }
        }
    if (!activate)
        return true;
    // CanReplaceMove: an HM can't be forgotten for a new move (with none, it can)
    if (sSel < MAX_MON_MOVES && sNewMove != MOVE_NONE && IsMoveHm(sMoves[sSel].move))
    {
        char msg[64];
        PlaySE(SE_FAILURE);
        UiGame_Text(gText_HMMovesCantBeForgotten2, msg, sizeof(msg));
        UiKit_Toast("%s", msg);
        return true;
    }
    Choose(sSel);
    return true;
}

// One move row (shared by FORGET and the MOVE RELEARNER): type, name, HM chip, PP
static void DrawMoveRow(const ForgetMove *m, int32_t y, bool on, int32_t i, bool known)
{
    const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
    char buf[16];
    UiW_Row(ROW_X, y, ROW_W, ROW_H, on, i);
    if (m->move == MOVE_NONE)
    {
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, ROW_X + 104, y + 4, "-", UI_COL_INK_SOFT);
        return;
    }
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + m->type), ROW_X + 28, y + 6, 2);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, ROW_X + 104, y + 4, m->name, ink);
    if (known && IsMoveHm(m->move))
        UiW_Chip(ROW_X + ROW_W - 200, y + 7, "HM", false);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)m->pp, (unsigned)m->maxPp);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, ROW_X + ROW_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, ink);
}

static void DrawRow(int32_t i)
{
    DrawMoveRow(&sMoves[i], RowY(i), i == sSel, i, i < MAX_MON_MOVES);
}

static void DrawCompare(int32_t x, const char *tag, uint32_t tagCol, const ForgetMove *m)
{
    char buf[32];
    UiW_Inset(x, 356, 300, 136);
    UiText_Draw(UI_FACE_PS2P, 16, x + 14, 368, tag, tagCol);
    if (m->move == MOVE_NONE)
        return;
    char pw[8], ac[8];
    if (m->power > 1) snprintf(pw, sizeof(pw), "%u", (unsigned)m->power); else snprintf(pw, sizeof(pw), "-");
    if (m->accuracy) snprintf(ac, sizeof(ac), "%u%%", (unsigned)m->accuracy); else snprintf(ac, sizeof(ac), "-");
    snprintf(buf, sizeof(buf), "%s · %s", pw, ac);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 300 - 14 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), 358, buf, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 14, 390, m->name, UI_COL_INK);
    UiW_TextWrapped(x + 14, 424, 272, 30, m->desc, UI_COL_INK_SOFT, 2);
}

// The POKéMON (shared by FORGET and the MOVE RELEARNER): picture, name, level, types, help box
static void DrawMonPanel(const char *name)
{
    char buf[48];
    const uint16_t species = UiGame_SafeSpecies(GetMonData(sMon, MON_DATA_SPECIES));
    UiW_Panel(16, 60, 300, 272);
    UiW_Inset(30, 74, 272, 176);
    UiSpr_MonFront(species, IsMonShiny(sMon), 30 + (272 - 128) / 2, 74 + (176 - 128) / 2, 2);
    snprintf(buf, sizeof(buf), "%s Lv%u", name, (unsigned)GetMonData(sMon, MON_DATA_LEVEL));
    UiText_Draw(UI_FACE_PS2P, 16, 30, 262, buf, UI_COL_INK);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gSpeciesInfo[species].types[0]), 30, 290, 2);
    if (gSpeciesInfo[species].types[1] != gSpeciesInfo[species].types[0])
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gSpeciesInfo[species].types[1]), 102, 290, 2);
    UiW_Inset(16, 344, 300, 148);
}

static void Build(void)
{
    char buf[96], name[24];
    UiKit_Header(Learning() ? "LEARN A MOVE" : "FORGET A MOVE", NULL, 0, 0, NULL, NULL, 0);
    {
        u8 nick[POKEMON_NAME_LENGTH + 1];
        GetMonData(sMon, MON_DATA_NICKNAME, nick);
        UiGame_Text(nick, name, sizeof(name));
    }
    if (Learning())
        snprintf(buf, sizeof(buf), "%s wants to learn %s", name, sMoves[MAX_MON_MOVES].name);
    else
        snprintf(buf, sizeof(buf), "%s", name);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    DrawMonPanel(name);
    UiW_TextWrapped(30, 352, 272, 30, Learning() ? "Choose a move to forget, or the new one to stop learning it."
                                                 : "Choose the move to forget.", UI_COL_INK, 4);

    // The moves and the new one
    UiW_Panel(332, 60, 612, 284);
    UiW_PanelHead(332, 60, 612, "MOVES", "PP");
    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
        DrawRow(i);
    if (Learning())
    {
        UiText_Draw(UI_FACE_PS2P, 16, ROW_X + 12, NEW_Y - 24, "NEW MOVE", UI_RGBA(180, 83, 9, 255));
        DrawRow(MAX_MON_MOVES);
    }

    // The chosen move against the new one
    if (sSel < MAX_MON_MOVES)
        DrawCompare(332, "FORGET", UI_RGBA(181, 53, 43, 255), &sMoves[sSel]);
    else
    {
        UiW_Inset(332, 356, 300, 136);
        UiText_Draw(UI_FACE_PS2P, 16, 346, 368, "DON'T LEARN", UI_COL_INK_SOFT);
        UiW_TextWrapped(346, 396, 272, 30, "It keeps the four moves it knows.", UI_COL_INK, 3);
    }
    if (Learning())
        DrawCompare(644, "LEARN", UI_RGBA(47, 158, 88, 255), &sMoves[MAX_MON_MOVES]);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sSel < MAX_MON_MOVES ? "Forget" : "Don't learn");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, Learning() ? "Don't learn" : "Cancel");
    UiKit_FooterClock();
}

const UiScreen gUiForgetMoveScreen = { Enter, Update, Build, NULL };

// =====================================================================================
// MOVE RELEARNER (move_relearner.c), hosted: the moves the POKéMON in VAR_0x8004 can remember,
// laid out as FORGET above. A move: "Teach X?" (YES / NO); with four moves already, "Delete an
// older move?" opens FORGET (the summary screen's hook) and comes back here. Circle or CANCEL:
// "Give up?". The game's texts; every step goes through move_relearner.c's MoveRelearner_*ForUi.

enum { RL_LIST, RL_ASK_TEACH, RL_ASK_DELETE, RL_ASK_STOP, RL_ASK_GIVE_UP, RL_RESULT };
#define RL_ROWS 5
static uint16_t sRlMoves[MAX_RELEARNER_MOVES];
static int32_t sRlCount, sRlState;
static UiList sRlList;
static UiMenu sRlAsk;
static ForgetMove sRlInfo;
static char sRlText[160];                    // the game's message for the current step
static bool sRlBack;                         // reopened after FORGET
static uint8_t sRlSlot;                      // FORGET's choice (MAX_MON_MOVES: none)

static void RlExpand(const u8 *text, uint16_t move, uint16_t forgotten)
{
    GetMonData(sMon, MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    StringCopy(gStringVar2, gMoveNames[UiGame_SafeMove(move)]);
    StringCopy(gStringVar3, gMoveNames[UiGame_SafeMove(forgotten)]);
    StringExpandPlaceholders(gStringVar4, text);
    UiGame_Text(gStringVar4, sRlText, sizeof(sRlText));
    for (char *c = sRlText; *c; c++)
        if (*c == '\n') *c = ' ';
}

static uint16_t RlMove(void)
{
    return sRlList.sel < sRlCount ? sRlMoves[sRlList.sel] : MOVE_NONE;
}

static void RlAsk(int32_t state, const char *title)
{
    sRlState = state;
    UiMenu_Open(&sRlAsk, title, 676, 150, 252);
    UiMenu_Add(&sRlAsk, "YES", 1);
    UiMenu_Add(&sRlAsk, "NO", 0);
}

static void RlExit(void)
{
    MoveRelearner_ExitForUi();
}

static void CB2_RelearnerAfterForget(void);
static void RlStartForget(void)
{
    ShowSelectMovePokemonSummaryScreen(gPlayerParty, (u8)gSpecialVar_0x8004, gPlayerPartyCount - 1,
                                       CB2_RelearnerAfterForget, RlMove());
}

static void RlEnter(void)
{
    sMon = &gPlayerParty[gSpecialVar_0x8004];
    sRlCount = MoveRelearner_MovesForUi(sRlMoves);
    sRlAsk.open = false;
    if (!sRlBack)
        UiList_Reset(&sRlList, sRlCount + 1, RL_ROWS);           // + CANCEL
    sRlState = RL_LIST;
    RlExpand(gText_TeachWhichMoveToPkmn, MOVE_NONE, MOVE_NONE);
    if (sRlBack)
    {
        sRlBack = false;
        const uint16_t move = RlMove();
        if (sRlSlot < MAX_MON_MOVES)
        {
            const uint16_t old = (uint16_t)GetMonData(sMon, MON_DATA_MOVE1 + sRlSlot);
            MoveRelearner_ReplaceForUi(sRlSlot, move);
            RlExpand(gText_MoveRelearnerPkmnForgotMoveAndLearnedNew, move, old);
            PlayFanfare(MUS_LEVEL_UP);
            sRlState = RL_RESULT;
        }
        else
        {
            // FORGET left with the new move or circle: "Stop trying to teach X?" (MENU_STATE_PRINT_STOP_TEACHING)
            RlExpand(gText_MoveRelearnerStopTryingToTeachMove, move, MOVE_NONE);
            RlAsk(RL_ASK_STOP, "STOP TEACHING?");
        }
    }
}

// The result waits for the level-up fanfare, as MENU_STATE_WAIT_FOR_FANFARE does before A
static bool RlResultReady(void)
{
    return IsFanfareTaskInactive();
}

static bool RlUpdate(const UiInput *in)
{
    if (sRlState == RL_RESULT)
    {
        if (RlResultReady() && ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap))
        {
            PlaySE(SE_SELECT);
            UiNav_Close(RlExit);
        }
        return true;
    }
    if (sRlAsk.open)
    {
        const int32_t id = UiMenu_Update(&sRlAsk, in);
        if (id == UI_MENU_NONE)
            return true;
        const bool yes = id == 1;
        const uint16_t move = RlMove();
        if (sRlState == RL_ASK_TEACH && yes)
        {
            if (MoveRelearner_TeachForUi(move))
            {
                RlExpand(gText_MoveRelearnerPkmnLearnedMove, move, MOVE_NONE);
                PlayFanfare(MUS_LEVEL_UP);
                sRlState = RL_RESULT;
                return true;
            }
            RlExpand(gText_MoveRelearnerPkmnTryingToLearnMove, move, MOVE_NONE);
            RlAsk(RL_ASK_DELETE, "DELETE A MOVE?");
            return true;
        }
        if (sRlState == RL_ASK_DELETE && yes)
        {
            sRlBack = true;
            sRlSlot = MAX_MON_MOVES;
            UiNav_Close(RlStartForget);
            return true;
        }
        // "Delete an older move?" NO, or "Stop teaching?" NO: the other question, as the GBA scene
        if (sRlState == RL_ASK_DELETE && !yes)
        {
            RlExpand(gText_MoveRelearnerStopTryingToTeachMove, move, MOVE_NONE);
            RlAsk(RL_ASK_STOP, "STOP TEACHING?");
            return true;
        }
        if (sRlState == RL_ASK_STOP && !yes)
        {
            RlExpand(gText_MoveRelearnerPkmnTryingToLearnMove, move, MOVE_NONE);
            RlAsk(RL_ASK_DELETE, "DELETE A MOVE?");
            return true;
        }
        if (sRlState == RL_ASK_GIVE_UP && yes)
        {
            MoveRelearner_GiveUpForUi();
            UiNav_Close(RlExit);
            return true;
        }
        sRlState = RL_LIST;
        RlExpand(gText_TeachWhichMoveToPkmn, MOVE_NONE, MOVE_NONE);
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        RlExpand(gText_MoveRelearnerGiveUp, MOVE_NONE, MOVE_NONE);
        RlAsk(RL_ASK_GIVE_UP, "GIVE UP?");
        return true;
    }
    const int32_t before = sRlList.sel;
    const int32_t a = UiList_Update(&sRlList, in, ROW_X, ROW_Y, ROW_W, ROW_H, false);
    if (sRlList.sel != before)
        PlaySE(SE_SELECT);
    if (a < 0)
        return true;
    PlaySE(SE_SELECT);
    if (a >= sRlCount)
    {
        RlExpand(gText_MoveRelearnerGiveUp, MOVE_NONE, MOVE_NONE);
        RlAsk(RL_ASK_GIVE_UP, "GIVE UP?");
        return true;
    }
    char title[24], mv[16];
    UiGame_Text(gMoveNames[UiGame_SafeMove(RlMove())], mv, sizeof(mv));
    snprintf(title, sizeof(title), "TEACH %s?", mv);
    RlExpand(gText_MoveRelearnerTeachMoveConfirm, RlMove(), MOVE_NONE);
    RlAsk(RL_ASK_TEACH, title);
    return true;
}

// The move's contest side (the GBA screen's CONTEST MOVES page): category, appeal and jam hearts
static void RlDrawContest(int32_t x, uint16_t move)
{
    static const char *const cats[5] = { "COOL", "BEAUTY", "CUTE", "SMART", "TOUGH" };
    UiW_Inset(x, 356, 300, 136);
    UiText_Draw(UI_FACE_PS2P, 16, x + 14, 368, "CONTEST", UI_RGBA(47, 158, 88, 255));
    if (move == MOVE_NONE)
        return;
    const struct ContestMove *cm = &gContestMoves[move];
    const struct ContestEffect *ce = &gContestEffects[cm->effect];
    const char *cat = cm->contestCategory < 5 ? cats[cm->contestCategory] : "";
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 300 - 14 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, cat), 358, cat, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 14, 392, "APPEAL", UI_COL_INK_SOFT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 14, 432, "JAM", UI_COL_INK_SOFT);
    for (int32_t i = 0; i < 8; i++)
    {
        UiDraw_Rect(x + 120 + i * 20, 404, 14, 14, i < ce->appeal / 10 ? UI_COL_HP_LOW : gUiTheme->paperShade);
        UiDraw_Rect(x + 120 + i * 20, 444, 14, 14, i < ce->jam / 10 ? UI_COL_INK : gUiTheme->paperShade);
    }
}

static void RlBuild(void)
{
    char name[24];
    {
        u8 nick[POKEMON_NAME_LENGTH + 1];
        GetMonData(sMon, MON_DATA_NICKNAME, nick);
        UiGame_Text(nick, name, sizeof(name));
    }
    UiKit_Header("MOVE RELEARNER", NULL, 0, 0, NULL, NULL, 0);
    UiW_HeaderValue(name, UI_COL_ON_FRAME);
    DrawMonPanel(name);
    UiW_TextWrapped(30, 352, 272, 30, sRlText, UI_COL_INK, 4);

    UiW_Panel(332, 60, 612, 284);
    UiW_PanelHead(332, 60, 612, "MOVES", "PP");
    UiDraw_PushClip(ROW_X, ROW_Y, ROW_W, RL_ROWS * ROW_H);
    for (int32_t r = 0; r < RL_ROWS && sRlList.first + r < sRlCount + 1; r++)
    {
        const int32_t i = sRlList.first + r, y = ROW_Y + r * ROW_H;
        const bool on = i == sRlList.sel;
        if (i == sRlCount)
        {
            UiW_Row(ROW_X, y, ROW_W, ROW_H, on, i);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, ROW_X + 104, y + 4, "CANCEL", on ? UI_COL_SELECT_INK : UI_COL_INK_SOFT);
            continue;
        }
        ForgetMove m;
        Fill(&m, sRlMoves[i], -1);
        DrawMoveRow(&m, y, on, i, false);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(ROW_X + ROW_W - 9, ROW_Y, RL_ROWS * ROW_H, sRlList.first, RL_ROWS, sRlCount + 1);

    Fill(&sRlInfo, RlMove(), -1);
    DrawCompare(332, "BATTLE", UI_RGBA(181, 53, 43, 255), &sRlInfo);
    RlDrawContest(644, RlMove());

    int32_t fx = 16;
    if (sRlState != RL_RESULT)
    {
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Teach");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Give up");
    }
    else if (RlResultReady())
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "OK");      // only once the fanfare is over
    UiKit_FooterClock();
    if (sRlAsk.open)
        UiMenu_Draw(&sRlAsk);
    if (sRlState == RL_RESULT)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(200, 200, 560, 144, "MOVE RELEARNER");
        UiW_TextWrapped(224, 250, 512, 30, sRlText, UI_COL_INK, 3);
    }
}

static const UiScreen sRelearnerScreen = { RlEnter, RlUpdate, RlBuild, NULL };

bool Ui_OpenMoveRelearner(void)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || gSpecialVar_0x8004 >= PARTY_SIZE)
        return false;
    sRlBack = false;
    UiCore_OpenHosted(&sRelearnerScreen, NULL);
    return true;
}

// Back from FORGET (the summary screen's callback): its choice, then this screen again
static void CB2_RelearnerAfterForget(void)
{
    sRlSlot = GetMoveSlotToReplace();
    UiCore_OpenHosted(&sRelearnerScreen, NULL);
}
