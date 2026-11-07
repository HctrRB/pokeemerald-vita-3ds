// Map, encounters, Match Call, condition and ribbons for the new UI (Core 1). Same data as the
// current Vita UI (vita_ui_map.c, vita_ui_pokenav.c).

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/vita_daynight.h"
#include "platform/vita_encounters.h"
#include "platform/vita_extras.h"
#include "platform/vita_ui.h"

#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "event_data.h"
#include "gym_leader_rematch.h"
#include "match_call.h"
#include "overworld.h"
#include "pokenav.h"
#include "pokemon.h"
#include "region_map.h"
#include "string_util.h"
#include "wild_encounter.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"

// ---------- Map ----------
uint16_t UiGame_MapSecAt(int32_t gx, int32_t gy)
{
    return GetRegionMapSecIdAt((u16)(gx + 1), (u16)(gy + 2));
}

void UiGame_MapSecName(uint16_t sec, char *out, size_t cap)
{
    if (sec >= MAPSEC_NONE) { snprintf(out, cap, "-"); return; }
    u8 buf[32];
    GetMapNameGeneric(buf, sec);
    UiGame_Text(buf, out, cap);
}

bool UiGame_PlayerMapPos(int32_t *gx, int32_t *gy)
{
    const uint16_t sec = CorrectSpecialMapSecId(gMapHeader.regionMapSectionId);
    if (sec >= MAPSEC_NONE || gRegionMapEntries[sec].width == 0)
        return false;
    *gx = gRegionMapEntries[sec].x;
    *gy = gRegionMapEntries[sec].y;
    return true;
}

bool UiGame_MapSecPos(uint16_t sec, int32_t *gx, int32_t *gy)
{
    sec = CorrectSpecialMapSecId(sec);
    if (sec >= MAPSEC_NONE || gRegionMapEntries[sec].width == 0)
        return false;
    *gx = gRegionMapEntries[sec].x;
    *gy = gRegionMapEntries[sec].y;
    return true;
}

uint16_t UiGame_PlayerMapSec(void)
{
    return CorrectSpecialMapSecId(gMapHeader.regionMapSectionId);
}

bool UiGame_CanFlyTo(uint16_t sec)
{
    return sec < MAPSEC_NONE && VitaGame_CanFlyToMapSec(sec);
}

// ---------- Wild encounters of an area ----------
static int32_t Push(UiWild *out, int32_t n, int32_t max, uint16_t species, uint8_t lo, uint8_t hi, uint8_t rod)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return n;
    for (int32_t i = 0; i < n; i++)
        if (out[i].species == species && out[i].rod == rod)
        {
            if (lo < out[i].minLv) out[i].minLv = lo;
            if (hi > out[i].maxLv) out[i].maxLv = hi;
            return n;
        }
    if (n < max)
        out[n++] = (UiWild){ species, lo, hi, rod };
    return n;
}

bool UiGame_TimedEncounters(void)
{
    return VitaExtras_GetFast(FAST_EXTRA_TIMED_ENCOUNTERS) != 0;
}

int32_t UiGame_CurrentTimePhase(void)
{
    unsigned sec = 12U * 3600U, rate = 1U;
    VitaDayNight_Time(&sec, &rate);
    return (int32_t)VitaEncounters_Phase(sec);
}

