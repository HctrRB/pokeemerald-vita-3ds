// POKéBLOCK CASE of the new UI, replacing pokeblock.c's screen in its four modes, and its FEED step
// replacing use_pokeblock.c (design/stitch/pokecubos, laid out as the BAG and the PokéNav CONDITION
// screens that are already built). GBA originals kept: the case sprite (shakes when the choice
// changes), every block in its colour's palette, the flavour row of the GBA case (a block lit for
// each flavour the block has, and FEEL), the list ending in STOW CASE, the game's texts.
//
// Modes and how they open:
//   field   the BAG's USE (pushed over it) or SELECT on the field (Ui_OpenPokeblockCaseField):
//           USE (FEED step), TOSS (YES / NO), SELECT moves a block to another place
//   battle  the Safari battle's POKéBLOCK (Ui_OpenPokeblockCaseBattle): a battle menu, as the BAG
//   feeder  the Safari feeder, Contest Lady: GIVE (Ui_OpenPokeblockCase from OpenPokeblockCase):
//           hosted over the black the script faded to, then the field and the script come back
// Every action goes through pokeblock.c's PokeblockCase_*ForUi, the GBA case's own code.
// FEED: YES closes the UI and plays the GBA eating scene (pokeblock_feed.c) full screen; back on
// the field the FEED step reopens with use_pokeblock.c's result lines, then the case.
// Core 1 only.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "field_screen_effect.h"
#include "item_menu.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "pokeblock.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Same geometry as the BAG (ui_scr_bag.c)
#define LIST_X 340
#define LIST_W 596
#define LIST_Y 106
#define ROW_H 40
#define ROWS 9
#define GBA_LIST_X 358
#define GBA_LIST_W 562
#define GBA_LIST_Y 108
#define GBA_ROW_H 38
#define GBA_ROWS 8

enum { ACT_USE, ACT_TOSS, ACT_CANCEL, ACT_YES, ACT_NO };
static const char *const sFlavors[5] = { "SPICY", "DRY", "SWEET", "BITTER", "SOUR" };

static uint8_t sMode = PBLOCK_CASE_FIELD;
static void (*sCallback)(void);           // hosted modes: the game's return callback
static UiBlock sBlocks[POKEBLOCKS_COUNT];
static int32_t sCount;
static UiList sList;
static UiMenu sMenu;
static bool sTossAsk;                     // sMenu is the TOSS YES / NO
static int32_t sMoveFrom = -1;            // SELECT: the block being moved
static int32_t sShake, sLastSel = -1;
static int32_t sActX[2], sActW, sActN;
static uint32_t sFrame;

static int32_t VisRows(void)
{
    return gUiTheme->gbaFrame ? GBA_ROWS : ROWS;
}

static void Refresh(void)
{
    PokeblockCase_CompactForUi();
    sCount = UiGame_Pokeblocks(sBlocks, POKEBLOCKS_COUNT);
    const int32_t sel = sList.sel;
    const int32_t first = sList.first;
    UiList_Reset(&sList, sCount + 1, VisRows());       // + STOW CASE
    sList.sel = sel <= sCount ? sel : sCount;
    sList.first = first;
    if (sList.first > sList.sel) sList.first = sList.sel;
    if (sList.sel >= sList.first + sList.rows) sList.first = sList.sel - sList.rows + 1;
    if (sList.first < 0) sList.first = 0;
}

static const UiBlock *Cur(void)
{
    return sList.sel < sCount ? &sBlocks[sList.sel] : NULL;
}

// The colour alone ("LITEBLUE" of "LITEBLUE POKéBLOCK"), where the full name does not fit
static const char *ColorName(const UiBlock *b)
{
    static char out[24];
    size_t i = 0;
    while (b->name[i] != '\0' && b->name[i] != ' ' && i < sizeof(out) - 1) { out[i] = b->name[i]; i++; }
    out[i] = '\0';
    return out;
}

static const char *UseLabel(void)
{
    return sMode == PBLOCK_CASE_GIVE ? "GIVE" : "USE";
}

static const char *ModeLabel(void)
{
    switch (sMode)
    {
    case PBLOCK_CASE_BATTLE: return "SAFARI BATTLE";
    case PBLOCK_CASE_FEEDER: return "POKéBLOCK FEEDER";
    case PBLOCK_CASE_GIVE:   return "CONTEST LADY";
    default:                 return "CHOOSE A POKéBLOCK";
    }
}

