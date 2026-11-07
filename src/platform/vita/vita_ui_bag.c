#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_extras.h"

#include "global.h"
#include "battle_pyramid.h"
#include "bike.h"
#include "coins.h"
#include "data.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "load_save.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "sound.h"
#include "task.h"
#include "constants/battle_frontier.h"
#include "constants/item.h"
#include "money.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/vars.h"

#include <stdio.h>
#include <string.h>

enum BagUiState
{
    BAG_STATE_LIST = 0,
    BAG_STATE_ACTION,
    BAG_STATE_TOSS_QTY,
    BAG_STATE_SELL_QTY
};

enum BagSortMode
{
    BAG_SORT_TYPE = 0,
    BAG_SORT_AZ,
    BAG_SORT_MOST,
    BAG_SORT_LEAST,
    BAG_SORT_COUNT
};

typedef struct
{
    uint16_t itemId;
    uint16_t quantity;
} CompactBagRow;

static uint8_t sPocket = 0; // 0..4 canonical Emerald pockets
static int32_t sCursor[5] = { 0 };
static int32_t sScroll[5] = { 0 };
static enum BagUiState sState = BAG_STATE_LIST;
static int32_t sActionCursor = 0;
static int32_t sTossQty = 1;
static bool sInShopSell = false;
static int32_t sSellQty = 1;

static CompactBagRow sRows[64];
static int32_t sRowCount = 0;

static void FormatDescriptionTwoLines(const char *src, char *line1, size_t sz1, char *line2, size_t sz2)
{
    line1[0] = '\0';
    line2[0] = '\0';
    if (src == NULL || sz1 == 0 || sz2 == 0) return;

    char clean[256];
    size_t len = 0;
    bool inSpace = false;
    for (size_t i = 0; src[i] != '\0' && len + 1 < sizeof(clean); i++)
    {
        char c = src[i];
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ')
        {
            if (!inSpace && len > 0)
            {
                clean[len++] = ' ';
                inSpace = true;
            }
        }
        else
        {
            clean[len++] = c;
            inSpace = false;
        }
    }
    if (len > 0 && clean[len - 1] == ' ')
        len--;
    clean[len] = '\0';

    const size_t maxPerLine = 48;
    if (len <= maxPerLine)
    {
        snprintf(line1, sz1, "%s", clean);
        return;
    }

    size_t split = maxPerLine;
    while (split > 20 && clean[split] != ' ')
        split--;

    if (clean[split] != ' ')
        split = maxPerLine;

    size_t l1Len = split < sz1 ? split : (sz1 - 1);
    memcpy(line1, clean, l1Len);
    line1[l1Len] = '\0';

    const char *rest = &clean[split];
    while (*rest == ' ') rest++;
    snprintf(line2, sz2, "%s", rest);
}

static void RebuildCompactPocket(void)
{
    sRowCount = 0;

    // Battle Pyramid uses its dedicated 10-slot bag
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        const u16 *const items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
        const u8 *const qtys = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
        for (int32_t i = 0; i < PYRAMID_BAG_ITEMS_COUNT && sRowCount < 64; i++)
        {
            if (items[i] != ITEM_NONE && qtys[i] > 0)
            {
                sRows[sRowCount].itemId = items[i];
                sRows[sRowCount].quantity = qtys[i];
                sRowCount++;
            }
        }
    }
    else
    {
        const struct BagPocket *const bp = &gBagPockets[sPocket];
        for (uint32_t i = 0; i < bp->capacity && sRowCount < 64; i++)
        {
            const uint16_t id = bp->itemSlots[i].itemId;
            const uint16_t qty = (uint16_t)(bp->itemSlots[i].quantity ^ gSaveBlock2Ptr->encryptionKey);
            if (id != ITEM_NONE && qty > 0)
            {
                sRows[sRowCount].itemId = id;
                sRows[sRowCount].quantity = qty;
                sRowCount++;
            }
        }
    }

    if (sCursor[sPocket] >= sRowCount)
        sCursor[sPocket] = (sRowCount > 0) ? (sRowCount - 1) : 0;
    if (sScroll[sPocket] > sCursor[sPocket])
        sScroll[sPocket] = sCursor[sPocket];
}

