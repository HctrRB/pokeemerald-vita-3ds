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
#include "window.h"
#include "gba/io_reg.h"
#include "sprite.h"
#include "palette.h"
#include "field_weather.h"
#include "fieldmap.h"
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
#include "voxel_battle.h"
#include "voxel_entities.h"
#include "voxel_lighting.h"
#include "voxel_grade.h"
#include "voxel_file.h"
#include "voxel_unswizzle.h"
#include "voxel_mesh_builder.h"
#include "voxel_tuning.h"

// External state
bool gVoxelModeEnabled = true;
bool gVoxelFirstPersonMode = false;
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
static GLuint sTreeTex = 0;
#define VOXEL_MAX_BUILDING_PAGES 272
#define VOXEL_BUILDING_CACHE_SLOTS 24
#define VOXEL_BUILDING_REQ_CAP 32
static GLuint sSpriteTex = 0;
static GLuint sUiTex = 0;
// Two overlay textures used in turn: uploading into the one drawn last frame made vitaGL copy it
// first (every frame in battles, where the sprites always move)
static GLuint sUiTexRing[2] = { 0, 0 };
static uint8_t sUiTexIdx = 0;
// The last overlay image converted for sUiTex (pass 6); cleared whenever sUiTex is recreated.
static bool sUiImageValid = false;
static bool sUiImageHasUi = false;
static uint32_t sUiImageHash = 0;
static GLuint sLampPoolTex = 0;
static GLuint sFogTex = 0;

// ---- Downscaled Scratch Diorama Blur (DoF) FBOs ----
#define VOXEL_DIORAMA_FBO_W 960
#define VOXEL_DIORAMA_FBO_H 544
#define VOXEL_SCRATCH_FBO_W 480
#define VOXEL_SCRATCH_FBO_H 272

static GLuint sDioramaFbo = 0;
static GLuint sDioramaColorTex = 0;
static GLuint sDioramaDepthRb = 0;
static GLuint sScratchFbo = 0;
static GLuint sScratchColorTex = 0;
// The wider blur level of the depth of field (Pass_DepthOfField), half resolution like the scratch
static GLuint sBlurFbo = 0;
static GLuint sBlurTex = 0;
static bool sDioramaFboReady = false;

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
#define VOXEL_ATLAS_SLOTS 5

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
#define CHUNK_TREE_CAP 768
#define CHUNK_BUILDING_CAP 12288
#define MAX_CHUNK_GROUND_SPANS 24
#define MAX_CHUNK_BUILDING_SPANS 4

#define MAX_CHUNKS 64
#define CHUNK_VERTS_PER_SLOT (CHUNK_GROUND_TOTAL_CAP + CHUNK_TREE_CAP + CHUNK_BUILDING_CAP)

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
    uint16_t buildingFirstVert;
    uint16_t totalVertCount;
    GLuint vbo;
    VoxelChunkGroundSpan groundSpans[MAX_CHUNK_GROUND_SPANS];
    VoxelChunkBuildingSpan buildingSpans[MAX_CHUNK_BUILDING_SPANS];

    // Where the chunk is drawn relative to where it was built: map crossings since its build
    // (ShiftChunkCoordinates), applied as a translation (ChunkOffsets_Apply).
    float drawOffsetX, drawOffsetZ;

    // While the chunk is being built: its parts in sChunkStaging. NULL otherwise; once uploaded
    // the geometry only lives in the VBO.
    VoxelGlVertex *groundVerts;
    VoxelGlVertex *treeVerts;
    VoxelGlVertex *buildingVerts;
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

static VoxelVertex sShadowVerts[MAX_SHADOW_VERTS];
static VoxelGlVertex sShadowGlVerts[MAX_SHADOW_VERTS];
static uint32_t sShadowGlCount = 0;

static VoxelVertex sReflectionVerts[MAX_REFLECTION_VERTS];
static VoxelGlVertex sReflectionGlVerts[MAX_REFLECTION_VERTS];
static uint32_t sReflectionGlCount = 0;

// Night street lamps (lantern glass & street light pools, ported from ctr_voxel.c)
#define VOXEL_LAMP_GLASS_CAP 1024
#define VOXEL_LAMP_GROUND_CAP 1024
static VoxelVertex sLampGlassScratch[VOXEL_LAMP_GLASS_CAP];
static VoxelVertex sLampGroundScratch[VOXEL_LAMP_GROUND_CAP];
static VoxelGlVertex sLampGlassVerts[VOXEL_LAMP_GLASS_CAP];
static VoxelGlVertex sLampGroundVerts[VOXEL_LAMP_GROUND_CAP];
static uint32_t sLampGlassCount = 0;
static uint32_t sLampGroundCount = 0;
static bool sLampsBuilt = false;

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
    { 0.35f,  0.45f, 0.18f, 0.75f, 0.00f, 0.00f },
    { 0.85f, -0.30f, 0.28f, 0.64f, 0.37f, 0.61f },
};

static const FogBank sSandBanks[2] = {
    { 0.30f, -4.5f, 1.4f, 0.83f, 0.00f, 0.00f },
    { 0.95f, -3.2f, 0.9f, 0.66f, 0.43f, 0.29f },
};

static VoxelGlVertex sFogSheets[2][VOXEL_FOG_BANK_VERTICES];


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
    false, 96, 220,
    -13.0f, -1.2f, 4.0f, 0.042f, 0.80f, 0xFF7EB8E6u, 0.79f, 2.0f, 2.5f,
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

// ---- Frustum Culling Helpers ----
typedef struct {
    float a, b, c, d;
} FrustumPlane;

static FrustumPlane sFrustumPlanes[6];

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
        float px = (sFrustumPlanes[i].a > 0.0f) ? maxX : minX;
        float py = (sFrustumPlanes[i].b > 0.0f) ? maxY : minY;
        float pz = (sFrustumPlanes[i].c > 0.0f) ? maxZ : minZ;

        if (sFrustumPlanes[i].a * px + sFrustumPlanes[i].b * py + sFrustumPlanes[i].c * pz + sFrustumPlanes[i].d < 0.0f)
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
    (void)slot;
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

    // Outdoors: robust vertical column [-4.0f, 14.0f] guaranteeing elevated terrain,
    // mountains, buildings, and tree canopies are never falsely culled.
    // Relaxed margins (-2.5f on left/west and +3.0f on south/bottom).
    float minX = (float)(sChunkOriginX + cx * CHUNK_SIZE) - 2.5f;
    float maxX = (float)(sChunkOriginX + (cx + 1) * CHUNK_SIZE) + 2.0f;
    float minZ = (float)(sChunkOriginY + cy * CHUNK_SIZE) - 2.0f;
    float maxZ = (float)(sChunkOriginY + (cy + 1) * CHUNK_SIZE) + 3.0f;

    return Frustum_IntersectsAABB(minX, -4.0f, minZ, maxX, 14.0f, maxZ);
}

// Whether the chunk would be in view once the camera has moved by (aheadX, aheadZ) tiles:
// the same test against the box moved the other way. Outdoors only (indoors everything in
// the layout is already drawn).
static bool Frustum_ChunkInViewAhead(int32_t cx, int32_t cy, float aheadX, float aheadZ)
{
    float minX = (float)(sChunkOriginX + cx * CHUNK_SIZE) - 2.5f - aheadX;
    float maxX = (float)(sChunkOriginX + (cx + 1) * CHUNK_SIZE) + 2.0f - aheadX;
    float minZ = (float)(sChunkOriginY + cy * CHUNK_SIZE) - 2.0f - aheadZ;
    float maxZ = (float)(sChunkOriginY + (cy + 1) * CHUNK_SIZE) + 3.0f - aheadZ;

    return Frustum_IntersectsAABB(minX, -4.0f, minZ, maxX, 14.0f, maxZ);
}

// Update frustum planes based on camera position and FOV
static void UpdateFrustumPlanes(void)
{
    float f = 1.0f / tanf(sCamera.fov * (3.14159265f / 360.0f));
    float aspect = 960.0f / 544.0f;
    float zNear = 1.0f, zFar = 62.0f;
    float proj[16] = {0};
    proj[0] = f / aspect;
    proj[5] = f;
    proj[10] = (zFar + zNear) / (zNear - zFar);
    proj[11] = -1.0f;
    proj[14] = (2.0f * zFar * zNear) / (zNear - zFar);

    float view[16];
    float forward[3], side[3], up[3];
    forward[0] = sCamera.targetX - sCamera.x;
    forward[1] = sCamera.targetY - sCamera.y;
    forward[2] = sCamera.targetZ - sCamera.z;
    float len = sqrtf(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
    if (len > 0.0f) { forward[0] /= len; forward[1] /= len; forward[2] /= len; }

    side[0] = -forward[2];
    side[1] = 0.0f;
    side[2] = forward[0];
    len = sqrtf(side[0]*side[0] + side[2]*side[2]);
    if (len > 0.0f) { side[0] /= len; side[2] /= len; }

    up[0] = side[1]*forward[2] - side[2]*forward[1];
    up[1] = side[2]*forward[0] - side[0]*forward[2];
    up[2] = side[0]*forward[1] - side[1]*forward[0];

    view[0] = side[0];     view[4] = side[1];     view[8]  = side[2];     view[12] = -(side[0] * sCamera.x + side[1] * sCamera.y + side[2] * sCamera.z);
    view[1] = up[0];       view[5] = up[1];       view[9]  = up[2];       view[13] = -(up[0] * sCamera.x + up[1] * sCamera.y + up[2] * sCamera.z);
    view[2] = -forward[0]; view[6] = -forward[1]; view[10] = -forward[2]; view[14] = -(-forward[0] * sCamera.x - forward[1] * sCamera.y - forward[2] * sCamera.z);
    view[3] = 0.0f;        view[7] = 0.0f;        view[11] = 0.0f;        view[15] = 1.0f;

    float mvp[16];
    Matrix4_Multiply(mvp, proj, view);
    ExtractFrustumPlanes(mvp);
}

// ---- GL Math Helpers ----
static void Voxel_gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    float f = 1.0f / tanf(fovy * (3.14159265f / 360.0f));
    float m[16] = {0};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    glMultMatrixf(m);
}

static void Voxel_gluLookAt(float eyex, float eyey, float eyez,
                            float centerx, float centery, float centerz,
                            float upx, float upy, float upz)
{
    float forward[3], side[3], up[3];
    float m[16];

    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;
    float len = sqrtf(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
    if (len > 0.0f) { forward[0] /= len; forward[1] /= len; forward[2] /= len; }

    side[0] = forward[1]*upz - forward[2]*upy;
    side[1] = forward[2]*upx - forward[0]*upz;
    side[2] = forward[0]*upy - forward[1]*upx;
    len = sqrtf(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);
    if (len > 0.0f) { side[0] /= len; side[1] /= len; side[2] /= len; }

    up[0] = side[1]*forward[2] - side[2]*forward[1];
    up[1] = side[2]*forward[0] - side[0]*forward[2];
    up[2] = side[0]*forward[1] - side[1]*forward[0];

    m[0] = side[0];    m[4] = side[1];    m[8]  = side[2];    m[12] = 0.0f;
    m[1] = up[0];      m[5] = up[1];      m[9]  = up[2];      m[13] = 0.0f;
    m[2] = -forward[0];m[6] = -forward[1];m[10] = -forward[2];m[14] = 0.0f;
    m[3] = 0.0f;       m[7] = 0.0f;       m[11] = 0.0f;       m[15] = 1.0f;

    glMultMatrixf(m);
    glTranslatef(-eyex, -eyey, -eyez);
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
    uint32_t lastUsedFrame;
} VoxelBuildingCacheSlot;

static VoxelBuildingCacheSlot sBuildingCache[VOXEL_BUILDING_CACHE_SLOTS];
static int8_t sBuildingPageToSlot[VOXEL_MAX_BUILDING_PAGES];
static uint8_t sBuildingPageHasAlpha[VOXEL_MAX_BUILDING_PAGES];
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
static int16_t sWorkerReadyPage = -1;
static uint32_t sWorkerReadyW = 0;
static uint32_t sWorkerReadyTexH = 0;
static bool sWorkerReadyHasAlpha = false;

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
                                      const bool hasAlpha, const uint16_t *const linearPixels)
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

    glGenTextures(1, &slot->tex);
    glBindTexture(GL_TEXTURE_2D, slot->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)bw, (GLsizei)btexH,
                 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, linearPixels);

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
    SDL_UnlockMutex(sBuildingIoMutex);

    if (page < 0 || page >= VOXEL_MAX_BUILDING_PAGES)
        return;

    UploadBuildingPageToLru(page, bw, btexH, hasAlpha, sWorkerLinearBuf);

    SDL_LockMutex(sBuildingIoMutex);
    sBuildingPageQueued[page] = false;
    sWorkerReadyPage = -1;
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
        if (VoxelBuildings_PageSize((uint32_t)page, &bw, &bh, &btexH) && (bw * btexH <= 512u * 512u))
        {
            memset(sWorkerMortonBuf, 0, (size_t)bw * btexH * sizeof(uint16_t));
            readOk = VoxelBuildings_ReadPageWorker((uint32_t)page, 0, bw * bh, sWorkerMortonBuf);
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
        const bool hasAlpha = ScanLinearPageAlpha(sWorkerLinearBuf, bw * bh);

        SDL_LockMutex(sBuildingIoMutex);
        sWorkerReadyPage = (int16_t)page;
        sWorkerReadyW = bw;
        sWorkerReadyTexH = btexH;
        sWorkerReadyHasAlpha = hasAlpha;
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
    if (sBuildingIoMutex != NULL)
        SDL_LockMutex(sBuildingIoMutex);
    if (VoxelBuildings_PageSize((uint32_t)page, &bw, &bh, &btexH) && (bw * btexH <= 512u * 512u))
    {
        memset(sBuildingPageMortonScratch, 0, (size_t)bw * btexH * sizeof(uint16_t));
        readOk = VoxelBuildings_ReadPage((uint32_t)page, 0, bw * bh, sBuildingPageMortonScratch);
    }
    if (sBuildingIoMutex != NULL)
        SDL_UnlockMutex(sBuildingIoMutex);

    if (!readOk)
        return 0;

    Voxel_UnswizzleMortonToLinearEx(sBuildingPageMortonScratch, sBuildingPageLinearScratch, bw, bh, btexH);
    const bool hasAlpha = ScanLinearPageAlpha(sBuildingPageLinearScratch, bw * bh);
    return UploadBuildingPageToLru(page, bw, btexH, hasAlpha, sBuildingPageLinearScratch);
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
    sLampsBuilt = false;
    // A chunk the worker computed for the previous map is dropped at commit.
    sWorldEpoch++;
}

