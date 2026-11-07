// Bag and item actions of the new UI (Core 1). The game-side behaviour is the one of the current
// Vita UI (src/platform/vita/vita_ui_bag.c ExecuteUseItemAutonomously and vita_ui_party.c
// ApplyBagItemToMon), which stays selectable while the new UI is built.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/vita_extras.h"

#include "global.h"
#include "battle_pyramid.h"
#include "bike.h"
#include "coins.h"
#include "data.h"
#include "event_data.h"
#include "evolution_scene.h"
#include "field_player_avatar.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "task.h"
#include "constants/battle_frontier.h"
#include "constants/item.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/vars.h"

// ---------- Pockets ----------

static bool InPyramid(void)
{
    return CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE;
}

int32_t UiGame_PocketItems(int32_t pocket, UiBagRow *rows, int32_t max)
{
    int32_t n = 0;
    if (InPyramid())
    {
        // The Battle Pyramid has its own 10-slot bag (shown in the ITEMS pocket)
        if (pocket != ITEMS_POCKET)
            return 0;
        const u16 *items = gSaveBlock2Ptr->frontier.pyramidBag.itemId[gSaveBlock2Ptr->frontier.lvlMode];
        const u8 *qty = gSaveBlock2Ptr->frontier.pyramidBag.quantity[gSaveBlock2Ptr->frontier.lvlMode];
        for (int32_t i = 0; i < PYRAMID_BAG_ITEMS_COUNT && n < max; i++)
            if (items[i] != ITEM_NONE && qty[i] > 0)
                rows[n++] = (UiBagRow){ items[i], qty[i] };
        return n;
    }
    const struct BagPocket *bp = &gBagPockets[pocket];
    for (uint32_t i = 0; i < bp->capacity && n < max; i++)
    {
        const uint16_t id = bp->itemSlots[i].itemId;
        const uint16_t q = (uint16_t)(bp->itemSlots[i].quantity ^ gSaveBlock2Ptr->encryptionKey);
        if (id != ITEM_NONE && q > 0)
            rows[n++] = (UiBagRow){ id, q };
    }
    return n;
}

