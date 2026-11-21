#include <stdint.h>
#include "vita_perf.h"
#include "vita_platform.h"
#include "platform/vita_extras.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#ifdef PLATFORM_VITA
#include <malloc.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/rtc.h>
#include <vitaGL.h>
extern int32_t _newlib_heap_size_user;
#else
#include <SDL.h>
#endif

#define PERF_SUMMARY_PATH VITA_DATA_DIR "perf_summary.txt"
#define PERF_CSV_PATH     VITA_DATA_DIR "perf_frames.csv"

// High-resolution timestamp in milliseconds
double VitaPerf_NowMs(void)
{
#ifdef PLATFORM_VITA
    return (double)sceKernelGetProcessTimeWide() / 1000.0;
#else
    return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency();
#endif
}

#ifdef VITA_TELEMETRY

// A summary block is written after this many 3D frames, or after PERF_WINDOW_MAX_MS
// (so 2D-only stretches don't make one block last a minute).
#define SAMPLE_WINDOW_FRAMES 120
#define PERF_WINDOW_MAX_MS 5000.0
// Frame times kept per window for the percentiles.
#define PERF_WINDOW_FRAME_CAP 1024

// Double-buffered static file streams to eliminate disk contention
static FILE *sSummaryFile = NULL;
static FILE *sCsvFile = NULL;
static char sSummaryBuf[32768];
static char sCsvBuf[32768];

// Per-frame temporary timers for Core 0
static double sFrameStartMs = 0.0;
static double sAtlasStartMs = 0.0;
static double sCullStartMs = 0.0;
static double sBuildStartMs = 0.0;
static double sDraw3DStartMs = 0.0;
static double sUiScanStartMs = 0.0;
static double sUiUpStartMs = 0.0;
static double sSwapStartMs = 0.0;
static double sSubStartMs[VITA_PERF_T_COUNT];

// Measured durations in current frame
static double sCurAtlasMs = 0.0;
static double sCurCullMs = 0.0;
static double sCurBuildMs = 0.0;
static double sCurDraw3DMs = 0.0;
static double sCurUiScanMs = 0.0;
static double sCurUiUpMs = 0.0;
static double sCurSwapMs = 0.0;
static double sCurSubMs[VITA_PERF_T_COUNT];
static uint32_t sCurCount[VITA_PERF_C_COUNT];
static uint32_t sCurEvents = 0;

static uint32_t sCurAtlasRows = 0;
static int32_t sCurCandidates = 0;
static int32_t sCurVisibleChunks = 0;
static int32_t sCurChunksBuilt = 0;
static int32_t sCurDrawCalls = 0;
static uint32_t sCurVertexCount = 0;

// Core 1 latest measurements (written by Core 1 only, sampled by Core 0)
static volatile double sCore1LogicLatest = 0.0;
static volatile double sCore1VBlankLatest = 0.0;
static volatile double sCore1WaitLatest = 0.0;
static volatile uint32_t sCore1TickCount = 0;
static uint32_t sUiCellsPeak, sUiCellsCount, sUiCellsStarved;   // Core 1 stores, Core 0 loads (relaxed atomics)

// Overall and Window Statistics
static uint32_t sTotalFrameIndex = 0;
static uint32_t sWindowFrameCount = 0;  // every Core 0 frame (2D or 3D)
static uint32_t sWindowRenderCount = 0; // 3D frames only
static uint32_t sWindowStartTicks = 0;
static double sWindowStartMs = 0.0;
static uint32_t sSampleBlockIndex = 0;
static uint32_t sWindowTargetFps = 30;
static double sWindowFrameMs[PERF_WINDOW_FRAME_CAP];
static uint32_t sWindowFrameMsCount = 0;
static uint32_t sWindowOverBudget = 0;
static uint32_t sWindowOverDouble = 0;
static uint32_t sWindowEventCount[8];
static uint32_t sWindowHoleFrames = 0; // 3D frames that showed at least one unbuilt chunk
static uint32_t sWindowHoleMax = 0;
static uint32_t sLastFrameTicks = 0;
static uint32_t sWindowTickRepeats = 0; // 3D frames that showed no new game frame
static uint32_t sWindowTickSkips = 0;   // 3D frames that came after more game frames than their share

// Whole-session totals over 3D frames, repeated in every summary block
static uint32_t sSessionFrames3D = 0;
static uint32_t sSessionOverBudget = 0;
static uint32_t sSessionOverDouble = 0;
static double sSessionMaxFrameMs = 0.0;
static size_t sSessionMinHeapFree = (size_t)-1;

// Running accumulators over a sample window
typedef struct {
    double sum;
    double min;
    double max;
} PerfMetricAccum;

static PerfMetricAccum mAtlasAnim, mCull, mBuild, mDraw3D, mUiScan, mUiUp, mSwap, mC0Active, mFrame, mOther;
static PerfMetricAccum mC1Logic, mC1VBlank, mC1Wait;
static PerfMetricAccum mVisChunks, mBuiltChunks, mDrawCalls, mVerts;
static PerfMetricAccum mSub[VITA_PERF_T_COUNT];
static double sWindowCount[VITA_PERF_C_COUNT];

static const char *const sSubNames[VITA_PERF_T_COUNT] = {
    [VITA_PERF_T_BUILD_GROUND] = "Build: ground+light",
    [VITA_PERF_T_BUILD_ATLAS_EXT] = "Build: atlas extend",
    [VITA_PERF_T_BUILD_TREES] = "Build: trees",
    [VITA_PERF_T_BUILD_BUILDINGS] = "Build: buildings",
    [VITA_PERF_T_BUILD_UPLOAD] = "Build: VBO upload",
    [VITA_PERF_T_ATLAS_COMPOSE] = "Atlas: compose CPU",
    [VITA_PERF_T_ATLAS_UPLOAD] = "Atlas: tex upload",
    [VITA_PERF_T_MAP_SHIFT] = "Map change work",
    [VITA_PERF_T_WORLD_INSTANCES] = "World instances",
    [VITA_PERF_T_ATLAS_SLOTS] = "Atlas slots",
    [VITA_PERF_T_WORKER_COMPUTE] = "Worker: compute",
    [VITA_PERF_T_WORKER_WAIT] = "Worker: Core 0 wait",
    [VITA_PERF_T_ENTITIES] = "Entities emit",
    [VITA_PERF_T_LIVE_DIGEST] = "Live map digest",
    [VITA_PERF_T_LIGHT_RAYS] = "Light: ray march",
    [VITA_PERF_T_LIGHT_CONTACT] = "Light: contact AO",
    [VITA_PERF_T_LIGHT_API] = "Light: all entries",
    [VITA_PERF_T_UI2_RENDER] = "New UI: render (C0)",
    [VITA_PERF_T_UI2_TICK] = "New UI: tick (C1)",
    [VITA_PERF_T_POST_DOF] = "Post: DoF blur",
    [VITA_PERF_T_POST_BLOOM] = "Post: Bloom",
    [VITA_PERF_T_POST_COMPOSITE] = "Post: Composite",
    [VITA_PERF_T_ENT_EMIT] = "Entities: emit",
    [VITA_PERF_T_ENT_DECODE] = "Entities: decode",
    [VITA_PERF_T_ENT_ATLAS] = "Entities: atlas wr",
    [VITA_PERF_T_ENT_PACK] = "Entities: pack",
    [VITA_PERF_T_ENT_LIGHTS] = "Entities: lights",
    [VITA_PERF_T_GPU_WORLD] = "GPU: world",
    [VITA_PERF_T_GPU_DOWN] = "GPU: downsample",
    [VITA_PERF_T_GPU_BLUR] = "GPU: wide blur",
    [VITA_PERF_T_GPU_BLOOM] = "GPU: bloom",
    [VITA_PERF_T_GPU_KAWASE] = "GPU: Kawase",
    [VITA_PERF_T_GPU_FINAL] = "GPU: final",
};

