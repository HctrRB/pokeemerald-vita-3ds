// The player's PC of the new UI (design/stitch/pc_casa): ITEM STORAGE and MAILBOX of the bedroom
// and POKéMON CENTER PCs (player_pc.c), and DECORATION of the bedroom and secret base PCs
// (decoration.c). The PC's own choice list stays the game's (mirrored); its entries open this
// screen on their tab, and closing it brings the list back (ReshowPlayerPC; the secret base PC's
// cancel script).
// The rules and the lines are the game's:
//   ITEMS, WITHDRAW: how many (one at once when there is one), into the BAG ("Withdrew..."; "There
//     is no more room in the BAG."), the list compacted; triangle tosses: important items can't
//     be, how many, "Is it okay to throw away...?" YES / NO; SELECT moves an item in the list
//     (MoveItemSlotInList, as ItemStorage_FinishItemSwap).
//   ITEMS, DEPOSIT: the BAG's five pockets; important items can't be stored, how many, into the
//     PC (AddPCItem, "Deposited..."; "There's no more room...").
//   MAILBOX: the PC's MAIL (slots PARTY_SIZE..), compacted as Mailbox_CompactMailList. READ shows
//     it as ReadMail lays it out; TO THE BAG warns that the message will be lost, YES / NO, then
//     returns the MAIL item ("The BAG is full." else the message is erased); GIVE: to a POKéMON
//     of the party that is not an EGG and holds nothing (GiveMailToMon keeps the message).
//   DECORATION: the eight categories and their stored decorations, the placed ones marked.
//     DECORATE (the bedroom: DOLL and CUSHION only; not one in use; while there is room) closes the
//     screen and starts the game's placement on the field; PUT AWAY (if something is placed) starts
//     the game's putting away; both come back to this tab when they end. TOSS: not one in use,
//     "...will be discarded" YES / NO. The lines are the game's (decoration.c *ForUi).

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/mail_ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "event_data.h"
#include "international_string_util.h"
#include "item.h"
#include "mail.h"
#include "main.h"
#include "pokemon.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "constants/decorations.h"
#include "constants/items.h"
#include "constants/songs.h"

extern void PlayerPC_ReturnFromUi(u8 taskId);

enum { TAB_ITEMS = UI_PC_TAB_ITEMS, TAB_MAIL = UI_PC_TAB_MAIL, TAB_DECOR = UI_PC_TAB_DECORATION, TAB_COUNT };
enum { MODE_WITHDRAW, MODE_DEPOSIT };
enum { ASK_NONE, ASK_WITHDRAW, ASK_TOSS, ASK_TOSS_CONFIRM, ASK_DEPOSIT, ASK_MAIL_TO_BAG, ASK_MAIL_GIVE, ASK_DECOR_TOSS };
enum { AFTER_NONE, AFTER_PLACE, AFTER_PUT_AWAY };

#define LIST_X 344
#define LIST_Y 112
#define LIST_W 588
#define ROW_H 40
#define ROWS 8

static const char *const sTabNames[TAB_COUNT] = { "ITEMS", "MAILBOX", "DECORATION" };

// DECORATION: categories on the left, a grid of pictures on the right
#define DEC_CAT_Y 104
#define DEC_CAT_H 44
#define DEC_GX 300
#define DEC_GY 106
#define DEC_COLS 7
#define DEC_ROWS 3
#define DEC_CW 84
#define DEC_CH 74
#define DEC_GAP 6
#define DEC_MAX 40
static const char *const sPocketNames[POCKETS_COUNT] = { "ITEMS", "BALLS", "TMs", "BERRIES", "KEY" };
static const uint8_t sPocketIds[POCKETS_COUNT] = { ITEMS_POCKET, BALLS_POCKET, TMHM_POCKET, BERRIES_POCKET, KEYITEMS_POCKET };

static uint8_t sTaskId, sKind;
static int32_t sTab, sMode, sPocket, sTabX[TAB_COUNT], sTabW[TAB_COUNT];
static const char *sTabs[TAB_COUNT];      // the tabs this PC shows
static int32_t sTabIds[TAB_COUNT], sTabCount;
// DECORATION
static int32_t sDecorCat, sDecorSel, sDecorFirst, sDecorFocus, sDecorCount;
static uint8_t sDecorIdx[DEC_MAX];        // inventory index of each shown decoration
static bool sDecorInUse[DEC_MAX];
static int32_t sAfter;                    // what the game does once the screen is closed
static bool sResumeDecor;                 // reopened after placing / putting away: keep the cursor
static int32_t sChipX[3], sChipW[3];
static UiList sList;
static UiBagRow sBag[64];
static int32_t sCount;
static int32_t sMoveFrom = -1;      // SELECT: the PC item being moved, -1 none
static int32_t sAsk;
static UiQty sQty;
static UiMenu sMenu;
static int32_t sQuantity;
static uint8_t sMailSlots[MAIL_COUNT];
static uint32_t sFrame;
static char sQuestion[128];   // the game's question while a YES / NO or a list is open

static void AfterClose(void)
{
    if (sAfter == AFTER_PLACE)
    {
        sResumeDecor = true;
        Decoration_StartPlaceFromUi(sTaskId, (u8)sDecorCat, sDecorIdx[sDecorSel]);
        return;
    }
    if (sAfter == AFTER_PUT_AWAY)
    {
        sResumeDecor = true;
        Decoration_StartPutAwayFromUi(sTaskId);
        return;
    }
    sResumeDecor = false;
    if (sKind != UI_PC_CENTER)
        Decoration_EndUiSession(sTaskId);
    if (sKind != UI_PC_SECRET_BASE)
        PlayerPC_ReturnFromUi(sTaskId);
}

