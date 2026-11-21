#ifndef GUARD_VITA_DAYNIGHT_H
#define GUARD_VITA_DAYNIGHT_H

#include <stdbool.h>
#include <stdint.h>

#define VITA_DAYNIGHT_SPEED_COUNT 7
#define VITA_DAYNIGHT_DAY_MS 86400000u

// The day's schedule, in minutes after midnight. Every day/night effect (light and tint, the
// shadow sun, the shafts and sky bloom, the moon, windows and street lamps, fireflies, the HD-2D
// grade) follows it through one state built once per frame (VitaDayNight_State).
#define VITA_DAYNIGHT_DAWN_MIN       (6u * 60u)        // night starts to lift
#define VITA_DAYNIGHT_SUNRISE_MIN    (7u * 60u)        // the sun rises; the moon has set
#define VITA_DAYNIGHT_MORNING_MIN    (8u * 60u)        // full morning light; lamps out
#define VITA_DAYNIGHT_NOON_START_MIN (12u * 60u)       // the sun at its highest...
#define VITA_DAYNIGHT_NOON_END_MIN   (14u * 60u)       // ...until here
#define VITA_DAYNIGHT_DUSK_MIN       (18u * 60u)       // the evening begins: warmer light
#define VITA_DAYNIGHT_SUNSET_MIN     (19u * 60u + 30u) // sunset colours; lamps coming on
#define VITA_DAYNIGHT_NIGHT_MIN      (21u * 60u)       // night: the sun has set, the moon rises
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

typedef struct
{
    float dx, dy, dz;
    float altitude;
    float strength;
} VitaDayNightPath;

uint32_t VitaDayNight_Speed(int32_t index);
void VitaDayNight_Advance(VitaDayNightClock *clock, uint64_t nowMs, uint32_t localSeconds, int32_t speed);
void VitaDayNight_Sync(VitaDayNightClock *clock, uint64_t nowMs, uint32_t localSeconds, int32_t speed);
// Everything the day/night effects read, for one frame (VitaDayNight_UpdateFrame). Outdoors with
// the day/night option on it follows the clock; otherwise it is a fixed noon.
typedef struct
{
    uint32_t timeMs;              // the clock this state was built from
    bool active;                  // day/night on and outdoors
    VitaDayNightLight light;      // tint, daylight (1 day, 0 night), glow (windows, lamps)
    VitaDayNightBlend moonlight;  // the night layer over the scene, before any screen fade
    VitaDayNightSun sun;          // the chunks' shadow sun, in 15-minute steps (each step relights)
    VitaDayNightPath sunPath;     // the sun on screen: shafts, sky bloom, glints
    VitaDayNightPath moonPath;    // the moon on screen
    float moonShafts;             // 0..1: how much the moon's shafts show (they need the dark)
    float day, dusk, night;       // the HD-2D grade's weights, summing to 1
} VitaDayNightState;

// Core 0, once per presented frame: advances the clock and builds the state every effect reads.
void VitaDayNight_UpdateFrame(void);
const VitaDayNightState *VitaDayNight_State(void);

VitaDayNightLight VitaDayNight_At(uint32_t timeMs);
VitaDayNightBlend VitaDayNight_Moonlight(const VitaDayNightLight *light, float fade, const float fadeRgb[3]);
VitaDayNightSun VitaDayNight_SunAt(uint32_t timeMs);
VitaDayNightPath VitaDayNight_SunPathAt(uint32_t timeMs);
VitaDayNightPath VitaDayNight_MoonPathAt(uint32_t timeMs);
uint32_t VitaDayNight_GetTimeMs(void);
void VitaDayNight_Update(void);
void VitaDayNight_Resume(void);
const VitaDayNightLight *VitaDayNight_Get(void);
const VitaDayNightSun *VitaDayNight_GetSun(void);
bool VitaDayNight_Time(uint32_t *seconds, uint32_t *rate);
bool VitaDayNight_IsShifted(void);
bool VitaDayNight_Active(void);
bool VitaDayNight_BerryDue(void);
uint32_t VitaDayNight_TakeBerryMinutes(void);
void VitaDayNight_Reset(void);
bool VitaDayNight_SyncToConsole(void);
bool VitaDayNight_Snapshot(VitaDayNightSnapshot *snapshot);
bool VitaDayNight_Restore(const VitaDayNightSnapshot *snapshot);

// Game environment queries
bool VitaGame_IsOutdoor(void);
bool VitaGame_IsOverworld(void);
bool VitaGame_LocalTimeSeconds(uint32_t *seconds);

#endif // GUARD_VITA_DAYNIGHT_H
