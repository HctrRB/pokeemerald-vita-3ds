// TRAINER CARD of the new UI (design/stitch/ficha): CARD (trainer, money, Pokédex, time, badges)
// and PASSPORT (records, Battle Frontier symbols). With the FRONTIER PASS (FLAG_SYS_FRONTIER_PASS),
// the pass's own pages too, so the GBA pass is not needed: MAP (the frontier map, each facility
// marked and described as ShowFrontierMap does) and RECORD (the recorded battle, played as the
// pass plays it, then back to this page). Only data the game has.

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "overworld.h"
#include "sound.h"
#include "strings.h"
#include "constants/battle_frontier.h"
#include "constants/flags.h"
#include "constants/game_stat.h"
#include "constants/songs.h"
#include "constants/trainers.h"

extern void FrontierPass_LandmarkForUi(u8 facility, const u8 **name, const u8 **desc, s16 *x, s16 *y, bool8 *wide);
extern bool8 FrontierPass_HasRecordForUi(void);
extern void FrontierPass_PlayRecordForUi(void (*back)(void));

enum { TAB_CARD, TAB_PASSPORT, TAB_MAP, TAB_RECORD, TAB_COUNT };
static const char *const sTabs[TAB_COUNT] = { UI_N("CARD"), UI_N("PASSPORT"), UI_N("MAP"), UI_N("RECORD") };
static const char *const sGbaPages[TAB_COUNT] = { UI_N("GOLD CARD"), UI_N("PASSPORT"), UI_N("FRONTIER MAP"), UI_N("BATTLE RECORD") };
static const char *const sBadgeNames[8] = { UI_N("STONE"), UI_N("KNUCKLE"), UI_N("DYNAMO"), UI_N("HEAT"), UI_N("BALANCE"), UI_N("FEATHER"), UI_N("MIND"), UI_N("RAIN") };
static const char *const sLeaderNames[8] = { UI_N("ROXANNE"), UI_N("BRAWLY"), UI_N("WATTSON"), UI_N("FLANNERY"), UI_N("NORMAN"), UI_N("WINONA"), UI_N("TATE/LIZA"), UI_N("WALLACE") };
static const char *const sGymTowns[8] = { UI_N("RUSTBORO"), UI_N("DEWFORD"), UI_N("MAUVILLE"), UI_N("LAVARIDGE"), UI_N("PETALBURG"), UI_N("FORTREE"), UI_N("MOSSDEEP"), UI_N("SOOTOPOLIS") };
static int32_t sTab, sBadge, sTabX[TAB_COUNT], sTabW[TAB_COUNT];
static int32_t sTabCount;        // CARD and PASSPORT; MAP and RECORD with the FRONTIER PASS
static int32_t sFacility;        // MAP: the facility marked
static bool sHasRecord;          // RECORD: a recorded battle can be played
static uint32_t sFrame;

// MAP: the map (only its own area, UI_FRONTIER_MAP_*) at x4 and the facility list, per theme
// (touch and drawing share them)
#define MAP_SCALE 4
#define MAP_PX_W (UI_FRONTIER_MAP_W * MAP_SCALE)
#define MAP_PX_H (UI_FRONTIER_MAP_H * MAP_SCALE)
#define GBA_MAP_X 44
#define GBA_MAP_Y 112
#define GBA_FAC_X 614
#define GBA_FAC_Y 108
#define GBA_FAC_W 316
#define GBA_FAC_PITCH 58
#define MAP_X 24
#define MAP_Y 72
#define FAC_X 560
#define FAC_Y 108
#define FAC_W 376
#define FAC_PITCH 50
// RECORD: its button
#define REC_BTN_X 380
#define REC_BTN_Y 330
#define REC_BTN_W 200

static const char *const sBadgeDescs[8] = {
    UI_N("Boosts ATTACK and enables CUT outside battle."),
    UI_N("POKéMON up to Lv.30 obey. Enables FLASH."),
    UI_N("Boosts SPEED and enables ROCK SMASH."),
    UI_N("POKéMON up to Lv.50 obey. Enables STRENGTH."),
    UI_N("Boosts DEFENSE and enables SURF outside battle."),
    UI_N("POKéMON up to Lv.70 obey. Enables FLY."),
    UI_N("Boosts SP.ATK & SP.DEF. Enables DIVE."),
    UI_N("All POKéMON obey! Enables WATERFALL."),
};

