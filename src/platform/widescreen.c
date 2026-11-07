// Vita widescreen: which layers of each frame fill the 24 px margins (see platform/widescreen.h).
#include <stdint.h>
#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "graphics.h"
#include "main.h"
#include "overworld.h"
#include "platform.h"
#include "platform/framedraw.h"
#include "scanline_effect.h"
#include "vita/vita_stage.h"

#ifdef WIDESCREEN_SUPPORTED
struct FastDrawWide gFastDrawWide;

// The main callback (gMain.callback2) the field / the battle ran from when it last called
// Widescreen_Set*Frame. A frame counts as a field or battle frame while that callback is still
// the current one. A flag set and cleared every frame isn't enough: some code waits for extra
// VBlanks without running the callback (IsDma3ManagerBusyWithBgCopy, used by menus), and those
// frames flashed black margins. The game thread is blocked while the main thread draws, so
// gMain, tilemaps and VRAM don't change during the draw.
static MainCallback sFieldCallback;
static MainCallback sBattleCallback;
static MainCallback sSceneCallback;
static u8 sSceneSpans[4];
static bool8 sSceneSprites;
static u16 sFillMaps[4][32 * 32];
static bool8 sFieldFrame;
static bool8 sBattleFrame;
static bool8 sBattleIntroSlide;
static u16 *sFieldMaps[4];
static u8 sFieldMapBase[4];

// Battle text box and menus (BG0, a 32x64 tile map): the original tilemap, without the windows
// printed over it, and the stretched 64x64 map drawn instead of BG0 (GBA layout).
static u16 sTextboxBase[32 * 64];
static bool8 sTextboxBaseLoaded;
static u16 sTextboxWide[64 * 64];

// Always on: every GBA screen is presented at 16:9 (docs/SCENES_16_9.md). The old WIDESCREEN
// option in the GBA options screen and its pokeemerald.cfg key no longer have any effect; the
// menu row is removed in a later cleanup.
bool8 Widescreen_IsOn(void)
{
    return TRUE;
}

void Widescreen_SetFieldFrame(void)
{
    u16 *const maps[4] = {NULL, gOverworldTilemapBuffer_Bg1, gOverworldTilemapBuffer_Bg2, gOverworldTilemapBuffer_Bg3};
    int32_t bg;

    for (bg = 1; bg < 4; bg++)
    {
        sFieldMaps[bg] = maps[bg];
        sFieldMapBase[bg] = GetBgAttribute(bg, BG_ATTR_MAPBASEINDEX);
    }
    sFieldCallback = gMain.callback2;
    sBattleCallback = NULL;
}

void Widescreen_SetBattleFrame(bool8 introSlide)
{
    sBattleCallback = gMain.callback2;
    sFieldCallback = NULL;
    sBattleIntroSlide = introSlide;
}

void Widescreen_SetSceneFrame(u8 bg0, u8 bg1, u8 bg2, u8 bg3, bool8 sprites)
{
    sSceneCallback = gMain.callback2;
    sSceneSpans[0] = bg0;
    sSceneSpans[1] = bg1;
    sSceneSpans[2] = bg2;
    sSceneSpans[3] = bg3;
    sSceneSprites = sprites;
}

// The most common entry of a BG's visible 30x20 tiles (in its first 32x32 block): the tile of
// its background pattern.
static u16 MostCommonMapEntry(const u16 *map, u16 hoffs, u16 voffs)
{
    // Open-addressing count of the up to 600 distinct entries.
    static u16 sKeys[1024];
    static u16 sCounts[1024];
    u16 best = 0, bestCount = 0;
    int32_t x, y;

    memset(sCounts, 0, sizeof(sCounts));
    for (y = 0; y < DISPLAY_HEIGHT / 8; y++)
    {
        for (x = 0; x < DISPLAY_WIDTH / 8; x++)
        {
            u16 entry = map[((y + voffs / 8) & 31) * 32 + ((x + hoffs / 8) & 31)];
            u32 slot = (entry * 2654435761u) >> 22;

            while (sCounts[slot] != 0 && sKeys[slot] != entry)
                slot = (slot + 1) & 1023;
            sKeys[slot] = entry;
            if (++sCounts[slot] > bestCount)
            {
                bestCount = sCounts[slot];
                best = entry;
            }
        }
    }
    return best;
}

