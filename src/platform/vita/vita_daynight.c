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
#include "platform/game_frame.h"

static const uint32_t sSpeeds[VITA_DAYNIGHT_SPEED_COUNT] = {1, 6, 12, 24, 60, 120, 1000};
static VitaDayNightClock sClock;
// Built on Core 0 only (VitaDayNight_UpdateFrame); the clock above is also advanced from Core 1.
static VitaDayNightState sState = {
    .timeMs = 13u * 3600u * 1000u,
    .light = {{1.0f, 1.0f, 1.0f}, 1.0f, 0.0f},
    .sun = {0.85f, 0.55f, 1.0f, 0},
    .day = 1.0f,
};
// Without the cycle (option off, indoors): early afternoon, the sun past its highest.
#define VITA_DAYNIGHT_FIXED_MS (13u * 3600u * 1000u)
#define PI_F 3.141592654f
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
    // rgb tint, daylight, glow (windows and lamps) at each point of the schedule; Hermite between.
    static const struct
    {
        uint32_t minute;
        VitaDayNightLight light;
    } keys[] = {
        {0,                            {{0.66f, 0.74f, 0.92f}, 0.00f, 1.00f}}, // midnight blue
        {VITA_DAYNIGHT_DAWN_MIN,       {{0.66f, 0.74f, 0.92f}, 0.00f, 1.00f}}, // night until dawn
        {VITA_DAYNIGHT_SUNRISE_MIN,    {{0.96f, 0.86f, 0.80f}, 0.40f, 0.30f}}, // sunrise: rose gold
        {VITA_DAYNIGHT_MORNING_MIN,    {{0.96f, 0.94f, 0.88f}, 0.75f, 0.00f}}, // morning; lamps out
        {VITA_DAYNIGHT_NOON_START_MIN, {{1.00f, 1.00f, 1.00f}, 1.00f, 0.00f}}, // noon
        {VITA_DAYNIGHT_NOON_END_MIN,   {{1.00f, 1.00f, 1.00f}, 1.00f, 0.00f}}, // early afternoon
        {VITA_DAYNIGHT_DUSK_MIN,       {{1.00f, 0.93f, 0.84f}, 0.85f, 0.00f}}, // evening begins
        {VITA_DAYNIGHT_SUNSET_MIN,     {{1.00f, 0.80f, 0.65f}, 0.35f, 0.30f}}, // sunset orange
        {VITA_DAYNIGHT_NIGHT_MIN,      {{0.66f, 0.74f, 0.92f}, 0.00f, 1.00f}}, // night
        {24u * 60u,                    {{0.66f, 0.74f, 0.92f}, 0.00f, 1.00f}}, // midnight
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

// The sun's angle along its arc, 0 at sunrise, pi at nightfall, or a negative value when it is
// down. It climbs to its highest by NOON_START, stays there (a flat top around pi / 2) until
// NOON_END and sets by NIGHT: one arc for the shadows (SunAt) and the screen (SunPathAt).
static float SunAngle(const float minute)
{
    const float rise = (float)VITA_DAYNIGHT_SUNRISE_MIN;
    const float noonStart = (float)VITA_DAYNIGHT_NOON_START_MIN;
    const float noonEnd = (float)VITA_DAYNIGHT_NOON_END_MIN;
    const float set = (float)VITA_DAYNIGHT_NIGHT_MIN;

    if (minute < rise || minute > set)
        return -1.0f;
    if (minute < noonStart)
        return PI_F * 0.45f * (minute - rise) / (noonStart - rise);
    if (minute < noonEnd)
        return PI_F * (0.45f + 0.10f * (minute - noonStart) / (noonEnd - noonStart));
    return PI_F * (0.55f + 0.45f * (minute - noonEnd) / (set - noonEnd));
}

// Shadows change in 15-minute steps: each step relights the visible chunks.
static uint32_t SunKey(const uint32_t timeMs)
{
    const uint32_t minute = (timeMs % VITA_DAYNIGHT_DAY_MS) / 60000u;
    return (minute >= VITA_DAYNIGHT_SUNRISE_MIN && minute < VITA_DAYNIGHT_NIGHT_MIN) ? (minute / 15u + 2u) : 1u;
}

VitaDayNightSun VitaDayNight_SunAt(const uint32_t timeMs)
{
    const uint32_t key = SunKey(timeMs);
    VitaDayNightSun sun = {0.0f, 0.55f, 0.0f, key};

    if (key != 1u)
    {
        const float angle = SunAngle((float)((key - 2u) * 15u));
        const float altitude = (angle >= 0.0f) ? sinf(angle) : 0.0f;

        sun.dx = -1.35f * cosf(angle);
        sun.dz = 0.35f + 0.20f * altitude;
        sun.strength = fminf(1.0f, altitude * 2.0f);
    }
    return sun;
}

VitaDayNightPath VitaDayNight_SunPathAt(const uint32_t timeMs)
{
    // The same arc as the shadows, continuous: shafts, sky bloom and water glints. Never fed to
    // VoxelLighting_SetSun, so chunks never rebuild from smooth sun motion.
    const float minF = (float)(timeMs % VITA_DAYNIGHT_DAY_MS) / 60000.0f;
    VitaDayNightPath path = {0.0f, 0.0f, 0.45f, 0.0f, 0.0f};
    const float angle = SunAngle(minF);
    if (angle >= 0.0f)
    {
        const float altitude = sinf(angle);
        path.dx = -1.25f * cosf(angle);
        path.dy = 0.22f + 0.78f * altitude;
        path.dz = -0.35f - 0.15f * altitude;
        path.altitude = altitude;
        path.strength = fminf(1.0f, altitude * 2.2f);
    }
    return path;
}

VitaDayNightPath VitaDayNight_MoonPathAt(const uint32_t timeMs)
{
    // The moon's arc: it rises at nightfall and sets at sunrise.
    const float night = (float)VITA_DAYNIGHT_NIGHT_MIN;
    const float rise = (float)VITA_DAYNIGHT_SUNRISE_MIN;
    float minF = (float)(timeMs % VITA_DAYNIGHT_DAY_MS) / 60000.0f;
    VitaDayNightPath path = {0.0f, 0.0f, 0.45f, 0.0f, 0.0f};
    if (minF >= night || minF <= rise)
    {
        if (minF < night)
            minF += 1440.0f;
        const float progress = (minF - night) / (1440.0f - night + rise);
        const float angle = progress * 3.141592654f;
        const float altitude = sinf(angle);
        path.dx = -1.10f * cosf(angle);
        path.dy = 0.25f + 0.70f * altitude;
        path.dz = -0.40f - 0.10f * altitude;
        path.altitude = altitude;
        path.strength = fminf(1.0f, altitude * 2.0f);
    }
    return path;
}

uint32_t VitaDayNight_GetTimeMs(void)
{
    return sClock.ready ? sClock.timeMs : (12u * 3600u * 1000u);
}

void VitaDayNight_Resume(void)
{
    sResume = true;
    sSampled = false;
}

void VitaDayNight_Update(void)
{
    const uint64_t now = VitaPlatform_GetMilliseconds();
    const int32_t speed = VitaExtras_GetFast(FAST_EXTRA_CLOCK_SPEED);

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
}

static float Clamp01(const float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

void VitaDayNight_UpdateFrame(void)
{
    VitaDayNight_Update();
    const bool active = sValid && VitaExtras_GetFast(FAST_EXTRA_DAY_NIGHT) == 1 && VitaGame_IsOutdoor();
    // One read of the clock, which Core 1 may also be advancing.
    const uint32_t timeMs = active ? __atomic_load_n(&sClock.timeMs, __ATOMIC_RELAXED) : VITA_DAYNIGHT_FIXED_MS;
    VitaDayNightState st;

    st.timeMs = timeMs;
    st.active = active;
    if (active)
    {
        st.light = VitaDayNight_At(timeMs);
        st.sun = VitaDayNight_SunAt(timeMs);
    }
    else
    {
        st.light = (VitaDayNightLight){{1.0f, 1.0f, 1.0f}, 1.0f, 0.0f};
        st.sun = (VitaDayNightSun){0.85f, 0.55f, 1.0f, 0};
    }
    st.moonlight = VitaDayNight_Moonlight(&st.light, 0.0f, st.light.rgb);
    st.sunPath = VitaDayNight_SunPathAt(timeMs);
    st.moonPath = VitaDayNight_MoonPathAt(timeMs);
    // The moon's shafts need the dark: none until the daylight has nearly gone.
    st.moonShafts = Clamp01(1.0f - st.light.daylight * 2.5f);

    // The grade's weights: dusk while the lamps come on and some daylight is left, night as it goes.
    const float dusk = Clamp01(st.light.glow * 4.0f) * Clamp01(st.light.daylight / 0.35f);
    const float night = Clamp01(1.0f - st.light.daylight / 0.35f);
    const float day = Clamp01(1.0f - dusk - night);
    const float sum = day + dusk + night;
    st.day = day / sum;
    st.dusk = dusk / sum;
    st.night = night / sum;

    sState = st;
}

const VitaDayNightState *VitaDayNight_State(void)
{
    return &sState;
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
    return &sState.light;
}

const VitaDayNightSun *VitaDayNight_GetSun(void)
{
    return &sState.sun;
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
    // The frame state is Core 0's: it follows the reset clock on its next frame.
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

/* Core 0: as the pinned game frame has it (VoxelWorld_CaptureField). */
bool VitaGame_IsOverworld(void)
{
    return GameFrame_Get()->field.overworldCallback;
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
