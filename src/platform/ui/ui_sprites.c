// Run-time sprites of the new UI: the original GBA sprites that depend on the species or the item
// (Pokémon icons and front sprites, item icons, footprints), decoded from the game's own tables
// into 64x64 cells of the UI texture (below the fixed atlas, UI_DYN_Y).
//
// Threads (docs/THREADS.md):
// - Core 1 owns the cache (which key is in which cell, when each cell was last drawn). On a miss
//   it decodes the sprite into a staging slot of a single-producer / single-consumer ring and
//   publishes it with a release store, before the draw list that uses it is published.
// - Core 0 (Ui_Render) consumes the ring with the texture bound and writes each staging slot
//   straight into its cell (vglGetTexDataPointer, as the voxel billboards do), then draws.
// A cell is only reused when no list drawn in the last UI_SPR_IDLE_TICKS ticks used it, so the
// GPU never reads a cell while it is rewritten, and vitaGL never copies the texture.

#include <string.h>
#include "ui_internal.h"
#include "global.h"
#include "data.h"
#include "decompress.h"
#include "item_icon.h"
#include "pokemon_icon.h"
#include "constants/species.h"

#ifdef PLATFORM_VITA
#include <vitaGL.h>
#else
#include <GL/gl.h>
#endif

extern const u8 *const gMonFootprintTable[];

#define UI_SPR_RING 16             // staging slots (64x64 RGBA5551 = 8 KB each)
#define UI_SPR_DECODES_PER_TICK 4  // bounds the Core 1 cost of opening a full screen
#define UI_SPR_IDLE_TICKS 16       // a cell is reusable after this many ticks without being drawn

enum { KIND_MON_ICON = 1, KIND_MON_FRONT, KIND_MON_FRONT_SHINY, KIND_ITEM, KIND_FOOTPRINT, KIND_TRAINER };
#define KEY(kind, id) (((uint32_t)(kind) << 16) | (uint32_t)(id))

// ---------- Core 1: cache ----------
static uint32_t sCellKey[UI_CELL_COUNT];       // 0 = free
static uint32_t sCellUsed[UI_CELL_COUNT];      // tick of the last list that drew it
static uint32_t sTick;
static uint32_t sDecodes, sDecodesLast;

// ---------- Ring (Core 1 writes head, Core 0 writes tail) ----------
typedef struct
{
    uint16_t cell;
    uint16_t pixels[UI_CELL * UI_CELL];
} UiSprUpload;

static UiSprUpload sRing[UI_SPR_RING];
static uint32_t sHead, sTail;                  // free-running counters

// GBA BGR555 palette entry -> RGBA5551 (index 0 of a sprite palette is transparent)
static inline uint16_t To5551(uint16_t bgr)
{
    const uint16_t r = bgr & 31, g = (bgr >> 5) & 31, b = (bgr >> 10) & 31;
    return (uint16_t)((r << 11) | (g << 6) | (b << 1) | 1);
}

// 4bpp GBA tiles (row-major, 32 bytes each) -> pixels of a staging cell at (x0, y0)
static void Tiles4bpp(uint16_t *dst, int32_t x0, int32_t y0, const uint8_t *tiles, int32_t tilesW, int32_t tilesH,
                      const uint16_t *bgrPal)
{
    uint16_t pal[16];
    pal[0] = 0;
    for (int32_t i = 1; i < 16; i++)
        pal[i] = To5551(bgrPal[i]);
    for (int32_t ty = 0; ty < tilesH; ty++)
        for (int32_t tx = 0; tx < tilesW; tx++)
        {
            const uint8_t *tile = &tiles[(ty * tilesW + tx) * 32];
            for (int32_t py = 0; py < 8; py++)
                for (int32_t px = 0; px < 8; px += 2)
                {
                    const uint8_t b = tile[py * 4 + px / 2];
                    uint16_t *row = &dst[(y0 + ty * 8 + py) * UI_CELL + x0 + tx * 8 + px];
                    row[0] = pal[b & 15];
                    row[1] = pal[b >> 4];
                }
        }
}

