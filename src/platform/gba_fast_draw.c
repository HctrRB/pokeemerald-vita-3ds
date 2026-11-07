#if defined(PLATFORM_VITA) && defined(VITA_PARALLEL_DRAW)
// Before global.h, whose macros clash with the C library headers SDL pulls in (as in sdl2.c).
#include <stdint.h>
#include <SDL2/SDL.h>
#include <arm_neon.h>
#include "vita/vita_platform.h"
#endif
#include "global.h"
#include <stdbool.h>
#include <string.h>
#include "platform/dma.h"
#include "platform/framedraw.h"

#if defined(PLATFORM_VITA) && defined(VITA_PARALLEL_DRAW)
// Parallel drawing. The main thread runs the frame's per-scanline work (VCount/HBlank
// interrupts and HBlank DMAs, which can change registers between lines) and captures the
// registers and palette each line would have been drawn with. Meanwhile two workers on the
// other cores draw alternate lines, each as soon as its capture is ready, and write them
// straight into the locked SDL texture. The renderer reads registers and palette only
// through REG_BASE and PLTT, so both are redirected to the capture of the line being drawn.
// This file is compiled again as gba_fast_draw_b.c and gba_fast_draw_c.c, which gives each
// worker its own copy of the redirect pointers and sprite lists.
#define FAST_DRAW_PARALLEL
#define DRAW_WORKERS 2

// While drawing, the renderer only reads display registers DISPCNT..BLDY (0x00-0x55), so only
// that range is captured per line. Palettes are captured only when they change mid-frame.
#define CAPTURE_REGS_SIZE 0x60

static uint8_t *const sLiveRegs = REG_BASE;
static uint8_t *const sLivePltt = PLTT;
static uint8_t *sLineRegs = REG_BASE;
static uint8_t *sLinePltt = PLTT;
#define REG_BASE sLineRegs
#define PLTT sLinePltt

extern uint8_t gFastDrawRegCaps[DISPLAY_HEIGHT][CAPTURE_REGS_SIZE];
extern uint8_t *gFastDrawPlttCaps[DISPLAY_HEIGHT];
// Number of lines captured so far this frame. Workers start drawing a line as soon as it is
// captured, so capturing and drawing overlap.
extern SDL_atomic_t gFastDrawLinesCaptured;
// Locked SDL texture the finished lines are copied to (NULL: only draw into the pixel buffer).
extern uint16_t *gFastDrawTexture;
extern int32_t gFastDrawTexturePitch;

#ifdef FAST_DRAW_INSTANCE_B
#define FAST_DRAW_SECONDARY
#define spriteSizes spriteSizes_B
#define memsetu16 memsetu16_B
#define frameskipcounter frameskipcounter_B
#define FAST_DRAW_LINES FastDraw_DrawLinesB
#elif defined(FAST_DRAW_INSTANCE_C)
#define FAST_DRAW_SECONDARY
#define spriteSizes spriteSizes_C
#define memsetu16 memsetu16_C
#define frameskipcounter frameskipcounter_C
#define FAST_DRAW_LINES FastDraw_DrawLinesC
#else
#define FAST_DRAW_LINES FastDraw_DrawLinesA
#endif
void FastDraw_DrawLinesA(uint16_t *pixels, int32_t first, int32_t step);
void FastDraw_DrawLinesB(uint16_t *pixels, int32_t first, int32_t step);
void FastDraw_DrawLinesC(uint16_t *pixels, int32_t first, int32_t step);
#endif // PLATFORM_VITA && VITA_PARALLEL_DRAW

#define mosaicBGEffectX (REG_MOSAIC & 0xF)
#define mosaicBGEffectY ((REG_MOSAIC >> 4) & 0xF)
#define mosaicSpriteEffectX ((REG_MOSAIC >> 8) & 0xF)
#define mosaicSpriteEffectY ((REG_MOSAIC >> 12) & 0xF)
#define applyBGHorizontalMosaicEffect(x) (x - (x % (mosaicBGEffectX+1)))
#define applyBGVerticalMosaicEffect(y) (y - (y % (mosaicBGEffectY+1)))
#define applySpriteHorizontalMosaicEffect(x) (x - (x % (mosaicSpriteEffectX+1)))
#define applySpriteVerticalMosaicEffect(y) (y - (y % (mosaicSpriteEffectY+1)))

#define getColorFromPixel(x) (pal[(paletteNum << 4) + (x)])
#define setAlphaBitIfEnabled(x) (((x) != 0) << 15)
#define getbgtilePixel(x) (((x) != 0) ? (getColorFromPixel(x) | setAlphaBitIfEnabled(x)) : 0)
#define get32BitDoublePixel(x, y) getbgtilePixel(x) | (getbgtilePixel(y) << 16)

#define getAlphaBit(x) ((x >> 15) & 1)
#define getRedChannel(x) ((x >>  0) & 0x1F)
#define getGreenChannel(x) ((x >>  5) & 0x1F)
#define getBlueChannel(x) ((x >>  10) & 0x1F)
#define isbgEnabled(x) ((REG_DISPCNT >> 8) & 0xF) & (1 << x)

#define inline_hack

extern void (*const gIntrTable[])(void);

struct scanlineData {
    uint16_t bgcnts[4];
    // Masks of the layer being drawn, indexed like its pixels (see SetupBgLayer).
    uint16_t *winMask;
    uint16_t *bgMask;
    // The same masks at GBA x = 0. With widescreen they also cover x = -24..-1 and 240..263.
    uint16_t *winMask0;
    uint16_t *bgMask0;
    //priority bookkeeping
    char bgtoprio[4]; //background to priority
    char prioritySortedBgs[4][4];
    uint8_t prioritySortedBgsCount[4];
};

// The layer being drawn. A text BG draws sLayerWidth pixels starting at GBA x = sLayerX0, from
// sLayerMap instead of its VRAM tilemap when set: 0/240/NULL normally, -24/288/the field's
// tilemap for the field's map layers in widescreen. Sprites are drawn at x in [sObjXMin, sObjXMax).
static int32_t sLayerX0 = 0;
static int32_t sLayerWidth = DISPLAY_WIDTH;
static uint16_t *sLayerMap;
static int32_t sObjXMin = 0;
static int32_t sObjXMax = DISPLAY_WIDTH;

#ifdef FAST_DRAW_PARALLEL
#define FRAME_EXTEND (gFastDrawWide.extend)
#else
#define FRAME_EXTEND 0
#endif

struct bgPriority {
    char priority;
    char subPriority;
};

static const uint16_t bgMapSizes[][2] =
{
    {32, 32},
    {64, 32},
    {32, 64},
    {64, 64},
};

static uint16_t alphaBlendColor(uint16_t targetA, uint16_t targetB)
{
    // If targetB is the chromaKey backdrop (0x7C1F in BGR555), do NOT blend against it;
    // blending against the chroma key corrupts semi-transparent particles with magenta/pink tint.
    // Return targetA without bit 15 (bit 15 = 0) so Pass 6 identifies it as semi-transparent
    // and preserves hardware alpha blending on the PowerVR SGX543!
    if ((targetB & 0x7FFF) == 0x7C1F)
        return (targetA & 0x7FFF);

    uint32_t eva = REG_BLDALPHA & 0x1F;
    uint32_t evb = (REG_BLDALPHA >> 8) & 0x1F;
    // shift right by 4 = division by 16
    uint32_t r = ((getRedChannel(targetA) * eva) + (getRedChannel(targetB) * evb)) >> 4;
    uint32_t g = ((getGreenChannel(targetA) * eva) + (getGreenChannel(targetB) * evb)) >> 4;
    uint32_t b = ((getBlueChannel(targetA) * eva) + (getBlueChannel(targetB) * evb)) >> 4;
    
    if (r > 31)
        r = 31;
    if (g > 31)
        g = 31;
    if (b > 31)
        b = 31;

     return r | (g << 5) | (b << 10) | (1 << 15);
}

static uint16_t alphaBrightnessIncrease(uint16_t targetA)
{
    // Preserve pure chromaKey backdrop unchanged
    if ((targetA & 0x7FFF) == 0x7C1F)
        return 0x7C1F;

    uint32_t evy = (REG_BLDY & 0x1F);
    uint32_t r = getRedChannel(targetA) + (31 - getRedChannel(targetA)) * evy / 16;
    uint32_t g = getGreenChannel(targetA) + (31 - getGreenChannel(targetA)) * evy / 16;
    uint32_t b = getBlueChannel(targetA) + (31 - getBlueChannel(targetA)) * evy / 16;
    
    if (r > 31)
        r = 31;
    if (g > 31)
        g = 31;
    if (b > 31)
        b = 31;
    
     return r | (g << 5) | (b << 10) | (1 << 15);
}

static uint16_t alphaBrightnessDecrease(uint16_t targetA)
{
    // Preserve pure chromaKey backdrop unchanged
    if ((targetA & 0x7FFF) == 0x7C1F)
        return 0x7C1F;

    uint32_t evy = (REG_BLDY & 0x1F);
    uint32_t r = getRedChannel(targetA) - getRedChannel(targetA) * evy / 16;
    uint32_t g = getGreenChannel(targetA) - getGreenChannel(targetA) * evy / 16;
    uint32_t b = getBlueChannel(targetA) - getBlueChannel(targetA) * evy / 16;
    
    if (r > 31)
        r = 31;
    if (g > 31)
        g = 31;
    if (b > 31)
        b = 31;
    
     return r | (g << 5) | (b << 10) | (1 << 15);
}

#define WINMASK_BG0    (1 << 0)
#define WINMASK_BG1    (1 << 1)
#define WINMASK_BG2    (1 << 2)
#define WINMASK_BG3    (1 << 3)
#define WINMASK_OBJ    (1 << 4)
#define WINMASK_CLR    (1 << 5)
#define WINMASK_WINOUT  (1 << 6)

// Fills [left, right) of the window mask, with GBA style wraparound when left > right.
// Same bounds as the per-pixel check it replaced (inside [left, right), or outside [right, left)).
static void FillWinRange(uint16_t *mask, uint32_t left, uint32_t right, uint16_t value)
{
    uint32_t x;

    if (left > right)
    {
        for (x = left; x < DISPLAY_WIDTH; x++)
            mask[x] = value;
        for (x = 0; x < right && x < DISPLAY_WIDTH; x++)
            mask[x] = value;
    }
    else
    {
        for (x = left; x < right && x < DISPLAY_WIDTH; x++)
            mask[x] = value;
    }
}

// Horizontal BG mosaic over the current layer's line. Mosaic blocks start at GBA x = 0, which
// is pixel -sLayerX0 of the layer (pixel 0 unless the layer is widescreen).
static void ApplyBGHorizontalMosaic(uint16_t *line)
{
    int32_t size = mosaicBGEffectX + 1;

    for (int32_t x = 0; x < sLayerWidth; x++)
    {
        int32_t offset = (x + sLayerX0) % size;
        int32_t src;

        if (offset < 0)
            offset += size;
        src = x - offset;
        line[x] = line[src < 0 ? 0 : src];
    }
}