// What the frame showed and the input state (VitaPerf_SetScene, VitaPerf_SetInput), Core 0 only.
static VitaPerfScene sScene;
static bool sInputFastForward = false;
static bool sInputUiOpen = false;
// Set by VitaPerf_SetScene in this frame: frames that skip the field passes (the Fly cloud
// frames, a frozen menu backdrop) leave sScene as the last field frame saw it.
static bool sSceneFresh = false;

#ifdef VITA_GPU_PROBE
// The automatic bench: when the player stands still in the 3D field for BENCH_STILL_FRAMES, it
// measures HIGH, then HIGH with one pass switched off at a time, then MED, LOW and OFF. HIGH
// comes first because it is what is being optimised, and OFF last because switching between OFF
// and LOW turns the relief on or off, which rebuilds the atlas and the chunks (a ~0.7 s freeze).
// Each step waits for BENCH_SETTLE_FRAMES calm frames (no events, no holes), then keeps
// BENCH_MEASURE_FRAMES frames marked bench_measured. Moving, a menu, a fade, a battle, Fly or fast
// forward aborts the run; the player has to move before the next one starts.
#define BENCH_STILL_FRAMES 120u
#define BENCH_SETTLE_FRAMES 20u
#define BENCH_MEASURE_FRAMES 60u
#define BENCH_STEP_MAX_FRAMES 400u
// Step 0: HIGH. Steps 1..VITA_PERF_PASS_COUNT: HIGH without one pass. Then MED, LOW, OFF.
#define BENCH_LOWER_FIRST (1u + (uint32_t)VITA_PERF_PASS_COUNT)
#define BENCH_STEPS (BENCH_LOWER_FIRST + (uint32_t)VOXEL_HD2D_HIGH)
// Below this, in tiles, the player has not moved.
#define BENCH_STILL_EPSILON 0.01f

static uint32_t sBenchRun = 0;        // runs started this session: the CSV's bench_run
static int32_t sBenchStep = -1;       // -1: no run under way
static uint32_t sBenchStepFrames = 0; // 3D frames in this step
static uint32_t sBenchSettle = 0;     // calm frames in a row
static uint32_t sBenchMeasured = 0;   // measured frames in this step
static uint32_t sBenchStill = 0;      // frames standing still with nothing blocking a run
static bool sBenchArmed = true;       // the player moved since the last run
static VitaPerfScene sBenchLastScene;
static bool sBenchLastSceneValid = false;
static VitaPerfGpuScene sGpuScene = VITA_PERF_GPU_NONE;

static const VitaPerfTimer kGpuSceneTimer[] = {
    [VITA_PERF_GPU_NONE] = VITA_PERF_T_GPU_WORLD, // never charged
    [VITA_PERF_GPU_WORLD] = VITA_PERF_T_GPU_WORLD,
    [VITA_PERF_GPU_DOWN] = VITA_PERF_T_GPU_DOWN,
    [VITA_PERF_GPU_BLUR] = VITA_PERF_T_GPU_BLUR,
    [VITA_PERF_GPU_BLOOM] = VITA_PERF_T_GPU_BLOOM,
    [VITA_PERF_GPU_KAWASE] = VITA_PERF_T_GPU_KAWASE,
    [VITA_PERF_GPU_FINAL] = VITA_PERF_T_GPU_FINAL,
};

static const char *const kPassNames[VITA_PERF_PASS_COUNT] = {
    [VITA_PERF_PASS_FOG] = "fog",
    [VITA_PERF_PASS_MOTES] = "motes",
    [VITA_PERF_PASS_SURFACEFX] = "surfacefx",
    [VITA_PERF_PASS_WEATHER] = "weather",
    [VITA_PERF_PASS_WORLDRAYS] = "worldrays",
    [VITA_PERF_PASS_DAYNIGHT] = "daynight",
    [VITA_PERF_PASS_LAMPS] = "lamps",
    [VITA_PERF_PASS_ATMOS2D] = "atmos2d",
    [VITA_PERF_PASS_SHADOWS] = "shadows",
    [VITA_PERF_PASS_REFLECTIONS] = "reflections",
    [VITA_PERF_PASS_ENTITIES] = "entities",
    [VITA_PERF_PASS_SKYBLOOM] = "skybloom",
    [VITA_PERF_PASS_SHAFTS] = "shafts",
    [VITA_PERF_PASS_KAWASE] = "kawase",
    [VITA_PERF_PASS_BLOOM] = "bloom",
    [VITA_PERF_PASS_BLUR] = "blur",
};

static bool BenchMaskStep(const int32_t step)
{
    return step >= 1 && step < (int32_t)BENCH_LOWER_FIRST;
}

static uint32_t BenchMask(void)
{
    return BenchMaskStep(sBenchStep) ? (1u << (uint32_t)(sBenchStep - 1)) : 0u;
}

int32_t VitaPerf_BenchLevel(void)
{
    if (sBenchStep < 0)
        return -1;
    if (sBenchStep < (int32_t)BENCH_LOWER_FIRST)
        return (int32_t)VOXEL_HD2D_HIGH;
    return (int32_t)VOXEL_HD2D_HIGH - 1 - (sBenchStep - (int32_t)BENCH_LOWER_FIRST);
}

float VitaPerf_BenchProgress(void)
{
    if (sBenchStep < 0)
        return -1.0f;
    const float stepDone = (sBenchMeasured >= BENCH_MEASURE_FRAMES)
        ? 1.0f : (float)sBenchMeasured / (float)BENCH_MEASURE_FRAMES;
    return ((float)sBenchStep + stepDone) / (float)BENCH_STEPS;
}

bool VitaPerf_PassSkipped(const VitaPerfPass pass)
{
    return sBenchStep >= 0 && (uint32_t)pass < VITA_PERF_PASS_COUNT && (BenchMask() & (1u << pass)) != 0u;
}

void VitaPerf_GpuScene(const VitaPerfGpuScene next)
{
    const double start = VitaPerf_NowMs();
#ifdef PLATFORM_VITA
    glFinish();
#endif
    const double waited = VitaPerf_NowMs() - start;
    if (sGpuScene != VITA_PERF_GPU_NONE)
        sCurSubMs[kGpuSceneTimer[sGpuScene]] += waited;
    sGpuScene = next;
    if (next != VITA_PERF_GPU_NONE)
        sCurCount[VITA_PERF_C_GXM_SCENES]++;
}
#endif // VITA_GPU_PROBE

// Letters of the CSV "events" column, by bit
static const char sEventLetters[] = "MXLSWFAC";

