// BAG of the new UI (design/stitch/mochila): pocket tabs, bag art by pocket, item card with its
// action buttons, item list. X opens the action popup (USE / GIVE / TOSS / REGISTER), SELECT
// sorts the pocket. GIVE mode (opened from the party's ITEM > GIVE): the only action is GIVE.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "sound.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/songs.h"

#define LIST_X 340
#define LIST_W 596
#define LIST_Y 106
#define ROW_H 40
#define ROWS 9
#define MAX_ROWS 64

enum { ACT_USE, ACT_GIVE, ACT_TOSS, ACT_REGISTER, ACT_CANCEL, ACT_COUNT };
static const char *const sActNames[ACT_COUNT] = { "USE", "GIVE", "TOSS", "REGISTER", "CANCEL" };
static const char *const sPocketTabs[POCKETS_COUNT] = { "ITEMS", "POKé BALLS", "TMs & HMs", "BERRIES", "KEY ITEMS" };
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

void UiBag_SetGiveTo(int32_t partySlot)
{
    sGiveTo = partySlot;
}

static void Refresh(void)
{
    sCount = UiGame_PocketItems(sPocket, sRows, MAX_ROWS);
    UiList_Reset(&sList[sPocket], sCount + 1, ROWS);      // + CLOSE BAG
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
    if (sPocket != BALLS_POCKET) sActs[sActCount++] = ACT_USE;
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
    case ACT_GIVE:
        if (sGiveTo >= 0)
        {
            uint16_t prev;
            UiGame_GiveItem(sGiveTo, item, &prev);
            UiMon m;
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
    const int32_t row = sList[sPocket].sel - sList[sPocket].first;
    UiMenu_Open(&sMenu, name, LIST_X + LIST_W - 260, LIST_Y + row * ROW_H, 240);
    for (int32_t i = 0; i < sActCount; i++)
        UiMenu_Add(&sMenu, sActNames[sActs[i]], sActs[i]);
    UiMenu_Add(&sMenu, "CANCEL", ACT_CANCEL);
}

static bool Update(const UiInput *in)
{
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
    int32_t tab = pyr ? -1 : UiKit_TabHit(in, sTabX, sTabW, POCKETS_COUNT);
    if (!pyr && (in->pressed & VITA_BTN_L)) tab = (sPocket + POCKETS_COUNT - 1) % POCKETS_COUNT;
    if (!pyr && (in->pressed & VITA_BTN_R)) tab = (sPocket + 1) % POCKETS_COUNT;
    if (tab >= 0 && tab != sPocket)
    {
        sPocket = tab;
        PlaySE(SE_SELECT);
        Refresh();
        return true;
    }
    if ((in->pressed & VITA_BTN_SELECT) && !pyr)
    {
        sSort = (sSort + 1) % UI_SORT_COUNT;
        UiGame_SortPocket(sPocket, sSort);
        PlaySE(SE_SELECT);
        UiKit_Toast("POCKET SORTED: %s", sSortNames[sSort]);
        Refresh();
        return true;
    }
    // Card buttons (touch)
    BuildActs(CurItem());
    for (int32_t i = 0; i < sActCount; i++)
        if (UiHit(in, sActX[i], 440, sActW, 40))
        {
            PlaySE(SE_SELECT);
            DoAction(sActs[i]);
            return true;
        }
    const int32_t a = UiList_Update(&sList[sPocket], in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
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

static void Build(void)
{
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