static void RenderBGScanlineWinBlend(int32_t bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    uint32_t charBaseBlock = (control >> 2) & 3;
    uint32_t screenBaseBlock = (control >> 8) & 0x1F;
    uint32_t bitsPerPixel = 4;//((control >> 7) & 1) ? 8 : 4;
    uint32_t mapWidth = bgMapSizes[control >> 14][0];
    uint32_t mapHeight = bgMapSizes[control >> 14][1];
    uint32_t mapWidthInPixels = mapWidth * 8;
    uint32_t mapHeightInPixels = mapHeight * 8;
    uint16_t *bgmap = sLayerMap != NULL ? sLayerMap : (uint16_t *)BG_SCREEN_ADDR(screenBaseBlock);
    uint16_t* mask = scanline->bgMask;
    uint8_t blendMode = (REG_BLDCNT >> 6) & 3;
    
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    //return;
    
    uint8_t *bgtiles = (uint8_t *)BG_CHAR_ADDR(charBaseBlock);
    uint16_t *pal = (uint16_t *)PLTT;
     
    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    
    uint32_t xx = hoffs & 0x1FF;
    uint32_t yy = (lineNum + voffs) & 0x1FF;
    
    
    if (yy > 255 && mapHeightInPixels > 256) {
        //the width check is for 512x512 mode support, it jumps by two screen bases instead
        bgmap += (mapWidthInPixels > 256) ? 0x800 : 0x400;
    }
    
    xx &= 0x1FF;
    yy &= 0xFF;
    
    uint32_t lineTile = ((lineNum + voffs) & 0xFF) / 8;
    uint32_t firstTile = bgmap[lineTile*32] & 0x3FF;
    uint32_t firstTileData = *(uint32_t*)&bgtiles[(firstTile * 32) + ((lineTile & 7) * bitsPerPixel)];
    if (firstTileData == 0 && mapWidthInPixels > 256)
    {
        for (uint32_t i = 1; i < 32; i++)
        {
            uint32_t tile = bgmap[lineTile*32+i] & 0x3FF;
            if (firstTile != tile)
                goto exitBlankScanlineLoop;
        }
        if (mapWidthInPixels > 256)
        {
            for (uint32_t i = 0; i < 32; i++)
            {
                uint32_t tile = bgmap[lineTile*32+i+0x400] & 0x3FF;
                if (firstTile != tile)
                    goto exitBlankScanlineLoop;
            }   
        }
        return;
    }
    exitBlankScanlineLoop:;
    
    uint32_t startTileX = xx / 8;
    uint32_t startTileY = yy / 8;
    uint32_t startTileYLoc = startTileY * 32;
    uint32_t visibleTilePixels = xx & 7;
    uint32_t tileY = yy & 7;
    uint32_t entry;
    
    if (startTileX > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + (startTileX & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + (startTileX & 31)];
    uint32_t tileNum = entry & 0x3FF;
    uint32_t paletteNum = (entry >> 12) & 0xF;
    uint32_t tileLoc;
    uint32_t pixel;
    uint32_t x = 0;
    //bool shouldBlend = true;
    
    //memset(line, 0x8F00, DISPLAY_WIDTH*2);
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    
    #define writeBgPixel(pixel, x)  \
        if (scanline->winMask[x] & WINMASK_CLR) { \
            switch (blendMode) { \
            case 1: \
                if ((mask[x] & (REG_BLDCNT >> 8))) \
                    line[x] = alphaBlendColor(pal[(paletteNum << 4) + (pixel)], line[x]) | 0x8000; \
                else \
                    line[x] = pal[(paletteNum << 4) + (pixel)] | 0x8000;\
                break; \
            case 2: \
                line[x] = alphaBrightnessIncrease(pal[(paletteNum << 4) + (pixel)]) | 0x8000; \
                break; \
            case 3: \
                line[x] = alphaBrightnessDecrease(pal[(paletteNum << 4) + (pixel)]) | 0x8000; \
                break; } \
        }else{ \
            line[x] = pal[(paletteNum << 4) + (pixel)] | 0x8000; \
        } \
        mask[x] = 1 << bgNum;
    
    #define writeBgPixelWin(pixel, x)   \
        if ( ((pixel)) != 0 ) { \
            if (scanline->winMask[x] & (1 << bgNum)) { \
                writeBgPixel(pixel, x) \
            } \
        }
    
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;

    //draw left most tile
    if (entry & (1 << 10))
    {
        for (int32_t i = visibleTilePixels; i < 8; i++)
        {
            pixel = bgtiles[tileLoc + (3-(i/2))];

            if (i & 1)
                pixel &= 0xF;
            else
                pixel >>= 4;

            //if (pixel != 0)
            writeBgPixelWin(pixel, x)
            x += 1;
        }
    }
    else
    {
        for (int32_t i = visibleTilePixels; i < 8; i++)
        {
            pixel = bgtiles[tileLoc + (i/2)];

            if (i & 1)
                pixel >>= 4;
            else
                pixel &= 0xF;

            writeBgPixelWin(pixel, x)
            x += 1;
        }
    }

    //draw all middle pixels
    for (int32_t currTile = 1; currTile < ( (sLayerWidth / 8)); currTile++)
    {
        if (( (startTileX+currTile) & 63) > 31 && mapWidthInPixels > 256)
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31) + 0x400];
        else
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31)];
        tileNum = entry & 0x3FF;
        paletteNum = (entry >> 12) & 0xF;
        if (entry & (1 << 11))
            tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
        else
            tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
        uint32_t pixel32 = *(uint32_t*)&bgtiles[tileLoc];
        if (pixel32 != 0) {
            if (entry & (1 << 10))
            {
                writeBgPixelWin(pixel32 >> 28, x)
                writeBgPixelWin((pixel32 >> 24) & 0xF, x+1)
                writeBgPixelWin((pixel32 >> 20) & 0xF, x+2)
                writeBgPixelWin((pixel32 >> 16) & 0xF, x+3)
                writeBgPixelWin((pixel32 >> 12) & 0xF, x+4)
                writeBgPixelWin((pixel32 >> 8) & 0xF, x+5)
                writeBgPixelWin((pixel32 >> 4) & 0xF, x+6)
                writeBgPixelWin(pixel32 & 0xF, x+7)
            }
            else
            {
                writeBgPixelWin(pixel32 & 0xF, x)
                writeBgPixelWin((pixel32 >> 4) & 0xF, x+1)
                writeBgPixelWin((pixel32 >> 8) & 0xF, x+2)
                writeBgPixelWin((pixel32 >> 12) & 0xF, x+3)
                writeBgPixelWin((pixel32 >> 16) & 0xF, x+4)
                writeBgPixelWin((pixel32 >> 20) & 0xF, x+5)
                writeBgPixelWin((pixel32 >> 24) & 0xF, x+6)
                writeBgPixelWin(pixel32 >> 28, x+7)
            }
        }
        x += 8;
    }
    //draw right most tile
    if (((startTileX+(sLayerWidth/8)) & 63) > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31)];
    tileNum = entry & 0x3FF;
    paletteNum = (entry >> 12) & 0xF;
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        if (entry & (1 << 10)) //horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixelWin(pixel, x)
                    //line[x] = pal[16 * paletteNum + pixel] | 0x8000;
                x += 1;
            }
        }
        else //not horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];
                
                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixelWin(pixel, x)
                x += 1;
            }
        }
    }
    //apply horiziontal mosaic if needed
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        ApplyBGHorizontalMosaic(line);
    }
}

#undef writeBgPixel
#undef writeBgPixelWin

static void RenderBGScanlineWin(int32_t bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    uint32_t charBaseBlock = (control >> 2) & 3;
    uint32_t screenBaseBlock = (control >> 8) & 0x1F;
    uint32_t bitsPerPixel = 4;//((control >> 7) & 1) ? 8 : 4;
    uint32_t mapWidth = bgMapSizes[control >> 14][0];
    uint32_t mapHeight = bgMapSizes[control >> 14][1];
    uint32_t mapWidthInPixels = mapWidth * 8;
    uint32_t mapHeightInPixels = mapHeight * 8;
    uint16_t *bgmap = sLayerMap != NULL ? sLayerMap : (uint16_t *)BG_SCREEN_ADDR(screenBaseBlock);
    uint16_t* mask = scanline->bgMask;
    
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    //return;
    
    uint8_t *bgtiles = (uint8_t *)BG_CHAR_ADDR(charBaseBlock);
    uint16_t *pal = (uint16_t *)PLTT;
     
    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    
    uint32_t xx = hoffs & 0x1FF;
    uint32_t yy = (lineNum + voffs) & 0x1FF;
    
    
    if (yy > 255 && mapHeightInPixels > 256) {
        //the width check is for 512x512 mode support, it jumps by two screen bases instead
        bgmap += (mapWidthInPixels > 256) ? 0x800 : 0x400;
    }
    
    xx &= 0x1FF;
    yy &= 0xFF;
    
    uint32_t lineTile = ((lineNum + voffs) & 0xFF) / 8;
    uint32_t firstTile = bgmap[lineTile*32] & 0x3FF;
    uint32_t firstTileData = *(uint32_t*)&bgtiles[(firstTile * 32) + ((lineTile & 7) * bitsPerPixel)];
    if (firstTileData == 0 && mapWidthInPixels > 256)
    {
        for (uint32_t i = 1; i < 32; i++)
        {
            uint32_t tile = bgmap[lineTile*32+i] & 0x3FF;
            if (firstTile != tile)
                goto exitBlankScanlineLoop;
        }
        if (mapWidthInPixels > 256)
        {
            for (uint32_t i = 0; i < 32; i++)
            {
                uint32_t tile = bgmap[lineTile*32+i+0x400] & 0x3FF;
                if (firstTile != tile)
                    goto exitBlankScanlineLoop;
            }   
        }
        return;
    }
    exitBlankScanlineLoop:;
    
    uint32_t startTileX = xx / 8;
    uint32_t startTileY = yy / 8;
    uint32_t startTileYLoc = startTileY * 32;
    uint32_t visibleTilePixels = xx & 7;
    uint32_t tileY = yy & 7;
    uint32_t entry;
    
    if (startTileX > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + (startTileX & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + (startTileX & 31)];
    uint32_t tileNum = entry & 0x3FF;
    uint32_t paletteNum = (entry >> 12) & 0xF;
    uint32_t tileLoc;
    uint32_t pixel;
    uint32_t x = 0;
    
    //memset(line, 0x8F00, DISPLAY_WIDTH*2);
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    
    #define writeBgPixel(pixel, x)  \
        line[x] = pal[(paletteNum << 4) + (pixel)] | 0x8000; \
        mask[x] = 1 << bgNum;
    
    #define writeBgPixelWin(pixel, x)   \
        if ( ((pixel)) != 0 ) { \
            if (scanline->winMask[x] & (1 << bgNum)) { \
                writeBgPixel(pixel, x) \
            } \
        }
    
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    //if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    //{
        //draw left most tile
        if (entry & (1 << 10))
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixelWin(pixel, x)
                x += 1;
            }
        }
        else
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];

                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixelWin(pixel, x)
                x += 1;
            }
        }
    //}
    
    //draw all middle pixels
    for (int32_t currTile = 1; currTile < ( (sLayerWidth / 8)); currTile++)
    {
        if (( (startTileX+currTile) & 63) > 31 && mapWidthInPixels > 256)
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31) + 0x400];
        else
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31)];
        tileNum = entry & 0x3FF;
        paletteNum = (entry >> 12) & 0xF;
        if (entry & (1 << 11))
            tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
        else
            tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
        uint32_t pixel32 = *(uint32_t*)&bgtiles[tileLoc];
        if (pixel32 != 0) {
            if (entry & (1 << 10))
            {
                writeBgPixelWin(pixel32 >> 28, x)
                writeBgPixelWin((pixel32 >> 24) & 0xF, x+1)
                writeBgPixelWin((pixel32 >> 20) & 0xF, x+2)
                writeBgPixelWin((pixel32 >> 16) & 0xF, x+3)
                writeBgPixelWin((pixel32 >> 12) & 0xF, x+4)
                writeBgPixelWin((pixel32 >> 8) & 0xF, x+5)
                writeBgPixelWin((pixel32 >> 4) & 0xF, x+6)
                writeBgPixelWin(pixel32 & 0xF, x+7)
            }
            else
            {
                writeBgPixelWin(pixel32 & 0xF, x)
                writeBgPixelWin((pixel32 >> 4) & 0xF, x+1)
                writeBgPixelWin((pixel32 >> 8) & 0xF, x+2)
                writeBgPixelWin((pixel32 >> 12) & 0xF, x+3)
                writeBgPixelWin((pixel32 >> 16) & 0xF, x+4)
                writeBgPixelWin((pixel32 >> 20) & 0xF, x+5)
                writeBgPixelWin((pixel32 >> 24) & 0xF, x+6)
                writeBgPixelWin(pixel32 >> 28, x+7)
            }
        }
        x += 8;
    }
    //draw right most tile
    if (((startTileX+(sLayerWidth/8)) & 63) > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31)];
    tileNum = entry & 0x3FF;
    paletteNum = (entry >> 12) & 0xF;
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        if (entry & (1 << 10)) //horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixelWin(pixel, x)
                    //line[x] = pal[16 * paletteNum + pixel] | 0x8000;
                x += 1;
            }
        }
        else //not horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];
                
                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixelWin(pixel, x)
                x += 1;
            }
        }
    }
    //apply horiziontal mosaic if needed
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        ApplyBGHorizontalMosaic(line);
    }
}

#undef writeBgPixel
#undef writeBgPixelWin

static void RenderBGScanlineBlend(int32_t bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    uint32_t charBaseBlock = (control >> 2) & 3;
    uint32_t screenBaseBlock = (control >> 8) & 0x1F;
    uint32_t bitsPerPixel = 4;//((control >> 7) & 1) ? 8 : 4;
    uint32_t mapWidth = bgMapSizes[control >> 14][0];
    uint32_t mapHeight = bgMapSizes[control >> 14][1];
    uint32_t mapWidthInPixels = mapWidth * 8;
    uint32_t mapHeightInPixels = mapHeight * 8;
    uint16_t *bgmap = sLayerMap != NULL ? sLayerMap : (uint16_t *)BG_SCREEN_ADDR(screenBaseBlock);
    uint16_t* mask = scanline->bgMask;
    uint8_t blendMode = (REG_BLDCNT >> 6) & 3;
    
    if (windowsEnabled)
    {
        if (!(REG_WINOUT & (1 << bgNum)))
            return;
    }
    
    uint8_t *bgtiles = (uint8_t *)BG_CHAR_ADDR(charBaseBlock);
    uint16_t *pal = (uint16_t *)PLTT;
     
    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    
    uint32_t xx = hoffs & 0x1FF;
    uint32_t yy = (lineNum + voffs) & 0x1FF;
    
    
    if (yy > 255 && mapHeightInPixels > 256) {
        //the width check is for 512x512 mode support, it jumps by two screen bases instead
        bgmap += (mapWidthInPixels > 256) ? 0x800 : 0x400;
    }
    
    xx &= 0x1FF;
    yy &= 0xFF;
    
    uint32_t lineTile = ((lineNum + voffs) & 0xFF) / 8;
    uint32_t firstTile = bgmap[lineTile*32] & 0x3FF;
    uint32_t firstTileData = *(uint32_t*)&bgtiles[(firstTile * 32) + ((lineTile & 7) * bitsPerPixel)];
    if (firstTileData == 0 && mapWidthInPixels > 256)
    {
        for (uint32_t i = 1; i < 32; i++)
        {
            uint32_t tile = bgmap[lineTile*32+i] & 0x3FF;
            if (firstTile != tile)
                goto exitBlankScanlineLoop;
        }
        if (mapWidthInPixels > 256)
        {
            for (uint32_t i = 0; i < 32; i++)
            {
                uint32_t tile = bgmap[lineTile*32+i+0x400] & 0x3FF;
                if (firstTile != tile)
                    goto exitBlankScanlineLoop;
            }   
        }
        return;
    }
    exitBlankScanlineLoop:;
    
    uint32_t startTileX = xx / 8;
    uint32_t startTileY = yy / 8;
    uint32_t startTileYLoc = startTileY * 32;
    uint32_t visibleTilePixels = xx & 7;
    uint32_t tileY = yy & 7;
    uint32_t entry;
    
    if (startTileX > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + (startTileX & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + (startTileX & 31)];
    uint32_t tileNum = entry & 0x3FF;
    uint32_t paletteNum = (entry >> 12) & 0xF;
    uint32_t tileLoc;
    uint32_t pixel;
    uint32_t x = 0;
    
    //memset(line, 0x8F00, DISPLAY_WIDTH*2);
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    
    #define writeBgPixel(pixel, x)  \
    if ( ((pixel)) != 0 ) { \
        switch (blendMode) {\
        case 1: \
            if ((mask[x] & (REG_BLDCNT >> 8))) \
                line[x] = alphaBlendColor(pal[(paletteNum << 4) + (pixel)], line[x]) | 0x8000; \
            else \
                line[x] = pal[(paletteNum << 4) + (pixel)] | 0x8000;\
            break; \
        case 2: \
            line[x] = alphaBrightnessIncrease(pal[(paletteNum << 4) + (pixel)]) | 0x8000; \
            break; \
        case 3: \
            line[x] = alphaBrightnessDecrease(pal[(paletteNum << 4) + (pixel)]) | 0x8000; \
            break; } \
        mask[x] = 1 << bgNum; \
    }
    
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    //if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        //draw left most tile
        if (entry & (1 << 10))
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixel(pixel, x)
                x += 1;
            }
        }
        else
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];

                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixel(pixel, x)
                x += 1;
            }
        }
    }
    
    //draw all middle pixels
    for (int32_t currTile = 1; currTile < ( (sLayerWidth / 8)); currTile++)
    {
        if (( (startTileX+currTile) & 63) > 31 && mapWidthInPixels > 256)
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31) + 0x400];
        else
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31)];
        tileNum = entry & 0x3FF;
        paletteNum = (entry >> 12) & 0xF;
        if (entry & (1 << 11))
            tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
        else
            tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
        uint32_t pixel32 = *(uint32_t*)&bgtiles[tileLoc];
        if (pixel32 != 0) {
            if (entry & (1 << 10))
            {
                writeBgPixel(pixel32 >> 28, x)
                writeBgPixel((pixel32 >> 24) & 0xF, x+1)
                writeBgPixel((pixel32 >> 20) & 0xF, x+2)
                writeBgPixel((pixel32 >> 16) & 0xF, x+3)
                writeBgPixel((pixel32 >> 12) & 0xF, x+4)
                writeBgPixel((pixel32 >> 8) & 0xF, x+5)
                writeBgPixel((pixel32 >> 4) & 0xF, x+6)
                writeBgPixel(pixel32 & 0xF, x+7)
            }
            else
            {
                writeBgPixel(pixel32 & 0xF, x)
                writeBgPixel((pixel32 >> 4) & 0xF, x+1)
                writeBgPixel((pixel32 >> 8) & 0xF, x+2)
                writeBgPixel((pixel32 >> 12) & 0xF, x+3)
                writeBgPixel((pixel32 >> 16) & 0xF, x+4)
                writeBgPixel((pixel32 >> 20) & 0xF, x+5)
                writeBgPixel((pixel32 >> 24) & 0xF, x+6)
                writeBgPixel(pixel32 >> 28, x+7)
            }
        }
        x += 8;
    }
    //draw right most tile
    if (((startTileX+(sLayerWidth/8)) & 63) > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31)];
    tileNum = entry & 0x3FF;
    paletteNum = (entry >> 12) & 0xF;
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        if (entry & (1 << 10)) //horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixel(pixel, x)
                    //line[x] = pal[16 * paletteNum + pixel] | 0x8000;
                x += 1;
            }
        }
        else //not horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];
                
                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixel(pixel, x)
                x += 1;
            }
        }
    }
    //apply horiziontal mosaic if needed
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        ApplyBGHorizontalMosaic(line);
    }
}
#undef writeBgPixel

