#ifndef GUARD_VITA_DAYNIGHT_H
#define GUARD_VITA_DAYNIGHT_H

#include <stdbool.h>
#include <stdint.h>

#define VITA_DAYNIGHT_SPEED_COUNT 7
#define VITA_DAYNIGHT_DAY_MS 86400000u
#define VITA_DAYNIGHT_BONUS_LIMIT_MS ((uint64_t)2147483647u * 60000u)

typedef struct
{
    uint64_t lastMs;
    uint32_t timeMs;
    uint64_t berryBonusMs;
    bool ready;
    bool shifted;
} VitaDayNightClock;

typedef struct
{
    uint32_t timeMs;
    uint64_t berryBonusMs;
    int64_t wallSeconds;
    bool shifted;
} VitaDayNightSnapshot;

typedef struct
{
    float rgb[3];
    float daylight;
    float glow;
} VitaDayNightLight;

typedef struct
{
    float rgb[3];
    float amount;
} VitaDayNightBlend;

typedef struct
{
    float dx, dz, strength;
    uint32_t key;
} VitaDayNightSun;

unsigned VitaDayNight_Speed(int index);
void VitaDayNight_Advance(VitaDayNightClock *clock, uint64_t nowMs, unsigned localSeconds, int speed);
void VitaDayNight_Sync(VitaDayNightClock *clock, uint64_t nowMs, unsigned localSeconds, int speed);
VitaDayNightLight VitaDayNight_At(unsigned timeMs);
VitaDayNightBlend VitaDayNight_Moonlight(const VitaDayNightLight *light, float fade, const float fadeRgb[3]);
VitaDayNightSun VitaDayNight_SunAt(unsigned timeMs);
void VitaDayNight_Update(void);
void VitaDayNight_Resume(void);
const VitaDayNightLight *VitaDayNight_Get(void);
const VitaDayNightSun *VitaDayNight_GetSun(void);
bool VitaDayNight_Time(unsigned *seconds, unsigned *rate);
bool VitaDayNight_IsShifted(void);
bool VitaDayNight_Active(void);
bool VitaDayNight_BerryDue(void);
unsigned VitaDayNight_TakeBerryMinutes(void);
void VitaDayNight_Reset(void);
bool VitaDayNight_SyncToConsole(void);
bool VitaDayNight_Snapshot(VitaDayNightSnapshot *snapshot);
bool VitaDayNight_Restore(const VitaDayNightSnapshot *snapshot);

// Game environment queries
bool VitaGame_IsOutdoor(void);
bool VitaGame_IsOverworld(void);
bool VitaGame_LocalTimeSeconds(unsigned *seconds);

#endif // GUARD_VITA_DAYNIGHT_H
