#include "global.h"
#include "platform/vita_encounters.h"
#include "platform/vita_extras.h"
#include "wild_encounter.h"
#include "pokemon.h"
#include "metatile_behavior.h"
#include "fieldmap.h"
#include "random.h"
#include "field_player_avatar.h"
#include "event_data.h"
#include "safari_zone.h"
#include "overworld.h"
#include "pokeblock.h"
#include "battle_setup.h"
#include "roamer.h"
#include "tv.h"
#include "link.h"
#include "script.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "constants/abilities.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/weather.h"

extern const u8 EventScript_RepelWoreOff[];

#define MAX_ENCOUNTER_RATE 2880

#define NUM_FEEBAS_SPOTS 6

// Number of accessible fishing spots in each section of Route 119
// Each section is an area of the route between the y coordinates in sRoute119WaterTileData
#define NUM_FISHING_SPOTS_1 131
#define NUM_FISHING_SPOTS_2 167
#define NUM_FISHING_SPOTS_3 149
#define NUM_FISHING_SPOTS (NUM_FISHING_SPOTS_1 + NUM_FISHING_SPOTS_2 + NUM_FISHING_SPOTS_3)

enum {
    WILD_AREA_LAND,
    WILD_AREA_WATER,
    WILD_AREA_ROCKS,
    WILD_AREA_FISHING,
};

#define WILD_CHECK_REPEL    (1 << 0)
#define WILD_CHECK_KEEN_EYE (1 << 1)

#define HEADER_NONE 0xFFFF

static const struct WildPokemonInfo *GetLandMonsInfo(u16 headerId)
{
    const struct WildPokemonHeader *header = &gWildMonHeaders[headerId];
    return VitaEncounters_Land(header->landMonsInfo, header->mapGroup, header->mapNum);
}

static u16 FeebasRandom(void);
static void FeebasSeedRng(u16 seed);
static bool8 IsWildLevelAllowedByRepel(u8 level);
static void ApplyFluteEncounterRateMod(u32 *encRate);
static void ApplyCleanseTagEncounterRateMod(u32 *encRate);
#ifdef BUGFIX
static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex, u32 size);
#else
static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex);
#endif
static bool8 IsAbilityAllowingEncounter(u8 level);

EWRAM_DATA static u8 sWildEncountersDisabled = 0;
EWRAM_DATA static u32 sFeebasRngValue = 0;

#include "data/wild_encounters.h"

static const struct WildPokemon sWildFeebas = {20, 25, SPECIES_FEEBAS};

static const u16 sRoute119WaterTileData[] =
{
//yMin, yMax, numSpots in previous sections
     0,  45,  0,
    46,  91,  NUM_FISHING_SPOTS_1,
    92, 139,  NUM_FISHING_SPOTS_1 + NUM_FISHING_SPOTS_2,
};

void DisableWildEncounters(bool8 disabled)
{
    sWildEncountersDisabled = disabled;
}

// Each fishing spot on Route 119 is given a number between 1 and NUM_FISHING_SPOTS inclusive.
// The number is determined by counting the valid fishing spots left to right top to bottom.
// The map is divided into three sections, with each section having a pre-counted number of
// fishing spots to start from to avoid counting a large number of spots at the bottom of the map.
// Note that a spot is considered valid if it is surfable and not a waterfall. To exclude all
// of the inaccessible water metatiles (so that they can't be selected as a Feebas spot) they
// use a different metatile that isn't actually surfable because it has MB_NORMAL instead.
// This function is given the coordinates and section of a fishing spot and returns which number it is.
static u16 GetFeebasFishingSpotId(s16 targetX, s16 targetY, u8 section)
{
    u16 x, y;
    u16 yMin = sRoute119WaterTileData[section * 3 + 0];
    u16 yMax = sRoute119WaterTileData[section * 3 + 1];
    u16 spotId = sRoute119WaterTileData[section * 3 + 2];

    for (y = yMin; y <= yMax; y++)
    {
        for (x = 0; x < gMapHeader.mapLayout->width; x++)
        {
            u8 behavior = MapGridGetMetatileBehaviorAt(x + MAP_OFFSET, y + MAP_OFFSET);
            if (MetatileBehavior_IsSurfableAndNotWaterfall(behavior) == TRUE)
            {
                spotId++;
                if (targetX == x && targetY == y)
                    return spotId;
            }
        }
    }
    return spotId + 1;
}

static bool8 CheckFeebas(void)
{
    u8 i;
    u16 feebasSpots[NUM_FEEBAS_SPOTS];
    s16 x, y;
    u8 route119Section = 0;
    u16 spotId;

    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE119)
     && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ROUTE119))
    {
        GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
        x -= MAP_OFFSET;
        y -= MAP_OFFSET;

        // Get which third of the map the player is in
        if (y >= sRoute119WaterTileData[3 * 0 + 0] && y <= sRoute119WaterTileData[3 * 0 + 1])
            route119Section = 0;
        if (y >= sRoute119WaterTileData[3 * 1 + 0] && y <= sRoute119WaterTileData[3 * 1 + 1])
            route119Section = 1;
        if (y >= sRoute119WaterTileData[3 * 2 + 0] && y <= sRoute119WaterTileData[3 * 2 + 1])
            route119Section = 2;

        // 50% chance of encountering Feebas (assuming this is a Feebas spot)
        if (Random() % 100 > 49)
            return FALSE;

        FeebasSeedRng(gSaveBlock1Ptr->dewfordTrends[0].rand);

        // Assign each Feebas spot to a random fishing spot.
        // Randomness is fixed depending on the seed above.
        for (i = 0; i != NUM_FEEBAS_SPOTS;)
        {
            feebasSpots[i] = FeebasRandom() % NUM_FISHING_SPOTS;
            if (feebasSpots[i] == 0)
                feebasSpots[i] = NUM_FISHING_SPOTS;

            // < 1 below is a pointless check, it will never be TRUE.
            // >= 4 to skip fishing spots 1-3, because these are inaccessible
            // spots at the top of the map, at (9,7), (7,13), and (15,16).
            // The first accessible fishing spot is spot 4 at (18,18).
            if (feebasSpots[i] < 1 || feebasSpots[i] >= 4)
                i++;
        }

        // Check which fishing spot the player is at, and see if
        // it matches any of the Feebas spots.
        spotId = GetFeebasFishingSpotId(x, y, route119Section);
        for (i = 0; i < NUM_FEEBAS_SPOTS; i++)
        {
            if (spotId == feebasSpots[i])
                return TRUE;
        }
    }
    return FALSE;
}

static u16 FeebasRandom(void)
{
    sFeebasRngValue = ISO_RANDOMIZE2(sFeebasRngValue);
    return sFeebasRngValue >> 16;
}

static void FeebasSeedRng(u16 seed)
{
    sFeebasRngValue = seed;
}

