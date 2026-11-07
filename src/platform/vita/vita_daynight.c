#include <stdint.h>
#include <math.h>
#include "global.h"
#include "main.h"
#include "overworld.h"
#include "rtc.h"
#include "constants/map_types.h"
#include "platform/vita_daynight.h"
#include "platform/vita_extras.h"
#include "vita_platform.h"

static const uint32_t sSpeeds[VITA_DAYNIGHT_SPEED_COUNT] = {1, 6, 12, 24, 60, 120, 1000};
static VitaDayNightClock sClock;
static VitaDayNightLight sLight = {{1.0f, 1.0f, 1.0f}, 1.0f, 0.0f};
static VitaDayNightSun sSun = {0.85f, 0.55f, 1.0f, 0};
static uint32_t sLocalSeconds;
static uint64_t sSampleMs;
static bool sSampled;
static bool sEnabled;
static bool sResume;
static bool sValid;
static int64_t sWallSeconds;

uint32_t VitaDayNight_Speed(const int32_t index)
{
    return (index >= 0 && index < VITA_DAYNIGHT_SPEED_COUNT) ? sSpeeds[index] : 1;
}

void VitaDayNight_Advance(VitaDayNightClock *const clock, const uint64_t nowMs,
                          const uint32_t localSeconds, const int32_t speed)
{
    const uint32_t rate = VitaDayNight_Speed(speed);

    if (!clock->ready || (rate == 1 && !clock->shifted))
    {
        clock->timeMs = (localSeconds % 86400u) * 1000u;
    }

    if (clock->ready && nowMs >= clock->lastMs)
    {
        const uint64_t elapsed = nowMs - clock->lastMs;
        const uint64_t room = VITA_DAYNIGHT_BONUS_LIMIT_MS - clock->berryBonusMs;

        if (rate != 1 || clock->shifted)
        {
            clock->timeMs = (uint32_t)((clock->timeMs + (elapsed % VITA_DAYNIGHT_DAY_MS) * rate)
                                      % VITA_DAYNIGHT_DAY_MS);
        }

        if (rate > 1)
        {
            clock->berryBonusMs += (elapsed > room / (rate - 1u)) ? room : (elapsed * (rate - 1u));
        }
    }

    if (rate > 1)
    {
        clock->shifted = true;
    }
    clock->lastMs = nowMs;
    clock->ready = true;
}

void VitaDayNight_Sync(VitaDayNightClock *const clock, const uint64_t nowMs,
                       const uint32_t localSeconds, const int32_t speed)
{
    if (clock->ready)
    {
        VitaDayNight_Advance(clock, nowMs, localSeconds, speed);
    }
    clock->timeMs = (localSeconds % 86400u) * 1000u;
    clock->shifted = false;
    clock->lastMs = nowMs;
    clock->ready = true;
}

VitaDayNightLight VitaDayNight_At(const uint32_t timeMs)
{
    static const struct
    {
        uint32_t minute;
        VitaDayNightLight light;
    } keys[] = {
        {0,    {{0.66f, 0.74f, 0.92f}, 0.0f,  1.00f}}, // 00:00 Midnight Blue
        {120,  {{0.66f, 0.74f, 0.92f}, 0.0f,  1.00f}}, // 02:00 Deep Night
        {240,  {{0.96f, 0.94f, 0.88f}, 0.75f, 0.00f}}, // 04:00 Dawn Gold
        {480,  {{0.96f, 0.94f, 0.88f}, 0.75f, 0.00f}}, // 08:00 Early Morning
        {600,  {{1.00f, 1.00f, 1.00f}, 1.00f, 0.00f}}, // 10:00 Pure Noon
        {900,  {{1.00f, 1.00f, 1.00f}, 1.00f, 0.00f}}, // 15:00 Bright Afternoon
        {1020, {{1.00f, 0.80f, 0.65f}, 0.35f, 0.25f}}, // 17:00 Sunset Orange
        {1080, {{1.00f, 0.80f, 0.65f}, 0.35f, 0.25f}}, // 18:00 Dusk
        {1200, {{0.66f, 0.74f, 0.92f}, 0.0f,  1.00f}}, // 20:00 Nightfall
        {1440, {{0.66f, 0.74f, 0.92f}, 0.0f,  1.00f}}, // 24:00 Cycle End
    };
    const uint32_t current = timeMs % VITA_DAYNIGHT_DAY_MS;

    for (size_t index = 1; index < (sizeof(keys) / sizeof(keys[0])); ++index)
    {
        const uint32_t segEndMs = keys[index].minute * 60000u;
        if (current < segEndMs)
        {
            const uint32_t segStartMs = keys[index - 1].minute * 60000u;
            VitaDayNightLight light = keys[index - 1].light;
            float blend = (float)(current - segStartMs) / (float)(segEndMs - segStartMs);

            // Hermite cubic smoothing
            blend = blend * blend * (3.0f - 2.0f * blend);
            for (size_t ch = 0; ch < 3; ++ch)
            {
                light.rgb[ch] += (keys[index].light.rgb[ch] - light.rgb[ch]) * blend;
            }
            light.daylight += (keys[index].light.daylight - light.daylight) * blend;
            light.glow += (keys[index].light.glow - light.glow) * blend;
            return light;
        }
    }
    return keys[0].light;
}