// The category's stored decorations (the game keeps them condensed) and which are placed
static void DecorRefresh(void)
{
    const struct DecorationInventory *inv = &gDecorationInventories[sDecorCat];
    sDecorCount = 0;
    for (int32_t i = 0; i < inv->size && sDecorCount < DEC_MAX; i++)
        if (inv->items[i] != DECOR_NONE)
            sDecorIdx[sDecorCount++] = (uint8_t)i;
    for (int32_t i = 0; i < sDecorCount; i++)
        sDecorInUse[i] = Decoration_InUseForUi((u8)sDecorCat, sDecorIdx[i]);
    if (sDecorSel >= sDecorCount) sDecorSel = sDecorCount > 0 ? sDecorCount - 1 : 0;
    const int32_t row = sDecorSel / DEC_COLS;
    if (row < sDecorFirst) sDecorFirst = row;
    if (row >= sDecorFirst + DEC_ROWS) sDecorFirst = row - DEC_ROWS + 1;
    if (sDecorCount == 0) { sDecorFirst = 0; sDecorFocus = 0; }
}

static uint8_t CurDecor(void)
{
    return sDecorCount > 0 ? gDecorationInventories[sDecorCat].items[sDecorIdx[sDecorSel]] : DECOR_NONE;
}

static int32_t MailCount(void)
{
    int32_t n = 0;
    for (int32_t i = PARTY_SIZE; i < MAIL_COUNT; i++)
        if (gSaveBlock1Ptr->mail[i].itemId != ITEM_NONE)
            sMailSlots[n++] = (uint8_t)i;
    return n;
}

// As Mailbox_CompactMailList: the PC's MAIL first, the empty slots after
static void CompactMail(void)
{
    struct Mail temp;
    for (int32_t i = PARTY_SIZE; i < MAIL_COUNT - 1; i++)
        for (int32_t j = i + 1; j < MAIL_COUNT; j++)
            if (gSaveBlock1Ptr->mail[i].itemId == ITEM_NONE)
                SWAP(gSaveBlock1Ptr->mail[i], gSaveBlock1Ptr->mail[j], temp);
}

static void Refresh(void)
{
    const int32_t sel = sList.sel;
    if (sTab == TAB_ITEMS && sMode == MODE_WITHDRAW)
        sCount = CountUsedPCItemSlots();
    else if (sTab == TAB_ITEMS)
        sCount = UiGame_PocketItems(sPocketIds[sPocket], sBag, 64);
    else if (sTab == TAB_MAIL)
        sCount = MailCount();
    else
    {
        DecorRefresh();
        sCount = 0;
    }
    UiList_Reset(&sList, sCount, ROWS);
    if (sel < sCount) sList.sel = sel;
    else if (sCount > 0) sList.sel = sCount - 1;
    if (sList.sel < sList.first) sList.first = sList.sel;
    if (sList.sel >= sList.first + ROWS) sList.first = sList.sel - ROWS + 1;
}

bool Ui_OpenPlayerPc(uint8_t taskId, uint8_t tab, uint8_t kind)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || tab >= TAB_COUNT || kind > UI_PC_SECRET_BASE)
        return false;
    sTabCount = 0;
    if (kind != UI_PC_SECRET_BASE) { sTabIds[sTabCount++] = TAB_ITEMS; sTabIds[sTabCount++] = TAB_MAIL; }
    if (kind != UI_PC_CENTER) sTabIds[sTabCount++] = TAB_DECOR;
    bool found = false;
    for (int32_t i = 0; i < sTabCount; i++)
    {
        sTabs[i] = sTabNames[sTabIds[i]];
        found |= sTabIds[i] == tab;
    }
    if (!found)
        return false;
    sTaskId = taskId;
    sTab = tab;
    sKind = kind;
    sAfter = AFTER_NONE;
    if (kind != UI_PC_CENTER)
        Decoration_InitForUi(kind == UI_PC_BEDROOM);
    UiCore_OpenFromScript(&gUiPlayerPcScreen, AfterClose);
    return true;
}

static int32_t TabSlot(int32_t tab)
{
    for (int32_t i = 0; i < sTabCount; i++)
        if (sTabIds[i] == tab)
            return i;
    return 0;
}

static void ToastText(const u8 *gameText)
{
    char msg[160];
    StringExpandPlaceholders(gStringVar4, gameText);
    UiGame_Text(gStringVar4, msg, sizeof(msg));
    UiKit_Toast("%s", msg);
}

// A line the game already expanded into gStringVar4
static void ToastVar4(void)
{
    char msg[160];
    UiGame_Text(gStringVar4, msg, sizeof(msg));
    UiKit_Toast("%s", msg);
}

static void Enter(void)
{
    if (!sResumeDecor)
        sDecorCat = sDecorSel = sDecorFirst = sDecorFocus = 0;
    sResumeDecor = false;
    sMode = MODE_WITHDRAW;
    sPocket = 0;
    sMoveFrom = -1;
    sAsk = ASK_NONE;
    sQty.open = false;
    sMenu.open = false;
    sList.sel = sList.first = 0;
    CompactPCItems();
    CompactMail();
    Refresh();
}

