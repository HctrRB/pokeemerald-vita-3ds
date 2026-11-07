/*
 * Logical map view for the voxel overworld. Derived from the MIT-licensed
 * voxel_world.c/.h of pokeemerald-multiplatform; see NOTICE.md.
 *
 * This header is deliberately free of both game and libctru types: the module
 * that implements it is a game translation unit (it reads gMapHeader and the
 * rest of the field state), while its only consumer that owns the GPU is
 * ctr_voxel.c, which must not see global.h. Map headers, layouts and tilesets
 * therefore cross the boundary as opaque pointers.
 */
#ifndef CTR_VOXEL_WORLD_H
#define CTR_VOXEL_WORLD_H

#include <stdbool.h>
#include <stdint.h>

/*
 * What a cell is drawn as when nothing modelled covers it. There is no wall,
 * roof, cliff or tree here: terrain is never raised from a guess. What stands
 * up outdoors is modelled from its drawing (buildings, trees, signs) or read
 * off it (relief); everything else is ground.
 */
typedef enum
{
    VOXEL_SHAPE_FLAT = 0,
    VOXEL_SHAPE_DECAL,
    VOXEL_SHAPE_FURNITURE,
    VOXEL_SHAPE_BED,
    VOXEL_SHAPE_TABLE,
    VOXEL_SHAPE_COUNTER,
    VOXEL_SHAPE_SIGN,
    VOXEL_SHAPE_WATER,
    VOXEL_SHAPE_VOID,
    VOXEL_SHAPE_COUNT
} VoxelVisualShape;

#define MAX_VOXEL_MAP_INSTANCES 16
/*
 * Tiles from the current map's edge within which a map is placed in the
 * world however many crossings away it is (VoxelWorld_BuildInstances): the
 * farthest the camera's view reaches past the player, and the chunk of
 * prefetch ring round it (ctr_voxel.c checks it against both).
 */
#define VOXEL_WORLD_REACH 56

typedef struct
{
    const void *header;           /* const struct MapHeader *  */
    const void *layout;           /* const struct MapLayout *  */
    const void *primaryTileset;   /* const struct Tileset *    */
    const void *secondaryTileset; /* const struct Tileset *    */
    int32_t mapGroup, mapNum;
    /* Index into gMapLayouts, the key of every prebuilt per-layout table. */
    int32_t layoutId;
    /* The layout its header names, before any script swapped another in:
     * the other drawing the same map is known by (voxel_building.c). */
    int32_t headerLayoutId;
    /* Read off the live grid (the current map): a script may have changed
     * any of its cells since its layout's tables were built. */
    bool live;
    int32_t originX, originY;
    int32_t width, height;
    bool indoor;
    /* Pixels the map stands over its base here, the way it was reached: what
     * the seams walked from the current map ask (VoxelRelief_SeamStep), so it
     * meets each map it was placed from whatever Hoenn's loops add up to. */
    int32_t lift;
} VoxelMapInstance;

/* Cheap enough to call once per frame: no payload is touched. */
bool VoxelWorld_IsMapAvailable(void);
/* The same during a battle, which keeps the map it was started from. */
bool VoxelWorld_IsBattleMapAvailable(void);

/*
 * Opens a batch of lookups during which resolved payload pointers may be
 * memoised. Resolving one costs a binary search over every asset in the build
 * plus the cache's own bookkeeping, and a mesh rebuild asks for the same
 * tileset's attributes once per tile; within a batch the answer cannot change,
 * because the asset cache only retires payloads between main-loop callbacks
 * and a rebuild runs to completion inside one.
 */
void VoxelWorld_BeginBatch(void);
/*
 * While frozen, lookups only read the batch memos and never add to them: the
 * renderer's chunk worker (Core 2) and other threads may look up at once, and
 * an entry added mid-read could be seen half written. A miss falls back to
 * the asset search, as a full memo does. Set by Core 0 around a worker job.
 */
void VoxelWorld_FreezeMemos(bool frozen);