static void Enter(void)
{
    sTab = TAB_CARD;
    sBadge = 0;
    sFacility = 0;
    sTabCount = FlagGet(FLAG_SYS_FRONTIER_PASS) ? TAB_COUNT : TAB_MAP;
    sHasRecord = sTabCount == TAB_COUNT && FrontierPass_HasRecordForUi();   // reads the record's sector, once
}

// The recorded battle (the pass's RECORD), from the field stopped as for the GBA scenes; back on
// the field the card opens again on this page
static void CB2_BackFromRecord(void)
{
    UiCore_ReopenAfterField(NULL);
}

static void StartRecord(void)
{
    FrontierPass_PlayRecordForUi(CB2_BackFromRecord);
}

static void PlayRecord(void)
{
    if (!sHasRecord)
    {
        PlaySE(SE_FAILURE);
        return;
    }
    PlaySE(SE_SELECT);
    UiCore_CloseToScene(StartRecord);
}

// Where a facility's marker is on screen (the map drawn at (mx, my)): its box, as DrawMap draws it
static void MarkerBox(int32_t f, int32_t mx, int32_t my, int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    const u8 *name, *desc;
    s16 lx, ly;
    bool8 wide;
    FrontierPass_LandmarkForUi((u8)f, &name, &desc, &lx, &ly, &wide);
    *w = (wide ? 32 : 16) * MAP_SCALE;
    *h = 16 * MAP_SCALE;
    *x = mx + (lx - UI_FRONTIER_MAP_X0) * MAP_SCALE - *w / 2;
    *y = my + (ly - UI_FRONTIER_MAP_Y0) * MAP_SCALE - *h / 2;
}

static void UpdateMap(const UiInput *in, bool gba)
{
    const int32_t before = sFacility;
    // A tap on a facility on the map marks it, as a tap on its row does
    for (int32_t f = 0; f < NUM_FRONTIER_FACILITIES; f++)
    {
        int32_t x, y, w, h;
        MarkerBox(f, gba ? GBA_MAP_X : MAP_X, gba ? GBA_MAP_Y : MAP_Y, &x, &y, &w, &h);
        if (UiHit(in, x, y, w, h))
            sFacility = f;
    }
    if (in->repeat & (VITA_BTN_UP | VITA_BTN_LEFT)) sFacility = (sFacility + NUM_FRONTIER_FACILITIES - 1) % NUM_FRONTIER_FACILITIES;
    if (in->repeat & (VITA_BTN_DOWN | VITA_BTN_RIGHT)) sFacility = (sFacility + 1) % NUM_FRONTIER_FACILITIES;
    for (int32_t f = 0; f < NUM_FRONTIER_FACILITIES; f++)
        if (gba ? UiHit(in, GBA_FAC_X, GBA_FAC_Y + f * GBA_FAC_PITCH, GBA_FAC_W, GBA_FAC_PITCH - 6)
                : UiHit(in, FAC_X, FAC_Y + f * FAC_PITCH, FAC_W, FAC_PITCH - 4))
            sFacility = f;
    if (sFacility != before) PlaySE(SE_SELECT);
}

static bool Update(const UiInput *in)
{
    const bool gba = gUiTheme->gbaFrame;
    sFrame++;
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = -1;
    if (gba)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, sTabCount);
        if (ph != 0)
            t = (sTab + sTabCount + (ph < 0 ? -1 : 1)) % sTabCount;
    }
    else
        t = UiKit_TabHit(in, sTabX, sTabW, sTabCount);
    if (in->pressed & VITA_BTN_L) t = (sTab + sTabCount - 1) % sTabCount;
    if (in->pressed & VITA_BTN_R) t = (sTab + 1) % sTabCount;
    if ((in->pressed & VITA_BTN_CROSS) && sTab <= TAB_PASSPORT) t = sTab ^ 1;   // the card's two sides
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); return true; }
    if (sTab == TAB_MAP)
    {
        UpdateMap(in, gba);
        return true;
    }
    if (sTab == TAB_RECORD)
    {
        if ((in->pressed & VITA_BTN_CROSS) || UiHit(in, REC_BTN_X, REC_BTN_Y, REC_BTN_W, 48))
            PlayRecord();
        return true;
    }
    const int32_t before = sBadge;
    if (sTab == TAB_CARD && (in->repeat & VITA_BTN_LEFT)) sBadge = (sBadge + 7) % 8;
    if (sTab == TAB_CARD && (in->repeat & VITA_BTN_RIGHT)) sBadge = (sBadge + 1) % 8;
    if (sTab == TAB_CARD)
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