int32_t UiGame_Wild(uint16_t sec, int32_t method, int32_t phase, int32_t rod, UiWild *out, int32_t max)
{
    int32_t n = 0;
    if (sec >= MAPSEC_NONE)
        return 0;
    for (uint32_t i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED); i++)
    {
        const struct WildPokemonHeader *h = &gWildMonHeaders[i];
        const struct MapHeader *mh = Overworld_GetMapHeaderByGroupAndId(h->mapGroup, h->mapNum);
        if (mh == NULL || CorrectSpecialMapSecId(mh->regionMapSectionId) != sec)
            continue;
        if (method == UI_WILD_GRASS && h->landMonsInfo != NULL)
        {
            const struct WildPokemonInfo *info = VitaEncounters_LandPhase(h->landMonsInfo, h->mapGroup, h->mapNum, (unsigned)phase);
            for (int32_t s = 0; s < LAND_WILD_COUNT; s++)
                n = Push(out, n, max, info->wildPokemon[s].species, info->wildPokemon[s].minLevel, info->wildPokemon[s].maxLevel, 0);
        }
        else if (method == UI_WILD_SURF && h->waterMonsInfo != NULL)
            for (int32_t s = 0; s < WATER_WILD_COUNT; s++)
                n = Push(out, n, max, h->waterMonsInfo->wildPokemon[s].species, h->waterMonsInfo->wildPokemon[s].minLevel, h->waterMonsInfo->wildPokemon[s].maxLevel, 0);
        else if (method == UI_WILD_ROCK && h->rockSmashMonsInfo != NULL)
            for (int32_t s = 0; s < ROCK_WILD_COUNT; s++)
                n = Push(out, n, max, h->rockSmashMonsInfo->wildPokemon[s].species, h->rockSmashMonsInfo->wildPokemon[s].minLevel, h->rockSmashMonsInfo->wildPokemon[s].maxLevel, 0);
        else if (method == UI_WILD_FISH && h->fishingMonsInfo != NULL)
            for (int32_t s = 0; s < FISH_WILD_COUNT; s++)
            {
                const uint8_t tier = (uint8_t)(s < 2 ? 1 : s < 5 ? 2 : 3);   // OLD / GOOD / SUPER ROD slots
                if (rod == 0 || rod == tier)
                    n = Push(out, n, max, h->fishingMonsInfo->wildPokemon[s].species, h->fishingMonsInfo->wildPokemon[s].minLevel, h->fishingMonsInfo->wildPokemon[s].maxLevel, tier);
            }
    }
    return n;
}

void UiGame_SpeciesName(uint16_t species, char *out, size_t cap)
{
    u8 buf[POKEMON_NAME_LENGTH + 1];
    GetSpeciesName(buf, species);
    UiGame_Text(buf, out, cap);
}

// ---------- Match Call ----------
static UiCaller sCallers[96];
static int32_t sCallerCount;

int32_t UiGame_Callers(const UiCaller **out)
{
    sCallerCount = 0;
    for (uint16_t i = 0; i < MC_HEADER_COUNT && sCallerCount < 96; i++)
    {
        if (!MatchCall_GetEnabled(i))
            continue;
        UiCaller *c = &sCallers[sCallerCount++];
        memset(c, 0, sizeof(*c));
        c->special = true;
        c->id = i;
        c->mapSec = MatchCall_GetMapSec(i);
        const uint32_t rem = MatchCall_GetRematchTableIdx(i);
        c->rematch = rem < REMATCH_TABLE_ENTRIES && gSaveBlock1Ptr->trainerRematches[rem] != 0;
        c->trainerPic = rem < REMATCH_TABLE_ENTRIES ? gTrainers[GetTrainerIdxByRematchIdx(rem)].trainerPic : 0xFFFF;
        const u8 *desc = NULL, *name = NULL;
        MatchCall_GetNameAndDesc(i, &desc, &name);
        if (name) UiGame_Text(name, c->name, sizeof(c->name)); else snprintf(c->name, sizeof(c->name), "CONTACT");
        if (desc) UiGame_Text(desc, c->desc, sizeof(c->desc));
    }
    for (uint16_t r = 0; r < REMATCH_SPECIAL_TRAINER_START && sCallerCount < 96; r++)
    {
        if (MatchCall_HasRematchId(r) || !FlagGet(TRAINER_REGISTERED_FLAGS_START + r))
            continue;
        UiCaller *c = &sCallers[sCallerCount++];
        memset(c, 0, sizeof(*c));
        c->id = r;
        const struct MapHeader *mh = Overworld_GetMapHeaderByGroupAndId(gRematchTable[r].mapGroup, gRematchTable[r].mapNum);
        c->mapSec = mh ? mh->regionMapSectionId : MAPSEC_NONE;
        c->rematch = gSaveBlock1Ptr->trainerRematches[r] != 0;
        const uint16_t tr = (uint16_t)GetTrainerIdxByRematchIdx(r);
        c->trainerPic = gTrainers[tr].trainerPic;
        UiGame_Text(gTrainers[tr].trainerName, c->name, sizeof(c->name));
        snprintf(c->desc, sizeof(c->desc), "TRAINER");
    }
    *out = sCallers;
    return sCallerCount;
}

void UiGame_CallMessage(const UiCaller *c, char *out, size_t cap)
{
    static u8 buf[512];
    buf[0] = EOS;
    if (c->special)
        MatchCall_GetMessage(c->id, buf);
    else
    {
        const uint16_t tr = (uint16_t)GetTrainerIdxByRematchIdx(c->id);
        if (!SelectMatchCallMessage(tr, buf))
            StringCopy(buf, gTrainers[tr].trainerName);
    }
    UiGame_Text(buf, out, cap);
}