static uint16_t CurItem(void)
{
    if (sCount == 0 || sTab == TAB_DECOR) return ITEM_NONE;
    if (sTab == TAB_ITEMS && sMode == MODE_WITHDRAW) return gSaveBlock1Ptr->pcItems[sList.sel].itemId;
    if (sTab == TAB_ITEMS) return sBag[sList.sel].item;
    return gSaveBlock1Ptr->mail[sMailSlots[sList.sel]].itemId;
}

static uint16_t CurQuantity(void)
{
    if (sTab == TAB_ITEMS && sMode == MODE_WITHDRAW) return gSaveBlock1Ptr->pcItems[sList.sel].quantity;
    if (sTab == TAB_ITEMS) return sBag[sList.sel].qty;
    return 1;
}

static void SetItemVars(uint16_t item, int32_t n)
{
    CopyItemName(item, gStringVar1);
    ConvertIntToDecimalStringN(gStringVar2, n, STR_CONV_MODE_LEFT_ALIGN, 3);
}

// ---------------------------------------------------------------- items

static void Withdraw(int32_t n)
{
    const uint16_t item = CurItem();
    if (!AddBagItem(item, (u16)n))
    {
        PlaySE(SE_FAILURE);
        ToastText(gText_NoRoomInBag);
        return;
    }
    PlaySE(SE_SELECT);
    SetItemVars(item, n);
    ToastText(gText_WithdrawXItems);
    RemovePCItem((u8)sList.sel, (u16)n);
    CompactPCItems();
    Refresh();
}

// As ItemStorage_DoItemToss: the importance is checked once the quantity is chosen
static void TossAsk(int32_t n)
{
    if (GetItemImportance(CurItem()))
    {
        ToastText(gText_TooImportantToToss);
        sAsk = ASK_NONE;
        return;
    }
    sQuantity = n;
    SetItemVars(CurItem(), n);
    StringExpandPlaceholders(gStringVar4, gText_ConfirmTossItems);
    UiGame_Text(gStringVar4, sQuestion, sizeof(sQuestion));
    UiMenu_Open(&sMenu, "TOSS?", 380, 200, 200);
    UiMenu_Add(&sMenu, "YES", 1);
    UiMenu_Add(&sMenu, "NO", 0);
    sAsk = ASK_TOSS_CONFIRM;
}

// As TryDepositItem: the importance is checked once the quantity is chosen
static void Deposit(int32_t n)
{
    const uint16_t item = CurItem();
    if (GetItemImportance(item))
    {
        ToastText(gText_CantStoreImportantItems);
        return;
    }
    if (!AddPCItem(item, (u16)n))
    {
        PlaySE(SE_FAILURE);
        ToastText(gText_NoRoomForItems);
        return;
    }
    PlaySE(SE_SELECT);
    SetItemVars(item, n);
    ToastText(gText_DepositedVar2Var1s);
    RemoveBagItem(item, (u16)n);
    Refresh();
}

static void ItemAction(bool toss)
{
    const uint16_t item = CurItem();
    const uint16_t have = CurQuantity();
    if (item == ITEM_NONE)
        return;
    if (sMode == MODE_DEPOSIT)
    {
        if (have == 1) { Deposit(1); return; }
        sAsk = ASK_DEPOSIT;
        UiQty_Open(&sQty, "DEPOSIT HOW MANY?", have);
        return;
    }
    if (toss)
    {
        if (have == 1) { TossAsk(1); return; }
        sAsk = ASK_TOSS;
        UiQty_Open(&sQty, "TOSS HOW MANY?", have);
        return;
    }
    if (have == 1) { Withdraw(1); return; }
    sAsk = ASK_WITHDRAW;
    UiQty_Open(&sQty, "WITHDRAW HOW MANY?", have);
}

// ---------------------------------------------------------------- mail

static void MailToBagAsk(void)
{
    UiGame_Text(gText_MessageWillBeLost, sQuestion, sizeof(sQuestion));
    UiMenu_Open(&sMenu, "TO THE BAG?", 380, 200, 240);
    UiMenu_Add(&sMenu, "YES", 1);
    UiMenu_Add(&sMenu, "NO", 0);
    // YES first and selected, as DisplayYesNoMenuDefaultYes
    sAsk = ASK_MAIL_TO_BAG;
}

static void MailToBag(void)
{
    struct Mail *mail = &gSaveBlock1Ptr->mail[sMailSlots[sList.sel]];
    if (!AddBagItem(mail->itemId, 1))
    {
        PlaySE(SE_FAILURE);
        ToastText(gText_BagIsFull);
        return;
    }
    PlaySE(SE_SELECT);
    ToastText(gText_MailToBagMessageErased);
    ClearMail(mail);
    CompactMail();
    Refresh();
}

static void MailGiveAsk(void)
{
    if (CalculatePlayerPartyCount() == 0)
    {
        PlaySE(SE_FAILURE);
        ToastText(gText_NoPokemon);
        return;
    }
    snprintf(sQuestion, sizeof(sQuestion), "Give the MAIL to which POKéMON?");
    UiMenu_Open(&sMenu, "GIVE TO", 560, 100, 300);
    for (int32_t i = 0; i < CalculatePlayerPartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        UiMenu_Add(&sMenu, m.name, i);
    }
    sAsk = ASK_MAIL_GIVE;
}