/* The current map, its connections and theirs, and every map further along
 * within VOXEL_WORLD_REACH of it (see voxel_world.c). */
void VoxelWorld_BuildInstances(void);
/*
 * The asset payloads the renderer reads for the maps on screen and one
 * crossing away - each layout's border, each tileset's attributes, metatiles,
 * palettes and tiles - nearest maps first, for reading them ahead of need.
 * The game only ever loads its current map's tilesets: a connection drawn
 * from others had them read off the card by the renderer, mid-frame.
 */
uint32_t VoxelWorld_NearbyPayloads(const void **payloads, uint32_t max);
/* Includes resolved dynamic object graphics, so a variable-driven NPC change
 * invalidates the prefetch list even while the map set is unchanged. */
uint32_t VoxelWorld_PayloadSignature(void);
uint32_t VoxelWorld_InstanceCount(void);
const VoxelMapInstance *VoxelWorld_Instance(uint32_t index);
const VoxelMapInstance *VoxelWorld_GetInstanceAt(int32_t worldX, int32_t worldY);
bool VoxelWorld_GetInstanceOffset(int32_t mapGroup, int32_t mapNum, int32_t *outOriginX, int32_t *outOriginY);

int32_t VoxelWorld_GetMetatileId(int32_t worldX, int32_t worldY);
/* The cell's collision bits (0: walkable). */
uint32_t VoxelWorld_GetCollision(int32_t worldX, int32_t worldY);
uint32_t VoxelWorld_GetMetatileBehavior(int32_t worldX, int32_t worldY);
/* Reflective behavior whose metatile art actually depicts water or ice. */
bool VoxelWorld_IsVisibleReflectiveSurface(int32_t worldX, int32_t worldY);
/* Where in the cell the GBA shows a priority-3 sprite (the clouds of
 * WEATHER_SUNNY_CLOUDS): a bit per pixel, row 0 the north one, bit 0 the west
 * column. False when the cell's art is not at hand (a neighbour drawn with
 * other tilesets than the ones loaded). */
bool VoxelWorld_UnderGroundMask(int32_t worldX, int32_t worldY, uint16_t rows[16]);
/* Explicit tileset identity: these tree IDs mean other art in other tilesets. */
bool VoxelWorld_UsesTreeSprites(const VoxelMapInstance *inst);

/* The metatile the 2D game tiles outside the map, or -1 if there is none.
 * Without it the view ends in the clear colour past the last map. */
int32_t VoxelWorld_BorderMetatile(int32_t worldX, int32_t worldY);
/* True where that metatile is water: it lies flush with the map's own water,
 * not stood up as the belt (VoxelMesh_EmitBorder). */
bool VoxelWorld_BorderIsWater(int32_t worldX, int32_t worldY);
VoxelVisualShape VoxelWorld_ClassifyTile(int32_t worldX, int32_t worldY);

void VoxelWorld_GetMapDimensions(int32_t *width, int32_t *height);
/* Whether the map joins others at its edges, walked across with no warp. */
bool VoxelWorld_MapIsConnected(void);
/* The edges of the current map (instance 0) where the camera stops, chosen
 * map by map (GetMapClosedEdges, field_camera.c; CTR_FIELD_EDGE_*). */
uint32_t VoxelWorld_ClosedEdges(void);
void VoxelWorld_GetPlayerWorldCoords(float *worldX, float *worldZ);
void VoxelWorld_GetLocation(int32_t *mapGroup, int32_t *mapNum);
/* Small renderer-facing classification of the original field weather state. */
typedef enum {
    VOXEL_WEATHER_CLEAR,
    VOXEL_WEATHER_SUN,
    VOXEL_WEATHER_RAIN,
    VOXEL_WEATHER_FOG,
    VOXEL_WEATHER_PARTICLES,
    VOXEL_WEATHER_SHADE,
    VOXEL_WEATHER_SAND,
    VOXEL_WEATHER_ASH
} VoxelWeatherClass;
VoxelWeatherClass VoxelWorld_Weather(void);
/* How much of the rain, the sand or the ash is in the air, 0-1, following the
 * game's own as it comes and goes: the rain's drops shown of all it can show
 * (a downpour is 1), the sand's and the ash's sprite blend (voxel_world.c). */
