#ifndef GUARD_VITA_PLATFORM_H
#define GUARD_VITA_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define VITA_DATA_DIR "ux0:data/POKEVITA/"

struct VitaLocalTime
{
    int32_t year;      // e.g. 2026
    int32_t month;     // 1-12
    int32_t day;       // 1-31
    int32_t dayOfWeek; // 0 = Sunday (sceRtcGetDayOfWeek; checked against a known Friday = 5)
    int32_t hour;
    int32_t minute;
    int32_t second;
};

// Physical buttons, as VitaPlatform_ReadButtons reports them.
enum
{
    VITA_BTN_UP       = 1 << 0,
    VITA_BTN_DOWN     = 1 << 1,
    VITA_BTN_LEFT     = 1 << 2,
    VITA_BTN_RIGHT    = 1 << 3,
    VITA_BTN_CROSS    = 1 << 4,
    VITA_BTN_CIRCLE   = 1 << 5,
    VITA_BTN_SQUARE   = 1 << 6,
    VITA_BTN_TRIANGLE = 1 << 7,
    VITA_BTN_L        = 1 << 8,
    VITA_BTN_R        = 1 << 9,
    VITA_BTN_START    = 1 << 10,
    VITA_BTN_SELECT   = 1 << 11,
    // Right stick pushed in a direction: buttons for the remapper only (never the d-pad)
    VITA_BTN_RS_UP    = 1 << 12,
    VITA_BTN_RS_DOWN  = 1 << 13,
    VITA_BTN_RS_LEFT  = 1 << 14,
    VITA_BTN_RS_RIGHT = 1 << 15,
};
#define VITA_BTN_UNBOUND 0xFF   // remap setting of an action with no button

// VITA_BTN_* bits for the buttons held right now; the left stick counts as the d-pad. The port
// menu maps them to GBA keys (VitaMenu_MapButtons).
uint32_t VitaPlatform_ReadButtons(void);

// Reads front touch panel position in screen coordinates (960x544).
// Returns true if touched, false otherwise.
bool VitaPlatform_ReadTouch(int32_t *outX, int32_t *outY);

// Reads front touch panel mapped to GBA coordinates (240x160).
// Returns true if touched within GBA screen area, false otherwise.
bool VitaPlatform_ReadTouchGBA(int32_t *outGbaX, int32_t *outGbaY);

// Local time as configured on the console. Returns 0 on success.
int32_t VitaPlatform_GetLocalTime(struct VitaLocalTime *out);

// Milliseconds elapsed since process launch (from sceKernelGetProcessTimeWide).
uint64_t VitaPlatform_GetMilliseconds(void);

// Wall clock in seconds (from sceRtcGetCurrentTick).
int64_t VitaPlatform_GetWallSeconds(void);

// Pins the calling thread to one of the three user cores (0-2).
void VitaPlatform_PinCurrentThread(const char *name, int32_t core);

// Yields the calling thread to the Vita kernel scheduler for `microseconds`.
void VitaPlatform_DelayMicroseconds(uint32_t microseconds);

// Current ARM clock, which plugins such as oclockvita may change at run time.
int32_t VitaPlatform_GetCpuMHz(void);

// Appends to ux0:data/POKEVITA/debug.log (readable on hardware and in Vita3K's ux0).
void VitaPlatform_Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Not keyed on NDEBUG: the decomp's include/config.h always defines it.
#ifdef VITA_LOGGING
#define VITA_LOG(...) VitaPlatform_Log(__VA_ARGS__)
#else
#define VITA_LOG(...) ((void)0)
#endif

// Creates ux0:data/POKEVITA/ and its subfolders. Safe to call when they already exist.
void VitaPlatform_Init(void);

// Ends the app (back to LiveArea).
void VitaPlatform_Exit(void) __attribute__((noreturn));

// Writes VITA_DATA_DIR + relative into dest.
void VitaPlatform_BuildPath(char *dest, size_t size, const char *relative);

// Loads a binary asset prioritizing app0:<relativePath> before ux0:data/POKEVITA/<relativePath>.
// Caller owns the returned heap buffer (free()). Sets *outSize on success, 0 on failure.
uint8_t *VitaPlatform_LoadAsset(const char *relativePath, size_t *outSize);

#endif // GUARD_VITA_PLATFORM_H
