#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "main.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "data.h"
#include "item.h"
#include "pokemon.h"
#include "safari_zone.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"

#include "vita_ui_internal.h"
#include "platform/vita_battle_ui.h"
#include "platform/vita_ui_art.h"
#include "platform/vita_ui_plate.h"
#include "vita_platform.h"

static const VitaUiRgb sTypeColours[NUMBER_OF_MON_TYPES] = {
    [TYPE_NORMAL]   = { 168, 168, 120 },
    [TYPE_FIGHTING] = { 192,  48,  40 },
    [TYPE_FLYING]   = { 168, 144, 240 },
    [TYPE_POISON]   = { 160,  64, 160 },
    [TYPE_GROUND]   = { 224, 192, 104 },
    [TYPE_ROCK]     = { 184, 160,  56 },
    [TYPE_BUG]      = { 168, 184,  32 },
    [TYPE_GHOST]    = { 112,  88, 152 },
    [TYPE_STEEL]    = { 184, 184, 208 },
    [TYPE_MYSTERY]  = { 104, 160, 144 },
    [TYPE_FIRE]     = { 240, 128,  48 },
    [TYPE_WATER]    = { 104, 144, 240 },
    [TYPE_GRASS]    = { 120, 200,  80 },
    [TYPE_ELECTRIC] = { 248, 208,  48 },
    [TYPE_PSYCHIC]  = { 248,  88, 136 },
    [TYPE_ICE]      = { 152, 216, 216 },
    [TYPE_DRAGON]   = { 112,  56, 248 },
    [TYPE_DARK]     = { 112,  88,  72 },
};

static const VitaUiRgb sHueFight  = { 228,  64,  52 };
static const VitaUiRgb sHueBall   = { 232, 180,  52 };
static const VitaUiRgb sHueBag    = {  64, 140, 212 };
static const VitaUiRgb sHueMon    = {  72, 184, 108 };
static const VitaUiRgb sHueRun    = { 208,  92,  60 };
static const VitaUiRgb sHueCancel = {  88, 112, 124 };
static const VitaUiRgb sHueEmpty  = { 140, 150, 154 };

static enum VitaBattleUiMode sCurrentMode = VITA_BATTLE_MODE_NONE;
static uint8_t sCursorPos = 0;
static uint8_t sBlinkPhase = 0;
static uint8_t sBattleSpeed = 1;

void VitaBattleUi_Init(void)
{
    sCurrentMode = VITA_BATTLE_MODE_NONE;
    sCursorPos = 0;
    sBlinkPhase = 0;
}

void VitaBattleUi_Shutdown(void)
{
    sCurrentMode = VITA_BATTLE_MODE_NONE;
}

bool VitaBattleUi_IsActive(void)
{
    return gMain.inBattle && (sCurrentMode != VITA_BATTLE_MODE_NONE);
}

void VitaBattleUi_SetMode(const enum VitaBattleUiMode mode)
{
    sCurrentMode = mode;
    sCursorPos = 0;
}

enum VitaBattleUiMode VitaBattleUi_GetMode(void)
{
    return sCurrentMode;
}

uint8_t VitaBattleUi_GetSpeed(void)
{
    return sBattleSpeed;
}

void VitaBattleUi_CycleSpeed(void)
{
    sBattleSpeed = (uint8_t)((sBattleSpeed % 4U) + 1U);
}