static bool MailGive(int32_t slot)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG))
    {
        PlaySE(SE_FAILURE);                          // IsSelectedMonNotEgg: no message
        return false;
    }
    PlaySE(SE_SELECT);
    if (GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NONE)
    {
        ToastText(gText_PkmnHoldingItemCantHoldMail);
        return true;
    }
    struct Mail *mail = &gSaveBlock1Ptr->mail[sMailSlots[sList.sel]];
    GiveMailToMon(mon, mail);
    ClearMail(mail);
    ToastText(gText_MailTransferredFromMailbox);
    CompactMail();
    Refresh();
    return true;
}

// ---------------------------------------------------------------- decoration

// As the game's menu: DECORATE needs a decoration at all, then AttemptPlace's checks
static bool DecorPlace(void)
{
    PlaySE(SE_SELECT);
    if (!Decoration_HasAnyForUi() || (sDecorCount > 0 && !Decoration_CanPlaceForUi((u8)sDecorCat, sDecorIdx[sDecorSel])))
    {
        ToastVar4();
        return true;
    }
    if (sDecorCount == 0)
        return true;
    sAfter = AFTER_PLACE;
    return false;   // close: the game places it on the field
}

static bool DecorPutAway(void)
{
    PlaySE(SE_SELECT);
    if (!Decoration_CanPutAwayForUi())
    {
        ToastVar4();
        return true;
    }
    sAfter = AFTER_PUT_AWAY;
    return false;
}

static void DecorToss(void)
{
    PlaySE(SE_SELECT);
    if (!Decoration_HasAnyForUi()) { ToastVar4(); return; }
    if (sDecorCount == 0) return;
    if (!Decoration_CanTossForUi((u8)sDecorCat, sDecorIdx[sDecorSel])) { ToastVar4(); return; }
    UiGame_Text(gStringVar4, sQuestion, sizeof(sQuestion));
    UiMenu_Open(&sMenu, "TOSS?", 380, 200, 200);
    UiMenu_Add(&sMenu, "YES", 1);   // YES first and selected, as DisplayYesNoMenuDefaultYes
    UiMenu_Add(&sMenu, "NO", 0);
    sAsk = ASK_DECOR_TOSS;
}

static void DecorSetCat(int32_t cat)
{
    if (cat == sDecorCat) return;
    PlaySE(SE_SELECT);
    sDecorCat = cat;
    sDecorSel = sDecorFirst = 0;
    DecorRefresh();
}

static bool DecorUpdate(const UiInput *in)
{
    // Touch: categories, cells, the action chips
    for (int32_t c = 0; c < DECORCAT_COUNT; c++)
        if (UiHit(in, 28, DEC_CAT_Y + c * DEC_CAT_H, 236, DEC_CAT_H - 4)) { DecorSetCat(c); sDecorFocus = 0; return true; }
    for (int32_t r = 0; r < DEC_ROWS; r++)
        for (int32_t c = 0; c < DEC_COLS; c++)
        {
            const int32_t i = (sDecorFirst + r) * DEC_COLS + c;
            if (i < sDecorCount && UiHit(in, DEC_GX + c * (DEC_CW + DEC_GAP), DEC_GY + r * (DEC_CH + DEC_GAP), DEC_CW, DEC_CH))
            {
                if (sDecorFocus == 1 && i == sDecorSel) return DecorPlace();
                sDecorSel = i;
                sDecorFocus = 1;
                PlaySE(SE_SELECT);
                return true;
            }
        }
    if (UiHit(in, sChipX[0], 66, sChipW[0], 30)) return DecorPlace();
    if (UiHit(in, sChipX[1], 66, sChipW[1], 30)) return DecorPutAway();
    if (UiHit(in, sChipX[2], 66, sChipW[2], 30)) { DecorToss(); return true; }
    // Scrolling the grid by dragging
    if (in->dragging && in->dragDy != 0 && sDecorCount > DEC_COLS * DEC_ROWS)
    {
        static int32_t acc;
        acc += in->dragDy;
        const int32_t maxFirst = (sDecorCount + DEC_COLS - 1) / DEC_COLS - DEC_ROWS;
        while (acc <= -(DEC_CH + DEC_GAP) && sDecorFirst < maxFirst) { sDecorFirst++; acc += DEC_CH + DEC_GAP; }
        while (acc >= DEC_CH + DEC_GAP && sDecorFirst > 0) { sDecorFirst--; acc -= DEC_CH + DEC_GAP; }
        if (sDecorFirst <= 0 || sDecorFirst >= maxFirst) acc = 0;
        return true;
    }

    if (in->pressed & VITA_BTN_SQUARE) return DecorPutAway();
    if (sDecorFocus == 0)
    {
        if ((in->repeat & VITA_BTN_UP) && sDecorCat > 0) { DecorSetCat(sDecorCat - 1); return true; }
        if ((in->repeat & VITA_BTN_DOWN) && sDecorCat < DECORCAT_COUNT - 1) { DecorSetCat(sDecorCat + 1); return true; }
        if ((in->pressed & (VITA_BTN_RIGHT | VITA_BTN_CROSS)) && sDecorCount > 0)
        {
            PlaySE(SE_SELECT);
            sDecorFocus = 1;
            return true;
        }
        if (in->pressed & VITA_BTN_CROSS) return DecorPlace();   // an empty category: the game's "no decorations" line
        return true;
    }
    // The grid
    int32_t s = sDecorSel;
    const int32_t col = sDecorSel % DEC_COLS;
    if (in->repeat & VITA_BTN_LEFT)
    {
        if (col == 0) { PlaySE(SE_SELECT); sDecorFocus = 0; return true; }
        s--;
    }
    if ((in->repeat & VITA_BTN_RIGHT) && col < DEC_COLS - 1 && s + 1 < sDecorCount) s++;
    if ((in->repeat & VITA_BTN_UP) && s >= DEC_COLS) s -= DEC_COLS;
    if ((in->repeat & VITA_BTN_DOWN) && s + DEC_COLS < sDecorCount) s += DEC_COLS;
    else if ((in->repeat & VITA_BTN_DOWN) && s / DEC_COLS < (sDecorCount - 1) / DEC_COLS) s = sDecorCount - 1;
    if (s != sDecorSel)
    {
        PlaySE(SE_SELECT);
        sDecorSel = s;
        const int32_t row = s / DEC_COLS;
        if (row < sDecorFirst) sDecorFirst = row;
        if (row >= sDecorFirst + DEC_ROWS) sDecorFirst = row - DEC_ROWS + 1;
        return true;
    }
    if (in->pressed & VITA_BTN_CROSS) return DecorPlace();
    if (in->pressed & VITA_BTN_TRIANGLE) { DecorToss(); return true; }
    return true;
}