// ---------- Closing ----------

static void ExitHosted(void)
{
    PokeblockCase_ExitForUi(sMode, sCallback);
}

static void CloseCase(void)
{
    PokeblockCase_CancelForUi();
    if (sMode == PBLOCK_CASE_FEEDER || sMode == PBLOCK_CASE_GIVE)
        UiNav_Close(ExitHosted);
    else if (sMode == PBLOCK_CASE_BATTLE)
        UiNav_Close(NULL);
}

// ---------- FEED step (use_pokeblock.c) ----------

static uint8_t sFeedBlock;                // slot of the block to feed
static UiList sMons;
static UiMenu sAsk;
static bool sResult;                      // reopened after the eating scene: the result lines
static char sResultText[5][48];
static int32_t sResultN;
static int16_t sResultGain[5];
static uint8_t sFedMon;
static bool sFeedFromCondition;           // fed from the PokéNav CONDITION tab (the scene in a window)
static void LaunchFeedScene(void);

static int16_t sGain[5], sSheenGain;

static void FeedEnter(void)
{
    sAsk.open = false;
    UiList_Reset(&sMons, UiGame_PartyCount(), 6);
    if (sResult)
        sMons.sel = sFedMon;
}

static void FeedAsk(void)
{
    UiMon m;
    if (!UiGame_Mon(sMons.sel, &m))
        return;
    if (m.egg)
    {
        PlaySE(SE_FAILURE);
        UiKit_Toast("An EGG can't eat POKéBLOCKS.");
        return;
    }
    uint8_t c[6];
    UiGame_Condition(sMons.sel, c);
    if (c[5] >= 255)
    {
        char t[48];
        UiGame_Text(gText_WontEatAnymore, t, sizeof(t));
        PlaySE(SE_FAILURE);
        UiKit_Toast("%s", t);
        return;
    }
    char title[24];
    snprintf(title, sizeof(title), "FEED %s?", m.name);
    const bool gba = gUiTheme->gbaFrame;
    UiMenu_Open(&sAsk, title, 676, gba ? 300 : 304, 252);
    UiMenu_Add(&sAsk, "YES", ACT_YES);
    UiMenu_Add(&sAsk, "NO", ACT_NO);
    PlaySE(SE_SELECT);
}

static bool FeedUpdate(const UiInput *in)
{
    sFrame++;
    const bool gba = gUiTheme->gbaFrame;
    if (sResult)
    {
        // use_pokeblock.c's result lines, then the case under it (as the game goes back to it)
        if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap)
        {
            PlaySE(SE_SELECT);
            sResult = false;
            return false;
        }
        return true;
    }
    if (sAsk.open)
    {
        const int32_t id = UiMenu_Update(&sAsk, in);
        if (id == ACT_YES)
        {
            sFedMon = (uint8_t)sMons.sel;
            sFeedFromCondition = false;
            UiCore_CloseToScene(LaunchFeedScene);
        }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        return false;                                     // back to the case
    }
    if (UiHit(in, 676, gba ? 420 : 424, 252, 42))
    {
        FeedAsk();
        return true;
    }
    const int32_t before = sMons.sel;
    const int32_t a = gba ? UiList_Update(&sMons, in, 22, 108, 276, 58, true)
                          : UiList_Update(&sMons, in, 22, 66, 288, 44, true);
    if (sMons.sel != before)
        PlaySE(SE_SELECT);
    if (a >= 0)
        FeedAsk();
    return true;
}