// LAND_WILD_COUNT
static u8 ChooseWildMonIndex_Land(void)
{
    u8 rand = Random() % ENCOUNTER_CHANCE_LAND_MONS_TOTAL;

    if (rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_0)
        return 0;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_0 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_1)
        return 1;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_1 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_2)
        return 2;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_2 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_3)
        return 3;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_3 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_4)
        return 4;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_4 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_5)
        return 5;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_5 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_6)
        return 6;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_6 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_7)
        return 7;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_7 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_8)
        return 8;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_8 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_9)
        return 9;
    else if (rand >= ENCOUNTER_CHANCE_LAND_MONS_SLOT_9 && rand < ENCOUNTER_CHANCE_LAND_MONS_SLOT_10)
        return 10;
    else
        return 11;
}

// ROCK_WILD_COUNT / WATER_WILD_COUNT
static u8 ChooseWildMonIndex_WaterRock(void)
{
    u8 rand = Random() % ENCOUNTER_CHANCE_WATER_MONS_TOTAL;

    if (rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_0)
        return 0;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_0 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_1)
        return 1;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_1 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_2)
        return 2;
    else if (rand >= ENCOUNTER_CHANCE_WATER_MONS_SLOT_2 && rand < ENCOUNTER_CHANCE_WATER_MONS_SLOT_3)
        return 3;
    else
        return 4;
}

// FISH_WILD_COUNT
static u8 ChooseWildMonIndex_Fishing(u8 rod)
{
    u8 wildMonIndex = 0;
    u8 rand = Random() % max(max(ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_TOTAL, ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_TOTAL),
                             ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_TOTAL);

    switch (rod)
    {
    case OLD_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_SLOT_0)
            wildMonIndex = 0;
        else
            wildMonIndex = 1;
        break;
    case GOOD_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_2)
            wildMonIndex = 2;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_2 && rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_3)
            wildMonIndex = 3;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_3 && rand < ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_4)
            wildMonIndex = 4;
        break;
    case SUPER_ROD:
        if (rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_5)
            wildMonIndex = 5;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_5 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_6)
            wildMonIndex = 6;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_6 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_7)
            wildMonIndex = 7;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_7 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_8)
            wildMonIndex = 8;
        if (rand >= ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_8 && rand < ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_9)
            wildMonIndex = 9;
        break;
    }
    return wildMonIndex;
}

static u8 ChooseWildMonLevel(const struct WildPokemon *wildPokemon)
{
    u8 min;
    u8 max;
    u8 range;
    u8 rand;

    // Make sure minimum level is less than maximum level
    if (wildPokemon->maxLevel >= wildPokemon->minLevel)
    {
        min = wildPokemon->minLevel;
        max = wildPokemon->maxLevel;
    }
    else
    {
        min = wildPokemon->maxLevel;
        max = wildPokemon->minLevel;
    }
    range = max - min + 1;
    rand = Random() % range;

    // check ability for max level mon
    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
    {
        u8 ability = GetMonAbility(&gPlayerParty[0]);
        if (ability == ABILITY_HUSTLE || ability == ABILITY_VITAL_SPIRIT || ability == ABILITY_PRESSURE)
        {
            if (Random() % 2 == 0)
                return max;

            if (rand != 0)
                rand--;
        }
    }
    return min + rand;
}

static u16 GetCurrentMapWildMonHeaderId(void)
{
    u16 i;

    for (i = 0; ; i++)
    {
        const struct WildPokemonHeader *wildHeader = &gWildMonHeaders[i];
        if (wildHeader->mapGroup == MAP_GROUP(MAP_UNDEFINED))
            break;

        if (gWildMonHeaders[i].mapGroup == gSaveBlock1Ptr->location.mapGroup &&
            gWildMonHeaders[i].mapNum == gSaveBlock1Ptr->location.mapNum)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ALTERING_CAVE) &&
                gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ALTERING_CAVE))
            {
                u16 alteringCaveId = VarGet(VAR_ALTERING_CAVE_WILD_SET);
                if (alteringCaveId >= NUM_ALTERING_CAVE_TABLES)
                    alteringCaveId = 0;

                i += alteringCaveId;
            }

            return i;
        }
    }

    return HEADER_NONE;
}

static u8 PickWildMonNature(void)
{
    u8 i;
    u8 j;
    struct Pokeblock *safariPokeblock;
    u8 natures[NUM_NATURES];

    if (GetSafariZoneFlag() == TRUE && Random() % 100 < 80)
    {
        safariPokeblock = SafariZoneGetActivePokeblock();
        if (safariPokeblock != NULL)
        {
            for (i = 0; i < NUM_NATURES; i++)
                natures[i] = i;
            for (i = 0; i < NUM_NATURES - 1; i++)
            {
                for (j = i + 1; j < NUM_NATURES; j++)
                {
                    if (Random() & 1)
                    {
                        u8 temp;
                        SWAP(natures[i], natures[j], temp);
                    }
                }
            }
            for (i = 0; i < NUM_NATURES; i++)
            {
                if (PokeblockGetGain(natures[i], safariPokeblock) > 0)
                    return natures[i];
            }
        }
    }
    // check synchronize for a Pokémon with the same ability
    if (!GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG)
        && GetMonAbility(&gPlayerParty[0]) == ABILITY_SYNCHRONIZE
        && Random() % 2 == 0)
    {
        return GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY) % NUM_NATURES;
    }

    // random nature
    return Random() % NUM_NATURES;
}

static void CreateWildMon(u16 species, u8 level)
{
    bool32 checkCuteCharm;

    ZeroEnemyPartyMons();
    checkCuteCharm = TRUE;

    switch (gSpeciesInfo[species].genderRatio)
    {
    case MON_MALE:
    case MON_FEMALE:
    case MON_GENDERLESS:
        checkCuteCharm = FALSE;
        break;
    }

    if (checkCuteCharm
        && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG)
        && GetMonAbility(&gPlayerParty[0]) == ABILITY_CUTE_CHARM
        && Random() % 3 != 0)
    {
        u16 leadingMonSpecies = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES);
        u32 leadingMonPersonality = GetMonData(&gPlayerParty[0], MON_DATA_PERSONALITY);
        u8 gender = GetGenderFromSpeciesAndPersonality(leadingMonSpecies, leadingMonPersonality);

        // misses mon is genderless check, although no genderless mon can have cute charm as ability
        if (gender == MON_FEMALE)
            gender = MON_MALE;
        else
            gender = MON_FEMALE;

        CreateMonWithGenderNatureLetter(&gEnemyParty[0], species, level, USE_RANDOM_IVS, gender, PickWildMonNature(), 0);
        return;
    }

    CreateMonWithNature(&gEnemyParty[0], species, level, USE_RANDOM_IVS, PickWildMonNature());
}
#ifdef BUGFIX
#define TRY_GET_ABILITY_INFLUENCED_WILD_MON_INDEX(wildPokemon, type, ability, ptr, count) TryGetAbilityInfluencedWildMonIndex(wildPokemon, type, ability, ptr, count)
#else
#define TRY_GET_ABILITY_INFLUENCED_WILD_MON_INDEX(wildPokemon, type, ability, ptr, count) TryGetAbilityInfluencedWildMonIndex(wildPokemon, type, ability, ptr)
#endif

