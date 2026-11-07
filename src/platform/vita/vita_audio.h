#ifndef GUARD_VITA_AUDIO_H
#define GUARD_VITA_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#define VITA_AUDIO_SAMPLE_RATE 48000
#define VITA_AUDIO_CHANNELS    2
#define VITA_AUDIO_SAMPLES_PER_FRAME (VITA_AUDIO_SAMPLE_RATE / 60) // 800 samples/frame at 60Hz

/**
 * @brief Initializes the PS Vita hardware audio port (SceAudioOut).
 * @return True on success, false on error.
 */
bool VitaAudio_Init(void);

/**
 * @brief Terminates the audio port and releases audio resources.
 */
void VitaAudio_Terminate(void);

/**
 * @brief Sets master volume level (0 to 10).
 * @param volume Volume level from 0 (mute) to 10 (max).
 */
void VitaAudio_SetVolume(uint32_t volume);

/**
 * @brief Feeds floating-point audio samples into the Vita audio pipeline.
 * Converts to signed 16-bit PCM, applies a DC-blocking filter, and outputs via SceAudioOut.
 * 
 * @param audioBuffer Interleaved stereo float samples [-1.0f, 1.0f].
 * @param byteCount Size of the incoming buffer in bytes.
 */
void VitaAudio_QueueAudio(const float *const audioBuffer, uint32_t byteCount);

#endif // GUARD_VITA_AUDIO_H
