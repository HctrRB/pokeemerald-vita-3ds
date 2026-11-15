#include "vita_audio.h"
#include "vita_platform.h"

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
    (void)volume;
}