static void UploadChunkVbo(VoxelChunkSlot *slot);

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
    sLampsBuilt = false;

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

    sChunkStaging = (VoxelGlVertex *)malloc((size_t)CHUNK_VERTS_PER_SLOT * sizeof(VoxelGlVertex));
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
        sChunks[i].totalVertCount = 0;
        sChunks[i].drawOffsetX = 0.0f;
        sChunks[i].drawOffsetZ = 0.0f;
        if (!sChunks[i].vbo)
            glGenBuffers(1, &sChunks[i].vbo);
        sChunks[i].groundVerts = NULL;
        sChunks[i].treeVerts = NULL;
        sChunks[i].buildingVerts = NULL;
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

// True for a walking crossing (shift camera, no snap); false for a cut/warp (snap camera)
static bool HandleMapChange(int32_t mapGroup, int32_t mapNum, float playerX, float playerZ)
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

    VoxelCamera_SetGround(&sCamera, VoxelEntities_PlayerGround(), 1);
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

static void AtlasSpares_PageReplaced(int32_t slotIdx, uint32_t page);

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
            AtlasSpares_PageReplaced((int32_t)(slot - sAtlasSlots), page);
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

static inline bool TextureIdleLongEnough(uint32_t idleSince)
{
    return (uint32_t)(sFrameCounter - idleSince) >= VOXEL_TEX_IDLE_FRAMES;
}

// Animated atlas pages: one spare per page, for the active slot only (pages 0-1 hold the
// current map's metatiles, the only ones animated). Rows are 16 px bands of the page.
#define VOXEL_ANIM_PAGES 2u
#define VOXEL_ATLAS_ROW_COUNT (VOXEL_ATLAS_H / VOXEL_ATLAS_SLOT)
#define VOXEL_ATLAS_ALL_ROWS ((uint32_t)((1ull << VOXEL_ATLAS_ROW_COUNT) - 1u))

typedef struct
{
    GLuint spare;       // the copy not drawn
    int32_t ownerSlot;      // atlas slot whose page it mirrors, -1 for none
    uint32_t idleSince; // sFrameCounter when the spare stopped being drawn
    uint32_t spareStale; // rows the spare lacks
    uint32_t drawnStale; // rows the drawn texture lacks (composed, not uploaded yet)
} VoxelAtlasPageSpare;

static VoxelAtlasPageSpare sAtlasSpares[VOXEL_ANIM_PAGES];

static void AtlasSpares_Reset(bool deleteTextures)
{
    for (uint32_t p = 0; p < VOXEL_ANIM_PAGES; p++)
    {
        if (deleteTextures && sAtlasSpares[p].spare)
            glDeleteTextures(1, &sAtlasSpares[p].spare);
        sAtlasSpares[p].spare = 0;
        sAtlasSpares[p].ownerSlot = -1;
        sAtlasSpares[p].idleSince = 0;
        sAtlasSpares[p].spareStale = VOXEL_ATLAS_ALL_ROWS;
        sAtlasSpares[p].drawnStale = 0;
    }
}

// Uploads `rows` of a page from the slot's linear copy into `tex` in one call.
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

// Makes page `page` of `slotIdx` the one its spare mirrors. A page left behind by another
// slot gets its pending rows written straight into its drawn texture (rare: slot switch).
static VoxelAtlasPageSpare *AtlasSpares_Claim(int32_t slotIdx, uint32_t page)
{
    VoxelAtlasPageSpare *const spare = &sAtlasSpares[page];
    if (spare->ownerSlot != slotIdx)
    {
        if (spare->ownerSlot >= 0 && spare->drawnStale != 0)
        {
            const VoxelAtlasSlot *const old = &sAtlasSlots[spare->ownerSlot];
            if (old->tex[page])
                UploadAtlasRows(old->tex[page], old->linear[page], spare->drawnStale);
        }
        spare->ownerSlot = slotIdx;
        spare->spareStale = VOXEL_ATLAS_ALL_ROWS;
        spare->drawnStale = 0;
    }
    return spare;
}

// A page was rewritten whole with glTexImage2D: its drawn texture is current, the spare is not.
static void AtlasSpares_PageReplaced(int32_t slotIdx, uint32_t page)
{
    if (page < VOXEL_ANIM_PAGES && sAtlasSpares[page].ownerSlot == slotIdx)
    {
        sAtlasSpares[page].spareStale = VOXEL_ATLAS_ALL_ROWS;
        sAtlasSpares[page].drawnStale = 0;
    }
}

// Brings the spare of an animated page up to date and swaps it in, once it has been idle
// long enough. Returns the number of 16 px rows written (0 when it has to wait).
static uint32_t AtlasSpares_Present(int32_t slotIdx, uint32_t page)
{
    VoxelAtlasSlot *const slot = &sAtlasSlots[slotIdx];
    VoxelAtlasPageSpare *const spare = &sAtlasSpares[page];
    uint32_t rowsWritten;

    if (spare->ownerSlot != slotIdx || spare->drawnStale == 0 || !slot->tex[page])
        return 0;
    if (!spare->spare)
    {
        // First use: a new texture is never in flight, so it is created already current.
        spare->spare = CreateRgba5551Texture(VOXEL_ATLAS_W, VOXEL_ATLAS_H, slot->linear[page]);
        rowsWritten = VOXEL_ATLAS_ROW_COUNT;
    }
    else
    {
        if (!TextureIdleLongEnough(spare->idleSince))
            return 0;
        UploadAtlasRows(spare->spare, slot->linear[page], spare->spareStale);
        rowsWritten = (uint32_t)__builtin_popcount(spare->spareStale);
    }

    const GLuint drawn = slot->tex[page];
    slot->tex[page] = spare->spare;
    spare->spare = drawn;
    spare->idleSince = sFrameCounter;
    spare->spareStale = spare->drawnStale;
    spare->drawnStale = 0;
    if (slotIdx == sActiveAtlasSlot)
        sAtlasTex[page] = slot->tex[page];
    return rowsWritten;
}

// Writes `rows` of a page straight into its drawn texture's memory, as the sprite atlas does:
// for the slots of neighbouring maps, which have no spare. The GPU may still be reading it for
// the previous frame: at worst a wave shows its next frame one frame early.
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
// Only the 64-px bands holding re-decoded slots are unswizzled and copied (the whole 512x256
// atlas took ~3 ms per walking frame in SESION 22).
static void WriteSpriteAtlasSlots(const uint16_t *morton, uint16_t *linear, uint32_t dirtySlots)
{
    uint32_t bands = 0;
    for (uint32_t s = 0; s < VOXEL_SPRITE_SLOTS; s++)
    {
        if (dirtySlots & (1u << s))
            bands |= 1u << (s / VOXEL_SPRITE_COLUMNS);
    }
    if (bands == 0)
        return;

    glBindTexture(GL_TEXTURE_2D, sSpriteTex);
#ifdef PLATFORM_VITA
    uint16_t *const texels = (uint16_t *)vglGetTexDataPointer(GL_TEXTURE_2D);
#else
    uint16_t *const texels = NULL;
#endif
    for (uint32_t band = 0; band < VOXEL_SPRITE_ATLAS_H / VOXEL_SPRITE_SLOT_DIM; band++)
    {
        if (!(bands & (1u << band)))
            continue;
        const uint32_t startY = band * VOXEL_SPRITE_SLOT_DIM;
        Voxel_UnswizzleMortonToLinearRows(morton, linear, VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_ATLAS_H,
                                          startY, VOXEL_SPRITE_SLOT_DIM);
        // The linear copy is bottom-up: atlas rows startY.. land on texture rows glY0..
        const uint32_t glY0 = VOXEL_SPRITE_ATLAS_H - (startY + VOXEL_SPRITE_SLOT_DIM);
        const uint16_t *const src = &linear[glY0 * VOXEL_SPRITE_ATLAS_W];
        if (texels != NULL)
            memcpy(&texels[glY0 * VOXEL_SPRITE_ATLAS_W], src,
                   (size_t)VOXEL_SPRITE_SLOT_DIM * VOXEL_SPRITE_ATLAS_W * sizeof(uint16_t));
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)glY0, VOXEL_SPRITE_ATLAS_W, VOXEL_SPRITE_SLOT_DIM,
                            GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, src);
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
            AtlasSpares_PageReplaced(sExtendSlot, page);
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

    int32_t victimSlot = 0;
    uint32_t oldestStamp = UINT32_MAX;
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        if (!sAtlasSlots[s].valid)
        {
            victimSlot = s;
            break;
        }
        if (s != sActiveAtlasSlot && sAtlasSlots[s].stamp < oldestStamp)
        {
            oldestStamp = sAtlasSlots[s].stamp;
            victimSlot = s;
        }
    }

    if (sAtlasSlots[victimSlot].valid)
        VitaPerf_MarkEvent(VITA_PERF_E_ATLAS_EVICT);
    ComposeAtlasSlot(&sAtlasSlots[victimSlot], inst);
    return victimSlot;
}

