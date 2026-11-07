// Game data and actions for the new UI's screens. Everything here runs on Core 1 (the game
// thread, from Ui_Tick between two game frames), so it reads and changes game state directly.
// The game-side logic is the same as the current Vita UI (src/platform/vita/vita_ui*.c), which
// stays selectable while the new UI is being built (the "NEW UI (TEST)" option).

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/vita_daynight.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "overworld.h"
#include "item.h"
#include "load_save.h"
#include "new_game.h"
#include "pokedex.h"
#include "string_util.h"
#include "money.h"
#include "platform.h"
#include "pokemon.h"
#include "region_map.h"
#include "safari_zone.h"
#include "save.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/species.h"
#include "constants/vars.h"
#include "constants/moves.h"
#include "constants/battle.h"
#include "battle.h"
#include "battle_main.h"
#include "contest_effect.h"
#include "data.h"
#include "pokemon_summary_screen.h"

// ---------- Text ----------

// GBA character -> Latin-1 code point (0 = skip). Covers the Western character set of the game.
static uint16_t GbaToLatin1(uint8_t c)
{
    if (c >= CHAR_A && c <= CHAR_Z) return (uint16_t)('A' + c - CHAR_A);
    if (c >= CHAR_a && c <= CHAR_z) return (uint16_t)('a' + c - CHAR_a);
    if (c >= CHAR_0 && c <= CHAR_9) return (uint16_t)('0' + c - CHAR_0);
    switch (c)
    {
    case CHAR_SPACE: case CHAR_SPACER: return ' ';
    case CHAR_A_GRAVE: return 0xC0; case CHAR_A_ACUTE: return 0xC1; case CHAR_A_CIRCUMFLEX: return 0xC2;
    case CHAR_C_CEDILLA: return 0xC7; case CHAR_E_GRAVE: return 0xC8; case CHAR_E_ACUTE: return 0xC9;
    case CHAR_E_CIRCUMFLEX: return 0xCA; case CHAR_E_DIAERESIS: return 0xCB; case CHAR_I_GRAVE: return 0xCC;
    case CHAR_I_ACUTE: return 0xCD; case CHAR_I_CIRCUMFLEX: return 0xCE; case CHAR_I_DIAERESIS: return 0xCF;
    case CHAR_O_GRAVE: return 0xD2; case CHAR_O_ACUTE: return 0xD3; case CHAR_O_CIRCUMFLEX: return 0xD4;
    case CHAR_U_GRAVE: return 0xD9; case CHAR_U_ACUTE: return 0xDA; case CHAR_U_CIRCUMFLEX: return 0xDB;
    case CHAR_N_TILDE: return 0xD1; case CHAR_ESZETT: return 0xDF;
    case CHAR_a_GRAVE: return 0xE0; case CHAR_a_ACUTE: return 0xE1; case CHAR_a_CIRCUMFLEX: return 0xE2;
    case CHAR_c_CEDILLA: return 0xE7; case CHAR_e_GRAVE: return 0xE8; case CHAR_e_ACUTE: return 0xE9;
    case CHAR_e_CIRCUMFLEX: return 0xEA; case CHAR_e_DIAERESIS: return 0xEB; case CHAR_i_GRAVE: return 0xEC;
    case CHAR_i_ACUTE: return 0xED; case CHAR_i_CIRCUMFLEX: return 0xEE; case CHAR_i_DIAERESIS: return 0xEF;
    case CHAR_o_GRAVE: return 0xF2; case CHAR_o_ACUTE: return 0xF3; case CHAR_o_CIRCUMFLEX: return 0xF4;
    case CHAR_u_GRAVE: return 0xF9; case CHAR_u_ACUTE: return 0xFA; case CHAR_u_CIRCUMFLEX: return 0xFB;
    case CHAR_n_TILDE: return 0xF1;
    case CHAR_A_DIAERESIS: return 0xC4; case CHAR_O_DIAERESIS: return 0xD6; case CHAR_U_DIAERESIS: return 0xDC;
    case CHAR_a_DIAERESIS: return 0xE4; case CHAR_o_DIAERESIS: return 0xF6; case CHAR_u_DIAERESIS: return 0xFC;
    case CHAR_MASCULINE_ORDINAL: return 0xBA; case CHAR_FEMININE_ORDINAL: return 0xAA;
    case CHAR_AMPERSAND: return '&'; case CHAR_PLUS: return '+'; case CHAR_EQUALS: return '=';
    case CHAR_SEMICOLON: return ';'; case CHAR_INV_QUESTION_MARK: return 0xBF; case CHAR_INV_EXCL_MARK: return 0xA1;
    case CHAR_PERCENT: return '%'; case CHAR_LEFT_PAREN: return '('; case CHAR_RIGHT_PAREN: return ')';
    case CHAR_LESS_THAN: return '<'; case CHAR_GREATER_THAN: return '>';
    case CHAR_EXCL_MARK: return '!'; case CHAR_QUESTION_MARK: return '?'; case CHAR_PERIOD: return '.';
    case CHAR_HYPHEN: return '-'; case CHAR_ELLIPSIS: return '.';
    case CHAR_DBL_QUOTE_LEFT: case CHAR_DBL_QUOTE_RIGHT: return '"';
    case CHAR_SGL_QUOTE_LEFT: case CHAR_SGL_QUOTE_RIGHT: return '\'';
    case CHAR_MALE: return '?'; case CHAR_FEMALE: return '?';       // not in the fonts: drawn as icons
    case CHAR_CURRENCY: return '$'; case CHAR_COMMA: return ','; case CHAR_MULT_SIGN: return 'x';
    case CHAR_SLASH: return '/'; case CHAR_COLON: return ':';
    default: return 0;
    }
}

