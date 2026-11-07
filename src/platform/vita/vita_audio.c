#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <psp2/audioout.h>

#include "vita_audio.h"
#include "vita_platform.h"

// Master volume multiplier (0.0f to 1.0f)
static float sMasterVolume = 1.0f;

bool VitaAudio_Init(void)
{
    VITA_LOG("[AUDIO] VitaAudio_Init: audio handled via SDL2 48 kHz stream.\n");
    return true;
}

void VitaAudio_Terminate(void)
{
}

void VitaAudio_SetVolume(const uint32_t volume)
{
    const uint32_t clampedVol = (volume > 10U) ? 10U : volume;
    sMasterVolume = (float)clampedVol / 10.0f;
    (void)sMasterVolume;
}

void VitaAudio_QueueAudio(const float *const audioBuffer, const uint32_t byteCount)
{
    (void)audioBuffer;
    (void)byteCount;
}