// Upload static chunk geometry to GPU VBO in CDRAM (Zero-Copy rendering on PS Vita)
static void UploadChunkVbo(VoxelChunkSlot *slot)
{
    if (!slot || slot->totalVertCount == 0)
        return;

    if (!slot->vbo)
        glGenBuffers(1, &slot->vbo);

    glBindBuffer(GL_ARRAY_BUFFER, slot->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(slot->totalVertCount * sizeof(VoxelGlVertex)), NULL, GL_STATIC_DRAW);

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
    uint32_t groundTotalCount;
    uint32_t treeCount;
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

static void PackGroundIntoJob(VoxelChunkJobOutput *out, const VoxelBuilder *builder, int32_t aSlot, bool outdoor)
{
    if (builder->count == 0)
        return;

    const VoxelAtlasSlot *const atlasSlot = &sAtlasSlots[aSlot];
    for (uint32_t page = 0; page < VOXEL_MAX_ATLAS_PAGES; page++)
    {
        for (uint32_t cutPass = 0; cutPass < 2u; cutPass++)
        {
            const bool wantCutout = (cutPass != 0u);
            const uint16_t firstVert = (uint16_t)out->groundTotalCount;
            uint16_t vertCount = 0;

            for (uint32_t i = 0; i + 2 < builder->count; i += 3)
            {
                uint32_t triPage = (uint32_t)(sScratch[i].u * 0.5f);
                if (triPage >= VOXEL_MAX_ATLAS_PAGES)
                    triPage = 0;
                if (triPage != page)
                    continue;
                if (IsAtlasTriangleCutout(atlasSlot, page, &sScratch[i]) != wantCutout)
                    continue;

                if (out->groundTotalCount + 3 <= CHUNK_GROUND_TOTAL_CAP)
                {
                    for (uint32_t j = 0; j < 3; j++)
                    {
                        VoxelGlVertex *dst = &out->groundVerts[out->groundTotalCount++];
                        dst->x = sScratch[i + j].x;
                        dst->y = sScratch[i + j].y;
                        dst->z = sScratch[i + j].z;
                        dst->u = sScratch[i + j].u - (float)page * 2.0f;
                        dst->v = sScratch[i + j].v;
                        BakeGradedVertexColor(sScratch[i + j].shade, outdoor, &dst->r, &dst->g, &dst->b);
                        dst->a = 255;
                    }
                    vertCount += 3;
                }
            }

            if (vertCount > 0 && out->groundSpanCount < MAX_CHUNK_GROUND_SPANS)
            {
                VoxelChunkGroundSpan *span = &out->groundSpans[out->groundSpanCount++];
                span->atlasSlot = (uint8_t)aSlot;
                span->page = (uint8_t)page;
                span->isCutout = (uint8_t)cutPass;
                span->pad = 0;
                span->firstVert = firstVert;
                span->vertCount = vertCount;
            }
        }
    }
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

static void ComputeChunkJob(const VoxelChunkJobInput *in, VoxelChunkJobOutput *out)
{
    const double computeStart = VitaPerf_NowMs();
    VoxelLightingWork lightBefore;
    VoxelLighting_GetWork(&lightBefore);
    const int32_t x0 = in->x0, y0 = in->y0, x1 = in->x1, y1 = in->y1;
    const VoxelMapInstance *inst0 = VoxelWorld_Instance(0);
    const uint32_t instanceCount = VoxelWorld_InstanceCount();

    out->groundVerts = sChunkStaging;
    out->treeVerts = sChunkStaging + CHUNK_GROUND_TOTAL_CAP;
    out->buildingVerts = out->treeVerts + CHUNK_TREE_CAP;
    out->groundTotalCount = 0;
    out->treeCount = 0;
    out->buildingCount = 0;
    out->groundSpanCount = 0;
    out->buildingSpanCount = 0;
    out->buildingPage = -1;
    out->usedAtlasMask = 0;
    out->uncoveredAtlasMask = 0;
    out->pageCount = 0;

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

        PackGroundIntoJob(out, &builder, aSlot, !inst->indoor);
    }
    out->groundMs = VitaPerf_NowMs() - phaseStart;

    // 2. Trees & Tall Grass across all overlapping instances
    phaseStart = VitaPerf_NowMs();
    VoxelBuilder_Init(&builder, sScratch, SCRATCH_VERTICES);
    VoxelBuilder_SetAtlas(&builder, in->activeMap);
    builder.originX = 0.0f;
    builder.originZ = 0.0f;

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
        uint32_t count = builder.count > CHUNK_TREE_CAP ? CHUNK_TREE_CAP : builder.count;
        PackVoxelVertices(sScratch, out->treeVerts, count, !in->indoor);
        out->treeCount = count;
    }
    out->treesMs = VitaPerf_NowMs() - phaseStart;

    // 3. Buildings & Indoor furniture across all overlapping instances
    phaseStart = VitaPerf_NowMs();
    VoxelBuilder_Init(&builder, sScratch, SCRATCH_VERTICES);
    VoxelBuilder_SetAtlas(&builder, in->activeMap);
    builder.originX = 0.0f;
    builder.originZ = 0.0f;

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
                        VoxelChunkBuildingSpan *bspan = &out->buildingSpans[out->buildingSpanCount++];
                        bspan->page = (int16_t)p;
                        bspan->firstVert = (uint16_t)countBefore;
                        bspan->vertCount = (uint16_t)(clampedEnd - countBefore);
                    }
                }
            }
        }
    }

    if (builder.count > 0)
    {
        uint32_t count = builder.count > CHUNK_BUILDING_CAP ? CHUNK_BUILDING_CAP : builder.count;
        PackVoxelVertices(sScratch, out->buildingVerts, count, !in->indoor);
        out->buildingCount = count;
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

static void CommitChunkJob(VoxelChunkSlot *slot, int32_t chunkX, int32_t chunkY, uint32_t hash,
                           const VoxelChunkJobOutput *out)
{
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
    slot->buildingPage = out->buildingPage;
    slot->groundTotalCount = out->groundTotalCount;
    slot->treeCount = out->treeCount;
    slot->buildingCount = out->buildingCount;
    // Built in the current coordinates, so drawn where it is built.
    slot->drawOffsetX = 0.0f;
    slot->drawOffsetZ = 0.0f;

    // Canonical vertical bounds matching Frustum_ChunkInView [-2.5f, 12.0f];
    // Eradicates iterating over up to 30,000 float vertices on CPU in every chunk build.
    slot->minY = -2.5f;
    slot->maxY = 12.0f;

    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        slot->atlasGen[s] = sAtlasSlots[s].generation;
        slot->atlasExt[s] = sAtlasSlots[s].extension;
    }
    slot->treeFirstVert = (uint16_t)slot->groundTotalCount;
    slot->buildingFirstVert = (uint16_t)(slot->groundTotalCount + slot->treeCount);
    slot->totalVertCount = (uint16_t)(slot->groundTotalCount + slot->treeCount + slot->buildingCount);
    VitaPerf_AddCount(VITA_PERF_C_BUILD_VERTS, slot->totalVertCount);

    // UploadChunkVbo reads the staging buffer through the slot's part pointers.
    slot->groundVerts = out->groundVerts;
    slot->treeVerts = out->treeVerts;
    slot->buildingVerts = out->buildingVerts;
    VitaPerf_SubBegin(VITA_PERF_T_BUILD_UPLOAD);
    UploadChunkVbo(slot);
    VitaPerf_SubEnd(VITA_PERF_T_BUILD_UPLOAD);
    // The staging buffer belongs to the next build now.
    slot->groundVerts = NULL;
    slot->treeVerts = NULL;
    slot->buildingVerts = NULL;

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
    ComputeChunkJob(&sJobIn, &sJobOut);
    ReportChunkJobWork(&sJobOut, false);

    // Metatiles missing from an atlas slot: extend the slot and compute again, so the chunk is
    // drawn complete (what the single-phase build did for the instance in place).
    if (sJobOut.uncoveredAtlasMask != 0 && ExtendJobAtlasSlots(&sJobIn, &sJobOut))
    {
        ComputeChunkJob(&sJobIn, &sJobOut);
        ReportChunkJobWork(&sJobOut, false);
    }

    CommitChunkJob(slot, chunkX, chunkY, hash, &sJobOut);
}

// ---- Chunk worker (Core 2) ----
//
// One chunk at a time is computed on Core 2 (ComputeChunkJob), the rest of its build staying
// on Core 0 (PrepareChunkJob, CommitChunkJob). The job runs in the window where Core 0 does
// not touch the voxel world: launched after the swap (ChunkWorker_Launch), waited for at the
// start of the next VoxelRenderer_RenderFrame (ChunkWorker_Join) and committed there, before
// the frame looks at the chunks. In that window the worker owns the lighting caches, the
// building module's caches, the mesh builder's window, sScratch and sChunkStaging; the world
// lookups' memos are frozen (VoxelWorld_FreezeMemos). A result is dropped when the world
// epoch or the generation of an atlas slot it uses changed meanwhile. Visible holes and
// warm-up/faded frames are still built on Core 0 (BuildChunk). See docs/THREADS.md.

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
static VoxelChunkJobInput sAsyncIn;
static VoxelChunkJobOutput sAsyncOut;
static int32_t sAsyncCx, sAsyncCy;
static uint32_t sAsyncHash;
static uint32_t sAsyncEpoch;
static uint32_t sAsyncAtlasGen[VOXEL_ATLAS_SLOTS];

static int32_t VoxelChunkWorkerMain(void *data)
{
    (void)data;
#ifdef PLATFORM_VITA
    VitaPlatform_PinCurrentThread("VoxelChunkWorker", 2);
#endif
    // Below the draw workers and the I/O worker that share Core 2.
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
    while (!SDL_AtomicGet(&sChunkWorkerStop))
    {
        if (SDL_SemWait(sChunkJobSem) != 0)
            continue;
        if (SDL_AtomicGet(&sChunkWorkerStop))
            break;
        ComputeChunkJob(&sAsyncIn, &sAsyncOut);
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

static void ChunkWorker_Stop(void)
{
    ChunkWorker_Join();
    if (sChunkWorkerThread != NULL)
    {
        SDL_AtomicSet(&sChunkWorkerStop, 1);
        SDL_SemPost(sChunkJobSem);
        SDL_WaitThread(sChunkWorkerThread, NULL);
        sChunkWorkerThread = NULL;
    }
    if (sChunkJobSem) { SDL_DestroySemaphore(sChunkJobSem); sChunkJobSem = NULL; }
    if (sChunkDoneSem) { SDL_DestroySemaphore(sChunkDoneSem); sChunkDoneSem = NULL; }
    sChunkJobState = CHUNK_JOB_NONE;
}

// Chooses the chunk the worker computes after this frame (Core 0, during the chunk pass).
static void ChunkWorker_Choose(int32_t cx, int32_t cy, uint32_t hash)
{
    sAsyncCx = cx;
    sAsyncCy = cy;
    sAsyncHash = hash;
    sChunkJobState = CHUNK_JOB_CHOSEN;
}

// After the swap (Core 0): prepares the chosen chunk and hands it to the worker.
static void ChunkWorker_Launch(void)
{
    if (sChunkJobState != CHUNK_JOB_CHOSEN || !ChunkWorker_Available())
        return;
    PrepareChunkJob(&sAsyncIn, sAsyncCx, sAsyncCy);
    sAsyncEpoch = sWorldEpoch;
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
        sAsyncAtlasGen[s] = sAtlasSlots[s].generation;
    VoxelWorld_FreezeMemos(true);
    sChunkJobState = CHUNK_JOB_RUNNING;
    SDL_SemPost(sChunkJobSem);
}

// Commits a finished job if the world it was computed in still stands (Core 0, before the
// chunk pass). A dropped job's chunk is simply chosen again.
static void ChunkWorker_Commit(void)
{
    if (sChunkJobState != CHUNK_JOB_DONE)
        return;
    sChunkJobState = CHUNK_JOB_NONE;

    bool current = (sAsyncEpoch == sWorldEpoch && !sNeedsFullWarmup);
    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS && current; s++)
    {
        if ((sAsyncOut.usedAtlasMask & (1u << s))
            && (!sAtlasSlots[s].valid || sAtlasSlots[s].generation != sAsyncAtlasGen[s]))
            current = false;
    }
    VoxelChunkSlot *slot = FindChunkSlot(sAsyncCx, sAsyncCy);
    // Rebuilt on Core 0 meanwhile (it became a hole or the frame warmed up).
    if (slot != NULL && slot->valid && slot->checkedEpoch == sWorldEpoch && slot->hash == sAsyncHash)
        current = false;
    if (!current)
    {
        VitaPerf_AddCount(VITA_PERF_C_WORKER_DROPPED, 1);
        return;
    }
    ReportChunkJobWork(&sAsyncOut, true);
    if (slot == NULL)
        slot = GetAvailableChunkSlot(sAsyncCx, sAsyncCy);
    // Missing metatiles: the chunk is committed as it is (drawn until the atlas grows) and the
    // extension makes it urgent, so Core 0 rebuilds it complete.
    if (sAsyncOut.uncoveredAtlasMask != 0)
        ExtendJobAtlasSlots(&sAsyncIn, &sAsyncOut);
    CommitChunkJob(slot, sAsyncCx, sAsyncCy, sAsyncHash, &sAsyncOut);
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
        *x += sMoveVelX / speed * VOXEL_PREFETCH_LOOKAHEAD;
        *z += sMoveVelZ / speed * VOXEL_PREFETCH_LOOKAHEAD;
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
                        0.96f, 1.04f, 0.0f, 0.07f, 0.0f,
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
            light.rays = 0.14f;
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
        const VitaDayNightLight *cycle = VitaDayNight_Get();
        VitaDayNightBlend moon = VitaDayNight_Moonlight(cycle, 0.0f, cycle->rgb);

        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            light.sun[channel] *= cycle->rgb[channel] * (1.0f - moon.amount);
            light.shade[channel] *= cycle->rgb[channel] * (1.0f - moon.amount);
            light.hazeRgb[channel] *= cycle->rgb[channel];
            light.hazeRgb[channel] += (moon.rgb[channel] - light.hazeRgb[channel]) * moon.amount;
        }
        light.rays *= cycle->daylight;
        light.motes *= cycle->daylight;
        light.dappleLow = 1.0f + (light.dappleLow - 1.0f) * cycle->daylight;
        light.dappleHigh = 1.0f + (light.dappleHigh - 1.0f) * cycle->daylight;
    }
    return light;
}

static float DappleLattice(int32_t x, int32_t y, int32_t period, uint32_t seed)
{
    uint32_t h = (uint32_t)(x % period) * 374761393u + (uint32_t)(y % period) * 668265263u
               + seed * 2246822519u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}