static void RenderBGScanlineNoEffect(int32_t bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    uint32_t charBaseBlock = (control >> 2) & 3;
    uint32_t screenBaseBlock = (control >> 8) & 0x1F;
    uint32_t bitsPerPixel = 4;//((control >> 7) & 1) ? 8 : 4;
    uint32_t mapWidth = bgMapSizes[control >> 14][0];
    uint32_t mapHeight = bgMapSizes[control >> 14][1];
    uint32_t mapWidthInPixels = mapWidth * 8;
    uint32_t mapHeightInPixels = mapHeight * 8;
    uint16_t *bgmap = sLayerMap != NULL ? sLayerMap : (uint16_t *)BG_SCREEN_ADDR(screenBaseBlock);
    uint16_t* mask = scanline->bgMask;
    
    if (windowsEnabled)
    {
        if (!(REG_WINOUT & (1 << bgNum)))
            return;
    }
    
    uint8_t *bgtiles = (uint8_t *)BG_CHAR_ADDR(charBaseBlock);
    uint16_t *pal = (uint16_t *)PLTT;
     
    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    
    uint32_t xx = hoffs & 0x1FF;
    uint32_t yy = (lineNum + voffs) & 0x1FF;
    
    
    if (yy > 255 && mapHeightInPixels > 256) {
        //the width check is for 512x512 mode support, it jumps by two screen bases instead
        bgmap += (mapWidthInPixels > 256) ? 0x800 : 0x400;
    }
    
    xx &= 0x1FF;
    yy &= 0xFF;
    
    uint32_t lineTile = ((lineNum + voffs) & 0xFF) / 8;
    uint32_t firstTile = bgmap[lineTile*32] & 0x3FF;
    uint32_t firstTileData = *(uint32_t*)&bgtiles[(firstTile * 32) + ((lineTile & 7) * bitsPerPixel)];
    if (firstTileData == 0 && mapWidthInPixels > 256)
    {
        for (uint32_t i = 1; i < 32; i++)
        {
            uint32_t tile = bgmap[lineTile*32+i] & 0x3FF;
            if (firstTile != tile)
                goto exitBlankScanlineLoop;
        }
        if (mapWidthInPixels > 256)
        {
            for (uint32_t i = 0; i < 32; i++)
            {
                uint32_t tile = bgmap[lineTile*32+i+0x400] & 0x3FF;
                if (firstTile != tile)
                    goto exitBlankScanlineLoop;
            }   
        }
        return;
    }
    exitBlankScanlineLoop:;
    
    uint32_t startTileX = xx / 8;
    uint32_t startTileY = yy / 8;
    uint32_t startTileYLoc = startTileY * 32;
    uint32_t visibleTilePixels = xx & 7;
    uint32_t tileY = yy & 7;
    uint32_t entry;
    
    if (startTileX > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + (startTileX & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + (startTileX & 31)];
    uint32_t tileNum = entry & 0x3FF;
    uint32_t paletteNum = (entry >> 12) & 0xF;
    uint32_t tileLoc;
    uint32_t pixel;
    uint32_t x = 0;
    
    //memset(line, 0x8F00, DISPLAY_WIDTH*2);
    //for (int i = 0; i < DISPLAY_WIDTH; i++)
    //{
    //  line[i] = 0x8F00;
    //}
    
    #define writeBgPixel(pixel, x)  \
        if ( ((pixel)) != 0 ) { \
            line[x] = pal[(paletteNum << 4) + (pixel)] | 0x8000; \
            mask[x] = 1 << bgNum; \
        } \
    
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    //if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        //draw left most tile
        if (entry & (1 << 10))
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixel(pixel, x)
                x += 1;
            }
        }
        else
        {
            for (int32_t i = visibleTilePixels; i < 8; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];

                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixel(pixel, x)
                x += 1;
            }
        }
    }
    
    //draw all middle pixels
    for (int32_t currTile = 1; currTile < ( (sLayerWidth / 8)); currTile++)
    {
        if (( (startTileX+currTile) & 63) > 31 && mapWidthInPixels > 256)
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31) + 0x400];
        else
            entry = bgmap[startTileYLoc + ((startTileX+currTile) & 31)];
        tileNum = entry & 0x3FF;
        paletteNum = (entry >> 12) & 0xF;
        if (entry & (1 << 11))
            tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
        else
            tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
        uint32_t pixel32 = *(uint32_t*)&bgtiles[tileLoc];
        if (pixel32 != 0) {
            if (entry & (1 << 10))
            {
                writeBgPixel(pixel32 >> 28, x)
                writeBgPixel((pixel32 >> 24) & 0xF, x+1)
                writeBgPixel((pixel32 >> 20) & 0xF, x+2)
                writeBgPixel((pixel32 >> 16) & 0xF, x+3)
                writeBgPixel((pixel32 >> 12) & 0xF, x+4)
                writeBgPixel((pixel32 >> 8) & 0xF, x+5)
                writeBgPixel((pixel32 >> 4) & 0xF, x+6)
                writeBgPixel(pixel32 & 0xF, x+7)
            }
            else
            {
                writeBgPixel(pixel32 & 0xF, x)
                writeBgPixel((pixel32 >> 4) & 0xF, x+1)
                writeBgPixel((pixel32 >> 8) & 0xF, x+2)
                writeBgPixel((pixel32 >> 12) & 0xF, x+3)
                writeBgPixel((pixel32 >> 16) & 0xF, x+4)
                writeBgPixel((pixel32 >> 20) & 0xF, x+5)
                writeBgPixel((pixel32 >> 24) & 0xF, x+6)
                writeBgPixel(pixel32 >> 28, x+7)
            }
        }
        x += 8;
    }
    //draw right most tile
    if (((startTileX+(sLayerWidth/8)) & 63) > 31 && mapWidthInPixels > 256)
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31) + 0x400];
    else
        entry = bgmap[startTileYLoc + ((startTileX+(sLayerWidth/8)) & 31)];
    tileNum = entry & 0x3FF;
    paletteNum = (entry >> 12) & 0xF;
    if (entry & (1 << 11))
        tileLoc = (tileNum * (bitsPerPixel * 8)) + (7 - tileY)*bitsPerPixel;
    else
        tileLoc = (tileNum * (bitsPerPixel * 8)) + tileY*bitsPerPixel;
    if((*(uint32_t*)&bgtiles[tileLoc]) != 0)
    {
        if (entry & (1 << 10)) //horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (3-(i/2))];

                if (i & 1)
                    pixel &= 0xF;
                else
                    pixel >>= 4;

                //if (pixel != 0)
                writeBgPixel(pixel, x)
                    //line[x] = pal[16 * paletteNum + pixel] | 0x8000;
                x += 1;
            }
        }
        else //not horizontally flipped
        {
            for (int32_t i = 0; i < visibleTilePixels; i++)
            {
                pixel = bgtiles[tileLoc + (i/2)];
                
                if (i & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;

                writeBgPixel(pixel, x)
                x += 1;
            }
        }
    }
    //apply horiziontal mosaic if needed
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        ApplyBGHorizontalMosaic(line);
    }
}
#undef writeBgPixel

static inline uint32_t getBgX(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2X;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3X;
    }
}

static inline uint32_t getBgY(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2Y;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3Y;
    }
}

static inline uint16_t getBgPA(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PA;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3PA;
    }
}

static inline uint16_t getBgPB(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PB;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3PB;
    }
}

static inline uint16_t getBgPC(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PC;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3PC;
    }
}

static inline uint16_t getBgPD(int32_t bgNumber)
{
    if (bgNumber == 2)
    {
        return REG_BG2PD;
    }
    else // BG3: only BG2 and BG3 are affine
    {
        return REG_BG3PD;
    }
}

static void RenderRotScaleBGScanlineWinBlend(int32_t bgNum, uint16_t control, uint16_t x, uint16_t y, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    uint32_t charBaseBlock = bgcnt->charBaseBlock;
    uint32_t screenBaseBlock = bgcnt->screenBaseBlock;

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;
    uint8_t blendMode = (REG_BLDCNT >> 6) & 3;
    uint16_t* mask = scanline->bgMask;

    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int32_t sizeX = 128;
    int32_t sizeY = 128;

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int32_t maskX = sizeX - 1;
    int32_t maskY = sizeY - 1;
    int32_t yshift = ((control >> 14) & 3) + 4;
    int32_t currentX = getBgX(bgNum);
    int32_t currentY = getBgY(bgNum);
    //sign extend 28 bit number
    currentX = ((currentX & (1 << 27)) ? currentX | 0xF0000000 : currentX);
    currentY = ((currentY & (1 << 27)) ? currentY | 0xF0000000 : currentY);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int32_t realX = currentX;
    int32_t realY = currentY;
    
    #define writeBgPixel(pixel, x)  \
        if (scanline->winMask[x] & WINMASK_CLR) { \
            switch (blendMode) { \
            case 1: \
                if ((mask[x] & (REG_BLDCNT >> 8))) \
                    line[x] = alphaBlendColor(pal[pixel], line[x]) | 0x8000; \
                else \
                    line[x] = pal[pixel] | 0x8000;\
                break; \
            case 2: \
                line[x] = alphaBrightnessIncrease(pal[pixel]) | 0x8000; \
                break; \
            case 3: \
                line[x] = alphaBrightnessDecrease(pal[pixel]) | 0x8000; \
                break; } \
        }else{ \
            line[x] = pal[pixel] | 0x8000; \
        } \
        mask[x] = 1 << bgNum;
    
    #define writeBgPixelWin(pixel, x)   \
        if ( ((pixel)) != 0 ) { \
            if (scanline->winMask[x] & (1 << bgNum)) { \
                writeBgPixel(pixel, x) \
            } \
        }

    if (bgcnt->areaOverflowMode)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8) & maskX;
            int32_t yyy = (realY >> 8) & maskY;

            int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int32_t tileX = xxx & 7;
            int32_t tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            writeBgPixelWin(pixel, x)

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8);
            int32_t yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int32_t tileX = xxx & 7;
                int32_t tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                writeBgPixelWin(pixel, x)
            }
            realX += pa;
            realY += pc;
        }
    }
    //apply mosaic effect if enabled
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint16_t color = line[applyBGHorizontalMosaicEffect(x)];
            line[x] = color;
            
        }
    }
}

#undef writeBgPixel
#undef writeBgPixelWin

static void RenderRotScaleBGScanlineWin(int32_t bgNum, uint16_t control, uint16_t x, uint16_t y, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    uint32_t charBaseBlock = bgcnt->charBaseBlock;
    uint32_t screenBaseBlock = bgcnt->screenBaseBlock;

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;
    uint16_t* mask = scanline->bgMask;

    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int32_t sizeX = 128;
    int32_t sizeY = 128;

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int32_t maskX = sizeX - 1;
    int32_t maskY = sizeY - 1;
    int32_t yshift = ((control >> 14) & 3) + 4;
    int32_t currentX = getBgX(bgNum);
    int32_t currentY = getBgY(bgNum);
    //sign extend 28 bit number
    currentX = ((currentX & (1 << 27)) ? currentX | 0xF0000000 : currentX);
    currentY = ((currentY & (1 << 27)) ? currentY | 0xF0000000 : currentY);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int32_t realX = currentX;
    int32_t realY = currentY;
    
    #define writeBgPixel(pixel, x)  \
        line[x] = pal[pixel] | 0x8000; \
        mask[x] = 1 << bgNum;
    
    #define writeBgPixelWin(pixel, x)   \
        if ( ((pixel)) != 0 ) { \
            if (scanline->winMask[x] & (1 << bgNum)) { \
                writeBgPixel(pixel, x) \
            } \
        }

    if (bgcnt->areaOverflowMode)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8) & maskX;
            int32_t yyy = (realY >> 8) & maskY;

            int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int32_t tileX = xxx & 7;
            int32_t tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            writeBgPixelWin(pixel, x)

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8);
            int32_t yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int32_t tileX = xxx & 7;
                int32_t tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                writeBgPixelWin(pixel, x)
            }
            realX += pa;
            realY += pc;
        }
    }
    //apply mosaic effect if enabled
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint16_t color = line[applyBGHorizontalMosaicEffect(x)];
            line[x] = color;
            
        }
    }
}

#undef writeBgPixel
#undef writeBgPixelWin