// ---------------------------------------------------------------- input

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sQty.open)
    {
        const int32_t n = UiQty_Update(&sQty, in);
        if (n > 0)
        {
            if (sAsk == ASK_WITHDRAW) Withdraw(n);
            else if (sAsk == ASK_DEPOSIT) Deposit(n);
            else if (sAsk == ASK_TOSS) { TossAsk(n); return true; }
            sAsk = ASK_NONE;
        }
        else if (n == 0)
            sAsk = ASK_NONE;
        return true;
    }
    if (sMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE)
            return true;
        if (sAsk == ASK_TOSS_CONFIRM && id == 1)
        {
            SetItemVars(CurItem(), sQuantity);
            ToastText(gText_ThrewAwayVar2Var1s);
            RemovePCItem((u8)sList.sel, (u16)sQuantity);
            CompactPCItems();
            Refresh();
        }
        else if (sAsk == ASK_MAIL_TO_BAG && id == 1)
            MailToBag();
        else if (sAsk == ASK_DECOR_TOSS && id == 1)
        {
            Decoration_TossForUi((u8)sDecorCat, sDecorIdx[sDecorSel]);
            ToastVar4();
            DecorRefresh();
        }
        else if (sAsk == ASK_MAIL_GIVE && id >= 0)
        {
            if (!MailGive(id)) { sMenu.open = true; return true; }   // an EGG: the list stays
        }
        sMenu.open = false;
        sAsk = ASK_NONE;
        return true;
    }

    if (in->pressed & VITA_BTN_CIRCLE)
    {
        if (sMoveFrom >= 0) { sMoveFrom = -1; PlaySE(SE_SELECT); return true; }
        if (sTab == TAB_DECOR && sDecorFocus == 1) { sDecorFocus = 0; PlaySE(SE_SELECT); return true; }
        PlaySE(SE_SELECT);
        return false;
    }
    // Tabs (L / R, touch)
    const int32_t slot = TabSlot(sTab);
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, sTabCount);
    if (in->pressed & VITA_BTN_L) t = (slot + sTabCount - 1) % sTabCount;
    if (in->pressed & VITA_BTN_R) t = (slot + 1) % sTabCount;
    if (t >= 0 && t != slot && sMoveFrom < 0)
    {
        sTab = sTabIds[t];
        sMode = MODE_WITHDRAW;
        sList.sel = sList.first = 0;
        PlaySE(SE_SELECT);
        Refresh();
        return true;
    }
    if (sTab == TAB_ITEMS && sMoveFrom < 0)
    {
        // WITHDRAW / DEPOSIT chips; in DEPOSIT the pocket chips and square for the next pocket
        int32_t m = sMode, p = sPocket;
        if (UiHit(in, 352, 66, 132, 30)) m = MODE_WITHDRAW;
        if (UiHit(in, 492, 66, 120, 30)) m = MODE_DEPOSIT;
        if (in->pressed & VITA_BTN_SQUARE) { if (sMode == MODE_WITHDRAW) m = MODE_DEPOSIT; else p = (sPocket + 1) % POCKETS_COUNT; }
        if (sMode == MODE_DEPOSIT && (in->pressed & VITA_BTN_SELECT)) m = MODE_WITHDRAW;
        for (int32_t i = 0; sMode == MODE_DEPOSIT && i < POCKETS_COUNT; i++)
            if (UiHit(in, 624 + i * 62, 66, 58, 30)) p = i;
        if (m != sMode || p != sPocket)
        {
            sMode = m;
            sPocket = p;
            sList.sel = sList.first = 0;
            PlaySE(SE_SELECT);
            Refresh();
            return true;
        }
    }
    if (sTab == TAB_DECOR)
        return DecorUpdate(in);
    const int32_t a = UiList_Update(&sList, in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
    if (sCount == 0)
        return true;
    if (sTab == TAB_ITEMS && sMode == MODE_WITHDRAW)
    {
        if (in->pressed & VITA_BTN_SELECT)
        {
            // Move an item: SELECT again (or X) puts it before the one under the cursor
            if (sMoveFrom < 0) { sMoveFrom = sList.sel; PlaySE(SE_SELECT); return true; }
        }
        if (sMoveFrom >= 0)
        {
            if ((in->pressed & VITA_BTN_SELECT) || a >= 0)
            {
                const int32_t to = sList.sel;
                PlaySE(SE_SELECT);
                if (to != sMoveFrom && to != sMoveFrom + 1)
                {
                    MoveItemSlotInList(gSaveBlock1Ptr->pcItems, (u32)sMoveFrom, (u32)to);
                    if (sMoveFrom < to) sList.sel--;
                }
                sMoveFrom = -1;
            }
            return true;
        }
        if (in->pressed & VITA_BTN_TRIANGLE) { ItemAction(true); return true; }
        if (a >= 0) ItemAction(false);
        return true;
    }
    if (sTab == TAB_ITEMS)
    {
        if (a >= 0) ItemAction(false);
        return true;
    }
    // MAILBOX
    if (a >= 0)
    {
        PlaySE(SE_SELECT);
        UiMailRead_Set(&gSaveBlock1Ptr->mail[sMailSlots[sList.sel]]);
        UiNav_Push(&gUiMailReadScreen);
        return true;
    }
    if (in->pressed & VITA_BTN_SQUARE) { PlaySE(SE_SELECT); MailToBagAsk(); return true; }
    if (in->pressed & VITA_BTN_TRIANGLE) { PlaySE(SE_SELECT); MailGiveAsk(); return true; }
    return true;
}

