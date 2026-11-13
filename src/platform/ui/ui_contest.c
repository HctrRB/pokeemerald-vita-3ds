// POKéMON CONTEST appeal choice of the new UI (design/stitch/concurso). Like the battle HUD it is an
// overlay over the game's own contest scene, shown only while contest.c waits in
// Task_HandleMoveSelectInput: the stage, the entrants' boxes and the applause meter above stay the
// game's. The game keeps reading the buttons (d-pad moves its cursor, ✕ appeals, ○ goes back to
// the "which move" prompt); a tap selects a move and a second tap on it appeals (A injected).
// Core 1, from Ui_Tick when no UI screen is open.

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"

#include "global.h"
#include "contest.h"
#include "contest_effect.h"
#include "main.h"
#include "constants/moves.h"

extern bool8 Contest_UiChoosingMove(void);
extern u8 Contest_UiChoice(void);
extern void Contest_UiSetChoice(u8 i);
extern u16 Contest_UiMove(u8 i);
extern u8 Contest_UiMoveTag(u8 i);
extern u8 Contest_UiAppealNumber(void);
extern void UiMirror_Inject(uint16_t keys);

#define PANEL_Y 340
#define MOVE_W 296
#define MOVE_H 82

static int32_t Hearts(uint8_t v)
{
    if (v == 0xFF)
        return 0;
    const int32_t n = v / 10;
    return n > 8 ? 8 : n;
}

static void DrawHearts(int32_t x, int32_t y, int32_t n, int32_t max, uint32_t on)
{
    for (int32_t i = 0; i < max; i++)
    {
        UiDraw_Rect(x + i * 14, y, 12, 12, i < n ? on : gUiTheme->paper2);
        UiDraw_Frame(x + i * 14, y, 12, 12, 1, gUiTheme->paperShade);
    }
}

bool UiContest_Draw(void)
{
    if (!Ui_IsEnabled() || !Contest_UiChoosingMove())
        return false;
    const uint8_t cur = Contest_UiChoice();
    int32_t tx = 0, ty = 0;
    const bool tap = UiMirror_Tap(&tx, &ty);
    char buf[64];

    // Moves (2 x 2)
    UiW_Panel(16, PANEL_Y, 616, 188);
    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
    {
        const u16 move = Contest_UiMove((u8)i);
        const int32_t x = 28 + (i % 2) * (MOVE_W + 8), y = PANEL_Y + 12 + (i / 2) * (MOVE_H + 8);
        if (move == MOVE_NONE)
        {
            UiW_Inset(x, y, MOVE_W, MOVE_H);
            continue;
        }
        const bool on = i == cur;
        if (tap && tx >= x && tx < x + MOVE_W && ty >= y && ty < y + MOVE_H)
        {
            if (on) UiMirror_Inject(A_BUTTON);
            else Contest_UiSetChoice((u8)i);
        }
        UiW_Row(x, y, MOVE_W, MOVE_H, on, i);
        UiDraw_Sprite((uint16_t)(UI_SPR_CONTEST_FIRST + gContestMoves[move].contestCategory), x + 30, y + 10, 2);
        UiGame_MoveName(move, buf, sizeof(buf));
        const uint8_t tag = Contest_UiMoveTag((u8)i);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 106, y + 4, buf, tag == 2 ? UI_COL_INK_SOFT : on ? UI_COL_SELECT_INK : UI_COL_INK);
        DrawHearts(x + 106, y + 46, Hearts(gContestEffects[gContestMoves[move].effect].appeal), 8, UI_COL_HP_LOW);
        if (tag)
            UiW_Chip(x + MOVE_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, tag == 1 ? "COMBO" : "REPEAT") - 16, y + 40,
                     tag == 1 ? "COMBO" : "REPEAT", tag == 1);
    }

    // The move under the cursor: appeal and jam hearts, effect
    const u16 move = Contest_UiMove(cur);
    UiW_Panel(644, PANEL_Y, 300, 188);
    snprintf(buf, sizeof(buf), "APPEAL %u", (unsigned)Contest_UiAppealNumber() + 1);
    UiText_Draw(UI_FACE_PS2P, 16, 660, PANEL_Y + 14, buf, UI_COL_INK);
    UiText_Draw(UI_FACE_PS2P, 16, 660, PANEL_Y + 42, "AP", UI_COL_INK_SOFT);
    DrawHearts(704, PANEL_Y + 44, Hearts(gContestEffects[gContestMoves[move].effect].appeal), 8, UI_COL_HP_LOW);
    UiText_Draw(UI_FACE_PS2P, 16, 660, PANEL_Y + 64, "JAM", UI_COL_INK_SOFT);
    DrawHearts(720, PANEL_Y + 66, Hearts(gContestEffects[gContestMoves[move].effect].jam), 8, gUiTheme->ink);
    char desc[160];
    UiGame_Text(gContestEffectDescriptionPointers[gContestMoves[move].effect], desc, sizeof(desc));
    UiW_TextWrapped(660, PANEL_Y + 92, 272, 28, desc, UI_COL_INK, 3);
    return true;
}
