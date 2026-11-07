// POKé MART of the new UI (design/stitch/tienda_comprar, tienda_vender), opened by the shop script
// (shop.c) instead of the GBA buy menu and the old sell bag. BUY: the clerk's items with price and
// how many you have; SELL: your items by pocket at half price. X asks the quantity and confirms.

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "item.h"
#include "money.h"
#include "overworld.h"
#include "shop.h"
#include "sound.h"
#include "constants/game_stat.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/songs.h"

#define LIST_X 340
#define LIST_W 596
#define LIST_Y 106
#define ROW_H 40
#define ROWS 9

extern void Shop_ReturnFrom3DsSell(u8 taskId);
extern void Shop_RecordPurchase(u16 itemId, u16 count);

static const char *const sTabs[2] = { "BUY", "SELL" };
static const char *const sPockets[4] = { "ITEMS", "BALLS", "TMs", "BERRIES" };
static const uint8_t sPocketIds[4] = { ITEMS_POCKET, BALLS_POCKET, TMHM_POCKET, BERRIES_POCKET };

static uint8_t sTaskId;
static const uint16_t *sItems;
static uint16_t sItemCount;
static int32_t sTab, sSellPocket, sTabX[2], sTabW[2];
static UiList sList;
static UiBagRow sRows[64];
static int32_t sCount;
static UiQty sQty;

static void AfterClose(void)
{
    Shop_ReturnFrom3DsSell(sTaskId);
}

bool Ui_OpenMart(uint8_t shopTaskId, bool sell, const uint16_t *items, uint16_t count)
{
    if (!Ui_IsEnabled())
        return false;
    sTaskId = shopTaskId;
    sItems = items;
    sItemCount = count;
    sTab = sell ? 1 : 0;
    UiCore_OpenFromScript(&gUiMartScreen, AfterClose);
    return true;
}

static uint16_t Owned(uint16_t item)
{
    UiBagRow rows[64];
    const int32_t n = UiGame_PocketItems(UiGame_ItemPocket(item), rows, 64);
    uint16_t q = 0;
    for (int32_t i = 0; i < n; i++)
        if (rows[i].item == item) q += rows[i].qty;
    return q;
}

static void Refresh(void)
{
    if (sTab == 0)
    {
        sCount = sItemCount;
        UiList_Reset(&sList, sCount, ROWS);
        return;
    }
    sCount = UiGame_PocketItems(sPocketIds[sSellPocket], sRows, 64);
    UiList_Reset(&sList, sCount, ROWS);
}

static uint16_t CurItem(void)
{
    if (sCount == 0) return ITEM_NONE;
    return sTab == 0 ? sItems[sList.sel] : sRows[sList.sel].item;
}

static uint32_t Price(uint16_t item)
{
    return sTab == 0 ? GetItemPrice(item) : GetItemPrice(item) / 2;
}

static void Enter(void)
{
    sQty.open = false;
    sList.sel = sList.first = 0;
    Refresh();
}

static void Confirm(int32_t n)
{
    const uint16_t item = CurItem();
    char name[24];
    UiGame_ItemName(item, name, sizeof(name));
    const uint32_t total = Price(item) * (uint32_t)n;
    if (sTab == 0)
    {
        if (!IsEnoughMoney(&gSaveBlock1Ptr->money, total)) { PlaySE(SE_FAILURE); UiKit_Toast("You don't have enough money."); return; }
        if (!AddBagItem(item, (u16)n)) { PlaySE(SE_FAILURE); UiKit_Toast("You have no more room for items."); return; }
        RemoveMoney(&gSaveBlock1Ptr->money, total);
        IncrementGameStat(GAME_STAT_SHOPPED);
        Shop_RecordPurchase(item, (u16)n);
        PlaySE(SE_SHOP);
        // 10 or more POKé BALLS: a PREMIER BALL as a bonus, as the game
        if (item == ITEM_POKE_BALL && n >= 10 && AddBagItem(ITEM_PREMIER_BALL, 1))
            UiKit_Toast("Here you go! You also get a PREMIER BALL as an added bonus.");
        else
            UiKit_Toast("Here you go! Thank you very much.");
    }
    else
    {
        RemoveBagItem(item, (u16)n);
        AddMoney(&gSaveBlock1Ptr->money, total);
        PlaySE(SE_SHOP);
        UiKit_Toast("Turned over the %s and received $%u.", name, (unsigned)total);
        Refresh();
    }
}