void UiGame_SortPocket(int32_t pocket, int32_t mode)
{
    if (InPyramid())
        return;
    struct BagPocket *bp = &gBagPockets[pocket];
    UiBagRow rows[64];
    const int32_t n = UiGame_PocketItems(pocket, rows, 64);
    char na[24], nb[24];
    for (int32_t i = 1; i < n; i++)
    {
        const UiBagRow key = rows[i];
        int32_t j = i - 1;
        while (j >= 0)
        {
            int32_t cmp;
            if (mode == UI_SORT_NUMBER) cmp = (int32_t)rows[j].item - (int32_t)key.item;
            else if (mode == UI_SORT_MOST) cmp = (int32_t)key.qty - (int32_t)rows[j].qty;
            else if (mode == UI_SORT_LEAST) cmp = (int32_t)rows[j].qty - (int32_t)key.qty;
            else
            {
                UiGame_ItemName(rows[j].item, na, sizeof(na));
                UiGame_ItemName(key.item, nb, sizeof(nb));
                cmp = strcmp(na, nb);
            }
            if (cmp <= 0)
                break;
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = key;
    }
    for (uint32_t i = 0; i < bp->capacity; i++)
    {
        bp->itemSlots[i].itemId = (int32_t)i < n ? rows[i].item : ITEM_NONE;
        bp->itemSlots[i].quantity = (uint16_t)(((int32_t)i < n ? rows[i].qty : 0) ^ gSaveBlock2Ptr->encryptionKey);
    }
}

void UiGame_ItemDescription(uint16_t itemId, char *out, size_t cap)
{
    UiGame_Text(GetItemDescription(itemId), out, cap);
}

int32_t UiGame_ItemPocket(uint16_t itemId)
{
    return (int32_t)GetPocketByItemId(itemId) - 1;   // POCKET_* (1-based) -> *_POCKET
}

bool UiGame_ItemIsTmHm(uint16_t itemId)
{
    return GetPocketByItemId(itemId) == POCKET_TM_HM;
}

uint16_t UiGame_ItemMove(uint16_t itemId)
{
    return ItemIdToBattleMoveId(itemId);
}

bool UiGame_CanToss(uint16_t itemId)
{
    return GetItemImportance(itemId) == 0 && !InPyramid();
}

bool UiGame_CanRegister(uint16_t itemId)
{
    const ItemUseFunc fn = GetItemFieldFunc(itemId);
    return GetPocketByItemId(itemId) == POCKET_KEY_ITEMS && fn != NULL && fn != ItemUseOutOfBattle_CannotUse;
}

bool UiGame_CanGive(uint16_t itemId)
{
    return GetItemImportance(itemId) == 0 && !InPyramid();
}

void UiGame_Toss(uint16_t itemId, uint16_t n)
{
    RemoveBagItem(itemId, n);
}

void UiGame_Register(uint16_t itemId)
{
    gSaveBlock1Ptr->registeredItem = (gSaveBlock1Ptr->registeredItem == itemId) ? ITEM_NONE : itemId;
}

// ---------- Using an item from the bag in the field ----------

static uint16_t sAfterItem;

static void AfterClose_EscapeRope(void)
{
    gSpecialVar_ItemId = ITEM_ESCAPE_ROPE;
    CreateTask(Task_UseDigEscapeRopeOnField, 8);
}

static void AfterClose_Bike(void)
{
    GetOnOffBike(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE);
}

static void AfterClose_FieldFunc(void)
{
    gSpecialVar_ItemId = sAfterItem;
    const u8 tid = CreateTask(GetItemFieldFunc(sAfterItem), 8);
    gTasks[tid].data[3] = 1;   // used from the field shortcut, as the current Vita UI does
}

UiUseResult UiGame_UseFromBag(uint16_t itemId, char *msg, size_t cap, void (**afterClose)(void))
{
    *afterClose = NULL;
    msg[0] = 0;
    if (itemId == ITEM_REPEL || itemId == ITEM_SUPER_REPEL || itemId == ITEM_MAX_REPEL)
    {
        if (VarGet(VAR_REPEL_STEP_COUNT) > 0) { snprintf(msg, cap, "The REPEL is still in effect."); return UI_USE_FAIL; }
        VarSet(VAR_REPEL_STEP_COUNT, GetItemHoldEffectParam(itemId));
        VarSet(VAR_LAST_REPEL, itemId);
        RemoveBagItem(itemId, 1);
        snprintf(msg, cap, "The REPEL was sprayed!");
        return UI_USE_DONE;
    }
    if (itemId == ITEM_ESCAPE_ROPE)
    {
        if (!CanUseDigOrEscapeRopeOnCurMap()) { snprintf(msg, cap, "It can't be used here."); return UI_USE_FAIL; }
        RemoveBagItem(itemId, 1);
        *afterClose = AfterClose_EscapeRope;
        return UI_USE_CLOSE;
    }
    if (itemId == ITEM_MACH_BIKE || itemId == ITEM_ACRO_BIKE)
    {
        if (!Overworld_IsBikingAllowed() || IsBikingDisallowedByPlayer()) { snprintf(msg, cap, "You can't ride a BIKE here."); return UI_USE_FAIL; }
        *afterClose = AfterClose_Bike;
        return UI_USE_CLOSE;
    }
    if (itemId == ITEM_SACRED_ASH)
    {
        bool any = false;
        for (int32_t i = 0; i < UiGame_PartyCount(); i++)
        {
            struct Pokemon *mon = &gPlayerParty[i];
            if (GetMonData(mon, MON_DATA_IS_EGG) == 0 && GetMonData(mon, MON_DATA_HP) == 0)
            {
                const uint16_t maxHp = (uint16_t)GetMonData(mon, MON_DATA_MAX_HP);
                const uint32_t zero = 0;
                SetMonData(mon, MON_DATA_HP, &maxHp);
                SetMonData(mon, MON_DATA_STATUS, &zero);
                any = true;
            }
        }
        if (!any) { snprintf(msg, cap, "It won't have any effect."); return UI_USE_FAIL; }
        RemoveBagItem(itemId, 1);
        snprintf(msg, cap, "All fainted POKéMON were revived!");
        return UI_USE_DONE;
    }
    const ItemUseFunc fn = GetItemFieldFunc(itemId);
    if (GetPocketByItemId(itemId) == POCKET_TM_HM || fn == ItemUseOutOfBattle_Medicine || fn == ItemUseOutOfBattle_RareCandy
        || fn == ItemUseOutOfBattle_PPUp || fn == ItemUseOutOfBattle_PPRecovery || fn == ItemUseOutOfBattle_ReduceEV
        || fn == ItemUseOutOfBattle_EvolutionStone || fn == ItemUseOutOfBattle_Berry)
        return UI_USE_TARGET;
    if (itemId == ITEM_COIN_CASE)
    {
        snprintf(msg, cap, "Your COINS: %u", (unsigned)GetCoins());
        return UI_USE_DONE;
    }
    if (fn != NULL && fn != ItemUseOutOfBattle_CannotUse)
    {
        sAfterItem = itemId;
        *afterClose = AfterClose_FieldFunc;
        return UI_USE_CLOSE;
    }
    snprintf(msg, cap, "It can't be used right now.");
    return UI_USE_FAIL;
}

// ---------- Item on a party POKéMON ----------

static void Consume(uint16_t item)
{
    const uint16_t move = ItemIdToBattleMoveId(item);
    if (move != MOVE_NONE && (IsMoveHm(move) || VitaExtras_GetFast(FAST_EXTRA_INFINITE_TMS) != 0))
        return;                     // HMs, and TMs with the infinite TMs extra, are not used up
    RemoveBagItem(item, 1);
}

static void Stats(struct Pokemon *mon, int16_t out[6], uint8_t *lv)
{
    static const u8 ids[6] = { MON_DATA_MAX_HP, MON_DATA_ATK, MON_DATA_DEF, MON_DATA_SPATK, MON_DATA_SPDEF, MON_DATA_SPEED };
    for (int32_t i = 0; i < 6; i++)
        out[i] = (int16_t)GetMonData(mon, ids[i]);
    *lv = (uint8_t)GetMonData(mon, MON_DATA_LEVEL);
}

static uint8_t sEvoSlot;
static uint16_t sEvoSpecies;

static void AfterClose_Evolve(void)
{
    CleanupOverworldWindowsAndTilemaps();
    gCB2_AfterEvolution = CB2_ReturnToField;
    BeginEvolutionScene(&gPlayerParty[sEvoSlot], sEvoSpecies, FALSE, sEvoSlot);
}

UiItemResult UiGame_ItemOnMon(int32_t slot, uint16_t item, UiStatDelta *delta, char *msg, size_t cap, void (**afterClose)(void))
{
    struct Pokemon *mon = &gPlayerParty[slot];
    *afterClose = NULL;
    msg[0] = 0;
    if (GetMonData(mon, MON_DATA_IS_EGG) != 0) { snprintf(msg, cap, "It can't be used on an EGG."); return UI_ITEM_FAIL; }

    const uint16_t move = ItemIdToBattleMoveId(item);
    if (GetPocketByItemId(item) == POCKET_TM_HM && move != MOVE_NONE)
    {
        if (MonKnowsMove(mon, move)) { snprintf(msg, cap, "It already knows that move."); return UI_ITEM_FAIL; }
        if (!CanMonLearnTMHM(mon, (u8)(item - ITEM_TM01))) { snprintf(msg, cap, "It can't learn that move."); return UI_ITEM_FAIL; }
        if (GiveMoveToMon(mon, move) != MON_HAS_MAX_MOVES)
        {
            Consume(item);
            snprintf(msg, cap, "It learned the move!");
            return UI_ITEM_DONE;
        }
        return UI_ITEM_FORGET_MOVE;    // the screen asks which move to forget (UiGame_ReplaceMove)
    }
    if (GetItemEffectType(item) == ITEM_EFFECT_EVO_STONE)
    {
        const uint16_t target = GetEvolutionTargetSpecies(mon, EVO_ITEM, item);
        if (target == SPECIES_NONE) { snprintf(msg, cap, "It won't have any effect."); return UI_ITEM_FAIL; }
        RemoveBagItem(item, 1);
        sEvoSlot = (uint8_t)slot;
        sEvoSpecies = target;
        *afterClose = AfterClose_Evolve;
        return UI_ITEM_CLOSE;
    }
    if (GetItemEffectType(item) == ITEM_EFFECT_PP_UP || item == ITEM_ETHER || item == ITEM_MAX_ETHER)
        return UI_ITEM_CHOOSE_MOVE;     // the screen asks which move (UiGame_ItemOnMove)

    Stats(mon, delta->before, &delta->levelBefore);
    if (PokemonUseItemEffects(mon, item, (u8)slot, 0, FALSE)) { snprintf(msg, cap, "It won't have any effect."); return UI_ITEM_FAIL; }
    Consume(item);
    CalculateMonStats(mon);
    Stats(mon, delta->after, &delta->levelAfter);
    bool changed = delta->levelAfter != delta->levelBefore;
    for (int32_t i = 0; i < 6; i++)
        changed |= delta->after[i] != delta->before[i];
    snprintf(msg, cap, "It was used.");
    return changed ? UI_ITEM_DELTA : UI_ITEM_DONE;
}

bool UiGame_ItemOnMove(int32_t slot, uint16_t item, int32_t moveIdx)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (PokemonUseItemEffects(mon, item, (u8)slot, (u8)moveIdx, FALSE))
        return false;
    Consume(item);
    return true;
}