static int32_t CalculateEffectivenessMultiplier(const uint8_t moveType, const uint16_t targetSpecies)
{
    if (moveType >= NUMBER_OF_MON_TYPES || targetSpecies == SPECIES_NONE)
        return 10;

    const uint8_t defType1 = gSpeciesInfo[targetSpecies].types[0];
    const uint8_t defType2 = gSpeciesInfo[targetSpecies].types[1];
    int32_t mult = 10;

    extern const u8 gTypeEffectiveness[336];
    for (size_t i = 0; i < sizeof(gTypeEffectiveness); i += 3)
    {
        if (gTypeEffectiveness[i] == moveType)
        {
            if (gTypeEffectiveness[i + 1] == defType1)
            {
                const uint8_t m = gTypeEffectiveness[i + 2];
                if (m == TYPE_MUL_NO_EFFECT) mult = 0;
                else if (m == TYPE_MUL_NOT_EFFECTIVE) mult = (mult * 5) / 10;
                else if (m == TYPE_MUL_SUPER_EFFECTIVE) mult = (mult * 20) / 10;
            }
            if (defType2 != defType1 && gTypeEffectiveness[i + 1] == defType2)
            {
                const uint8_t m = gTypeEffectiveness[i + 2];
                if (m == TYPE_MUL_NO_EFFECT) mult = 0;
                else if (m == TYPE_MUL_NOT_EFFECTIVE) mult = (mult * 5) / 10;
                else if (m == TYPE_MUL_SUPER_EFFECTIVE) mult = (mult * 20) / 10;
            }
        }
    }
    return mult;
}

static uint16_t GetItemQuantityInBag(const uint16_t itemId)
{
    const uint8_t pocket = GetPocketByItemId(itemId);
    if (pocket == 0 || pocket > POCKETS_COUNT)
        return 0;

    const struct BagPocket *const bp = &gBagPockets[pocket - 1];
    for (uint16_t i = 0; i < bp->capacity; ++i)
    {
        if (bp->itemSlots[i].itemId == itemId)
            return (uint16_t)(bp->itemSlots[i].quantity ^ gSaveBlock2Ptr->encryptionKey);
    }
    return 0;
}

static uint16_t FindBestQuickBall(uint16_t *const outCount)
{
    static const uint16_t sBallPriority[] = {
        ITEM_ULTRA_BALL, ITEM_GREAT_BALL, ITEM_POKE_BALL,
        ITEM_NET_BALL, ITEM_DIVE_BALL, ITEM_NEST_BALL,
        ITEM_REPEAT_BALL, ITEM_TIMER_BALL, ITEM_LUXURY_BALL, ITEM_PREMIER_BALL
    };

    for (size_t i = 0; i < ARRAY_COUNT(sBallPriority); ++i)
    {
        const uint16_t ball = sBallPriority[i];
        const uint16_t count = GetItemQuantityInBag(ball);
        if (count > 0)
        {
            if (outCount != NULL)
                *outCount = count;
            return ball;
        }
    }
    if (outCount != NULL)
        *outCount = 0;
    return ITEM_NONE;
}

