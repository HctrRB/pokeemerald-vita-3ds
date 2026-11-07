#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"

#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "contest_effect.h"
#include "data.h"
#include "evolution_scene.h"
#include "item.h"
#include "item_menu.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "region_map.h"
#include "sound.h"
#include "constants/item_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "battle_anim.h"
#include "constants/pokemon.h"
#include "constants/songs.h"
#include "constants/species.h"

#include <stdio.h>
#include <string.h>

enum PartyUiMode
{
    PARTY_MODE_BROWSE = 0,
    PARTY_MODE_POPUP,
    PARTY_MODE_SWITCH,
    PARTY_MODE_SUMMARY,
    PARTY_MODE_BAG_TARGET,
    PARTY_MODE_SOFTBOILED_TARGET,
    PARTY_MODE_MOVE_SELECT,
    PARTY_MODE_REPLACE_5TH,
    PARTY_MODE_DELTA_CARD
};

enum SummaryTab
{
    SUM_TAB_INFO = 0,
    SUM_TAB_SKILLS,
    SUM_TAB_BATTLE_MOVES,
    SUM_TAB_CONTEST_MOVES,
    SUM_TAB_COUNT
};

typedef struct
{
    char label[20];
    uint16_t fieldMove; // 0 if standard action
    uint8_t actionType; // 1: Summary, 2: Switch, 3: Give Item, 4: Take Item, 5: Field Move, 6: Cancel
} PartyPopupItem;

static enum PartyUiMode sMode = PARTY_MODE_BROWSE;
static int32_t sSlot = 0;
static int32_t sSwitchFrom = -1;
static uint8_t sSumTab = SUM_TAB_INFO;
static int32_t sMoveCursor = 0;
static int32_t sMoveSwapFrom = -1;
static uint16_t sPendingBagItem = ITEM_NONE;
static uint16_t sPendingLearnMove = MOVE_NONE;
static PartyPopupItem sPopup[8];
static int32_t sPopupCount = 0;
static int32_t sPopupCursor = 0;

// Stat delta snapshot for Rare Candy / Vitamins / Stones
static int16_t sBeforeStats[6];
static int16_t sAfterStats[6];
static uint8_t sBeforeLevel = 0;
static uint8_t sAfterLevel = 0;

static void SnapshotMonStats(struct Pokemon *mon, int16_t outStats[6], uint8_t *outLv)
{
    outStats[0] = (int16_t)GetMonData(mon, MON_DATA_MAX_HP);
    outStats[1] = (int16_t)GetMonData(mon, MON_DATA_ATK);
    outStats[2] = (int16_t)GetMonData(mon, MON_DATA_DEF);
    outStats[3] = (int16_t)GetMonData(mon, MON_DATA_SPEED);
    outStats[4] = (int16_t)GetMonData(mon, MON_DATA_SPATK);
    outStats[5] = (int16_t)GetMonData(mon, MON_DATA_SPDEF);
    if (outLv != NULL)
        *outLv = (uint8_t)GetMonData(mon, MON_DATA_LEVEL);
}

static bool IsFieldMoveId(const uint16_t move)
{
    switch (move)
    {
    case MOVE_CUT:
    case MOVE_FLASH:
    case MOVE_ROCK_SMASH:
    case MOVE_STRENGTH:
    case MOVE_SURF:
    case MOVE_FLY:
    case MOVE_DIVE:
    case MOVE_WATERFALL:
    case MOVE_TELEPORT:
    case MOVE_DIG:
    case MOVE_SECRET_POWER:
    case MOVE_SWEET_SCENT:
    case MOVE_SOFT_BOILED:
    case MOVE_MILK_DRINK:
        return true;
    default:
        return false;
    }
}

static void BuildPartyPopup(const int32_t slot)
{
    sPopupCount = 0;
    sPopupCursor = 0;
    struct Pokemon *const mon = &gPlayerParty[slot];
    const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;

    if (gMain.inBattle)
    {
        snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "SWITCH");
        sPopup[sPopupCount].fieldMove = 0;
        sPopup[sPopupCount++].actionType = 20;

        snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "SUMMARY");
        sPopup[sPopupCount].fieldMove = 0;
        sPopup[sPopupCount++].actionType = 1;

        snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "CANCEL");
        sPopup[sPopupCount].fieldMove = 0;
        sPopup[sPopupCount++].actionType = 6;
        return;
    }

    snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "SUMMARY");
    sPopup[sPopupCount].fieldMove = 0;
    sPopup[sPopupCount++].actionType = 1;

    if (!isEgg)
    {
        for (int32_t m = 0; m < MAX_MON_MOVES && sPopupCount < 6; m++)
        {
            const uint16_t mv = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + m);
            if (mv != MOVE_NONE && IsFieldMoveId(mv))
            {
                VitaUiArt_GbaToAscii(gMoveNames[mv], sPopup[sPopupCount].label, sizeof(sPopup[0].label));
                sPopup[sPopupCount].fieldMove = mv;
                sPopup[sPopupCount++].actionType = 5;
            }
        }
    }

    if (CalculatePlayerPartyCount() > 1)
    {
        snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "SWITCH");
        sPopup[sPopupCount].fieldMove = 0;
        sPopup[sPopupCount++].actionType = 2;
    }

    if (!isEgg)
    {
        const uint16_t held = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
        if (held != ITEM_NONE)
        {
            snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "TAKE ITEM");
            sPopup[sPopupCount].fieldMove = 0;
            sPopup[sPopupCount++].actionType = 4;
        }
        else
        {
            snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "GIVE ITEM");
            sPopup[sPopupCount].fieldMove = 0;
            sPopup[sPopupCount++].actionType = 3;
        }
    }

    snprintf(sPopup[sPopupCount].label, sizeof(sPopup[0].label), "CANCEL");
    sPopup[sPopupCount].fieldMove = 0;
    sPopup[sPopupCount++].actionType = 6;
}

void VitaUiParty_Reset(const uint16_t pendingBagItem)
{
    sPendingBagItem = pendingBagItem;
    sPendingLearnMove = MOVE_NONE;
    sSwitchFrom = -1;
    sMoveSwapFrom = -1;
    sMode = (pendingBagItem != ITEM_NONE) ? PARTY_MODE_BAG_TARGET : PARTY_MODE_BROWSE;
    const int32_t cnt = (int32_t)CalculatePlayerPartyCount();
    if (sSlot >= cnt)
        sSlot = 0;
}

bool VitaUiParty_HasActiveModal(void)
{
    return sMode != PARTY_MODE_BROWSE;
}

static void ConsumePendingItemIfNeeded(const uint16_t item)
{
    const uint16_t move = ItemIdToBattleMoveId(item);
    if (move != MOVE_NONE)
    {
        if (!IsMoveHm(move) && VitaExtras_GetFast(FAST_EXTRA_INFINITE_TMS) == 0)
            RemoveBagItem(item, 1);
        return;
    }
    RemoveBagItem(item, 1);
}