// Decodes a sprite into a staging cell. Runs on Core 1 (the game thread), reading ROM tables.
static bool Decode(uint32_t key, uint16_t *dst)
{
    const uint32_t kind = key >> 16;
    const uint16_t id = (uint16_t)key;
    memset(dst, 0, UI_CELL * UI_CELL * sizeof(uint16_t));
    switch (kind)
    {
    case KIND_MON_ICON:
    {
        // 32x64: frame 0 at the top, frame 1 below (the two-frame party bounce)
        const uint8_t *tiles = gMonIconTable[id];
        const uint16_t *pal = GetValidMonIconPalettePtr(id);
        if (tiles == NULL || pal == NULL)
            return false;
        Tiles4bpp(dst, 0, 0, tiles, 4, 4, pal);
        Tiles4bpp(dst, 0, 32, tiles + 512, 4, 4, pal);
        return true;
    }
    case KIND_MON_FRONT:
    case KIND_MON_FRONT_SHINY:
    {
        static uint8_t tiles[64 * 64 / 2 * 2];
        static uint16_t pal[16 * 16];
        if (gMonStillFrontPicTable[id].data == NULL)
            return false;
        LZ77UnCompWram(gMonStillFrontPicTable[id].data, tiles);
        LZ77UnCompWram((kind == KIND_MON_FRONT_SHINY ? gMonShinyPaletteTable : gMonPaletteTable)[id].data, pal);
        Tiles4bpp(dst, 0, 0, tiles, 8, 8, pal);
        return true;
    }
    case KIND_ITEM:
    {
        static uint8_t tiles[512];
        static uint16_t pal[16];
        const void *pic = GetItemIconPicOrPalette(id, 0), *palGfx = GetItemIconPicOrPalette(id, 1);
        if (pic == NULL || palGfx == NULL)
            return false;
        LZ77UnCompWram(pic, tiles);                 // 3x3 tiles (24x24)
        LZ77UnCompWram(palGfx, pal);
        Tiles4bpp(dst, 0, 0, tiles, 3, 3, pal);
        return true;
    }
    case KIND_TRAINER:
    {
        static uint8_t tiles[64 * 64 / 2];
        static uint16_t pal[16];
        LZ77UnCompWram(gTrainerFrontPicTable[id].data, tiles);
        LZ77UnCompWram(gTrainerFrontPicPaletteTable[id].data, pal);
        Tiles4bpp(dst, 0, 0, tiles, 8, 8, pal);
        return true;
    }
    case KIND_FOOTPRINT:
    {
        // 16x16, 1bpp in 2x2 tiles of 8 bytes (bit 0 = leftmost pixel); white, tinted when drawn
        const uint8_t *fp = gMonFootprintTable[id];
        if (fp == NULL)
            return false;
        for (int32_t t = 0; t < 4; t++)
            for (int32_t py = 0; py < 8; py++)
                for (int32_t px = 0; px < 8; px++)
                    if ((fp[t * 8 + py] >> px) & 1)
                        dst[((t / 2) * 8 + py) * UI_CELL + (t % 2) * 8 + px] = 0xFFFF;
        return true;
    }
    default:
        return false;
    }
}

// Cell holding `key`, decoding it on a miss; -1 when it cannot be shown this tick.
static int32_t Lookup(uint32_t key)
{
    int32_t freeCell = -1, oldest = -1;
    for (int32_t c = 0; c < UI_CELL_COUNT; c++)
    {
        if (sCellKey[c] == key)
        {
            sCellUsed[c] = sTick;
            return c;
        }
        if (sCellKey[c] == 0)
        {
            if (freeCell < 0)
                freeCell = c;
        }
        else if (oldest < 0 || sCellUsed[c] < sCellUsed[oldest])
            oldest = c;
    }
    // A free cell, else the least recently drawn one if no recent list can still be using it
    const int32_t victim = freeCell >= 0 ? freeCell
                         : (oldest >= 0 && sTick - sCellUsed[oldest] >= UI_SPR_IDLE_TICKS) ? oldest : -1;
    if (victim < 0 || sDecodes >= UI_SPR_DECODES_PER_TICK)
        return -1;
    const uint32_t tail = __atomic_load_n(&sTail, __ATOMIC_ACQUIRE);
    if (sHead - tail >= UI_SPR_RING)
        return -1;                                   // Core 0 has not drawn for a while: wait
    UiSprUpload *up = &sRing[sHead % UI_SPR_RING];
    sDecodes++;
    if (!Decode(key, up->pixels))
        return -1;
    up->cell = (uint16_t)victim;
    __atomic_store_n(&sHead, sHead + 1, __ATOMIC_RELEASE);
    sCellKey[victim] = key;
    sCellUsed[victim] = sTick;
    return victim;
}

static bool DrawCell(uint32_t key, int32_t sx, int32_t sy, int32_t sw, int32_t sh, int32_t x, int32_t y, int32_t scale, uint32_t rgba)
{
    const int32_t c = Lookup(key);
    if (c < 0)
        return false;
    if (scale < 1)
        scale = 1;
    const uint32_t u = (uint32_t)(c % UI_CELL_COLS) * UI_CELL + sx, v = UI_DYN_Y + (uint32_t)(c / UI_CELL_COLS) * UI_CELL + sy;
    UiDraw_Quad(x, y, sw * scale, sh * scale, u, v, u + sw, v + sh, rgba);
    return true;
}