static float Quintic(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float DappleNoise(float u, float v, int32_t period, uint32_t seed)
{
    int32_t x = (int32_t)u, y = (int32_t)v;
    float fx = Quintic(u - (float)x), fy = Quintic(v - (float)y);
    float a = DappleLattice(x, y, period, seed), b = DappleLattice(x + 1, y, period, seed);
    float c = DappleLattice(x, y + 1, period, seed), d = DappleLattice(x + 1, y + 1, period, seed);
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
            float n = DappleNoise(u * 3.0f,  v * 2.0f, 3,  0xF06B10u) * 0.50f
                    + DappleNoise(u * 6.0f,  v * 4.0f, 6,  0xF06B21u) * 0.28f
                    + DappleNoise(u * 12.0f, v * 8.0f, 12, 0xF06B32u) * 0.15f
                    + DappleNoise(u * 24.0f, v * 16.0f, 24, 0xF06B43u) * 0.07f;
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

    for (uint32_t b = 0; b < 2; ++b)
    {
        uint32_t idx = 0;
        for (int32_t gz = 0; gz < VOXEL_FOG_GRID; ++gz)
        {
            for (int32_t gx = 0; gx < VOXEL_FOG_GRID; ++gx)
            {
                static const int32_t cx[6] = {0, 1, 1, 0, 1, 0};
                static const int32_t cz[6] = {0, 0, 1, 0, 1, 1};
                for (int32_t k = 0; k < 6; ++k)
                {
                    int32_t vx = gx + cx[k];
                    int32_t vz = gz + cz[k];
                    float fx = (float)vx / (float)VOXEL_FOG_GRID;
                    float fz = (float)vz / (float)VOXEL_FOG_GRID;
                    float edgeX = fx < 0.5f ? fx * 4.0f : (1.0f - fx) * 4.0f;
                    float edgeZ = fz < 0.5f ? fz * 4.0f : (1.0f - fz) * 4.0f;
                    if (edgeX > 1.0f) edgeX = 1.0f;
                    if (edgeZ > 1.0f) edgeZ = 1.0f;
                    float edge = edgeX * edgeZ;
                    edge = edge * edge * (3.0f - 2.0f * edge);

                    VoxelGlVertex *v = &sFogSheets[b][idx++];
                    v->x = (fx - 0.5f) * 2.0f * VOXEL_FOG_SPAN_X;
                    v->y = sFogBanks[b].height;
                    v->z = VOXEL_FOG_SOUTH - fz * (VOXEL_FOG_SOUTH + VOXEL_FOG_NORTH);
                    v->u = v->x / VOXEL_FOG_PERIOD_X;
                    v->v = v->z / VOXEL_FOG_PERIOD_Z;
                    v->r = 255;
                    v->g = 255;
                    v->b = 255;
                    v->a = (uint8_t)(edge * 255.0f + 0.5f);
                }
            }
        }
    }
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

static void DrawFogBanks(const FogBank banks[2], float amount, uint8_t r, uint8_t g, uint8_t b, float seconds)
{
    if (!sFogTex || amount <= 0.005f)
        return;

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
    for (uint32_t bk = 0; bk < bankCount; ++bk)
    {
        float uOff = fmodf(seconds * banks[bk].driftX / VOXEL_FOG_PERIOD_X + banks[bk].offsetU, 1.0f);
        float vOff = fmodf(seconds * banks[bk].driftZ / VOXEL_FOG_PERIOD_Z + banks[bk].offsetV, 1.0f);
        float tx = sCamera.targetX - uOff * VOXEL_FOG_PERIOD_X;
        float tz = sCamera.targetZ - vOff * VOXEL_FOG_PERIOD_Z;
        float ty = sCamera.ground;
        float alphaScale = (banks == sSandBanks ? 0.61f : banks[bk].alpha) * amount;

        for (uint32_t i = 0; i < VOXEL_FOG_BANK_VERTICES; ++i)
        {
            sBatchGlVerts[i] = sFogSheets[bk][i];
            sBatchGlVerts[i].x += tx;
            sBatchGlVerts[i].y = ty + banks[bk].height;
            sBatchGlVerts[i].z += tz;
            sBatchGlVerts[i].u = sBatchGlVerts[i].x / VOXEL_FOG_PERIOD_X;
            sBatchGlVerts[i].v = sBatchGlVerts[i].z / VOXEL_FOG_PERIOD_Z;
            sBatchGlVerts[i].r = r;
            sBatchGlVerts[i].g = g;
            sBatchGlVerts[i].b = b;
            sBatchGlVerts[i].a = (uint8_t)((float)sFogSheets[bk][i].a * alphaScale + 0.5f);
        }
        glDrawArrays(GL_TRIANGLES, 0, VOXEL_FOG_BANK_VERTICES);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

static void DrawWeather(const WeatherKind *kind, float amount, float bolt, float seconds)
{
    if (!kind || amount <= 0.005f)
        return;

    uint32_t count = (uint32_t)(amount * (float)kind->maxCount + 0.5f);
    if (count < kind->minCount) count = kind->minCount;
    if (count > kind->maxCount) count = kind->maxCount;
    if (count > 128) count = 128;

    float offX = fmodf((seconds * kind->vx - sCamera.targetX) / VOXEL_WEATHER_SPAN_X, 1.0f);
    float offY = fmodf((seconds * kind->vy - sCamera.ground) / VOXEL_WEATHER_SPAN_Y, 1.0f);
    float offZ = fmodf((seconds * kind->vz - sCamera.targetZ) / VOXEL_WEATHER_SPAN_Z, 1.0f);
    if (offX < 0.0f) offX += 1.0f;
    if (offY < 0.0f) offY += 1.0f;
    if (offZ < 0.0f) offZ += 1.0f;

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
    float acrossX = kind->width;

    const VitaDayNightLight *cycle = VitaDayNight_Get();
    VitaDayNightBlend moon = VitaDayNight_Moonlight(cycle, 0.0f, cycle->rgb);
    uint8_t baseR = (uint8_t)((kind->colour & 0xFFu) * (cycle->rgb[0] * (1.0f - moon.amount) + moon.rgb[0] * moon.amount));
    uint8_t baseG = (uint8_t)(((kind->colour >> 8) & 0xFFu) * (cycle->rgb[1] * (1.0f - moon.amount) + moon.rgb[1] * moon.amount));
    uint8_t baseB = (uint8_t)(((kind->colour >> 16) & 0xFFu) * (cycle->rgb[2] * (1.0f - moon.amount) + moon.rgb[2] * moon.amount));
    float baseAlpha = (kind->alpha + 0.16f * bolt) * (0.35f + 0.65f * amount);
    if (baseAlpha > 1.0f) baseAlpha = 1.0f;

    static const float rectCx[6]  = {-1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f};
    static const float rectCy[6]  = {-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f};
    static const float flakeCx[6] = { 0.0f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f};
    static const float flakeCy[6] = {-1.0f,  0.0f,  1.0f, -1.0f,  1.0f,  0.0f};
    const float *cxTab = kind->flake ? flakeCx : rectCx;
    const float *cyTab = kind->flake ? flakeCy : rectCy;

    uint32_t vOut = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        float px = sWeatherSeeds[i][0] + offX; if (px >= 1.0f) px -= 1.0f;
        float py = sWeatherSeeds[i][1] + offY; if (py >= 1.0f) py -= 1.0f;
        float pz = sWeatherSeeds[i][2] + offZ; if (pz >= 1.0f) pz -= 1.0f;

        float dx = 0.5f - fabsf(px - 0.5f);
        float dy = 0.5f - fabsf(py - 0.5f);
        float dz = 0.5f - fabsf(pz - 0.5f);
        float fadeX = dx * (VOXEL_WEATHER_SPAN_X / VOXEL_WEATHER_FADE_XZ);
        float fadeZ = dz * (VOXEL_WEATHER_SPAN_Z / VOXEL_WEATHER_FADE_XZ);
        float fadeY = dy * (VOXEL_WEATHER_SPAN_Y / VOXEL_WEATHER_FADE_Y);
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
        float nearStartSq = kind->nearStart * kind->nearStart;
        if (distSq <= nearStartSq) continue;
        float nearEnd = kind->nearStart + kind->nearRamp;
        float nearEndSq = nearEnd * nearEnd;
        float lens = 1.0f;
        if (distSq < nearEndSq)
        {
            float dist = sqrtf(distSq);
            lens = (dist - kind->nearStart) / kind->nearRamp;
            if (lens <= 0.01f) continue;
        }

        uint8_t a = (uint8_t)(baseAlpha * fade * lens * 255.0f + 0.5f);
        if (a == 0) continue;

        for (int32_t k = 0; k < 6; ++k)
        {
            VoxelGlVertex *v = &sBatchGlVerts[vOut++];
            v->x = wx + acrossX * cxTab[k] + alongX * cyTab[k];
            v->y = wy + alongY * cyTab[k];
            v->z = wz + alongZ * cyTab[k];
            v->u = 0.0f;
            v->v = 0.0f;
            v->r = baseR;
            v->g = baseG;
            v->b = baseB;
            v->a = a;
        }
    }

    if (vOut > 0)
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, vOut);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
    }
}

static void DrawMotes(float strength, float seconds)
{
    if (strength <= 0.005f)
        return;

    float pitchRad = sCamera.pitch * (3.14159265f / 180.0f);
    float upY = cosf(pitchRad) * VOXEL_MOTE_RADIUS;
    float upZ = sinf(pitchRad) * VOXEL_MOTE_RADIUS;
    float rightX = VOXEL_MOTE_RADIUS;

    static const float dCx[6] = { 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f};
    static const float dCy[6] = {-1.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f};

    float ox = sCamera.targetX - 0.5f * VOXEL_MOTE_SPAN_X;
    float oy = sCamera.ground;
    float oz = sCamera.targetZ - VOXEL_MOTE_SPAN_Z + 4.0f;

    uint32_t vOut = 0;
    for (uint32_t i = 0; i < VOXEL_MOTE_COUNT; ++i)
    {
        const MoteSpec *m = &sMoteSpecs[i];
        float glint = 0.5f + 0.5f * sinf(seconds * m->glintSpeed + m->glintPhase);
        glint *= glint;
        float wx = ox + fmodf(m->x + seconds * m->driftX + VOXEL_MOTE_SPAN_X * 64.0f, VOXEL_MOTE_SPAN_X);
        float wz = oz + fmodf(m->z + seconds * m->driftZ + VOXEL_MOTE_SPAN_Z * 64.0f, VOXEL_MOTE_SPAN_Z);
        float wy = oy + m->y + m->bobAmp * sinf(seconds * m->bobSpeed + m->bobPhase);
        uint8_t a = (uint8_t)(VOXEL_MOTE_ALPHA * strength * (0.25f + 0.75f * glint) * 255.0f + 0.5f);
        if (a == 0) continue;

        for (int32_t k = 0; k < 6; ++k)
        {
            VoxelGlVertex *v = &sBatchGlVerts[vOut++];
            v->x = wx + rightX * dCx[k];
            v->y = wy + upY * dCy[k];
            v->z = wz + upZ * dCy[k];
            v->u = 0.0f;
            v->v = 0.0f;
            v->r = 255;
            v->g = 238;
            v->b = 185;
            v->a = a;
        }
    }

    if (vOut > 0)
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sBatchGlVerts[0].x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sBatchGlVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, vOut);
        glDisableClientState(GL_COLOR_ARRAY);
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

static void DrawRays(float strength, float seconds)
{
    if (strength <= 0.005f)
        return;

    static const struct {
        float xCenter;
        float halfWidth;
        float reachY;
        float weight;
    } shafts[4] = {
        { 0.14f, 0.11f, 0.88f, 1.00f },
        { 0.36f, 0.08f, 0.76f, 0.72f },
        { 0.58f, 0.12f, 0.84f, 0.85f },
        { 0.80f, 0.07f, 0.68f, 0.58f },
    };

    const float drift = 0.03f * sinf(seconds * 0.45f);
    const float skew = tanf(35.0f * (3.14159265f / 180.0f)) * (544.0f / 960.0f);
    uint32_t vOut = 0;

    for (int32_t i = 0; i < 4; ++i)
    {
        const float xc = shafts[i].xCenter + drift;
        const float hw = shafts[i].halfWidth;
        const float yBot = shafts[i].reachY;
        const float xb = xc + yBot * skew;
        float aTop = strength * shafts[i].weight * 0.65f;
        if (aTop > 0.22f) aTop = 0.22f;
        aTop *= VOXEL_RAYS_OPACITY;
        const uint8_t alphaTop = (uint8_t)(aTop * 255.0f + 0.5f);

        // Left feather -> center
        PushFlatVertex2D(&vOut, xc - hw, 0.0f, 255, 235, 176, 0);
        PushFlatVertex2D(&vOut, xc,      0.0f, 255, 235, 176, alphaTop);
        PushFlatVertex2D(&vOut, xb,      yBot, 255, 235, 176, 0);

        PushFlatVertex2D(&vOut, xc - hw, 0.0f, 255, 235, 176, 0);
        PushFlatVertex2D(&vOut, xb,      yBot, 255, 235, 176, 0);
        PushFlatVertex2D(&vOut, xb - hw, yBot, 255, 235, 176, 0);

        // Center -> right feather
        PushFlatVertex2D(&vOut, xc,      0.0f, 255, 235, 176, alphaTop);
        PushFlatVertex2D(&vOut, xc + hw, 0.0f, 255, 235, 176, 0);
        PushFlatVertex2D(&vOut, xb + hw, yBot, 255, 235, 176, 0);

        PushFlatVertex2D(&vOut, xc,      0.0f, 255, 235, 176, alphaTop);
        PushFlatVertex2D(&vOut, xb + hw, yBot, 255, 235, 176, 0);
        PushFlatVertex2D(&vOut, xb,      yBot, 255, 235, 176, 0);
    }

    glDisable(GL_DEPTH_TEST);
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
}

static void UpdateAndDrawNightLamps(float glow)
{
    if (glow <= 0.01f)
        return;

    if (!sLampsBuilt)
    {
        // (built with sScratch, which the chunk worker uses)
        EnsureWorkerIdle();
        VoxelBuilder glassB, groundB;
        VoxelBuilder_Init(&glassB, sLampGlassScratch, VOXEL_LAMP_GLASS_CAP);
        VoxelBuilder_Init(&groundB, sLampGroundScratch, VOXEL_LAMP_GROUND_CAP);

        uint32_t instCount = VoxelWorld_InstanceCount();
        for (uint32_t i = 0; i < instCount; ++i)
        {
            const VoxelMapInstance *inst = VoxelWorld_Instance(i);
            if (!inst || inst->indoor) continue;
            glassB.base = VoxelRelief_Base(inst);
            groundB.base = VoxelRelief_Base(inst);
            VoxelBuilder_SetStretch(&glassB, inst);
            VoxelBuilder_SetStretch(&groundB, inst);
            VoxelSign_EmitNight(&glassB, &groundB, inst,
                                inst->originX, inst->originY,
                                inst->originX + inst->width, inst->originY + inst->height);
        }

        sLampGlassCount = glassB.count;
        sLampGroundCount = groundB.count;
        sLampsBuilt = true;
    }

    if (sLampGlassCount == 0 && sLampGroundCount == 0)
        return;

    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    if (sLampGroundCount > 0 && sLampPoolTex != 0)
    {
        float k = glow * 0.5f;
        uint8_t r = (uint8_t)(k * 255.0f + 0.5f);
        uint8_t g = (uint8_t)(k * 150.0f + 0.5f);
        uint8_t b = (uint8_t)(k * 60.0f + 0.5f);
        for (uint32_t i = 0; i < sLampGroundCount; ++i)
        {
            sLampGroundVerts[i].x = sLampGroundScratch[i].x;
            sLampGroundVerts[i].y = sLampGroundScratch[i].y + 0.015f;
            sLampGroundVerts[i].z = sLampGroundScratch[i].z;
            sLampGroundVerts[i].u = sLampGroundScratch[i].u;
            sLampGroundVerts[i].v = sLampGroundScratch[i].v;
            sLampGroundVerts[i].r = r;
            sLampGroundVerts[i].g = g;
            sLampGroundVerts[i].b = b;
            sLampGroundVerts[i].a = 255;
        }
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sLampPoolTex);
        glBlendFunc(GL_DST_COLOR, GL_ONE);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sLampGroundVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sLampGroundVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sLampGroundVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, sLampGroundCount);
    }

    if (sLampGlassCount > 0)
    {
        uint8_t a = (uint8_t)(glow * 220.0f + 0.5f);
        for (uint32_t i = 0; i < sLampGlassCount; ++i)
        {
            sLampGlassVerts[i].x = sLampGlassScratch[i].x;
            sLampGlassVerts[i].y = sLampGlassScratch[i].y;
            sLampGlassVerts[i].z = sLampGlassScratch[i].z;
            sLampGlassVerts[i].u = 0.0f;
            sLampGlassVerts[i].v = 0.0f;
            sLampGlassVerts[i].r = 255;
            sLampGlassVerts[i].g = 191;
            sLampGlassVerts[i].b = 107;
            sLampGlassVerts[i].a = a;
        }
        glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), &sLampGlassVerts[0].x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), &sLampGlassVerts[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), &sLampGlassVerts[0].r);
        glDrawArrays(GL_TRIANGLES, 0, sLampGlassCount);
        glEnable(GL_TEXTURE_2D);
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Mouse look / orbit
void VoxelRenderer_HandleMouseMotion(int32_t dx, int32_t dy)
{
    (void)dx; (void)dy;
}