static void FeedBuild(void)
{
    const bool gba = gUiTheme->gbaFrame;
    // (after the eating scene the block is eaten and the case compacted: its slot holds another)
    const UiBlock *b = NULL;
    for (int32_t i = 0; i < sCount && !sResult; i++)
        if (sBlocks[i].slot == sFeedBlock) b = &sBlocks[i];
    UiMon m;
    const bool have = UiGame_Mon(sMons.sel, &m);
    uint8_t c[6] = { 0 };
    if (have) UiGame_Condition(sMons.sel, c);
    const bool can = have && !m.egg && c[5] < 255 && !sResult && b != NULL;
    if (can)
        UiGame_PokeblockGains(sMons.sel, sFeedBlock, sGain, &sSheenGain);
    const int16_t *gain = sResult ? sResultGain : can ? sGain : NULL;
    char buf[48];

    if (gba)
    {
        snprintf(buf, sizeof(buf), "%s", b ? b->name : "");
        UiW_GbaTopBar(UI_GBA_BG_POKENAV, UI_SPR_START_BAG, "POKéBLOCK CASE", 0, 0, false, NULL);
        int32_t rx = UI_SCREEN_W - 14;
        UiW_GbaHelpBadge(&rx, "O", "BACK", true);
        UiW_GbaHelpBadge(&rx, "X", "FEED", false);
        UiCond_PartyListGba(sMons.sel);
        if (have)
        {
            UiW_GbaWin(322, 60, 326, 414, UI_GBA_WIN_DEFAULT);
            UiW_GbaSectionStrip(334, 72, 302, 28, UI_GBA_TAG_TEAL, "CONDITION", m.name);
            UiCond_Radar(sMons.sel, 485, 290, 92.0f, true, sResult ? NULL : gain);
            UiW_GbaWin(660, 60, 286, 414, UI_GBA_WIN_DEFAULT);
            UiW_GbaSectionStrip(672, 72, 262, 28, UI_GBA_TAG_GOLD, "ATTRIBUTES", NULL);
            UiCond_Attrs(sMons.sel, true, gain, sResult ? 0 : sSheenGain, "FEED", can);
        }
        // The block being fed, as the GBA FEED screen shows it on the top bar
        if (b != NULL)
        {
            UiSpr_Pokeblock(b->color, 272, 14, 3);
            UiW_GbaTextMid(UI_FACE_VT323, 28, 304, 4, 42, buf, C_WHITE, UI_GBA_SHAD_DARK);
        }
    }
    else
    {
        static const char *const tabs[1] = { "FEED A POKéMON" };
        static int32_t tabX[1], tabW[1];
        UiKit_Header("POKéBLOCK CASE", tabs, 1, 0, tabX, tabW, 8);
        if (b != NULL)
        {
            snprintf(buf, sizeof(buf), "%s  Lv%u", b->name, (unsigned)b->level);
            UiW_HeaderValue(buf, UI_COL_ON_FRAME);
        }
        UiCond_PartyList(sMons.sel);
        if (have)
        {
            snprintf(buf, sizeof(buf), "%s · CONDITION", m.name);
            UiW_Panel(328, 60, 320, 432);
            UiW_PanelHead(328, 60, 320, buf, NULL);
            UiCond_Radar(sMons.sel, 488, 300, 92.0f, false, sResult ? NULL : gain);
            UiW_Panel(660, 60, 284, 432);
            UiW_PanelHead(660, 60, 284, "ATTRIBUTES", NULL);
            UiCond_Attrs(sMons.sel, false, gain, sResult ? 0 : sSheenGain, "FEED", can);
        }
        int32_t fx = 16;
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Feed");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiKit_FooterClock();
    }
    if (sAsk.open)
        UiMenu_Draw(&sAsk);
    if (sResult)
    {
        const int32_t h = 76 + sResultN * 34;
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(220, (UI_SCREEN_H - h) / 2, 520, h, "POKéBLOCK CASE");
        for (int32_t i = 0; i < sResultN; i++)
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 244, (UI_SCREEN_H - h) / 2 + 52 + i * 34, sResultText[i], UI_COL_INK);
    }
}

static const UiScreen sFeedScreen = { FeedEnter, FeedUpdate, FeedBuild, NULL };

// After YES: the field fades out and the GBA eating scene plays (use_pokeblock.c FeedPokeblockToMon)
static void CB2_AfterFeedScene(void);
static void LaunchFeedScene(void)
{
    gPokeblockMonId = sFedMon;
    gSpecialVar_ItemId = sFeedBlock;
    gMain.savedCallback = CB2_AfterFeedScene;
    PreparePokeblockFeedScene();
}

static void PrepareReopenFeed(void)
{
    sMode = PBLOCK_CASE_FIELD;
    Refresh();
}

// Back on the PokéNav CONDITION tab: the result line as a toast (its radar shows the new values)
static void PrepareReopenCondition(void)
{
    sResult = false;
    UiKit_Toast("%s", sResultText[0]);
}