static void Resume(void)
{
    Refresh();
}

// ---------------------------------------------------------------- drawing

static void BuildDecor(void)
{
    char buf[160], name[24];
    u8 text[32];
    CopyDecorationCategoryName(text, (u8)sDecorCat);
    UiGame_Text(text, name, sizeof(name));
    snprintf(buf, sizeof(buf), "%s %u/%u", name, (unsigned)GetNumOwnedDecorationsInCategory((u8)sDecorCat),
             (unsigned)gDecorationInventories[sDecorCat].size);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    // Categories, as the game's list: name and owned / room (DECORATE in the bedroom: DOLL and CUSHION only)
    UiW_Panel(16, 60, 260, 432);
    UiW_PanelHead(16, 60, 260, "CATEGORY", NULL);
    for (int32_t c = 0; c < DECORCAT_COUNT; c++)
    {
        const int32_t y = DEC_CAT_Y + c * DEC_CAT_H;
        const bool on = c == sDecorCat;
        UiW_Row(28, y, 236, DEC_CAT_H - 4, on, c);
        if (on && sDecorFocus == 0)
            UiDraw_Frame(28, y, 236, DEC_CAT_H - 4, 3, UI_COL_GOLD);
        const uint32_t ink = on ? UI_COL_SELECT_INK : Decoration_CategoryPlaceableForUi((u8)c) ? UI_COL_INK : UI_COL_INK_SOFT;
        CopyDecorationCategoryName(text, (u8)c);
        UiGame_Text(text, name, sizeof(name));
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 40, y, DEC_CAT_H - 4, name, ink);
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)GetNumOwnedDecorationsInCategory((u8)c), (unsigned)gDecorationInventories[c].size);
        UiText_DrawMid(UI_FACE_VT323, UI_BODY_PX, 252 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y, DEC_CAT_H - 4, buf, ink);
    }

    // Actions and the grid
    UiW_Panel(288, 60, 656, 288);
    static const char *const sChips[3] = { "DECORATE", "PUT AWAY", "TOSS" };
    int32_t cx = 300;
    for (int32_t i = 0; i < 3; i++)
    {
        sChipX[i] = cx;
        sChipW[i] = UiW_Chip(cx, 68, sChips[i], i == 0);
        cx += sChipW[i] + 8;
    }
    for (int32_t r = 0; r < DEC_ROWS; r++)
        for (int32_t c = 0; c < DEC_COLS; c++)
        {
            const int32_t i = (sDecorFirst + r) * DEC_COLS + c;
            if (i >= sDecorCount)
                break;
            const int32_t x = DEC_GX + c * (DEC_CW + DEC_GAP), y = DEC_GY + r * (DEC_CH + DEC_GAP);
            const bool on = i == sDecorSel;
            UiW_Row(x, y, DEC_CW, DEC_CH, on && sDecorFocus == 1, 0);
            if (on)
                UiDraw_Frame(x, y, DEC_CW, DEC_CH, sDecorFocus == 1 ? 3 : 2, sDecorFocus == 1 ? UI_COL_GOLD : UI_COL_INK_SOFT);
            UiSpr_Decoration(gDecorationInventories[sDecorCat].items[sDecorIdx[i]], x + (DEC_CW - 56) / 2, y + (DEC_CH - 56) / 2, 56);
            if (sDecorInUse[i])
            {
                UiDraw_Rect(x + DEC_CW - 16, y + 6, 10, 10, UI_COL_LINE_DARK);
                UiDraw_Rect(x + DEC_CW - 15, y + 7, 8, 8, UI_COL_GOLD);
            }
        }
    if (sDecorCount == 0)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, DEC_GX + 8, DEC_GY + 8, "There are no decorations here.", UI_COL_INK_SOFT);
    const int32_t rows = (sDecorCount + DEC_COLS - 1) / DEC_COLS;
    if (rows > DEC_ROWS)
        UiW_ScrollBar(926, DEC_GY, DEC_ROWS * DEC_CH + (DEC_ROWS - 1) * DEC_GAP, sDecorFirst, DEC_ROWS, rows);

    // The decoration under the cursor, or the game's question
    UiW_Inset(288, 360, 656, 132);
    const uint8_t decor = CurDecor();
    if (sMenu.open)
        UiW_TextWrapped(304, 368, 624, 30, sQuestion, UI_COL_INK, 4);
    else if (decor != DECOR_NONE)
    {
        UiGame_Text(gDecorations[decor].name, name, sizeof(name));
        UiText_Draw(UI_FACE_PS2P, 16, 304, 372, name, UI_COL_INK);
        if (sDecorInUse[sDecorSel])
        {
            const int32_t nx = 304 + UiText_Width(UI_FACE_PS2P, 16, name) + 16;
            UiDraw_Rect(nx, 374, 10, 10, UI_COL_LINE_DARK);
            UiDraw_Rect(nx + 1, 375, 8, 8, UI_COL_GOLD);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, nx + 18, 366, "In use", UI_COL_INK_SOFT);
        }
        UiGame_Text(gDecorations[decor].description, buf, sizeof(buf));
        UiW_TextWrapped(304, 398, 624, 30, buf, UI_COL_INK, 3);
    }
    else
        UiW_TextWrapped(304, 368, 624, 30, "Choose a category. The decorations you buy are sent here.", UI_COL_INK_SOFT, 4);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Decorate");
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Toss");
    UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "Put away");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
}