// Initialize the voxel renderer. Call after OpenGL context is created.
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
            glGenTextures(1, &sTreeTex);
            glBindTexture(GL_TEXTURE_2D, sTreeTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_TREE_TEXTURE_DIM, VOXEL_TREE_TEXTURE_DIM,
                         0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, sTreeLinear);
            VITA_LOG("[VOXEL] Loaded trees.rgba5551 successfully.\n");
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
    AtlasSpares_Reset(false);

    for (int32_t p = 0; p < VOXEL_MAX_ATLAS_PAGES; p++)
        sAtlasTex[p] = 0;
    sAtlasPageCount = 0;

    for (int32_t i = 0; i < VOXEL_BUILDING_CACHE_SLOTS; i++)
    {
        sBuildingCache[i].page = -1;
        sBuildingCache[i].tex = 0;
        sBuildingCache[i].lastUsedFrame = 0;
    }
    memset(sBuildingPageToSlot, -1, sizeof(sBuildingPageToSlot));
    memset(sBuildingPageHasAlpha, 0, sizeof(sBuildingPageHasAlpha));
    memset(sBuildingPageQueued, 0, sizeof(sBuildingPageQueued));
    sBuildingReqHead = 0;
    sBuildingReqTail = 0;
    sBuildingReqCount = 0;
    sWorkerReadyPage = -1;
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

    // Pre-allocate 2D UI overlay texture at maximum dimensions once
    glGenTextures(2, sUiTexRing);
    sUiImageValid = false;
    for (int32_t t = 0; t < 2; t++)
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

    // Generate 16x16 RGBA5551 radial luminance texture (512 B) for night street-lamp pools & 3D battle ovals
    {
        uint16_t lampPoolPixels[16 * 16];
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
                const uint16_t c5 = (uint16_t)(falloff * 31.0f + 0.5f);
                lampPoolPixels[y * 16 + x] = (uint16_t)((c5 << 11) | (c5 << 6) | (c5 << 1) | 1u);
            }
        }
        glGenTextures(1, &sLampPoolTex);
        glBindTexture(GL_TEXTURE_2D, sLampPoolTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, lampPoolPixels);
    }

    // Pre-allocate Diorama offscreen FBO (960x544 RGBA5551 color + Depth16 renderbuffer)
    glGenTextures(1, &sDioramaColorTex);
    glBindTexture(GL_TEXTURE_2D, sDioramaColorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_FBO_H, 0,
                 GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);

    glGenRenderbuffers(1, &sDioramaDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, sDioramaDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, VOXEL_DIORAMA_FBO_W, VOXEL_DIORAMA_FBO_H);

    glGenFramebuffers(1, &sDioramaFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sDioramaFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sDioramaColorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sDioramaDepthRb);

    // Downscaled targets of the depth of field (480x272, half resolution, no depth): the scene
    // downsample (the light blur) and the wider blur. RGBA8: the blur averages taps by
    // blending, and 5-bit channels would band and darken (512 KB each).
    GLuint *const blurTex[2] = { &sScratchColorTex, &sBlurTex };
    GLuint *const blurFbo[2] = { &sScratchFbo, &sBlurFbo };
    for (int32_t i = 0; i < 2; ++i)
    {
        glGenTextures(1, blurTex[i]);
        glBindTexture(GL_TEXTURE_2D, *blurTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VOXEL_SCRATCH_FBO_W, VOXEL_SCRATCH_FBO_H, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, blurFbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, *blurFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *blurTex[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    sDioramaFboReady = (sDioramaFbo != 0 && sScratchFbo != 0 && sBlurFbo != 0);

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

// Shutdown and release resources
void VoxelRenderer_Shutdown(void)
{
    ChunkWorker_Stop();
    if (sBuildingIoThread != NULL)
    {
        SDL_AtomicSet(&sBuildingIoStop, 1);
        if (sBuildingReqSem)
            SDL_SemPost(sBuildingReqSem);
        if (sBuildingConsumedSem)
            SDL_SemPost(sBuildingConsumedSem);
        SDL_WaitThread(sBuildingIoThread, NULL);
        sBuildingIoThread = NULL;
    }
    if (sBuildingReqSem) { SDL_DestroySemaphore(sBuildingReqSem); sBuildingReqSem = NULL; }
    if (sBuildingConsumedSem) { SDL_DestroySemaphore(sBuildingConsumedSem); sBuildingConsumedSem = NULL; }
    if (sBuildingIoMutex) { SDL_DestroyMutex(sBuildingIoMutex); sBuildingIoMutex = NULL; }

    VoxelBuildings_Shutdown();
    VoxelRegions_Shutdown();
    VoxelRelief_Shutdown();
    VoxelSign_Shutdown();

    for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
    {
        for (int32_t p = 0; p < VOXEL_MAX_ATLAS_PAGES; p++)
        {
            if (sAtlasSlots[s].tex[p]) { glDeleteTextures(1, &sAtlasSlots[s].tex[p]); sAtlasSlots[s].tex[p] = 0; }
        }
        sAtlasSlots[s].valid = false;
    }
    sActiveAtlasSlot = -1;
    sAtlasPageCount = 0;
    AtlasExtend_Cancel();
    AtlasSpares_Reset(true);
    if (sTreeTex) { glDeleteTextures(1, &sTreeTex); sTreeTex = 0; }
    for (int32_t i = 0; i < VOXEL_BUILDING_CACHE_SLOTS; i++)
    {
        if (sBuildingCache[i].tex) { glDeleteTextures(1, &sBuildingCache[i].tex); sBuildingCache[i].tex = 0; }
        sBuildingCache[i].page = -1;
    }
    memset(sBuildingPageToSlot, -1, sizeof(sBuildingPageToSlot));
    if (sSpriteTex) { glDeleteTextures(1, &sSpriteTex); sSpriteTex = 0; }
    if (sUiTexRing[0]) { glDeleteTextures(2, sUiTexRing); sUiTexRing[0] = sUiTexRing[1] = 0; }
    sUiTex = 0;
    sUiImageValid = false;
    if (sLampPoolTex) { glDeleteTextures(1, &sLampPoolTex); sLampPoolTex = 0; }
    if (sFogTex) { glDeleteTextures(1, &sFogTex); sFogTex = 0; }
    if (sDioramaFbo) { glDeleteFramebuffers(1, &sDioramaFbo); sDioramaFbo = 0; }
    if (sDioramaColorTex) { glDeleteTextures(1, &sDioramaColorTex); sDioramaColorTex = 0; }
    if (sDioramaDepthRb) { glDeleteRenderbuffers(1, &sDioramaDepthRb); sDioramaDepthRb = 0; }
    if (sScratchFbo) { glDeleteFramebuffers(1, &sScratchFbo); sScratchFbo = 0; }
    if (sScratchColorTex) { glDeleteTextures(1, &sScratchColorTex); sScratchColorTex = 0; }
    if (sBlurFbo) { glDeleteFramebuffers(1, &sBlurFbo); sBlurFbo = 0; }
    if (sBlurTex) { glDeleteTextures(1, &sBlurTex); sBlurTex = 0; }
    sDioramaFboReady = false;

    if (sChunkStaging)
    {
        free(sChunkStaging);
        sChunkStaging = NULL;
    }
    for (int32_t i = 0; i < MAX_CHUNKS; i++)
    {
        if (sChunks[i].vbo)
        {
            glDeleteBuffers(1, &sChunks[i].vbo);
            sChunks[i].vbo = 0;
        }
        sChunks[i].valid = false;
        sChunks[i].groundVerts = NULL;
        sChunks[i].treeVerts = NULL;
        sChunks[i].buildingVerts = NULL;
    }
    sChunksAllocated = false;
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
    extern bool8 BattleTransition_IsActiveForUi(void);
    if (BattleTransition_IsActiveForUi())
        return true;

    extern bool8 Shop_IsBuyMenuActive(void);
    if (Shop_IsBuyMenuActive())
        return true;

    const bool inFieldMove = FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON)
                          || FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    const bool inFlyAnim = FieldEffectActiveListContains(FLDEFF_USE_FLY)
                        || FieldEffectActiveListContains(FLDEFF_FLY_IN);
    if (inFieldMove || inFlyAnim)
        return true;

    if ((REG_DISPCNT & 0x0100) == 0)
        return false;

    // New UI: the visible field windows are drawn by its mirrors, the GBA frame is not needed
    if (Ui_MirrorCovers())
        return false;

    // Standard message box active (NPC dialogues, signposts, text)
    extern u8 GetFieldMessageBoxMode(void);
    if (GetFieldMessageBoxMode() != 0) // 0 == FIELD_MESSAGE_BOX_HIDDEN
        return true;

    // Map name popup banner (e.g. "LITTLEROOT TOWN")
    extern u8 GetMapNamePopUpWindowId(void);
    if (GetMapNamePopUpWindowId() != 0xFF) // 0xFF == WINDOW_NONE
        return true;

    // Active text printer on standard window 0
    extern bool16 IsTextPrinterActive(u8 id);
    if (IsTextPrinterActive(0))
        return true;

    // Any secondary active window on BG0 (YES/NO, multichoice menus, etc.)
    for (int32_t w = 1; w < WINDOWS_MAX; w++)
    {
        if (gWindows[w].window.bg == 0 && gWindows[w].tileData != NULL)
            return true;
    }

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
    float fadeAmount;                   // palette fade, 0..1
    float fadeRgb[3];
    VoxelWeatherClass weather;
    float weatherAmount;
    float fogDensity;
    float bolt;                         // lightning flash, 0..1
    bool underground;
    VoxelLight light;
    float seconds;                      // atmosphere animation clock
    VoxelChunkSlot *visible[MAX_CHUNKS];
    int32_t visibleCount;
    VoxelFrameGrade grade;
    int32_t renderW, renderH;
    bool dioramaActive;                 // the 3D scene goes to the FBO for the depth of field
} VoxelFrame;

// Chunk builds left after the visibility loop: unbuilt chunks outside the view (pre-warm) and
// visible chunks whose content is only dated (stale).
typedef struct
{
    double frameStartMs;
    double budgetMs;                    // frame time the builds may use (ChunkBuildFits)
    int32_t built;                          // chunks built this frame
    bool indoor;
    float playerX, playerZ;
    float lookX, lookZ;                 // where pre-warming aims (MovementLookahead)
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
    VitaDayNight_Update();
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
        const bool worldStands = current != NULL && gSaveBlock1Ptr != NULL
            && current->mapGroup == gSaveBlock1Ptr->location.mapGroup
            && current->mapNum == gSaveBlock1Ptr->location.mapNum;
        if (inBattle3D || sNeedsFullWarmup || !worldStands)
            EnsureWorkerIdle();
        else
            VitaPerf_AddCount(VITA_PERF_C_WORKER_OVERLAP, 1);
    }

    if (!VoxelWorld_IsMapAvailable() && !inBattle3D)
        return false;

    // Zero-Stall Multicore Concurrency: when Core 1 is mutating map layout in
    // LoadMapFromCameraTransition, render the active valid diorama using previous instances and snapshot
    const bool mapLocked = (sMapMutationLock != 0);
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
    f->startMs = frameStartMs;
    f->inBattle3D = inBattle3D;
    f->mapLocked = mapLocked;
    f->inst0 = inst0;
    return true;
}

// Stage 1: follow camera (field or 3D battle), map changes and crossings.
static void Frame_FollowCamera(VoxelFrame *const f)
{
    const bool inBattle3D = f->inBattle3D;
    const bool mapLocked = f->mapLocked;
    const VoxelMapInstance *const inst0 = f->inst0;
    int32_t mapGroup = inst0->mapGroup;
    int32_t mapNum   = inst0->mapNum;
    bool mapChanged = (mapGroup != sMeshMapGroup || mapNum != sMeshMapNum);
    bool crossed = false;

    const VoxelEntitySnapshot *snap = VoxelEntities_GetVisibleSnapshot();
    float playerWorldX = (snap != NULL) ? snap->playerWorldX : 0.0f;
    float playerWorldZ = (snap != NULL) ? snap->playerWorldZ : 0.0f;
    if (snap != NULL && snap->mapGroup >= 0 && snap->mapNum >= 0)
    {
        int32_t snapOffX = 0, snapOffY = 0;
        if (VoxelWorld_GetInstanceOffset(snap->mapGroup, snap->mapNum, &snapOffX, &snapOffY))
        {
            playerWorldX += (float)snapOffX;
            playerWorldZ += (float)snapOffY;
        }
    }

    if (!mapLocked && (mapChanged || sNeedCameraSnap))
    {
        int32_t shiftTilesX = 0, shiftTilesY = 0;
        if (!sNeedCameraSnap)
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
            crossed = HandleMapChange(mapGroup, mapNum, playerWorldX, playerWorldZ);
            VitaPerf_SubEnd(VITA_PERF_T_MAP_SHIFT);
        }
        else
        {
            VoxelCamera_SetGround(&sCamera, VoxelEntities_PlayerGround(), 1);
            VoxelCamera_Snap(&sCamera, playerWorldX, playerWorldZ);
            sLastPlayerX = playerWorldX;
            sLastPlayerZ = playerWorldZ;
            sNeedCameraSnap = false;
        }

        if (crossed)
        {
            VitaPerf_MarkEvent(VITA_PERF_E_CROSS);
            VitaPerf_SubBegin(VITA_PERF_T_MAP_SHIFT);
            ShiftChunkCoordinates(shiftTilesX, shiftTilesY);
            VitaPerf_SubEnd(VITA_PERF_T_MAP_SHIFT);
        }
        else
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
    }
    else
    {
        if (sBattleState.begun)
        {
            sBattleState.begun = false;
            sBattleState.chosen = false;
            sNeedCameraSnap = true;
        }

        // Smooth camera tracking continues on every frame (except crossing frames where VoxelCamera_Shift already shifted camera, matching Emerald 3DS: CtrVoxel_Update)
        if (!sNeedCameraSnap && !crossed)
        {
            const float playerGround = VoxelEntities_PlayerGround();
            VoxelCamera_SetGround(&sCamera, playerGround, 0);
            VoxelCamera_Update(&sCamera, playerWorldX, playerWorldZ);
        }
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

// Stage 2: metatile atlas slots for the current map (multi-atlas LRU, as Emerald 3DS).
static void Frame_UpdateAtlas(const VoxelFrame *const f)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    const bool mapChanged = f->mapChanged;
    VitaPerf_SubBegin(VITA_PERF_T_ATLAS_SLOTS);
    bool tilesetChanged = (inst0->primaryTileset != sLastPrimary || inst0->secondaryTileset != sLastSecondary);
    // Composing, extending or activating an atlas slot changes the maps the worker reads.
    if (tilesetChanged || sActiveAtlasSlot < 0 || mapChanged || sNeedAtlasExtension || sExtendSlot >= 0)
        EnsureWorkerIdle();
    if (tilesetChanged || sActiveAtlasSlot < 0)
    {
        memset(sVoxelTilesetAnimDirty, 0, sizeof(sVoxelTilesetAnimDirty));
        sVoxelTilesetAnimPending = false;
        int32_t slotIdx = EnsureAtlasSlotForInstance(inst0);
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
    AtlasExtend_Step();

    VitaPerf_SubEnd(VITA_PERF_T_ATLAS_SLOTS);
}

// Stage 3: animated tiles (water, flowers, shores) recomposed and presented.
static void Frame_AnimateTiles(const VoxelFrame *const f)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    VitaPerf_MarkAtlasAnimStart();
    uint32_t rowsUpdated = 0;

    // Recomposing on the CPU is cheap (0.15-0.3 ms) and runs whenever the game animated a
    // tile; the GPU copy follows through the page spares (AtlasSpares_Present below).
    // (not while the slot is being extended: its linear pages are half composed; the dirty
    // tiles stay pending until it ends)
    if (sVoxelTilesetAnimPending && sAtlasPageCount > 0 && inst0 != NULL && sActiveAtlasSlot >= 0
        && !AtlasExtend_Busy(sActiveAtlasSlot))
    {
        // Primary tiles (0..511) in bytes 0..63; Secondary tiles (512..1023) in bytes 64..127.
        const uint32_t *const dirtyWords = (const uint32_t *)sVoxelTilesetAnimDirty;
        bool primaryDirty = false;
        for (uint32_t w = 0; w < 16; w++)
        {
            if (dirtyWords[w] != 0)
            {
                primaryDirty = true;
                break;
            }
        }
        bool secondaryDirty = false;
        for (uint32_t w = 16; w < 32; w++)
        {
            if (dirtyWords[w] != 0)
            {
                secondaryDirty = true;
                break;
            }
        }

        if (primaryDirty || secondaryDirty)
        {
            VoxelAtlasSlot *slot = &sAtlasSlots[sActiveAtlasSlot];
            const uint32_t maxPages = (sAtlasPageCount > 2) ? 2 : sAtlasPageCount;

            for (uint32_t page = 0; page < maxPages; page++)
            {
                // O(1) 32-word bitmask check: skip page immediately if it references NO dirty tiles
                const uint32_t *const pageWords = (const uint32_t *)slot->map.tileMask[page];
                bool pageHasDirty = false;
                for (uint32_t w = 0; w < 32; w++)
                {
                    if ((dirtyWords[w] & pageWords[w]) != 0)
                    {
                        pageHasDirty = true;
                        break;
                    }
                }
                if (!pageHasDirty)
                    continue;

                uint32_t changedRows = 0;
                VitaPerf_SubBegin(VITA_PERF_T_ATLAS_COMPOSE);
                uint32_t changed = VoxelAtlas_RefreshAnimatedPageRows(inst0, &slot->map, page,
                                                                       slot->linear[page], (const uint8_t *)BG_VRAM,
                                                                       sVoxelTilesetAnimDirty, &changedRows);
                VitaPerf_SubEnd(VITA_PERF_T_ATLAS_COMPOSE);
                if (changed > 0 && changedRows != 0)
                {
                    VoxelAtlasPageSpare *const spare = AtlasSpares_Claim(sActiveAtlasSlot, page);
                    spare->drawnStale |= changedRows;
                    spare->spareStale |= changedRows;
                }
            }
        }
        // Neighbouring maps drawn from other slots (another secondary tileset, the same primary):
        // their sea and flowers move with the current map's
        if (primaryDirty)
        {
            uint8_t primaryMask[128];
            memcpy(primaryMask, sVoxelTilesetAnimDirty, 64);
            memset(primaryMask + 64, 0, 64);
            const void *const primary = sAtlasSlots[sActiveAtlasSlot].primaryTileset;
            for (int32_t s = 0; s < VOXEL_ATLAS_SLOTS; s++)
            {
                VoxelAtlasSlot *const other = &sAtlasSlots[s];
                // (on screen last frame: Frame_SelectChunks stamps the slots it draws)
                if (s == sActiveAtlasSlot || !other->valid || other->primaryTileset != primary
                    || (uint32_t)(sFrameCounter - other->stamp) > 2u || AtlasExtend_Busy(s))
                    continue;
                const uint32_t pages = other->pageCount < VOXEL_ANIM_PAGES ? other->pageCount : VOXEL_ANIM_PAGES;
                for (uint32_t page = 0; page < pages; page++)
                {
                    const uint32_t *const pageWords = (const uint32_t *)other->map.tileMask[page];
                    bool pageHasDirty = false;
                    for (uint32_t w = 0; w < 16; w++)
                    {
                        if ((dirtyWords[w] & pageWords[w]) != 0)
                        {
                            pageHasDirty = true;
                            break;
                        }
                    }
                    if (!pageHasDirty || !other->tex[page])
                        continue;
                    uint32_t changedRows = 0;
                    VitaPerf_SubBegin(VITA_PERF_T_ATLAS_COMPOSE);
                    VoxelAtlas_RefreshAnimatedPrimaryRows(other->primaryTileset, other->secondaryTileset, &other->map,
                                                          page, other->linear[page], (const uint8_t *)BG_VRAM,
                                                          primaryMask, &changedRows);
                    VitaPerf_SubEnd(VITA_PERF_T_ATLAS_COMPOSE);
                    if (changedRows == 0)
                        continue;
                    WriteAtlasRowsDirect(other->tex[page], other->linear[page], changedRows);
                    // A spare left from when this slot was active lacks these rows now
                    if (sAtlasSpares[page].ownerSlot == s)
                        sAtlasSpares[page].spareStale |= changedRows;
                }
            }
        }
        memset(sVoxelTilesetAnimDirty, 0, sizeof(sVoxelTilesetAnimDirty));
        sVoxelTilesetAnimPending = false;
    }

    // Swap in the animated pages whose spare is idle long enough (may lag a few frames). The
    // owner may be a slot that was active until now: it is still drawn for neighbouring maps.
    VitaPerf_SubBegin(VITA_PERF_T_ATLAS_UPLOAD);
    for (uint32_t page = 0; page < VOXEL_ANIM_PAGES; page++)
    {
        if (sAtlasSpares[page].ownerSlot >= 0 && !AtlasExtend_Busy(sAtlasSpares[page].ownerSlot))
            rowsUpdated += AtlasSpares_Present(sAtlasSpares[page].ownerSlot, page);
    }
    VitaPerf_SubEnd(VITA_PERF_T_ATLAS_UPLOAD);
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
        sLampsBuilt = false;
    }
    
    f->indoor = indoor;
}