void UiPokeblock_FeedFromCondition(uint8_t partySlot, uint8_t pkblSlot)
{
    sFedMon = partySlot;
    sFeedBlock = pkblSlot;
    sFeedFromCondition = true;
    UiCore_CloseToSceneInWindow(LaunchFeedScene, "POKéBLOCK");
}

// Back from the scene: the block's effect (AddPokeblockToConditions), the result lines, the field
static void CB2_AfterFeedScene(void)
{
    static const u8 *const words[5] = { gText_Coolness, gText_Beauty3, gText_Cuteness, gText_Smartness, gText_Toughness };
    int16_t sheen;
    UiGame_PokeblockGains(sFedMon, sFeedBlock, sResultGain, &sheen);
    UiGame_FeedPokeblock(sFedMon, sFeedBlock);
    PokeblockCase_CompactForUi();
    sResultN = 0;
    UiMon m;
    UiGame_Mon(sFedMon, &m);
    char word[24], enh[24];
    UiGame_Text(gText_WasEnhanced, enh, sizeof(enh));
    for (int32_t i = 0; i < 5; i++)
        if (sResultGain[i] > 0)
        {
            UiGame_Text(words[i], word, sizeof(word));             // "Coolness " (trailing space)
            snprintf(sResultText[sResultN++], sizeof(sResultText[0]), "%s's %s%s", m.name, word, enh);
        }
    if (sResultN == 0)
        UiGame_Text(gText_NothingChanged, sResultText[sResultN++], sizeof(sResultText[0]));
    sResult = !sFeedFromCondition;
    UiCore_ReopenAfterField(sFeedFromCondition ? PrepareReopenCondition : PrepareReopenFeed);
}

// ---------- CASE ----------

static void Enter(void)
{
    sMenu.open = false;
    sTossAsk = false;
    sMoveFrom = -1;
    sShake = 0;
    sLastSel = -1;
    UiList_Reset(&sList, 0, VisRows());
    Refresh();
}

static void OpenActions(void)
{
    const UiBlock *b = Cur();
    if (b == NULL)
        return;
    const int32_t row = sList.sel - sList.first;
    if (gUiTheme->gbaFrame)
    {
        int32_t my = GBA_LIST_Y + row * GBA_ROW_H;
        if (my > 270) my = 270;
        UiMenu_Open(&sMenu, ColorName(b), 724, my, 210);
    }
    else
        UiMenu_Open(&sMenu, ColorName(b), LIST_X + LIST_W - 280, LIST_Y + row * ROW_H, 260);
    UiMenu_Add(&sMenu, UseLabel(), ACT_USE);
    if (sMode == PBLOCK_CASE_FIELD)
        UiMenu_Add(&sMenu, "TOSS", ACT_TOSS);
    UiMenu_Add(&sMenu, "CANCEL", ACT_CANCEL);
    sTossAsk = false;
}

static void DoUse(void)
{
    const UiBlock *b = Cur();
    if (b == NULL)
        return;
    switch (sMode)
    {
    case PBLOCK_CASE_BATTLE:
        PokeblockCase_UseInBattleForUi(b->slot);
        UiNav_Close(NULL);
        break;
    case PBLOCK_CASE_FEEDER:
        PokeblockCase_UseOnFeederForUi(b->slot);
        UiNav_Close(ExitHosted);
        break;
    case PBLOCK_CASE_GIVE:
        PokeblockCase_GiveToLadyForUi(b->slot);
        UiNav_Close(ExitHosted);
        break;
    default:
        sFeedBlock = b->slot;
        sResult = false;
        UiNav_Push(&sFeedScreen);
        break;
    }
}

static void AskToss(void)
{
    const UiBlock *b = Cur();
    if (b == NULL)
        return;
    // gText_ThrowAwayVar1: "Throw away this {block}?"
    UiMenu_Open(&sMenu, "THROW AWAY?", gUiTheme->gbaFrame ? 724 : LIST_X + LIST_W - 280, 200, gUiTheme->gbaFrame ? 210 : 260);
    UiMenu_Add(&sMenu, "YES", ACT_YES);
    UiMenu_Add(&sMenu, "NO", ACT_NO);
    sTossAsk = true;
}