static size_t PutLatin1(char *dst, size_t at, size_t cap, uint16_t cp)
{
    if (cp < 0x80)
    {
        if (at + 1 < cap) dst[at++] = (char)cp;
    }
    else if (at + 2 < cap)
    {
        dst[at++] = (char)(0xC0 | (cp >> 6));
        dst[at++] = (char)(0x80 | (cp & 0x3F));
    }
    return at;
}

size_t UiGame_Text(const uint8_t *src, char *dst, size_t cap)
{
    size_t at = 0;
    if (cap == 0)
        return 0;
    for (; src != NULL && *src != EOS; src++)
    {
        const uint8_t c = *src;
        if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR)
        {
            at = PutLatin1(dst, at, cap, ' ');
            continue;
        }
        if (c == EXT_CTRL_CODE_BEGIN)
        {
            src++;                      // control code + its argument bytes are skipped
            if (*src == EOS) break;
            src += GetExtCtrlCodeLength(*src) - 1;
            continue;
        }
        if (c == CHAR_PK) { at = PutLatin1(dst, at, cap, 'P'); at = PutLatin1(dst, at, cap, 'K'); continue; }
        if (c == CHAR_MN) { at = PutLatin1(dst, at, cap, 'M'); at = PutLatin1(dst, at, cap, 'N'); continue; }
        const uint16_t cp = GbaToLatin1(c);
        if (cp != 0)
            at = PutLatin1(dst, at, cap, cp);
    }
    dst[at] = 0;
    return at;
}

// ---------- Player state ----------

uint32_t UiGame_Money(void)
{
    return GetMoney(&gSaveBlock1Ptr->money);
}

void UiGame_Clock(char *out, size_t cap)
{
    unsigned sec = 12U * 3600U, rate = 1U;
    VitaDayNight_Time(&sec, &rate);
    snprintf(out, cap, "%02u:%02u", (sec / 3600U) % 24U, (sec / 60U) % 60U);
}

bool UiGame_IsNight(void)
{
    unsigned sec = 12U * 3600U, rate = 1U;
    VitaDayNight_Time(&sec, &rate);
    const unsigned hh = (sec / 3600U) % 24U;
    return hh < 6U || hh >= 20U;
}

unsigned UiGame_ClockRate(void)
{
    unsigned sec = 0, rate = 1U;
    VitaDayNight_Time(&sec, &rate);
    return rate;
}

void UiGame_ZoneName(char *out, size_t cap)
{
    u8 name[32];
    GetMapNameGeneric(name, CorrectSpecialMapSecId(gMapHeader.regionMapSectionId));
    UiGame_Text(name, out, cap);
}

void UiGame_PlayerName(char *out, size_t cap)
{
    UiGame_Text(gSaveBlock2Ptr->playerName, out, cap);
}

bool UiGame_PlayerIsMale(void)
{
    return gSaveBlock2Ptr->playerGender == MALE;
}