static bool Update(const UiInput *in)
{
    if (sQty.open)
    {
        const int32_t n = UiQty_Update(&sQty, in);
        if (n > 0) Confirm(n);
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, 2);
    if (in->pressed & (VITA_BTN_L | VITA_BTN_R)) t = sTab ^ 1;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); sList.sel = sList.first = 0; Refresh(); return true; }
    if (sTab == 1)
    {
        int32_t p = sSellPocket;
        if (in->pressed & VITA_BTN_TRIANGLE) p = (sSellPocket + 1) % 4;
        for (int32_t i = 0; i < 4; i++)
            if (UiHit(in, 352 + i * 112, 70, 104, 30)) p = i;
        if (p != sSellPocket) { sSellPocket = p; PlaySE(SE_SELECT); sList.sel = sList.first = 0; Refresh(); return true; }
    }
    const int32_t a = UiList_Update(&sList, in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
    if (a < 0 || sCount == 0)
        return true;
    const uint16_t item = CurItem();
    const uint32_t price = Price(item);
    if (sTab == 1 && (price == 0 || GetItemImportance(item)))
    {
        PlaySE(SE_FAILURE);
        UiKit_Toast("Oh, no, I can't buy that.");
        return true;
    }
    int32_t max = sTab == 0 ? (price ? (int32_t)(UiGame_Money() / price) : 99) : sRows[a].qty;
    if (max > 99) max = 99;
    if (max < 1) { PlaySE(SE_FAILURE); UiKit_Toast("You don't have enough money."); return true; }
    PlaySE(SE_SELECT);
    UiQty_Open(&sQty, sTab == 0 ? "HOW MANY?" : "SELL HOW MANY?", max);
    return true;
}

static void Build(void)
{
    char buf[96], name[24];
    UiKit_Header("POKé MART", sTabs, 2, sTab, sTabX, sTabW, 12);
    UiKit_HeaderMoney();
    const uint16_t item = CurItem();
    // Item card
    UiW_Panel(16, 60, 312, 200);
    if (item != ITEM_NONE)
    {
        UiW_Inset(32, 76, 56, 56);
        UiSpr_ItemIcon(item, 36, 80, 2);
        UiGame_ItemName(item, name, sizeof(name));
        UiText_Draw(UI_FACE_PS2P, 16, 100, 82, name, UI_COL_INK);
        snprintf(buf, sizeof(buf), "In bag: x%u", (unsigned)Owned(item));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 100, 104, buf, UI_COL_INK);
        UiGame_ItemDescription(item, buf, sizeof(buf));
        UiW_TextWrapped(32, 140, 280, 30, buf, UI_COL_INK, 4);
    }
    UiW_Panel(16, 272, 312, 220);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 32, 288, sTab == 0 ? "Price" : "You get", UI_COL_INK);
    snprintf(buf, sizeof(buf), "$%u", (unsigned)(item ? Price(item) : 0));
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 312 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), 288, buf, UI_COL_MONEY);
    if (sTab == 0 && item == ITEM_POKE_BALL)
        UiW_TextWrapped(32, 330, 280, 30, "Buy 10 POKé BALLS, get a PREMIER BALL.", UI_COL_INK_SOFT, 3);
    if (sTab == 1)
        UiW_TextWrapped(32, 330, 280, 30, "Items sell for half their price.", UI_COL_INK_SOFT, 3);

    // List
    UiW_Panel(332, 60, 612, 432);
    if (sTab == 0)
        UiW_PanelHead(332, 60, 612, "FOR SALE", "In bag");
    else
        for (int32_t i = 0; i < 4; i++)
            UiW_Chip(352 + i * 112, 72, sPockets[i], i == sSellPocket);
    UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
    for (int32_t r = 0; r < ROWS && sList.first + r < sCount; r++)
    {
        const int32_t i = sList.first + r, y = LIST_Y + r * ROW_H;
        const uint16_t it = sTab == 0 ? sItems[i] : sRows[i].item;
        const bool on = i == sList.sel;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        UiSpr_ItemIcon(it, LIST_X + 28, y + 8, 1);
        UiGame_ItemName(it, name, sizeof(name));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, name, ink);
        snprintf(buf, sizeof(buf), "x%u", (unsigned)(sTab == 0 ? Owned(it) : sRows[i].qty));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 380, y + 4, buf, on ? ink : UI_COL_INK_SOFT);
        const uint32_t p = sTab == 0 ? GetItemPrice(it) : GetItemPrice(it) / 2;
        if (sTab == 1 && (p == 0 || GetItemImportance(it))) snprintf(buf, sizeof(buf), "-");
        else snprintf(buf, sizeof(buf), "$%u", (unsigned)p);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 28 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, ink);
    }
    UiDraw_PopClip();
    if (sCount == 0)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, LIST_Y + 4, "Nothing to sell here.", UI_COL_INK_SOFT);
    UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, sList.first, ROWS, sCount);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sTab == 0 ? "Buy" : "Sell");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    if (sTab == 1) UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Pocket");
    UiKit_FooterClock();
    if (sQty.open)
    {
        UiQty_Draw(&sQty, item);
        snprintf(buf, sizeof(buf), "%s $%u", sTab == 0 ? "Total" : "You get", (unsigned)(Price(item) * (uint32_t)sQty.value));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 300, 324, buf, UI_COL_MONEY);
    }
}

const UiScreen gUiMartScreen = { Enter, Update, Build, NULL };