VitaDayNightBlend VitaDayNight_Moonlight(const VitaDayNightLight *const light, const float fade,
                                         const float fadeRgb[3])
{
    VitaDayNightBlend blend = {{70.0f / 255.0f, 92.0f / 255.0f, 136.0f / 255.0f}, 0.0f};
    const float moon = 0.06f * light->glow * light->glow;
    const float remaining = moon * (1.0f - fade);

    blend.amount = remaining + fade;
    if (blend.amount > 0.0f)
    {
        for (size_t ch = 0; ch < 3; ++ch)
        {
            blend.rgb[ch] = (blend.rgb[ch] * remaining + fadeRgb[ch] * fade) / blend.amount;
        }
    }
    return blend;
}

static uint32_t SunKey(const uint32_t timeMs)
{
    const uint32_t minute = (timeMs % VITA_DAYNIGHT_DAY_MS) / 60000u;
    return (minute >= 240u && minute < 1200u) ? (minute / 15u + 2u) : 1u;
}

VitaDayNightSun VitaDayNight_SunAt(const uint32_t timeMs)
{
    const uint32_t key = SunKey(timeMs);
    VitaDayNightSun sun = {0.0f, 0.55f, 0.0f, key};

    if (key != 1u)
    {
        const float progress = ((float)((key - 2u) * 15u) - 240.0f) / 960.0f;
        const float altitude = sinf(progress * 3.141592654f);

        sun.dx = -1.35f * cosf(progress * 3.141592654f);
        sun.dz = 0.35f + 0.20f * altitude;
        sun.strength = fminf(1.0f, altitude * 2.0f);
    }
    return sun;
}

void VitaDayNight_Resume(void)
{
    sResume = true;
    sSampled = false;
}

void VitaDayNight_Update(void)
{
    const uint64_t now = VitaPlatform_GetMilliseconds();
    const bool enabled = (VitaExtras_GetFast(FAST_EXTRA_DAY_NIGHT) == 1);
    const int32_t speed = VitaExtras_GetFast(FAST_EXTRA_CLOCK_SPEED);

    sLight = (VitaDayNightLight){{1.0f, 1.0f, 1.0f}, 1.0f, 0.0f};
    sSun = (VitaDayNightSun){0.85f, 0.55f, 1.0f, 0};

    if (!sSampled || !sEnabled || now < sSampleMs || (now - sSampleMs) >= 1000)
    {
        const int64_t wall = VitaPlatform_GetWallSeconds();

        if (!VitaGame_LocalTimeSeconds(&sLocalSeconds))
        {
            sSampled = false;
            sResume = true;
            return;
        }

        if (sResume && sClock.ready && sClock.shifted && sWallSeconds > 0 && wall > sWallSeconds)
        {
            sClock.timeMs = (uint32_t)((sClock.timeMs
                         + (uint64_t)(wall - sWallSeconds) % 86400u * 1000u) % VITA_DAYNIGHT_DAY_MS);
        }
        sWallSeconds = wall;
        sSampleMs = now;
        sSampled = true;
    }

    if (sResume)
    {
        sClock.lastMs = now;
    }
    sResume = false;

    VitaDayNight_Advance(&sClock, now, sLocalSeconds, speed);
    sEnabled = true;
    sValid = true;

    if (enabled && VitaGame_IsOutdoor())
    {
        sLight = VitaDayNight_At(sClock.timeMs);
        sSun = VitaDayNight_SunAt(sClock.timeMs);
    }
}

bool VitaDayNight_Time(uint32_t *const seconds, uint32_t *const rate)
{
    VitaDayNight_Update();
    if (!sValid)
    {
        return false;
    }
    *seconds = sClock.timeMs / 1000u;
    *rate = VitaDayNight_Speed(VitaExtras_GetFast(FAST_EXTRA_CLOCK_SPEED));
    return true;
}

bool VitaDayNight_IsShifted(void)
{
    return sClock.shifted;
}

bool VitaDayNight_Active(void)
{
    VitaDayNight_Update();
    return sValid;
}

