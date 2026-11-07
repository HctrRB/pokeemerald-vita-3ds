#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "mods/mod_manager.h"
#include <setjmp.h>
#include "vita/vita_perf.h"
#include "vita/vita_platform.h"
#include "platform/vita_ui.h"
#include "platform/ui.h"
#include "constants/moves.h"
#include "vita/vita_reset.h"
// Catch-up game frames run their scanline events without being drawn (see the main loop).
#define VITA_SKIP_CATCHUP_DRAW
#if defined(VITA_PARALLEL_DRAW)
// The draw workers write each frame straight into a texture in GPU memory, while the GPU may
// still be drawing the previous frame from its texture. Writing into that same texture mixed
// lines of two frames (visible shaking when the screen scrolls), so frames rotate through
// several textures and a texture is only rewritten once the GPU is done with it.
#define VITA_TEXTURE_RING 3
static SDL_Texture *sFrameTextures[VITA_TEXTURE_RING];
static int32_t sFrameTextureIndex;
#endif

#include "global.h"
#include "main.h"
#include "option_menu.h"
#include "battle.h"
#include "platform.h"
#include "rtc.h"
#include "gba/defines.h"
#include "gba/m4a_internal.h"
#include "cgb_audio.h"
#include "gba/flash_internal.h"
#include "platform/dma.h"
#include "platform/framedraw.h"
#include "platform/voxel_snapshot.h"


#ifdef PLATFORM_VITA
#include <vitaGL.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include "vita/vita_audio.h"
#include "vita/vita_stage.h"
#else
#include <GL/gl.h>
#endif

#ifdef VOXEL_SUPPORTED
#include "voxel/voxel_renderer.h"
#include "voxel/voxel_world.h"
#include "voxel/voxel_battle.h"
#endif

#include "platform/vita_daynight.h"
#include "platform/vita_extras.h"
#include "field_effect.h"
#include "constants/field_effects.h"

SDL_Thread *mainLoopThread;
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
#define VITA_GAME_TEXTURE_RING 2
static GLuint sGameTextureRing[VITA_GAME_TEXTURE_RING] = {0, 0};
static uint8_t sGameTextureUploadIdx = 0;
#define VITA_AUDIO_RATE 48000
static SDL_AudioStream *sAudioStream;

// Per-frame timing for hardware profiling.
// Main thread: draw = DrawFrame + texture upload, render = SDL copies, vblank = DMAs and the
// VBlank interrupt (includes audio mixing), present = SDL_RenderPresent (includes vsync wait).
// Game thread: logic = from being released after a VBlank to the next VBlankIntrWait.
// draw is split into frame = DrawFrame (the renderer) and upload = SDL_UpdateTexture.
// "shown" counts SDL_RenderPresent calls per second: what is actually displayed, which is
// lower than fps when one loop iteration runs several game frames to catch up.
enum { PERF_DRAW, PERF_DRAWFRAME, PERF_UPLOAD, PERF_RENDER, PERF_VBLANK, PERF_PRESENT, PERF_LOGIC, PERF_COUNT };
static double sPerfTotal[PERF_COUNT], sPerfWorstMain, sPerfFrameStart;
static int32_t sPerfFrames, sPerfPresents, sPerfSkipped;
static u8 sPlatformSettings[PLATFORM_SETTING_COUNT]; // defined (with defaults) further down
static Uint64 sPerfFpsStart;

static double PerfNowMs(void)
{
    return SDL_GetPerformanceCounter() * 1000.0 / SDL_GetPerformanceFrequency();
}

static void PerfFrameDone(double mainWork)
{
    if (mainWork > sPerfWorstMain)
        sPerfWorstMain = mainWork;
    if (sPerfFpsStart == 0)
        sPerfFpsStart = SDL_GetPerformanceCounter();
    if (++sPerfFrames < 600)
        return;
    // Suppressed VITA_LOG to prevent 400ms flash memory write stalls on PS Vita SD card.
    // Performance metrics are recorded with buffered I/O via VitaPerf (perf_frames.csv & perf_summary.txt).
    memset(sPerfTotal, 0, sizeof(sPerfTotal));
    sPerfWorstMain = 0;
    sPerfFrames = 0;
    sPerfPresents = 0;
    sPerfSkipped = 0;
    sPerfFpsStart = SDL_GetPerformanceCounter();
}
SDL_AudioDeviceID sdlAudioDevice;
SDL_sem *vBlankSemaphore;
static SDL_sem *sFrameReadySem;
SDL_atomic_t isFrameAvailable;
bool speedUp = false;
uint32_t videoScale = 1;
bool isRunning = true;
bool paused = false;
double simTime = 0;
double lastGameTime = 0;
double curGameTime = 0;
double fixedTimestep = 1.0 / 60.0; // 16.666667ms
double timeScale = 1.0;
struct SiiRtcInfo internalClock;

static FILE *sSaveFile = NULL;
static char sSavePath[1024] = "pokeemerald.sav";
static char sConfigPath[1024] = "pokeemerald.cfg";
// Integer scale on by default: 3x pixel-perfect (Display Mode 1).
// Buttons: A Cross, B Circle, L, R, Start, Select, fast forward Triangle, menu Square.
// Filter: sharp bilinear. Overlay: none.
static u8 sPlatformSettings[PLATFORM_SETTING_COUNT] = {
    0, 4, 1, 1, 1, 10, 0, 3, 0, 1,
    4, 5, 8, 9, 10, 11, 7, 6,
    2, 0, 0, 0, 0,
    0, 0, 1, 1,
    0, 0
};

static int32_t sRenderWidth = 960;
static int32_t sRenderHeight = 544;
static bool sFastForward = false;
bool gUiVoxelFrame;   // Core 0: this frame is the 3D world (read by the new UI mirrors on Core 1)

int32_t VitaPlatform_GetRenderWidth(void)
{
    return sRenderWidth;
}

int32_t VitaPlatform_GetRenderHeight(void)
{
    return sRenderHeight;
}

void VitaPlatform_ApplyRenderResolution(void)
{
    const uint8_t resMode = sPlatformSettings[PLATFORM_SETTING_VOXEL_RES];
    const int32_t newW = (resMode == 1) ? 720 : 960;
    const int32_t newH = (resMode == 1) ? 408 : 544;

    if (newW != sRenderWidth || newH != sRenderHeight)
    {
        sRenderWidth = newW;
        sRenderHeight = newH;
#ifdef PLATFORM_VITA
        vglSwapResolution(sRenderWidth, sRenderHeight);
#endif
    }
}

void VitaPlatform_ApplyTargetFramerate(void)
{
#ifdef PLATFORM_VITA
    const uint8_t fpsMode = sPlatformSettings[PLATFORM_SETTING_VOXEL_FPS];
    const int32_t interval = (fpsMode == 0 && !sFastForward) ? 2 : 1;
    eglSwapInterval(NULL, interval);
#endif
}

extern void AgbMain(void);
extern void DoSoftReset(void);

int32_t DoMain(void *param);
void ProcessEvents(void);
void VDraw(SDL_Texture *texture);

static void ReadSaveFile(const char *path);
static void ReadConfigFile(void);
static void StoreConfigFile(void);
static void ApplyPlatformSettings(void);
static void StoreSaveFile(void);
static void CloseSaveFile(void);

static void UpdateInternalClock(void);

static void RenderGameTexture(void);

static uint16_t sFrameImage[FRAME_LINE_WIDTH * DISPLAY_HEIGHT];
// Set by the port menu; the game thread resets itself when it next wakes up.
static SDL_atomic_t sResetRequested;
// SoftReset() jumps back to the start of the game thread.
static jmp_buf sGameResetJump;
static bool sGameResetJumpValid;