// ---------- Condition ----------
void UiGame_Condition(int32_t slot, uint8_t out[6])
{
    static const u8 ids[6] = { MON_DATA_COOL, MON_DATA_BEAUTY, MON_DATA_CUTE, MON_DATA_SMART, MON_DATA_TOUGH, MON_DATA_SHEEN };
    for (int32_t i = 0; i < 6; i++)
        out[i] = (uint8_t)GetMonData(&gPlayerParty[slot], ids[i]);
}

// ---------- Ribbons ----------
int32_t UiGame_Ribbons(int32_t slot, uint8_t *ids, int32_t max)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    int32_t n = 0;
    if (GetMonData(mon, MON_DATA_CHAMPION_RIBBON) && n < max) ids[n++] = CHAMPION_RIBBON;
    static const u8 ranks[5] = { MON_DATA_COOL_RIBBON, MON_DATA_BEAUTY_RIBBON, MON_DATA_CUTE_RIBBON, MON_DATA_SMART_RIBBON, MON_DATA_TOUGH_RIBBON };
    for (int32_t c = 0; c < 5; c++)
    {
        const uint8_t rank = (uint8_t)GetMonData(mon, ranks[c]);
        for (uint8_t r = 0; r < rank && r < 4 && n < max; r++)
            ids[n++] = (uint8_t)(COOL_RIBBON_NORMAL + c * 4 + r);
    }
    static const u8 others[11] = { MON_DATA_WINNING_RIBBON, MON_DATA_VICTORY_RIBBON, MON_DATA_ARTIST_RIBBON, MON_DATA_EFFORT_RIBBON,
                                   MON_DATA_MARINE_RIBBON, MON_DATA_LAND_RIBBON, MON_DATA_SKY_RIBBON, MON_DATA_COUNTRY_RIBBON,
                                   MON_DATA_NATIONAL_RIBBON, MON_DATA_EARTH_RIBBON, MON_DATA_WORLD_RIBBON };
    for (int32_t i = 0; i < 11 && n < max; i++)
        if (GetMonData(mon, others[i]))
            ids[n++] = (uint8_t)(WINNING_RIBBON + i);
    return n;
}

void UiGame_RibbonText(uint8_t id, char *name, size_t nc, char *desc, size_t dc)
{
    static const char *const cats[5] = { "COOL", "BEAUTY", "CUTE", "SMART", "TOUGH" };
    static const char *const ranks[4] = { "", " SUPER", " HYPER", " MASTER" };
    static const char *const rankDesc[4] = { "Normal Rank", "Super Rank", "Hyper Rank", "Master Rank" };
    static const char *const other[11][2] = {
        { "WINNING", "Ribbon for clearing LV50 at the BATTLE TOWER." }, { "VICTORY", "Won for clearing LV100 at the BATTLE TOWER." },
        { "ARTIST", "Ribbon for being chosen as a super sketch model." }, { "EFFORT", "Ribbon awarded for being an exceptionally hard worker." },
        { "MARINE", "A special gift ribbon." }, { "LAND", "A special gift ribbon." }, { "SKY", "A special gift ribbon." },
        { "COUNTRY", "POKéMON LEAGUE Champion Ribbon." }, { "NATIONAL", "Overcoming difficult challenges." },
        { "EARTH", "Won for clearing the MT. BATTLE challenge." }, { "WORLD", "POKéMON WORLD Champion Ribbon." } };
    if (id == CHAMPION_RIBBON) { snprintf(name, nc, "CHAMPION RIBBON"); snprintf(desc, dc, "Champion-beating, HALL OF FAME Member Ribbon."); }
    else if (id <= TOUGH_RIBBON_MASTER)
    {
        const int32_t c = (id - COOL_RIBBON_NORMAL) / 4, r = (id - COOL_RIBBON_NORMAL) % 4;
        snprintf(name, nc, "%s%s RIBBON", cats[c], ranks[r]);
        snprintf(desc, dc, "HOENN %s CONTEST %s winner!", cats[c], rankDesc[r]);
    }
    else
    {
        snprintf(name, nc, "%s RIBBON", other[id - WINNING_RIBBON][0]);
        snprintf(desc, dc, "%s", other[id - WINNING_RIBBON][1]);
    }
}

