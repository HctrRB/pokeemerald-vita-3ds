#ifndef GUARD_VITA_ENCOUNTERS_H
#define GUARD_VITA_ENCOUNTERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    VITA_TIME_MORNING,
    VITA_TIME_DAY,
    VITA_TIME_NIGHT,
    VITA_TIME_COUNT
};

typedef struct
{
    uint8_t mapGroup;
    uint8_t mapNum;
    uint8_t slot;
    uint16_t expectedSpecies;
    uint16_t species[VITA_TIME_COUNT];
} VitaTimedEncounterRule;

unsigned VitaEncounters_Phase(const unsigned seconds);

uint16_t VitaEncounters_Resolve(const VitaTimedEncounterRule *rules, const size_t count,
                                const unsigned mapGroup, const unsigned mapNum, const unsigned slot,
                                const uint16_t original, const unsigned phase);

struct WildPokemonInfo;
const struct WildPokemonInfo *VitaEncounters_Land(const struct WildPokemonInfo *original,
                                                  const unsigned mapGroup, const unsigned mapNum);

const struct WildPokemonInfo *VitaEncounters_LandPhase(const struct WildPokemonInfo *original,
                                                       const unsigned mapGroup, const unsigned mapNum, const unsigned phase);
bool VitaEncounters_PlayerInWater(const bool surfing, const bool underwater);

#endif // GUARD_VITA_ENCOUNTERS_H