static void ToggleMove(void)
{
    if (sMode != PBLOCK_CASE_FIELD)
        return;
    if (sMoveFrom < 0)
    {
        if (sList.sel >= sCount)
            return;
        sMoveFrom = sList.sel;
    }
    else
    {
        const int32_t to = sList.sel < sCount ? sList.sel : sCount - 1;
        PokeblockCase_MoveForUi((u8)sBlocks[sMoveFrom].slot, (u8)sBlocks[to].slot);
        sMoveFrom = -1;
        sList.sel = to;
        Refresh();
    }
    PlaySE(SE_SELECT);
}

static bool Update(const UiInput *in)
{
    sFrame++;
    const bool gba = gUiTheme->gbaFrame;
    if (sMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == ACT_USE)
            DoUse();
        else if (id == ACT_TOSS)
            AskToss();
        else if (id == ACT_YES && sTossAsk)
        {
            const UiBlock *b = Cur();
            if (b != NULL)
            {
                char name[24];
                snprintf(name, sizeof(name), "%s", b->name);
                PokeblockCase_TossForUi(b->slot);
                UiKit_Toast("The %s was thrown away.", name);         // gText_Var1ThrownAway
                Refresh();
            }
            sTossAsk = false;
        }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        if (sMoveFrom >= 0)
        {
            sMoveFrom = -1;
            return true;
        }
        if (sMode == PBLOCK_CASE_FIELD)
        {
            PokeblockCase_CancelForUi();
            return false;                                    // back to the BAG, or closes
        }
        CloseCase();
        return true;
    }
    if (in->pressed & VITA_BTN_SELECT)
    {
        ToggleMove();
        return true;
    }
    if (!gba)
        for (int32_t i = 0; i < sActN; i++)
            if (Cur() != NULL && UiHit(in, sActX[i], 440, sActW, 40))
            {
                PlaySE(SE_SELECT);
                if (i == 0) DoUse(); else AskToss();
                return true;
            }
    sList.rows = VisRows();
    const int32_t a = gba ? UiList_Update(&sList, in, GBA_LIST_X, GBA_LIST_Y, GBA_LIST_W, GBA_ROW_H, false)
                          : UiList_Update(&sList, in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
    if (sList.sel != sLastSel)
    {
        if (sLastSel >= 0)
            sShake = 12;                                     // the GBA case shakes on every move
        sLastSel = sList.sel;
    }
    if (a >= 0)
    {
        PlaySE(SE_SELECT);
        if (sMoveFrom >= 0)
            ToggleMove();
        else if (a == sCount)
        {
            if (sMode == PBLOCK_CASE_FIELD)
            {
                PokeblockCase_CancelForUi();
                return false;
            }
            CloseCase();
        }
        else
            OpenActions();
    }
    return true;
}

static int32_t ShakeDx(void)
{
    if (sShake <= 0)
        return 0;
    sShake--;
    return (sShake & 2) ? 3 : -3;
}

// The GBA case's flavour row: a block in the flavour's colour, lit when the block has it
static void FlavorCell(int32_t x, int32_t y, int32_t i, const UiBlock *b, bool gba)
{
    const bool on = b != NULL && b->flavor[i] > 0;
    UiSpr_PokeblockTint((uint8_t)(i + 1), x, y + 3, 3, on ? 0xFFFFFFFFu : UI_RGBA(255, 255, 255, 70));
    char v[24];
    snprintf(v, sizeof(v), "%s %u", sFlavors[i], b != NULL ? (unsigned)b->flavor[i] : 0u);
    UiW_GbaText(UI_FACE_VT323, 24, x + 30, y, v, on ? UI_RGBA(49, 49, 49, 255) : UI_RGBA(144, 150, 140, 255), UI_GBA_SHAD_LIGHT);
    (void)gba;
}

// The card's flavour rows: the block lit for each flavour it has, the name, the value at the right
static void FlavorRow(int32_t y, int32_t i, const UiBlock *b)
{
    const bool on = b->flavor[i] > 0;
    UiSpr_PokeblockTint((uint8_t)(i + 1), 34, y + 3, 3, on ? 0xFFFFFFFFu : UI_RGBA(255, 255, 255, 70));
    const uint32_t ink = on ? UI_COL_INK : UI_COL_INK_SOFT;
    UiText_DrawMid(UI_FACE_VT323, 24, 66, y, 28, sFlavors[i], ink);
    char v[8];
    snprintf(v, sizeof(v), "%u", (unsigned)b->flavor[i]);
    UiText_DrawMid(UI_FACE_VT323, 24, 304 - UiText_Width(UI_FACE_VT323, 24, v), y, 28, v, ink);
}