static void SortCurrentPocket(const enum BagSortMode mode)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return;

    struct BagPocket *const bp = &gBagPockets[sPocket];
    CompactBagRow temp[64];
    int32_t n = 0;
    for (uint32_t i = 0; i < bp->capacity && n < 64; i++)
    {
        const uint16_t id = bp->itemSlots[i].itemId;
        const uint16_t qty = (uint16_t)(bp->itemSlots[i].quantity ^ gSaveBlock2Ptr->encryptionKey);
        if (id != ITEM_NONE && qty > 0)
        {
            temp[n].itemId = id;
            temp[n].quantity = qty;
            n++;
        }
    }

    for (int32_t i = 1; i < n; i++)
    {
        const CompactBagRow key = temp[i];
        int32_t j = i - 1;
        while (j >= 0)
        {
            int32_t cmp = 0;
            if (mode == BAG_SORT_TYPE)
                cmp = (int32_t)temp[j].itemId - (int32_t)key.itemId;
            else if (mode == BAG_SORT_MOST)
                cmp = (int32_t)key.quantity - (int32_t)temp[j].quantity;
            else if (mode == BAG_SORT_LEAST)
                cmp = (int32_t)temp[j].quantity - (int32_t)key.quantity;
            else
            {
                uint8_t g1[24], g2[24];
                char a1[24], a2[24];
                CopyItemName(temp[j].itemId, g1);
                CopyItemName(key.itemId, g2);
                VitaUiArt_GbaToAscii(g1, a1, sizeof(a1));
                VitaUiArt_GbaToAscii(g2, a2, sizeof(a2));
                cmp = strcmp(a1, a2);
            }
            if (cmp <= 0)
                break;
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }

    for (uint32_t i = 0; i < bp->capacity; i++)
    {
        if ((int32_t)i < n)
        {
            bp->itemSlots[i].itemId = temp[i].itemId;
            bp->itemSlots[i].quantity = (uint16_t)(temp[i].quantity ^ gSaveBlock2Ptr->encryptionKey);
        }
        else
        {
            bp->itemSlots[i].itemId = ITEM_NONE;
            bp->itemSlots[i].quantity = (uint16_t)(0U ^ gSaveBlock2Ptr->encryptionKey);
        }
    }
    RebuildCompactPocket();
}

void VitaUiBag_Reset(void)
{
    sState = BAG_STATE_LIST;
    sInShopSell = VitaUI_IsShopSellActive();
    RebuildCompactPocket();
}

void VitaUiBag_SetShopSellMode(const bool inShopSell)
{
    sInShopSell = inShopSell;
    sState = BAG_STATE_LIST;
    if (inShopSell)
    {
        if (sPocket == (POCKET_KEY_ITEMS - 1))
            sPocket = 0;
        RebuildCompactPocket();
    }
}

bool VitaUiBag_IsShopSellMode(void)
{
    return sInShopSell;
}

bool VitaUiBag_HasActiveModal(void)
{
    return sState != BAG_STATE_LIST;
}