// ---------- MAP and RECORD (the FRONTIER PASS's pages) ----------

typedef struct
{
    char name[24];
    char desc[96];
    s16 x, y;            // the marker's centre on the 240 x 160 map
    bool8 wide;          // a rectangle (MAP_INDICATOR_RECTANGLE), else a square
} Landmark;

static void GetLandmark(int32_t f, Landmark *l)
{
    const u8 *name, *desc;
    FrontierPass_LandmarkForUi((u8)f, &name, &desc, &l->x, &l->y, &l->wide);
    UiGame_Text(name, l->name, sizeof(l->name));
    UiGame_Text(desc, l->desc, sizeof(l->desc));
}

// The map with the marked facility's marker blinking on it (the pass's map cursor)
static void DrawMap(int32_t mx, int32_t my, uint32_t edge)
{
    UiSpr_FrontierMap(mx, my, MAP_SCALE);
    if ((sFrame >> 4) & 1)
        return;
    int32_t x, y, w, h;
    MarkerBox(sFacility, mx, my, &x, &y, &w, &h);
    UiDraw_Frame(x, y, w, h, 4, edge);
}

static bool HasSymbol(int32_t f, bool *gold)
{
    *gold = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1);
    return *gold || FlagGet(FLAG_SYS_TOWER_SILVER + f * 2);
}