static void RenderRotScaleBGScanlineBlend(int32_t bgNum, uint16_t control, uint16_t x, uint16_t y, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    uint32_t charBaseBlock = bgcnt->charBaseBlock;
    uint32_t screenBaseBlock = bgcnt->screenBaseBlock;

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;
    uint8_t blendMode = (REG_BLDCNT >> 6) & 3;
    uint16_t* mask = scanline->bgMask;
    
    if (windowsEnabled)
    {
        if (!(REG_WINOUT & (1 << bgNum)))
            return;
    }

    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int32_t sizeX = 128;
    int32_t sizeY = 128;
    

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int32_t maskX = sizeX - 1;
    int32_t maskY = sizeY - 1;
    int32_t yshift = ((control >> 14) & 3) + 4;
    int32_t currentX = getBgX(bgNum);
    int32_t currentY = getBgY(bgNum);
    //sign extend 28 bit number
    currentX = ((currentX & (1 << 27)) ? currentX | 0xF0000000 : currentX);
    currentY = ((currentY & (1 << 27)) ? currentY | 0xF0000000 : currentY);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int32_t realX = currentX;
    int32_t realY = currentY;
    
    #define writeBgPixel(pixel, x)  \
    if ( ((pixel)) != 0 ) { \
        switch (blendMode) {\
        case 1: \
            if ((mask[x] & (REG_BLDCNT >> 8))) \
                line[x] = alphaBlendColor(pal[pixel], line[x]) | 0x8000; \
            else \
                line[x] = pal[pixel] | 0x8000;\
            break; \
        case 2: \
            line[x] = alphaBrightnessIncrease(pal[pixel]) | 0x8000; \
            break; \
        case 3: \
            line[x] = alphaBrightnessDecrease(pal[pixel]) | 0x8000; \
            break; } \
        mask[x] = 1 << bgNum; \
    }

    if (bgcnt->areaOverflowMode)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8) & maskX;
            int32_t yyy = (realY >> 8) & maskY;

            int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int32_t tileX = xxx & 7;
            int32_t tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            //if (pixel != 0) {
            //    line[x] = pal[pixel] | 0x8000;
            //}
            writeBgPixel(pixel, x)

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8);
            int32_t yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int32_t tileX = xxx & 7;
                int32_t tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                writeBgPixel(pixel, x)
            }
            realX += pa;
            realY += pc;
        }
    }
    //apply mosaic effect if enabled
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint16_t color = line[applyBGHorizontalMosaicEffect(x)];
            line[x] = color;
            
        }
    }
}

#undef writeBgPixel

static void RenderRotScaleBGScanlineNoEffect(int32_t bgNum, uint16_t control, uint16_t x, uint16_t y, int32_t lineNum, uint16_t *line, struct scanlineData* scanline, bool windowsEnabled)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    uint32_t charBaseBlock = bgcnt->charBaseBlock;
    uint32_t screenBaseBlock = bgcnt->screenBaseBlock;

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;
    
    if (windowsEnabled)
    {
        if (!(REG_WINOUT & (1 << bgNum)))
            return;
    }

    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int32_t sizeX = 128;
    int32_t sizeY = 128;

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int32_t maskX = sizeX - 1;
    int32_t maskY = sizeY - 1;
    int32_t yshift = ((control >> 14) & 3) + 4;
    int32_t currentX = getBgX(bgNum);
    int32_t currentY = getBgY(bgNum);
    //sign extend 28 bit number
    currentX = ((currentX & (1 << 27)) ? currentX | 0xF0000000 : currentX);
    currentY = ((currentY & (1 << 27)) ? currentY | 0xF0000000 : currentY);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int32_t realX = currentX;
    int32_t realY = currentY;

    if (bgcnt->areaOverflowMode)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8) & maskX;
            int32_t yyy = (realY >> 8) & maskY;

            int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int32_t tileX = xxx & 7;
            int32_t tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            if (pixel != 0) {
                line[x] = pal[pixel] | 0x8000;
            }

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            int32_t xxx = (realX >> 8);
            int32_t yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int32_t tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int32_t tileX = xxx & 7;
                int32_t tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                if (pixel != 0) {
                    line[x] = pal[pixel] | 0x8000;
                }
            }
            realX += pa;
            realY += pc;
        }
    }
    //apply mosaic effect if enabled
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint16_t color = line[applyBGHorizontalMosaicEffect(x)];
            line[x] = color;
            
        }
    }
}

const u8 spriteSizes[][2] =
{
    {8, 16},
    {8, 32},
    {16, 32},
    {32, 64},
};

static void DrawSpritesWinMask(struct scanlineData* scanline, uint16_t vcount)
{
    int32_t i;
    void *objtiles = VRAM_ + 0x10000;

    int16_t matrix[2][2] = {};

    if (!(REG_DISPCNT & (1 << 6)))
    {
        puts("2-D OBJ Character mapping not supported.");
    }

    for (i = 127; i >= 0; i--)
    {
        struct OamData *oam = &((struct OamData *)OAM)[i];
        if (oam->objMode != 2)
            continue;
        
        uint32_t width;
        uint32_t height;

        bool isAffine  = oam->affineMode & 1;
        bool doubleSizeOrDisabled = (oam->affineMode >> 1) & 1;

        if (!(isAffine) && doubleSizeOrDisabled) // disable for non-affine
        {
            continue;
        }

        if (oam->shape == 0)
        {
            width = (1 << oam->size) * 8;
            height = (1 << oam->size) * 8;
        }
        else if (oam->shape == 1) // wide
        {
            width = spriteSizes[oam->size][1];
            height = spriteSizes[oam->size][0];
        }
        else if (oam->shape == 2) // tall
        {
            width = spriteSizes[oam->size][0];
            height = spriteSizes[oam->size][1];
        }
        else
        {
            continue; // prohibited, do not draw
        }

        int32_t rect_width = width;
        int32_t rect_height = height;

        int32_t half_width = width / 2;
        int32_t half_height = height / 2;

        int32_t x = oam->x;
        int32_t y = oam->y;

        if (x >= sObjXMax)
            x -= 512;
        if (y >= DISPLAY_HEIGHT)
            y -= 256;

        if (isAffine)
        {
            //TODO: there is probably a better way to do this
            u8 matrixNum = oam->matrixNum * 4;

            struct OamData *oam1 = &((struct OamData *)OAM)[matrixNum];
            struct OamData *oam2 = &((struct OamData *)OAM)[matrixNum + 1];
            struct OamData *oam3 = &((struct OamData *)OAM)[matrixNum + 2];
            struct OamData *oam4 = &((struct OamData *)OAM)[matrixNum + 3];

            matrix[0][0] = oam1->affineParam;
            matrix[0][1] = oam2->affineParam;
            matrix[1][0] = oam3->affineParam;
            matrix[1][1] = oam4->affineParam;

            if (doubleSizeOrDisabled) // double size for affine
            {
                rect_width *= 2;
                rect_height *= 2;
                half_width *= 2;
                half_height *= 2;
            }
        }
        else
        {
            // Identity
            matrix[0][0] = 0x100;
            matrix[0][1] = 0;
            matrix[1][0] = 0;
            matrix[1][1] = 0x100;
        }

        x += half_width;
        y += half_height;

        // Does this sprite actually draw on this scanline?
        if (vcount >= (y - half_height) && vcount < (y + half_height))
        {
            int32_t local_y = (oam->mosaic == 1) ? applySpriteVerticalMosaicEffect(vcount) - y : vcount - y;
            bool flipX  = !isAffine && ((oam->matrixNum >> 3) & 1);
            bool flipY  = !isAffine && ((oam->matrixNum >> 4) & 1);
            bool is8BPP  = oam->bpp & 1;

            for (int32_t local_x = -half_width; local_x <= half_width; local_x++)
            {
                uint8_t *tiledata = (uint8_t *)objtiles;
                uint16_t *palette = (uint16_t *)(PLTT + 0x200);
                int32_t local_mosaicX;
                int32_t tex_x;
                int32_t tex_y;

                int32_t global_x = local_x + x;

                if (global_x < sObjXMin || global_x >= sObjXMax)
                    continue;

                if (oam->mosaic == 1)
                {
                    //mosaic effect has to be applied to global coordinates otherwise the mosaic will scroll
                    local_mosaicX = applySpriteHorizontalMosaicEffect(global_x) - x;
                    tex_x = ((matrix[0][0] * local_mosaicX + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_mosaicX + matrix[1][1] * local_y) >> 8) + (height / 2);
                }else{
                    tex_x = ((matrix[0][0] * local_x + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_x + matrix[1][1] * local_y) >> 8) + (height / 2);
                }


                /* Check if transformed coordinates are inside bounds. */

                if (tex_x >= width || tex_y >= height || tex_x < 0 || tex_y < 0)
                    continue;

                if (flipX)
                    tex_x = width  - tex_x - 1;
                if (flipY)
                    tex_y = height - tex_y - 1;

                int32_t tile_x = tex_x % 8;
                int32_t tile_y = tex_y % 8;
                int32_t block_x = tex_x / 8;
                int32_t block_y = tex_y / 8;
                int32_t block_offset = ((block_y * (REG_DISPCNT & 0x40 ? (width / 8) : 16)) + block_x);
                uint16_t pixel = 0;

                if (!is8BPP)
                {
                    pixel = tiledata[(block_offset + oam->tileNum) * 32 + (tile_y * 4) + (tile_x / 2)];
                    if (tile_x & 1)
                        pixel >>= 4;
                    else
                        pixel &= 0xF;
                    palette += oam->paletteNum * 16;
                }
                else
                {
                    pixel = tiledata[(block_offset * 2 + oam->tileNum) * 32 + (tile_y * 8) + tile_x];
                }

                if (pixel != 0)
                {
                    //this code runs if pixel is to be drawn
                    if (global_x < sObjXMax && global_x >= sObjXMin)
                    {
                        if (scanline->winMask[global_x] & WINMASK_WINOUT)
                            scanline->winMask[global_x] = (REG_WINOUT >> 8) & 0x3F;
                    }
                }
            }
        }
    }
}

static void inline_hack DrawAffineSprite(int32_t SpriteIndex, struct scanlineData* scanline, uint16_t vcount, bool windowsEnabled, uint16_t* pixels, bool IsInsideWinIn)
{
    struct OamData *oam = &((struct OamData *)OAM)[SpriteIndex];
    void *objtiles = VRAM_ + 0x10000;
    uint32_t blendMode = (REG_BLDCNT >> 6) & 3;
    bool winShouldBlendPixel = true;
    uint32_t width;
    uint32_t height;

    bool isAffine  = oam->affineMode & 1;
    bool doubleSizeOrDisabled = (oam->affineMode >> 1) & 1;
    bool isSemiTransparent = (oam->objMode == 1);

    if (!(isAffine) && doubleSizeOrDisabled) // disable for non-affine
    {
        return;
    }

    if (oam->shape == 0)
    {
        width = (1 << oam->size) * 8;
        height = (1 << oam->size) * 8;
    }
    else if (oam->shape == 1) // wide
    {
        width = spriteSizes[oam->size][1];
        height = spriteSizes[oam->size][0];
    }
    else if (oam->shape == 2) // tall
    {
        width = spriteSizes[oam->size][0];
        height = spriteSizes[oam->size][1];
    }
    else
    {
        return; // prohibited, do not draw
    }

    int32_t rect_width = width;
    int32_t rect_height = height;

    int32_t half_width = width / 2;
    int32_t half_height = height / 2;

    int32_t x = oam->x;
    int32_t y = oam->y;

    if (x >= sObjXMax)
        x -= 512;
    if (y >= DISPLAY_HEIGHT)
        y -= 256;
    
    int16_t matrix[2][2];

    if (isAffine)
    {
        //TODO: there is probably a better way to do this
        u8 matrixNum = oam->matrixNum * 4;

        struct OamData *oam1 = &((struct OamData *)OAM)[matrixNum];
        struct OamData *oam2 = &((struct OamData *)OAM)[matrixNum + 1];
        struct OamData *oam3 = &((struct OamData *)OAM)[matrixNum + 2];
        struct OamData *oam4 = &((struct OamData *)OAM)[matrixNum + 3];

        matrix[0][0] = oam1->affineParam;
        matrix[0][1] = oam2->affineParam;
        matrix[1][0] = oam3->affineParam;
        matrix[1][1] = oam4->affineParam;

        if (doubleSizeOrDisabled) // double size for affine
        {
            rect_width *= 2;
            rect_height *= 2;
            half_width *= 2;
            half_height *= 2;
        }
    }
    else
    {
        // Identity
        matrix[0][0] = 0x100;
        matrix[0][1] = 0;
        matrix[1][0] = 0;
        matrix[1][1] = 0x100;
    }

    x += half_width;
    y += half_height;

    // Does this sprite actually draw on this scanline?
    if (vcount >= (y - half_height) && vcount < (y + half_height))
    {
        int32_t local_y = (oam->mosaic == 1) ? applySpriteVerticalMosaicEffect(vcount) - y : vcount - y;
        bool flipX  = !isAffine && ((oam->matrixNum >> 3) & 1);
        bool flipY  = !isAffine && ((oam->matrixNum >> 4) & 1);
        bool is8BPP  = oam->bpp & 1;

        for (int32_t local_x = -half_width; local_x <= half_width; local_x++)
        {
            uint8_t *tiledata = (uint8_t *)objtiles;
            uint16_t *palette = (uint16_t *)(PLTT + 0x200);
            int32_t local_mosaicX;
            int32_t tex_x;
            int32_t tex_y;

            int32_t global_x = local_x + x;

            if (global_x < sObjXMin || global_x >= sObjXMax)
                continue;

            if (isAffine)
            {
                if (oam->mosaic == 1)
                {
                    //mosaic effect has to be applied to global coordinates otherwise the mosaic will scroll
                    local_mosaicX = applySpriteHorizontalMosaicEffect(global_x) - x;
                    tex_x = ((matrix[0][0] * local_mosaicX + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_mosaicX + matrix[1][1] * local_y) >> 8) + (height / 2);
                }
                else
                {
                    tex_x = ((matrix[0][0] * local_x + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_x + matrix[1][1] * local_y) >> 8) + (height / 2);
                }
            }
            else
            {
                tex_x = (oam->mosaic == 1 ? applySpriteHorizontalMosaicEffect(global_x) - x : local_x) + (width / 2);
                tex_y = local_y + (height / 2);
            }


            /* Check if transformed coordinates are inside bounds. */

            if (tex_x >= width || tex_y >= height || tex_x < 0 || tex_y < 0)
                continue;

            if (flipX)
                tex_x = width  - tex_x - 1;
            if (flipY)
                tex_y = height - tex_y - 1;

            int32_t tile_x = tex_x % 8;
            int32_t tile_y = tex_y % 8;
            int32_t block_x = tex_x / 8;
            int32_t block_y = tex_y / 8;
            int32_t block_offset = ((block_y * (REG_DISPCNT & 0x40 ? (width / 8) : 16)) + block_x);
            uint16_t pixel = 0;

            if (!is8BPP)
            {
                pixel = tiledata[(block_offset + oam->tileNum) * 32 + (tile_y * 4) + (tile_x / 2)];
                if (tile_x & 1)
                    pixel >>= 4;
                else
                    pixel &= 0xF;
                palette += oam->paletteNum * 16;
            }
            else
            {
                pixel = tiledata[(block_offset * 2 + oam->tileNum) * 32 + (tile_y * 8) + tile_x];
            }

            if (pixel != 0)
            {
                uint16_t color = palette[pixel];;
                
                //if sprite mode is 2 then write to the window mask instead
                /*if (isObjWin)
                {
                    if (scanline->winMask[global_x] & WINMASK_WINOUT)
                    scanline->winMask[global_x] = (REG_WINOUT >> 8) & 0x3F;
                    continue;
                }*/
                if (windowsEnabled && !(scanline->winMask[global_x] & WINMASK_OBJ) && IsInsideWinIn == true)
                {
                    continue;
                }
                
                //this code runs if pixel is to be drawn
                if (global_x < sObjXMax && global_x >= sObjXMin)
                {
                    //check if its enabled in the window (if window is enabled)
                    if (IsInsideWinIn == true)
                        winShouldBlendPixel = (windowsEnabled == false || scanline->winMask[global_x] & WINMASK_CLR);
                    else
                        winShouldBlendPixel = (windowsEnabled == false || REG_WINOUT & WINOUT_WIN01_CLR);
                    //has to be separated from the blend mode switch statement because of OBJ semi transparancy feature
                    if ((blendMode == 1 && REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel) || isSemiTransparent)
                    {
                        uint16_t targetA = color;

                        if (scanline->bgMask[global_x] & (REG_BLDCNT >> 8))
                            color = alphaBlendColor(targetA, pixels[global_x]);
                    }
                    else if (REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel)
                    {
                        switch (blendMode)
                        {
                        case 2:
                            color = alphaBrightnessIncrease(color);
                            break;
                        case 3:
                            color = alphaBrightnessDecrease(color);
                            break;
                        }
                    }
                    
                    //write pixel to pixel framebuffer
                    pixels[global_x] = color | (1 << 15);
                    scanline->bgMask[global_x] = (1 << 4); // top most obj pixel bit
                }
            }
        }
    }
}

