// Game side of the BAG and POKÃ©MON screens opened by the battle controller (battle_controller_player.c)
// instead of the GBA ones: the same checks and effects as item_use.c's ItemUseInBattle_* and
// party_menu.c's TrySwitchInPokemon. Core 1 only (the battle waits for the UI to close).

#include <stdio.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"

#include "global.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "item.h"
#include "item_use.h"
#include "party_menu.h"
#include "pokemon.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "battle_anim.h"
#include "item_menu.h"
#include "main.h"
#include "strings.h"
#include "string_util.h"

static bool sBattleMenu;

bool UiGame_InBattleMenu(void)
{
    return sBattleMenu;
}

void UiGame_BattleMenuEnd(void)
{
    sBattleMenu = false;
}

static bool CanOpenInBattle(void)
{
    // The PYRAMID bag keeps the old screen; multi battles use the party in multi mode (2026-11-13)
    return Ui_IsEnabled() && !Ui_IsOpen() && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE;
}

bool Ui_OpenBattleBag(void)
{
    if (!CanOpenInBattle())
        return false;
    gSpecialVar_ItemId = ITEM_NONE;
    sBattleMenu = true;
    UiCore_OpenFromScript(&gUiBagScreen, NULL);
    return true;
}

bool Ui_OpenBattleParty(void)
{
    if (!CanOpenInBattle())
        return false;
    gSelectedMonPartyId = PARTY_SIZE;
    gPartyMenuUseExitCallback = FALSE;
    sBattleMenu = true;
    UiCore_OpenFromScript(&gUiPartyScreen, NULL);
    return true;
}

static u8 PartyAction(void)
{
    return gBattleBufferA[gBattlerInMenuId][1] & 0xF;
}

bool UiGame_MonIsInBattle(int32_t slot)
{
    for (u32 b = 0; b < gBattlersCount; b++)
        if (GetBattlerSide(b) == B_SIDE_PLAYER && gBattlerPartyIndexes[b] == slot)
            return true;
    return false;
}

// Multi battle: party slots 3-5 hold the partner's POKÃ©MON (shown greyed, never chosen)
bool UiGame_IsMultiPartner(int32_t slot)
{
    return gMain.inBattle && IsMultiBattle() && slot >= MULTI_PARTY_SIZE;
}

// NULL when the POKÃ©MON is sent out (the UI then closes), otherwise why not
const char *UiGame_BattleSwitch(int32_t slot)
{
    static char sWhy[96];
    if (UiGame_IsMultiPartner(slot))
    {
        // TrySwitchInPokemon: "You can't switch {partner}'s POKéMON with one of yours!"
        StringCopy(gStringVar1, GetTrainerPartnerName());
        StringExpandPlaceholders(gStringVar4, gText_CantSwitchWithAlly);
        UiGame_Text(gStringVar4, sWhy, sizeof(sWhy));
        return sWhy;
    }
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG)) return "An EGG can't battle!";
    if (GetMonData(mon, MON_DATA_HP) == 0) return "It has no energy left to battle!";
    if (UiGame_MonIsInBattle(slot)) return "It's already in battle!";
    if (slot == gBattleStruct->prevSelectedPartySlot) return "It has already been selected.";
    if (PartyAction() == PARTY_ACTION_ABILITY_PREVENTS) return "An ability prevents switching!";
    if (PartyAction() == PARTY_ACTION_CANT_SWITCH) return "It can't be switched out!";
    gSelectedMonPartyId = (u8)slot;
    gPartyMenuUseExitCallback = TRUE;
    return NULL;
}

// Circle on the battle party: not allowed when a POKÃ©MON has to be sent out
bool UiGame_BattlePartyCanCancel(void)
{
    if (PartyAction() == PARTY_ACTION_SEND_OUT)
        return false;
    for (u32 b = 0; b < gBattlersCount; b++)
        if (GetBattlerSide(b) == B_SIDE_PLAYER)
            return GetMonData(&gPlayerParty[gBattlerPartyIndexes[b]], MON_DATA_HP) != 0;
    return true;
}

bool UiGame_BattleItemUsable(uint16_t item)
{
    return GetItemBattleUsage(item) != 0 && GetItemBattleFunc(item) != NULL;
}

// USE in the battle bag
UiBattleUse UiGame_BattleUse(uint16_t item, char *msg, size_t cap)
{
    const ItemUseFunc fn = GetItemBattleFunc(item);
    msg[0] = '\0';
    if (!UiGame_BattleItemUsable(item))
    {
        snprintf(msg, cap, "It can't be used now.");
        return UI_BUSE_FAIL;
    }
    if (fn == ItemUseInBattle_PokeBall)
    {
        if (IsPlayerPartyAndPokemonStorageFull())
        {
            snprintf(msg, cap, "The BOX is full! You can't throw it.");
            return UI_BUSE_FAIL;
        }
        RemoveBagItem(item, 1);
    }
    else if (fn == ItemUseInBattle_StatIncrease)
    {
        const u16 partyId = gBattlerPartyIndexes[gBattlerInMenuId];
        if (ExecuteTableBasedItemEffect(&gPlayerParty[partyId], item, (u8)partyId, 0))
        {
            snprintf(msg, cap, "It won't have any effect.");
            return UI_BUSE_FAIL;
        }
        RemoveBagItem(item, 1);
    }
    else if (fn == ItemUseInBattle_Escape)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        {
            snprintf(msg, cap, "This isn't the time to use that!");
            return UI_BUSE_FAIL;
        }
        RemoveBagItem(item, 1);
    }
    else
        return UI_BUSE_TARGET;                         // medicine, PP, berries: on which POKÃ©MON
    gSpecialVar_ItemId = item;
    return UI_BUSE_CLOSE;
}

// The item on a POKÃ©MON from the battle bag (moveIdx < 0: not chosen yet)
UiItemResult UiGame_BattleItemOnMon(int32_t slot, uint16_t item, int32_t moveIdx, char *msg, size_t cap)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG))
    {
        snprintf(msg, cap, "It can't be used on an EGG.");
        return UI_ITEM_FAIL;
    }
    if (moveIdx < 0 && GetItemBattleFunc(item) == ItemUseInBattle_PPRecovery && (item == ITEM_ETHER || item == ITEM_MAX_ETHER))
        return UI_ITEM_CHOOSE_MOVE;
    if (ExecuteTableBasedItemEffect(mon, item, (u8)slot, (u8)(moveIdx < 0 ? 0 : moveIdx)))
    {
        snprintf(msg, cap, "It won't have any effect.");
        return UI_ITEM_FAIL;
    }
    if (item != ITEM_BLUE_FLUTE && item != ITEM_RED_FLUTE && item != ITEM_YELLOW_FLUTE)
        RemoveBagItem(item, 1);
    gSpecialVar_ItemId = item;
    return UI_ITEM_CLOSE;
}
