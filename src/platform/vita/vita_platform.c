#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/cpu.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/rtc.h>

#include "vita_platform.h"
#include "vita_perf.h"
#include "vita_audio.h"
#include "vita_input.h"
#include "platform/vita_extras.h"
#include "gba/io_reg.h"

// 64 MB newlib heap: right-sized above peak dynamic usage (~38 MB) to avoid hogging physical RAM.
int32_t _newlib_heap_size_user = 64 * 1024 * 1024;

#define VITA_LOG_PATH VITA_DATA_DIR "debug.log"

#ifdef VITA_LOGGING
static FILE *sLogFile = NULL;
static char sLogBuffer[32768];
#endif

void VitaPlatform_Log(const char *const fmt, ...)
{
#ifdef VITA_LOGGING
    if (sLogFile == NULL)
    {
        sLogFile = fopen(VITA_LOG_PATH, "a");
        if (sLogFile == NULL)
        {
            return;
        }
        setvbuf(sLogFile, sLogBuffer, _IOFBF, sizeof(sLogBuffer));
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(sLogFile, fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

void VitaPlatform_Init(void)
{
    // Initialize directory structure in ux0:data/POKEVITA
    const int32_t mkdirRoot = sceIoMkdir("ux0:data/POKEVITA", 0777);
    const int32_t mkdirSaves = sceIoMkdir("ux0:data/POKEVITA/saves", 0777);
    (void)mkdirRoot;
    (void)mkdirSaves;
#ifdef VITA_LOGGING
    remove(VITA_LOG_PATH);
#endif

    // Overclock to 444 MHz CPU / 222 MHz GPU for 60 FPS in 3D diorama/voxel mode
    const int32_t setArmResult = scePowerSetArmClockFrequency(444);
    const int32_t setBusResult = scePowerSetBusClockFrequency(222);
    const int32_t setGpuResult = scePowerSetGpuClockFrequency(222);
    const int32_t setXbarResult = scePowerSetGpuXbarClockFrequency(166);

    if (setArmResult < 0 || setBusResult < 0 || setGpuResult < 0 || setXbarResult < 0)
    {
        VITA_LOG("[POWER] Clock configuration returned status: ARM=%d BUS=%d GPU=%d XBAR=%d\n",
                 setArmResult, setBusResult, setGpuResult, setXbarResult);
    }

    VITA_LOG("[POWER] Clocks: CPU %d MHz, Bus %d MHz, GPU %d MHz, Xbar %d MHz\n",
             scePowerGetArmClockFrequency(), scePowerGetBusClockFrequency(),
             scePowerGetGpuClockFrequency(), scePowerGetGpuXbarClockFrequency());

    // Initialize native hardware input and touch subsystems
    if (!VitaInput_Init())
    {
        VITA_LOG("[INIT] VitaInput_Init reported failure!\n");
    }

    // Initialize native hardware audio subsystem (SceAudioOut)
    if (!VitaAudio_Init())
    {
        VITA_LOG("[INIT] VitaAudio_Init reported failure!\n");
    }

    struct VitaLocalTime now;
    if (VitaPlatform_GetLocalTime(&now) == 0)
    {
        VITA_LOG("[RTC] Local time: %04d-%02d-%02d %02d:%02d:%02d (Day of week: %d)\n",
                 now.year, now.month, now.day, now.hour, now.minute, now.second, now.dayOfWeek);
    }

    VitaExtras_Init();
    VITA_LOG("[INIT] Vita platform initialized successfully. Data dir: %s\n", VITA_DATA_DIR);
}

void VitaPlatform_Exit(void)
{
#ifdef VITA_LOGGING
    if (sLogFile != NULL)
    {
        fflush(sLogFile);
        fclose(sLogFile);
        sLogFile = NULL;
    }
#endif
    VitaPerf_Shutdown();
    VitaAudio_Terminate();
    sceKernelExitProcess(0);
    for (;;)
    {
        // Unreachable
    }
}

void VitaPlatform_BuildPath(char *const dest, const size_t size, const char *const relative)
{
    if (dest == NULL || relative == NULL || size == 0U)
    {
        return;
    }
    snprintf(dest, size, "%s%s", VITA_DATA_DIR, relative);
}

uint8_t *VitaPlatform_LoadAsset(const char *const relativePath, size_t *const outSize)
{
    if (outSize != NULL)
    {
        *outSize = 0U;
    }
    if (relativePath == NULL || relativePath[0] == '\0')
    {
        return NULL;
    }

    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "app0:%s", relativePath);
    FILE *file = fopen(fullPath, "rb");
    if (file == NULL)
    {
        snprintf(fullPath, sizeof(fullPath), "%s%s", VITA_DATA_DIR, relativePath);
        file = fopen(fullPath, "rb");
    }
    if (file == NULL)
    {
        snprintf(fullPath, sizeof(fullPath), "app0:assets/%s", relativePath);
        file = fopen(fullPath, "rb");
    }
    if (file == NULL)
    {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }
    const long rawSize = ftell(file);
    if (rawSize <= 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    const size_t size = (size_t)rawSize;
    uint8_t *const buf = (uint8_t *)malloc(size);
    if (buf == NULL)
    {
        fclose(file);
        return NULL;
    }

    const size_t readBytes = fread(buf, 1U, size, file);
    fclose(file);
    if (readBytes != size)
    {
        free(buf);
        return NULL;
    }

    if (outSize != NULL)
    {
        *outSize = size;
    }
    return buf;
}

uint32_t VitaPlatform_ReadButtons(void)
{
    VitaInput_Poll(NULL);
    return VitaInput_GetButtonsHeld();
}

void VitaPlatform_PinCurrentThread(const char *const name, const int32_t core)
{
    const SceUID thid = sceKernelGetThreadId();
    const int32_t before = sceKernelGetThreadCpuAffinityMask(thid);
    const int32_t ret = sceKernelChangeThreadCpuAffinityMask(thid, SCE_KERNEL_CPU_MASK_USER_0 << core);

    VITA_LOG("[THREAD] %s: affinity 0x%x -> core %d (ret 0x%x)\n", name, before, core, ret);
    (void)before; // only read by VITA_LOG, which may be compiled out
    (void)ret;
}

void VitaPlatform_DelayMicroseconds(const uint32_t microseconds)
{
    if (microseconds == 0U)
    {
        return;
    }
    const int32_t ret = sceKernelDelayThread(microseconds);
    if (ret < 0)
    {
        VITA_LOG("[THREAD] sceKernelDelayThread(%u) failed: 0x%x\n", microseconds, ret);
    }
}

int32_t VitaPlatform_GetCpuMHz(void)
{
    return scePowerGetArmClockFrequency();
}

int32_t VitaPlatform_GetLocalTime(struct VitaLocalTime *const out)
{
    if (out == NULL)
    {
        return -1;
    }

    SceDateTime now;
    const int32_t ret = sceRtcGetCurrentClockLocalTime(&now);
    if (ret < 0)
    {
        return ret;
    }

    out->year      = now.year;
    out->month     = now.month;
    out->day       = now.day;
    out->dayOfWeek = sceRtcGetDayOfWeek(now.year, now.month, now.day);
    out->hour      = now.hour;
    out->minute    = now.minute;
    out->second    = now.second;
    return 0;
}

bool VitaPlatform_ReadTouch(int32_t *const outX, int32_t *const outY)
{
    return VitaInput_ReadTouchScreen(outX, outY);
}

bool VitaPlatform_ReadTouchGBA(int32_t *const outGbaX, int32_t *const outGbaY)
{
    return VitaInput_ReadTouchGBA(outGbaX, outGbaY);
}

uint64_t VitaPlatform_GetMilliseconds(void)
{
    return (uint64_t)sceKernelGetProcessTimeWide() / 1000ULL;
}

int64_t VitaPlatform_GetWallSeconds(void)
{
    SceRtcTick tick;
    if (sceRtcGetCurrentTick(&tick) < 0)
    {
        return 0;
    }
    return (int64_t)(tick.tick / 1000000ULL);
}