static void ResetAccum(PerfMetricAccum *const a)
{
    a->sum = 0.0;
    a->min = 1e9;
    a->max = 0.0;
}

static void AddAccum(PerfMetricAccum *const a, const double val)
{
    a->sum += val;
    if (val < a->min) a->min = val;
    if (val > a->max) a->max = val;
}

static double MinOrZero(const PerfMetricAccum *const a)
{
    return (a->min > a->max) ? 0.0 : a->min;
}

static void ResetWindowAccums(void)
{
    ResetAccum(&mAtlasAnim);
    ResetAccum(&mCull);
    ResetAccum(&mBuild);
    ResetAccum(&mDraw3D);
    ResetAccum(&mUiScan);
    ResetAccum(&mUiUp);
    ResetAccum(&mSwap);
    ResetAccum(&mC0Active);
    ResetAccum(&mFrame);
    ResetAccum(&mOther);
    ResetAccum(&mC1Logic);
    ResetAccum(&mC1VBlank);
    ResetAccum(&mC1Wait);
    ResetAccum(&mVisChunks);
    ResetAccum(&mBuiltChunks);
    ResetAccum(&mDrawCalls);
    ResetAccum(&mVerts);
    for (uint32_t i = 0; i < VITA_PERF_T_COUNT; i++)
        ResetAccum(&mSub[i]);
    memset(sWindowCount, 0, sizeof(sWindowCount));
    memset(sWindowEventCount, 0, sizeof(sWindowEventCount));
    sWindowHoleFrames = 0;
    sWindowHoleMax = 0;
    sWindowTickRepeats = 0;
    sWindowTickSkips = 0;

    sWindowFrameCount = 0;
    sWindowRenderCount = 0;
    sWindowFrameMsCount = 0;
    sWindowOverBudget = 0;
    sWindowOverDouble = 0;
    sWindowStartTicks = sCore1TickCount;
    sWindowStartMs = VitaPerf_NowMs();
}

void VitaPerf_Init(void)
{
    sSummaryFile = fopen(PERF_SUMMARY_PATH, "w");
    if (sSummaryFile != NULL)
    {
        setvbuf(sSummaryFile, sSummaryBuf, _IOFBF, sizeof(sSummaryBuf));
        fprintf(sSummaryFile, "================================================================================\n");
        fprintf(sSummaryFile, "  POKÉMON EMERALD 3DS (PS VITA PORT) — TELEMETRY & HARDWARE PROFILER LOG\n");
        fprintf(sSummaryFile, "  CPU Frequency: %d MHz | Platform: Sony PlayStation Vita (ARM Cortex-A9)\n", VitaPlatform_GetCpuMHz());
        fprintf(sSummaryFile, "  frame = Core 0 wall time from frame start to after the swap; other = frame minus\n");
        fprintf(sSummaryFile, "  the named stages. Events: M map reset, X map crossing, L live tile, S sun moved,\n");
        fprintf(sSummaryFile, "  W warm-up, F faded (sync builds), A atlas extended, C atlas slot recomposed.\n");
        fprintf(sSummaryFile, "================================================================================\n\n");
        fflush(sSummaryFile);
    }

    sCsvFile = fopen(PERF_CSV_PATH, "w");
    if (sCsvFile != NULL)
    {
        setvbuf(sCsvFile, sCsvBuf, _IOFBF, sizeof(sCsvBuf));
        fprintf(sCsvFile, "frame,render3d,target_fps,frame_ms,c0_active_ms,other_ms,"
                          "atlas_ms,atlas_compose_ms,atlas_upload_ms,atlas_uploads,"
                          "cull_ms,build_ms,build_ground_ms,build_atlas_ext_ms,build_trees_ms,build_bldg_ms,build_upload_ms,"
                          "light_rays,build_verts,"
                          "draw3d_ms,ui_scan_ms,ui_up_ms,swap_ms,entities_ms,digest_ms,shift_ms,"
                          "c1_logic_ms,c1_vblank_ms,c1_pacing_ms,chunks_vis,chunks_built,draw_calls,verts,events,holes,"
                          "light_ray_ms,light_contact_ms,light_steps,light_cells,light_blocks,light_api_ms,logic_ticks,world_ms,atlas_slots_ms,"
                          "worker_built,worker_dropped,worker_compute_ms,worker_wait_ms,"
                          "ui2_render_ms,ui2_tick_ms,ui2_quads,"
                          "ent_emit_ms,ent_decode_ms,ent_atlas_ms,ent_pack_ms,ent_lights_ms,"
                          "hd2d_level,dof_on,fast_forward,ui_open,map_group,map_num,indoor,weather,daylight,lamps,"
                          "in_battle,faded,"
                          "gpu_world_ms,gpu_down_ms,gpu_blur_ms,gpu_bloom_ms,gpu_kawase_ms,gpu_final_ms,gpu_total_ms,"
                          "gxm_scenes,bench_run,bench_step,bench_level,probe_mask,bench_measured\n");
        fflush(sCsvFile);
    }

    sTotalFrameIndex = 0;
    sSampleBlockIndex = 0;
    ResetWindowAccums();
}

void VitaPerf_Shutdown(void)
{
    if (sSummaryFile != NULL)
    {
        fprintf(sSummaryFile, "\n[TELEMETRY ENDED] Total frames logged: %u\n", sTotalFrameIndex);
        fflush(sSummaryFile);
        fclose(sSummaryFile);
        sSummaryFile = NULL;
    }
    if (sCsvFile != NULL)
    {
        fflush(sCsvFile);
        fclose(sCsvFile);
        sCsvFile = NULL;
    }
}

// Core 1 Callbacks. Each variable has Core 1 as its only writer.
void VitaPerf_Core1LogicTick(void)
{
    sCore1TickCount++;
}

void VitaPerf_Core1LogicDone(const double logicMs)
{
    sCore1LogicLatest = logicMs;
}

void VitaPerf_Core1VBlankDone(const double vblankMs)
{
    sCore1VBlankLatest = vblankMs;
}

void VitaPerf_Core1WaitDone(const double waitMs)
{
    sCore1WaitLatest = waitMs;
}

void VitaPerf_Core1UiCells(const uint32_t peakLive, const uint32_t count, const uint32_t starved)
{
    __atomic_store_n(&sUiCellsPeak, peakLive, __ATOMIC_RELAXED);
    __atomic_store_n(&sUiCellsCount, count, __ATOMIC_RELAXED);
    __atomic_store_n(&sUiCellsStarved, starved, __ATOMIC_RELAXED);
}

// Core 0 Stage Markers
void VitaPerf_BeginFrame(void)
{
    sSceneFresh = false;
    sFrameStartMs = VitaPerf_NowMs();
    sDraw3DStartMs = 0.0;
    sCurAtlasMs = 0.0;
    sCurCullMs = 0.0;
    sCurBuildMs = 0.0;
    sCurDraw3DMs = 0.0;
    sCurUiScanMs = 0.0;
    sCurUiUpMs = 0.0;
    sCurSwapMs = 0.0;
    memset(sCurSubMs, 0, sizeof(sCurSubMs));
    memset(sCurCount, 0, sizeof(sCurCount));
    sCurEvents = 0;

    sCurAtlasRows = 0;
    sCurCandidates = 0;
    sCurVisibleChunks = 0;
    sCurChunksBuilt = 0;
    sCurDrawCalls = 0;
    sCurVertexCount = 0;
}