static void ApplyBagItemToMon(const int32_t slot)
{
    struct Pokemon *const mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG) != 0)
    {
        PlaySE(SE_FAILURE);
        VitaUi_ShowToast("CANNOT USE ON EGG");
        return;
    }

    const uint16_t item = sPendingBagItem;
    const uint8_t pocket = GetPocketByItemId(item);

    // Give item mode (encoded with bit 15 when giving from Bag)
    if ((item & 0x8000U) != 0U)
    {
        const uint16_t realItem = (uint16_t)(item & 0x7FFFU);
        const uint16_t prevHeld = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
        if (prevHeld != ITEM_NONE)
            AddBagItem(prevHeld, 1);
        RemoveBagItem(realItem, 1);
        SetMonData(mon, MON_DATA_HELD_ITEM, &realItem);
        PlaySE(SE_SELECT);
        VitaUi_ShowToast("HELD ITEM EQUIPPED");
        sPendingBagItem = ITEM_NONE;
        VitaUI_Open(VITA_UI_SEC_BAG);
        return;
    }

    // TM / HM teaching
    const uint16_t tmMove = ItemIdToBattleMoveId(item);
    if (pocket == POCKET_TM_HM && tmMove != MOVE_NONE)
    {
        const uint8_t tmIdx = (uint8_t)(item - ITEM_TM01);
        if (!CanMonLearnTMHM(mon, tmIdx) || MonKnowsMove(mon, tmMove))
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast(MonKnowsMove(mon, tmMove) ? "ALREADY KNOWS MOVE" : "INCOMPATIBLE TM/HM");
            return;
        }
        if (GiveMoveToMon(mon, tmMove) != MON_HAS_MAX_MOVES)
        {
            ConsumePendingItemIfNeeded(item);
            PlaySE(SE_SUCCESS);
            VitaUi_ShowToast("LEARNED NEW MOVE!");
            sPendingBagItem = ITEM_NONE;
            VitaUI_Open(VITA_UI_SEC_BAG);
            return;
        }
        sPendingLearnMove = tmMove;
        sMoveCursor = 0;
        sMode = PARTY_MODE_REPLACE_5TH;
        PlaySE(SE_SELECT);
        return;
    }

    // Evolution Stone
    if (GetItemEffectType(item) == ITEM_EFFECT_EVO_STONE)
    {
        const uint16_t targetSpecies = GetEvolutionTargetSpecies(mon, EVO_ITEM, item);
        if (targetSpecies == SPECIES_NONE)
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("IT WOULD HAVE NO EFFECT");
            return;
        }
        RemoveBagItem(item, 1);
        sPendingBagItem = ITEM_NONE;
        PlaySE(SE_SELECT);
        VitaUI_Close();
        CleanupOverworldWindowsAndTilemaps();
        gCB2_AfterEvolution = CB2_ReturnToField;
        BeginEvolutionScene(mon, targetSpecies, FALSE, (u8)slot);
        return;
    }

    // PP Up / PP Max / Ether / Elixir require selecting a move slot
    const uint8_t effType = GetItemEffectType(item);
    if (effType == ITEM_EFFECT_PP_UP || item == ITEM_ETHER || item == ITEM_MAX_ETHER)
    {
        sMoveCursor = 0;
        sMode = PARTY_MODE_MOVE_SELECT;
        PlaySE(SE_SELECT);
        return;
    }

    // Medicine / Rare Candy / Vitamins / EV Berries
    SnapshotMonStats(mon, sBeforeStats, &sBeforeLevel);
    if (PokemonUseItemEffects(mon, item, (u8)slot, 0, FALSE))
    {
        PlaySE(SE_FAILURE);
        VitaUi_ShowToast("IT WONT HAVE ANY EFFECT");
        return;
    }

    ConsumePendingItemIfNeeded(item);
    CalculateMonStats(mon);
    SnapshotMonStats(mon, sAfterStats, &sAfterLevel);
    PlaySE(SE_USE_ITEM);

    if (gMain.inBattle)
    {
        gSpecialVar_ItemId = item;
        sPendingBagItem = ITEM_NONE;
        sMode = PARTY_MODE_BROWSE;
        VitaUI_Close();
        return;
    }

    bool statChanged = (sAfterLevel != sBeforeLevel);
    for (int32_t i = 0; i < 6; i++)
    {
        if (sAfterStats[i] != sBeforeStats[i])
            statChanged = true;
    }
    if (statChanged)
    {
        sMode = PARTY_MODE_DELTA_CARD;
        return;
    }

    VitaUi_ShowToast("ITEM APPLIED!");
    if (!CheckBagHasItem(item, 1))
    {
        sPendingBagItem = ITEM_NONE;
        VitaUI_Open(VITA_UI_SEC_BAG);
    }
}

static void ExecutePopupChoice(void)
{
    const PartyPopupItem *const act = &sPopup[sPopupCursor];
    struct Pokemon *const mon = &gPlayerParty[sSlot];
    const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;

    if (act->actionType == 20)
    {
        if (isEgg)
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("AN EGG CANT BATTLE");
            return;
        }
        if (GetMonData(mon, MON_DATA_HP) == 0)
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("POKEMON HAS NO ENERGY");
            return;
        }
        for (u32 b = 0; b < gBattlersCount; b++)
        {
            if (GetBattlerSide(b) == B_SIDE_PLAYER && sSlot == gBattlerPartyIndexes[b])
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("ALREADY IN BATTLE");
                return;
            }
        }
        gSelectedMonPartyId = (u8)sSlot;
        gPartyMenuUseExitCallback = TRUE;
        sMode = PARTY_MODE_BROWSE;
        PlaySE(SE_SELECT);
        VitaUI_Close();
        return;
    }

    if (act->actionType == 1)
    {
        sSumTab = SUM_TAB_INFO;
        sMoveCursor = 0;
        sMoveSwapFrom = -1;
        sMode = PARTY_MODE_SUMMARY;
        PlaySE(SE_SELECT);
    }
    else if (act->actionType == 2)
    {
        sSwitchFrom = sSlot;
        sMode = PARTY_MODE_SWITCH;
        PlaySE(SE_SELECT);
    }
    else if (act->actionType == 3)
    {
        sMode = PARTY_MODE_BROWSE;
        PlaySE(SE_SELECT);
        VitaUI_Open(VITA_UI_SEC_BAG);
    }
    else if (act->actionType == 4)
    {
        const uint16_t held = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
        if (held != ITEM_NONE && AddBagItem(held, 1))
        {
            const uint16_t none = ITEM_NONE;
            SetMonData(mon, MON_DATA_HELD_ITEM, &none);
            PlaySE(SE_SELECT);
            VitaUi_ShowToast("TOOK HELD ITEM");
        }
        else
        {
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("BAG IS FULL");
        }
        sMode = PARTY_MODE_BROWSE;
    }
    else if (act->actionType == 5)
    {
        if (act->fieldMove == MOVE_SOFT_BOILED || act->fieldMove == MOVE_MILK_DRINK)
        {
            const uint16_t hp = (uint16_t)GetMonData(mon, MON_DATA_HP);
            const uint16_t cost = (uint16_t)(GetMonData(mon, MON_DATA_MAX_HP) / 5U);
            if (hp <= cost || cost == 0)
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("NOT ENOUGH HP");
                return;
            }
            sSwitchFrom = sSlot;
            sMode = PARTY_MODE_SOFTBOILED_TARGET;
            PlaySE(SE_SELECT);
            return;
        }
        if (act->fieldMove == MOVE_FLY)
        {
            uint8_t fail = 0;
            if (VitaGame_TryUseFieldMove((uint8_t)sSlot, MOVE_FLY, &fail))
            {
                sMode = PARTY_MODE_BROWSE;
                PlaySE(SE_SELECT);
                VitaUI_OpenFlyMap((uint8_t)sSlot);
                return;
            }
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast(fail == 1 ? "NEW BADGE REQUIRED" : "CANT FLY FROM HERE");
            sMode = PARTY_MODE_BROWSE;
            return;
        }
        uint8_t fail = 0;
        sMode = PARTY_MODE_BROWSE;
        VitaUI_Close();
        if (!VitaGame_TryUseFieldMove((uint8_t)sSlot, act->fieldMove, &fail))
        {
            VitaUI_Open(VITA_UI_SEC_POKEMON);
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast(fail == 1 ? "NEW BADGE REQUIRED" : "CANT USE THAT HERE");
        }
    }
    else
    {
        sMode = PARTY_MODE_BROWSE;
        PlaySE(SE_SELECT);
    }
}