void VitaPlatform_RequestReset(void)
{
    SDL_AtomicSet(&sResetRequested, 1);
}

void VitaPlatform_RequestQuit(void)
{
    isRunning = false;
}

static GLuint sNativeUiTexture = 0;
static bool sNativeUiTextureAllocated = false;
static int32_t sCachedUiFilter = -1;

static void ShowNativeUi(void)
{
    const uint16_t *canvas = NULL;
    int32_t dirtyY0 = 0;
    int32_t dirtyY1 = 0;
    const bool hasDirtyRows = VitaUI_RenderDirty(&canvas, &dirtyY0, &dirtyY1);
    const int32_t filter = sPlatformSettings[PLATFORM_SETTING_FILTER];
    const bool isFullscreen = (filter != 0 || sPlatformSettings[PLATFORM_SETTING_UI_SCALE] != 0);
    const GLenum minMagFilter = (filter == 0 && sPlatformSettings[PLATFORM_SETTING_UI_SCALE] == 0) ? GL_NEAREST : GL_LINEAR;

    if (sNativeUiTexture == 0)
    {
        glGenTextures(1, &sNativeUiTexture);
        glBindTexture(GL_TEXTURE_2D, sNativeUiTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, minMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VITA_UI_WIDTH, VITA_UI_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, canvas);
        sNativeUiTextureAllocated = true;
        sCachedUiFilter = filter;
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, sNativeUiTexture);
        if (filter != sCachedUiFilter)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minMagFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, minMagFilter);
            sCachedUiFilter = filter;
        }

        if (!sNativeUiTextureAllocated)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VITA_UI_WIDTH, VITA_UI_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, canvas);
            sNativeUiTextureAllocated = true;
        }
        else if (hasDirtyRows && canvas != NULL && dirtyY1 > dirtyY0)
        {
            glTexSubImage2D(
                GL_TEXTURE_2D, 0,
                0, dirtyY0,
                VITA_UI_WIDTH, dirtyY1 - dirtyY0,
                GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                &canvas[(size_t)dirtyY0 * VITA_UI_WIDTH]);
        }
    }

    glViewport(0, 0, sRenderWidth, sRenderHeight);
    glClearColor(0.04f, 0.12f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 960.0, 544.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    // Dynamic quad: Fullscreen (725.33x544 with Sharp Bilinear) or Pixel Perfect (640x480)
    const float uiVerts[8] = {
        isFullscreen ? 117.3333f : 160.0f, isFullscreen ? 0.0f : 32.0f,
        isFullscreen ? 842.6667f : 800.0f, isFullscreen ? 0.0f : 32.0f,
        isFullscreen ? 842.6667f : 800.0f, isFullscreen ? 544.0f : 512.0f,
        isFullscreen ? 117.3333f : 160.0f, isFullscreen ? 544.0f : 512.0f
    };
    static const float uiUvs[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, uiVerts);
    glTexCoordPointer(2, GL_FLOAT, 0, uiUvs);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_FALSE);

#ifdef PLATFORM_VITA
    eglSwapInterval(NULL, 1);
    vglSwapBuffers(GL_FALSE);
#endif
}

// Draws the game texture in native widescreen (16:9 at 960x544) or centered fit-height into the back buffer.
#ifdef WIDESCREEN_SUPPORTED
// GBA pixels over which a still stage picture's repeated edges fade to black (Emerald 3DS,
// 3ds_video.c FADE_ACROSS). The Vita shows only ~21 of them, so the fade stops part-way.
#define STAGE_FADE_ACROSS_GBA_PX 56.0f

// Two quads over the margins, transparent at the picture's edge and darkest at the screen's.
static void DrawStageMarginFade(void)
{
    const float marginGbaPx = WIDESCREEN_PICTURE_X0 / WIDESCREEN_SCALE;
    const float outerAlpha = marginGbaPx >= STAGE_FADE_ACROSS_GBA_PX ? 1.0f
                                                                     : marginGbaPx / STAGE_FADE_ACROSS_GBA_PX;
    const float x0 = WIDESCREEN_PICTURE_X0;
    const float x1 = WIDESCREEN_PICTURE_X1;
    const float w = WIDESCREEN_SCREEN_W;
    const float h = WIDESCREEN_SCREEN_H;
    const float verts[16] = {
        0.0f, 0.0f, x0, 0.0f, x0, h, 0.0f, h,   // left margin
        x1, 0.0f, w, 0.0f, w, h, x1, h,         // right margin
    };
    const float colors[32] = {
        0.0f, 0.0f, 0.0f, outerAlpha, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,       0.0f, 0.0f, 0.0f, outerAlpha,
        0.0f, 0.0f, 0.0f, 0.0f,       0.0f, 0.0f, 0.0f, outerAlpha,
        0.0f, 0.0f, 0.0f, outerAlpha, 0.0f, 0.0f, 0.0f, 0.0f,
    };

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}
#endif