static void ExecuteUseItemAutonomously(const uint16_t itemId)
{
    // Repels
    if (itemId == ITEM_REPEL || itemId == ITEM_SUPER_REPEL || itemId == ITEM_MAX_REPEL)
    {
        if (VarGet(VAR_REPEL_STEP_COUNT) > 0)
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("REPEL STILL IN EFFECT");
            return;
        }
        VarSet(VAR_REPEL_STEP_COUNT, GetItemHoldEffectParam(itemId));
        VarSet(VAR_LAST_REPEL, itemId);
        RemoveBagItem(itemId, 1);
        RebuildCompactPocket();
        PlaySE(SE_REPEL);
        VitaUi_ShowToast("REPEL SPRAYED!");
        return;
    }

    // Escape Rope
    if (itemId == ITEM_ESCAPE_ROPE)
    {
        if (!CanUseDigOrEscapeRopeOnCurMap())
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("CANT USE ESCAPE ROPE HERE");
            return;
        }
        RemoveBagItem(itemId, 1);
        PlaySE(SE_SELECT);
        VitaUI_Close();
        gSpecialVar_ItemId = ITEM_ESCAPE_ROPE;
        CreateTask(Task_UseDigEscapeRopeOnField, 8);
        return;
    }

    // Acro / Mach Bike
    if (itemId == ITEM_MACH_BIKE || itemId == ITEM_ACRO_BIKE)
    {
        if (!Overworld_IsBikingAllowed() || IsBikingDisallowedByPlayer())
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("CANT RIDE BIKE HERE");
            return;
        }
        PlaySE(SE_SELECT);
        VitaUI_Close();
        GetOnOffBike(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE);
        return;
    }

    // Sacred Ash heals entire party directly
    if (itemId == ITEM_SACRED_ASH)
    {
        bool healedAny = false;
        const int32_t cnt = (int32_t)CalculatePlayerPartyCount();
        for (int32_t i = 0; i < cnt; i++)
        {
            struct Pokemon *const mon = &gPlayerParty[i];
            if (GetMonData(mon, MON_DATA_IS_EGG) == 0 && GetMonData(mon, MON_DATA_HP) == 0)
            {
                const uint16_t maxHp = (uint16_t)GetMonData(mon, MON_DATA_MAX_HP);
                const uint32_t zero = 0;
                SetMonData(mon, MON_DATA_HP, &maxHp);
                SetMonData(mon, MON_DATA_STATUS, &zero);
                healedAny = true;
            }
        }
        if (!healedAny)
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("NO FAINTED POKEMON");
            return;
        }
        RemoveBagItem(itemId, 1);
        RebuildCompactPocket();
        PlaySE(SE_USE_ITEM);
        VitaUi_ShowToast("SACRED ASH REVIVED ALL!");
        return;
    }

    // TM / HM, Medicine, Vitamins, Rare Candy, Berries, Evo Stones -> Autonomous Party Target
    const ItemUseFunc fieldFn = GetItemFieldFunc(itemId);
    if (sPocket == (POCKET_TM_HM - 1)
        || fieldFn == ItemUseOutOfBattle_Medicine
        || fieldFn == ItemUseOutOfBattle_RareCandy
        || fieldFn == ItemUseOutOfBattle_PPUp
        || fieldFn == ItemUseOutOfBattle_PPRecovery
        || fieldFn == ItemUseOutOfBattle_ReduceEV
        || fieldFn == ItemUseOutOfBattle_EvolutionStone
        || fieldFn == ItemUseOutOfBattle_Berry)
    {
        PlaySE(SE_SELECT);
        sState = BAG_STATE_LIST;
        VitaUI_OpenPartyForBagItem(itemId);
        return;
    }

    // Coin Case quick display
    if (itemId == ITEM_COIN_CASE)
    {
        char msg[32];
        snprintf(msg, sizeof(msg), "COINS: %u", (uint32_t)GetCoins());
        PlaySE(SE_SELECT);
        VitaUi_ShowToast(msg);
        return;
    }

    // Fishing Rods / Itemfinder / Pokeblock Case / Wailmer Pail
    if (fieldFn != NULL && fieldFn != ItemUseOutOfBattle_CannotUse)
    {
        PlaySE(SE_SELECT);
        VitaUI_Close();
        gSpecialVar_ItemId = itemId;
        const u8 tid = CreateTask(fieldFn, 8);
        gTasks[tid].data[3] = 1; // Executed from field shortcut / bottom screen
        return;
    }

    PlaySE(SE_FAILURE);
    VitaUi_ShowToast("CANT USE THAT RIGHT NOW");
}

