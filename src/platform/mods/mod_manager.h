#ifndef GUARD_MOD_MANAGER_H
#define GUARD_MOD_MANAGER_H

#include "global.h"
#include "data.h"
#include <stdbool.h>

void ModManager_Init(void);

// Trainer Override API
const struct Trainer *ModManager_GetTrainer(u16 trainerId);

// Graphics Override API
// Returns TRUE if overridden and copies the raw 4bpp uncompressed data into destBuffer.
bool8 ModManager_GetTrainerFrontPicOverride(u16 trainerPicId, void *destBuffer);

// Starter Override API
u16 ModManager_GetStarterSpecies(u8 slot, u16 vanillaSpecies);
u8 ModManager_GetStarterLevel(u8 slot, u8 vanillaLevel);

const struct MapHeader *ModManager_GetMapHeaderByMap(u16 mapGroup, u16 mapNum, const struct MapHeader *vanilla);

#endif // GUARD_MOD_MANAGER_H