// Stage 5: screen fade, weather and the light of the frame.
static void Frame_ReadAtmosphere(VoxelFrame *const f)
{
    f->fadeAmount = 0.0f;
    f->fadeRgb[0] = 0.0f;
    f->fadeRgb[1] = 0.0f;
    f->fadeRgb[2] = 0.0f;
    VoxelWorld_ScreenFade(&f->fadeAmount, f->fadeRgb);

    f->weather = VoxelWorld_Weather();
    f->weatherAmount = VoxelWorld_WeatherAmount();
    f->fogDensity = VoxelWorld_FogDensity();
    f->bolt = VoxelWorld_Lightning();
    f->underground = VoxelWorld_Underground();
    f->light = LightFor(f->indoor, f->weather, f->weatherAmount, f->bolt);
    static uint64_t sVoxelAtmosphereStart = 0;
    if (sVoxelAtmosphereStart == 0)
        sVoxelAtmosphereStart = SDL_GetPerformanceCounter();
    f->seconds = (float)((double)(SDL_GetPerformanceCounter() - sVoxelAtmosphereStart) / (double)SDL_GetPerformanceFrequency());
}

// Whether a visible chunk must be rebuilt. `urgent`: drawn as it is, the chunk would be wrong,
// not just dated (no geometry, or texture coordinates into an atlas that was recomposed or
// extended since). `hash` is set, and `hashValid`, when the content hash had to be computed.
static bool ChunkNeedsRebuild(VoxelChunkSlot *const slot, const int32_t cx0, const int32_t cy0, const bool indoor,
                              bool *const urgent, uint32_t *const hash, bool *const hashValid)
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
    // was off screen when the change happened.
    if (slot->checkedEpoch != sWorldEpoch)
    {
        *hash = ChunkContentHash(cx0, cy0, indoor);
        *hashValid = true;
        if (slot->hash != *hash)
            return true;
        slot->checkedEpoch = sWorldEpoch;
    }
    return false;
}

