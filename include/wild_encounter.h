#ifndef GUARD_WILD_ENCOUNTER_H
#define GUARD_WILD_ENCOUNTER_H

#include "constants/wild_encounter.h"

struct WildPokemon
{
    u8 minLevel;
    u8 maxLevel;
    u16 species;
};

struct WildPokemonInfo
{
    u8 encounterRate;
    const struct WildPokemon *wildPokemon;
};

struct WildPokemonHeader
{
    u8 mapGroup;
    u8 mapNum;
    const struct WildPokemonInfo *landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo;
    const struct WildPokemonInfo *rockSmashMonsInfo;
    const struct WildPokemonInfo *fishingMonsInfo;
};

extern struct WildPokemonHeader *gWildMonHeaders;
extern const struct WildPokemonHeader gWildMonHeaders_ROM[];

void DisableWildEncounters(bool8 disabled);
bool8 StandardWildEncounter(u16 curMetatileBehavior, u16 prevMetatileBehavior);
bool8 SweetScentWildEncounter(void);
bool8 DoesCurrentMapHaveFishingMons(void);
void FishingWildEncounter(u8 rod);
u16 GetLocalWildMon(bool8 *isWaterMon);
u16 GetLocalWaterMon(void);
bool8 UpdateRepelCounter(void);
void VisibleWild_Update(void);
void VisibleWild_CameraShift(s16 dx, s16 dy);
bool8 VisibleWild_StepOnto(void);
bool8 VisibleWild_IsSprite(u8 spriteId);
bool8 VisibleWild_SpriteTile(u8 spriteId, s16 *x16, s16 *y16);

#endif // GUARD_WILD_ENCOUNTER_H