bool VitaUiParty_Update(const VitaUiInput *in)
{
    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();
    if (partyCnt <= 0)
        return false;

    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);

    if (sMode == PARTY_MODE_DELTA_CARD)
    {
        if ((in->newButtons & (aBtn | bBtn)) || in->touchPressed)
        {
            PlaySE(SE_SELECT);
            if (sPendingBagItem != ITEM_NONE && CheckBagHasItem(sPendingBagItem, 1))
                sMode = PARTY_MODE_BAG_TARGET;
            else
            {
                sPendingBagItem = ITEM_NONE;
                sMode = PARTY_MODE_BROWSE;
                VitaUI_Open(VITA_UI_SEC_BAG);
            }
            return true;
        }
        return false;
    }

    if (sMode == PARTY_MODE_MOVE_SELECT || sMode == PARTY_MODE_REPLACE_5TH)
    {
        const int32_t maxSlots = (sMode == PARTY_MODE_REPLACE_5TH) ? 5 : 4;
        if (in->newButtons & VITA_BTN_UP)
        {
            sMoveCursor = (sMoveCursor <= 0) ? (maxSlots - 1) : (sMoveCursor - 1);
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sMoveCursor = (sMoveCursor + 1) % maxSlots;
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & aBtn)
        {
            struct Pokemon *const mon = &gPlayerParty[sSlot];
            if (sMode == PARTY_MODE_REPLACE_5TH)
            {
                if (sMoveCursor < 4)
                {
                    const uint16_t oldMv = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + sMoveCursor);
                    if (IsMoveHm(oldMv))
                    {
                        PlaySE(SE_FAILURE);
                        VitaUi_ShowToast("HM MOVES CANT BE FORGOTTEN");
                        return true;
                    }
                    RemoveMonPPBonus(mon, (u8)sMoveCursor);
                    SetMonMoveSlot(mon, sPendingLearnMove, (u8)sMoveCursor);
                    ConsumePendingItemIfNeeded(sPendingBagItem);
                    PlaySE(SE_SUCCESS);
                    VitaUi_ShowToast("1, 2, AND POOF! LEARNED!");
                }
                sPendingLearnMove = MOVE_NONE;
                sPendingBagItem = ITEM_NONE;
                sMode = PARTY_MODE_BROWSE;
                VitaUI_Open(VITA_UI_SEC_BAG);
                return true;
            }
            if (!PokemonUseItemEffects(mon, sPendingBagItem, (u8)sSlot, (u8)sMoveCursor, FALSE))
            {
                ConsumePendingItemIfNeeded(sPendingBagItem);
                PlaySE(SE_USE_ITEM);
                VitaUi_ShowToast("MOVE PP RESTORED / UP!");
                sPendingBagItem = ITEM_NONE;
                sMode = PARTY_MODE_BROWSE;
                VitaUI_Open(VITA_UI_SEC_BAG);
            }
            else
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("NO EFFECT ON THIS MOVE");
            }
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sMode = PARTY_MODE_BAG_TARGET;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    if (sMode == PARTY_MODE_SUMMARY)
    {
        const bool isEgg = GetMonData(&gPlayerParty[sSlot], MON_DATA_IS_EGG) != 0;
        if (!isEgg)
        {
            if (in->newButtons & VITA_BTN_LEFT)
            {
                sSumTab = (sSumTab == 0) ? (SUM_TAB_COUNT - 1) : (uint8_t)(sSumTab - 1U);
                sMoveSwapFrom = -1;
                PlaySE(SE_SELECT);
                return true;
            }
            if (in->newButtons & VITA_BTN_RIGHT)
            {
                sSumTab = (uint8_t)((sSumTab + 1U) % SUM_TAB_COUNT);
                sMoveSwapFrom = -1;
                PlaySE(SE_SELECT);
                return true;
            }
            for (uint8_t t = 0; t < SUM_TAB_COUNT; t++)
            {
                if (VitaUi_HitRect(in, 114 + (int32_t)t * 50, 26, 48, 15))
                {
                    sSumTab = t;
                    sMoveSwapFrom = -1;
                    PlaySE(SE_SELECT);
                    return true;
                }
            }
        }
        if (sSumTab == SUM_TAB_BATTLE_MOVES || sSumTab == SUM_TAB_CONTEST_MOVES)
        {
            if (in->newButtons & VITA_BTN_UP)
            {
                sMoveCursor = (sMoveCursor <= 0) ? 3 : (sMoveCursor - 1);
                PlaySE(SE_SELECT);
                return true;
            }
            if (in->newButtons & VITA_BTN_DOWN)
            {
                sMoveCursor = (sMoveCursor + 1) % 4;
                PlaySE(SE_SELECT);
                return true;
            }
            if ((in->newButtons & aBtn) && sSumTab == SUM_TAB_BATTLE_MOVES)
            {
                if (sMoveSwapFrom < 0)
                {
                    sMoveSwapFrom = sMoveCursor;
                    PlaySE(SE_SELECT);
                }
                else
                {
                    if (sMoveSwapFrom != sMoveCursor)
                    {
                        struct Pokemon *const mon = &gPlayerParty[sSlot];
                        const uint16_t m1 = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + sMoveSwapFrom);
                        const uint16_t m2 = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + sMoveCursor);
                        if (m1 != MOVE_NONE && m2 != MOVE_NONE)
                        {
                            const uint8_t pp1 = (uint8_t)GetMonData(mon, MON_DATA_PP1 + sMoveSwapFrom);
                            const uint8_t pp2 = (uint8_t)GetMonData(mon, MON_DATA_PP1 + sMoveCursor);
                            const uint8_t b = (uint8_t)GetMonData(mon, MON_DATA_PP_BONUSES);
                            const uint8_t s1 = (uint8_t)((b >> (sMoveSwapFrom * 2)) & 3U);
                            const uint8_t s2 = (uint8_t)((b >> (sMoveCursor * 2)) & 3U);
                            const uint8_t nb = (uint8_t)((b & ~(3U << (sMoveSwapFrom * 2)) & ~(3U << (sMoveCursor * 2)))
                                                       | (s1 << (sMoveCursor * 2)) | (s2 << (sMoveSwapFrom * 2)));
                            SetMonData(mon, MON_DATA_MOVE1 + sMoveSwapFrom, &m2);
                            SetMonData(mon, MON_DATA_MOVE1 + sMoveCursor, &m1);
                            SetMonData(mon, MON_DATA_PP1 + sMoveSwapFrom, &pp2);
                            SetMonData(mon, MON_DATA_PP1 + sMoveCursor, &pp1);
                            SetMonData(mon, MON_DATA_PP_BONUSES, &nb);
                        }
                    }
                    sMoveSwapFrom = -1;
                    PlaySE(SE_SELECT);
                }
                return true;
            }
        }
        else
        {
            if ((in->newButtons & VITA_BTN_UP) && sSlot > 0)
            {
                sSlot--;
                if (GetMonData(&gPlayerParty[sSlot], MON_DATA_IS_EGG) != 0)
                    sSumTab = SUM_TAB_INFO;
                PlaySE(SE_SELECT);
                return true;
            }
            if ((in->newButtons & VITA_BTN_DOWN) && sSlot + 1 < partyCnt)
            {
                sSlot++;
                if (GetMonData(&gPlayerParty[sSlot], MON_DATA_IS_EGG) != 0)
                    sSumTab = SUM_TAB_INFO;
                PlaySE(SE_SELECT);
                return true;
            }
        }
        if ((in->newButtons & bBtn) || VitaUi_HitRect(in, 6, 186, 102, 18))
        {
            if (sMoveSwapFrom >= 0)
                sMoveSwapFrom = -1;
            else
                sMode = PARTY_MODE_BROWSE;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    if (sMode == PARTY_MODE_POPUP)
    {
        if (in->newButtons & VITA_BTN_UP)
        {
            sPopupCursor = (sPopupCursor <= 0) ? (sPopupCount - 1) : (sPopupCursor - 1);
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sPopupCursor = (sPopupCursor + 1) % sPopupCount;
            PlaySE(SE_SELECT);
            return true;
        }

        const int32_t ph = sPopupCount * 20 + 8;
        const int32_t py = 204 - ph;
        for (int32_t i = 0; i < sPopupCount; i++)
        {
            if (VitaUi_HitRect(in, 196, py + 4 + i * 20, 118, 18))
            {
                sPopupCursor = i;
                ExecutePopupChoice();
                return true;
            }
        }
        if (in->touchPressed && !VitaUi_HitRect(in, 196, py, 118, ph))
        {
            sMode = PARTY_MODE_BROWSE;
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & aBtn)
        {
            ExecutePopupChoice();
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sMode = PARTY_MODE_BROWSE;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    // Browse / Switch / BagTarget / SoftboiledTarget slot navigation
    bool changed = false;
    if ((in->newButtons & VITA_BTN_DOWN) && sSlot + 1 < partyCnt)
    {
        sSlot++;
        PlaySE(SE_SELECT);
        changed = true;
    }
    else if ((in->newButtons & VITA_BTN_UP) && sSlot > 0)
    {
        sSlot--;
        PlaySE(SE_SELECT);
        changed = true;
    }
    else if ((in->newButtons & VITA_BTN_RIGHT) && sSlot == 0 && partyCnt > 1)
    {
        sSlot = 1;
        PlaySE(SE_SELECT);
        changed = true;
    }
    else if ((in->newButtons & VITA_BTN_LEFT) && sSlot > 0)
    {
        sSlot = 0;
        PlaySE(SE_SELECT);
        changed = true;
    }

    // Touch hit on Lead (left) or slots 1..5 (right)
    int32_t tappedSlot = -1;
    if (VitaUi_HitRect(in, 6, 28, 102, 92))
        tappedSlot = 0;
    for (int32_t i = 1; i < partyCnt; i++)
    {
        if (VitaUi_HitRect(in, 112, 28 + (i - 1) * 35, 202, 33))
            tappedSlot = i;
    }

    if (tappedSlot >= 0)
    {
        sSlot = tappedSlot;
        changed = true;
    }

    if ((in->newButtons & aBtn) || tappedSlot >= 0)
    {
        if (sMode == PARTY_MODE_SWITCH)
        {
            if (sSwitchFrom >= 0 && sSwitchFrom != sSlot)
            {
                struct Pokemon tmp = gPlayerParty[sSwitchFrom];
                gPlayerParty[sSwitchFrom] = gPlayerParty[sSlot];
                gPlayerParty[sSlot] = tmp;
                PlaySE(SE_SELECT);
            }
            sSwitchFrom = -1;
            sMode = PARTY_MODE_BROWSE;
            return true;
        }
        if (sMode == PARTY_MODE_SOFTBOILED_TARGET)
        {
            if (sSwitchFrom >= 0 && sSwitchFrom != sSlot && GetMonData(&gPlayerParty[sSlot], MON_DATA_IS_EGG) == 0)
            {
                struct Pokemon *const src = &gPlayerParty[sSwitchFrom];
                struct Pokemon *const dst = &gPlayerParty[sSlot];
                const uint16_t cost = (uint16_t)(GetMonData(src, MON_DATA_MAX_HP) / 5U);
                const uint16_t dstHp = (uint16_t)GetMonData(dst, MON_DATA_HP);
                const uint16_t dstMax = (uint16_t)GetMonData(dst, MON_DATA_MAX_HP);
                if (dstHp > 0 && dstHp < dstMax)
                {
                    const uint16_t newSrcHp = (uint16_t)(GetMonData(src, MON_DATA_HP) - cost);
                    uint16_t newDstHp = (uint16_t)(dstHp + cost);
                    if (newDstHp > dstMax) newDstHp = dstMax;
                    SetMonData(src, MON_DATA_HP, &newSrcHp);
                    SetMonData(dst, MON_DATA_HP, &newDstHp);
                    PlaySE(SE_USE_ITEM);
                    VitaUi_ShowToast("HP SHARED!");
                    sMode = PARTY_MODE_BROWSE;
                    return true;
                }
            }
            PlaySE(SE_FAILURE);
            return true;
        }
        if (sMode == PARTY_MODE_BAG_TARGET)
        {
            ApplyBagItemToMon(sSlot);
            return true;
        }
        BuildPartyPopup(sSlot);
        sMode = PARTY_MODE_POPUP;
        PlaySE(SE_SELECT);
        return true;
    }

    if (in->newButtons & bBtn)
    {
        if (sMode == PARTY_MODE_SWITCH || sMode == PARTY_MODE_SOFTBOILED_TARGET)
        {
            sSwitchFrom = -1;
            sMode = PARTY_MODE_BROWSE;
            PlaySE(SE_SELECT);
            return true;
        }
        if (sMode == PARTY_MODE_BAG_TARGET)
        {
            sPendingBagItem = ITEM_NONE;
            sMode = PARTY_MODE_BROWSE;
            PlaySE(SE_SELECT);
            VitaUI_Open(VITA_UI_SEC_BAG);
            return true;
        }
        if (gMain.inBattle)
        {
            u32 playerBattler = 0;
            for (u32 b = 0; b < gBattlersCount; b++)
            {
                if (GetBattlerSide(b) == B_SIDE_PLAYER)
                {
                    playerBattler = b;
                    break;
                }
            }
            if (GetMonData(&gPlayerParty[gBattlerPartyIndexes[playerBattler]], MON_DATA_HP) == 0)
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("MUST CHOOSE A POKEMON");
                return true;
            }

            gPartyMenuUseExitCallback = FALSE;
            PlaySE(SE_WIN_OPEN);
            VitaUI_Close();
            return true;
        }
    }

    return changed;
}

static void DrawStatusBadge(uint16_t *dst, const size_t pitch, struct Pokemon *mon, const int32_t x, const int32_t y)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    const uint16_t hp = (uint16_t)GetMonData(mon, MON_DATA_HP);
    if (hp == 0)
    {
        VitaUiArt_FillRect(dst, pitch, clip, x, y, 20, 9, UI_INK_RED);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "FNT", x + 10, y - 1, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        return;
    }
    const uint32_t st = GetMonData(mon, MON_DATA_STATUS);
    if (st == 0)
        return;

    const char *lbl = "SLP";
    uint16_t col = UI_INK_MUTED;
    if (st & STATUS1_PSN_ANY)      { lbl = "PSN"; col = VitaUi_PackRgb565(168, 68, 188); }
    else if (st & STATUS1_PARALYSIS) { lbl = "PAR"; col = UI_HP_YELLOW; }
    else if (st & STATUS1_BURN)    { lbl = "BRN"; col = UI_INK_RED; }
    else if (st & STATUS1_FREEZE)  { lbl = "FRZ"; col = VitaUi_PackRgb565(72, 180, 224); }
    VitaUiArt_FillRect(dst, pitch, clip, x, y, 20, 9, col);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lbl, x + 10, y - 1, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
}