bool VitaBattleUi_Update(const uint32_t buttons)
{
    if (!VitaBattleUi_IsActive())
        return false;

    sBlinkPhase = (uint8_t)(sBlinkPhase + 1U);

    if (buttons & VITA_BTN_R)
    {
        VitaBattleUi_CycleSpeed();
        return true;
    }

    if (sCurrentMode == VITA_BATTLE_MODE_ACTION)
    {
        if (buttons & VITA_BTN_LEFT)
        {
            if (sCursorPos == 1) sCursorPos = 0;
            else if (sCursorPos == 3) sCursorPos = 2;
        }
        else if (buttons & VITA_BTN_RIGHT)
        {
            if (sCursorPos == 0) sCursorPos = 1;
            else if (sCursorPos == 2) sCursorPos = 3;
        }
        else if (buttons & VITA_BTN_UP)
        {
            if (sCursorPos == 2) sCursorPos = 0;
            else if (sCursorPos == 3) sCursorPos = 1;
        }
        else if (buttons & VITA_BTN_DOWN)
        {
            if (sCursorPos == 0) sCursorPos = 2;
            else if (sCursorPos == 1) sCursorPos = 3;
        }

        if (buttons & VITA_BTN_CROSS)
        {
            gActionSelectionCursor[gActiveBattler] = sCursorPos;
            if (sCursorPos == 0)
                sCurrentMode = VITA_BATTLE_MODE_MOVES;
            return true;
        }
    }
    else if (sCurrentMode == VITA_BATTLE_MODE_MOVES)
    {
        if (buttons & VITA_BTN_LEFT)
        {
            if (sCursorPos == 1) sCursorPos = 0;
            else if (sCursorPos == 3) sCursorPos = 2;
        }
        else if (buttons & VITA_BTN_RIGHT)
        {
            if (sCursorPos == 0) sCursorPos = 1;
            else if (sCursorPos == 2) sCursorPos = 3;
        }
        else if (buttons & VITA_BTN_UP)
        {
            if (sCursorPos == 2) sCursorPos = 0;
            else if (sCursorPos == 3) sCursorPos = 1;
            else if (sCursorPos == 4) sCursorPos = 2;
        }
        else if (buttons & VITA_BTN_DOWN)
        {
            if (sCursorPos == 0) sCursorPos = 2;
            else if (sCursorPos == 1) sCursorPos = 3;
            else if (sCursorPos == 2 || sCursorPos == 3) sCursorPos = 4; // Cancel
        }
        else if (buttons & VITA_BTN_CIRCLE)
        {
            sCurrentMode = VITA_BATTLE_MODE_ACTION;
            sCursorPos = 0;
            return true;
        }

        if (buttons & VITA_BTN_CROSS)
        {
            if (sCursorPos == 4)
            {
                sCurrentMode = VITA_BATTLE_MODE_ACTION;
                sCursorPos = 0;
            }
            else
            {
                gMoveSelectionCursor[gActiveBattler] = sCursorPos;
                const bool isDouble = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0;
                if (isDouble)
                    sCurrentMode = VITA_BATTLE_MODE_TARGET;
                else
                    sCurrentMode = VITA_BATTLE_MODE_NONE;
            }
            return true;
        }
    }
    else if (sCurrentMode == VITA_BATTLE_MODE_TARGET)
    {
        if (buttons & (VITA_BTN_UP | VITA_BTN_LEFT))
            sCursorPos = 0;
        else if (buttons & (VITA_BTN_DOWN | VITA_BTN_RIGHT))
            sCursorPos = 1;
        else if (buttons & VITA_BTN_CIRCLE)
        {
            sCurrentMode = VITA_BATTLE_MODE_MOVES;
            sCursorPos = 0;
            return true;
        }
        else if (buttons & VITA_BTN_CROSS)
        {
            sCurrentMode = VITA_BATTLE_MODE_NONE;
            return true;
        }
    }

    return false;
}