const VitaDayNightLight *VitaDayNight_Get(void)
{
    return &sLight;
}

const VitaDayNightSun *VitaDayNight_GetSun(void)
{
    return &sSun;
}

bool VitaDayNight_BerryDue(void)
{
    VitaDayNight_Update();
    return sValid && (sClock.berryBonusMs >= 60000u);
}

uint32_t VitaDayNight_TakeBerryMinutes(void)
{
    VitaDayNight_Update();
    if (!sValid)
    {
        return 0;
    }
    const uint32_t minutes = (uint32_t)(sClock.berryBonusMs / 60000u);
    sClock.berryBonusMs %= 60000u;
    return minutes;
}

void VitaDayNight_Reset(void)
{
    sClock = (VitaDayNightClock){0};
    sSampled = sEnabled = sResume = sValid = false;
    sWallSeconds = 0;
    sLight = (VitaDayNightLight){{1.0f, 1.0f, 1.0f}, 1.0f, 0.0f};
    sSun = (VitaDayNightSun){0.85f, 0.55f, 1.0f, 0};
}

bool VitaDayNight_SyncToConsole(void)
{
    const uint64_t now = VitaPlatform_GetMilliseconds();
    const int32_t speed = VitaExtras_Get("clock_speed");
    uint32_t local = 0;

    if (!VitaGame_LocalTimeSeconds(&local))
    {
        return false;
    }

    if (sResume)
    {
        sClock.ready = false;
    }
    if (sClock.ready)
    {
        VitaDayNight_Advance(&sClock, now, local, speed);
    }

    VitaExtras_Set("clock_speed", 0);
    VitaDayNight_Sync(&sClock, now, local, 0);
    sLocalSeconds = local;
    sSampleMs = now;
    sSampled = true;
    sResume = false;
    sWallSeconds = VitaPlatform_GetWallSeconds();

    VitaDayNight_Update();
    return true;
}

bool VitaDayNight_Snapshot(VitaDayNightSnapshot *const snapshot)
{
    VitaDayNight_Update();
    if (!sValid || snapshot == NULL)
    {
        return false;
    }
    *snapshot = (VitaDayNightSnapshot){sClock.timeMs, sClock.berryBonusMs,
                                       VitaPlatform_GetWallSeconds(), sClock.shifted};
    return snapshot->wallSeconds > 0;
}

bool VitaDayNight_Restore(const VitaDayNightSnapshot *const snapshot)
{
    const int64_t wall = VitaPlatform_GetWallSeconds();

    if (snapshot == NULL || snapshot->timeMs >= VITA_DAYNIGHT_DAY_MS
     || snapshot->berryBonusMs > VITA_DAYNIGHT_BONUS_LIMIT_MS
     || snapshot->wallSeconds <= 0 || wall <= 0)
    {
        return false;
    }

    VitaDayNight_Reset();
    sClock.timeMs = snapshot->timeMs;
    if (snapshot->shifted && wall > snapshot->wallSeconds)
    {
        sClock.timeMs = (uint32_t)((sClock.timeMs
                     + (uint64_t)(wall - snapshot->wallSeconds) % 86400u * 1000u) % VITA_DAYNIGHT_DAY_MS);
    }
    sClock.berryBonusMs = snapshot->berryBonusMs;
    sClock.shifted = snapshot->shifted;
    sClock.ready = true;
    sClock.lastMs = VitaPlatform_GetMilliseconds();
    sWallSeconds = wall;
    sResume = true;
    return true;
}

bool VitaGame_IsOutdoor(void)
{
    if (gMapHeader.mapLayout == NULL)
    {
        return false;
    }

    switch (gMapHeader.mapType)
    {
    case MAP_TYPE_TOWN:
    case MAP_TYPE_CITY:
    case MAP_TYPE_ROUTE:
    case MAP_TYPE_OCEAN_ROUTE:
        return true;
    default:
        return false;
    }
}

bool VitaGame_IsOverworld(void)
{
    if (gMapHeader.mapLayout == NULL || gSaveBlock1Ptr == NULL)
    {
        return false;
    }
    return (gMain.callback2 == CB2_Overworld || gMain.callback2 == CB2_OverworldBasic);
}

bool VitaGame_LocalTimeSeconds(uint32_t *const seconds)
{
    if (seconds == NULL)
    {
        return false;
    }

    struct VitaLocalTime now;
    if (VitaPlatform_GetLocalTime(&now) == 0)
    {
        *seconds = (uint32_t)(now.hour % 24) * 3600u + (uint32_t)(now.minute % 60) * 60u + (uint32_t)(now.second % 60);
        return true;
    }

    return false;
}