static void RenderSummaryView(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    struct Pokemon *const mon = &gPlayerParty[sSlot];
    const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
    const uint16_t species = (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
    const bool shiny = !isEgg && IsMonShiny(mon);

    // Left summary portrait plate
    VitaUiPlate_Draw(dst, pitch, clip, 6, 26, 102, 180, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 15);
    uint8_t nickGba[POKEMON_NAME_LENGTH + 2];
    GetMonNickname(mon, nickGba);
    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, nickGba, 11, 28, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    if (isEgg)
    {
        VitaUiArt_DrawMonIcon(dst, pitch, clip, SPECIES_EGG, 0, 25, 58, 2);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "MYSTERIOUS EGG", 57, 134, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }
    else
    {
        VitaUiArt_DrawMonFrontPic(dst, pitch, clip, species, shiny, 25, 48, 64, false);
        char lvBuf[20];
        const uint8_t gender = GetMonGender(mon);
        const char *gStr = (gender == MON_MALE) ? " M" : ((gender == MON_FEMALE) ? " F" : "");
        snprintf(lvBuf, sizeof(lvBuf), "LV.%u%s%s", (uint32_t)GetMonData(mon, MON_DATA_LEVEL), gStr, shiny ? " *" : "");
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, lvBuf, 12, 116, shiny ? UI_INK_GOLD_DARK : UI_INK_DARK, VITA_UI_NO_SHADOW);
        DrawStatusBadge(dst, pitch, mon, 80, 117);

        VitaUiArt_DrawTypeBadge(dst, pitch, clip, gSpeciesInfo[species].types[0], 12, 132);
        if (gSpeciesInfo[species].types[1] != gSpeciesInfo[species].types[0])
            VitaUiArt_DrawTypeBadge(dst, pitch, clip, gSpeciesInfo[species].types[1], 46, 132);

        const uint16_t held = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
        if (held != ITEM_NONE)
        {
            uint8_t itemGba[20];
            CopyItemName(held, itemGba);
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, itemGba, 12, 148, UI_INK_BLUE, VITA_UI_NO_SHADOW);
        }
        else
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "NO ITEM HELD", 12, 148, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
    }

    VitaUiPlate_Draw(dst, pitch, clip, 10, 184, 94, 17, 3, kPlateSageDark, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUiArt_DrawStrAsciiInPlate(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "B: BACK TO PARTY", 10, 184, 94, 17, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Right 4-tab Summary pane
    static const char *const sTabNames[SUM_TAB_COUNT] = { "INFO", "SKILLS", "BATTLE", "CONTEST" };
    for (uint8_t t = 0; t < SUM_TAB_COUNT; t++)
    {
        const bool sel = (sSumTab == t);
        VitaUiPlate_Draw(
            dst, pitch, clip,
            114 + (int32_t)t * 50, 26, 48, 15, 2,
            sel ? kPlateSage : kPlateSageDark,
            isEgg && t > 0 ? VITA_PLATE_DISABLED : (sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL),
            blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL_NARROW, sTabNames[t],
            114 + (int32_t)t * 50, 26, 48, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    VitaUiPlate_Draw(dst, pitch, clip, 114, 43, 200, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);

    if (isEgg)
    {
        const uint8_t cycles = (uint8_t)GetMonData(mon, MON_DATA_FRIENDSHIP);
        const char *eggMsg = (cycles <= 5) ? "SOUNDS CAN BE HEARD COMING\nFROM INSIDE! ALMOST READY!"
                           : (cycles <= 10) ? "IT APPEARS TO MOVE OCCASIONALLY.\nIT MAY BE CLOSE TO HATCHING."
                           : "WHAT WILL HATCH FROM THIS?\nIT WILL TAKE SOME TIME.";
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, eggMsg, 122, 68, UI_INK_DARK, VITA_UI_NO_SHADOW);
        return;
    }

    if (sSumTab == SUM_TAB_INFO)
    {
        const uint8_t nature = GetNature(mon);
        char natAsc[16];
        VitaUiArt_GbaToAscii(gNatureNamePointers[nature], natAsc, sizeof(natAsc));
        char line[48];
        snprintf(line, sizeof(line), "SPECIES: #%03u   OT ID: %05u",
                 SpeciesToNationalPokedexNum(species),
                 (uint32_t)(GetMonData(mon, MON_DATA_OT_ID) & 0xFFFFU));
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, line, 122, 49, UI_INK_DARK, VITA_UI_NO_SHADOW);

        snprintf(line, sizeof(line), "NATURE: %s", natAsc);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, line, 122, 64, UI_INK_BLUE, VITA_UI_NO_SHADOW);

        const uint8_t metLoc = (uint8_t)GetMonData(mon, MON_DATA_MET_LOCATION);
        uint8_t locGba[28];
        char locAsc[28];
        GetMapNameGeneric(locGba, metLoc);
        VitaUiArt_GbaToAscii(locGba, locAsc, sizeof(locAsc));
        snprintf(line, sizeof(line), "MET AT: %s (LV.%u)", locAsc, (uint32_t)GetMonData(mon, MON_DATA_MET_LEVEL));
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, line, 122, 79, UI_INK_MUTED, VITA_UI_NO_SHADOW);

        VitaUiArt_FillRect(dst, pitch, clip, 122, 95, 184, 1, UI_BORDER_SOFT);

        const uint8_t abil = GetMonAbility(mon);
        VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, gAbilityNames[abil], 122, 100, UI_INK_DARK, VITA_UI_NO_SHADOW);
        if (gAbilityDescriptionPointers[abil] != NULL)
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gAbilityDescriptionPointers[abil], 122, 114, UI_INK_MUTED, VITA_UI_NO_SHADOW);

        const uint32_t exp = GetMonData(mon, MON_DATA_EXP);
        const uint8_t lv = (uint8_t)GetMonData(mon, MON_DATA_LEVEL);
        const uint32_t nextExp = (lv < MAX_LEVEL)
            ? (gExperienceTables[gSpeciesInfo[species].growthRate][lv + 1] - exp)
            : 0U;
        snprintf(line, sizeof(line), "EXP: %u   NEXT LV: %u", (uint32_t)exp, (uint32_t)nextExp);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, line, 122, 184, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }
    else if (sSumTab == SUM_TAB_SKILLS)
    {
        const bool showIvEv = VitaExtras_GetFast(FAST_EXTRA_IVS_EVS) != 0;
        const uint8_t nature = GetNature(mon);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "STAT", 122, 48, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        if (showIvEv)
        {
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "VAL", 214, 48, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "IV", 254, 48, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "EV", 298, 48, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
        else
        {
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "VALUE", 214, 48, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }

        static const char *const sStatLbl[6] = { "HP", "ATTACK", "DEFENSE", "SP.ATK", "SP.DEF", "SPEED" };
        const int32_t vals[6] = {
            (int32_t)GetMonData(mon, MON_DATA_MAX_HP),
            (int32_t)GetMonData(mon, MON_DATA_ATK),
            (int32_t)GetMonData(mon, MON_DATA_DEF),
            (int32_t)GetMonData(mon, MON_DATA_SPATK),
            (int32_t)GetMonData(mon, MON_DATA_SPDEF),
            (int32_t)GetMonData(mon, MON_DATA_SPEED)
        };
        const int32_t ivs[6] = {
            (int32_t)GetMonData(mon, MON_DATA_HP_IV),
            (int32_t)GetMonData(mon, MON_DATA_ATK_IV),
            (int32_t)GetMonData(mon, MON_DATA_DEF_IV),
            (int32_t)GetMonData(mon, MON_DATA_SPATK_IV),
            (int32_t)GetMonData(mon, MON_DATA_SPDEF_IV),
            (int32_t)GetMonData(mon, MON_DATA_SPEED_IV)
        };
        const int32_t evs[6] = {
            (int32_t)GetMonData(mon, MON_DATA_HP_EV),
            (int32_t)GetMonData(mon, MON_DATA_ATK_EV),
            (int32_t)GetMonData(mon, MON_DATA_DEF_EV),
            (int32_t)GetMonData(mon, MON_DATA_SPATK_EV),
            (int32_t)GetMonData(mon, MON_DATA_SPDEF_EV),
            (int32_t)GetMonData(mon, MON_DATA_SPEED_EV)
        };
        // gNatureStatTable columns: 0=Atk, 1=Def, 2=Speed, 3=SpAtk, 4=SpDef
        const int32_t natCols[6] = { -1, 0, 1, 3, 4, 2 };

        for (int32_t s = 0; s < 6; s++)
        {
            const int32_t ry = 63 + s * 20;
            uint16_t lblCol = UI_INK_DARK;
            if (natCols[s] >= 0)
            {
                const s8 mod = gNatureStatTable[nature][natCols[s]];
                if (mod > 0) lblCol = UI_INK_RED;
                else if (mod < 0) lblCol = UI_INK_BLUE;
            }
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, sStatLbl[s], 122, ry, lblCol, VITA_UI_NO_SHADOW);

            char num[16];
            if (s == 0)
                snprintf(num, sizeof(num), "%u/%d", (uint32_t)GetMonData(mon, MON_DATA_HP), (int32_t)vals[0]);
            else
                snprintf(num, sizeof(num), "%d", (int32_t)vals[s]);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, num, 214, ry, UI_INK_DARK, VITA_UI_NO_SHADOW);

            if (showIvEv)
            {
                snprintf(num, sizeof(num), "%d", (int32_t)ivs[s]);
                VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, num, 254, ry,
                                            ivs[s] >= 30 ? UI_INK_GOLD_DARK : UI_INK_MUTED, VITA_UI_NO_SHADOW);
                snprintf(num, sizeof(num), "%d", (int32_t)evs[s]);
                VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, num, 298, ry,
                                            evs[s] >= 252 ? UI_INK_GOLD_DARK : UI_INK_MUTED, VITA_UI_NO_SHADOW);
            }
        }
        VitaUi_DrawHpBar(dst, pitch, 122, 187, 180, 6,
                         (uint16_t)GetMonData(mon, MON_DATA_HP),
                         (uint16_t)GetMonData(mon, MON_DATA_MAX_HP));
    }
    else
    {
        // Battle Moves or Contest Moves
        const uint8_t ppBonuses = (uint8_t)GetMonData(mon, MON_DATA_PP_BONUSES);
        for (int32_t m = 0; m < 4; m++)
        {
            const uint16_t mv = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + m);
            const int32_t ry = 46 + m * 25;
            const bool sel = (m == sMoveCursor);
            const bool swap = (m == sMoveSwapFrom);

            if (sel || swap)
            {
                VitaUiPlate_Draw(
                    dst, pitch, clip, 118, ry, 192, 23, 3,
                    swap ? kPlateGold : kPlateCardSel,
                    VITA_PLATE_FOCUS, blinkPhase, 0);
            }
            if (mv == MOVE_NONE)
            {
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "---", 156, ry + 5, UI_INK_MUTED, VITA_UI_NO_SHADOW);
                continue;
            }

            if (sSumTab == SUM_TAB_BATTLE_MOVES)
            {
                VitaUiArt_DrawTypeBadge(dst, pitch, clip, gBattleMoves[mv].type, 122, ry + 5);
                VitaUiArt_DrawCategoryBadge(dst, pitch, clip, mv, 236, ry + 5);
            }
            else
            {
                static const char *const sContestLbl[5] = { "COOL", "BEAU", "CUTE", "SMRT", "TOUG" };
                const uint8_t cat = gContestMoves[mv].contestCategory;
                VitaUiArt_FillRect(dst, pitch, clip, 122, ry + 5, 30, 12, UI_INK_BLUE);
                VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, sContestLbl[cat % 5U], 137, ry + 5, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            }

            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gMoveNames[mv], 155, ry + 5, UI_INK_DARK, VITA_UI_NO_SHADOW);

            const uint8_t pp = (uint8_t)GetMonData(mon, MON_DATA_PP1 + m);
            const uint8_t maxPp = CalculatePPWithBonus(mv, ppBonuses, (u8)m);
            char ppBuf[16];
            snprintf(ppBuf, sizeof(ppBuf), "%u/%u", pp, maxPp);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, ppBuf, 306, ry + 5, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }

        // Selected move stats & description
        VitaUiArt_FillRect(dst, pitch, clip, 120, 148, 188, 1, UI_BORDER_SOFT);
        const uint16_t selMv = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + sMoveCursor);
        if (selMv != MOVE_NONE)
        {
            char statLine[48];
            if (sSumTab == SUM_TAB_BATTLE_MOVES)
            {
                const uint8_t pwr = gBattleMoves[selMv].power;
                const uint8_t acc = gBattleMoves[selMv].accuracy;
                snprintf(statLine, sizeof(statLine), "POW: %s   ACC: %s",
                         pwr == 0 ? "---" : "", acc == 0 ? "---" : "");
                if (pwr > 0 && acc > 0)
                    snprintf(statLine, sizeof(statLine), "POW: %u   ACC: %u%%", pwr, acc);
                else if (pwr > 0)
                    snprintf(statLine, sizeof(statLine), "POW: %u   ACC: ---", pwr);
                else if (acc > 0)
                    snprintf(statLine, sizeof(statLine), "POW: ---   ACC: %u%%", acc);
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, statLine, 122, 151, UI_INK_BLUE, VITA_UI_NO_SHADOW);
                if (gMoveDescriptionPointers[selMv - 1] != NULL)
                    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gMoveDescriptionPointers[selMv - 1], 122, 164, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
            else
            {
                const uint8_t eff = gContestMoves[selMv].effect;
                snprintf(statLine, sizeof(statLine), "APPEAL: %u HEARTS   JAM: %u",
                         gContestEffects[eff].appeal / 10U, gContestEffects[eff].jam / 10U);
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, statLine, 122, 151, UI_INK_RED, VITA_UI_NO_SHADOW);
                if (gContestEffectDescriptionPointers[eff] != NULL)
                    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gContestEffectDescriptionPointers[eff], 122, 164, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
        }
    }
}