static bool8 TryGenerateWildMon(const struct WildPokemonInfo *wildMonInfo, u8 area, u8 flags)
{
    u8 wildMonIndex = 0;
    u8 level;

    switch (area)
    {
    case WILD_AREA_LAND:
        if (TRY_GET_ABILITY_INFLUENCED_WILD_MON_INDEX(wildMonInfo->wildPokemon, TYPE_STEEL, ABILITY_MAGNET_PULL, &wildMonIndex, LAND_WILD_COUNT))
            break;
        if (TRY_GET_ABILITY_INFLUENCED_WILD_MON_INDEX(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, LAND_WILD_COUNT))
            break;

        wildMonIndex = ChooseWildMonIndex_Land();
        break;
    case WILD_AREA_WATER:
        if (TRY_GET_ABILITY_INFLUENCED_WILD_MON_INDEX(wildMonInfo->wildPokemon, TYPE_ELECTRIC, ABILITY_STATIC, &wildMonIndex, WATER_WILD_COUNT))
            break;

        wildMonIndex = ChooseWildMonIndex_WaterRock();
        break;
    case WILD_AREA_ROCKS:
        wildMonIndex = ChooseWildMonIndex_WaterRock();
        break;
    }

    level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);
    if (flags & WILD_CHECK_REPEL && !IsWildLevelAllowedByRepel(level))
        return FALSE;
    if (gMapHeader.mapLayoutId != LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS && flags & WILD_CHECK_KEEN_EYE && !IsAbilityAllowingEncounter(level))
        return FALSE;

    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level);
    return TRUE;
}

static u16 GenerateFishingWildMon(const struct WildPokemonInfo *wildMonInfo, u8 rod)
{
    u8 wildMonIndex = ChooseWildMonIndex_Fishing(rod);
    u8 level = ChooseWildMonLevel(&wildMonInfo->wildPokemon[wildMonIndex]);

    CreateWildMon(wildMonInfo->wildPokemon[wildMonIndex].species, level);
    return wildMonInfo->wildPokemon[wildMonIndex].species;
}

static bool8 SetUpMassOutbreakEncounter(u8 flags)
{
    u16 i;

    if (flags & WILD_CHECK_REPEL && !IsWildLevelAllowedByRepel(gSaveBlock1Ptr->outbreakPokemonLevel))
        return FALSE;

    CreateWildMon(gSaveBlock1Ptr->outbreakPokemonSpecies, gSaveBlock1Ptr->outbreakPokemonLevel);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(&gEnemyParty[0], gSaveBlock1Ptr->outbreakPokemonMoves[i], i);

    return TRUE;
}

static bool8 DoMassOutbreakEncounterTest(void)
{
    if (gSaveBlock1Ptr->outbreakPokemonSpecies != SPECIES_NONE
     && gSaveBlock1Ptr->location.mapNum == gSaveBlock1Ptr->outbreakLocationMapNum
     && gSaveBlock1Ptr->location.mapGroup == gSaveBlock1Ptr->outbreakLocationMapGroup)
    {
        if (Random() % 100 < gSaveBlock1Ptr->outbreakPokemonProbability)
            return TRUE;
    }
    return FALSE;
}

static bool8 EncounterOddsCheck(u16 encounterRate)
{
    if (Random() % MAX_ENCOUNTER_RATE < encounterRate)
        return TRUE;
    else
        return FALSE;
}

// Wild encounter rate (OPTIONS, CHEATS) in percent: random encounters while
// walking and surfing, and how often visible wild Pokemon appear.
static u32 EncounterRatePercent(void)
{
    static const u8 percent[] = {100, 0, 25, 50, 200}; // NORMAL, OFF, 1/4, 1/2, 2X
    u32 setting = (u32)VitaExtras_GetFast(FAST_EXTRA_ENCOUNTER_RATE);

    return setting < ARRAY_COUNT(percent) ? percent[setting] : 100;
}

// Returns true if it will try to create a wild encounter.
static bool8 WildEncounterCheck(u32 encounterRate, bool8 ignoreAbility)
{
    encounterRate *= 16;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_MACH_BIKE | PLAYER_AVATAR_FLAG_ACRO_BIKE))
        encounterRate = encounterRate * 80 / 100;
    ApplyFluteEncounterRateMod(&encounterRate);
    ApplyCleanseTagEncounterRateMod(&encounterRate);
    if (!ignoreAbility && !GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
    {
        u32 ability = GetMonAbility(&gPlayerParty[0]);

        if (ability == ABILITY_STENCH && gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
            encounterRate = encounterRate * 3 / 4;
        else if (ability == ABILITY_STENCH)
            encounterRate /= 2;
        else if (ability == ABILITY_ILLUMINATE)
            encounterRate *= 2;
        else if (ability == ABILITY_WHITE_SMOKE)
            encounterRate /= 2;
        else if (ability == ABILITY_ARENA_TRAP)
            encounterRate *= 2;
        else if (ability == ABILITY_SAND_VEIL && gSaveBlock1Ptr->weather == WEATHER_SANDSTORM)
            encounterRate /= 2;
    }
    if (!ignoreAbility)
        encounterRate = encounterRate * EncounterRatePercent() / 100;
    if (encounterRate > MAX_ENCOUNTER_RATE)
        encounterRate = MAX_ENCOUNTER_RATE;
    return EncounterOddsCheck(encounterRate);
}

// When you first step on a different type of metatile, there's a 40% chance it
// skips the wild encounter check entirely.
static bool8 AllowWildCheckOnNewMetatile(void)
{
    if (Random() % 100 >= 60)
        return FALSE;
    else
        return TRUE;
}

static bool8 AreLegendariesInSootopolisPreventingEncounters(void)
{
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_SOOTOPOLIS_CITY)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_SOOTOPOLIS_CITY))
    {
        return FALSE;
    }

    return FlagGet(FLAG_LEGENDARIES_IN_SOOTOPOLIS);
}

/*
 * Visible wild Pokemon (ported from Emerald 3DS, CC BY-NC-SA 4.0):
 * The map's wild Pokemon wander its encounter tiles on HGSS overworld sprites
 * (follower_mon.c, normal or shiny colours on ext palettes), else as their
 * party icons, and touching one starts a battle with that exact Pokemon.
 * FEW and SOME come on top of random encounters; MANY replaces them.
 */
#include "event_object_movement.h"
#include "field_effect.h"
#include "follower_mon.h"
#include "pokemon_icon.h"
#include "sound.h"
#include "constants/field_effects.h"
#include "constants/pokemon_icon.h"
#include "constants/songs.h"

static const u8 sVisibleMost[] = {0, 4, 8, 12};