static void BuildMapGba(void)
{
    Landmark sel;
    GetLandmark(sFacility, &sel);
    UiW_GbaWin(14, 60, 572, 472, UI_GBA_WIN_GOLD);
    UiW_GbaSectionStrip(30, 74, 540, 26, UI_GBA_TAG_GOLD, UI_TR("BATTLE FRONTIER"), NULL);
    UiW_GbaBox(GBA_MAP_X - 6, GBA_MAP_Y - 6, MAP_PX_W + 12, MAP_PX_H + 12, 4, UI_RGBA(150, 132, 72, 255), UI_RGBA(41, 49, 49, 255),
               UI_RGBA(41, 49, 49, 255), 0, 0);
    DrawMap(GBA_MAP_X, GBA_MAP_Y, UI_RGBA(255, 90, 32, 255));
    UiW_GbaBox(30, 414, 540, 106, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiDraw_PushClip(34, 418, 532, 98);
    UiW_GbaTextWrapped(UI_FACE_VT323, 24, 44, 424, 516, 30, sel.desc, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 3);
    UiDraw_PopClip();

    // The facilities, as the passport's rows; the marked one in the badge case's ring
    UiW_GbaWin(598, 60, 348, 472, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(GBA_FAC_X, 74, GBA_FAC_W, 26, UI_GBA_TAG_GOLD, UI_TR("FACILITIES"), NULL);
    for (int32_t f = 0; f < NUM_FRONTIER_FACILITIES; f++)
    {
        Landmark l;
        GetLandmark(f, &l);
        const int32_t fy = GBA_FAC_Y + f * GBA_FAC_PITCH;
        bool gold;
        const bool got = HasSymbol(f, &gold);
        if (f == sFacility)
            UiW_GbaRingBox(GBA_FAC_X, fy, GBA_FAC_W, GBA_FAC_PITCH - 6, 4, UI_RGBA(255, 90, 32, 255), UI_RGBA(230, 148, 65, 255),
                           UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
        else
            UiW_GbaBox(GBA_FAC_X, fy, GBA_FAC_W, GBA_FAC_PITCH - 6, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255),
                       UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        UiDraw_SpriteTint((uint16_t)((gold ? UI_SPR_FRONTIER_GOLD_1 : UI_SPR_FRONTIER_SILVER_1) + f), GBA_FAC_X + 10, fy + 10, 2,
                          got ? 0xFFFFFFFFu : UI_RGBA(180, 175, 160, 110));
        UiW_GbaTextMid(UI_FACE_VT323, 24, GBA_FAC_X + 52, fy, GBA_FAC_PITCH - 6, l.name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }
}

static void BuildRecordGba(void)
{
    UiW_GbaWin(14, 60, 932, 472, UI_GBA_WIN_GOLD);
    UiW_GbaSectionStrip(30, 74, 900, 26, UI_GBA_TAG_GOLD, UI_TR("BATTLE RECORD"), NULL);
    UiW_GbaBox(130, 170, 700, 120, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    char buf[64];
    UiGame_Text(sHasRecord ? gText_ViewRecordedBattle : gText_ThereIsNoBattleRecord, buf, sizeof(buf));
    UiW_GbaTextCentred(UI_FACE_VT323, 32, 130, 170, 700, 120, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    if (sHasRecord)
    {
        UiW_GbaBox(REC_BTN_X, REC_BTN_Y, REC_BTN_W, 48, 4, UI_RGBA(192, 136, 16, 255), UI_RGBA(255, 238, 168, 255),
                   UI_RGBA(232, 200, 96, 255), C_WHITE, 0);
        UiW_GbaTextCentred(UI_FACE_PS2P, 16, REC_BTN_X, REC_BTN_Y, REC_BTN_W, 48, UI_TR("VIEW"), UI_RGBA(92, 70, 18, 255), C_WHITE);
    }
}

static void BuildMap(void)
{
    Landmark sel;
    GetLandmark(sFacility, &sel);
    UiW_Panel(16, 60, 528, 432);
    UiW_Inset(MAP_X - 4, MAP_Y - 4, MAP_PX_W + 8, MAP_PX_H + 8);
    DrawMap(MAP_X, MAP_Y, UI_COL_SELECT);
    UiW_TextWrapped(MAP_X, MAP_Y + MAP_PX_H + 12, MAP_PX_W, 34, sel.desc, UI_COL_INK, 3);

    UiW_Panel(552, 60, 392, 432);
    UiW_PanelHead(552, 60, 392, UI_TR("FACILITIES"), NULL);
    for (int32_t f = 0; f < NUM_FRONTIER_FACILITIES; f++)
    {
        Landmark l;
        GetLandmark(f, &l);
        const int32_t y = FAC_Y + f * FAC_PITCH;
        const bool on = f == sFacility;
        bool gold;
        const bool got = HasSymbol(f, &gold);
        UiW_Row(FAC_X, y, FAC_W, FAC_PITCH - 4, on, f);
        UiDraw_SpriteTint((uint16_t)((gold ? UI_SPR_FRONTIER_GOLD_1 : UI_SPR_FRONTIER_SILVER_1) + f), FAC_X + 8, y + 7, 2,
                          got ? 0xFFFFFFFFu : UI_RGBA(200, 192, 170, 120));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, FAC_X + 52, y + 4, l.name, on ? UI_COL_SELECT_INK : UI_COL_INK);
    }
}

static void BuildRecord(void)
{
    char buf[64];
    UiW_Panel(16, 60, 928, 432);
    UiW_PanelHead(16, 60, 928, UI_TR("BATTLE RECORD"), NULL);
    UiGame_Text(sHasRecord ? gText_ViewRecordedBattle : gText_ThereIsNoBattleRecord, buf, sizeof(buf));
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, (UI_SCREEN_W - UiText_Width(UI_FACE_VT323, UI_DIALOG_PX, buf)) / 2, 210, buf, UI_COL_INK);
    if (sHasRecord)
        UiW_Button(REC_BTN_X, REC_BTN_Y, REC_BTN_W, UI_TR("VIEW"), UI_BTN_GREEN, false);
}

static void BuildGba(void)
{
    char buf[96], name[16];
    const uint16_t tid = (uint16_t)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8));
    UiGame_PlayerName(name, sizeof(name));
    UiW_GbaTopBar(UI_GBA_BG_CARD, UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY,
                  UI_TR("TRAINER CARD"), sTabCount, sTab, false, UiLocale_Lookup(sGbaPages[sTab]));

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", UI_TR("BACK"), true);
    if (sTab == TAB_CARD)
        UiW_GbaHelpBadge(&rx, "<>", UI_TR("BADGE"), false);
    if (sTab <= TAB_PASSPORT)
        UiW_GbaHelpBadge(&rx, "X", UI_TR("FLIP CARD"), false);
    if (sTab == TAB_MAP)
    {
        UiW_GbaHelpBadge(&rx, "<>", UI_TR("FACILITY"), false);
        BuildMapGba();
        return;
    }
    if (sTab == TAB_RECORD)
    {
        if (sHasRecord)
            UiW_GbaHelpBadge(&rx, "X", UI_TR("VIEW"), false);
        BuildRecordGba();
        return;
    }

    if (sTab == TAB_CARD)
    {
        // Left Gold Trainer Card (14, 60, 572, 472)
        UiW_GbaWin(14, 60, 572, 472, UI_GBA_WIN_GOLD);
        // Emerald banner strip (.tcard-banner: 30, 76, 540, 42)
        UiW_GbaBox(30, 76, 540, 42, 4, UI_RGBA(41, 49, 49, 255), UI_RGBA(45, 138, 107, 255), UI_RGBA(23, 94, 71, 255),
                   UI_RGBA(115, 205, 164, 140), 0);
        UiW_GbaTextMid(UI_FACE_PS2P, 16, 42, 76, 42, UI_TR("HOENN TRAINER CARD"), C_WHITE, UI_GBA_SHAD_DARK);

        const int32_t stars = (UiGame_BadgeCount() >= 8 ? 1 : 0) + (FlagGet(FLAG_SYS_GAME_CLEAR) ? 1 : 0)
                            + (UiGame_DexOwned() >= HOENN_DEX_COUNT ? 1 : 0)
                            + (gSaveBlock2Ptr->frontier.battlePoints >= 50 ? 1 : 0);
        for (int32_t s = 0; s < 4; s++)
            UiW_GbaStar(346 + s * 18, 91, s < stars ? UI_RGBA(255, 230, 57, 255) : UI_RGBA(16, 68, 50, 255));

        snprintf(buf, sizeof(buf), UI_TR("IDNo. %05u"), (unsigned)tid);
        UiW_GbaBox(436, 82, 124, 30, 3, UI_RGBA(16, 24, 24, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(24, 32, 32, 255), 0, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 24, 436, 82, 124, 30, buf, C_WHITE, UI_GBA_SHAD_DARK);

        // 6 info rows on left (30, 128, 376)
        CardRowGba(30, 128, 376, UI_TR("NAME"), name, UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
        CardRowGba(30, 182, 376, UI_TR("MONEY"), buf, UI_RGBA(176, 120, 8, 255));
        snprintf(buf, sizeof(buf), "%u", (unsigned)UiGame_DexOwned());
        CardRowGba(30, 236, 376, UI_TR("POKéDEX"), buf, UI_RGBA(49, 49, 49, 255));
        UiGame_PlayTime(buf, sizeof(buf));
        CardRowGba(30, 290, 376, UI_TR("PLAY TIME"), buf, UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(GAME_STAT_ENTERED_HOF));
        CardRowGba(30, 344, 376, UI_TR("HALL OF FAME"), FlagGet(FLAG_SYS_GAME_CLEAR) ? buf : "-", UI_RGBA(49, 49, 49, 255));
        snprintf(buf, sizeof(buf), UI_TR("%u BP"), (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        CardRowGba(30, 398, 376, UI_TR("BATTLE POINTS"), buf, UI_RGBA(23, 94, 71, 255));

        // Portrait grid well (.tcard-photo-well: 418, 128, 152, 316)
        UiW_GbaBox(418, 128, 152, 316, 4, UI_RGBA(150, 132, 72, 255), UI_RGBA(247, 243, 220, 255), UI_RGBA(247, 243, 220, 255), UI_GBA_SHAD_LIGHT, 0);
        for (int32_t g = 16; g < 146; g += 16)
            UiDraw_Rect(421 + g, 131, 1, 310, UI_RGBA(45, 138, 107, 28));
        for (int32_t g = 16; g < 310; g += 16)
            UiDraw_Rect(421, 131 + g, 146, 1, UI_RGBA(45, 138, 107, 28));
        UiSpr_Trainer(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 430, 198, 2);
        UiW_GbaBox(426, 402, 136, 32, 3, UI_RGBA(24, 32, 32, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(40, 56, 64, 255), 0, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 24, 426, 402, 136, 32,
                           FlagGet(FLAG_SYS_GAME_CLEAR) ? UI_TR("CHAMPION RANK") : UI_TR("CHALLENGER"), UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);

        // Bottom foot bar (.tcard-foot)
        UiW_GbaBox(30, 474, 540, 38, 3, UI_RGBA(184, 164, 88, 255), UI_RGBA(232, 220, 158, 255), UI_RGBA(232, 220, 158, 255), C_WHITE, 0);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 42, 474, 38, UI_TR("HOENN LEAGUE · EMERALD"), UI_RGBA(92, 70, 18, 255), C_WHITE);
        snprintf(buf, sizeof(buf), UI_TR("BADGES: %u / 8"), (unsigned)UiGame_BadgeCount());
        UiW_GbaTextMid(UI_FACE_VT323, 24, 558 - UiText_Width(UI_FACE_VT323, 24, buf), 474, 38, buf, UI_RGBA(92, 70, 18, 255), C_WHITE);

        // Right Badge Case panel (598, 60, 348, 472) matching renderCard in index.html
        UiW_GbaWin(598, 60, 348, 472, UI_GBA_WIN_DEFAULT);
        snprintf(buf, sizeof(buf), "%u / 8", (unsigned)UiGame_BadgeCount());
        UiW_GbaSectionStrip(614, 74, 316, 26, UI_GBA_TAG_GOLD, UI_TR("BADGE CASE"), buf);

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
            UiW_GbaText(UI_FACE_VT323, 24, bx + 48, by + 8, got ? UiLocale_Lookup(sBadgeNames[i]) : UI_TR("LOCKED"), UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            UiW_GbaText(UI_FACE_VT323, 20, bx + 48, by + 34, UiLocale_Lookup(sLeaderNames[i]), UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
            UiDraw_PopClip();
        }

        // Bottom .sum-memo badge description box (614, 392, 316, 124)
        const bool selGot = FlagGet(FLAG_BADGE01_GET + sBadge);
        UiW_GbaBox(614, 392, 316, 124, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        snprintf(buf, sizeof(buf), UI_TR("%s BADGE · %s GYM"), UiLocale_Lookup(sBadgeNames[sBadge]), UiLocale_Lookup(sGymTowns[sBadge]));
        UiDraw_PushClip(622, 396, 300, 116);
        UiW_GbaText(UI_FACE_VT323, 24, 624, 400, buf, UI_RGBA(23, 94, 71, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), UI_TR("Leader %s (%s)"), UiLocale_Lookup(sLeaderNames[sBadge]), selGot ? UI_TR("OBTAINED") : UI_TR("LOCKED"));
        UiW_GbaText(UI_FACE_VT323, 20, 624, 424, buf, selGot ? UI_RGBA(176, 120, 8, 255) : UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaTextWrapped(UI_FACE_VT323, 24, 624, 448, 296, 24, UiLocale_Lookup(sBadgeDescs[sBadge]), UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
        UiDraw_PopClip();
        return;
    }

    // Tab 1: PASSPORT (Gold Card Back 14, 60, 572, 472 + Frontier Symbols 598, 60, 348, 472)
    UiW_GbaWin(14, 60, 572, 472, UI_GBA_WIN_GOLD);
    UiW_GbaBox(30, 76, 540, 42, 4, UI_RGBA(41, 49, 49, 255), UI_RGBA(45, 138, 107, 255), UI_RGBA(23, 94, 71, 255),
               UI_RGBA(115, 205, 164, 140), 0);
    UiW_GbaTextMid(UI_FACE_PS2P, 16, 42, 76, 42, UI_TR("TRAINER RECORDS · BACK"), C_WHITE, UI_GBA_SHAD_DARK);
    snprintf(buf, sizeof(buf), UI_TR("IDNo. %05u"), (unsigned)tid);
    UiW_GbaBox(436, 82, 124, 30, 3, UI_RGBA(16, 24, 24, 255), UI_RGBA(24, 32, 32, 255), UI_RGBA(24, 32, 32, 255), 0, 0);
    UiW_GbaTextCentred(UI_FACE_VT323, 24, 436, 82, 124, 30, buf, C_WHITE, UI_GBA_SHAD_DARK);

    static const uint8_t stats[5] = { GAME_STAT_ENTERED_HOF, GAME_STAT_POKEMON_CAPTURES, GAME_STAT_POKEMON_TRADES, GAME_STAT_POKEBLOCKS_WITH_FRIENDS, GAME_STAT_WON_CONTEST };
    static const char *const labels[5] = { UI_N("HALL OF FAME DEBUTS"), UI_N("POKéMON CAUGHT"), UI_N("LINK TRADES"), UI_N("POKéBLOCKS W/ FRIENDS"), UI_N("CONTESTS WON") };
    for (int32_t i = 0; i < 5; i++)
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(stats[i]));
        CardRowGba(30, 128 + i * 54, 540, UiLocale_Lookup(labels[i]), buf, UI_RGBA(49, 49, 49, 255));
    }
    snprintf(buf, sizeof(buf), UI_TR("%u BP"), (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
    CardRowGba(30, 398, 540, UI_TR("BATTLE FRONTIER POINTS"), buf, UI_RGBA(23, 94, 71, 255));

    UiW_GbaBox(30, 474, 540, 38, 3, UI_RGBA(184, 164, 88, 255), UI_RGBA(232, 220, 158, 255), UI_RGBA(232, 220, 158, 255), C_WHITE, 0);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 42, 474, 38, UI_TR("BATTLE FRONTIER PASSPORT"), UI_RGBA(92, 70, 18, 255), C_WHITE);

    // Right Frontier Passport panel (598, 60, 348, 472) with 7 horizontal .fpass-row rows
    UiW_GbaWin(598, 60, 348, 472, UI_GBA_WIN_DEFAULT);
    int32_t symCount = 0;
    for (int32_t f = 0; f < 7; f++)
        if (FlagGet(FLAG_SYS_TOWER_SILVER + f * 2) || FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1))
            symCount++;
    snprintf(buf, sizeof(buf), "%d / 7", (int)symCount);
    UiW_GbaSectionStrip(614, 74, 316, 26, UI_GBA_TAG_GOLD, UI_TR("FRONTIER SYMBOLS"), buf);

    static const char *const facNames[7] = {
        UI_N("BATTLE TOWER"), UI_N("BATTLE DOME"), UI_N("BATTLE PALACE"), UI_N("BATTLE ARENA"),
        UI_N("BATTLE FACTORY"), UI_N("BATTLE PIKE"), UI_N("BATTLE PYRAMID")
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
        UiW_GbaTextMid(UI_FACE_VT323, 24, 666, fy, 52, UiLocale_Lookup(facNames[f]), UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        const char *rk = gold ? UI_TR("GOLD") : silver ? UI_TR("SILVER") : UI_TR("NONE");
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
    UiKit_Header(UI_TR("TRAINER CARD"), sTabs, sTabCount, sTab, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), UI_TR("IDNo. %05u"), (unsigned)(gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8)));
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    UiGame_PlayerName(name, sizeof(name));
    int32_t fx = 16;
    if (sTab == TAB_MAP || sTab == TAB_RECORD)
    {
        if (sTab == TAB_MAP)
            BuildMap();
        else
            BuildRecord();
        if (sTab == TAB_RECORD && sHasRecord)
            UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("View"));
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        UiKit_FooterClock();
        return;
    }
    if (sTab == TAB_CARD)
    {
        UiW_Panel(16, 60, 440, 432);
        UiW_Inset(32, 76, 152, 152);
        UiSpr_Trainer(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 44, 88, 2);
        UiText_Draw(UI_FACE_PS2P, 16, 200, 84, name, UI_COL_INK);
        snprintf(buf, sizeof(buf), "$%u", (unsigned)UiGame_Money());
        Row(200, 112, 240, UI_TR("MONEY"), buf, UI_COL_MONEY);
        snprintf(buf, sizeof(buf), "%u", (unsigned)UiGame_DexOwned());
        Row(200, 150, 240, UI_TR("POKéDEX"), buf, UI_COL_INK);
        UiGame_PlayTime(buf, sizeof(buf));
        Row(200, 188, 240, UI_TR("TIME"), buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), UI_TR("%u BP"), (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        Row(32, 250, 408, UI_TR("BATTLE POINTS"), buf, UI_COL_INK);
        snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(GAME_STAT_ENTERED_HOF));
        Row(32, 288, 408, UI_TR("HALL OF FAME"), FlagGet(FLAG_SYS_GAME_CLEAR) ? buf : "-", UI_COL_INK);

        UiW_Panel(468, 60, 476, 432);
        snprintf(buf, sizeof(buf), "%u / 8", (unsigned)UiGame_BadgeCount());
        UiW_PanelHead(468, 60, 476, UI_TR("BADGES"), buf);
        for (int32_t i = 0; i < 8; i++)
        {
            const int32_t x = 492 + (i % 4) * 110, y = 116 + (i / 4) * 92;
            const bool got = FlagGet(FLAG_BADGE01_GET + i);
            UiW_Inset(x, y, 104, 86);
            if (i == sBadge)
                UiDraw_Frame(x, y, 104, 86, 3, UI_COL_SELECT);
            // Unearned badges: a faded silhouette
            UiDraw_SpriteTint((uint16_t)(UI_SPR_BADGE_FIRST + i), x + 36, y + 8, 2, got ? 0xFFFFFFFFu : UI_RGBA(200, 192, 170, 120));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 52 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, got ? UiLocale_Lookup(sBadgeNames[i]) : "-") / 2, y + 46, got ? UiLocale_Lookup(sBadgeNames[i]) : "-", UI_COL_INK);
        }
        if (FlagGet(FLAG_BADGE01_GET + sBadge))
            snprintf(buf, sizeof(buf), UI_TR("%s BADGE"), UiLocale_Lookup(sBadgeNames[sBadge]));
        else
            snprintf(buf, sizeof(buf), UI_TR("Not obtained yet."));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 492, 316, buf, UI_COL_INK);
    }
    else
    {
        UiW_Panel(16, 60, 440, 432);
        UiW_PanelHead(16, 60, 440, UI_TR("TRAINER RECORDS"), NULL);
        static const uint8_t stats[5] = { GAME_STAT_ENTERED_HOF, GAME_STAT_POKEMON_CAPTURES, GAME_STAT_POKEMON_TRADES, GAME_STAT_POKEBLOCKS_WITH_FRIENDS, GAME_STAT_WON_CONTEST };
        static const char *const labels[5] = { UI_N("HALL OF FAME"), UI_N("POKéMON CAUGHT"), UI_N("TRADES"), UI_N("POKéBLOCKS W/ FRIENDS"), UI_N("CONTESTS WON") };
        for (int32_t i = 0; i < 5; i++)
        {
            snprintf(buf, sizeof(buf), "%u", (unsigned)GetGameStat(stats[i]));
            Row(32, 110 + i * 40, 408, UiLocale_Lookup(labels[i]), buf, UI_COL_INK);
        }
        snprintf(buf, sizeof(buf), "%u", (unsigned)gSaveBlock2Ptr->frontier.battlePoints);
        Row(32, 310, 408, UI_TR("BATTLE FRONTIER BP"), buf, UI_COL_INK);

        UiW_Panel(468, 60, 476, 432);
        UiW_PanelHead(468, 60, 476, UI_TR("FRONTIER SYMBOLS"), NULL);
        // Silver and gold symbol of the 7 facilities (FLAG_SYS_TOWER_SILVER .. FLAG_SYS_PYRAMID_GOLD)
        for (int32_t f = 0; f < 7; f++)
        {
            const int32_t x = 492 + (f % 4) * 110, y = 116 + (f / 4) * 92;
            const bool silver = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2), gold = FlagGet(FLAG_SYS_TOWER_SILVER + f * 2 + 1);
            UiW_Inset(x, y, 104, 86);
            UiDraw_SpriteTint((uint16_t)((gold ? UI_SPR_FRONTIER_GOLD_1 : UI_SPR_FRONTIER_SILVER_1) + f), x + 36, y + 8, 2,
                              (silver || gold) ? 0xFFFFFFFFu : UI_RGBA(200, 192, 170, 120));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 40, y + 46, gold ? UI_TR("GOLD") : silver ? UI_TR("SILVER") : "-", UI_COL_INK);
        }
    }
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Flip card"));
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
    UiKit_FooterClock();
}

const UiScreen gUiCardScreen = { Enter, Update, Build, NULL };
