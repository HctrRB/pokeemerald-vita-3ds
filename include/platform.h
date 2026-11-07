#ifndef GUARD_PLATFORM_H
#define GUARD_PLATFORM_H

#include "global.h"
#include "siirtc.h"

void Platform_StoreSaveFile(void);
void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size);
/**
 * @brief Feeds floating-point audio buffer into the platform audio queue.
 * @param audioBuffer Interleaved stereo float samples [-1.0f, 1.0f].
 * @param bufferSizeBytes Total size of incoming audio buffer in bytes (samples * sizeof(float)).
 */
void Platform_QueueAudio(float *audioBuffer, u32 bufferSizeBytes);
u16 Platform_GetKeyInput(void);

enum PlatformSetting
{
    PLATFORM_SETTING_FULLSCREEN,
    PLATFORM_SETTING_WINDOW_SCALE,
    PLATFORM_SETTING_INTEGER_SCALE,
    PLATFORM_SETTING_VSYNC,
    PLATFORM_SETTING_BORDER,
    PLATFORM_SETTING_VOLUME,
    PLATFORM_SETTING_WIDESCREEN, // Vita only
    PLATFORM_SETTING_FAST_FORWARD_SPEED,  // Vita: 2-5 (x normal speed)
    PLATFORM_SETTING_FAST_FORWARD_TOGGLE, // Vita: 0 = while held, 1 = a press toggles it
    PLATFORM_SETTING_SAVE_FILE,           // Vita: 1-3 (saves/pokeemerald.sav, pokeemerald_2.sav, ...)
    PLATFORM_SETTING_BUTTON_FIRST,        // Vita: the physical button (bit of VITA_BTN_*) of each
    PLATFORM_SETTING_BUTTON_LAST = PLATFORM_SETTING_BUTTON_FIRST + 7, // VITA_ACTION_*
    PLATFORM_SETTING_FILTER,              // Vita: 0 nearest, 1 bilinear, 2 sharp bilinear
    PLATFORM_SETTING_OVERLAY,             // Vita: 0 none, 1 scanlines, 2 LCD grid, 3 CRT mask
    PLATFORM_SETTING_AUTO_RUN,            // Vita: run without holding B (B walks)
    PLATFORM_SETTING_HM_SHORTCUTS,        // Vita: field HMs work with the badge and the HM in the bag
    PLATFORM_SETTING_STATS_INFO,          // Vita: nature colours and IV/EV pages in the summary
    PLATFORM_SETTING_EXP_SHARE,           // Vita: 0 = off, 1 = global exp share
    PLATFORM_SETTING_EXP_MULTIPLIER,      // Vita: 0 = 1x, 1 = 2x, 2 = 4x, 3 = 10x
    PLATFORM_SETTING_VOXEL_3D,            // Vita: 0 = off (2D), 1 = 2.5D hardware voxel overworld
    PLATFORM_SETTING_UI_SCALE,            // Vita: 0 = PIXEL PERFECT (640x480), 1 = FULLSCREEN (725x544)
    PLATFORM_SETTING_VOXEL_RES,           // Vita: 0 = 960x544 (Native), 1 = 720x408 (Reduced)
    PLATFORM_SETTING_VOXEL_FPS,           // Vita: 0 = 30 FPS (Locked), 1 = Libre (Up to 60)
    PLATFORM_SETTING_COUNT,
};

u8 Platform_GetSetting(enum PlatformSetting setting);
void Platform_SetSetting(enum PlatformSetting setting, u8 value);
bool8 Platform_IsVoxelOverworldActive(void);
int VitaPlatform_GetRenderWidth(void);
int VitaPlatform_GetRenderHeight(void);
void VitaPlatform_ApplyRenderResolution(void);
void VitaPlatform_ApplyTargetFramerate(void);
void Platform_GetStatus(struct SiiRtcInfo *rtc);
void Platform_SetStatus(struct SiiRtcInfo *rtc);
void Platform_GetDateTime(struct SiiRtcInfo *rtc);
void Platform_SetDateTime(struct SiiRtcInfo *rtc);
void Platform_GetTime(struct SiiRtcInfo *rtc);
void Platform_SetTime(struct SiiRtcInfo *rtc);
void Platform_SetAlarm(u8 *alarmData);

#endif