static u32 VisibleSetting(void)
{
    u32 setting = (u32)VitaExtras_GetFast(FAST_EXTRA_VISIBLE_WILD);

    return setting < ARRAY_COUNT(sVisibleMost) ? setting : 0;
}

#define VISIBLE_MOST() sVisibleMost[VisibleSetting()]
#define VISIBLE_REPLACES_RANDOM() (VisibleSetting() == ARRAY_COUNT(sVisibleMost) - 1)

#define VW_MAX       12
#define VW_MAGIC     0x5657
#define VW_RANGE_X   11
#define VW_RANGE_Y   6
#define VW_NEAREST   3
#define VW_FORGET    15
#define VW_TRIES     48
#define VW_GROW      12
#define VW_GROW_BACK 8
#define VW_VANISH    12
#define VW_NONE      MAX_SPRITES
#define VW_ICON_BYTES 0x200
#define VW_ICON_FRAME 14

struct VisibleWild
{
    bool8 active, water, shiny;
    bool8 hgss;
    bool8 vanishing;
    u8 spriteId;
    u8 matrix;
    u8 scaleTime, scaleFrames;
    u8 level, progress;
    s8 dx, dy;
    s8 faceX, faceY;
    u16 species, idle, sparkle;
    s16 x, y;
    u32 personality;
};

static struct VisibleWild sVisible[VW_MAX];
static u8 sVisibleMapGroup = 0xFF, sVisibleMapNum = 0xFF;
static bool8 sVisibleShifted;
static u16 sVisibleSpawnTimer, sVisibleRetry;
static u32 sVisibleFrames;

#define VW_ABS(n) ((n) < 0 ? -(n) : (n))

static u32 VisibleOtId(void)
{
    return gSaveBlock2Ptr->playerTrainerId[0]
         | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
         | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
         | (gSaveBlock2Ptr->playerTrainerId[3] << 24);
}

static bool8 VisibleSpriteValid(const struct VisibleWild *v)
{
    return v->spriteId < MAX_SPRITES && gSprites[v->spriteId].inUse
        && gSprites[v->spriteId].data[7] == VW_MAGIC;
}

static u16 VisiblePaletteTag(u16 species)
{
    return POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species];
}

static bool8 VisibleLoadPalette(u16 species)
{
    if (IndexOfSpritePaletteTag(VisiblePaletteTag(species)) == 0xFF)
        LoadMonIconPalette(species);
    return IndexOfSpritePaletteTag(VisiblePaletteTag(species)) != 0xFF;
}

static void VisibleDropSprite(struct VisibleWild *v)
{
    if (VisibleSpriteValid(v))
    {
        struct Sprite *sprite = &gSprites[v->spriteId];

        if (v->matrix != 0xFF)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            sprite->oam.matrixNum = 0;
            FreeOamMatrix(v->matrix);
        }
        if (v->hgss)
        {
            FollowerMon_WildFree(v - sVisible, sprite);
        }
        else
        {
            u32 i;
            bool8 shared = FALSE;

            FreeAndDestroyMonIconSprite(sprite);
            for (i = 0; i < VW_MAX; i++)
                if (&sVisible[i] != v && sVisible[i].active && !sVisible[i].hgss && VisibleSpriteValid(&sVisible[i])
                 && gMonIconPaletteIndices[sVisible[i].species] == gMonIconPaletteIndices[v->species])
                    shared = TRUE;
            if (!shared)
                FreeMonIconPalette(v->species);
        }
    }
    else if (v->hgss)
    {
        FollowerMon_WildFree(v - sVisible, NULL);
    }
    v->spriteId = VW_NONE;
    v->matrix = 0xFF;
}

static void VisibleForgetSprite(struct VisibleWild *v)
{
    if (v->hgss)
        FollowerMon_WildFree(v - sVisible, NULL);
    v->spriteId = VW_NONE;
    v->matrix = 0xFF;
}

static void VisibleRemove(struct VisibleWild *v)
{
    if (v->active)
        VisibleDropSprite(v);
    v->active = FALSE;
}

static void VisibleRemoveAll(void)
{
    u32 i;

    for (i = 0; i < VW_MAX; i++)
        VisibleRemove(&sVisible[i]);
}

static u32 VisibleCount(void)
{
    u32 i, count = 0;

    for (i = 0; i < VW_MAX; i++)
        count += sVisible[i].active && !sVisible[i].vanishing;
    return count;
}

static struct VisibleWild *VisibleAt(s16 x, s16 y, const struct VisibleWild *except)
{
    u32 i;

    for (i = 0; i < VW_MAX; i++)
    {
        struct VisibleWild *v = &sVisible[i];

        if (!v->active || v == except)
            continue;
        if ((v->x == x && v->y == y) || (v->progress && v->x + v->dx == x && v->y + v->dy == y))
            return v;
    }
    return NULL;
}

static bool8 VisibleTileKind(s16 x, s16 y, bool8 water)
{
    u32 behavior = MapGridGetMetatileBehaviorAt(x, y);

    if (water ? !MetatileBehavior_IsWaterWildEncounter(behavior)
              : !MetatileBehavior_IsLandWildEncounter(behavior))
        return FALSE;
    return MapGridGetCollisionAt(x, y) == 0;
}

static bool8 VisibleTileFits(s16 x, s16 y, bool8 water)
{
    return VisibleTileKind(x, y, water) && GetObjectEventIdByXY(x, y) == OBJECT_EVENTS_COUNT;
}

static bool8 VisiblePlayerInWater(void)
{
    return TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING | PLAYER_AVATAR_FLAG_UNDERWATER) != 0;
}

u32 ShinyOdds_WildPersonality(u16 species, u32 otId); // pokemon.c

static void SpriteCB_VisibleIcon(struct Sprite *sprite)
{
    if (sprite->data[6] > 0)
    {
        sprite->data[6]--;
        return;
    }
    {
        const u8 *icon = (const u8 *)sprite->images;

        if (icon != NULL)
            CpuCopy16(icon + VW_ICON_BYTES * sprite->data[5],
                      (void *)(OBJ_VRAM0 + sprite->oam.tileNum * TILE_SIZE_4BPP), VW_ICON_BYTES);
    }
    sprite->data[5] ^= 1;
    sprite->data[6] = VW_ICON_FRAME;
}

static void VisibleSparkle(const struct VisibleWild *v)
{
    gFieldEffectArguments[0] = v->x - MAP_OFFSET;
    gFieldEffectArguments[1] = v->y - MAP_OFFSET;
    gFieldEffectArguments[2] = 2;
    FieldEffectStart(FLDEFF_SPARKLE);
}