static void RenderGameTexture(void)
{
    glViewport(0, 0, sRenderWidth, sRenderHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 960.0, 544.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    VitaStage_UpdateSceneState();
    const int32_t stageKind = VitaStage_GetActiveKind();

    float vx0, vx1, vy0 = 0.0f, vy1 = 544.0f;
    float u0, u1;

#ifdef WIDESCREEN_SUPPORTED
    if (gFastDrawWide.output)
    {
        // Widescreen frame: the visible part of the 288 px line at the uniform x3.4 scale
        vx0 = 0.0f;
        vx1 = WIDESCREEN_SCREEN_W;
        u0 = WIDESCREEN_VISIBLE_U0;
        u1 = WIDESCREEN_VISIBLE_U1;
    }
    else
#endif
    {
        // Standard GBA 3:2 aspect ratio cleanly scaled to full 544p height (816x544 centered in 960x544)
        const float gameWidth = (544.0f * (float)DISPLAY_WIDTH) / (float)DISPLAY_HEIGHT; // 816.0f
        vx0 = (960.0f - gameWidth) / 2.0f; // 72.0f
        vx1 = vx0 + gameWidth;             // 888.0f
#ifdef WIDESCREEN_SUPPORTED
        u0 = (float)FRAME_LINE_X0 / (float)FRAME_LINE_WIDTH;
        u1 = (float)(FRAME_LINE_X0 + DISPLAY_WIDTH) / (float)FRAME_LINE_WIDTH;
#else
        u0 = 0.0f;
        u1 = 1.0f;
#endif
    }

    const bool inFieldMove = FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON)
                          || FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    if (inFieldMove)
    {
        vx0 = 0.0f;
        vx1 = 960.0f;
        u0 = (float)FRAME_LINE_X0 / (float)FRAME_LINE_WIDTH;
        u1 = (float)(FRAME_LINE_X0 + DISPLAY_WIDTH) / (float)FRAME_LINE_WIDTH;
    }

    glBindTexture(GL_TEXTURE_2D, sGameTextureRing[sGameTextureUploadIdx]);
    static int32_t sCachedFilter = -1;
    const int32_t filter = sPlatformSettings[PLATFORM_SETTING_FILTER];
    if (filter != sCachedFilter)
    {
        for (int32_t r = 0; r < VITA_GAME_TEXTURE_RING; r++)
        {
            glBindTexture(GL_TEXTURE_2D, sGameTextureRing[r]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        }
        glBindTexture(GL_TEXTURE_2D, sGameTextureRing[sGameTextureUploadIdx]);
        sCachedFilter = filter;
    }

#ifdef WIDESCREEN_SUPPORTED
    const bool wideOutput = gFastDrawWide.output;
#else
    const bool wideOutput = false;
#endif
    if (stageKind == VITA_STAGE_INTRO_BIKE && !wideOutput)
    {
        // Bike ride wraps across whole 960 px screen
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        vx0 = 0.0f;
        vx1 = 960.0f;
#ifdef WIDESCREEN_SUPPORTED
        const float uMid0 = (float)FRAME_LINE_X0 / (float)FRAME_LINE_WIDTH;
        const float uMid1 = (float)(FRAME_LINE_X0 + DISPLAY_WIDTH) / (float)FRAME_LINE_WIDTH;
        const float uMargin = (72.0f / 816.0f) * (uMid1 - uMid0);
        u0 = uMid0 - uMargin;
        u1 = uMid1 + uMargin;
#else
        u0 = -72.0f / 816.0f;
        u1 = 1.0f + 72.0f / 816.0f;
#endif
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    }

    const float quadVerts[8] = {
        vx0, vy0,
        vx1, vy0,
        vx1, vy1,
        vx0, vy1
    };
    const float quadUvs[8] = {
        u0, 0.0f,
        u1, 0.0f,
        u1, 1.0f,
        u0, 1.0f
    };

    float quadR = 1.0f, quadG = 1.0f, quadB = 1.0f;
#ifdef PLATFORM_VITA
    float moonAmount = 0.0f;
    float moonR = 0.0f, moonG = 0.0f, moonB = 0.0f;
    bool hasMoon = false;

    if (VitaExtras_GetFast(FAST_EXTRA_DAY_NIGHT) == 1 && VitaGame_IsOutdoor() && VitaGame_IsOverworld())
    {
        const VitaDayNightLight *const light = VitaDayNight_Get();
        const VitaDayNightBlend moon = VitaDayNight_Moonlight(light, 0.0f, light->rgb);

        if (light->daylight < 0.98f)
        {
            quadR = light->rgb[0];
            quadG = light->rgb[1];
            quadB = light->rgb[2];
        }
        if (moon.amount > 0.005f)
        {
            hasMoon = true;
            moonAmount = moon.amount * 0.70f;
            moonR = moon.rgb[0];
            moonG = moon.rgb[1];
            moonB = moon.rgb[2];
        }
    }
#endif

    glColor4f(quadR, quadG, quadB, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, quadVerts);
    glTexCoordPointer(2, GL_FLOAT, 0, quadUvs);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

#ifdef PLATFORM_VITA
    if (hasMoon)
    {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(moonR, moonG, moonB, moonAmount);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }
#endif
    glDisableClientState(GL_VERTEX_ARRAY);

    // Without widescreen, the intro's leaves art beside the picture. With it, the margins already
    // hold the scene (widescreen.c); the leaves scene, whose maps end at the screen's edge, gets
    // its drawn continuation instead of repeated edge tiles; a still stage fades to black.
    if (!wideOutput)
    {
        VitaStage_RenderWidescreenMargins(vx0, vx1, vy0, vy1);
    }
#ifdef WIDESCREEN_SUPPORTED
    else
    {
        if (stageKind == VITA_STAGE_INTRO_LEAVES)
            VitaStage_RenderLeavesMargins(WIDESCREEN_PICTURE_X0, WIDESCREEN_PICTURE_X1, WIDESCREEN_SCALE);
        if (Widescreen_StageFadesMargins())
            DrawStageMarginFade();
    }
#endif
    glDepthMask(GL_FALSE);
}

// Keep audio engine and VBlank interrupts ticking at 60 Hz during menus without Core 0 spin-waiting
static inline void TickMenuAudioAndVBlank(uint64_t menuIterationStart)
{
    if (!SDL_AtomicGet(&isFrameAvailable) && sFrameReadySem != NULL)
    {
        SDL_SemWaitTimeout(sFrameReadySem, 16);
    }
    while (sFrameReadySem != NULL && SDL_SemTryWait(sFrameReadySem) == 0)
    {
    }
    SDL_AtomicSet(&isFrameAvailable, 0);

    const uint64_t freq = SDL_GetPerformanceFrequency();
    const uint64_t elapsedTicks = SDL_GetPerformanceCounter() - menuIterationStart;
    const double elapsedUs = ((double)elapsedTicks * 1000000.0) / (double)freq;
    if (elapsedUs < 16500.0)
    {
        VitaPlatform_DelayMicroseconds((uint32_t)(16666.0 - elapsedUs));
    }
}


int main(int argc, char **argv)
{
#ifdef PLATFORM_VITA
    VitaPlatform_PinCurrentThread("MainThread", 0);
#endif
    // Open an output console on Windows


    VitaPlatform_Init();
    VitaPerf_Init();
    ModManager_Init();
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        DBGPRINTF("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // The config says which save file (1-3) to open.
    VitaPlatform_BuildPath(sConfigPath, sizeof(sConfigPath), "pokeemerald.cfg");
    ReadConfigFile();
    VitaUI_GetSavePath(sSavePath, sizeof(sSavePath));
    ReadSaveFile(sSavePath);

    // Initialize hardware accelerated OpenGL via vitaGL
#ifdef PLATFORM_VITA
    VITA_LOG("Initializing vitaGL with custom right-sized memory pools...\n");
    vglSetupGarbageCollector(127, SCE_KERNEL_CPU_MASK_USER_2);
    vglSetupRuntimeShaderCompiler(SHARK_OPT_FAST, GL_TRUE, GL_FALSE, GL_TRUE);
    vglUseLowPrecision(GL_TRUE);
    vglUseVramForUSSE(GL_TRUE);
    vglUseCachedMem(GL_TRUE);
    vglUseTripleBuffering(GL_TRUE);
    if (!vglInitWithCustomSizes(8 * 1024 * 1024, 960, 544, 16 * 1024 * 1024, 48 * 1024 * 1024, 8 * 1024 * 1024, 0, SCE_GXM_MULTISAMPLE_NONE))
    {
        VITA_LOG("vglInitWithCustomSizes failed!\n");
    }
    else
    {
        VITA_LOG("vglInitWithCustomSizes succeeded (8MB pool, 16MB RAM, 48MB CDRAM, 8MB PHYCONT)\n");
    }
    VitaPerf_LogMemory("after vglInit");
#endif
#ifdef VOXEL_SUPPORTED
    VoxelRenderer_Init();
    VitaPerf_LogMemory("after VoxelRenderer_Init");
#endif

    ApplyPlatformSettings();

    glGenTextures(VITA_GAME_TEXTURE_RING, sGameTextureRing);
    for (int32_t i = 0; i < VITA_GAME_TEXTURE_RING; i++)
    {
        glBindTexture(GL_TEXTURE_2D, sGameTextureRing[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_LINE_WIDTH, DISPLAY_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    }

    VitaUI_Prewarm();
    // New UI atlas, before the game thread starts (its tables are read from Core 1)
    Ui_Init();


    simTime = curGameTime = lastGameTime = SDL_GetPerformanceCounter();

    isFrameAvailable.value = 0;
    vBlankSemaphore = SDL_CreateSemaphore(0);
    sFrameReadySem = SDL_CreateSemaphore(0);

    SDL_AudioSpec want;

    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = VITA_AUDIO_RATE;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 512;
    cgb_audio_init(want.freq);
    sAudioStream = NULL;

    sdlAudioDevice = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (sdlAudioDevice == 0)
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    else
    {
        if (want.format != AUDIO_F32) /* we let this one thing change. */
            SDL_Log("We didn't get Float32 audio format.");
        SDL_PauseAudioDevice(sdlAudioDevice, 0);
    }
    VITA_LOG("audio: device %s at %d Hz\n", sdlAudioDevice ? "open" : "FAILED", want.freq);
    if (sdlTexture != NULL)
        VDraw(sdlTexture);
    // The game's memory at power on, for resets and switching save files.
    VitaReset_TakeBootSnapshot();
    // SDL's default Vita thread stack is small; the game logic runs on this thread.
    mainLoopThread = SDL_CreateThreadWithStackSize(DoMain, "AgbMain", 4 * 1024 * 1024, NULL);

    double accumulator = 0.0;

    memset(&internalClock, 0, sizeof(internalClock));
    internalClock.status = SIIRTCINFO_24HOUR;
    UpdateInternalClock();

    while (isRunning)
    {
        ProcessEvents();

        {
            static uint32_t sPrevButtons;
            const uint32_t buttons = VitaPlatform_ReadButtons();
            const uint32_t menuButton = VitaUI_ActionButton(VITA_ACTION_MENU);
            const uint32_t startButton = VITA_BTN_START;

            extern void CB2_Overworld(void);
            const bool isStandaloneOpts = (gMain.callback2 == CB2_VitaStandaloneOptions);
            const bool inBattleMenu = gMain.inBattle && (VitaUI_GetSection() == VITA_UI_SEC_BAG || VitaUI_GetSection() == VITA_UI_SEC_POKEMON);
            const bool isShopSell = VitaUI_IsShopSellActive();
            const bool canOpenMenu = (gMain.callback2 == CB2_Overworld) || isStandaloneOpts || inBattleMenu || isShopSell;

            if (VitaUI_IsOpen() && !canOpenMenu)
            {
                VitaUI_Close();
            }

            // With the new UI on, START in the field belongs to it (handled on Core 1 in Ui_Tick);
            // the options button still opens the old options, so the extra can be switched off.
            const bool newUiStart = Ui_IsEnabled() && !VitaUI_IsOpen() && !isStandaloneOpts && !inBattleMenu && !isShopSell;
            if (Ui_IsOpen())
            {
                // nothing: the new UI has the buttons
            }
            else if ((buttons & startButton) && !(sPrevButtons & startButton) && !newUiStart)
            {
                if (VitaUI_IsOpen())
                {
                    if (!gMain.inBattle)
                        VitaUI_Close();
                }
                else if (gMain.callback2 == CB2_Overworld)
                    VitaUI_Open(VITA_UI_SEC_MAP);
            }
            else if ((buttons & menuButton) && !(sPrevButtons & menuButton) && !Ui_IsEnabled())
            {
                if (VitaUI_IsOpen())
                    VitaUI_Close();
                else if (gMain.callback2 == CB2_Overworld)
                    VitaUI_Open(VITA_UI_SEC_OPTIONS);
            }

            sPrevButtons = buttons;
            if (VitaUI_IsOpen())
            {
                const uint64_t iterStart = SDL_GetPerformanceCounter();
                VitaUI_Update(buttons);
                if (VitaUI_IsOpen())
                    ShowNativeUi();

                TickMenuAudioAndVBlank(iterStart);
                accumulator = 0.0;
                lastGameTime = SDL_GetPerformanceCounter();
                continue;
            }
            sFastForward = VitaUI_FastForwardActive(buttons);
            timeScale = sFastForward ? Platform_GetSetting(PLATFORM_SETTING_FAST_FORWARD_SPEED) : 1.0;
        }

        bool voxelActive = false;
#ifdef VOXEL_SUPPORTED
        voxelActive = Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D)
            && (VoxelWorld_IsMapAvailable()
                || (VoxelWorld_IsBattleMapAvailable()
                    && VitaExtras_GetFast(FAST_EXTRA_VOXEL_BATTLE) != 0
                    && !VoxelBattle_IsLink()));
#endif
        __atomic_store_n(&gUiVoxelFrame, voxelActive, __ATOMIC_RELAXED);   // the new UI mirrors need the 3D frame

        if (paused)
        {
            VitaPlatform_DelayMicroseconds(16000);
            lastGameTime = SDL_GetPerformanceCounter();
            continue;
        }

        {
            const bool is30FpsLocked = (sPlatformSettings[PLATFORM_SETTING_VOXEL_FPS] == 0) && !sFastForward && (timeScale <= 1.0);
            double dt = ((voxelActive && is30FpsLocked) ? (2.0 * fixedTimestep) : fixedTimestep) / timeScale;

            // In 3D, render right after the game has produced a new frame (two at 30 FPS) instead
            // of on Core 0's own 60 Hz timer. The two clocks drift (Core 1 skews its period by
            // up to 500 us to follow the audio queue), so a free-running renderer drew the same
            // game frame twice and then skipped one: the player and the camera stuttered while
            // running even when every frame was fast.
            if (voxelActive && !sFastForward && timeScale <= 1.0 && sFrameReadySem != NULL)
            {
                const int32_t ticksNeeded = is30FpsLocked ? 2 : 1;
                for (int32_t got = 0; got < ticksNeeded;)
                {
                    // A game that stops posting (a long load) still gets a frame every 50 ms.
                    if (SDL_SemWaitTimeout(sFrameReadySem, 50) != 0)
                        break;
                    got++;
                }
                // Frames posted while the last one rendered are behind us: draw the latest.
                while (SDL_SemTryWait(sFrameReadySem) == 0)
                {
                }
                accumulator = dt;
                lastGameTime = SDL_GetPerformanceCounter();
            }

            curGameTime = SDL_GetPerformanceCounter();
            double deltaTime = (double)((curGameTime - lastGameTime) / (double)SDL_GetPerformanceFrequency());
            // A long stall counts as one frame (catching it up would run seconds of game unseen).
            // Measured against the normal frame time, so fast forward isn't clamped to 1 frame.
            if (deltaTime > (fixedTimestep * 5))
                deltaTime = fixedTimestep;
            lastGameTime = curGameTime;

            accumulator += deltaTime;

            // Sleep Core 0 cleanly in kernel wait state until the next 60 Hz tick is due (eliminates 97% spin-wait)
            if (accumulator < dt)
            {
                const double remUs = (dt - accumulator) * 1000000.0;
                const uint32_t sleepUs = (remUs > 250.0) ? (uint32_t)(remUs - 150.0) : 50u;
                VitaPlatform_DelayMicroseconds(sleepUs);
                continue;
            }

            if (accumulator > dt * 2.0)
                accumulator = dt * 2.0;

            while (accumulator >= dt)
            {
                VitaPerf_BeginFrame();
                if (!voxelActive && !SDL_AtomicGet(&isFrameAvailable))
                {
                    if (sFrameReadySem != NULL)
                        SDL_SemWaitTimeout(sFrameReadySem, 35);
                    if (!SDL_AtomicGet(&isFrameAvailable))
                        break;
                }
                while (sFrameReadySem != NULL && SDL_SemTryWait(sFrameReadySem) == 0)
                {
                }

                bool didPresent3D = false;
                bool didPresent2D = false;
                sPerfFrameStart = PerfNowMs();

#ifdef VITA_TEXTURE_RING
                sFrameTextureIndex = (sFrameTextureIndex + 1) % VITA_TEXTURE_RING;
                sdlTexture = sFrameTextures[sFrameTextureIndex];
#endif
                if (voxelActive)
                {
                    if (VoxelRenderer_RenderFrame())
                        didPresent3D = true;
                }
                else
                {
                    VDraw(NULL);
                    const double perfT1 = PerfNowMs();
                    sPerfTotal[PERF_DRAW] += perfT1 - sPerfFrameStart;
                    RenderGameTexture();
                    Ui_Render(sRenderWidth, sRenderHeight);
                    sPerfTotal[PERF_RENDER] += PerfNowMs() - perfT1;
                    didPresent2D = true;
                }

                SDL_AtomicSet(&isFrameAvailable, 0);

                if (didPresent3D)
                {
                    const double perfPresentStart = PerfNowMs();
#ifdef PLATFORM_VITA
                    eglSwapInterval(NULL, is30FpsLocked ? 2 : 1);
#endif
#ifdef VOXEL_SUPPORTED
                    VoxelRenderer_SwapBuffers();
#endif
                    sPerfTotal[PERF_PRESENT] += PerfNowMs() - perfPresentStart;
                    sPerfPresents++;
                }
                else if (didPresent2D)
                {
                    const double perfPresentStart = PerfNowMs();
#ifdef PLATFORM_VITA
                    eglSwapInterval(NULL, 1);
                    vglSwapBuffers(GL_FALSE);
#endif
                    if (sdlRenderer)
                        SDL_RenderPresent(sdlRenderer);
                    sPerfTotal[PERF_PRESENT] += PerfNowMs() - perfPresentStart;
                    sPerfPresents++;
                }

                PerfFrameDone(PerfNowMs() - sPerfFrameStart);
                VitaPerf_EndFrame(didPresent3D, (voxelActive && is30FpsLocked) ? 30u : 60u);
                if (voxelActive)
                    accumulator = 0.0;
                else
                    accumulator -= dt;
            }
        }
    }

    //StoreSaveFile();
    CloseSaveFile();
    // Quit from the port menu. The game thread is blocked mid-frame, so end the process here
    // rather than tear SDL down under it.
    VitaPlatform_Exit();

    SDL_DestroyWindow(sdlWindow);
    ModManager_Shutdown();
    SDL_Quit();
    return 0;
}

static void ReadSaveFile(const char *path)
{
    // Check whether the saveFile exists, and create it if not
    sSaveFile = fopen(path, "r+b");
    if (sSaveFile == NULL)
    {
        sSaveFile = fopen(path, "w+b");
    }

    if (sSaveFile == NULL)
    {
        memset(FLASH_BASE, 0xFF, sizeof(FLASH_BASE));
        SDL_Log("Unable to open save file: %s", path);
        return;
    }

    fseek(sSaveFile, 0, SEEK_END);
    int32_t fileSize = ftell(sSaveFile);
    fseek(sSaveFile, 0, SEEK_SET);

    // Only read as many bytes as fit inside the buffer
    // or as many bytes as are in the file
    int32_t bytesToRead = (fileSize < sizeof(FLASH_BASE)) ? fileSize : sizeof(FLASH_BASE);

    int32_t bytesRead = fread(FLASH_BASE, 1, bytesToRead, sSaveFile);

    // Fill the buffer if the savefile was just created or smaller than the buffer itself
    for (int32_t i = bytesRead; i < sizeof(FLASH_BASE); i++)
    {
        FLASH_BASE[i] = 0xFF;
    }
}

static void ReadConfigFile(void)
{
    FILE *configFile = fopen(sConfigPath, "r");
    char line[64];
    uint32_t value;

    if (configFile == NULL)
        return;
    while (fgets(line, sizeof(line), configFile) != NULL)
    {
        if (sscanf(line, "fullscreen=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_FULLSCREEN] = value != 0;
        else if (sscanf(line, "windowScale=%u", &value) == 1 && value >= 2 && value <= 5)
            sPlatformSettings[PLATFORM_SETTING_WINDOW_SCALE] = value;
        else if (sscanf(line, "integerScale=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_INTEGER_SCALE] = value != 0;
        else if (sscanf(line, "vsync=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_VSYNC] = value != 0;
        else if (sscanf(line, "border=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_BORDER] = value != 0;
        else if (sscanf(line, "volume=%u", &value) == 1 && value <= 10)
            sPlatformSettings[PLATFORM_SETTING_VOLUME] = value;
        else if (sscanf(line, "widescreen=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_WIDESCREEN] = value != 0;
        else if (sscanf(line, "fastForwardSpeed=%u", &value) == 1 && value >= 2 && value <= 5)
            sPlatformSettings[PLATFORM_SETTING_FAST_FORWARD_SPEED] = value;
        else if (sscanf(line, "fastForwardToggle=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_FAST_FORWARD_TOGGLE] = value != 0;
        else if (sscanf(line, "saveFile=%u", &value) == 1 && value >= 1 && value <= 3)
            sPlatformSettings[PLATFORM_SETTING_SAVE_FILE] = value;
        else if (sscanf(line, "filter=%u", &value) == 1 && value <= 2)
        {
            sPlatformSettings[PLATFORM_SETTING_FILTER] = value;
            sPlatformSettings[PLATFORM_SETTING_UI_SCALE] = (value != 0) ? 1 : 0;
        }
        else if (sscanf(line, "overlay=%u", &value) == 1 && value <= 3)
            sPlatformSettings[PLATFORM_SETTING_OVERLAY] = value;
        else if (sscanf(line, "autoRun=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_AUTO_RUN] = value != 0;
        else if (sscanf(line, "hmShortcuts=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_HM_SHORTCUTS] = value != 0;
        else if (sscanf(line, "statsInfo=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_STATS_INFO] = value != 0;
        else if (sscanf(line, "expShare=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_EXP_SHARE] = value != 0;
        else if (sscanf(line, "expMultiplier=%u", &value) == 1 && value <= 3)
            sPlatformSettings[PLATFORM_SETTING_EXP_MULTIPLIER] = value;
        else if (sscanf(line, "uiScale=%u", &value) == 1)
            sPlatformSettings[PLATFORM_SETTING_UI_SCALE] = value != 0;
        else if (sscanf(line, "voxelRes=%u", &value) == 1 && value <= 1)
            sPlatformSettings[PLATFORM_SETTING_VOXEL_RES] = value;
        else if (sscanf(line, "voxelFps=%u", &value) == 1 && value <= 1)
            sPlatformSettings[PLATFORM_SETTING_VOXEL_FPS] = value;
        else
        {
            uint32_t action;
            if (sscanf(line, "button%u=%u", &action, &value) == 2
             && action <= PLATFORM_SETTING_BUTTON_LAST - PLATFORM_SETTING_BUTTON_FIRST && (value < 16 || value == 0xFF))   // + right stick, unbound
                sPlatformSettings[PLATFORM_SETTING_BUTTON_FIRST + action] = value;
        }
    }
    fclose(configFile);
}

static void StoreConfigFile(void)
{
    FILE *configFile = fopen(sConfigPath, "w");

    if (configFile == NULL)
        return;
    fprintf(configFile, "fullscreen=%u\n", sPlatformSettings[PLATFORM_SETTING_FULLSCREEN]);
    fprintf(configFile, "windowScale=%u\n", sPlatformSettings[PLATFORM_SETTING_WINDOW_SCALE]);
    fprintf(configFile, "integerScale=%u\n", sPlatformSettings[PLATFORM_SETTING_INTEGER_SCALE]);
    fprintf(configFile, "vsync=%u\n", sPlatformSettings[PLATFORM_SETTING_VSYNC]);
    fprintf(configFile, "border=%u\n", sPlatformSettings[PLATFORM_SETTING_BORDER]);
    fprintf(configFile, "volume=%u\n", sPlatformSettings[PLATFORM_SETTING_VOLUME]);
    fprintf(configFile, "widescreen=%u\n", sPlatformSettings[PLATFORM_SETTING_WIDESCREEN]);
    fprintf(configFile, "fastForwardSpeed=%u\n", sPlatformSettings[PLATFORM_SETTING_FAST_FORWARD_SPEED]);
    fprintf(configFile, "fastForwardToggle=%u\n", sPlatformSettings[PLATFORM_SETTING_FAST_FORWARD_TOGGLE]);
    fprintf(configFile, "saveFile=%u\n", sPlatformSettings[PLATFORM_SETTING_SAVE_FILE]);
    fprintf(configFile, "filter=%u\n", sPlatformSettings[PLATFORM_SETTING_FILTER]);
    fprintf(configFile, "overlay=%u\n", sPlatformSettings[PLATFORM_SETTING_OVERLAY]);
    fprintf(configFile, "autoRun=%u\n", sPlatformSettings[PLATFORM_SETTING_AUTO_RUN]);
    fprintf(configFile, "hmShortcuts=%u\n", sPlatformSettings[PLATFORM_SETTING_HM_SHORTCUTS]);
    fprintf(configFile, "statsInfo=%u\n", sPlatformSettings[PLATFORM_SETTING_STATS_INFO]);
    fprintf(configFile, "expShare=%u\n", sPlatformSettings[PLATFORM_SETTING_EXP_SHARE]);
    fprintf(configFile, "expMultiplier=%u\n", sPlatformSettings[PLATFORM_SETTING_EXP_MULTIPLIER]);
    fprintf(configFile, "uiScale=%u\n", sPlatformSettings[PLATFORM_SETTING_UI_SCALE]);
    fprintf(configFile, "voxelRes=%u\n", sPlatformSettings[PLATFORM_SETTING_VOXEL_RES]);
    fprintf(configFile, "voxelFps=%u\n", sPlatformSettings[PLATFORM_SETTING_VOXEL_FPS]);
    for (int32_t i = PLATFORM_SETTING_BUTTON_FIRST; i <= PLATFORM_SETTING_BUTTON_LAST; i++)
        fprintf(configFile, "button%d=%u\n", i - PLATFORM_SETTING_BUTTON_FIRST, sPlatformSettings[i]);
    fclose(configFile);
}

static void ApplyPlatformSettings(void)
{
#ifdef PLATFORM_VITA
    VitaAudio_SetVolume(sPlatformSettings[PLATFORM_SETTING_VOLUME]);
#endif
    VitaPlatform_ApplyRenderResolution();
    VitaPlatform_ApplyTargetFramerate();
    if (sdlRenderer == NULL)
        return;
    SDL_RenderSetVSync(sdlRenderer, sPlatformSettings[PLATFORM_SETTING_VSYNC]);
}

static void StoreSaveFile()
{
    if (sSaveFile != NULL)
    {
        fseek(sSaveFile, 0, SEEK_SET);
        fwrite(FLASH_BASE, 1, sizeof(FLASH_BASE), sSaveFile);
        // Vita apps are usually killed from LiveArea rather than closed, so never leave
        // a save sitting in the stdio buffer.
        fflush(sSaveFile);
    }
}

void Platform_StoreSaveFile(void)
{
    StoreSaveFile();
}

void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size)
{
    DBGPRINTF("ReadFlash(sectorNum=0x%04X,offset=0x%08X,size=0x%02X)\n",sectorNum,offset,size);
    FILE * savefile = fopen(sSavePath, "r+b");
    if (savefile == NULL)
    {
        VITA_LOG("Error opening save file: %s\n", sSavePath);
        return;
    }
    if (fseek(savefile, (sectorNum << gFlash->sector.shift) + offset, SEEK_SET))
    {
        fclose(savefile);
        return;
    }
    if (fread(dest, 1, size, savefile) != size)
    {
        fclose(savefile);
        return;
    }
    fclose(savefile);
}

void Platform_QueueAudio(float *audioBuffer, u32 bufferSizeBytes)
{
    if (sdlAudioDevice != 0 && audioBuffer != NULL && bufferSizeBytes > 0)
    {
        const uint32_t queued = SDL_GetQueuedAudioSize(sdlAudioDevice);
        // 6-frame (~100 ms) cushion at 48 kHz stereo float prevents audible dropouts without noticeable latency
        const uint32_t maxQueue = 6 * (VITA_AUDIO_RATE / 60) * 2 * sizeof(float);

        if (queued > maxQueue)
        {
            if (sFastForward || speedUp)
            {
                SDL_ClearQueuedAudio(sdlAudioDevice);
            }
            else
            {
                // In normal gameplay, if audio backed up due to a frametime hitch,
                // do NOT clear the existing buffer (which causes an audible silence/pop).
                // Instead, drop the newest frame to let hardware smoothly drain the queue.
                return;
            }
        }

        static float sAdjustedAudio[8192];
        int32_t floatCount = bufferSizeBytes / sizeof(float);
        if (floatCount > 8192)
            floatCount = 8192;

        float volume = sPlatformSettings[PLATFORM_SETTING_VOLUME] / 10.0f;
        for (int32_t i = 0; i < floatCount; i++)
            sAdjustedAudio[i] = audioBuffer[i] * volume;

        // DC-blocking high-pass, like the capacitor on the GBA's audio output: CGB channels
        // left at a non-zero level otherwise cause pops.
        static float sPrevIn[2], sPrevOut[2];
        for (int32_t i = 0; i < floatCount; i++)
        {
            int32_t ch = i & 1;
            float out = sAdjustedAudio[i] - sPrevIn[ch] + 0.9995f * sPrevOut[ch];
            sPrevIn[ch] = sAdjustedAudio[i];
            sPrevOut[ch] = out;
            sAdjustedAudio[i] = out;
        }

        if (sAudioStream != NULL)
        {
            static float sResampled[8192];
            int32_t got;

            SDL_AudioStreamPut(sAudioStream, sAdjustedAudio, floatCount * sizeof(float));
            while ((got = SDL_AudioStreamGet(sAudioStream, sResampled, sizeof(sResampled))) > 0)
                SDL_QueueAudio(sdlAudioDevice, sResampled, got);
            return;
        }

        if (SDL_QueueAudio(sdlAudioDevice, sAdjustedAudio, floatCount * sizeof(float)) < 0)
            SDL_Log("Failed to queue audio: %s", SDL_GetError());
    }
}

u8 Platform_GetSetting(enum PlatformSetting setting)
{
    return sPlatformSettings[setting];
}

void Platform_SetSetting(enum PlatformSetting setting, u8 value)
{
    sPlatformSettings[setting] = value;
    if (setting == PLATFORM_SETTING_FILTER)
        sPlatformSettings[PLATFORM_SETTING_UI_SCALE] = (value != 0) ? 1 : 0;
    else if (setting == PLATFORM_SETTING_UI_SCALE)
        sPlatformSettings[PLATFORM_SETTING_FILTER] = (value != 0) ? 2 : 0;
    else if (setting == PLATFORM_SETTING_VOXEL_RES)
        VitaPlatform_ApplyRenderResolution();
    else if (setting == PLATFORM_SETTING_VOXEL_FPS)
        VitaPlatform_ApplyTargetFramerate();
    if (setting == PLATFORM_SETTING_VSYNC && sdlRenderer != NULL)
        SDL_RenderSetVSync(sdlRenderer, value);
    StoreConfigFile();
}

bool8 Platform_IsVoxelOverworldActive(void)
{
#ifdef VOXEL_SUPPORTED
    return Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D) && VoxelWorld_IsMapAvailable();
#else
    return FALSE;
#endif
}



static void CloseSaveFile()
{
    if (sSaveFile != NULL)
    {
        fclose(sSaveFile);
        sSaveFile = NULL;
    }
}

// Key mappings
#define KEY_A_BUTTON      SDLK_z
#define KEY_B_BUTTON      SDLK_x
#define KEY_START_BUTTON  SDLK_RETURN
#define KEY_SELECT_BUTTON SDLK_BACKSLASH
#define KEY_L_BUTTON      SDLK_a
#define KEY_R_BUTTON      SDLK_s
#define KEY_DPAD_UP       SDLK_UP
#define KEY_DPAD_DOWN     SDLK_DOWN
#define KEY_DPAD_LEFT     SDLK_LEFT
#define KEY_DPAD_RIGHT    SDLK_RIGHT

#define HANDLE_KEYUP(key) \
case KEY_##key:  keyboardKeys &= ~key; break;

#define HANDLE_KEYDOWN(key) \
case KEY_##key:  keyboardKeys |= key; break;

static u16 keyboardKeys;


void ProcessEvents(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            isRunning = false;
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            HANDLE_KEYUP(A_BUTTON)
            HANDLE_KEYUP(B_BUTTON)
            HANDLE_KEYUP(START_BUTTON)
            HANDLE_KEYUP(SELECT_BUTTON)
            HANDLE_KEYUP(L_BUTTON)
            HANDLE_KEYUP(R_BUTTON)
            HANDLE_KEYUP(DPAD_UP)
            HANDLE_KEYUP(DPAD_DOWN)
            HANDLE_KEYUP(DPAD_LEFT)
            HANDLE_KEYUP(DPAD_RIGHT)
            case SDLK_SPACE:
                if (speedUp)
                {
                    speedUp = false;
                    timeScale = 1.0;
                    SDL_ClearQueuedAudio(sdlAudioDevice);
                    SDL_PauseAudioDevice(sdlAudioDevice, 0);
                }
                break;
            }
            break;
        case SDL_MOUSEMOTION:
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            HANDLE_KEYDOWN(A_BUTTON)
            HANDLE_KEYDOWN(B_BUTTON)
            HANDLE_KEYDOWN(START_BUTTON)
            HANDLE_KEYDOWN(SELECT_BUTTON)
            HANDLE_KEYDOWN(L_BUTTON)
            HANDLE_KEYDOWN(R_BUTTON)
            HANDLE_KEYDOWN(DPAD_UP)
            HANDLE_KEYDOWN(DPAD_DOWN)
            HANDLE_KEYDOWN(DPAD_LEFT)
            HANDLE_KEYDOWN(DPAD_RIGHT)
            case SDLK_r:
                if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
                {
                    DoSoftReset();
                }
                break;
            case SDLK_p:
                if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
                {
                    paused = !paused;
                }
                break;
            case SDLK_SPACE:
                if (!speedUp)
                {
                    speedUp = true;
                    timeScale = 5.0;
                    SDL_PauseAudioDevice(sdlAudioDevice, 1);
                }
                break;
            }
            break;
        }
    }
}


u16 Platform_GetKeyInput(void)
{
    // When native Emerald 3DS UI is open, do not deliver keys to in-game movement/events
    if (VitaUI_IsOpen() || Ui_ConsumesInput())
        return 0;

    // Read straight from sceCtrl on the game thread; mask out START in overworld where VitaUI handles it
    uint16_t keys = VitaUI_MapButtons(VitaPlatform_ReadButtons()) | Ui_TakeInjectedKeys();
    extern void CB2_Overworld(void);
    if (gMain.callback2 == CB2_Overworld)
        keys &= ~START_BUTTON;
    return keys;
}


// Draws the next frame; sFrameImage keeps it in the GBA's BGR555 (the port menu shows it
// dimmed behind itself).
void VDraw(SDL_Texture *texture)
{
    (void)texture;
    uint16_t *gbaImage = sFrameImage;
    // New UI battle HUD: the GBA text box and menus (BG0, BG1 while the level-up box shows) are its
    gFastDrawDispcntClear = Ui_BattleDispcntClear();

    static int32_t sClassicFrames = 0;
    sClassicFrames++;
#if defined(VITA_PARALLEL_DRAW)
    {
        static uint16_t sConverted[FRAME_LINE_WIDTH * DISPLAY_HEIGHT];
        double perfT0 = PerfNowMs();
        DrawFrameToTexture(gbaImage, sConverted, FRAME_LINE_WIDTH * sizeof(uint16_t));
        double perfT1 = PerfNowMs();

        sGameTextureUploadIdx = (uint8_t)((sGameTextureUploadIdx + 1) % VITA_GAME_TEXTURE_RING);
        glBindTexture(GL_TEXTURE_2D, sGameTextureRing[sGameTextureUploadIdx]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAME_LINE_WIDTH, DISPLAY_HEIGHT, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sConverted);

        sPerfTotal[PERF_DRAWFRAME] += perfT1 - perfT0;
        sPerfTotal[PERF_UPLOAD] += PerfNowMs() - perfT1;
        gFastDrawDispcntClear = 0;
        REG_VCOUNT = 161; // prep for being in VBlank period
        return;
    }
#endif
    memset(gbaImage, 0, sizeof(sFrameImage));
    double perfT0 = PerfNowMs();
    DrawFrame(gbaImage);
    gFastDrawDispcntClear = 0;
    double perfT1 = PerfNowMs();

    static uint16_t sConvertedFallback[FRAME_LINE_WIDTH * DISPLAY_HEIGHT];
    for (int32_t i = 0; i < FRAME_LINE_WIDTH * DISPLAY_HEIGHT; i++)
    {
        uint16_t c = gbaImage[i];
        uint16_t r = c & 0x1F;
        uint16_t g = (c >> 5) & 0x1F;
        uint16_t b = (c >> 10) & 0x1F;
        sConvertedFallback[i] = (r << 11) | (((g << 1) | (g >> 4)) << 5) | b;
    }
    sGameTextureUploadIdx = (uint8_t)((sGameTextureUploadIdx + 1) % VITA_GAME_TEXTURE_RING);
    glBindTexture(GL_TEXTURE_2D, sGameTextureRing[sGameTextureUploadIdx]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAME_LINE_WIDTH, DISPLAY_HEIGHT, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sConvertedFallback);

    sPerfTotal[PERF_DRAWFRAME] += perfT1 - perfT0;
    sPerfTotal[PERF_UPLOAD] += PerfNowMs() - perfT1;
    REG_VCOUNT = 161; // prep for being in VBlank period
}

int32_t DoMain(void *data)
{
#ifdef PLATFORM_VITA
    VitaPlatform_PinCurrentThread("AgbMain", 1);
#endif
    // SoftReset() comes back here from wherever the game called it, and the game starts over.
    setjmp(sGameResetJump);
    sGameResetJumpValid = true;
    AgbMain();
    return 0;
}

void VBlankIntrWait(void)
{
    static double sLogicStart;
    if (sLogicStart != 0)
    {
        const double logicMs = PerfNowMs() - sLogicStart;
        sPerfTotal[PERF_LOGIC] += logicMs;
        VitaPerf_Core1LogicDone(logicMs);
    }

    // 1. Dispatch Line 150 VCount match interrupt on Core 1 (triggers m4aSoundVSync for 48 kHz audio mixing)
    GbaEngine_StepDirectHardwareEvents();

    // 2. Execute VBlank interrupt & m4aSoundMain on Core 1 (USER_1)
    {
        const double perfVblankStart = PerfNowMs();
        REG_DISPSTAT |= INTR_FLAG_VBLANK;
        RunDMAs(DMA_HBLANK);
        if (REG_IE & INTR_FLAG_VBLANK)
        {
            gIntrTable[4]();
        }
        REG_DISPSTAT &= ~INTR_FLAG_VBLANK;
        const double vblankMs = PerfNowMs() - perfVblankStart;
        sPerfTotal[PERF_VBLANK] += vblankMs;
        VitaPerf_Core1VBlankDone(vblankMs);
    }

    // New UI: input, screens and actions between two game frames, then its draw list for Core 0
    Ui_Tick();

    // 3. Publish atomic snapshot of active entities to Core 0 (Zero-Copy Double Buffer)
    VoxelEntities_PublishSnapshot();

    // 4. Signal Core 0 that a new frame state is ready for GPU rendering. The profiler counts
    // the game frame here, where Core 0 can see it, not at the start of the VBlank work.
    SDL_AtomicSet(&isFrameAvailable, 1);
    VitaPerf_Core1LogicTick();
    if (sFrameReadySem != NULL)
        SDL_SemPost(sFrameReadySem);

    // 5. Autonomous Cadence Governance (Deterministic 60.00 Hz Master Clock with Fractional Audio Drift Skew)
    const double waitStart = PerfNowMs();
    if (sFastForward || speedUp)
    {
        static uint64_t sLastFastTick = 0;
        const uint64_t now = SDL_GetPerformanceCounter();
        const uint64_t freq = SDL_GetPerformanceFrequency();
        const double speed = (timeScale > 1.0) ? timeScale : 2.0;
        const uint64_t targetTicks = (uint64_t)((double)freq / (60.0 * speed));
        if (sLastFastTick != 0)
        {
            const uint64_t elapsed = now - sLastFastTick;
            if (elapsed < targetTicks)
            {
                const uint32_t delayUs = (uint32_t)(((targetTicks - elapsed) * 1000000ULL) / freq);
                if (delayUs > 50)
                    VitaPlatform_DelayMicroseconds(delayUs);
            }
        }
        sLastFastTick = SDL_GetPerformanceCounter();
    }
    else
    {
        static uint64_t sLastNormalTick = 0;
        const uint64_t freq = SDL_GetPerformanceFrequency();
        uint64_t targetFrameTicks = freq / 60; // Exact 16.666 ms cadence (60.00 Hz)

        // Fractional Audio PLL: Compensate for microscopic clock drift between CPU timer and SceAudioOut DAC
        if (sdlAudioDevice != 0)
        {
            const uint32_t queued = SDL_GetQueuedAudioSize(sdlAudioDevice);
            const uint32_t targetQueue = 3 * (VITA_AUDIO_RATE / 60) * 2 * sizeof(float); // 19,200 bytes (~50 ms)
            const uint32_t frameBytes = (VITA_AUDIO_RATE / 60) * 2 * sizeof(float);       // 6,400 bytes (~16.6 ms)

            if (queued > targetQueue + frameBytes)
            {
                // Queue slightly ahead: apply transparent +500 us skew (~58 Hz) to gently lower depth without stalling
                targetFrameTicks += (freq * 500ULL) / 1000000ULL;
            }
            else if (queued + frameBytes < targetQueue)
            {
                // Queue slightly behind: apply transparent -500 us skew (~62 Hz) to prevent buffer underrun
                targetFrameTicks -= (freq * 500ULL) / 1000000ULL;
            }
        }

        const uint64_t now = SDL_GetPerformanceCounter();
        if (sLastNormalTick != 0)
        {
            const uint64_t elapsed = now - sLastNormalTick;
            if (elapsed < targetFrameTicks)
            {
                const uint32_t delayUs = (uint32_t)(((targetFrameTicks - elapsed) * 1000000ULL) / freq);
                if (delayUs > 600)
                    VitaPlatform_DelayMicroseconds(delayUs - 300);
                while (SDL_GetPerformanceCounter() - sLastNormalTick < targetFrameTicks)
                {
                    // Exact microsecond spin-wait
                }
            }
        }
        sLastNormalTick = SDL_GetPerformanceCounter();
    }

    const double waitMs = PerfNowMs() - waitStart;
    VitaPerf_Core1WaitDone(waitMs);

    sLogicStart = PerfNowMs();
    if (SDL_AtomicGet(&sResetRequested))
    {
        SDL_AtomicSet(&sResetRequested, 0);
        SoftReset(RESET_ALL);
    }
}

u8 BinToBcd(u8 bin)
{
    int32_t placeCounter = 1;
    u8 out = 0;
    do
    {
        out |= (bin % 10) * placeCounter;
        placeCounter *= 16;
    }
    while ((bin /= 10) > 0);

    return out;
}

void Platform_GetStatus(struct SiiRtcInfo *rtc)
{
    rtc->status = internalClock.status;
}

void Platform_SetStatus(struct SiiRtcInfo *rtc)
{
    internalClock.status = rtc->status;
}

static void UpdateInternalClock(void)
{
    // The console's configured local time (newlib's localtime() reported the wrong hour in Vita3K).
    struct VitaLocalTime now;

    if (VitaPlatform_GetLocalTime(&now) == 0)
    {
        internalClock.year = BinToBcd(now.year - 2000);
        internalClock.month = BinToBcd(now.month);
        internalClock.day = BinToBcd(now.day);
        internalClock.dayOfWeek = BinToBcd(now.dayOfWeek);
        internalClock.hour = BinToBcd(now.hour);
        internalClock.minute = BinToBcd(now.minute);
        internalClock.second = BinToBcd(now.second);
        return;
    }
    time_t rawTime = time(NULL);
    struct tm *time = localtime(&rawTime);

    internalClock.year = BinToBcd(time->tm_year - 100);
    internalClock.month = BinToBcd(time->tm_mon + 1);
    internalClock.day = BinToBcd(time->tm_mday);
    internalClock.dayOfWeek = BinToBcd(time->tm_wday);
    internalClock.hour = BinToBcd(time->tm_hour);
    internalClock.minute = BinToBcd(time->tm_min);
    internalClock.second = BinToBcd(time->tm_sec);
}

void Platform_GetDateTime(struct SiiRtcInfo *rtc)
{
    UpdateInternalClock();

    rtc->year = internalClock.year;
    rtc->month = internalClock.month;
    rtc->day = internalClock.day;
    rtc->dayOfWeek = internalClock.dayOfWeek;
    rtc->hour = internalClock.hour;
    rtc->minute = internalClock.minute;
    rtc->second = internalClock.second;
    DBGPRINTF("GetDateTime: %d-%02d-%02d %02d:%02d:%02d\n", ConvertBcdToBinary(rtc->year),
                                                         ConvertBcdToBinary(rtc->month),
                                                         ConvertBcdToBinary(rtc->day),
                                                         ConvertBcdToBinary(rtc->hour),
                                                         ConvertBcdToBinary(rtc->minute),
                                                         ConvertBcdToBinary(rtc->second));
}

void Platform_SetDateTime(struct SiiRtcInfo *rtc)
{
    internalClock.month = rtc->month;
    internalClock.day = rtc->day;
    internalClock.dayOfWeek = rtc->dayOfWeek;
    internalClock.hour = rtc->hour;
    internalClock.minute = rtc->minute;
    internalClock.second = rtc->second;
}

void Platform_GetTime(struct SiiRtcInfo *rtc)
{
    UpdateInternalClock();

    rtc->hour = internalClock.hour;
    rtc->minute = internalClock.minute;
    rtc->second = internalClock.second;
    DBGPRINTF("GetTime: %02d:%02d:%02d\n", ConvertBcdToBinary(rtc->hour),
                                        ConvertBcdToBinary(rtc->minute),
                                        ConvertBcdToBinary(rtc->second));
}

void Platform_SetTime(struct SiiRtcInfo *rtc)
{
    internalClock.hour = rtc->hour;
    internalClock.minute = rtc->minute;
    internalClock.second = rtc->second;
}

void Platform_SetAlarm(u8 *alarmData)
{
    // TODO
}

void SoftReset(u32 resetFlags)
{
    // Power cycle: all game memory back to its boot state, the save file (which the port menu
    // may have switched) read again, and the game thread restarted. Runs on the game thread;
    // the main thread doesn't touch game memory until the game waits for VBlank again.
    if (sGameResetJumpValid && VitaReset_RestoreBootSnapshot())
    {
        CloseSaveFile();
        VitaUI_GetSavePath(sSavePath, sizeof(sSavePath));
        ReadSaveFile(sSavePath);
        SDL_ClearQueuedAudio(sdlAudioDevice);
        longjmp(sGameResetJump, 1);
    }
    VITA_LOG("Soft Reset called. Exiting.\n");
    VitaPlatform_Exit();
}