void VitaUiParty_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase, const uint32_t animTick)
{
    if (sMode == PARTY_MODE_SUMMARY)
    {
        RenderSummaryView(dst, pitch, blinkPhase);
        return;
    }

    const VitaUiClip *const clip = VitaUi_FullClip();
    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();
    const uint8_t iconFrame = (uint8_t)((animTick >> 4) & 1U);

    // Emerald 3DS v4.2 Party Layout: Lead slot (102x96) on left + prompt box below, 5 slots (202x33) on right
    for (int32_t i = 0; i < PARTY_SIZE; i++)
    {
        const bool isLead = (i == 0);
        const int32_t cx = isLead ? 6 : 112;
        const int32_t cy = isLead ? 28 : (28 + (i - 1) * 35);
        const int32_t cw = isLead ? 102 : 202;
        const int32_t ch = isLead ? 96 : 33;

        if (i >= partyCnt)
        {
            VitaUiPlate_Draw(dst, pitch, clip, cx, cy, cw, ch, 4, kPlateSageDark, VITA_PLATE_DISABLED, blinkPhase, 0);
            continue;
        }

        struct Pokemon *const mon = &gPlayerParty[i];
        const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
        const uint16_t species = (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
        const uint16_t hp = (uint16_t)GetMonData(mon, MON_DATA_HP);
        const uint16_t maxHp = (uint16_t)GetMonData(mon, MON_DATA_MAX_HP);
        const bool isSel = (i == sSlot);
        const bool isSwapping = (i == sSwitchFrom);

        const VitaUiRgb plateRgb = isSwapping ? kPlateGold : (isSel ? kPlateCardSel : kPlateCard);
        VitaUiPlate_Draw(
            dst, pitch, clip, cx, cy, cw, ch, 4,
            plateRgb, isSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);

        uint8_t nickGba[POKEMON_NAME_LENGTH + 2];
        GetMonNickname(mon, nickGba);

        if (isLead)
        {
            if (isEgg)
                VitaUiArt_DrawMonIcon(dst, pitch, clip, SPECIES_EGG, 0, cx + 26, cy + 8, 2);
            else
                VitaUiArt_DrawMonFrontPic(dst, pitch, clip, species, IsMonShiny(mon), cx + 27, cy + 4, 48, false);

            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, nickGba, cx + 8, cy + 54, UI_INK_DARK, VITA_UI_NO_SHADOW);
            if (!isEgg)
            {
                char lvStr[20];
                snprintf(lvStr, sizeof(lvStr), "LV.%u", (uint32_t)GetMonData(mon, MON_DATA_LEVEL));
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lvStr, cx + 8, cy + 67, UI_INK_MUTED, VITA_UI_NO_SHADOW);
                DrawStatusBadge(dst, pitch, mon, cx + 74, cy + 67);
                VitaUi_DrawHpBar(dst, pitch, cx + 8, cy + 80, 86, 5, hp, maxHp);
                snprintf(lvStr, sizeof(lvStr), "%u/%u", hp, maxHp);
                VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lvStr, cx + 94, cy + 67, UI_INK_DARK, VITA_UI_NO_SHADOW);
            }
        }
        else
        {
            VitaUiArt_DrawMonIcon(dst, pitch, clip, isEgg ? SPECIES_EGG : species, isSel ? iconFrame : 0U, cx + 4, cy + 1, 1);
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, nickGba, cx + 38, cy + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
            if (!isEgg)
            {
                char lvStr[24];
                snprintf(lvStr, sizeof(lvStr), "LV.%u", (uint32_t)GetMonData(mon, MON_DATA_LEVEL));
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lvStr, cx + 38, cy + 17, UI_INK_MUTED, VITA_UI_NO_SHADOW);
                DrawStatusBadge(dst, pitch, mon, cx + 78, cy + 18);
                VitaUi_DrawHpBar(dst, pitch, cx + 106, cy + 8, 88, 5, hp, maxHp);
                snprintf(lvStr, sizeof(lvStr), "%u / %u", hp, maxHp);
                VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lvStr, cx + 194, cy + 16, UI_INK_DARK, VITA_UI_NO_SHADOW);

                if (GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NONE)
                    VitaUiArt_FillRect(dst, pitch, clip, cx + 28, cy + 22, 5, 5, UI_INK_GOLD);

                // Compatibility tag when teaching a TM/HM
                if (sMode == PARTY_MODE_BAG_TARGET && GetPocketByItemId(sPendingBagItem) == POCKET_TM_HM)
                {
                    const uint16_t mv = ItemIdToBattleMoveId(sPendingBagItem);
                    const bool knows = MonKnowsMove(mon, mv);
                    const bool able = CanMonLearnTMHM(mon, (uint8_t)(sPendingBagItem - ITEM_TM01));
                    VitaUiArt_DrawStrAsciiRight(
                        dst, pitch, clip, VITA_FONT_SMALL_NARROW,
                        knows ? "LEARNED" : (able ? "ABLE!" : "NO"),
                        cx + 194, cy + 16, knows ? UI_INK_BLUE : (able ? UI_HP_GREEN : UI_INK_RED), VITA_UI_NO_SHADOW);
                }
            }
        }
    }

    // Left bottom contextual prompt card
    VitaUiPlate_Draw(dst, pitch, clip, 6, 128, 102, 74, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 14);
    const char *hdr = "PARTY MENU";
    const char *sub = "CHOOSE A\nPOKEMON.";
    if (sMode == PARTY_MODE_SWITCH)           { hdr = "SWITCH"; sub = "MOVE TO\nWHICH SLOT?"; }
    else if (sMode == PARTY_MODE_BAG_TARGET)  { hdr = "USE ITEM"; sub = "USE ON WHICH\nPOKEMON?"; }
    else if (sMode == PARTY_MODE_SOFTBOILED_TARGET) { hdr = "SHARE HP"; sub = "HEAL WHICH\nALLY?"; }
    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, hdr, 11, 130, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, sub, 11, 148, UI_INK_DARK, VITA_UI_NO_SHADOW);

    // Action popup with blue Field Move plates
    if (sMode == PARTY_MODE_POPUP)
    {
        const int32_t ph = sPopupCount * 20 + 8;
        const int32_t py = 204 - ph;
        VitaUiPlate_Draw(dst, pitch, clip, 196, py, 118, ph, 4, kPlatePopup, VITA_PLATE_NORMAL, blinkPhase, 0);
        for (int32_t i = 0; i < sPopupCount; i++)
        {
            const bool pSel = (i == sPopupCursor);
            const bool isField = (sPopup[i].actionType == 5);
            const VitaUiRgb rgb = isField ? kPlateBlueMove : (pSel ? kPlateSage : kPlateSageDark);
            VitaUiPlate_Draw(
                dst, pitch, clip, 200, py + 4 + i * 20, 110, 18, 3,
                rgb, pSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
            VitaUiArt_DrawStrAsciiInPlate(
                dst, pitch, clip, VITA_FONT_SMALL, sPopup[i].label,
                200, py + 4 + i * 20, 110, 18, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        }
    }
    else if (sMode == PARTY_MODE_MOVE_SELECT || sMode == PARTY_MODE_REPLACE_5TH)
    {
        struct Pokemon *const mon = &gPlayerParty[sSlot];
        const int32_t rows = (sMode == PARTY_MODE_REPLACE_5TH) ? 5 : 4;
        VitaUiPlate_Draw(dst, pitch, clip, 120, 42, 192, 24 + rows * 24, 4, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 16);
        VitaUiArt_DrawStrAsciiCenter(
            dst, pitch, clip, VITA_FONT_SMALL,
            (sMode == PARTY_MODE_REPLACE_5TH) ? "FORGET WHICH MOVE?" : "SELECT A MOVE",
            216, 44, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        for (int32_t m = 0; m < rows; m++)
        {
            const uint16_t mv = (m < 4) ? (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + m) : sPendingLearnMove;
            const int32_t my = 62 + m * 23;
            const bool mSel = (m == sMoveCursor);
            VitaUiPlate_Draw(
                dst, pitch, clip, 126, my, 180, 20, 3,
                m == 4 ? kPlateBlueMove : (mSel ? kPlateSage : kPlateSageDark),
                mSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
            if (mv != MOVE_NONE)
            {
                VitaUiArt_DrawTypeBadge(dst, pitch, clip, gBattleMoves[mv].type, 130, my + 4);
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, gMoveNames[mv], 166, my + 3, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            }
            else
            {
                VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "---", 166, my + 3, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            }
        }
    }
    else if (sMode == PARTY_MODE_DELTA_CARD)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 126, 44, 184, 148, 4, kPlateCream, VITA_PLATE_FOCUS, blinkPhase, 16);
        char hdrBuf[32];
        if (sAfterLevel > sBeforeLevel)
            snprintf(hdrBuf, sizeof(hdrBuf), "GREW TO LV. %u!", sAfterLevel);
        else
            snprintf(hdrBuf, sizeof(hdrBuf), "STATS INCREASED!");
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, hdrBuf, 218, 46, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        static const char *const sNames[6] = { "MAX HP", "ATTACK", "DEFENSE", "SPEED", "SP.ATK", "SP.DEF" };
        for (int32_t s = 0; s < 6; s++)
        {
            const int32_t ry = 66 + s * 19;
            const int32_t diff = (int32_t)sAfterStats[s] - (int32_t)sBeforeStats[s];
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, sNames[s], 134, ry, UI_INK_DARK, VITA_UI_NO_SHADOW);
            char vBuf[24];
            if (diff > 0)
                snprintf(vBuf, sizeof(vBuf), "%d (+%d)", (int32_t)sAfterStats[s], (int32_t)diff);
            else
                snprintf(vBuf, sizeof(vBuf), "%d", (int32_t)sAfterStats[s]);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, vBuf, 302, ry,
                                        diff > 0 ? UI_HP_GREEN : UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
    }
}