static void inline_hack DrawNonAffineSprite(int32_t SpriteIndex, struct scanlineData* scanline, uint16_t vcount, bool windowsEnabled, uint16_t* pixels, bool IsInsideWinIn)
{
    struct OamData *oam = &((struct OamData *)OAM)[SpriteIndex];
    void *objtiles = VRAM_ + 0x10000;
    uint32_t blendMode = (REG_BLDCNT >> 6) & 3;
    bool winShouldBlendPixel = true;
    uint32_t width;
    uint32_t height;

    bool isAffine  = oam->affineMode & 1;
    bool doubleSizeOrDisabled = (oam->affineMode >> 1) & 1;
    bool isSemiTransparent = (oam->objMode == 1);

    if (!(isAffine) && doubleSizeOrDisabled) // disable for non-affine
    {
        return;
    }

    if (oam->shape == 0)
    {
        width = (1 << oam->size) * 8;
        height = (1 << oam->size) * 8;
    }
    else if (oam->shape == 1) // wide
    {
        width = spriteSizes[oam->size][1];
        height = spriteSizes[oam->size][0];
    }
    else if (oam->shape == 2) // tall
    {
        width = spriteSizes[oam->size][0];
        height = spriteSizes[oam->size][1];
    }
    else
    {
        return; // prohibited, do not draw
    }

    int32_t half_height = height / 2;
    int32_t x = oam->x;
    int32_t y = oam->y;

    if (x >= sObjXMax)
        x -= 512;
    if (y >= DISPLAY_HEIGHT)
        y -= 256;

    y += half_height;

    // Does this sprite actually draw on this scanline?
    if (vcount >= (y - half_height) && vcount < (y + half_height))
    {
        int32_t local_y = (oam->mosaic == 1) ? applySpriteVerticalMosaicEffect(vcount) - y : vcount - y;
        //int palette = oam->paletteNum;
        bool flipX  = !isAffine && ((oam->matrixNum >> 3) & 1);
        bool flipY  = !isAffine && ((oam->matrixNum >> 4) & 1);
        uint8_t *tiledata = (uint8_t *)objtiles;
        uint16_t *palette = (uint16_t *)OBJ_PLTT;
        palette += oam->paletteNum * 16; //choose the palette
        int32_t tex_y = local_y + (height / 2);
        if (flipY)
            tex_y = height - tex_y - 1;
        //run the block drawing loop in reverse if sprite is flipped otherwise run as normal
        int32_t blockStart, blockEnd, blockIncrement;
        if (flipX)
        {
            blockStart = width / TILE_WIDTH-1;
            blockEnd = -1;
            blockIncrement = -1;
        }
        else
        {
            blockStart = 0;
            blockEnd = width / TILE_WIDTH;
            blockIncrement = 1;
        }
        
        for (int32_t block_x = blockStart; block_x != blockEnd; block_x += blockIncrement)
        {
            int32_t tile_y = tex_y & (TILE_HEIGHT-1);
            int32_t block_y = tex_y / TILE_HEIGHT;
            int32_t block_offset = ((block_y * (REG_DISPCNT & DISPCNT_OBJ_1D_MAP ? (width / 8) : 16)) + block_x);
            uint8_t* pixelData = &tiledata[(oam->tileNum + block_offset) * TILE_SIZE_4BPP + (tile_y * 4)];
            uint32_t pixel32 = *(uint32_t*)pixelData; //load whole tile worth of palette pixels
            
            if (x >= sObjXMin && x + TILE_WIDTH <= sObjXMax)
            {
                if (windowsEnabled)
                {
                    if (blendMode != 0 || isSemiTransparent) //Windowing and blending
                    {
                        #define writeSpritePixelWinBlend(pixel, x) \
                            if (pixel && ((IsInsideWinIn && scanline->winMask[x] & WINMASK_OBJ) || (!IsInsideWinIn && REG_WINOUT & WINOUT_WIN01_OBJ))) { \
                                uint16_t color = palette[pixel]; \
                                winShouldBlendPixel = (IsInsideWinIn && scanline->winMask[x] & WINMASK_CLR) || (!IsInsideWinIn && REG_WINOUT & WINOUT_WIN01_CLR); \
                                \
                                if (((blendMode == 1 && REG_BLDCNT & BLDCNT_TGT1_OBJ) && winShouldBlendPixel) || isSemiTransparent) \
                                { \
                                    if (scanline->bgMask[x] & (REG_BLDCNT >> 8)) \
                                        color = alphaBlendColor(color, pixels[x]); \
                                } \
                                else if(REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel) \
                                { \
                                    switch (blendMode) \
                                    { \
                                        case 2: \
                                            color = alphaBrightnessIncrease(color); \
                                            break; \
                                        case 3: \
                                            color = alphaBrightnessDecrease(color); \
                                            break; \
                                    } \
                                } \
                                pixels[x] = color | (1 << 15); \
                                scanline->bgMask[x] = (1 << 4); \
                            }

                        if (flipX)
                        {
                            writeSpritePixelWinBlend(pixel32 >> 28, x)
                            writeSpritePixelWinBlend((pixel32 >> 24) & 0xF, x+1)
                            writeSpritePixelWinBlend((pixel32 >> 20) & 0xF, x+2)
                            writeSpritePixelWinBlend((pixel32 >> 16) & 0xF, x+3)
                            writeSpritePixelWinBlend((pixel32 >> 12) & 0xF, x+4)
                            writeSpritePixelWinBlend((pixel32 >> 8) & 0xF, x+5)
                            writeSpritePixelWinBlend((pixel32 >> 4) & 0xF, x+6)
                            writeSpritePixelWinBlend(pixel32 & 0xF, x+7)
                        }
                        else
                        {
                            writeSpritePixelWinBlend(pixel32 & 0xF, x)
                            writeSpritePixelWinBlend((pixel32 >> 4) & 0xF, x+1)
                            writeSpritePixelWinBlend((pixel32 >> 8) & 0xF, x+2)
                            writeSpritePixelWinBlend((pixel32 >> 12) & 0xF, x+3)
                            writeSpritePixelWinBlend((pixel32 >> 16) & 0xF, x+4)
                            writeSpritePixelWinBlend((pixel32 >> 20) & 0xF, x+5)
                            writeSpritePixelWinBlend((pixel32 >> 24) & 0xF, x+6)
                            writeSpritePixelWinBlend(pixel32 >> 28, x+7)
                        }
                        #undef writeSpritePixelWinBlend
                    }
                    else //Windowing
                    {
                        #define writeSpritePixelWin(pixel, x) \
                            if (pixel && ((IsInsideWinIn && scanline->winMask[x] & WINMASK_OBJ) || (!IsInsideWinIn && REG_WINOUT & WINOUT_WIN01_OBJ))) { \
                                pixels[x] = palette[pixel] | (1 << 15); \
                                scanline->bgMask[x] = (1 << 4); \
                            }

                        if (flipX)
                        {
                            writeSpritePixelWin(pixel32 >> 28, x)
                            writeSpritePixelWin((pixel32 >> 24) & 0xF, x+1)
                            writeSpritePixelWin((pixel32 >> 20) & 0xF, x+2)
                            writeSpritePixelWin((pixel32 >> 16) & 0xF, x+3)
                            writeSpritePixelWin((pixel32 >> 12) & 0xF, x+4)
                            writeSpritePixelWin((pixel32 >> 8) & 0xF, x+5)
                            writeSpritePixelWin((pixel32 >> 4) & 0xF, x+6)
                            writeSpritePixelWin(pixel32 & 0xF, x+7)
                        }
                        else
                        {
                            writeSpritePixelWin(pixel32 & 0xF, x)
                            writeSpritePixelWin((pixel32 >> 4) & 0xF, x+1)
                            writeSpritePixelWin((pixel32 >> 8) & 0xF, x+2)
                            writeSpritePixelWin((pixel32 >> 12) & 0xF, x+3)
                            writeSpritePixelWin((pixel32 >> 16) & 0xF, x+4)
                            writeSpritePixelWin((pixel32 >> 20) & 0xF, x+5)
                            writeSpritePixelWin((pixel32 >> 24) & 0xF, x+6)
                            writeSpritePixelWin(pixel32 >> 28, x+7)
                        }
                        #undef writeSpritePixelWin
                    }
                }
                else //Choose between Blend and none
                {
                    if (blendMode != 0 || isSemiTransparent) //Blend
                    {
                        #define writeSpritePixelBlend(pixel, x) \
                        if (pixel) { \
                            uint16_t color = palette[pixel]; \
                            \
                            if ( (blendMode == 1 && REG_BLDCNT & BLDCNT_TGT1_OBJ) || isSemiTransparent) \
                            { \
                                if (scanline->bgMask[x] & (REG_BLDCNT >> 8)) \
                                    color = alphaBlendColor(color, pixels[x]); \
                            } \
                            else if(REG_BLDCNT & BLDCNT_TGT1_OBJ) \
                            { \
                                switch (blendMode) \
                                { \
                                    case 2: \
                                        color = alphaBrightnessIncrease(color); \
                                        break; \
                                    case 3: \
                                        color = alphaBrightnessDecrease(color); \
                                        break; \
                                } \
                            } \
                            \
                            pixels[x] = color | (1 << 15); \
                            scanline->bgMask[x] = (1 << 4); \
                        }

                        if (flipX)
                        {
                            writeSpritePixelBlend(pixel32 >> 28, x)
                            writeSpritePixelBlend((pixel32 >> 24) & 0xF, x+1)
                            writeSpritePixelBlend((pixel32 >> 20) & 0xF, x+2)
                            writeSpritePixelBlend((pixel32 >> 16) & 0xF, x+3)
                            writeSpritePixelBlend((pixel32 >> 12) & 0xF, x+4)
                            writeSpritePixelBlend((pixel32 >> 8) & 0xF, x+5)
                            writeSpritePixelBlend((pixel32 >> 4) & 0xF, x+6)
                            writeSpritePixelBlend(pixel32 & 0xF, x+7)
                        }
                        else
                        {
                            writeSpritePixelBlend(pixel32 & 0xF, x)
                            writeSpritePixelBlend((pixel32 >> 4) & 0xF, x+1)
                            writeSpritePixelBlend((pixel32 >> 8) & 0xF, x+2)
                            writeSpritePixelBlend((pixel32 >> 12) & 0xF, x+3)
                            writeSpritePixelBlend((pixel32 >> 16) & 0xF, x+4)
                            writeSpritePixelBlend((pixel32 >> 20) & 0xF, x+5)
                            writeSpritePixelBlend((pixel32 >> 24) & 0xF, x+6)
                            writeSpritePixelBlend(pixel32 >> 28, x+7)
                        }
                        #undef writeSpritePixelBlend
                    }
                    else //None
                    {
                        #define writeSpritePixel(pixel, x) \
                            if (pixel) { \
                                pixels[x] = palette[pixel] | (1 << 15); \
                                scanline->bgMask[x] = (1 << 4); \
                            }

                        if (flipX)
                        {
                            writeSpritePixel(pixel32 >> 28, x)
                            writeSpritePixel((pixel32 >> 24) & 0xF, x+1)
                            writeSpritePixel((pixel32 >> 20) & 0xF, x+2)
                            writeSpritePixel((pixel32 >> 16) & 0xF, x+3)
                            writeSpritePixel((pixel32 >> 12) & 0xF, x+4)
                            writeSpritePixel((pixel32 >> 8) & 0xF, x+5)
                            writeSpritePixel((pixel32 >> 4) & 0xF, x+6)
                            writeSpritePixel(pixel32 & 0xF, x+7)
                        }
                        else
                        {
                            writeSpritePixel(pixel32 & 0xF, x)
                            writeSpritePixel((pixel32 >> 4) & 0xF, x+1)
                            writeSpritePixel((pixel32 >> 8) & 0xF, x+2)
                            writeSpritePixel((pixel32 >> 12) & 0xF, x+3)
                            writeSpritePixel((pixel32 >> 16) & 0xF, x+4)
                            writeSpritePixel((pixel32 >> 20) & 0xF, x+5)
                            writeSpritePixel((pixel32 >> 24) & 0xF, x+6)
                            writeSpritePixel(pixel32 >> 28, x+7)
                        }
                        #undef writeSpritePixel
                    }
                }
            }
            else //handle tiles that are partially cut off screen, for the sake of readibility the writeSpritePixel handles all special effect spaces, should affect performance very little
            {
                bool winShouldDraw = true;
                #define writeSpritePixel(pixel, x) \
                    winShouldDraw = windowsEnabled == false || ((IsInsideWinIn && scanline->winMask[x] & WINMASK_OBJ) || (!IsInsideWinIn && REG_WINOUT & WINOUT_WIN01_OBJ));\
                    if (pixel && winShouldDraw) { \
                        uint16_t color = palette[pixel]; \
                        \
                        winShouldBlendPixel = windowsEnabled == false || ( (IsInsideWinIn && scanline->winMask[x] & WINMASK_CLR) || (!IsInsideWinIn && REG_WINOUT & WINOUT_WIN01_CLR) ); \
                        if ((blendMode == 1 && REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel) || isSemiTransparent) \
                        { \
                            if (scanline->bgMask[x] & (REG_BLDCNT >> 8)) \
                                color = alphaBlendColor(color, pixels[x]); \
                        } \
                        else if(REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel) \
                        { \
                            switch (blendMode) \
                            { \
                                case 2: \
                                    color = alphaBrightnessIncrease(color); \
                                    break; \
                                case 3: \
                                    color = alphaBrightnessDecrease(color); \
                                    break; \
                            } \
                        } \
                        \
                        pixels[x] = color | (1 << 15); \
                        scanline->bgMask[x] = (1 << 4); \
                    }

                if (x > sObjXMin - TILE_WIDTH && x < sObjXMin) //left side
                {
                    int32_t pixelOffset = sObjXMin - x;
                    int32_t amountOfPixelsToBeDrawn = TILE_WIDTH - pixelOffset;
                    if (flipX)
                    {
                        for (int32_t i = 0; i < amountOfPixelsToBeDrawn; i++)
                        {
                            writeSpritePixel((pixel32 >> (28-((pixelOffset+i)*4))) & 0xF, sObjXMin+i);
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < amountOfPixelsToBeDrawn; i++)
                        {
                            writeSpritePixel((pixel32 >> ((pixelOffset+i)*4)) & 0xF, sObjXMin+i);
                        }
                    }
                }
                else if(x < sObjXMax && x + TILE_WIDTH > sObjXMax) //right side
                {
                    int32_t amountOfPixelsToBeDrawn = sObjXMax-x;
                    if (flipX)
                    {
                        for (int32_t i = 0; i < amountOfPixelsToBeDrawn; i++)
                        {
                            writeSpritePixel((pixel32 >> (28-(i*4))) & 0xF, x+i);
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < amountOfPixelsToBeDrawn; i++)
                        {
                            writeSpritePixel((pixel32 >> i*4) & 0xF, x+i);
                        }
                    }
                }
                #undef writeSpritePixel
            }
            x += TILE_WIDTH; //Move on onto the next block
        }
    }
}