// A 32x32 map filled with BG bg's background pattern tile, read from its live map in VRAM.
static u16 *BuildFillMap(int32_t bg)
{
    u16 control = *(vu16 *)(REG_ADDR_BG0CNT + bg * 2);
    const u16 *live = (const u16 *)BG_SCREEN_ADDR((control >> 8) & 0x1F);
    u16 entry = MostCommonMapEntry(live, *(vu16 *)(REG_ADDR_BG0HOFS + bg * 4), *(vu16 *)(REG_ADDR_BG0VOFS + bg * 4));
    int32_t i;

    for (i = 0; i < 32 * 32; i++)
        sFillMaps[bg][i] = entry;
    return sFillMaps[bg];
}

// The BG drawn behind all the others (the highest priority value; on a tie, the higher BG),
// among the enabled text BGs, or -1.
static int32_t GetBackTextBg(void)
{
    int32_t mode = REG_DISPCNT & 7;
    int32_t numTextBgs = mode == 0 ? 4 : mode == 1 ? 2 : 0;
    int32_t bg, back = -1, backPriority = -1;

    for (bg = 0; bg < numTextBgs; bg++)
    {
        int32_t priority = *(vu16 *)(REG_ADDR_BG0CNT + bg * 2) & 3;

        if (!(REG_DISPCNT & (DISPCNT_BG0_ON << bg)))
            continue;
        if (priority >= backPriority)
        {
            back = bg;
            backPriority = priority;
        }
    }
    return back;
}

// Entry (x, y) of a 64x64 tile map in the GBA layout (four 32x32 blocks).
static u32 WideMapIndex(int32_t x, int32_t y)
{
    x &= 63;
    y &= 63;
    return (y & 31) * 32 + (x & 31) + ((x & 32) << 5) + ((y & 32) << 6);
}

// Builds the stretched text box map from BG0's live map. Tile columns are GBA columns (8 px), so
// -3..-1 and 30..32 are the margins. On the rows of the text box frames, the frame's two edge
// columns on each side (0-1 and 28-29) move 3 columns out, and the inside of the box fills the
// gap. Printed text is never moved: windows reach columns 1 and 28 (action prompt, action and
// move menus), so a column only moves or gets filled while it still holds the original tilemap
// tile, not a window's. Other rows stay centred.
static void BuildBattleTextboxMap(const u16 *live)
{
    int32_t x, y;

    for (y = 0; y < 64; y++)
    {
        const u16 *liveRow = &live[y * 32];
        const u16 *baseRow = &sTextboxBase[y * 32];
        bool8 isBoxRow = (baseRow[0] & 0x3FF) != 0;

        for (x = -3; x < 33; x++)
        {
            u16 entry;

            if (!isBoxRow)
                entry = (x >= 0 && x < 30) ? liveRow[x] : 0;
            else if (x < -1)
                entry = baseRow[x + 3];  // left frame, moved out
            else if (x == -1)
                entry = baseRow[2];      // inside of the box
            else if (x < 2)
                entry = liveRow[x] == baseRow[x] ? baseRow[2] : liveRow[x];
            else if (x < 28)
                entry = liveRow[x];
            else if (x < 30)
                entry = liveRow[x] == baseRow[x] ? baseRow[27] : liveRow[x];
            else if (x == 30)
                entry = baseRow[27];     // inside of the box
            else
                entry = baseRow[x - 3];  // right frame, moved out
            sTextboxWide[WideMapIndex(x, y)] = entry;
        }
    }
}

// --- Stage scenes -------------------------------------------------------------------------------
// Margin policy for the scenes staged as one GBA picture (intro, title, credits, cable car,
// legendary cutscenes), after Emerald 3DS (3ds_video.c: sScrolls, MeasureStill, DrawStageBg).
// Copyright (c) 2026 Zallax and Emerald 3DS contributors, CC BY-NC-SA 4.0 (see NOTICE.md).

