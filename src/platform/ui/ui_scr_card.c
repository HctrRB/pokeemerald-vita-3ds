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
static const char *const sLeaderNames[8] = { "ROXANNE", "BRAWLY", "WATTSON", "FLANNERY", "NORMAN", "WINONA", "TATE/LIZA", "WALLACE" };
static const char *const sGymTowns[8] = { "RUSTBORO", "DEWFORD", "MAUVILLE", "LAVARIDGE", "PETALBURG", "FORTREE", "MOSSDEEP", "SOOTOPOLIS" };
static int32_t sTab, sBadge, sTabX[2], sTabW[2];

static const char *const sBadgeDescs[8] = {
    "Boosts ATTACK and enables CUT outside battle.",
    "POKéMON up to Lv.30 obey. Enables FLASH.",
    "Boosts SPEED and enables ROCK SMASH.",
    "POKéMON up to Lv.50 obey. Enables STRENGTH.",
    "Boosts DEFENSE and enables SURF outside battle.",
    "POKéMON up to Lv.70 obey. Enables FLY.",
    "Boosts SP.ATK & SP.DEF. Enables DIVE.",
    "All POKéMON obey! Enables WATERFALL.",
};

static void Enter(void)
{
    sTab = 0;
    sBadge = 0;
}

static bool Update(const UiInput *in)
{
    const bool gba = gUiTheme->gbaFrame;
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = -1;
    if (gba)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, 2);
        if (ph != 0)
            t = sTab ^ 1;
    }
    else
        t = UiKit_TabHit(in, sTabX, sTabW, 2);
    if (in->pressed & (VITA_BTN_L | VITA_BTN_R | VITA_BTN_CROSS)) t = sTab ^ 1;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); }
    const int32_t before = sBadge;
    if (in->repeat & VITA_BTN_LEFT) sBadge = (sBadge + 7) % 8;
    if (in->repeat & VITA_BTN_RIGHT) sBadge = (sBadge + 1) % 8;
    if (sTab == 0)
    {
        if (gba)
        {
            for (int32_t i = 0; i < 8; i++)
                if (UiHit(in, 614 + (i & 1) * 162, 108 + (i >> 1) * 69, 154, 62)) sBadge = i;
        }
        else
        {
            for (int32_t i = 0; i < 8; i++)
                if (UiHit(in, 492 + (i % 4) * 110, 116 + (i / 4) * 92, 104, 86)) sBadge = i;
        }
    }
    if (sBadge != before) PlaySE(SE_SELECT);
    return true;
}

static void Row(int32_t x, int32_t y, int32_t w, const char *k, const char *v, uint32_t vcol)
{
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x, y, k, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - UiText_Width(UI_FACE_VT323, UI_BODY_PX, v), y, v, vcol);
    UiDraw_Rect(x, y + 32, w, 2, UI_COL_PAPER2);
}

static void CardRowGba(int32_t x, int32_t y, int32_t w, const char *k, const char *v, uint32_t vcol)
{
    UiW_GbaBox(x, y, w, 46, 3, UI_RGBA(200, 180, 104, 255), UI_RGBA(255, 253, 245, 255), UI_RGBA(255, 253, 245, 255), C_WHITE, 0);
    UiDraw_PushClip(x + 4, y + 2, w - 8, 42);
    UiW_GbaTextMid(UI_FACE_VT323, 24, x + 12, y, 46, k, UI_RGBA(110, 88, 32, 255), UI_GBA_SHAD_LIGHT);
    UiW_GbaTextMid(UI_FACE_VT323, 28, x + w - 12 - UiText_Width(UI_FACE_VT323, 28, v), y, 46, v, vcol, UI_GBA_SHAD_LIGHT);
    UiDraw_PopClip();
}