// OAM only changes during VBlank, so the sprites that can appear on each priority are
// collected once per frame instead of scanning all 128 entries for every priority of every
// scanline. Kept in the original drawing order. The visibility test matches
// DrawNonAffineSprite.
struct SpriteList
{
    uint8_t count;
    uint8_t index[128];
    bool8 complex[128];
    int16_t top[128];
    int16_t bottom[128];
};

static struct SpriteList sSpriteLists[4];

static void BuildSpriteLists(void)
{
    int32_t i;

    for (i = 0; i < 4; i++)
        sSpriteLists[i].count = 0;

    for (i = 127; i >= 0; i--)
    {
        struct OamData *oam = &((struct OamData *)OAM)[i];
        struct SpriteList *list;
        int32_t top, bottom, height;
        bool8 complex;

        if (oam->objMode == 2)
            continue;
        if (!(oam->affineMode & 1) && (oam->affineMode & 2)) // disabled
            continue;
        if (oam->shape == 3) // prohibited
            continue;

        complex = (oam->affineMode & 1) || (oam->bpp & 1) || oam->mosaic == 1;
        if (complex)
        {
            // Affine/8bpp/mosaic sprites keep doing their own full bounds test
            top = -32768;
            bottom = 32767;
        }
        else
        {
            if (oam->shape == 0)
                height = (1 << oam->size) * 8;
            else if (oam->shape == 1)
                height = spriteSizes[oam->size][0];
            else
                height = spriteSizes[oam->size][1];
            top = oam->y;
            if (top >= DISPLAY_HEIGHT)
                top -= 256;
            bottom = top + height;
            if (bottom <= 0 || top >= DISPLAY_HEIGHT)
                continue;
        }

        list = &sSpriteLists[oam->priority];
        list->index[list->count] = i;
        list->complex[list->count] = complex;
        list->top[list->count] = top;
        list->bottom[list->count] = bottom;
        list->count++;
    }
}

// Parts of this code heavily borrowed from NanoboyAdvance.
extern u32 gScreenOam[4];
static void DrawSprites(struct scanlineData* scanline, uint16_t vcount, bool windowsEnabled, uint8_t priority, uint16_t* pixels, bool IsInsideWinIn)
{
    int32_t SpriteIndex;
    
    if (windowsEnabled == true && IsInsideWinIn == false)
    {
        if (!(REG_WINOUT & WINOUT_WIN01_OBJ))
            return;
    }

    if (!(REG_DISPCNT & (1 << 6)))
    {
        puts("2-D OBJ Character mapping not supported.");
    }

    struct SpriteList *list = &sSpriteLists[priority];

    // Sprites use GBA x coordinates, clipped to [sObjXMin, sObjXMax).
    scanline->winMask = scanline->winMask0;
    scanline->bgMask = scanline->bgMask0;
    for (int32_t i = 0; i < list->count; i++)
    {
        if ((int32_t)vcount < list->top[i] || (int32_t)vcount >= list->bottom[i])
            continue;
        SpriteIndex = list->index[i];
        if (gFastDrawScreenObjOnly && !(gScreenOam[SpriteIndex >> 5] & (1u << (SpriteIndex & 31))))
            continue;
        if (list->complex[i])
            DrawAffineSprite(SpriteIndex, scanline, vcount, windowsEnabled, pixels, IsInsideWinIn);
        else
            DrawNonAffineSprite(SpriteIndex, scanline, vcount, windowsEnabled, pixels, IsInsideWinIn);
    }
}

// What SetupBgLayer returns for a text BG: 0 (centred), WIDE_BG_WIDE, or these flags.
#define EDGE_LEFT  (1 << 4) // repeat the screen's first 8 pixels into the left margin
#define EDGE_RIGHT (1 << 5) // repeat its last 8 pixels into the right margin

// Tilemap entry a text BG shows at GBA pixel (x, vcount), from sLayerMap when set.
static uint16_t BgMapEntryAt(uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t vcount, int32_t x)
{
    uint32_t mapWidthInPixels = bgMapSizes[control >> 14][0] * 8;
    uint32_t mapHeightInPixels = bgMapSizes[control >> 14][1] * 8;
    uint16_t *bgmap = sLayerMap != NULL ? sLayerMap : (uint16_t *)BG_SCREEN_ADDR((control >> 8) & 0x1F);
    uint32_t xx = (hoffs + x) & (mapWidthInPixels - 1);
    uint32_t yy = (vcount + voffs) & (mapHeightInPixels - 1);
    uint32_t index = ((yy & 0xFF) / 8) * 32 + (xx & 0xFF) / 8;

    if (xx > 255)
        index += 0x400;
    if (yy > 255)
        index += mapWidthInPixels > 256 ? 0x800 : 0x400;
    return bgmap[index];
}

// Whether the tile a text BG shows at GBA pixel (x, vcount) has any non-transparent pixel on
// that pixel row.
static bool BgHasPixelsAt(uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t vcount, int32_t x)
{
    uint16_t entry = BgMapEntryAt(control, hoffs, voffs, vcount, x);
    uint32_t row = (vcount + voffs) & 7;
    uint8_t *tiles = (uint8_t *)BG_CHAR_ADDR((control >> 2) & 3);

    if (entry & (1 << 11))
        row = 7 - row;
    if (control & BGCNT_256COLOR)
        return *(uint32_t *)&tiles[(entry & 0x3FF) * 64 + row * 8] != 0
            || *(uint32_t *)&tiles[(entry & 0x3FF) * 64 + row * 8 + 4] != 0;
    return *(uint32_t *)&tiles[(entry & 0x3FF) * 32 + row * 4] != 0;
}

// Whether the screen edge of a text BG at GBA pixel x (0 or DISPLAY_WIDTH - 8) can be repeated
// into the margin: its tile equals the next one inwards (a repeating background), or its row of
// pixels is a single colour (plain or striped). Frames, text and pictures are neither.
static bool BgEdgeRepeats(uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t vcount, int32_t x)
{
    int32_t inner = x == 0 ? 8 : x - 8;
    uint16_t entry = BgMapEntryAt(control, hoffs, voffs, vcount, x);
    uint32_t row = (vcount + voffs) & 7;
    uint8_t *tiles = (uint8_t *)BG_CHAR_ADDR((control >> 2) & 3);

    if (entry == BgMapEntryAt(control, hoffs, voffs, vcount, inner)
     && entry == BgMapEntryAt(control, hoffs, voffs, vcount, x + 7))
        return true;
    if (entry != BgMapEntryAt(control, hoffs, voffs, vcount, x + 7))
        return false; // the 8 pixels span two tiles
    if (entry & (1 << 11))
        row = 7 - row;
    if (control & BGCNT_256COLOR)
    {
        uint8_t *p = &tiles[(entry & 0x3FF) * 64 + row * 8];
        for (int32_t i = 1; i < 8; i++)
        {
            if (p[i] != p[0])
                return false;
        }
        return true;
    }
    else
    {
        uint32_t pixels = *(uint32_t *)&tiles[(entry & 0x3FF) * 32 + row * 4];
        return pixels == (pixels & 0xF) * 0x11111111u;
    }
}

// Prepares drawing BG bgnum (a text BG unless rotScale) and returns how it fills the widescreen
// margins (see DrawTextBg). A BG with a RAM tilemap (field map layers, battle text
// box) is drawn from it. The layer state is left set for drawing the GBA's 240 pixels, which is
// what the rotation/scaling BGs use.
static int32_t SetupBgLayer(struct scanlineData *scanline, int32_t bgnum, bool rotScale, uint16_t *control,
                        uint16_t hoffs, uint16_t voffs, int32_t vcount)
{
    int32_t how = 0;

    sLayerMap = NULL;
    sLayerX0 = 0;
    sLayerWidth = DISPLAY_WIDTH;
    scanline->winMask = scanline->winMask0;
    scanline->bgMask = scanline->bgMask0;
#ifdef FAST_DRAW_PARALLEL
    if (!rotScale)
    {
        how = gFastDrawWide.bgSpan[bgnum];
        if (gFastDrawWide.bgMaps[bgnum] != NULL)
        {
            if (((*control >> 8) & 0x1F) == gFastDrawWide.bgMapBase[bgnum])
            {
                sLayerMap = gFastDrawWide.bgMaps[bgnum];
                *control = (*control & 0x3FFF) | (gFastDrawWide.bgMapSize[bgnum] << 14);
            }
            else if (how == WIDE_BG_WIDE)
            {
                how = WIDE_BG_EDGES; // its 32x32 VRAM map would wrap into the margins
            }
        }
        if (how == WIDE_BG_EDGE_TILE)
            return EDGE_LEFT | EDGE_RIGHT; // a still stage picture: its edge tile, always
        if (how == WIDE_BG_AUTO)
        {
            // Art in the 16 map pixels the GBA never shows on the right means the map continues
            // past the screen (a looping scene); empty columns mean it doesn't.
            // Without it, the edges are repeated unconditionally: in a full-screen picture a
            // layer that left its margin empty would show the layers behind it.
            if (BgHasPixelsAt(*control, hoffs, voffs, vcount, DISPLAY_WIDTH)
             || BgHasPixelsAt(*control, hoffs, voffs, vcount, DISPLAY_WIDTH + 8))
                return WIDE_BG_WIDE;
            return EDGE_LEFT | EDGE_RIGHT;
        }
        if (how == WIDE_BG_FILL)
        {
            // Repeat an edge that is background; the pattern tile fills the other margins.
            how = WIDE_BG_FILL;
            if (BgEdgeRepeats(*control, hoffs, voffs, vcount, 0))
                how |= EDGE_LEFT;
            if (BgEdgeRepeats(*control, hoffs, voffs, vcount, DISPLAY_WIDTH - 8))
                how |= EDGE_RIGHT;
        }
        else if (how == WIDE_BG_EDGES)
        {
            // Only repeat an edge that belongs to a background; frames, text and pictures leave
            // the margin to the BGs behind them.
            how = 0;
            if (BgEdgeRepeats(*control, hoffs, voffs, vcount, 0))
                how |= EDGE_LEFT;
            if (BgEdgeRepeats(*control, hoffs, voffs, vcount, DISPLAY_WIDTH - 8))
                how |= EDGE_RIGHT;
        }
    }
#endif
    return how;
}

typedef void (*TextBgRenderer)(int32_t bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int32_t lineNum,
                               uint16_t *line, struct scanlineData *scanline, bool windowsEnabled);

// Draws a text BG over [x, x + width) (GBA x, width a multiple of 8), showing what the GBA
// would show at srcX.
static void DrawTextBgSpan(TextBgRenderer render, int32_t bgnum, uint16_t control, uint16_t hoffs, uint16_t voffs,
                           int32_t vcount, uint16_t *pixels, struct scanlineData *scanline, bool windowsEnabled,
                           int32_t x, int32_t width, int32_t srcX)
{
    sLayerX0 = srcX;
    sLayerWidth = width;
    scanline->winMask = scanline->winMask0 + x;
    scanline->bgMask = scanline->bgMask0 + x;
    render(bgnum, control, hoffs + srcX, voffs, vcount, pixels + x, scanline, windowsEnabled);
}