// How a layer's horizontal scroll is judged (3ds_video.c RecordScroll): over the last
// STAGE_STILL_FRAMES frames, a range wider than STAGE_STILL_SPAN pixels is a pan; a shake of a
// few pixels is not. A layer that panned keeps counting as scrolling for
// STAGE_SCROLL_HOLD_FRAMES, so a scene that pauses its scenery does not switch filling.
#define STAGE_STILL_FRAMES 32
#define STAGE_STILL_SPAN 4
#define STAGE_SCROLL_HOLD_FRAMES 120
#define STAGE_SCROLL_MASK 511
// Frames between two measurements of a still layer's hidden columns (its art does not move).
#define STAGE_MEASURE_INTERVAL 8
// Tile columns a 32-tile map has that the GBA screen (30 tiles) never shows.
#define STAGE_HIDDEN_FIRST_COLUMN 30
#define STAGE_MEASURED_FIRST_COLUMN 28
#define STAGE_MAP_COLUMNS 32
#define STAGE_SCREEN_ROWS (DISPLAY_HEIGHT / 8)

static u32 sStageFrame;
static u32 sStageLayerKey[4];
static u16 sStageScrollHistory[4][STAGE_STILL_FRAMES];
static bool8 sStageScrolls[4];
static u32 sStageMovedFrame[4];
static u16 sStageScrollX[4];
static u16 sStageScrollY[4];
static u32 sStageMeasuredFrame[4];
static bool8 sStagePanorama[4];
static bool8 sStageFade;

static bool8 TileHasPixels(const u32 chars, const u16 entry, const bool8 color256)
{
    const u32 tileBytes = color256 ? 64 : 32;
    const u32 address = chars + (entry & 0x3FF) * tileBytes;
    const u32 *words = (const u32 *)address;
    u32 i;

    if (address + tileBytes > BG_VRAM + 0x10000)
        return FALSE;
    for (i = 0; i < tileBytes / 4; i++)
    {
        if (words[i] != 0)
            return TRUE;
    }
    return FALSE;
}

// A still layer is a panorama (made to wrap, like the bike ride's sky) when the two map columns
// the GBA never shows hold art about as dense as the two just inside the screen's right edge.
static bool8 StageLayerIsPanorama(const int32_t bg, const u16 control)
{
    const u32 chars = BG_CHAR_ADDR((control >> 2) & 3);
    const u16 *map = (const u16 *)BG_SCREEN_ADDR((control >> 8) & 0x1F);
    const bool8 color256 = (control & BGCNT_256COLOR) != 0;
    const u32 rowMask = ((control >> 14) & 2) ? 63 : 31;
    u32 inside = 0;
    u32 beyond = 0;
    u32 y;
    u32 x;

    if ((control >> 14) & 1)
        return TRUE; // a 64-tile-wide map holds real art past the screen
    for (y = 0; y < STAGE_SCREEN_ROWS; y++)
    {
        const u32 row = (y + sStageScrollY[bg] / 8) & rowMask;
        const u16 *rowBase = &map[(row & 31) * 32 + ((row & 32) ? 1024 : 0)];

        for (x = STAGE_MEASURED_FIRST_COLUMN; x < STAGE_MAP_COLUMNS; x++)
        {
            const u32 column = (x + sStageScrollX[bg] / 8) & 31;

            if (TileHasPixels(chars, rowBase[column], color256))
                (x < STAGE_HIDDEN_FIRST_COLUMN) ? inside++ : beyond++;
        }
    }
    return beyond != 0 && beyond * 4 >= inside * 3;
}

// A horizontal scroll written line by line by a scanline effect (the title's drifting clouds):
// the register only holds the first line of it, and the layer's map is made to wrap, since the
// wave itself brings the hidden columns onto the GBA screen.
static bool8 HofsIsLineDriven(const int32_t bg)
{
    const uintptr_t hofs = (uintptr_t)(REG_ADDR_BG0HOFS + bg * 4);

    return gScanlineEffect.state != 0 && (uintptr_t)gScanlineEffect.dmaDest == hofs;
}