uint16_t UiGame_RegisteredItem(void)
{
    const uint16_t item = gSaveBlock1Ptr->registeredItem;
    return (item != ITEM_NONE && CheckBagHasItem(item, 1)) ? item : ITEM_NONE;
}

void UiGame_ItemName(uint16_t itemId, char *out, size_t cap)
{
    u8 name[ITEM_NAME_LENGTH + 4];
    CopyItemName(itemId, name);
    UiGame_Text(name, out, cap);
}

uint16_t UiGame_RepelSteps(void)
{
    return VarGet(VAR_REPEL_STEP_COUNT);
}

bool UiGame_InSafari(void)
{
    return GetSafariZoneFlag();
}

uint8_t UiGame_SafariBalls(void)
{
    return gNumSafariBalls;
}

bool UiGame_HasFlag(uint16_t flag)
{
    return FlagGet(flag);
}

// ---------- Party ----------

int32_t UiGame_PartyCount(void)
{
    return (int32_t)CalculatePlayerPartyCount();
}

bool UiGame_Mon(int32_t slot, UiMon *out)
{
    memset(out, 0, sizeof(*out));
    if (slot < 0 || slot >= UiGame_PartyCount())
        return false;
    struct Pokemon *mon = &gPlayerParty[slot];
    out->species = (uint16_t)GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
    out->egg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
    out->level = (uint8_t)GetMonData(mon, MON_DATA_LEVEL);
    out->hp = (uint16_t)GetMonData(mon, MON_DATA_HP);
    out->maxHp = (uint16_t)GetMonData(mon, MON_DATA_MAX_HP);
    out->item = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
    out->shiny = !out->egg && IsMonShiny(mon);
    out->gender = out->egg ? MON_GENDERLESS : GetMonGender(mon);
    out->status = GetMonData(mon, MON_DATA_STATUS);
    u8 nick[POKEMON_NAME_LENGTH + 1];
    GetMonData(mon, MON_DATA_NICKNAME, nick);
    nick[POKEMON_NAME_LENGTH] = EOS;
    if (out->egg)
        snprintf(out->name, sizeof(out->name), "EGG");
    else
        UiGame_Text(nick, out->name, sizeof(out->name));
    return true;
}

// ---------- Save ----------

bool UiGame_Save(void)
{
    IncrementGameStat(GAME_STAT_SAVED_GAME);
    const u8 saveType = gDifferentSaveFile ? SAVE_OVERWRITE_DIFFERENT_FILE : SAVE_NORMAL;
    if (TrySavingData(saveType) != SAVE_STATUS_OK)
        return false;
    Platform_StoreSaveFile();
    VitaNuzlocke_SaveForSlot((int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE));
    gDifferentSaveFile = FALSE;
    return true;
}

uint8_t UiGame_SaveSlot(void)
{
    return (uint8_t)(Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE) + 1);
}

bool UiGame_SaveOverwritesOtherGame(void)
{
    return gDifferentSaveFile;
}

uint8_t UiGame_BadgeCount(void)
{
    uint8_t n = 0;
    for (uint16_t f = FLAG_BADGE01_GET; f <= FLAG_BADGE08_GET; f++)
        if (FlagGet(f))
            n++;
    return n;
}

uint16_t UiGame_DexOwned(void)
{
    return GetHoennPokedexCount(FLAG_GET_CAUGHT);
}

void UiGame_PlayTime(char *out, size_t cap)
{
    snprintf(out, cap, "%u:%02u", (unsigned)gSaveBlock2Ptr->playTimeHours, (unsigned)gSaveBlock2Ptr->playTimeMinutes);
}

// ---------- Summary ----------

int32_t UiGame_MonStatusIcon(int32_t slot)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return -1;
    if (GetMonData(mon, MON_DATA_HP) == 0)
        return 6;                                         // FNT
    const uint32_t st = GetMonData(mon, MON_DATA_STATUS);
    if (st & (STATUS1_POISON | STATUS1_TOXIC_POISON)) return 0;
    if (st & STATUS1_PARALYSIS) return 1;
    if (st & STATUS1_SLEEP) return 2;
    if (st & STATUS1_FREEZE) return 3;
    if (st & STATUS1_BURN) return 4;
    if (CheckPartyPokerus(mon, 1)) return 5;              // PKRS
    return -1;
}