static void VisibleRustle(const struct VisibleWild *v)
{
    u32 behavior = MapGridGetMetatileBehaviorAt(v->x, v->y);
    u8 effect;

    if (v->water)
        effect = FLDEFF_JUMP_SMALL_SPLASH;
    else if (MetatileBehavior_IsTallGrass(behavior))
        effect = FLDEFF_JUMP_TALL_GRASS;
    else if (MetatileBehavior_IsLongGrass(behavior))
        effect = FLDEFF_JUMP_LONG_GRASS;
    else
        effect = FLDEFF_DUST;
    gFieldEffectArguments[0] = v->x;
    gFieldEffectArguments[1] = v->y;
    gFieldEffectArguments[2] = gObjectEvents[gPlayerAvatar.objectEventId].previousElevation;
    gFieldEffectArguments[3] = 2;
    FieldEffectStart(effect);
}

static void VisibleScaleStep(struct VisibleWild *v)
{
    struct Sprite *sprite = &gSprites[v->spriteId];
    u32 t, inv, scale;

    if (v->scaleFrames == 0)
        return;
    if (v->scaleTime < v->scaleFrames)
        v->scaleTime++;
    t = v->scaleTime * 256 / v->scaleFrames;
    if (v->vanishing)
        t = 256 - t;
    inv = 256 - t;
    scale = 256 - inv * inv / 256;
    if (v->scaleTime >= v->scaleFrames && !v->vanishing)
    {
        sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
        sprite->oam.matrixNum = 0;
        if (v->matrix != 0xFF)
            FreeOamMatrix(v->matrix);
        v->matrix = 0xFF;
        v->scaleFrames = 0;
        if (v->hgss)
        {
            FollowerMon_WildFace(sprite, v->faceX, v->faceY, FALSE);
            StartSpriteAnim(sprite, sprite->animNum);
        }
        return;
    }
    if (v->matrix == 0xFF)
        return;
    if (scale < 16)
        scale = 16;
    SetOamMatrix(v->matrix, 0x10000 / scale, 0, 0, 0x10000 / scale);
}

static void VisibleStartScale(struct VisibleWild *v, u8 frames)
{
    struct Sprite *sprite = &gSprites[v->spriteId];

    v->scaleTime = 0;
    v->scaleFrames = frames;
    if (v->matrix == 0xFF)
        v->matrix = AllocOamMatrix();
    if (v->matrix == 0xFF)
    {
        v->scaleTime = frames;
        return;
    }
    if (v->hgss)
        FollowerMon_WildFace(sprite, 0, 1, FALSE);
    sprite->oam.affineMode = ST_OAM_AFFINE_NORMAL;
    sprite->oam.matrixNum = v->matrix;
    VisibleScaleStep(v);
}

static void VisibleVanish(struct VisibleWild *v)
{
    if (v->vanishing)
        return;
    if (!VisibleSpriteValid(v))
    {
        VisibleRemove(v);
        return;
    }
    v->vanishing = TRUE;
    v->dx = v->dy = 0;
    v->progress = 0;
    VisibleStartScale(v, VW_VANISH);
}

static void VisiblePlace(struct VisibleWild *v)
{
    struct Sprite *sprite = &gSprites[v->spriteId];
    s16 x, y, step = v->progress / 2;

    SetSpritePosToMapCoords(v->x, v->y, &x, &y);
    sprite->x = x + 8 + v->dx * step;
    sprite->y = y + v->dy * step;
    if (v->hgss)
    {
        sprite->y += 16 + sprite->centerToCornerVecY;
        if (v->scaleFrames == 0)
            FollowerMon_WildFace(sprite, v->faceX, v->faceY, v->progress != 0);
    }
    sprite->subpriority = 200 - ((sprite->y + 64) >> 1 & 0x7F);
}

static bool8 VisibleMakeSprite(struct VisibleWild *v, u8 grow)
{
    struct Sprite *sprite;

    v->matrix = 0xFF;
    v->spriteId = FollowerMon_WildSprite(v - sVisible, v->species, v->personality, v->shiny);
    v->hgss = v->spriteId < MAX_SPRITES;
    if (!v->hgss)
    {
        if (!VisibleLoadPalette(v->species))
        {
            v->spriteId = VW_NONE;
            return FALSE;
        }
        v->spriteId = CreateMonIcon(v->species, SpriteCB_VisibleIcon, 0, 0, 1, v->personality, TRUE);
        if (v->spriteId >= MAX_SPRITES)
        {
            v->spriteId = VW_NONE;
            return FALSE;
        }
        gSprites[v->spriteId].affineAnims = gDummySpriteAffineAnimTable;
    }
    sprite = &gSprites[v->spriteId];
    sprite->coordOffsetEnabled = TRUE;
    sprite->oam.priority = 2;
    sprite->data[7] = VW_MAGIC;
    sprite->data[5] = 0;
    sprite->data[6] = 0;
    v->scaleFrames = 0;
    VisiblePlace(v);
    VisibleStartScale(v, grow);
    VisiblePlace(v);
    return TRUE;
}

static bool8 VisibleTrySpawn(u16 headerId)
{
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct VisibleWild *v = NULL;
    const struct WildPokemonInfo *info;
    const struct WildPokemon *wild;
    u32 i, tries;
    s16 x, y;
    bool8 water;

    for (i = 0; i < VW_MAX && v == NULL; i++)
        if (!sVisible[i].active)
            v = &sVisible[i];
    if (v == NULL)
        return FALSE;
    for (tries = 0; tries < VW_TRIES; tries++)
    {
        x = player->currentCoords.x + (s16)(Random() % (2 * VW_RANGE_X + 1)) - VW_RANGE_X;
        y = player->currentCoords.y + (s16)(Random() % (2 * VW_RANGE_Y + 1)) - VW_RANGE_Y;
        if (VW_ABS(x - player->currentCoords.x) < VW_NEAREST && VW_ABS(y - player->currentCoords.y) < VW_NEAREST)
            continue;
        if (VisibleAt(x, y, NULL) != NULL)
            continue;
        water = MetatileBehavior_IsWaterWildEncounter(MapGridGetMetatileBehaviorAt(x, y));
        info = water ? gWildMonHeaders[headerId].waterMonsInfo : GetLandMonsInfo(headerId);
        if (info == NULL || !VisibleTileFits(x, y, water))
            continue;
        if (water && AreLegendariesInSootopolisPreventingEncounters())
            continue;
        wild = &info->wildPokemon[water ? ChooseWildMonIndex_WaterRock() : ChooseWildMonIndex_Land()];
        v->level = ChooseWildMonLevel(wild);
        if (!IsWildLevelAllowedByRepel(v->level))
            return FALSE;
        v->species = wild->species;
        v->personality = ShinyOdds_WildPersonality(v->species, VisibleOtId());
        v->shiny = GET_SHINY_VALUE(VisibleOtId(), v->personality) < SHINY_ODDS;
        v->faceX = 0;
        v->faceY = 1;
        v->water = water;
        v->x = x;
        v->y = y;
        v->dx = v->dy = 0;
        v->progress = 0;
        v->vanishing = FALSE;
        v->idle = VW_GROW + 30 + Random() % 90;
        v->sparkle = 0;
        if (!VisibleMakeSprite(v, VW_GROW))
            return FALSE;
        v->active = TRUE;
        VisibleRustle(v);
        if (v->shiny)
        {
            VisibleSparkle(v);
            PlaySE(SE_SHINY);
        }
        return TRUE;
    }
    return FALSE;
}

