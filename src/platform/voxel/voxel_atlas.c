/*
 * Metatile atlas builder. See voxel_atlas.h and NOTICE.md.
 */

/* Before global.h, which redefines abs() as a macro over stdlib's prototype. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "fieldmap.h"
#include "decompress.h"
#include "constants/rgb.h"
#include "port_platform.h"
#include "port_log.h"

#include "3ds_video.h"
#include "voxel_atlas.h"
#include "voxel_building.h"
#include "voxel_grade.h"
#include "voxel_relief.h"

/* Largest 4bpp tileset payload: 512 tiles of 32 bytes, doubled for headroom
 * because a compressed payload only declares its size at run time. */
#define VOXEL_TILE_SCRATCH 0x10000u

#define TILES_PER_TILESET 512u
#define SUBTILE_PX 8u

/* In texels; see VoxelAtlas_SlotUV. */
#define VOXEL_ATLAS_EDGE_INSET (1.0f / 32.0f)

void VoxelAtlas_SlotUV(uint32_t slot, float *u0, float *v0, float *u1, float *v1)
{
    uint32_t page = slot / VOXEL_ATLAS_MAX_SLOTS;
    slot %= VOXEL_ATLAS_MAX_SLOTS;
    float x = (float)(slot % VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT;
    float y = (float)(slot / VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT;

    /*
     * Row 0 in memory is v=1, the same convention the 2D compositor uses.
     * The slot's rows stop a thirty-second of a texel short of its north and
     * south edges. The slots touch, with no gutter, and while the camera
     * scrolls north or south a tile's edge passes through pixel centres now
     * and then: the sample there sits on v0 or v1 exactly, and a rounding
     * error in the interpolation (or, at v0, nearest sampling's choice of
     * the larger side) took a row of the slot above or below - a one-pixel
     * line of another metatile's art right along the tile edge. Inside the
     * slot no sample moves by more than that thirty-second.
     */
    *u0 = page * 2.0f + x / VOXEL_ATLAS_W;
    *u1 = page * 2.0f + (x + VOXEL_ATLAS_SLOT) / VOXEL_ATLAS_W;
    *v0 = 1.0f - (y + VOXEL_ATLAS_EDGE_INSET) / VOXEL_ATLAS_H;
    *v1 = 1.0f - (y + VOXEL_ATLAS_SLOT - VOXEL_ATLAS_EDGE_INSET) / VOXEL_ATLAS_H;
}

void VoxelAtlas_SolidUV(VoxelSolidColor color, float *u0, float *v0, float *u1, float *v1)
{
    VoxelAtlas_SlotUV(VOXEL_SOLID_BASE + (uint32_t)color, u0, v0, u1, v1);
}

/*
 * BGR555, the same encoding the tilesets use, so one conversion path serves
 * both.
 *
 * Brighter than the reference's glColor3f triples, which run from 0.10 to
 * 0.40. Those were picked for a renderer that draws them at full brightness on
 * a monitor; here they sit next to metatile art on a 3DS panel, and 0.20 grey
 * reads as a hole in the room rather than as a television.
 */
static const uint16_t sSolidColors[VOXEL_SOLID_COUNT] = {
    [VOXEL_SOLID_WOOD]      = RGB2(18, 13, 8),  /* table sides  */
    [VOXEL_SOLID_WOOD_DARK] = RGB2(13, 9, 6),   /* table legs   */
    [VOXEL_SOLID_GREY]      = RGB2(14, 14, 15), /* set stand    */
    [VOXEL_SOLID_BLACK]     = RGB2(8, 8, 9),    /* set body     */
    [VOXEL_SOLID_SHEET]     = RGB2(26, 24, 22), /* bedding      */
};

static void FillSolidSlots(uint16_t *dest)
{
    for (uint32_t c = 0; c < VOXEL_SOLID_COUNT; ++c)
    {
        uint32_t slot = VOXEL_SOLID_BASE + c;
        uint32_t baseX = (slot % VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT;
        uint32_t baseY = (slot / VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT;
        uint16_t texel = VoxelGrade_RGBA5551(sSolidColors[c]);

        for (uint32_t y = 0; y < VOXEL_ATLAS_SLOT; ++y)
        {
            uint32_t glY = VOXEL_ATLAS_H - 1 - (baseY + y);
            uint16_t *row = dest + glY * VOXEL_ATLAS_W + baseX;
            for (uint32_t x = 0; x < VOXEL_ATLAS_SLOT; ++x)
                row[x] = texel;
        }
    }
}

struct AtlasSource
{
    const uint8_t *primaryTiles, *secondaryTiles;
    const uint16_t *primaryMetatiles, *secondaryMetatiles;
    const uint16_t *primaryPalettes, *secondaryPalettes;
    uint32_t primaryCount, secondaryCount;
    /* Every colour a metatile entry can name, already converted to the
     * texture format, and whether it is black in the source. Index 0 of each
     * palette is the opaque backdrop layer 0 draws with. */
    uint16_t texel[16][16];
    bool black[16][16];
};

static uint16_t LookupColor(const struct AtlasSource *src, uint32_t paletteId, uint32_t colorIdx)
{
    const uint16_t *palette = paletteId < NUM_PALS_IN_PRIMARY
                            ? src->primaryPalettes : src->secondaryPalettes;
    if (palette == NULL)
        return 0;
    return palette[paletteId * 16 + colorIdx];
}

static void BuildColorTable(struct AtlasSource *src)
{
    uint16_t backdrop = src->primaryPalettes != NULL ? src->primaryPalettes[0] : 0;

    for (uint32_t p = 0; p < 16; ++p)
        for (uint32_t c = 0; c < 16; ++c)
        {
            uint16_t bgr15 = c == 0 ? backdrop : LookupColor(src, p, c);

            src->texel[p][c] = VoxelGrade_RGBA5551(bgr15);
            src->black[p][c] = (bgr15 & 0x7FFF) == 0;
        }
}


/* The metatile's entry table, or NULL if this id has no composable data. */
static const uint16_t *MetatileEntries(const struct AtlasSource *src, uint32_t m)
{
    if (m < NUM_METATILES_IN_PRIMARY)
    {
        if (src->primaryMetatiles == NULL || m >= src->primaryCount)
            return NULL;
        return src->primaryMetatiles + m * NUM_TILES_PER_METATILE;
    }
    else
    {
        uint32_t local = m - NUM_METATILES_IN_PRIMARY;

        if (src->secondaryMetatiles == NULL || local >= src->secondaryCount)
            return NULL;
        return src->secondaryMetatiles + local * NUM_TILES_PER_METATILE;
    }
}

/*
 * What an atlas id is drawn from: a real metatile, whole, or a ground variant
 * (voxel_atlas.h) - the metatile of the variant's tileset less the quarters
 * of its upper layer a model stands for. buildings.bin names a variant's
 * tileset by a layout that draws it. False for a variant of another pair.
 */
static const struct Tileset *sVariantTileset[VOXEL_VARIANTS];
static bool sVariantsResolved;
static const struct Tileset *sCutTileset[VOXEL_CUTS];
static bool sCutsResolved;

static const struct Tileset *TilesetOf(uint32_t layoutId, uint32_t metatile)
{
    const struct MapLayout *layout = Port_GetMapLayoutById((u16)layoutId);

    if (layout == NULL)
        return NULL;
    return metatile < NUM_METATILES_IN_PRIMARY ? layout->primaryTileset : layout->secondaryTileset;
}

/* A cut tile (voxel_relief.h): its metatile and the rows of its background. */
static bool CutSourceOf(const struct Tileset *primary, const struct Tileset *secondary,
                        uint32_t i, uint32_t *metatile, const uint8_t **cut)
{
    uint32_t layoutId;

    if (!sCutsResolved && VoxelRelief_CutCount() > 0)
    {
        for (uint32_t k = 0; k < VOXEL_CUTS; ++k)
        {
            uint32_t m;
            const uint8_t *rows;

            sCutTileset[k] = VoxelRelief_CutVariant(k, &layoutId, &m, &rows)
                           ? TilesetOf(layoutId, m) : NULL;
        }
        sCutsResolved = true;
    }
    if (i >= VOXEL_CUTS || !VoxelRelief_CutVariant(i, &layoutId, metatile, cut))
        return false;
    return sCutTileset[i] != NULL
        && sCutTileset[i] == (*metatile < NUM_METATILES_IN_PRIMARY ? primary : secondary);
}

static bool AtlasSourceOf(const struct Tileset *primary, const struct Tileset *secondary,
                          uint32_t id, uint32_t *metatile, uint32_t *hidden, const uint8_t **cut)
{
    uint32_t layoutId, quarters, i = id - VOXEL_METATILE_REAL;

    *cut = NULL;
    if (id < VOXEL_METATILE_REAL)
    {
        *metatile = id;
        *hidden = 0;
        return true;
    }
    if (id >= VOXEL_CUT_FIRST)
    {
        *hidden = 0;
        return CutSourceOf(primary, secondary, id - VOXEL_CUT_FIRST, metatile, cut);
    }
    if (!sVariantsResolved)
    {
        for (uint32_t k = 0; k < VOXEL_VARIANTS; ++k)
        {
            uint32_t m;
            const struct MapLayout *layout;

            sVariantTileset[k] = NULL;
            if (!VoxelBuildings_Variant(k, &layoutId, &m, &quarters))
                continue;
            layout = Port_GetMapLayoutById((u16)layoutId);
            if (layout != NULL)
                sVariantTileset[k] = m < NUM_METATILES_IN_PRIMARY
                                   ? layout->primaryTileset : layout->secondaryTileset;
        }
        /* only once the file is read: an atlas composed before it would
         * otherwise leave every variant unresolved for good */
        sVariantsResolved = VoxelBuildings_Variant(0, &layoutId, &i, &quarters);
        i = id - VOXEL_METATILE_REAL;
    }
    if (i >= VOXEL_VARIANTS || !VoxelBuildings_Variant(i, &layoutId, metatile, hidden))
        return false;
    return sVariantTileset[i] != NULL
        && sVariantTileset[i] == (*metatile < NUM_METATILES_IN_PRIMARY ? primary : secondary);
}

/*
 * One tile of a metatile entry over its 8x8 block, in the PICA's order. Layer
 * 0 is the opaque base; layer 1 keys out index 0 and, `hide`, draws nothing.
 * `*black` turns false on anything drawn that is not black, and on a tile
 * that is missing.
 */
static void ComposeTile(const struct AtlasSource *src, uint16_t entry, uint32_t layer, bool hide,
                        uint16_t *dest, uint32_t quadX, uint32_t quadY, uint32_t width,
                        uint32_t height, bool *black)
{
    uint32_t tileId = entry & 0x3FF;
    uint32_t paletteId = (entry >> 12) & 0xF;
    bool flipX = (entry >> 10) & 1;
    uint32_t flipY = (entry >> 11) & 1 ? SUBTILE_PX - 1 : 0;
    const uint8_t *tile;
    uint16_t texels[16];
    uint32_t used = 0;

    if (tileId < TILES_PER_TILESET)
        tile = src->primaryTiles != NULL ? src->primaryTiles + tileId * 32 : NULL;
    else
        tile = src->secondaryTiles != NULL
             ? src->secondaryTiles + (tileId - TILES_PER_TILESET) * 32 : NULL;
    if (tile == NULL)
    {
        *black = false;
        return;
    }
    if (hide)
        return;
    memcpy(texels, src->texel[paletteId], sizeof(texels));
    /* Index 0 of layer 0 is the backdrop, whatever palette. */
    texels[0] = src->texel[0][0];
    /*
     * A row's eight pixels are written directly to OpenGL linear row-major layout,
     * where row y is mapped to glY = height - 1 - y.
     */
    for (uint32_t py = 0; py < SUBTILE_PX; ++py)
    {
        const uint8_t *row = tile + (py ^ flipY) * 4;
        uint32_t glY = height - 1 - (quadY + py);
        uint16_t *dstRow = dest + glY * width + quadX;
        uint32_t bits = (uint32_t)row[0] | (uint32_t)row[1] << 8 | (uint32_t)row[2] << 16
                      | (uint32_t)row[3] << 24;

        if (flipX)
        {
            /* the nibbles of each byte swapped, then the bytes reversed */
            bits = (bits & 0x0F0F0F0Fu) << 4 | (bits >> 4 & 0x0F0F0F0Fu);
            bits = bits >> 24 | (bits >> 8 & 0xFF00u) | (bits << 8 & 0xFF0000u) | bits << 24;
        }
        if (layer == 0)
        {
            for (uint32_t px = 0; px < SUBTILE_PX; ++px, bits >>= 4)
            {
                uint32_t colorIdx = bits & 0xF;

                used |= 1u << colorIdx;
                dstRow[px] = texels[colorIdx];
            }
        }
        else
        {
            for (uint32_t px = 0; bits != 0; ++px, bits >>= 4)
            {
                uint32_t colorIdx = bits & 0xF;

                if (colorIdx == 0)
                    continue;
                used |= 1u << colorIdx;
                dstRow[px] = texels[colorIdx];
            }
        }
    }
    /* black only if every colour it drew is */
    for (uint32_t c = 0; used != 0 && *black; ++c, used >>= 1)
        if ((used & 1u) && !(c == 0 ? src->black[0][0] : src->black[paletteId][c]))
            *black = false;
}

/* A quadrant of a metatile: both its layers over one 8x8 block. */
static bool ComposeQuadrant(const struct AtlasSource *src, uint16_t lower, uint16_t upper,
                            bool hideUpper, uint16_t *dest, uint32_t quadX, uint32_t quadY,
                            uint32_t width, uint32_t height)
{
    bool black = true;

    ComposeTile(src, lower, 0, false, dest, quadX, quadY, width, height, &black);
    ComposeTile(src, upper, 1, hideUpper, dest, quadX, quadY, width, height, &black);
    return black;
}

/*
 * What a cut tile's background becomes (VoxelRelief_CutClear, _CutFringe):
 * the pixels that are clear, and those kept for the polygon's edge with the
 * colour of the rock pixel beside them. It follows from the cut's rows alone
 * and takes far longer to work out than the tile takes to compose - a pass
 * of the animated sea round Lilycove's rocks spent 14 ms on it (Azahar's
 * count) - so it is kept for the cuts met last.
 */
#define CUT_PLAN_SETS 128u
struct CutPlan
{
    uint8_t rows[32];       /* the cut's, which name it */
    uint32_t stamp;
    uint16_t clear[16];     /* bit x of row y: clear */
    uint16_t fringe[16];    /* bit x of row y: the colour of the rock pixel beside */
    uint8_t from[128];      /* which one: kCutAround[nibble 16y + x] */
};
/* VoxelRelief_CutFringe's neighbours, in its order */
static const int8_t kCutAround[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
static struct CutPlan sCutPlans[CUT_PLAN_SETS][2];
static uint32_t sCutPlanStamp;

static const struct CutPlan *CutPlanOf(const uint8_t *cut)
{
    uint32_t hash = 0;
    struct CutPlan *set, *plan;

    for (uint32_t i = 0; i < 32; ++i)
        hash = hash * 31u + cut[i];
    set = sCutPlans[hash % CUT_PLAN_SETS];
    for (uint32_t way = 0; way < 2; ++way)
        if (set[way].stamp != 0 && memcmp(set[way].rows, cut, sizeof(set[way].rows)) == 0)
        {
            set[way].stamp = ++sCutPlanStamp;
            return &set[way];
        }
    plan = set[0].stamp <= set[1].stamp ? &set[0] : &set[1];
    memcpy(plan->rows, cut, sizeof(plan->rows));
    plan->stamp = ++sCutPlanStamp;
    VoxelRelief_CutClear(cut, plan->clear);
    memset(plan->fringe, 0, sizeof(plan->fringe));
    memset(plan->from, 0, sizeof(plan->from));
    for (uint32_t y = 0; y < 16; ++y)
        for (uint32_t x = 0; x < 16; ++x)
        {
            int32_t from = VoxelRelief_CutFringe(cut, plan->clear, (int32_t)x, (int32_t)y);

            if (from >= 0 && !((plan->clear[y] >> x) & 1u))
            {
                uint32_t k = 0;

                while (k < 7 && (int32_t)((y + kCutAround[k][1]) * 16 + x + kCutAround[k][0]) != from)
                    ++k;
                plan->fringe[y] |= (uint16_t)(1u << x);
                plan->from[(y * 16 + x) / 2] |= (uint8_t)(k << ((x & 1) * 4));
            }
        }
    return plan;
}

/*
 * Composes one metatile straight into its packed slot of the destination,
 * less the upper-layer quarters in `hidden`. Returns whether everything it
 * drew is black - what an interior is filled with outside its rooms (see
 * sVoid below). A quadrant whose tiles are missing draws nothing and does not
 * count as black.
 */
static bool ComposeAt(const struct AtlasSource *src, const uint16_t *entries,
                      uint16_t *dest, uint32_t baseX, uint32_t baseY, uint32_t width,
                      uint32_t hidden, const uint8_t *cut)
{
    bool black = true;
    const uint32_t height = (width == VOXEL_ATLAS_W) ? VOXEL_ATLAS_H : width;

    for (uint32_t quadrant = 0; quadrant < 4; ++quadrant)
    {
        uint32_t qx = baseX + (quadrant % 2) * SUBTILE_PX;
        uint32_t qy = baseY + (quadrant / 2) * SUBTILE_PX;
        bool dark = ComposeQuadrant(src, entries[quadrant], entries[4 + quadrant],
                                    (hidden >> quadrant) & 1u, dest, qx, qy, width, height);

        black = black && dark;
    }
    /* a cut tile's background is clear: the alpha test leaves the ground
     * drawn flat under it to show (less what its straight diagonals take
     * in: VoxelRelief_CutClear) */
    if (cut != NULL)
    {
        const struct CutPlan *plan = CutPlanOf(cut);

#define LINEAR_TEXEL(x, y) (((height - 1 - (baseY + (y))) * width) + (baseX + (x)))
        for (uint32_t y = 0; y < VOXEL_ATLAS_SLOT && y < 16; ++y)
        {
            for (uint32_t bits = plan->clear[y]; bits != 0; bits &= bits - 1u)
            {
                uint32_t x = (uint32_t)__builtin_ctz(bits);

                dest[LINEAR_TEXEL(x, y)] &= (uint16_t)~1u;
            }
            for (uint32_t bits = plan->fringe[y]; bits != 0; bits &= bits - 1u)
            {
                uint32_t x = (uint32_t)__builtin_ctz(bits);
                uint32_t k = (plan->from[(y * 16 + x) / 2] >> ((x & 1) * 4)) & 7u;

                /* kept for the polygon's edge: the rock's colour */
                dest[LINEAR_TEXEL(x, y)] = dest[LINEAR_TEXEL(x + kCutAround[k][0], y + kCutAround[k][1])];
            }
        }
#undef LINEAR_TEXEL
    }
    return black;
}

static bool ComposeMetatile(const struct AtlasSource *src, const uint16_t *entries,
                            uint16_t *dest, uint32_t slot, uint32_t hidden, const uint8_t *cut)
{
    return ComposeAt(src, entries, dest, (slot % VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT,
                     (slot / VOXEL_ATLAS_COLUMNS) * VOXEL_ATLAS_SLOT, VOXEL_ATLAS_W, hidden, cut);
}

/*
 * The metatiles that draw nothing but black: what an interior is filled with
 * outside its rooms. Which id that is depends on the tileset pair - 0x001 in
 * the Building tileset, 0x201 in a house's, 0x208 in a gym's, 0x245 in Mirage
 * Tower - so it is read off the drawing when the pair's atlas is composed,
 * and remembered for the pairs most recently used - twice as many as there
 * are atlases.
 */
#define VOID_PAIRS 8
static struct
{
    const struct Tileset *primary, *secondary;
    uint8_t black[VOXEL_METATILE_IDS / 8];
    uint32_t stamp;
} sVoid[VOID_PAIRS];
static uint32_t sVoidStamp;

static uint8_t *VoidBits(const struct Tileset *primary, const struct Tileset *secondary,
                         bool claim)
{
    uint32_t oldest = 0;

    for (uint32_t i = 0; i < VOID_PAIRS; ++i)
    {
        if (sVoid[i].primary == primary && sVoid[i].secondary == secondary && primary != NULL)
        {
            /* Used, so kept: an interior whose atlas is still resident must
             * not lose its filler to pairs composed after it. */
            sVoid[i].stamp = ++sVoidStamp;
            return sVoid[i].black;
        }
        if (sVoid[i].stamp < sVoid[oldest].stamp)
            oldest = i;
    }
    if (!claim)
        return NULL;
    memset(&sVoid[oldest], 0, sizeof(sVoid[oldest]));
    sVoid[oldest].primary = primary;
    sVoid[oldest].secondary = secondary;
    sVoid[oldest].stamp = ++sVoidStamp;
    return sVoid[oldest].black;
}

bool VoxelAtlas_IsVoid(const VoxelMapInstance *inst, int32_t metatile)
{
    const uint8_t *bits;

    if (inst == NULL || metatile < 0 || metatile >= (int32_t)VOXEL_METATILE_IDS)
        return false;
    bits = VoidBits(inst->primaryTileset, inst->secondaryTileset, false);
    return bits != NULL && (bits[metatile >> 3] & (1u << (metatile & 7))) != 0;
}

/*
 * The composition, a piece at a time. A town's atlas is a few hundred
 * metatiles and two decompressed tilesets: 70-130 ms on an Old 3DS, which in
 * one go was the stall of every first sight of a new map. As a job it is
 * spread over frames against their budget - each tileset's tiles a step, then
 * a handful of metatiles a step - and composed into a map of its own, which
 * the caller swaps in with the upload so no chunk ever sees a half-built one.
 */
enum { ATLAS_LOAD_PRIMARY, ATLAS_LOAD_SECONDARY, ATLAS_COMPOSE, ATLAS_DONE };

static void JobSource(const VoxelAtlasJob *job, struct AtlasSource *src)
{
    /* Re-resolved every step: a payload the asset cache retires between
     * frames comes back on its next resolution. */
    src->primaryTiles = job->primaryLoaded ? job->primaryTiles : NULL;
    src->secondaryTiles = job->secondaryLoaded ? job->secondaryTiles : NULL;
    src->primaryMetatiles = Voxel_ResolveMetatiles(job->primary);
    src->secondaryMetatiles = Voxel_ResolveMetatiles(job->secondary);
    src->primaryPalettes = Voxel_ResolvePalettes(job->primary);
    src->secondaryPalettes = Voxel_ResolvePalettes(job->secondary);
    src->primaryCount = Voxel_MetatileCount(job->primary, NUM_METATILES_IN_PRIMARY);
    src->secondaryCount = Voxel_MetatileCount(job->secondary,
                                              NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY);
}

void VoxelAtlas_JobCancel(VoxelAtlasJob *job)
{
    free(job->primaryTiles);
    free(job->secondaryTiles);
    job->primaryTiles = job->secondaryTiles = NULL;
    job->active = false;
}

bool VoxelAtlas_JobBegin(VoxelAtlasJob *job, const VoxelMapInstance *inst, uint16_t *dest,
                         const VoxelAtlasMap *base)
{
    memset(job, 0, sizeof(*job));
    if (inst == NULL || dest == NULL)
        return false;
    job->primary = inst->primaryTileset;
    job->secondary = inst->secondaryTileset;
    job->mapGroup = inst->mapGroup;
    job->mapNum = inst->mapNum;
    job->dest = dest;
    job->extend = base != NULL;
    if (base != NULL)
        job->map = *base;
    /* Only the ids the maps on this tileset pair actually reference, as they
     * stand now: the maps on screen may change before the job ends, and the
     * next extension catches up with anything they add. */
    VoxelWorld_MarkUsedMetatiles(job->primary, job->secondary, job->used);
    /* and the pair's ground variants, a few slots: composed whether or not
     * a model on screen stands on them yet */
    for (uint32_t id = VOXEL_METATILE_REAL; id < VOXEL_METATILE_IDS; ++id)
    {
        uint32_t m, hidden;
        const uint8_t *cut;

        if (AtlasSourceOf(job->primary, job->secondary, id, &m, &hidden, &cut) && m < VOXEL_METATILE_REAL && job->used[m])
            job->used[id] = job->used[m];
    }
    /* An older in-flight chunk can request an extension after all of its
     * materials were already published. Do not decode both tilesets and
     * recompose/upload every existing page just to publish the same map.
     * Match the compose pass's retry rules, including derived absent ids. */
    if (base != NULL)
    {
        bool needed = false;
        for (uint32_t id = 0; id < VOXEL_METATILE_IDS; ++id)
        {
            uint32_t slot = base->slotOf[id];
            if (job->used[id] && (slot == 0 || slot == VOXEL_SLOT_PENDING
             || (slot == VOXEL_SLOT_ABSENT && id >= VOXEL_METATILE_REAL)))
            {
                needed = true;
                break;
            }
        }
        if (!needed)
        {
            job->ok = true;
            return false;
        }
    }
    job->primaryTiles = malloc(VOXEL_TILE_SCRATCH);
    job->secondaryTiles = malloc(VOXEL_TILE_SCRATCH);
    if (job->primaryTiles == NULL || job->secondaryTiles == NULL)
    {
        VoxelAtlas_JobCancel(job);
        PORT_LOG("[ERROR] VOXEL atlas: no heap for tileset scratch\n");
        return false;
    }
    job->active = true;
    memset(dest, 0, VOXEL_ATLAS_PIXELS * sizeof(uint16_t));
    FillSolidSlots(dest);
    job->priority = 2;
    job->phase = ATLAS_LOAD_PRIMARY;
    return true;
}

bool VoxelAtlas_JobStep(VoxelAtlasJob *job, uint32_t metatiles)
{
    struct AtlasSource src;
    uint8_t *voidBits;

    if (!job->active || job->pageReady)
        return true;
    switch (job->phase)
    {
    case ATLAS_LOAD_PRIMARY:
    case ATLAS_LOAD_SECONDARY:
    {
        bool primary = job->phase == ATLAS_LOAD_PRIMARY;
        uint8_t *tiles = primary ? job->primaryTiles : job->secondaryTiles;
        const struct Tileset *ts = primary ? job->primary : job->secondary;
        memset(tiles, 0, VOXEL_TILE_SCRATCH);
        if (ts && ts->tiles)
        {
            if (ts->isCompressed)
                LZDecompressWram(ts->tiles, tiles);
            else
                memcpy(tiles, ts->tiles, 16384);
            if (primary) job->primaryLoaded = true;
            else job->secondaryLoaded = true;
        }
        job->phase = primary ? ATLAS_LOAD_SECONDARY : ATLAS_COMPOSE;
        return false;
    }
    case ATLAS_COMPOSE:
        break;
    default:
        return true;
    }

    memset(&src, 0, sizeof(src));
    JobSource(job, &src);
    BuildColorTable(&src);
    voidBits = VoidBits(job->primary, job->secondary, true);

    for (; job->next < VOXEL_METATILE_IDS && metatiles > 0; ++job->next)
    {
        VoxelAtlasMap *map = &job->map;
        uint32_t m = job->next, base, hidden, slot = map->slotOf[m];
        const uint16_t *entries;
        const uint8_t *cut;

        --metatiles; /* Bound lookups too, including unused and absent ids. */
        if (slot == VOXEL_SLOT_ABSENT && m >= VOXEL_METATILE_REAL && job->used[m])
            slot = map->slotOf[m] = 0;
        if (slot == VOXEL_SLOT_ABSENT)
            continue;
        if (job->priority == 2 ? job->used[m] != 2 : job->used[m] == 2)
            continue;
        if (!slot && !job->used[m])
            continue;
        entries = AtlasSourceOf(job->primary, job->secondary, m, &base, &hidden, &cut)
                ? MetatileEntries(&src, base) : NULL;
        if (entries == NULL)
        {
            map->slotOf[m] = VOXEL_SLOT_ABSENT;
            continue;
        }
        if (!slot || slot == VOXEL_SLOT_PENDING)
        {
            if (map->used >= VOXEL_ATLAS_PAGES * VOXEL_SOLID_BASE)
            {
                map->overflowed = true;
                map->slotOf[m] = VOXEL_SLOT_PENDING;
                continue;
            }
            slot = (map->used / VOXEL_SOLID_BASE) * VOXEL_ATLAS_MAX_SLOTS
                 + map->used % VOXEL_SOLID_BASE + 1u;
            map->slotOf[m] = (uint16_t)slot;
            ++map->used;
            ++job->added;
        }
        if ((slot - 1u) / VOXEL_ATLAS_MAX_SLOTS != job->page)
            continue;
        bool isAnim = false;
        const struct Tileset *secTileset = (const struct Tileset *)job->secondary;
        const bool secHasAnim = (secTileset != NULL && secTileset->callback != NULL);
        for (uint32_t t = 0; t < NUM_TILES_PER_METATILE; ++t)
        {
            uint32_t tile = entries[t] & 0x3FF;
            map->tileMask[job->page][tile >> 3] |= (uint8_t)(1u << (tile & 7));
            // Primary animated tiles are strictly 432..511; secondary animated tiles are >= 608 (only if secondary anim callback exists)
            if ((tile >= 432 && tile <= 511) || (tile >= 608 && secHasAnim))
                isAnim = true;
        }
        if (isAnim && job->page < VOXEL_ATLAS_PAGES && map->animMetatileCount[job->page] < VOXEL_ATLAS_MAX_ANIM_METATILES)
        {
            bool alreadyInList = false;
            for (uint8_t k = 0; k < map->animMetatileCount[job->page]; k++)
            {
                if (map->animMetatiles[job->page][k] == (uint16_t)m)
                {
                    alreadyInList = true;
                    break;
                }
            }
            if (!alreadyInList)
                map->animMetatiles[job->page][map->animMetatileCount[job->page]++] = (uint16_t)m;
        }
        voidBits[m >> 3] &= (uint8_t)~(1u << (m & 7));
        if (ComposeMetatile(&src, entries, job->dest,
                            (slot - 1u) % VOXEL_ATLAS_MAX_SLOTS, hidden, cut))
            voidBits[m >> 3] |= (uint8_t)(1u << (m & 7));
    }
    if (job->next < VOXEL_METATILE_IDS)
        return false;
    if (job->priority == 2)
    {
        job->priority = 1;
        job->next = 0;
        return false;
    }
    job->pageReady = true;
    job->ok = job->map.used != 0 && !job->map.overflowed;
    if (job->page + 1u >= VoxelAtlas_PageCount(&job->map))
    {
        VoxelAtlas_JobCancel(job);
        job->phase = ATLAS_DONE;
    }
    return true;
}

uint32_t VoxelAtlas_PageCount(const VoxelAtlasMap *map)
{
    return (map->used + VOXEL_SOLID_BASE - 1u) / VOXEL_SOLID_BASE;
}

bool VoxelAtlas_JobNextPage(VoxelAtlasJob *job)
{
    if (!job->pageReady || !job->active)
        return false;
    ++job->page;
    job->priority = 2;
    job->next = 0;
    job->pageReady = false;
    memset(job->dest, 0, VOXEL_ATLAS_PIXELS * sizeof(uint16_t));
    FillSolidSlots(job->dest);
    return true;
}

uint32_t VoxelAtlas_RefreshAnimatedPageRows(const VoxelMapInstance *inst, const VoxelAtlasMap *map,
                                        uint32_t page, uint16_t *dest, const uint8_t *liveTiles,
                                        const uint8_t dirtyTiles[128], uint32_t *rows)
{
    struct AtlasSource src;
    uint8_t *voidBits;
    uint32_t changed = 0;

    if (rows != NULL)
        *rows = 0;
    if (inst == NULL || map == NULL || dest == NULL || liveTiles == NULL || dirtyTiles == NULL)
        return 0;
    memset(&src, 0, sizeof(src));
    /* The game has already applied TransferTilesetAnimsBuffer to logical GBA
     * VRAM. Both tilesets occupy the first 0x8000 bytes in 4bpp order. */
    src.primaryTiles = liveTiles;
    src.secondaryTiles = liveTiles + TILES_PER_TILESET * 32;
    src.primaryMetatiles = Voxel_ResolveMetatiles(inst->primaryTileset);
    src.secondaryMetatiles = Voxel_ResolveMetatiles(inst->secondaryTileset);
    src.primaryPalettes = Voxel_ResolvePalettes(inst->primaryTileset);
    src.secondaryPalettes = Voxel_ResolvePalettes(inst->secondaryTileset);
    src.primaryCount = Voxel_MetatileCount(inst->primaryTileset, NUM_METATILES_IN_PRIMARY);
    src.secondaryCount = Voxel_MetatileCount(inst->secondaryTileset,
                                             NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY);

    /* Pages >= 2 belong to adjacent maps; inst0 has no metatiles on them. */
    if (page >= 2 || page >= VOXEL_ATLAS_PAGES)
        return 0;

    const uint8_t animCount = map->animMetatileCount[page];
    if (animCount == 0)
        return 0;

    BuildColorTable(&src);
    voidBits = VoidBits(inst->primaryTileset, inst->secondaryTileset, true);

    for (uint8_t i = 0; i < animCount; ++i)
    {
        uint32_t m = map->animMetatiles[page][i];
        uint32_t slot = map->slotOf[m], base, hidden;
        const uint8_t *cut;

        if (slot == 0 || slot == VOXEL_SLOT_ABSENT || slot == VOXEL_SLOT_PENDING
         || (slot - 1u) / VOXEL_ATLAS_MAX_SLOTS != page)
            continue;
        if (!AtlasSourceOf(inst->primaryTileset, inst->secondaryTileset, m, &base, &hidden, &cut))
            continue;
        const uint16_t *entries = MetatileEntries(&src, base);
        if (entries == NULL)
            continue;
        bool affected = false;
        for (uint32_t t = 0; t < NUM_TILES_PER_METATILE; ++t)
        {
            uint32_t tile = entries[t] & 0x3FF;
            if (dirtyTiles[tile >> 3] & (1u << (tile & 7)))
            {
                affected = true;
                break;
            }
        }
        if (!affected)
            continue;
        if (voidBits != NULL)
            voidBits[m >> 3] &= (uint8_t)~(1u << (m & 7));
        if (ComposeMetatile(&src, entries, dest, (slot - 1u) % VOXEL_ATLAS_MAX_SLOTS, hidden, cut) && voidBits != NULL)
            voidBits[m >> 3] |= (uint8_t)(1u << (m & 7));
        if (rows != NULL)
            *rows |= 1u << (((slot - 1u) % VOXEL_ATLAS_MAX_SLOTS) / VOXEL_ATLAS_COLUMNS);
        ++changed;
    }
    return changed;
}

uint32_t VoxelAtlas_RefreshAnimatedPage(const VoxelMapInstance *inst, const VoxelAtlasMap *map,
                                    uint32_t page, uint16_t *dest, const uint8_t *liveTiles,
                                    const uint8_t dirtyTiles[128])
{
    return VoxelAtlas_RefreshAnimatedPageRows(inst, map, page, dest, liveTiles, dirtyTiles, NULL);
}

uint32_t VoxelAtlas_RefreshAnimated(const VoxelMapInstance *inst, const VoxelAtlasMap *map,
                                    uint16_t *dest, const uint8_t *liveTiles,
                                    const uint8_t dirtyTiles[128])
{
    return VoxelAtlas_RefreshAnimatedPage(inst, map, 0, dest, liveTiles, dirtyTiles);
}

bool VoxelAtlas_Build(const VoxelMapInstance *inst, uint16_t *dest, VoxelAtlasMap *map,
                      bool extend)
{
    static VoxelAtlasJob job;

    if (!VoxelAtlas_JobBegin(&job, inst, dest, extend ? map : NULL))
    {
        if (!extend)
            memset(map, 0, sizeof(*map));
        return job.ok; /* An unchanged extension succeeds without touching dest. */
    }
    while (!VoxelAtlas_JobStep(&job, VOXEL_METATILE_IDS))
        ;
    *map = job.map;
    VoxelAtlas_JobCancel(&job); /* This convenience API returns page zero only. */
    return job.ok;
}

/* The animation pass (voxel_atlas.h). A block's flags: */
#define ANIM_HIDE 1u    /* its upper layer is hidden (a ground variant) */
#define ANIM_ANY 2u     /* drawn alike in every pair: primary tiles and palettes */
#define ANIM_CUT 4u     /* the first of a cut slot's four, composed whole */
#define ANIM_PART 8u    /* one of the other three: composed with the first */
#define ANIM_BLACK 16u  /* everything it drew is black */

/*
 * The states the pass's blocks have been composed in. The sea and the flowers
 * go round the same eight frames, so once the pass has composed each of them
 * it only finds them again, by the content of the tiles written (the key):
 * on an Old 3DS a pass took up to 19 frames of VOXEL_ANIM_COMPOSE_MS each,
 * the sea composing all the time (port.log 2026-10-28). Kept for the blocks
 * of one pass: a new one (VoxelAnim_Begin) forgets them.
 */
#define ANIM_STATES 8u
static struct
{
    uint32_t key[2];
    uint32_t stamp;
    bool valid;
    uint8_t black[VOXEL_ANIM_BLOCKS / 8u];
} sAnimStates[ANIM_STATES];
static uint16_t (*sAnimStatePixels)[VOXEL_ANIM_BLOCKS][64];
static uint32_t sAnimStateStamp;
/* The tiles a pass composes from (VoxelAnim_Compose): both tilesets, BG VRAM's
 * first 0x8000 bytes, as they were when it began. */
static uint8_t sAnimTiles[2u * TILES_PER_TILESET * 32u] __attribute__((aligned(4)));

static void AnimKey(const VoxelAnimPass *pass, const uint8_t *liveTiles, uint32_t key[2])
{
    uint32_t a = 2166136261u, b = 0x9E3779B9u;

    for (uint32_t i = 0; i < sizeof(pass->work); ++i)
        for (uint32_t bits = pass->work[i]; bits != 0; bits &= bits - 1u)
        {
            uint32_t words[8];

            memcpy(words, liveTiles + (i * 8u + __builtin_ctz(bits)) * 32u, sizeof(words));
            for (uint32_t w = 0; w < 8; ++w)
            {
                a = (a ^ words[w]) * 16777619u;
                b = (b + words[w]) * 2654435761u ^ (b >> 15);
            }
        }
    key[0] = a;
    key[1] = b;
}

void VoxelAnim_Begin(VoxelAnimPass *pass, const uint8_t work[128])
{
    for (uint32_t s = 0; s < ANIM_STATES; ++s)
        sAnimStates[s].valid = false;
    pass->out = pass->pixels;
    pass->state = -1;
    memcpy(pass->work, work, sizeof(pass->work));
    memset(pass->primary, 0, sizeof(pass->primary));
    memset(pass->secondary, 0, sizeof(pass->secondary));
    memset(pass->shared, 0, sizeof(pass->shared));
    memset(pass->hash, 0, sizeof(pass->hash));
    pass->targets = pass->blocks = pass->composed = 0;
    pass->full = false;
}

void VoxelAnim_Again(VoxelAnimPass *pass)
{
    pass->composed = 0;
}

/* Whether an entry is drawn from the pair's own tiles or palettes. */
static bool AnimOwnEntry(uint16_t entry)
{
    return (entry & 0x3FF) >= TILES_PER_TILESET || ((entry >> 12) & 0xF) >= NUM_PALS_IN_PRIMARY;
}

/* The block of a quadrant, found among the pass's or added: its index + 1,
 * 0 when there is no room. */
static uint32_t AnimBlockOf(VoxelAnimPass *pass, uint32_t pair, uint16_t lower, uint16_t upper,
                            bool hide)
{
    uint32_t flags = hide ? ANIM_HIDE : 0;
    uint32_t h;

    if (!AnimOwnEntry(lower) && (hide || !AnimOwnEntry(upper)))
        flags |= ANIM_ANY;
    if (hide)
        upper = 0;
    h = ((uint32_t)lower * 40503u ^ (uint32_t)upper * 2654435761u ^ flags
       ^ ((flags & ANIM_ANY) ? 0u : pair * 97u)) & (VOXEL_ANIM_HASH - 1u);
    for (uint32_t probe = 0; probe < VOXEL_ANIM_HASH; ++probe, h = (h + 1u) & (VOXEL_ANIM_HASH - 1u))
    {
        uint32_t index = pass->hash[h];
        const VoxelAnimBlock *block;

        if (index == 0)
            break;
        block = &pass->block[index - 1u];
        if (block->lower == lower && block->upper == upper
         && (block->flags & (ANIM_HIDE | ANIM_ANY | ANIM_CUT | ANIM_PART)) == flags
         && ((flags & ANIM_ANY) || block->pair == pair))
            return index;
    }
    if (pass->blocks >= VOXEL_ANIM_BLOCKS || pass->hash[h] != 0)
        return 0;
    pass->block[pass->blocks] = (VoxelAnimBlock){lower, upper, (uint8_t)pair, (uint8_t)flags};
    pass->hash[h] = (uint16_t)++pass->blocks;
    return pass->blocks;
}

static int32_t AnimTargetOrder(const void *a, const void *b)
{
    const VoxelAnimTarget *x = a, *y = b;

    return (int32_t)(x->page * VOXEL_ATLAS_MAX_SLOTS + x->slot) - (int32_t)(y->page * VOXEL_ATLAS_MAX_SLOTS + y->slot);
}

/*
 * Whether a cut tile shows any of the pass's tiles. Most of its background
 * is clear and what is kept beside the rock takes the rock's colours: only
 * the pixels with colours of their own are seen (CutPlanOf), and those show
 * the lower layer only where the upper one is clear. The sea round Lilycove's
 * rocks moves under them unseen, and each of its seventy cut tiles took
 * longer to redraw than the rest of the pass.
 */
static bool AnimCutShows(const VoxelAnimPass *pass, const uint16_t *entries, uint32_t hidden,
                         const uint8_t *cut, const uint8_t *liveTiles)
{
    const struct CutPlan *plan = CutPlanOf(cut);

    for (uint32_t q = 0; q < 4; ++q)
    {
        uint32_t lower = entries[q] & 0x3FF, upper = entries[4 + q] & 0x3FF;
        bool hide = (hidden >> q) & 1u;
        bool lowerWritten = (pass->work[lower >> 3] >> (lower & 7)) & 1u;
        bool upperWritten = !hide && ((pass->work[upper >> 3] >> (upper & 7)) & 1u);
        const uint8_t *tile = liveTiles + upper * 32u;
        bool flipX = (entries[4 + q] >> 10) & 1;
        uint32_t flipY = (entries[4 + q] >> 11) & 1 ? 7 : 0;

        if (!lowerWritten && !upperWritten)
            continue;
        for (uint32_t y = 0; y < SUBTILE_PX; ++y)
        {
            uint32_t row = (q / 2u) * SUBTILE_PX + y;
            uint32_t seen = (~(uint32_t)(plan->clear[row] | plan->fringe[row]) >> ((q % 2u) * SUBTILE_PX))
                          & 0xFFu;
            const uint8_t *bytes;
            uint32_t bits;

            if (seen == 0)
                continue;
            if (upperWritten || hide || liveTiles == NULL)
                return true;
            /* the seen pixels a nibble each, as the tile's row holds them */
            seen = (seen | seen << 12) & 0x000F000Fu;
            seen = (seen | seen << 6) & 0x03030303u;
            seen = (seen | seen << 3) & 0x11111111u;
            bytes = tile + (y ^ flipY) * 4;
            bits = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16
                 | (uint32_t)bytes[3] << 24;
            if (flipX)
            {
                bits = (bits & 0x0F0F0F0Fu) << 4 | (bits >> 4 & 0x0F0F0F0Fu);
                bits = bits >> 24 | (bits >> 8 & 0xFF00u) | (bits << 8 & 0xFF0000u) | bits << 24;
            }
            /* a nibble's low bit set where the upper tile draws: the rest
             * of the seen pixels show the lower layer */
            bits |= bits >> 2;
            bits |= bits >> 1;
            if (seen & ~bits & 0x11111111u)
                return true;
        }
    }
    return false;
}

/* Atlas id `m` in `slot` (page*512 + slot + 1) as a target of the pass; false
 * (and the pass full) when there is no room. */
static bool AnimAddTarget(VoxelAnimPass *pass, uint32_t pair, uint32_t m, uint32_t slot,
                          const uint16_t *entries, uint32_t hidden, const uint8_t *cut)
{
    VoxelAnimTarget *target;

    if (pass->targets >= VOXEL_ANIM_TARGETS)
    {
        pass->full = true;
        return false;
    }
    target = &pass->target[pass->targets];
    target->slot = (uint16_t)((slot - 1u) % VOXEL_ATLAS_MAX_SLOTS);
    target->page = (uint8_t)((slot - 1u) / VOXEL_ATLAS_MAX_SLOTS);
    target->id = (uint16_t)m;
    target->pair = (uint8_t)pair;
    if (cut != NULL)
    {
        /* its edge is drawn from across the slot: composed whole, its own */
        if (pass->blocks + 4u > VOXEL_ANIM_BLOCKS)
        {
            pass->full = true;
            return false;
        }
        for (uint32_t q = 0; q < 4; ++q)
        {
            pass->block[pass->blocks] = (VoxelAnimBlock){(uint16_t)m, 0, (uint8_t)pair,
                                                         (uint8_t)(q == 0 ? ANIM_CUT : ANIM_PART)};
            target->block[q] = (uint16_t)pass->blocks++;
        }
    }
    else
        for (uint32_t q = 0; q < 4; ++q)
        {
            uint32_t index = AnimBlockOf(pass, pair, entries[q], entries[4 + q], (hidden >> q) & 1u);

            if (index == 0)
            {
                pass->full = true;
                return false;
            }
            target->block[q] = (uint16_t)(index - 1u);
        }
    ++pass->targets;
    return true;
}

/*
 * The slots between two runs of a row a few apart, redrawn as they are so the
 * two runs and the gap go as one GPU copy. Each copy was a command of the GX
 * queue of its own, with the split of the frame's command list before it: a
 * pass of Mossdeep's sea was some 40 runs and added 4 ms to the GPU's frame
 * on an Old 3DS (port.log 2026-10-28) for 118 KiB. A gap is filled only when
 * every slot in it holds a metatile the pass can draw - not a free slot an
 * extension may fill meanwhile, not one of a map beside drawn with secondary
 * tiles BG VRAM does not hold.
 */
#define ANIM_GAP_MAX 8u
/* the room in a pass gaps leave free */
#define ANIM_GAP_ROOM 256u

static void AnimFillGaps(VoxelAnimPass *pass, uint32_t pair, uint32_t first, const struct AtlasSource *src,
                         const VoxelMapInstance *tilesets, bool shared, const VoxelAtlasMap *map)
{
    static uint16_t idOf[VOXEL_ATLAS_PAGES * VOXEL_ATLAS_MAX_SLOTS];
    uint32_t last = pass->targets, filled = 0;

    memset(idOf, 0xFF, sizeof(idOf));
    for (uint32_t m = 0; m < VOXEL_METATILE_IDS; ++m)
    {
        uint32_t slot = map->slotOf[m];

        if (slot != 0 && slot != VOXEL_SLOT_ABSENT && slot != VOXEL_SLOT_PENDING
         && slot - 1u < VOXEL_ATLAS_PAGES * VOXEL_ATLAS_MAX_SLOTS && idOf[slot - 1u] == 0xFFFFu)
            idOf[slot - 1u] = (uint16_t)m;
    }
    for (uint32_t i = first; i + 1u < last && !pass->full; ++i)
    {
        const VoxelAnimTarget *a = &pass->target[i], *b = &pass->target[i + 1u];
        uint32_t gap = b->slot - a->slot - 1u, k;

        /* never at the cost of the animated slots of the atlases after */
        if (b->page != a->page || b->slot <= a->slot + 1u || gap > ANIM_GAP_MAX
         || a->slot / VOXEL_ATLAS_COLUMNS != b->slot / VOXEL_ATLAS_COLUMNS
         || pass->targets + gap > VOXEL_ANIM_TARGETS - ANIM_GAP_ROOM
         || pass->blocks + 4u * gap > VOXEL_ANIM_BLOCKS - ANIM_GAP_ROOM)
            continue;
        for (k = 1; k <= gap; ++k)
        {
            uint32_t m = idOf[a->page * VOXEL_ATLAS_MAX_SLOTS + a->slot + k], base, hidden;
            const uint8_t *cut;
            const uint16_t *entries;

            if (m == 0xFFFFu
             || !AtlasSourceOf(tilesets->primaryTileset, tilesets->secondaryTileset, m, &base, &hidden, &cut)
             || (entries = MetatileEntries(src, base)) == NULL)
                break;
            if (shared)
            {
                uint32_t t = 0;

                while (t < NUM_TILES_PER_METATILE && (entries[t] & 0x3FF) < TILES_PER_TILESET)
                    ++t;
                if (t < NUM_TILES_PER_METATILE)
                    break;
            }
        }
        if (k <= gap)
            continue;
        for (k = 1; k <= gap; ++k)
        {
            uint32_t slot = a->page * VOXEL_ATLAS_MAX_SLOTS + a->slot + k;
            uint32_t m = idOf[slot], base, hidden;
            const uint8_t *cut;

            AtlasSourceOf(tilesets->primaryTileset, tilesets->secondaryTileset, m, &base, &hidden, &cut);
            AnimAddTarget(pass, pair, m, slot + 1u, MetatileEntries(src, base), hidden, cut);
            ++filled;
        }
    }
    if (filled != 0)
        qsort(pass->target + first, pass->targets - first, sizeof(*pass->target), AnimTargetOrder);
}

bool VoxelAnim_AddAtlas(VoxelAnimPass *pass, uint32_t pair, const VoxelMapInstance *tilesets,
                        bool shared, const VoxelAtlasMap *map, const uint8_t *liveTiles)
{
    struct AtlasSource src;
    uint32_t first = pass->targets;

    if (pair >= VOXEL_ANIM_PAIRS || tilesets == NULL || map == NULL)
        return false;
    pass->primary[pair] = tilesets->primaryTileset;
    pass->secondary[pair] = tilesets->secondaryTileset;
    pass->shared[pair] = shared;
    memset(&src, 0, sizeof(src));
    src.primaryMetatiles = Voxel_ResolveMetatiles(tilesets->primaryTileset);
    src.secondaryMetatiles = Voxel_ResolveMetatiles(tilesets->secondaryTileset);
    src.primaryCount = Voxel_MetatileCount(tilesets->primaryTileset, NUM_METATILES_IN_PRIMARY);
    src.secondaryCount = Voxel_MetatileCount(tilesets->secondaryTileset,
                                             NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY);
    for (uint32_t m = 0; m < VOXEL_METATILE_IDS && !pass->full; ++m)
    {
        const uint16_t *entries;
        bool affected = false;
        uint32_t slot = map->slotOf[m], base, hidden;
        const uint8_t *cut;

        if (slot == 0 || slot == VOXEL_SLOT_ABSENT || slot == VOXEL_SLOT_PENDING)
            continue;
        if (!AtlasSourceOf(tilesets->primaryTileset, tilesets->secondaryTileset, m, &base, &hidden, &cut))
            continue;
        entries = MetatileEntries(&src, base);
        if (entries == NULL)
            continue;
        for (uint32_t t = 0; t < NUM_TILES_PER_METATILE; ++t)
        {
            uint32_t tile = entries[t] & 0x3FF;

            if (shared && tile >= TILES_PER_TILESET)
            {
                /* drawn with secondary tiles BG VRAM does not hold */
                affected = false;
                break;
            }
            if (pass->work[tile >> 3] & (1u << (tile & 7)))
                affected = true;
            if (affected && !shared)
                break;
        }
        if (!affected || (cut != NULL && !AnimCutShows(pass, entries, hidden, cut, liveTiles)))
            continue;
        if (!AnimAddTarget(pass, pair, m, slot, entries, hidden, cut))
            break;
    }
    /* by page and slot, which runs are made of */
    qsort(pass->target + first, pass->targets - first, sizeof(*pass->target), AnimTargetOrder);
    if (!pass->full)
        AnimFillGaps(pass, pair, first, &src, tilesets, shared, map);
    return !pass->full;
}

static void AnimSource(const VoxelAnimPass *pass, uint32_t pair, const uint8_t *liveTiles,
                       struct AtlasSource *src)
{
    memset(src, 0, sizeof(*src));
    /* The game has already applied TransferTilesetAnimsBuffer to logical GBA
     * VRAM. Both tilesets occupy the first 0x8000 bytes in 4bpp order. */
    src->primaryTiles = liveTiles;
    src->secondaryTiles = pass->shared[pair] ? NULL : liveTiles + TILES_PER_TILESET * 32;
    src->primaryMetatiles = Voxel_ResolveMetatiles(pass->primary[pair]);
    src->secondaryMetatiles = Voxel_ResolveMetatiles(pass->secondary[pair]);
    src->primaryPalettes = Voxel_ResolvePalettes(pass->primary[pair]);
    src->secondaryPalettes = Voxel_ResolvePalettes(pass->secondary[pair]);
    src->primaryCount = Voxel_MetatileCount(pass->primary[pair], NUM_METATILES_IN_PRIMARY);
    src->secondaryCount = Voxel_MetatileCount(pass->secondary[pair],
                                              NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY);
    BuildColorTable(src);
}

bool VoxelAnim_Compose(VoxelAnimPass *pass, const uint8_t *liveTiles, bool (*stillTime)(void))
{
    struct AtlasSource src;
    uint32_t srcPair = VOXEL_ANIM_PAIRS;
    bool some = false;

    if (pass->composed >= pass->blocks)
        return true;
    if (liveTiles == NULL)
        return false;
    if (pass->composed == 0)
    {
        uint32_t victim = 0;

        /*
         * Composed from the tiles as they are now, whatever the game writes
         * while it takes: on an Old 3DS a pass took up to 37 frames, past the
         * next write (16), so composed from the live tiles it came out of
         * two states, was never kept and every pass was composed again
         * (port.log 2026-10-28: 15 kept of 601). Taken when it is not kept.
         */
        AnimKey(pass, liveTiles, pass->key);
        pass->out = pass->pixels;
        pass->state = -1;
        pass->kept = false;
        if (sAnimStatePixels == NULL)
            sAnimStatePixels = malloc(ANIM_STATES * sizeof(*sAnimStatePixels));
        for (uint32_t s = 0; s < ANIM_STATES && sAnimStatePixels != NULL; ++s)
        {
            if (sAnimStates[s].valid && sAnimStates[s].key[0] == pass->key[0]
             && sAnimStates[s].key[1] == pass->key[1])
            {
                sAnimStates[s].stamp = ++sAnimStateStamp;
                pass->out = sAnimStatePixels[s];
                for (uint32_t i = 0; i < pass->blocks; ++i)
                    pass->block[i].flags = (uint8_t)((sAnimStates[s].black[i >> 3] >> (i & 7)) & 1u
                                                     ? pass->block[i].flags | ANIM_BLACK
                                                     : pass->block[i].flags & ~ANIM_BLACK);
                pass->composed = pass->blocks;
                pass->kept = true;
                goto composed;
            }
            if (!sAnimStates[s].valid
             || (sAnimStates[victim].valid && sAnimStates[s].stamp < sAnimStates[victim].stamp))
                victim = s;
        }
        memcpy(sAnimTiles, liveTiles, sizeof(sAnimTiles));
        if (sAnimStatePixels != NULL)
        {
            /* composed straight into the state it will be */
            sAnimStates[victim].valid = false;
            pass->state = (int32_t)victim;
            pass->out = sAnimStatePixels[victim];
        }
    }
    for (; pass->composed < pass->blocks; ++pass->composed)
    {
        VoxelAnimBlock *block = &pass->block[pass->composed];
        uint16_t *pixels = pass->out[pass->composed];
        bool black;

        if (block->flags & ANIM_PART)
            continue;
        /* one at least, or a frame short of time would never get on */
        if (some && stillTime != NULL && !stillTime())
            return false;
        some = true;
        if (block->pair != srcPair)
        {
            srcPair = block->pair;
            AnimSource(pass, srcPair, sAnimTiles, &src);
        }
        if (block->flags & ANIM_CUT)
        {
            uint16_t whole[VOXEL_ATLAS_SLOT * VOXEL_ATLAS_SLOT];
            uint32_t base, hidden;
            const uint8_t *cut;
            const uint16_t *entries = NULL;

            memset(whole, 0, sizeof(whole));
            black = false;
            if (AtlasSourceOf(pass->primary[srcPair], pass->secondary[srcPair], block->lower,
                              &base, &hidden, &cut)
             && (entries = MetatileEntries(&src, base)) != NULL)
                black = ComposeAt(&src, entries, whole, 0, 0, VOXEL_ATLAS_SLOT, hidden, cut);
            /* a 16x16 texture is its four blocks, quadrant by quadrant: this
             * one's and the three after it */
            memcpy(pixels, whole, sizeof(whole));
        }
        else
        {
            memset(pixels, 0, sizeof(pass->pixels[0]));
            black = ComposeQuadrant(&src, block->lower, block->upper, (block->flags & ANIM_HIDE) != 0,
                                    pixels, 0, 0, 8, 8);
        }
        block->flags = (uint8_t)(black ? block->flags | ANIM_BLACK : block->flags & ~ANIM_BLACK);
    }
    if (pass->state >= 0)
    {
        uint32_t s = (uint32_t)pass->state;

        memset(sAnimStates[s].black, 0, sizeof(sAnimStates[s].black));
        for (uint32_t i = 0; i < pass->blocks; ++i)
            if (pass->block[i].flags & ANIM_BLACK)
                sAnimStates[s].black[i >> 3] |= (uint8_t)(1u << (i & 7));
        sAnimStates[s].key[0] = pass->key[0];
        sAnimStates[s].key[1] = pass->key[1];
        sAnimStates[s].stamp = ++sAnimStateStamp;
        sAnimStates[s].valid = true;
        pass->state = -1;
    }
composed:
    /* what draws nothing but black, as the atlas composition keeps it */
    for (uint32_t i = 0; i < pass->targets; ++i)
    {
        const VoxelAnimTarget *target = &pass->target[i];
        uint8_t *voidBits = VoidBits(pass->primary[target->pair], pass->secondary[target->pair], true);
        bool black = true;

        if (voidBits == NULL)
            continue;
        if (pass->block[target->block[0]].flags & ANIM_CUT)
            black = (pass->block[target->block[0]].flags & ANIM_BLACK) != 0;
        else
            for (uint32_t q = 0; q < 4; ++q)
                black = black && (pass->block[target->block[q]].flags & ANIM_BLACK) != 0;
        if (black)
            voidBits[target->id >> 3] |= (uint8_t)(1u << (target->id & 7));
        else
            voidBits[target->id >> 3] &= (uint8_t)~(1u << (target->id & 7));
    }
    return true;
}

uint32_t VoxelAnim_Run(const VoxelAnimPass *pass, uint32_t first)
{
    const VoxelAnimTarget *start = &pass->target[first];
    uint32_t count = 1;

    while (first + count < pass->targets && count < VOXEL_ANIM_RUN_MAX)
    {
        const VoxelAnimTarget *next = &pass->target[first + count];

        if (next->pair != start->pair || next->page != start->page || next->slot != start->slot + count
         || next->slot / VOXEL_ATLAS_COLUMNS != start->slot / VOXEL_ATLAS_COLUMNS)
            break;
        ++count;
    }
    return count;
}

void VoxelAnim_WriteRun(const VoxelAnimPass *pass, uint32_t first, uint32_t count, uint16_t *out)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        const VoxelAnimTarget *target = &pass->target[first + i];

        for (uint32_t q = 0; q < 4; ++q)
            memcpy(out + ((q / 2u) * 2u * count + 2u * i + q % 2u) * 64u,
                   pass->out[target->block[q]], sizeof(pass->pixels[0]));
    }
}

uint32_t VoxelAnim_RunOffset(const VoxelAnimPass *pass, uint32_t first)
{
    uint32_t slot = pass->target[first].slot;

    /* block (2c, 2r) of VOXEL_ATLAS_W / 8 to a row, 128 bytes each */
    return (2u * (slot / VOXEL_ATLAS_COLUMNS) * (VOXEL_ATLAS_W / 8u)
          + 2u * (slot % VOXEL_ATLAS_COLUMNS)) * 128u;
}
