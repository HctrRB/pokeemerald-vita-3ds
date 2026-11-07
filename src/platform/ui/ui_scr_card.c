// TRAINER CARD of the new UI (design/stitch/ficha): CARD (trainer, money, Pokédex, time, badges)
// and PASSPORT (records, Battle Frontier symbols). Only data the game has.

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "overworld.h"
#include "sound.h"
#include "constants/flags.h"
#include "constants/game_stat.h"
#include "constants/songs.h"
#include "constants/trainers.h"

static const char *const sTabs[2] = { "CARD", "PASSPORT" };
static const char *const sBadgeNames[8] = { "STONE", "KNUCKLE", "DYNAMO", "HEAT", "BALANCE", "FEATHER", "MIND", "RAIN" };
static int32_t sTab, sBadge, sTabX[2], sTabW[2];

static void Enter(void)
{
    sTab = 0;
    sBadge = 0;
}

static bool Update(const UiInput *in)
{
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, 2);
    if (in->pressed & (VITA_BTN_L | VITA_BTN_R | VITA_BTN_CROSS)) t = sTab ^ 1;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); }
    const int32_t before = sBadge;
    if (in->repeat & VITA_BTN_LEFT) sBadge = (sBadge + 7) % 8;
    if (in->repeat & VITA_BTN_RIGHT) sBadge = (sBadge + 1) % 8;
    for (int32_t i = 0; i < 8; i++)
        if (UiHit(in, 492 + (i % 4) * 110, 116 + (i / 4) * 92, 104, 86)) sBadge = i;
    if (sBadge != before) PlaySE(SE_SELECT);
    return true;
}

static void Row(int32_t x, int32_t y, int32_t w, const char *k, const char *v, uint32_t vcol)
{
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x, y, k, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - UiText_Width(UI_FACE_VT323, UI_BODY_PX, v), y, v, vcol);
    UiDraw_Rect(x, y + 32, w, 2, UI_COL_PAPER2);
}

static void Build(void)
{
    char buf[48], name[16];
    UiKit_Header("TRAINER CARD", sTabs, 2, sTab, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), "IDNo. %05u", (unsigned)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8)));
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    UiGame_PlayerName(name, sizeof(name));
    if (sTab == 0)
    {
        UiW_Panel(16, 60, 440, 432);
        UiW_Inset(32, 76, 152, 152);
        UiSpr_Trainer(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 44, 88, 2);
        UiText_Draw(UI_FACE_PS2P, 16, 200, 84, name, UI_COL_INK);
        snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
        Row(200, 112, 240, "MONEY", buf, UI_COL_MONEY);
        snprintf(buf, sizeof(buf), "%u", (unsigned)UiGame_DexOwned());
        Row(200, 150, 240, "POKéDEX", buf, UI_COL_INK);
        UiGame_PlayTime(buf, sizeof(buf));
        Row(200, 188, 240, "TIME", buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), "%u BP", (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        Row(32, 250, 408, "BATTLE POINTS", buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(GAME_STAT_ENTERED_HOF));
        Row(32, 288, 408, "HALL OF FAME", FlagGet(FLAG_SYS_GAME_CLEAR) ? buf : "-", UI_COL_INK);

        UiW_Panel(468, 60, 476, 432);
        snprintf(buf, sizeof(buf), "%u / 8", (unsigned)UiGame_BadgeCount());
        UiW_PanelHead(468, 60, 476, "BADGES", buf);
        for (int32_t i = 0; i < 8; i++)
        {
            const int32_t x = 492 + (i % 4) * 110, y = 116 + (i / 4) * 92;
            const bool got = FlagGet(FLAG_BADGE01_GET + i);
            UiW_Inset(x, y, 104, 86);
            if (i == sBadge)
                UiDraw_Frame(x, y, 104, 86, 3, UI_COL_SELECT);
            // Unearned badges: a faded silhouette
            UiDraw_SpriteTint((uint16_t)(UI_SPR_BADGE_FIRST + i), x + 36, y + 8, 2, got ? 0xFFFFFFFFu : UI_RGBA(200, 192, 170, 120));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 52 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, got ? sBadgeNames[i] : "-") / 2, y + 46, got ? sBadgeNames[i] : "-", UI_COL_INK);
        }
        if (FlagGet(FLAG_BADGE01_GET + sBadge))
            snprintf(buf, sizeof(buf), "%s BADGE", sBadgeNames[sBadge]);
        else
            snprintf(buf, sizeof(buf), "Not obtained yet.");
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 492, 316, buf, UI_COL_INK);
    }
    else
    {
        UiW_Panel(16, 60, 440, 432);
        UiW_PanelHead(16, 60, 440, "TRAINER RECORDS", NULL);
        static const uint8_t stats[5] = { GAME_STAT_ENTERED_HOF, GAME_STAT_POKEMON_CAPTURES, GAME_STAT_POKEMON_TRADES, GAME_STAT_POKEBLOCKS_WITH_FRIENDS, GAME_STAT_WON_CONTEST };
        static const char *const labels[5] = { "HALL OF FAME", "POKéMON CAUGHT", "TRADES", "POKéBLOCKS W/ FRIENDS", "CONTESTS WON" };
        for (int32_t i = 0; i < 5; i++)
        {
            snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(stats[i]));
            Row(32, 110 + i * 40, 408, labels[i], buf, UI_COL_INK);
        }
        snprintf(buf, sizeof(buf), "%u", (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        Row(32, 310, 408, "BATTLE FRONTIER BP", buf, UI_COL_INK);

        UiW_Panel(468, 60, 476, 432);
        UiW_PanelHead(468, 60, 476, "FRONTIER SYMBOLS", NULL);
        // Silver and gold symbol of the 7 facilities (FLAG_SYS_TOWER_SILVER .. FLAG_SYS_PYRAMID_GOLD)
        for (int32_t f = 0; f < 7; f++)
        {
            const int32_t x = 492 + (f % 4) * 110, y = 116 + (f / 4) * 92;
            const bool silver = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2), gold = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1);
            UiW_Inset(x, y, 104, 86);
            UiDraw_SpriteTint((uint16_t)((gold ? UI_SPR_FRONTIER_GOLD_1 : UI_SPR_FRONTIER_SILVER_1) + f), x + 36, y + 8, 2,
                              (silver || gold) ? 0xFFFFFFFFu : UI_RGBA(200, 192, 170, 120));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 40, y + 46, gold ? "GOLD" : silver ? "SILVER" : "-", UI_COL_INK);
        }
    }
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Flip card");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiKit_FooterClock();
}

const UiScreen gUiCardScreen = { Enter, Update, Build, NULL };
