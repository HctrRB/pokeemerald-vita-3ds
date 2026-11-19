// BAG of the new UI (design/stitch/mochila): pocket tabs, bag art by pocket, item card with its
// action buttons, item list. X opens the action popup (USE / GIVE / TOSS / REGISTER), SELECT
// sorts the pocket. GIVE mode (opened from the party's ITEM > GIVE): the only action is GIVE.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "mail.h"
#include "sound.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/songs.h"

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
#define MAX_ROWS 64

enum { ACT_USE, ACT_CHECK, ACT_GIVE, ACT_TOSS, ACT_REGISTER, ACT_CANCEL, ACT_COUNT };
static const char *const sActNames[ACT_COUNT] = { "USE", "CHECK", "GIVE", "TOSS", "REGISTER", "CANCEL" };
static const char *const sPocketTabs[POCKETS_COUNT] = { "ITEMS", "POKé BALLS", "TMs & HMs", "BERRIES", "KEY ITEMS" };
static const char *const sPocketShort[POCKETS_COUNT] = { "ITEMS", "BALLS", "TMs", "BERRIES", "KEY" };
static const char *const sPocketCat[POCKETS_COUNT] = { "ITEM", "BALL", "TM / HM", "BERRY", "KEY ITEM" };
static const char *const sSortNames[UI_SORT_COUNT] = { "NUMBER", "A TO Z", "MOST", "LEAST" };

static int32_t sPocket = BALLS_POCKET;
static UiList sList[POCKETS_COUNT];
static UiBagRow sRows[MAX_ROWS];
static int32_t sCount;
static int32_t sGiveTo = -1;
static int32_t sSort;
static UiMenu sMenu;
static UiQty sQty;
static int32_t sTabX[POCKETS_COUNT], sTabW[POCKETS_COUNT];
static int32_t sActs[4], sActCount, sActX[4], sActW;

static int32_t BagVisRows(void)
{
    return gUiTheme->gbaFrame ? GBA_ROWS : ROWS;
}

void UiBag_SetGiveTo(int32_t partySlot)
{
    sGiveTo = partySlot;
}

static void Refresh(void)
{
    sCount = UiGame_PocketItems(sPocket, sRows, MAX_ROWS);
    UiList_Reset(&sList[sPocket], sCount + 1, BagVisRows());      // + CLOSE BAG
}

static uint16_t CurItem(void)
{
    const int32_t i = sList[sPocket].sel;
    return i < sCount ? sRows[i].item : ITEM_NONE;
}

// Actions of an item, in the order of the card buttons and the popup
static void BuildActs(uint16_t item)
{
    sActCount = 0;
    if (item == ITEM_NONE)
        return;
    if (UiGame_InBattleMenu())
    {
        if (UiGame_BattleItemUsable(item)) sActs[sActCount++] = ACT_USE;
        return;
    }
    if (sGiveTo >= 0)
    {
        if (UiGame_CanGive(item)) sActs[sActCount++] = ACT_GIVE;
        return;
    }
    if (sPocket != BALLS_POCKET) sActs[sActCount++] = ItemIsMail(item) ? ACT_CHECK : ACT_USE;
    if (UiGame_CanGive(item)) sActs[sActCount++] = ACT_GIVE;
    if (UiGame_CanToss(item) && sPocket != TMHM_POCKET) sActs[sActCount++] = ACT_TOSS;
    if (UiGame_CanRegister(item)) sActs[sActCount++] = ACT_REGISTER;
}

static void Enter(void)
{
    sPocket = ITEMS_POCKET;            // menus always open on their first tab
    sMenu.open = false;
    sQty.open = false;
    Refresh();
}

static void Resume(void)
{
    Refresh();
}