// Records this frame's horizontal scroll of a BG and decides whether it is panning. A new
// control register, mode or enable bit starts the judgement over.
static void TrackStageScroll(const int32_t bg)
{
    const u16 display = REG_DISPCNT;
    const u32 key = *(vu16 *)(REG_ADDR_BG0CNT + bg * 2) | ((u32)(display & 7) << 16)
                  | ((u32)((display >> (8 + bg)) & 1) << 20);
    const bool8 lineDriven = HofsIsLineDriven(bg);
    const u16 scroll = lineDriven ? 0 : (*(vu16 *)(REG_ADDR_BG0HOFS + bg * 4) & STAGE_SCROLL_MASK);
    int32_t low = 0;
    int32_t high = 0;
    u32 i;

    sStageScrollX[bg] = *(vu16 *)(REG_ADDR_BG0HOFS + bg * 4);
    sStageScrollY[bg] = *(vu16 *)(REG_ADDR_BG0VOFS + bg * 4);
    if (key != sStageLayerKey[bg])
    {
        sStageLayerKey[bg] = key;
        sStageScrolls[bg] = FALSE;
        sStageMeasuredFrame[bg] = 0;
        for (i = 0; i < STAGE_STILL_FRAMES; i++)
            sStageScrollHistory[bg][i] = scroll;
    }
    sStageScrollHistory[bg][sStageFrame % STAGE_STILL_FRAMES] = scroll;
    for (i = 0; i < STAGE_STILL_FRAMES; i++)
    {
        // Unwrapped against the current value, so 511 -> 0 is one step.
        const int32_t delta = ((((int32_t)sStageScrollHistory[bg][i] - (int32_t)scroll) + 256) & STAGE_SCROLL_MASK) - 256;

        if (delta < low)
            low = delta;
        if (delta > high)
            high = delta;
    }
    if (lineDriven || high - low > STAGE_STILL_SPAN)
    {
        sStageScrolls[bg] = TRUE;
        sStageMovedFrame[bg] = sStageFrame;
    }
    else if (sStageScrolls[bg] && sStageFrame - sStageMovedFrame[bg] > STAGE_SCROLL_HOLD_FRAMES)
    {
        sStageScrolls[bg] = FALSE;
    }
}

// A layer that pans continues as the GBA wraps it. A still layer is its picture: it repeats its
// edge tile, unless it is a panorama standing behind scenery that scrolls (the bike ride's sky).
static u8 StageLayerSpan(const int32_t bg, const bool8 sceneScrolls)
{
    const u16 control = *(vu16 *)(REG_ADDR_BG0CNT + bg * 2);

    if (sStageScrolls[bg])
        return WIDE_BG_WIDE;
    if (!sceneScrolls)
        return WIDE_BG_EDGE_TILE;
    if (sStageMeasuredFrame[bg] == 0 || sStageFrame - sStageMeasuredFrame[bg] >= STAGE_MEASURE_INTERVAL)
    {
        sStagePanorama[bg] = StageLayerIsPanorama(bg, control);
        sStageMeasuredFrame[bg] = sStageFrame;
    }
    return sStagePanorama[bg] ? WIDE_BG_WIDE : WIDE_BG_EDGE_TILE;
}

static void PrepareStageFrame(void)
{
    const u16 display = REG_DISPCNT;
    const int32_t mode = display & 7;
    const int32_t textBgs = mode == 0 ? 4 : mode == 1 ? 2 : 0;
    bool8 sceneScrolls = FALSE;
    bool8 anyWide = FALSE;
    bool8 anyStill = FALSE;
    int32_t bg;

    sStageFrame++;
    for (bg = 0; bg < textBgs; bg++)
    {
        TrackStageScroll(bg);
        if ((display & (DISPCNT_BG0_ON << bg)) && sStageScrolls[bg])
            sceneScrolls = TRUE;
    }
    for (bg = 0; bg < textBgs; bg++)
    {
        if (!(display & (DISPCNT_BG0_ON << bg)))
            continue;
        gFastDrawWide.bgSpan[bg] = StageLayerSpan(bg, sceneScrolls);
        if (gFastDrawWide.bgSpan[bg] == WIDE_BG_WIDE)
            anyWide = TRUE;
        else
            anyStill = TRUE;
    }
    gFastDrawWide.extendObj = TRUE;
    sStageFade = anyStill && !anyWide;
}

bool8 Widescreen_StageFadesMargins(void)
{
    return sStageFade;
}

