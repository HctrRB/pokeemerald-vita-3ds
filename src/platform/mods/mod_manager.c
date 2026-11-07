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
extern const u8 gSpeciesNames_ROM[][POKEMON_NAME_LENGTH + 1];
u8 gSpeciesNames[NUM_SPECIES][POKEMON_NAME_LENGTH + 1];
struct SpeciesInfo gSpeciesInfo[NUM_SPECIES];
struct BattleMove gBattleMoves[MOVES_COUNT];
struct Item gItems[ITEMS_COUNT];

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
    memcpy(gWildMonHeaders, gWildMonHeaders_ROM, numWildHeaders * sizeof(struct WildPokemonHeader));

    memcpy(gSpeciesInfo, gSpeciesInfo_ROM, sizeof(gSpeciesInfo));
    memcpy(gSpeciesNames, gSpeciesNames_ROM, sizeof(gSpeciesNames));
    memcpy(gBattleMoves, gBattleMoves_ROM, sizeof(gBattleMoves));
    memcpy(gItems, gItems_ROM, sizeof(gItems));
}

bool8 gModsEnabled = FALSE;
char *gActiveModSelector = NULL;

// The RAM copies of the ROM tables (wild encounters, species, moves, items) are needed
// with or without mod support; leaving them unfilled crashes on the first map load.
void ModManager_Init(void) { InitRAMShadows(); }
void ModManager_Shutdown(void) {}
bool8 ModManager_IsEnabled(void) { return FALSE; }
char *ModManager_ReadFileToString(const char *path) { (void)path; return NULL; }
const struct Trainer *ModManager_GetTrainer(u16 trainerId) { return &gTrainers[trainerId]; }
bool8 ModManager_GetTrainerFrontPicOverride(u16 trainerPicId, void *destBuffer) { (void)trainerPicId; (void)destBuffer; return FALSE; }
u16 ModManager_GetStarterSpecies(u8 slot, u16 vanillaSpecies) { (void)slot; return vanillaSpecies; }
u8 ModManager_GetStarterLevel(u8 slot, u8 vanillaLevel) { (void)slot; return vanillaLevel; }
const struct WildPokemonHeader *ModManager_GetWildMonHeader(u16 headerId, const struct WildPokemonHeader *vanilla) { (void)headerId; return vanilla; }
const struct WildPokemonHeader *ModManager_GetWildMonHeaderByMap(u8 mapGroup, u8 mapNum, const struct WildPokemonHeader *vanilla) { (void)mapGroup; (void)mapNum; return vanilla; }
const struct MapHeader *ModManager_GetMapHeaderByMap(u16 mapGroup, u16 mapNum, const struct MapHeader *vanilla) { (void)mapGroup; (void)mapNum; return vanilla; }