void VitaPerf_MarkAtlasAnimStart(void)
{
    sAtlasStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkAtlasAnimEnd(const uint32_t rowsUpdated)
{
    sCurAtlasMs = VitaPerf_NowMs() - sAtlasStartMs;
    sCurAtlasRows = rowsUpdated;
}

void VitaPerf_MarkCullStart(void)
{
    sCullStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkCullEnd(const int32_t candidates, const int32_t visible)
{
    const double elapsed = VitaPerf_NowMs() - sCullStartMs;
    sCurCullMs = (elapsed > sCurBuildMs) ? (elapsed - sCurBuildMs) : 0.0;
    sCurCandidates = candidates;
    sCurVisibleChunks = visible;
}

void VitaPerf_MarkBuildStart(void)
{
    sBuildStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkBuildEnd(const int32_t chunksBuilt)
{
    sCurBuildMs += (VitaPerf_NowMs() - sBuildStartMs);
    sCurChunksBuilt += chunksBuilt;
}

void VitaPerf_MarkDraw3DStart(void)
{
    sDraw3DStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkDraw3DEnd(const int32_t drawCalls, const uint32_t vertexCount)
{
    if (sDraw3DStartMs > 0.0)
    {
        sCurDraw3DMs = VitaPerf_NowMs() - sDraw3DStartMs;
        sDraw3DStartMs = 0.0;
    }
    else
    {
        sCurDraw3DMs = 0.0;
    }
    sCurDrawCalls = drawCalls;
    sCurVertexCount = vertexCount;
}

void VitaPerf_MarkUiScanlinesStart(void)
{
    sUiScanStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkUiScanlinesEnd(void)
{
    sCurUiScanMs = VitaPerf_NowMs() - sUiScanStartMs;
}

void VitaPerf_MarkUiUploadStart(void)
{
    sUiUpStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkUiUploadEnd(void)
{
    sCurUiUpMs = VitaPerf_NowMs() - sUiUpStartMs;
}

void VitaPerf_MarkSwapStart(void)
{
    sSwapStartMs = VitaPerf_NowMs();
}

void VitaPerf_MarkSwapEnd(void)
{
    sCurSwapMs = VitaPerf_NowMs() - sSwapStartMs;
}

void VitaPerf_SubBegin(const VitaPerfTimer timer)
{
    if ((uint32_t)timer < VITA_PERF_T_COUNT)
        sSubStartMs[timer] = VitaPerf_NowMs();
}

void VitaPerf_SubEnd(const VitaPerfTimer timer)
{
    if ((uint32_t)timer < VITA_PERF_T_COUNT)
        sCurSubMs[timer] += VitaPerf_NowMs() - sSubStartMs[timer];
}

void VitaPerf_AddTime(const VitaPerfTimer timer, const double ms)
{
    if ((uint32_t)timer < VITA_PERF_T_COUNT)
        sCurSubMs[timer] += ms;
}

void VitaPerf_AddCount(const VitaPerfCounter counter, const uint32_t amount)
{
    if ((uint32_t)counter < VITA_PERF_C_COUNT)
        sCurCount[counter] += amount;
}

void VitaPerf_MarkEvent(const VitaPerfEvent event)
{
    sCurEvents |= (uint32_t)event;
}

void VitaPerf_SetScene(const VitaPerfScene *const scene)
{
    if (scene != NULL)
    {
        sScene = *scene;
        sSceneFresh = true;
    }
}

void VitaPerf_SetInput(const bool fastForward, const bool uiOpen)
{
    sInputFastForward = fastForward;
    sInputUiOpen = uiOpen;
}

#ifdef VITA_GPU_PROBE
static void BenchLog(const char *const what)
{
    if (sSummaryFile == NULL)
        return;
    fprintf(sSummaryFile, "[BENCH] run %u %s at frame %u: map %d.%d (%.2f, %.2f)%s, weather %d, daylight %.2f%s\n",
            sBenchRun, what, sTotalFrameIndex, sScene.mapGroup, sScene.mapNum, (double)sScene.playerX,
            (double)sScene.playerZ, sScene.indoor ? " indoor" : "", sScene.weather, (double)sScene.daylight,
            sScene.lamps ? ", lamps" : "");
}

static void BenchStop(const char *const why)
{
    BenchLog(why);
    sBenchStep = -1;
    sBenchArmed = false;
    sBenchStill = 0;
}

static void BenchStartStep(const int32_t step)
{
    sBenchStep = step;
    sBenchStepFrames = 0;
    sBenchSettle = 0;
    sBenchMeasured = 0;
}

// Whether this 3D frame counts as measured: a run under way, and BENCH_SETTLE_FRAMES calm frames
// before it in the same step. Called before the frame's CSV line.
static bool BenchFrameMeasured(void)
{
    if (sBenchStep < 0)
        return false;
    const bool calm = (sCurEvents == 0u) && (sCurCount[VITA_PERF_C_HOLES] == 0u);
    sBenchSettle = calm ? sBenchSettle + 1u : 0u;
    return sBenchSettle > BENCH_SETTLE_FRAMES;
}

// After the frame's CSV line: start, advance or abort the run for the next frame.
static void BenchAdvance(const bool measured)
{
    const bool moved = sBenchLastSceneValid
        && (fabsf(sScene.playerX - sBenchLastScene.playerX) > BENCH_STILL_EPSILON
            || fabsf(sScene.playerZ - sBenchLastScene.playerZ) > BENCH_STILL_EPSILON
            || sScene.mapGroup != sBenchLastScene.mapGroup || sScene.mapNum != sBenchLastScene.mapNum);
    const bool blocked = !sSceneFresh || sInputFastForward || sInputUiOpen || sScene.faded || sScene.inBattle;
    sBenchLastScene = sScene;
    sBenchLastSceneValid = true;

    if (sBenchStep >= 0)
    {
        if (moved || blocked)
        {
            BenchStop(moved ? "aborted (the player moved)" : "aborted (menu, fade, battle, Fly or fast forward)");
            return;
        }
        sBenchStepFrames++;
        if (measured)
            sBenchMeasured++;
        if (sBenchMeasured < BENCH_MEASURE_FRAMES && sBenchStepFrames < BENCH_STEP_MAX_FRAMES)
            return;
        if (sBenchMeasured < BENCH_MEASURE_FRAMES && sSummaryFile != NULL)
            fprintf(sSummaryFile, "[BENCH] run %u step %d never settled (%u measured frames)\n",
                    sBenchRun, sBenchStep, sBenchMeasured);
        if ((uint32_t)sBenchStep + 1u >= BENCH_STEPS)
        {
            BenchStop("finished");
            return;
        }
        BenchStartStep(sBenchStep + 1);
        if (sSummaryFile != NULL && BenchMaskStep(sBenchStep))
            fprintf(sSummaryFile, "[BENCH] run %u step %d: HIGH without %s\n", sBenchRun, sBenchStep,
                    kPassNames[sBenchStep - 1]);
        else if (sSummaryFile != NULL)
            fprintf(sSummaryFile, "[BENCH] run %u step %d: level %d\n", sBenchRun, sBenchStep,
                    (int)VitaPerf_BenchLevel());
        return;
    }

    if (moved)
    {
        sBenchArmed = true;
        sBenchStill = 0;
        return;
    }
    if (blocked || !sBenchArmed)
    {
        sBenchStill = 0;
        return;
    }
    if (++sBenchStill < BENCH_STILL_FRAMES)
        return;
    sBenchRun++;
    BenchStartStep(0);
    BenchLog("started");
}
#endif // VITA_GPU_PROBE

void VitaPerf_LogMemory(const char *stage)
{
    if (sSummaryFile == NULL)
        return;
#ifdef PLATFORM_VITA
    const struct mallinfo info = mallinfo();
    fprintf(sSummaryFile, "[MEMORY %s] KB free / total: newlib %u / %u | vitaGL VRAM %u / %u | RAM %u / %u | PHYCONT %u / %u\n",
            stage,
            (uint32_t)(((size_t)_newlib_heap_size_user - (size_t)info.uordblks) / 1024u),
            (uint32_t)((size_t)_newlib_heap_size_user / 1024u),
            (uint32_t)(vglMemFree(VGL_MEM_VRAM) / 1024u), (uint32_t)(vglMemTotal(VGL_MEM_VRAM) / 1024u),
            (uint32_t)(vglMemFree(VGL_MEM_RAM) / 1024u), (uint32_t)(vglMemTotal(VGL_MEM_RAM) / 1024u),
            (uint32_t)(vglMemFree(VGL_MEM_PHYCONT) / 1024u), (uint32_t)(vglMemTotal(VGL_MEM_PHYCONT) / 1024u));
#else
    fprintf(sSummaryFile, "[MEMORY %s] not measured on this platform\n", stage);
#endif
    fflush(sSummaryFile);
}

static int32_t CompareDouble(const void *a, const void *b)
{
    const double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double Percentile(const double *sorted, const uint32_t count, const double fraction)
{
    if (count == 0)
        return 0.0;
    uint32_t index = (uint32_t)(fraction * (double)(count - 1) + 0.5);
    return sorted[index < count ? index : count - 1];
}

static void WriteSummaryBlock(void)
{
    sSampleBlockIndex++;
    const double now = VitaPerf_NowMs();
    const double windowElapsedMs = now - sWindowStartMs;
    const double windowSec = windowElapsedMs / 1000.0;
    const uint32_t logicTicks = sCore1TickCount - sWindowStartTicks;

    const double renderFps = (windowSec > 0.0) ? ((double)sWindowRenderCount / windowSec) : 0.0;
    const double logicFps  = (windowSec > 0.0) ? ((double)logicTicks / windowSec) : 0.0;
    const double c0Load    = (windowElapsedMs > 0.0) ? (mFrame.sum / windowElapsedMs) * 100.0 : 0.0;

    const double avgFrames = (sWindowRenderCount > 0) ? (double)sWindowRenderCount : 1.0;
    // Core 1 values are sampled once per Core 0 frame (2D or 3D).
    const double avgSamples = (sWindowFrameCount > 0) ? (double)sWindowFrameCount : 1.0;
    const double c1Load = (avgSamples > 0.0) ? ((mC1Logic.sum + mC1VBlank.sum) / avgSamples) * logicFps / 10.0 : 0.0;

    qsort(sWindowFrameMs, sWindowFrameMsCount, sizeof(sWindowFrameMs[0]), CompareDouble);
    const double p50 = Percentile(sWindowFrameMs, sWindowFrameMsCount, 0.50);
    const double p95 = Percentile(sWindowFrameMs, sWindowFrameMsCount, 0.95);
    const double p99 = Percentile(sWindowFrameMs, sWindowFrameMsCount, 0.99);
    const double budgetMs = 1000.0 / (double)sWindowTargetFps;
    // Chunks built on Core 0 and on the worker: the per-chunk lines cover both.
    const double builtChunks = mBuiltChunks.sum + sWindowCount[VITA_PERF_C_WORKER_BUILT];

    fprintf(sSummaryFile, "================================================================================\n");
    fprintf(sSummaryFile, "[SAMPLE #%u] Frames %u-%u (Duration: %.2f s, 3D frames: %u)\n",
            sSampleBlockIndex, sTotalFrameIndex - sWindowFrameCount, sTotalFrameIndex, windowSec, sWindowRenderCount);
    fprintf(sSummaryFile, "  FPS: Render %.1f (Target: %u) | Logic %.1f Hz (Target: 60)\n", renderFps, sWindowTargetFps, logicFps);
    fprintf(sSummaryFile, "  CPU LOAD: Core 0 (Render) = %.1f%% | Core 1 (GBA) = %.1f%%\n", c0Load, c1Load);
    fprintf(sSummaryFile, "  FRAME (3D): p50 %.2f | p95 %.2f | p99 %.2f | max %.2f ms | over %.1f ms: %u | over %.1f ms: %u\n",
            p50, p95, p99, mFrame.max, budgetMs, sWindowOverBudget, budgetMs * 2.0, sWindowOverDouble);
    fprintf(sSummaryFile, "  HOLES (unbuilt chunks in view): %u frames, max %u in one frame\n",
            sWindowHoleFrames, sWindowHoleMax);
    fprintf(sSummaryFile, "  WORKER (Core 2): %.0f chunks built, %.0f dropped | compute %.2f ms total | %.0f frames overlapped a job, %.0f held back (atlas extension) | Core 0 waited %.2f ms total, max %.2f ms\n",
            sWindowCount[VITA_PERF_C_WORKER_BUILT], sWindowCount[VITA_PERF_C_WORKER_DROPPED],
            mSub[VITA_PERF_T_WORKER_COMPUTE].sum, sWindowCount[VITA_PERF_C_WORKER_OVERLAP],
            sWindowCount[VITA_PERF_C_WORKER_HELD],
            mSub[VITA_PERF_T_WORKER_WAIT].sum, mSub[VITA_PERF_T_WORKER_WAIT].max);
    fprintf(sSummaryFile, "  PACING: %u frames repeated a game frame, %u frames skipped game frames\n",
            sWindowTickRepeats, sWindowTickSkips);
    fprintf(sSummaryFile, "  UI SPRITE CELLS (since boot): peak %u of %u drawn at once, %u sprites skipped for lack of a cell\n",
            __atomic_load_n(&sUiCellsPeak, __ATOMIC_RELAXED), __atomic_load_n(&sUiCellsCount, __ATOMIC_RELAXED),
            __atomic_load_n(&sUiCellsStarved, __ATOMIC_RELAXED));
    fprintf(sSummaryFile, "  EVENTS:");
    for (uint32_t i = 0; i < sizeof(sEventLetters) - 1; i++)
        fprintf(sSummaryFile, " %c=%u", sEventLetters[i], sWindowEventCount[i]);
    fprintf(sSummaryFile, "\n");
    fprintf(sSummaryFile, "--------------------------------------------------------------------------------\n");
    fprintf(sSummaryFile, "  CORE 0 BREAKDOWN (Render Thread, per 3D frame)\n");
    fprintf(sSummaryFile, "    - Atlas Anim Upload:  avg %5.2f ms (min %5.2f, max %5.2f)\n", mAtlasAnim.sum / avgFrames, MinOrZero(&mAtlasAnim), mAtlasAnim.max);
    fprintf(sSummaryFile, "    - Frustum Culling:    avg %5.2f ms (min %5.2f, max %5.2f)\n", mCull.sum / avgFrames, MinOrZero(&mCull), mCull.max);
    fprintf(sSummaryFile, "    - Chunk Mesh Build:   avg %5.2f ms (min %5.2f, max %5.2f) [total built: %.0f]\n", mBuild.sum / avgFrames, MinOrZero(&mBuild), mBuild.max, builtChunks);
    fprintf(sSummaryFile, "    - 3D Draw Calls (P1-6):avg %5.2f ms (min %5.2f, max %5.2f) [calls: %.1f, verts: %.0f]\n", mDraw3D.sum / avgFrames, MinOrZero(&mDraw3D), mDraw3D.max, mDrawCalls.sum / avgFrames, mVerts.sum / avgFrames);
    fprintf(sSummaryFile, "    - UI GBA Scanlines:   avg %5.2f ms (min %5.2f, max %5.2f)\n", mUiScan.sum / avgFrames, MinOrZero(&mUiScan), mUiScan.max);
    fprintf(sSummaryFile, "    - UI Texture Upload:  avg %5.2f ms (min %5.2f, max %5.2f)\n", mUiUp.sum / avgFrames, MinOrZero(&mUiUp), mUiUp.max);
    fprintf(sSummaryFile, "    - vglSwapBuffers:     avg %5.2f ms (min %5.2f, max %5.2f)\n", mSwap.sum / avgFrames, MinOrZero(&mSwap), mSwap.max);
    fprintf(sSummaryFile, "    - Other (unnamed):    avg %5.2f ms (min %5.2f, max %5.2f)\n", mOther.sum / avgFrames, MinOrZero(&mOther), mOther.max);
    fprintf(sSummaryFile, "    * Named stages:       avg %5.2f ms (min %5.2f, max %5.2f)\n", mC0Active.sum / avgFrames, MinOrZero(&mC0Active), mC0Active.max);
    fprintf(sSummaryFile, "    * Whole frame:        avg %5.2f ms (min %5.2f, max %5.2f)\n", mFrame.sum / avgFrames, MinOrZero(&mFrame), mFrame.max);
    fprintf(sSummaryFile, "  SUB-TIMERS (window total / max in one frame)\n");
    for (uint32_t i = 0; i < VITA_PERF_T_COUNT; i++)
        fprintf(sSummaryFile, "    - %-20s total %8.2f ms, max %6.2f ms\n", sSubNames[i], mSub[i].sum, mSub[i].max);
    if (builtChunks > 0.0)
    {
        fprintf(sSummaryFile, "  PER CHUNK: %.2f ms | ground+light %.2f | trees %.2f | buildings %.2f | upload %.2f | %.0f rays | %.0f verts\n",
                mBuild.sum / builtChunks,
                mSub[VITA_PERF_T_BUILD_GROUND].sum / builtChunks,
                mSub[VITA_PERF_T_BUILD_TREES].sum / builtChunks,
                mSub[VITA_PERF_T_BUILD_BUILDINGS].sum / builtChunks,
                mSub[VITA_PERF_T_BUILD_UPLOAD].sum / builtChunks,
                sWindowCount[VITA_PERF_C_LIGHT_RAYS] / builtChunks,
                sWindowCount[VITA_PERF_C_BUILD_VERTS] / builtChunks);
        fprintf(sSummaryFile, "  LIGHT PER CHUNK: all entries %.2f ms (of it rays %.2f ms, contact AO %.2f ms) | %.0f steps | %.0f cells classified | %.0f blocks\n",
                mSub[VITA_PERF_T_LIGHT_API].sum / builtChunks,
                mSub[VITA_PERF_T_LIGHT_RAYS].sum / builtChunks,
                mSub[VITA_PERF_T_LIGHT_CONTACT].sum / builtChunks,
                sWindowCount[VITA_PERF_C_LIGHT_STEPS] / builtChunks,
                sWindowCount[VITA_PERF_C_LIGHT_CELLS] / builtChunks,
                sWindowCount[VITA_PERF_C_LIGHT_BLOCKS] / builtChunks);
    }
    if (sWindowCount[VITA_PERF_C_ATLAS_UPLOADS] > 0.0)
    {
        fprintf(sSummaryFile, "  ATLAS: %.0f uploads | compose %.3f ms/upload | upload %.3f ms/upload\n",
                sWindowCount[VITA_PERF_C_ATLAS_UPLOADS],
                mSub[VITA_PERF_T_ATLAS_COMPOSE].sum / sWindowCount[VITA_PERF_C_ATLAS_UPLOADS],
                mSub[VITA_PERF_T_ATLAS_UPLOAD].sum / sWindowCount[VITA_PERF_C_ATLAS_UPLOADS]);
    }
    fprintf(sSummaryFile, "--------------------------------------------------------------------------------\n");
    fprintf(sSummaryFile, "  CORE 1 BREAKDOWN (GBA Simulation Thread)\n");
    fprintf(sSummaryFile, "    - GBA Logic (AgbMain):avg %5.2f ms (min %5.2f, max %5.2f)\n", mC1Logic.sum / avgSamples, MinOrZero(&mC1Logic), mC1Logic.max);
    fprintf(sSummaryFile, "    - Audio 48k & VBlank: avg %5.2f ms (min %5.2f, max %5.2f)\n", mC1VBlank.sum / avgSamples, MinOrZero(&mC1VBlank), mC1VBlank.max);
    fprintf(sSummaryFile, "    - Pacing idle (60 Hz):avg %5.2f ms (min %5.2f, max %5.2f)\n", mC1Wait.sum / avgSamples, MinOrZero(&mC1Wait), mC1Wait.max);
    fprintf(sSummaryFile, "--------------------------------------------------------------------------------\n");
    fprintf(sSummaryFile, "  SCENE COMPLEXITY\n");
    fprintf(sSummaryFile, "    - Visible Chunks:     avg %5.1f\n", mVisChunks.sum / avgFrames);
#ifdef PLATFORM_VITA
    {
        const struct mallinfo info = mallinfo();
        const size_t heapFree = (size_t)_newlib_heap_size_user - (size_t)info.uordblks;
        if (heapFree < sSessionMinHeapFree)
            sSessionMinHeapFree = heapFree;
        fprintf(sSummaryFile, "  MEMORY (KB free): newlib %u (session min %u) | vitaGL VRAM %u | RAM %u | PHYCONT %u\n",
                (uint32_t)(heapFree / 1024u), (uint32_t)(sSessionMinHeapFree / 1024u),
                (uint32_t)(vglMemFree(VGL_MEM_VRAM) / 1024u),
                (uint32_t)(vglMemFree(VGL_MEM_RAM) / 1024u),
                (uint32_t)(vglMemFree(VGL_MEM_PHYCONT) / 1024u));
    }
#endif
    fprintf(sSummaryFile, "  SESSION (3D): %u frames | over %.1f ms: %u (%.2f%%) | over %.1f ms: %u (%.2f%%) | max %.2f ms\n",
            sSessionFrames3D, budgetMs, sSessionOverBudget,
            sSessionFrames3D ? 100.0 * sSessionOverBudget / sSessionFrames3D : 0.0,
            budgetMs * 2.0, sSessionOverDouble,
            sSessionFrames3D ? 100.0 * sSessionOverDouble / sSessionFrames3D : 0.0,
            sSessionMaxFrameMs);
    fprintf(sSummaryFile, "================================================================================\n\n");

    fflush(sSummaryFile);
    // Avoid synchronous fflush(sCsvFile) every 120 frames on Core 0 to prevent SD card stalls;
    // stdio buffering + VitaPerf_Shutdown() handle CSV flushing safely.
}

void VitaPerf_EndFrame(const bool didRender3D, const uint32_t targetFps)
{
    sTotalFrameIndex++;
    sWindowFrameCount++;
    if (didRender3D)
        sWindowRenderCount++;
    if (targetFps != 0)
        sWindowTargetFps = targetFps;

    // Game frames since the last Core 0 frame: 1 per 3D frame at 60 FPS, 2 at 30. Anything else
    // means the renderer drew one game frame twice (0) or skipped some (more).
    const uint32_t tickNow = sCore1TickCount;
    const uint32_t logicTicks = tickNow - sLastFrameTicks;
    sLastFrameTicks = tickNow;
    if (didRender3D)
    {
        const uint32_t expected = (sWindowTargetFps == 30) ? 2u : 1u;
        if (logicTicks < expected)
            sWindowTickRepeats++;
        else if (logicTicks > expected)
            sWindowTickSkips++;
    }
    const double frameMs = VitaPerf_NowMs() - sFrameStartMs;
    const double c0Active = sCurAtlasMs + sCurCullMs + sCurBuildMs + sCurDraw3DMs + sCurUiScanMs + sCurUiUpMs + sCurSwapMs;
    const double otherMs = (frameMs > c0Active) ? (frameMs - c0Active) : 0.0;
    const double c1Logic = sCore1LogicLatest;
    const double c1VBlank = sCore1VBlankLatest;
    const double c1Wait = sCore1WaitLatest;
    const double budgetMs = 1000.0 / (double)sWindowTargetFps;

    // Accumulate to window stats
    if (didRender3D)
    {
        AddAccum(&mAtlasAnim, sCurAtlasMs);
        AddAccum(&mCull, sCurCullMs);
        AddAccum(&mBuild, sCurBuildMs);
        AddAccum(&mDraw3D, sCurDraw3DMs);
        AddAccum(&mUiScan, sCurUiScanMs);
        AddAccum(&mUiUp, sCurUiUpMs);
        AddAccum(&mSwap, sCurSwapMs);
        AddAccum(&mC0Active, c0Active);
        AddAccum(&mFrame, frameMs);
        AddAccum(&mOther, otherMs);
        AddAccum(&mVisChunks, (double)sCurVisibleChunks);
        AddAccum(&mBuiltChunks, (double)sCurChunksBuilt);
        AddAccum(&mDrawCalls, (double)sCurDrawCalls);
        AddAccum(&mVerts, (double)sCurVertexCount);
        for (uint32_t i = 0; i < VITA_PERF_T_COUNT; i++)
            AddAccum(&mSub[i], sCurSubMs[i]);
        for (uint32_t i = 0; i < VITA_PERF_C_COUNT; i++)
            sWindowCount[i] += (double)sCurCount[i];
        if (sWindowFrameMsCount < PERF_WINDOW_FRAME_CAP)
            sWindowFrameMs[sWindowFrameMsCount++] = frameMs;
        if (sCurCount[VITA_PERF_C_HOLES] > 0)
        {
            sWindowHoleFrames++;
            if (sCurCount[VITA_PERF_C_HOLES] > sWindowHoleMax)
                sWindowHoleMax = sCurCount[VITA_PERF_C_HOLES];
        }

        sSessionFrames3D++;
        if (frameMs > budgetMs)
        {
            sWindowOverBudget++;
            sSessionOverBudget++;
        }
        if (frameMs > budgetMs * 2.0)
        {
            sWindowOverDouble++;
            sSessionOverDouble++;
        }
        if (frameMs > sSessionMaxFrameMs)
            sSessionMaxFrameMs = frameMs;
    }
    for (uint32_t i = 0; i < sizeof(sEventLetters) - 1; i++)
    {
        if (sCurEvents & (1u << i))
            sWindowEventCount[i]++;
    }
    AddAccum(&mC1Logic, c1Logic);
    AddAccum(&mC1VBlank, c1VBlank);
    AddAccum(&mC1Wait, c1Wait);

#ifdef VITA_GPU_PROBE
    const bool benchMeasured = didRender3D && BenchFrameMeasured();
    const int32_t benchStep = sBenchStep;
    const int32_t benchLevel = VitaPerf_BenchLevel();
    const uint32_t probeMask = (sBenchStep >= 0) ? BenchMask() : 0u;
    const uint32_t benchRun = (sBenchStep >= 0) ? sBenchRun : 0u;
#else
    const bool benchMeasured = false;
    const int32_t benchStep = -1;
    const int32_t benchLevel = -1;
    const uint32_t probeMask = 0u;
    const uint32_t benchRun = 0u;
#endif

    // CSV line export (buffered, near zero overhead)
    if (sCsvFile != NULL)
    {
        char events[sizeof(sEventLetters)];
        uint32_t eventLen = 0;
        for (uint32_t i = 0; i < sizeof(sEventLetters) - 1; i++)
        {
            if (sCurEvents & (1u << i))
                events[eventLen++] = sEventLetters[i];
        }
        events[eventLen] = '\0';

        fprintf(sCsvFile,
                "%u,%d,%u,%.2f,%.2f,%.2f,"
                "%.2f,%.2f,%.2f,%u,"
                "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                "%u,%u,"
                "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
                "%.2f,%.2f,%.2f,%d,%d,%d,%u,%s,%u,"
                "%.2f,%.2f,%u,%u,%u,%.2f,%u,%.2f,%.2f,"
                "%u,%u,%.2f,%.2f,"
                "%.3f,%.3f,%u,",
                sTotalFrameIndex, didRender3D ? 1 : 0, sWindowTargetFps, frameMs, c0Active, otherMs,
                sCurAtlasMs, sCurSubMs[VITA_PERF_T_ATLAS_COMPOSE], sCurSubMs[VITA_PERF_T_ATLAS_UPLOAD],
                sCurCount[VITA_PERF_C_ATLAS_UPLOADS],
                sCurCullMs, sCurBuildMs, sCurSubMs[VITA_PERF_T_BUILD_GROUND], sCurSubMs[VITA_PERF_T_BUILD_ATLAS_EXT],
                sCurSubMs[VITA_PERF_T_BUILD_TREES], sCurSubMs[VITA_PERF_T_BUILD_BUILDINGS], sCurSubMs[VITA_PERF_T_BUILD_UPLOAD],
                sCurCount[VITA_PERF_C_LIGHT_RAYS], sCurCount[VITA_PERF_C_BUILD_VERTS],
                sCurDraw3DMs, sCurUiScanMs, sCurUiUpMs, sCurSwapMs, sCurSubMs[VITA_PERF_T_ENTITIES],
                sCurSubMs[VITA_PERF_T_LIVE_DIGEST], sCurSubMs[VITA_PERF_T_MAP_SHIFT],
                c1Logic, c1VBlank, c1Wait,
                sCurVisibleChunks, sCurChunksBuilt, sCurDrawCalls, sCurVertexCount, events,
                sCurCount[VITA_PERF_C_HOLES],
                sCurSubMs[VITA_PERF_T_LIGHT_RAYS], sCurSubMs[VITA_PERF_T_LIGHT_CONTACT],
                sCurCount[VITA_PERF_C_LIGHT_STEPS], sCurCount[VITA_PERF_C_LIGHT_CELLS],
                sCurCount[VITA_PERF_C_LIGHT_BLOCKS], sCurSubMs[VITA_PERF_T_LIGHT_API], logicTicks,
                sCurSubMs[VITA_PERF_T_WORLD_INSTANCES], sCurSubMs[VITA_PERF_T_ATLAS_SLOTS],
                sCurCount[VITA_PERF_C_WORKER_BUILT], sCurCount[VITA_PERF_C_WORKER_DROPPED],
                sCurSubMs[VITA_PERF_T_WORKER_COMPUTE], sCurSubMs[VITA_PERF_T_WORKER_WAIT],
                sCurSubMs[VITA_PERF_T_UI2_RENDER], sCurSubMs[VITA_PERF_T_UI2_TICK], sCurCount[VITA_PERF_C_UI2_QUADS]);

        const double gpuTotal = sCurSubMs[VITA_PERF_T_GPU_WORLD] + sCurSubMs[VITA_PERF_T_GPU_DOWN]
                              + sCurSubMs[VITA_PERF_T_GPU_BLUR] + sCurSubMs[VITA_PERF_T_GPU_BLOOM]
                              + sCurSubMs[VITA_PERF_T_GPU_KAWASE] + sCurSubMs[VITA_PERF_T_GPU_FINAL];
        fprintf(sCsvFile,
                "%.3f,%.3f,%.3f,%.3f,%.3f,"
                "%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%d,%d,%d,"
                "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,"
                "%u,%d,%d,%u,%d\n",
                sCurSubMs[VITA_PERF_T_ENT_EMIT], sCurSubMs[VITA_PERF_T_ENT_DECODE], sCurSubMs[VITA_PERF_T_ENT_ATLAS],
                sCurSubMs[VITA_PERF_T_ENT_PACK], sCurSubMs[VITA_PERF_T_ENT_LIGHTS],
                (int)sScene.hd2dLevel, sScene.dofOn ? 1 : 0, sInputFastForward ? 1 : 0, sInputUiOpen ? 1 : 0,
                (int)sScene.mapGroup, (int)sScene.mapNum, sScene.indoor ? 1 : 0, (int)sScene.weather,
                (double)sScene.daylight, sScene.lamps ? 1 : 0, sScene.inBattle ? 1 : 0, sScene.faded ? 1 : 0,
                sCurSubMs[VITA_PERF_T_GPU_WORLD], sCurSubMs[VITA_PERF_T_GPU_DOWN], sCurSubMs[VITA_PERF_T_GPU_BLUR],
                sCurSubMs[VITA_PERF_T_GPU_BLOOM], sCurSubMs[VITA_PERF_T_GPU_KAWASE], sCurSubMs[VITA_PERF_T_GPU_FINAL],
                gpuTotal, sCurCount[VITA_PERF_C_GXM_SCENES],
                benchRun, (int)benchStep, (int)benchLevel, probeMask, benchMeasured ? 1 : 0);
    }

#ifdef VITA_GPU_PROBE
    if (didRender3D)
        BenchAdvance(benchMeasured);
#endif

    // Flush a summary block every SAMPLE_WINDOW_FRAMES 3D frames, or every PERF_WINDOW_MAX_MS
    const bool windowFull = (sWindowRenderCount >= SAMPLE_WINDOW_FRAMES)
        || (VitaPerf_NowMs() - sWindowStartMs >= PERF_WINDOW_MAX_MS);
    if (windowFull && sSummaryFile != NULL)
    {
        WriteSummaryBlock();
        ResetWindowAccums();
    }
    else if (windowFull)
    {
        ResetWindowAccums();
    }
}

#else // !VITA_TELEMETRY

void VitaPerf_Init(void) {}
void VitaPerf_Shutdown(void) {}
void VitaPerf_Core1LogicTick(void) {}
void VitaPerf_Core1LogicDone(const double logicMs) { (void)logicMs; }
void VitaPerf_Core1VBlankDone(const double vblankMs) { (void)vblankMs; }
void VitaPerf_Core1WaitDone(const double waitMs) { (void)waitMs; }
void VitaPerf_Core1UiCells(const uint32_t peakLive, const uint32_t count, const uint32_t starved) { (void)peakLive; (void)count; (void)starved; }
void VitaPerf_BeginFrame(void) {}
void VitaPerf_MarkAtlasAnimStart(void) {}
void VitaPerf_MarkAtlasAnimEnd(const uint32_t rowsUpdated) { (void)rowsUpdated; }
void VitaPerf_MarkCullStart(void) {}
void VitaPerf_MarkCullEnd(const int32_t candidates, const int32_t visible) { (void)candidates; (void)visible; }
void VitaPerf_MarkBuildStart(void) {}
void VitaPerf_MarkBuildEnd(const int32_t chunksBuilt) { (void)chunksBuilt; }
void VitaPerf_MarkDraw3DStart(void) {}
void VitaPerf_MarkDraw3DEnd(const int32_t drawCalls, const uint32_t vertexCount) { (void)drawCalls; (void)vertexCount; }
void VitaPerf_MarkUiScanlinesStart(void) {}
void VitaPerf_MarkUiScanlinesEnd(void) {}
void VitaPerf_MarkUiUploadStart(void) {}
void VitaPerf_MarkUiUploadEnd(void) {}
void VitaPerf_MarkSwapStart(void) {}
void VitaPerf_MarkSwapEnd(void) {}
void VitaPerf_SubBegin(const VitaPerfTimer timer) { (void)timer; }
void VitaPerf_SubEnd(const VitaPerfTimer timer) { (void)timer; }
void VitaPerf_AddTime(const VitaPerfTimer timer, const double ms) { (void)timer; (void)ms; }
void VitaPerf_AddCount(const VitaPerfCounter counter, const uint32_t amount) { (void)counter; (void)amount; }
void VitaPerf_MarkEvent(const VitaPerfEvent event) { (void)event; }
void VitaPerf_SetScene(const VitaPerfScene *const scene) { (void)scene; }
void VitaPerf_SetInput(const bool fastForward, const bool uiOpen) { (void)fastForward; (void)uiOpen; }
void VitaPerf_LogMemory(const char *stage) { (void)stage; }
void VitaPerf_EndFrame(const bool didRender3D, const uint32_t targetFps) { (void)didRender3D; (void)targetFps; }

#endif // VITA_TELEMETRY