float VoxelWorld_WeatherAmount(void);
/* A thunderstorm's bolt, 0-1: the game's flash of the palettes. */
float VoxelWorld_Lightning(void);
/* How thick the game's fog is right now, 0-1: its sprites' blend as it fades
 * in and out, 0 when there are none (voxel_world.c). */
float VoxelWorld_FogDensity(void);
/* A cave, tunnel or other map under the ground (MAP_TYPE_UNDERGROUND). */
bool VoxelWorld_Underground(void);
/* The palette fade on the backgrounds as a blend towards rgb (0-1) by amount,
 * false when there is none (voxel_world.c). */
bool VoxelWorld_ScreenFade(float *amount, float rgb[3]);

/*
 * Content signature of one rectangle of world tiles: what decides whether the
 * geometry built from it is still the geometry that grid would produce.
 *
 * Per rectangle rather than per layout. A signature over the whole live grid
 * answers a question nobody asked - "has anything anywhere changed" - and it
 * changes wholesale when the player crosses a border, because the backup
 * layout is recomposed around the new map. Every chunk on screen was then
 * rebuilt for a crossing that moved them and changed none of them. A chunk
 * that hashes its own 64 tiles rebuilds when its own 64 tiles change, and a
 * crossing costs nothing.
 */
uint32_t VoxelWorld_BlockHash(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/*
 * A layout's own drawing as its tables were built from it, not the live
 * grid: its metatiles row by row (block words, `*width` x `*height`), or
 * NULL. Resolved through the asset layer, so not to be kept past the batch.
 */
const uint16_t *VoxelWorld_LayoutDrawing(int32_t layoutId, int32_t *width, int32_t *height);

/* Changes whenever any block of the live grid does; see voxel_world.c. */
uint32_t VoxelWorld_LiveDigest(void);

/*
 * Payload resolvers. Every one of these pointers is an INCBIN stub that the
 * linker kept but whose bytes live in the game data, so none of them may be
 * dereferenced before going through the asset layer.
 */
const uint16_t *Voxel_ResolveMap(const void *layout);
const uint16_t *Voxel_ResolveMetatiles(const void *tileset);
const uint16_t *Voxel_ResolveAttributes(const void *tileset);
const uint16_t *Voxel_ResolvePalettes(const void *tileset);
/* Number of metatiles the tileset's metatile asset actually contains. */
uint32_t Voxel_MetatileCount(const void *tileset, uint32_t limit);

/*
 * Marks, in a 1024-entry table, every metatile id referenced by the instances
 * that use this tileset pair, borders included. Composing only those is what
 * keeps an atlas rebuild a map-change hitch instead of a whole second of work:
 * a map typically references a couple of hundred of the 1024 ids.
 */
/* NULL restores whole-map collection for offline callers. */
void VoxelWorld_SetMaterialView(const int32_t rect[4], int32_t margin);
void VoxelWorld_MarkUsedMetatiles(const void *primaryTileset, const void *secondaryTileset,
                                  uint8_t *used);
/*
 * Tiles are the one payload that cannot be resolved in place: most tilesets
 * are LZ77-compressed, so the caller supplies the scratch buffer to expand
 * into. Returns false when the tileset has no usable tile data.
 */
/* Re-resolves the payload each step; no asset-cache pointer survives a frame.
 * Zero-initialize before use. A step writes at most `bytes` destination bytes. */
typedef struct {
    uint32_t source, written, size, packedSize, remaining, distance;
    uint8_t flags, bits;
    bool initialized, done, ok;
} VoxelTileLoad;
bool Voxel_LoadTilesStep(const void *tileset, uint8_t *dest, uint32_t destSize,
                         VoxelTileLoad *load, uint32_t bytes);

#endif