static void VisiblePrefetch(u16 headerId)
{
    static u16 sHeader = HEADER_NONE;
    static u32 sSignature, sLastLook;
    const struct WildPokemonInfo *land, *water;
    u16 species[LAND_WILD_COUNT + WATER_WILD_COUNT];
    u32 i, j, count = 0, signature = 0;

    if (headerId == sHeader && sVisibleFrames - sLastLook < 60)
        return;
    sLastLook = sVisibleFrames;
    land = headerId != HEADER_NONE ? GetLandMonsInfo(headerId) : NULL;
    water = headerId != HEADER_NONE ? gWildMonHeaders[headerId].waterMonsInfo : NULL;
    for (i = 0; land != NULL && i < LAND_WILD_COUNT; i++)
        species[count++] = land->wildPokemon[i].species;
    for (i = 0; water != NULL && i < WATER_WILD_COUNT; i++)
        species[count++] = water->wildPokemon[i].species;
    for (i = 0; i < count; i++)
        signature = signature * 31 + species[i];
    if (headerId == sHeader && signature == sSignature)
        return;
    sHeader = headerId;
    sSignature = signature;
    for (i = 0; i < count; i++)
    {
        for (j = 0; j < i && species[j] != species[i]; j++)
            ;
        if (j == i)
            FollowerMon_WildPrefetch(species[i]);
    }
}

static void VisibleStartBattle(struct VisibleWild *v)
{
    ZeroEnemyPartyMons();
    CreateMon(&gEnemyParty[0], v->species, v->level, USE_RANDOM_IVS, TRUE, v->personality, OT_ID_PLAYER_ID, 0);
    VisibleRemove(v);
    BattleSetup_StartWildBattle();
}