// With the worker, the chunks that are not holes are not built on Core 0 at all: one a frame
// goes to Core 2 (computed after the swap, committed next frame). First a chunk about to enter
// the view, then the dated chunk nearest to the player, then the pre-warm chunk nearest to the
// lookahead point. The others wait for later frames.
// While an atlas slot is being extended (after a crossing) nothing is handed over: every frame
// of the extension waits for the worker before touching the atlas (Frame_UpdateAtlas), so a job
// would only move its cost onto Core 0 (waits of up to 31 ms, logs/SESION 28). Those chunks are
// not holes; they wait the few frames the extension lasts.
static void BuildQueue_SendToWorker(VoxelBuildQueue *const q)
{
    if (!ChunkWorker_Available() || sSyncBuildingLoads)
        return;
    if (sExtendSlot >= 0)
    {
        q->staleCount = 0;
        q->prewarmCount = 0;
        VitaPerf_AddCount(VITA_PERF_C_WORKER_HELD, 1);
        return;
    }

    int32_t pickCx = 0, pickCy = 0;
    bool picked = false;
    float bestDist = FLT_MAX;
    for (int32_t pass = 0; pass < 3 && !picked; pass++)
    {
        const int32_t count = (pass == 1) ? q->staleCount : q->prewarmCount;
        for (int32_t p = 0; p < count; p++)
        {
            if (pass == 0 && !q->prewarmSoon[p])
                continue;
            const int32_t cx = (pass == 1) ? q->staleCx[p] : q->prewarmCx[p];
            const int32_t cy = (pass == 1) ? q->staleCy[p] : q->prewarmCy[p];
            const float aimX = (pass == 1) ? q->playerX : q->lookX;
            const float aimZ = (pass == 1) ? q->playerZ : q->lookZ;
            const float dx = (float)(sChunkOriginX + cx * CHUNK_SIZE + CHUNK_SIZE / 2) - aimX;
            const float dz = (float)(sChunkOriginY + cy * CHUNK_SIZE + CHUNK_SIZE / 2) - aimZ;
            const float dist = dx * dx + dz * dz;
            if (dist < bestDist)
            {
                bestDist = dist;
                pickCx = cx;
                pickCy = cy;
                picked = true;
            }
        }
    }
    if (picked && sChunkJobState == CHUNK_JOB_NONE)
    {
        const int32_t cx0 = sChunkOriginX + pickCx * CHUNK_SIZE;
        const int32_t cy0 = sChunkOriginY + pickCy * CHUNK_SIZE;
        ChunkWorker_Choose(pickCx, pickCy, ChunkContentHash(cx0, cy0, q->indoor));
    }
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

    // Update camera frustum planes for hardware culling
    UpdateFrustumPlanes();

    // The chunk the worker computed since the last frame, if the world it was computed in
    // still stands (after this frame's map change, epoch and atlas updates).
    ChunkWorker_Commit();

    const float camTargetX = sCamera.targetX;
    const float camTargetZ = sCamera.targetZ;

    int32_t minTileY = (int32_t)floorf(camTargetZ - VOXEL_VIEW_NORTH_TILES);
    int32_t maxTileY = (int32_t)ceilf(camTargetZ + VOXEL_VIEW_SOUTH_TILES);

    int32_t minChunkY = FloorDivChunk(minTileY - sChunkOriginY);
    int32_t maxChunkY = FloorDivChunk(maxTileY - sChunkOriginY);

    VoxelChunkSlot **const visibleChunks = f->visible;
    int32_t visibleChunkCount = 0;
    VoxelBuildQueue q;
    q.frameStartMs = f->startMs;
    q.built = 0;
    q.indoor = indoor;
    q.playerX = f->playerX;
    q.playerZ = f->playerZ;
    q.prewarmCount = 0;
    q.staleCount = 0;

    const bool isWarmingUp = (sWarmupFrames > 0 || sNeedsFullWarmup);
    if (sWarmupFrames > 0)
    {
        sWarmupFrames--;
        if (sWarmupFrames == 0)
            VitaPerf_LogMemory("map warm-up done");
    }
    sSyncBuildingLoads = (isWarmingUp || f->fadeAmount >= 0.99f);
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

    // The candidate area reaches one chunk further on the side the player is heading to, so
    // chunks about to enter the view are built before they do. That extra band is only ever
    // pre-warmed, never drawn: the drawn set must not depend on the movement, or chunks at
    // the edge would appear and vanish as the speed changes (a crossing used to drop them for
    // one long frame, showing the background).
    const int32_t baseMinChunkY = minChunkY;
    const int32_t baseMaxChunkY = maxChunkY;
    if (sMoveVelZ < -VOXEL_MOVE_THRESHOLD) minTileY -= CHUNK_SIZE;
    if (sMoveVelZ > VOXEL_MOVE_THRESHOLD) maxTileY += CHUNK_SIZE;
    minChunkY = FloorDivChunk(minTileY - sChunkOriginY);
    maxChunkY = FloorDivChunk(maxTileY - sChunkOriginY);
    const int32_t extraLeft = (sMoveVelX < -VOXEL_MOVE_THRESHOLD) ? CHUNK_SIZE : 0;
    const int32_t extraRight = (sMoveVelX > VOXEL_MOVE_THRESHOLD) ? CHUNK_SIZE : 0;

    const float moveSpeed = sqrtf(sMoveVelX * sMoveVelX + sMoveVelZ * sMoveVelZ);
    const bool moving = moveSpeed > VOXEL_MOVE_THRESHOLD;
    const float aheadX = moving ? sMoveVelX / moveSpeed * VOXEL_VIEW_AHEAD_TILES : 0.0f;
    const float aheadZ = moving ? sMoveVelZ / moveSpeed * VOXEL_VIEW_AHEAD_TILES : 0.0f;
    int32_t holesLeft = 0;

    // Visible frustum chunks: drawn, and built now when they are holes
    VitaPerf_MarkCullStart();
    int32_t candidateChunks = 0;
    for (int32_t cy = maxChunkY; cy >= minChunkY; cy--)
    {
        float chunkZ = (float)(sChunkOriginY + cy * CHUNK_SIZE);
        // Frustum width expands with distance north of camera:
        // Expanded lateral bounds, especially on the left (west) and south (bottom)
        float lateralMarginLeft = (indoor || chunkZ < camTargetZ - VOXEL_VIEW_FAR_ROW_TILES) ? VOXEL_VIEW_FAR_LEFT : VOXEL_VIEW_NEAR_LEFT;
        float lateralMarginRight = (indoor || chunkZ < camTargetZ - VOXEL_VIEW_FAR_ROW_TILES) ? VOXEL_VIEW_FAR_RIGHT : VOXEL_VIEW_NEAR_RIGHT;
        int32_t rowMinTileX = (int32_t)floorf(camTargetX - lateralMarginLeft);
        int32_t rowMaxTileX = (int32_t)ceilf(camTargetX + lateralMarginRight);
        const int32_t baseMinChunkX = FloorDivChunk(rowMinTileX - sChunkOriginX);
        const int32_t baseMaxChunkX = FloorDivChunk(rowMaxTileX - sChunkOriginX);
        int32_t minChunkX = FloorDivChunk(rowMinTileX - extraLeft - sChunkOriginX);
        int32_t maxChunkX = FloorDivChunk(rowMaxTileX + extraRight - sChunkOriginX);
        const bool rowInBase = (cy >= baseMinChunkY && cy <= baseMaxChunkY);

        for (int32_t cx = minChunkX; cx <= maxChunkX; cx++)
        {
            candidateChunks++;
            VoxelChunkSlot *slot = FindChunkSlot(cx, cy);
            const bool inBase = rowInBase && cx >= baseMinChunkX && cx <= baseMaxChunkX;
            const bool inFrustum = inBase && Frustum_ChunkInView(cx, cy, slot);

            if (!inFrustum)
            {
                // Unbuilt chunk outside the view: pre-warm it. Those the view reaches within
                // VOXEL_VIEW_AHEAD_TILES of movement come first and count as holes.
                if ((!slot || !slot->valid) && q.prewarmCount < VOXEL_PREFETCH_MAX)
                {
                    q.prewarmCx[q.prewarmCount] = cx;
                    q.prewarmCy[q.prewarmCount] = cy;
                    q.prewarmSoon[q.prewarmCount] = moving && !indoor
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
            const bool needRebuild = ChunkNeedsRebuild(slot, cx0, cy0, indoor, &urgent, &hash, &hashValid);

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
                if (sSyncBuildingLoads
                    || ChunkBuildFits(q.frameStartMs, q.budgetMs, q.built, urgent))
                {
                    if (!hashValid)
                        hash = ChunkContentHash(cx0, cy0, indoor);
                    if (!slot)
                        slot = GetAvailableChunkSlot(cx, cy);
                    EnsureWorkerIdle();
                    TimedBuildChunk(slot, cx, cy, hash);
                    q.built++;
                }
                else
                {
                    // Left for a later frame; its checkedEpoch stays old, so it is compared again.
                    // Missing or drawn with an outdated atlas: wrong on screen either way.
                    holesLeft++;
                }
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

    BuildQueue_SendToWorker(&q);
    BuildQueue_BuildStale(&q);
    BuildQueue_Prewarm(&q);

    VitaPerf_AddCount(VITA_PERF_C_HOLES, (uint32_t)holesLeft);
    sNeedsFullWarmup = false;
    VitaPerf_MarkCullEnd(candidateChunks, visibleChunkCount);
    f->visibleCount = visibleChunkCount;
}

// Stage 7: entity billboards, their cast shadows and water reflections (field only).
static void Frame_EmitEntities(const VoxelFrame *const f)
{
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
        uint32_t redecodedSlots = VoxelEntities_Emit(&entityBuilder, sSpriteAtlasMorton,
                                                     &sCamera, &shadowBuilder, &reflectionBuilder);
        uint32_t dirtySlots = VoxelEntities_TakeDirtySlots();
        if (sSpriteAtlasNeedsAll)
        {
            // A new texture: every slot the atlas already holds, not only those just decoded.
            dirtySlots = 0xFFFFFFFFu;
            sSpriteAtlasNeedsAll = false;
        }
        if ((redecodedSlots > 0 || dirtySlots != 0) && sSpriteTex != 0)
            WriteSpriteAtlasSlots(sSpriteAtlasMorton, sSpriteAtlasLinear, dirtySlots);

        sEntityGlCount = entityBuilder.count;
        if (sEntityGlCount > 0)
        {
            PackVoxelVertices(sEntityVerts, sEntityGlVerts, sEntityGlCount, !indoor);
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
                sReflectionGlVerts[i].a = 140;
            }
        }
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
static void Frame_BeginPasses(VoxelFrame *const f)
{
    const VoxelMapInstance *const inst0 = f->inst0;
    const float fadeAmount = f->fadeAmount;
    const float *const fadeRgb = f->fadeRgb;
    const int32_t renderW = VitaPlatform_GetRenderWidth();
    const int32_t renderH = VitaPlatform_GetRenderHeight();

    const bool dioramaSetting = (VitaExtras_GetFast(FAST_EXTRA_VOXEL_BLUR) != 0);
    const bool isIndoor = (inst0 != NULL && inst0->indoor);
    const bool dioramaActive = sDioramaFboReady && dioramaSetting && !isIndoor && (fadeAmount < 0.99f);

    if (dioramaActive)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, sDioramaFbo);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 8. OpenGL Setup & Hardware Render Passes
    glViewport(0, 0, renderW, renderH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    Voxel_gluPerspective(sCamera.fov, 960.0f / 544.0f, 1.0f, 62.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Voxel_gluLookAt(sCamera.x, sCamera.y, sCamera.z,
                    sCamera.targetX, sCamera.targetY, sCamera.targetZ,
                    0.0f, 1.0f, 0.0f);

    glDepthMask(GL_TRUE);
    glClearColor(fadeAmount >= 0.99f ? fadeRgb[0] : 0.0f,
                 fadeAmount >= 0.99f ? fadeRgb[1] : 0.0f,
                 fadeAmount >= 0.99f ? fadeRgb[2] : 0.0f, 1.0f);
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

// Pass 2: trees and tall grass (tree atlas), from the chunk VBOs.
static void Pass_Trees(const VoxelFrame *const f)
{
    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    if (sTreeTex)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glBindTexture(GL_TEXTURE_2D, sTreeTex);

        for (int32_t i = 0; i < visibleChunkCount; i++)
        {
            const VoxelChunkSlot *const chunk = visibleChunks[i];
            const uint32_t count = chunk->treeCount;
            if (count == 0) continue;

            glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
            glVertexPointer(3, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, x));
            glTexCoordPointer(2, GL_FLOAT, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, u));
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VoxelGlVertex), (const void *)offsetof(VoxelGlVertex, r));

            ChunkOffsets_Apply(chunk);
            glDrawArrays(GL_TRIANGLES, (GLint)chunk->treeFirstVert, (GLsizei)count);
        }
    }
}

// Pass 3: buildings and indoor furniture, one building page at a time. Returns whether
// alpha testing was left on.
static bool Pass_Buildings(const VoxelFrame *const f)
{
    VoxelChunkSlot *const *const visibleChunks = f->visible;
    const int32_t visibleChunkCount = f->visibleCount;
    int32_t activeBuildingPages[16];
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
            if (!alreadyInList && activeBuildingPageCount < 16)
                activeBuildingPages[activeBuildingPageCount++] = p;
        }
    }

    bool buildingAlphaEnabled = false;
    glDisable(GL_ALPHA_TEST);

    for (int32_t ap = 0; ap < activeBuildingPageCount; ap++)
    {
        const int32_t p = activeBuildingPages[ap];
        if (p < 0 || p >= VOXEL_MAX_BUILDING_PAGES) continue;
        const GLuint bTex = RequestOrLoadBuildingPage(p, sSyncBuildingLoads);
        if (!bTex) continue;

        const bool pageNeedsAlpha = (sBuildingPageHasAlpha[p] == 2u);
        if (pageNeedsAlpha != buildingAlphaEnabled)
        {
            if (pageNeedsAlpha)
            {
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.0f);
            }
            else
            {
                glDisable(GL_ALPHA_TEST);
            }
            buildingAlphaEnabled = pageNeedsAlpha;
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
    return buildingAlphaEnabled;
}