void Widescreen_PrepareDraw(void)
{
    int32_t bg;

    sStageFade = FALSE;
    sFieldFrame = sFieldCallback != NULL && gMain.callback2 == sFieldCallback;
    sBattleFrame = sBattleCallback != NULL && gMain.callback2 == sBattleCallback;
    gFastDrawWide.output = Widescreen_IsOn();
    gFastDrawWide.extendObj = FALSE;
    for (bg = 0; bg < 4; bg++)
    {
        gFastDrawWide.bgSpan[bg] = WIDE_BG_CENTRED;
        gFastDrawWide.bgMaps[bg] = sFieldFrame ? sFieldMaps[bg] : NULL;
        gFastDrawWide.bgMapBase[bg] = sFieldMapBase[bg];
        gFastDrawWide.bgMapSize[bg] = 1; // 64x32
    }
    if (gFastDrawCentredOnly)
    {
        // An overlay shown centred (pass 6 text over the 3D world): nothing in the margins.
        gFastDrawWide.extend = FALSE;
        return;
    }
    if (gFastDrawWide.output && sFieldFrame)
    {
        // The map layers. BG0 (text boxes, menus) stays centred.
        gFastDrawWide.bgSpan[1] = gFastDrawWide.bgSpan[2] = gFastDrawWide.bgSpan[3] = WIDE_BG_WIDE;
        gFastDrawWide.extendObj = TRUE;
    }
    else if (gFastDrawWide.output && sBattleFrame)
    {
        // The battle background (BG3) is a 64x32 map whose scene repeats every 256 px, so it
        // has content for the margins; so does the looping entry background of the intro.
        // BG1/BG2 hold battle weather/environmental backgrounds (sandstorm, heat wave, etc.)
        // which must span the full 16:9 widescreen display.
        gFastDrawWide.bgSpan[3] = WIDE_BG_WIDE;
        gFastDrawWide.bgSpan[1] = WIDE_BG_WIDE;
        gFastDrawWide.bgSpan[2] = WIDE_BG_WIDE;
        gFastDrawWide.extendObj = TRUE;

        // BG0, the text box and menus, is stretched (only with its usual 32x64 map).
        if (((REG_BG0CNT >> 14) & 3) == 2)
        {
            if (!sTextboxBaseLoaded)
            {
                LZDecompressWram(gBattleTextboxTilemap, sTextboxBase);
                sTextboxBaseLoaded = TRUE;
            }
            BuildBattleTextboxMap((const u16 *)BG_SCREEN_ADDR((REG_BG0CNT >> 8) & 0x1F));
            gFastDrawWide.bgMaps[0] = sTextboxWide;
            gFastDrawWide.bgMapBase[0] = (REG_BG0CNT >> 8) & 0x1F;
            gFastDrawWide.bgMapSize[0] = 3; // 64x64
            gFastDrawWide.bgSpan[0] = WIDE_BG_WIDE;
        }
    }
    else if (gFastDrawWide.output && sSceneCallback != NULL && gMain.callback2 == sSceneCallback)
    {
        for (bg = 0; bg < 4; bg++)
        {
            gFastDrawWide.bgSpan[bg] = sSceneSpans[bg];
            if (sSceneSpans[bg] == WIDE_BG_FILL)
                gFastDrawWide.bgFillMaps[bg] = BuildFillMap(bg);
        }
        gFastDrawWide.extendObj = sSceneSprites;
    }
    else if (gFastDrawWide.output && VitaStage_IsStageActive())
    {
        PrepareStageFrame();
    }
    else if (gFastDrawWide.output)
    {
        // Other scenes (menus): the BG at the back is the background, usually one repeating
        // pattern tile; the margins repeat that tile on every line. Boxes, panels and text on
        // the BGs above stay centred. (Wrapping the map doesn't work: the columns the GBA never
        // shows hold filler or the other side of the screen.)
        int32_t back = GetBackTextBg();

        if (back >= 0)
        {
            gFastDrawWide.bgSpan[back] = WIDE_BG_FILL;
            gFastDrawWide.bgFillMaps[back] = BuildFillMap(back);
        }
    }
    gFastDrawWide.extend = gFastDrawWide.extendObj;
    for (bg = 0; bg < 4; bg++)
    {
        if (gFastDrawWide.bgSpan[bg] != WIDE_BG_CENTRED)
            gFastDrawWide.extend = TRUE;
    }
}
#endif