bool VitaUiBag_Update(const VitaUiInput *in)
{
    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);

    if (sState == BAG_STATE_SELL_QTY)
    {
        if (sRowCount <= 0)
        {
            sState = BAG_STATE_LIST;
            return true;
        }
        const uint16_t itemId = sRows[sCursor[sPocket]].itemId;
        const int32_t maxQty = (int32_t)sRows[sCursor[sPocket]].quantity;
        if (in->newButtons & VITA_BTN_UP)
        {
            sSellQty++;
            if (sSellQty > maxQty) sSellQty = 1;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sSellQty--;
            if (sSellQty < 1) sSellQty = maxQty;
        }
        if (in->newButtons & VITA_BTN_RIGHT)
        {
            sSellQty += 10;
            if (sSellQty > maxQty) sSellQty = maxQty;
        }
        if (in->newButtons & VITA_BTN_LEFT)
        {
            sSellQty -= 10;
            if (sSellQty < 1) sSellQty = 1;
        }

        if (VitaUi_HitRect(in, 170, 114, 30, 20))
        {
            sSellQty -= 10;
            if (sSellQty < 1) sSellQty = 1;
            PlaySE(SE_SELECT);
            return true;
        }
        if (VitaUi_HitRect(in, 280, 114, 30, 20))
        {
            sSellQty += 10;
            if (sSellQty > maxQty) sSellQty = maxQty;
            PlaySE(SE_SELECT);
            return true;
        }

        if (in->newButtons & aBtn)
        {
            const uint32_t unitPrice = (uint32_t)(GetItemPrice(itemId) / 2);
            const uint32_t totalPrice = unitPrice * (uint32_t)sSellQty;
            AddMoney(&gSaveBlock1Ptr->money, totalPrice);
            RemoveBagItem(itemId, (u16)sSellQty);
            RebuildCompactPocket();
            sState = BAG_STATE_LIST;
            PlaySE(SE_SHOP);
            char toastMsg[36];
            snprintf(toastMsg, sizeof(toastMsg), "SOLD FOR $%u!", (uint32_t)totalPrice);
            VitaUi_ShowToast(toastMsg);
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sState = BAG_STATE_LIST;
            PlaySE(SE_SELECT);
            return true;
        }
        return (in->newButtons != 0);
    }

    if (sState == BAG_STATE_TOSS_QTY)
    {
        if (sRowCount <= 0)
        {
            sState = BAG_STATE_LIST;
            return true;
        }
        const int32_t maxQty = (int32_t)sRows[sCursor[sPocket]].quantity;
        if (in->newButtons & VITA_BTN_UP)    sTossQty++;
        if (in->newButtons & VITA_BTN_DOWN)  sTossQty--;
        if (in->newButtons & VITA_BTN_RIGHT) sTossQty += 10;
        if (in->newButtons & VITA_BTN_LEFT)  sTossQty -= 10;
        if (sTossQty < 1) sTossQty = 1;
        if (sTossQty > maxQty) sTossQty = maxQty;

        if (in->newButtons & aBtn)
        {
            RemoveBagItem(sRows[sCursor[sPocket]].itemId, (u16)sTossQty);
            RebuildCompactPocket();
            sState = BAG_STATE_LIST;
            PlaySE(SE_SELECT);
            VitaUi_ShowToast("THREW AWAY ITEM(S)");
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sState = BAG_STATE_ACTION;
            PlaySE(SE_SELECT);
            return true;
        }
        return (in->newButtons != 0);
    }

    if (sState == BAG_STATE_ACTION)
    {
        if (sRowCount <= 0)
        {
            sState = BAG_STATE_LIST;
            return true;
        }
        const uint16_t itemId = sRows[sCursor[sPocket]].itemId;
        if (gMain.inBattle)
        {
            const int32_t actionCount = 2;
            if (in->newButtons & VITA_BTN_UP)
            {
                sActionCursor = (sActionCursor <= 0) ? (actionCount - 1) : (sActionCursor - 1);
                PlaySE(SE_SELECT);
                return true;
            }
            if (in->newButtons & VITA_BTN_DOWN)
            {
                sActionCursor = (sActionCursor + 1) % actionCount;
                PlaySE(SE_SELECT);
                return true;
            }
            for (int32_t i = 0; i < actionCount; i++)
            {
                if (VitaUi_HitRect(in, 206, (162 - (actionCount * 20 + 8)) + 4 + i * 20, 104, 19))
                {
                    sActionCursor = i;
                    goto run_battle_action;
                }
            }
            if (in->newButtons & aBtn)
            {
            run_battle_action:
                if (sActionCursor == 0)
                {
                    sState = BAG_STATE_LIST;
                    if (GetItemBattleUsage(itemId) == 0)
                    {
                        PlaySE(SE_FAILURE);
                        VitaUi_ShowToast("CANT USE THAT HERE");
                        return true;
                    }

                    const ItemUseFunc fieldFn = GetItemFieldFunc(itemId);
                    if (fieldFn == ItemUseOutOfBattle_Medicine
                        || fieldFn == ItemUseOutOfBattle_PPRecovery
                        || fieldFn == ItemUseOutOfBattle_PPUp
                        || fieldFn == ItemUseOutOfBattle_Berry)
                    {
                        PlaySE(SE_SELECT);
                        VitaUI_OpenPartyForBagItem(itemId);
                        return true;
                    }

                    PlaySE(SE_SELECT);
                    gSpecialVar_ItemId = itemId;
                    VitaUI_Close();
                    return true;
                }
                else
                {
                    sState = BAG_STATE_LIST;
                    PlaySE(SE_SELECT);
                    return true;
                }
            }
            if (in->newButtons & bBtn)
            {
                sState = BAG_STATE_LIST;
                PlaySE(SE_SELECT);
                return true;
            }
            return false;
        }

        if (sInShopSell)
        {
            const int32_t actionCount = 2;
            if (in->newButtons & VITA_BTN_UP)
            {
                sActionCursor = (sActionCursor <= 0) ? (actionCount - 1) : (sActionCursor - 1);
                PlaySE(SE_SELECT);
                return true;
            }
            if (in->newButtons & VITA_BTN_DOWN)
            {
                sActionCursor = (sActionCursor + 1) % actionCount;
                PlaySE(SE_SELECT);
                return true;
            }
            for (int32_t i = 0; i < actionCount; i++)
            {
                if (VitaUi_HitRect(in, 206, (162 - (actionCount * 20 + 8)) + 4 + i * 20, 104, 19))
                {
                    sActionCursor = i;
                    goto run_sell_action;
                }
            }
            if (in->newButtons & aBtn)
            {
            run_sell_action:
                if (sActionCursor == 0) // SELL
                {
                    const uint32_t price = GetItemPrice(itemId);
                    const bool isKey = (sPocket == (POCKET_KEY_ITEMS - 1));
                    const bool isHm = (sPocket == (POCKET_TM_HM - 1) && IsMoveHm(ItemIdToBattleMoveId(itemId)));

                    if (isKey || isHm || price == 0)
                    {
                        PlaySE(SE_FAILURE);
                        VitaUi_ShowToast("CANT SELL THAT ITEM!");
                        sState = BAG_STATE_LIST;
                        return true;
                    }

                    const int32_t maxQty = (int32_t)sRows[sCursor[sPocket]].quantity;
                    if (maxQty == 1)
                    {
                        const uint32_t unitPrice = (uint32_t)(price / 2);
                        AddMoney(&gSaveBlock1Ptr->money, unitPrice);
                        RemoveBagItem(itemId, 1);
                        RebuildCompactPocket();
                        sState = BAG_STATE_LIST;
                        PlaySE(SE_SHOP);
                        char toastMsg[36];
                        snprintf(toastMsg, sizeof(toastMsg), "SOLD FOR $%u!", (uint32_t)unitPrice);
                        VitaUi_ShowToast(toastMsg);
                        return true;
                    }
                    else
                    {
                        sSellQty = 1;
                        sState = BAG_STATE_SELL_QTY;
                        PlaySE(SE_SELECT);
                        return true;
                    }
                }
                else // CANCEL
                {
                    sState = BAG_STATE_LIST;
                    PlaySE(SE_SELECT);
                    return true;
                }
            }
            if (in->newButtons & bBtn)
            {
                sState = BAG_STATE_LIST;
                PlaySE(SE_SELECT);
                return true;
            }
            return false;
        }

        const bool isKey = (sPocket == (POCKET_KEY_ITEMS - 1));
        const bool isTmHm = (sPocket == (POCKET_TM_HM - 1));
        const int32_t actionCount = isKey ? 3 : (isTmHm ? 3 : 4);

        if (in->newButtons & VITA_BTN_UP)
        {
            sActionCursor = (sActionCursor <= 0) ? (actionCount - 1) : (sActionCursor - 1);
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sActionCursor = (sActionCursor + 1) % actionCount;
            PlaySE(SE_SELECT);
            return true;
        }
        for (int32_t i = 0; i < actionCount; i++)
        {
            if (VitaUi_HitRect(in, 206, 86 + i * 20, 104, 19))
            {
                sActionCursor = i;
                goto run_action;
            }
        }
        if (in->newButtons & aBtn)
        {
        run_action:
            if (sActionCursor == 0)
            {
                sState = BAG_STATE_LIST;
                ExecuteUseItemAutonomously(itemId);
                return true;
            }
            if (isKey && sActionCursor == 1)
            {
                gSaveBlock1Ptr->registeredItem = (gSaveBlock1Ptr->registeredItem == itemId) ? ITEM_NONE : itemId;
                sState = BAG_STATE_LIST;
                PlaySE(SE_SELECT);
                VitaUi_ShowToast(gSaveBlock1Ptr->registeredItem != ITEM_NONE ? "REGISTERED TO [Y/SEL]!" : "UNREGISTERED ITEM");
                return true;
            }
            if (!isKey && sActionCursor == 1)
            {
                if (IsMoveHm(ItemIdToBattleMoveId(itemId)))
                {
                    PlaySE(SE_FAILURE);
                    VitaUi_ShowToast("HMS CANNOT BE HELD");
                    return true;
                }
                sState = BAG_STATE_LIST;
                PlaySE(SE_SELECT);
                VitaUI_OpenPartyForBagItem((uint16_t)(itemId | 0x8000U));
                return true;
            }
            if (!isKey && !isTmHm && sActionCursor == 2)
            {
                sTossQty = 1;
                sState = BAG_STATE_TOSS_QTY;
                PlaySE(SE_SELECT);
                return true;
            }
            sState = BAG_STATE_LIST;
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sState = BAG_STATE_LIST;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    // Pocket switching (LEFT / RIGHT or pocket pills)
    bool changed = false;
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    {
        if (in->newButtons & VITA_BTN_LEFT)
        {
            sPocket = (sPocket == 0) ? 4U : (uint8_t)(sPocket - 1U);
            RebuildCompactPocket();
            PlaySE(SE_SELECT);
            changed = true;
        }
        else if (in->newButtons & VITA_BTN_RIGHT)
        {
            sPocket = (uint8_t)((sPocket + 1U) % 5U);
            RebuildCompactPocket();
            PlaySE(SE_SELECT);
            changed = true;
        }
        for (uint8_t p = 0; p < 5; p++)
        {
            if (VitaUi_HitRect(in, 96 + (int32_t)p * 44, 26, 42, 15))
            {
                sPocket = p;
                RebuildCompactPocket();
                PlaySE(SE_SELECT);
                changed = true;
            }
        }
    }

    // List up/down
    if (sRowCount > 0)
    {
        int32_t cur = sCursor[sPocket];
        if ((in->newButtons & VITA_BTN_UP) && cur > 0)
        {
            cur--;
            PlaySE(SE_SELECT);
            changed = true;
        }
        else if ((in->newButtons & VITA_BTN_DOWN) && cur + 1 < sRowCount)
        {
            cur++;
            PlaySE(SE_SELECT);
            changed = true;
        }
        sCursor[sPocket] = cur;
        if (cur < sScroll[sPocket]) sScroll[sPocket] = cur;
        if (cur >= sScroll[sPocket] + 6) sScroll[sPocket] = cur - 5;

        for (int32_t r = 0; r < 6; r++)
        {
            if (VitaUi_HitRect(in, 98, 46 + r * 20, 212, 19) && (sScroll[sPocket] + r) < sRowCount)
            {
                if (sCursor[sPocket] == sScroll[sPocket] + r)
                {
                    sActionCursor = 0;
                    sState = BAG_STATE_ACTION;
                }
                else
                {
                    sCursor[sPocket] = sScroll[sPocket] + r;
                }
                PlaySE(SE_SELECT);
                return true;
            }
        }

        if (in->newButtons & aBtn)
        {
            sActionCursor = 0;
            sState = BAG_STATE_ACTION;
            PlaySE(SE_SELECT);
            return true;
        }
    }

    // Sort pocket with TRIANGLE
    if (in->newButtons & VITA_BTN_TRIANGLE)
    {
        static uint8_t sNextSort = 0;
        SortCurrentPocket((enum BagSortMode)sNextSort);
        sNextSort = (uint8_t)((sNextSort + 1U) % BAG_SORT_COUNT);
        PlaySE(SE_SELECT);
        VitaUi_ShowToast("POCKET SORTED!");
        return true;
    }

    if (in->newButtons & bBtn)
    {
        if (gMain.inBattle)
        {
            gSpecialVar_ItemId = ITEM_NONE;
            PlaySE(SE_WIN_OPEN);
            VitaUI_Close();
            return true;
        }
        if (sInShopSell)
        {
            PlaySE(SE_WIN_OPEN);
            VitaUI_Close();
            return true;
        }
    }

    return changed;
}

static void FormatItemDisplayName(const uint16_t itemId, char *out, const size_t cap)
{
    uint8_t gbaName[32];
    CopyItemName(itemId, gbaName);
    char base[28];
    VitaUiArt_GbaToAscii(gbaName, base, sizeof(base));

    const uint16_t mv = ItemIdToBattleMoveId(itemId);
    if (mv != MOVE_NONE)
    {
        char mvAsc[20];
        VitaUiArt_GbaToAscii(gMoveNames[mv], mvAsc, sizeof(mvAsc));
        snprintf(out, cap, "%s %s", base, mvAsc);
    }
    else
    {
        snprintf(out, cap, "%s", base);
    }
}

void VitaUiBag_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase)
{
    const VitaUiClip *const clip = VitaUi_FullClip();

    // Left Bag Showcase + Selected Item Icon
    VitaUiPlate_Draw(dst, pitch, clip, 6, 26, 86, 138, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 14);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, sInShopSell ? "MART SELL" : "HOENN BAG", 49, 28, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    VitaUiArt_DrawBagPic(dst, pitch, clip, gSaveBlock2Ptr->playerGender == FEMALE, (uint8_t)(sPocket + 1U), 17, 46);

    if (sRowCount > 0)
    {
        const uint16_t selItem = sRows[sCursor[sPocket]].itemId;
        VitaUiPlate_Draw(dst, pitch, clip, 31, 118, 36, 36, 3, kPlateCardSel, VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawItemIcon(dst, pitch, clip, selItem, 37, 124, false);
    }

    // Pocket tabs on right
    static const char *const sPocketTabs[5] = { "ITEMS", "BALLS", "TM/HM", "BERRY", "KEY" };
    for (uint8_t p = 0; p < 5; p++)
    {
        const bool sel = (sPocket == p);
        VitaUiPlate_Draw(
            dst, pitch, clip,
            96 + (int32_t)p * 44, 26, 42, 15, 2,
            sel ? kPlateSage : kPlateSageDark,
            sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL_NARROW, sPocketTabs[p],
            96 + (int32_t)p * 44, 26, 42, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    // Compact Item List
    VitaUiPlate_Draw(dst, pitch, clip, 96, 43, 218, 121, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);

    if (sRowCount == 0)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "POCKET IS EMPTY", 205, 96, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    }
    else
    {
        for (int32_t r = 0; r < 6 && (sScroll[sPocket] + r) < sRowCount; r++)
        {
            const int32_t idx = sScroll[sPocket] + r;
            const CompactBagRow *const row = &sRows[idx];
            const int32_t ry = 46 + r * 19;
            const bool isSel = (idx == sCursor[sPocket]);

            if (isSel)
                VitaUiPlate_Draw(dst, pitch, clip, 100, ry, 210, 18, 3, kPlateCardSel, VITA_PLATE_FOCUS, blinkPhase, 0);

            char label[44];
            FormatItemDisplayName(row->itemId, label, sizeof(label));
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, label, 106, ry + 2, UI_INK_DARK, VITA_UI_NO_SHADOW);

            if (sInShopSell)
            {
                const uint32_t unitPrice = (uint32_t)(GetItemPrice(row->itemId) / 2);
                if (unitPrice > 0 && sPocket != (POCKET_KEY_ITEMS - 1))
                {
                    char qStr[20];
                    snprintf(qStr, sizeof(qStr), "$%u  x%u", (uint32_t)unitPrice, (uint32_t)row->quantity);
                    VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, qStr, 304, ry + 2, UI_INK_DARK, VITA_UI_NO_SHADOW);
                }
                else
                {
                    VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "---", 304, ry + 2, UI_INK_DARK, VITA_UI_NO_SHADOW);
                }
            }
            else if (sPocket == (POCKET_KEY_ITEMS - 1))
            {
                if (gSaveBlock1Ptr->registeredItem == row->itemId)
                    VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "[REG]", 304, ry + 2, UI_INK_BLUE, VITA_UI_NO_SHADOW);
            }
            else if (!(sPocket == (POCKET_TM_HM - 1) && IsMoveHm(ItemIdToBattleMoveId(row->itemId))))
            {
                char qStr[12];
                snprintf(qStr, sizeof(qStr), "x%u", row->quantity);
                VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, qStr, 304, ry + 2, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            }
        }
    }

    // Bottom full-width item description bar
    VitaUiPlate_Draw(dst, pitch, clip, 6, 166, 308, 40, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
    if (sRowCount > 0)
    {
        const uint8_t *const desc = GetItemDescription(sRows[sCursor[sPocket]].itemId);
        if (desc != NULL)
        {
            char rawAsc[256];
            VitaUiArt_GbaToAscii(desc, rawAsc, sizeof(rawAsc));
            char line1[64], line2[64];
            FormatDescriptionTwoLines(rawAsc, line1, sizeof(line1), line2, sizeof(line2));
            if (line2[0] == '\0')
            {
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, line1, 12, 179, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
            else
            {
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, line1, 12, 172, UI_INK_DARK, VITA_UI_NO_SHADOW);
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, line2, 12, 186, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
        }
    }
    else
    {
        if (sInShopSell)
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "LEFT/RIGHT: SWITCH POCKET   B: EXIT TO SHOP", 12, 180, UI_INK_DARK, VITA_UI_NO_SHADOW);
        else
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "TRIANGLE: SORT POCKET   LEFT/RIGHT: SWITCH POCKET", 12, 180, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }

    // Action / Toss / Sell modals
    if (sState == BAG_STATE_ACTION && sRowCount > 0)
    {
        const bool isKey = (sPocket == (POCKET_KEY_ITEMS - 1));
        const bool isTmHm = (sPocket == (POCKET_TM_HM - 1));
        const char *acts[4];
        int32_t cnt;
        if (gMain.inBattle)
        {
            acts[0] = "USE";
            acts[1] = "CANCEL";
            cnt = 2;
        }
        else if (sInShopSell)
        {
            acts[0] = "SELL";
            acts[1] = "CANCEL";
            cnt = 2;
        }
        else
        {
            acts[0] = "USE";
            acts[1] = isKey ? "REGISTER" : "GIVE";
            acts[2] = (isKey || isTmHm) ? "CANCEL" : "TOSS";
            acts[3] = "CANCEL";
            cnt = (isKey || isTmHm) ? 3 : 4;
        }
        const int32_t ph = cnt * 20 + 8;
        const int32_t py = 162 - ph;

        if (sInShopSell)
        {
            VitaUiPlate_Draw(dst, pitch, clip, 202, py, 112, ph, 4, kPlateCream, VITA_PLATE_FOCUS, blinkPhase, 0);
            for (int32_t i = 0; i < cnt; i++)
            {
                const bool aSel = (i == sActionCursor);
                VitaUiPlate_Draw(
                    dst, pitch, clip, 206, py + 4 + i * 20, 104, 18, 3,
                    aSel ? kPlateCardSel : kPlateCard,
                    aSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
                VitaUiArt_DrawStrAsciiInPlate(
                    dst, pitch, clip, VITA_FONT_SMALL, acts[i],
                    206, py + 4 + i * 20, 104, 18, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
        }
        else
        {
            VitaUiPlate_Draw(dst, pitch, clip, 202, py, 112, ph, 4, kPlatePopup, VITA_PLATE_NORMAL, blinkPhase, 0);
            for (int32_t i = 0; i < cnt; i++)
            {
                const bool aSel = (i == sActionCursor);
                VitaUiPlate_Draw(
                    dst, pitch, clip, 206, py + 4 + i * 20, 104, 18, 3,
                    aSel ? kPlateSage : kPlateSageDark,
                    aSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
                VitaUiArt_DrawStrAsciiInPlate(
                    dst, pitch, clip, VITA_FONT_SMALL, acts[i],
                    206, py + 4 + i * 20, 104, 18, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            }
        }
    }
    else if (sState == BAG_STATE_TOSS_QTY && sRowCount > 0)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 184, 108, 130, 54, 4, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 15);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "TOSS HOW MANY?", 249, 110, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        char qBuf[24];
        snprintf(qBuf, sizeof(qBuf), "< -10  x%02d  +10 >", (int32_t)sTossQty);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, qBuf, 249, 134, UI_INK_MINT, VITA_UI_NO_SHADOW);
    }
    else if (sState == BAG_STATE_SELL_QTY && sRowCount > 0)
    {
        const uint16_t itemId = sRows[sCursor[sPocket]].itemId;
        const uint32_t unitPrice = (uint32_t)(GetItemPrice(itemId) / 2);
        const uint32_t totalPrice = unitPrice * (uint32_t)sSellQty;

        VitaUiPlate_Draw(dst, pitch, clip, 166, 92, 150, 72, 4, kPlateCream, VITA_PLATE_FOCUS, blinkPhase, 16);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "SELL HOW MANY?", 241, 96, UI_INK_DARK, VITA_UI_NO_SHADOW);
        VitaUiPlate_Draw(dst, pitch, clip, 172, 112, 138, 22, 3, kPlateCardSel, VITA_PLATE_NORMAL, blinkPhase, 0);
        char qBuf[24];
        snprintf(qBuf, sizeof(qBuf), "< -10  x%02d  +10 >", (int32_t)sSellQty);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, qBuf, 241, 116, UI_INK_DARK, VITA_UI_NO_SHADOW);
        char payBuf[32];
        snprintf(payBuf, sizeof(payBuf), "PAYOUT: $%u", (uint32_t)totalPrice);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, payBuf, 241, 142, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }
}