static void BuildGba(void)
{
    char buf[96];
    const UiBlock *b = Cur();
    UiW_GbaTopBar(UI_GBA_BG_BAG, UI_SPR_START_BAG, "POKéBLOCK CASE", 0, 0, false, NULL);
    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", sMoveFrom >= 0 ? "CANCEL" : "CLOSE", true);
    if (sMode == PBLOCK_CASE_FIELD)
        UiW_GbaHelpBadge(&rx, "SELECT", sMoveFrom >= 0 ? "PUT" : "MOVE", false);
    UiW_GbaHelpBadge(&rx, "X", sMode == PBLOCK_CASE_FIELD ? "OPTIONS" : UseLabel(), false);

    // Left column, as the BAG's: the mode's sign, then the case on the green mat and three boxes
    UiW_Bevel(14, 60, 316, 52, 4, UI_RGBA(20, 26, 28, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(32, 46, 54, 255),
              2, UI_RGBA(56, 104, 136, 255), UI_RGBA(28, 52, 68, 255), 3);
    UiW_Bevel(24, 68, 296, 34, 3, UI_RGBA(40, 56, 64, 255), UI_RGBA(248, 232, 152, 255), UI_RGBA(232, 200, 104, 255),
              2, UI_RGBA(255, 253, 240, 255), UI_RGBA(208, 176, 80, 255), 2);
    UiW_GbaTextCentred(UI_FACE_VT323, 28, 24, 68, 296, 34, ModeLabel(), UI_RGBA(40, 56, 64, 255), UI_RGBA(255, 253, 240, 255));

    const int32_t stageY = 120, stageH = 312;
    UiW_GbaWin(14, stageY, 316, stageH, UI_GBA_WIN_DARK);
    const int32_t matY = stageY + 18, matH = stageH - 36;
    UiW_GbaBox(26, matY, 142, matH, 4, UI_RGBA(20, 26, 28, 255), UI_RGBA(62, 146, 68, 255), UI_RGBA(62, 146, 68, 255), UI_RGBA(148, 246, 148, 255), 0);
    UiDraw_PushClip(29, matY + 3, 136, matH - 6);
    for (int32_t cy = 0; cy < matH; cy += 16)
        for (int32_t cx = ((cy / 16) & 1) ? 16 : 0; cx < 136; cx += 32)
            UiDraw_Rect(29 + cx, matY + 3 + cy, 16, 16, UI_RGBA(88, 186, 88, 255));
    UiDraw_PopClip();
    UiSpr_PokeblockCase(33 + ShakeDx(), matY + (matH - 128) / 2, 2);

    const int32_t boxX = 178, boxW = 140, boxH = 74;
    UiW_GbaBox(boxX, matY, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, matY + 8, "LEVEL", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    snprintf(buf, sizeof(buf), b ? "Lv%u" : "-", b ? (unsigned)b->level : 0u);
    UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, matY + 34, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    const int32_t b2Y = matY + 84;
    UiW_GbaBox(boxX, b2Y, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b2Y + 8, "FEEL", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    snprintf(buf, sizeof(buf), b ? "%u" : "-", b ? (unsigned)b->feel : 0u);
    UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b2Y + 34, buf, UI_RGBA(32, 156, 8, 255), UI_GBA_SHAD_LIGHT);
    const int32_t b3Y = b2Y + 84;
    UiW_GbaBox(boxX, b3Y, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b3Y + 8, "IN CASE", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    snprintf(buf, sizeof(buf), "%d / %d", (int)sCount, POKEBLOCKS_COUNT);
    UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b3Y + 34, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    // Right gold ledger, as the BAG's pocket
    UiW_GbaWin(342, 60, 604, 372, UI_GBA_WIN_GOLD);
    UiW_GbaTextMid(UI_FACE_VT323, 28, 362, 70, 32, "POKéBLOCKS", UI_RGBA(92, 50, 16, 255), UI_RGBA(255, 250, 224, 255));
    UiW_GbaTextMid(UI_FACE_VT323, 28, 878, 70, 32, "LV", UI_RGBA(92, 50, 16, 255), UI_RGBA(255, 250, 224, 255));
    UiDraw_Rect(358, 103, 572, 3, UI_RGBA(200, 160, 56, 255));
    sList.rows = GBA_ROWS;
    UiDraw_PushClip(GBA_LIST_X, GBA_LIST_Y, GBA_LIST_W, GBA_ROWS * GBA_ROW_H);
    for (int32_t r = 0; r < GBA_ROWS && sList.first + r < sCount + 1; r++)
    {
        const int32_t i = sList.first + r, y = GBA_LIST_Y + r * GBA_ROW_H;
        const bool on = (i == sList.sel);
        if (on)
        {
            UiW_GbaBox(GBA_LIST_X, y + 1, GBA_LIST_W - 6, GBA_ROW_H - 2, 4, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(GBA_LIST_X + 9, y + 14, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(GBA_LIST_X + 8, y + 13, true, UI_RGBA(230, 8, 8, 255));
        }
        else if (i < sCount)
            for (int32_t dx = 8; dx < GBA_LIST_W - 14; dx += 10)
                UiDraw_Rect(GBA_LIST_X + dx, y + GBA_ROW_H - 2, 6, 2, UI_RGBA(226, 212, 152, 255));
        if (i == sMoveFrom)
            UiDraw_Frame(GBA_LIST_X, y + 1, GBA_LIST_W - 6, GBA_ROW_H - 2, 3, gUiTheme->selEdge);
        if (i == sCount)
        {
            UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + 60, y, GBA_ROW_H, "STOW CASE",
                           on ? UI_RGBA(49, 49, 49, 255) : UI_RGBA(106, 100, 84, 255), UI_GBA_SHAD_LIGHT);
            continue;
        }
        UiSpr_Pokeblock(sBlocks[i].color, GBA_LIST_X + 26, y + 7, 3);
        UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + 60, y, GBA_ROW_H, sBlocks[i].name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "Lv%u", (unsigned)sBlocks[i].level);
        UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + GBA_LIST_W - 16 - UiText_Width(UI_FACE_VT323, 28, buf),
                       y, GBA_ROW_H, buf, UI_RGBA(74, 74, 82, 255), UI_GBA_SHAD_LIGHT);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(924, GBA_LIST_Y, GBA_ROWS * GBA_ROW_H, sList.first, GBA_ROWS, sCount + 1);

    // Bottom message: the block and its flavours, as the GBA case's lower panel
    UiW_GbaMsg(14, 438, 932, 98, false, true);
    UiW_GbaBox(38, 454, 66, 66, 4, UI_RGBA(40, 56, 64, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 0);
    if (b == NULL)
    {
        UiSpr_PokeblockCase(39, 455, 1);
        UiW_GbaTextMid(UI_FACE_VT323, 28, 118, 444, 86, sCount ? "Put the CASE away." : "The CASE is empty.",
                       UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }
    else
    {
        UiSpr_Pokeblock(b->color, 55, 471, 4);
        UiW_GbaText(UI_FACE_PS2P, 16, 118, 452, b->name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        for (int32_t i = 0; i < 5; i++)
            FlavorCell(118 + i * 134, 482, i, b, true);
    }
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGba();
        return;
    }
    char buf[96];
    const UiBlock *b = Cur();
    static int32_t tabX[1], tabW[1];
    const char *const tabs[1] = { ModeLabel() };
    UiKit_Header("POKéBLOCK CASE", tabs, 1, 0, tabX, tabW, 8);
    snprintf(buf, sizeof(buf), "%d / %d", (int)sCount, POKEBLOCKS_COUNT);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    // The case, as the BAG's art
    UiW_PanelDark(16, 60, 304, 136);
    UiSpr_PokeblockCase(22 + ShakeDx(), 64, 2);
    UiText_Draw(UI_FACE_PS2P, 16, 160, 100, "IN CASE", UI_COL_TITLE);
    snprintf(buf, sizeof(buf), "%d / %d", (int)sCount, POKEBLOCKS_COUNT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 160, 122, buf, UI_COL_ON_SOFT);

    // Block card, as the BAG's item card
    UiW_Panel(16, 208, 304, 284);
    sActN = 0;
    if (b == NULL)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 32, 224, sCount ? "Put the CASE away." : "The CASE is empty.", UI_COL_INK_SOFT);
    else
    {
        UiW_Inset(32, 222, 56, 56);
        UiSpr_Pokeblock(b->color, 36, 226, 6);
        UiText_Draw(UI_FACE_PS2P, 16, 100, 228, ColorName(b), UI_COL_INK);
        snprintf(buf, sizeof(buf), "Lv%u", (unsigned)b->level);
        const int32_t cw = UiW_Chip(100, 252, buf, false);
        snprintf(buf, sizeof(buf), "FEEL %u", (unsigned)b->feel);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 100 + cw + 10, 248, buf, UI_COL_INK);
        for (int32_t i = 0; i < 5; i++)
            FlavorRow(290 + i * 28, i, b);
        sActN = sMode == PBLOCK_CASE_FIELD ? 2 : 1;
        sActW = (272 - (sActN - 1) * 6) / sActN;
        for (int32_t i = 0; i < sActN; i++)
        {
            sActX[i] = 32 + i * (sActW + 6);
            UiW_Button(sActX[i], 440, sActW, i == 0 ? UseLabel() : "TOSS", i == 0 ? UI_BTN_GREEN : UI_BTN_AMBER, false);
        }
    }

    // List, as the BAG's pocket
    UiW_Panel(332, 60, 612, 432);
    UiW_PanelHead(332, 60, 612, "POKéBLOCKS", sMode == PBLOCK_CASE_FIELD ? (sMoveFrom >= 0 ? "SELECT: put here" : "SELECT: move") : NULL);
    sList.rows = ROWS;
    UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
    for (int32_t r = 0; r < ROWS && sList.first + r < sCount + 1; r++)
    {
        const int32_t i = sList.first + r, y = LIST_Y + r * ROW_H;
        const bool on = i == sList.sel;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        if (i == sMoveFrom)
            UiDraw_Frame(LIST_X, y, LIST_W - 12, ROW_H, 3, gUiTheme->selEdge);
        if (i == sCount)
        {
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, "STOW CASE", on ? ink : UI_COL_INK_SOFT);
            continue;
        }
        UiSpr_Pokeblock(sBlocks[i].color, LIST_X + 28, y + 8, 3);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, sBlocks[i].name, ink);
        snprintf(buf, sizeof(buf), "Lv%u", (unsigned)sBlocks[i].level);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 24 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, on ? ink : UI_COL_INK_SOFT);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, sList.first, ROWS, sCount + 1);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sMode == PBLOCK_CASE_FIELD ? "Options" : sMode == PBLOCK_CASE_GIVE ? "Give" : "Use");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, sMoveFrom >= 0 ? "Cancel" : "Back");
    if (sMode == PBLOCK_CASE_FIELD)
        UiW_FooterKey(&fx, "SELECT", sMoveFrom >= 0 ? "Put" : "Move");
    UiKit_FooterClock();
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
}