void UiGame_Summary(int32_t slot, UiSummary *s)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    memset(s, 0, sizeof(*s));
    const uint16_t sp = (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
    u8 buf[64];
    s->dexNo = SpeciesToNationalPokedexNum(sp);
    GetSpeciesName(buf, sp);
    UiGame_Text(buf, s->species, sizeof(s->species));
    s->types[0] = gSpeciesInfo[sp].types[0];
    s->types[1] = gSpeciesInfo[sp].types[1];
    GetMonData(mon, MON_DATA_OT_NAME, buf);
    buf[PLAYER_NAME_LENGTH] = EOS;
    UiGame_Text(buf, s->ot, sizeof(s->ot));
    s->otId = (uint16_t)(GetMonData(mon, MON_DATA_OT_ID) & 0xFFFF);
    const uint8_t nature = GetNature(mon);
    UiGame_Text(gNatureNamePointers[nature], s->nature, sizeof(s->nature));
    // Nature: +10 % on one stat, -10 % on another (ATK, DEF, SPEED, SP.ATK, SP.DEF); same = neutral
    static const int8_t toDisplay[5] = { 1, 2, 5, 3, 4 };
    if (nature / 5 != nature % 5)
    {
        s->natureUp = toDisplay[nature / 5];
        s->natureDown = toDisplay[nature % 5];
    }
    const uint8_t ability = GetAbilityBySpecies(sp, (u8)GetMonData(mon, MON_DATA_ABILITY_NUM));
    UiGame_Text(gAbilityNames[ability], s->ability, sizeof(s->ability));
    UiGame_Text(gAbilityDescriptionPointers[ability], s->abilityDesc, sizeof(s->abilityDesc));
    GetMapNameGeneric(buf, (u16)GetMonData(mon, MON_DATA_MET_LOCATION));
    UiGame_Text(buf, s->metAt, sizeof(s->metAt));
    s->metLevel = (uint8_t)GetMonData(mon, MON_DATA_MET_LEVEL);
    s->item = (uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM);
    s->level = (uint8_t)GetMonData(mon, MON_DATA_LEVEL);
    const uint8_t rate = gSpeciesInfo[sp].growthRate;
    const uint32_t exp = GetMonData(mon, MON_DATA_EXP);
    if (s->level < MAX_LEVEL)
    {
        const uint32_t lo = gExperienceTables[rate][s->level], hi = gExperienceTables[rate][s->level + 1];
        s->expToNext = hi > exp ? hi - exp : 0;
        s->expPct = hi > lo ? (uint8_t)((exp - lo) * 100 / (hi - lo)) : 100;
    }
    s->exp = exp;
    static const u8 statIds[6] = { MON_DATA_MAX_HP, MON_DATA_ATK, MON_DATA_DEF, MON_DATA_SPATK, MON_DATA_SPDEF, MON_DATA_SPEED };
    static const u8 ivIds[6] = { MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV, MON_DATA_SPATK_IV, MON_DATA_SPDEF_IV, MON_DATA_SPEED_IV };
    static const u8 evIds[6] = { MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV };
    for (int32_t i = 0; i < 6; i++)
    {
        s->stats[i] = (uint16_t)GetMonData(mon, statIds[i]);
        s->ivs[i] = (uint8_t)GetMonData(mon, ivIds[i]);
        s->evs[i] = (uint8_t)GetMonData(mon, evIds[i]);
    }
    s->hp = (uint16_t)GetMonData(mon, MON_DATA_HP);
    s->showIvEv = VitaExtras_GetFast(FAST_EXTRA_IVS_EVS) != 0;
    s->eggCycles = (uint8_t)GetMonData(mon, MON_DATA_FRIENDSHIP);   // an egg's friendship counts its cycles
}