static void DoAction(int32_t act)
{
    const uint16_t item = CurItem();
    char name[24], msg[80];
    UiGame_ItemName(item, name, sizeof(name));
    switch (act)
    {
    case ACT_USE:
    {
        if (UiGame_InBattleMenu())
        {
            const UiBattleUse b = UiGame_BattleUse(item, msg, sizeof(msg));
            if (b == UI_BUSE_CLOSE)
                UiNav_Close(NULL);
            else if (b == UI_BUSE_TARGET)
            {
                UiTarget_Set(UI_TARGET_BATTLE, item);
                UiNav_Push(&gUiTargetScreen);
            }
            else
            {
                PlaySE(SE_FAILURE);
                UiKit_Toast("%s", msg);
            }
            break;
        }
        if (item == ITEM_POKEBLOCK_CASE)
        {
            // The new UI's POKéBLOCK CASE over the BAG (ui_scr_pokeblock.c); ○ comes back here
            PlaySE(SE_SELECT);
            UiPokeblock_PushFromBag();
            break;
        }
        void (*after)(void) = NULL;
        const UiUseResult r = UiGame_UseFromBag(item, msg, sizeof(msg), &after);
        if (r == UI_USE_CLOSE)
            UiNav_Close(after);
        else if (r == UI_USE_TARGET)
        {
            UiTarget_Set(UiGame_ItemIsTmHm(item) ? UI_TARGET_TM : UI_TARGET_USE, item);
            UiNav_Push(&gUiTargetScreen);
        }
        else
        {
            PlaySE(r == UI_USE_DONE ? SE_USE_ITEM : SE_FAILURE);
            UiKit_Toast("%s", msg);
            Refresh();
        }
        break;
    }
    case ACT_CHECK:
    {
        static struct Mail sBlankMail;
        memset(&sBlankMail, 0, sizeof(sBlankMail));
        sBlankMail.itemId = item;
        PlaySE(SE_SELECT);
        UiMailRead_Open(&sBlankMail, "BAG");
        break;
    }
    case ACT_GIVE:
        if (sGiveTo >= 0)
        {
            UiMon m;
            UiGame_Mon(sGiveTo, &m);
            if (ItemIsMail(item))
            {
                if (m.item != ITEM_NONE)
                {
                    PlaySE(SE_FAILURE);
                    UiKit_Toast("Can't transfer MAIL to a POKéMON holding an item.");
                    break;
                }
                void (*after)(void) = NULL;
                if (UiGame_GiveMailBegin(sGiveTo, item, &after))
                {
                    PlaySE(SE_SELECT);
                    UiNav_Close(after);
                }
                else
                {
                    PlaySE(SE_FAILURE);
                    UiKit_Toast("No room for MAIL.");
                }
                break;
            }
            uint16_t prev = ITEM_NONE;
            if (!UiGame_GiveItem(sGiveTo, item, &prev))
            {
                PlaySE(SE_FAILURE);
                UiKit_Toast(ItemIsMail(prev) ? "Please remove the MAIL." : "The BAG is full. The POKéMON's item couldn't be taken.");
                break;
            }
            UiGame_Mon(sGiveTo, &m);
            UiKit_Toast("%s was given the %s to hold.", m.name, name);
            sGiveTo = -1;
            UiNav_Pop();                                   // back to the party
        }
        else
        {
            UiTarget_Set(UI_TARGET_GIVE, item);
            UiNav_Push(&gUiTargetScreen);
        }
        break;
    case ACT_TOSS:
    {
        char title[24];
        snprintf(title, sizeof(title), "TOSS");
        UiQty_Open(&sQty, title, sRows[sList[sPocket].sel].qty);
        break;
    }
    case ACT_REGISTER:
        UiGame_Register(item);
        PlaySE(SE_SELECT);
        UiKit_Toast(UiGame_RegisteredItem() == item ? "%s registered to SELECT." : "%s is no longer registered.", name);
        break;
    default:
        break;
    }
}

static void OpenMenu(void)
{
    const uint16_t item = CurItem();
    BuildActs(item);
    char name[24];
    UiGame_ItemName(item, name, sizeof(name));
    if (gUiTheme->gbaFrame)
    {
        const int32_t row = sList[sPocket].sel - sList[sPocket].first;
        int32_t my = GBA_LIST_Y + row * GBA_ROW_H;
        if (my > 270) my = 270;
        UiMenu_Open(&sMenu, name, 738, my, 196);
    }
    else
    {
        const int32_t row = sList[sPocket].sel - sList[sPocket].first;
        UiMenu_Open(&sMenu, name, LIST_X + LIST_W - 260, LIST_Y + row * ROW_H, 240);
    }
    for (int32_t i = 0; i < sActCount; i++)
        UiMenu_Add(&sMenu, sActNames[sActs[i]], sActs[i]);
    UiMenu_Add(&sMenu, "CANCEL", ACT_CANCEL);
}