static void RenderBattleActionsDeck(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    const bool inSafari = (gBattleTypeFlags & BATTLE_TYPE_SAFARI) != 0;
    uint16_t quickCount = 0;
    const uint16_t quickBall = inSafari ? ITEM_NONE : FindBestQuickBall(&quickCount);
    const bool hasQuickBall = (quickBall != ITEM_NONE && quickCount > 0);

    const int32_t fightW = hasQuickBall ? 224 : 308;
    const uint8_t fightState = (sCursorPos == 0) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;
    VitaUiPlate_DrawBtaPlate(dst, pitch, clip, hasQuickBall ? VITA_BTA_FIGHT_WIDE : VITA_BTA_FIGHT_FULL,
                             6, 120, fightW, 46, sHueFight, fightState, sBlinkPhase);

    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_NORMAL, inSafari ? "BALL" : "FIGHT",
                           24, 134, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    if (hasQuickBall)
    {
        const uint8_t ballState = (sCursorPos == 4) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;
        VitaUiPlate_DrawBtaPlate(dst, pitch, clip, VITA_BTA_BALL, 236, 120, 78, 46, sHueBall, ballState, sBlinkPhase);
        char countBuf[16];
        snprintf(countBuf, sizeof(countBuf), "x%d", (int32_t)quickCount);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, countBuf, 275, 148, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }

    // Bottom action row: BAG, POKEMON, RUN
    static const int32_t rowX[3] = { 6, 110, 214 };
    static const char *const labels[3] = { "BAG", "POKEMON", "RUN" };
    static const char *const safariLabels[3] = { "POKEBLOCK", "GO NEAR", "RUN" };
    const VitaUiRgb hues[3] = { sHueBag, sHueMon, sHueRun };

    for (int32_t k = 0; k < 3; ++k)
    {
        const uint8_t state = (sCursorPos == k + 1) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;
        VitaUiPlate_DrawBtaPlate(dst, pitch, clip, VITA_BTA_BOTTOM, rowX[k], 174, 100, 48, hues[k], state, sBlinkPhase);

        const char *const lbl = inSafari ? safariLabels[k] : labels[k];
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lbl, rowX[k] + 50, 198, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        if (k == 1 && !inSafari)
        {
            for (int32_t b = 0; b < PARTY_SIZE; ++b)
            {
                const struct Pokemon *const mon = &gPlayerParty[b];
                const uint16_t species = (uint16_t)GetMonData((struct Pokemon *)mon, MON_DATA_SPECIES);
                uint8_t ballElem = VITA_BTA_BALL_EMPTY;
                if (species != SPECIES_NONE)
                {
                    const uint16_t hp = (uint16_t)GetMonData((struct Pokemon *)mon, MON_DATA_HP);
                    const uint32_t st1 = (uint32_t)GetMonData((struct Pokemon *)mon, MON_DATA_STATUS);
                    if (hp == 0) ballElem = VITA_BTA_BALL_FAINT;
                    else if (st1 != 0) ballElem = VITA_BTA_BALL_STATUS;
                    else ballElem = VITA_BTA_BALL_OK;
                }
                VitaUiPlate_DrawBtaElem(dst, pitch, clip, ballElem, rowX[1] + 16 + b * 12, 180, hues[1]);
            }
        }
    }
}

static void RenderBattleMovesDeck(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    const struct BattlePokemon *const mon = &gBattleMons[gActiveBattler];
    const uint8_t opponent = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    const uint16_t targetSpecies = (opponent < MAX_BATTLERS_COUNT) ? gBattleMons[opponent].species : SPECIES_NONE;

    static const int32_t moveX[4] = { 8, 164, 8, 164 };
    static const int32_t moveY[4] = { 24, 24, 106, 106 };

    for (int32_t i = 0; i < MAX_MON_MOVES; ++i)
    {
        const uint16_t move = mon->moves[i];
        const int32_t x = moveX[i];
        const int32_t y = moveY[i];

        if (move == MOVE_NONE)
        {
            VitaUiPlate_DrawBtaPlate(dst, pitch, clip, VITA_BTA_MOVE, x, y, 148, 76, sHueEmpty, VITA_PLATE_DISABLED, 0);
            continue;
        }

        const uint8_t moveType = gBattleMoves[move].type;
        const VitaUiRgb typeCol = (moveType < NUMBER_OF_MON_TYPES) ? sTypeColours[moveType] : sHueEmpty;
        const uint8_t state = (sCursorPos == i) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;

        VitaUiPlate_DrawBtaPlate(dst, pitch, clip, VITA_BTA_MOVE, x, y, 148, 76, typeCol, state, sBlinkPhase);

        char moveName[32];
        VitaUiArt_GbaToAscii(gMoveNames[move], moveName, sizeof(moveName));
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_NORMAL, moveName, x + 10, y + 6, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        // PP String: "PP XX/YY"
        char ppBuf[24];
        snprintf(ppBuf, sizeof(ppBuf), "PP %d/%d", (int32_t)mon->pp[i], (int32_t)gBattleMoves[move].pp);
        const uint16_t ppCol = (mon->pp[i] == 0) ? UI_INK_RED
                             : (mon->pp[i] <= gBattleMoves[move].pp / 4) ? UI_INK_GOLD
                             : UI_INK_LIGHT;
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, ppBuf, x + 10, y + 32, ppCol, VITA_UI_NO_SHADOW);

        // Move split category: PHY / SPC / STA
        const uint8_t category = (gBattleMoves[move].power == 0) ? MOVE_CATEGORY_STATUS
                               : (IS_MOVE_PHYSICAL(move, moveType) ? MOVE_CATEGORY_PHYSICAL : MOVE_CATEGORY_SPECIAL);
        const char *const catStr = (category == MOVE_CATEGORY_PHYSICAL) ? "PHY"
                                 : (category == MOVE_CATEGORY_SPECIAL) ? "SPC" : "STA";
        VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, catStr, x + 138, y + 32, UI_INK_MINT, VITA_UI_NO_SHADOW);

        // Effectiveness hint against target
        const int32_t mult = CalculateEffectivenessMultiplier(moveType, targetSpecies);
        if (mult == 0)
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "NO EFFECT", x + 10, y + 54, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        else if (mult > 10)
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "SUPER EFF.", x + 10, y + 54, UI_INK_GOLD, VITA_UI_NO_SHADOW);
        else if (mult < 10)
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "NOT VERY", x + 10, y + 54, UI_INK_RED, VITA_UI_NO_SHADOW);
    }

    // Cancel Button
    const uint8_t cancelState = (sCursorPos == 4) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;
    VitaUiPlate_DrawBtaPlate(dst, pitch, clip, VITA_BTA_CANCEL, 84, 192, 152, 26, sHueCancel, cancelState, sBlinkPhase);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_NORMAL, "CANCEL", 160, 196, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
}

