#include <stdint.h>
#include "global.h"
#include "wild_encounter.h"
#include "constants/maps.h"
#include "constants/species.h"
#include "platform/vita_daynight.h"
#include "platform/vita_encounters.h"
#include "platform/vita_extras.h"

#define ENCOUNTER_RULE(route, slotIndex, expected, morning, day, night) \
    { \
        (uint8_t)MAP_GROUP(MAP_##route), \
        (uint8_t)MAP_NUM(MAP_##route), \
        (uint8_t)(slotIndex), \
        (uint16_t)SPECIES_##expected, \
        { (uint16_t)SPECIES_##morning, (uint16_t)SPECIES_##day, (uint16_t)SPECIES_##night } \
    }

static const VitaTimedEncounterRule sRules[] = {
    // Route 101 (Altered: 20% + 20% + 10% + 5% = 55%)
    ENCOUNTER_RULE(ROUTE101, 0, WURMPLE,   TAILLOW,   WURMPLE,   HOOTHOOT),
    ENCOUNTER_RULE(ROUTE101, 1, POOCHYENA, LEDYBA,    POOCHYENA, POOCHYENA),
    ENCOUNTER_RULE(ROUTE101, 2, WURMPLE,   TAILLOW,   WURMPLE,   SPINARAK),
    ENCOUNTER_RULE(ROUTE101, 6, WURMPLE,   WURMPLE,   ZIGZAGOON, ODDISH),

    // Route 102 (Altered: 20% + 20% + 10% + 10% = 60%)
    ENCOUNTER_RULE(ROUTE102, 0, POOCHYENA, SURSKIT,   POOCHYENA, POOCHYENA),
    ENCOUNTER_RULE(ROUTE102, 1, WURMPLE,   TAILLOW,   WURMPLE,   ODDISH),
    ENCOUNTER_RULE(ROUTE102, 3, WURMPLE,   TAILLOW,   WURMPLE,   ZUBAT),
    ENCOUNTER_RULE(ROUTE102, 5, LOTAD,     LOTAD,     SEEDOT,    ODDISH),

    // Route 103 (Altered: 20% + 20% + 10% = 50%)
    ENCOUNTER_RULE(ROUTE103, 0, POOCHYENA, WINGULL,   POOCHYENA, HOOTHOOT),
    ENCOUNTER_RULE(ROUTE103, 1, POOCHYENA, POOCHYENA, ZIGZAGOON, POOCHYENA),
    ENCOUNTER_RULE(ROUTE103, 2, POOCHYENA, WINGULL,   ZIGZAGOON, ZUBAT),

    // Route 104 (Altered: 20% + 20% + 10% + 10% = 60%)
    ENCOUNTER_RULE(ROUTE104, 0, POOCHYENA, TAILLOW,   WINGULL,   POOCHYENA),
    ENCOUNTER_RULE(ROUTE104, 1, WURMPLE,   WURMPLE,   WURMPLE,   ZUBAT),
    ENCOUNTER_RULE(ROUTE104, 4, MARILL,    MARILL,    MARILL,    ODDISH),
    ENCOUNTER_RULE(ROUTE104, 5, POOCHYENA, TAILLOW,   WINGULL,   POOCHYENA),

    // Route 110
    ENCOUNTER_RULE(ROUTE110, 0, POOCHYENA, PLUSLE,    MINUN,     ODDISH),
    ENCOUNTER_RULE(ROUTE110, 3, ELECTRIKE, ELECTRIKE, ELECTRIKE, GLOOM),
    ENCOUNTER_RULE(ROUTE110, 7, GULPIN,    GULPIN,    GULPIN,    ZUBAT),

    // Route 116
    ENCOUNTER_RULE(ROUTE116, 0, POOCHYENA, TAILLOW,   POOCHYENA, POOCHYENA),
    ENCOUNTER_RULE(ROUTE116, 1, NINCADA,   NINCADA,   NINCADA,   ZUBAT),
    ENCOUNTER_RULE(ROUTE116, 4, NINCADA,   TAILLOW,   NINCADA,   POOCHYENA),
    ENCOUNTER_RULE(ROUTE116, 7, TAILLOW,   TAILLOW,   TAILLOW,   ZUBAT),

    // Route 117
    ENCOUNTER_RULE(ROUTE117, 0, POOCHYENA, MARILL,    POOCHYENA, VOLBEAT),
    ENCOUNTER_RULE(ROUTE117, 1, POOCHYENA, ILLUMISE,  POOCHYENA, ILLUMISE),
    ENCOUNTER_RULE(ROUTE117, 3, ODDISH,    MARILL,    ODDISH,    ODDISH),
    ENCOUNTER_RULE(ROUTE117, 7, ILLUMISE,  ROSELIA,   ROSELIA,   VOLBEAT),

    // Route 119
    ENCOUNTER_RULE(ROUTE119, 0, ZIGZAGOON, LINOONE,   ZIGZAGOON, ODDISH),
    ENCOUNTER_RULE(ROUTE119, 2, ZIGZAGOON, ZIGZAGOON, LINOONE,   GLOOM),
    ENCOUNTER_RULE(ROUTE119, 7, ODDISH,    TROPIUS,   ODDISH,    GLOOM),

    // Route 120
    ENCOUNTER_RULE(ROUTE120, 0, POOCHYENA, MARILL,    POOCHYENA, MIGHTYENA),
    ENCOUNTER_RULE(ROUTE120, 6, ODDISH,    MARILL,    ODDISH,    GLOOM),

    // Route 121
    ENCOUNTER_RULE(ROUTE121, 0, POOCHYENA, WINGULL,   POOCHYENA, SHUPPET),
    ENCOUNTER_RULE(ROUTE121, 3, SHUPPET,   ODDISH,    GLOOM,     SHUPPET),
    ENCOUNTER_RULE(ROUTE121, 9, WINGULL,   WINGULL,   WINGULL,   DUSKULL),

    // Route 123
    ENCOUNTER_RULE(ROUTE123, 0, POOCHYENA, WINGULL,   POOCHYENA, SHUPPET),
    ENCOUNTER_RULE(ROUTE123, 3, SHUPPET,   ODDISH,    GLOOM,     SHUPPET),
    ENCOUNTER_RULE(ROUTE123, 9, WINGULL,   WINGULL,   WINGULL,   DUSKULL),
};

uint32_t VitaEncounters_Phase(const uint32_t seconds)
{
    const uint32_t hour = (seconds % 86400u) / 3600u;

    if (hour >= 4 && hour < 10)
    {
        return VITA_TIME_MORNING;
    }
    return (hour >= 10 && hour < 20) ? VITA_TIME_DAY : VITA_TIME_NIGHT;
}

uint16_t VitaEncounters_Resolve(const VitaTimedEncounterRule *const rules, const size_t count,
                                const uint32_t mapGroup, const uint32_t mapNum, const uint32_t slot,
                                const uint16_t original, const uint32_t phase)
{
    if (phase >= VITA_TIME_COUNT || rules == NULL)
    {
        return original;
    }

    for (size_t index = 0; index < count; ++index)
    {
        if (rules[index].mapGroup == mapGroup
         && rules[index].mapNum == mapNum
         && rules[index].slot == slot
         && rules[index].expectedSpecies == original)
        {
            return rules[index].species[phase];
        }
    }

    return original;
}

// Land slots of a map for one time of day (the new UI's encounter table), even if the clock is elsewhere
const struct WildPokemonInfo *VitaEncounters_LandPhase(const struct WildPokemonInfo *const original,
                                                       const uint32_t mapGroup, const uint32_t mapNum, const uint32_t phase)
{
    static struct WildPokemon slots[12];
    static struct WildPokemonInfo timed;
    if (original == NULL || original->wildPokemon == NULL || VitaExtras_GetFast(FAST_EXTRA_TIMED_ENCOUNTERS) != 1)
        return original;
    for (size_t index = 0; index < ARRAY_COUNT(slots); ++index)
    {
        slots[index] = original->wildPokemon[index];
        slots[index].species = VitaEncounters_Resolve(sRules, ARRAY_COUNT(sRules), mapGroup, mapNum,
                                                     (uint32_t)index, slots[index].species, phase);
    }
    timed = *original;
    timed.wildPokemon = slots;
    return &timed;
}

const struct WildPokemonInfo *VitaEncounters_Land(const struct WildPokemonInfo *const original,
                                                  const uint32_t mapGroup, const uint32_t mapNum)
{
    static struct WildPokemon slots[12];
    static struct WildPokemonInfo timed;
    uint32_t seconds = 0;
    uint32_t rate = 0;
    bool changed = false;

    if (original == NULL || original->wildPokemon == NULL
     || VitaExtras_GetFast(FAST_EXTRA_TIMED_ENCOUNTERS) != 1
     || !VitaDayNight_Time(&seconds, &rate))
    {
        return original;
    }

    const uint32_t phase = VitaEncounters_Phase(seconds);

    for (size_t index = 0; index < ARRAY_COUNT(slots); ++index)
    {
        slots[index] = original->wildPokemon[index];
        slots[index].species = VitaEncounters_Resolve(sRules, ARRAY_COUNT(sRules),
                                                     mapGroup, mapNum,
                                                     (uint32_t)index, slots[index].species, phase);
        changed |= (slots[index].species != original->wildPokemon[index].species);
    }

    if (!changed)
    {
        return original;
    }

    timed = *original;
    timed.wildPokemon = slots;
    return &timed;
}

bool VitaEncounters_PlayerInWater(const bool surfing, const bool underwater)
{
    return surfing || underwater;
}
