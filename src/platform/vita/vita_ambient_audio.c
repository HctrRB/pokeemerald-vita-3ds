#include "vita_ambient_audio.h"

// Lock-free packed scene state written by Core 0, read by Core 1
static volatile uint32_t sPackedTarget = 0;

void VitaAmbientAudio_SetScene(const bool indoor, const bool underground, const int32_t weatherClass,
                               const float weatherAmount, const float nightGlow)
{
    uint32_t wAmt = (uint32_t)(weatherAmount * 255.0f + 0.5f);
    if (wAmt > 255u) wAmt = 255u;
    uint32_t nGlow = (uint32_t)(nightGlow * 255.0f + 0.5f);
    if (nGlow > 255u) nGlow = 255u;
    uint32_t wCls = (uint32_t)(weatherClass & 0xFu);
    uint32_t flags = (indoor ? 1u : 0u) | (underground ? 2u : 0u);
    sPackedTarget = flags | (wCls << 2) | (wAmt << 8) | (nGlow << 16);
}

static inline float FastRandSigned(uint32_t *const state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return (float)((int32_t)(x >> 16) - 32768) * (1.0f / 32768.0f);
}

void VitaAmbientAudio_MixStereo(float *const stereoBuffer, const int32_t floatCount, const float masterVolume)
{
    if (!stereoBuffer || floatCount < 2 || masterVolume <= 0.001f)
        return;

    const uint32_t packed = sPackedTarget;
    const bool indoor = (packed & 1u) != 0u;
    const bool underground = (packed & 2u) != 0u;
    const int32_t weatherClass = (int32_t)((packed >> 2) & 0xFu);
    const float weatherAmount = (float)((packed >> 8) & 0xFFu) * (1.0f / 255.0f);
    const float nightGlow = (float)((packed >> 16) & 0xFFu) * (1.0f / 255.0f);

    // Target gains (kept subtle so GBA BGM stays front-and-center)
    // weatherClass: 2 = VOXEL_WEATHER_RAIN, 3 = VOXEL_WEATHER_SAND, 4 = VOXEL_WEATHER_ASH
    const float targetRain = (!indoor && weatherClass == 2) ? (weatherAmount * 0.045f) : (indoor && weatherClass == 2 ? weatherAmount * 0.014f : 0.0f);
    const float targetWind = (!indoor && (weatherClass == 3 || weatherClass == 2)) ? (0.018f + weatherAmount * 0.020f) : (!indoor ? 0.006f : 0.0f);
    const float targetCave = underground ? 0.025f : 0.0f;
    const float targetCrickets = (!indoor && !underground && weatherClass != 2 && weatherClass != 3) ? (nightGlow * 0.012f) : 0.0f;

    static float sRainGain = 0.0f, sWindGain = 0.0f, sCaveGain = 0.0f, sCricketGain = 0.0f;
    static uint32_t sRng = 0x5A17C0DEu;
    static float sRainLpL = 0.0f, sRainLpR = 0.0f;
    static float sWindLp = 0.0f, sCaveLp = 0.0f;
    // Quadrature recurrence oscillator for crickets (~4.2 kHz @ 48 kHz: w = 0.54978 rad, cos=0.8526, sin=0.5225)
    static float sOscC = 1.0f, sOscS = 0.0f;
    static uint32_t sSampleCounter = 0;
    static float sWindLfo = 0.5f, sCricketEnv = 0.0f;

    const float smooth = 0.002f;
    const int32_t frames = floatCount >> 1;

    for (int32_t i = 0; i < frames; ++i)
    {
        if ((sSampleCounter & 63u) == 0u)
        {
            sRainGain    += (targetRain - sRainGain) * 0.08f;
            sWindGain    += (targetWind - sWindGain) * 0.08f;
            sCaveGain    += (targetCave - sCaveGain) * 0.08f;
            sCricketGain += (targetCrickets - sCricketGain) * 0.08f;

            // Slow wind gust & cricket chirp pulse (updated at 750 Hz, zero libm)
            const uint32_t t = sSampleCounter >> 6;
            sWindLfo = 0.55f + 0.45f * (float)((t & 511u) < 256u ? (t & 255u) : (255u - (t & 255u))) * (1.0f / 255.0f);
            const uint32_t chirpPhase = t % 1500u;
            const bool chirpOn = (chirpPhase < 180u) && ((chirpPhase % 45u) < 22u);
            sCricketEnv += ((chirpOn ? 1.0f : 0.0f) - sCricketEnv) * 0.35f;

            // Re-normalize quadrature oscillator periodically to prevent drift
            const float mag = sOscC * sOscC + sOscS * sOscS;
            if (mag > 1.05f || mag < 0.95f)
            {
                sOscC = 1.0f;
                sOscS = 0.0f;
            }
        }
        sSampleCounter++;

        if (sRainGain < 0.0002f && sWindGain < 0.0002f && sCaveGain < 0.0002f && sCricketGain < 0.0002f)
            continue;

        const float n1 = FastRandSigned(&sRng);
        const float n2 = FastRandSigned(&sRng);
        (void)smooth;

        // Rain: soft stereo pink-ish noise
        sRainLpL += (n1 - sRainLpL) * 0.22f;
        sRainLpR += (n2 - sRainLpR) * 0.22f;

        // Wind: low-pass modulated noise
        sWindLp += (n1 - sWindLp) * 0.035f;

        // Cave: deep rumble
        sCaveLp += (n1 - sCaveLp) * 0.012f;

        // Cricket: recurrence oscillator
        const float nextC = sOscC * 0.85264f - sOscS * 0.52250f;
        const float nextS = sOscS * 0.85264f + sOscC * 0.52250f;
        sOscC = nextC;
        sOscS = nextS;

        const float common = sWindLp * (sWindGain * sWindLfo)
                           + sCaveLp * sCaveGain
                           + nextS * (sCricketGain * sCricketEnv);

        const float addL = (sRainLpL * sRainGain + common) * masterVolume;
        const float addR = (sRainLpR * sRainGain + common) * masterVolume;

        stereoBuffer[i * 2 + 0] += addL;
        stereoBuffer[i * 2 + 1] += addR;
    }
}
