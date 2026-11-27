#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "mod_manager.h"

#include "wild_encounter.h"
#include "pokemon.h"
#include "battle.h"
#include "item.h"
#include "constants/species.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/trainers.h"
#include "malloc.h"

struct WildPokemonHeader *gWildMonHeaders = NULL;

static bool sRAMShadowsInitialized = false;

static void InitRAMShadows(void) {
    if (sRAMShadowsInitialized) return;
    sRAMShadowsInitialized = true;
    
    // Determine size of gWildMonHeaders_ROM
    int32_t numWildHeaders = 0;
    while (gWildMonHeaders_ROM[numWildHeaders].mapGroup != 0xFF) { // MAP_GROUP(MAP_UNDEFINED) is usually 0xFF
        numWildHeaders++;
    }
    numWildHeaders++; // Include the terminator

    gWildMonHeaders = malloc(numWildHeaders * sizeof(struct WildPokemonHeader));
    // Nothing writes the table yet: without the copy, the game reads the ROM one unchanged.
    if (gWildMonHeaders == NULL)
    {
        gWildMonHeaders = (struct WildPokemonHeader *)gWildMonHeaders_ROM;
        return;
    }
    memcpy(gWildMonHeaders, gWildMonHeaders_ROM, numWildHeaders * sizeof(struct WildPokemonHeader));
}

// Initializes the mutable RAM copy of the wild encounter headers at boot. Species info, names,
// moves and items are read from their const ROM tables (see gBattleMoves in pokemon.h).
// Pass-through hooks preserve vanilla lookups without dynamic JSON loading.
void ModManager_Init(void) { InitRAMShadows(); }
const struct Trainer *ModManager_GetTrainer(u16 trainerId) { return &gTrainers[trainerId]; }
bool8 ModManager_GetTrainerFrontPicOverride(u16 trainerPicId, void *destBuffer) { (void)trainerPicId; (void)destBuffer; return FALSE; }
u16 ModManager_GetStarterSpecies(u8 slot, u16 vanillaSpecies) { (void)slot; return vanillaSpecies; }
u8 ModManager_GetStarterLevel(u8 slot, u8 vanillaLevel) { (void)slot; return vanillaLevel; }
const struct MapHeader *ModManager_GetMapHeaderByMap(u16 mapGroup, u16 mapNum, const struct MapHeader *vanilla) { (void)mapGroup; (void)mapNum; return vanilla; }

