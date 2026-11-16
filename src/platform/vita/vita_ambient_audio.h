#ifndef VITA_AMBIENT_AUDIO_H
#define VITA_AMBIENT_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

// Updates the current ambient audio target (called from Core 0 / game state; lock-free).
void VitaAmbientAudio_SetScene(bool indoor, bool underground, int32_t weatherClass,
                               float weatherAmount, float nightGlow);

// Mixes subtle procedural ambient layers (rain, wind, cave rumble, night crickets)
// into interleaved 48 kHz stereo float buffer on Core 1 (< 0.15 ms, zero libm in inner loop).
void VitaAmbientAudio_MixStereo(float *stereoBuffer, int32_t floatCount, float masterVolume);

#endif // VITA_AMBIENT_AUDIO_H