bool UiGame_ReplaceMove(int32_t slot, uint16_t item, int32_t moveIdx)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (IsMoveHm((uint16_t)GetMonData(mon, MON_DATA_MOVE1 + moveIdx)))
        return false;               // HM moves can't be forgotten
    RemoveMonPPBonus(mon, (u8)moveIdx);
    SetMonMoveSlot(mon, ItemIdToBattleMoveId(item), (u8)moveIdx);
    Consume(item);
    return true;
}

bool UiGame_CanLearnTm(int32_t slot, uint16_t item, bool *known)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    const uint16_t move = ItemIdToBattleMoveId(item);
    *known = MonKnowsMove(mon, move);
    return GetMonData(mon, MON_DATA_IS_EGG) == 0 && !*known && CanMonLearnTMHM(mon, (u8)(item - ITEM_TM01));
}

void UiGame_GiveItem(int32_t slot, uint16_t item, uint16_t *previous)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    *previous = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
    if (*previous != ITEM_NONE)
        AddBagItem(*previous, 1);
    RemoveBagItem(item, 1);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
}

bool UiGame_TakeItem(int32_t slot, uint16_t *taken)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    *taken = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
    if (*taken == ITEM_NONE || !AddBagItem(*taken, 1))
        return false;
    const uint16_t none = ITEM_NONE;
    SetMonData(mon, MON_DATA_HELD_ITEM, &none);
    return true;
}

void UiGame_MoveInfo(int32_t slot, int32_t moveIdx, uint16_t *move, uint8_t *pp, uint8_t *maxPp)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    *move = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + moveIdx);
    *pp = (uint8_t)GetMonData(mon, MON_DATA_PP1 + moveIdx);
    *maxPp = *move ? (uint8_t)CalculatePPWithBonus(*move, (u8)GetMonData(mon, MON_DATA_PP_BONUSES), (u8)moveIdx) : 0;
}

void UiGame_MoveName(uint16_t move, char *out, size_t cap)
{
    UiGame_Text(gMoveNames[move], out, cap);
}