void UiGame_Move(int32_t slot, int32_t idx, UiMove *m)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    memset(m, 0, sizeof(*m));
    m->move = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + idx);
    if (m->move == MOVE_NONE)
        return;
    const struct BattleMove *bm = &gBattleMoves[m->move];
    UiGame_Text(gMoveNames[m->move], m->name, sizeof(m->name));
    UiGame_Text(gMoveDescriptionPointers[m->move - 1], m->desc, sizeof(m->desc));
    m->type = bm->type;
    m->power = bm->power;
    m->accuracy = bm->accuracy;
    m->pp = (uint8_t)GetMonData(mon, MON_DATA_PP1 + idx);
    m->maxPp = (uint8_t)CalculatePPWithBonus(m->move, (u8)GetMonData(mon, MON_DATA_PP_BONUSES), (u8)idx);
    m->category = bm->power == 0 ? 2 : IS_MOVE_PHYSICAL(m->move, bm->type) ? 0 : 1;
    const struct ContestMove *cm = &gContestMoves[m->move];
    m->contestCategory = cm->contestCategory;
    m->appeal = gContestEffects[cm->effect].appeal / 10;
    m->jam = gContestEffects[cm->effect].jam / 10;
    UiGame_Text(gContestEffectDescriptionPointers[cm->effect], m->contestDesc, sizeof(m->contestDesc));
}

// Swaps two moves with their PP and PP Ups (as the summary screen's reorder)
void UiGame_SwapMoves(int32_t slot, int32_t a, int32_t b)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    const uint16_t ma = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + a), mb = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + b);
    const uint8_t pa = (uint8_t)GetMonData(mon, MON_DATA_PP1 + a), pb = (uint8_t)GetMonData(mon, MON_DATA_PP1 + b);
    uint8_t bonuses = (uint8_t)GetMonData(mon, MON_DATA_PP_BONUSES);
    const uint8_t ba = (bonuses >> (a * 2)) & 3, bb = (bonuses >> (b * 2)) & 3;
    bonuses = (uint8_t)((bonuses & ~((3 << (a * 2)) | (3 << (b * 2)))) | (bb << (a * 2)) | (ba << (b * 2)));
    SetMonData(mon, MON_DATA_MOVE1 + a, &mb);
    SetMonData(mon, MON_DATA_MOVE1 + b, &ma);
    SetMonData(mon, MON_DATA_PP1 + a, &pb);
    SetMonData(mon, MON_DATA_PP1 + b, &pa);
    SetMonData(mon, MON_DATA_PP_BONUSES, &bonuses);
}

void UiGame_SwapPartyMons(int32_t a, int32_t b)
{
    struct Pokemon tmp = gPlayerParty[a];
    gPlayerParty[a] = gPlayerParty[b];
    gPlayerParty[b] = tmp;
}

uint16_t UiGame_FieldMove(int32_t slot, int32_t idx)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return MOVE_NONE;
    const uint16_t mv = (uint16_t)GetMonData(mon, MON_DATA_MOVE1 + idx);
    switch (mv)
    {
    case MOVE_CUT: case MOVE_FLASH: case MOVE_ROCK_SMASH: case MOVE_STRENGTH: case MOVE_SURF: case MOVE_FLY:
    case MOVE_DIVE: case MOVE_WATERFALL: case MOVE_TELEPORT: case MOVE_DIG: case MOVE_SECRET_POWER:
    case MOVE_SWEET_SCENT: case MOVE_SOFT_BOILED: case MOVE_MILK_DRINK:
        return mv;
    default:
        return MOVE_NONE;
    }
}

// SOFTBOILED / MILK DRINK: 1/5 of the user's max HP goes to the target
bool UiGame_ShareHp(int32_t from, int32_t to, char *msg, size_t cap)
{
    struct Pokemon *a = &gPlayerParty[from], *b = &gPlayerParty[to];
    const uint16_t cost = (uint16_t)(GetMonData(a, MON_DATA_MAX_HP) / 5);
    const uint16_t hpA = (uint16_t)GetMonData(a, MON_DATA_HP), hpB = (uint16_t)GetMonData(b, MON_DATA_HP);
    const uint16_t maxB = (uint16_t)GetMonData(b, MON_DATA_MAX_HP);
    if (from == to || GetMonData(b, MON_DATA_IS_EGG) || hpB == 0 || hpB >= maxB)
    {
        snprintf(msg, cap, "It can't be used on that POKéMON.");
        return false;
    }
    if (hpA <= cost) { snprintf(msg, cap, "Not enough HP left."); return false; }
    const uint16_t give = (uint16_t)((maxB - hpB) < cost ? (maxB - hpB) : cost);
    const uint16_t na = (uint16_t)(hpA - cost), nb = (uint16_t)(hpB + give);
    SetMonData(a, MON_DATA_HP, &na);
    SetMonData(b, MON_DATA_HP, &nb);
    snprintf(msg, cap, "HP was restored by %u point(s).", (unsigned)give);
    return true;
}
