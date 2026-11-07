#include <stdint.h>
#include "platform/vita_nuzlocke.h"
#include "vita_platform.h"
#include "platform.h"
#include "global.h"
#include "event_data.h"
#include "constants/flags.h"

#include <stdio.h>
#include <string.h>

enum
{
    NUZ_FILE_MAGIC   = 0x315A554EU, // 'NUZ1'
    NUZ_CHAMPION_CAP = 58U,
    NUZ_GYM_COUNT    = 8U
};

typedef struct
{
    uint32_t magic;
    VitaNuzState state;
} VitaNuzDiskRecord;

static const uint8_t sGymLevelCaps[NUZ_GYM_COUNT] = { 15, 19, 24, 29, 31, 33, 42, 46 };
static const char *const sGymLeaderNames[NUZ_GYM_COUNT + 1] = {
    "ROXANNE", "BRAWLY", "WATTSON", "FLANNERY",
    "NORMAN", "WINONA", "TATE & LIZA", "JUAN", "WALLACE"
};

static VitaNuzState sNuzState;
static bool sNuzInitialized = false;

static void BuildNuzPath(char *dest, const size_t size, const int32_t saveSlot)
{
    char rel[48];
    const int32_t slot = (saveSlot < 1 || saveSlot > 3) ? 1 : saveSlot;
    snprintf(rel, sizeof(rel), "saves/nuzlocke_%d.bin", (int32_t)slot);
    VitaPlatform_BuildPath(dest, size, rel);
}

void VitaNuzlocke_Reset(void)
{
    memset(&sNuzState, 0, sizeof(sNuzState));
}

void VitaNuzlocke_LoadForSlot(const int32_t saveSlot)
{
    char path[256];
    VitaNuzDiskRecord rec;
    VitaNuzlocke_Reset();
    BuildNuzPath(path, sizeof(path), saveSlot);

    FILE *const f = fopen(path, "rb");
    if (f == NULL)
        return;

    if (fread(&rec, sizeof(rec), 1, f) == 1 && rec.magic == NUZ_FILE_MAGIC)
    {
        sNuzState = rec.state;
        if (sNuzState.fallenCount > VITA_NUZ_MAX_FALLEN)
            sNuzState.fallenCount = VITA_NUZ_MAX_FALLEN;
    }
    fclose(f);
}

void VitaNuzlocke_SaveForSlot(const int32_t saveSlot)
{
    char path[256];
    VitaNuzDiskRecord rec;
    rec.magic = NUZ_FILE_MAGIC;
    rec.state = sNuzState;
    BuildNuzPath(path, sizeof(path), saveSlot);

    FILE *const f = fopen(path, "wb");
    if (f == NULL)
        return;

    fwrite(&rec, sizeof(rec), 1, f);
    fflush(f);
    fclose(f);
}

void VitaNuzlocke_Init(void)
{
    if (sNuzInitialized)
        return;
    sNuzInitialized = true;
    const int32_t slot = (int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE);
    VitaNuzlocke_LoadForSlot(slot);
}

enum VitaNuzAreaStatus VitaNuzlocke_GetArea(const uint16_t mapSec)
{
    VitaNuzlocke_Init();
    if (mapSec >= VITA_NUZ_MAX_AREAS)
        return VITA_NUZ_AREA_NONE;
    return (enum VitaNuzAreaStatus)sNuzState.areaStatus[mapSec];
}

void VitaNuzlocke_SetArea(const uint16_t mapSec, const enum VitaNuzAreaStatus status)
{
    VitaNuzlocke_Init();
    if (mapSec >= VITA_NUZ_MAX_AREAS)
        return;
    if (sNuzState.areaStatus[mapSec] == (uint8_t)status)
        return;
    sNuzState.areaStatus[mapSec] = (uint8_t)status;
    VitaNuzlocke_SaveForSlot((int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE));
}

static int32_t FindFallenIndex(const uint32_t personality, const uint32_t otId, const bool isShedinja)
{
    const uint8_t shedFlag = isShedinja ? 1U : 0U;
    for (uint16_t i = 0; i < sNuzState.fallenCount; i++)
    {
        if (sNuzState.fallen[i].personality == personality
            && sNuzState.fallen[i].otId == otId
            && sNuzState.fallen[i].isShedinja == shedFlag)
        {
            return (int32_t)i;
        }
    }
    return -1;
}

bool VitaNuzlocke_IsFallen(const uint32_t personality, const uint32_t otId, const bool isShedinja)
{
    VitaNuzlocke_Init();
    return FindFallenIndex(personality, otId, isShedinja) >= 0;
}

void VitaNuzlocke_SetFallen(const uint32_t personality, const uint32_t otId, const bool isShedinja, const bool fallen)
{
    VitaNuzlocke_Init();
    const int32_t existing = FindFallenIndex(personality, otId, isShedinja);
    if (fallen)
    {
        if (existing >= 0 || sNuzState.fallenCount >= VITA_NUZ_MAX_FALLEN)
            return;
        VitaNuzFallenEntry *const dst = &sNuzState.fallen[sNuzState.fallenCount++];
        dst->personality = personality;
        dst->otId = otId;
        dst->isShedinja = isShedinja ? 1U : 0U;
    }
    else
    {
        if (existing < 0)
            return;
        const uint16_t last = --sNuzState.fallenCount;
        if ((uint16_t)existing < last)
            sNuzState.fallen[existing] = sNuzState.fallen[last];
    }
    VitaNuzlocke_SaveForSlot((int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE));
}

void VitaNuzlocke_GetCounts(uint32_t *outCaught, uint32_t *outMissed, uint32_t *outFallen)
{
    VitaNuzlocke_Init();
    uint32_t caught = 0;
    uint32_t missed = 0;
    for (uint32_t i = 0; i < VITA_NUZ_MAX_AREAS; i++)
    {
        if (sNuzState.areaStatus[i] == VITA_NUZ_AREA_CAUGHT)
            caught++;
        else if (sNuzState.areaStatus[i] == VITA_NUZ_AREA_MISSED)
            missed++;
    }
    if (outCaught != NULL)
        *outCaught = caught;
    if (outMissed != NULL)
        *outMissed = missed;
    if (outFallen != NULL)
        *outFallen = sNuzState.fallenCount;
}

uint8_t VitaNuzlocke_CountBadges(void)
{
    uint8_t badges = 0;
    for (uint16_t flag = FLAG_BADGE01_GET; flag <= FLAG_BADGE08_GET; flag++)
    {
        if (FlagGet(flag))
            badges++;
    }
    return badges;
}

uint8_t VitaNuzlocke_GetLevelCap(const uint8_t badgeCount, const char **outLeaderName)
{
    if (badgeCount < NUZ_GYM_COUNT)
    {
        if (outLeaderName != NULL)
            *outLeaderName = sGymLeaderNames[badgeCount];
        return sGymLevelCaps[badgeCount];
    }
    if (outLeaderName != NULL)
        *outLeaderName = sGymLeaderNames[NUZ_GYM_COUNT];
    return NUZ_CHAMPION_CAP;
}