static void BuildGba(void)
{
    char buf[96], name[16];
    const uint16_t tid = (uint16_t)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8));
    UiGame_PlayerName(name, sizeof(name));
    UiW_GbaTopBar(UI_GBA_BG_CARD, UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY,
                  "TRAINER CARD", 2, sTab, false, sTab == 0 ? "GOLD CARD" : "PASSPORT");

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "BACK", true);
    if (sTab == 0)
        UiW_GbaHelpBadge(&rx, "<>", "BADGE", false);
    UiW_GbaHelpBadge(&rx, "X", "FLIP CARD", false);

    if (sTab == 0)
    {
        // Left Gold Trainer Card (14, 60, 572, 472)
        UiW_GbaWin(14, 60, 572, 472, UI_GBA_WIN_GOLD);
        // Emerald banner strip (.tcard-banner: 30, 76, 540, 42)
        UiW_GbaBox(30, 76, 540, 42, 4, UI_RGBA(41, 49, 49, 255), UI_RGBA(45, 138, 107, 255), UI_RGBA(23, 94, 71, 255),
                   UI_RGBA(115, 205, 164, 140), 0);
        UiW_GbaTextMid(UI_FACE_PS2P, 16, 42, 76, 42, "HOENN TRAINER CARD", C_WHITE, UI_GBA_SHAD_DARK);

        const int32_t stars = (UiGame_BadgeCount() >= 8 ? 1 : 0) + (FlagGet(FLAG_SYS_GAME_CLEAR) ? 1 : 0)
                            + (UiGame_DexOwned() >= HOENN_DEX_COUNT ? 1 : 0)
                            + (gSaveBlock2Ptr->frontier.battlePoints >= 50 ? 1 : 0);
        for (int32_t s = 0; s < 4; s++)
            UiW_GbaStar(346 + s * 18, 91, s < stars ? UI_RGBA(255, 230, 57, 255) : UI_RGBA(16, 68, 50, 255));

        snprintf(buf, sizeof(buf), UiLocale_Translate("IDNo. %05u"), (unsigned)tid);
        UiW_GbaBox(436, 82, 124, 30, 3, UI_RGBA(16, 24, 24, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(24, 32, 32, 255), 0, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 24, 436, 82, 124, 30, buf, C_WHITE, UI_GBA_SHAD_DARK);

        // 6 info rows on left (30, 128, 376)
        CardRowGba(30, 128, 376, "NAME", name, UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
        CardRowGba(30, 182, 376, "MONEY", buf, UI_RGBA(176, 120, 8, 255));
        snprintf(buf, sizeof(buf), "%u", (unsigned)UiGame_DexOwned());
        CardRowGba(30, 236, 376, "POKéDEX", buf, UI_RGBA(49, 49, 49, 255));
        UiGame_PlayTime(buf, sizeof(buf));
        CardRowGba(30, 290, 376, "PLAY TIME", buf, UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(GAME_STAT_ENTERED_HOF));
        CardRowGba(30, 344, 376, "HALL OF FAME", FlagGet(FLAG_SYS_GAME_CLEAR) ? buf : "-", UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), "%u BP", (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        CardRowGba(30, 398, 376, "BATTLE POINTS", buf, UI_RGBA(23, 94, 71, 255));

        // Portrait grid well (.tcard-photo-well: 418, 128, 152, 316)
        UiW_GbaBox(418, 128, 152, 316, 4, UI_RGBA(150, 132, 72, 255), UI_RGBA(247, 243, 220, 255), UI_RGBA(247, 243, 220, 255), UI_GBA_SHAD_LIGHT, 0);
        for (int32_t g = 16; g < 146; g += 16)
            UiDraw_Rect(421 + g, 131, 1, 310, UI_RGBA(45, 138, 107, 28));
        for (int32_t g = 16; g < 310; g += 16)
            UiDraw_Rect(421, 131 + g, 146, 1, UI_RGBA(45, 138, 107, 28));
        UiSpr_Trainer(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 430, 198, 2);
        UiW_GbaBox(426, 402, 136, 32, 3, UI_RGBA(24, 32, 32, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(40, 56, 64, 255), 0, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 24, 426, 402, 136, 32,
                           FlagGet(FLAG_SYS_GAME_CLEAR) ? "CHAMPION RANK" : "CHALLENGER", UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);

        // Bottom foot bar (.tcard-foot)
        UiW_GbaBox(30, 474, 540, 38, 3, UI_RGBA(184, 164, 88, 255), UI_RGBA(232, 220, 158, 255), UI_RGBA(232, 220, 158, 255), C_WHITE, 0);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 42, 474, 38, "HOENN LEAGUE · EMERALD", UI_RGBA(92, 70, 18, 255), C_WHITE);
        snprintf(buf, sizeof(buf), UiLocale_Translate("BADGES: %u / 8"), (unsigned)UiGame_BadgeCount());
        UiW_GbaTextMid(UI_FACE_VT323, 24, 558 - UiText_Width(UI_FACE_VT323, 24, buf), 474, 38, buf, UI_RGBA(92, 70, 18, 255), C_WHITE);

        // Right Badge Case panel (598, 60, 348, 472) matching renderCard in index.html
        UiW_GbaWin(598, 60, 348, 472, UI_GBA_WIN_DEFAULT);
        snprintf(buf, sizeof(buf), "%u / 8", (unsigned)UiGame_BadgeCount());
        UiW_GbaSectionStrip(614, 74, 316, 26, UI_GBA_TAG_GOLD, "BADGE CASE", buf);

        for (int32_t i = 0; i < 8; i++)
        {
            const int32_t bx = 614 + (i & 1) * 162, by = 108 + (i >> 1) * 69;
            const bool got = FlagGet(FLAG_BADGE01_GET + i), on = (i == sBadge);
            if (on)
                UiW_GbaRingBox(bx, by, 154, 62, 4, UI_RGBA(255, 90, 32, 255), UI_RGBA(230, 148, 65, 255),
                               UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            else
                UiW_GbaBox(bx, by, 154, 62, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
            UiDraw_SpriteTint((uint16_t)(UI_SPR_BADGE_FIRST + i), bx + 10, by + 15, 2,
                              got ? 0xFFFFFFFFu : UI_RGBA(180, 175, 160, 110));
            UiDraw_PushClip(bx + 46, by + 4, 104, 54);
            UiW_GbaText(UI_FACE_VT323, 24, bx + 48, by + 8, got ? sBadgeNames[i] : "LOCKED", UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            UiW_GbaText(UI_FACE_VT323, 20, bx + 48, by + 34, sLeaderNames[i], UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
            UiDraw_PopClip();
        }

        // Bottom .sum-memo badge description box (614, 392, 316, 124)
        const bool selGot = FlagGet(FLAG_BADGE01_GET + sBadge);
        UiW_GbaBox(614, 392, 316, 124, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        snprintf(buf, sizeof(buf), UiLocale_Translate("%s BADGE · %s GYM"), sBadgeNames[sBadge], sGymTowns[sBadge]);
        UiDraw_PushClip(622, 396, 300, 116);
        UiW_GbaText(UI_FACE_VT323, 24, 624, 400, buf, UI_RGBA(23, 94, 71, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), UiLocale_Translate("Leader %s (%s)"), sLeaderNames[sBadge], selGot ? "OBTAINED" : "LOCKED");
        UiW_GbaText(UI_FACE_VT323, 20, 624, 424, buf, selGot ? UI_RGBA(176, 120, 8, 255) : UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaTextWrapped(UI_FACE_VT323, 24, 624, 448, 296, 24, sBadgeDescs[sBadge], UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
        UiDraw_PopClip();
        return;
    }

    // Tab 1: PASSPORT (Gold Card Back 14, 60, 572, 472 + Frontier Symbols 598, 60, 348, 472)
    UiW_GbaWin(14, 60, 572, 472, UI_GBA_WIN_GOLD);
    UiW_GbaBox(30, 76, 540, 42, 4, UI_RGBA(41, 49, 49, 255), UI_RGBA(45, 138, 107, 255), UI_RGBA(23, 94, 71, 255),
               UI_RGBA(115, 205, 164, 140), 0);
    UiW_GbaTextMid(UI_FACE_PS2P, 16, 42, 76, 42, "TRAINER RECORDS · BACK", C_WHITE, UI_GBA_SHAD_DARK);
    snprintf(buf, sizeof(buf), UiLocale_Translate("IDNo. %05u"), (unsigned)tid);
    UiW_GbaBox(436, 82, 124, 30, 3, UI_RGBA(16, 24, 24, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(24, 32, 32, 255), 0, 0);
    UiW_GbaTextCentred(UI_FACE_VT323, 24, 436, 82, 124, 30, buf, C_WHITE, UI_GBA_SHAD_DARK);

    static const uint8_t stats[5] = { GAME_STAT_ENTERED_HOF, GAME_STAT_POKEMON_CAPTURES, GAME_STAT_POKEMON_TRADES, GAME_STAT_POKEBLOCKS_WITH_FRIENDS, GAME_STAT_WON_CONTEST };
    static const char *const labels[5] = { "HALL OF FAME DEBUTS", "POKéMON CAUGHT", "LINK TRADES", "POKéBLOCKS W/ FRIENDS", "CONTESTS WON" };
    for (int32_t i = 0; i < 5; i++)
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(stats[i]));
        CardRowGba(30, 128 + i * 54, 540, labels[i], buf, UI_RGBA(49, 49, 49, 255));
    }
    snprintf(buf, sizeof(buf), "%u BP", (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
    CardRowGba(30, 398, 540, "BATTLE FRONTIER POINTS", buf, UI_RGBA(23, 94, 71, 255));

    UiW_GbaBox(30, 474, 540, 38, 3, UI_RGBA(184, 164, 88, 255), UI_RGBA(232, 220, 158, 255), UI_RGBA(232, 220, 158, 255), C_WHITE, 0);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 42, 474, 38, "BATTLE FRONTIER PASSPORT", UI_RGBA(92, 70, 18, 255), C_WHITE);

    // Right Frontier Passport panel (598, 60, 348, 472) with 7 horizontal .fpass-row rows
    UiW_GbaWin(598, 60, 348, 472, UI_GBA_WIN_DEFAULT);
    int32_t symCount = 0;
    for (int32_t f = 0; f < 7; f++)
        if (FlagGet(FLAG_SYS_TOWER_SILVER + f * 2) || FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1))
            symCount++;
    snprintf(buf, sizeof(buf), "%d / 7", (int)symCount);
    UiW_GbaSectionStrip(614, 74, 316, 26, UI_GBA_TAG_GOLD, "FRONTIER SYMBOLS", buf);

    static const char *const facNames[7] = {
        "BATTLE TOWER", "BATTLE DOME", "BATTLE PALACE", "BATTLE ARENA",
        "BATTLE FACTORY", "BATTLE PIKE", "BATTLE PYRAMID"
    };
    for (int32_t f = 0; f < 7; f++)
    {
        const int32_t fy = 108 + f * 58;
        const bool silver = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2);
        const bool gold = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1);
        UiW_GbaBox(614, fy, 316, 52, 4, gold ? UI_RGBA(216, 160, 40, 255) : UI_RGBA(184, 190, 176, 255),
                   gold ? UI_RGBA(255, 250, 224, 255) : UI_RGBA(244, 245, 236, 255),
                   gold ? UI_RGBA(255, 250, 224, 255) : UI_RGBA(244, 245, 236, 255), C_WHITE, 0);
        UiDraw_SpriteTint((uint16_t)((gold ? UI_SPR_FRONTIER_GOLD_1 : UI_SPR_FRONTIER_SILVER_1) + f), 624, fy + 10, 2,
                          (silver || gold) ? 0xFFFFFFFFu : UI_RGBA(180, 175, 160, 110));
        UiW_GbaTextMid(UI_FACE_VT323, 24, 666, fy, 52, facNames[f], UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        const char *rk = gold ? "GOLD" : silver ? "SILVER" : "NONE";
        const int32_t rw = UiText_Width(UI_FACE_VT323, 20, rk) + 16;
        const int32_t rxPill = 920 - rw;
        UiW_GbaBox(rxPill, fy + 13, rw, 26, 3,
                   gold ? UI_RGBA(192, 136, 16, 255) : silver ? UI_RGBA(120, 144, 168, 255) : UI_RGBA(184, 190, 176, 255),
                   gold ? UI_RGBA(255, 238, 168, 255) : silver ? UI_RGBA(232, 240, 248, 255) : UI_RGBA(236, 238, 230, 255),
                   gold ? UI_RGBA(255, 238, 168, 255) : silver ? UI_RGBA(232, 240, 248, 255) : UI_RGBA(236, 238, 230, 255), 0, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 20, rxPill, fy + 13, rw, 26, rk,
                           gold ? UI_RGBA(140, 88, 0, 255) : silver ? UI_RGBA(48, 76, 108, 255) : UI_RGBA(120, 128, 120, 255), C_WHITE);
    }
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGba();
        return;
    }
    char buf[48], name[16];
    UiKit_Header("TRAINER CARD", sTabs, 2, sTab, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), UiLocale_Translate("IDNo. %05u"), (unsigned)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8)));
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
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 52 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, got ? UiLocale_Translate(sBadgeNames[i]) : "-") / 2, y + 46, got ? UiLocale_Translate(sBadgeNames[i]) : "-", UI_COL_INK);
        }
        if (FlagGet(FLAG_BADGE01_GET + sBadge))
            snprintf(buf, sizeof(buf), UiLocale_Translate("%s BADGE"), UiLocale_Translate(sBadgeNames[sBadge]));
        else
            snprintf(buf, sizeof(buf), "%s", UiLocale_Translate("Not obtained yet."));
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