// Pass 4: cast and decal silhouette shadows (Emerald 3DS GL_COMBINE, 0xFF281408u).
static void Pass_CastShadows(void)
{
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

// Pass 5.1: day/night and atmosphere modulation of the whole 3D world, in hardware
// (destination colour blending).
static void Pass_DayNightGrade(const VoxelFrame *const f)
{
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

// Pass 5.2: night street lamps and their light pools (Emerald 3DS VoxelSign_EmitNight).
static void Pass_NightLamps(const VoxelFrame *const f)
{
    const bool indoor = f->indoor;
    const bool inBattle3D = f->inBattle3D;
    if (!indoor && !inBattle3D && VitaGame_IsOutdoor())
    {
        const VitaDayNightLight *dn = VitaDayNight_Get();
        if (dn != NULL && dn->glow > 0.01f)
        {
            UpdateAndDrawNightLamps(dn->glow);
        }
    }
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
    if (!indoor)
    {
        if (fogDensity > 0.005f)
        {
            uint8_t fr = underground ? 228 : 238;
            uint8_t fg = underground ? 236 : 244;
            uint8_t fb = underground ? 250 : 252;
            DrawFogBanks(sFogBanks, fogDensity, fr, fg, fb, seconds);
        }
        if (weather == VOXEL_WEATHER_SAND && weatherAmount > 0.005f)
        {
            DrawFogBanks(sSandBanks, weatherAmount, 224, 190, 136, seconds);
        }

        // ---- PASS 5.6: 3D Sunlit Dust Motes ----
        if (f->light.motes > 0.005f)
        {
            DrawMotes(f->light.motes, seconds);
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
                DrawWeather(wKind, weatherAmount, bolt, seconds);
            }
        }
    }
}

// Pass 5.9: 2D atmosphere: sun rays, lightning, cave gloom, and moonlight merged with the
// screen fade. Weather and day/night tint and haze are already baked per vertex, so a clear
// day draws no fullscreen quad here.
static void Pass_Atmosphere2D(const VoxelFrame *const f)
{
    const bool indoor = f->indoor;
    const float bolt = f->bolt;
    const bool underground = f->underground;
    const float seconds = f->seconds;
    const float fadeAmount = f->fadeAmount;
    const float *const fadeRgb = f->fadeRgb;
    static const float sUnitQuad[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glViewport(0, 0, VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());
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

    // 1. Diagonal Sun Rays (VOXEL_WEATHER_SUN & clear sunny daylight)
    if (!indoor && f->light.rays > 0.005f)
    {
        DrawRays(f->light.rays, seconds);
    }

    // 2. Lightning Flash (thunderstorm bolt)
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

    // 4. Single Merged Moonlight + Screen Fade Overlay (VitaDayNight_Moonlight merges both in one pass)
    {
        float overlayR = fadeRgb[0];
        float overlayG = fadeRgb[1];
        float overlayB = fadeRgb[2];
        float overlayA = fadeAmount;
        if (!indoor)
        {
            const VitaDayNightLight *const dn = VitaDayNight_Get();
            const VitaDayNightBlend moon = VitaDayNight_Moonlight(dn, fadeAmount, fadeRgb);
            overlayR = moon.rgb[0];
            overlayG = moon.rgb[1];
            overlayB = moon.rgb[2];
            overlayA = moon.amount;
        }
        if (overlayA > 0.005f)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(overlayR, overlayG, overlayB, overlayA);
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
static void Pass_DepthOfField(const VoxelFrame *const f)
{
    const int32_t renderW = f->renderW;
    const int32_t renderH = f->renderH;
    const float uMax = (float)renderW / (float)VOXEL_DIORAMA_FBO_W;
    const float vMax = (float)renderH / (float)VOXEL_DIORAMA_FBO_H;
    const int32_t scratchW = renderW / 2;
    const int32_t scratchH = renderH / 2;
    const float uS = (float)scratchW / (float)VOXEL_SCRATCH_FBO_W;
    const float vS = (float)scratchH / (float)VOXEL_SCRATCH_FBO_H;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

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

    // 1. Downsample: one bilinear tap per half-size texel
    glBindFramebuffer(GL_FRAMEBUFFER, sScratchFbo);
    glViewport(0, 0, scratchW, scratchH);
    glBindTexture(GL_TEXTURE_2D, sDioramaColorTex);
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

    // 2. Wider blur -> sBlurTex
    glBindFramebuffer(GL_FRAMEBUFFER, sBlurFbo);
    glViewport(0, 0, scratchW, scratchH);
    BlurTaps(sScratchColorTex, uS, vS, (float)VOXEL_SCRATCH_FBO_W, (float)VOXEL_SCRATCH_FBO_H, 1.0f, 1.0f);

    // 3. The sharp scene to the screen, then the bands
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, renderW, renderH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBindTexture(GL_TEXTURE_2D, sDioramaColorTex);
    {
        const float baseUvs[8] = { 0.0f, vMax, uMax, vMax, uMax, 0.0f, 0.0f, 0.0f };
        glVertexPointer(2, GL_FLOAT, 0, quad);
        glTexCoordPointer(2, GL_FLOAT, 0, baseUvs);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    const float yTop = VOXEL_DOF_TOP_ROWS / 544.0f;
    const float yBot = 1.0f - VOXEL_DOF_BOTTOM_ROWS / 544.0f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_COLOR_ARRAY);
    // Light blur: fades in over the first 60 % of each band
    glBindTexture(GL_TEXTURE_2D, sScratchColorTex);
    DofBand(yTop, 0.0f, uS, vS, VOXEL_DOF_TOP_ALPHA, 0.0f, 0.6f);
    DofBand(yBot, 1.0f, uS, vS, VOXEL_DOF_BOTTOM_ALPHA, 0.0f, 0.6f);
    // Wider blur: only the outer 40 % of each band, strongest at the screen edge
    glBindTexture(GL_TEXTURE_2D, sBlurTex);
    DofBand(yTop, 0.0f, uS, vS, VOXEL_DOF_TOP_ALPHA, 0.6f, 1.0f);
    DofBand(yBot, 1.0f, uS, vS, VOXEL_DOF_BOTTOM_ALPHA, 0.6f, 1.0f);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

// Pass 6: the GBA's 2D layer over the 3D world (text, menus, dialogue on BG0, the 3D battle
// foreground, HM and Fly animations). Kept as it is: the new menu system will replace it.
static void Pass_Overlay2D(const VoxelFrame *const f)
{
    const bool inBattle3D = f->inBattle3D;
    const float fadeAmount = f->fadeAmount;
    const bool inFieldMove = FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON)
                          || FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    const bool inFlyAnim = FieldEffectActiveListContains(FLDEFF_USE_FLY)
                        || FieldEffectActiveListContains(FLDEFF_FLY_IN);
    // Battle transition on the field: drawn 16:9, stretched as the field-move banners
    extern bool8 BattleTransition_IsActiveForUi(void);
    const bool inTransition = !inBattle3D && BattleTransition_IsActiveForUi();

    if (inBattle3D || (REG_DISPCNT & 0x0100) || inFieldMove || inFlyAnim || inTransition)
    {
        // In 3D battles, draw smooth 32-level radial ovals beneath battling Pokémon using sLampPoolTex (16x16 RGBA5551)
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
            extern bool8 Shop_IsBuyMenuActive(void);
            const bool inBuyMenu = Shop_IsBuyMenuActive();
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
                    sUiTexIdx ^= 1;
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

// Render one frame of the 3D voxel world
bool VoxelRenderer_RenderFrame(void)
{
    VoxelFrame f;
    if (!Frame_Begin(&f))
        return false;

    Frame_FollowCamera(&f);
    Frame_UpdateAtlas(&f);
    Frame_AnimateTiles(&f);
    Frame_TrackWorldChanges(&f);
    Frame_ReadAtmosphere(&f);
    Frame_SelectChunks(&f);
    BuildFrameGrade(&f.grade, &f.light, f.indoor);
    Frame_EmitEntities(&f);
    Frame_BeginPasses(&f);

    // Faded out completely (a warp): no 3D geometry. The visible chunks were built above all
    // the same, so the fade-in has nothing left to build.
    if (f.fadeAmount < 0.99f)
    {
        Pass_SetupWorld3D();

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
        Pass_NightLamps(&f);
        Pass_Atmosphere3D(&f);
        Pass_Atmosphere2D(&f);
        VitaPerf_MarkDraw3DEnd(sFrameDrawCalls, sFrameVertCount);
    }

    if (f.dioramaActive)
        Pass_DepthOfField(&f);

    Pass_Overlay2D(&f);

    // New UI over everything (Core 1 list, one draw call)
    Ui_Render(VitaPlatform_GetRenderWidth(), VitaPlatform_GetRenderHeight());

    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    REG_VCOUNT = 161;
    return true;
}

void VoxelRenderer_SwapBuffers(void)
{
    VitaPerf_MarkSwapStart();
#ifdef PLATFORM_VITA
    vglSwapBuffers(GL_FALSE);
#else
    SDL_GL_SwapWindow(sdlWindow);
#endif
    VitaPerf_MarkSwapEnd();
    // Core 0 is done with the voxel world until the next frame: the chunk chosen this frame
    // is computed on Core 2 meanwhile. Not while Core 1 is rebuilding the map layout.
    if (sMapMutationLock == 0)
        ChunkWorker_Launch();
}

#endif // VOXEL_SUPPORTED || PLATFORM_VITA || NATIVE_LINUX
#endif // PLATFORM_SDL2
