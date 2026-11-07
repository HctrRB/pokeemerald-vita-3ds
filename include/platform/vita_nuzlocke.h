#ifndef GUARD_VITA_NUZLOCKE_H
#define GUARD_VITA_NUZLOCKE_H

#include <stdbool.h>
#include <stdint.h>

enum VitaNuzAreaStatus
{
    VITA_NUZ_AREA_NONE      = 0,
    VITA_NUZ_AREA_AVAILABLE = 1,
    VITA_NUZ_AREA_CAUGHT    = 2,
    VITA_NUZ_AREA_MISSED    = 3
};

enum
{
    VITA_NUZ_MAX_AREAS  = 256,
    VITA_NUZ_MAX_FALLEN = 96
};

typedef struct
{
    uint32_t personality;
    uint32_t otId;
    uint8_t isShedinja;
    uint8_t pad[3];
} VitaNuzFallenEntry;

typedef struct
{
    uint8_t areaStatus[VITA_NUZ_MAX_AREAS];
    uint16_t fallenCount;
    VitaNuzFallenEntry fallen[VITA_NUZ_MAX_FALLEN];
} VitaNuzState;

void VitaNuzlocke_Init(void);
void VitaNuzlocke_Reset(void);
void VitaNuzlocke_LoadForSlot(int32_t saveSlot);
void VitaNuzlocke_SaveForSlot(int32_t saveSlot);

enum VitaNuzAreaStatus VitaNuzlocke_GetArea(uint16_t mapSec);
void VitaNuzlocke_SetArea(uint16_t mapSec, enum VitaNuzAreaStatus status);

bool VitaNuzlocke_IsFallen(uint32_t personality, uint32_t otId, bool isShedinja);
void VitaNuzlocke_SetFallen(uint32_t personality, uint32_t otId, bool isShedinja, bool fallen);

void VitaNuzlocke_GetCounts(uint32_t *outCaught, uint32_t *outMissed, uint32_t *outFallen);
uint8_t VitaNuzlocke_GetLevelCap(uint8_t badgeCount, const char **outLeaderName);
uint8_t VitaNuzlocke_CountBadges(void);

#endif // GUARD_VITA_NUZLOCKE_H
