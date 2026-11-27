/*
 * Hardware-accelerated 3D Voxel Overworld Renderer for PlayStation Vita (vitaGL).
 * Replaces old heuristic tile-extrusion with true 3D diorama engine from emerald-3ds:
 * - 3D Polygonal buildings with 45-degree pitched roofs, chamfers and eaves (buildings.bin)
 * - Continuous terrain elevation, slopes, terraces and jump ledges (relief.bin)
 * - 3D Wooden signposts (signposts.bin)
 * - Dedicated clean-alpha billboard tree cards and tall grass (trees.rgba5551)
 * - 512x256 Metatile atlas and dynamic sprite slot decoding with cast shadows
 */

#ifdef PLATFORM_SDL2
#if defined(VOXEL_SUPPORTED) || defined(PLATFORM_VITA) || defined(NATIVE_LINUX)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#ifdef PLATFORM_VITA
#include <vitaGL.h>
#else
#include <GL/gl.h>
#endif

#include "../vita/vita_perf.h"

static int32_t sFrameDrawCalls = 0;
static uint32_t sFrameVertCount = 0;

static inline void VoxelGl_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    glDrawArrays(mode, first, count);
    sFrameDrawCalls++;
    sFrameVertCount += (uint32_t)count;
}
#define glDrawArrays VoxelGl_DrawArrays

#include <SDL2/SDL.h>

#include "global.h"
#include "main.h"
#include "battle.h"
#include "window.h"
#include "gba/io_reg.h"
#include "sprite.h"
#include "palette.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "metatile_behavior.h"
#include "constants/metatile_behaviors.h"
#include "decompress.h"
#include "constants/map_types.h"
#include "field_effect.h"
#include "constants/field_effects.h"
#include "port_platform.h"
#include "platform.h"
#include "platform/framedraw.h"
#include "platform/vita_daynight.h"
#include "platform/vita_extras.h"
#include "platform/ui.h"
#include "platform/voxel_snapshot.h"
#include "platform/game_frame.h"
#include "../vita/vita_platform.h"

#include "voxel_renderer.h"
#include "voxel_world.h"
#include "voxel_camera.h"
#include "voxel_atlas.h"
#include "voxel_tree.h"
#include "voxel_building.h"
#include "voxel_relief.h"
#include "voxel_regions.h"
#include "voxel_sign.h"
#include "voxel_lamps.h"
#include "voxel_battle.h"
#include "voxel_entities.h"
#include "voxel_lighting.h"
#include "voxel_pointlights.h"
#include "voxel_hd2d_shader.h"
#include "voxel_grade.h"
#include "voxel_file.h"
#include "voxel_unswizzle.h"
#include "voxel_mesh_builder.h"
#include "voxel_tuning.h"
#include "voxel_fly_game.h"

// External state
extern SDL_Window *sdlWindow;

// ---- Hardware Vertex Format for vitaGL ----
typedef struct
{
    float x, y, z;
    float u, v;
    uint8_t r, g, b, a;
} VoxelGlVertex;

// ---- Textures ----
#define VOXEL_MAX_ATLAS_PAGES 4
static GLuint sAtlasTex[VOXEL_MAX_ATLAS_PAGES] = {0};
static uint32_t sAtlasPageCount = 0;
static GLuint sTreeTex[3] = { 0, 0, 0 };
static uint16_t sFlowerFrames[3][16 * 16];
static bool sFlowerFramesReady = false;
static GLuint sTreeShaderProg = 0;
static GLint sTreeShaderUMvp = -1;
static GLint sTreeShaderUChunkDelta = -1;
static GLint sTreeShaderUSwayA = -1;
static GLint sTreeShaderUSwayB = -1;
static GLint sTreeShaderUWind = -1;
static GLint sTreeShaderUPlayerXZ = -1;
static GLint sTreeShaderUTex = -1;
static bool sTreeShaderReady = false;
#define VOXEL_MAX_BUILDING_PAGES 272
#define VOXEL_BUILDING_CACHE_SLOTS 56
#define VOXEL_BUILDING_REQ_CAP 32
static GLuint sSpriteTex = 0;
static GLuint sUiTex = 0;
// Four overlay textures used in turn: vitaGL's FRAME_PURGE_FREQ is 4 swaps, so a 4-slot
// ring ensures glTexSubImage2D never hits a texture still in flight on the GPU.
#define VOXEL_UI_TEX_RING 4u
static GLuint sUiTexRing[VOXEL_UI_TEX_RING] = { 0, 0, 0, 0 };
static uint8_t sUiTexIdx = 0;
// The last overlay image converted for sUiTex (pass 6); cleared whenever sUiTex is recreated.
static bool sUiImageValid = false;
static bool sUiImageHasUi = false;
static uint32_t sUiImageHash = 0;
static GLuint sLampPoolTex = 0;   // radial falloff in RGB: lamp pools, water glints, battle ovals
static GLuint sLampHaloTex = 0;   // soft radial alpha, white: the night lamps' halo
static GLuint sRippleTex = 0;
static GLuint sSoftMistTex = 0;
static GLuint sFogTex = 0;
static GLuint sVignetteTex = 0;
static GLuint sWeatherAtlasTex = 0;

// ---- Downscaled Scratch Diorama Blur (DoF) & HD-2D Bloom FBOs ----
#define VOXEL_DIORAMA_FBO_W 960
#define VOXEL_DIORAMA_FBO_H 544
// The scene textures and depth: the world's 544 rows plus one row of 32-px tiles above them, where
// HIGH draws the shaft profiles (VOXEL_SHAFT_BAND_Y) inside the world scene instead of in targets
// of their own. Nothing reads past row 543 otherwise (vMax is renderH over this height).
#define VOXEL_DIORAMA_TEX_H 576
#define VOXEL_SHAFT_BAND_Y VOXEL_DIORAMA_FBO_H
#define VOXEL_SCRATCH_FBO_W 480
#define VOXEL_SCRATCH_FBO_H 272
#define VOXEL_BLOOM_FBO_W   240
#define VOXEL_BLOOM_FBO_H   136
// HIGH's half-resolution target with the bloom source as a band above the downsample
// (rows VOXEL_BLOOM_BAND_Y up), so the bright pass, sky bloom and shafts are drawn in the
// downsample's scene instead of a 240x136 target of their own.
#define VOXEL_BAND_TEX_H    (VOXEL_SCRATCH_FBO_H + VOXEL_BLOOM_FBO_H)
#define VOXEL_BLOOM_BAND_Y  VOXEL_SCRATCH_FBO_H
#define VOXEL_POST_TARGET_BUFFERS 2u

// Double-buffered post-processing targets prevent Core 0 from stalling in vitaGL/SceGxm
// when frame N+1 begins writing to FBOs while frame N is still finishing on the GPU.
typedef struct
{
    GLuint dioramaFbo;
    GLuint dioramaColorTex;
    GLuint dioramaColorTexFast;
    GLuint dioramaActiveTex;
    GLuint dioramaDepthRb;
    GLuint scratchFbo;
    GLuint scratchColorTex;
    // The wider blur level of the depth of field (Pass_DepthOfField), half resolution like the scratch
    GLuint blurFbo;
    GLuint blurTex;
    GLuint bloomFboA;
    GLuint bloomTexA;
    GLuint bloomFboB;
    GLuint bloomTexB;
    // HIGH: downsample + bloom source band (VOXEL_BAND_TEX_H); 0 when it could not be made.
    GLuint bandFbo;
    GLuint bandTex;
} PostRenderTargets;

static PostRenderTargets sPostTargets[VOXEL_POST_TARGET_BUFFERS];
static uint32_t sPostTargetIndex = 0u;
static bool sDioramaFboReady = false;

static GLuint sMenuBgFbo = 0;
static GLuint sMenuBgTex = 0;
static GLuint sMenuBgDepthRb = 0;
static bool sMenuBgValid = false;
static int32_t sMenuBgPreset = 0;
static bool sMenuBgBlur = false;
static int32_t sMenuBgBloom = 10;
static int32_t sMenuBgSkyBloom = 5;
static int32_t sMenuBgShafts = 5;

// GRAPHICS -> HD-2D (voxel_hd2d), independent of the framerate:
//   OFF  classic renderer;
//   LOW  the world's HD-2D effects (relief, sway, flowers, particles, fireflies, surface FX), no post;
//   MED  + the post pass at its light settings (RGBA5551 scene, no wide blur, softer bloom/grade);
//   HIGH + the post pass at full quality.
// != HD2D_PRESET_OFF asks for the world effects, Hd2dPost() for the post pass.
typedef enum
{
    HD2D_PRESET_OFF = VOXEL_HD2D_OFF,
    HD2D_PRESET_LOW = VOXEL_HD2D_LOW,
    HD2D_PRESET_MED = VOXEL_HD2D_MED,
    HD2D_PRESET_HIGH = VOXEL_HD2D_HIGH,
} Hd2dPreset;

static inline Hd2dPreset GetHd2dPreset(void)
{
    // The measuring build's automatic bench steps through the levels without touching the option.
    const int32_t benchLevel = VitaPerf_BenchLevel();
    const int32_t level = (benchLevel >= 0) ? benchLevel : VitaExtras_GetFast(FAST_EXTRA_VOXEL_HD2D);
    return (level >= VOXEL_HD2D_OFF && level < VOXEL_HD2D_COUNT) ? (Hd2dPreset)level : HD2D_PRESET_HIGH;
}

// The post pass (bloom, sky bloom, shader shafts, cinematic grade): MED and HIGH.
static inline bool Hd2dPost(const Hd2dPreset preset)
{
    return preset >= HD2D_PRESET_MED;
}

// Animated tiles tracking (water waves, flowers, waterfalls)
static uint8_t sVoxelTilesetAnimDirty[128];
static bool sVoxelTilesetAnimPending = false;

void VoxelRenderer_NotifyTilesetAnimWrite(const void *dest, uint32_t bytes)
{
    uintptr_t base = (uintptr_t)BG_VRAM;
    uintptr_t address = (uintptr_t)dest;
    if (address >= base && address - base < 0x8000 && bytes > 0)
    {
        uint32_t offset = (uint32_t)(address - base);
        if (bytes > 0x8000 - offset) bytes = 0x8000 - offset;
        uint32_t firstTile = offset / 32;
        uint32_t tileCount = (offset % 32 + bytes + 31) / 32;
        if (firstTile < 1024)
        {
            if (firstTile + tileCount > 1024) tileCount = 1024 - firstTile;
            for (uint32_t t = firstTile; t < firstTile + tileCount; ++t)
                sVoxelTilesetAnimDirty[t >> 3] |= (1u << (t & 7));
            sVoxelTilesetAnimPending = true;
        }
    }
}

// ---- Textures & Atlas State (Multi-Atlas LRU Cache matching Emerald 3DS) ----
#define VOXEL_ATLAS_SLOTS 8

typedef struct
{
    GLuint tex[VOXEL_MAX_ATLAS_PAGES];
    const void *primaryTileset;
    const void *secondaryTileset;
    uint32_t stamp;
    uint32_t generation;
    uint32_t extension;
    VoxelAtlasMap map;
    uint64_t cutoutMask[VOXEL_MAX_ATLAS_PAGES][8];
    uint32_t pageCount;
    bool valid;
    uint16_t linear[VOXEL_MAX_ATLAS_PAGES][VOXEL_ATLAS_PIXELS];
} VoxelAtlasSlot;

static VoxelAtlasSlot sAtlasSlots[VOXEL_ATLAS_SLOTS];
static uint32_t sSlotPendingPrimary[VOXEL_ATLAS_SLOTS][16];
static int32_t sActiveAtlasSlot = -1;

// Shared zero-allocation scratch buffers for unswizzling building pages, metatile atlases, and sprite atlases
static uint16_t sBuildingPageMortonScratch[512 * 512];
static uint16_t sBuildingPageLinearScratch[512 * 512];
#define sAtlasLinear sBuildingPageLinearScratch
#define sSpriteAtlasLinear sBuildingPageLinearScratch

static VoxelAtlasMap sAtlasMap;
static const void *sLastPrimary = NULL;
static const void *sLastSecondary = NULL;
static bool sNeedsFullWarmup = true;
static int32_t sWarmupFrames = 10;
static bool sNeedAtlasExtension = false;

static uint16_t sTreeMorton[VOXEL_TREE_TEXTURE_DIM * VOXEL_TREE_TEXTURE_DIM];
static uint16_t sTreeLinear[VOXEL_TREE_TEXTURE_DIM * VOXEL_TREE_TEXTURE_DIM];

static uint16_t sSpriteAtlasMorton[VOXEL_SPRITE_PIXELS];

// ---- Camera & Tracking ----
static VoxelCamera sCamera;
static bool sNeedCameraSnap = true;
static uint32_t sLastLiveDigest = 0;
static uint32_t sFrameCounter = 0;


typedef struct {
    int32_t mapGroup;
    int32_t mapNum;
    int32_t originX;
    int32_t originY;
} VoxelPreviousOrigin;

static VoxelPreviousOrigin sPreviousOrigins[MAX_VOXEL_MAP_INSTANCES];
static uint32_t sPreviousOriginCount = 0;
static int32_t sMeshMapGroup = -1;
static int32_t sMeshMapNum = -1;
static float sLastPlayerX = 0.0f;
static float sLastPlayerZ = 0.0f;

// ---- 3D Battle Stage Camera ----
typedef struct
{
    bool begun;
    bool chosen;
    bool glided;
    VoxelCamera field;
    float targetX;
    float targetZ;
    float ground;
    uint32_t frames;
    uint32_t glide;
} VoxelBattleCamState;

static VoxelBattleCamState sBattleState;

static inline float Voxel_ClampF(const float v, const float lo, const float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float Voxel_SmootherF(const float p)
{
    return p * p * p * (p * (p * 6.0f - 15.0f) + 10.0f);
}

static void BattleCamera(void)
{
    const float degrees = 3.14159265f / 180.0f;
    const VoxelCamera * const from = &sBattleState.field;
    float u, p, q, t, arc, fov, punch, quake, x, z, ground, pitch, distance;

    ++sBattleState.frames;
    if (sBattleState.frames >= 30)
    {
        sBattleState.glided = true;
    }
    if (sBattleState.chosen && sBattleState.glided && sBattleState.glide < 60)
    {
        ++sBattleState.glide;
    }

    u = (float)sBattleState.glide / 60.0f;
    p = Voxel_ClampF(u / 0.78f, 0.0f, 1.0f);
    q = Voxel_ClampF((u - 0.78f) / 0.22f, 0.0f, 1.0f);
    t = Voxel_SmootherF(p);
    arc = (p > 0.0f && p < 1.0f) ? sinf(p * 3.14159265f) : 0.0f;

    x = from->targetX + (sBattleState.targetX - from->targetX) * t;
    z = from->targetZ + (sBattleState.targetZ - from->targetZ) * t;
    ground = from->ground + (sBattleState.ground - from->ground) * t;
    pitch = from->pitch + (VOXEL_BATTLE_PITCH - from->pitch) * t;
    distance = from->distance + (VOXEL_BATTLE_DISTANCE - from->distance) * t;

    pitch += 16.0f * arc;
    if (pitch > 75.0f)
    {
        pitch = 75.0f;
    }

    fov = from->fov * (1.0f - 0.40f * arc * arc);
    if (arc > 0.0f)
    {
        distance *= tanf(from->fov * 0.5f * degrees) / tanf(fov * 0.5f * degrees);
    }
    else
    {
        fov = from->fov;
    }

    punch = (q > 0.0f && q < 1.0f) ? expf(-6.0f * q) * sinf(3.0f * 3.14159265f * q) : 0.0f;
    quake = (q > 0.0f && q < 1.0f) ? 0.35f * expf(-7.0f * q) : 0.0f;
    distance *= 1.0f - 0.08f * punch;
    x += quake * sinf((float)sBattleState.glide * 2.7f);
    z += quake * cosf((float)sBattleState.glide * 3.9f);
    sCamera.fov = fov;

    VoxelCamera_Frame(&sCamera, x, z, ground, pitch, distance);
}

// ---- Chunk System ----
#define CHUNK_SIZE 8
#define CHUNK_GROUND_CAP 1536
#define CHUNK_GROUND_TOTAL_CAP (VOXEL_MAX_ATLAS_PAGES * CHUNK_GROUND_CAP)
#define CHUNK_TREE_CAP 2560
#define CHUNK_BUILDING_CAP 12288
// Night lamps (VoxelLamps_EmitChunk): lantern glass, then the pools of light, after the buildings.
#define CHUNK_LAMP_GLASS_CAP 512
#define CHUNK_LAMP_POOL_CAP 768
#define CHUNK_LAMP_CAP (CHUNK_LAMP_GLASS_CAP + CHUNK_LAMP_POOL_CAP)
#define MAX_CHUNK_GROUND_SPANS 24
#define MAX_CHUNK_BUILDING_SPANS 4

#define MAX_CHUNKS 160
#define CHUNK_WORKER_BATCH 4
#define CHUNK_WORKER_BATCH_NORMAL 2
#define CHUNK_VERTS_PER_SLOT (CHUNK_GROUND_TOTAL_CAP + CHUNK_TREE_CAP + CHUNK_BUILDING_CAP + CHUNK_LAMP_CAP)

typedef struct
{
    uint8_t atlasSlot;
    uint8_t page;
    uint8_t isCutout;
    uint8_t pad;
    uint16_t firstVert;
    uint16_t vertCount;
} VoxelChunkGroundSpan;

typedef struct
{
    int16_t page;
    uint16_t firstVert;
    uint16_t vertCount;
    uint16_t glowVertCount; // upright wall triangles packed at [firstVert .. firstVert + glowVertCount)
} VoxelChunkBuildingSpan;

typedef struct
{
    bool valid;
    bool uncovered;
    int32_t chunkX, chunkY;
    float minY, maxY;
    uint32_t hash;
    uint32_t lastUsedFrame;
    uint32_t checkedEpoch; // sWorldEpoch when its hash was last known current
    uint8_t usedAtlasMask;
    uint8_t groundSpanCount;
    uint8_t buildingSpanCount;
    uint32_t atlasGen[VOXEL_ATLAS_SLOTS];
    uint32_t atlasExt[VOXEL_ATLAS_SLOTS];

    int32_t buildingPage;
    uint32_t groundTotalCount;
    uint32_t treeCount;
    uint32_t buildingCount;
    uint16_t treeFirstVert;
    uint16_t treeOpaqueCount;
    uint16_t treeStaticCount;
    uint16_t buildingFirstVert;
    // Night lamps: lampGlassCount glass vertices from lampFirstVert, then lampPoolCount pool ones.
    uint16_t lampFirstVert;
    uint16_t lampGlassCount;
    uint16_t lampPoolCount;
    uint16_t totalVertCount;
    uint16_t vboCap;
    // The lamps' light centres, in the chunk's build coordinates (add drawOffsetX/Z).
    uint8_t lampLightCount;
    float lampLights[VOXEL_LAMP_CHUNK_LIGHTS][3];
    GLuint vbo;
    VoxelChunkGroundSpan groundSpans[MAX_CHUNK_GROUND_SPANS];
    VoxelChunkBuildingSpan buildingSpans[MAX_CHUNK_BUILDING_SPANS];

    // Where the chunk is drawn relative to where it was built: map crossings since its build
    // (ShiftChunkCoordinates), applied as a translation (ChunkOffsets_Apply).
    float drawOffsetX, drawOffsetZ;

    // How far its building models reach past its own cells, in tiles (0: not past them), and
    // the top of the highest. A model is emitted only by the chunk holding its north-west
    // cell but stretches up to a dozen cells east and south of it (Rustboro's Devon Corp:
    // 10x9); the culling tests the chunk with these, or the whole model vanished as soon as
    // that one corner left the view. Relative to the chunk, so a map crossing keeps them.
    float bldWest, bldEast, bldNorth, bldSouth, bldTop;

    // While the chunk is being built: its parts in sChunkStaging. NULL otherwise; once uploaded
    // the geometry only lives in the VBO.
    VoxelGlVertex *groundVerts;
    VoxelGlVertex *treeVerts;
    VoxelGlVertex *buildingVerts;
    VoxelGlVertex *lampVerts;
} VoxelChunkSlot;

static VoxelChunkSlot sChunks[MAX_CHUNKS];
// One chunk's worth of packed vertices, filled by BuildChunk and uploaded to the chunk's VBO.
// Replaces a CPU copy per slot (64 x 460 KB = 29.5 MB) kept only to move vertices on crossings.
static VoxelGlVertex *sChunkStaging = NULL;
static bool sChunksAllocated = false;
static int32_t sChunkOriginX = 0;
static int32_t sChunkOriginY = 0;
// Bumped whenever chunk content may have changed under cached chunks (live tile change, map
// crossing, sun moved). A chunk whose checkedEpoch is older has its hash compared the next time
// it is in view, so chunks off screen at the change are not left with the old content.
static uint32_t sWorldEpoch = 1;

// Geometry batching staging buffer: collapses ~100 draw calls into 4-6 per frame
#define BATCH_GL_VERTS_CAP 32768
static VoxelGlVertex sBatchGlVerts[BATCH_GL_VERTS_CAP];

// Scratch vertex buffer for building
#define SCRATCH_VERTICES 16384
static VoxelVertex sScratch[SCRATCH_VERTICES];


// Entity, Shadow & Reflection dynamic vertex buffers
#define MAX_ENTITY_VERTS 2048
#define MAX_SHADOW_VERTS 1024
#define MAX_REFLECTION_VERTS 1024
static VoxelVertex sEntityVerts[MAX_ENTITY_VERTS];
static VoxelGlVertex sEntityGlVerts[MAX_ENTITY_VERTS];
static uint32_t sEntityGlCount = 0;
// The cards a lamp lights, again, with the light as their colour: added after the night veil
// (Pass_EntityLamps), since a card's own colour is already near white and cannot take more.
static VoxelGlVertex sEntityLitVerts[MAX_ENTITY_VERTS];
static uint32_t sEntityLitCount = 0;

static VoxelVertex sShadowVerts[MAX_SHADOW_VERTS];
static VoxelGlVertex sShadowGlVerts[MAX_SHADOW_VERTS];
static uint32_t sShadowGlCount = 0;

static VoxelVertex sReflectionVerts[MAX_REFLECTION_VERTS];
static VoxelGlVertex sReflectionGlVerts[MAX_REFLECTION_VERTS];
static uint32_t sReflectionGlCount = 0;


// ---- Emerald 3DS Weather & Atmospheric Lighting System (ported from ctr_voxel.c) ----

typedef struct
{
    float sun[3], shade[3];
    float haze;
    float dappleLow, dappleHigh;
    float rays;
    float bloom;
    float motes;
    float hazeRgb[3];
    float hazeStart, hazeRamp;
} VoxelLight;


typedef struct
{
    float height, driftX, driftZ, alpha, offsetU, offsetV;
} FogBank;

static const FogBank sFogBanks[2] = {
    { 0.35f,  0.45f, 0.18f, 0.56f, 0.00f, 0.00f },
    { 0.85f, -0.30f, 0.28f, 0.48f, 0.37f, 0.61f },
};

static const FogBank sSandBanks[2] = {
    { 0.30f, -4.5f, 1.4f, 0.83f, 0.00f, 0.00f },
    { 0.95f, -3.2f, 0.9f, 0.66f, 0.43f, 0.29f },
};

static float sFogNodeX[VOXEL_FOG_GRID + 1];
static float sFogNodeZ[VOXEL_FOG_GRID + 1];
static float sFogNodeY[VOXEL_FOG_GRID + 1][VOXEL_FOG_GRID + 1];
static uint8_t sFogNodeEdgeA[VOXEL_FOG_GRID + 1][VOXEL_FOG_GRID + 1];
static int32_t sFogSnapCellX = 0x7FFFFFFF;
static int32_t sFogSnapCellZ = 0x7FFFFFFF;
static bool sFogCacheValid = false;

// World coordinates are relative to the current map, so a walking crossing moves every tile by
// the new map's origin. The world-anchored atmosphere (fog, motes, weather, rays, wisps) wraps its
// pattern on coordinates + this anchor, which absorbs every crossing, so nothing re-spawns at the
// border. Whole tiles; a crossing adds at most a map width, so int32 cannot overflow in play.
static int32_t sAtmoAnchorX = 0;
static int32_t sAtmoAnchorZ = 0;
static void SurfaceFx_ShiftCoordinates(int32_t shiftX, int32_t shiftZ);

#define ATMO_TWO_PI 6.283185307179586

// x reduced to [0, period), in double: a product of a long session's clock keeps its fraction.
static inline double AtmoWrapD(const double x, const double period)
{
    return x - floor(x / period) * period;
}

// The anchor reduced to [0, period): added to an effect's camera or UV origin once per pass, so
// particle loops keep their cost and floats keep their precision. `period` is in whole tiles.
static inline float AtmoAnchorWrap(const int32_t anchor, const int32_t period)
{
    int32_t r = anchor % period;
    if (r < 0)
        r += period;
    return (float)r;
}


typedef struct
{
    bool flake;
    uint32_t minCount, maxCount;
    float vx, vy, vz;
    float width, length;
    uint32_t colour;
    float alpha;
    float nearStart, nearRamp;
} WeatherKind;

static const WeatherKind sRainKind = {
    false, 56, 96,
    -2.2f, -15.0f, 0.6f, 0.035f, 1.05f, 0xFFF4E8DEu, 0.95f, 2.0f, 2.5f,
};

static const WeatherKind sSandKind = {
    false, 128, 340,
    -14.0f, -1.3f, 4.0f, 0.062f, 0.95f, 0xFF5488B8u, 0.92f, 2.0f, 2.5f,
};

static const WeatherKind sAshKind = {
    true, 96, 256,
    0.55f, -0.85f, 0.25f, 0.14f, 0.14f, 0xFFE2E6EAu, 0.88f, 2.0f, 2.5f,
};

static const WeatherKind sSnowKind = {
    true, 88, 220,
    -0.85f, -1.65f, 0.45f, 0.15f, 0.15f, 0xFFFFFFFFu, 0.95f, 2.0f, 2.5f,
};

static float sWeatherSeeds[VOXEL_WEATHER_COUNT][3];


typedef struct
{
    float x, z, y, bobAmp, bobSpeed, bobPhase;
    float driftX, driftZ, glintSpeed, glintPhase;
} MoteSpec;

static const MoteSpec sMoteSpecs[VOXEL_MOTE_COUNT] = {
    {  0.7f,  1.2f, 0.95f, 0.22f, 0.9f, 0.2f,  0.18f,  0.07f, 2.1f, 0.0f },
    {  3.4f,  2.8f, 1.35f, 0.28f, 0.7f, 1.4f, -0.14f,  0.11f, 1.7f, 1.1f },
    {  6.1f,  0.6f, 0.75f, 0.18f, 1.1f, 2.7f,  0.12f, -0.09f, 2.4f, 2.3f },
    {  8.9f,  4.1f, 1.60f, 0.30f, 0.8f, 3.9f,  0.20f,  0.06f, 1.9f, 3.5f },
    { 11.3f,  1.9f, 1.10f, 0.24f, 1.0f, 5.1f, -0.16f,  0.13f, 2.2f, 4.7f },
    { 14.2f,  3.5f, 0.85f, 0.20f, 0.9f, 0.8f,  0.15f, -0.08f, 1.6f, 5.9f },
    { 16.8f,  0.9f, 1.45f, 0.26f, 0.7f, 2.0f, -0.19f,  0.10f, 2.5f, 0.6f },
    { 19.5f,  2.4f, 1.00f, 0.22f, 1.1f, 3.3f,  0.13f,  0.12f, 1.8f, 1.8f },
    {  1.8f,  5.6f, 1.25f, 0.25f, 0.8f, 4.5f, -0.11f, -0.10f, 2.3f, 3.0f },
    {  4.6f,  7.2f, 0.80f, 0.19f, 1.0f, 5.7f,  0.17f,  0.08f, 2.0f, 4.2f },
    {  7.4f,  6.0f, 1.50f, 0.29f, 0.7f, 1.1f, -0.15f,  0.09f, 1.7f, 5.4f },
    { 10.1f,  8.3f, 0.90f, 0.21f, 0.9f, 2.4f,  0.19f, -0.07f, 2.4f, 0.3f },
    { 12.9f,  5.2f, 1.30f, 0.27f, 1.1f, 3.6f, -0.13f,  0.14f, 1.9f, 1.5f },
    { 15.7f,  7.7f, 1.05f, 0.23f, 0.8f, 4.8f,  0.16f, -0.11f, 2.1f, 2.7f },
    { 18.3f,  6.4f, 0.70f, 0.17f, 1.0f, 6.0f, -0.18f,  0.06f, 2.6f, 3.9f },
    { 20.9f,  8.9f, 1.55f, 0.30f, 0.7f, 0.5f,  0.14f,  0.10f, 1.6f, 5.1f },
    {  0.4f, 10.5f, 1.15f, 0.24f, 0.9f, 1.7f,  0.20f, -0.08f, 2.2f, 0.9f },
    {  3.1f, 12.1f, 0.85f, 0.20f, 1.1f, 2.9f, -0.12f,  0.12f, 1.8f, 2.1f },
    {  5.9f,  9.8f, 1.40f, 0.28f, 0.8f, 4.1f,  0.15f,  0.07f, 2.5f, 3.3f },
    {  8.6f, 11.4f, 0.95f, 0.22f, 1.0f, 5.3f, -0.17f, -0.09f, 2.0f, 4.5f },
    { 11.5f, 10.2f, 1.65f, 0.31f, 0.7f, 0.7f,  0.11f,  0.13f, 1.7f, 5.7f },
    { 14.0f, 12.7f, 0.78f, 0.18f, 0.9f, 1.9f, -0.19f,  0.08f, 2.3f, 0.4f },
    { 16.9f,  9.5f, 1.20f, 0.25f, 1.1f, 3.1f,  0.16f, -0.10f, 1.9f, 1.6f },
    { 19.7f, 11.8f, 1.00f, 0.21f, 0.8f, 4.3f, -0.14f,  0.11f, 2.4f, 2.8f },
    {  2.2f, 14.3f, 1.48f, 0.27f, 1.0f, 5.5f,  0.18f,  0.06f, 1.6f, 4.0f },
    {  5.0f, 16.0f, 0.88f, 0.20f, 0.7f, 0.9f, -0.15f, -0.12f, 2.1f, 5.2f },
    {  7.8f, 13.6f, 1.32f, 0.26f, 0.9f, 2.2f,  0.13f,  0.09f, 2.5f, 0.2f },
    { 10.6f, 15.2f, 0.72f, 0.18f, 1.1f, 3.4f, -0.20f,  0.07f, 1.8f, 1.4f },
    { 13.4f, 16.8f, 1.58f, 0.29f, 0.8f, 4.6f,  0.17f, -0.08f, 2.2f, 2.6f },
    { 16.1f, 14.1f, 1.08f, 0.23f, 1.0f, 5.8f, -0.11f,  0.14f, 1.9f, 3.8f },
    { 18.9f, 15.7f, 0.92f, 0.21f, 0.7f, 1.2f,  0.14f, -0.09f, 2.3f, 5.0f },
    { 21.4f, 17.3f, 1.38f, 0.27f, 0.9f, 2.5f, -0.16f,  0.10f, 1.7f, 6.1f },
};

// ---- Frustum Culling & Single-Source Camera Matrices ----
typedef struct {
    float a, b, c, d;
} FrustumPlane;

typedef struct {
    float proj[16];
    float view[16];
    float mvp[16];
    float zNear;
    float zFar;
    float minX, maxX, minZ, maxZ;
    float yMin, yMax;
} VoxelViewState;

static FrustumPlane sFrustumPlanes[6];
static VoxelViewState sViewState;

static void Matrix4_Multiply(float *out, const float *a, const float *b)
{
    for (int32_t i = 0; i < 4; i++)
    {
        for (int32_t j = 0; j < 4; j++)
        {
            out[i * 4 + j] = a[0 * 4 + j] * b[i * 4 + 0] +
                             a[1 * 4 + j] * b[i * 4 + 1] +
                             a[2 * 4 + j] * b[i * 4 + 2] +
                             a[3 * 4 + j] * b[i * 4 + 3];
        }
    }
}

static void ExtractFrustumPlanes(const float *m)
{
    // Left: row 3 + row 0
    sFrustumPlanes[0].a = m[3] + m[0];
    sFrustumPlanes[0].b = m[7] + m[4];
    sFrustumPlanes[0].c = m[11] + m[8];
    sFrustumPlanes[0].d = m[15] + m[12];

    // Right: row 3 - row 0
    sFrustumPlanes[1].a = m[3] - m[0];
    sFrustumPlanes[1].b = m[7] - m[4];
    sFrustumPlanes[1].c = m[11] - m[8];
    sFrustumPlanes[1].d = m[15] - m[12];

    // Bottom: row 3 + row 1
    sFrustumPlanes[2].a = m[3] + m[1];
    sFrustumPlanes[2].b = m[7] + m[5];
    sFrustumPlanes[2].c = m[11] + m[9];
    sFrustumPlanes[2].d = m[15] + m[13];

    // Top: row 3 - row 1
    sFrustumPlanes[3].a = m[3] - m[1];
    sFrustumPlanes[3].b = m[7] - m[5];
    sFrustumPlanes[3].c = m[11] - m[9];
    sFrustumPlanes[3].d = m[15] - m[13];

    // Near: row 3 + row 2
    sFrustumPlanes[4].a = m[3] + m[2];
    sFrustumPlanes[4].b = m[7] + m[6];
    sFrustumPlanes[4].c = m[11] + m[10];
    sFrustumPlanes[4].d = m[15] + m[14];

    // Far: row 3 - row 2
    sFrustumPlanes[5].a = m[3] - m[2];
    sFrustumPlanes[5].b = m[7] - m[6];
    sFrustumPlanes[5].c = m[11] - m[10];
    sFrustumPlanes[5].d = m[15] - m[14];

    for (int32_t i = 0; i < 6; i++)
    {
        float len = sqrtf(sFrustumPlanes[i].a * sFrustumPlanes[i].a +
                          sFrustumPlanes[i].b * sFrustumPlanes[i].b +
                          sFrustumPlanes[i].c * sFrustumPlanes[i].c);
        if (len > 0.0f)
        {
            sFrustumPlanes[i].a /= len;
            sFrustumPlanes[i].b /= len;
            sFrustumPlanes[i].c /= len;
            sFrustumPlanes[i].d /= len;
        }
    }
}

static bool Frustum_IntersectsAABB(float minX, float minY, float minZ,
                                   float maxX, float maxY, float maxZ)
{
    for (int32_t i = 0; i < 6; i++)
    {
        const float px = (sFrustumPlanes[i].a > 0.0f) ? maxX : minX;
        const float py = (sFrustumPlanes[i].b > 0.0f) ? maxY : minY;
        const float pz = (sFrustumPlanes[i].c > 0.0f) ? maxZ : minZ;

        // Planes are unit-length, so signed distance is in world tiles; VOXEL_FRUSTUM_PLANE_SLACK
        // keeps tall roofs, tree crowns and south-edge structures from popping out at screen borders.
        if (sFrustumPlanes[i].a * px + sFrustumPlanes[i].b * py + sFrustumPlanes[i].c * pz + sFrustumPlanes[i].d < -VOXEL_FRUSTUM_PLANE_SLACK)
            return false;
    }
    return true;
}

static inline int32_t FloorDivChunk(int32_t a)
{
    int32_t q = a / CHUNK_SIZE;
    int32_t r = a % CHUNK_SIZE;
    if (r != 0 && ((r < 0) != (CHUNK_SIZE < 0)))
        --q;
    return q;
}

static bool Frustum_ChunkInView(int32_t cx, int32_t cy, const VoxelChunkSlot *slot)
{
    const VoxelMapInstance *inst = VoxelWorld_Instance(0);
    // Indoors (gyms, houses, Pokemon Centers, Marts) and caves:
    // Render all chunks in the layout so tall structures (11-tile gym walls, pillars, rock ceilings)
    // are never culled or vanish when moving around the room!
    if (inst != NULL && (inst->indoor || gMapHeader.mapType == MAP_TYPE_UNDERGROUND))
    {
        int32_t x0 = FloorDivChunk(inst->originX - sChunkOriginX) - 1;
        int32_t x1 = FloorDivChunk(inst->originX + inst->width - sChunkOriginX + CHUNK_SIZE - 1) + 1;
        int32_t y0 = FloorDivChunk(inst->originY - sChunkOriginY) - 1;
        int32_t y1 = FloorDivChunk(inst->originY + inst->height - sChunkOriginY + CHUNK_SIZE - 1) + 1;
        return (cx >= x0 && cx <= x1 && cy >= y0 && cy <= y1);
    }

    const float baseX0 = (float)(sChunkOriginX + cx * CHUNK_SIZE);
    const float baseX1 = (float)(sChunkOriginX + (cx + 1) * CHUNK_SIZE);
    const float baseZ0 = (float)(sChunkOriginY + cy * CHUNK_SIZE);
    const float baseZ1 = (float)(sChunkOriginY + (cy + 1) * CHUNK_SIZE);

    float minX, maxX, minZ, maxZ, minY, maxY;
    if (slot != NULL && slot->valid)
    {
        // Tight AABB from actual emitted vertices (Core 2) + building model reach
        minX = baseX0 - slot->bldWest - 0.5f;
        maxX = baseX1 + slot->bldEast + 0.5f;
        minZ = baseZ0 - slot->bldNorth - 0.5f;
        maxZ = baseZ1 + slot->bldSouth + 0.5f;
        minY = slot->minY;
        maxY = fmaxf(slot->maxY, slot->bldTop);
    }
    else
    {
        // Unbuilt chunk: conservative vertical column + oblique relief Z-stretch margin so tall
        // cliffs (e.g. Ever Grande City) and sea-level terraces are never missed before building.
        minX = baseX0 - 1.0f;
        maxX = baseX1 + 1.0f;
        minZ = baseZ0 - 2.0f;
        maxZ = baseZ1 + 6.0f;
        minY = sViewState.yMin - 1.0f;
        maxY = sViewState.yMax + 2.0f;
    }

    return Frustum_IntersectsAABB(minX, minY, minZ, maxX, maxY, maxZ);
}

// How many chunks west and north of the candidate area a chunk may stand and still have
// geometry (buildings or relief-stretched tall cliffs) reaching into the view.
#define VOXEL_BUILDING_REACH_CHUNKS_X 3
#define VOXEL_BUILDING_REACH_CHUNKS_Y 4

// Whether a built chunk's geometry (buildings or oblique relief-stretched cliffs/trees) reaches
// into the tile window [wx0, wx1] x [wz0, wz1].
static bool ChunkModelsReachWindow(const VoxelChunkSlot *slot, int32_t cx, int32_t cy,
                                   float wx0, float wx1, float wz0, float wz1)
{
    if (slot == NULL || !slot->valid)
        return false;
    if (slot->bldWest <= 0.0f && slot->bldEast <= 0.0f
     && slot->bldNorth <= 0.0f && slot->bldSouth <= 0.0f)
        return false;
    const float x0 = (float)(sChunkOriginX + cx * CHUNK_SIZE) - slot->bldWest;
    const float x1 = (float)(sChunkOriginX + (cx + 1) * CHUNK_SIZE) + slot->bldEast;
    const float z0 = (float)(sChunkOriginY + cy * CHUNK_SIZE) - slot->bldNorth;
    const float z1 = (float)(sChunkOriginY + (cy + 1) * CHUNK_SIZE) + slot->bldSouth;
    return x1 >= wx0 && x0 <= wx1 && z1 >= wz0 && z0 <= wz1;
}

// Whether the chunk would be in view once the camera has moved by (aheadX, aheadZ) tiles:
// the same test against the box moved the other way. Outdoors only (indoors everything in
// the layout is already drawn).
static bool Frustum_ChunkInViewAhead(int32_t cx, int32_t cy, float aheadX, float aheadZ)
{
    const float minX = (float)(sChunkOriginX + cx * CHUNK_SIZE) - 1.0f - aheadX;
    const float maxX = (float)(sChunkOriginX + (cx + 1) * CHUNK_SIZE) + 1.0f - aheadX;
    const float minZ = (float)(sChunkOriginY + cy * CHUNK_SIZE) - 1.0f - aheadZ;
    const float maxZ = (float)(sChunkOriginY + (cy + 1) * CHUNK_SIZE) + 1.0f - aheadZ;

    return Frustum_IntersectsAABB(minX, sViewState.yMin - 1.0f, minZ, maxX, sViewState.yMax + 2.0f, maxZ);
}

// Single-source camera matrices, dynamic zNear/zFar, projected ground bounds, and frustum planes.
static void ComputeViewAndFrustum(void)
{
    const float aspect = 960.0f / 544.0f;
    const float tanY = tanf(sCamera.fov * (3.14159265f / 360.0f));
    const float tanX = tanY * aspect;
    const float f = 1.0f / tanY;

    float forward[3], side[3], up[3];
    forward[0] = sCamera.targetX - sCamera.x;
    forward[1] = sCamera.targetY - sCamera.y;
    forward[2] = sCamera.targetZ - sCamera.z;
    float len = sqrtf(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    if (len > 1e-5f)
    {
        forward[0] /= len;
        forward[1] /= len;
        forward[2] /= len;
    }
    else
    {
        forward[0] = 0.0f;
        forward[1] = -0.5735764f;
        forward[2] = -0.8191520f;
    }

    side[0] = -forward[2];
    side[1] = 0.0f;
    side[2] = forward[0];
    len = sqrtf(side[0] * side[0] + side[2] * side[2]);
    if (len > 1e-5f)
    {
        side[0] /= len;
        side[2] /= len;
    }
    else
    {
        side[0] = 1.0f;
        side[2] = 0.0f;
    }

    up[0] = side[1] * forward[2] - side[2] * forward[1];
    up[1] = side[2] * forward[0] - side[0] * forward[2];
    up[2] = side[0] * forward[1] - side[1] * forward[0];

    // Project the 4 frustum corner rays onto [yMin, yMax] to obtain exact world XZ bounds.
    // yMin always reaches Hoenn sea level (0) even atop tall plateaus like Ever Grande City;
    // yMax reaches tall cliffs + roofs even when standing at sea level.
    float tallest = VoxelBuildings_MaxTop();
    if (tallest < 4.0f) tallest = 4.0f;
    if (tallest > 14.0f) tallest = 14.0f;
    const float yMin = fminf(sCamera.ground - 4.0f, -2.5f);
    const float yMax = fmaxf(sCamera.ground + tallest, 18.0f);
    // Include a height sample just below the camera eye so bottom rays (sy=-1) project to
    // the true south-most visible roofs/canopies even when yMax sits above sCamera.y.
    const float yNearEye = fmaxf(yMin, fminf(yMax, sCamera.y - 0.5f));
    const float heights[3] = { yMin, yNearEye, yMax };

    float minX = sCamera.targetX, maxX = sCamera.targetX;
    float minZ = sCamera.targetZ, maxZ = sCamera.targetZ;
    const float maxRayT = 4.0f * VOXEL_VIEW_REACH_MAX;
    for (int32_t sy = -1; sy <= 1; sy += 2)
    {
        for (int32_t sx = -1; sx <= 1; sx += 2)
        {
            const float rx = forward[0] + (float)sx * tanX * side[0] + (float)sy * tanY * up[0];
            const float ry = forward[1] + (float)sx * tanX * side[1] + (float)sy * tanY * up[1];
            const float rz = forward[2] + (float)sx * tanX * side[2] + (float)sy * tanY * up[2];
            for (int32_t h = 0; h < 3; ++h)
            {
                float t = (ry < -0.001f) ? (heights[h] - sCamera.y) / ry : -1.0f;
                if (t < 0.0f || t > maxRayT)
                    t = maxRayT;
                const float wx = sCamera.x + rx * t;
                const float wz = sCamera.z + rz * t;
                if (wx < minX) minX = wx;
                if (wx > maxX) maxX = wx;
                if (wz < minZ) minZ = wz;
                if (wz > maxZ) maxZ = wz;
            }
        }
    }

    const float loX = sCamera.targetX - VOXEL_VIEW_REACH_MAX;
    const float hiX = sCamera.targetX + VOXEL_VIEW_REACH_MAX;
    const float loZ = sCamera.targetZ - VOXEL_VIEW_REACH_MAX;
    const float hiZ = sCamera.targetZ + VOXEL_VIEW_REACH_MAX;
    if (minX < loX) minX = loX;
    if (maxX > hiX) maxX = hiX;
    if (minZ < loZ) minZ = loZ;
    if (maxZ > hiZ) maxZ = hiZ;

    minX -= VOXEL_VIEW_SLACK_TILES;
    maxX += VOXEL_VIEW_SLACK_TILES;
    minZ -= VOXEL_VIEW_SLACK_TILES;
    maxZ += VOXEL_VIEW_SLACK_TILES;

    // Dynamic zNear/zFar keep zFar/zNear <= ~65 even for high aerial poses (24-bit Z-buffer).
    const float eyeHeight = fabsf(sCamera.y - sCamera.ground);
    const float zNear = Voxel_ClampF(eyeHeight * 0.08f, VOXEL_VIEW_NEAR_MIN, VOXEL_VIEW_NEAR_MAX);
    float maxDistSq = 0.0f;
    const float cxList[2] = { minX, maxX };
    const float czList[2] = { minZ, maxZ };
    const float dyEye = yMin - sCamera.y;
    for (int32_t ix = 0; ix < 2; ++ix)
    {
        for (int32_t iz = 0; iz < 2; ++iz)
        {
            const float dxEye = cxList[ix] - sCamera.x;
            const float dzEye = czList[iz] - sCamera.z;
            const float d2 = dxEye * dxEye + dyEye * dyEye + dzEye * dzEye;
            if (d2 > maxDistSq)
                maxDistSq = d2;
        }
    }
    const float zFar = Voxel_ClampF(sqrtf(maxDistSq) + VOXEL_VIEW_FAR_MARGIN,
                                    VOXEL_VIEW_FAR_MIN, VOXEL_VIEW_FAR_MAX);

    memset(sViewState.proj, 0, sizeof(sViewState.proj));
    sViewState.proj[0]  = f / aspect;
    sViewState.proj[5]  = f;
    sViewState.proj[10] = (zFar + zNear) / (zNear - zFar);
    sViewState.proj[11] = -1.0f;
    sViewState.proj[14] = (2.0f * zFar * zNear) / (zNear - zFar);

    sViewState.view[0]  = side[0];     sViewState.view[4]  = side[1];     sViewState.view[8]  = side[2];
    sViewState.view[12] = -(side[0] * sCamera.x + side[1] * sCamera.y + side[2] * sCamera.z);
    sViewState.view[1]  = up[0];       sViewState.view[5]  = up[1];       sViewState.view[9]  = up[2];
    sViewState.view[13] = -(up[0] * sCamera.x + up[1] * sCamera.y + up[2] * sCamera.z);
    sViewState.view[2]  = -forward[0]; sViewState.view[6]  = -forward[1]; sViewState.view[10] = -forward[2];
    sViewState.view[14] = (forward[0] * sCamera.x + forward[1] * sCamera.y + forward[2] * sCamera.z);
    sViewState.view[3]  = 0.0f;        sViewState.view[7]  = 0.0f;        sViewState.view[11] = 0.0f;
    sViewState.view[15] = 1.0f;

    sViewState.zNear = zNear;
    sViewState.zFar  = zFar;
    sViewState.minX  = minX;
    sViewState.maxX  = maxX;
    sViewState.minZ  = minZ;
    sViewState.maxZ  = maxZ;
    sViewState.yMin  = yMin;
    sViewState.yMax  = yMax;

    Matrix4_Multiply(sViewState.mvp, sViewState.proj, sViewState.view);
    ExtractFrustumPlanes(sViewState.mvp);
}

// Exact trapezoidal X span of the frustum across a chunk row [rowZ0, rowZ1] over [yMin, yMax].
static void Frustum_RowBoundsX(float rowZ0, float rowZ1, float *outMinX, float *outMaxX)
{
    const FrustumPlane *const pL = &sFrustumPlanes[0];
    const FrustumPlane *const pR = &sFrustumPlanes[1];
    if (pL->a > 0.01f && pR->a < -0.01f)
    {
        float xMin = FLT_MAX;
        float xMax = -FLT_MAX;
        const float zs[2] = { rowZ0, rowZ1 };
        const float ys[2] = { sViewState.yMin, sViewState.yMax };
        for (int32_t iz = 0; iz < 2; ++iz)
        {
            for (int32_t iy = 0; iy < 2; ++iy)
            {
                const float xL = -(pL->b * ys[iy] + pL->c * zs[iz] + pL->d) / pL->a;
                const float xR = -(pR->b * ys[iy] + pR->c * zs[iz] + pR->d) / pR->a;
                if (xL < xMin) xMin = xL;
                if (xR > xMax) xMax = xR;
            }
        }
        xMin -= VOXEL_VIEW_SLACK_TILES;
        xMax += VOXEL_VIEW_SLACK_TILES;
        if (xMin < sViewState.minX) xMin = sViewState.minX;
        if (xMax > sViewState.maxX) xMax = sViewState.maxX;
        *outMinX = xMin;
        *outMaxX = xMax;
        return;
    }
    *outMinX = sViewState.minX;
    *outMaxX = sViewState.maxX;
}

// Convert VoxelVertex shade into Emerald 3DS warm-sun / cool-shadow graded RGB
static inline void BakeGradedVertexColor(float shade, bool outdoor, uint8_t *outR, uint8_t *outG, uint8_t *outB)
{
    if (shade < 0.0f) shade = 0.0f;
    if (shade > 1.0f) shade = 1.0f;
    if (!outdoor)
    {
        const uint8_t c = (uint8_t)(shade * 255.0f + 0.5f);
        *outR = c;
        *outG = c;
        *outB = c;
        return;
    }

    float t = (shade - VOXEL_GRADE_SHADE_LOW) * (1.0f / (1.0f - VOXEL_GRADE_SHADE_LOW));
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float r = shade * (0.93f + (1.00f - 0.93f) * t);
    float g = shade * (0.97f + (0.99f - 0.97f) * t);
    float b = shade * (1.05f + (0.95f - 1.05f) * t);
    if (r > 1.0f) r = 1.0f;
    if (g > 1.0f) g = 1.0f;
    if (b > 1.0f) b = 1.0f;

    *outR = (uint8_t)(r * 255.0f + 0.5f);
    *outG = (uint8_t)(g * 255.0f + 0.5f);
    *outB = (uint8_t)(b * 255.0f + 0.5f);
}

// Convert VoxelVertex (floats) to VoxelGlVertex (hardware vertex format)
static inline void PackVoxelVertices(const VoxelVertex *const src, VoxelGlVertex *const dst, const uint32_t count,
                                     const bool outdoor)
{
    for (uint32_t i = 0; i < count; i++)
    {
        dst[i].x = src[i].x;
        dst[i].y = src[i].y;
        dst[i].z = src[i].z;
        dst[i].u = src[i].u;
        dst[i].v = src[i].v;
        BakeGradedVertexColor(src[i].shade, outdoor, &dst[i].r, &dst[i].g, &dst[i].b);
        dst[i].a = 255;
    }
}

// Per-frame lighting & distance haze parameters applied on the CPU during batch assembly
// (replaces expensive per-fragment GL_FOG and eliminates Pass 5.9's fullscreen multiply quad)
typedef struct
{
    uint16_t scaleR, scaleG, scaleB; // 8.8 fixed-point [0..384]
    uint8_t hazeR, hazeG, hazeB;
    bool scaleIdentity;
    bool hasHaze;
    float camX, camY, camZ;
    float hazeStart2;
    float hazeInvSpan2;
} VoxelFrameGrade;

static inline void BuildFrameGrade(VoxelFrameGrade *const out, const VoxelLight *const light, const bool indoor)
{
    if (indoor)
    {
        out->scaleR = 256u;
        out->scaleG = 256u;
        out->scaleB = 256u;
        out->hazeR = 0u;
        out->hazeG = 0u;
        out->hazeB = 0u;
        out->scaleIdentity = true;
        out->hasHaze = false;
        out->camX = sCamera.x;
        out->camY = sCamera.y;
        out->camZ = sCamera.z;
        out->hazeStart2 = 0.0f;
        out->hazeInvSpan2 = 0.0f;
        return;
    }

    // BakeGradedVertexColor already incorporates clear-daylight warm/cool balance {1.00, 0.99, 0.95};
    // normalize against clear-sky baseline so noon stays crisp and weather/dusk/night modulate accurately.
    float sr = light->sun[0];
    float sg = light->sun[1] * (1.0f / 0.99f);
    float sb = light->sun[2] * (1.0f / 0.95f);
    if (sr < 0.0f) sr = 0.0f;
    if (sg < 0.0f) sg = 0.0f;
    if (sb < 0.0f) sb = 0.0f;
    if (sr > 1.5f) sr = 1.5f;
    if (sg > 1.5f) sg = 1.5f;
    if (sb > 1.5f) sb = 1.5f;

    out->scaleR = (uint16_t)(sr * 256.0f + 0.5f);
    out->scaleG = (uint16_t)(sg * 256.0f + 0.5f);
    out->scaleB = (uint16_t)(sb * 256.0f + 0.5f);
    // If all color components clamp to 1.0f or are within [246u..] (>= 0.96),
    // modulating with GL_DST_COLOR is either a literal no-op or visually imperceptible (< 4% change).
    // Skipping the fullscreen blend quad eliminates PowerVR SGX543 TBDR 960x544 framebuffer readback, saving 1.5-2.5 ms GPU time.
    out->scaleIdentity = (out->scaleR >= 246u && out->scaleG >= 246u && out->scaleB >= 246u);

    out->camX = sCamera.x;
    out->camY = sCamera.y;
    out->camZ = sCamera.z;

    // Nominal clear overworld atmospheric haze is 0.12f; activate software distance haze
    // calculations only in true fog/sand/ash weather (> 0.15f) so standard clear daylight
    // takes the direct fast memcpy path instead of calculating distance on 25k vertices per frame!
    if (light->haze > 0.15f)
    {
        const float pitchRad = sCamera.pitch * (3.14159265f / 180.0f);
        const float eye = sCamera.distance / cosf(pitchRad);
        const float fogStart = eye * light->hazeStart;
        const float fogEnd = fogStart + (eye * light->hazeRamp) / fmaxf(light->haze, 0.05f);
        const float start2 = fogStart * fogStart;
        const float end2 = fogEnd * fogEnd;
        out->hasHaze = (end2 > start2 + 0.01f);
        out->hazeStart2 = start2;
        out->hazeInvSpan2 = out->hasHaze ? (1.0f / (end2 - start2)) : 0.0f;
        out->hazeR = (uint8_t)(Voxel_ClampF(light->hazeRgb[0], 0.0f, 1.0f) * 255.0f + 0.5f);
        out->hazeG = (uint8_t)(Voxel_ClampF(light->hazeRgb[1], 0.0f, 1.0f) * 255.0f + 0.5f);
        out->hazeB = (uint8_t)(Voxel_ClampF(light->hazeRgb[2], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
    else
    {
        out->hasHaze = false;
        out->hazeStart2 = 0.0f;
        out->hazeInvSpan2 = 0.0f;
        out->hazeR = 0u;
        out->hazeG = 0u;
        out->hazeB = 0u;
    }
}

static inline bool ChunkIsInsideHazeStart(const VoxelChunkSlot *const chunk, const VoxelFrameGrade *const grade)
{
    if (!grade->hasHaze)
        return true;

    const float camX = grade->camX;
    const float camZ = grade->camZ;
    const float minX = (float)(sChunkOriginX + chunk->chunkX * CHUNK_SIZE);
    const float maxX = minX + (float)CHUNK_SIZE;
    const float minZ = (float)(sChunkOriginY + chunk->chunkY * CHUNK_SIZE);
    const float maxZ = minZ + (float)CHUNK_SIZE;

    const float dx = fmaxf(fabsf(minX - camX), fabsf(maxX - camX));
    const float dz = fmaxf(fabsf(minZ - camZ), fabsf(maxZ - camZ));
    const float dy = fabsf(grade->camY);
    const float maxDist2 = dx * dx + dz * dz + dy * dy;

    return (maxDist2 <= grade->hazeStart2);
}

static inline void BatchCopyVerticesGraded(VoxelGlVertex *const dst, const VoxelGlVertex *const src,
                                           const uint32_t count, const VoxelFrameGrade *const grade)
{
    if (grade->scaleIdentity && !grade->hasHaze)
    {
        if (dst != src)
            memcpy(dst, src, count * sizeof(VoxelGlVertex));
        return;
    }

    const uint32_t sr = grade->scaleR;
    const uint32_t sg = grade->scaleG;
    const uint32_t sb = grade->scaleB;
    const bool hasHaze = grade->hasHaze;
    const float camX = grade->camX;
    const float camY = grade->camY;
    const float camZ = grade->camZ;
    const float start2 = grade->hazeStart2;
    const float invSpan2 = grade->hazeInvSpan2;
    const int32_t hr = grade->hazeR;
    const int32_t hg = grade->hazeG;
    const int32_t hb = grade->hazeB;

    if (!hasHaze)
    {
        if (dst != src)
            memcpy(dst, src, count * sizeof(VoxelGlVertex));

        for (uint32_t i = 0; i < count; ++i)
        {
            const uint32_t r = ((uint32_t)dst[i].r * sr) >> 8;
            const uint32_t g = ((uint32_t)dst[i].g * sg) >> 8;
            const uint32_t b = ((uint32_t)dst[i].b * sb) >> 8;
            dst[i].r = (uint8_t)(r > 255u ? 255u : r);
            dst[i].g = (uint8_t)(g > 255u ? 255u : g);
            dst[i].b = (uint8_t)(b > 255u ? 255u : b);
        }
        return;
    }

    if (dst != src)
        memcpy(dst, src, count * sizeof(VoxelGlVertex));

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t r = ((uint32_t)dst[i].r * sr) >> 8;
        uint32_t g = ((uint32_t)dst[i].g * sg) >> 8;
        uint32_t b = ((uint32_t)dst[i].b * sb) >> 8;
        if (r > 255u) r = 255u;
        if (g > 255u) g = 255u;
        if (b > 255u) b = 255u;

        const float dx = dst[i].x - camX;
        const float dy = dst[i].y - camY;
        const float dz = dst[i].z - camZ;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > start2)
        {
            float ft = (d2 - start2) * invSpan2;
            if (ft > 1.0f) ft = 1.0f;
            const int32_t t256 = (int32_t)(ft * 256.0f);
            r = (uint32_t)((int32_t)r + (((hr - (int32_t)r) * t256) >> 8));
            g = (uint32_t)((int32_t)g + (((hg - (int32_t)g) * t256) >> 8));
            b = (uint32_t)((int32_t)b + (((hb - (int32_t)b) * t256) >> 8));
        }

        dst[i].r = (uint8_t)r;
        dst[i].g = (uint8_t)g;
        dst[i].b = (uint8_t)b;
    }
}

// ---- Bounded 24-Slot Building LRU Cache + Core 2 Async I/O Worker ----
typedef struct
{
    int16_t page;
    GLuint tex;
    GLuint glowTex;
    uint32_t lastUsedFrame;
} VoxelBuildingCacheSlot;

static VoxelBuildingCacheSlot sBuildingCache[VOXEL_BUILDING_CACHE_SLOTS];
static int8_t sBuildingPageToSlot[VOXEL_MAX_BUILDING_PAGES];
static uint8_t sBuildingPageHasAlpha[VOXEL_MAX_BUILDING_PAGES];
// Per-page 32x32 UV occupancy bitmask of glow > 0 texels (published with release fence on load).
static uint32_t sBuildingGlowGrid[VOXEL_MAX_BUILDING_PAGES][32];
static volatile uint8_t sBuildingGlowGridReady[VOXEL_MAX_BUILDING_PAGES];
static bool sSyncBuildingLoads = true;

static SDL_Thread *sBuildingIoThread = NULL;
static SDL_mutex *sBuildingIoMutex = NULL;
static SDL_sem *sBuildingReqSem = NULL;
static SDL_sem *sBuildingConsumedSem = NULL;
static SDL_atomic_t sBuildingIoStop;

static int16_t sBuildingReqQueue[VOXEL_BUILDING_REQ_CAP];
static uint8_t sBuildingReqHead = 0;
static uint8_t sBuildingReqTail = 0;
static uint8_t sBuildingReqCount = 0;
static bool sBuildingPageQueued[VOXEL_MAX_BUILDING_PAGES];

static uint16_t sWorkerMortonBuf[512 * 512];
static uint16_t sWorkerLinearBuf[512 * 512];
static uint8_t sWorkerGlowMortonBuf[512 * 512];
static uint16_t sWorkerGlowLinearBuf[512 * 512];
static uint8_t sBuildingGlowMortonScratch[512 * 512];
static int16_t sWorkerReadyPage = -1;
static uint32_t sWorkerReadyW = 0;
static uint32_t sWorkerReadyTexH = 0;
static bool sWorkerReadyHasAlpha = false;
static bool sWorkerReadyHasGlow = false;

static bool ScanLinearPageAlpha(const uint16_t *const texels, const uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        if ((texels[i] & 1u) == 0u)
            return true;
    }
    return false;
}

static GLuint UploadBuildingPageToLru(const int32_t page, const uint32_t bw, const uint32_t btexH,
                                      const bool hasAlpha, const uint16_t *const linearPixels,
                                      const uint16_t *const linearGlowPixels)
{
    const int32_t existing = sBuildingPageToSlot[page];
    if (existing >= 0)
    {
        sBuildingCache[existing].lastUsedFrame = sFrameCounter;
        return sBuildingCache[existing].tex;
    }

    int32_t chosen = -1;
    uint32_t oldestFrame = UINT32_MAX;
    for (int32_t i = 0; i < VOXEL_BUILDING_CACHE_SLOTS; ++i)
    {
        if (sBuildingCache[i].page < 0 || sBuildingCache[i].tex == 0)
        {
            chosen = i;
            break;
        }
        if (sBuildingCache[i].lastUsedFrame < oldestFrame)
        {
            oldestFrame = sBuildingCache[i].lastUsedFrame;
            chosen = i;
        }
    }
    if (chosen < 0)
        chosen = 0;

    VoxelBuildingCacheSlot *const slot = &sBuildingCache[chosen];
    if (slot->page >= 0 && slot->page < VOXEL_MAX_BUILDING_PAGES)
    {
        sBuildingPageToSlot[slot->page] = -1;
    }
    if (slot->tex != 0)
    {
        glDeleteTextures(1, &slot->tex);
        slot->tex = 0;
    }
    if (slot->glowTex != 0)
    {
        glDeleteTextures(1, &slot->glowTex);
        slot->glowTex = 0;
    }

    glGenTextures(1, &slot->tex);
    glBindTexture(GL_TEXTURE_2D, slot->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)bw, (GLsizei)btexH,
                 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, linearPixels);

    if (linearGlowPixels != NULL)
    {
        glGenTextures(1, &slot->glowTex);
        glBindTexture(GL_TEXTURE_2D, slot->glowTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)bw, (GLsizei)btexH,
                     0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, linearGlowPixels);
    }

    slot->page = (int16_t)page;
    slot->lastUsedFrame = sFrameCounter;
    sBuildingPageToSlot[page] = (int8_t)chosen;
    sBuildingPageHasAlpha[page] = hasAlpha ? 2u : 1u;
    return slot->tex;
}

static void UploadReadyBuildingPage(void)
{
    if (sBuildingIoMutex == NULL)
        return;

    SDL_LockMutex(sBuildingIoMutex);
    const int32_t page = sWorkerReadyPage;
    const uint32_t bw = sWorkerReadyW;
    const uint32_t btexH = sWorkerReadyTexH;
    const bool hasAlpha = sWorkerReadyHasAlpha;
    const bool hasGlow = sWorkerReadyHasGlow;
    SDL_UnlockMutex(sBuildingIoMutex);

    if (page < 0 || page >= VOXEL_MAX_BUILDING_PAGES)
        return;

    UploadBuildingPageToLru(page, bw, btexH, hasAlpha, sWorkerLinearBuf,
                            hasGlow ? sWorkerGlowLinearBuf : NULL);

    SDL_LockMutex(sBuildingIoMutex);
    sBuildingPageQueued[page] = false;
    sWorkerReadyPage = -1;
    sWorkerReadyHasGlow = false;
    SDL_UnlockMutex(sBuildingIoMutex);

    SDL_SemPost(sBuildingConsumedSem);
}

static int32_t VoxelBuildingIoWorkerMain(void *data)
{
    (void)data;
#ifdef PLATFORM_VITA
    VitaPlatform_PinCurrentThread("VoxelIoWorker", 2);
#endif

    while (!SDL_AtomicGet(&sBuildingIoStop))
    {
        if (SDL_SemWait(sBuildingReqSem) != 0)
            continue;
        if (SDL_AtomicGet(&sBuildingIoStop))
            break;

        SDL_LockMutex(sBuildingIoMutex);
        if (sBuildingReqCount == 0)
        {
            SDL_UnlockMutex(sBuildingIoMutex);
            continue;
        }
        const int32_t page = sBuildingReqQueue[sBuildingReqHead];
        sBuildingReqHead = (uint8_t)((sBuildingReqHead + 1u) % VOXEL_BUILDING_REQ_CAP);
        sBuildingReqCount--;

        if (page < 0 || page >= VOXEL_MAX_BUILDING_PAGES || sBuildingPageToSlot[page] >= 0)
        {
            if (page >= 0 && page < VOXEL_MAX_BUILDING_PAGES)
                sBuildingPageQueued[page] = false;
            SDL_UnlockMutex(sBuildingIoMutex);
            continue;
        }
        // Unlock immediately: Core 2 now reads independently from sWorkerPageFile without blocking Core 0!
        SDL_UnlockMutex(sBuildingIoMutex);

        uint32_t bw = 0, bh = 0, btexH = 0;
        bool readOk = false;
        bool hasGlow = false;
        if (VoxelBuildings_PageSize((uint32_t)page, &bw, &bh, &btexH) && (bw * btexH <= 512u * 512u))
        {
            memset(sWorkerMortonBuf, 0, (size_t)bw * btexH * sizeof(uint16_t));
            readOk = VoxelBuildings_ReadPageWorker((uint32_t)page, 0, bw * bh, sWorkerMortonBuf);
            if (readOk && VoxelBuildings_PageHasGlow((uint32_t)page))
            {
                hasGlow = VoxelBuildings_ReadGlowWorker((uint32_t)page, 0, bw * bh, sWorkerGlowMortonBuf);
            }
        }

        if (!readOk)
        {
            SDL_LockMutex(sBuildingIoMutex);
            sBuildingPageQueued[page] = false;
            SDL_UnlockMutex(sBuildingIoMutex);
            continue;
        }

        // Unswizzle Morton -> Linear & detect cutout alpha on Core 2 (zero load on Core 0 / Core 1)
        Voxel_UnswizzleMortonToLinearEx(sWorkerMortonBuf, sWorkerLinearBuf, bw, bh, btexH);
        if (hasGlow)
        {
            Voxel_UnswizzleMortonGlowToRgba4444(sWorkerGlowMortonBuf, sWorkerGlowLinearBuf, bw, bh, btexH,
                                                sBuildingGlowGrid[page]);
            __atomic_store_n(&sBuildingGlowGridReady[page], 1u, __ATOMIC_RELEASE);
        }
        const bool hasAlpha = ScanLinearPageAlpha(sWorkerLinearBuf, bw * bh);

        SDL_LockMutex(sBuildingIoMutex);
        sWorkerReadyPage = (int16_t)page;
        sWorkerReadyW = bw;
        sWorkerReadyTexH = btexH;
        sWorkerReadyHasAlpha = hasAlpha;
        sWorkerReadyHasGlow = hasGlow;
        SDL_UnlockMutex(sBuildingIoMutex);

        SDL_SemWait(sBuildingConsumedSem);
    }
    VoxelBuildings_CloseWorkerFile();
    return 0;
}

static GLuint RequestOrLoadBuildingPage(const int32_t page, const bool synchronous)
{
    if (page < 0 || page >= VOXEL_MAX_BUILDING_PAGES)
        return 0;

    const int32_t cachedSlot = sBuildingPageToSlot[page];
    if (cachedSlot >= 0)
    {
        sBuildingCache[cachedSlot].lastUsedFrame = sFrameCounter;
        return sBuildingCache[cachedSlot].tex;
    }

    if (!synchronous && sBuildingIoThread != NULL)
    {
        SDL_LockMutex(sBuildingIoMutex);
        if (!sBuildingPageQueued[page] && sBuildingReqCount < VOXEL_BUILDING_REQ_CAP)
        {
            sBuildingPageQueued[page] = true;
            sBuildingReqQueue[sBuildingReqTail] = (int16_t)page;
            sBuildingReqTail = (uint8_t)((sBuildingReqTail + 1u) % VOXEL_BUILDING_REQ_CAP);
            sBuildingReqCount++;
            SDL_UnlockMutex(sBuildingIoMutex);
            SDL_SemPost(sBuildingReqSem);
        }
        else
        {
            SDL_UnlockMutex(sBuildingIoMutex);
        }
        return 0;
    }

    UploadReadyBuildingPage();
    if (sBuildingPageToSlot[page] >= 0)
    {
        const int32_t slotIdx = sBuildingPageToSlot[page];
        sBuildingCache[slotIdx].lastUsedFrame = sFrameCounter;
        return sBuildingCache[slotIdx].tex;
    }

    uint32_t bw = 0, bh = 0, btexH = 0;
    bool readOk = false;
    bool hasGlow = false;
    if (sBuildingIoMutex != NULL)
        SDL_LockMutex(sBuildingIoMutex);
    if (VoxelBuildings_PageSize((uint32_t)page, &bw, &bh, &btexH) && (bw * btexH <= 512u * 512u))
    {
        memset(sBuildingPageMortonScratch, 0, (size_t)bw * btexH * sizeof(uint16_t));
        readOk = VoxelBuildings_ReadPage((uint32_t)page, 0, bw * bh, sBuildingPageMortonScratch);
        if (readOk && VoxelBuildings_PageHasGlow((uint32_t)page))
        {
            hasGlow = VoxelBuildings_ReadGlow((uint32_t)page, 0, bw * bh, sBuildingGlowMortonScratch);
        }
    }
    if (sBuildingIoMutex != NULL)
        SDL_UnlockMutex(sBuildingIoMutex);

    if (!readOk)
        return 0;

    Voxel_UnswizzleMortonToLinearEx(sBuildingPageMortonScratch, sBuildingPageLinearScratch, bw, bh, btexH);
    const bool hasAlpha = ScanLinearPageAlpha(sBuildingPageLinearScratch, bw * bh);
    // Reuse sBuildingPageMortonScratch (512*512*2 B) for linear RGBA4444 glow upload to save 512 KiB BSS.
    uint16_t *glowLinear = NULL;
    if (hasGlow)
    {
        Voxel_UnswizzleMortonGlowToRgba4444(sBuildingGlowMortonScratch, sBuildingPageMortonScratch, bw, bh, btexH,
                                            sBuildingGlowGrid[page]);
        __atomic_store_n(&sBuildingGlowGridReady[page], 1u, __ATOMIC_RELEASE);
        glowLinear = sBuildingPageMortonScratch;
    }
    return UploadBuildingPageToLru(page, bw, btexH, hasAlpha, sBuildingPageLinearScratch, glowLinear);
}

// Spatial Hash Grid (32x32) for O(1) chunk lookups
#define CHUNK_GRID_W 32
#define CHUNK_GRID_H 32
#define CHUNK_GRID_MASK_X 31
#define CHUNK_GRID_MASK_Y 31
static VoxelChunkSlot *sChunkGrid[CHUNK_GRID_W][CHUNK_GRID_H];

// Invalidate chunk cache (e.g. on warp / scene cut)
static void InvalidateChunkCache(void)
{
    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        sChunks[i].valid = false;
        sChunks[i].lastUsedFrame = 0;
    }
    memset(sChunkGrid, 0, sizeof(sChunkGrid));
    sChunkOriginX = 0;
    sChunkOriginY = 0;
    sNeedsFullWarmup = true;
    sWarmupFrames = 10;
    // A chunk the worker computed for the previous map is dropped at commit.
    sWorldEpoch++;
}

static void UploadChunkVbo(VoxelChunkSlot *slot, const bool gpuMayRead);

// Walking into a connected map re-bases the world coordinates by whole tiles. Cached chunks
// keep their vertices as built and only move their drawing offset (drawOffsetX/Z, applied as a
// translation when drawn): nothing is re-uploaded. Moving every vertex and re-uploading the 64
// VBOs took 34-45 ms here (logs/SESION 17-18, shift_ms).
static void ShiftChunkCoordinates(int32_t shiftTilesX, int32_t shiftTilesY)
{
    int32_t totalX = sChunkOriginX + shiftTilesX;
    int32_t totalY = sChunkOriginY + shiftTilesY;
    int32_t shiftCx = FloorDivChunk(totalX);
    int32_t shiftCy = FloorDivChunk(totalY);
    sChunkOriginX = totalX - shiftCx * CHUNK_SIZE;
    sChunkOriginY = totalY - shiftCy * CHUNK_SIZE;

    float sx = (float)shiftTilesX;
    float sz = (float)shiftTilesY;

    memset(sChunkGrid, 0, sizeof(sChunkGrid));
    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        if (!sChunks[i].valid) continue;

        sChunks[i].chunkX += shiftCx;
        sChunks[i].chunkY += shiftCy;
        sChunks[i].drawOffsetX += sx;
        sChunks[i].drawOffsetZ += sz;

        uint32_t gx = (uint32_t)sChunks[i].chunkX & CHUNK_GRID_MASK_X;
        uint32_t gy = (uint32_t)sChunks[i].chunkY & CHUNK_GRID_MASK_Y;
        sChunkGrid[gx][gy] = &sChunks[i];
    }
}

// Draws of chunk VBOs go through these: the modelview pushed once per pass, and a chunk
// moved by crossings drawn translated by its offset. Chunks built since the last crossing
// (offset 0) need no matrix change.
static float sAppliedOffsetX, sAppliedOffsetZ;

static void ChunkOffsets_Begin(void)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    sAppliedOffsetX = 0.0f;
    sAppliedOffsetZ = 0.0f;
}

static inline void ChunkOffsets_Apply(const VoxelChunkSlot *chunk)
{
    if (chunk->drawOffsetX == sAppliedOffsetX && chunk->drawOffsetZ == sAppliedOffsetZ)
        return;
    glPopMatrix();
    glPushMatrix();
    if (chunk->drawOffsetX != 0.0f || chunk->drawOffsetZ != 0.0f)
        glTranslatef(chunk->drawOffsetX, 0.0f, chunk->drawOffsetZ);
    sAppliedOffsetX = chunk->drawOffsetX;
    sAppliedOffsetZ = chunk->drawOffsetZ;
}

static void ChunkOffsets_End(void)
{
    glPopMatrix();
}

// Allocate the build staging buffer and the chunk slots' VBO names once
static bool EnsureChunkPool(void)
{
    if (sChunksAllocated) return true;

    sChunkStaging = (VoxelGlVertex *)malloc((size_t)CHUNK_WORKER_BATCH * (size_t)CHUNK_VERTS_PER_SLOT * sizeof(VoxelGlVertex));
    if (!sChunkStaging)
        return false;

    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        sChunks[i].valid = false;
        sChunks[i].buildingPage = -1;
        sChunks[i].groundTotalCount = 0;
        sChunks[i].groundSpanCount = 0;
        sChunks[i].treeCount = 0;
        sChunks[i].buildingCount = 0;
        sChunks[i].treeFirstVert = 0;
        sChunks[i].buildingFirstVert = 0;
        sChunks[i].lampFirstVert = 0;
        sChunks[i].lampGlassCount = 0;
        sChunks[i].lampPoolCount = 0;
        sChunks[i].lampLightCount = 0;
        sChunks[i].totalVertCount = 0;
        sChunks[i].vboCap = 0;
        sChunks[i].drawOffsetX = 0.0f;
        sChunks[i].drawOffsetZ = 0.0f;
        if (!sChunks[i].vbo)
            glGenBuffers(1, &sChunks[i].vbo);
        sChunks[i].groundVerts = NULL;
        sChunks[i].treeVerts = NULL;
        sChunks[i].buildingVerts = NULL;
        sChunks[i].lampVerts = NULL;
    }
    sChunksAllocated = true;
    return true;
}

// Track map origins to smoothly handle walking crossings across zone boundaries
static void RememberInstanceOrigins(void)
{
    uint32_t count = VoxelWorld_InstanceCount();
    sPreviousOriginCount = 0;
    for (uint32_t i = 0; i < count && i < MAX_VOXEL_MAP_INSTANCES; ++i)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(i);
        if (!inst) continue;
        sPreviousOrigins[sPreviousOriginCount].mapGroup = inst->mapGroup;
        sPreviousOrigins[sPreviousOriginCount].mapNum = inst->mapNum;
        sPreviousOrigins[sPreviousOriginCount].originX = inst->originX;
        sPreviousOrigins[sPreviousOriginCount].originY = inst->originY;
        ++sPreviousOriginCount;
    }
}

// True for a walking/biking crossing (shift camera, no snap); false for a cut/warp (snap camera).
// Combines deterministic `warpOccurred` from Core 1's LoadMapFromWarp with a generous spatial
// sanity guard (VOXEL_CROSSING_STEP) so no warp or script cut can ever shift instead of snap.
static bool HandleMapChange(int32_t mapGroup, int32_t mapNum, float playerX, float playerZ, bool warpOccurred)
{
    if (!warpOccurred)
    {
        for (uint32_t i = 0; i < sPreviousOriginCount; ++i)
        {
            if (sPreviousOrigins[i].mapGroup != mapGroup || sPreviousOrigins[i].mapNum != mapNum)
                continue;
            if (sPreviousOrigins[i].originX == 0 && sPreviousOrigins[i].originY == 0)
                break;

            if (fabsf(playerX - (sLastPlayerX - (float)sPreviousOrigins[i].originX)) > VOXEL_CROSSING_STEP
             || fabsf(playerZ - (sLastPlayerZ - (float)sPreviousOrigins[i].originY)) > VOXEL_CROSSING_STEP)
                break;

            VoxelCamera_Shift(&sCamera, (float)-sPreviousOrigins[i].originX,
                                      (float)-sPreviousOrigins[i].originY);
            sLastPlayerX -= (float)sPreviousOrigins[i].originX;
            sLastPlayerZ -= (float)sPreviousOrigins[i].originY;
            return true;
        }
    }

    VoxelCamera_SetGround(&sCamera, VoxelEntities_PlayerGround(), 1, 0.0f);
    VoxelCamera_Snap(&sCamera, playerX, playerZ);
    sLastPlayerX = playerX;
    sLastPlayerZ = playerZ;
    return false;
}

static inline VoxelChunkSlot *FindChunkSlot(int32_t chunkX, int32_t chunkY)
{
    uint32_t gx = (uint32_t)chunkX & CHUNK_GRID_MASK_X;
    uint32_t gy = (uint32_t)chunkY & CHUNK_GRID_MASK_Y;
    VoxelChunkSlot *slot = sChunkGrid[gx][gy];
    if (slot && slot->valid && slot->chunkX == chunkX && slot->chunkY == chunkY)
        return slot;

    // Linear fallback: resolve spatial grid collisions and repair grid entry
    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        if (sChunks[i].valid && sChunks[i].chunkX == chunkX && sChunks[i].chunkY == chunkY)
        {
            sChunkGrid[gx][gy] = &sChunks[i];
            return &sChunks[i];
        }
    }
    return NULL;
}

static VoxelChunkSlot *GetAvailableChunkSlot(int32_t chunkX, int32_t chunkY)
{
    uint32_t gx = (uint32_t)chunkX & CHUNK_GRID_MASK_X;
    uint32_t gy = (uint32_t)chunkY & CHUNK_GRID_MASK_Y;
    int32_t oldestIdx = -1;
    uint32_t oldestFrame = UINT32_MAX;

    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        if (!sChunks[i].valid)
        {
            uint32_t prevGx = (uint32_t)sChunks[i].chunkX & CHUNK_GRID_MASK_X;
            uint32_t prevGy = (uint32_t)sChunks[i].chunkY & CHUNK_GRID_MASK_Y;
            if (sChunkGrid[prevGx][prevGy] == &sChunks[i])
            {
                sChunkGrid[prevGx][prevGy] = NULL;
            }
            sChunkGrid[gx][gy] = &sChunks[i];
            return &sChunks[i];
        }
    }

    // Prefer evicting dormant chunks not drawn in the current nor preceding frame
    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        if (sChunks[i].lastUsedFrame + 1 < sFrameCounter && sChunks[i].lastUsedFrame < oldestFrame)
        {
            oldestFrame = sChunks[i].lastUsedFrame;
            oldestIdx = i;
        }
    }

    // Fallback: If all chunks were used recently, evict the oldest not drawn in the current frame
    if (oldestIdx < 0)
    {
        for (int32_t i = 0; i < MAX_CHUNKS; i++)
        {
            if (sChunks[i].lastUsedFrame != sFrameCounter && sChunks[i].lastUsedFrame < oldestFrame)
            {
                oldestFrame = sChunks[i].lastUsedFrame;
                oldestIdx = i;
            }
        }
    }

    // If all chunks were used this frame (rare fallback), pick the overall oldest
    if (oldestIdx < 0)
    {
        oldestIdx = 0;
        oldestFrame = sChunks[0].lastUsedFrame;
        for (int32_t i = 1; i < MAX_CHUNKS; i++)
        {
            if (sChunks[i].lastUsedFrame < oldestFrame)
            {
                oldestFrame = sChunks[i].lastUsedFrame;
                oldestIdx = i;
            }
        }
    }

    VoxelChunkSlot *oldest = &sChunks[oldestIdx];
    uint32_t ogx = (uint32_t)oldest->chunkX & CHUNK_GRID_MASK_X;
    uint32_t ogy = (uint32_t)oldest->chunkY & CHUNK_GRID_MASK_Y;
    if (sChunkGrid[ogx][ogy] == oldest)
    {
        sChunkGrid[ogx][ogy] = NULL;
    }
    oldest->valid = false;
    sChunkGrid[gx][gy] = oldest;
    return oldest;
}

// Process all pending atlas job pages: unswizzle and upload to hardware textures
static void ProcessAtlasJobPages(VoxelAtlasSlot *slot, VoxelAtlasJob *job)
{
    while (job->active)
    {
        while (!VoxelAtlas_JobStep(job, VOXEL_METATILE_IDS))
            ;
        uint32_t page = job->page;
        if (page < VOXEL_MAX_ATLAS_PAGES)
        {
            if (!slot->tex[page])
            {
                glGenTextures(1, &slot->tex[page]);
                glBindTexture(GL_TEXTURE_2D, slot->tex[page]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, slot->tex[page]);
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_ATLAS_W, VOXEL_ATLAS_H,
                         0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, slot->linear[page]);
            if (page + 1 > slot->pageCount)
                slot->pageCount = page + 1;
        }
        if (job->page + 1 < VOXEL_MAX_ATLAS_PAGES)
            job->dest = slot->linear[job->page + 1];
        if (!VoxelAtlas_JobNextPage(job))
            break;
    }
    VoxelAtlas_JobCancel(job);
}

// Bind active atlas slot parameters to the renderer state
static inline void ActivateAtlasSlot(int32_t slotIdx)
{
    sActiveAtlasSlot = slotIdx;
    sAtlasSlots[slotIdx].stamp = sFrameCounter;
    sAtlasMap = sAtlasSlots[slotIdx].map;
    sAtlasPageCount = sAtlasSlots[slotIdx].pageCount;
    for (int32_t p = 0; p < VOXEL_MAX_ATLAS_PAGES; p++)
        sAtlasTex[p] = sAtlasSlots[slotIdx].tex[p];
}

// ---- Textures rewritten while the GPU draws them ----
//
// vitaGL's glTexSubImage2D, on a texture drawn in the last FRAME_PURGE_FREQ (4) swaps,
// allocates a whole new texture and copies the old one into it, reading it back from
// CDRAM (source/textures.c): ~3.3 ms for a 512x256 page whatever the size of the update,
// plus a 256 KB allocation freed later by the garbage collector. The textures rewritten
// at run time (the animated atlas pages and the sprite atlas) therefore keep spare copies:
// updates go into a copy that has not been drawn for VOXEL_TEX_IDLE_FRAMES frames (vitaGL
// then writes the rows straight in), and that copy becomes the one drawn.

static GLuint CreateRgba5551Texture(uint32_t width, uint32_t height, const void *pixels)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height,
                 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
    return tex;
}

#define VOXEL_ANIM_PAGES 2u

// Uploads `rows` of a page from the slot's linear copy into `tex` in one call (host/non-Vita fallback).
static void UploadAtlasRows(GLuint tex, const uint16_t *linear, uint32_t rows)
{
    if (rows == 0)
        return;
    const uint32_t minRow = (uint32_t)__builtin_ctz(rows);
    const uint32_t maxRow = 31u - (uint32_t)__builtin_clz(rows);
    const uint32_t startY = minRow * VOXEL_ATLAS_SLOT;
    const uint32_t numY = (maxRow - minRow + 1u) * VOXEL_ATLAS_SLOT;
    // The linear copy is stored bottom-up, as glTexImage2D expects it.
    const uint32_t glYOffset = VOXEL_ATLAS_H - (startY + numY);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)glYOffset, VOXEL_ATLAS_W, (GLsizei)numY,
                    GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, &linear[glYOffset * VOXEL_ATLAS_W]);
    VitaPerf_AddCount(VITA_PERF_C_ATLAS_UPLOADS, 1);
}

// Writes `rows` of a page straight into its drawn texture's memory on Vita (vglGetTexDataPointer)
// so active and neighbouring map slots update in the exact same frame without vitaGL GC copies.
static void WriteAtlasRowsDirect(GLuint tex, const uint16_t *linear, uint32_t rows)
{
    if (rows == 0 || !tex)
        return;
#ifdef PLATFORM_VITA
    glBindTexture(GL_TEXTURE_2D, tex);
    uint16_t *const texels = (uint16_t *)vglGetTexDataPointer(GL_TEXTURE_2D);
    if (texels != NULL)
    {
        const uint32_t minRow = (uint32_t)__builtin_ctz(rows);
        const uint32_t maxRow = 31u - (uint32_t)__builtin_clz(rows);
        const uint32_t startY = minRow * VOXEL_ATLAS_SLOT;
        const uint32_t numY = (maxRow - minRow + 1u) * VOXEL_ATLAS_SLOT;
        const uint32_t glYOffset = VOXEL_ATLAS_H - (startY + numY);   // bottom-up, as the copy
        memcpy(&texels[glYOffset * VOXEL_ATLAS_W], &linear[glYOffset * VOXEL_ATLAS_W],
               (size_t)numY * VOXEL_ATLAS_W * sizeof(uint16_t));
        VitaPerf_AddCount(VITA_PERF_C_ATLAS_UPLOADS, 1);
        return;
    }
#endif
    UploadAtlasRows(tex, linear, rows);
}

// Sprite atlas: VoxelEntities_Emit gives the billboards the texture coordinates of the frames
// it just decoded, so the texture must change in the same frame (a spare swapped in later
// showed walking frames late or another character's). It is written straight into the
// texture's memory, as vitaGL's own direct path does, without the copy glTexSubImage2D makes
// of a texture drawn recently. The GPU may still be reading it for the previous frame: at
// worst a billboard shows its new frame one frame early.
static bool sSpriteAtlasNeedsAll = true;
//
// Only the 64x64 slots re-decoded this frame are unswizzled, given their relief and copied (the
// whole 512x256 atlas took ~3 ms per walking frame in SESION 22; a whole row of 8 slots per
// changed slot still cost up to 6-10 ms in logs/sesion 51). The relief stops at each slot's edge,
// as it already did at the top and bottom of each row: slots are different sprites.
static void WriteSpriteAtlasSlots(const uint16_t *morton, uint16_t *linear, uint32_t dirtySlots)
{
    if (dirtySlots == 0)
        return;

    glBindTexture(GL_TEXTURE_2D, sSpriteTex);
#ifdef PLATFORM_VITA
    uint16_t *const texels = (uint16_t *)vglGetTexDataPointer(GL_TEXTURE_2D);
#else
    uint16_t *const texels = NULL;
#endif
    for (uint32_t s = 0; s < VOXEL_SPRITE_SLOTS; s++)
    {
        if (!(dirtySlots & (1u << s)))
            continue;
        const uint32_t x0 = (s % VOXEL_SPRITE_COLUMNS) * VOXEL_SPRITE_SLOT_DIM;
        const uint32_t y0 = (s / VOXEL_SPRITE_COLUMNS) * VOXEL_SPRITE_SLOT_DIM;
        Voxel_UnswizzleMortonToLinearRect(morton, linear, VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_ATLAS_H,
                                          x0, y0, VOXEL_SPRITE_SLOT_DIM, VOXEL_SPRITE_SLOT_DIM);
        // The linear copy is bottom-up: atlas rows y0.. land on texture rows glY0..
        const uint32_t glY0 = VOXEL_SPRITE_ATLAS_H - (y0 + VOXEL_SPRITE_SLOT_DIM);
        uint16_t *const src = &linear[glY0 * VOXEL_SPRITE_ATLAS_W + x0];
        VoxelGrade_ApplyRectRelief(src, VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_SLOT_DIM, VOXEL_SPRITE_SLOT_DIM);
        if (texels != NULL)
        {
            for (uint32_t row = 0; row < VOXEL_SPRITE_SLOT_DIM; row++)
                memcpy(&texels[(glY0 + row) * VOXEL_SPRITE_ATLAS_W + x0], &src[row * VOXEL_SPRITE_ATLAS_W],
                       (size_t)VOXEL_SPRITE_SLOT_DIM * sizeof(uint16_t));
        }
        else
        {
            // Without direct access: the slot's whole row of slots, which the linear copy holds
            // up to date (no reliance on GL_UNPACK_ROW_LENGTH).
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)glY0, VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_SLOT_DIM,
                            GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, &linear[glY0 * VOXEL_SPRITE_ATLAS_W]);
        }
    }
}

static void UpdateAtlasCutoutMask(VoxelAtlasSlot *const slot)
{
    memset(slot->cutoutMask, 0, sizeof(slot->cutoutMask));
    for (uint32_t id = VOXEL_CUT_FIRST; id < VOXEL_METATILE_IDS; ++id)
    {
        const uint16_t raw = slot->map.slotOf[id];
        if (raw >= 1u && raw < VOXEL_SLOT_PENDING)
        {
            const uint32_t packed = (uint32_t)(raw - 1u);
            const uint32_t pg = packed / VOXEL_ATLAS_MAX_SLOTS;
            const uint32_t sl = packed % VOXEL_ATLAS_MAX_SLOTS;
            if (pg < VOXEL_MAX_ATLAS_PAGES)
            {
                slot->cutoutMask[pg][sl >> 6] |= (1ULL << (sl & 63u));
            }
        }
    }
}

static inline bool IsAtlasTriangleCutout(const VoxelAtlasSlot *const slot, const uint32_t page,
                                         const VoxelVertex *const tri)
{
    float uMid = ((tri[0].u + tri[1].u + tri[2].u) * (1.0f / 3.0f)) - (float)page * 2.0f;
    float vMid = (tri[0].v + tri[1].v + tri[2].v) * (1.0f / 3.0f);
    if (uMid < 0.0f) uMid = 0.0f;
    if (uMid > 0.9999f) uMid = 0.9999f;
    if (vMid < 0.0f) vMid = 0.0f;
    if (vMid > 0.9999f) vMid = 0.9999f;
    const uint32_t col = (uint32_t)(uMid * (float)VOXEL_ATLAS_COLUMNS);
    const uint32_t row = (uint32_t)((1.0f - vMid) * (float)VOXEL_ATLAS_ROWS);
    const uint32_t sl = row * VOXEL_ATLAS_COLUMNS + col;
    if (sl >= VOXEL_ATLAS_MAX_SLOTS)
        return false;
    return (slot->cutoutMask[page][sl >> 6] & (1ULL << (sl & 63u))) != 0ULL;
}

// Compose all pages of a metatile atlas into a given atlas slot
static bool ComposeAtlasSlot(VoxelAtlasSlot *slot, const VoxelMapInstance *inst)
{
    if (!slot || !inst || !inst->secondaryTileset)
        return false;

    static uint32_t sAtlasGenCounter = 1;
    slot->generation = ++sAtlasGenCounter;
    slot->extension = 0;

    slot->primaryTileset = inst->primaryTileset;
    slot->secondaryTileset = inst->secondaryTileset;
    slot->stamp = sFrameCounter;
    slot->pageCount = 0;
    memset(sSlotPendingPrimary[slot - sAtlasSlots], 0, sizeof(sSlotPendingPrimary[0]));

    VoxelAtlasJob job;
    if (VoxelAtlas_JobBegin(&job, inst, slot->linear[0], NULL))
    {
        ProcessAtlasJobPages(slot, &job);
        slot->map = job.map;
        UpdateAtlasCutoutMask(slot);
        slot->valid = true;
        return true;
    }
    return false;
}

static bool AtlasExtend_Busy(int32_t slotIdx);

// Extend an existing atlas slot in place when moving into an adjacent or connected zone
static bool ExtendAtlasSlot(VoxelAtlasSlot *slot, const VoxelMapInstance *inst)
{
    if (!slot || !inst || !slot->valid || !inst->secondaryTileset)
        return false;
    // Being extended over several frames (AtlasExtend_*): that job composes into the same
    // linear pages. Its end bumps `extension`, so chunks left uncovered are rebuilt then.
    if (AtlasExtend_Busy((int32_t)(slot - sAtlasSlots)))
        return false;

    VoxelAtlasJob job;
    if (!VoxelAtlas_JobBegin(&job, inst, slot->linear[0], &slot->map))
    {
        // No new metatiles required by this zone
        return false;
    }

    slot->stamp = sFrameCounter;
    ProcessAtlasJobPages(slot, &job);

    if (job.map.overflowed)
    {
        // Atlas capacity exceeded on extension: recompose cleanly
        return ComposeAtlasSlot(slot, inst);
    }

    slot->extension++;
    slot->map = job.map;
    UpdateAtlasCutoutMask(slot);
    return true;
}

// ---- Atlas extension spread over frames ----
//
// The current map's slot is extended when the map changes (a crossing brings maps on screen
// with metatiles not composed yet). Done at once, that decoded both tilesets and recomposed and
// uploaded every page in the crossing frame: 9-12 ms (atlas_slots_ms, logs/sesion 24), on top
// of the crossing's other work, so the player visibly skipped a step. The atlas job was
// written to be spread over frames (VoxelAtlas_JobStep); here it gets a time budget a frame.
// Meanwhile the slot keeps serving its current map: an extension keeps every metatile where it
// was, so a page uploaded early only adds slots. The slot's linear pages are half composed
// until the end, so its tile animation waits (AtlasExtend_Busy). One job at a time.

static VoxelAtlasJob sExtendJob;
static int32_t sExtendSlot = -1;
static uint32_t sExtendGeneration;
static int32_t sExtendMapGroup, sExtendMapNum;

static bool AtlasExtend_Busy(int32_t slotIdx)
{
    return sExtendSlot >= 0 && sExtendSlot == slotIdx;
}

static void AtlasExtend_Cancel(void)
{
    if (sExtendSlot < 0)
        return;
    VoxelAtlas_JobCancel(&sExtendJob);
    sExtendSlot = -1;
}

// Starts extending `slotIdx` for `inst` unless a job is already running (a later map change
// asks again once it ends). Returns false when there is nothing to add.
static bool AtlasExtend_Start(int32_t slotIdx, const VoxelMapInstance *inst)
{
    if (sExtendSlot >= 0 || slotIdx < 0 || inst == NULL || !inst->secondaryTileset)
        return sExtendSlot >= 0;
    VoxelAtlasSlot *const slot = &sAtlasSlots[slotIdx];
    if (!slot->valid)
        return false;
    if (!VoxelAtlas_JobBegin(&sExtendJob, inst, slot->linear[0], &slot->map))
        return false;
    sExtendSlot = slotIdx;
    sExtendGeneration = slot->generation;
    sExtendMapGroup = inst->mapGroup;
    sExtendMapNum = inst->mapNum;
    slot->stamp = sFrameCounter;
    return true;
}

static void AtlasExtend_Finish(void)
{
    VoxelAtlasSlot *const slot = &sAtlasSlots[sExtendSlot];
    const int32_t slotIdx = sExtendSlot;
    const bool overflowed = sExtendJob.map.overflowed;
    const VoxelAtlasMap *const map = &sExtendJob.map;

    if (!overflowed)
    {
        slot->extension++;
        slot->map = *map;
        UpdateAtlasCutoutMask(slot);
    }
    VoxelAtlas_JobCancel(&sExtendJob);
    sExtendSlot = -1;
    if (overflowed)
    {
        // Capacity exceeded: recompose cleanly, at once (rare), for the map it was started for.
        for (uint32_t i = 0; i < VoxelWorld_InstanceCount(); i++)
        {
            const VoxelMapInstance *inst = VoxelWorld_Instance(i);
            if (inst != NULL && inst->mapGroup == sExtendMapGroup && inst->mapNum == sExtendMapNum)
            {
                ComposeAtlasSlot(slot, inst);
                break;
            }
        }
    }
    if (slotIdx == sActiveAtlasSlot)
        ActivateAtlasSlot(slotIdx);
}

// Runs the extension for up to VOXEL_ATLAS_EXTEND_BUDGET_MS, at most one page upload a frame.
static void AtlasExtend_Step(void)
{
    if (sExtendSlot < 0)
        return;
    VoxelAtlasSlot *const slot = &sAtlasSlots[sExtendSlot];
    // Recomposed for another tileset meanwhile: the job no longer applies.
    if (!slot->valid || slot->generation != sExtendGeneration)
    {
        AtlasExtend_Cancel();
        return;
    }
    const double start = VitaPerf_NowMs();
    while (VitaPerf_NowMs() - start < VOXEL_ATLAS_EXTEND_BUDGET_MS)
    {
        if (!VoxelAtlas_JobStep(&sExtendJob, VOXEL_ATLAS_EXTEND_STEP_METATILES))
            continue;
        // The current page is composed: upload it, as ProcessAtlasJobPages does.
        const uint32_t page = sExtendJob.page;
        if (page < VOXEL_MAX_ATLAS_PAGES)
        {
            if (!slot->tex[page])
                slot->tex[page] = CreateRgba5551Texture(VOXEL_ATLAS_W, VOXEL_ATLAS_H, slot->linear[page]);
            else
            {
                glBindTexture(GL_TEXTURE_2D, slot->tex[page]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_ATLAS_W, VOXEL_ATLAS_H,
                             0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, slot->linear[page]);
            }
            if (page + 1 > slot->pageCount)
                slot->pageCount = page + 1;
        }
        if (sExtendJob.page + 1 < VOXEL_MAX_ATLAS_PAGES)
            sExtendJob.dest = slot->linear[sExtendJob.page + 1];
        if (!VoxelAtlas_JobNextPage(&sExtendJob))
            AtlasExtend_Finish();
        break;
    }
}

// Find or compose the atlas slot for any map instance (active map or connected neighbor)
static int32_t EnsureAtlasSlotForInstance(const VoxelMapInstance *inst)
{
    if (!inst || !inst->secondaryTileset)
        return (sActiveAtlasSlot >= 0) ? sActiveAtlasSlot : 0;

    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        if (sAtlasSlots[s].valid &&
            sAtlasSlots[s].primaryTileset == inst->primaryTileset &&
            sAtlasSlots[s].secondaryTileset == inst->secondaryTileset)
        {
            sAtlasSlots[s].stamp = sFrameCounter;
            return s;
        }
    }

    int32_t victimSlot = -1;
    uint32_t oldestStamp = UINT32_MAX;
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        if (!sAtlasSlots[s].valid)
        {
            victimSlot = s;
            break;
        }
        if (s != sActiveAtlasSlot && s != sExtendSlot
            && (uint32_t)(sFrameCounter - sAtlasSlots[s].stamp) > 1u
            && sAtlasSlots[s].stamp < oldestStamp)
        {
            oldestStamp = sAtlasSlots[s].stamp;
            victimSlot = s;
        }
    }
    if (victimSlot < 0)
    {
        oldestStamp = UINT32_MAX;
        for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
        {
            if (s != sActiveAtlasSlot && s != sExtendSlot && sAtlasSlots[s].stamp < oldestStamp)
            {
                oldestStamp = sAtlasSlots[s].stamp;
                victimSlot = s;
            }
        }
    }
    if (victimSlot < 0)
        victimSlot = (sActiveAtlasSlot == 0) ? 1 : 0;
    if (victimSlot == sExtendSlot)
        AtlasExtend_Cancel();

    if (sAtlasSlots[victimSlot].valid)
        VitaPerf_MarkEvent(VITA_PERF_E_ATLAS_EVICT);
    ComposeAtlasSlot(&sAtlasSlots[victimSlot], inst);
    return victimSlot;
}

// Upload static chunk geometry to GPU VBO in CDRAM (Zero-Copy rendering on PS Vita).
// Reuses existing VBO storage via glBufferSubData when the slot was not drawn in the last 4 frames
// (vitaGL FRAME_PURGE_FREQ=4); orphans via glBufferData when the GPU may still read the old VBO.
static void UploadChunkVbo(VoxelChunkSlot *slot, const bool gpuMayRead)
{
    if (!slot || slot->totalVertCount == 0)
        return;

    if (!slot->vbo)
    {
        glGenBuffers(1, &slot->vbo);
        slot->vboCap = 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, slot->vbo);
    if (slot->totalVertCount > slot->vboCap || gpuMayRead)
    {
        uint32_t allocCap = ((uint32_t)slot->totalVertCount + 255u) & ~255u;
        if (allocCap > 65535u)
            allocCap = 65535u;
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(allocCap * sizeof(VoxelGlVertex)), NULL, GL_STATIC_DRAW);
        slot->vboCap = (uint16_t)allocCap;
    }

    if (slot->groundTotalCount > 0)
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        (GLsizeiptr)(slot->groundTotalCount * sizeof(VoxelGlVertex)),
                        slot->groundVerts);
    }
    if (slot->treeCount > 0)
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        (GLintptr)(slot->treeFirstVert * sizeof(VoxelGlVertex)),
                        (GLsizeiptr)(slot->treeCount * sizeof(VoxelGlVertex)),
                        slot->treeVerts);
    }
    if (slot->buildingCount > 0)
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        (GLintptr)(slot->buildingFirstVert * sizeof(VoxelGlVertex)),
                        (GLsizeiptr)(slot->buildingCount * sizeof(VoxelGlVertex)),
                        slot->buildingVerts);
    }
    if (slot->lampGlassCount + slot->lampPoolCount > 0)
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        (GLintptr)(slot->lampFirstVert * sizeof(VoxelGlVertex)),
                        (GLsizeiptr)((slot->lampGlassCount + slot->lampPoolCount) * sizeof(VoxelGlVertex)),
                        slot->lampVerts);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---- Chunk builds in three phases ----
//
// PrepareChunkJob (Core 0): the chunk's window and the atlas slot of every map instance it
// touches; resolving a slot may compose it, which uploads textures.
// ComputeChunkJob: the geometry, into sChunkStaging, and what the build needs from Core 0
// afterwards (atlas slots missing metatiles, building pages). No GL, no change to the atlas
// slots, the chunk slots or the building cache: the part a worker thread can run (E4).
// CommitChunkJob (Core 0): extends atlas slots and computes again if metatiles were missing,
// requests the building pages, fills the chunk slot and uploads its VBO.
//
// What ComputeChunkJob still reads or writes that others share, to settle before it leaves
// Core 0: the map instances and the live
// layout (VoxelWorld), the lighting caches (voxel_lighting.c, also used by the entities),
// the building module's layout caches, the mesh builder's window (VoxelMesh_BeginWindow),
// sScratch and sChunkStaging, the atlas maps it reads (sAtlasSlots[].map, sAtlasMap) and
// the cutout masks.

#define CHUNK_JOB_MAX_GROUND_INSTANCES MAX_VOXEL_MAP_INSTANCES
#define CHUNK_JOB_MAX_PAGES 8

typedef struct
{
    int32_t x0, y0, x1, y1;
    bool indoor;
    bool hasBorder;
    // Instances drawn as ground, in instance order: overlapping the chunk, or instance 0
    // drawing the border ground of a chunk that reaches past every map.
    uint32_t groundCount;
    struct
    {
        uint8_t instance;
        uint8_t atlasSlot;
        bool overlaps;
        bool border;
    } ground[CHUNK_JOB_MAX_GROUND_INSTANCES];
    // The active slot's map, for trees and buildings (they never used the per-map atlas).
    const VoxelAtlasMap *activeMap;
} VoxelChunkJobInput;

typedef struct
{
    VoxelGlVertex *groundVerts;
    VoxelGlVertex *treeVerts;
    VoxelGlVertex *buildingVerts;
    VoxelGlVertex *lampVerts;
    uint32_t groundTotalCount;
    uint32_t treeCount;
    uint32_t treeOpaqueCount;
    uint32_t treeStaticCount;
    uint32_t buildingCount;
    uint8_t groundSpanCount;
    uint8_t buildingSpanCount;
    VoxelChunkGroundSpan groundSpans[MAX_CHUNK_GROUND_SPANS];
    VoxelChunkBuildingSpan buildingSpans[MAX_CHUNK_BUILDING_SPANS];
    int32_t buildingPage;
    uint8_t usedAtlasMask;
    // Atlas slots whose metatiles this chunk found missing (the ground it drew there is
    // incomplete until the slot is extended).
    uint8_t uncoveredAtlasMask;
    // Measured by the job itself (it may run on the worker, which must not touch the
    // profiler): phase times and the lighting work, added to the frame at commit.
    double groundMs, treesMs, buildingsMs, computeMs;
    VoxelLightingWork light;
    // Building pages its models are drawn from, to load before they show.
    uint32_t pageCount;
    int16_t pages[CHUNK_JOB_MAX_PAGES];
    // How far its building models reach past its cells (VoxelChunkSlot::bldWest...).
    float bldWest, bldEast, bldNorth, bldSouth, bldTop;
    // Exact vertical AABB of all emitted geometry in this chunk (Core 2).
    float minY, maxY;
    // Night lamps (VoxelChunkSlot::lampFirstVert...).
    uint32_t lampGlassCount, lampPoolCount, lampLightCount;
    float lampLights[VOXEL_LAMP_CHUNK_LIGHTS][3];
} VoxelChunkJobOutput;

static void PrepareChunkJob(VoxelChunkJobInput *in, int32_t chunkX, int32_t chunkY)
{
    in->x0 = sChunkOriginX + chunkX * CHUNK_SIZE;
    in->y0 = sChunkOriginY + chunkY * CHUNK_SIZE;
    in->x1 = in->x0 + CHUNK_SIZE;
    in->y1 = in->y0 + CHUNK_SIZE;
    in->groundCount = 0;
    in->activeMap = &sAtlasMap;

    const uint32_t instanceCount = VoxelWorld_InstanceCount();
    const VoxelMapInstance *inst0 = VoxelWorld_Instance(0);
    in->indoor = (inst0 != NULL && inst0->indoor);

    // Border ground if chunk reaches outside all instances
    bool completelyInsideInst0 = false;
    if (inst0 != NULL)
    {
        completelyInsideInst0 = (in->x0 >= inst0->originX && in->x1 <= inst0->originX + inst0->width &&
                                 in->y0 >= inst0->originY && in->y1 <= inst0->originY + inst0->height);
    }
    in->hasBorder = false;
    if (!in->indoor && inst0 && !completelyInsideInst0)
    {
        for (int32_t y = in->y0; y < in->y1 && !in->hasBorder; y++)
        {
            for (int32_t x = in->x0; x < in->x1 && !in->hasBorder; x++)
            {
                if (VoxelWorld_GetInstanceAt(x, y) == NULL)
                    in->hasBorder = true;
            }
        }
    }

    for (uint32_t i = 0; i < instanceCount && in->groundCount < CHUNK_JOB_MAX_GROUND_INSTANCES; i++)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(i);
        if (!inst) continue;
        const bool overlaps = (in->x0 < inst->originX + inst->width && in->x1 > inst->originX
                               && in->y0 < inst->originY + inst->height && in->y1 > inst->originY);
        const bool border = (i == 0 && in->hasBorder);
        if (!overlaps && !border)
            continue;
        in->ground[in->groundCount].instance = (uint8_t)i;
        in->ground[in->groundCount].atlasSlot = (uint8_t)EnsureAtlasSlotForInstance(inst);
        in->ground[in->groundCount].overlaps = overlaps;
        in->ground[in->groundCount].border = border;
        in->groundCount++;
    }
}

static uint8_t sScratchTriBucket[SCRATCH_VERTICES / 3];

static inline void ExpandChunkVertexBounds(VoxelChunkJobOutput *const out,
                                           const float x0, const float y0,
                                           const float x1, const float y1,
                                           const float vx, const float vy, const float vz)
{
    if (vy < out->minY) out->minY = vy;
    if (vy > out->maxY) out->maxY = vy;
    const float west = x0 - vx;
    const float east = vx - x1;
    const float north = y0 - vz;
    const float south = vz - y1;
    if (west > out->bldWest) out->bldWest = west;
    if (east > out->bldEast) out->bldEast = east;
    if (north > out->bldNorth) out->bldNorth = north;
    if (south > out->bldSouth) out->bldSouth = south;
}

static void PackGroundIntoJob(VoxelChunkJobOutput *const out, const VoxelBuilder *const builder,
                              const int32_t aSlot, const bool outdoor,
                              const float x0, const float y0, const float x1, const float y1)
{
    if (builder->count == 0)
        return;

    const VoxelAtlasSlot *const atlasSlot = &sAtlasSlots[aSlot];
    const uint32_t triCount = builder->count / 3u;
    uint32_t bucketCounts[VOXEL_MAX_ATLAS_PAGES * 2u] = {0};
    uint32_t bucketOffsets[VOXEL_MAX_ATLAS_PAGES * 2u] = {0};
    uint32_t bucketEnd[VOXEL_MAX_ATLAS_PAGES * 2u] = {0};

    for (uint32_t t = 0; t < triCount; t++)
    {
        const uint32_t i = t * 3u;
        uint32_t triPage = (uint32_t)(sScratch[i].u * 0.5f);
        if (triPage >= VOXEL_MAX_ATLAS_PAGES)
            triPage = 0;
        const uint32_t cutPass = IsAtlasTriangleCutout(atlasSlot, triPage, &sScratch[i]) ? 1u : 0u;
        const uint32_t bucket = (triPage << 1u) | cutPass;
        sScratchTriBucket[t] = (uint8_t)bucket;
        bucketCounts[bucket]++;
    }

    uint32_t cursor = out->groundTotalCount;
    for (uint32_t page = 0; page < VOXEL_MAX_ATLAS_PAGES; page++)
    {
        for (uint32_t cutPass = 0; cutPass < 2u; cutPass++)
        {
            const uint32_t bucket = (page << 1u) | cutPass;
            const uint32_t room = (cursor < CHUNK_GROUND_TOTAL_CAP)
                ? (CHUNK_GROUND_TOTAL_CAP - cursor) / 3u : 0u;
            const uint32_t fitTris = (bucketCounts[bucket] <= room) ? bucketCounts[bucket] : room;

            bucketOffsets[bucket] = cursor;
            bucketEnd[bucket] = cursor + fitTris * 3u;
            if (fitTris > 0 && out->groundSpanCount < MAX_CHUNK_GROUND_SPANS)
            {
                VoxelChunkGroundSpan *const span = &out->groundSpans[out->groundSpanCount++];
                span->atlasSlot = (uint8_t)aSlot;
                span->page = (uint8_t)page;
                span->isCutout = (uint8_t)cutPass;
                span->pad = 0;
                span->firstVert = (uint16_t)cursor;
                span->vertCount = (uint16_t)(fitTris * 3u);
            }
            cursor += fitTris * 3u;
        }
    }

    for (uint32_t t = 0; t < triCount; t++)
    {
        const uint32_t bucket = sScratchTriBucket[t];
        const uint32_t dstBase = bucketOffsets[bucket];
        if (dstBase + 3u > bucketEnd[bucket])
            continue;
        bucketOffsets[bucket] = dstBase + 3u;

        const uint32_t page = bucket >> 1u;
        const uint32_t srcBase = t * 3u;
        for (uint32_t j = 0; j < 3u; j++)
        {
            VoxelGlVertex *const dst = &out->groundVerts[dstBase + j];
            const float vx = sScratch[srcBase + j].x;
            const float vy = sScratch[srcBase + j].y;
            const float vz = sScratch[srcBase + j].z;
            dst->x = vx;
            dst->y = vy;
            dst->z = vz;
            ExpandChunkVertexBounds(out, x0, y0, x1, y1, vx, vy, vz);
            dst->u = sScratch[srcBase + j].u - (float)page * 2.0f;
            dst->v = sScratch[srcBase + j].v;
            BakeGradedVertexColor(sScratch[srcBase + j].shade, outdoor, &dst->r, &dst->g, &dst->b);
            dst->a = 255;
        }
    }
    out->groundTotalCount = cursor;
}

static void NoteJobPage(VoxelChunkJobOutput *out, int32_t page)
{
    for (uint32_t k = 0; k < out->pageCount; k++)
    {
        if (out->pages[k] == page)
            return;
    }
    if (out->pageCount < CHUNK_JOB_MAX_PAGES)
        out->pages[out->pageCount++] = (int16_t)page;
}

static VoxelGlVertex sTreeSwayTmp[CHUNK_TREE_CAP];

// Tag top vertices of tall grass, flowers, and ash grass with sway weight 1.0 (a = 65),
// then partition triangles into 3 contiguous ranges for SGX543 HSR + sway:
//   [0 .. *outOpaqueCount)            : opaque tree cores/ground (GL_ALPHA_TEST off)
//   [*outOpaqueCount .. staticCount)  : alpha-tested tree borders/foliage (GL_ALPHA_TEST on)
//   [staticCount .. count)            : swaying foliage (sTreeShaderProg)
static uint32_t TagAndPartitionTreeSway(VoxelGlVertex *const verts, const uint32_t count,
                                        uint8_t *const triOpaque, uint32_t *const outOpaqueCount)
{
    const float grassTopV = 1.0f - 32.0f / (float)VOXEL_TREE_TEXTURE_DIM;
    const float grassMinU = 79.5f / (float)VOXEL_TREE_TEXTURE_DIM;
    const float grassMaxU = 96.5f / (float)VOXEL_TREE_TEXTURE_DIM;
    const float flowerTopV = 1.0f - 112.0f / (float)VOXEL_TREE_TEXTURE_DIM;
    const float flowerMaxU = 32.5f / (float)VOXEL_TREE_TEXTURE_DIM;

    for (uint32_t t = 0; t + 2u < count; t += 3u)
    {
        float minY = verts[t].y, maxY = verts[t].y;
        for (uint32_t k = 1; k < 3u; ++k)
        {
            if (verts[t + k].y < minY) minY = verts[t + k].y;
            if (verts[t + k].y > maxY) maxY = verts[t + k].y;
        }
        // Skip flat ground triangles (which can share UV edges at any map elevation).
        const bool isStanding = (maxY - minY) >= 0.20f;
        for (uint32_t k = 0; k < 3u; ++k)
        {
            VoxelGlVertex *const gv = &verts[t + k];
            const bool isTopVert = isStanding && (gv->y - minY) > 0.15f;
            const bool isGrassTop = isTopVert && (fabsf(gv->v - grassTopV) < 0.002f && gv->u >= grassMinU && gv->u <= grassMaxU);
            const bool isFlowerOrAshTop = isTopVert && (fabsf(gv->v - flowerTopV) < 0.002f && gv->u <= flowerMaxU);
            gv->a = (isGrassTop || isFlowerOrAshTop) ? 65u : 255u;
        }
    }

    uint32_t staticCount = 0u;
    uint32_t swayCount = 0u;
    for (uint32_t t = 0; t + 2u < count; t += 3u)
    {
        const bool isSwayTri = (verts[t].a < 255u || verts[t + 1u].a < 255u || verts[t + 2u].a < 255u);
        if (isSwayTri)
        {
            sTreeSwayTmp[swayCount++] = verts[t];
            sTreeSwayTmp[swayCount++] = verts[t + 1u];
            sTreeSwayTmp[swayCount++] = verts[t + 2u];
        }
        else
        {
            const uint32_t dstTri = staticCount / 3u;
            const uint32_t srcTri = t / 3u;
            if (staticCount != t)
            {
                verts[staticCount]      = verts[t];
                verts[staticCount + 1u] = verts[t + 1u];
                verts[staticCount + 2u] = verts[t + 2u];
            }
            triOpaque[dstTri] = triOpaque[srcTri];
            staticCount += 3u;
        }
    }
    if (swayCount > 0u)
        memcpy(&verts[staticCount], sTreeSwayTmp, swayCount * sizeof(VoxelGlVertex));

    uint32_t opaqueCount = 0u;
    uint32_t cutoutCount = 0u;
    for (uint32_t t = 0; t + 2u < staticCount; t += 3u)
    {
        if (triOpaque[t / 3u] != 0u)
        {
            if (opaqueCount != t)
            {
                verts[opaqueCount]      = verts[t];
                verts[opaqueCount + 1u] = verts[t + 1u];
                verts[opaqueCount + 2u] = verts[t + 2u];
            }
            opaqueCount += 3u;
        }
        else
        {
            sTreeSwayTmp[cutoutCount++] = verts[t];
            sTreeSwayTmp[cutoutCount++] = verts[t + 1u];
            sTreeSwayTmp[cutoutCount++] = verts[t + 2u];
        }
    }
    if (cutoutCount > 0u)
        memcpy(&verts[opaqueCount], sTreeSwayTmp, cutoutCount * sizeof(VoxelGlVertex));

    *outOpaqueCount = opaqueCount;
    return staticCount;
}

// Partition a building span's triangles in-place on Core 2 so only upright wall triangles
// (|normal.y| < 0.35) whose UV bounding box overlaps a glow > 0 cell in the page's 32x32
// occupancy grid sit at the front of the span. Falls back to upright-only before page load.
static uint32_t PartitionBuildingGlowTriangles(VoxelVertex *const verts, const uint32_t vertCount,
                                               const int32_t page)
{
    const bool hasGrid = (page >= 0 && page < VOXEL_MAX_BUILDING_PAGES)
        && (__atomic_load_n(&sBuildingGlowGridReady[page], __ATOMIC_ACQUIRE) != 0u);
    const uint32_t *const grid = hasGrid ? sBuildingGlowGrid[page] : NULL;

    uint32_t write = 0;
    for (uint32_t i = 0; i + 2u < vertCount; i += 3u)
    {
        const VoxelVertex *const a = &verts[i];
        const VoxelVertex *const b = &verts[i + 1u];
        const VoxelVertex *const c = &verts[i + 2u];
        const float e1x = b->x - a->x, e1y = b->y - a->y, e1z = b->z - a->z;
        const float e2x = c->x - a->x, e2y = c->y - a->y, e2z = c->z - a->z;
        const float nx = e1y * e2z - e1z * e2y;
        const float ny = e1z * e2x - e1x * e2z;
        const float nz = e1x * e2y - e1y * e2x;
        const float lenSq = nx * nx + ny * ny + nz * nz;
        // Winding-independent upright wall test: |ny| < 0.35 * |n| -> ny^2 < 0.1225 * lenSq
        if (lenSq <= 1e-8f || (ny * ny) >= 0.1225f * lenSq)
            continue;

        if (grid != NULL)
        {
            const float uMin = fminf(a->u, fminf(b->u, c->u));
            const float uMax = fmaxf(a->u, fmaxf(b->u, c->u));
            const float vMin = fminf(a->v, fminf(b->v, c->v));
            const float vMax = fmaxf(a->v, fmaxf(b->v, c->v));
            int32_t gx0 = (int32_t)(uMin * 32.0f);
            int32_t gx1 = (int32_t)(uMax * 32.0f);
            int32_t gy0 = (int32_t)(vMin * 32.0f);
            int32_t gy1 = (int32_t)(vMax * 32.0f);
            if (gx0 < 0) gx0 = 0;
            if (gx0 > 31) gx0 = 31;
            if (gx1 < 0) gx1 = 0;
            if (gx1 > 31) gx1 = 31;
            if (gy0 < 0) gy0 = 0;
            if (gy0 > 31) gy0 = 31;
            if (gy1 < 0) gy1 = 0;
            if (gy1 > 31) gy1 = 31;
            const int32_t spanX = gx1 - gx0;
            const uint32_t xMask = (spanX >= 31) ? 0xFFFFFFFFu : (((1u << (spanX + 1)) - 1u) << gx0);
            bool hitsGlow = false;
            for (int32_t gy = gy0; gy <= gy1; ++gy)
            {
                if ((grid[gy] & xMask) != 0u)
                {
                    hitsGlow = true;
                    break;
                }
            }
            if (!hitsGlow)
                continue;
        }

        if (write != i)
        {
            const VoxelVertex t0 = verts[write];
            const VoxelVertex t1 = verts[write + 1u];
            const VoxelVertex t2 = verts[write + 2u];
            verts[write]      = verts[i];
            verts[write + 1u] = verts[i + 1u];
            verts[write + 2u] = verts[i + 2u];
            verts[i]          = t0;
            verts[i + 1u]     = t1;
            verts[i + 2u]     = t2;
        }
        write += 3u;
    }
    return write;
}

// The chunk's night lamps (VoxelLamps_EmitChunk): lantern glass and pools of light into the
// lamp part of the staging buffer, their light centres into the output. Uses sScratch, free once
// the buildings are packed. Static geometry, built day or night: at night it is drawn with the
// glow as a constant colour (Pass_NightLamps), so the clock never rebuilds a chunk.
static void EmitChunkLamps(const VoxelChunkJobInput *in, VoxelChunkJobOutput *out)
{
    VoxelBuilder glass, pool;
    VoxelBuilder_Init(&glass, sScratch, CHUNK_LAMP_GLASS_CAP);
    VoxelBuilder_Init(&pool, sScratch + CHUNK_LAMP_GLASS_CAP, CHUNK_LAMP_POOL_CAP);

    const uint32_t instanceCount = VoxelWorld_InstanceCount();
    for (uint32_t i = 0; i < instanceCount && out->lampLightCount < VOXEL_LAMP_CHUNK_LIGHTS; i++)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(i);
        if (!inst || inst->indoor || in->x0 >= inst->originX + inst->width || in->x1 <= inst->originX
            || in->y0 >= inst->originY + inst->height || in->y1 <= inst->originY)
            continue;
        VoxelBuilder_SetStretch(&glass, inst);
        VoxelBuilder_SetStretch(&pool, inst);
        out->lampLightCount += VoxelLamps_EmitChunk(&glass, &pool, inst, in->x0, in->y0, in->x1, in->y1,
                                                    out->lampLights + out->lampLightCount,
                                                    VOXEL_LAMP_CHUNK_LIGHTS - out->lampLightCount);
    }
    if (glass.count + pool.count == 0)
        return;

    // Lamps and pools reach past the chunk's cells: the culling bounds take them in.
    for (uint32_t k = 0; k < glass.count; k++)
        ExpandChunkVertexBounds(out, (float)in->x0, (float)in->y0, (float)in->x1, (float)in->y1,
                                sScratch[k].x, sScratch[k].y, sScratch[k].z);
    for (uint32_t k = 0; k < pool.count; k++)
    {
        const VoxelVertex *const v = &sScratch[CHUNK_LAMP_GLASS_CAP + k];
        ExpandChunkVertexBounds(out, (float)in->x0, (float)in->y0, (float)in->x1, (float)in->y1, v->x, v->y, v->z);
    }
    PackVoxelVertices(sScratch, out->lampVerts, glass.count, true);
    PackVoxelVertices(sScratch + CHUNK_LAMP_GLASS_CAP, out->lampVerts + glass.count, pool.count, true);
    // The pools lie on the ground they light: lifted off it (and drawn with a depth offset).
    for (uint32_t k = 0; k < pool.count; k++)
        out->lampVerts[glass.count + k].y += VOXEL_LAMP_POOL_LIFT;
    out->lampGlassCount = glass.count;
    out->lampPoolCount = pool.count;
}

static void ComputeChunkJob(const VoxelChunkJobInput *in, VoxelChunkJobOutput *out, VoxelGlVertex *staging)
{
    const double computeStart = VitaPerf_NowMs();
    VoxelLightingWork lightBefore;
    VoxelLighting_GetWork(&lightBefore);
    const int32_t x0 = in->x0, y0 = in->y0, x1 = in->x1, y1 = in->y1;
    const VoxelMapInstance *inst0 = VoxelWorld_Instance(0);
    const uint32_t instanceCount = VoxelWorld_InstanceCount();

    out->groundVerts = staging;
    out->treeVerts = staging + CHUNK_GROUND_TOTAL_CAP;
    out->buildingVerts = out->treeVerts + CHUNK_TREE_CAP;
    out->lampVerts = out->buildingVerts + CHUNK_BUILDING_CAP;
    out->lampGlassCount = 0;
    out->lampPoolCount = 0;
    out->lampLightCount = 0;
    out->groundTotalCount = 0;
    out->treeCount = 0;
    out->buildingCount = 0;
    out->groundSpanCount = 0;
    out->buildingSpanCount = 0;
    out->buildingPage = -1;
    out->usedAtlasMask = 0;
    out->uncoveredAtlasMask = 0;
    out->pageCount = 0;
    out->bldWest = out->bldEast = out->bldNorth = out->bldSouth = 0.0f;
    out->bldTop = 0.0f;
    out->minY = FLT_MAX;
    out->maxY = -FLT_MAX;

    VoxelMesh_BeginWindow(x0 - 1, y0 - VOXEL_CHUNK_MARGIN_NORTH, x1 + 1, y1 + 1);

    // 1. Ground & Relief across all overlapping instances (each bound to its own tileset atlas!)
    double phaseStart = VitaPerf_NowMs();
    VoxelBuilder builder;
    for (uint32_t g = 0; g < in->groundCount; g++)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(in->ground[g].instance);
        if (!inst) continue;
        const int32_t aSlot = in->ground[g].atlasSlot;
        out->usedAtlasMask |= (uint8_t)(1u << aSlot);

        VoxelBuilder_Init(&builder, sScratch, SCRATCH_VERTICES);
        VoxelBuilder_SetAtlas(&builder, &sAtlasSlots[aSlot].map);
        builder.originX = 0.0f;
        builder.originZ = 0.0f;

        if (in->ground[g].overlaps)
        {
            builder.lighting = !inst->indoor;
            builder.lightingRefine = true;
            builder.base = VoxelRelief_Base(inst);
            VoxelBuilder_SetStretch(&builder, inst);
            VoxelMesh_EmitInstance(&builder, inst, x0, y0, x1, y1);
        }
        if (in->ground[g].border)
        {
            builder.lighting = !inst0->indoor;
            builder.lightingRefine = false;
            builder.base = VoxelRelief_Base(inst0);
            VoxelMesh_EmitBorder(&builder, x0, y0, x1, y1);
        }
        if (builder.uncovered > 0)
            out->uncoveredAtlasMask |= (uint8_t)(1u << aSlot);

        PackGroundIntoJob(out, &builder, aSlot, !inst->indoor,
                          (float)x0, (float)y0, (float)x1, (float)y1);
    }
    out->groundMs = VitaPerf_NowMs() - phaseStart;

    // 2. Trees & Tall Grass across all overlapping instances
    phaseStart = VitaPerf_NowMs();
    VoxelBuilder_Init(&builder, sScratch, SCRATCH_VERTICES);
    VoxelBuilder_SetAtlas(&builder, in->activeMap);
    builder.originX = 0.0f;
    builder.originZ = 0.0f;
    builder.triOpaque = sScratchTriBucket;

    for (uint32_t i = 0; i < instanceCount; i++)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(i);
        if (!inst) continue;
        if (x0 < inst->originX + inst->width && x1 > inst->originX
            && y0 < inst->originY + inst->height && y1 > inst->originY)
        {
            builder.lighting = !inst->indoor;
            builder.lightingRefine = false;
            builder.base = VoxelRelief_Base(inst);
            VoxelBuilder_SetStretch(&builder, inst);
            VoxelTree_EmitInstance(&builder, inst, x0, y0, x1, y1);
        }
    }

    if (in->hasBorder)
    {
        builder.lighting = !inst0->indoor;
        builder.lightingRefine = false;
        builder.base = VoxelRelief_Base(inst0);
        VoxelTree_EmitBorder(&builder, x0, y0, x1, y1);
    }

    if (builder.count > 0)
    {
        const uint32_t count = builder.count > CHUNK_TREE_CAP ? CHUNK_TREE_CAP : builder.count;
        for (uint32_t k = 0; k < count; k++)
        {
            const VoxelVertex *const v = &sScratch[k];
            ExpandChunkVertexBounds(out, (float)x0, (float)y0, (float)x1, (float)y1, v->x, v->y, v->z);
        }
        PackVoxelVertices(sScratch, out->treeVerts, count, !in->indoor);
        out->treeStaticCount = TagAndPartitionTreeSway(out->treeVerts, count, sScratchTriBucket, &out->treeOpaqueCount);
        out->treeCount = count;
    }
    else
    {
        out->treeOpaqueCount = 0u;
    }
    out->treesMs = VitaPerf_NowMs() - phaseStart;

    // 3. Buildings & Indoor furniture across all overlapping instances
    phaseStart = VitaPerf_NowMs();
    VoxelBuilder_Init(&builder, sScratch, SCRATCH_VERTICES);
    VoxelBuilder_SetAtlas(&builder, in->activeMap);
    builder.originX = 0.0f;
    builder.originZ = 0.0f;
    builder.triOpaque = sScratchTriBucket;

    for (uint32_t i = 0; i < instanceCount; i++)
    {
        const VoxelMapInstance *inst = VoxelWorld_Instance(i);
        if (!inst) continue;
        if (x0 < inst->originX + inst->width && x1 > inst->originX
            && y0 < inst->originY + inst->height && y1 > inst->originY)
        {
            builder.lighting = !inst->indoor;
            builder.lightingRefine = false;
            builder.base = VoxelRelief_Base(inst);
            VoxelBuilder_SetStretch(&builder, inst);
            uint32_t countBefore = builder.count;
            VoxelBuildings_EmitInstance(&builder, inst, x0, y0, x1, y1);
            if (builder.count > countBefore)
            {
                int32_t p = VoxelBuildings_PageOf(inst);
                if (p >= 0)
                {
                    NoteJobPage(out, p);
                    if (out->buildingPage < 0)
                        out->buildingPage = p;
                    uint32_t clampedEnd = builder.count > CHUNK_BUILDING_CAP ? CHUNK_BUILDING_CAP : builder.count;
                    if (clampedEnd > countBefore && out->buildingSpanCount < MAX_CHUNK_BUILDING_SPANS)
                    {
                        const uint32_t spanVerts = clampedEnd - countBefore;
                        uint32_t glowVerts = 0;
                        if (!inst->indoor && VoxelBuildings_PageHasGlow((uint32_t)p))
                        {
                            glowVerts = PartitionBuildingGlowTriangles(&sScratch[countBefore], spanVerts, p);
                        }
                        VoxelChunkBuildingSpan *bspan = &out->buildingSpans[out->buildingSpanCount++];
                        bspan->page = (int16_t)p;
                        bspan->firstVert = (uint16_t)countBefore;
                        bspan->vertCount = (uint16_t)spanVerts;
                        bspan->glowVertCount = (uint16_t)glowVerts;
                    }
                }
            }
        }
    }

    if (builder.count > 0)
    {
        uint32_t count = builder.count > CHUNK_BUILDING_CAP ? CHUNK_BUILDING_CAP : builder.count;
        // How far the models reach past the chunk's cells (built with origin 0, so in the
        // same coordinates as x0..y1): read off their vertices, a pass over at most
        // CHUNK_BUILDING_CAP of them, on the worker as the rest of the build.
        float maxY = out->bldTop;
        for (uint32_t k = 0; k < count; k++)
        {
            const VoxelVertex *const v = &sScratch[k];
            ExpandChunkVertexBounds(out, (float)x0, (float)y0, (float)x1, (float)y1, v->x, v->y, v->z);
            if (v->y > maxY) maxY = v->y;
        }
        out->bldTop = maxY;
        PackVoxelVertices(sScratch, out->buildingVerts, count, !in->indoor);
        out->buildingCount = count;
    }
    if (!in->indoor)
        EmitChunkLamps(in, out);
    if (out->minY > out->maxY)
    {
        out->minY = -2.5f;
        out->maxY = 2.0f;
    }
    else
    {
        out->minY -= 0.25f;
        out->maxY += 0.25f;
    }
    out->buildingsMs = VitaPerf_NowMs() - phaseStart;
    {
        VoxelLightingWork lightAfter;
        VoxelLighting_GetWork(&lightAfter);
        out->light.rays = lightAfter.rays - lightBefore.rays;
        out->light.steps = lightAfter.steps - lightBefore.steps;
        out->light.cells = lightAfter.cells - lightBefore.cells;
        out->light.blocks = lightAfter.blocks - lightBefore.blocks;
        out->light.rayMs = lightAfter.rayMs - lightBefore.rayMs;
        out->light.contactMs = lightAfter.contactMs - lightBefore.contactMs;
        out->light.apiMs = lightAfter.apiMs - lightBefore.apiMs;
    }
    out->computeMs = VitaPerf_NowMs() - computeStart;
}

static void CommitChunkJob(VoxelChunkSlot *const slot, const int32_t chunkX, const int32_t chunkY,
                           const uint32_t hash, const VoxelChunkJobOutput *const out,
                           const uint32_t *const atlasGen, const uint32_t *const atlasExt)
{
    // Check if the SGX543 GPU command queue (VOXEL_VITAGL_PURGE_FRAMES) may still read this slot's VBO.
    const bool gpuMayRead = (slot->valid && slot->vboCap > 0u
                             && (uint32_t)(sFrameCounter - slot->lastUsedFrame) <= VOXEL_VITAGL_PURGE_FRAMES);

    // Building pages first: synchronous while warming up or faded, as before.
    for (uint32_t k = 0; k < out->pageCount; k++)
        RequestOrLoadBuildingPage(out->pages[k], sSyncBuildingLoads);

    slot->chunkX = chunkX;
    slot->chunkY = chunkY;
    slot->hash = hash;
    slot->lastUsedFrame = sFrameCounter;
    slot->checkedEpoch = sWorldEpoch;
    slot->uncovered = (out->uncoveredAtlasMask != 0);
    if (slot->uncovered)
        sNeedAtlasExtension = true;
    slot->usedAtlasMask = out->usedAtlasMask;
    slot->groundSpanCount = out->groundSpanCount;
    memcpy(slot->groundSpans, out->groundSpans, sizeof(out->groundSpans[0]) * out->groundSpanCount);
    slot->buildingSpanCount = out->buildingSpanCount;
    memcpy(slot->buildingSpans, out->buildingSpans, sizeof(out->buildingSpans[0]) * out->buildingSpanCount);
    slot->bldWest = out->bldWest;
    slot->bldEast = out->bldEast;
    slot->bldNorth = out->bldNorth;
    slot->bldSouth = out->bldSouth;
    slot->bldTop = out->bldTop;
    slot->buildingPage = out->buildingPage;
    slot->groundTotalCount = out->groundTotalCount;
    slot->treeCount = out->treeCount;
    slot->treeOpaqueCount = (uint16_t)out->treeOpaqueCount;
    slot->treeStaticCount = (uint16_t)out->treeStaticCount;
    slot->buildingCount = out->buildingCount;
    slot->lampGlassCount = (uint16_t)out->lampGlassCount;
    slot->lampPoolCount = (uint16_t)out->lampPoolCount;
    slot->lampLightCount = (uint8_t)out->lampLightCount;
    memcpy(slot->lampLights, out->lampLights, sizeof(out->lampLights[0]) * out->lampLightCount);
    // Built in the current coordinates, so drawn where it is built.
    slot->drawOffsetX = 0.0f;
    slot->drawOffsetZ = 0.0f;

    slot->minY = out->minY;
    slot->maxY = out->maxY;

    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        slot->atlasGen[s] = (atlasGen != NULL) ? atlasGen[s] : sAtlasSlots[s].generation;
        slot->atlasExt[s] = (atlasExt != NULL) ? atlasExt[s] : sAtlasSlots[s].extension;
    }
    slot->treeFirstVert = (uint16_t)slot->groundTotalCount;
    slot->buildingFirstVert = (uint16_t)(slot->groundTotalCount + slot->treeCount);
    slot->lampFirstVert = (uint16_t)(slot->buildingFirstVert + slot->buildingCount);
    slot->totalVertCount = (uint16_t)(slot->lampFirstVert + slot->lampGlassCount + slot->lampPoolCount);
    VitaPerf_AddCount(VITA_PERF_C_BUILD_VERTS, slot->totalVertCount);

    // UploadChunkVbo reads the staging buffer through the slot's part pointers.
    slot->groundVerts = out->groundVerts;
    slot->treeVerts = out->treeVerts;
    slot->buildingVerts = out->buildingVerts;
    slot->lampVerts = out->lampVerts;
    VitaPerf_SubBegin(VITA_PERF_T_BUILD_UPLOAD);
    UploadChunkVbo(slot, gpuMayRead);
    VitaPerf_SubEnd(VITA_PERF_T_BUILD_UPLOAD);
    // The staging buffer belongs to the next build now.
    slot->groundVerts = NULL;
    slot->treeVerts = NULL;
    slot->buildingVerts = NULL;
    slot->lampVerts = NULL;

    slot->valid = true;
}

// Adds what a computed job measured to the current frame's profile (Core 0 only).
static void ReportChunkJobWork(const VoxelChunkJobOutput *out, bool onWorker)
{
    VitaPerf_AddTime(VITA_PERF_T_BUILD_GROUND, out->groundMs);
    VitaPerf_AddTime(VITA_PERF_T_BUILD_TREES, out->treesMs);
    VitaPerf_AddTime(VITA_PERF_T_BUILD_BUILDINGS, out->buildingsMs);
    VitaPerf_AddCount(VITA_PERF_C_LIGHT_RAYS, out->light.rays);
    VitaPerf_AddCount(VITA_PERF_C_LIGHT_STEPS, out->light.steps);
    VitaPerf_AddCount(VITA_PERF_C_LIGHT_CELLS, out->light.cells);
    VitaPerf_AddCount(VITA_PERF_C_LIGHT_BLOCKS, out->light.blocks);
    VitaPerf_AddTime(VITA_PERF_T_LIGHT_RAYS, out->light.rayMs);
    VitaPerf_AddTime(VITA_PERF_T_LIGHT_CONTACT, out->light.contactMs);
    VitaPerf_AddTime(VITA_PERF_T_LIGHT_API, out->light.apiMs);
    if (onWorker)
    {
        VitaPerf_AddTime(VITA_PERF_T_WORKER_COMPUTE, out->computeMs);
        VitaPerf_AddCount(VITA_PERF_C_WORKER_BUILT, 1);
    }
}

// Extends the atlas slots a computed job found missing metatiles in. True if any grew.
static bool ExtendJobAtlasSlots(const VoxelChunkJobInput *in, const VoxelChunkJobOutput *out)
{
    bool extended = false;
    VitaPerf_SubBegin(VITA_PERF_T_BUILD_ATLAS_EXT);
    for (uint32_t g = 0; g < in->groundCount; g++)
    {
        const int32_t aSlot = in->ground[g].atlasSlot;
        if (!(out->uncoveredAtlasMask & (1u << aSlot)))
            continue;
        const VoxelMapInstance *inst = VoxelWorld_Instance(in->ground[g].instance);
        if (inst != NULL && ExtendAtlasSlot(&sAtlasSlots[aSlot], inst))
        {
            extended = true;
            VitaPerf_MarkEvent(VITA_PERF_E_ATLAS_EXT);
            if (aSlot == sActiveAtlasSlot)
                ActivateAtlasSlot(aSlot);
        }
    }
    VitaPerf_SubEnd(VITA_PERF_T_BUILD_ATLAS_EXT);
    return extended;
}

// Build a single 8x8 chunk covering all overlapping map instances and borders, now: the three
// phases one after the other on Core 0.
static void BuildChunk(VoxelChunkSlot *slot, int32_t chunkX, int32_t chunkY, uint32_t hash)
{
    static VoxelChunkJobInput sJobIn;
    static VoxelChunkJobOutput sJobOut;

    PrepareChunkJob(&sJobIn, chunkX, chunkY);
    ComputeChunkJob(&sJobIn, &sJobOut, sChunkStaging);
    ReportChunkJobWork(&sJobOut, false);

    // Metatiles missing from an atlas slot: extend the slot and compute again, so the chunk is
    // drawn complete (what the single-phase build did for the instance in place).
    if (sJobOut.uncoveredAtlasMask != 0 && ExtendJobAtlasSlots(&sJobIn, &sJobOut))
    {
        ComputeChunkJob(&sJobIn, &sJobOut, sChunkStaging);
        ReportChunkJobWork(&sJobOut, false);
    }

    CommitChunkJob(slot, chunkX, chunkY, hash, &sJobOut, NULL, NULL);
}

// ---- Chunk worker (Core 2) ----
//
// Adaptive batches of 2 (normal traversal) or 4 (visible holes / Fly) chunks are computed on Core 2
// (ComputeChunkJob) while Core 0 renders, with cooperative abort on map changes (sAsyncAbortBatch).
// PrepareChunkJob and CommitChunkJob stay on Core 0 (GL and atlas ownership).
// the frame looks at the chunks. In that window the worker owns the lighting caches, the
// building module's caches, the mesh builder's window, sScratch and sChunkStaging; the world
// lookups' memos are frozen (VoxelWorld_FreezeMemos). A result is dropped when the world
// epoch or the generation of an atlas slot it uses changed meanwhile. Visible holes and
// warm-up/faded frames are still built on Core 0 (BuildChunk). See docs/reference/THREADS.md.

typedef enum
{
    CHUNK_JOB_NONE,    // nothing chosen
    CHUNK_JOB_CHOSEN,  // chosen this frame, launched after the swap
    CHUNK_JOB_RUNNING, // on the worker
    CHUNK_JOB_DONE,    // computed, to commit
} ChunkJobState;

static SDL_Thread *sChunkWorkerThread = NULL;
static SDL_sem *sChunkJobSem = NULL;
static SDL_sem *sChunkDoneSem = NULL;
static SDL_atomic_t sChunkWorkerStop;
static ChunkJobState sChunkJobState = CHUNK_JOB_NONE;
static int32_t sAsyncBatchCount = 0;
static int32_t sAsyncDoneCount = 0;
static volatile int32_t sAsyncAbortBatch = 0;
static VoxelChunkJobInput sAsyncIn[CHUNK_WORKER_BATCH];
static VoxelChunkJobOutput sAsyncOut[CHUNK_WORKER_BATCH];
static int32_t sAsyncCx[CHUNK_WORKER_BATCH];
static int32_t sAsyncCy[CHUNK_WORKER_BATCH];
static uint32_t sAsyncHash[CHUNK_WORKER_BATCH];
static uint32_t sAsyncEpoch;
static uint32_t sAsyncAtlasGen[VOXEL_ATLAS_SLOTS];
static uint32_t sAsyncAtlasExt[VOXEL_ATLAS_SLOTS];

static int32_t VoxelChunkWorkerMain(void *data)
{
    (void)data;
#ifdef PLATFORM_VITA
    VitaPlatform_PinCurrentThread("VoxelChunkWorker", 2);
#endif
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
    while (!SDL_AtomicGet(&sChunkWorkerStop))
    {
        if (SDL_SemWait(sChunkJobSem) != 0)
            continue;
        if (SDL_AtomicGet(&sChunkWorkerStop))
            break;
        int32_t done = 0;
        for (int32_t i = 0; i < sAsyncBatchCount; ++i)
        {
            if (i > 0 && __atomic_load_n(&sAsyncAbortBatch, __ATOMIC_ACQUIRE) != 0)
                break;
            VoxelGlVertex *const staging = sChunkStaging + (size_t)i * (size_t)CHUNK_VERTS_PER_SLOT;
            ComputeChunkJob(&sAsyncIn[i], &sAsyncOut[i], staging);
            done = i + 1;
        }
        sAsyncDoneCount = done;
        SDL_SemPost(sChunkDoneSem);
    }
    return 0;
}

static void ChunkWorker_Start(void)
{
#if VOXEL_CHUNK_WORKER
    if (sChunkWorkerThread != NULL)
        return;
    SDL_AtomicSet(&sChunkWorkerStop, 0);
    sChunkJobSem = SDL_CreateSemaphore(0);
    sChunkDoneSem = SDL_CreateSemaphore(0);
    if (sChunkJobSem && sChunkDoneSem)
        sChunkWorkerThread = SDL_CreateThreadWithStackSize(VoxelChunkWorkerMain, "VoxelChunkWorker", 1024 * 1024, NULL);
    sChunkJobState = CHUNK_JOB_NONE;
#endif
}

static inline bool ChunkWorker_Available(void)
{
    return sChunkWorkerThread != NULL;
}

// Waits for a running job (Core 0). Afterwards nothing runs on the worker.
static void ChunkWorker_Join(void)
{
    // A chunk chosen by a frame that was never presented: chosen again from this frame's view.
    if (sChunkJobState == CHUNK_JOB_CHOSEN)
        sChunkJobState = CHUNK_JOB_NONE;
    if (sChunkJobState != CHUNK_JOB_RUNNING)
        return;
    VitaPerf_SubBegin(VITA_PERF_T_WORKER_WAIT);
    __atomic_store_n(&sAsyncAbortBatch, 1, __ATOMIC_RELEASE);
    SDL_SemWait(sChunkDoneSem);
    VitaPerf_SubEnd(VITA_PERF_T_WORKER_WAIT);
    VoxelWorld_FreezeMemos(false);
    sChunkJobState = CHUNK_JOB_DONE;
}

// At the top of a frame (Core 0): true while the job still runs. A finished job is collected
// (to commit) without waiting.
static bool ChunkWorker_Poll(void)
{
    if (sChunkJobState == CHUNK_JOB_CHOSEN)
        sChunkJobState = CHUNK_JOB_NONE;
    if (sChunkJobState != CHUNK_JOB_RUNNING)
        return false;
    if (SDL_SemTryWait(sChunkDoneSem) == 0)
    {
        VoxelWorld_FreezeMemos(false);
        sChunkJobState = CHUNK_JOB_DONE;
        return false;
    }
    return true;
}

// A frame that starts while the job runs goes on alongside it, touching nothing the job owns
// (no instance rebuild, no build, entities without lighting), and waits here only where it
// needs the world for itself: a map, tileset or tile change, the sun moving, a hole to build,
// lamps to rebuild. Waiting at the top of every frame cost Core 0 up to 35 ms behind a long
// chunk (logs/sesion 25-26).
static bool sWorkerBusyThisFrame = false;

static void EnsureWorkerIdle(void)
{
    if (!sWorkerBusyThisFrame)
        return;
    ChunkWorker_Join();
    sWorkerBusyThisFrame = false;
}

// Chooses a batch of up to CHUNK_WORKER_BATCH chunks for Core 2 to compute after this frame.
static void ChunkWorker_ChooseBatch(const int32_t *cx, const int32_t *cy, const uint32_t *hash, int32_t count)
{
    if (count <= 0)
        return;
    if (count > CHUNK_WORKER_BATCH)
        count = CHUNK_WORKER_BATCH;
    for (int32_t i = 0; i < count; ++i)
    {
        sAsyncCx[i] = cx[i];
        sAsyncCy[i] = cy[i];
        sAsyncHash[i] = hash[i];
    }
    sAsyncBatchCount = count;
    sChunkJobState = CHUNK_JOB_CHOSEN;
}

// After the swap (Core 0): prepares the chosen batch and hands it to Core 2.
static void ChunkWorker_Launch(void)
{
    if (sChunkJobState != CHUNK_JOB_CHOSEN || sAsyncBatchCount <= 0 || !ChunkWorker_Available())
        return;
    for (int32_t i = 0; i < sAsyncBatchCount; ++i)
        PrepareChunkJob(&sAsyncIn[i], sAsyncCx[i], sAsyncCy[i]);
    sAsyncEpoch = sWorldEpoch;
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        sAsyncAtlasGen[s] = sAtlasSlots[s].generation;
        sAsyncAtlasExt[s] = sAtlasSlots[s].extension;
    }
    __atomic_store_n(&sAsyncAbortBatch, 0, __ATOMIC_RELEASE);
    sAsyncDoneCount = 0;
    VoxelWorld_FreezeMemos(true);
    sChunkJobState = CHUNK_JOB_RUNNING;
    SDL_SemPost(sChunkJobSem);
}

// Commits a finished batch if the world it was computed in still stands (Core 0, before the
// chunk pass). Dropped jobs are simply chosen again.
static void ChunkWorker_Commit(void)
{
    if (sChunkJobState != CHUNK_JOB_DONE)
        return;
    sChunkJobState = CHUNK_JOB_NONE;

    const int32_t count = sAsyncDoneCount;
    sAsyncBatchCount = 0;
    sAsyncDoneCount = 0;
    for (int32_t i = 0; i < count; ++i)
    {
        bool current = (sAsyncEpoch == sWorldEpoch && !sNeedsFullWarmup);
        for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS && current; s++)
        {
            if ((sAsyncOut[i].usedAtlasMask & (1u << s))
                && (!sAtlasSlots[s].valid || sAtlasSlots[s].generation != sAsyncAtlasGen[s]))
                current = false;
        }
        VoxelChunkSlot *slot = FindChunkSlot(sAsyncCx[i], sAsyncCy[i]);
        // Rebuilt on Core 0 meanwhile (it became a hole or the frame warmed up).
        if (slot != NULL && slot->valid && slot->checkedEpoch == sWorldEpoch && slot->hash == sAsyncHash[i])
            current = false;
        if (!current)
        {
            VitaPerf_AddCount(VITA_PERF_C_WORKER_DROPPED, 1);
            continue;
        }
        ReportChunkJobWork(&sAsyncOut[i], true);
        if (slot == NULL)
            slot = GetAvailableChunkSlot(sAsyncCx[i], sAsyncCy[i]);
        if (sAsyncOut[i].uncoveredAtlasMask != 0 && ExtendJobAtlasSlots(&sAsyncIn[i], &sAsyncOut[i]))
        {
            // Never commit a half-black chunk: recompute immediately against the extended atlas.
            ComputeChunkJob(&sAsyncIn[i], &sAsyncOut[i], sChunkStaging);
            ReportChunkJobWork(&sAsyncOut[i], false);
            CommitChunkJob(slot, sAsyncCx[i], sAsyncCy[i], sAsyncHash[i], &sAsyncOut[i], NULL, NULL);
        }
        else
        {
            CommitChunkJob(slot, sAsyncCx[i], sAsyncCy[i], sAsyncHash[i], &sAsyncOut[i], sAsyncAtlasGen, sAsyncAtlasExt);
        }
    }
}

// ---- Chunk build budget and pre-warming direction ----
//
// A chunk build costs several milliseconds (8.4 ms on average in logs/SESION 17), so builds
// are limited by time, not by count: one starts only if the frame, with that build and the
// passes still to come, stays inside its budget. A visible hole is always filled (one per
// frame); rebuilds of chunks still drawn and pre-warming wait for room.

static double sBuildCostMs = VOXEL_BUILD_COST_INIT_MS;
static float sMoveVelX = 0.0f, sMoveVelZ = 0.0f;
static float sMovePrevX = 0.0f, sMovePrevZ = 0.0f;
static bool sMovePrevValid = false;

static bool ChunkBuildFits(double frameStartMs, double budgetMs, int32_t builtThisFrame, bool fillsHole)
{
    // A visible hole shows the black background, worse than a late frame: up to
    // VOXEL_HOLE_BUILDS_PER_FRAME are filled whatever the budget (as many builds as before E1).
    if (fillsHole && builtThisFrame < VOXEL_HOLE_BUILDS_PER_FRAME)
        return true;
    const double elapsedMs = VitaPerf_NowMs() - frameStartMs;
    return elapsedMs + sBuildCostMs + VOXEL_BUILD_TAIL_MS <= budgetMs;
}

static void TimedBuildChunk(VoxelChunkSlot *slot, int32_t cx, int32_t cy, uint32_t hash)
{
    VitaPerf_MarkBuildStart();
    const double startMs = VitaPerf_NowMs();
    BuildChunk(slot, cx, cy, hash);
    const double costMs = VitaPerf_NowMs() - startMs;
    // Moving average, so one expensive chunk doesn't block building for long.
    sBuildCostMs += (costMs - sBuildCostMs) * 0.25;
    VitaPerf_MarkBuildEnd(1);
}

// Smoothed player velocity in tiles per frame. A frame that re-bases the coordinates (map
// change or crossing) gives no sample, but keeps the velocity: walking on into the next map
// keeps pre-warming ahead.
static void UpdateMovementDirection(float playerX, float playerZ, bool rebased)
{
    if (!sMovePrevValid)
    {
        sMoveVelX = 0.0f;
        sMoveVelZ = 0.0f;
    }
    else if (!rebased)
    {
        const float dx = playerX - sMovePrevX;
        const float dz = playerZ - sMovePrevZ;
        // A jump of several tiles is a warp, not movement.
        if (fabsf(dx) < 2.0f && fabsf(dz) < 2.0f)
        {
            sMoveVelX += (dx - sMoveVelX) * 0.15f;
            sMoveVelZ += (dz - sMoveVelZ) * 0.15f;
        }
        else
        {
            sMoveVelX = 0.0f;
            sMoveVelZ = 0.0f;
        }
    }
    sMovePrevX = playerX;
    sMovePrevZ = playerZ;
    sMovePrevValid = true;
}

static void MovementLookahead(float *x, float *z)
{
    const float speed = sqrtf(sMoveVelX * sMoveVelX + sMoveVelZ * sMoveVelZ);
    if (speed > VOXEL_MOVE_THRESHOLD)
    {
        const float lookahead = Voxel_ClampF(VOXEL_PREFETCH_LOOKAHEAD + speed * VOXEL_PREFETCH_SPEED_GAIN,
                                             VOXEL_PREFETCH_LOOKAHEAD, VOXEL_PREFETCH_LOOKAHEAD_MAX);
        *x += sMoveVelX / speed * lookahead;
        *z += sMoveVelZ / speed * lookahead;
    }
}

// ---- Emerald 3DS Lighting, Weather, Fog, Motes, Rays & Night Street Lamps ----


static void BrightenCrowns(uint16_t *texels)
{
    static const struct
    {
        uint32_t x, y, w, h;
    } crowns[] = {{0, 0, 32, 36}, {32, 32, 16, 32}, {64, 0, 16, 32}, {0, 64, 128, 32}};

    for (uint32_t c = 0; c < sizeof(crowns) / sizeof(crowns[0]); ++c)
    {
        for (uint32_t y = crowns[c].y; y < crowns[c].y + crowns[c].h; ++y)
        {
            for (uint32_t x = crowns[c].x; x < crowns[c].x + crowns[c].w; ++x)
            {
                VoxelGrade_Brighten(&texels[CtrVideo_Texel(x, y, VOXEL_TREE_TEXTURE_DIM)], 1,
                                    VOXEL_TREE_BRIGHTNESS);
            }
        }
    }
}

static VoxelLight LightMix(const VoxelLight *a, const VoxelLight *b, float t)
{
    VoxelLight out = *a;
    for (int32_t i = 0; i < 3; ++i)
    {
        out.sun[i] += (b->sun[i] - a->sun[i]) * t;
        out.shade[i] += (b->shade[i] - a->shade[i]) * t;
        out.hazeRgb[i] += (b->hazeRgb[i] - a->hazeRgb[i]) * t;
    }
    out.haze += (b->haze - a->haze) * t;
    out.dappleLow += (b->dappleLow - a->dappleLow) * t;
    out.dappleHigh += (b->dappleHigh - a->dappleHigh) * t;
    out.rays += (b->rays - a->rays) * t;
    out.bloom += (b->bloom - a->bloom) * t;
    out.motes += (b->motes - a->motes) * t;
    out.hazeStart += (b->hazeStart - a->hazeStart) * t;
    out.hazeRamp += (b->hazeRamp - a->hazeRamp) * t;
    return out;
}

static VoxelLight FogLight(const VoxelLight *clear, bool cave)
{
    VoxelLight fog = *clear;
    if (cave)
    {
        fog.sun[0] = 0.76f; fog.sun[1] = 0.80f; fog.sun[2] = 0.88f;
        fog.shade[0] = 0.58f; fog.shade[1] = 0.64f; fog.shade[2] = 0.78f;
        fog.haze = 0.86f;
        fog.dappleLow = fog.dappleHigh = 1.0f;
        fog.hazeRgb[0] = 0.10f; fog.hazeRgb[1] = 0.12f; fog.hazeRgb[2] = 0.17f;
        fog.hazeStart = 0.86f;
        fog.hazeRamp = 0.55f;
        fog.rays = 0.0f;
        fog.bloom = 0.0f;
        fog.motes = 0.0f;
    }
    else
    {
        fog.sun[0] = 0.95f; fog.sun[1] = 0.98f; fog.sun[2] = 1.00f;
        fog.haze = 0.70f;
        fog.dappleLow = 0.95f; fog.dappleHigh = 1.02f;
        fog.rays = 0.0f;
        fog.bloom = 0.18f;
        fog.motes = 0.0f;
        fog.hazeRgb[0] = 0.80f; fog.hazeRgb[1] = 0.83f; fog.hazeRgb[2] = 0.87f;
        fog.hazeStart = 0.80f;
        fog.hazeRamp = 0.65f;
    }
    return LightMix(clear, &fog, VoxelWorld_FogDensity());
}

static VoxelLight BaseLightFor(bool indoor, VoxelWeatherClass weather, float weatherAmount, float bolt)
{
    VoxelLight light = {{1.00f, 0.99f, 0.95f}, {0.93f, 0.97f, 1.05f}, VOXEL_HAZE_MAX,
                        0.96f, 1.04f, 0.20f, 0.07f, 0.0f,
                        {(VOXEL_HAZE_COLOUR & 255) / 255.0f,
                         ((VOXEL_HAZE_COLOUR >> 8) & 255) / 255.0f,
                         ((VOXEL_HAZE_COLOUR >> 16) & 255) / 255.0f},
                        VOXEL_HAZE_START, VOXEL_HAZE_RAMP};

    if (!indoor)
    {
        switch (weather)
        {
        case VOXEL_WEATHER_SUN:
            light.sun[0] = 1.04f; light.sun[1] = 1.01f; light.sun[2] = 0.93f;
            light.haze = 0.12f;
            light.dappleLow = 0.94f; light.dappleHigh = 1.07f;
            light.rays = 0.30f;
            light.bloom = 0.10f;
            light.motes = 1.00f;
            break;
        case VOXEL_WEATHER_RAIN:
            light.sun[0] = 0.81f; light.sun[1] = 0.88f; light.sun[2] = 0.98f;
            light.shade[0] = 0.82f; light.shade[1] = 0.89f; light.shade[2] = 1.00f;
            light.haze = 0.30f + 0.18f * weatherAmount;
            light.dappleLow = light.dappleHigh = 1.0f;
            light.rays = 0.0f;
            light.bloom = 0.08f + 0.30f * bolt;
            light.motes = 0.0f;
            light.hazeRgb[0] = 0.62f; light.hazeRgb[1] = 0.68f; light.hazeRgb[2] = 0.76f;
            light.hazeStart = 0.85f;
            for (int32_t c = 0; c < 3; ++c)
            {
                light.sun[c] += (1.55f - light.sun[c]) * bolt;
                light.shade[c] += (1.45f - light.shade[c]) * bolt;
                light.hazeRgb[c] += (0.92f - light.hazeRgb[c]) * bolt;
            }
            break;
        case VOXEL_WEATHER_SAND:
        {
            VoxelLight sand = light;
            sand.sun[0] = 1.04f; sand.sun[1] = 0.93f; sand.sun[2] = 0.76f;
            sand.shade[0] = 0.96f; sand.shade[1] = 0.86f; sand.shade[2] = 0.72f;
            sand.haze = 0.79f;
            sand.hazeRgb[0] = 0.86f; sand.hazeRgb[1] = 0.70f; sand.hazeRgb[2] = 0.48f;
            sand.hazeStart = 0.55f;
            sand.hazeRamp = 0.95f;
            sand.dappleLow = sand.dappleHigh = 1.0f;
            sand.rays = 0.0f;
            sand.bloom = 0.16f;
            sand.motes = 0.0f;
            light = LightMix(&light, &sand, weatherAmount);
            break;
        }
        case VOXEL_WEATHER_ASH:
        {
            VoxelLight ash = light;
            ash.sun[0] = 0.90f; ash.sun[1] = 0.87f; ash.sun[2] = 0.84f;
            ash.shade[0] = 0.80f; ash.shade[1] = 0.80f; ash.shade[2] = 0.83f;
            ash.haze = 0.50f;
            ash.hazeRgb[0] = 0.66f; ash.hazeRgb[1] = 0.63f; ash.hazeRgb[2] = 0.61f;
            ash.hazeStart = 0.75f;
            ash.hazeRamp = 0.90f;
            ash.dappleLow = 0.96f; ash.dappleHigh = 1.01f;
            ash.rays = 0.0f;
            ash.bloom = 0.10f;
            ash.motes = 0.0f;
            light = LightMix(&light, &ash, weatherAmount);
            break;
        }
        case VOXEL_WEATHER_FOG:
            light = FogLight(&light, VoxelWorld_Underground());
            break;
        case VOXEL_WEATHER_PARTICLES:
            light.sun[0] = 0.93f; light.sun[1] = 0.92f; light.sun[2] = 0.88f;
            light.haze = 0.36f;
            light.dappleLow = 0.92f; light.dappleHigh = 1.03f;
            light.rays = 0.0f;
            light.bloom = 0.12f;
            light.motes = 0.0f;
            break;
        case VOXEL_WEATHER_SHADE:
            light.sun[0] = 0.92f; light.sun[1] = 0.95f; light.sun[2] = 0.98f;
            light.haze = 0.25f;
            light.dappleLow = 0.92f; light.dappleHigh = 1.03f;
            light.rays = 0.0f;
            light.bloom = 0.12f;
            light.motes = 0.40f;
            break;
        default:
            break;
        }
        return light;
    }

    for (int32_t i = 0; i < 3; ++i)
        light.sun[i] = light.shade[i] = 1.0f;
    light.haze = 0.0f;
    light.dappleLow = light.dappleHigh = 1.0f;
    light.rays = 0.0f;
    light.bloom = 0.0f;
    light.motes = 0.0f;
    return light;
}

static VoxelLight LightFor(bool indoor, VoxelWeatherClass weather, float weatherAmount, float bolt)
{
    VoxelLight light = BaseLightFor(indoor, weather, weatherAmount, bolt);
    if (!indoor)
    {
        const VitaDayNightLight *const cycle = VitaDayNight_Get();
        const VitaDayNightBlend moon = VitaDayNight_State()->moonlight;

        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            light.sun[channel] *= cycle->rgb[channel] * (1.0f - moon.amount);
            light.shade[channel] *= cycle->rgb[channel] * (1.0f - moon.amount);
            light.hazeRgb[channel] *= cycle->rgb[channel];
            light.hazeRgb[channel] += (moon.rgb[channel] - light.hazeRgb[channel]) * moon.amount;
        }
        // Keep full sun-ray & sky-bloom strength through dawn and sunset (daylight >= 0.35),
        // letting sunP.strength handle the 05:30/19:30 horizon fade and cutting cleanly to 0 at night.
        light.rays *= Voxel_ClampF(cycle->daylight / 0.35f, 0.0f, 1.0f);
        light.motes *= cycle->daylight;
        light.dappleLow = 1.0f + (light.dappleLow - 1.0f) * cycle->daylight;
        light.dappleHigh = 1.0f + (light.dappleHigh - 1.0f) * cycle->daylight;
    }
    return light;
}

static float DappleLattice(int32_t x, int32_t y, int32_t periodX, int32_t periodY, uint32_t seed)
{
    uint32_t h = (uint32_t)(x % periodX) * 374761393u + (uint32_t)(y % periodY) * 668265263u
               + seed * 2246822519u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}

static float Quintic(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float DappleNoise(float u, float v, int32_t periodX, int32_t periodY, uint32_t seed)
{
    int32_t x = (int32_t)u, y = (int32_t)v;
    float fx = Quintic(u - (float)x), fy = Quintic(v - (float)y);
    float a = DappleLattice(x, y, periodX, periodY, seed), b = DappleLattice(x + 1, y, periodX, periodY, seed);
    float c = DappleLattice(x, y + 1, periodX, periodY, seed), d = DappleLattice(x + 1, y + 1, periodX, periodY, seed);
    float top = a + (b - a) * fx, bottom = c + (d - c) * fx;

    return top + (bottom - top) * fy;
}

static void MakeFog(void)
{
    uint32_t fogPixels[VOXEL_FOG_DIM * VOXEL_FOG_DIM];
    for (int32_t y = 0; y < VOXEL_FOG_DIM; ++y)
    {
        float v = (float)y / (float)VOXEL_FOG_DIM;
        for (int32_t x = 0; x < VOXEL_FOG_DIM; ++x)
        {
            float u = (float)x / (float)VOXEL_FOG_DIM;
            // Wrap U by periodX (3,6,12,24) and V by periodY (2,4,8,16) so the cloud/fog
            // texture tiles 100% seamlessly in both axes with zero horizontal seams.
            float n = DappleNoise(u * 3.0f,  v * 2.0f,  3,  2, 0xF06B10u) * 0.50f
                    + DappleNoise(u * 6.0f,  v * 4.0f,  6,  4, 0xF06B21u) * 0.28f
                    + DappleNoise(u * 12.0f, v * 8.0f,  12, 8, 0xF06B32u) * 0.15f
                    + DappleNoise(u * 24.0f, v * 16.0f, 24, 16, 0xF06B43u) * 0.07f;
            float t = (n - 0.34f) * (1.0f / (0.64f - 0.34f));
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            t = t * t * (3.0f - 2.0f * t);
            uint8_t a = (uint8_t)(t * 255.0f + 0.5f);
            fogPixels[y * VOXEL_FOG_DIM + x] = ((uint32_t)a << 24) | 0x00FFFFFFu;
        }
    }

    glGenTextures(1, &sFogTex);
    glBindTexture(GL_TEXTURE_2D, sFogTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_FOG_DIM, VOXEL_FOG_DIM, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, fogPixels);

    sFogCacheValid = false;
}

static void MakeWeather(void)
{
    uint32_t seed = 0xC0FFEE11u;
    for (uint32_t i = 0; i < VOXEL_WEATHER_COUNT; ++i)
    {
        for (uint32_t c = 0; c < 3; ++c)
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            sWeatherSeeds[i][c] = (float)(seed & 0x7FFFu) / 32768.0f;
        }
    }
}

static float SurfaceWorldY(const float wx, const float wz)
{
    const VoxelMapInstance *const inst = VoxelWorld_GetInstanceAt((int32_t)floorf(wx), (int32_t)floorf(wz));
    const float base = (inst != NULL) ? VoxelRelief_Base(inst) : 0.0f;
    return base + VoxelRelief_SurfaceAt(wx, wz);
}

static float SmoothedFogWorldY(const float wx, const float wz)
{
    const float c  = SurfaceWorldY(wx, wz);
    const float nx = SurfaceWorldY(wx - 1.5f, wz);
    const float px = SurfaceWorldY(wx + 1.5f, wz);
    const float nz = SurfaceWorldY(wx, wz - 1.5f);
    const float pz = SurfaceWorldY(wx, wz + 1.5f);
    return c * 0.40f + (nx + px + nz + pz) * 0.15f;
}

static void DrawFogBanks(const FogBank banks[2], float amount, uint8_t r, uint8_t g, uint8_t b, double clock)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_FOG))
        return;
    if (!sFogTex || amount <= 0.005f)
        return;

    // Expand fog bank span dynamically when the procedural camera frustum widens (e.g. pitch 25°).
    const float viewHalfX = fmaxf(fabsf(sViewState.maxX - sCamera.targetX),
                                  fabsf(sCamera.targetX - sViewState.minX));
    const float viewSouth = fmaxf(sViewState.maxZ - sCamera.targetZ, 0.0f);
    const float viewNorth = fmaxf(sCamera.targetZ - sViewState.minZ, 0.0f);
    const float spanX = fmaxf(VOXEL_FOG_SPAN_X, viewHalfX + 4.0f);
    const float spanSouth = fmaxf(VOXEL_FOG_SOUTH, viewSouth + 4.0f);
    const float spanNorth = fmaxf(VOXEL_FOG_NORTH, viewNorth + 4.0f);

    const float stepX = (2.0f * spanX) / (float)VOXEL_FOG_GRID;
    const float stepZ = (spanSouth + spanNorth) / (float)VOXEL_FOG_GRID;
    const int32_t snapCellX = (int32_t)floorf(sCamera.targetX / stepX);
    const int32_t snapCellZ = (int32_t)floorf(sCamera.targetZ / stepZ);

    static float sFogCachedSpanX = 0.0f;
    static float sFogCachedSpanN = 0.0f;
    if (!sFogCacheValid || snapCellX != sFogSnapCellX || snapCellZ != sFogSnapCellZ
     || fabsf(spanX - sFogCachedSpanX) > 2.0f || fabsf(spanNorth - sFogCachedSpanN) > 2.0f)
    {
        sFogSnapCellX = snapCellX;
        sFogSnapCellZ = snapCellZ;
        sFogCachedSpanX = spanX;
        sFogCachedSpanN = spanNorth;
        sFogCacheValid = true;
        const float snapX = (float)snapCellX * stepX;
        const float snapZ = (float)snapCellZ * stepZ;
        for (int32_t vx = 0; vx <= VOXEL_FOG_GRID; ++vx)
            sFogNodeX[vx] = snapX + ((float)vx - 0.5f * (float)VOXEL_FOG_GRID) * stepX;
        for (int32_t vz = 0; vz <= VOXEL_FOG_GRID; ++vz)
        {
            sFogNodeZ[vz] = snapZ + spanSouth - (float)vz * stepZ;
            for (int32_t vx = 0; vx <= VOXEL_FOG_GRID; ++vx)
                sFogNodeY[vz][vx] = SmoothedFogWorldY(sFogNodeX[vx], sFogNodeZ[vz]);
        }
    }

    const float maxRx = spanX - stepX;
    const float maxSouth = spanSouth - stepZ;
    const float maxNorth = spanNorth - stepZ;
    const float invFadeX = 1.0f / (maxRx * 0.30f);
    const float invFadeSouth = 1.0f / (maxSouth * 0.40f);
    const float invFadeNorth = 1.0f / (maxNorth * 0.30f);

    for (int32_t vz = 0; vz <= VOXEL_FOG_GRID; ++vz)
    {
        const float dzCam = sFogNodeZ[vz] - sCamera.targetZ;
        float edgeZ = (dzCam > 0.0f) ? ((maxSouth - dzCam) * invFadeSouth)
                                     : ((maxNorth + dzCam) * invFadeNorth);
        if (edgeZ < 0.0f) edgeZ = 0.0f;
        if (edgeZ > 1.0f) edgeZ = 1.0f;

        for (int32_t vx = 0; vx <= VOXEL_FOG_GRID; ++vx)
        {
            const float dxCam = fabsf(sFogNodeX[vx] - sCamera.targetX);
            float edgeX = (maxRx - dxCam) * invFadeX;
            if (edgeX < 0.0f) edgeX = 0.0f;
            if (edgeX > 1.0f) edgeX = 1.0f;
            float edge = edgeX * edgeZ;
            edge = edge * edge * (3.0f - 2.0f * edge);
            sFogNodeEdgeA[vz][vx] = (uint8_t)(edge * 255.0f + 0.5f);
        }
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sFogTex);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);

    const uint32_t bankCount = (banks == sSandBanks) ? 1 : 2;
    const float invPeriodX = 1.0f / VOXEL_FOG_PERIOD_X;
    const float invPeriodZ = 1.0f / VOXEL_FOG_PERIOD_Z;
    static const int32_t cx[6] = {0, 1, 1, 0, 1, 0};
    static const int32_t cz[6] = {0, 0, 1, 0, 1, 1};

    // u = (wx + anchor) / period - drift, with the anchor folded into the per-bank offset.
    const float anchorU = AtmoAnchorWrap(sAtmoAnchorX, (int32_t)VOXEL_FOG_PERIOD_X) * invPeriodX;
    const float anchorV = AtmoAnchorWrap(sAtmoAnchorZ, (int32_t)VOXEL_FOG_PERIOD_Z) * invPeriodZ;

    for (uint32_t bk = 0; bk < bankCount; ++bk)
    {
        const float uOff = (float)AtmoWrapD(clock * banks[bk].driftX * invPeriodX + banks[bk].offsetU, 1.0) - anchorU;
        const float vOff = (float)AtmoWrapD(clock * banks[bk].driftZ * invPeriodZ + banks[bk].offsetV, 1.0) - anchorV;
        const float alphaScale = (banks == sSandBanks ? 0.24f : banks[bk].alpha) * amount;
        const float hOff = banks[bk].height;
        uint32_t vOut = 0;

        for (int32_t gz = 0; gz < VOXEL_FOG_GRID; ++gz)
        {
            for (int32_t gx = 0; gx < VOXEL_FOG_GRID; ++gx)
            {
                if ((sFogNodeEdgeA[gz][gx] | sFogNodeEdgeA[gz][gx + 1] |
                     sFogNodeEdgeA[gz + 1][gx] | sFogNodeEdgeA[gz + 1][gx + 1]) == 0u)
                    continue;
                for (int32_t k = 0; k < 6; ++k)
                {
                    const int32_t vx = gx + cx[k];
                    const int32_t vz = gz + cz[k];
                    const float wx = sFogNodeX[vx];
                    const float wz = sFogNodeZ[vz];
                    VoxelGlVertex *const v = &sBatchGlVerts[vOut++];
                    v->x = wx;
                    v->y = sFogNodeY[vz][vx] + hOff;
                    v->z = wz;
                    v->u = wx * invPeriodX - uOff;
                    v->v = wz * invPeriodZ - vOff;
                    v->r = r;
                    v->g = g;
                    v->b = b;
                    v->a = (uint8_t)((float)sFogNodeEdgeA[vz][vx] * alphaScale + 0.5f);
                }
            }
        }
        if (vOut > 0)
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vOut);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}



static void DrawWeather(const WeatherKind *kind, float amount, float bolt, double clock, Hd2dPreset hd2d)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_WEATHER))
        return;
    if (!kind || amount <= 0.005f)
        return;

    const bool useHd2dAtlas = (hd2d != HD2D_PRESET_OFF && sWeatherAtlasTex != 0);
    const uint32_t maxDropCap = (useHd2dAtlas && kind == &sRainKind) ? 384u
                              : (useHd2dAtlas && kind == &sSandKind) ? 340u : 128u;
    uint32_t count = (uint32_t)(amount * (float)(useHd2dAtlas && (kind == &sRainKind || kind == &sSandKind) ? maxDropCap : kind->maxCount) + 0.5f);
    if (count < kind->minCount) count = kind->minCount;
    if (count > maxDropCap) count = maxDropCap;

    const float anchoredX = sCamera.targetX + AtmoAnchorWrap(sAtmoAnchorX, (int32_t)VOXEL_WEATHER_SPAN_X);
    const float anchoredZ = sCamera.targetZ + AtmoAnchorWrap(sAtmoAnchorZ, (int32_t)VOXEL_WEATHER_SPAN_Z);
    // Fast drops: the clock product is wrapped in double, so it keeps its fraction all session.
    const float offX = (float)AtmoWrapD((clock * kind->vx - anchoredX) / VOXEL_WEATHER_SPAN_X, 1.0);
    const float offY = (float)AtmoWrapD((clock * kind->vy - sCamera.ground) / VOXEL_WEATHER_SPAN_Y, 1.0);
    const float offZ = (float)AtmoWrapD((clock * kind->vz - anchoredZ) / VOXEL_WEATHER_SPAN_Z, 1.0);

    float baseX = sCamera.targetX - 0.5f * VOXEL_WEATHER_SPAN_X;
    float baseY = sCamera.ground + VOXEL_WEATHER_LOW;
    float baseZ = sCamera.targetZ - VOXEL_WEATHER_NORTH;

    float vx = kind->vx, vy = kind->vy, vz = kind->vz;
    float vLen = sqrtf(vx * vx + vy * vy + vz * vz);
    float alongX = 0.0f, alongY = -kind->length, alongZ = 0.0f;
    if (!kind->flake && vLen > 0.001f)
    {
        alongX = (vx / vLen) * kind->length;
        alongY = (vy / vLen) * kind->length;
        alongZ = (vz / vLen) * kind->length;
    }
    else if (kind->flake)
    {
        float pitchRad = sCamera.pitch * (3.14159265f / 180.0f);
        alongX = 0.0f;
        alongY = cosf(pitchRad) * kind->length;
        alongZ = sinf(pitchRad) * kind->length;
    }
    float acrossX = kind->width * (useHd2dAtlas ? 1.85f : 1.0f);

    const VitaDayNightLight *const cycle = VitaDayNight_Get();
    const VitaDayNightBlend moon = VitaDayNight_State()->moonlight;
    uint8_t baseR = (uint8_t)((kind->colour & 0xFFu) * (cycle->rgb[0] * (1.0f - moon.amount) + moon.rgb[0] * moon.amount));
    uint8_t baseG = (uint8_t)(((kind->colour >> 8) & 0xFFu) * (cycle->rgb[1] * (1.0f - moon.amount) + moon.rgb[1] * moon.amount));
    uint8_t baseB = (uint8_t)(((kind->colour >> 16) & 0xFFu) * (cycle->rgb[2] * (1.0f - moon.amount) + moon.rgb[2] * moon.amount));
    float baseAlpha = (kind->alpha + 0.16f * bolt) * (0.35f + 0.65f * amount);
    if (baseAlpha > 1.0f) baseAlpha = 1.0f;

    static const float rectCx[6]  = {-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f};
    static const float rectCy[6]  = {-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f};
    static const float flakeCx[6] = { 0.0f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f};
    static const float flakeCy[6] = {-1.0f,  0.0f,  1.0f, -1.0f,  1.0f,  0.0f};
    const float *cxTab = (kind->flake && !useHd2dAtlas) ? flakeCx : rectCx;
    const float *cyTab = (kind->flake && !useHd2dAtlas) ? flakeCy : rectCy;

    const float u0 = 0.0f, u1 = 0.5f;
    const float v0 = (kind == &sRainKind || kind == &sSandKind) ? 0.0f : 0.5f;
    const float v1 = (kind == &sRainKind || kind == &sSandKind) ? 0.5f : 1.0f;
    const float scaleFadeX = VOXEL_WEATHER_SPAN_X / VOXEL_WEATHER_FADE_XZ;
    const float scaleFadeZ = VOXEL_WEATHER_SPAN_Z / VOXEL_WEATHER_FADE_XZ;
    const float scaleFadeY = VOXEL_WEATHER_SPAN_Y / VOXEL_WEATHER_FADE_Y;
    const float nearStartSq = kind->nearStart * kind->nearStart;
    const float nearEnd = kind->nearStart + kind->nearRamp;
    const float nearEndSq = nearEnd * nearEnd;
    const float invNearSpanSq = (nearEndSq > nearStartSq) ? (1.0f / (nearEndSq - nearStartSq)) : 1.0f;

    float offVx[6], offVy[6], offVz[6], tabU[6], tabV[6];
    for (int32_t k = 0; k < 6; ++k)
    {
        offVx[k] = acrossX * cxTab[k] + alongX * cyTab[k];
        offVy[k] = alongY * cyTab[k];
        offVz[k] = alongZ * cyTab[k];
        tabU[k] = (cxTab[k] > 0.0f) ? u1 : u0;
        tabV[k] = (cyTab[k] > 0.0f) ? v1 : v0;
    }

    uint32_t vOut = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        const uint32_t si = i % VOXEL_WEATHER_COUNT;
        const float jitter = (float)(i / VOXEL_WEATHER_COUNT) * 0.37f;
        float px = sWeatherSeeds[si][0] + jitter + offX; px -= floorf(px);
        float py = sWeatherSeeds[si][1] + jitter * 0.53f + offY; py -= floorf(py);
        float pz = sWeatherSeeds[si][2] + jitter * 0.79f + offZ; pz -= floorf(pz);

        float dx = 0.5f - fabsf(px - 0.5f);
        float dy = 0.5f - fabsf(py - 0.5f);
        float dz = 0.5f - fabsf(pz - 0.5f);
        float fadeX = dx * scaleFadeX;
        float fadeZ = dz * scaleFadeZ;
        float fadeY = dy * scaleFadeY;
        float fade = fadeX < fadeZ ? fadeX : fadeZ;
        if (fadeY < fade) fade = fadeY;
        if (fade > 1.0f) fade = 1.0f;
        if (fade <= 0.01f) continue;

        float wx = baseX + px * VOXEL_WEATHER_SPAN_X;
        float wy = baseY + py * VOXEL_WEATHER_SPAN_Y;
        float wz = baseZ + pz * VOXEL_WEATHER_SPAN_Z;

        float camDx = wx - sCamera.x;
        float camDy = wy - sCamera.y;
        float camDz = wz - sCamera.z;
        float distSq = camDx * camDx + camDy * camDy + camDz * camDz;
        if (distSq <= nearStartSq) continue;
        float lens = 1.0f;
        if (distSq < nearEndSq)
        {
            lens = (distSq - nearStartSq) * invNearSpanSq;
            if (lens <= 0.01f) continue;
        }

        uint8_t a = (uint8_t)(baseAlpha * fade * lens * 255.0f + 0.5f);
        if (a == 0) continue;

        for (int32_t k = 0; k < 6; ++k)
        {
            VoxelGlVertex *v = &sBatchGlVerts[vOut++];
            v->x = wx + offVx[k];
            v->y = wy + offVy[k];
            v->z = wz + offVz[k];
            v->u = tabU[k];
            v->v = tabV[k];
            v->r = baseR;
            v->g = baseG;
            v->b = baseB;
            v->a = a;
        }
    }

    if (vOut > 0)
    {
        if (useHd2dAtlas)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, sWeatherAtlasTex);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        if (useHd2dAtlas)
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, vOut);
        glDisableClientState(GL_COLOR_ARRAY);
        if (useHd2dAtlas)
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
    }
}



static void DrawMotes(float strength, float seconds, bool fireflies, Hd2dPreset hd2d)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_MOTES))
        return;
    if (strength <= 0.005f)
        return;

    const bool useHd2dAtlas = (hd2d != HD2D_PRESET_OFF && sWeatherAtlasTex != 0);
    const float radius = useHd2dAtlas ? (VOXEL_MOTE_RADIUS * (fireflies ? 2.1f : 1.7f)) : VOXEL_MOTE_RADIUS;
    float pitchRad = sCamera.pitch * (3.14159265f / 180.0f);
    float upY = cosf(pitchRad) * radius;
    float upZ = sinf(pitchRad) * radius;

    static const float dCx[6] = { 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f};
    static const float dCy[6] = {-1.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f};
    static const float qCx[6] = {-1.0f,  1.0f, 1.0f, -1.0f, 1.0f, -1.0f};
    static const float qCy[6] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f,  1.0f};
    const float *cxTab = useHd2dAtlas ? qCx : dCx;
    const float *cyTab = useHd2dAtlas ? qCy : dCy;

    float ox = sCamera.targetX - 0.5f * VOXEL_MOTE_SPAN_X;
    float oz = sCamera.targetZ - VOXEL_MOTE_SPAN_Z + 4.0f;
    // The wrap runs on anchored coordinates; ox/oz place the result back in map coordinates.
    const float anchoredX = sCamera.targetX + AtmoAnchorWrap(sAtmoAnchorX, (int32_t)VOXEL_MOTE_SPAN_X);
    const float anchoredZ = sCamera.targetZ + AtmoAnchorWrap(sAtmoAnchorZ, (int32_t)VOXEL_MOTE_SPAN_Z);

    uint32_t vOut = 0;
    const uint8_t cr = fireflies ? 210u : 255u;
    const uint8_t cg = fireflies ? 255u : 238u;
    const uint8_t cb = fireflies ? 130u : 185u;

    const uint32_t moteTotal = (hd2d != HD2D_PRESET_OFF) ? ((VOXEL_MOTE_COUNT * 3u) / 2u) : VOXEL_MOTE_COUNT;
    for (uint32_t i = 0; i < moteTotal; ++i)
    {
        const MoteSpec *m = &sMoteSpecs[i % VOXEL_MOTE_COUNT];
        const float jx = (float)(i / VOXEL_MOTE_COUNT) * 7.3f;
        const float jz = (float)(i / VOXEL_MOTE_COUNT) * 5.9f;
        const bool isLeaf = (useHd2dAtlas && !fireflies && (i & 3u) == 3u);
        float glint = 0.5f + 0.5f * sinf(seconds * m->glintSpeed + m->glintPhase + (float)i);
        glint *= glint;
        const float relX = m->x + jx + seconds * (isLeaf ? m->driftX * 2.2f : m->driftX) - anchoredX;
        const float relZ = m->z + jz + seconds * (isLeaf ? m->driftZ * 1.8f : m->driftZ) - anchoredZ;
        const float lx = relX - floorf(relX / VOXEL_MOTE_SPAN_X) * VOXEL_MOTE_SPAN_X;
        const float lz = relZ - floorf(relZ / VOXEL_MOTE_SPAN_Z) * VOXEL_MOTE_SPAN_Z;
        const float edgeX = fminf(lx, VOXEL_MOTE_SPAN_X - lx) * 0.4f;
        const float edgeZ = fminf(lz, VOXEL_MOTE_SPAN_Z - lz) * 0.4f;
        const float edgeFade = fminf(1.0f, fminf(edgeX, edgeZ));
        if (edgeFade <= 0.02f) continue;
        const float baseA = (fireflies ? 0.72f : (isLeaf ? 0.78f : VOXEL_MOTE_ALPHA)) * edgeFade;
        uint8_t a = (uint8_t)(baseA * strength * (0.25f + 0.75f * glint) * 255.0f + 0.5f);
        if (a == 0) continue;

        float wx = ox + lx;
        float wz = oz + lz;
        float wy = SurfaceWorldY(wx, wz) + m->y + m->bobAmp * sinf(seconds * m->bobSpeed + m->bobPhase);

        const float lr = isLeaf ? (radius * 1.45f) : radius;
        const float lUpY = isLeaf ? (upY * 1.45f) : upY;
        const float lUpZ = isLeaf ? (upZ * 1.45f) : upZ;
        const float u0 = isLeaf ? 0.5f : 0.0f;
        const float u1 = isLeaf ? 1.0f : 0.5f;
        const uint8_t pr = isLeaf ? 140u : cr;
        const uint8_t pg = isLeaf ? 220u : cg;
        const uint8_t pb = isLeaf ? 110u : cb;

        for (int32_t k = 0; k < 6; ++k)
        {
            VoxelGlVertex *v = &sBatchGlVerts[vOut++];
            v->x = wx + lr * cxTab[k];
            v->y = wy + lUpY * cyTab[k];
            v->z = wz + lUpZ * cyTab[k];
            v->u = (cxTab[k] > 0.0f) ? u1 : u0;
            v->v = (cyTab[k] > 0.0f) ? 1.0f : 0.5f;
            v->r = pr;
            v->g = pg;
            v->b = pb;
            v->a = a;
        }
    }

    if (vOut > 0)
    {
        if (useHd2dAtlas)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, sWeatherAtlasTex);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        if (useHd2dAtlas)
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, vOut);
        glDisableClientState(GL_COLOR_ARRAY);
        if (useHd2dAtlas)
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
    }
}



static inline void PushFlatVertex2D(uint32_t *const vOut, const float x, const float y,
                                    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
    VoxelGlVertex *const v = &sBatchGlVerts[(*vOut)++];
    v->x = x;
    v->y = y;
    v->z = 0.0f;
    v->u = 0.0f;
    v->v = 0.0f;
    v->r = r;
    v->g = g;
    v->b = b;
    v->a = a;
}

static inline void PushFlatVertex3D(uint32_t *const vOut, const float x, const float y, const float z,
                                    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
    VoxelGlVertex *const v = &sBatchGlVerts[(*vOut)++];
    v->x = x;
    v->y = y;
    v->z = z;
    v->u = 0.0f;
    v->v = 0.0f;
    v->r = r;
    v->g = g;
    v->b = b;
    v->a = a;
}

// Mouse look / orbit
void VoxelRenderer_HandleMouseMotion(int32_t dx, int32_t dy)
{
    (void)dx; (void)dy;
}

// Pre-decode all 3 original GBA flower frames (0, 1, 2) and pack Frame0 into free slot (0, 112) of sTreeTex.
static void InitFlowerFrames(uint16_t *const treeLinear)
{
    extern const u16 gTilesetAnims_General_Flower_Frame0[];
    extern const u16 gTilesetAnims_General_Flower_Frame1[];
    extern const u16 gTilesetAnims_General_Flower_Frame2[];
    static const uint16_t kFlowerPal02[16] = {
        0x0000u,
        (22u << 0) | (31u << 5) | (16u << 10),
        (16u << 0) | (24u << 5) | (12u << 10),
        (7u  << 0) | (17u << 5) | (6u  << 10),
        (7u  << 0) | (10u << 5) | (0u  << 10),
        (27u << 0) | (18u << 5) | (14u << 10),
        (13u << 0) | (11u << 5) | (11u << 10),
        (20u << 0) | (12u << 5) | (11u << 10),
        (8u  << 0) | (7u  << 5) | (6u  << 10),
        (31u << 0) | (24u << 5) | (18u << 10),
        (27u << 0) | (13u << 5) | (12u << 10),
        (25u << 0) | (8u  << 5) | (10u << 10),
        (20u << 0) | (26u << 5) | (24u << 10),
        0x0000u, 0x0000u, 0x0000u
    };
    const u16 *const frames[3] = {
        gTilesetAnims_General_Flower_Frame0,
        gTilesetAnims_General_Flower_Frame1,
        gTilesetAnims_General_Flower_Frame2,
    };

    for (int32_t fi = 0; fi < 3; ++fi)
    {
        const uint8_t *const tiles4bpp = (const uint8_t *)frames[fi];
        for (int32_t ty = 0; ty < 2; ++ty)
        {
            for (int32_t tx = 0; tx < 2; ++tx)
            {
                const uint8_t *const tile = tiles4bpp + (ty * 2 + tx) * 32;
                for (int32_t py = 0; py < 8; ++py)
                {
                    const int32_t localGlY = 15 - (ty * 8 + py);
                    for (int32_t px = 0; px < 8; ++px)
                    {
                        const uint8_t byte = tile[py * 4 + (px >> 1)];
                        const uint8_t idx = (px & 1) ? (byte >> 4) : (byte & 0x0Fu);
                        uint16_t col = 0u;
                        if (idx > 0u && idx < 13u)
                            col = VoxelGrade_RGBA5551(kFlowerPal02[idx]);
                        sFlowerFrames[fi][localGlY * 16 + (tx * 8 + px)] = col;
                    }
                }
            }
        }
    }
    sFlowerFramesReady = true;
    for (int32_t localGlY = 0; localGlY < 16; ++localGlY)
    {
        const int32_t glY = (int32_t)VOXEL_TREE_TEXTURE_DIM - 1 - (112 + (15 - localGlY));
        memcpy(&treeLinear[glY * VOXEL_TREE_TEXTURE_DIM], &sFlowerFrames[0][localGlY * 16], 16u * sizeof(uint16_t));
    }
}

// Pack original GBA ash-covered tall grass sprite (gTileset_Fallarbor MB_ASHGRASS) into free slot (16, 112).
static void InitAshGrassTexel(uint16_t *const treeLinear)
{
    extern const struct Tileset gTileset_Fallarbor;
    bool packedAsh = false;

    if (gTileset_Fallarbor.tiles != NULL && gTileset_Fallarbor.metatiles != NULL
        && gTileset_Fallarbor.metatileAttributes != NULL && gTileset_Fallarbor.palettes != NULL)
    {
        int32_t ashMt = -1;
        for (int32_t m = 0; m < (int32_t)(NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY); ++m)
        {
            if ((gTileset_Fallarbor.metatileAttributes[m] & METATILE_ATTR_BEHAVIOR_MASK) == MB_ASHGRASS)
            {
                ashMt = m;
                break;
            }
        }
        uint8_t *const secTiles = (ashMt >= 0) ? (uint8_t *)malloc(16384u) : NULL;
        if (secTiles != NULL)
        {
            if (gTileset_Fallarbor.isCompressed)
                LZDecompressWram(gTileset_Fallarbor.tiles, secTiles);
            else
                memcpy(secTiles, gTileset_Fallarbor.tiles, 16384u);

            const u16 *const mt = gTileset_Fallarbor.metatiles + (uint32_t)ashMt * 8u;
            const u16 *const pals = (const u16 *)gTileset_Fallarbor.palettes;
            for (int32_t ty = 0; ty < 2; ++ty)
            {
                for (int32_t tx = 0; tx < 2; ++tx)
                {
                    const u16 entry = mt[4 + ty * 2 + tx];
                    const uint32_t tileIdx = entry & 0x3FFu;
                    const bool hFlip = ((entry >> 10) & 1u) != 0u;
                    const bool vFlip = ((entry >> 11) & 1u) != 0u;
                    const uint32_t palIdx = (entry >> 12) & 0x0Fu;
                    const uint8_t *const tile = (tileIdx >= 512u) ? (secTiles + (tileIdx - 512u) * 32u) : NULL;
                    const u16 *const pal = pals + palIdx * 16u;
                    for (int32_t py = 0; py < 8; ++py)
                    {
                        const int32_t sy = ty * 8 + py;
                        const int32_t dstGlY = (int32_t)VOXEL_TREE_TEXTURE_DIM - 1 - (112 + sy);
                        const int32_t srcGlY = (int32_t)VOXEL_TREE_TEXTURE_DIM - 1 - (32 + sy);
                        const int32_t fy = vFlip ? (7 - py) : py;
                        for (int32_t px = 0; px < 8; ++px)
                        {
                            const int32_t sx = tx * 8 + px;
                            const uint16_t maskTexel = treeLinear[srcGlY * VOXEL_TREE_TEXTURE_DIM + (80 + sx)];
                            uint16_t col = 0u;
                            if ((maskTexel & 1u) != 0u && tile != NULL)
                            {
                                const int32_t fx = hFlip ? (7 - px) : px;
                                const uint8_t byte = tile[fy * 4 + (fx >> 1)];
                                const uint8_t idx = (fx & 1) ? (byte >> 4) : (byte & 0x0Fu);
                                if (idx > 0u)
                                {
                                    col = VoxelGrade_RGBA5551(pal[idx]);
                                    packedAsh = true;
                                }
                            }
                            treeLinear[dstGlY * VOXEL_TREE_TEXTURE_DIM + (16 + sx)] = col;
                        }
                    }
                }
            }
            free(secTiles);
        }
    }
    if (!packedAsh)
    {
        for (int32_t sy = 0; sy < 16; ++sy)
        {
            const int32_t dstGlY = (int32_t)VOXEL_TREE_TEXTURE_DIM - 1 - (112 + sy);
            const int32_t srcGlY = (int32_t)VOXEL_TREE_TEXTURE_DIM - 1 - (32 + sy);
            for (int32_t sx = 0; sx < 16; ++sx)
            {
                const uint16_t t = treeLinear[srcGlY * VOXEL_TREE_TEXTURE_DIM + (80 + sx)];
                uint16_t outCol = 0u;
                if ((t & 1u) != 0u)
                {
                    const uint32_t r = ((t >> 11) & 31u);
                    const uint32_t g = ((t >> 6) & 31u);
                    const uint32_t b = ((t >> 1) & 31u);
                    const uint32_t lum = (r * 3u + g * 6u + b) / 10u;
                    const uint32_t ar = (r + lum * 2u + 14u) / 3u;
                    const uint32_t ag = (g + lum * 2u + 15u) / 3u;
                    const uint32_t ab = (b + lum * 2u + 16u) / 3u;
                    outCol = (uint16_t)(((ar > 31u ? 31u : ar) << 11)
                                      | ((ag > 31u ? 31u : ag) << 6)
                                      | ((ab > 31u ? 31u : ab) << 1) | 1u);
                }
                treeLinear[dstGlY * VOXEL_TREE_TEXTURE_DIM + (16 + sx)] = outCol;
            }
        }
    }
}

// Compile foliage wind-sway and player-bend shader for Pass_Trees (falls back to fixed-function if unavailable).
static void InitTreeSwayShader(void)
{
    static const char *const kTreeSwayVs =
        "attribute vec3 aPos;\n"
        "attribute vec2 aUv;\n"
        "attribute vec4 aCol;\n"
        "uniform mat4 uMvp;\n"
        "uniform vec3 uChunkDelta;\n"
        "uniform vec4 uSwayA;\n"
        "uniform float uSwayB;\n"
        "uniform vec2 uWind;\n"
        "uniform vec2 uPlayerXZ;\n"
        "varying vec2 vUv;\n"
        "varying vec3 vCol;\n"
        "void main() {\n"
        "    float w = clamp((255.0 - floor(aCol.a * 255.0 + 0.5)) / 190.0, 0.0, 1.0);\n"
        "    vec3 p = aPos + uChunkDelta;\n"
        "    if (w > 0.001) {\n"
        "        float ph = dot(p.xz, vec2(0.37, 0.61));\n"
        "        float s = (sin(uSwayA.x + ph) + 0.5 * sin(uSwayA.y + ph * 0.5 + 1.3)) / 1.5;\n"
        "        float gust = 0.7 + 0.3 * sin(uSwayA.z + dot(p.xz, vec2(0.08, 0.05)));\n"
        "        float d = w * 0.14 * gust * (0.35 + 0.65 * s);\n"
        "        p.x += uWind.x * d;\n"
        "        p.z += uWind.y * d;\n"
        "        float w2 = w * w;\n"
        "        float s2 = sin(uSwayA.w + ph * 1.9 + 0.7) * 0.6 + sin(uSwayB + ph * 0.8) * 0.4;\n"
        "        p.x += -uWind.y * (w2 * 0.06 * gust * s2);\n"
        "        p.z +=  uWind.x * (w2 * 0.06 * gust * s2);\n"
        "        p.y -= w2 * 0.02 * (0.5 + 0.5 * s);\n"
        "        if (w > 0.5) {\n"
        "            vec2 dv = p.xz - uPlayerXZ;\n"
        "            float dist = length(dv);\n"
        "            float k = w * clamp(1.0 - dist / 1.20, 0.0, 1.0);\n"
        "            k *= k;\n"
        "            vec2 pushDir = (dist > 0.001) ? (dv / dist) : vec2(0.0, 1.0);\n"
        "            p.xz += pushDir * (0.72 * k);\n"
        "            p.y -= 0.28 * k;\n"
        "        }\n"
        "    }\n"
        "    gl_Position = uMvp * vec4(p, 1.0);\n"
        "    vUv = aUv;\n"
        "    vCol = aCol.rgb;\n"
        "}\n";
    static const char *const kTreeSwayFs =
        "varying vec2 vUv;\n"
        "varying vec3 vCol;\n"
        "uniform sampler2D uTex;\n"
        "void main() {\n"
        "    vec4 t = texture2D(uTex, vUv);\n"
        "    if (t.a <= 0.05) discard;\n"
        "    gl_FragColor = vec4(t.rgb * vCol, t.a);\n"
        "}\n";

    const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char *vsSrc = kTreeSwayVs;
    const char *fsSrc = kTreeSwayFs;
    glShaderSource(vs, 1, &vsSrc, NULL);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fsSrc, NULL);
    glCompileShader(fs);

    GLint vsOk = 0, fsOk = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &vsOk);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &fsOk);
    if (vsOk && fsOk)
    {
        const GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glBindAttribLocation(prog, 0, "aPos");
        glBindAttribLocation(prog, 1, "aUv");
        glBindAttribLocation(prog, 2, "aCol");
        glLinkProgram(prog);

        GLint linkOk = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &linkOk);
        if (linkOk)
        {
            sTreeShaderProg = prog;
            sTreeShaderUMvp = glGetUniformLocation(prog, "uMvp");
            sTreeShaderUChunkDelta = glGetUniformLocation(prog, "uChunkDelta");
            sTreeShaderUSwayA = glGetUniformLocation(prog, "uSwayA");
            sTreeShaderUSwayB = glGetUniformLocation(prog, "uSwayB");
            sTreeShaderUWind = glGetUniformLocation(prog, "uWind");
            sTreeShaderUPlayerXZ = glGetUniformLocation(prog, "uPlayerXZ");
            sTreeShaderUTex = glGetUniformLocation(prog, "uTex");
            sTreeShaderReady = true;
            VITA_LOG("[VOXEL] Compiled foliage sway shader successfully.\n");
        }
        else
        {
            glDeleteProgram(prog);
            VITA_LOG("[VOXEL] Foliage sway shader link failed; using fixed-function.\n");
        }
    }
    else
    {
        VITA_LOG("[VOXEL] Foliage sway shader compile failed (vs=%d fs=%d); using fixed-function.\n", vsOk, fsOk);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

static GLuint CreatePostColorTexture(const GLsizei width, const GLsizei height,
                                     const GLenum filter, const GLenum type)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0)
        return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, type, NULL);
    return tex;
}

static void AttachColorFbo(GLuint *const fboOut, const GLuint tex)
{
    glGenFramebuffers(1, fboOut);
    if (*fboOut == 0)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, *fboOut);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
}

static bool InitPostTargets(PostRenderTargets *const t)
{
    t->dioramaColorTex = CreatePostColorTexture(VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_TEX_H, GL_NEAREST, GL_UNSIGNED_BYTE);
    t->dioramaColorTexFast = CreatePostColorTexture(VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_TEX_H, GL_NEAREST, GL_UNSIGNED_SHORT_5_5_5_1);
    glGenRenderbuffers(1, &t->dioramaDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, t->dioramaDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_TEX_H);

    t->dioramaActiveTex = t->dioramaColorTex;
    AttachColorFbo(&t->dioramaFbo, t->dioramaActiveTex);
    if (t->dioramaFbo != 0)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t->dioramaDepthRb);

    t->scratchColorTex = CreatePostColorTexture(VOXEL_SCRATCH_FBO_W, VOXEL_SCRATCH_FBO_H, GL_LINEAR, GL_UNSIGNED_BYTE);
    AttachColorFbo(&t->scratchFbo, t->scratchColorTex);

    t->blurTex = CreatePostColorTexture(VOXEL_SCRATCH_FBO_W, VOXEL_SCRATCH_FBO_H, GL_LINEAR, GL_UNSIGNED_BYTE);
    AttachColorFbo(&t->blurFbo, t->blurTex);

    t->bloomTexA = CreatePostColorTexture(VOXEL_BLOOM_FBO_W, VOXEL_BLOOM_FBO_H, GL_LINEAR, GL_UNSIGNED_BYTE);
    AttachColorFbo(&t->bloomFboA, t->bloomTexA);

    t->bloomTexB = CreatePostColorTexture(VOXEL_BLOOM_FBO_W, VOXEL_BLOOM_FBO_H, GL_LINEAR, GL_UNSIGNED_BYTE);
    AttachColorFbo(&t->bloomFboB, t->bloomTexB);

    // Optional: without it HIGH keeps the separate bloom and shaft targets.
    t->bandTex = CreatePostColorTexture(VOXEL_SCRATCH_FBO_W, VOXEL_BAND_TEX_H, GL_LINEAR, GL_UNSIGNED_BYTE);
    t->bandFbo = 0;
    if (t->bandTex != 0)
        AttachColorFbo(&t->bandFbo, t->bandTex);
    if (t->bandFbo == 0)
        VITA_LOG("[VOXEL] Post band target not created, HIGH keeps its separate bloom target\n");

    // Without these targets every post effect (bloom, sky bloom, shafts, grade, DoF) is off for the
    // whole run: say which one failed.
    if (t->dioramaFbo == 0 || t->scratchFbo == 0 || t->blurFbo == 0
     || t->bloomFboA == 0 || t->bloomFboB == 0)
    {
        VITA_LOG("[VOXEL ERROR] Post targets: diorama fbo=%u tex=%u fast=%u depth=%u, scratch fbo=%u tex=%u, "
                 "blur fbo=%u tex=%u, bloom fbo=%u/%u tex=%u/%u\n",
                 (unsigned)t->dioramaFbo, (unsigned)t->dioramaColorTex, (unsigned)t->dioramaColorTexFast,
                 (unsigned)t->dioramaDepthRb, (unsigned)t->scratchFbo, (unsigned)t->scratchColorTex,
                 (unsigned)t->blurFbo, (unsigned)t->blurTex, (unsigned)t->bloomFboA, (unsigned)t->bloomFboB,
                 (unsigned)t->bloomTexA, (unsigned)t->bloomTexB);
    }
    return (t->dioramaFbo != 0 && t->scratchFbo != 0 && t->blurFbo != 0);
}

// Initialize the voxel renderer. Call after OpenGL context is created.
// (1 - d^2)^2: a lamp's pool of light, d the distance from the centre in radii.
static float PoolFalloff(const float d2)
{
    const float inv = 1.0f - d2;
    return inv * inv;
}

// A bright core fading out softly: the halo round a lamp's glass.
static float HaloFalloff(const float d2)
{
    const float inv = 1.0f - d2;
    return inv * inv * expf(-2.5f * d2);
}

// A 64x64 RGBA8 radial texture, profile(d^2) at each texel and 0 past the edge (d = 1): into RGB
// (alpha 1), or into alpha (white RGB).
static GLuint CreateRadialTexture(float (*const profile)(float), const bool inAlpha)
{
    enum { DIM = 64 };
    static uint32_t pixels[DIM * DIM];
    for (int32_t y = 0; y < DIM; y++)
    {
        const float dy = ((float)y + 0.5f) / (DIM / 2) - 1.0f;
        for (int32_t x = 0; x < DIM; x++)
        {
            const float dx = ((float)x + 0.5f) / (DIM / 2) - 1.0f;
            const float d2 = dx * dx + dy * dy;
            const float value = d2 < 1.0f ? Voxel_ClampF(profile(d2), 0.0f, 1.0f) : 0.0f;
            const uint32_t level = (uint32_t)(value * 255.0f + 0.5f);
            pixels[y * DIM + x] = inAlpha ? ((level << 24) | 0x00FFFFFFu)
                                          : (0xFF000000u | (level << 16) | (level << 8) | level);
        }
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DIM, DIM, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return tex;
}

bool VoxelRenderer_Init(void)
{
    VITA_LOG("[VOXEL] Initializing Emerald-3DS architecture on PS Vita (vitaGL)...\n");

    if (!EnsureChunkPool())
    {
        VITA_LOG("[VOXEL ERROR] Failed to allocate chunk vertex pool!\n");
        return false;
    }
    ChunkWorker_Start();

    // Load precomputed Hoenn binary datasets
    VoxelGrade_Init();
    if (!VoxelBuildings_Init())
    {
        VITA_LOG("[VOXEL ERROR] Failed to load buildings.bin! Buildings will not render in 3D.\n");
    }
    VoxelRegions_Init();
    VoxelRelief_Init();
    VoxelSign_Init();

    // Load 128x128 tree texture atlas
    FILE *treeFile = VoxelFile_Open(VOXEL_TREE_TEXTURE_PATH);
    if (treeFile)
    {
        size_t readCount = fread(sTreeMorton, 2, VOXEL_TREE_TEXTURE_DIM * VOXEL_TREE_TEXTURE_DIM, treeFile);
        fclose(treeFile);
        if (readCount == VOXEL_TREE_TEXTURE_DIM * VOXEL_TREE_TEXTURE_DIM)
        {
            VoxelGrade_Texels(sTreeMorton, VOXEL_TREE_TEXTURE_DIM * VOXEL_TREE_TEXTURE_DIM);
            BrightenCrowns(sTreeMorton);
            Voxel_UnswizzleMortonToLinear(sTreeMorton, sTreeLinear,
                                          VOXEL_TREE_TEXTURE_DIM, VOXEL_TREE_TEXTURE_DIM);
            InitFlowerFrames(sTreeLinear);
            InitAshGrassTexel(sTreeLinear);
            VoxelTree_SetAtlasPixels(sTreeLinear);
            // Pre-bake 3 tree atlas textures (one per flower animation frame) so Pass_Trees
            // only switches texture bind without ever calling glTexSubImage2D on an active texture.
            glGenTextures(3, sTreeTex);
            for (uint32_t ff = 0; ff < 3u; ++ff)
            {
                if (sFlowerFramesReady)
                {
                    for (uint32_t y = 0; y < 16u; ++y)
                        memcpy(&sTreeLinear[y * VOXEL_TREE_TEXTURE_DIM], &sFlowerFrames[ff][y * 16u], 16u * sizeof(uint16_t));
                }
                glBindTexture(GL_TEXTURE_2D, sTreeTex[ff]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_TREE_TEXTURE_DIM, VOXEL_TREE_TEXTURE_DIM,
                             0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, sTreeLinear);
            }
            if (sFlowerFramesReady)
            {
                for (uint32_t y = 0; y < 16u; ++y)
                    memcpy(&sTreeLinear[y * VOXEL_TREE_TEXTURE_DIM], &sFlowerFrames[0][y * 16u], 16u * sizeof(uint16_t));
            }
            VITA_LOG("[VOXEL] Loaded trees.rgba5551 successfully (3 flower frames pre-baked).\n");

            InitTreeSwayShader();
        }
    }
    else
    {
        VITA_LOG("[VOXEL WARNING] Could not open %s\n", VOXEL_TREE_TEXTURE_PATH);
    }

    // Initialize Multi-Atlas Slots
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        sAtlasSlots[s].valid = false;
        sAtlasSlots[s].primaryTileset = NULL;
        sAtlasSlots[s].secondaryTileset = NULL;
        sAtlasSlots[s].stamp = 0;
        sAtlasSlots[s].generation = 0;
        sAtlasSlots[s].extension = 0;
        sAtlasSlots[s].pageCount = 0;
        for (int32_t p = 0; p < VOXEL_MAX_ATLAS_PAGES; p++)
            sAtlasSlots[s].tex[p] = 0;
    }
    sActiveAtlasSlot = -1;
    AtlasExtend_Cancel();
    memset(sSlotPendingPrimary, 0, sizeof(sSlotPendingPrimary));

    for (int32_t p = 0; p < VOXEL_MAX_ATLAS_PAGES; p++)
        sAtlasTex[p] = 0;
    sAtlasPageCount = 0;

    for (int32_t i = 0; i < VOXEL_BUILDING_CACHE_SLOTS; i++)
    {
        sBuildingCache[i].page = -1;
        sBuildingCache[i].tex = 0;
        sBuildingCache[i].glowTex = 0;
        sBuildingCache[i].lastUsedFrame = 0;
    }
    memset(sBuildingPageToSlot, -1, sizeof(sBuildingPageToSlot));
    memset(sBuildingPageHasAlpha, 0, sizeof(sBuildingPageHasAlpha));
    memset(sBuildingPageQueued, 0, sizeof(sBuildingPageQueued));
    sBuildingReqHead = 0;
    sBuildingReqTail = 0;
    sBuildingReqCount = 0;
    sWorkerReadyPage = -1;
    sWorkerReadyHasGlow = false;
    SDL_AtomicSet(&sBuildingIoStop, 0);
    sBuildingIoMutex = SDL_CreateMutex();
    sBuildingReqSem = SDL_CreateSemaphore(0);
    sBuildingConsumedSem = SDL_CreateSemaphore(0);
    if (sBuildingIoMutex && sBuildingReqSem && sBuildingConsumedSem)
    {
        sBuildingIoThread = SDL_CreateThreadWithStackSize(VoxelBuildingIoWorkerMain, "VoxelIoWorker", 256 * 1024, NULL);
    }

    sSpriteTex = CreateRgba5551Texture(VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_ATLAS_H, NULL);
    sSpriteAtlasNeedsAll = true;

    // Pre-allocate 2D UI overlay texture ring at maximum dimensions once
    glGenTextures((GLsizei)VOXEL_UI_TEX_RING, sUiTexRing);
    sUiImageValid = false;
    for (uint32_t t = 0; t < VOXEL_UI_TEX_RING; t++)
    {
        glBindTexture(GL_TEXTURE_2D, sUiTexRing[t]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_LINE_WIDTH, DISPLAY_HEIGHT,
                     0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, NULL);
    }
    sUiTexIdx = 0;
    sUiTex = sUiTexRing[0];

    // Radial textures (64x64 RGBA8, 16 KB each): the pool falloff in RGB (night lamp pools, water
    // glints, 3D battle ovals; 256 levels, it was 16x16 RGBA5551 and drawn in rings) and the night
    // lamps' halo in alpha.
    sLampPoolTex = CreateRadialTexture(PoolFalloff, false);
    sLampHaloTex = CreateRadialTexture(HaloFalloff, true);

    // Generate 16x16 RGBA4444 concentric ring texture (512 B) for rain ripples on water/puddles
    {
        uint16_t ripplePixels[16 * 16];
        for (int32_t y = 0; y < 16; y++)
        {
            const float dy = ((float)y + 0.5f - 8.0f) / 7.5f;
            for (int32_t x = 0; x < 16; x++)
            {
                const float dx = ((float)x + 0.5f - 8.0f) / 7.5f;
                const float d = sqrtf(dx * dx + dy * dy);
                const float ring = expf(-((d - 0.68f) * (d - 0.68f)) / (2.0f * 0.13f * 0.13f));
                const uint16_t a4 = (d < 1.0f) ? (uint16_t)(ring * 15.0f + 0.5f) : 0u;
                ripplePixels[y * 16 + x] = (uint16_t)(0xFFF0u | a4);
            }
        }
        glGenTextures(1, &sRippleTex);
        glBindTexture(GL_TEXTURE_2D, sRippleTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, ripplePixels);
    }

    // Generate 16x16 RGBA4444 soft radial alpha texture (512 B) for water/ground mist (zero dark square edges)
    {
        uint16_t mistPixels[16 * 16];
        for (int32_t y = 0; y < 16; y++)
        {
            const float dy = ((float)y + 0.5f - 8.0f) / 7.5f;
            for (int32_t x = 0; x < 16; x++)
            {
                const float dx = ((float)x + 0.5f - 8.0f) / 7.5f;
                const float d2 = dx * dx + dy * dy;
                float falloff = 0.0f;
                if (d2 < 1.0f)
                {
                    const float inv = 1.0f - d2;
                    falloff = inv * inv;
                }
                const uint16_t a4 = (uint16_t)(falloff * 15.0f + 0.5f);
                mistPixels[y * 16 + x] = (uint16_t)(0xFFF0u | a4);
            }
        }
        glGenTextures(1, &sSoftMistTex);
        glBindTexture(GL_TEXTURE_2D, sSoftMistTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, mistPixels);
    }

    // Pre-allocate double-buffered offscreen post-processing targets (960x544, 480x272, 240x136)
    bool allPostReady = true;
    for (uint32_t i = 0; i < VOXEL_POST_TARGET_BUFFERS; ++i)
    {
        if (!InitPostTargets(&sPostTargets[i]))
            allPostReady = false;
    }

    // 64x64 RGBA8 radial vignette texture (16 KB)
    {
        uint32_t vigPixels[64 * 64];
        for (int32_t y = 0; y < 64; ++y)
        {
            const float dy = ((float)y + 0.5f - 32.0f) / 32.0f;
            for (int32_t x = 0; x < 64; ++x)
            {
                const float dx = ((float)x + 0.5f - 32.0f) / 32.0f;
                const float r2 = dx * dx + dy * dy;
                float t = (r2 - 0.35f) / 1.45f;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                t = t * t * (3.0f - 2.0f * t);
                const uint32_t v = (uint32_t)((1.0f - 0.42f * t) * 255.0f + 0.5f);
                vigPixels[y * 64 + x] = 0xFF000000u | (v << 16) | (v << 8) | v;
            }
        }
        glGenTextures(1, &sVignetteTex);
        glBindTexture(GL_TEXTURE_2D, sVignetteTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, vigPixels);
    }

    // 64x64 RGBA8 2x2 weather/particle atlas (16 KB):
    // [0,0]=rain streak, [1,0]=splash ring, [0,1]=soft glow dot, [1,1]=leaf
    {
        uint32_t waPixels[64 * 64];
        for (int32_t y = 0; y < 64; ++y)
        {
            for (int32_t x = 0; x < 64; ++x)
            {
                const int32_t qx = x >> 5;
                const int32_t qy = y >> 5;
                const float lx = ((float)(x & 31) + 0.5f - 16.0f) / 15.0f;
                const float ly = ((float)(y & 31) + 0.5f - 16.0f) / 15.0f;
                float a = 0.0f;
                if (qx == 0 && qy == 0)
                {
                    // Soft rain streak capsule
                    const float ax = fabsf(lx) * 3.8f;
                    const float ay = fabsf(ly);
                    if (ax < 1.0f && ay < 1.0f)
                        a = (1.0f - ax * ax) * (1.0f - ay * ay);
                }
                else if (qx == 1 && qy == 0)
                {
                    // Splash ring
                    const float r = sqrtf(lx * lx + ly * ly);
                    const float d = fabsf(r - 0.68f) / 0.24f;
                    if (d < 1.0f)
                        a = (1.0f - d * d);
                }
                else if (qx == 0 && qy == 1)
                {
                    // Gaussian glow dot (motes, fireflies, water glints, bloom emitters)
                    const float r2 = lx * lx + ly * ly;
                    if (r2 < 1.0f)
                    {
                        const float inv = 1.0f - r2;
                        a = inv * inv;
                    }
                }
                else
                {
                    // Leaf silhouette
                    const float u = lx * 0.85f + ly * 0.52f;
                    const float v = -lx * 0.52f + ly * 0.85f;
                    const float r2 = (u * u) / 0.65f + (v * v) / 0.18f;
                    if (r2 < 1.0f)
                        a = 1.0f - r2 * r2;
                }
                const uint32_t alpha = (uint32_t)(a * 255.0f + 0.5f);
                waPixels[y * 64 + x] = (alpha << 24) | 0x00FFFFFFu;
            }
        }
        glGenTextures(1, &sWeatherAtlasTex);
        glBindTexture(GL_TEXTURE_2D, sWeatherAtlasTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, waPixels);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    sDioramaFboReady = allPostReady;
    if (!allPostReady)
        VITA_LOG("[VOXEL ERROR] Post targets unavailable: no bloom, sky bloom, shafts, grade or DoF this run\n");

    // Frozen 3D+PostFX backdrop target used while pause/UI menus are open on the overworld
    sMenuBgTex = CreatePostColorTexture(VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_FBO_H, GL_NEAREST, GL_UNSIGNED_BYTE);
    if (sMenuBgTex != 0)
    {
        glGenRenderbuffers(1, &sMenuBgDepthRb);
        glBindRenderbuffer(GL_RENDERBUFFER, sMenuBgDepthRb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_FBO_H);
        AttachColorFbo(&sMenuBgFbo, sMenuBgTex);
        if (sMenuBgFbo != 0)
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sMenuBgDepthRb);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    VoxelHd2dShader_Init();

    MakeFog();
    MakeWeather();

    VoxelCamera_Init(&sCamera);
    sNeedCameraSnap = true;
    sLastPrimary = NULL;
    sLastSecondary = NULL;
    InvalidateChunkCache();

    VITA_LOG("[VOXEL] Voxel renderer initialized.\n");
    return true;
}

// Visible tile rows and columns of BG0 (the GBA screen in 8-px tiles) and its map row width.
#define UI_BG0_VISIBLE_COLS (DISPLAY_WIDTH / 8)
#define UI_BG0_VISIBLE_ROWS (DISPLAY_HEIGHT / 8)
#define UI_BG0_MAP_COLS 32
#define UI_TILE_INDEX_MASK 0x3FF

// Whether BG0's tilemap in VRAM shows any tile on screen. Text boxes, menus and popups are
// drawn into it and cleared from it when they close, so this is what the player actually sees,
// whatever state the game's text logic is in (a message that finished printing marks the box
// as hidden while it is still on screen waiting for A).
static bool Bg0ShowsTiles(void)
{
    const u16 control = REG_BG0CNT;
    const u16 *const map = (const u16 *)BG_SCREEN_ADDR((control >> 8) & 0x1F);

    for (int32_t y = 0; y < UI_BG0_VISIBLE_ROWS; y++)
    {
        const u16 *const row = &map[y * UI_BG0_MAP_COLS];

        for (int32_t x = 0; x < UI_BG0_VISIBLE_COLS; x++)
        {
            if (row[x] & UI_TILE_INDEX_MASK)
                return true;
        }
    }
    return false;
}

// Checks if 2D UI overlay (Pass 6) needs to be rendered on top of the 3D diorama.
// In 3D battles, UI (HP boxes, battle action decks) is always active.
// In overworld, BG0 is inspected for active window allocations (dialogues, map popups, menus).
static inline bool VoxelRenderer_HasActiveUi(bool inBattle3D)
{
    if (inBattle3D)
        return true;
    const GameFrameOverlay *const overlay = &GameFrame_Get()->overlay;
    if (overlay->battleTransition)
        return true;

    if (overlay->buyMenu)
        return true;

    if (overlay->fieldMoveShowMon || overlay->flyAnim)
        return true;

    if ((REG_DISPCNT & 0x0100) == 0)
        return false;

    // New UI: the visible field windows are drawn by its mirrors, the GBA frame is not needed
    if (Ui_MirrorCovers())
        return false;

    // Standard message box active (NPC dialogues, signposts, text)
    if (overlay->messageBox)
        return true;

    // Map name popup banner (e.g. "LITTLEROOT TOWN")
    if (overlay->mapNamePopUp)
        return true;

    // Active text printer on standard window 0
    if (overlay->textPrinter0)
        return true;

    // Any secondary active window on BG0 (YES/NO, multichoice menus, etc.)
    if (overlay->bg0Window)
        return true;

    // A message box that finished printing and waits for A, or anything else still drawn on BG0
    return Bg0ShowsTiles();
}

// State of one frame, filled by the stages of VoxelRenderer_RenderFrame and read by its passes.
typedef struct
{
    double startMs;                     // VitaPerf_NowMs() at the start of the frame
    bool inBattle3D;                    // the 3D battle stage is drawn instead of the field
    bool mapLocked;                     // Core 1 is rebuilding the layout (sMapMutationLock)
    const VoxelMapInstance *inst0;      // the current map
    bool mapChanged;                    // the current map is not the one the chunks were built for
    bool crossed;                       // the map change was a walk across a connection
    float playerX, playerZ;             // player position in the current map's tiles
    bool indoor;
    float fadeAmount;                   // real GBA palette fade, 0..1 (used for atmosphere snap & sync builds)
    float drawFade;                     // drawn screen fade, 0..1 (0 when Fly noFade hides fade under clouds)
    float fadeRgb[3];
    VoxelFlySnapshot fly;               // Core 1 Fly cinematic snapshot for this frame
    VoxelWeatherClass weather;
    float weatherAmount;
    float fogDensity;
    float bolt;                         // lightning flash, 0..1
    bool underground;
    bool liveChanged;
    VoxelLight light;
    double clock;                       // atmosphere animation clock, seconds (frozen under menus)
    float seconds;                      // clock as float, for terms whose speeds keep its error small
    float swayPhase[5];                 // foliage sway phases, radians (Frame_DeriveClock)
    VoxelHd2dShaftClock shaftClock[2];  // the shafts' wrapped animation, sun then moon pass
    float nightGlow;                    // lamp and window light, 0..1 (0 indoors, by day, in battle)
    VoxelChunkSlot *visible[MAX_CHUNKS];
    int32_t visibleCount;
    int32_t holesLeft;                  // visible chunks still awaiting build (Core 2)
    VoxelFrameGrade grade;
    int32_t renderW, renderH;
    // The world scene's viewport: renderW x renderH, or half of each for a 3D battle in HIGH
    // (battleHalf: the stage is blurred whole, so it is drawn at the size it survives at).
    int32_t worldW, worldH;
    bool battleHalf;
    bool dioramaActive;                 // the 3D scene goes to the FBO for the depth of field
    Hd2dPreset hd2dPreset;              // 0=OFF, 1=LOW (60 FPS), 2=HIGH (30 FPS)
    bool hd2dMarkedGrade;               // true=HD-2D cinematic grading, false=GBA faithful
    float playerScreenY;                // 0..1 screen Y of the player (focal plane center)
} VoxelFrame;

// Chunk builds left after the visibility loop: urgent visible holes (center-out), unbuilt
// chunks outside the view (pre-warm), and visible chunks whose content is only dated (stale).
typedef struct
{
    double frameStartMs;
    double budgetMs;                    // frame time the builds may use (ChunkBuildFits)
    int32_t built;                      // chunks built this frame
    bool indoor;
    float playerX, playerZ;
    float targetX, targetZ;             // camera target: holes are filled nearest to this first
    float lookX, lookZ;                 // where pre-warming aims (MovementLookahead)
    int32_t holeCx[MAX_CHUNKS];
    int32_t holeCy[MAX_CHUNKS];
    uint32_t holeHash[MAX_CHUNKS];
    bool holeHashValid[MAX_CHUNKS];
    int32_t holeCount;
    int32_t prewarmCx[VOXEL_PREFETCH_MAX];
    int32_t prewarmCy[VOXEL_PREFETCH_MAX];
    bool prewarmSoon[VOXEL_PREFETCH_MAX]; // the view reaches it within VOXEL_VIEW_AHEAD_TILES
    int32_t prewarmCount;
    int32_t staleCx[VOXEL_STALE_MAX];
    int32_t staleCy[VOXEL_STALE_MAX];
    int32_t staleCount;
} VoxelBuildQueue;

// The content hash a chunk is built for (rebuilt when it changes).
static inline uint32_t ChunkContentHash(const int32_t cx0, const int32_t cy0, const bool indoor)
{
    const int32_t cx1 = cx0 + CHUNK_SIZE;
    const int32_t cy1 = cy0 + CHUNK_SIZE;
    return indoor
        ? VoxelWorld_BlockHash(cx0 - 1, cy0 - VOXEL_CHUNK_MARGIN_NORTH - 1, cx1 + 1, cy1 + 2)
        : VoxelLighting_Hash(cx0, cy0, cx1, cy1);
}

// Start of the frame: counters, the chunk worker's state, the map instances. False when
// there is no 3D world to draw.
static bool Frame_Begin(VoxelFrame *const f)
{
    const double frameStartMs = VitaPerf_NowMs();
    sFrameDrawCalls = 0;
    sFrameVertCount = 0;
    // The day/night state every pass of this frame reads (one schedule, one clock read).
    VitaDayNight_UpdateFrame();
    sFrameCounter++;

    const bool inBattle3D = VoxelWorld_IsBattleMapAvailable()
        && VitaExtras_GetFast(FAST_EXTRA_VOXEL_BATTLE) != 0
        && !VoxelBattle_IsLink();

    // The chunk worker may still be computing (EnsureWorkerIdle). The frame goes on alongside
    // it only while the world it reads stands as it was: same current map, no warm-up, not a
    // 3D battle (which builds its own stage). Otherwise it waits now.
    sWorkerBusyThisFrame = ChunkWorker_Poll();
    if (sWorkerBusyThisFrame)
    {
        const VoxelMapInstance *const current = VoxelWorld_InstanceCount() > 0 ? VoxelWorld_Instance(0) : NULL;
        const GameFrameField *const field = &GameFrame_Get()->field;
        const bool worldStands = current != NULL && field->saveLoaded
            && current->mapGroup == field->mapGroup
            && current->mapNum == field->mapNum;
        if (inBattle3D || sNeedsFullWarmup || !worldStands)
            EnsureWorkerIdle();
        else
            VitaPerf_AddCount(VITA_PERF_C_WORKER_OVERLAP, 1);
    }

    if (!VoxelWorld_IsMapAvailable() && !inBattle3D)
        return false;

    // Zero-Stall Multicore Concurrency: when Core 1 is mutating map layout in
    // LoadMapFromCameraTransition / DoMapLoadLoop, render using previous instances and snapshot.
    // (and the frame Core 0 pinned is of the map the grid holds: GameFrame_MapSettled)
    const bool mapLocked = !GameFrame_MapSettled();
    // (with the worker busy the instances stay as they are: it reads them, and the current map
    // has not changed)
    if (!inBattle3D && !mapLocked && !sWorkerBusyThisFrame)
    {
        VoxelWorld_BeginBatch();
        VitaPerf_SubBegin(VITA_PERF_T_WORLD_INSTANCES);
        VoxelWorld_BuildInstances();
        VitaPerf_SubEnd(VITA_PERF_T_WORLD_INSTANCES);
    }

    if (VoxelWorld_InstanceCount() == 0)
        return false;

    const VoxelMapInstance *inst0 = VoxelWorld_Instance(0);
    if (!inst0)
        return false;
    /* The game frame was pinned for this whole Core 0 frame (MainThread_Run, GameFrame_Pin): the
     * camera, Z-shift, ground height and billboards all see the same one. */
    f->startMs = frameStartMs;
    f->inBattle3D = inBattle3D;
    f->mapLocked = mapLocked;
    f->mapChanged = false;
    f->crossed = false;
    f->liveChanged = false;
    f->inst0 = inst0;
    f->fly = *VoxelFly_GetSnapshot();
    return true;
}

// Stage 1: follow camera (field or 3D battle), map changes and crossings.
typedef struct
{
    bool     active;
    uint16_t prevHp[MAX_BATTLERS_COUNT];
    double   prevMs;
    float    flashTimer;
    float    flashDur;
    float    flashAlpha;
    float    flashRgb[3];
    float    shakeTimer;
    float    shakeDur;
    float    shakeAmp;
} VoxelBattleFxState;

static VoxelBattleFxState sBattleFx = {0};

static void BattleTypeToFlashRgb(const uint8_t moveType, float outRgb[3])
{
    switch (moveType)
    {
        case TYPE_FIRE:     outRgb[0] = 1.00f; outRgb[1] = 0.42f; outRgb[2] = 0.14f; break;
        case TYPE_WATER:    outRgb[0] = 0.25f; outRgb[1] = 0.72f; outRgb[2] = 1.00f; break;
        case TYPE_GRASS:    outRgb[0] = 0.28f; outRgb[1] = 0.92f; outRgb[2] = 0.42f; break;
        case TYPE_ELECTRIC: outRgb[0] = 1.00f; outRgb[1] = 0.92f; outRgb[2] = 0.24f; break;
        case TYPE_ICE:      outRgb[0] = 0.62f; outRgb[1] = 0.96f; outRgb[2] = 1.00f; break;
        case TYPE_FIGHTING: outRgb[0] = 0.95f; outRgb[1] = 0.35f; outRgb[2] = 0.22f; break;
        case TYPE_POISON:   outRgb[0] = 0.78f; outRgb[1] = 0.28f; outRgb[2] = 0.90f; break;
        case TYPE_GROUND:   outRgb[0] = 0.90f; outRgb[1] = 0.68f; outRgb[2] = 0.32f; break;
        case TYPE_FLYING:   outRgb[0] = 0.68f; outRgb[1] = 0.82f; outRgb[2] = 1.00f; break;
        case TYPE_PSYCHIC:  outRgb[0] = 1.00f; outRgb[1] = 0.34f; outRgb[2] = 0.68f; break;
        case TYPE_BUG:      outRgb[0] = 0.68f; outRgb[1] = 0.88f; outRgb[2] = 0.24f; break;
        case TYPE_ROCK:     outRgb[0] = 0.82f; outRgb[1] = 0.72f; outRgb[2] = 0.45f; break;
        case TYPE_GHOST:    outRgb[0] = 0.56f; outRgb[1] = 0.36f; outRgb[2] = 0.92f; break;
        case TYPE_DRAGON:   outRgb[0] = 0.48f; outRgb[1] = 0.42f; outRgb[2] = 1.00f; break;
        case TYPE_DARK:     outRgb[0] = 0.46f; outRgb[1] = 0.36f; outRgb[2] = 0.52f; break;
        case TYPE_STEEL:    outRgb[0] = 0.78f; outRgb[1] = 0.86f; outRgb[2] = 0.94f; break;
        default:            outRgb[0] = 1.00f; outRgb[1] = 0.96f; outRgb[2] = 0.88f; break;
    }
}

/* Last warp version observed from Core 1's LoadMapFromWarp (reset on SoftReset). */
static uint32_t sLastFlyWarpVersion = 1u;

/* Full Mode B Fly camera (tuning knobs live in voxel_tuning.h). */
static void FlyCamera(const VoxelFlyFrame *const fly)
{
    const float lift = Voxel_ClampF(fly->lift, 0.0f, 1.0f);
    const float pitch = sCamera.pitch + (VOXEL_FLY_PITCH - sCamera.pitch) * lift;
    float distance;

    if (fly->lift < 0.0f)
    {
        distance = sCamera.distance * (1.0f + Voxel_ClampF(fly->lift, -1.0f, 0.0f));
    }
    else
    {
        const float targetDist = fmaxf(sCamera.distance, VOXEL_FLY_DISTANCE);
        distance = sCamera.distance + (targetDist - sCamera.distance) * lift;
    }

    const float lead = VOXEL_FLY_LEAD * lift * (fly->oldMap ? 1.0f : -1.0f);
    const float dx = fly->dirX * lead;
    const float dz = fly->dirZ * lead;

    VoxelCamera_Frame(&sCamera, sCamera.targetX + dx, sCamera.targetZ + dz,
                      sCamera.ground, pitch, distance);
    sCamera.swoopX += dx;
    sCamera.swoopZ += dz;
    sCamera.y += VOXEL_FLY_RISE * Voxel_ClampF(fly->rise, 0.0f, 1.0f);
}

/* Deterministic warp detection via Core 1's LoadMapFromWarp counter (covers same-map warps too). */
static bool DetectWarpSnap(const VoxelFrame *const f)
{
    if (f->mapLocked || f->fly.warpVersion == 0u || f->fly.warpVersion == sLastFlyWarpVersion)
        return false;
    sLastFlyWarpVersion = f->fly.warpVersion;
    sNeedCameraSnap = true;
    return true;
}

static void Frame_FollowCamera(VoxelFrame *const f)
{
    const bool inBattle3D = f->inBattle3D;
    const bool mapLocked = f->mapLocked;
    const VoxelMapInstance *const inst0 = f->inst0;
    const int32_t mapGroup = inst0->mapGroup;
    const int32_t mapNum   = inst0->mapNum;
    const bool mapChanged = (mapGroup != sMeshMapGroup || mapNum != sMeshMapNum);
    const bool warpOccurred = DetectWarpSnap(f);
    bool crossed = false;

    static double sPrevCamMs = 0.0;
    float camDt = (sPrevCamMs > 0.0) ? (float)((f->startMs - sPrevCamMs) * 0.001) : (1.0f / 30.0f);
    if (camDt < 0.001f) camDt = 0.001f;
    if (camDt > VOXEL_FOLLOW_DT_MAX) camDt = VOXEL_FOLLOW_DT_MAX;
    sPrevCamMs = f->startMs;

    float playerWorldX = 0.0f, playerWorldZ = 0.0f;
    VoxelEntities_GetPlayerCameraTarget(&playerWorldX, &playerWorldZ);

    if (!mapLocked && (mapChanged || sNeedCameraSnap))
    {
        int32_t shiftTilesX = 0, shiftTilesY = 0;
        if (mapChanged && !sNeedCameraSnap)
        {
            for (uint32_t i = 0; i < sPreviousOriginCount; ++i)
            {
                if (sPreviousOrigins[i].mapGroup == mapGroup && sPreviousOrigins[i].mapNum == mapNum)
                {
                    shiftTilesX = -sPreviousOrigins[i].originX;
                    shiftTilesY = -sPreviousOrigins[i].originY;
                    break;
                }
            }
            VitaPerf_SubBegin(VITA_PERF_T_MAP_SHIFT);
            crossed = HandleMapChange(mapGroup, mapNum, playerWorldX, playerWorldZ, warpOccurred);
            VitaPerf_SubEnd(VITA_PERF_T_MAP_SHIFT);
        }
        else
        {
            VoxelCamera_SetGround(&sCamera, VoxelEntities_PlayerGround(), 1, camDt);
            VoxelCamera_Snap(&sCamera, playerWorldX, playerWorldZ);
            sLastPlayerX = playerWorldX;
            sLastPlayerZ = playerWorldZ;
            sNeedCameraSnap = false;
        }

        /* Only invalidate the chunk cache on an actual non-crossing map change; a camera snap
         * on the same map (same-map warp or battle return) keeps all valid cached chunks intact. */
        if (crossed)
        {
            VitaPerf_MarkEvent(VITA_PERF_E_CROSS);
            VitaPerf_SubBegin(VITA_PERF_T_MAP_SHIFT);
            ShiftChunkCoordinates(shiftTilesX, shiftTilesY);
            VitaPerf_SubEnd(VITA_PERF_T_MAP_SHIFT);
            // Coordinates gained shiftTiles; the anchor loses them so coordinate + anchor, the
            // frame the atmosphere wraps on, is the same before and after the crossing.
            sAtmoAnchorX -= shiftTilesX;
            sAtmoAnchorZ -= shiftTilesY;
            SurfaceFx_ShiftCoordinates(shiftTilesX, shiftTilesY);
        }
        else if (mapChanged)
        {
            VitaPerf_MarkEvent(VITA_PERF_E_MAP_RESET);
            InvalidateChunkCache();
        }
    }

    // Camera tracking: 3D Battle stage camera when in battle, smooth overworld tracking otherwise
    if (inBattle3D)
    {
        if (!sBattleState.begun)
        {
            sBattleState.field = sCamera;
            VoxelBattle_BeginStage();
            sBattleState.begun = true;
            sBattleState.frames = 0;
            sBattleState.glide = 0;
            sBattleState.glided = false;
            sBattleState.chosen = false;
        }
        if (!sBattleState.chosen)
        {
            sBattleState.chosen = VoxelBattle_StepStage(32, 16,
                                                       &sBattleState.targetX,
                                                       &sBattleState.targetZ,
                                                       &sBattleState.ground);
        }
        BattleCamera();
        if (!sBattleFx.active)
        {
            sBattleFx.active = true;
            sBattleFx.prevMs = f->startMs;
            sBattleFx.flashTimer = 0.0f;
            sBattleFx.shakeTimer = 0.0f;
            for (int32_t b = 0; b < MAX_BATTLERS_COUNT; b++)
                sBattleFx.prevHp[b] = GameFrame_Get()->battle.hp[b];
        }
        else
        {
            float dt = (float)((f->startMs - sBattleFx.prevMs) * 0.001);
            if (dt < 0.0f) dt = 0.0f;
            if (dt > 0.05f) dt = 0.05f;
            sBattleFx.prevMs = f->startMs;

            bool hitOccurred = false;
            for (int32_t b = 0; b < MAX_BATTLERS_COUNT; b++)
            {
                const uint16_t curHp = GameFrame_Get()->battle.hp[b];
                if (curHp < sBattleFx.prevHp[b])
                    hitOccurred = true;
                sBattleFx.prevHp[b] = curHp;
            }
            if (hitOccurred)
            {
                const bool isCrit = GameFrame_Get()->battle.critical;
                const uint8_t mType = GameFrame_Get()->battle.moveType;

                BattleTypeToFlashRgb(mType, sBattleFx.flashRgb);
                if (isCrit)
                {
                    sBattleFx.flashRgb[0] = 0.72f + 0.28f * sBattleFx.flashRgb[0];
                    sBattleFx.flashRgb[1] = 0.72f + 0.28f * sBattleFx.flashRgb[1];
                    sBattleFx.flashRgb[2] = 0.72f + 0.28f * sBattleFx.flashRgb[2];
                    sBattleFx.flashDur = 0.28f;
                    sBattleFx.flashAlpha = 0.52f;
                    sBattleFx.shakeDur = 0.24f;
                    sBattleFx.shakeAmp = 0.22f;
                }
                else
                {
                    sBattleFx.flashDur = 0.18f;
                    sBattleFx.flashAlpha = 0.28f;
                    sBattleFx.shakeDur = 0.14f;
                    sBattleFx.shakeAmp = 0.11f;
                }
                sBattleFx.flashTimer = sBattleFx.flashDur;
                sBattleFx.shakeTimer = sBattleFx.shakeDur;
            }
            if (sBattleFx.flashTimer > 0.0f)
            {
                sBattleFx.flashTimer -= dt;
                if (sBattleFx.flashTimer < 0.0f) sBattleFx.flashTimer = 0.0f;
            }
            if (sBattleFx.shakeTimer > 0.0f)
            {
                sBattleFx.shakeTimer -= dt;
                if (sBattleFx.shakeTimer < 0.0f) sBattleFx.shakeTimer = 0.0f;
            }
            if (sBattleFx.shakeTimer > 0.0f && sBattleFx.shakeDur > 0.0f)
            {
                const float ratio = sBattleFx.shakeTimer / sBattleFx.shakeDur;
                const float amp = sBattleFx.shakeAmp * ratio * ratio;
                const float tSec = (float)(f->startMs * 0.001);
                sCamera.targetX += sinf(tSec * 62.0f) * amp;
                sCamera.targetZ += cosf(tSec * 49.0f) * amp * 0.7f;
            }
        }
    }
    else
    {
        sBattleFx.active = false;
        sBattleFx.flashTimer = 0.0f;
        sBattleFx.shakeTimer = 0.0f;
        if (sBattleState.begun)
        {
            /* Snap directly back to the overworld camera on the exact exit frame without
             * flagging sNeedCameraSnap or invalidating the still-valid overworld chunk cache. */
            sBattleState.begun = false;
            sBattleState.chosen = false;
            VoxelCamera_SetGround(&sCamera, VoxelEntities_PlayerGround(), 1, camDt);
            VoxelCamera_Snap(&sCamera, playerWorldX, playerWorldZ);
        }
        else if (!sNeedCameraSnap)
        {
            // Smooth framerate-independent camera tracking continues on every frame, including
            // crossing frames (right after VoxelCamera_Shift), so biking across borders never pauses 1 frame.
            const float playerGround = VoxelEntities_PlayerGround();
            VoxelCamera_SetGround(&sCamera, playerGround, 0, camDt);
            VoxelCamera_Update(&sCamera, playerWorldX, playerWorldZ, camDt);
        }
        if (f->fly.active)
            FlyCamera(&f->fly.frame);
    }

    if (!mapLocked)
    {
        sLastPlayerX = playerWorldX;
        sLastPlayerZ = playerWorldZ;
        RememberInstanceOrigins();
        sMeshMapGroup = mapGroup;
        sMeshMapNum = mapNum;
    }
    
    f->mapChanged = mapChanged;
    f->crossed = crossed;
    f->playerX = playerWorldX;
    f->playerZ = playerWorldZ;
}

// Proactively extend already-cached direct neighbor atlases incrementally (AtlasExtend_Step,
// <=4 ms/frame) on calm frames before the player crosses the border. Returns true when every
// direct neighbor is already covered (no extension job started).
static bool ExtendDirectNeighborAtlases(const VoxelMapInstance *const inst0)
{
    const uint32_t instCount = VoxelWorld_InstanceCount();
    for (uint32_t i = 1u; i < instCount; ++i)
    {
        const VoxelMapInstance *const nInst = VoxelWorld_Instance(i);
        if (nInst == NULL || nInst->secondaryTileset == NULL)
            continue;
        if (nInst->originX > inst0->width + VOXEL_NEIGHBOR_MARGIN_TILES
         || nInst->originX + nInst->width < -VOXEL_NEIGHBOR_MARGIN_TILES
         || nInst->originY > inst0->height + VOXEL_NEIGHBOR_MARGIN_TILES
         || nInst->originY + nInst->height < -VOXEL_NEIGHBOR_MARGIN_TILES)
            continue;

        for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; ++s)
        {
            if (!sAtlasSlots[s].valid
             || sAtlasSlots[s].primaryTileset != nInst->primaryTileset
             || sAtlasSlots[s].secondaryTileset != nInst->secondaryTileset)
                continue;
            if (AtlasExtend_Start(s, nInst))
            {
                EnsureWorkerIdle();
                return false;
            }
            break;
        }
    }
    return true;
}

// Stage 2: metatile atlas slots for the current map (multi-atlas LRU, as Emerald 3DS).
static void Frame_UpdateAtlas(const VoxelFrame *const f)
{
    {
        static int32_t sLastReliefMode = -1;
        const int32_t wantRelief = (GetHd2dPreset() != HD2D_PRESET_OFF) ? 1 : 0;
        if (sLastReliefMode != wantRelief)
        {
            VoxelGrade_SetReliefEnabled(wantRelief != 0);
            if (sLastReliefMode != -1)
            {
                EnsureWorkerIdle();
                AtlasExtend_Cancel();
                for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; ++s)
                    sAtlasSlots[s].valid = false;
                sActiveAtlasSlot = -1;
                sExtendSlot = -1;
                sNeedAtlasExtension = false;
                sNeedsFullWarmup = true;
                sWorldEpoch++;
            }
            sLastReliefMode = wantRelief;
        }
    }
    const VoxelMapInstance *const inst0 = f->inst0;
    const bool mapChanged = f->mapChanged;
    static bool sNeighborsAtlasChecked = false;
    VitaPerf_SubBegin(VITA_PERF_T_ATLAS_SLOTS);
    bool tilesetChanged = (inst0->primaryTileset != sLastPrimary || inst0->secondaryTileset != sLastSecondary);
    if (tilesetChanged || mapChanged)
        sNeighborsAtlasChecked = false;
    // Composing, extending or activating an atlas slot changes the maps the worker reads.
    if (tilesetChanged || sActiveAtlasSlot < 0 || mapChanged || sNeedAtlasExtension || sExtendSlot >= 0)
        EnsureWorkerIdle();
    if (tilesetChanged || sActiveAtlasSlot < 0)
    {
        memset(sVoxelTilesetAnimDirty, 0, sizeof(sVoxelTilesetAnimDirty));
        sVoxelTilesetAnimPending = false;
        int32_t slotIdx = EnsureAtlasSlotForInstance(inst0);
        // Mark primary animated tiles dirty when reactivating a cached slot so water/lakes sync immediately.
        memset(sSlotPendingPrimary[slotIdx], 0xFF, 64);
        if (sNeedsFullWarmup)
        {
            // Warp: the screen is dark and every chunk is built now, so extend at once and
            // build them complete.
            AtlasExtend_Cancel();
            ExtendAtlasSlot(&sAtlasSlots[slotIdx], inst0);
        }
        else
        {
            AtlasExtend_Start(slotIdx, inst0);
        }
        ActivateAtlasSlot(slotIdx);

        sLastPrimary = inst0->primaryTileset;
        sLastSecondary = inst0->secondaryTileset;
    }
    else if ((mapChanged || sNeedAtlasExtension) && sActiveAtlasSlot >= 0)
    {
        if (sNeedsFullWarmup)
        {
            sNeedAtlasExtension = false;
            AtlasExtend_Cancel();
            if (ExtendAtlasSlot(&sAtlasSlots[sActiveAtlasSlot], inst0))
                ActivateAtlasSlot(sActiveAtlasSlot);
        }
        else if (sExtendSlot < 0 || sExtendSlot == sActiveAtlasSlot)
        {
            // Asked again while it runs: its end bumps `extension`, and anything still missing
            // then asks once more.
            sNeedAtlasExtension = false;
            AtlasExtend_Start(sActiveAtlasSlot, inst0);
        }
    }
    if (sExtendSlot < 0 && !sNeighborsAtlasChecked && !mapChanged && !tilesetChanged
        && !f->fly.active && !sNeedsFullWarmup)
    {
        sNeighborsAtlasChecked = ExtendDirectNeighborAtlases(inst0);
    }
    AtlasExtend_Step();

    // Preload the current outdoor layout's glow page before chunk building so Core 2 has its
    // 32x32 UV occupancy bitmask ready from the very first chunk (eliminating non-window walls).
    if (!inst0->indoor)
    {
        const int32_t p0 = VoxelBuildings_PageOf(inst0);
        if (p0 >= 0 && p0 < VOXEL_MAX_BUILDING_PAGES
            && VoxelBuildings_PageHasGlow((uint32_t)p0)
            && __atomic_load_n(&sBuildingGlowGridReady[p0], __ATOMIC_ACQUIRE) == 0u)
        {
            RequestOrLoadBuildingPage(p0, sNeedsFullWarmup);
        }
    }

    VitaPerf_SubEnd(VITA_PERF_T_ATLAS_SLOTS);
}

static bool MaskWordsAny(const uint32_t *const words, const uint32_t count)
{
    for (uint32_t w = 0; w < count; w++)
    {
        if (words[w] != 0)
            return true;
    }
    return false;
}

static bool MaskWordsIntersect(const uint32_t *const a, const uint32_t *const b, const uint32_t count)
{
    for (uint32_t w = 0; w < count; w++)
    {
        if ((a[w] & b[w]) != 0)
            return true;
    }
    return false;
}

static uint32_t AnimateActiveAtlas(const VoxelMapInstance *const inst0)
{
    VoxelAtlasSlot *const slot = &sAtlasSlots[sActiveAtlasSlot];
    const uint32_t *const dirtyWords = (const uint32_t *)sVoxelTilesetAnimDirty;
    const uint32_t maxPages = (sAtlasPageCount > VOXEL_ANIM_PAGES) ? VOXEL_ANIM_PAGES : sAtlasPageCount;
    uint32_t rowsUpdated = 0;

    if (!MaskWordsAny(dirtyWords, 32u))
        return 0;

    for (uint32_t page = 0; page < maxPages; page++)
    {
        const uint32_t *const pageWords = (const uint32_t *)slot->map.tileMask[page];
        if (!MaskWordsIntersect(dirtyWords, pageWords, 32u) || !slot->tex[page])
            continue;

        uint32_t changedRows = 0;
        VitaPerf_SubBegin(VITA_PERF_T_ATLAS_COMPOSE);
        const uint32_t changed = VoxelAtlas_RefreshAnimatedPageRows(inst0, &slot->map, page,
                                                                    slot->linear[page], (const uint8_t *)BG_VRAM,
                                                                    sVoxelTilesetAnimDirty, &changedRows);
        VitaPerf_SubEnd(VITA_PERF_T_ATLAS_COMPOSE);
        if (changed > 0 && changedRows != 0)
        {
            VitaPerf_SubBegin(VITA_PERF_T_ATLAS_UPLOAD);
            WriteAtlasRowsDirect(slot->tex[page], slot->linear[page], changedRows);
            VitaPerf_SubEnd(VITA_PERF_T_ATLAS_UPLOAD);
            rowsUpdated += (uint32_t)__builtin_popcount(changedRows);
        }
    }
    memset(sSlotPendingPrimary[sActiveAtlasSlot], 0, sizeof(sSlotPendingPrimary[0]));
    return rowsUpdated;
}

static void AccumulatePrimaryDirty(const void *const primary)
{
    const uint32_t *const dirtyWords = (const uint32_t *)sVoxelTilesetAnimDirty;

    if (primary == NULL || !MaskWordsAny(dirtyWords, 16u))
        return;

    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        if (s == sActiveAtlasSlot || !sAtlasSlots[s].valid || sAtlasSlots[s].primaryTileset != primary)
            continue;
        for (uint32_t w = 0; w < 16u; w++)
            sSlotPendingPrimary[s][w] |= dirtyWords[w];
    }
}

static uint32_t SyncVisiblePrimarySlots(const void *const primary)
{
    uint32_t rowsUpdated = 0;
    uint8_t primaryMask[128];

    if (primary == NULL)
        return 0;

    memset(primaryMask + 64, 0, 64);
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        VoxelAtlasSlot *const other = &sAtlasSlots[s];
        if (s == sActiveAtlasSlot || !other->valid || other->primaryTileset != primary
            || (uint32_t)(sFrameCounter - other->stamp) > 2u || AtlasExtend_Busy(s))
            continue;
        if (!MaskWordsAny(sSlotPendingPrimary[s], 16u))
            continue;

        memcpy(primaryMask, sSlotPendingPrimary[s], 64);
        const uint32_t pages = (other->pageCount < VOXEL_ANIM_PAGES) ? other->pageCount : VOXEL_ANIM_PAGES;
        for (uint32_t page = 0; page < pages; page++)
        {
            const uint32_t *const pageWords = (const uint32_t *)other->map.tileMask[page];
            if (!MaskWordsIntersect(sSlotPendingPrimary[s], pageWords, 16u) || !other->tex[page])
                continue;

            uint32_t changedRows = 0;
            VitaPerf_SubBegin(VITA_PERF_T_ATLAS_COMPOSE);
            VoxelAtlas_RefreshAnimatedPrimaryRows(other->primaryTileset, other->secondaryTileset, &other->map,
                                                  page, other->linear[page], (const uint8_t *)BG_VRAM,
                                                  primaryMask, &changedRows);
            VitaPerf_SubEnd(VITA_PERF_T_ATLAS_COMPOSE);
            if (changedRows != 0)
            {
                VitaPerf_SubBegin(VITA_PERF_T_ATLAS_UPLOAD);
                WriteAtlasRowsDirect(other->tex[page], other->linear[page], changedRows);
                VitaPerf_SubEnd(VITA_PERF_T_ATLAS_UPLOAD);
                rowsUpdated += (uint32_t)__builtin_popcount(changedRows);
            }
        }
        memset(sSlotPendingPrimary[s], 0, sizeof(sSlotPendingPrimary[0]));
        // Spread secondary-zone atlas refreshes across frames so compose CPU spikes don't stack.
        if (rowsUpdated > 0u)
            break;
    }
    return rowsUpdated;
}

// Stage 3: animated tiles (water, flowers, shores) recomposed and presented synchronously across zones.
static void Frame_AnimateTiles(const VoxelFrame *const f)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    VitaPerf_MarkAtlasAnimStart();
    uint32_t rowsUpdated = 0;

    if (sVoxelTilesetAnimPending && sAtlasPageCount > 0 && inst0 != NULL && sActiveAtlasSlot >= 0
        && !AtlasExtend_Busy(sActiveAtlasSlot))
    {
        rowsUpdated += AnimateActiveAtlas(inst0);
        AccumulatePrimaryDirty(sAtlasSlots[sActiveAtlasSlot].primaryTileset);
        memset(sVoxelTilesetAnimDirty, 0, sizeof(sVoxelTilesetAnimDirty));
        sVoxelTilesetAnimPending = false;
    }

    if (sActiveAtlasSlot >= 0 && sAtlasSlots[sActiveAtlasSlot].valid)
        rowsUpdated += SyncVisiblePrimarySlots(sAtlasSlots[sActiveAtlasSlot].primaryTileset);

    VitaPerf_MarkAtlasAnimEnd(rowsUpdated);
}

// Stage 4: live map changes (scripts, doors) and the sun; resets the caches they date.
static void Frame_TrackWorldChanges(VoxelFrame *const f)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    const bool crossed = f->crossed;
    const bool mapChanged = f->mapChanged;
    VitaPerf_SubBegin(VITA_PERF_T_LIVE_DIGEST);
    uint32_t liveDigest = VoxelWorld_LiveDigest();
    VitaPerf_SubEnd(VITA_PERF_T_LIVE_DIGEST);
    bool liveChanged = false;
    if (liveDigest != sLastLiveDigest)
    {
        sLastLiveDigest = liveDigest;
        liveChanged = true;
        VitaPerf_MarkEvent(VITA_PERF_E_LIVE);
    }

    const bool indoor = inst0->indoor;
    const VitaDayNightSun *sun = VitaDayNight_GetSun();
    if (sun != NULL)
    {
        // (the worker's rays read the sun: a change waits for it)
        if (sun->dx != gVoxelSun.dx || sun->dz != gVoxelSun.dz || sun->strength != gVoxelSun.strength
            || sun->key != gVoxelSun.key)
            EnsureWorkerIdle();
        if (VoxelLighting_SetSun(sun->dx, sun->dz, sun->strength, sun->key))
        {
            sWorldEpoch++;
            VitaPerf_MarkEvent(VITA_PERF_E_SUN);
        }
    }

    // Resets of the lighting and building caches, and the world epoch: the worker must be done.
    if (liveChanged || crossed || mapChanged)
        EnsureWorkerIdle();
    if (liveChanged || crossed)
        sWorldEpoch++;
    if (mapChanged || liveChanged || crossed)
    {
        VitaPerf_SubBegin(VITA_PERF_T_MAP_SHIFT);
        VoxelBuildings_WorldChanged();
        VoxelBuildings_MapChanged();
        VoxelLighting_Reset();
        VitaPerf_SubEnd(VITA_PERF_T_MAP_SHIFT);
    }
    
    f->indoor = indoor;
    f->liveChanged = liveChanged;
}

// Stage 5: screen fade, weather and the light of the frame.
static void Frame_ReadAtmosphere(VoxelFrame *const f)
{
    f->fadeAmount = 0.0f;
    f->fadeRgb[0] = 0.0f;
    f->fadeRgb[1] = 0.0f;
    f->fadeRgb[2] = 0.0f;
    VoxelWorld_ScreenFade(&f->fadeAmount, f->fadeRgb);
    f->drawFade = (f->fly.active && f->fly.frame.noFade) ? 0.0f : f->fadeAmount;

    const float targetWeatherAmt = VoxelWorld_WeatherAmount();
    const float targetFogDensity = VoxelWorld_FogDensity();
    f->weather = VoxelWorld_Weather();
    f->bolt = VoxelWorld_Lightning();
    f->underground = VoxelWorld_Underground();
    const VoxelLight targetLight = LightFor(f->indoor, f->weather, targetWeatherAmt, f->bolt);

    static uint64_t sVoxelAtmosphereStart = 0;
    if (sVoxelAtmosphereStart == 0)
        sVoxelAtmosphereStart = SDL_GetPerformanceCounter();
    f->clock = (double)(SDL_GetPerformanceCounter() - sVoxelAtmosphereStart) / (double)SDL_GetPerformanceFrequency();
    f->hd2dPreset = GetHd2dPreset();
    f->hd2dMarkedGrade = Hd2dPost(f->hd2dPreset);
    {
        // Read once: the windows, the lamps and the light on characters agree within the frame.
        const VitaDayNightLight *const dn = VitaDayNight_Get();
        f->nightGlow = (!f->indoor && !f->inBattle3D && dn != NULL && VitaGame_IsOutdoor())
                     ? Voxel_ClampF(dn->glow, 0.0f, 1.0f) : 0.0f;
    }

    // Smooth zone-crossing & weather transitions in Core 0 (snaps on warp fades / indoor switches)
    static bool sAtmosSmoothReady = false;
    static bool sSmoothedIndoor = false;
    static VoxelLight sSmoothedLight;
    static float sSmoothedFog = 0.0f;
    static float sSmoothedWeather = 0.0f;
    static double sPrevAtmosClock = 0.0;

    if (!sAtmosSmoothReady || f->fadeAmount >= 0.99f || f->indoor != sSmoothedIndoor)
    {
        sSmoothedLight = targetLight;
        sSmoothedFog = targetFogDensity;
        sSmoothedWeather = targetWeatherAmt;
        sSmoothedIndoor = f->indoor;
        sAtmosSmoothReady = true;
    }
    else
    {
        float dt = (float)(f->clock - sPrevAtmosClock);
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.10f) dt = 0.10f;
        const float k = 1.0f - expf(-dt / 0.40f);
        for (int32_t c = 0; c < 3; ++c)
        {
            sSmoothedLight.sun[c]     += (targetLight.sun[c]     - sSmoothedLight.sun[c])     * k;
            sSmoothedLight.shade[c]   += (targetLight.shade[c]   - sSmoothedLight.shade[c])   * k;
            sSmoothedLight.hazeRgb[c] += (targetLight.hazeRgb[c] - sSmoothedLight.hazeRgb[c]) * k;
        }
        sSmoothedLight.haze       += (targetLight.haze       - sSmoothedLight.haze)       * k;
        sSmoothedLight.dappleLow  += (targetLight.dappleLow  - sSmoothedLight.dappleLow)  * k;
        sSmoothedLight.dappleHigh += (targetLight.dappleHigh - sSmoothedLight.dappleHigh) * k;
        sSmoothedLight.rays       += (targetLight.rays       - sSmoothedLight.rays)       * k;
        sSmoothedLight.bloom      += (targetLight.bloom      - sSmoothedLight.bloom)      * k;
        sSmoothedLight.motes      += (targetLight.motes      - sSmoothedLight.motes)      * k;
        sSmoothedFog              += (targetFogDensity       - sSmoothedFog)              * k;
        sSmoothedWeather          += (targetWeatherAmt       - sSmoothedWeather)          * k;
    }
    sPrevAtmosClock = f->clock;
    f->light = sSmoothedLight;
    f->fogDensity = sSmoothedFog;
    f->weatherAmount = sSmoothedWeather;
}

// Everything animated reads the clock through what this derives, once per frame and after the
// menu freeze. Every shader runs in half precision (vglUseLowPrecision in MainThread_Boot), so the
// shaders get phases wrapped here in double, never the raw clock, which would stop resolving a
// frame's step within minutes.
static void Frame_DeriveClock(VoxelFrame *const f)
{
    // The foliage sway's speeds, in the order of uSwayA.xyzw then uSwayB (InitTreeSwayShader).
    static const double kSwaySpeed[5] = { 1.7, 0.9, 0.35, 2.3, 3.7 };
    f->seconds = (float)f->clock;
    for (int32_t i = 0; i < 5; ++i)
        f->swayPhase[i] = (float)AtmoWrapD(f->clock * kSwaySpeed[i], ATMO_TWO_PI);
    // The shafts move slower by night (pass 1).
    VoxelHd2dShader_ShaftClockAt(f->clock, 1.0f, &f->shaftClock[0]);
    VoxelHd2dShader_ShaftClockAt(f->clock, 0.45f, &f->shaftClock[1]);
}

// Whether a visible chunk must be rebuilt. `urgent`: drawn as it is, the chunk would be wrong,
// not just dated (no geometry, or texture coordinates into an atlas that was recomposed or
// extended since). `hash` is set, and `hashValid`, when the content hash had to be computed.
static bool ChunkNeedsRebuild(VoxelChunkSlot *const slot, const int32_t cx0, const int32_t cy0, const bool indoor,
                              int32_t *const hashBudget, bool *const urgent, uint32_t *const hash, bool *const hashValid)
{
    *urgent = false;
    if (!slot || !slot->valid)
    {
        *urgent = true;
        return true;
    }
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        if (slot->usedAtlasMask & (1u << s))
        {
            if (!sAtlasSlots[s].valid || slot->atlasGen[s] != sAtlasSlots[s].generation)
            {
                *urgent = true;
                return true;
            }
            if (slot->uncovered && slot->atlasExt[s] != sAtlasSlots[s].extension)
            {
                *urgent = true;
                return true;
            }
        }
    }
    // The hash is only compared once per world epoch (sWorldEpoch): a chunk that still matches
    // is marked checked; one that does not keeps being compared until it is rebuilt, also if it
    // was off screen when the change happened. Non-urgent epoch verification is amortized by
    // hashBudget so a zone crossing or sun tick does not hash all visible chunks in one frame.
    if (slot->checkedEpoch != sWorldEpoch)
    {
        if (hashBudget != NULL && *hashBudget <= 0)
            return false;
        if (hashBudget != NULL)
            (*hashBudget)--;
        *hash = ChunkContentHash(cx0, cy0, indoor);
        *hashValid = true;
        if (slot->hash != *hash)
            return true;
        slot->checkedEpoch = sWorldEpoch;
    }
    return false;
}

// With the worker, non-hole chunks are offloaded to Core 2 in adaptive batches (2 during normal
// traversal, 4 when holes exist or during Fly), prioritized: holes nearest camera target,
// pre-warm chunks about to enter the view, dated chunks nearest the player, and lookahead chunks.
// While an atlas slot is being extended (sExtendSlot >= 0), dispatches wait so Core 0 never
// stalls behind a worker touching the same atlas maps.
// Build visible holes nearest to the camera target first so rapid camera pullbacks fill
// from the center outward. Any holes left after budget exhaustion remain in q->hole* so
// BuildQueue_SendToWorker can hand them to Core 2 immediately.
static int32_t BuildQueue_BuildHoles(VoxelBuildQueue *const q, VoxelChunkSlot **const visibleChunks,
                                     int32_t *const visibleChunkCount)
{
    // If Core 2 is already computing a batch in parallel, never stall Core 0 with EnsureWorkerIdle()
    // only to rebuild the same hole on Core 0 and drop Core 2's finished work next frame.
    if (!sSyncBuildingLoads && sWorkerBusyThisFrame && ChunkWorker_Available())
        return q->holeCount;

    while (q->holeCount > 0)
    {
        if (!sSyncBuildingLoads && !ChunkBuildFits(q->frameStartMs, q->budgetMs, q->built, true))
            break;

        int32_t best = 0;
        float bestDist = FLT_MAX;
        for (int32_t p = 0; p < q->holeCount; p++)
        {
            const float dx = (float)(sChunkOriginX + q->holeCx[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->targetX;
            const float dz = (float)(sChunkOriginY + q->holeCy[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->targetZ;
            const float dist = dx * dx + dz * dz;
            if (dist < bestDist)
            {
                bestDist = dist;
                best = p;
            }
        }

        const int32_t cx = q->holeCx[best];
        const int32_t cy = q->holeCy[best];
        uint32_t hash = q->holeHash[best];
        const bool hashValid = q->holeHashValid[best];
        const int32_t last = q->holeCount - 1;
        q->holeCx[best] = q->holeCx[last];
        q->holeCy[best] = q->holeCy[last];
        q->holeHash[best] = q->holeHash[last];
        q->holeHashValid[best] = q->holeHashValid[last];
        q->holeCount = last;

        const int32_t cx0 = sChunkOriginX + cx * CHUNK_SIZE;
        const int32_t cy0 = sChunkOriginY + cy * CHUNK_SIZE;
        if (!hashValid)
            hash = ChunkContentHash(cx0, cy0, q->indoor);

        VoxelChunkSlot *slot = FindChunkSlot(cx, cy);
        if (!slot)
            slot = GetAvailableChunkSlot(cx, cy);
        EnsureWorkerIdle();
        TimedBuildChunk(slot, cx, cy, hash);
        q->built++;

        if (slot != NULL && slot->valid)
        {
            slot->lastUsedFrame = sFrameCounter;
            for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
            {
                if (slot->usedAtlasMask & (1u << s))
                    sAtlasSlots[s].stamp = sFrameCounter;
            }
            if (*visibleChunkCount < MAX_CHUNKS)
                visibleChunks[(*visibleChunkCount)++] = slot;
        }
    }
    return q->holeCount;
}

static void BuildQueue_SendToWorker(VoxelBuildQueue *const q, const bool flyActive)
{
    if (!ChunkWorker_Available() || sSyncBuildingLoads)
        return;
    if (sExtendSlot >= 0)
    {
        q->holeCount = 0;
        q->staleCount = 0;
        q->prewarmCount = 0;
        VitaPerf_AddCount(VITA_PERF_C_WORKER_HELD, 1);
        return;
    }

    if (sChunkJobState == CHUNK_JOB_NONE)
    {
        int32_t batchCx[CHUNK_WORKER_BATCH];
        int32_t batchCy[CHUNK_WORKER_BATCH];
        uint32_t batchHash[CHUNK_WORKER_BATCH];
        int32_t batchCount = 0;
        // Adaptive batch size: 4 chunks during Fly or when visible holes exist (max throughput),
        // 2 chunks during normal traversal so Core 2 finishes within ~1 frame and rarely stalls
        // Core 0 on sudden map crossings or tile animations.
        const int32_t maxBatch = (q->holeCount > 0 || flyActive)
                               ? CHUNK_WORKER_BATCH
                               : CHUNK_WORKER_BATCH_NORMAL;

        // Pass 0: visible holes nearest to camera target (highest priority).
        // Pass 1: pre-warm chunks about to enter the view.
        // Pass 2: dated chunks nearest to the player.
        // Pass 3: pre-warm chunks nearest to the lookahead point.
        for (int32_t pass = 0; pass < 4 && batchCount < maxBatch; pass++)
        {
            while (batchCount < maxBatch)
            {
                const int32_t count = (pass == 0) ? q->holeCount
                                    : (pass == 2) ? q->staleCount
                                    : q->prewarmCount;
                int32_t bestIdx = -1;
                float bestDist = FLT_MAX;
                for (int32_t p = 0; p < count; p++)
                {
                    if (pass == 1 && !q->prewarmSoon[p])
                        continue;
                    const int32_t cx = (pass == 0) ? q->holeCx[p]
                                     : (pass == 2) ? q->staleCx[p]
                                     : q->prewarmCx[p];
                    const int32_t cy = (pass == 0) ? q->holeCy[p]
                                     : (pass == 2) ? q->staleCy[p]
                                     : q->prewarmCy[p];
                    bool dup = false;
                    for (int32_t b = 0; b < batchCount; ++b)
                    {
                        if (batchCx[b] == cx && batchCy[b] == cy)
                        {
                            dup = true;
                            break;
                        }
                    }
                    if (dup)
                        continue;
                    const float aimX = (pass == 0) ? q->targetX
                                     : (pass == 2) ? q->playerX
                                     : q->lookX;
                    const float aimZ = (pass == 0) ? q->targetZ
                                     : (pass == 2) ? q->playerZ
                                     : q->lookZ;
                    const float dx = (float)(sChunkOriginX + cx * CHUNK_SIZE + CHUNK_SIZE / 2) - aimX;
                    const float dz = (float)(sChunkOriginY + cy * CHUNK_SIZE + CHUNK_SIZE / 2) - aimZ;
                    const float dist = dx * dx + dz * dz;
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestIdx = p;
                    }
                }
                if (bestIdx < 0)
                    break;
                const int32_t cx = (pass == 0) ? q->holeCx[bestIdx]
                                 : (pass == 2) ? q->staleCx[bestIdx]
                                 : q->prewarmCx[bestIdx];
                const int32_t cy = (pass == 0) ? q->holeCy[bestIdx]
                                 : (pass == 2) ? q->staleCy[bestIdx]
                                 : q->prewarmCy[bestIdx];
                const int32_t cx0 = sChunkOriginX + cx * CHUNK_SIZE;
                const int32_t cy0 = sChunkOriginY + cy * CHUNK_SIZE;
                batchCx[batchCount] = cx;
                batchCy[batchCount] = cy;
                batchHash[batchCount] = (pass == 0 && q->holeHashValid[bestIdx])
                                      ? q->holeHash[bestIdx]
                                      : ChunkContentHash(cx0, cy0, q->indoor);
                batchCount++;
            }
        }
        if (batchCount > 0)
            ChunkWorker_ChooseBatch(batchCx, batchCy, batchHash, batchCount);
    }
    q->holeCount = 0;
    q->staleCount = 0;
    q->prewarmCount = 0;
}

// Visible chunks that are only dated, nearest to the player first, while the frame has room.
// Those left keep their old checkedEpoch and are listed again next frame.
static void BuildQueue_BuildStale(VoxelBuildQueue *const q)
{
    while (q->staleCount > 0)
    {
        int32_t best = 0;
        float bestDist = FLT_MAX;
        for (int32_t p = 0; p < q->staleCount; p++)
        {
            const float dx = (float)(sChunkOriginX + q->staleCx[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->playerX;
            const float dz = (float)(sChunkOriginY + q->staleCy[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->playerZ;
            const float dist = dx * dx + dz * dz;
            if (dist < bestDist)
            {
                bestDist = dist;
                best = p;
            }
        }
        if (!ChunkBuildFits(q->frameStartMs, q->budgetMs, q->built, false))
            break;
        const int32_t cx = q->staleCx[best];
        const int32_t cy = q->staleCy[best];
        q->staleCx[best] = q->staleCx[q->staleCount - 1];
        q->staleCy[best] = q->staleCy[q->staleCount - 1];
        q->staleCount--;

        VoxelChunkSlot *slot = FindChunkSlot(cx, cy);
        if (slot != NULL && slot->valid)
        {
            const int32_t cx0 = sChunkOriginX + cx * CHUNK_SIZE;
            const int32_t cy0 = sChunkOriginY + cy * CHUNK_SIZE;
            const uint32_t hash = ChunkContentHash(cx0, cy0, q->indoor);
            EnsureWorkerIdle();
            TimedBuildChunk(slot, cx, cy, hash);
            q->built++;
        }
    }
}

// Unbuilt chunks outside the view, nearest to the lookahead point first, while the frame has
// room for another build.
static void BuildQueue_Prewarm(VoxelBuildQueue *const q)
{
    while (q->prewarmCount > 0)
    {
        int32_t best = 0;
        float bestDist = FLT_MAX;
        for (int32_t p = 0; p < q->prewarmCount; p++)
        {
            const float dx = (float)(sChunkOriginX + q->prewarmCx[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->lookX;
            const float dz = (float)(sChunkOriginY + q->prewarmCy[p] * CHUNK_SIZE + CHUNK_SIZE / 2) - q->lookZ;
            // Chunks about to enter the view go before any other.
            const float dist = dx * dx + dz * dz - (q->prewarmSoon[p] ? 1.0e6f : 0.0f);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = p;
            }
        }
        if (!sSyncBuildingLoads
            && !ChunkBuildFits(q->frameStartMs, q->budgetMs, q->built,
                               // a chunk about to enter the view is guaranteed only as the frame's
                               // first build: two mountain chunks made 40 ms frames (SESION 22)
                               q->prewarmSoon[best] && q->built == 0))
            break;
        const int32_t cx = q->prewarmCx[best];
        const int32_t cy = q->prewarmCy[best];
        q->prewarmCx[best] = q->prewarmCx[q->prewarmCount - 1];
        q->prewarmCy[best] = q->prewarmCy[q->prewarmCount - 1];
        q->prewarmSoon[best] = q->prewarmSoon[q->prewarmCount - 1];
        q->prewarmCount--;

        VoxelChunkSlot *slot = FindChunkSlot(cx, cy);
        if (!slot || !slot->valid)
        {
            const int32_t cx0 = sChunkOriginX + cx * CHUNK_SIZE;
            const int32_t cy0 = sChunkOriginY + cy * CHUNK_SIZE;
            const uint32_t hash = ChunkContentHash(cx0, cy0, q->indoor);

            slot = GetAvailableChunkSlot(cx, cy);
            EnsureWorkerIdle();
            TimedBuildChunk(slot, cx, cy, hash);
            q->built++;
        }
    }
}

// Stage 6: the chunks around the camera target. Visible ones are frustum-culled into
// f->visible; holes are built now, the rest queued for the worker or the frame's spare time.
static void Frame_SelectChunks(VoxelFrame *const f)
{
    const bool indoor = f->indoor;
    const bool fullLayout = indoor || (gMapHeader.mapType == MAP_TYPE_UNDERGROUND);
    const VoxelMapInstance *const inst0 = f->inst0;

    // Single-source camera matrices, dynamic zNear/zFar, projected bounds, and frustum planes.
    ComputeViewAndFrustum();

    // The chunk the worker computed since the last frame, if the world it was computed in
    // still stands (after this frame's map change, epoch and atlas updates).
    ChunkWorker_Commit();

    const float camTargetX = sCamera.targetX;
    const float camTargetZ = sCamera.targetZ;

    // Indoors and caves scan the full layout (clamped to VOXEL_VIEW_REACH_MAX) so tall gym
    // walls and rock ceilings are never clipped; outdoors scan the camera's projected trapezoid.
    int32_t minTileY, maxTileY;
    int32_t layoutMinTileX = 0, layoutMaxTileX = 0;
    if (fullLayout && inst0 != NULL)
    {
        const float loZ = camTargetZ - VOXEL_VIEW_REACH_MAX;
        const float hiZ = camTargetZ + VOXEL_VIEW_REACH_MAX;
        const float loX = camTargetX - VOXEL_VIEW_REACH_MAX;
        const float hiX = camTargetX + VOXEL_VIEW_REACH_MAX;
        const float z0 = fmaxf((float)(inst0->originY - CHUNK_SIZE), loZ);
        const float z1 = fminf((float)(inst0->originY + inst0->height + CHUNK_SIZE), hiZ);
        const float x0 = fmaxf((float)(inst0->originX - CHUNK_SIZE), loX);
        const float x1 = fminf((float)(inst0->originX + inst0->width + CHUNK_SIZE), hiX);
        minTileY = (int32_t)floorf(z0);
        maxTileY = (int32_t)ceilf(z1);
        layoutMinTileX = (int32_t)floorf(x0);
        layoutMaxTileX = (int32_t)ceilf(x1);
    }
    else
    {
        minTileY = (int32_t)floorf(sViewState.minZ);
        maxTileY = (int32_t)ceilf(sViewState.maxZ);
    }

    int32_t minChunkY = FloorDivChunk(minTileY - sChunkOriginY);
    int32_t maxChunkY = FloorDivChunk(maxTileY - sChunkOriginY);

    VoxelChunkSlot **const visibleChunks = f->visible;
    int32_t visibleChunkCount = 0;
    static VoxelBuildQueue q;
    q.frameStartMs = f->startMs;
    q.built = 0;
    q.indoor = indoor;
    q.playerX = f->playerX;
    q.playerZ = f->playerZ;
    q.targetX = camTargetX;
    q.targetZ = camTargetZ;
    q.holeCount = 0;
    q.prewarmCount = 0;
    q.staleCount = 0;

    const bool flyActive = f->fly.active;
    if (flyActive)
        sWarmupFrames = 0;
    const bool isWarmingUp = !flyActive && (sWarmupFrames > 0 || sNeedsFullWarmup);
    if (sWarmupFrames > 0)
    {
        sWarmupFrames--;
        if (sWarmupFrames == 0)
            VitaPerf_LogMemory("map warm-up done");
    }
    sSyncBuildingLoads = !flyActive && (isWarmingUp || f->fadeAmount >= 0.99f);
    if (isWarmingUp)
        VitaPerf_MarkEvent(VITA_PERF_E_WARMUP);
    else if (sSyncBuildingLoads)
        VitaPerf_MarkEvent(VITA_PERF_E_SYNC);
    // Builds are limited by time (ChunkBuildFits), except while warming up or faded out.
    q.budgetMs = (Platform_GetSetting(PLATFORM_SETTING_VOXEL_FPS) == 0) ? (1000.0 / 30.0) : (1000.0 / 60.0);

    // Where pre-warming aims: ahead of the player along the recent movement.
    UpdateMovementDirection(f->playerX, f->playerZ, f->crossed || f->mapChanged);
    q.lookX = f->playerX;
    q.lookZ = f->playerZ;
    MovementLookahead(&q.lookX, &q.lookZ);

    // The candidate area reaches 1 chunk (or 2 chunks at Mach Bike speed) further on the side
    // the player is heading to, so chunks about to enter the view are built before they do.
    // That extra band is only ever pre-warmed, never drawn: the drawn set (baseMin/Max) stays
    // independent of movement speed.
    const float moveSpeed = sqrtf(sMoveVelX * sMoveVelX + sMoveVelZ * sMoveVelZ);
    const bool moving = moveSpeed > VOXEL_MOVE_THRESHOLD;
    const int32_t prewarmTiles = (moveSpeed >= VOXEL_FAST_MOVE_SPEED) ? (2 * CHUNK_SIZE) : CHUNK_SIZE;
    const int32_t baseMinChunkY = minChunkY;
    const int32_t baseMaxChunkY = maxChunkY;
    const int32_t baseMinTileY = minTileY;
    const int32_t baseMaxTileY = maxTileY;
    if (!fullLayout)
    {
        if (sMoveVelZ < -VOXEL_MOVE_THRESHOLD) minTileY -= prewarmTiles;
        if (sMoveVelZ > VOXEL_MOVE_THRESHOLD) maxTileY += prewarmTiles;
        minChunkY = FloorDivChunk(minTileY - sChunkOriginY);
        maxChunkY = FloorDivChunk(maxTileY - sChunkOriginY);
    }
    const int32_t extraLeft = (!fullLayout && sMoveVelX < -VOXEL_MOVE_THRESHOLD) ? prewarmTiles : 0;
    const int32_t extraRight = (!fullLayout && sMoveVelX > VOXEL_MOVE_THRESHOLD) ? prewarmTiles : 0;

    const float aheadX = moving ? sMoveVelX / moveSpeed * VOXEL_VIEW_AHEAD_TILES : 0.0f;
    const float aheadZ = moving ? sMoveVelZ / moveSpeed * VOXEL_VIEW_AHEAD_TILES : 0.0f;
    int32_t epochHashBudget = (f->liveChanged || f->crossed || sSyncBuildingLoads)
                            ? MAX_CHUNKS
                            : VOXEL_EPOCH_HASH_BUDGET;

    // Visible frustum chunks: drawn, and holes queued for center-out building
    VitaPerf_MarkCullStart();
    int32_t candidateChunks = 0;
    // Outdoors the scan also reaches a few chunks west and north of the candidate area: a
    // building model stands in the chunk of its north-west cell and may reach into the view
    // from there. Those chunks are only drawn (never built or pre-warmed from here).
    const int32_t reachY = fullLayout ? 0 : VOXEL_BUILDING_REACH_CHUNKS_Y;
    const int32_t reachX = fullLayout ? 0 : VOXEL_BUILDING_REACH_CHUNKS_X;
    for (int32_t cy = maxChunkY; cy >= minChunkY - reachY; cy--)
    {
        int32_t rowMinTileX, rowMaxTileX;
        if (fullLayout)
        {
            rowMinTileX = layoutMinTileX;
            rowMaxTileX = layoutMaxTileX;
        }
        else
        {
            const float rowZ0 = (float)(sChunkOriginY + cy * CHUNK_SIZE);
            const float rowZ1 = rowZ0 + (float)CHUNK_SIZE;
            float rMinX, rMaxX;
            Frustum_RowBoundsX(rowZ0, rowZ1, &rMinX, &rMaxX);
            rowMinTileX = (int32_t)floorf(rMinX);
            rowMaxTileX = (int32_t)ceilf(rMaxX);
        }
        const int32_t baseMinChunkX = FloorDivChunk(rowMinTileX - sChunkOriginX);
        const int32_t baseMaxChunkX = FloorDivChunk(rowMaxTileX - sChunkOriginX);
        const int32_t minChunkX = FloorDivChunk(rowMinTileX - extraLeft - sChunkOriginX);
        const int32_t maxChunkX = FloorDivChunk(rowMaxTileX + extraRight - sChunkOriginX);
        const bool rowInBase = (cy >= baseMinChunkY && cy <= baseMaxChunkY);

        for (int32_t cx = minChunkX - reachX; cx <= maxChunkX; cx++)
        {
            VoxelChunkSlot *slot = FindChunkSlot(cx, cy);
            // whether its building models reach into this row's window (the base area, not
            // the movement's extra band, so the drawn set stays independent of the speed)
            const bool modelsReach = !fullLayout
                && ChunkModelsReachWindow(slot, cx, cy, (float)rowMinTileX, (float)rowMaxTileX,
                                          (float)baseMinTileY, (float)baseMaxTileY);
            if (cy < minChunkY || cx < minChunkX)
            {
                // outside the candidate area: drawn only for its models
                if (!modelsReach || !Frustum_ChunkInView(cx, cy, slot))
                    continue;
            }
            else
                candidateChunks++;
            const bool inBase = (rowInBase && cx >= baseMinChunkX && cx <= baseMaxChunkX) || modelsReach;
            const bool inFrustum = inBase && Frustum_ChunkInView(cx, cy, slot);

            if (!inFrustum)
            {
                // Unbuilt chunk outside the view: pre-warm it. Those the view reaches within
                // VOXEL_VIEW_AHEAD_TILES of movement come first and count as holes.
                if ((!slot || !slot->valid) && q.prewarmCount < VOXEL_PREFETCH_MAX)
                {
                    q.prewarmCx[q.prewarmCount] = cx;
                    q.prewarmCy[q.prewarmCount] = cy;
                    q.prewarmSoon[q.prewarmCount] = moving && !fullLayout
                        && Frustum_ChunkInViewAhead(cx, cy, aheadX, aheadZ);
                    q.prewarmCount++;
                }
                continue;
            }

            const int32_t cx0 = sChunkOriginX + cx * CHUNK_SIZE;
            const int32_t cy0 = sChunkOriginY + cy * CHUNK_SIZE;
            uint32_t hash = 0;
            bool hashValid = false;
            bool urgent = false;
            const bool needRebuild = ChunkNeedsRebuild(slot, cx0, cy0, indoor, &epochHashBudget, &urgent, &hash, &hashValid);

            if (needRebuild && !urgent && !sSyncBuildingLoads)
            {
                // Only dated (sun moved, tile changed): rebuilt after this loop, nearest to the
                // player first, so a change of light spreads out from where the player looks.
                if (q.staleCount < VOXEL_STALE_MAX)
                {
                    q.staleCx[q.staleCount] = cx;
                    q.staleCy[q.staleCount] = cy;
                    q.staleCount++;
                }
            }
            else if (needRebuild)
            {
                // Urgent hole or outdated atlas: queue for radial center-out building after the scan.
                if (q.holeCount < MAX_CHUNKS)
                {
                    q.holeCx[q.holeCount] = cx;
                    q.holeCy[q.holeCount] = cy;
                    q.holeHash[q.holeCount] = hash;
                    q.holeHashValid[q.holeCount] = hashValid;
                    q.holeCount++;
                }
                continue;
            }

            // If chunk has valid geometry, draw it!
            if (slot != NULL && slot->valid)
            {
                slot->lastUsedFrame = sFrameCounter;
                // The atlas slots it is textured from are in use: the LRU must not recompose
                // them for another tileset while it is on screen (EnsureAtlasSlotForInstance).
                for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
                {
                    if (slot->usedAtlasMask & (1u << s))
                        sAtlasSlots[s].stamp = sFrameCounter;
                }
                if (visibleChunkCount < MAX_CHUNKS)
                {
                    visibleChunks[visibleChunkCount++] = slot;
                }
            }
        }
    }

    // During Fly with Core 2 available, offload all chunk builds to Core 2 (4-chunk batches)
    // so Core 0 never stalls or drops frames rendering clouds/HD-2D.
    int32_t holesLeft;
    if (flyActive && ChunkWorker_Available())
    {
        holesLeft = q.holeCount;
        BuildQueue_SendToWorker(&q, true);
    }
    else
    {
        holesLeft = BuildQueue_BuildHoles(&q, visibleChunks, &visibleChunkCount);
        BuildQueue_SendToWorker(&q, false);
        BuildQueue_BuildStale(&q);
        BuildQueue_Prewarm(&q);
    }

    VitaPerf_AddCount(VITA_PERF_C_HOLES, (uint32_t)holesLeft);
    sNeedsFullWarmup = false;
    VitaPerf_MarkCullEnd(candidateChunks, visibleChunkCount);
    f->visibleCount = visibleChunkCount;
    f->holesLeft = holesLeft;
}

// What the night lamps cost at each HD-2D level (indexed by Hd2dPreset): the same look, cheaper
// below. The halo is the only glow without the post pass (OFF, LOW); with it the bloom adds its own.
typedef struct
{
    float haloSize;         // halo radius, in tiles
    float haloAlpha;        // halo strength at full glow
    uint8_t maxLights;      // lamps lighting the characters (VOXEL_POINTLIGHT_MAX at most)
    bool perVertex;         // characters lit at each corner (feet to head) rather than once a card
} LampLevel;

static const LampLevel kLampLevels[] = {
    [HD2D_PRESET_OFF]  = { 0.50f, 0.85f, 4,  false },
    [HD2D_PRESET_LOW]  = { 0.60f, 0.85f, 8,  true },
    [HD2D_PRESET_MED]  = { 0.55f, 0.55f, 12, true },
    [HD2D_PRESET_HIGH] = { 0.50f, 0.40f, 16, true },
};

static inline const LampLevel *LampLevelOf(const VoxelFrame *const f)
{
    return &kLampLevels[(uint32_t)f->hd2dPreset < 4u ? (uint32_t)f->hd2dPreset : (uint32_t)HD2D_PRESET_HIGH];
}

// The light point lights for this frame's characters: the cave lantern (HD-2D) and, at night, the
// lamps of the visible chunks nearest the camera's target (VoxelLamps_EmitChunk), steady.
static void UpdatePointLights(const VoxelFrame *const f)
{
    VoxelPointLights_Clear();
    if (f->inBattle3D)
        return;

    if (f->underground && f->hd2dPreset != HD2D_PRESET_OFF)
    {
        // Warm explorer lantern in dark caves
        VoxelPointLights_Add(f->playerX + 0.5f, sCamera.ground + 0.9f, f->playerZ + 0.5f,
                             1.0f, 0.78f, 0.48f, 4.8f, 0.75f, true, 0xCA7E11u);
    }

    if (f->nightGlow > 0.02f)
    {
        // The nearest maxLights lamps, kept sorted by distance (a handful: insertion is enough).
        enum { CAP = VOXEL_POINTLIGHT_MAX };
        float nearX[CAP], nearY[CAP], nearZ[CAP], nearD2[CAP];
        uint32_t nearCount = 0;
        const uint32_t maxLights = LampLevelOf(f)->maxLights < CAP ? LampLevelOf(f)->maxLights : CAP;
        for (int32_t c = 0; c < f->visibleCount; c++)
        {
            const VoxelChunkSlot *const chunk = f->visible[c];
            for (uint32_t l = 0; l < chunk->lampLightCount; l++)
            {
                const float x = chunk->lampLights[l][0] + chunk->drawOffsetX;
                const float z = chunk->lampLights[l][2] + chunk->drawOffsetZ;
                const float dx = x - sCamera.targetX, dz = z - sCamera.targetZ;
                const float d2 = dx * dx + dz * dz;
                if (nearCount == maxLights && d2 >= nearD2[nearCount - 1])
                    continue;
                uint32_t at = nearCount < maxLights ? nearCount++ : nearCount - 1;
                while (at > 0 && nearD2[at - 1] > d2)
                {
                    nearX[at] = nearX[at - 1];
                    nearY[at] = nearY[at - 1];
                    nearZ[at] = nearZ[at - 1];
                    nearD2[at] = nearD2[at - 1];
                    --at;
                }
                nearX[at] = x;
                nearY[at] = chunk->lampLights[l][1];
                nearZ[at] = z;
                nearD2[at] = d2;
            }
        }
        for (uint32_t i = 0; i < nearCount; i++)
            VoxelPointLights_Add(nearX[i], nearY[i], nearZ[i], VOXEL_LAMP_R, VOXEL_LAMP_G, VOXEL_LAMP_B,
                                 VOXEL_LAMP_LIGHT_RADIUS, f->nightGlow * VOXEL_LAMP_LIGHT_GAIN, false, 0u);
    }

    VoxelPointLights_BeginFrame(f->seconds);
}

// The point lights on the packed character cards (6 vertices each), as copies of the lit cards
// whose colour is the light (sEntityLitVerts, drawn by Pass_EntityLamps). It cannot go into the
// cards' own colour: that is already near white, and the night veil drawn after them would dim
// whatever was added.
static void LightCards(const VoxelFrame *const f, const VoxelGlVertex *const verts, const uint32_t count)
{
    const bool perVertex = LampLevelOf(f)->perVertex;
    sEntityLitCount = 0;
    for (uint32_t i = 0; i + 5 < count; i += 6)
    {
        float light[6][3];
        bool lit = false;
        if (!perVertex)
        {
            const float cx = (verts[i].x + verts[i + 2].x) * 0.5f;
            const float cy = (verts[i].y + verts[i + 2].y) * 0.5f;
            const float cz = (verts[i].z + verts[i + 2].z) * 0.5f;
            VoxelPointLights_SampleAdd(cx, cy, cz, &light[0][0], &light[0][1], &light[0][2]);
            for (uint32_t k = 1; k < 6; ++k)
                for (uint32_t ch = 0; ch < 3; ch++)
                    light[k][ch] = light[0][ch];
        }
        else
        {
            for (uint32_t k = 0; k < 6; ++k)
                VoxelPointLights_SampleAdd(verts[i + k].x, verts[i + k].y, verts[i + k].z,
                                           &light[k][0], &light[k][1], &light[k][2]);
        }
        for (uint32_t k = 0; k < 6 && !lit; ++k)
            lit = light[k][0] + light[k][1] + light[k][2] > 0.01f;
        if (!lit)
            continue;
        for (uint32_t k = 0; k < 6; ++k)
        {
            VoxelGlVertex *const v = &sEntityLitVerts[sEntityLitCount++];
            *v = verts[i + k];
            v->r = (uint8_t)Voxel_ClampF(light[k][0] * 255.0f + 0.5f, 0.0f, 255.0f);
            v->g = (uint8_t)Voxel_ClampF(light[k][1] * 255.0f + 0.5f, 0.0f, 255.0f);
            v->b = (uint8_t)Voxel_ClampF(light[k][2] * 255.0f + 0.5f, 0.0f, 255.0f);
            v->a = 255;
        }
    }
}

// Stage 7: entity billboards, their cast shadows and water reflections (field only).
static void Frame_EmitEntities(const VoxelFrame *const f)
{
    UpdatePointLights(f);
    const bool inBattle3D = f->inBattle3D;
    const bool indoor = f->indoor;
    if (!inBattle3D)
    {
        VitaPerf_SubBegin(VITA_PERF_T_ENTITIES);
        VoxelBuilder entityBuilder, shadowBuilder, reflectionBuilder;
        VoxelBuilder_Init(&entityBuilder, sEntityVerts, MAX_ENTITY_VERTS);
        VoxelBuilder_Init(&shadowBuilder, sShadowVerts, MAX_SHADOW_VERTS);
        VoxelBuilder_Init(&reflectionBuilder, sReflectionVerts, MAX_REFLECTION_VERTS);
        // VoxelEntities_Emit already samples VoxelLighting_SampleAlone onto card->shade;
        // keeping entityBuilder.lighting false avoids double-darkening and 4 extra shadow rays per NPC!
        entityBuilder.lighting = false;
        entityBuilder.lightingRefine = false;

        // While the chunk worker owns the lighting caches the billboards keep their last shade.
        VoxelEntities_AllowLighting(!sWorkerBusyThisFrame);
        VitaPerf_SubBegin(VITA_PERF_T_ENT_EMIT);
        const uint32_t redecodedSlots = VoxelEntities_Emit(&entityBuilder, sSpriteAtlasMorton,
                                                           &sCamera, &shadowBuilder, &reflectionBuilder);
        VitaPerf_SubEnd(VITA_PERF_T_ENT_EMIT);
        uint32_t dirtySlots = VoxelEntities_TakeDirtySlots();
        if (sSpriteAtlasNeedsAll)
        {
            // A new texture: every slot the atlas already holds, not only those just decoded.
            dirtySlots = 0xFFFFFFFFu;
            sSpriteAtlasNeedsAll = false;
        }
        if ((redecodedSlots > 0 || dirtySlots != 0) && sSpriteTex != 0)
        {
            VitaPerf_SubBegin(VITA_PERF_T_ENT_ATLAS);
            WriteSpriteAtlasSlots(sSpriteAtlasMorton, sSpriteAtlasLinear, dirtySlots);
            VitaPerf_SubEnd(VITA_PERF_T_ENT_ATLAS);
        }

        VitaPerf_SubBegin(VITA_PERF_T_ENT_PACK);
        sEntityGlCount = entityBuilder.count;
        if (sEntityGlCount > 0)
        {
            PackVoxelVertices(sEntityVerts, sEntityGlVerts, sEntityGlCount, !indoor);
            sEntityLitCount = 0;
            if (VoxelPointLights_Count() > 0)
            {
                VitaPerf_SubEnd(VITA_PERF_T_ENT_PACK);
                VitaPerf_SubBegin(VITA_PERF_T_ENT_LIGHTS);
                LightCards(f, sEntityGlVerts, sEntityGlCount);
                VitaPerf_SubEnd(VITA_PERF_T_ENT_LIGHTS);
                VitaPerf_SubBegin(VITA_PERF_T_ENT_PACK);
            }
        }

        sShadowGlCount = shadowBuilder.count;
        if (sShadowGlCount > 0)
        {
            for (uint32_t i = 0; i < sShadowGlCount; i++)
            {
                sShadowGlVerts[i].x = sShadowVerts[i].x;
                sShadowGlVerts[i].y = sShadowVerts[i].y;
                sShadowGlVerts[i].z = sShadowVerts[i].z;
                sShadowGlVerts[i].u = sShadowVerts[i].u;
                sShadowGlVerts[i].v = sShadowVerts[i].v;
                // Emerald 3DS dark indigo-blue cast shadow color (0xFF281408u)
                sShadowGlVerts[i].r = 8;
                sShadowGlVerts[i].g = 20;
                sShadowGlVerts[i].b = 40;
                float a = sShadowVerts[i].shade;
                if (a < 0.0f) a = 0.0f;
                if (a > 1.0f) a = 1.0f;
                sShadowGlVerts[i].a = (uint8_t)(a * 255.0f);
            }
        }

        // Water reflections on water surface
        sReflectionGlCount = reflectionBuilder.count;
        if (sReflectionGlCount > 0)
        {
            for (uint32_t i = 0; i < sReflectionGlCount; i++)
            {
                sReflectionGlVerts[i].x = sReflectionVerts[i].x;
                sReflectionGlVerts[i].y = sReflectionVerts[i].y;
                sReflectionGlVerts[i].z = sReflectionVerts[i].z;
                sReflectionGlVerts[i].u = sReflectionVerts[i].u;
                sReflectionGlVerts[i].v = sReflectionVerts[i].v;
                float s = sReflectionVerts[i].shade;
                if (s < 0.0f) s = 0.0f;
                if (s > 1.0f) s = 1.0f;
                sReflectionGlVerts[i].r = (uint8_t)(s * 180.0f);
                sReflectionGlVerts[i].g = (uint8_t)(s * 210.0f);
                sReflectionGlVerts[i].b = (uint8_t)(s * 255.0f);
                sReflectionGlVerts[i].a = (f->hd2dPreset != HD2D_PRESET_OFF && !indoor && f->weather == VOXEL_WEATHER_RAIN) ? 185 : 140;
            }
        }
        VitaPerf_SubEnd(VITA_PERF_T_ENT_PACK);
        VitaPerf_SubEnd(VITA_PERF_T_ENTITIES);
    }
    else
    {
        sEntityGlCount = 0;
        sShadowGlCount = 0;
        sReflectionGlCount = 0;
    }
}

// Render target (the FBO when the depth of field is on), 3D camera and clear.
static void Frame_BeginPasses(VoxelFrame *const f, const GLuint defaultTargetFbo)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    const float fadeAmount = f->drawFade;
    const float *const fadeRgb = f->fadeRgb;
    const int32_t renderW = VitaPlatform_GetRenderWidth();
    const int32_t renderH = VitaPlatform_GetRenderHeight();

    const bool dioramaSetting = (VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLUR) != 0);
    const bool isIndoor = (inst0 != NULL && inst0->indoor);
    const bool hd2dOn = Hd2dPost(f->hd2dPreset);
    // The scene goes to the post FBO for the HD-2D post effects (bloom, sky bloom, shafts, grade)
    // or for the depth of field. The DoF option only decides the blurred bands: it used to gate
    // every post effect, so DoF OFF silently lost bloom, sky bloom and shafts.
    const bool dioramaActive = sDioramaFboReady && (fadeAmount < 0.99f)
                             && (hd2dOn || (dioramaSetting && !isIndoor));

    bool battleHalf = false;
    if (dioramaActive)
    {
        sPostTargetIndex ^= 1u;
        PostRenderTargets *const pt = &sPostTargets[sPostTargetIndex];
        // A 3D battle in HIGH: the stage is drawn at half size in the scene target's lower left and
        // read from there by the post pass (Pass_DepthOfField), with no downsample.
        battleHalf = f->inBattle3D && f->hd2dPreset == HD2D_PRESET_HIGH && pt->bandFbo != 0
                  && VoxelHd2dShader_PackReady() && VoxelHd2dShader_BandReady();
        glBindFramebuffer(GL_FRAMEBUFFER, pt->dioramaFbo);
        const GLuint wantTex = (f->hd2dPreset == HD2D_PRESET_HIGH) ? pt->dioramaColorTex : pt->dioramaColorTexFast;
        if (wantTex != 0 && wantTex != pt->dioramaActiveTex)
        {
            pt->dioramaActiveTex = wantTex;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pt->dioramaActiveTex, 0);
        }
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, defaultTargetFbo);
    }
    VitaPerf_GpuScene(VITA_PERF_GPU_WORLD);
    {
        const VitaDayNightLight *const dn = VitaDayNight_Get();
        const VitaPerfScene scene = {
            .hd2dLevel = (int32_t)f->hd2dPreset,
            .dofOn = dioramaSetting,
            .mapGroup = (inst0 != NULL) ? inst0->mapGroup : -1,
            .mapNum = (inst0 != NULL) ? inst0->mapNum : -1,
            .indoor = isIndoor,
            .weather = (int32_t)f->weather,
            .daylight = (dn != NULL) ? dn->daylight : 1.0f,
            .lamps = (f->nightGlow > 0.0f),
            .inBattle = f->inBattle3D,
            .faded = (fadeAmount >= 0.99f),
            .playerX = f->playerX,
            .playerZ = f->playerZ,
        };
        VitaPerf_SetScene(&scene);
    }

    // 8. OpenGL Setup & Hardware Render Passes
    f->battleHalf = battleHalf;
    f->worldW = battleHalf ? renderW / 2 : renderW;
    f->worldH = battleHalf ? renderH / 2 : renderH;
    glViewport(0, 0, f->worldW, f->worldH);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(sViewState.proj);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(sViewState.view);

    glDepthMask(GL_TRUE);
    {
        static bool sPrevClearDark = true;
        const bool isWhite = (fadeRgb[0] > 0.85f && fadeRgb[1] > 0.85f && fadeRgb[2] > 0.85f);
        const bool suppressWhite = (fadeAmount >= 0.99f && isWhite && (f->mapChanged || sPrevClearDark));
        const float cr = (fadeAmount >= 0.99f && !suppressWhite) ? fadeRgb[0] : 0.0f;
        const float cg = (fadeAmount >= 0.99f && !suppressWhite) ? fadeRgb[1] : 0.0f;
        const float cb = (fadeAmount >= 0.99f && !suppressWhite) ? fadeRgb[2] : 0.0f;
        sPrevClearDark = (fadeAmount < 0.50f || (!isWhite && fadeAmount >= 0.90f));
        glClearColor(cr, cg, cb, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // When the screen is completely faded out (100% black/white during a warp transition),
    // skip 3D geometry rasterization (Passes 1-5).
    // All visible chunks have ALREADY been built and warmed up in RAM above, guaranteeing ZERO
    // warmup hitch or stutter on fade-in while reducing GPU draw call load to 0 ms.
    // Upload at most 1 decoded building page per frame from Core 2 worker
    UploadReadyBuildingPage();
    
    f->renderW = renderW;
    f->renderH = renderH;
    f->dioramaActive = dioramaActive;

    // Project player target onto screen Y [0..1] (top=0, bottom=1) so DoF focal plane tracks the player
    {
        const float dx = sCamera.targetX - sCamera.x;
        const float dy = (sCamera.targetY + 0.6f) - sCamera.y;
        const float dz = sCamera.targetZ - sCamera.z;
        float fx = sCamera.targetX - sCamera.x;
        float fy = sCamera.targetY - sCamera.y;
        float fz = sCamera.targetZ - sCamera.z;
        const float flen = sqrtf(fx * fx + fy * fy + fz * fz);
        if (flen > 0.001f) { fx /= flen; fy /= flen; fz /= flen; }
        // Camera right = normalize(forward x (0,1,0)) = (-fz, 0, fx)
        float rx = -fz, rz = fx;
        const float rlen = sqrtf(rx * rx + rz * rz);
        if (rlen > 0.001f) { rx /= rlen; rz /= rlen; }
        // Camera up = right x forward
        const float ux = -rz * fy;
        const float uy = rz * fx - rx * fz;
        const float uz = rx * fy;
        const float zCam = dx * fx + dy * fy + dz * fz;
        const float yCam = dx * ux + dy * uy + dz * uz;
        float sy = 0.54f;
        if (zCam > 0.1f)
        {
            const float fProj = 1.0f / tanf(sCamera.fov * (3.14159265f / 360.0f));
            const float ndcY = (yCam * fProj) / zCam;
            sy = 0.5f - 0.5f * ndcY;
        }
        if (sy < 0.25f) sy = 0.25f;
        if (sy > 0.78f) sy = 0.78f;
        f->playerScreenY = sy;
    }
}

// GL state of the 3D passes.
static void Pass_SetupWorld3D(void)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    // Keep GL_ALPHA_TEST OFF on opaque geometry so PowerVR SGX543MP4+ TBDR Early-Z / HSR runs at full speed!
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

// Pass 1: ground and relief, opaque first (early-Z / HSR), then cliff cutouts.
static void Pass_Ground(const VoxelFrame *const f)
{
    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    for (uint32_t cutPass = 0; cutPass < 2u; cutPass++)
    {
        if (VitaPerf_PassSkipped((cutPass == 0u) ? VITA_PERF_PASS_GROUND : VITA_PERF_PASS_CUTOUTS))
            continue;
        if (cutPass == 1u)
        {
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.0f);
        }

        for (int32_t aSlot = 0; aSlot < VOXEL_ATLAS_SLOTS; aSlot++)
        {
            if (!sAtlasSlots[aSlot].valid)
                continue;

            for (uint32_t page = 0; page < sAtlasSlots[aSlot].pageCount; page++)
            {
                const GLuint tex = sAtlasSlots[aSlot].tex[page];
                if (!tex)
                    continue;

                bool bound = false;
                GLuint lastVbo = 0;

                for (int32_t i = 0; i < visibleChunkCount; i++)
                {
                    const VoxelChunkSlot *const chunk = visibleChunks[i];
                    if (!(chunk->usedAtlasMask & (1u << aSlot)))
                        continue;

                    for (uint8_t sp = 0; sp < chunk->groundSpanCount; sp++)
                    {
                        const VoxelChunkGroundSpan *const span = &chunk->groundSpans[sp];
                        if (span->atlasSlot != (uint8_t)aSlot || span->page != (uint8_t)page
                         || span->isCutout != (uint8_t)cutPass || span->vertCount == 0)
                            continue;

                        if (!bound)
                        {
                            glBindTexture(GL_TEXTURE_2D, tex);
                            bound = true;
                        }

                        if (lastVbo != chunk->vbo)
                        {
                            glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
                            glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
                            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
                            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));
                            lastVbo = chunk->vbo;
                        }

                        ChunkOffsets_Apply(chunk);
                        glDrawArrays(GL_TRIANGLES, (GLint)span->firstVert, (GLsizei)span->vertCount);
                    }
                }
            }
        }
    }
}

// Pass 2: trees, tall grass and flowers (tree atlas), from the chunk VBOs.
static void Pass_Trees(const VoxelFrame *const f)
{
    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    if (!sTreeTex[0])
        return;

    uint32_t flowerFrame = 0u;
    if (sFlowerFramesReady && f->hd2dPreset != HD2D_PRESET_OFF)
    {
        static const uint8_t kFlowerSeq[4] = { 0u, 1u, 0u, 2u };
        const uint32_t step = (uint32_t)(f->seconds * (60.0f / 16.0f)) & 3u;
        flowerFrame = kFlowerSeq[step];
    }
    glBindTexture(GL_TEXTURE_2D, sTreeTex[flowerFrame]);

    const bool useSwayShader = (sTreeShaderReady && f->hd2dPreset != HD2D_PRESET_OFF);
    bool hasSway = false;

    // 2A-0: Opaque tree cores & ground plates with GL_ALPHA_TEST OFF so SGX543 HSR rejects hidden trees
    glDisable(GL_ALPHA_TEST);

    const bool drawCores = !VitaPerf_PassSkipped(VITA_PERF_PASS_TREECORES);
    for (int32_t i = 0; drawCores && i < visibleChunkCount; i++)
    {
        const VoxelChunkSlot *const chunk = visibleChunks[i];
        if (chunk->treeOpaqueCount == 0u) continue;

        glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));

        ChunkOffsets_Apply(chunk);
        glDrawArrays(GL_TRIANGLES, (GLint)chunk->treeFirstVert, (GLsizei)chunk->treeOpaqueCount);
    }

    // 2A-1: Alpha-tested tree edges (and all foliage when sway shader is off)
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

    const bool drawStrips = !VitaPerf_PassSkipped(VITA_PERF_PASS_TREESTRIPS);
    for (int32_t i = 0; i < visibleChunkCount; i++)
    {
        const VoxelChunkSlot *const chunk = visibleChunks[i];
        if (chunk->treeCount == 0u) continue;
        const uint32_t endVert = useSwayShader ? chunk->treeStaticCount : chunk->treeCount;
        if (useSwayShader && chunk->treeCount > chunk->treeStaticCount)
            hasSway = true;
        if (!drawStrips || endVert <= chunk->treeOpaqueCount) continue;

        glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));

        ChunkOffsets_Apply(chunk);
        glDrawArrays(GL_TRIANGLES, (GLint)(chunk->treeFirstVert + chunk->treeOpaqueCount),
                     (GLsizei)(endVert - chunk->treeOpaqueCount));
    }

    // 2B: Swaying tall grass, ash grass & flowers only (no static tree crowns in shader)
    if (useSwayShader && hasSway && !VitaPerf_PassSkipped(VITA_PERF_PASS_SWAY))
    {
        if (sAppliedOffsetX != 0.0f || sAppliedOffsetZ != 0.0f)
        {
            glPopMatrix();
            glPushMatrix();
            sAppliedOffsetX = 0.0f;
            sAppliedOffsetZ = 0.0f;
        }

        glDisable(GL_ALPHA_TEST);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glUseProgram(sTreeShaderProg);
        glUniformMatrix4fv(sTreeShaderUMvp, 1, GL_FALSE, sViewState.mvp);
        glUniform4fv(sTreeShaderUSwayA, 1, f->swayPhase);
        glUniform1f(sTreeShaderUSwayB, f->swayPhase[4]);
        glUniform2f(sTreeShaderUWind, 0.85f, 0.52f);
        glUniform2f(sTreeShaderUPlayerXZ, sCamera.targetX, sCamera.targetZ);
        glUniform1i(sTreeShaderUTex, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        for (int32_t i = 0; i < visibleChunkCount; i++)
        {
            const VoxelChunkSlot *const chunk = visibleChunks[i];
            if (chunk->treeCount <= chunk->treeStaticCount) continue;
            const uint32_t swayCount = chunk->treeCount - chunk->treeStaticCount;

            glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
            glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));
            glUniform3f(sTreeShaderUChunkDelta, chunk->drawOffsetX, 0.0f, chunk->drawOffsetZ);
            glDrawArrays(GL_TRIANGLES, (GLint)(chunk->treeFirstVert + chunk->treeStaticCount), (GLsizei)swayCount);
        }

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
        glUseProgram(0);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
    }
}

// Pass 3: buildings and indoor furniture, one building page at a time. Returns whether
// alpha testing was left on.
static bool Pass_Buildings(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_BUILDINGS))
        return false;
    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    int32_t activeBuildingPages[32];
    int32_t activeBuildingPageCount = 0;
    for (int32_t i = 0; i < visibleChunkCount; i++)
    {
        const VoxelChunkSlot *const chunk = visibleChunks[i];
        for (uint8_t sp = 0; sp < chunk->buildingSpanCount; sp++)
        {
            const int32_t p = chunk->buildingSpans[sp].page;
            if (p < 0 || chunk->buildingSpans[sp].vertCount == 0)
                continue;
            bool alreadyInList = false;
            for (int32_t k = 0; k < activeBuildingPageCount; k++)
            {
                if (activeBuildingPages[k] == p)
                {
                    alreadyInList = true;
                    break;
                }
            }
            if (!alreadyInList && activeBuildingPageCount < 32)
                activeBuildingPages[activeBuildingPageCount++] = p;
        }
    }

    // Two clean passes for PowerVR SGX543MP4+ TBDR HSR: opaque building pages first
    // (GL_ALPHA_TEST OFF), then cutout pages (GL_ALPHA_TEST ON).
    bool buildingAlphaEnabled = false;
    glDisable(GL_ALPHA_TEST);

    for (uint32_t alphaPass = 0u; alphaPass < 2u; alphaPass++)
    {
        const bool wantAlpha = (alphaPass == 1u);
        for (int32_t ap = 0; ap < activeBuildingPageCount; ap++)
        {
            const int32_t p = activeBuildingPages[ap];
            if (p < 0 || p >= VOXEL_MAX_BUILDING_PAGES) continue;
            const GLuint bTex = RequestOrLoadBuildingPage(p, sSyncBuildingLoads);
            if (!bTex) continue;

            const bool pageNeedsAlpha = (sBuildingPageHasAlpha[p] == 2u);
            if (pageNeedsAlpha != wantAlpha)
                continue;

            if (wantAlpha && !buildingAlphaEnabled)
            {
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);
                buildingAlphaEnabled = true;
            }

            glBindTexture(GL_TEXTURE_2D, bTex);

            GLuint lastVbo = 0;
            for (int32_t i = 0; i < visibleChunkCount; i++)
            {
                const VoxelChunkSlot *const chunk = visibleChunks[i];

                for (uint8_t sp = 0; sp < chunk->buildingSpanCount; sp++)
                {
                    const VoxelChunkBuildingSpan *const bspan = &chunk->buildingSpans[sp];
                    if (bspan->page != (int16_t)p || bspan->vertCount == 0)
                        continue;

                    if (lastVbo != chunk->vbo)
                    {
                        glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
                        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
                        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
                        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));
                        lastVbo = chunk->vbo;
                    }

                    ChunkOffsets_Apply(chunk);
                    glDrawArrays(GL_TRIANGLES, (GLint)(chunk->buildingFirstVert + bspan->firstVert), (GLsizei)bspan->vertCount);
                }
            }
        }
    }
    return buildingAlphaEnabled;
}

// Pass 4: cast and decal silhouette shadows (Emerald 3DS GL_COMBINE, 0xFF281408u).
static void Pass_CastShadows(void)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_SHADOWS))
        return;
    if (sShadowGlCount > 0 && sSpriteTex != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glBindTexture(GL_TEXTURE_2D, sSpriteTex);

        // Pure flat dark-indigo silhouette RGB from vertex primary color + sprite alpha * vertex alpha
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sShadowGlVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sShadowGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sShadowGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sShadowGlCount);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// Pass 4.5: character and cloud reflections on water.
static void Pass_Reflections(void)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_REFLECTIONS))
        return;
    if (sReflectionGlCount > 0)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glBindTexture(GL_TEXTURE_2D, sSpriteTex);

        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sReflectionGlVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sReflectionGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sReflectionGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sReflectionGlCount);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// Pass 5: entity billboards, with the player's silhouette where it is occluded.
static void Pass_Entities(void)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_ENTITIES))
        return;
    if (sEntityGlCount > 0)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glBindTexture(GL_TEXTURE_2D, sSpriteTex);

        // Reversed-depth flat warm-ivory silhouette when occluded behind 3D roofs or tree crowns (ctr_voxel.c:L8798)
        int32_t playerFirst = VoxelEntities_PlayerVertexFirst();
        if (playerFirst >= 0 && (uint32_t)(playerFirst + 6) <= sEntityGlCount)
        {
            VoxelGlVertex silVerts[6];
            for (int32_t k = 0; k < 6; ++k)
            {
                silVerts[k] = sEntityGlVerts[playerFirst + k];
                silVerts[k].r = 240;
                silVerts[k].g = 232;
                silVerts[k].b = 216;
                silVerts[k].a = 160;
            }
            glDepthFunc(GL_GREATER);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

            glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &silVerts[0].x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &silVerts[0].u);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &silVerts[0].r);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
        }

        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sEntityGlVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sEntityGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sEntityGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sEntityGlCount);
    }
}

// Pass 5.15: the lamps' light on the characters (LightCards), after the night veil: their lit
// cards again, textured with the sprite atlas and coloured by the light, added (GL_SRC_ALPHA, GL_ONE:
// the sprite's transparent texels add nothing, so no alpha test). Same vertices as Pass_Entities,
// so GL_LEQUAL lands on the same depth.
static void Pass_EntityLamps(void)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_LAMPS))
        return;
    if (sEntityLitCount == 0 || sSpriteTex == 0)
        return;
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sSpriteTex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthFunc(GL_LEQUAL);   // (the 3D passes' own, Pass_SetupWorld3D)
    glDepthMask(GL_FALSE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sEntityLitVerts[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sEntityLitVerts[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sEntityLitVerts[0].r);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sEntityLitCount);
    sFrameDrawCalls++;
    sFrameVertCount += sEntityLitCount;
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Pass 5.1: day/night and atmosphere modulation of the whole 3D world, in hardware
// (destination colour blending).
static void Pass_DayNightGrade(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_DAYNIGHT))
        return;
    const bool indoor = f->indoor;
    if (!indoor && !f->grade.scaleIdentity)
    {
        const float sr = (float)f->grade.scaleR * (1.0f / 256.0f);
        const float sg = (float)f->grade.scaleG * (1.0f / 256.0f);
        const float sb = (float)f->grade.scaleB * (1.0f / 256.0f);
        const float cr = sr > 1.0f ? 1.0f : sr;
        const float cg = sg > 1.0f ? 1.0f : sg;
        const float cb = sb > 1.0f ? 1.0f : sb;

        if (cr < 0.96f || cg < 0.96f || cb < 0.96f)
        {
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_ALPHA_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);

            glColor4f(cr, cg, cb, 1.0f);

            static const float sNdcQuad[8] = {
                -1.0f, -1.0f,
                 1.0f, -1.0f,
                 1.0f,  1.0f,
                -1.0f,  1.0f
            };

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, sNdcQuad);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glDisableClientState(GL_VERTEX_ARRAY);

            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();

            glMatrixMode(GL_MODELVIEW);

            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

// Pass 5.15: lit building windows at night using buildings.bin L8 glow masks.
// Drawn after Pass_DayNightGrade so the night veil never dims lit windows, and
// before HD-2D post-processing so bright windows pick up a soft bloom halo.
static void Pass_BuildingGlow(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_LAMPS))
        return;
    if (f->nightGlow <= 0.01f)
        return;

    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    int32_t glowPages[16];
    int32_t glowPageCount = 0;

    for (int32_t i = 0; i < visibleChunkCount; i++)
    {
        const VoxelChunkSlot *const chunk = visibleChunks[i];
        for (uint8_t sp = 0; sp < chunk->buildingSpanCount; sp++)
        {
            const int32_t p = chunk->buildingSpans[sp].page;
            if (p < 0 || p >= VOXEL_MAX_BUILDING_PAGES || chunk->buildingSpans[sp].glowVertCount == 0)
                continue;
            const int32_t slotIdx = sBuildingPageToSlot[p];
            if (slotIdx < 0 || sBuildingCache[slotIdx].glowTex == 0)
                continue;
            bool already = false;
            for (int32_t k = 0; k < glowPageCount; k++)
            {
                if (glowPages[k] == p)
                {
                    already = true;
                    break;
                }
            }
            if (!already && glowPageCount < 16)
                glowPages[glowPageCount++] = p;
        }
    }

    if (glowPageCount == 0)
        return;

    // (a softer window light where the bloom spreads it)
    const float strength = Hd2dPost(f->hd2dPreset)
        ? VOXEL_WINDOW_GLOW_STRENGTH_HD2D
        : VOXEL_WINDOW_GLOW_STRENGTH_STD;
    const float alpha = Voxel_ClampF(f->nightGlow * strength, 0.0f, 1.0f);

    // Pure additive blend with GL_ALPHA_TEST OFF avoids SGX543 TBDR punch-through discard stalls
    // now that glowVertCount contains only triangles overlapping glow > 0 UV cells.
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4f(VOXEL_WINDOW_GLOW_R, VOXEL_WINDOW_GLOW_G, VOXEL_WINDOW_GLOW_B, alpha);

    ChunkOffsets_Begin();
    for (int32_t gp = 0; gp < glowPageCount; gp++)
    {
        const int32_t p = glowPages[gp];
        const int32_t slotIdx = sBuildingPageToSlot[p];
        if (slotIdx < 0 || sBuildingCache[slotIdx].glowTex == 0)
            continue;

        glBindTexture(GL_TEXTURE_2D, sBuildingCache[slotIdx].glowTex);
        GLuint lastVbo = 0;

        for (int32_t i = 0; i < visibleChunkCount; i++)
        {
            const VoxelChunkSlot *const chunk = visibleChunks[i];
            for (uint8_t sp = 0; sp < chunk->buildingSpanCount; sp++)
            {
                const VoxelChunkBuildingSpan *const bspan = &chunk->buildingSpans[sp];
                if (bspan->page != (int16_t)p || bspan->glowVertCount == 0)
                    continue;

                if (lastVbo != chunk->vbo)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
                    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
                    glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
                    lastVbo = chunk->vbo;
                }

                ChunkOffsets_Apply(chunk);
                glDrawArrays(GL_TRIANGLES, (GLint)(chunk->buildingFirstVert + bspan->firstVert), (GLsizei)bspan->glowVertCount);
                sFrameDrawCalls++;
                sFrameVertCount += bspan->glowVertCount;
            }
        }
    }
    ChunkOffsets_End();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Draws one part of the visible chunks' lamp geometry (VoxelChunkSlot::lampFirstVert...) from
// their VBOs: the pools (pool) or the glass. State is set by the caller.
static void DrawChunkLampPart(const VoxelFrame *const f, const bool pool)
{
    GLuint lastVbo = 0;
    ChunkOffsets_Begin();
    for (int32_t i = 0; i < f->visibleCount; i++)
    {
        const VoxelChunkSlot *const chunk = f->visible[i];
        const uint32_t count = pool ? chunk->lampPoolCount : chunk->lampGlassCount;
        if (count == 0 || chunk->vbo == 0)
            continue;
        if (lastVbo != chunk->vbo)
        {
            glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
            glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
            lastVbo = chunk->vbo;
        }
        ChunkOffsets_Apply(chunk);
        const uint32_t first = chunk->lampFirstVert + (pool ? chunk->lampGlassCount : 0u);
        glDrawArrays(GL_TRIANGLES, (GLint)first, (GLsizei)count);
        sFrameDrawCalls++;
        sFrameVertCount += count;
    }
    ChunkOffsets_End();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// One camera-facing halo per visible lamp light, pulled toward the camera so its own lantern does
// not cut it. Built per frame into sBatchGlVerts: a few dozen vertices.
static void DrawLampHalos(const VoxelFrame *const f, const LampLevel *const level)
{
    const float *const view = sViewState.view;
    const float right[3] = { view[0], view[4], view[8] };
    const float up[3] = { view[1], view[5], view[9] };
    const float toCamera[3] = { view[2], view[6], view[10] };
    static const float kCornerX[6] = { -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f };
    static const float kCornerY[6] = { -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
    const float size = level->haloSize;
    uint32_t vertCount = 0;

    for (int32_t i = 0; i < f->visibleCount && vertCount + 6u <= VOXEL_LAMP_HALO_MAX * 6u; i++)
    {
        const VoxelChunkSlot *const chunk = f->visible[i];
        for (uint32_t l = 0; l < chunk->lampLightCount && vertCount + 6u <= VOXEL_LAMP_HALO_MAX * 6u; l++)
        {
            const float c[3] = {
                chunk->lampLights[l][0] + chunk->drawOffsetX + toCamera[0] * VOXEL_LAMP_HALO_LIFT,
                chunk->lampLights[l][1] + toCamera[1] * VOXEL_LAMP_HALO_LIFT,
                chunk->lampLights[l][2] + chunk->drawOffsetZ + toCamera[2] * VOXEL_LAMP_HALO_LIFT,
            };
            for (uint32_t k = 0; k < 6; k++)
            {
                VoxelGlVertex *const v = &sBatchGlVerts[vertCount++];
                v->x = c[0] + (right[0] * kCornerX[k] + up[0] * kCornerY[k]) * size;
                v->y = c[1] + (right[1] * kCornerX[k] + up[1] * kCornerY[k]) * size;
                v->z = c[2] + (right[2] * kCornerX[k] + up[2] * kCornerY[k]) * size;
                v->u = kCornerX[k] * 0.5f + 0.5f;
                v->v = kCornerY[k] * 0.5f + 0.5f;
            }
        }
    }
    if (vertCount == 0 || sLampHaloTex == 0)
        return;
    glBindTexture(GL_TEXTURE_2D, sLampHaloTex);
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertCount);
    sFrameDrawCalls++;
    sFrameVertCount += vertCount;
}

// Pass 5.2: night lamps (VoxelLamps_EmitChunk). Their geometry is static in the chunk VBOs and
// the glow is a constant colour, so a frame costs two draws per lamp chunk plus one for the halos,
// and no vertex is written:
//   pools: the ground under each lamp brightened, dst * (1 + gain * falloff * amber), drawn with
//          a depth offset over the ground it lies on;
//   glass: additive amber, bright enough for the bloom to spread it (MED, HIGH);
//   halos: one billboard per lamp, sized and weighted by the HD-2D level (LampLevel).
static void Pass_NightLamps(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_LAMPS))
        return;
    const float glow = f->nightGlow;
    if (glow <= 0.01f)
        return;
    bool anyLamp = false;
    for (int32_t i = 0; i < f->visibleCount && !anyLamp; i++)
        anyLamp = f->visible[i]->lampLightCount > 0;
    if (!anyLamp)
        return;

    const LampLevel *const level = LampLevelOf(f);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if (sLampPoolTex != 0)
    {
        const float k = glow * VOXEL_LAMP_POOL_GAIN;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sLampPoolTex);
        glBlendFunc(GL_DST_COLOR, GL_ONE);
        glColor4f(VOXEL_LAMP_R * k, VOXEL_LAMP_G * k, VOXEL_LAMP_B * k, 1.0f);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -2.0f);
        DrawChunkLampPart(f, true);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(VOXEL_LAMP_R, VOXEL_LAMP_G, VOXEL_LAMP_B, Voxel_ClampF(glow * VOXEL_LAMP_GLASS_GAIN, 0.0f, 1.0f));
    DrawChunkLampPart(f, false);

    glEnable(GL_TEXTURE_2D);
    glColor4f(VOXEL_LAMP_R, VOXEL_LAMP_G, VOXEL_LAMP_B, Voxel_ClampF(glow * level->haloAlpha, 0.0f, 1.0f));
    DrawLampHalos(f, level);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

enum
{
    SFX_WATER_RIPPLE = 0,
    SFX_WATER_GLINT  = 1,
    SFX_WATER_MIST   = 2,
    SFX_ASH_SETTLE   = 3,
    SFX_GRASS_LEAF   = 4,
    SFX_GRASS_ASH    = 5,
    SURFACE_FX_CAP   = 64
};

typedef struct
{
    float   wx, wy, wz;
    float   driftX, driftZ;
    float   age, life;
    uint8_t kind;
} VoxelSurfaceFx;

static VoxelSurfaceFx sSurfaceFx[SURFACE_FX_CAP] = {0};
static uint32_t sSurfaceFxRng = 0x9E3779B9u;
static double sSurfaceFxPrevMs = 0.0;
static int32_t sLastGrassPlayerX = 0x7FFFFFFF;
static int32_t sLastGrassPlayerZ = 0x7FFFFFFF;

static inline float SurfaceFx_Rand01(void)
{
    sSurfaceFxRng = sSurfaceFxRng * 1664525u + 1013904223u;
    return (float)((sSurfaceFxRng >> 8) & 0xFFFFu) * (1.0f / 65536.0f);
}

// Called by the crossing itself, not by the pass: a crossing frame can end before the passes
// (full fade, Fly cloud frame), and the effects must not stay in the old map's coordinates.
static void SurfaceFx_ShiftCoordinates(const int32_t shiftX, const int32_t shiftZ)
{
    const float sx = (float)shiftX;
    const float sz = (float)shiftZ;
    for (int32_t i = 0; i < SURFACE_FX_CAP; ++i)
    {
        sSurfaceFx[i].wx += sx;
        sSurfaceFx[i].wz += sz;
    }
    if (sLastGrassPlayerX != 0x7FFFFFFF)
    {
        sLastGrassPlayerX += shiftX;
        sLastGrassPlayerZ += shiftZ;
    }
}

static void DrawSurfaceFx(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_SURFACEFX))
        return;
    if (f->indoor || f->inBattle3D || f->hd2dPreset == HD2D_PRESET_OFF)
        return;

    // A crossing already moved the live effects (SurfaceFx_ShiftCoordinates); a warp lands
    // somewhere else behind a fade, so they are dropped.
    if (f->mapChanged && !f->crossed)
    {
        for (int32_t i = 0; i < SURFACE_FX_CAP; ++i)
            sSurfaceFx[i].age = sSurfaceFx[i].life;
        sSurfaceFxPrevMs = f->startMs;
        sLastGrassPlayerX = 0x7FFFFFFF;
        sLastGrassPlayerZ = 0x7FFFFFFF;
        return;
    }

    // Spawn leaf / ash puffs when the player steps onto a tall-grass, long-grass, or ash-grass cell
    if (f->hd2dPreset != HD2D_PRESET_OFF)
    {
        float pwx = 0.0f, pwz = 0.0f;
        VoxelWorld_GetPlayerWorldCoords(&pwx, &pwz);
        const int32_t cellX = (int32_t)floorf(pwx);
        const int32_t cellZ = (int32_t)floorf(pwz);
        if (cellX != sLastGrassPlayerX || cellZ != sLastGrassPlayerZ)
        {
            const bool firstInit = (sLastGrassPlayerX == 0x7FFFFFFF);
            sLastGrassPlayerX = cellX;
            sLastGrassPlayerZ = cellZ;
            if (!firstInit)
            {
                const u8 beh = (u8)VoxelWorld_GetMetatileBehavior(cellX, cellZ);
                const bool isAshGrass = MetatileBehavior_IsAshGrass(beh);
                const bool isGrass = isAshGrass || MetatileBehavior_IsTallGrass(beh) || MetatileBehavior_IsLongGrass(beh);
                if (isGrass)
                {
                    int32_t burst = 0;
                    for (int32_t i = 0; i < SURFACE_FX_CAP && burst < 5; ++i)
                    {
                        if (sSurfaceFx[i].age < sSurfaceFx[i].life)
                            continue;
                        const float ang = SurfaceFx_Rand01() * 6.2831853f;
                        const float spd = 1.1f + SurfaceFx_Rand01() * 1.4f;
                        sSurfaceFx[i].kind = isAshGrass ? SFX_GRASS_ASH : SFX_GRASS_LEAF;
                        sSurfaceFx[i].wx = sCamera.targetX + (SurfaceFx_Rand01() - 0.5f) * 0.35f;
                        sSurfaceFx[i].wz = sCamera.targetZ + (SurfaceFx_Rand01() - 0.5f) * 0.25f;
                        sSurfaceFx[i].wy = SurfaceWorldY(sSurfaceFx[i].wx, sSurfaceFx[i].wz) + 0.30f;
                        sSurfaceFx[i].driftX = cosf(ang) * spd;
                        sSurfaceFx[i].driftZ = sinf(ang) * spd;
                        sSurfaceFx[i].age = 0.0f;
                        sSurfaceFx[i].life = 0.42f + SurfaceFx_Rand01() * 0.16f;
                        burst++;
                    }
                }
            }
        }
    }

    const bool isPausedHd2d = (!f->inBattle3D && Ui_IsOpen());
    float dt = (!isPausedHd2d && sSurfaceFxPrevMs > 0.0) ? (float)((f->startMs - sSurfaceFxPrevMs) * 0.001) : 0.0f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;
    sSurfaceFxPrevMs = f->startMs;

    const VitaDayNightLight *const dn = VitaDayNight_Get();
    const float daylight = (dn != NULL) ? dn->daylight : 1.0f;
    const float glow     = (dn != NULL) ? dn->glow     : 0.0f;
    const bool isRain    = (f->weather == VOXEL_WEATHER_RAIN && f->weatherAmount > 0.10f);
    const bool isAsh     = (f->weather == VOXEL_WEATHER_ASH  && f->weatherAmount > 0.10f);
    const bool isSand    = (f->weather == VOXEL_WEATHER_SAND && f->weatherAmount > 0.10f);
    const bool allowMist = (!isRain && (glow > 0.10f || daylight < 0.55f));

    for (int32_t i = 0; i < SURFACE_FX_CAP; ++i)
    {
        if (sSurfaceFx[i].age < sSurfaceFx[i].life)
        {
            sSurfaceFx[i].age += dt;
            sSurfaceFx[i].wx  += sSurfaceFx[i].driftX * dt;
            sSurfaceFx[i].wz  += sSurfaceFx[i].driftZ * dt;
        }
    }

    int32_t spawned = 0;
    const int32_t maxAttempts = (!isPausedHd2d && (sFrameCounter & 1u) == 0u) ? 24 : 0;
    for (int32_t attempt = 0; attempt < maxAttempts && spawned < 5; ++attempt)
    {
        int32_t freeIdx = -1;
        for (int32_t i = 0; i < SURFACE_FX_CAP; ++i)
        {
            if (sSurfaceFx[i].age >= sSurfaceFx[i].life)
            {
                freeIdx = i;
                break;
            }
        }
        if (freeIdx < 0)
            break;

        const float wx = sCamera.targetX + (SurfaceFx_Rand01() - 0.5f) * 24.0f;
        const float wz = sCamera.targetZ - 14.0f + SurfaceFx_Rand01() * 18.0f;
        const int32_t tx = (int32_t)floorf(wx);
        const int32_t tz = (int32_t)floorf(wz);
        const VoxelVisualShape shape = VoxelWorld_ClassifyTile(tx, tz);

        if (shape == VOXEL_SHAPE_WATER)
        {
            const float surfY = SurfaceWorldY(wx, wz);
            VoxelSurfaceFx *const s = &sSurfaceFx[freeIdx];
            if (isRain)
            {
                s->kind = SFX_WATER_RIPPLE;
                s->wx = wx;
                s->wy = surfY + 0.045f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 0.60f + SurfaceFx_Rand01() * 0.35f;
                s->driftX = 0.0f;
                s->driftZ = 0.0f;
                spawned++;
            }
            else if ((allowMist && SurfaceFx_Rand01() < 0.35f) || SurfaceFx_Rand01() < 0.12f)
            {
                s->kind = SFX_WATER_MIST;
                s->wx = wx;
                s->wy = surfY + 0.16f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 6.5f + SurfaceFx_Rand01() * 3.5f;
                s->driftX = (SurfaceFx_Rand01() - 0.5f) * 0.10f;
                s->driftZ = (SurfaceFx_Rand01() - 0.5f) * 0.07f;
                spawned++;
            }
            else if (SurfaceFx_Rand01() < 0.08f
                     && VoxelWorld_ClassifyTile(tx - 1, tz) == VOXEL_SHAPE_WATER
                     && VoxelWorld_ClassifyTile(tx + 1, tz) == VOXEL_SHAPE_WATER
                     && VoxelWorld_ClassifyTile(tx, tz - 1) == VOXEL_SHAPE_WATER
                     && VoxelWorld_ClassifyTile(tx, tz + 1) == VOXEL_SHAPE_WATER)
            {
                int32_t activeGlints = 0;
                for (int32_t g = 0; g < SURFACE_FX_CAP; ++g)
                {
                    if (sSurfaceFx[g].age < sSurfaceFx[g].life && sSurfaceFx[g].kind == SFX_WATER_GLINT)
                        activeGlints++;
                }
                if (activeGlints < 6)
                {
                    const float cx = (float)tx + 0.25f + SurfaceFx_Rand01() * 0.50f;
                    const float cz = (float)tz + 0.25f + SurfaceFx_Rand01() * 0.50f;
                    s->kind = SFX_WATER_GLINT;
                    s->wx = cx;
                    s->wy = SurfaceWorldY(cx, cz) + 0.045f;
                    s->wz = cz;
                    s->age = 0.0f;
                    s->life = 1.10f + SurfaceFx_Rand01() * 0.70f;
                    s->driftX = 0.0f;
                    s->driftZ = 0.0f;
                    spawned++;
                }
            }
        }
        else if (shape == VOXEL_SHAPE_FLAT && VoxelWorld_GetCollision(tx, tz) == 0u)
        {
            if (isRain)
            {
                const float surfY = SurfaceWorldY(wx, wz);
                VoxelSurfaceFx *const s = &sSurfaceFx[freeIdx];
                s->kind = SFX_WATER_RIPPLE;
                s->wx = wx;
                s->wy = surfY + 0.045f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 0.35f + SurfaceFx_Rand01() * 0.20f;
                s->driftX = 0.0f;
                s->driftZ = 0.0f;
                spawned++;
            }
            else if (f->fogDensity > 0.01f && SurfaceFx_Rand01() < 0.22f)
            {
                const float surfY = SurfaceWorldY(wx, wz);
                VoxelSurfaceFx *const s = &sSurfaceFx[freeIdx];
                s->kind = SFX_WATER_MIST;
                s->wx = wx;
                s->wy = surfY + 0.16f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 6.5f + SurfaceFx_Rand01() * 3.5f;
                s->driftX = (SurfaceFx_Rand01() - 0.5f) * 0.10f;
                s->driftZ = (SurfaceFx_Rand01() - 0.5f) * 0.07f;
                spawned++;
            }
            else if (isSand && SurfaceFx_Rand01() < 0.45f)
            {
                const float surfY = SurfaceWorldY(wx, wz);
                VoxelSurfaceFx *const s = &sSurfaceFx[freeIdx];
                s->kind = SFX_WATER_MIST;
                s->wx = wx;
                s->wy = surfY + 0.12f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 1.4f + SurfaceFx_Rand01() * 0.8f;
                s->driftX = -3.8f - SurfaceFx_Rand01() * 1.8f;
                s->driftZ = 0.9f + SurfaceFx_Rand01() * 0.5f;
                spawned++;
            }
            else if (isAsh)
            {
                const float surfY = SurfaceWorldY(wx, wz);
                VoxelSurfaceFx *const s = &sSurfaceFx[freeIdx];
                s->kind = SFX_ASH_SETTLE;
                s->wx = wx;
                s->wy = surfY + 0.045f;
                s->wz = wz;
                s->age = 0.0f;
                s->life = 0.70f + SurfaceFx_Rand01() * 0.50f;
                s->driftX = 0.0f;
                s->driftZ = 0.0f;
                spawned++;
            }
        }
    }

    static const float qx[6] = {-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f};
    static const float qz[6] = {-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f};
    static const float qu[6] = { 0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f};
    static const float qv[6] = { 0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f};

    uint32_t nRipple = 0, nMist = 0, nGlint = 0, nLeaf = 0;
    uint32_t offMist = SURFACE_FX_CAP * 6u;
    uint32_t offGlint = SURFACE_FX_CAP * 12u;
    uint32_t offLeaf = SURFACE_FX_CAP * 18u;

    for (int32_t i = 0; i < SURFACE_FX_CAP; ++i)
    {
        const VoxelSurfaceFx *const s = &sSurfaceFx[i];
        if (s->age >= s->life || s->life <= 0.0f)
            continue;
        const float t = s->age / s->life;
        if (s->kind == SFX_WATER_RIPPLE && sRippleTex != 0)
        {
            const float rad = 0.14f + 0.42f * t;
            const uint8_t a = (uint8_t)((1.0f - t) * 245.0f * f->weatherAmount);
            if (a < 2u) continue;
            for (int32_t k = 0; k < 6; ++k)
            {
                VoxelGlVertex *const v = &sBatchGlVerts[nRipple++];
                v->x = s->wx + qx[k] * rad; v->y = s->wy; v->z = s->wz + qz[k] * rad;
                v->u = qu[k]; v->v = qv[k];
                v->r = 225; v->g = 242; v->b = 255; v->a = a;
            }
        }
        else if (s->kind == SFX_WATER_MIST && sSoftMistTex != 0)
        {
            const float rad = isSand ? 1.15f : 1.65f;
            float env = sinf(t * 3.14159265f);
            env = env * env * (3.0f - 2.0f * env);
            const uint8_t a = (uint8_t)(env * (isSand ? 115.0f : 48.0f));
            if (a < 2u) continue;
            const uint8_t mr = isSand ? 192 : 210;
            const uint8_t mg = isSand ? 144 : 228;
            const uint8_t mb = isSand ? 88  : 248;
            for (int32_t k = 0; k < 6; ++k)
            {
                VoxelGlVertex *const v = &sBatchGlVerts[offMist + nMist++];
                v->x = s->wx + qx[k] * rad; v->y = s->wy; v->z = s->wz + qz[k] * rad;
                v->u = qu[k]; v->v = qv[k];
                v->r = mr; v->g = mg; v->b = mb; v->a = a;
            }
        }
        else if ((s->kind == SFX_WATER_GLINT || s->kind == SFX_ASH_SETTLE) && sLampPoolTex != 0)
        {
            const float env = sinf(t * 3.14159265f);
            const float rad = (s->kind == SFX_ASH_SETTLE) ? 0.11f : (0.07f + 0.04f * env);
            const uint8_t str = (uint8_t)(env * ((s->kind == SFX_ASH_SETTLE) ? 145.0f : (daylight > 0.45f ? 95.0f : 70.0f)));
            if (str < 2u) continue;
            const uint8_t cr = (s->kind == SFX_ASH_SETTLE) ? str : (daylight > 0.45f ? str : (uint8_t)(str * 0.78f));
            const uint8_t cg = (s->kind == SFX_ASH_SETTLE) ? (uint8_t)(str * 0.55f) : (uint8_t)(str * 0.94f);
            const uint8_t cb = (s->kind == SFX_ASH_SETTLE) ? (uint8_t)(str * 0.25f) : (daylight > 0.45f ? (uint8_t)(str * 0.75f) : str);
            for (int32_t k = 0; k < 6; ++k)
            {
                VoxelGlVertex *const v = &sBatchGlVerts[offGlint + nGlint++];
                v->x = s->wx + qx[k] * rad; v->y = s->wy; v->z = s->wz + qz[k] * rad;
                v->u = qu[k]; v->v = qv[k];
                v->r = cr; v->g = cg; v->b = cb; v->a = 255;
            }
        }
        else if ((s->kind == SFX_GRASS_LEAF || s->kind == SFX_GRASS_ASH) && sSoftMistTex != 0)
        {
            const float arcY = sinf(t * 3.14159265f) * 0.42f;
            const float rad = 0.14f;
            const uint8_t a = (uint8_t)((1.0f - t) * 235.0f);
            if (a < 2u) continue;
            const bool isAsh = (s->kind == SFX_GRASS_ASH);
            const uint8_t lr = isAsh ? 205 : 74;
            const uint8_t lg = isAsh ? 210 : 186;
            const uint8_t lb = isAsh ? 216 : 64;
            for (int32_t k = 0; k < 6; ++k)
            {
                VoxelGlVertex *const v = &sBatchGlVerts[offLeaf + nLeaf++];
                v->x = s->wx + qx[k] * rad;
                v->y = s->wy + arcY + (1.0f - qz[k]) * rad;
                v->z = s->wz + qz[k] * (rad * 0.5f);
                v->u = qu[k]; v->v = qv[k];
                v->r = lr; v->g = lg; v->b = lb; v->a = a;
            }
        }
    }

    if (nRipple == 0 && nMist == 0 && nGlint == 0 && nLeaf == 0)
        return;

    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    if (nRipple > 0)
    {
        glBindTexture(GL_TEXTURE_2D, sRippleTex);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)nRipple);
        sFrameDrawCalls++;
        sFrameVertCount += nRipple;
    }
    if (nMist > 0)
    {
        glBindTexture(GL_TEXTURE_2D, sSoftMistTex);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offMist].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offMist].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[offMist].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)nMist);
        sFrameDrawCalls++;
        sFrameVertCount += nMist;
    }
    if (nGlint > 0)
    {
        glBindTexture(GL_TEXTURE_2D, sLampPoolTex);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offGlint].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offGlint].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[offGlint].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)nGlint);
        sFrameDrawCalls++;
        sFrameVertCount += nGlint;
    }
    if (nLeaf > 0)
    {
        glBindTexture(GL_TEXTURE_2D, sSoftMistTex);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offLeaf].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[offLeaf].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[offLeaf].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)nLeaf);
        sFrameDrawCalls++;
        sFrameVertCount += nLeaf;
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}



static void DrawWorldRays(const float strength, const float seconds, const bool isMoon, const VitaDayNightPath *const path)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_WORLDRAYS))
        return;
    if (strength <= 0.005f || path == NULL)
        return;

    static const struct {
        float x, z;
        float halfWidth;
        float height;
        float weight;
        float phase;
    } kShafts[10] = {
        {  2.8f,  3.2f, 0.95f, 24.0f, 1.00f, 0.0f },
        {  8.4f,  6.8f, 0.76f, 22.5f, 0.78f, 1.1f },
        { 14.2f,  2.5f, 1.05f, 25.5f, 0.92f, 2.3f },
        { 20.5f,  7.4f, 0.80f, 23.0f, 0.70f, 3.4f },
        { 25.6f,  4.0f, 0.90f, 24.5f, 0.82f, 4.2f },
        {  5.2f, 13.5f, 1.00f, 25.0f, 0.88f, 0.7f },
        { 11.8f, 17.2f, 0.82f, 22.0f, 0.74f, 1.9f },
        { 17.6f, 12.8f, 1.10f, 26.0f, 0.96f, 2.8f },
        { 23.4f, 16.5f, 0.74f, 22.0f, 0.65f, 3.9f },
        { 27.1f, 11.2f, 0.88f, 23.5f, 0.80f, 5.0f },
    };

    const float spanX = 28.0f;
    const float spanZ = 22.0f;
    const float ox = sCamera.targetX - 0.5f * spanX;
    const float oz = sCamera.targetZ - 0.72f * spanZ;
    const float anchoredX = sCamera.targetX + AtmoAnchorWrap(sAtmoAnchorX, (int32_t)spanX);
    const float anchoredZ = sCamera.targetZ + AtmoAnchorWrap(sAtmoAnchorZ, (int32_t)spanZ);
    const uint8_t r = isMoon ? 165u : 255u;
    const uint8_t g = isMoon ? 195u : 235u;
    const uint8_t b = isMoon ? 255u : 176u;
    uint32_t vOut = 0;

    for (int32_t i = 0; i < 10; ++i)
    {
        const float drift = 0.28f * sinf(seconds * 0.42f + kShafts[i].phase);
        const float relX = kShafts[i].x + drift - anchoredX;
        const float relZ = kShafts[i].z - anchoredZ;
        const float lx = relX - floorf(relX / spanX) * spanX;
        const float lz = relZ - floorf(relZ / spanZ) * spanZ;
        const float edgeX = fminf(lx, spanX - lx) * 0.35f;
        const float edgeZ = fminf(lz, spanZ - lz) * 0.35f;
        const float edgeFade = fminf(1.0f, fminf(edgeX, edgeZ));
        if (edgeFade <= 0.02f)
            continue;

        const float wx = ox + lx;
        const float wz = oz + lz;
        const float wy = SurfaceWorldY(wx, wz);
        const float h = kShafts[i].height;
        const float topX = wx + path->dx * (h * 0.38f);
        const float topY = wy + h;
        const float topZ = wz + path->dz * (h * 0.25f);
        const float botY = wy - 0.15f;
        const float midT = 0.55f;
        const float midX = wx + (topX - wx) * midT;
        const float midY = botY + (topY - botY) * midT;
        const float midZ = wz + (topZ - wz) * midT;

        float camDx = sCamera.x - wx;
        float camDz = sCamera.z - wz;
        float camLen = sqrtf(camDx * camDx + camDz * camDz);
        if (camLen < 0.001f) camLen = 1.0f;
        const float hx = (-camDz / camLen) * kShafts[i].halfWidth;
        const float hz = ( camDx / camLen) * kShafts[i].halfWidth;

        float aTop = strength * kShafts[i].weight * 0.58f * edgeFade;
        if (aTop > 0.22f) aTop = 0.22f;
        aTop *= VOXEL_RAYS_OPACITY;
        const uint8_t alphaTop = (uint8_t)(aTop * 255.0f + 0.5f);
        if (alphaTop == 0u)
            continue;

        // Upper fade (topY alpha 0 -> midY alphaTop) so nearby/distant shafts never show a hard cut at top
        PushFlatVertex3D(&vOut, topX - hx, topY, topZ - hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, topX,      topY, topZ,      r, g, b, 0);
        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);

        PushFlatVertex3D(&vOut, topX - hx, topY, topZ - hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);
        PushFlatVertex3D(&vOut, midX - hx, midY, midZ - hz, r, g, b, 0);

        PushFlatVertex3D(&vOut, topX,      topY, topZ,      r, g, b, 0);
        PushFlatVertex3D(&vOut, topX + hx, topY, topZ + hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, midX + hx, midY, midZ + hz, r, g, b, 0);

        PushFlatVertex3D(&vOut, topX,      topY, topZ,      r, g, b, 0);
        PushFlatVertex3D(&vOut, midX + hx, midY, midZ + hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);

        // Lower fade (midY alphaTop -> botY alpha 0)
        PushFlatVertex3D(&vOut, midX - hx, midY, midZ - hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);
        PushFlatVertex3D(&vOut, wx,        botY, wz,        r, g, b, 0);

        PushFlatVertex3D(&vOut, midX - hx, midY, midZ - hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, wx,        botY, wz,        r, g, b, 0);
        PushFlatVertex3D(&vOut, wx - hx,   botY, wz - hz,   r, g, b, 0);

        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);
        PushFlatVertex3D(&vOut, midX + hx, midY, midZ + hz, r, g, b, 0);
        PushFlatVertex3D(&vOut, wx + hx,   botY, wz + hz,   r, g, b, 0);

        PushFlatVertex3D(&vOut, midX,      midY, midZ,      r, g, b, alphaTop);
        PushFlatVertex3D(&vOut, wx + hx,   botY, wz + hz,   r, g, b, 0);
        PushFlatVertex3D(&vOut, wx,        botY, wz,        r, g, b, 0);
    }

    if (vOut == 0u)
        return;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vOut);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// High-altitude cloud wisps for the 3D Fly cinematic (independent of DrawFogBanks terrain cache).
static void DrawCloudWisps(const VoxelFlyFrame *const fly, const double clock)
{
    const float strength = Voxel_ClampF(fly->wisps, 0.0f, 1.0f);
    if (!sFogTex || strength <= 0.005f)
        return;

    static const struct { float height, speedScale, alpha, period, offU, offV; } kBanks[2] = {
        { 24.0f, 0.7f, 0.55f, 22.0f, 0.00f, 0.00f },
        { 33.0f, 1.0f, 0.75f, 16.0f, 0.41f, 0.67f },
    };
    const float wispSpeed = 9.0f;
    const float span = 44.0f;
    const int32_t grid = VOXEL_FOG_GRID;
    const float step = (2.0f * span) / (float)grid;
    const float cx = sCamera.targetX;
    const float cz = sCamera.targetZ;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sFogTex);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);

    for (uint32_t b = 0u; b < 2u; ++b)
    {
        const float wy = sCamera.ground + kBanks[b].height;
        // Wrapped to the texture period: the UVs stay small (they are interpolated in half precision).
        const double period = (double)kBanks[b].period;
        const double speed = (double)(wispSpeed * kBanks[b].speedScale);
        const float driftX = (float)AtmoWrapD(-(double)fly->dirX * speed * clock, period)
                           + AtmoAnchorWrap(sAtmoAnchorX, (int32_t)kBanks[b].period);
        const float driftZ = (float)AtmoWrapD(-(double)fly->dirZ * speed * clock, period)
                           + AtmoAnchorWrap(sAtmoAnchorZ, (int32_t)kBanks[b].period);
        const float invP = 1.0f / kBanks[b].period;
        const float baseAlpha = strength * kBanks[b].alpha;
        uint32_t vOut = 0u;

        for (int32_t iz = 0; iz < grid; ++iz)
        {
            const float z0 = cz - span + (float)iz * step;
            const float z1 = z0 + step;
            const float ez0 = Voxel_ClampF((span - fabsf(z0 - cz)) / (span * 0.35f), 0.0f, 1.0f);
            const float ez1 = Voxel_ClampF((span - fabsf(z1 - cz)) / (span * 0.35f), 0.0f, 1.0f);
            const float sz0 = ez0 * ez0 * (3.0f - 2.0f * ez0);
            const float sz1 = ez1 * ez1 * (3.0f - 2.0f * ez1);

            for (int32_t ix = 0; ix < grid; ++ix)
            {
                const float x0 = cx - span + (float)ix * step;
                const float x1 = x0 + step;
                const float ex0 = Voxel_ClampF((span - fabsf(x0 - cx)) / (span * 0.35f), 0.0f, 1.0f);
                const float ex1 = Voxel_ClampF((span - fabsf(x1 - cx)) / (span * 0.35f), 0.0f, 1.0f);
                const float sx0 = ex0 * ex0 * (3.0f - 2.0f * ex0);
                const float sx1 = ex1 * ex1 * (3.0f - 2.0f * ex1);

                const float xs[4] = { x0, x1, x1, x0 };
                const float zs[4] = { z0, z0, z1, z1 };
                const float as[4] = { sx0 * sz0, sx1 * sz0, sx1 * sz1, sx0 * sz1 };
                static const int32_t kIdx[6] = { 0, 1, 2, 0, 2, 3 };
                for (int32_t k = 0; k < 6; ++k)
                {
                    const int32_t q = kIdx[k];
                    VoxelGlVertex *const v = &sBatchGlVerts[vOut++];
                    v->x = xs[q];
                    v->y = wy;
                    v->z = zs[q];
                    v->u = (xs[q] + driftX) * invP + kBanks[b].offU;
                    v->v = (zs[q] + driftZ) * invP + kBanks[b].offV;
                    v->r = 244u;
                    v->g = 248u;
                    v->b = 251u;
                    v->a = (uint8_t)(baseAlpha * as[q] * 255.0f + 0.5f);
                }
            }
        }
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vOut);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Passes 5.4-5.8: 3D fog and sand banks, dust motes and weather particles.
static void Pass_Atmosphere3D(const VoxelFrame *const f)
{
    const bool indoor = f->indoor;
    const float fogDensity = f->fogDensity;
    const bool underground = f->underground;
    const VoxelWeatherClass weather = f->weather;
    const float weatherAmount = f->weatherAmount;
    const float bolt = f->bolt;
    const float seconds = f->seconds;
    if (f->crossed || f->mapChanged)
        sFogCacheValid = false;
    if (!indoor)
    {
        if (fogDensity > 0.005f)
        {
            uint8_t fr = underground ? 228 : 238;
            uint8_t fg = underground ? 236 : 244;
            uint8_t fb = underground ? 250 : 252;
            DrawFogBanks(sFogBanks, fogDensity, fr, fg, fb, f->clock);
        }
        if (weather == VOXEL_WEATHER_SAND && weatherAmount > 0.005f)
        {
            DrawFogBanks(sSandBanks, weatherAmount, 224, 190, 136, f->clock);
        }
        if (f->fly.active && f->fly.frame.wisps > 0.005f)
        {
            DrawCloudWisps(&f->fly.frame, f->clock);
        }

        // ---- PASS 5.6: 3D Sunlit Dust Motes & Night Fireflies ----
        {
            const VitaDayNightLight *const dn = VitaDayNight_Get();
            const float nightGlow = (dn != NULL) ? dn->glow : 0.0f;
            const bool fireflies = (f->hd2dPreset != HD2D_PRESET_OFF && !f->underground && nightGlow > 0.25f && weather != VOXEL_WEATHER_RAIN);
            const float moteStr = fireflies ? nightGlow : f->light.motes;
            if (moteStr > 0.005f)
            {
                DrawMotes(moteStr, seconds, fireflies, f->hd2dPreset);
            }
        }

        // ---- PASS 5.7: World-anchored Surface FX (water ripples, water glints, low water mist, sand gusts, ash embers) ----
        DrawSurfaceFx(f);

        // ---- PASS 5.75: World-anchored 3D Sun/Moon Rays (60 Classic, or fallback if HD-2D shaft shader unavailable) ----
        if (!underground && (!Hd2dPost(f->hd2dPreset) || !VoxelHd2dShader_ShaftsReady()
                             || !f->dioramaActive))
        {
            const VitaDayNightState *const dn = VitaDayNight_State();
            const VitaDayNightPath *const sunP = &dn->sunPath;
            const VitaDayNightPath *const moonP = &dn->moonPath;
            if (f->light.rays * sunP->strength > 0.005f)
            {
                DrawWorldRays(f->light.rays * sunP->strength, seconds, false, sunP);
            }
            else
            {
                // The same moon as the shader shafts: on its arc, and only in the dark.
                const float moonAmt = moonP->strength * 0.48f * dn->moonShafts;
                if (moonAmt > 0.048f && f->weatherAmount < 0.25f)
                    DrawWorldRays(moonAmt, seconds, true, moonP);
            }
        }

        // ---- PASS 5.8: 3D Weather Particles (Rain, Sandstorm, Volcanic Ash, Snow) ----
        if (weatherAmount > 0.005f)
        {
            const WeatherKind *wKind = NULL;
            if (weather == VOXEL_WEATHER_RAIN)           wKind = &sRainKind;
            else if (weather == VOXEL_WEATHER_SAND)      wKind = &sSandKind;
            else if (weather == VOXEL_WEATHER_ASH)       wKind = &sAshKind;
            else if (weather == VOXEL_WEATHER_PARTICLES) wKind = &sSnowKind;

            if (wKind != NULL)
            {
                DrawWeather(wKind, weatherAmount, bolt, f->clock, f->hd2dPreset);
            }
        }
    }
}

// Pass 5.9: 2D atmosphere: sun rays, lightning, cave gloom, and moonlight merged with the
// screen fade. Weather and day/night tint and haze are already baked per vertex, so a clear
// day draws no fullscreen quad here.
// Whether the night / fade overlay of Pass_Atmosphere2D is left to the post pass: its downsample
// and composite apply it (mix(c, overlay.rgb, overlay.a)) instead of a full-screen blended quad.
// Same image: it is the world scene's last layer, and the downsample feeds the blur and bloom.
static bool OverlayInPost(const VoxelFrame *const f)
{
    return f->dioramaActive && Hd2dPost(f->hd2dPreset) && VoxelHd2dShader_Ready();
}

// The night / fade overlay of this frame: rgb and alpha (0 when nothing is drawn).
static void AtmosphereOverlay(const VoxelFrame *const f, float overlay[4])
{
    const float fadeAmount = f->drawFade;
    const float *const fadeRgb = f->fadeRgb;
    overlay[0] = fadeRgb[0];
    overlay[1] = fadeRgb[1];
    overlay[2] = fadeRgb[2];
    overlay[3] = fadeAmount;
    if (!f->indoor)
    {
        const VitaDayNightLight *const dn = VitaDayNight_Get();
        const VitaDayNightBlend moon = VitaDayNight_Moonlight(dn, fadeAmount, fadeRgb);
        overlay[0] = moon.rgb[0];
        overlay[1] = moon.rgb[1];
        overlay[2] = moon.rgb[2];
        overlay[3] = moon.amount;
    }
    if (overlay[3] <= 0.005f || VitaPerf_PassSkipped(VITA_PERF_PASS_ATMOS2D))
        overlay[3] = 0.0f;
}

static void Pass_Atmosphere2D(const VoxelFrame *const f)
{
    if (VitaPerf_PassSkipped(VITA_PERF_PASS_ATMOS2D))
        return;
    const bool indoor = f->indoor;
    const float bolt = f->bolt;
    const bool underground = f->underground;
    static const float sUnitQuad[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glViewport(0, 0, f->worldW, f->worldH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);

    // 1. Lightning Flash (thunderstorm bolt)
    if (!indoor && bolt > 0.01f)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.90f, 0.94f, 1.00f, bolt * 0.45f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, sUnitQuad);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // 3. Underground Cave Gloom Vignette (matching DrawGloom in ctr_voxel.c)
    if (underground)
    {
        uint32_t vGloom = 0;
        PushFlatVertex2D(&vGloom, 0.0f, 0.00f, 8, 10, 20, 128);
        PushFlatVertex2D(&vGloom, 1.0f, 0.00f, 8, 10, 20, 128);
        PushFlatVertex2D(&vGloom, 1.0f, 0.28f, 8, 10, 20,   0);
        PushFlatVertex2D(&vGloom, 0.0f, 0.00f, 8, 10, 20, 128);
        PushFlatVertex2D(&vGloom, 1.0f, 0.28f, 8, 10, 20,   0);
        PushFlatVertex2D(&vGloom, 0.0f, 0.28f, 8, 10, 20,   0);

        PushFlatVertex2D(&vGloom, 0.0f, 0.78f, 8, 10, 20,   0);
        PushFlatVertex2D(&vGloom, 1.0f, 0.78f, 8, 10, 20,   0);
        PushFlatVertex2D(&vGloom, 1.0f, 1.00f, 8, 10, 20, 107);
        PushFlatVertex2D(&vGloom, 0.0f, 0.78f, 8, 10, 20,   0);
        PushFlatVertex2D(&vGloom, 1.0f, 1.00f, 8, 10, 20, 107);
        PushFlatVertex2D(&vGloom, 0.0f, 1.00f, 8, 10, 20, 107);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vGloom);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // 4. Single Merged Moonlight + Screen Fade Overlay (VitaDayNight_Moonlight merges both in one pass);
    // with the HD-2D post pass it is applied there (OverlayInPost).
    if (!OverlayInPost(f))
    {
        float overlay[4];
        AtmosphereOverlay(f, overlay);
        if (overlay[3] > 0.0f)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(overlay[0], overlay[1], overlay[2], overlay[3]);
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, sUnitQuad);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glDisableClientState(GL_VERTEX_ARRAY);
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

// Depth of field: the FBO scene downscaled to the scratch buffer, then presented with the
// blurred top and bottom bands.
// Draws `tex` over the bound target as four bilinear taps offset diagonally by (dx, dy) texels
// of `tex`, averaged by blending (alpha 1, 1/2, 1/3, 1/4: the running mean). Unit quad, ortho
// (0..1, 0..1); uv spans (0, 0)-(uMax, vMax).
static void BlurTaps(GLuint tex, float uMax, float vMax, float texW, float texH, float dx, float dy)
{
    static const float verts[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    static const float sign[4][2] = { { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f } };
    glBindTexture(GL_TEXTURE_2D, tex);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glDisable(GL_BLEND);
    for (int32_t i = 0; i < 4; ++i)
    {
        const float ox = sign[i][0] * dx / texW, oy = sign[i][1] * dy / texH;
        const float uvs[8] = { ox, oy, uMax + ox, oy, uMax + ox, vMax + oy, ox, vMax + oy };
        if (i == 1)
        {
            // Blended taps leave the alpha of the first one (1): blending it would lower it and
            // weaken the next passes, which blend by source alpha
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f / (float)(i + 1));
        glTexCoordPointer(2, GL_FLOAT, 0, uvs);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

static float DofSmooth(float e0, float e1, float x)
{
    float t = (x - e0) / (e1 - e0);
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}

// One blurred band over the screen (ortho 0..1, y down) as a strip of DOF_STEPS rows, from y0
// (focus side) to y1 (screen edge). The alpha of each row is peak * smoothstep(e0, e1, d), d = 0
// at the focus edge and 1 at the screen edge. The blur texture is read at the same place (its v
// is flipped: the FBO origin is at the bottom).
#define DOF_STEPS 12
static void DofBand(float y0, float y1, float uMax, float vMax, float peak, float e0, float e1)
{
    float verts[(DOF_STEPS + 1) * 4], uvs[(DOF_STEPS + 1) * 4];
    uint8_t colors[(DOF_STEPS + 1) * 8];
    for (int32_t k = 0; k <= DOF_STEPS; ++k)
    {
        const float d = (float)k / (float)DOF_STEPS;
        const float y = y0 + (y1 - y0) * d, v = vMax * (1.0f - y);
        const uint8_t a = (uint8_t)(peak * DofSmooth(e0, e1, d) * 255.0f + 0.5f);
        verts[k * 4 + 0] = 0.0f; verts[k * 4 + 1] = y; verts[k * 4 + 2] = 1.0f; verts[k * 4 + 3] = y;
        uvs[k * 4 + 0] = 0.0f; uvs[k * 4 + 1] = v; uvs[k * 4 + 2] = uMax; uvs[k * 4 + 3] = v;
        for (int32_t c = 0; c < 2; ++c)
        {
            colors[k * 8 + c * 4 + 0] = 255;
            colors[k * 8 + c * 4 + 1] = 255;
            colors[k * 8 + c * 4 + 2] = 255;
            colors[k * 8 + c * 4 + 3] = a;
        }
    }
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (DOF_STEPS + 1) * 2);
}

// Depth of field of the diorama (tilt-shift): only the top of the screen (the distance) and the
// bottom (the nearest ground) soften, progressively; sharp around the player. A light blur:
//   1. The scene is reduced to half size with one bilinear tap per texel (exactly a 2x2 box).
//      Stretched back with bilinear filtering, that is the light blur.
//   2. A wider level from it: four diagonal taps one texel out (Kawase style).
//   3. Over the sharp scene, each band gets the light blur fading in from the focus edge and the
//      wider one fading in near the screen edge (smoothstep ramps, 12 rows per band).
// Fixed function only: 5 draws of 480x272 pixels, then the two bands twice.
// The sun (0) and moon (1) passes of the sky bloom and shafts this frame: their screen paths
// and strengths, from the frame's day/night state.
typedef struct
{
    bool on;
    float amt;
    VitaDayNightPath path;
} ShaftPass;

static void ShaftPasses(const VoxelFrame *const f, ShaftPass out[2])
{
    const VitaDayNightState *const dn = VitaDayNight_State();
    out[0].path = dn->sunPath;
    out[0].amt = f->light.rays * dn->sunPath.strength;
    out[1].path = dn->moonPath;
    out[1].amt = (f->weatherAmount < 0.25f) ? (dn->moonPath.strength * 0.48f * dn->moonShafts) : 0.0f;
    for (int32_t k = 0; k < 2; ++k)
        out[k].on = out[k].amt > 0.005f;
}


// The HD-2D composite's parameters for the frame (grade, DoF bands, bloom, battle terms, overlay).
// Worked out before the post targets are drawn: HIGH's pack pass needs them ahead of the composite.
typedef struct
{
    float shadowTint[3], highTint[3];
    float uvTop, uvBot, topAlpha, botAlpha;
    float bloomAmt, gradeAmt, saturation, curveAmt, vignetteAmt;
    float backBlur, backDim;
    float overlay[4];
    float lowHpPulse;
} PostCompose;

static void PostComposeOf(const VoxelFrame *const f, const bool hd2dHigh, const bool dofEnabled,
                          const bool haveBloom, PostCompose *const out)
{
    // Stronger HD-2D split-tone tints + saturation + S-curve (requested more aggressive grading)
    static const float kShadowTint[3][3] = {
        { 0.84f, 0.89f, 1.12f }, // Day (crisp cool indigo shadows)
        { 0.96f, 0.82f, 1.14f }, // Dusk (deep violet shadows)
        { 0.50f, 0.58f, 0.96f }, // Night (deep midnight blue shadows)
    };
    static const float kHighTint[3][3] = {
        { 1.03f, 0.99f, 0.88f }, // Day (warm golden highlights)
        { 1.08f, 0.92f, 0.74f }, // Dusk (sunset amber)
        { 0.72f, 0.78f, 0.96f }, // Night (darker ambient so lamps & bloom pop)
    };
    static const float kLutCurve[3] = { 0.16f, 0.24f, 0.12f };

    // The day/night grade weights of the frame's state (indoors: day).
    const VitaDayNightState *const dnState = VitaDayNight_State();
    const float glow  = f->indoor ? 0.0f : dnState->light.glow;
    const float dusk  = f->indoor ? 0.0f : dnState->dusk;
    const float night = f->indoor ? 0.0f : dnState->night;
    const float day   = f->indoor ? 1.0f : dnState->day;
    const float sum   = 1.0f;

    for (int32_t k = 0; k < 3; ++k)
    {
        out->shadowTint[k] = (kShadowTint[0][k] * day + kShadowTint[1][k] * dusk + kShadowTint[2][k] * night) / sum;
        out->highTint[k]   = (kHighTint[0][k]   * day + kHighTint[1][k]   * dusk + kHighTint[2][k]   * night) / sum;
    }
    const float baseCurve = (kLutCurve[0] * day + kLutCurve[1] * dusk + kLutCurve[2] * night) / sum;

    float yTop = f->playerScreenY - (hd2dHigh ? 0.11f : 0.16f);
    float yBot = f->playerScreenY + (hd2dHigh ? 0.15f : 0.16f);
    if (yTop < 0.16f) yTop = 0.16f;
    if (yTop > 0.46f) yTop = 0.46f;
    if (yBot < 0.58f) yBot = 0.58f;
    if (yBot > 0.88f) yBot = 0.88f;
    // In shader vUv.y=1 is top, vUv.y=0 is bottom -> invert screen-down Y
    out->uvTop = 1.0f - yTop;
    out->uvBot = 1.0f - yBot;
    const float indoorScale = f->indoor ? 0.62f : 1.0f;
    out->topAlpha = dofEnabled ? ((hd2dHigh ? 0.82f : VOXEL_DOF_TOP_ALPHA) * indoorScale) : 0.0f;
    out->botAlpha = dofEnabled ? ((hd2dHigh ? 0.55f : VOXEL_DOF_BOTTOM_ALPHA) * indoorScale) : 0.0f;

    const bool marked      = f->hd2dMarkedGrade;
    const float nightBlend = Voxel_ClampF(glow * 4.0f, 0.0f, 1.0f);
    const float outBloom   = hd2dHigh
                           ? (0.96f + (1.15f - 0.96f) * nightBlend)
                           : (0.32f + (0.34f - 0.32f) * nightBlend);
    out->bloomAmt    = haveBloom ? (f->indoor ? (hd2dHigh ? 0.42f : 0.18f) : outBloom) : 0.0f;
    out->gradeAmt    = marked ? (hd2dHigh ? 1.00f : 0.85f) : 0.25f;
    out->saturation  = marked ? (hd2dHigh ? 1.16f : 1.10f) : 1.02f;
    out->curveAmt    = marked ? (baseCurve * (hd2dHigh ? 1.15f : 0.85f)) : (baseCurve * 0.35f);
    out->vignetteAmt = marked ? (hd2dHigh ? 0.28f : 0.16f) : 0.08f;
    out->backBlur    = f->inBattle3D ? 1.0f : 0.0f;
    out->backDim     = f->inBattle3D ? (hd2dHigh ? 0.45f : 0.35f) : 0.0f;

    // The overlay is the scene's last layer: the composite applies it to the full-res scene.
    // In a 3D battle the backdrop comes from the blur, which the downsample already covered.
    AtmosphereOverlay(f, out->overlay);
    if (f->inBattle3D)
        out->overlay[3] = 0.0f;

    out->lowHpPulse = 0.0f;
    const GameFrameBattle *const battle = &GameFrame_Get()->battle;
    if (f->inBattle3D && battle->maxHp[0] > 0u && battle->hp[0] > 0u)
    {
        const float hpRatio = (float)battle->hp[0] / (float)battle->maxHp[0];
        if (hpRatio < 0.25f)
            out->lowHpPulse = (0.45f + 0.55f * sinf(f->seconds * 6.5f)) * (1.0f - hpRatio * 4.0f);
    }
}

// Into the bound bloom source (its own target, or HIGH's band): the directional sky bloom and the
// shader light shafts of each pass that is on, additive, before the bloom blur so sky and beams
// bloom at all hours. src: where each pass's shaft profile is.
static void DrawSkyBloomAndShafts(const VoxelFrame *const f, const ShaftPass shaftPasses[2], const bool skyOrShafts,
                                  const float skyBloomScale, const bool shaderShafts, const float shaftsScale,
                                  const bool hd2dHigh, const float aspect, const VoxelHd2dShaftSource src[2])
{
    // Directional sky bloom + procedural light shafts added BEFORE 2-pass Kawase blur so sky & beams bloom at all hours
    if (skyOrShafts)
    {
        const float glow = Voxel_ClampF(VitaDayNight_State()->light.glow, 0.0f, 1.0f);

        for (int32_t pass = 0; pass < 2; ++pass)
        {
            const bool isDay = (pass == 0);
            const float amt = shaftPasses[pass].amt;
            if (!shaftPasses[pass].on)
                continue;

            static const float kShaftDay[3]  = { 1.00f, 0.92f, 0.74f };
            static const float kShaftGold[3] = { 1.00f, 0.72f, 0.38f };
            static const float kShaftMoon[3] = { 0.52f, 0.66f, 1.00f };
            const VitaDayNightPath p = shaftPasses[pass].path;
            const float goldK = isDay ? Voxel_ClampF(glow * 4.0f, 0.0f, 1.0f) : 0.0f;
            float col[3];
            for (int32_t k = 0; k < 3; ++k)
                col[k] = isDay ? (kShaftDay[k] + (kShaftGold[k] - kShaftDay[k]) * goldK) : kShaftMoon[k];

            if (sWeatherAtlasTex != 0 && skyBloomScale > 0.0f && !VitaPerf_PassSkipped(VITA_PERF_PASS_SKYBLOOM))
            {
                static const float qx[6] = {-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f};
                static const float qy[6] = {-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f};
                const float sx = 0.5f - p.dx * 0.30f;
                const float sy = 0.94f;
                const float rx = 0.38f, ry = 0.30f;
                const float skyStr = amt * (isDay ? 155.0f : 115.0f) * skyBloomScale;
                for (int32_t k = 0; k < 6; ++k)
                {
                    VoxelGlVertex *const v = &sBatchGlVerts[k];
                    v->x = sx + qx[k] * rx;
                    v->y = sy + qy[k] * ry;
                    v->z = 0.0f;
                    v->u = (qx[k] > 0.0f) ? 0.5f : 0.0f;
                    v->v = (qy[k] > 0.0f) ? 1.0f : 0.5f;
                    v->r = (uint8_t)Voxel_ClampF(col[0] * skyStr, 0.0f, 255.0f);
                    v->g = (uint8_t)Voxel_ClampF(col[1] * skyStr, 0.0f, 255.0f);
                    v->b = (uint8_t)Voxel_ClampF(col[2] * skyStr, 0.0f, 255.0f);
                    v->a = 255;
                }
                glBindTexture(GL_TEXTURE_2D, sWeatherAtlasTex);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnableClientState(GL_COLOR_ARRAY);
                glVertexPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
                glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisableClientState(GL_COLOR_ARRAY);
            }

            if (shaderShafts)
            {
                const float gain = (isDay
                    ? (amt * (hd2dHigh ? 1.55f : 1.15f))
                    : (amt * (hd2dHigh ? 0.52f : 0.30f))) * shaftsScale;
                const float elev = Voxel_ClampF((p.altitude - 0.15f) / 0.75f, 0.0f, 1.0f);
                const float lenExp = 0.62f + elev * 0.35f;
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
                VoxelHd2dShader_Shafts((uint32_t)pass, p.dx, p.dy, lenExp, &f->shaftClock[pass], aspect,
                                       col[0] * gain, col[1] * gain, col[2] * gain,
                                       &src[pass]);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                sFrameDrawCalls++;
                sFrameVertCount += 4u;
            }
        }
    }
}

// The bright pass's threshold and gain for the frame (WORLD BLOOM option, day/night, weather).
static void BloomBrightOf(const VoxelFrame *const f, const bool hd2dHigh, float *const thresh, float *const gain)
{
    const int32_t bloomLevel = VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLOOM);
    *gain = Voxel_ClampF((float)bloomLevel * 0.1f, 0.0f, 1.0f);
    const VitaDayNightLight *const bdn = (!f->indoor) ? VitaDayNight_Get() : NULL;
    const float glow = (bdn != NULL) ? Voxel_ClampF(bdn->glow, 0.0f, 1.0f) : 0.0f;
    // Balanced daytime threshold (0.63) keeps grass/sand crisp while letting sunlit highlights bloom; dusk/night stay at 0.44.
    const float dayThresh = hd2dHigh ? 0.63f : 0.67f;
    const float indoorThresh = 0.84f;
    const float outThresh = hd2dHigh
        ? (dayThresh + (0.44f - dayThresh) * glow)
        : (dayThresh + (0.54f - dayThresh) * glow);
    *thresh = (f->weather == VOXEL_WEATHER_SAND) ? 0.86f
            : (f->indoor ? indoorThresh : outThresh);
}

static void Pass_DepthOfField(const VoxelFrame *const f, const GLuint targetFbo)
{
    const int32_t renderW = f->renderW;
    const int32_t renderH = f->renderH;
    const float uMax = (float)renderW / (float)VOXEL_DIORAMA_FBO_W;
    const float vMax = (float)renderH / (float)VOXEL_DIORAMA_TEX_H;
    const int32_t scratchW = renderW / 2;
    const int32_t scratchH = renderH / 2;
    const float uS = (float)scratchW / (float)VOXEL_SCRATCH_FBO_W;
    const float vS = (float)scratchH / (float)VOXEL_SCRATCH_FBO_H;
    const bool hd2dOn = Hd2dPost(f->hd2dPreset);   // the post effects (MED / HIGH)
    const bool hd2dHigh = (f->hd2dPreset == HD2D_PRESET_HIGH);
    const PostRenderTargets *const pt = &sPostTargets[sPostTargetIndex];
    // The DoF bands; with DoF OFF the pass still runs for bloom, sky bloom and shafts. The HD-2D
    // composite leaves them out of 3D battles (its backdrop is blurred whole); the fixed-function
    // path draws them there too, as before.
    const bool dofSetting = !f->fly.active && (VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLUR) != 0);
    const bool dofEnabled = dofSetting && !f->inBattle3D;
    // The wider blur (step 2) feeds only the DoF bands and the 3D battle backdrop.
    const bool useBlurFbo = (f->inBattle3D || (dofSetting && (!hd2dOn || hd2dHigh)))
                         && !VitaPerf_PassSkipped(VITA_PERF_PASS_BLUR);
    const bool bloomReady = hd2dOn && (pt->bloomFboA != 0 && pt->bloomFboB != 0)
                         && !VitaPerf_PassSkipped(VITA_PERF_PASS_BLOOM);
    // HIGH packs everything of the composite that is not full resolution into the blur target,
    // after the bloom (VoxelHd2dShader_Pack); that pass replaces the wide blur below.
    const bool packPath = hd2dHigh && VoxelHd2dShader_PackReady() && pt->blurFbo != 0;
    const float uBl = (float)(renderW / 4) / (float)VOXEL_BLOOM_FBO_W;
    const float vBl = (float)(renderH / 4) / (float)VOXEL_BLOOM_FBO_H;
    const int32_t bloomW = renderW / 4;
    const int32_t bloomH = renderH / 4;
    const float aspect = (float)renderW / (float)renderH;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    // Sky bloom and shafts: which passes run.
    const float skyBloomScale = Voxel_ClampF((float)VitaExtras_GetFast(FAST_EXTRA_VOXEL_SKY_BLOOM) * 0.2f, 0.0f, 2.0f);
    const float shaftsScale   = Voxel_ClampF((float)VitaExtras_GetFast(FAST_EXTRA_VOXEL_SHAFTS) * 0.2f, 0.0f, 2.0f);
    const bool skyOrShafts = !f->indoor && !f->inBattle3D && !f->underground
                          && (skyBloomScale > 0.0f || shaftsScale > 0.0f);
    const bool shaderShafts = skyOrShafts && hd2dOn && shaftsScale > 0.0f && VoxelHd2dShader_ShaftsReady()
                           && !VitaPerf_PassSkipped(VITA_PERF_PASS_SHAFTS);
    ShaftPass shaftPasses[2];
    ShaftPasses(f, shaftPasses);
    float brightThresh, brightGain;
    BloomBrightOf(f, hd2dHigh, &brightThresh, &brightGain);

    // HIGH with the RGBA8 scene: the shaft profiles go in the scene target's band and the bloom
    // source in the band of bandTex, inside scenes that run anyway (5 render targets a frame
    // instead of 7-8). The bloom blur must run: it is what reads the band.
    const bool bandPath = packPath && bloomReady && pt->bandFbo != 0 && VoxelHd2dShader_BandReady()
                       && pt->dioramaActiveTex == pt->dioramaColorTex
                       && !VitaPerf_PassSkipped(VITA_PERF_PASS_KAWASE);
    // A 3D battle in HIGH drew its stage at half size in the scene target (Frame_BeginPasses): it
    // is the blur's source as it is, there is no downsample.
    const bool battleHalf = f->battleHalf && packPath;
    const float uHalf = (float)f->worldW / (float)VOXEL_DIORAMA_FBO_W;
    const float vHalf = (float)f->worldH / (float)VOXEL_DIORAMA_TEX_H;
    const GLuint downFbo = bandPath ? pt->bandFbo : pt->scratchFbo;
    const GLuint downTex = bandPath ? pt->bandTex : pt->scratchColorTex;
    const float vDown = bandPath ? (float)scratchH / (float)VOXEL_BAND_TEX_H : vS;
    const float downTexelY = 1.0f / (float)(bandPath ? VOXEL_BAND_TEX_H : VOXEL_SCRATCH_FBO_H);
    VoxelHd2dShaftSource shaftSrc[2] = { { 0, 1.0f, 0.5f }, { 0, 1.0f, 0.5f } };

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 0. HIGH: the shaft profiles as rows of the band above the world, while the scene target is
    // still bound.
    if (bandPath && shaderShafts)
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        for (int32_t pass = 0; pass < 2; ++pass)
        {
            if (!shaftPasses[pass].on)
                continue;
            VoxelHd2dShader_ShaftLutBand((uint32_t)pass, shaftPasses[pass].path.dx, shaftPasses[pass].path.dy,
                                         &f->shaftClock[pass], aspect, 2.0f, pt->dioramaActiveTex,
                                         VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_TEX_H, VOXEL_SHAFT_BAND_Y,
                                         &shaftSrc[pass]);
            sFrameDrawCalls++;
            sFrameVertCount += 4u;
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    VitaPerf_SubBegin(VITA_PERF_T_POST_DOF);
    // 1. Downsample: 2x2 box filter in HD-2D to eliminate roof/wall horizontal stripe aliasing.
    // At half size each pixel's centre falls on the shared corner of its 2x2 block, so one
    // bilinear tap is that block's average: the filter is switched to linear for this draw only
    // (the composite reads the scene texture unfiltered).
    glBindFramebuffer(GL_FRAMEBUFFER, downFbo);
    VitaPerf_GpuScene(VITA_PERF_GPU_DOWN);
    glViewport(0, 0, scratchW, scratchH);
    glBindTexture(GL_TEXTURE_2D, pt->dioramaActiveTex);
    if (hd2dOn && VoxelHd2dShader_Ready())
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        float overlay[4];
        AtmosphereOverlay(f, overlay);
        if (!battleHalf)
            VoxelHd2dShader_Downsample(pt->dioramaActiveTex, uMax, vMax, overlay);
        if (bandPath)
        {
            // The bloom source in the band, from the scene still filtered: the bright pass, then
            // the sky bloom and the shafts (their profiles are in the scene's band).
            sFrameDrawCalls++;
            sFrameVertCount += 4u;
            glViewport(0, VOXEL_BLOOM_BAND_Y, bloomW, bloomH);
            // From the half-size battle stage the taps sit half a texel around each pixel: the
            // texels themselves, the 2x2 box one tap of the downsample gave.
            const float tapStep = battleHalf ? 0.5f : 1.0f;
            VoxelHd2dShader_BrightPassBox4(pt->dioramaActiveTex, battleHalf ? uHalf : uMax, battleHalf ? vHalf : vMax,
                                           tapStep / (float)VOXEL_DIORAMA_FBO_W, tapStep / (float)VOXEL_DIORAMA_TEX_H,
                                           overlay, brightThresh, brightGain);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            DrawSkyBloomAndShafts(f, shaftPasses, skyOrShafts, skyBloomScale, shaderShafts, shaftsScale,
                                  hd2dHigh, aspect, shaftSrc);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glDisable(GL_BLEND);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
        }
        glBindTexture(GL_TEXTURE_2D, pt->dioramaActiveTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        sFrameDrawCalls++;
        sFrameVertCount += 4u;
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        {
            const float uvs[8] = { 0.0f, 0.0f, uMax, 0.0f, uMax, vMax, 0.0f, vMax };
            glVertexPointer(2, GL_FLOAT, 0, quad);
            glTexCoordPointer(2, GL_FLOAT, 0, uvs);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // 1.5 HIGH: the 8-tap bloom blur from the band into bloomFboB.
    GLuint activeBloomTex = 0;
    if (bandPath)
    {
        VitaPerf_SubBegin(VITA_PERF_T_POST_BLOOM);
        glDisable(GL_BLEND);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboB);
        VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
        glViewport(0, 0, bloomW, bloomH);
        if (VoxelHd2dShader_Kawase8Band(pt->bandTex, 0.0f, (float)VOXEL_BLOOM_BAND_Y / (float)VOXEL_BAND_TEX_H,
                                        (float)bloomW / (float)VOXEL_SCRATCH_FBO_W, (float)bloomH / (float)VOXEL_BAND_TEX_H,
                                        1.0f / (float)VOXEL_SCRATCH_FBO_W, 1.0f / (float)VOXEL_BAND_TEX_H))
        {
            activeBloomTex = pt->bloomTexB;
            sFrameDrawCalls++;
            sFrameVertCount += 4u;
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        VitaPerf_SubEnd(VITA_PERF_T_POST_BLOOM);
    }

    // 2. Wider blur -> pt->blurTex (480x272): only for the DoF bands (not in HD-2D LOW) and the 3D battle backdrop.
    if (useBlurFbo && !packPath)
    {
        const float blurStep = hd2dHigh ? 1.55f : 1.0f;
        glBindFramebuffer(GL_FRAMEBUFFER, pt->blurFbo);
        VitaPerf_GpuScene(VITA_PERF_GPU_BLUR);
        glViewport(0, 0, scratchW, scratchH);
        if (VoxelHd2dShader_Ready())
        {
            glDisable(GL_BLEND);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            VoxelHd2dShader_CopyBox2x2(pt->scratchColorTex, uS, vS,
                                       blurStep / (float)VOXEL_SCRATCH_FBO_W,
                                       blurStep / (float)VOXEL_SCRATCH_FBO_H);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            sFrameDrawCalls++;
            sFrameVertCount += 4u;
        }
        else
        {
            BlurTaps(pt->scratchColorTex, uS, vS, (float)VOXEL_SCRATCH_FBO_W, (float)VOXEL_SCRATCH_FBO_H, blurStep, blurStep);
        }
    }
    VitaPerf_SubEnd(VITA_PERF_T_POST_DOF);

    // 2.5 Quarter-res HD-2D Bloom (240x136), in targets of its own (MED, or HIGH without the band)
    if (bloomReady && !bandPath)
    {
        VitaPerf_SubBegin(VITA_PERF_T_POST_BLOOM);
        const float uB = (float)bloomW / (float)VOXEL_BLOOM_FBO_W;
        const float vB = (float)bloomH / (float)VOXEL_BLOOM_FBO_H;

        // The shafts' beam profiles are evaluated first, in their own small targets, because the
        // bloom scene below reads them.
        if (shaderShafts)
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            for (int32_t pass = 0; pass < 2; ++pass)
            {
                if (!shaftPasses[pass].on)
                    continue;
                VoxelHd2dShader_BeginShaftLut((uint32_t)pass);
                VitaPerf_GpuScene(VITA_PERF_GPU_BLOOM);
                VoxelHd2dShader_ShaftLut((uint32_t)pass, shaftPasses[pass].path.dx, shaftPasses[pass].path.dy,
                                         &f->shaftClock[pass], aspect, hd2dHigh ? 2.0f : 1.0f);
                VoxelHd2dShader_ShaftSourceOwn((uint32_t)pass, &shaftSrc[pass]);
                sFrameDrawCalls++;
                sFrameVertCount += 4u;
            }
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboA);
        VitaPerf_GpuScene(VITA_PERF_GPU_BLOOM);
        glViewport(0, 0, bloomW, bloomH);

        // Bright-pass at 240x136 (bravely-emerald style: extract real 3D bright pixels with depth occlusion)
        // The shader bright pass writes every pixel opaque with blending off; a vitaGL clear is a
        // full-target draw of its own, so it is only needed when that pass does not run.
        if (brightGain <= 0.0f || !VoxelHd2dShader_Ready())
        {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        if (brightGain <= 0.0f)
        {
            // 0 = OFF: keep bloomFboA cleared so sky bloom & light shafts still run without world bright-pass bloom
        }
        else if (VoxelHd2dShader_Ready())
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            VoxelHd2dShader_BrightPass(pt->scratchColorTex, uS, vS, brightThresh, brightGain);
            sFrameDrawCalls++;
            sFrameVertCount += 4u;
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnable(GL_BLEND);
        }
        else
        {
            const float gain = (f->indoor ? 0.35f : (hd2dHigh ? 0.65f : 0.45f)) * brightGain;
            const float uvs[8] = { 0.0f, 0.0f, uS, 0.0f, uS, vS, 0.0f, vS };
            glBindTexture(GL_TEXTURE_2D, pt->scratchColorTex);
            glDisable(GL_BLEND);
            glColor4f(gain, gain, gain, 1.0f);
            glVertexPointer(2, GL_FLOAT, 0, quad);
            glTexCoordPointer(2, GL_FLOAT, 0, uvs);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        DrawSkyBloomAndShafts(f, shaftPasses, skyOrShafts, skyBloomScale, shaderShafts, shaftsScale,
                              hd2dHigh, aspect, shaftSrc);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDisable(GL_BLEND);

        // In HIGH mode, blur the bloom for soft halos on lamps, sky & shafts: one 8-tap pass
        // (bloomFboA -> bloomFboB, VoxelHd2dShader_Kawase8) with the reach of the two 4-tap Kawase
        // passes it replaced, one render target less. The 2-pass paths remain as fallbacks.
        bool bloomInB = false;
        if (hd2dHigh && !VitaPerf_PassSkipped(VITA_PERF_PASS_KAWASE) && VoxelHd2dShader_Ready())
        {
            glDisable(GL_BLEND);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboB);
            VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
            glViewport(0, 0, bloomW, bloomH);
            bloomInB = VoxelHd2dShader_Kawase8(pt->bloomTexA, uB, vB,
                                               1.0f / (float)VOXEL_BLOOM_FBO_W, 1.0f / (float)VOXEL_BLOOM_FBO_H);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            if (bloomInB)
            {
                sFrameDrawCalls++;
                sFrameVertCount += 4u;
            }
        }
        if (bloomInB)
        {
            activeBloomTex = pt->bloomTexB;
        }
        else if (hd2dHigh && !VitaPerf_PassSkipped(VITA_PERF_PASS_KAWASE))
        {
            if (VoxelHd2dShader_Ready())
            {
                glDisable(GL_BLEND);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
                glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboB);
                VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
                glViewport(0, 0, bloomW, bloomH);
                VoxelHd2dShader_CopyBox2x2(pt->bloomTexA, uB, vB,
                                           1.50f / (float)VOXEL_BLOOM_FBO_W,
                                           1.50f / (float)VOXEL_BLOOM_FBO_H);
                glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboA);
                VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
                glViewport(0, 0, bloomW, bloomH);
                VoxelHd2dShader_CopyBox2x2(pt->bloomTexB, uB, vB,
                                           2.75f / (float)VOXEL_BLOOM_FBO_W,
                                           2.75f / (float)VOXEL_BLOOM_FBO_H);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                sFrameDrawCalls += 2u;
                sFrameVertCount += 8u;
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboB);
                VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
                glViewport(0, 0, bloomW, bloomH);
                BlurTaps(pt->bloomTexA, uB, vB, (float)VOXEL_BLOOM_FBO_W, (float)VOXEL_BLOOM_FBO_H, 1.50f, 1.50f);
                glBindFramebuffer(GL_FRAMEBUFFER, pt->bloomFboA);
                VitaPerf_GpuScene(VITA_PERF_GPU_KAWASE);
                glViewport(0, 0, bloomW, bloomH);
                BlurTaps(pt->bloomTexB, uB, vB, (float)VOXEL_BLOOM_FBO_W, (float)VOXEL_BLOOM_FBO_H, 2.75f, 2.75f);
            }
            activeBloomTex = pt->bloomTexA;
        }
        else
        {
            activeBloomTex = pt->bloomTexA;
        }
        VitaPerf_SubEnd(VITA_PERF_T_POST_BLOOM);
    }

    const bool shaderComposite = hd2dOn && VoxelHd2dShader_Ready();
    PostCompose pc;
    if (shaderComposite)
        PostComposeOf(f, hd2dHigh, dofEnabled, activeBloomTex != 0, &pc);

    // 2.7 HIGH: the pack pass into the blur target. The 4-tap blur of the downsample, the DoF
    // weight, the overlay, the bloom and the battle's back blur and dim, folded into what the
    // full-resolution scene keeps and what is added to it.
    if (shaderComposite && packPath)
    {
        VitaPerf_SubBegin(VITA_PERF_T_POST_DOF);
        glBindFramebuffer(GL_FRAMEBUFFER, pt->blurFbo);
        VitaPerf_GpuScene(VITA_PERF_GPU_BLUR);
        glViewport(0, 0, scratchW, scratchH);
        glDisable(GL_BLEND);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        VoxelHd2dPack pk;
        pk.srcTex = battleHalf ? pt->dioramaActiveTex : downTex;
        pk.srcU = battleHalf ? uHalf : uS;
        pk.srcV = battleHalf ? vHalf : vDown;
        pk.texelX = 1.55f / (float)(battleHalf ? VOXEL_DIORAMA_FBO_W : VOXEL_SCRATCH_FBO_W);
        pk.texelY = 1.55f * (battleHalf ? 1.0f / (float)VOXEL_DIORAMA_TEX_H : downTexelY);
        pk.bloomTex = (activeBloomTex != 0) ? activeBloomTex : pt->scratchColorTex;
        pk.bloomU = uBl;
        pk.bloomV = vBl;
        pk.bloomAmt = pc.bloomAmt;
        pk.uvTop = pc.uvTop;
        pk.uvBot = pc.uvBot;
        pk.topAlpha = pc.topAlpha;
        pk.botAlpha = pc.botAlpha;
        pk.minBlur = pc.backBlur;
        pk.dim = 1.0f - pc.backDim;
        memcpy(pk.overlay, pc.overlay, sizeof(pk.overlay));
        if (battleHalf)
        {
            AtmosphereOverlay(f, pk.srcOverlay);
            // The stage is read between its texels, as the downsample's own target was.
            glBindTexture(GL_TEXTURE_2D, pt->dioramaActiveTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            memset(pk.srcOverlay, 0, sizeof(pk.srcOverlay));
        }
        VoxelHd2dShader_Pack(&pk);
        if (battleHalf)
        {
            glBindTexture(GL_TEXTURE_2D, pt->dioramaActiveTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        sFrameDrawCalls++;
        sFrameVertCount += 4u;
        VitaPerf_SubEnd(VITA_PERF_T_POST_DOF);
    }

    VitaPerf_SubBegin(VITA_PERF_T_POST_COMPOSITE);
    // 3. Final composite to targetFbo (0 = screen, sMenuBgFbo = menu freeze capture)
    glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);
    VitaPerf_GpuScene(VITA_PERF_GPU_FINAL);
    glViewport(0, 0, renderW, renderH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (shaderComposite)
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_BLEND);

        if (packPath)
        {
            VoxelHd2dGrade grade;
            grade.gradeAmt = pc.gradeAmt;
            grade.saturation = pc.saturation;
            grade.curveAmt = pc.curveAmt;
            grade.vignetteAmt = pc.vignetteAmt;
            memcpy(grade.shadowTint, pc.shadowTint, sizeof(grade.shadowTint));
            memcpy(grade.highTint, pc.highTint, sizeof(grade.highTint));
            if (f->inBattle3D)
            {
                VoxelHd2dShader_CompositePackedBattle(pt->blurTex, uS, vS, &grade, pc.lowHpPulse);
            }
            else
            {
                // The measuring build's composite ladder (bench masks): the reads without the
                // grade, then a single read of the scene.
                const VoxelHd2dCompVariant variant = VitaPerf_PassSkipped(VITA_PERF_PASS_COMPREADS)
                    ? VOXEL_HD2D_COMP_COPY
                    : (VitaPerf_PassSkipped(VITA_PERF_PASS_COMPGRADE) ? VOXEL_HD2D_COMP_READS : VOXEL_HD2D_COMP_FULL);
                VoxelHd2dShader_CompositePacked(pt->dioramaActiveTex, uMax, vMax, pt->blurTex, uS, vS,
                                                &grade, variant);
            }
        }
        else
        {
            const GLuint sceneOrBattleTex = f->inBattle3D ? (useBlurFbo ? pt->blurTex : pt->scratchColorTex) : pt->dioramaActiveTex;
            const float uMain = f->inBattle3D ? uS : uMax;
            const float vMain = f->inBattle3D ? vS : vMax;
            VoxelHd2dShader_Composite(sceneOrBattleTex,
                                      useBlurFbo ? pt->blurTex : pt->scratchColorTex,
                                      (activeBloomTex != 0) ? activeBloomTex : pt->scratchColorTex,
                                      uMain, vMain, uS, vS, uBl, vBl,
                                      pc.uvTop, pc.uvBot, pc.topAlpha, pc.botAlpha,
                                      pc.bloomAmt, pc.gradeAmt, pc.curveAmt, pc.vignetteAmt,
                                      pc.saturation, pc.backBlur, pc.backDim,
                                      0.0f, 0.0f, pc.lowHpPulse,
                                      pc.shadowTint, pc.highTint, pc.overlay);
        }

        sFrameDrawCalls++;
        sFrameVertCount += 4u;
        VitaPerf_SubEnd(VITA_PERF_T_POST_COMPOSITE);
        return;
    }

    if (hd2dOn && f->inBattle3D)
    {
        // F5: Blurred & dark split-toned 3D battle stage (Octopath / Triangle Strategy framing)
        glBindTexture(GL_TEXTURE_2D, useBlurFbo ? pt->blurTex : pt->scratchColorTex);
        glColor4f(0.58f, 0.62f, 0.74f, 1.0f);
        const float uvs[8] = { 0.0f, vS, uS, vS, uS, 0.0f, 0.0f, 0.0f };
        glVertexPointer(2, GL_FLOAT, 0, quad);
        glTexCoordPointer(2, GL_FLOAT, 0, uvs);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, pt->dioramaActiveTex);
        {
            const float baseUvs[8] = { 0.0f, vMax, uMax, vMax, uMax, 0.0f, 0.0f, 0.0f };
            glVertexPointer(2, GL_FLOAT, 0, quad);
            glTexCoordPointer(2, GL_FLOAT, 0, baseUvs);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        if (dofSetting)
        {
            float yTop = hd2dOn ? (f->playerScreenY - 0.16f) : (VOXEL_DOF_TOP_ROWS / 544.0f);
            float yBot = hd2dOn ? (f->playerScreenY + 0.16f) : (1.0f - VOXEL_DOF_BOTTOM_ROWS / 544.0f);
            if (hd2dOn)
            {
                if (yTop < 0.14f) yTop = 0.14f;
                if (yTop > 0.44f) yTop = 0.44f;
                if (yBot < 0.58f) yBot = 0.58f;
                if (yBot > 0.88f) yBot = 0.88f;
            }
            const float indoorScale = f->indoor ? 0.62f : 1.0f;
            const float topAlpha = VOXEL_DOF_TOP_ALPHA * indoorScale;
            const float botAlpha = VOXEL_DOF_BOTTOM_ALPHA * indoorScale;

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnableClientState(GL_COLOR_ARRAY);

            glBindTexture(GL_TEXTURE_2D, pt->scratchColorTex);
            if (useBlurFbo)
            {
                DofBand(yTop, 0.0f, uS, vS, topAlpha, 0.0f, 0.55f);
                DofBand(yBot, 1.0f, uS, vS, botAlpha, 0.0f, 0.55f);
                glBindTexture(GL_TEXTURE_2D, pt->blurTex);
                DofBand(yTop, 0.0f, uS, vS, topAlpha, 0.55f, 1.0f);
                DofBand(yBot, 1.0f, uS, vS, botAlpha, 0.55f, 1.0f);
            }
            else
            {
                // 60 FPS LOW mode: single smooth bilinear blur band from sScratchColorTex
                DofBand(yTop, 0.0f, uS, vS, topAlpha * 0.85f, 0.0f, 1.0f);
                DofBand(yBot, 1.0f, uS, vS, botAlpha * 0.85f, 0.0f, 1.0f);
            }
            glDisableClientState(GL_COLOR_ARRAY);
        }
    }

    // Additive HD-2D Bloom overlay
    if (activeBloomTex != 0)
    {
        const float uB = (float)(renderW / 4) / (float)VOXEL_BLOOM_FBO_W;
        const float vB = (float)(renderH / 4) / (float)VOXEL_BLOOM_FBO_H;
        const float bloomGain = hd2dHigh ? 0.36f : 0.24f;
        const float bUvs[8] = { 0.0f, vB, uB, vB, uB, 0.0f, 0.0f, 0.0f };
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBindTexture(GL_TEXTURE_2D, activeBloomTex);
        glColor4f(bloomGain, bloomGain, bloomGain, 1.0f);
        glVertexPointer(2, GL_FLOAT, 0, quad);
        glTexCoordPointer(2, GL_FLOAT, 0, bUvs);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // Split-tone cinematic grading (GBA faithful vs HD-2D marked) + radial vignette combined into minimal passes
    if (hd2dOn)
    {
        const bool marked = f->hd2dMarkedGrade;
        glEnable(GL_BLEND);

        // 1. Warm top-down sun wash + lifted slate-indigo shadows (untextured, disable TEXCOORD_ARRAY)
        if (!f->inBattle3D)
        {
            const uint8_t topA = marked ? 46u : 10u;
            const uint8_t botA = marked ? 16u : 0u;
            uint32_t vTone = 0;
            PushFlatVertex2D(&vTone, 0.0f, 0.0f, 255, 212, 148, topA);
            PushFlatVertex2D(&vTone, 1.0f, 0.0f, 255, 212, 148, topA);
            PushFlatVertex2D(&vTone, 1.0f, 1.0f, 28,  34,  62,  botA);
            PushFlatVertex2D(&vTone, 0.0f, 0.0f, 255, 212, 148, topA);
            PushFlatVertex2D(&vTone, 1.0f, 1.0f, 28,  34,  62,  botA);
            PushFlatVertex2D(&vTone, 0.0f, 1.0f, 28,  34,  62,  botA);

            glDisable(GL_TEXTURE_2D);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vTone);
            glDisableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnable(GL_TEXTURE_2D);
        }

        // 2. Combined cool shadow split-tone + radial vignette in ONE GL_DST_COLOR pass
        if (sVignetteTex != 0)
        {
            static const float vUvs[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
            const float coolR = marked ? 0.85f : 0.97f;
            const float coolG = marked ? 0.91f : 0.98f;
            const float coolB = 1.00f;
            glBindTexture(GL_TEXTURE_2D, sVignetteTex);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glColor4f(coolR, coolG, coolB, 1.0f);
            glVertexPointer(2, GL_FLOAT, 0, quad);
            glTexCoordPointer(2, GL_FLOAT, 0, vUvs);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    VitaPerf_SubEnd(VITA_PERF_T_POST_COMPOSITE);
}

// Pass 6: the GBA's 2D layer over the 3D world (text, menus, dialogue on BG0, the 3D battle
// foreground, HM and Fly animations). Kept as it is: the new menu system will replace it.
static void Pass_Overlay2D(const VoxelFrame *const f)
{
    const bool inBattle3D = f->inBattle3D;
    const float fadeAmount = f->fadeAmount;
    const GameFrameOverlay *const overlay = &GameFrame_Get()->overlay;
    const bool inFieldMove = overlay->fieldMoveShowMon;
    const bool inFlyAnim = overlay->flyAnim;
    // Battle transition on the field: drawn 16:9, stretched as the field-move banners
    const bool inTransition = !inBattle3D && overlay->battleTransition;

    if (inBattle3D || (REG_DISPCNT & 0x0100) || inFieldMove || inFlyAnim || inTransition)
    {
        // In 3D battles, draw crisp foreground HD-2D weather/ambient particles over the blurred backdrop (1 draw call)
        if (inBattle3D && fadeAmount < 0.99f && f->hd2dPreset != HD2D_PRESET_OFF && sWeatherAtlasTex != 0)
        {
            const VitaDayNightLight *const dn = (!f->indoor) ? VitaDayNight_Get() : NULL;
            const bool night = (dn != NULL && dn->glow > 0.25f);
            const bool isSand = (f->weather == VOXEL_WEATHER_SAND);
            const bool isRain = (f->weather == VOXEL_WEATHER_RAIN);
            const bool isAshOrSnow = (f->weather == VOXEL_WEATHER_ASH || f->weather == VOXEL_WEATHER_PARTICLES);
            const uint8_t mr = isSand ? 235u : isRain ? 185u : isAshOrSnow ? 230u : (f->underground ? 255u : (night ? 185u : 255u));
            const uint8_t mg = isSand ? 195u : isRain ? 215u : isAshOrSnow ? 235u : (f->underground ? 180u : (night ? 235u : 238u));
            const uint8_t mb = isSand ? 135u : isRain ? 255u : isAshOrSnow ? 245u : (f->underground ? 120u : (night ? 255u : 175u));
            const float secs = f->seconds;
            const uint32_t pCount = (isSand || isRain) ? 64u : 28u;
            uint32_t vBM = 0;
            static const float mx[6] = {-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f};
            static const float my[6] = {-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f};
            for (uint32_t i = 0; i < pCount; ++i)
            {
                const float speedX = isSand ? 0.42f : isRain ? 0.14f : 0.022f;
                const float speedY = isSand ? 0.06f : isRain ? 0.85f : -0.018f;
                const float px = fmodf(0.037f * (float)i + secs * (speedX + 0.03f * (float)(i & 3u)), 1.0f);
                const float py = fmodf(0.11f * (float)i + secs * (speedY + 0.04f * (float)(i & 3u)) + 10.0f, 0.90f);
                const float pulse = 0.5f + 0.5f * sinf(secs * (1.8f + 0.4f * (float)(i & 3u)) + (float)i * 1.3f);
                const uint8_t ma = (uint8_t)((isRain || isSand ? 0.45f : (0.24f + 0.50f * pulse * pulse)) * 255.0f);
                const float rx = isRain ? 0.004f : isSand ? 0.018f : (0.011f + 0.006f * pulse);
                const float ry = isRain ? 0.045f : isSand ? 0.008f : (rx * (960.0f / 544.0f));
                const float u0 = 0.0f, u1 = 0.5f;
                const float v0 = isRain ? 0.0f : 0.5f, v1 = isRain ? 0.5f : 1.0f;
                for (int32_t k = 0; k < 6; ++k)
                {
                    VoxelGlVertex *const v = &sBatchGlVerts[vBM++];
                    v->x = px + mx[k] * rx;
                    v->y = py + my[k] * ry;
                    v->z = 0.0f;
                    v->u = (mx[k] > 0.0f) ? u1 : u0;
                    v->v = (my[k] > 0.0f) ? v1 : v0;
                    v->r = mr; v->g = mg; v->b = mb; v->a = ma;
                }
            }
            glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDisable(GL_ALPHA_TEST);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, sWeatherAtlasTex);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, (isRain || isSand) ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vBM);
            glDisableClientState(GL_COLOR_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisable(GL_BLEND);
        }

        // A3: Impact flash (white critical flash / move-type colored flash) in 3D battles
        if (inBattle3D && fadeAmount < 0.99f && f->hd2dPreset != HD2D_PRESET_OFF && sBattleFx.flashTimer > 0.0f && sBattleFx.flashDur > 0.0f)
        {
            const float ratio = sBattleFx.flashTimer / sBattleFx.flashDur;
            const float alpha = sBattleFx.flashAlpha * ratio * ratio;
            const uint8_t fr = (uint8_t)(sBattleFx.flashRgb[0] * 255.0f);
            const uint8_t fg = (uint8_t)(sBattleFx.flashRgb[1] * 255.0f);
            const uint8_t fb = (uint8_t)(sBattleFx.flashRgb[2] * 255.0f);
            const uint8_t fa = (uint8_t)(alpha * 255.0f);
            if (fa > 1u)
            {
                static const float fx[6] = { 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f };
                static const float fy[6] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f };
                for (int32_t k = 0; k < 6; ++k)
                {
                    VoxelGlVertex *const v = &sBatchGlVerts[k];
                    v->x = fx[k]; v->y = fy[k]; v->z = 0.0f;
                    v->u = 0.0f; v->v = 0.0f;
                    v->r = fr; v->g = fg; v->b = fb; v->a = fa;
                }
                glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glDisable(GL_ALPHA_TEST);
                glDisable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);
                glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisableClientState(GL_COLOR_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisable(GL_BLEND);
                glEnable(GL_TEXTURE_2D);
            }
        }

        // In 3D battles, draw smooth radial ovals beneath battling Pokémon using sLampPoolTex
        if (inBattle3D && fadeAmount < 0.99f && sLampPoolTex != 0)
        {
            VoxelBattleShadow bShadows[4];
            uint32_t bShadowCount = VoxelBattle_Shadows(bShadows, 4);
            if (bShadowCount > 0)
            {
                uint32_t vShadow = 0;
                for (uint32_t s = 0; s < bShadowCount; s++)
                {
                    const float x0 = bShadows[s].x - bShadows[s].rx;
                    const float x1 = bShadows[s].x + bShadows[s].rx;
                    const float y0 = bShadows[s].y - bShadows[s].ry;
                    const float y1 = bShadows[s].y + bShadows[s].ry;
                    static const float qx[6] = {0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f};
                    static const float qy[6] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f};
                    for (int32_t k = 0; k < 6; ++k)
                    {
                        VoxelGlVertex *v = &sBatchGlVerts[vShadow++];
                        v->x = x0 + (x1 - x0) * qx[k];
                        v->y = y0 + (y1 - y0) * qy[k];
                        v->z = 0.0f;
                        v->u = qx[k];
                        v->v = qy[k];
                        v->r = 107;
                        v->g = 107;
                        v->b = 107;
                        v->a = 255;
                    }
                }

                glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0.0, (double)DISPLAY_WIDTH, (double)DISPLAY_HEIGHT, 0.0, -1.0, 1.0);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, sLampPoolTex);
                glEnable(GL_BLEND);
                // dst * (1 - texLum * 0.42): smooth 32-level soft shadow from 16x16 RGBA5551 luminance!
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                glDisable(GL_ALPHA_TEST);

                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);
                glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
                glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].u);
                glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vShadow);
                glDisableClientState(GL_COLOR_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        }

        extern void DrawFrame(uint16_t *pixels);
        extern int32_t gFastDrawBackdropOverride;
        const bool needUi = VoxelRenderer_HasActiveUi(inBattle3D);
        if (needUi)
        {
            const bool inBuyMenu = overlay->buyMenu;
            // The 3D battle uses the margins (stretched text box, weather); every other overlay
            // is shown centred, so only the GBA's 240 px are drawn, converted and uploaded.
            const bool overlayWide = inBattle3D;
            const int32_t frameW = FRAME_LINE_WIDTH;
            const int32_t frameH = DISPLAY_HEIGHT;
            const int32_t uploadX0 = overlayWide ? 0 : FRAME_LINE_X0;
            const int32_t uploadW = overlayWide ? FRAME_LINE_WIDTH : DISPLAY_WIDTH;

            // Layers hidden from the overlay. They are applied to each line's captured registers,
            // never written to REG_DISPCNT / REG_BLDCNT, which the game thread on Core 1 owns.
            if (inBattle3D)
            {
                gFastDrawDispcntClear = DISPCNT_BG3_ON | Ui_BattleDispcntClear(); // the 2D battle terrain (+ the GBA HUD under the new one)
            }
            else if (inBuyMenu)
            {
                gFastDrawDispcntClear = DISPCNT_BG2_ON | DISPCNT_BG3_ON; // the 2D map viewport
            }
            else
            {
                gFastDrawDispcntClear = DISPCNT_BG1_ON | DISPCNT_BG2_ON | DISPCNT_BG3_ON; // the field map
                gFastDrawScreenObjOnly = true; // screen objects only (no map NPCs)
            }
            // Outside battles, no blending against the backdrop, which is the chroma key here.
            gFastDrawBldcntClear = inBattle3D ? 0 : (BLDCNT_TGT1_BD | BLDCNT_TGT2_BD);
            gFastDrawCentredOnly = !overlayWide;

            static uint16_t gbaImage[FRAME_LINE_WIDTH * DISPLAY_HEIGHT] __attribute__((aligned(4)));
            static uint16_t uiImage[FRAME_LINE_WIDTH * DISPLAY_HEIGHT] __attribute__((aligned(4)));
            const uint16_t chromaKey = 0x7C1Fu;
            const int32_t totalPixels = frameW * frameH;
            bool hasUi = false;

            // Direct backdrop override protects Pass 6 from PLTT race conditions with Core 1
            gFastDrawBackdropOverride = chromaKey;
            const uint32_t backdropPair = ((uint32_t)chromaKey << 16) | chromaKey;
            uint32_t *const gbaWords = (uint32_t *)gbaImage;
            const int32_t wordTotal = totalPixels / 2;
            for (int32_t k = 0; k < wordTotal; k++)
                gbaWords[k] = backdropPair;

            VitaPerf_MarkUiScanlinesStart();
            DrawFrame(gbaImage);
            VitaPerf_MarkUiScanlinesEnd();
            gFastDrawScreenObjOnly = false;
            gFastDrawDispcntClear = 0;
            gFastDrawBldcntClear = 0;
            gFastDrawCentredOnly = FALSE;
            gFastDrawBackdropOverride = -1;

            VitaPerf_MarkUiUploadStart();
            const uint16_t backdrop = chromaKey;

            // Convert GBA BGR555 to RGBA4444 (16-bit with 4-bit hardware alpha), packed to the
            // uploaded width
            const uint8_t rawEva = (REG_BLDALPHA & 0x1F);
            const uint8_t evaAlpha = (rawEva > 15) ? 15 : ((rawEva == 0) ? 7 : rawEva);

            // An overlay identical to the last one uploaded (a dialogue waiting for a button, the
            // HUD) skips the conversion and the upload: the texture already holds it. Uploading
            // into a texture drawn last frame makes vitaGL copy all of it first.
            uint32_t uiHash = 2166136261u;
            uiHash = (uiHash ^ (uint32_t)evaAlpha) * 16777619u;
            uiHash = (uiHash ^ ((uint32_t)uploadX0 | ((uint32_t)uploadW << 12) | ((uint32_t)frameH << 24))) * 16777619u;
            for (int32_t k = 0; k < wordTotal; k++)
                uiHash = (uiHash ^ gbaWords[k]) * 16777619u;
            const bool uiUnchanged = sUiImageValid && uiHash == sUiImageHash;

            // Two pixels at a time: most of the overlay is the chroma key (transparent). uploadX0
            // and uploadW are even, so the rows are word aligned.
            const uint32_t keyPair = (((uint32_t)backdrop << 16) | backdrop) & 0x7FFF7FFFu;
            for (int32_t y = 0; y < frameH && !uiUnchanged; y++)
            {
                const uint32_t *const src = (const uint32_t *)&gbaImage[y * frameW + uploadX0];
                uint32_t *const dst = (uint32_t *)&uiImage[y * uploadW];

                for (int32_t x = 0; x < uploadW / 2; x++)
                {
                    const uint32_t pair = src[x];
                    if ((pair & 0x7FFF7FFFu) == keyPair)
                    {
                        dst[x] = 0;
                        continue;
                    }
                    uint32_t out = 0;
                    for (int32_t h = 0; h < 2; h++)
                    {
                        const uint16_t color = (uint16_t)(pair >> (h * 16));
                        if ((color & 0x7FFFu) == backdrop)
                            continue;
                        hasUi = true;
                        const uint16_t r = (color & 0x1F) >> 1;
                        const uint16_t g = ((color >> 5) & 0x1F) >> 1;
                        const uint16_t b = ((color >> 10) & 0x1F) >> 1;
                        // Bit 15 == 0 indicates a semi-transparent pixel blended over backdrop (sandstorm, weather)
                        const uint16_t a = (color & 0x8000u) ? 0x0Fu : (uint16_t)evaAlpha;
                        out |= (uint32_t)((r << 12) | (g << 8) | (b << 4) | a) << (h * 16);
                    }
                    dst[x] = out;
                }
            }

            if (uiUnchanged)
            {
                hasUi = sUiImageHasUi;
            }
            else
            {
                // With UI the image is uploaded below; without it nothing is drawn. Either way the
                // same hash next frame can reuse this result.
                sUiImageHash = uiHash;
                sUiImageHasUi = hasUi;
                sUiImageValid = true;
            }

            if (hasUi && sUiTex != 0)
            {
                if (!uiUnchanged)
                {
                    sUiTexIdx = (uint8_t)((sUiTexIdx + 1u) & (VOXEL_UI_TEX_RING - 1u));
                    sUiTex = sUiTexRing[sUiTexIdx];
                    glBindTexture(GL_TEXTURE_2D, sUiTex);
                    // A centred overlay is sampled only inside the GBA's 240 px, so the stale
                    // margins of the texture are never shown.
                    glTexSubImage2D(GL_TEXTURE_2D, 0, uploadX0, 0, uploadW, frameH,
                                    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, uiImage);
                }
                VitaPerf_MarkUiUploadEnd();
                glBindTexture(GL_TEXTURE_2D, sUiTex);

                glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();

                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_ALPHA_TEST);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

                float u0, u1, vx0, vx1;
                if (inBattle3D)
                {
                    // In 3D battle: 16:9 fullscreen with the same uniform x3.4 mapping as every
                    // other widescreen frame (the visible part of the 288 px line)
                    vx0 = 0.0f;
                    vx1 = 1.0f;
                    u0 = WIDESCREEN_VISIBLE_U0;
                    u1 = WIDESCREEN_VISIBLE_U1;
                }
                else if (inFieldMove || inTransition)
                {
                    // Field move banner (MO / HM): stretches GBA active 240 px frame to full 16:9 fullscreen (960x544)
                    vx0 = 0.0f;
                    vx1 = 1.0f;
                    u0 = (float)FRAME_LINE_X0 / (float)FRAME_LINE_WIDTH;
                    u1 = (float)(FRAME_LINE_X0 + DISPLAY_WIDTH) / (float)FRAME_LINE_WIDTH;
                }
                else
                {
                    // In overworld: conversations (dialogue box), fly bird, and map name popups are 240 px centered
                    const float quadW = (544.0f * (float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT) / 960.0f;
                    vx0 = (1.0f - quadW) / 2.0f;
                    vx1 = vx0 + quadW;
                    u0 = (float)FRAME_LINE_X0 / (float)FRAME_LINE_WIDTH;
                    u1 = (float)(FRAME_LINE_X0 + DISPLAY_WIDTH) / (float)FRAME_LINE_WIDTH;
                }

                const float uiVerts[8] = {
                    vx0, 0.0f,
                    vx1, 0.0f,
                    vx1, 1.0f,
                    vx0, 1.0f
                };
                const float uiUvs[8] = {
                    u0, 0.0f,
                    u1, 0.0f,
                    u1, 1.0f,
                    u0, 1.0f
                };
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glVertexPointer(2, GL_FLOAT, 0, uiVerts);
                glTexCoordPointer(2, GL_FLOAT, 0, uiUvs);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);

                glDisable(GL_ALPHA_TEST);
                glDisable(GL_BLEND);
                glDepthMask(GL_FALSE);
            }
            else
            {
                VitaPerf_MarkUiUploadEnd();
            }
        }
    }
}

// Measuring builds: a thin bar across the top while the automatic bench runs, so the tester
// knows to stand still and when it is over. Nothing in other builds.
static void DrawBenchProgress(void)
{
    const float progress = VitaPerf_BenchProgress();
    if (progress < 0.0f)
        return;
    static const float kBarHeight = 6.0f / 544.0f;
    const float fill = Voxel_ClampF(progress, 0.0f, 1.0f);
    const float back[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, kBarHeight, 0.0f, kBarHeight };
    const float bar[8] = { 0.0f, 0.0f, fill, 0.0f, fill, kBarHeight, 0.0f, kBarHeight };

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glVertexPointer(2, GL_FLOAT, 0, back);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glColor4f(1.0f, 0.75f, 0.1f, 1.0f);
    glVertexPointer(2, GL_FLOAT, 0, bar);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}

static void DrawMenuFrozenBackdrop(void)
{
    const int32_t renderW = VitaPlatform_GetRenderWidth();
    const int32_t renderH = VitaPlatform_GetRenderHeight();
    static const float pos[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    static const float uvs[8] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    VitaPerf_GpuScene(VITA_PERF_GPU_FINAL);
    glViewport(0, 0, renderW, renderH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, sMenuBgTex);
    glVertexPointer(2, GL_FLOAT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// Fullscreen churning cloud cover for the 3D Fly cinematic (vitaGL ortho 0..1, 16:9).
// Cool sky/cloud-shadow backdrop shared by DrawCloudCover and RenderFlyCloudFrame so the
// 3 layered white scrolling cloud sheets always show clear depth and motion instead of flat white.
static const float kFlyCloudBackdropRgb[3] = {
    156.0f / 255.0f, 186.0f / 255.0f, 218.0f / 255.0f
};

static void DrawCloudCover(const float amount)
{
    const float k = Voxel_ClampF(amount, 0.0f, 1.0f);
    if (k <= 0.005f)
        return;

    static uint64_t sCloudStart = 0;
    if (sCloudStart == 0)
        sCloudStart = SDL_GetPerformanceCounter();
    const double seconds = (double)(SDL_GetPerformanceCounter() - sCloudStart)
                         / (double)SDL_GetPerformanceFrequency();

    glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static const float sQuad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, sQuad);

    glDisable(GL_TEXTURE_2D);
    glColor4f(kFlyCloudBackdropRgb[0], kFlyCloudBackdropRgb[1], kFlyCloudBackdropRgb[2], k * k);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    if (sFogTex != 0)
    {
        static const struct {
            float repeats, speedU, speedV;
            float r, g, b, alpha;
        } kSheets[3] = {
            { 1.2f,  0.022f,  0.010f, 0.78f, 0.85f, 0.93f, 0.90f }, /* deep cloud shadows */
            { 2.0f, -0.042f,  0.020f, 0.96f, 0.97f, 0.99f, 0.92f }, /* main bright billows */
            { 3.4f,  0.068f, -0.030f, 1.00f, 1.00f, 1.00f, 0.72f }, /* fast wispy foreground */
        };
        const float aspect = 544.0f / 960.0f;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sFogTex);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        for (uint32_t s = 0u; s < 3u; ++s)
        {
            float u = (float)fmod(seconds * kSheets[s].speedU, 1.0);
            float v = (float)fmod(seconds * kSheets[s].speedV, 1.0);
            if (u < 0.0f) u += 1.0f;
            if (v < 0.0f) v += 1.0f;
            const float ru = kSheets[s].repeats;
            const float rv = kSheets[s].repeats * aspect;
            const float uvs[8] = { u, v, u + ru, v, u + ru, v + rv, u, v + rv };

            glColor4f(kSheets[s].r, kSheets[s].g, kSheets[s].b, k * kSheets[s].alpha);
            glTexCoordPointer(2, GL_FLOAT, 0, uvs);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void VoxelRenderer_DrawFlyCloudCover(void)
{
    const VoxelFlySnapshot *const snap = VoxelFly_GetSnapshot();
    if (snap != NULL && snap->active && snap->frame.whiteout > 0.005f)
        DrawCloudCover(snap->frame.whiteout);
}

// Under 100% opaque Fly cloud cover: skip 3D rendering so Core 0 animates layered clouds
// at locked 30/60 FPS (~1 ms) while Core 2 streams all aerial destination chunks in 4-chunk batches.
static void RenderFlyCloudFrame(const VoxelFrame *const f)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    VitaPerf_GpuScene(VITA_PERF_GPU_FINAL);
    glClearColor(kFlyCloudBackdropRgb[0], kFlyCloudBackdropRgb[1], kFlyCloudBackdropRgb[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    DrawCloudCover(f->fly.frame.whiteout);
    Ui_Render(VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
    if (f->holesLeft == 0)
        VoxelFly_NotifyFrameDrawn(f->fly.warpVersion);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    REG_VCOUNT = 161;
}

// Render one frame of the 3D voxel world
bool VoxelRenderer_RenderFrame(void)
{
    VoxelFrame f;
    if (!Frame_Begin(&f))
        return false;

    float curFade = 0.0f;
    float curFadeRgb[3] = { 0.0f, 0.0f, 0.0f };
    VoxelWorld_ScreenFade(&curFade, curFadeRgb);
    const Hd2dPreset curPreset = GetHd2dPreset();
    const bool curBlur = (VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLUR) != 0);
    const int32_t curBloom = VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLOOM);
    const int32_t curSkyBloom = VitaExtras_GetFast(FAST_EXTRA_VOXEL_SKY_BLOOM);
    const int32_t curShafts = VitaExtras_GetFast(FAST_EXTRA_VOXEL_SHAFTS);
    const bool menuMapChanged = (f.inst0->mapGroup != sMeshMapGroup || f.inst0->mapNum != sMeshMapNum);
    const bool menuFreeze = (sMenuBgFbo != 0 && sMenuBgTex != 0
                             && Ui_IsOpen() && !f.inBattle3D && !f.fly.active && curFade < 0.99f);
    if (!menuFreeze)
    {
        sMenuBgValid = false;
    }
    else if (sMenuBgValid && !menuMapChanged && sMenuBgPreset == (int32_t)curPreset && sMenuBgBlur == curBlur
             && sMenuBgBloom == curBloom && sMenuBgSkyBloom == curSkyBloom && sMenuBgShafts == curShafts)
    {
        f.inBattle3D = false;
        f.fadeAmount = curFade;
        f.drawFade = curFade;
        f.hd2dPreset = curPreset;
        DrawMenuFrozenBackdrop();
        Pass_Overlay2D(&f);
        Ui_Render(VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
            REG_VCOUNT = 161;
        return true;
    }

    Frame_FollowCamera(&f);
    Frame_UpdateAtlas(&f);
    const bool freezeMenuHd2d = (!f.inBattle3D && !f.fly.active && Ui_IsOpen());
    if (!freezeMenuHd2d)
        Frame_AnimateTiles(&f);
    Frame_TrackWorldChanges(&f);
    Frame_ReadAtmosphere(&f);
    {
        static bool sPauseFrozenActive = false;
        static double sPauseFrozenClock = 0.0;
        if (freezeMenuHd2d)
        {
            if (!sPauseFrozenActive)
            {
                sPauseFrozenClock = f.clock;
                sPauseFrozenActive = true;
            }
            else
            {
                f.clock = sPauseFrozenClock;
            }
        }
        else
        {
            sPauseFrozenActive = false;
        }
    }
    Frame_DeriveClock(&f);
    Frame_SelectChunks(&f);
    if (f.fly.active && f.fly.frame.whiteout >= 0.99f)
    {
        RenderFlyCloudFrame(&f);
        return true;
    }

    BuildFrameGrade(&f.grade, &f.light, f.indoor);
    Frame_EmitEntities(&f);
    const GLuint finalTargetFbo = menuFreeze ? sMenuBgFbo : 0;
    Frame_BeginPasses(&f, finalTargetFbo);

    // Faded out completely (a warp): no 3D geometry. The visible chunks were built above all
    // the same, so the fade-in has nothing left to build.
    if (f.drawFade < 0.99f)
    {
        Pass_SetupWorld3D();
        // Measuring build: the world scene through an 8x8 scissor leaves its vertex and tiling
        // work without its pixels (the clear before it is not cut).
        const bool worldPixelsCut = VitaPerf_PassSkipped(VITA_PERF_PASS_WORLDPIXELS);
        if (worldPixelsCut)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, 8, 8);
        }

        // Chunk VBOs (passes 1-3) are drawn with their crossing offsets.
        ChunkOffsets_Begin();
        VitaPerf_MarkDraw3DStart();
        Pass_Ground(&f);
        Pass_Trees(&f);
        const bool buildingAlphaEnabled = Pass_Buildings(&f);
        ChunkOffsets_End();

        // Unbind VBO so following passes can use standard client-side arrays
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (buildingAlphaEnabled)
            glDisable(GL_ALPHA_TEST);

        Pass_CastShadows();
        Pass_Reflections();
        Pass_Entities();
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_ALPHA_TEST);

        Pass_DayNightGrade(&f);
        Pass_EntityLamps();
        Pass_BuildingGlow(&f);
        Pass_NightLamps(&f);
        Pass_Atmosphere3D(&f);
        Pass_Atmosphere2D(&f);
        if (worldPixelsCut)
            glDisable(GL_SCISSOR_TEST);
        VitaPerf_MarkDraw3DEnd(sFrameDrawCalls, sFrameVertCount);
        VoxelFly_NotifyFrameDrawn(f.fly.warpVersion);
    }

    if (f.dioramaActive)
        Pass_DepthOfField(&f, finalTargetFbo);

    if (menuFreeze)
    {
        DrawMenuFrozenBackdrop();
        sMenuBgValid = true;
        sMenuBgPreset = (int32_t)curPreset;
        sMenuBgBlur = curBlur;
        sMenuBgBloom = curBloom;
        sMenuBgSkyBloom = curSkyBloom;
        sMenuBgShafts = curShafts;
    }

    Pass_Overlay2D(&f);

    if (f.fly.active && f.fly.frame.whiteout > 0.005f)
        DrawCloudCover(f.fly.frame.whiteout);

    // New UI over everything (Core 1 list, one draw call)
    Ui_Render(VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
    DrawBenchProgress();

    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    REG_VCOUNT = 161;
    return true;
}

void VoxelRenderer_SwapBuffers(void)
{
    // Measuring builds: the frame's last scene ends here, before the swap's own wait.
    VitaPerf_GpuScene(VITA_PERF_GPU_NONE);
    VitaPerf_MarkSwapStart();
#ifdef PLATFORM_VITA
    vglSwapBuffers(GL_FALSE);
#else
    SDL_GL_SwapWindow(sdlWindow);
#endif
    VitaPerf_MarkSwapEnd();
    // Core 0 is done with the voxel world until the next frame: the chunk chosen this frame
    // is computed on Core 2 meanwhile. Not while Core 1 is rebuilding the map layout.
    if (GameFrame_MapSettled() && !sMenuBgValid)
        ChunkWorker_Launch();
}

void VoxelRenderer_OnReset(void)
{
    ChunkWorker_Join();
    sWorkerBusyThisFrame = false;
    AtlasExtend_Cancel();
    sMeshMapGroup = -1;
    sMeshMapNum = -1;
    sPreviousOriginCount = 0;
    sLastFlyWarpVersion = 1u;
    sNeedCameraSnap = true;
    sBattleState.begun = false;
    sBattleState.chosen = false;
    sBattleFx.active = false;
    sBattleFx.flashTimer = 0.0f;
    sBattleFx.shakeTimer = 0.0f;
    sMenuBgValid = false;
    InvalidateChunkCache();
    VoxelEntities_Reset();
}

#endif // VOXEL_SUPPORTED || PLATFORM_VITA || NATIVE_LINUX
#endif // PLATFORM_SDL2
