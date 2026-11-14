// "Which move should be forgotten?" of the new UI (design/stitch/olvidar_movimiento), replacing the
// summary screen's SELECT_MOVE mode (ShowSelectMovePokemonSummaryScreen): a level up in battle or
// outside it, an evolution, the Move Relearner. Hosted, as that screen is a scene of its own: it
// starts and ends as the summary screen does (sprites and palettes freed, gLastViewedMonIndex) and
// leaves its choice where the callers read it (GetMoveSlotToReplace, VAR_0x8005): the slot of the
// move to forget, or MAX_MON_MOVES (the new move chosen, or circle) to stop learning it. HM moves
// can't be chosen (the game's "HM moves can't be forgotten now."). The cursor goes round the
// known moves and the new one, skipping empty slots, as ChangeSelectedMove.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "battle_main.h"
#include "data.h"
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
        if (s == MAX_MON_MOVES || sMoves[s].move != MOVE_NONE)
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
        if ((i == MAX_MON_MOVES || sMoves[i].move != MOVE_NONE) && UiHit(in, ROW_X, RowY(i), ROW_W, ROW_H))
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

static void DrawRow(int32_t i)
{
    const ForgetMove *m = &sMoves[i];
    const int32_t y = RowY(i);
    const bool on = i == sSel;
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
    if (i < MAX_MON_MOVES && IsMoveHm(m->move))
        UiW_Chip(ROW_X + ROW_W - 200, y + 7, "HM", false);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)m->pp, (unsigned)m->maxPp);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, ROW_X + ROW_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, ink);
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

static void Build(void)
{
    char buf[96], name[24];
    UiKit_Header("LEARN A MOVE", NULL, 0, 0, NULL, NULL, 0);
    {
        u8 nick[POKEMON_NAME_LENGTH + 1];
        GetMonData(sMon, MON_DATA_NICKNAME, nick);
        UiGame_Text(nick, name, sizeof(name));
    }
    snprintf(buf, sizeof(buf), "%s wants to learn %s", name, sMoves[MAX_MON_MOVES].name);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    // The POKéMON
    const uint16_t species = (uint16_t)GetMonData(sMon, MON_DATA_SPECIES);
    UiW_Panel(16, 60, 300, 272);
    UiW_Inset(30, 74, 272, 176);
    UiSpr_MonFront(species, IsMonShiny(sMon), 30 + (272 - 128) / 2, 74 + (176 - 128) / 2, 2);
    snprintf(buf, sizeof(buf), "%s Lv%u", name, (unsigned)GetMonData(sMon, MON_DATA_LEVEL));
    UiText_Draw(UI_FACE_PS2P, 16, 30, 262, buf, UI_COL_INK);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gSpeciesInfo[species].types[0]), 30, 290, 2);
    if (gSpeciesInfo[species].types[1] != gSpeciesInfo[species].types[0])
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gSpeciesInfo[species].types[1]), 102, 290, 2);
    UiW_Inset(16, 344, 300, 148);
    UiW_TextWrapped(30, 352, 272, 30, "Choose a move to forget, or the new one to stop learning it.", UI_COL_INK, 4);

    // The moves and the new one
    UiW_Panel(332, 60, 612, 284);
    UiW_PanelHead(332, 60, 612, "MOVES", "PP");
    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
        DrawRow(i);
    UiText_Draw(UI_FACE_PS2P, 16, ROW_X + 12, NEW_Y - 24, "NEW MOVE", UI_RGBA(180, 83, 9, 255));
    DrawRow(MAX_MON_MOVES);

    // The chosen move against the new one
    if (sSel < MAX_MON_MOVES)
        DrawCompare(332, "FORGET", UI_RGBA(181, 53, 43, 255), &sMoves[sSel]);
    else
    {
        UiW_Inset(332, 356, 300, 136);
        UiText_Draw(UI_FACE_PS2P, 16, 346, 368, "DON'T LEARN", UI_COL_INK_SOFT);
        UiW_TextWrapped(346, 396, 272, 30, "It keeps the four moves it knows.", UI_COL_INK, 3);
    }
    DrawCompare(644, "LEARN", UI_RGBA(47, 158, 88, 255), &sMoves[MAX_MON_MOVES]);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sSel < MAX_MON_MOVES ? "Forget" : "Don't learn");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Don't learn");
    UiKit_FooterClock();
}

const UiScreen gUiForgetMoveScreen = { Enter, Update, Build, NULL };