static void VisibleMove(struct VisibleWild *v, s16 px, s16 py, bool8 inWater)
{
    static const s8 sSteps[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    u32 dir;
    s16 tx, ty;

    if (v->progress)
    {
        if (++v->progress >= 32)
        {
            v->x += v->dx;
            v->y += v->dy;
            v->dx = v->dy = 0;
            v->progress = 0;
            v->idle = 40 + Random() % 120;
        }
        return;
    }
    if (v->idle && --v->idle)
        return;
    dir = Random() % 4;
    tx = v->x + sSteps[dir][0];
    ty = v->y + sSteps[dir][1];
    if (tx == px && ty == py)
    {
        if (v->water == inWater)
            v->idle = 0xFFFF;
        else
            v->idle = 60;
        return;
    }
    if (!VisibleTileFits(tx, ty, v->water) || VisibleAt(tx, ty, v) != NULL)
    {
        v->idle = 30;
        return;
    }
    v->dx = v->faceX = sSteps[dir][0];
    v->dy = v->faceY = sSteps[dir][1];
    v->progress = 1;
}

void VisibleWild_CameraShift(s16 dx, s16 dy)
{
    u32 i;

    for (i = 0; i < VW_MAX; i++)
    {
        sVisible[i].x -= dx;
        sVisible[i].y -= dy;
    }
    sVisibleShifted = TRUE;
}

bool8 VisibleWild_StepOnto(void)
{
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    bool8 inWater = VisiblePlayerInWater();
    s16 px = player->currentCoords.x, py = player->currentCoords.y;
    u32 i;

    for (i = 0; i < VW_MAX; i++)
    {
        struct VisibleWild *v = &sVisible[i];

        if (!v->active || v->vanishing || v->spriteId == VW_NONE || v->water != inWater)
            continue;
        if ((v->x == px && v->y == py) || (v->progress && v->x + v->dx == px && v->y + v->dy == py))
        {
            VisibleStartBattle(v);
            return TRUE;
        }
    }
    return FALSE;
}

bool8 VisibleWild_IsSprite(u8 spriteId)
{
    return spriteId < MAX_SPRITES && gSprites[spriteId].inUse
        && gSprites[spriteId].data[7] == VW_MAGIC;
}

void VisibleWild_Update(void)
{
    struct ObjectEvent *player;
    u16 headerId;
    u32 i, percent, most;
    bool8 inWater, idle, settled;

    sVisibleFrames++;
    most = VISIBLE_MOST();
    if (!most || gMain.callback2 != CB2_Overworld)
    {
        if (!most)
            VisibleRemoveAll();
        return;
    }
    for (i = 0; i < VW_MAX; i++)
        if (sVisible[i].active && sVisible[i].spriteId != VW_NONE && !VisibleSpriteValid(&sVisible[i]))
            VisibleForgetSprite(&sVisible[i]);
    if (gSaveBlock1Ptr->location.mapGroup != sVisibleMapGroup || gSaveBlock1Ptr->location.mapNum != sVisibleMapNum)
    {
        if (sVisibleShifted)
        {
            for (i = 0; i < VW_MAX; i++)
            {
                if (!sVisible[i].active)
                    continue;
                if (!VisibleTileKind(sVisible[i].x, sVisible[i].y, sVisible[i].water))
                    VisibleVanish(&sVisible[i]);
            }
        }
        else
        {
            VisibleRemoveAll();
            sVisibleSpawnTimer = 15;
        }
        sVisibleMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sVisibleMapNum = gSaveBlock1Ptr->location.mapNum;
    }
    sVisibleShifted = FALSE;

    headerId = GetCurrentMapWildMonHeaderId();
    VisiblePrefetch(headerId);
    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    inWater = VisiblePlayerInWater();
    idle = !ArePlayerFieldControlsLocked() && !ScriptContext_IsEnabled()
        && gPlayerAvatar.tileTransitionState == T_NOT_MOVING;
    settled = FollowerMon_PalettesSettled();

    for (i = VW_MAX; i-- > 0 && VisibleCount() > most;)
        if (sVisible[i].active && !sVisible[i].vanishing)
            VisibleVanish(&sVisible[i]);

    if (sVisibleRetry)
        sVisibleRetry--;
    for (i = 0; i < VW_MAX; i++)
    {
        struct VisibleWild *v = &sVisible[i];

        if (!v->active)
            continue;
        if (VW_ABS(v->x - player->currentCoords.x) > VW_FORGET || VW_ABS(v->y - player->currentCoords.y) > VW_FORGET)
        {
            VisibleRemove(v);
            continue;
        }
        if (v->spriteId == VW_NONE)
        {
            if (!settled || sVisibleRetry || v->vanishing)
                continue;
            if (!VisibleMakeSprite(v, VW_GROW_BACK))
            {
                sVisibleRetry = 30;
                continue;
            }
        }
        if (v->scaleFrames)
        {
            VisibleScaleStep(v);
            if (v->vanishing && v->scaleTime >= v->scaleFrames)
            {
                VisibleRemove(v);
                continue;
            }
        }
        else if (idle)
        {
            bool8 touched = v->idle == 0xFFFF
                || (v->x == player->currentCoords.x && v->y == player->currentCoords.y && !v->progress);

            if (touched && v->water == inWater && (v->idle != 0xFFFF || IsWildLevelAllowedByRepel(v->level)))
            {
                VisibleStartBattle(v);
                return;
            }
            if (v->idle == 0xFFFF)
                v->idle = 60;
            VisibleMove(v, player->currentCoords.x, player->currentCoords.y, inWater);
            if (v->shiny && ++v->sparkle >= 40)
            {
                v->sparkle = 0;
                VisibleSparkle(v);
            }
        }
        VisiblePlace(v);
    }

    percent = EncounterRatePercent();
    if (!idle || !settled || headerId == HEADER_NONE || percent == 0 || sWildEncountersDisabled)
        return;
    if (sVisibleSpawnTimer)
    {
        sVisibleSpawnTimer--;
        return;
    }
    if (VisibleCount() < most)
    {
        if (VisibleTrySpawn(headerId))
            sVisibleSpawnTimer = 60 * 100 / percent;
        else
            sVisibleSpawnTimer = 15;
    }
}

bool8 StandardWildEncounter(u16 curMetatileBehavior, u16 prevMetatileBehavior)
{
    u16 headerId;
    struct Roamer *roamer;

    if (sWildEncountersDisabled == TRUE)
        return FALSE;

    headerId = GetCurrentMapWildMonHeaderId();
    if (headerId == HEADER_NONE)
    {
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS)
        {
            headerId = GetBattlePikeWildMonHeaderId();
            if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(gBattlePikeWildMonHeaders[headerId].landMonsInfo->encounterRate, FALSE) != TRUE)
                return FALSE;
            else if (TryGenerateWildMon(gBattlePikeWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, WILD_CHECK_KEEN_EYE) != TRUE)
                return FALSE;
            else if (!TryGenerateBattlePikeWildMon(TRUE))
                return FALSE;

            BattleSetup_StartBattlePikeWildBattle();
            return TRUE;
        }
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
        {
            headerId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(gBattlePyramidWildMonHeaders[headerId].landMonsInfo->encounterRate, FALSE) != TRUE)
                return FALSE;
            else if (TryGenerateWildMon(gBattlePyramidWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, WILD_CHECK_KEEN_EYE) != TRUE)
                return FALSE;

            GenerateBattlePyramidWildMon();
            BattleSetup_StartWildBattle();
            return TRUE;
        }
    }
    else
    {
        if (MetatileBehavior_IsLandWildEncounter(curMetatileBehavior) == TRUE)
        {
            if (GetLandMonsInfo(headerId) == NULL)
                return FALSE;
            else if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(GetLandMonsInfo(headerId)->encounterRate, FALSE) != TRUE)
                return FALSE;

            if (TryStartRoamerEncounter() == TRUE)
            {
                roamer = &gSaveBlock1Ptr->roamer;
                if (!IsWildLevelAllowedByRepel(roamer->level))
                    return FALSE;

                BattleSetup_StartRoamerBattle();
                return TRUE;
            }
            else
            {
                if (VISIBLE_REPLACES_RANDOM())
                    return FALSE;
                if (DoMassOutbreakEncounterTest() == TRUE && SetUpMassOutbreakEncounter(WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
                {
                    BattleSetup_StartWildBattle();
                    return TRUE;
                }

                // try a regular wild land encounter
                if (TryGenerateWildMon(GetLandMonsInfo(headerId), WILD_AREA_LAND, WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
                {
                    BattleSetup_StartWildBattle();
                    return TRUE;
                }

                return FALSE;
            }
        }
        else if (MetatileBehavior_IsWaterWildEncounter(curMetatileBehavior) == TRUE
                 || (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) && MetatileBehavior_IsBridgeOverWater(curMetatileBehavior) == TRUE))
        {
            if (AreLegendariesInSootopolisPreventingEncounters() == TRUE)
                return FALSE;
            else if (gWildMonHeaders[headerId].waterMonsInfo == NULL)
                return FALSE;
            else if (prevMetatileBehavior != curMetatileBehavior && !AllowWildCheckOnNewMetatile())
                return FALSE;
            else if (WildEncounterCheck(gWildMonHeaders[headerId].waterMonsInfo->encounterRate, FALSE) != TRUE)
                return FALSE;

            if (TryStartRoamerEncounter() == TRUE)
            {
                roamer = &gSaveBlock1Ptr->roamer;
                if (!IsWildLevelAllowedByRepel(roamer->level))
                    return FALSE;

                BattleSetup_StartRoamerBattle();
                return TRUE;
            }
            else // try a regular surfing encounter
            {
                if (VISIBLE_REPLACES_RANDOM())
                    return FALSE;
                if (TryGenerateWildMon(gWildMonHeaders[headerId].waterMonsInfo, WILD_AREA_WATER, WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
                {
                    BattleSetup_StartWildBattle();
                    return TRUE;
                }

                return FALSE;
            }
        }
    }

    return FALSE;
}

void RockSmashWildEncounter(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();

    if (headerId != HEADER_NONE)
    {
        const struct WildPokemonInfo *wildPokemonInfo = gWildMonHeaders[headerId].rockSmashMonsInfo;

        if (wildPokemonInfo == NULL)
        {
            gSpecialVar_Result = FALSE;
        }
        else if (WildEncounterCheck(wildPokemonInfo->encounterRate, TRUE) == TRUE
         && TryGenerateWildMon(wildPokemonInfo, WILD_AREA_ROCKS, WILD_CHECK_REPEL | WILD_CHECK_KEEN_EYE) == TRUE)
        {
            BattleSetup_StartWildBattle();
            gSpecialVar_Result = TRUE;
        }
        else
        {
            gSpecialVar_Result = FALSE;
        }
    }
    else
    {
        gSpecialVar_Result = FALSE;
    }
}

bool8 SweetScentWildEncounter(void)
{
    s16 x, y;
    u16 headerId;

    PlayerGetDestCoords(&x, &y);
    headerId = GetCurrentMapWildMonHeaderId();
    if (headerId == HEADER_NONE)
    {
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS)
        {
            headerId = GetBattlePikeWildMonHeaderId();
            if (TryGenerateWildMon(gBattlePikeWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, 0) != TRUE)
                return FALSE;

            TryGenerateBattlePikeWildMon(FALSE);
            BattleSetup_StartBattlePikeWildBattle();
            return TRUE;
        }
        if (gMapHeader.mapLayoutId == LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR)
        {
            headerId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            if (TryGenerateWildMon(gBattlePyramidWildMonHeaders[headerId].landMonsInfo, WILD_AREA_LAND, 0) != TRUE)
                return FALSE;

            GenerateBattlePyramidWildMon();
            BattleSetup_StartWildBattle();
            return TRUE;
        }
    }
    else
    {
        if (MetatileBehavior_IsLandWildEncounter(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
        {
            if (GetLandMonsInfo(headerId) == NULL)
                return FALSE;

            if (TryStartRoamerEncounter() == TRUE)
            {
                BattleSetup_StartRoamerBattle();
                return TRUE;
            }

            if (DoMassOutbreakEncounterTest() == TRUE)
                SetUpMassOutbreakEncounter(0);
            else
                TryGenerateWildMon(GetLandMonsInfo(headerId), WILD_AREA_LAND, 0);

            BattleSetup_StartWildBattle();
            return TRUE;
        }
        else if (MetatileBehavior_IsWaterWildEncounter(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
        {
            if (AreLegendariesInSootopolisPreventingEncounters() == TRUE)
                return FALSE;
            if (gWildMonHeaders[headerId].waterMonsInfo == NULL)
                return FALSE;

            if (TryStartRoamerEncounter() == TRUE)
            {
                BattleSetup_StartRoamerBattle();
                return TRUE;
            }

            TryGenerateWildMon(gWildMonHeaders[headerId].waterMonsInfo, WILD_AREA_WATER, 0);
            BattleSetup_StartWildBattle();
            return TRUE;
        }
    }

    return FALSE;
}

bool8 DoesCurrentMapHaveFishingMons(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();

    if (headerId != HEADER_NONE && gWildMonHeaders[headerId].fishingMonsInfo != NULL)
        return TRUE;
    else
        return FALSE;
}

void FishingWildEncounter(u8 rod)
{
    u16 species;

    if (CheckFeebas() == TRUE)
    {
        u8 level = ChooseWildMonLevel(&sWildFeebas);

        species = sWildFeebas.species;
        CreateWildMon(species, level);
    }
    else
    {
        species = GenerateFishingWildMon(gWildMonHeaders[GetCurrentMapWildMonHeaderId()].fishingMonsInfo, rod);
    }
    IncrementGameStat(GAME_STAT_FISHING_ENCOUNTERS);
    SetPokemonAnglerSpecies(species);
    BattleSetup_StartWildBattle();
}

u16 GetLocalWildMon(bool8 *isWaterMon)
{
    u16 headerId;
    const struct WildPokemonInfo *landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo;

    *isWaterMon = FALSE;
    headerId = GetCurrentMapWildMonHeaderId();
    if (headerId == HEADER_NONE)
        return SPECIES_NONE;
    landMonsInfo = GetLandMonsInfo(headerId);
    waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;
    // Neither
    if (landMonsInfo == NULL && waterMonsInfo == NULL)
        return SPECIES_NONE;
    // Land Pokémon
    else if (landMonsInfo != NULL && waterMonsInfo == NULL)
        return landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
    // Water Pokémon
    else if (landMonsInfo == NULL && waterMonsInfo != NULL)
    {
        *isWaterMon = TRUE;
        return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
    }
    // Either land or water Pokémon
    if ((Random() % 100) < 80)
    {
        return landMonsInfo->wildPokemon[ChooseWildMonIndex_Land()].species;
    }
    else
    {
        *isWaterMon = TRUE;
        return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
    }
}

u16 GetLocalWaterMon(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();

    if (headerId != HEADER_NONE)
    {
        const struct WildPokemonInfo *waterMonsInfo = gWildMonHeaders[headerId].waterMonsInfo;

        if (waterMonsInfo)
            return waterMonsInfo->wildPokemon[ChooseWildMonIndex_WaterRock()].species;
    }
    return SPECIES_NONE;
}

bool8 UpdateRepelCounter(void)
{
    u16 steps;

    if (InBattlePike() || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return FALSE;
    if (InUnionRoom() == TRUE)
        return FALSE;

    steps = VarGet(VAR_REPEL_STEP_COUNT);

    if (steps != 0)
    {
        steps--;
        VarSet(VAR_REPEL_STEP_COUNT, steps);
        if (steps == 0)
        {
            ScriptContext_SetupScript(EventScript_RepelWoreOff);
            return TRUE;
        }
    }
    return FALSE;
}

static bool8 IsWildLevelAllowedByRepel(u8 wildLevel)
{
    u8 i;

    if (!VarGet(VAR_REPEL_STEP_COUNT))
        return TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
        {
            u8 ourLevel = GetMonData(&gPlayerParty[i], MON_DATA_LEVEL);

            if (wildLevel < ourLevel)
                return FALSE;
            else
                return TRUE;
        }
    }

    return FALSE;
}

static bool8 IsAbilityAllowingEncounter(u8 level)
{
    u8 ability;

    if (GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        return TRUE;

    ability = GetMonAbility(&gPlayerParty[0]);
    if (ability == ABILITY_KEEN_EYE || ability == ABILITY_INTIMIDATE)
    {
        u8 playerMonLevel = GetMonData(&gPlayerParty[0], MON_DATA_LEVEL);
        if (playerMonLevel > 5 && level <= playerMonLevel - 5 && !(Random() % 2))
            return FALSE;
    }

    return TRUE;
}

static bool8 TryGetRandomWildMonIndexByType(const struct WildPokemon *wildMon, u8 type, u8 numMon, u8 *monIndex)
{
    u8 validIndexes[numMon]; // variable length array, an interesting feature
    u8 i, validMonCount;

    for (i = 0; i < numMon; i++)
        validIndexes[i] = 0;

    for (validMonCount = 0, i = 0; i < numMon; i++)
    {
        if (gSpeciesInfo[wildMon[i].species].types[0] == type || gSpeciesInfo[wildMon[i].species].types[1] == type)
            validIndexes[validMonCount++] = i;
    }

    if (validMonCount == 0 || validMonCount == numMon)
        return FALSE;

    *monIndex = validIndexes[Random() % validMonCount];
    return TRUE;
}
#ifdef BUGFIX
static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex, u32 size)
#else
static bool8 TryGetAbilityInfluencedWildMonIndex(const struct WildPokemon *wildMon, u8 type, u8 ability, u8 *monIndex)
#endif
{
    if (GetMonData(&gPlayerParty[0], MON_DATA_SANITY_IS_EGG))
        return FALSE;
    else if (GetMonAbility(&gPlayerParty[0]) != ability)
        return FALSE;
    else if (Random() % 2 != 0)
        return FALSE;

#ifdef BUGFIX
    return TryGetRandomWildMonIndexByType(wildMon, type, size, monIndex);
#else
    return TryGetRandomWildMonIndexByType(wildMon, type, LAND_WILD_COUNT, monIndex);
#endif
}

static void ApplyFluteEncounterRateMod(u32 *encRate)
{
    if (FlagGet(FLAG_SYS_ENC_UP_ITEM) == TRUE)
        *encRate += *encRate / 2;
    else if (FlagGet(FLAG_SYS_ENC_DOWN_ITEM) == TRUE)
        *encRate = *encRate / 2;
}

static void ApplyCleanseTagEncounterRateMod(u32 *encRate)
{
    if (GetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM) == ITEM_CLEANSE_TAG)
        *encRate = *encRate * 2 / 3;
}