// Areas where a species can be met in the wild (any method): out[mapSec] = 1
int32_t UiGame_SpeciesAreas(uint16_t species, uint8_t *out, int32_t count)
{
    memset(out, 0, (size_t)count);
    int32_t n = 0;
    for (uint32_t i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED); i++)
    {
        const struct WildPokemonHeader *h = &gWildMonHeaders[i];
        const struct MapHeader *mh = Overworld_GetMapHeaderByGroupAndId(h->mapGroup, h->mapNum);
        if (mh == NULL) continue;
        const uint16_t sec = CorrectSpecialMapSecId(mh->regionMapSectionId);
        if (sec >= count || out[sec]) continue;
        const struct WildPokemonInfo *infos[4] = { h->landMonsInfo, h->waterMonsInfo, h->rockSmashMonsInfo, h->fishingMonsInfo };
        const int32_t sizes[4] = { LAND_WILD_COUNT, WATER_WILD_COUNT, ROCK_WILD_COUNT, FISH_WILD_COUNT };
        for (int32_t k = 0; k < 4 && !out[sec]; k++)
            if (infos[k] != NULL)
                for (int32_t s = 0; s < sizes[k]; s++)
                    if (infos[k]->wildPokemon[s].species == species) { out[sec] = 1; n++; break; }
    }
    return n;
}

// ---------- POKEBLOCKS (CONDITION tab) ----------
#include "item.h"
#include "pokeblock.h"
#include "constants/items.h"

bool UiGame_HasPokeblockCase(void)
{
    return CheckBagHasItem(ITEM_POKEBLOCK_CASE, 1);
}

int32_t UiGame_Pokeblocks(UiBlock *out, int32_t max)
{
    int32_t n = 0;
    for (int32_t i = 0; i < POKEBLOCKS_COUNT && n < max; i++)
    {
        const struct Pokeblock *b = &gSaveBlock1Ptr->pokeblocks[i];
        if (b->color == PBLOCK_CLR_NONE)
            continue;
        u8 name[32];
        PokeblockCopyName(b, name);
        UiGame_Text(name, out[n].name, sizeof(out[n].name));
        out[n].slot = (uint8_t)i;
        out[n].color = b->color;
        out[n].level = GetHighestPokeblocksFlavorLevel(b);
        out[n].feel = b->feel;
        n++;
    }
    return n;
}

// Same result as the GBA feeding scene plus use_pokeblock.c's AddPokeblockToConditions.
// Returns 1 loved, 0 ate, -1 disliked, -2 sheen maxed (not eaten), -3 egg.
int32_t UiGame_FeedPokeblock(int32_t slot, int32_t pkblSlot)
{
    static const u32 data[5] = { MON_DATA_COOL, MON_DATA_TOUGH, MON_DATA_SMART, MON_DATA_CUTE, MON_DATA_BEAUTY };
    static const u8 flavor[5] = { FLAVOR_SPICY, FLAVOR_SOUR, FLAVOR_BITTER, FLAVOR_SWEET, FLAVOR_DRY };
    struct Pokemon *mon = &gPlayerParty[slot];
    struct Pokeblock *b = &gSaveBlock1Ptr->pokeblocks[pkblSlot];
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return -3;
    if (GetMonData(mon, MON_DATA_SHEEN) == MAX_SHEEN)
        return -2;
    const s16 gain = PokeblockGetGain(GetNature(mon), b);
    s16 boost[5] = { b->spicy, b->sour, b->bitter, b->sweet, b->dry };
    const s8 dir = gain > 0 ? 1 : gain < 0 ? -1 : 0;
    for (int32_t i = 0; i < 5 && dir; i++)
    {
        s8 extra = (s8)(boost[i] / 10);
        if (boost[i] % 10 >= 5)
            extra++;
        if (GetMonFlavorRelation(mon, flavor[i]) == dir)
            boost[i] += extra * dir;
    }
    for (int32_t i = 0; i < 5; i++)
    {
        s16 v = (s16)GetMonData(mon, data[i]) + boost[i];
        if (v < 0) v = 0;
        if (v > MAX_CONDITION) v = MAX_CONDITION;
        u8 d = (u8)v;
        SetMonData(mon, data[i], &d);
    }
    s16 sheen = (s16)GetMonData(mon, MON_DATA_SHEEN) + b->feel;
    if (sheen > MAX_SHEEN) sheen = MAX_SHEEN;
    u8 d = (u8)sheen;
    SetMonData(mon, MON_DATA_SHEEN, &d);
    TryClearPokeblock((u8)pkblSlot);
    return dir;
}