// Draws a text BG: over the GBA's 240 pixels, then the widescreen margins as `how` says. WIDE
// draws 288 pixels of the map (starting 24 pixels left, so the scroll moves 24 pixels left too);
// EDGE_LEFT / EDGE_RIGHT repeat the screen's first / last 8 pixels into that margin.
static void DrawTextBg(TextBgRenderer render, int32_t how, int32_t bgnum, uint16_t control, uint16_t hoffs, uint16_t voffs,
                       int32_t vcount, uint16_t *pixels, struct scanlineData *scanline, bool windowsEnabled)
{
#ifdef FAST_DRAW_PARALLEL
    int32_t x;

    if (how == WIDE_BG_WIDE)
    {
        DrawTextBgSpan(render, bgnum, control, hoffs, voffs, vcount, pixels, scanline, windowsEnabled,
                       -WIDESCREEN_MARGIN, WIDESCREEN_WIDTH, -WIDESCREEN_MARGIN);
        return;
    }
    if ((how & 0xF) == WIDE_BG_FILL && gFastDrawWide.bgFillMaps[bgnum] != NULL)
    {
        // Margins that don't repeat the edge: from a map of the background's pattern tile,
        // aligned with the screen.
        uint16_t *map = sLayerMap;

        sLayerMap = gFastDrawWide.bgFillMaps[bgnum];
        if (!(how & EDGE_LEFT))
            DrawTextBgSpan(render, bgnum, control & 0x3FFF, hoffs, voffs, vcount, pixels, scanline, windowsEnabled,
                           -WIDESCREEN_MARGIN, WIDESCREEN_MARGIN, -WIDESCREEN_MARGIN);
        if (!(how & EDGE_RIGHT))
            DrawTextBgSpan(render, bgnum, control & 0x3FFF, hoffs, voffs, vcount, pixels, scanline, windowsEnabled,
                           DISPLAY_WIDTH, WIDESCREEN_MARGIN, DISPLAY_WIDTH);
        sLayerMap = map;
    }
    if (how & EDGE_LEFT)
    {
        for (x = -WIDESCREEN_MARGIN; x < 0; x += 8)
            DrawTextBgSpan(render, bgnum, control, hoffs, voffs, vcount, pixels, scanline, windowsEnabled, x, 8, 0);
    }
    if (how & EDGE_RIGHT)
    {
        for (x = DISPLAY_WIDTH; x < DISPLAY_WIDTH + WIDESCREEN_MARGIN; x += 8)
            DrawTextBgSpan(render, bgnum, control, hoffs, voffs, vcount, pixels, scanline, windowsEnabled,
                           x, 8, DISPLAY_WIDTH - 8);
    }
#endif
    DrawTextBgSpan(render, bgnum, control, hoffs, voffs, vcount, pixels, scanline, windowsEnabled, 0, DISPLAY_WIDTH, 0);
}