static void Build(void)
{
    char buf[96], name[24];
    UiKit_Header("PC", sTabs, sTabCount, TabSlot(sTab), sTabX, sTabW, 12);
    const uint16_t item = CurItem();

    if (sTab == TAB_DECOR)
        BuildDecor();
    else if (sTab == TAB_ITEMS)
    {
        snprintf(buf, sizeof(buf), "STORED %u / %u", (unsigned)CountUsedPCItemSlots(), (unsigned)PC_ITEMS_COUNT);
        UiW_HeaderValue(buf, UI_COL_ON_FRAME);
        // Item card
        UiW_Panel(16, 60, 312, 216);
        if (item != ITEM_NONE)
        {
            UiW_Inset(32, 76, 56, 56);
            UiSpr_ItemIcon(item, 36, 80, 2);
            UiGame_ItemName(item, name, sizeof(name));
            UiText_Draw(UI_FACE_PS2P, 16, 100, 82, name, UI_COL_INK);
            uint16_t inPc = 0;
            for (int32_t i = 0; i < PC_ITEMS_COUNT; i++)
                if (gSaveBlock1Ptr->pcItems[i].itemId == item) inPc += gSaveBlock1Ptr->pcItems[i].quantity;
            snprintf(buf, sizeof(buf), "PC x%u · BAG x%u", (unsigned)inPc, (unsigned)CountTotalItemQuantityInBag(item));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 100, 102, buf, UI_COL_INK);
            UiGame_ItemDescription(item, buf, sizeof(buf));
            UiW_TextWrapped(32, 144, 280, 30, buf, UI_COL_INK, 4);
        }
        UiW_Inset(16, 288, 312, 204);
        UiW_TextWrapped(30, 296, 284, 30, sMenu.open ? sQuestion : sMoveFrom >= 0 ? "Move it where? SELECT or X puts it here."
                        : sMode == MODE_WITHDRAW ? "X takes items out to the BAG. TRIANGLE throws them away. SELECT moves one."
                                                 : "Choose an item from the BAG to store in the PC.", UI_COL_INK, 6);
        // List
        UiW_Panel(332, 60, 612, 432);
        UiW_Chip(352, 68, "WITHDRAW", sMode == MODE_WITHDRAW);
        UiW_Chip(492, 68, "DEPOSIT", sMode == MODE_DEPOSIT);
        if (sMode == MODE_DEPOSIT)
            for (int32_t i = 0; i < POCKETS_COUNT; i++)
                UiW_Chip(624 + i * 62, 68, sPocketNames[i], i == sPocket);
        UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
        for (int32_t r = 0; r < ROWS && sList.first + r < sCount; r++)
        {
            const int32_t i = sList.first + r, y = LIST_Y + r * ROW_H;
            const uint16_t it = sMode == MODE_WITHDRAW ? gSaveBlock1Ptr->pcItems[i].itemId : sBag[i].item;
            const uint16_t q = sMode == MODE_WITHDRAW ? gSaveBlock1Ptr->pcItems[i].quantity : sBag[i].qty;
            const bool on = i == sList.sel;
            UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
            if (i == sMoveFrom)
                UiDraw_Frame(LIST_X, y, LIST_W - 12, ROW_H, 3, UI_COL_GOLD);
            UiSpr_ItemIcon(it, LIST_X + 28, y + 8, 1);
            UiGame_ItemName(it, name, sizeof(name));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 60, y + 4, name, on ? UI_COL_SELECT_INK : UI_COL_INK);
            snprintf(buf, sizeof(buf), "x%u", (unsigned)q);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 28 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf,
                        on ? UI_COL_SELECT_INK : UI_COL_INK_SOFT);
        }
        UiDraw_PopClip();
        if (sCount == 0)
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, LIST_Y + 4, sMode == MODE_WITHDRAW ? "There are no items." : "No items in this pocket.",
                        UI_COL_INK_SOFT);
        UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, sList.first, ROWS, sCount);
        int32_t fx = 16;
        if (sMode == MODE_WITHDRAW)
        {
            UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Withdraw");
            UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Toss");
            UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "Deposit");
        }
        else
        {
            UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Deposit");
            UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "Pocket");
        }
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    }
    else
    {
        snprintf(buf, sizeof(buf), "MAIL %u / %u", (unsigned)sCount, (unsigned)(MAIL_COUNT - PARTY_SIZE));
        UiW_HeaderValue(buf, UI_COL_ON_FRAME);
        UiW_Panel(16, 60, 420, 432);
        UiW_PanelHead(16, 60, 420, "STORED MAIL", NULL);
        for (int32_t r = 0; r < ROWS && sList.first + r < sCount; r++)
        {
            const int32_t i = sList.first + r, y = LIST_Y + r * ROW_H;
            const struct Mail *mail = &gSaveBlock1Ptr->mail[sMailSlots[i]];
            const bool on = i == sList.sel;
            UiW_Row(28, y, 396, ROW_H, on, i);
            UiSpr_ItemIcon(mail->itemId, 40, y + 8, 1);
            u8 sender[PLAYER_NAME_LENGTH + 1];
            StringCopy(sender, mail->playerName);
            ConvertInternationalPlayerNameStripChar(sender, CHAR_SPACE);
            UiGame_Text(sender, name, sizeof(name));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 72, y + 4, name, on ? UI_COL_SELECT_INK : UI_COL_INK);
        }
        if (sCount == 0)
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 40, LIST_Y + 4, "There's no MAIL here.", UI_COL_INK_SOFT);
        UiW_Panel(448, 60, 496, 432);
        if (sCount > 0)
            UiMail_Draw(&gSaveBlock1Ptr->mail[sMailSlots[sList.sel]], 456, 76, 2);
        if (sMenu.open)
        {
            UiW_Inset(456, 404, 480, 80);
            UiW_TextWrapped(470, 410, 452, 30, sQuestion, UI_COL_INK, 2);
        }
        int32_t fx = 16;
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Read");
        UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "To the BAG");
        UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Give");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    }
    UiKit_FooterClock();
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
    if (sQty.open)
        UiQty_Draw(&sQty, item);
}