bool UiSpr_MonIcon(uint16_t species, uint8_t frame, int32_t x, int32_t y, int32_t scale)
{
    if (species == SPECIES_NONE || species > SPECIES_EGG)
        return false;
    return DrawCell(KEY(KIND_MON_ICON, species), 0, (frame & 1) ? 32 : 0, 32, 32, x, y, scale, 0xFFFFFFFFu);
}

bool UiSpr_MonFront(uint16_t species, bool shiny, int32_t x, int32_t y, int32_t scale)
{
    if (species == SPECIES_NONE || species > SPECIES_EGG)
        return false;
    return DrawCell(KEY(shiny ? KIND_MON_FRONT_SHINY : KIND_MON_FRONT, species), 0, 0, 64, 64, x, y, scale, 0xFFFFFFFFu);
}

bool UiSpr_ItemIcon(uint16_t itemId, int32_t x, int32_t y, int32_t scale)
{
    if (itemId == 0)
        return false;
    return DrawCell(KEY(KIND_ITEM, itemId), 0, 0, 24, 24, x, y, scale, 0xFFFFFFFFu);
}

bool UiSpr_Trainer(uint16_t trainerPic, int32_t x, int32_t y, int32_t scale)
{
    return DrawCell(KEY(KIND_TRAINER, trainerPic), 0, 0, 64, 64, x, y, scale, 0xFFFFFFFFu);
}

// Sprite drawn with a colour multiply (silhouettes, unearned badges)
bool UiSpr_MonFrontTint(uint16_t species, int32_t x, int32_t y, int32_t scale, uint32_t rgba)
{
    if (species == SPECIES_NONE || species > SPECIES_EGG)
        return false;
    return DrawCell(KEY(KIND_MON_FRONT, species), 0, 0, 64, 64, x, y, scale, rgba);
}

bool UiSpr_MonFrontSized(uint16_t species, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (species == SPECIES_NONE || species > SPECIES_EGG)
        return false;
    const int32_t c = Lookup(KEY(KIND_MON_FRONT, species));
    if (c < 0)
        return false;
    const uint32_t u = (uint32_t)(c % UI_CELL_COLS) * UI_CELL, v = UI_DYN_Y + (uint32_t)(c / UI_CELL_COLS) * UI_CELL;
    UiDraw_Quad(x, y, w, h, u, v, u + 64, v + 64, 0xFFFFFFFFu);
    return true;
}

bool UiSpr_TrainerSized(uint16_t pic, int32_t x, int32_t y, int32_t w, int32_t h)
{
    const int32_t c = Lookup(KEY(KIND_TRAINER, pic));
    if (c < 0)
        return false;
    const uint32_t u = (uint32_t)(c % UI_CELL_COLS) * UI_CELL, v = UI_DYN_Y + (uint32_t)(c / UI_CELL_COLS) * UI_CELL;
    UiDraw_Quad(x, y, w, h, u, v, u + 64, v + 64, 0xFFFFFFFFu);
    return true;
}

bool UiSpr_Footprint(uint16_t species, int32_t x, int32_t y, int32_t scale, uint32_t rgba)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return false;
    return DrawCell(KEY(KIND_FOOTPRINT, species), 0, 0, 16, 16, x, y, scale, rgba);
}

void UiSpr_BeginTick(void)
{
    sTick++;
    sDecodesLast = sDecodes;
    sDecodes = 0;
}

uint32_t UiSpr_DecodesLastTick(void)
{
    return sDecodesLast;
}

// ---------- Core 0 ----------
void UiSpr_FlushUploads(void)
{
    const uint32_t head = __atomic_load_n(&sHead, __ATOMIC_ACQUIRE);
    uint32_t tail = sTail;
    if (tail == head)
        return;
#ifdef PLATFORM_VITA
    uint16_t *const texels = (uint16_t *)vglGetTexDataPointer(GL_TEXTURE_2D);
#else
    uint16_t *const texels = NULL;
#endif
    for (; tail != head; tail++)
    {
        const UiSprUpload *up = &sRing[tail % UI_SPR_RING];
        const uint32_t cx = (uint32_t)(up->cell % UI_CELL_COLS) * UI_CELL;
        const uint32_t cy = UI_DYN_Y + (uint32_t)(up->cell / UI_CELL_COLS) * UI_CELL;
        if (texels != NULL)
        {
            for (uint32_t r = 0; r < UI_CELL; r++)
                memcpy(&texels[(cy + r) * UI_TEX_W + cx], &up->pixels[r * UI_CELL], UI_CELL * sizeof(uint16_t));
        }
        else
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)cx, (GLint)cy, UI_CELL, UI_CELL, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, up->pixels);
        }
    }
    __atomic_store_n(&sTail, head, __ATOMIC_RELEASE);
}