static void DrawScanline(uint16_t *pixels, uint16_t vcount)
{
    uint32_t mode = REG_DISPCNT & 3;
    uint8_t numOfBgs = (mode == 0 ? 4 : 3);
    int32_t bgnum, prnum;
    struct scanlineData scanline;
    uint32_t blendMode = (REG_BLDCNT >> 6) & 3;
    int32_t xpos;
    // Pixels this line covers, in GBA x: [0, 240), or [-24, 264) on a widescreen field frame.
    int32_t lineXMin = FRAME_EXTEND ? -FRAME_LINE_X0 : 0;
    int32_t lineXMax = DISPLAY_WIDTH - lineXMin;
    uint16_t winMaskStore[FRAME_LINE_WIDTH], bgMaskStore[FRAME_LINE_WIDTH];

    scanline.winMask0 = scanline.winMask = winMaskStore + FRAME_LINE_X0;
    scanline.bgMask0 = scanline.bgMask = bgMaskStore + FRAME_LINE_X0;
#ifdef FAST_DRAW_PARALLEL
    sObjXMin = gFastDrawWide.extendObj ? lineXMin : 0;
    sObjXMax = gFastDrawWide.extendObj ? lineXMax : DISPLAY_WIDTH;
#endif


    //initialize all priority bookkeeping data
    memset(scanline.prioritySortedBgsCount, 0, sizeof(scanline.prioritySortedBgsCount));
    
    //windows code!!!
    bool windowsEnabled = false;
    bool IsInsideWinIn;
    uint16_t WIN0bottom, WIN0top, WIN0right, WIN0left;
    uint16_t WIN1bottom, WIN1top, WIN1right, WIN1left;
    bool WIN0enable, WIN1enable;
    WIN0enable = false;
    WIN1enable = false;

    //figure out if WIN0 masks on this scanline
    if (REG_DISPCNT & DISPCNT_WIN0_ON)
    {
        //acquire the window coordinates
        WIN0bottom = (REG_WIN0V & 0xFF); //y2;
        WIN0top = (REG_WIN0V & 0xFF00) >> 8; //y1;
        WIN0right = (REG_WIN0H & 0xFF); //x2
        WIN0left = (REG_WIN0H & 0xFF00) >> 8; //x1
        
        //figure out WIN Y wraparound and check bounds accordingly
        if (WIN0top > WIN0bottom) {
            if (vcount >= WIN0top || vcount < WIN0bottom)
                WIN0enable = true;
        } else {
            if (vcount >= WIN0top && vcount < WIN0bottom)
                WIN0enable = true;
        }
        
        windowsEnabled = true;
    }
    //figure out if WIN1 masks on this scanline
    if (REG_DISPCNT & DISPCNT_WIN1_ON)
    {
        WIN1bottom = (REG_WIN0V & 0xFF); //y2;
        WIN1top = (REG_WIN0V & 0xFF00) >> 8; //y1;
        WIN1right = (REG_WIN0H & 0xFF); //x2
        WIN1left = (REG_WIN0H & 0xFF00) >> 8; //x1
        
        if (WIN1top > WIN1bottom) {
            if (vcount >= WIN1top || vcount < WIN1bottom)
                WIN1enable = true;
        } else {
            if (vcount >= WIN1top && vcount < WIN1bottom)
                WIN1enable = true;
        }
        
        windowsEnabled = true;
    }
    //enable windows if OBJwin is enabled
    if (REG_DISPCNT & DISPCNT_OBJWIN_ON && REG_DISPCNT & DISPCNT_OBJ_ON)
        windowsEnabled = true;
    
    IsInsideWinIn = (WIN0enable || WIN1enable || (REG_DISPCNT & DISPCNT_OBJWIN_ON && REG_DISPCNT & DISPCNT_OBJ_ON));
    
    
    //draw to pixel mask (same priority as the GBA: WIN0 over WIN1 over outside)
    if (IsInsideWinIn)
    {
        uint16_t outside = (REG_WINOUT & 0x3F) | WINMASK_WINOUT;
        for (xpos = 0; xpos < DISPLAY_WIDTH; xpos++)
            scanline.winMask[xpos] = outside;
        if (WIN1enable)
            FillWinRange(scanline.winMask, WIN1left, WIN1right, (REG_WININ >> 8) & 0x3F);
        if (WIN0enable)
            FillWinRange(scanline.winMask, WIN0left, WIN0right, REG_WININ & 0x3F);
        // Window coordinates only cover the GBA screen: each margin repeats the window state of
        // the screen edge next to it (a full-width window covers it, a centred one doesn't).
        for (xpos = lineXMin; xpos < 0; xpos++)
            scanline.winMask[xpos] = scanline.winMask[0];
        for (xpos = DISPLAY_WIDTH; xpos < lineXMax; xpos++)
            scanline.winMask[xpos] = scanline.winMask[DISPLAY_WIDTH - 1];
    }

    //draw to window mask if OBJwin is enabled
    if (REG_DISPCNT & DISPCNT_OBJWIN_ON && REG_DISPCNT & DISPCNT_OBJ_ON)
        DrawSpritesWinMask(&scanline, vcount);

    // Which BGs the window enables on every pixel / on at least one pixel, so a BG that the
    // window fully enables can skip the per-pixel test and one it fully hides is skipped.
    uint16_t winAnd = 0xFFFF, winOr = 0;
    if (IsInsideWinIn)
    {
        for (xpos = lineXMin; xpos < lineXMax; xpos++)
        {
            winAnd &= scanline.winMask[xpos];
            winOr |= scanline.winMask[xpos];
        }
    }

    //init bgmask for alpha blending
    for (int32_t i = lineXMin; i < lineXMax; i++)
    {
        scanline.bgMask[i] = 0x20; //backdrop bit
    }

    for (bgnum = 0; bgnum < numOfBgs; bgnum++)
    {
        uint16_t bgcnt = *(uint16_t*)(REG_ADDR_BG0CNT + bgnum * 2);
        uint16_t priority;
        scanline.bgcnts[bgnum] = bgcnt;
        scanline.bgtoprio[bgnum] = priority = (bgcnt & 3);
        
        uint8_t priorityCount = scanline.prioritySortedBgsCount[priority];
        scanline.prioritySortedBgs[priority][priorityCount] = bgnum;
        scanline.prioritySortedBgsCount[priority]++;
    }
    
    switch (mode)
    {
    case 0:
        for (prnum = 3; prnum >= 0; prnum--)
        {
            for (int32_t prsub = scanline.prioritySortedBgsCount[prnum] - 1; prsub >= 0; prsub--)
            {
                bgnum = scanline.prioritySortedBgs[prnum][prsub];
                if (isbgEnabled(bgnum))
                {
                    uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                    uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                    uint16_t bgcnt = scanline.bgcnts[bgnum];
                    int32_t how = SetupBgLayer(&scanline, bgnum, mode == 1 && bgnum == 2, &bgcnt, bghoffs, bgvoffs, vcount);
                    bool doesBGblend = (blendMode != 0 && REG_BLDCNT & (1 << bgnum));
                    
                    if (IsInsideWinIn)
                    {
                        //blending check
                        if (!(winOr & (1 << bgnum)))
                            ; // the window hides this BG on the whole line
                        else if (doesBGblend)
                            DrawTextBg(RenderBGScanlineWinBlend, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                        else if (winAnd & (1 << bgnum))
                            DrawTextBg(RenderBGScanlineNoEffect, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, false); // window enables this BG on every pixel
                        else
                            DrawTextBg(RenderBGScanlineWin, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);

                    } else {

                        if (doesBGblend && (windowsEnabled == false || (REG_WINOUT & WINOUT_WIN01_CLR)) )
                            DrawTextBg(RenderBGScanlineBlend, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                        else
                            DrawTextBg(RenderBGScanlineNoEffect, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                    }
                
                }
            }
            if (REG_DISPCNT & DISPCNT_OBJ_ON)
                DrawSprites(&scanline, vcount, windowsEnabled, prnum, pixels, IsInsideWinIn);
        }
        
        break;
    case 1:
        for (prnum = 3; prnum >= 0; prnum--)
        {
            for (int32_t prsub = scanline.prioritySortedBgsCount[prnum] - 1; prsub >= 0; prsub--)
            {
                bgnum = scanline.prioritySortedBgs[prnum][prsub];
                if (isbgEnabled(bgnum))
                {
                    uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                    uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                    uint16_t bgcnt = scanline.bgcnts[bgnum];
                    int32_t how = SetupBgLayer(&scanline, bgnum, mode == 1 && bgnum == 2, &bgcnt, bghoffs, bgvoffs, vcount);
                    bool doesBGblend = (blendMode != 0 && REG_BLDCNT & (1 << bgnum));
                    
                    
                    if (bgnum != 2)
                    {
                        if (IsInsideWinIn)
                        {
                            //blending check
                            if (!(winOr & (1 << bgnum)))
                                ; // the window hides this BG on the whole line
                            else if (doesBGblend)
                                DrawTextBg(RenderBGScanlineWinBlend, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            else if (winAnd & (1 << bgnum))
                                DrawTextBg(RenderBGScanlineNoEffect, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, false); // window enables this BG on every pixel
                            else
                                DrawTextBg(RenderBGScanlineWin, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            
                        } else {
                            
                            if (doesBGblend)
                                DrawTextBg(RenderBGScanlineBlend, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            else
                                DrawTextBg(RenderBGScanlineNoEffect, how, bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                        }
                    }
                    else
                    {
                        if (IsInsideWinIn)
                        {
                            //blending check
                            if (doesBGblend)
                                RenderRotScaleBGScanlineWinBlend(bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            else
                                RenderRotScaleBGScanlineWin(bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            
                        } else {
                            
                            if (doesBGblend && (windowsEnabled == false || (REG_WINOUT & WINOUT_WIN01_CLR)))
                                RenderRotScaleBGScanlineBlend(bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                            else
                                RenderRotScaleBGScanlineNoEffect(bgnum, bgcnt, bghoffs, bgvoffs, vcount, pixels, &scanline, windowsEnabled);
                        }
                    }
                }
            }
            if (REG_DISPCNT & DISPCNT_OBJ_ON)
                DrawSprites(&scanline, vcount, windowsEnabled, prnum, pixels, IsInsideWinIn);
        }
        break;
    default:
        DBGPRINTF("Video mode %u is unsupported.\n", mode);
        break;
    }
}

uint16_t *memsetu16(uint16_t *dst, uint16_t fill, size_t count)
{
    uint16_t *const start = dst;
    for (int32_t i = 0; i < count; i++)
    {
        *dst++ = fill;
    }
    return start;
}

uint32_t frameskipcounter = 0;

// Backdrop and scanline for line i, using whatever REG_BASE / PLTT currently refer to.
static void DrawLine(uint16_t *pixels, int32_t i)
{
    uint16_t backdropColor;
    if (gFastDrawBackdropOverride >= 0)
    {
        // When an explicit backdrop chromaKey is provided (e.g. Pass 6 UI overlay),
        // consume it directly without reading PLTT[0] or applying brightness/blending.
        backdropColor = (uint16_t)gFastDrawBackdropOverride;
    }
    else
    {
        // backdrop color brightness effects
        uint32_t blendMode = (REG_BLDCNT >> 6) & 3;
        backdropColor = *(uint16_t *)PLTT;
        if (REG_BLDCNT & BLDCNT_TGT1_BD)
        {
            switch (blendMode)
            {
            case 2:
                backdropColor = alphaBrightnessIncrease(backdropColor);
                break;
            case 3:
                backdropColor = alphaBrightnessDecrease(backdropColor);
                break;
            }
        }
    }

    uint16_t *line = &pixels[i * FRAME_LINE_WIDTH];
#ifdef FAST_DRAW_PARALLEL
    if (gFastDrawWide.extend)
    {
        memsetu16(line, backdropColor, FRAME_LINE_WIDTH);
    }
    else
    {
        // Widescreen output of a scene that stays 240 px wide:
        // When rendering for chromaKey overlay (gFastDrawBackdropOverride >= 0), margins MUST be
        // filled with backdropColor, never 0 (which would be detected as opaque black UI pixels)!
        const uint16_t marginColor = (gFastDrawBackdropOverride >= 0) ? backdropColor : 0;
        if (gFastDrawWide.output)
        {
            memsetu16(line, marginColor, FRAME_LINE_X0);
            memsetu16(line + FRAME_LINE_X0 + DISPLAY_WIDTH, marginColor, FRAME_LINE_X0);
        }
        memsetu16(line + FRAME_LINE_X0, backdropColor, DISPLAY_WIDTH);
    }
#else
    memsetu16(line, backdropColor, DISPLAY_WIDTH);
#endif
    DrawScanline(line + FRAME_LINE_X0, i);
}

#ifdef FAST_DRAW_PARALLEL
// Converts finished line i to BGR565 into the locked SDL texture. The Vita SDL renderer has no
// native 1555 format: an ABGR1555 texture made SDL convert every pixel to ABGR8888 in software
// on each update (~9 ms per frame on hardware). BGR565 is native, so each worker converts and
// writes its own lines here with NEON, 8 pixels at a time, in parallel and off the main thread.
// GBA BGR555 (R bits 0-4, G 5-9, B 10-14) -> RGB565 (R 11-15, G 5-10, B 0-4) for OpenGL / vitaGL;
// green is widened to 6 bits by repeating its top bit, so 0 and 31 map to 0 and 63.
static void CopyLineToTexture(const uint16_t *pixels, int32_t i)
{
    const uint16_t *src;
    uint16_t *dst;
    const uint16x8_t mask5 = vdupq_n_u16(0x1F);

    int32_t x0 = gFastDrawWide.output ? 0 : FRAME_LINE_X0;
    int32_t x1 = FRAME_LINE_WIDTH - x0;

    if (gFastDrawTexture == NULL)
        return;
    src = &pixels[i * FRAME_LINE_WIDTH];
    dst = (uint16_t *)((uint8_t *)gFastDrawTexture + i * gFastDrawTexturePitch);
    // All 288 pixels with widescreen, else the middle 240. 24, 240 and 288 are multiples of 8.
    for (int32_t x = x0; x < x1; x += 8)
    {
        uint16x8_t c = vld1q_u16(src + x);
        uint16x8_t r = vandq_u16(c, mask5);
        uint16x8_t g = vandq_u16(vshrq_n_u16(c, 5), mask5);
        uint16x8_t b = vandq_u16(vshrq_n_u16(c, 10), mask5);
        uint16x8_t g6 = vorrq_u16(vshlq_n_u16(g, 1), vshrq_n_u16(g, 4));
        vst1q_u16(dst + x, vorrq_u16(vorrq_u16(vshlq_n_u16(r, 11), vshlq_n_u16(g6, 5)), b));
    }
}

// Draws lines first, first+step, ... from their captures (one copy of this per core).
void FAST_DRAW_LINES(uint16_t *pixels, int32_t first, int32_t step)
{
    BuildSpriteLists();
    for (int32_t i = first; i < DISPLAY_HEIGHT; i += step)
    {
        while (SDL_AtomicGet(&gFastDrawLinesCaptured) <= i)
            ; // the main thread is still capturing this line (a few microseconds per line)
        sLineRegs = gFastDrawRegCaps[i];
        sLinePltt = gFastDrawPlttCaps[i];
        DrawLine(pixels, i);
        CopyLineToTexture(pixels, i);
    }
    sLineRegs = sLiveRegs;
    sLinePltt = sLivePltt;
}
#endif

#ifndef FAST_DRAW_SECONDARY
// Per-scanline hardware events before the line is drawn: VCount match interrupt.
static void StartLine(int32_t i)
{
    REG_VCOUNT = i;
    if(((REG_DISPSTAT >> 8) & 0xFF) == REG_VCOUNT)
    {
        REG_DISPSTAT |= INTR_FLAG_VCOUNT;
#ifndef PLATFORM_VITA
        // On single-threaded PC ports, StartLine drives VCount interrupt (m4aSoundVSync).
        // On PS Vita, Core 1 exclusively owns all hardware events and audio mixing via GbaEngine_StepDirectHardwareEvents.
        if (REG_IE & INTR_FLAG_VCOUNT)
            gIntrTable[0]();
#endif
    }
}

// Per-scanline hardware events after the line is drawn: HBlank DMAs and interrupt.
static void EndLine(void)
{
#ifndef PLATFORM_VITA
    REG_DISPSTAT |= INTR_FLAG_HBLANK;

    RunDMAs(DMA_HBLANK);

    if (REG_DISPSTAT & DISPSTAT_HBLANK_INTR)
        gIntrTable[3]();

    REG_DISPSTAT &= ~INTR_FLAG_HBLANK;
    REG_DISPSTAT &= ~INTR_FLAG_VCOUNT;
#endif
}

#ifndef FAST_DRAW_SECONDARY
int32_t gFastDrawBackdropOverride = -1;
bool8 gFastDrawScreenObjOnly = FALSE;
u16 gFastDrawDispcntClear = 0;
u16 gFastDrawBldcntClear = 0;
bool8 gFastDrawCentredOnly = FALSE;
#endif

#ifdef FAST_DRAW_PARALLEL
uint8_t gFastDrawRegCaps[DISPLAY_HEIGHT][CAPTURE_REGS_SIZE];
uint8_t *gFastDrawPlttCaps[DISPLAY_HEIGHT];
SDL_atomic_t gFastDrawLinesCaptured;
uint16_t *gFastDrawTexture;
int32_t gFastDrawTexturePitch;
static uint8_t sPlttStore[DISPLAY_HEIGHT][PLTT_SIZE];
static int32_t sPlttStoreCount;

// The main thread captures; the workers draw alternate lines.
struct DrawWorker
{
    const char *name;
    int32_t core;
    int32_t first;
    void (*drawLines)(uint16_t *pixels, int32_t first, int32_t step);
    SDL_sem *start;
    SDL_sem *done;
    double busyMs;
};
static struct DrawWorker sWorkers[DRAW_WORKERS] = {
    {"DrawWorker1", 2, 0, FastDraw_DrawLinesB},
    {"DrawWorker2", 2, 1, FastDraw_DrawLinesC}, // Pinned to Core 2 so Core 1 remains 100% dedicated to AgbMain & 48 kHz audio
};
static uint16_t *sWorkerPixels;
static bool sWorkersReady, sWorkersFailed;

// Timing breakdown in debug.log, to see where the frame goes on hardware.
static double sStatCapture, sStatWait, sStatPltt;
static int32_t sStatFrames;

static double DrawNowMs(void)
{
    return SDL_GetPerformanceCounter() * 1000.0 / SDL_GetPerformanceFrequency();
}

// Byte offsets of DISPCNT and BLDCNT in the captured register block.
#define CAPTURE_DISPCNT_OFFSET 0x00
#define CAPTURE_BLDCNT_OFFSET 0x50

static void CaptureLine(int32_t i)
{
    memcpy(gFastDrawRegCaps[i], sLiveRegs, CAPTURE_REGS_SIZE);
    *(uint16_t *)&gFastDrawRegCaps[i][CAPTURE_DISPCNT_OFFSET] &= (uint16_t)~gFastDrawDispcntClear;
    *(uint16_t *)&gFastDrawRegCaps[i][CAPTURE_BLDCNT_OFFSET] &= (uint16_t)~gFastDrawBldcntClear;
    // Palettes rarely change between lines: share the previous line's copy unless it did.
    if (i == 0 || memcmp(gFastDrawPlttCaps[i - 1], sLivePltt, PLTT_SIZE) != 0)
    {
        if (i == 0)
            sPlttStoreCount = 0;
        memcpy(sPlttStore[sPlttStoreCount], sLivePltt, PLTT_SIZE);
        gFastDrawPlttCaps[i] = sPlttStore[sPlttStoreCount++];
    }
    else
    {
        gFastDrawPlttCaps[i] = gFastDrawPlttCaps[i - 1];
    }
}

static int32_t DrawWorkerMain(void *data)
{
    struct DrawWorker *worker = data;

    VitaPlatform_PinCurrentThread(worker->name, worker->core);
    for (;;)
    {
        SDL_SemWait(worker->start);
        double t0 = DrawNowMs();
        worker->drawLines(sWorkerPixels, worker->first, DRAW_WORKERS);
        worker->busyMs += DrawNowMs() - t0;
        SDL_SemPost(worker->done);
    }
    return 0;
}

static bool StartWorkers(void)
{
    int32_t i;

    if (sWorkersReady || sWorkersFailed)
        return sWorkersReady;
    for (i = 0; i < DRAW_WORKERS; i++)
    {
        sWorkers[i].start = SDL_CreateSemaphore(0);
        sWorkers[i].done = SDL_CreateSemaphore(0);
        if (sWorkers[i].start == NULL || sWorkers[i].done == NULL
         || SDL_CreateThreadWithStackSize(DrawWorkerMain, sWorkers[i].name, 256 * 1024, &sWorkers[i]) == NULL)
        {
            sWorkersFailed = true;
            break;
        }
    }
    // A worker that did start never gets work if a later one failed: drawing falls back to one core.
    sWorkersReady = !sWorkersFailed;
    VITA_LOG("parallel draw: %s (%d workers)\n", sWorkersReady ? "on" : SDL_GetError(), DRAW_WORKERS);
    return sWorkersReady;
}

static void StartFrameWorkers(uint16_t *pixels)
{
    int32_t i;

    sWorkerPixels = pixels;
    for (i = 0; i < DRAW_WORKERS; i++)
        SDL_SemPost(sWorkers[i].start);
}

static void WaitFrameWorkers(void)
{
    int32_t i;

    for (i = 0; i < DRAW_WORKERS; i++)
        SDL_SemWait(sWorkers[i].done);
}

static void LogDrawStats(double captureMs, double waitMs)
{
    sStatCapture += captureMs;
    sStatWait += waitMs;
    sStatPltt += sPlttStoreCount;
    if (++sStatFrames < 600)
        return;
    // Suppressed: VITA_LOG to debug.log causes synchronous flash write spikes (~400ms) on Vita SD card.
    // Telemetry is captured in VitaPerf asynchronously.
    sStatCapture = sStatWait = sStatPltt = 0;
    sWorkers[0].busyMs = sWorkers[1].busyMs = 0;
    sStatFrames = 0;
}

#ifdef VITA_DRAW_SELFCHECK
// Every SELFCHECK_INTERVAL frames, draws the frame the original sequential way while
// capturing, then redraws it in parallel from the captures and compares the two images.
#define SELFCHECK_INTERVAL 30
static uint16_t sCheckPixels[FRAME_LINE_WIDTH * DISPLAY_HEIGHT];
static uint32_t sCheckFrames, sCheckMismatches;

static void DrawFrameSelfCheck(uint16_t *pixels)
{
    int32_t i, first = -1, last = -1;
    uint16_t *texture = gFastDrawTexture;

    // Reference: the original sequential order, shown on screen.
    BuildSpriteLists();
    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        StartLine(i);
        CaptureLine(i);
        DrawLine(pixels, i);
        CopyLineToTexture(pixels, i);
        EndLine();
    }
    // The same frame drawn by the workers from the captures, kept off screen.
    gFastDrawTexture = NULL;
    SDL_AtomicSet(&gFastDrawLinesCaptured, DISPLAY_HEIGHT);
    StartFrameWorkers(sCheckPixels);
    WaitFrameWorkers();
    gFastDrawTexture = texture;
    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        if (memcmp(&pixels[i * FRAME_LINE_WIDTH], &sCheckPixels[i * FRAME_LINE_WIDTH], FRAME_LINE_WIDTH * sizeof(uint16_t)) != 0)
        {
            if (first < 0)
                first = i;
            last = i;
        }
    }
    sCheckFrames++;
    if (first >= 0)
    {
        sCheckMismatches++;
        VITA_LOG("drawcheck MISMATCH in check %u: lines %d-%d\n", sCheckFrames, first, last);
    }
    if (sCheckFrames % 20 == 0)
        VITA_LOG("drawcheck: %u frames compared, %u mismatched\n", sCheckFrames, sCheckMismatches);
}
#endif // VITA_DRAW_SELFCHECK
#endif // FAST_DRAW_PARALLEL

void RunFrameLineEvents(void)
{
    int32_t i;

    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        StartLine(i);
        EndLine();
    }
}

// Direct event dispatch: triggers audio mixing (VCount match) and VBlank prep in < 0.001 ms without a 160-line loop
void GbaEngine_StepDirectHardwareEvents(void)
{
    // 1. VCount match interrupt: triggers m4aSoundVSync for 48 kHz audio mixing
    const uint16_t vcountTarget = (REG_DISPSTAT >> 8) & 0xFF;
    REG_VCOUNT = vcountTarget;
    if (((REG_DISPSTAT >> 8) & 0xFF) == REG_VCOUNT)
    {
        REG_DISPSTAT |= INTR_FLAG_VCOUNT;
        if (REG_IE & INTR_FLAG_VCOUNT)
            gIntrTable[0]();
        REG_DISPSTAT &= ~INTR_FLAG_VCOUNT;
    }

    // 2. Direct HBlank DMAs and interrupt
    REG_DISPSTAT |= INTR_FLAG_HBLANK;
    RunDMAs(DMA_HBLANK);
    if (REG_DISPSTAT & DISPSTAT_HBLANK_INTR)
        gIntrTable[3]();
    REG_DISPSTAT &= ~INTR_FLAG_HBLANK;

    // 3. Prep VCount for VBlank
    REG_VCOUNT = 160;
}

void DrawFrame(uint16_t *pixels)
{
    int32_t i;

#ifdef FAST_DRAW_PARALLEL
    Widescreen_PrepareDraw(); // before the workers start: they read gFastDrawWide
    if (StartWorkers())
    {
#ifdef VITA_DRAW_SELFCHECK
        static uint32_t sFrame;
        if (++sFrame % SELFCHECK_INTERVAL == 0)
        {
            DrawFrameSelfCheck(pixels);
            return;
        }
#endif
        // Run every line's hardware events in order, capturing the state each line sees.
        // The workers draw each line as soon as its capture is published.
        SDL_AtomicSet(&gFastDrawLinesCaptured, 0);
        StartFrameWorkers(pixels);
        double t0 = DrawNowMs();
        for (i = 0; i < DISPLAY_HEIGHT; i++)
        {
            StartLine(i);
            CaptureLine(i);
            SDL_AtomicSet(&gFastDrawLinesCaptured, i + 1);
            EndLine();
        }
        double t1 = DrawNowMs();
        WaitFrameWorkers();
        LogDrawStats(t1 - t0, DrawNowMs() - t1);
        return;
    }
#endif

    BuildSpriteLists();

    //memsetu16(pixels, *(uint16_t *)PLTT, DISPLAY_WIDTH * DISPLAY_HEIGHT);

    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        StartLine(i);
#ifdef FAST_DRAW_PARALLEL
        // Sequential fallback: draw from the line's capture too, so the layer masks apply.
        CaptureLine(i);
        sLineRegs = gFastDrawRegCaps[i];
        sLinePltt = gFastDrawPlttCaps[i];
#endif
        DrawLine(pixels, i);
#ifdef FAST_DRAW_PARALLEL
        sLineRegs = sLiveRegs;
        sLinePltt = sLivePltt;
        CopyLineToTexture(pixels, i);
#endif
        EndLine();
    }
}

#ifdef FAST_DRAW_PARALLEL
void DrawFrameToTexture(uint16_t *pixels, void *texture, int32_t pitch)
{
    gFastDrawTexture = texture;
    gFastDrawTexturePitch = pitch;
    DrawFrame(pixels);
    gFastDrawTexture = NULL;
}
#endif
#endif // !FAST_DRAW_SECONDARY
