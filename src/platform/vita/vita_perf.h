#ifndef GUARD_VITA_PERF_H
#define GUARD_VITA_PERF_H

/*
 * Frame telemetry: perf_frames.csv (one line per Core 0 frame) and perf_summary.txt
 * (one block per sample window). Core 0 calls everything except the Core1* functions,
 * which only AgbMain (Core 1) calls.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns current high-resolution timestamp in milliseconds
double VitaPerf_NowMs(void);

// Telemetry system lifecycle
void VitaPerf_Init(void);
void VitaPerf_Shutdown(void);

// Core 0 (Render & Main Loop) Stage Markers
void VitaPerf_BeginFrame(void);
void VitaPerf_MarkAtlasAnimStart(void);
void VitaPerf_MarkAtlasAnimEnd(uint32_t rowsUpdated);
void VitaPerf_MarkCullStart(void);
void VitaPerf_MarkCullEnd(int32_t candidates, int32_t visible);
void VitaPerf_MarkBuildStart(void);
// chunksBuilt: chunks built since the matching MarkBuildStart (added to the frame count).
void VitaPerf_MarkBuildEnd(int32_t chunksBuilt);
void VitaPerf_MarkDraw3DStart(void);
void VitaPerf_MarkDraw3DEnd(int32_t drawCalls, uint32_t vertexCount);
void VitaPerf_MarkUiScanlinesStart(void);
void VitaPerf_MarkUiScanlinesEnd(void);
void VitaPerf_MarkUiUploadStart(void);
void VitaPerf_MarkUiUploadEnd(void);
void VitaPerf_MarkSwapStart(void);
void VitaPerf_MarkSwapEnd(void);
// targetFps: the cadence Core 0 is paced at this frame (30 or 60).
void VitaPerf_EndFrame(bool didRender3D, uint32_t targetFps);

// Sub-timers inside the stages above, accumulated per frame. A sub-timer may be
// nested in another one with a different id (BUILD_ATLAS_EXT runs inside BUILD_GROUND).
typedef enum
{
    VITA_PERF_T_BUILD_GROUND,    // ground, relief and their lighting rays (includes BUILD_ATLAS_EXT)
    VITA_PERF_T_BUILD_ATLAS_EXT, // atlas extension for metatiles a chunk found missing
    VITA_PERF_T_BUILD_TREES,
    VITA_PERF_T_BUILD_BUILDINGS,
    VITA_PERF_T_BUILD_UPLOAD,    // VBO upload of a built chunk
    VITA_PERF_T_ATLAS_COMPOSE,   // CPU recomposition of animated metatiles
    VITA_PERF_T_ATLAS_UPLOAD,    // glTexSubImage2D of the recomposed rows
    VITA_PERF_T_MAP_SHIFT,       // map change work: HandleMapChange, chunk shift, building/lighting resets
    VITA_PERF_T_WORLD_INSTANCES, // VoxelWorld_BuildInstances (every frame the layout is not locked)
    VITA_PERF_T_ATLAS_SLOTS,     // atlas slot for the current map: compose, extend, activate (map/tileset changes)
    VITA_PERF_T_WORKER_COMPUTE,  // chunk computed on the Core 2 worker (its own time, committed this frame)
    VITA_PERF_T_WORKER_WAIT,     // Core 0 blocked at the frame start waiting for the worker
    VITA_PERF_T_ENTITIES,        // billboards, shadows, reflections
    VITA_PERF_T_LIVE_DIGEST,     // per-frame hash of the live map layout
    VITA_PERF_T_LIGHT_RAYS,      // in chunk builds: marching shadow rays (part of the build phases)
    VITA_PERF_T_LIGHT_CONTACT,   // in chunk builds: contact AO tests (part of the build phases)
    VITA_PERF_T_LIGHT_API,       // in chunk builds: all time inside the lighting entry points
    VITA_PERF_T_UI2_RENDER,      // new UI: Core 0 pass (vertices + draw call)
    VITA_PERF_T_UI2_TICK,        // new UI: Core 1 time to build the list drawn this frame
    VITA_PERF_T_POST_DOF,        // HD-2D post: downsample + Kawase blur
    VITA_PERF_T_POST_BLOOM,      // HD-2D post: bright pass + emitters + bloom blur
    VITA_PERF_T_POST_COMPOSITE,  // HD-2D post: final composite + split-tone + vignette
    VITA_PERF_T_ENT_EMIT,        // inside ENTITIES: VoxelEntities_Emit (includes ENT_DECODE)
    VITA_PERF_T_ENT_DECODE,      // inside ENT_EMIT: sprite frames decoded into the atlas (RefreshSlot)
    VITA_PERF_T_ENT_ATLAS,       // inside ENTITIES: WriteSpriteAtlasSlots
    VITA_PERF_T_ENT_PACK,        // inside ENTITIES: vertex packing, shadow and reflection copies
    VITA_PERF_T_ENT_LIGHTS,      // inside ENTITIES: LightCards
    // GPU time per render target, probe builds only (VitaPerf_GpuScene): the time glFinish waited
    // when the scene ended, i.e. the GPU work CPU submission did not already cover.
    VITA_PERF_T_GPU_WORLD,       // the 3D scene (passes 1-5.9); without post, also pass 6 and the UI
    VITA_PERF_T_GPU_DOWN,        // half-res downsample
    VITA_PERF_T_GPU_BLUR,        // wide blur
    VITA_PERF_T_GPU_BLOOM,       // bloom target: clear, bright pass, sky bloom, shafts
    VITA_PERF_T_GPU_KAWASE,      // both Kawase passes
    VITA_PERF_T_GPU_FINAL,       // composite, pass 6 and the new UI
    VITA_PERF_T_COUNT
} VitaPerfTimer;

void VitaPerf_SubBegin(VitaPerfTimer timer);
void VitaPerf_SubEnd(VitaPerfTimer timer);
// Adds time measured elsewhere to a sub-timer of the current frame.
void VitaPerf_AddTime(VitaPerfTimer timer, double ms);

typedef enum
{
    VITA_PERF_C_LIGHT_RAYS,   // shadow rays cast while building chunks
    VITA_PERF_C_BUILD_VERTS,  // vertices produced by chunk builds
    VITA_PERF_C_ATLAS_UPLOADS, // glTexSubImage2D calls of the atlas animation
    VITA_PERF_C_HOLES,        // chunks in view left without geometry this frame (background shows)
    VITA_PERF_C_LIGHT_STEPS,  // ray points tested while building chunks
    VITA_PERF_C_LIGHT_CELLS,  // lighting cells classified (cache misses) while building chunks
    VITA_PERF_C_LIGHT_BLOCKS, // block bounds worked out while building chunks
    VITA_PERF_C_WORKER_BUILT, // chunks computed on the worker and committed
    VITA_PERF_C_WORKER_DROPPED, // worker results dropped (the world changed meanwhile)
    VITA_PERF_C_WORKER_OVERLAP, // 3D frames that started while the worker was still computing
    VITA_PERF_C_WORKER_HELD,  // frames that handed no chunk to the worker because the atlas was being extended
    VITA_PERF_C_UI2_QUADS,    // new UI: quads drawn this frame
    VITA_PERF_C_GXM_SCENES,   // render-target scenes this frame (probe builds only)
    VITA_PERF_C_COUNT
} VitaPerfCounter;

void VitaPerf_AddCount(VitaPerfCounter counter, uint32_t amount);

// Events that explain a frame, written as letters in the CSV "events" column.
typedef enum
{
    VITA_PERF_E_MAP_RESET = 1u << 0,  // 'M' new map: chunk cache invalidated
    VITA_PERF_E_CROSS = 1u << 1,      // 'X' walked into a connected map: chunks shifted
    VITA_PERF_E_LIVE = 1u << 2,       // 'L' a live tile changed (door, script)
    VITA_PERF_E_SUN = 1u << 3,        // 'S' the sun moved: visible chunks will rebuild
    VITA_PERF_E_WARMUP = 1u << 4,     // 'W' warm-up frame: builds unbounded
    VITA_PERF_E_SYNC = 1u << 5,       // 'F' screen faded: builds and page loads synchronous
    VITA_PERF_E_ATLAS_EXT = 1u << 6,  // 'A' atlas extended during a build
    VITA_PERF_E_ATLAS_EVICT = 1u << 7, // 'C' an atlas slot in use was recomposed for another tileset
} VitaPerfEvent;

void VitaPerf_MarkEvent(VitaPerfEvent event);

// Writes one line to perf_summary.txt with the free memory of the newlib heap and of each
// vitaGL pool, labelled with `stage` (e.g. "after vglInit"). For one-off points, not per frame.
void VitaPerf_LogMemory(const char *stage);

// What the frame showed, written with each CSV line so one session's log can be split by zone.
// Filled by the 3D renderer on Core 0 (VitaPerf_SetScene) once per 3D frame.
typedef struct
{
    int32_t hd2dLevel;     // VOXEL_HD2D_* drawn this frame
    bool dofOn;            // DEPTH OF FIELD option
    int32_t mapGroup, mapNum;
    bool indoor;
    int32_t weather;       // VoxelWeatherClass
    float daylight;        // 0 night .. 1 day
    bool lamps;            // night lamps and window glow drawn (nightGlow > 0)
    bool inBattle;
    bool faded;            // screen fully faded (no 3D drawn)
    float playerX, playerZ;
} VitaPerfScene;

void VitaPerf_SetScene(const VitaPerfScene *scene);
// Core 0 input state, once per presentation loop iteration (sdl2.c).
void VitaPerf_SetInput(bool fastForward, bool uiOpen);

// Passes the automatic bench can switch off, one at a time (probe builds only).
typedef enum
{
    VITA_PERF_PASS_FOG,         // DrawFogBanks (fog and sand banks)
    VITA_PERF_PASS_MOTES,       // DrawMotes (dust motes, fireflies)
    VITA_PERF_PASS_SURFACEFX,   // DrawSurfaceFx
    VITA_PERF_PASS_WEATHER,     // DrawWeather
    VITA_PERF_PASS_WORLDRAYS,   // DrawWorldRays (OFF / LOW)
    VITA_PERF_PASS_DAYNIGHT,    // Pass_DayNightGrade
    VITA_PERF_PASS_LAMPS,       // Pass_NightLamps, Pass_EntityLamps, Pass_BuildingGlow
    VITA_PERF_PASS_ATMOS2D,     // Pass_Atmosphere2D
    VITA_PERF_PASS_SHADOWS,     // Pass_CastShadows
    VITA_PERF_PASS_REFLECTIONS, // Pass_Reflections
    VITA_PERF_PASS_ENTITIES,    // Pass_Entities (billboards)
    VITA_PERF_PASS_SKYBLOOM,    // sky bloom quads in the bloom target
    VITA_PERF_PASS_SHAFTS,      // shader light shafts
    VITA_PERF_PASS_KAWASE,      // the two Kawase passes
    VITA_PERF_PASS_BLOOM,       // the whole bloom target (bright pass, sky bloom, shafts, Kawase)
    VITA_PERF_PASS_BLUR,        // the wide blur level
    VITA_PERF_PASS_COUNT
} VitaPerfPass;

// The render target a GPU scene draws into, for VitaPerf_GpuScene.
typedef enum
{
    VITA_PERF_GPU_NONE,
    VITA_PERF_GPU_WORLD,
    VITA_PERF_GPU_DOWN,
    VITA_PERF_GPU_BLUR,
    VITA_PERF_GPU_BLOOM,
    VITA_PERF_GPU_KAWASE,
    VITA_PERF_GPU_FINAL,
} VitaPerfGpuScene;

#ifdef VITA_GPU_PROBE
// Called right after each glBindFramebuffer of the 3D frame with the scene about to start, and
// with VITA_PERF_GPU_NONE before the swap: waits for the GPU (glFinish) and charges the wait to the
// scene that just ended. Only at a render-target change: vitaGL's glFinish ends the GXM scene, so
// a probe in mid-scene would split it and lose its depth.
void VitaPerf_GpuScene(VitaPerfGpuScene next);
// True while the automatic bench has this pass switched off.
bool VitaPerf_PassSkipped(VitaPerfPass pass);
// The HD-2D level the bench imposes this frame, or -1 to use the player's option.
int32_t VitaPerf_BenchLevel(void);
#else
static inline void VitaPerf_GpuScene(const VitaPerfGpuScene next) { (void)next; }
static inline bool VitaPerf_PassSkipped(const VitaPerfPass pass) { (void)pass; return false; }
static inline int32_t VitaPerf_BenchLevel(void) { return -1; }
#endif

// Core 1 (AgbMain GBA Simulation) Stage Markers
void VitaPerf_Core1LogicTick(void);
void VitaPerf_Core1LogicDone(double logicMs);
void VitaPerf_Core1VBlankDone(double vblankMs);
void VitaPerf_Core1WaitDone(double waitMs);

#ifdef __cplusplus
}
#endif

#endif // GUARD_VITA_PERF_H