static bool Update(const UiInput *in)
{
    const bool gba = gUiTheme->gbaFrame;
    if (sQty.open)
    {
        const int32_t n = UiQty_Update(&sQty, in);
        if (n > 0)
        {
            char name[24];
            const uint16_t item = CurItem();
            UiGame_ItemName(item, name, sizeof(name));
            UiGame_Toss(item, (uint16_t)n);
            UiKit_Toast("Threw away %d %s.", (int)n, name);
            Refresh();
        }
        return true;
    }
    if (sMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id >= 0 && id != ACT_CANCEL)
            DoAction(id);
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        sGiveTo = -1;
        return false;
    }
    // Battle Pyramid: one list (ITEMS), no pockets and no sorting, as its GBA bag
    const bool pyr = UiGame_InPyramid();
    int32_t tab = -1;
    if (!pyr)
    {
        if (gba)
        {
            if (UiHit(in, 66, 68, 44, 34)) tab = (sPocket + POCKETS_COUNT - 1) % POCKETS_COUNT;
            else if (UiHit(in, 276, 68, 44, 34)) tab = (sPocket + 1) % POCKETS_COUNT;
            else
            {
                for (int32_t i = 0; i < POCKETS_COUNT; i++)
                    if (UiHit(in, 24 + i * 60, 110, 56, 28))
                        tab = i;
            }
        }
        else
            tab = UiKit_TabHit(in, sTabX, sTabW, POCKETS_COUNT);
        if (in->pressed & (VITA_BTN_L | VITA_BTN_LEFT)) tab = (sPocket + POCKETS_COUNT - 1) % POCKETS_COUNT;
        if (in->pressed & (VITA_BTN_R | VITA_BTN_RIGHT)) tab = (sPocket + 1) % POCKETS_COUNT;
    }
    if (tab >= 0 && tab != sPocket)
    {
        sPocket = tab;
        PlaySE(SE_SELECT);
        Refresh();
        return true;
    }
    const bool sortHit = gba && !pyr && UiHit(in, 178, 256, 140, 72);
    if (((in->pressed & (VITA_BTN_SELECT | VITA_BTN_TRIANGLE)) || sortHit) && !pyr)
    {
        sSort = (sSort + 1) % UI_SORT_COUNT;
        UiGame_SortPocket(sPocket, sSort);
        PlaySE(SE_SELECT);
        UiKit_Toast("POCKET SORTED: %s", sSortNames[sSort]);
        Refresh();
        return true;
    }
    if (!gba)
    {
        // Card buttons (touch)
        BuildActs(CurItem());
        if (sActCount > 0)
        {
            sActW = (272 - (sActCount - 1) * 6) / sActCount;
            for (int32_t i = 0; i < sActCount; i++)
                sActX[i] = 32 + i * (sActW + 6);
        }
        for (int32_t i = 0; i < sActCount; i++)
            if (UiHit(in, sActX[i], 440, sActW, 40))
            {
                PlaySE(SE_SELECT);
                DoAction(sActs[i]);
                return true;
            }
    }
    sList[sPocket].rows = BagVisRows();
    const int32_t lx = gba ? GBA_LIST_X : LIST_X;
    const int32_t ly = gba ? GBA_LIST_Y : LIST_Y;
    const int32_t lw = gba ? GBA_LIST_W : LIST_W;
    const int32_t rh = gba ? GBA_ROW_H : ROW_H;
    const int32_t a = UiList_Update(&sList[sPocket], in, lx, ly, lw, rh, false);
    if (a >= 0)
    {
        PlaySE(SE_SELECT);
        if (a == sCount)
        {
            sGiveTo = -1;
            return false;                                  // CLOSE BAG
        }
        OpenMenu();
    }
    return true;
}