const UiScreen gUiPlayerPcScreen = { Enter, Update, Build, Resume };

// ---------------------------------------------------------------- reading a mail

static const struct Mail *sReading;

void UiMailRead_Set(const struct Mail *mail)
{
    sReading = mail;
}

// A mail as ReadMail shows it, the 240 x 160 picture at (x, y), times scale (2: the text face
// matches the GBA font's 16 px lines)
void UiMail_Draw(const struct Mail *mail, int32_t x, int32_t y, int32_t scale)
{
    static struct MailUiLayout sLayout;
    char text[96];
    Mail_LayoutForUi(mail, &sLayout);
    UiSpr_MailStationery(sLayout.mailType, x, y, scale);
    const uint16_t c = sLayout.textColor;
    const uint32_t ink = UI_RGBA(((c & 31) * 255) / 31, (((c >> 5) & 31) * 255) / 31, (((c >> 10) & 31) * 255) / 31, 255);
    // GBA FONT_NORMAL: 16 px lines, the text face at 32 at x2; a line of the game is 16 px from its top
    const uint8_t px = UI_BODY_PX, face = UI_FACE_VT323;
    for (int32_t i = 0; i < sLayout.numLines; i++)
    {
        UiGame_Text(sLayout.lines[i].text, text, sizeof(text));
        UiText_DrawMid(face, px, x + (16 + sLayout.lines[i].x) * scale, y + (24 + sLayout.lines[i].y) * scale, 14 * scale, text, ink);
    }
    UiGame_Text(sLayout.signature, text, sizeof(text));
    const int32_t sw = UiText_Width(face, px, text);
    UiText_DrawMid(face, px, x + (16 + 104 + sLayout.signatureWidth / 2) * scale - sw / 2, y + (24 + sLayout.signatureY) * scale, 14 * scale,
                   text, ink);
    if (sLayout.iconSpecies != SPECIES_NONE)
        UiSpr_MonIcon(sLayout.iconSpecies, 0, x + sLayout.iconX * scale - 16 * scale, y + sLayout.iconY * scale - 16 * scale, scale);
}

static void ReadEnter(void)
{
}

static bool ReadUpdate(const UiInput *in)
{
    if (in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE) || in->tap)
    {
        PlaySE(SE_SELECT);
        return false;
    }
    return true;
}

static void ReadBuild(void)
{
    UiKit_Header("PC", sTabs, sTabCount, TabSlot(TAB_MAIL), NULL, NULL, 12);
    char name[24];
    if (sReading != NULL)
    {
        UiGame_ItemName(sReading->itemId, name, sizeof(name));
        UiW_HeaderValue(name, UI_COL_ON_FRAME);
        UiMail_Draw(sReading, (UI_SCREEN_W - 480) / 2, 60 + (432 - 320) / 2, 2);
    }
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Close");
    UiKit_FooterClock();
}

const UiScreen gUiMailReadScreen = { ReadEnter, ReadUpdate, ReadBuild, NULL };