static void Resume(void)
{
    Refresh();
}

const UiScreen gUiPokeblockScreen = { Enter, Update, Build, Resume };

// ---------- Entry points ----------

void UiPokeblock_PushFromBag(void)
{
    sMode = PBLOCK_CASE_FIELD;
    UiNav_Push(&gUiPokeblockScreen);
}

bool Ui_OpenPokeblockCase(uint8_t caseId, void (*callback)(void))
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || (caseId != PBLOCK_CASE_FEEDER && caseId != PBLOCK_CASE_GIVE))
        return false;
    sMode = caseId;
    sCallback = callback;
    UiCore_OpenHosted(&gUiPokeblockScreen, NULL);
    return true;
}

// SELECT: UseRegisteredKeyItemOnField froze the field; closing the case unfreezes it
static void AfterCloseField(void)
{
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
}

bool Ui_OpenPokeblockCaseField(void)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || gMain.callback2 != CB2_Overworld)
        return false;
    sMode = PBLOCK_CASE_FIELD;
    UiCore_OpenFromScript(&gUiPokeblockScreen, AfterCloseField);
    return true;
}

bool Ui_OpenPokeblockCaseBattle(void)
{
    sMode = PBLOCK_CASE_BATTLE;
    gSpecialVar_ItemId = 0;
    return UiGame_OpenBattleMenu(&gUiPokeblockScreen);
}