static void BuildGba(void)
{
    const bool pyr = UiGame_InPyramid();
    char buf[128];
    UiW_GbaTopBar(UI_GBA_BG_BAG, UI_SPR_START_BAG, pyr ? "BATTLE BAG" : "BAG", 0, 0, false, NULL);

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "CLOSE", true);
    if (!pyr)
        UiW_GbaHelpBadge(&rx, "TRI", "SORT", false);
    UiW_GbaHelpBadge(&rx, "X", sGiveTo >= 0 ? "GIVE" : "OPTIONS", false);

    // Left column (14, 60, 316, 372)
    // 1. Pocket signboard (.bag-pocket-sign)
    UiW_Bevel(14, 60, 316, pyr ? 52 : 86, 4, UI_RGBA(20, 26, 28, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(32, 46, 54, 255),
              2, UI_RGBA(56, 104, 136, 255), UI_RGBA(28, 52, 68, 255), 3);
    UiW_Bevel(24, 68, 34, 34, 4, UI_RGBA(40, 56, 64, 255), UI_RGBA(248, 224, 136, 255), UI_RGBA(224, 168, 48, 255),
              1, UI_RGBA(255, 246, 200, 255), UI_RGBA(192, 136, 24, 255), 0);
    UiDraw_Sprite(UI_SPR_BALL_POKE, 29, 73, 1);

    UiW_Bevel(66, 68, 254, 34, 3, UI_RGBA(40, 56, 64, 255), UI_RGBA(248, 232, 152, 255), UI_RGBA(232, 200, 104, 255),
              2, UI_RGBA(255, 253, 240, 255), UI_RGBA(208, 176, 80, 255), 2);
    if (!pyr)
    {
        UiW_GbaArrowH(75, 80, false, UI_RGBA(255, 253, 240, 255));
        UiW_GbaArrowH(74, 79, false, UI_RGBA(208, 48, 48, 255));
    }
    UiW_GbaTextCentred(UI_FACE_VT323, 28, 86, 68, 214, 34, pyr ? "PYRAMID BAG" : sPocketTabs[sPocket],
                       UI_RGBA(40, 56, 64, 255), UI_RGBA(255, 253, 240, 255));
    if (!pyr)
    {
        UiW_GbaArrowH(305, 80, true, UI_RGBA(255, 253, 240, 255));
        UiW_GbaArrowH(304, 79, true, UI_RGBA(208, 48, 48, 255));
    }

    if (!pyr)
    {
        for (int32_t i = 0; i < POCKETS_COUNT; i++)
        {
            const int32_t tx = 24 + i * 60;
            const bool on = (i == sPocket);
            if (on)
            {
                UiW_GbaRingBox(tx, 110, 56, 28, 3, UI_RGBA(255, 90, 32, 255), UI_RGBA(255, 230, 57, 255),
                               UI_RGBA(255, 246, 205, 255), UI_RGBA(248, 224, 136, 255), C_WHITE, 0);
                UiW_GbaTextCentred(UI_FACE_VT323, 20, tx, 110, 56, 28, sPocketShort[i], UI_RGBA(41, 49, 49, 255), C_WHITE);
            }
            else
            {
                UiW_GbaBox(tx, 110, 56, 28, 3, UI_RGBA(16, 21, 24, 255), UI_RGBA(27, 40, 46, 255), UI_RGBA(27, 40, 46, 255), 0, 0);
                UiW_GbaTextCentred(UI_FACE_VT323, 20, tx, 110, 56, 28, sPocketShort[i], UI_RGBA(156, 192, 202, 255), UI_GBA_SHAD_DARK);
            }
        }
    }

    // 2. Stage card (.gba-win.dark-well.bag-stage-card)
    const int32_t stageY = pyr ? 120 : 154;
    const int32_t stageH = 432 - stageY;
    UiW_GbaWin(14, stageY, 316, stageH, UI_GBA_WIN_DARK);

    // Green checkerboard mat (.bag-mat)
    const int32_t matY = stageY + 18;
    const int32_t matH = stageH - 36;
    UiW_GbaBox(26, matY, 142, matH, 4, UI_RGBA(20, 26, 28, 255), UI_RGBA(62, 146, 68, 255), UI_RGBA(62, 146, 68, 255), UI_RGBA(148, 246, 148, 255), 0);
    UiDraw_PushClip(29, matY + 3, 136, matH - 6);
    for (int32_t cy = 0; cy < matH; cy += 16)
        for (int32_t cx = ((cy / 16) & 1) ? 16 : 0; cx < 136; cx += 32)
            UiDraw_Rect(29 + cx, matY + 3 + cy, 16, 16, UI_RGBA(88, 186, 88, 255));
    UiDraw_PopClip();

    const uint16_t bagBase = UiGame_PlayerIsMale() ? UI_SPR_BAG_MALE_FIRST : UI_SPR_BAG_FEMALE_FIRST;
    UiDraw_Sprite((uint16_t)(bagBase + (pyr ? ITEMS_POCKET : sPocket)), 33, matY + (matH - 128) / 2, 2);

    // 3 Stats cards beside mat (.bag-mini-box: 178, matY, 140, matH)
    const int32_t boxX = 178, boxW = 140, boxH = 74;
    // 1) MONEY box
    UiW_GbaBox(boxX, matY, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, matY + 8, "MONEY", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    snprintf(buf, sizeof(buf), "$%d", (int)UiGame_Money());
    UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, matY + 34, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    // 2) SORT BY / CAPACITY box (tap to cycle sort)
    const int32_t b2Y = matY + 84;
    UiW_GbaBox(boxX, b2Y, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    if (pyr)
    {
        UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b2Y + 8, "CAPACITY", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "%d / %d", (int)sCount, PYRAMID_BAG_ITEMS_COUNT);
        UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b2Y + 34, buf, UI_RGBA(32, 156, 8, 255), UI_GBA_SHAD_LIGHT);
    }
    else
    {
        UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b2Y + 8, "SORT BY", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b2Y + 34, sSortNames[sSort], UI_RGBA(32, 156, 8, 255), UI_GBA_SHAD_LIGHT);
    }

    // 3) IN POCKET / GIVE TO box (.bag-mini-box)
    const int32_t b3Y = b2Y + 84;
    UiW_GbaBox(boxX, b3Y, boxW, boxH, 3, UI_RGBA(20, 26, 28, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 2);
    if (sGiveTo >= 0)
    {
        UiMon gm;
        UiGame_Mon(sGiveTo, &gm);
        UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b3Y + 8, "GIVE TO", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiDraw_PushClip(boxX + 8, b3Y + 30, boxW - 14, 38);
        UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b3Y + 34, gm.name, UI_RGBA(32, 104, 168, 255), UI_GBA_SHAD_LIGHT);
        UiDraw_PopClip();
    }
    else
    {
        UiW_GbaText(UI_FACE_VT323, 24, boxX + 10, b3Y + 8, "IN POCKET", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "%d KINDS", (int)sCount);
        UiW_GbaText(UI_FACE_VT323, 28, boxX + 10, b3Y + 34, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }

    // Right gold ledger (.gba-win.bag-gold)
    UiW_GbaWin(342, 60, 604, 372, UI_GBA_WIN_GOLD);
    snprintf(buf, sizeof(buf), "%s POCKET", pyr ? "PYRAMID" : sPocketTabs[sPocket]);
    UiW_GbaTextMid(UI_FACE_VT323, 28, 362, 70, 32, buf, UI_RGBA(92, 50, 16, 255), UI_RGBA(255, 250, 224, 255));
    UiW_GbaTextMid(UI_FACE_VT323, 28, 868, 70, 32, "QTY", UI_RGBA(92, 50, 16, 255), UI_RGBA(255, 250, 224, 255));
    UiDraw_Rect(358, 103, 572, 3, UI_RGBA(200, 160, 56, 255));

    UiList *const l = &sList[sPocket];
    l->rows = GBA_ROWS;
    UiDraw_PushClip(GBA_LIST_X, GBA_LIST_Y, GBA_LIST_W, GBA_ROWS * GBA_ROW_H);
    for (int32_t r = 0; r < GBA_ROWS && l->first + r < sCount + 1; r++)
    {
        const int32_t i = l->first + r;
        const int32_t y = GBA_LIST_Y + r * GBA_ROW_H;
        const bool on = (i == l->sel);
        if (on)
        {
            UiW_GbaBox(GBA_LIST_X, y + 1, GBA_LIST_W - 6, GBA_ROW_H - 2, 4, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(GBA_LIST_X + 9, y + 14, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(GBA_LIST_X + 8, y + 13, true, UI_RGBA(230, 8, 8, 255));
        }
        else if (i < sCount)
        {
            for (int32_t dx = 8; dx < GBA_LIST_W - 14; dx += 10)
                UiDraw_Rect(GBA_LIST_X + dx, y + GBA_ROW_H - 2, 6, 2, UI_RGBA(226, 212, 152, 255));
        }
        if (i == sCount)
        {
            UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + 60, y, GBA_ROW_H, "CLOSE BAG",
                           on ? UI_RGBA(49, 49, 49, 255) : UI_RGBA(106, 100, 84, 255), UI_GBA_SHAD_LIGHT);
            continue;
        }
        UiSpr_ItemIcon(sRows[i].item, GBA_LIST_X + 26, y + 7, 1);
        char name[24];
        UiGame_ItemName(sRows[i].item, name, sizeof(name));
        const int32_t nw = UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + 60, y, GBA_ROW_H, name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (UiGame_RegisteredItem() == sRows[i].item)
            UiW_Chip(GBA_LIST_X + 70 + nw, y + 6, "SEL", true);
        if (sPocket != KEYITEMS_POCKET)
        {
            snprintf(buf, sizeof(buf), "x%2u", (unsigned)sRows[i].qty);
            UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + GBA_LIST_W - 16 - UiText_Width(UI_FACE_VT323, 28, buf),
                           y, GBA_ROW_H, buf, UI_RGBA(74, 74, 82, 255), UI_GBA_SHAD_LIGHT);
        }
    }
    UiDraw_PopClip();
    UiW_ScrollBar(924, GBA_LIST_Y, GBA_ROWS * GBA_ROW_H, l->first, GBA_ROWS, sCount + 1);

    // Bottom GBA footer description (.gba-msg)
    UiW_GbaMsg(14, 438, 932, 98, false, true);
    UiW_GbaBox(38, 454, 66, 66, 4, UI_RGBA(40, 56, 64, 255), C_WHITE, C_WHITE, UI_GBA_SHAD_LIGHT, 0);
    const uint16_t item = CurItem();
    if (item == ITEM_NONE)
    {
        UiDraw_Sprite(UI_SPR_BALL_POKE, 59, 475, 1);
        UiW_GbaTextMid(UI_FACE_VT323, 28, 118, 444, 86, sCount ? "Close the BAG and return to the game." : "This pocket is currently empty.",
                       UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }
    else
    {
        UiSpr_ItemIcon(item, 47, 463, 2);
        char name[24];
        UiGame_ItemName(item, name, sizeof(name));
        const int32_t tw = UiW_GbaText(UI_FACE_PS2P, 16, 118, 452, name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (sPocket != KEYITEMS_POCKET)
        {
            snprintf(buf, sizeof(buf), "IN BAG: x%u", (unsigned)sRows[l->sel].qty);
            UiW_GbaText(UI_FACE_PS2P, 16, 134 + tw, 452, buf, UI_RGBA(32, 156, 8, 255), UI_GBA_SHAD_LIGHT);
        }
        UiGame_ItemDescription(item, buf, sizeof(buf));
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 118, 476, 786, 26, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
    }
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
    if (sQty.open)
        UiQty_Draw(&sQty, item);
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGba();
        return;
    }
    const bool pyr = UiGame_InPyramid();
    if (pyr)
        UiKit_Header("BATTLE BAG", sPocketTabs, 1, 0, sTabX, sTabW, 8);
    else
        UiKit_Header("BAG", sPocketTabs, POCKETS_COUNT, sPocket, sTabX, sTabW, 8);
    UiKit_HeaderMoney();
    if (sGiveTo >= 0)
    {
        UiMon m;
        UiGame_Mon(sGiveTo, &m);
        char buf[40];
        snprintf(buf, sizeof(buf), "GIVE TO %s", m.name);
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf) + 24;
        UiW_Capsule(UI_SCREEN_W - 180 - w, 8, w, 32);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, UI_SCREEN_W - 180 - w + 12, 8, buf, UI_COL_ON_FRAME);
    }

    // Bag art and pocket
    UiW_PanelDark(16, 60, 304, 136);
    char buf[96];
    if (pyr)
    {
        // PYRAMID BAG: kinds used of the 10 slots
        UiText_Draw(UI_FACE_PS2P, 16, 32, 76, "PYRAMID BAG", UI_COL_TITLE);
        snprintf(buf, sizeof(buf), "%d / %d kinds", (int)sCount, PYRAMID_BAG_ITEMS_COUNT);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 32, 100, buf, UI_COL_ON_SOFT);
        for (int32_t i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
            UiW_Bar(32 + i * 27, 156, 23, 16, i < sCount ? 100 : 0, gUiTheme->tabTop, gUiTheme->tabBot);
    }
    else
    {
        const uint16_t bagBase = UiGame_PlayerIsMale() ? UI_SPR_BAG_MALE_FIRST : UI_SPR_BAG_FEMALE_FIRST;
        UiDraw_Sprite((uint16_t)(bagBase + sPocket), 22, 64, 2);
        UiText_Draw(UI_FACE_PS2P, 16, 160, 100, sPocketTabs[sPocket], UI_COL_TITLE);
        snprintf(buf, sizeof(buf), "%d kinds", (int)sCount);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 160, 122, buf, UI_COL_ON_SOFT);
    }

    // Item card
    UiW_Panel(16, 208, 304, 284);
    const uint16_t item = CurItem();
    if (item == ITEM_NONE)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 32, 224, sCount ? "Close the BAG." : "This pocket is empty.", UI_COL_INK_SOFT);
    else
    {
        UiW_Inset(32, 222, 56, 56);
        UiSpr_ItemIcon(item, 36, 226, 2);
        char name[24];
        UiGame_ItemName(item, name, sizeof(name));
        UiText_Draw(UI_FACE_PS2P, 16, 100, 228, name, UI_COL_INK);
        const int32_t cw = UiW_Chip(100, 252, sPocketCat[sPocket], false);
        if (sPocket != KEYITEMS_POCKET)
        {
            snprintf(buf, sizeof(buf), "x%u", (unsigned)sRows[sList[sPocket].sel].qty);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 100 + cw + 10, 248, buf, UI_COL_INK);
        }
        UiGame_ItemDescription(item, buf, sizeof(buf));
        UiW_TextWrapped(32, 288, 272, 30, buf, UI_COL_INK, 4);
        BuildActs(item);
        if (sActCount > 0)
        {
            static const uint8_t colours[ACT_COUNT] = { UI_BTN_GREEN, UI_BTN_AMBER, UI_BTN_RED, UI_BTN_AMBER, UI_BTN_GREEN };
            sActW = (272 - (sActCount - 1) * 6) / sActCount;
            for (int32_t i = 0; i < sActCount; i++)
            {
                sActX[i] = 32 + i * (sActW + 6);
                const char *label = sActs[i] == ACT_REGISTER && UiGame_RegisteredItem() == item ? "UNREG." : sActNames[sActs[i]];
                UiW_Button(sActX[i], 440, sActW, label, colours[sActs[i]], false);
            }
        }
    }

    // List
    UiW_Panel(332, 60, 612, 432);
    snprintf(buf, sizeof(buf), "%s POCKET", sPocketTabs[sPocket]);
    char sortBuf[24];
    snprintf(sortBuf, sizeof(sortBuf), "Sort: %s", sSortNames[sSort]);
    UiW_PanelHead(332, 60, 612, buf, sortBuf);
    const UiList *l = &sList[sPocket];
    UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
    for (int32_t r = 0; r < ROWS && l->first + r < sCount + 1; r++)
    {
        const int32_t i = l->first + r, y = LIST_Y + r * ROW_H;
        const bool on = i == l->sel;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        if (i == sCount)
        {
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, "CLOSE BAG", on ? ink : UI_COL_INK_SOFT);
            continue;
        }
        UiSpr_ItemIcon(sRows[i].item, LIST_X + 28, y + 8, 1);
        char name[24];
        UiGame_ItemName(sRows[i].item, name, sizeof(name));
        const int32_t nw = UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, name, ink);
        if (UiGame_RegisteredItem() == sRows[i].item)
            UiW_Chip(LIST_X + 72 + nw, y + 8, "SEL", true);
        if (sPocket != KEYITEMS_POCKET)
        {
            snprintf(buf, sizeof(buf), "x%u", (unsigned)sRows[i].qty);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 24 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, on ? ink : UI_COL_INK_SOFT);
        }
    }
    UiDraw_PopClip();
    UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, l->first, ROWS, sCount + 1);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Options");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiW_FooterKey(&fx, "SELECT", "Sort");
    UiKit_FooterClock();
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
    if (sQty.open)
        UiQty_Draw(&sQty, CurItem());
}

const UiScreen gUiBagScreen = { Enter, Update, Build, Resume };