static void RenderBattleTargetDeck(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    VitaUiPlate_Draw(dst, pitch, clip, 20, 20, 280, 200, 6, kPlateCard, VITA_PLATE_NORMAL, 0, 18);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_NORMAL, "CHOOSE TARGET", 160, 24, UI_INK_DARK, VITA_UI_NO_SHADOW);

    for (int32_t i = 0; i < 2; ++i)
    {
        const uint8_t target = GetBattlerAtPosition(i == 0 ? B_POSITION_OPPONENT_LEFT : B_POSITION_OPPONENT_RIGHT);
        if (target >= MAX_BATTLERS_COUNT || (gAbsentBattlerFlags & (1u << target)))
            continue;

        const struct BattlePokemon *const mon = &gBattleMons[target];
        const int32_t y = 60 + i * 60;
        const uint8_t state = (sCursorPos == i) ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL;

        VitaUiPlate_Draw(dst, pitch, clip, 40, y, 240, 48, 4, kPlateSage, state, sBlinkPhase, 0);

        char nameBuf[32];
        VitaUiArt_GbaToAscii(mon->nickname, nameBuf, sizeof(nameBuf));
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_NORMAL, nameBuf, 52, y + 14, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        char hpBuf[24];
        snprintf(hpBuf, sizeof(hpBuf), "HP: %d/%d", (int32_t)mon->hp, (int32_t)mon->maxHP);
        VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL, hpBuf, 268, y + 16, UI_INK_MINT, VITA_UI_NO_SHADOW);
    }
}

void VitaBattleUi_Render(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    VitaUiPlate_BlitBattleBackdrop(dst, pitch, clip);

    // Speed indicator in top right header
    char spdBuf[16];
    snprintf(spdBuf, sizeof(spdBuf), "SPD %ux", (uint32_t)sBattleSpeed);
    VitaUiPlate_Draw(dst, pitch, clip, 256, 4, 58, 16, 2, kPlateSageDark, VITA_PLATE_NORMAL, 0, 0);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, spdBuf, 285, 6, UI_INK_MINT, VITA_UI_NO_SHADOW);

    switch (sCurrentMode)
    {
    case VITA_BATTLE_MODE_ACTION:
        RenderBattleActionsDeck(dst, pitch, clip);
        break;
    case VITA_BATTLE_MODE_MOVES:
        RenderBattleMovesDeck(dst, pitch, clip);
        break;
    case VITA_BATTLE_MODE_TARGET:
        RenderBattleTargetDeck(dst, pitch, clip);
        break;
    default:
        break;
    }
}
