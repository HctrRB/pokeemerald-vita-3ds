#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_daynight.h"
#include "platform/vita_encounters.h"
#include "platform/vita_extras.h"

#include "global.h"
#include "3ds_bottom_art.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "region_map.h"
#include "sound.h"
#include "wild_encounter.h"
#include "constants/flags.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/species.h"

#include <stdio.h>
#include <string.h>

enum
{
    HOENN_GRID_W = 28,
    HOENN_GRID_H = 15,
    MAP_PANEL_X  = 6,
    MAP_PANEL_Y  = 26,
    MAP_CELL_PX  = 6,
    MAP_VIEW_W   = HOENN_GRID_W * MAP_CELL_PX, // 168
    MAP_VIEW_H   = HOENN_GRID_H * MAP_CELL_PX, // 90
    MAX_WILD_SPECIES = 18
};

enum WildMethodTab
{
    WILD_TAB_LAND = 0,
    WILD_TAB_WATER,
    WILD_TAB_ROCK,
    WILD_TAB_FISH,
    WILD_TAB_COUNT
};

typedef struct
{
    uint16_t species;
    uint8_t minLv;
    uint8_t maxLv;
    uint8_t rodTier; // 0: Old, 1: Good, 2: Super
} WildEntry;

static bool sHoennDecoded = false;
static uint16_t sHoennImg[256 * 160];

static int32_t sCurX = 4;
static int32_t sCurY = 11;
static int32_t sPlayerGridX = 4;
static int32_t sPlayerGridY = 11;
static bool sZoom2x = false;
static bool sFlyMode = false;
static uint8_t sFlySlot = 0;
static uint8_t sWildTab = WILD_TAB_LAND;
static int32_t sWildScroll = 0;

static void EnsureHoennMapDecoded(void)
{
    if (sHoennDecoded)
        return;

    static uint8_t sTiles[0x4000];
    static uint16_t sMap[0x1000];
    uint16_t pal565[32];

    LZ77UnCompWram(GetRegionMapBg_GfxLZ(), sTiles);
    LZ77UnCompWram(GetRegionMapBg_TilemapLZ(), sMap);

    const uint16_t *const rawPal = GetRegionMapBg_Pal();
    for (int32_t i = 0; i < 32; i++)
    {
        uint16_t c565 = VitaUi_Bgr555ToRgb565(rawPal[i]);
        // Natural warm ochre recalibration for baked route tiles (avoids AGB-001 neon orange glare)
        const VitaUiRgb rgb = VitaUi_UnpackRgb565(c565);
        if (rgb.r > 220 && rgb.g > 130 && rgb.g < 195 && rgb.b < 40)
            c565 = VitaUi_PackRgb565(194, 150, 72);
        else if (rgb.r > 235 && rgb.g >= 195 && rgb.b < 90)
            c565 = VitaUi_PackRgb565(214, 180, 108);
        pal565[i] = c565;
    }
    if (pal565[0] == 0)
        pal565[0] = VitaUi_PackRgb565(58, 128, 184);

    // 8bpp affine tilemap (64x64 tiles of 64 bytes; tilemap stores 8-bit tile indices)
    const uint8_t *const mapBytes = (const uint8_t *)sMap;
    for (int32_t y = 0; y < 160; y++)
    {
        const int32_t ty = y >> 3;
        const int32_t py = y & 7;
        uint16_t *const dstRow = &sHoennImg[y * 256];
        for (int32_t x = 0; x < 256; x++)
        {
            const int32_t tx = x >> 3;
            const int32_t px = x & 7;
            const uint8_t tileIdx = (tx < 64 && ty < 64) ? mapBytes[ty * 64 + tx] : 0;
            const uint8_t v = sTiles[(size_t)tileIdx * 64U + py * 8 + px];
            dstRow[x] = (v >= 112 && v < 144) ? pal565[v - 112] : pal565[0];
        }
    }
    sHoennDecoded = true;
}

void VitaUiMap_BlitHoennThumbnail(
    uint16_t *dst,
    const size_t pitch,
    const int32_t dstX,
    const int32_t dstY,
    const int32_t dstW,
    const int32_t dstH)
{
    EnsureHoennMapDecoded();
    const VitaUiClip *const clip = VitaUi_FullClip();
    for (int32_t y = 0; y < dstH; y++)
    {
        const int32_t py = dstY + y;
        if (py < clip->y0 || py >= clip->y1)
            continue;
        const int32_t srcY = 16 + (y * (HOENN_GRID_H * 8)) / dstH;
        for (int32_t x = 0; x < dstW; x++)
        {
            const int32_t px = dstX + x;
            if (px < clip->x0 || px >= clip->x1)
                continue;
            const int32_t srcX = 8 + (x * (HOENN_GRID_W * 8)) / dstW;
            dst[(size_t)py * pitch + (size_t)px] = sHoennImg[srcY * 256 + srcX];
        }
    }
}

static void SyncPlayerGridPosition(void)
{
    const uint16_t rawSec = gMapHeader.regionMapSectionId;
    const uint16_t sec = CorrectSpecialMapSecId(rawSec);
    if (sec < MAPSEC_NONE && gRegionMapEntries[sec].width > 0)
    {
        sPlayerGridX = gRegionMapEntries[sec].x;
        sPlayerGridY = gRegionMapEntries[sec].y;
    }
}

void VitaUiMap_Prewarm(void)
{
    EnsureHoennMapDecoded();
}

void VitaUiMap_Reset(const bool flyMode, const uint8_t flyPartySlot)
{
    EnsureHoennMapDecoded();
    SyncPlayerGridPosition();
    sCurX = sPlayerGridX;
    sCurY = sPlayerGridY;
    sFlyMode = flyMode;
    sFlySlot = flyPartySlot;
    sWildScroll = 0;
}

static void PushWildEntry(
    WildEntry *list,
    int32_t *count,
    const uint16_t species,
    const uint8_t minLv,
    const uint8_t maxLv,
    const uint8_t rodTier)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return;
    for (int32_t i = 0; i < *count; i++)
    {
        if (list[i].species == species && list[i].rodTier == rodTier)
        {
            if (minLv < list[i].minLv) list[i].minLv = minLv;
            if (maxLv > list[i].maxLv) list[i].maxLv = maxLv;
            return;
        }
    }
    if (*count < MAX_WILD_SPECIES)
    {
        list[*count].species = species;
        list[*count].minLv = minLv;
        list[*count].maxLv = maxLv;
        list[*count].rodTier = rodTier;
        (*count)++;
    }
}

static int32_t GatherWildForSection(const uint16_t mapSec, const uint8_t tab, WildEntry *out)
{
    int32_t count = 0;
    if (mapSec >= MAPSEC_NONE)
        return 0;

    for (uint32_t i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED); i++)
    {
        const struct MapHeader *const mh = Overworld_GetMapHeaderByGroupAndId(
            gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum);
        if (mh == NULL || CorrectSpecialMapSecId(mh->regionMapSectionId) != mapSec)
            continue;

        if (tab == WILD_TAB_LAND && gWildMonHeaders[i].landMonsInfo != NULL)
        {
            const struct WildPokemon *slots = gWildMonHeaders[i].landMonsInfo->wildPokemon;
            if (VitaExtras_GetFast(FAST_EXTRA_TIMED_ENCOUNTERS) != 0)
            {
                const struct WildPokemonInfo *const timed = VitaEncounters_Land(
                    gWildMonHeaders[i].landMonsInfo, gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum);
                if (timed != NULL && timed->wildPokemon != NULL)
                    slots = timed->wildPokemon;
            }
            for (int32_t s = 0; s < LAND_WILD_COUNT; s++)
                PushWildEntry(out, &count, slots[s].species, slots[s].minLevel, slots[s].maxLevel, 0);
        }
        else if (tab == WILD_TAB_WATER && gWildMonHeaders[i].waterMonsInfo != NULL)
        {
            const struct WildPokemon *const slots = gWildMonHeaders[i].waterMonsInfo->wildPokemon;
            for (int32_t s = 0; s < WATER_WILD_COUNT; s++)
                PushWildEntry(out, &count, slots[s].species, slots[s].minLevel, slots[s].maxLevel, 0);
        }
        else if (tab == WILD_TAB_ROCK && gWildMonHeaders[i].rockSmashMonsInfo != NULL)
        {
            const struct WildPokemon *const slots = gWildMonHeaders[i].rockSmashMonsInfo->wildPokemon;
            for (int32_t s = 0; s < ROCK_WILD_COUNT; s++)
                PushWildEntry(out, &count, slots[s].species, slots[s].minLevel, slots[s].maxLevel, 0);
        }
        else if (tab == WILD_TAB_FISH && gWildMonHeaders[i].fishingMonsInfo != NULL)
        {
            const struct WildPokemon *const slots = gWildMonHeaders[i].fishingMonsInfo->wildPokemon;
            for (int32_t s = 0; s < FISH_WILD_COUNT; s++)
            {
                const uint8_t tier = (s < 2) ? 0U : ((s < 5) ? 1U : 2U);
                PushWildEntry(out, &count, slots[s].species, slots[s].minLevel, slots[s].maxLevel, tier);
            }
        }
    }
    return count;
}

bool VitaUiMap_Update(const VitaUiInput *in)
{
    bool changed = false;
    int32_t dx = 0;
    int32_t dy = 0;
    if (in->newButtons & VITA_BTN_LEFT)  dx = -1;
    if (in->newButtons & VITA_BTN_RIGHT) dx =  1;
    if (in->newButtons & VITA_BTN_UP)    dy = -1;
    if (in->newButtons & VITA_BTN_DOWN)  dy =  1;

    if (dx != 0 || dy != 0)
    {
        const int32_t nx = sCurX + dx;
        const int32_t ny = sCurY + dy;
        if (nx >= 0 && nx < HOENN_GRID_W && ny >= 0 && ny < HOENN_GRID_H)
        {
            sCurX = nx;
            sCurY = ny;
            sWildScroll = 0;
            PlaySE(SE_SELECT);
            changed = true;
        }
    }

    // Touch directly on Hoenn map viewport
    if (VitaUi_HitRect(in, MAP_PANEL_X + 3, MAP_PANEL_Y + 14, MAP_VIEW_W, MAP_VIEW_H))
    {
        const int32_t relX = in->touchX - (MAP_PANEL_X + 3);
        const int32_t relY = in->touchY - (MAP_PANEL_Y + 14);
        if (!sZoom2x)
        {
            sCurX = relX / MAP_CELL_PX;
            sCurY = relY / MAP_CELL_PX;
        }
        else
        {
            int32_t camX = sCurX - 7;
            int32_t camY = sCurY - 3;
            if (camX < 0) camX = 0;
            if (camY < 0) camY = 0;
            if (camX > HOENN_GRID_W - 14) camX = HOENN_GRID_W - 14;
            if (camY > HOENN_GRID_H - 7) camY = HOENN_GRID_H - 7;
            sCurX = camX + relX / (MAP_CELL_PX * 2);
            sCurY = camY + relY / (MAP_CELL_PX * 2);
        }
        if (sCurX >= HOENN_GRID_W) sCurX = HOENN_GRID_W - 1;
        if (sCurY >= HOENN_GRID_H) sCurY = HOENN_GRID_H - 1;
        sWildScroll = 0;
        PlaySE(SE_SELECT);
        changed = true;
    }

    // Zoom toggle (SELECT or Zoom icon tap)
    if ((in->newButtons & VITA_BTN_SELECT) || VitaUi_HitRect(in, MAP_PANEL_X + 150, MAP_PANEL_Y + 1, 22, 13))
    {
        sZoom2x = !sZoom2x;
        PlaySE(SE_SELECT);
        changed = true;
    }

    // Wild method tab cycle (TRIANGLE or tapping method pills on right panel)
    if (in->newButtons & VITA_BTN_TRIANGLE)
    {
        sWildTab = (uint8_t)((sWildTab + 1U) % WILD_TAB_COUNT);
        sWildScroll = 0;
        PlaySE(SE_SELECT);
        changed = true;
    }
    for (uint8_t t = 0; t < WILD_TAB_COUNT; t++)
    {
        if (VitaUi_HitRect(in, 182 + (int32_t)t * 33, 27, 32, 14))
        {
            sWildTab = t;
            sWildScroll = 0;
            PlaySE(SE_SELECT);
            changed = true;
        }
    }

    // HM02 Fly execution on A / Fly button tap
    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);
    if (sFlyMode)
    {
        if ((in->newButtons & aBtn) || VitaUi_HitRect(in, MAP_PANEL_X + 4, 184, 166, 20))
        {
            const uint16_t sec = GetRegionMapSecIdAt((u16)(sCurX + 1), (u16)(sCurY + 2));
            if (VitaGame_CanFlyToMapSec(sec))
            {
                PlaySE(SE_SELECT);
                sFlyMode = false;
                VitaUI_Close();
                VitaGame_ExecuteFlyToMapSec(sFlySlot, sec, 0);
                return true;
            }
            PlaySE(SE_FAILURE);
        }
        else if (in->newButtons & bBtn)
        {
            sFlyMode = false;
            PlaySE(SE_SELECT);
            VitaUI_Open(VITA_UI_SEC_POKEMON);
            return true;
        }
    }

    return changed;
}

static void DrawHoennViewport(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase)
{
    EnsureHoennMapDecoded();
    const int32_t vx = MAP_PANEL_X + 3;
    const int32_t vy = MAP_PANEL_Y + 14;
    const int32_t cellPx = sZoom2x ? (MAP_CELL_PX * 2) : MAP_CELL_PX;

    int32_t camX = 0;
    int32_t camY = 0;
    if (sZoom2x)
    {
        camX = sCurX - 7;
        camY = sCurY - 3;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > HOENN_GRID_W - 14) camX = HOENN_GRID_W - 14;
        if (camY > HOENN_GRID_H - 7) camY = HOENN_GRID_H - 7;
    }

    for (int32_t y = 0; y < MAP_VIEW_H; y++)
    {
        uint16_t *const dstRow = &dst[(size_t)(vy + y) * pitch + (size_t)vx];
        const int32_t srcY = 16 + camY * 8 + (y * 8) / cellPx;
        for (int32_t x = 0; x < MAP_VIEW_W; x++)
        {
            const int32_t srcX = 8 + camX * 8 + (x * 8) / cellPx;
            dstRow[x] = sHoennImg[srcY * 256 + srcX];
        }
    }

    // Highlight visited Fly destinations in Fly mode
    if (sFlyMode)
    {
        for (int32_t gy = 0; gy < HOENN_GRID_H; gy++)
        {
            for (int32_t gx = 0; gx < HOENN_GRID_W; gx++)
            {
                const uint16_t sec = GetRegionMapSecIdAt((u16)(gx + 1), (u16)(gy + 2));
                if (!VitaGame_CanFlyToMapSec(sec))
                    continue;
                const int32_t px = vx + (gx - camX) * cellPx;
                const int32_t py = vy + (gy - camY) * cellPx;
                if (px >= vx && px + cellPx <= vx + MAP_VIEW_W && py >= vy && py + cellPx <= vy + MAP_VIEW_H)
                    VitaUiArt_FillRect(dst, pitch, VitaUi_FullClip(), px + 1, py + 1, cellPx - 2, 2, UI_INK_MINT);
            }
        }
    }

    // Player head icon sprite (16x16 genuine decomp sprite)
    const int32_t plX = vx + (sPlayerGridX - camX) * cellPx;
    const int32_t plY = vy + (sPlayerGridY - camY) * cellPx;
    const int32_t iconX = plX + (cellPx - 16) / 2;
    const int32_t iconY = plY + (cellPx - 16) / 2;
    const VitaUiClip mapClip = { vx, vy, vx + MAP_VIEW_W, vy + MAP_VIEW_H };
    const bool isMale = (gSaveBlock2Ptr->playerGender == MALE);
    const uint8_t *const pGfx = isMale ? GetRegionMapPlayerIcon_BrendanGfx() : GetRegionMapPlayerIcon_MayGfx();
    const uint16_t *const pPal = isMale ? GetRegionMapPlayerIcon_BrendanPal() : GetRegionMapPlayerIcon_MayPal();
    VitaUiArt_DrawTiles4bppBgrPal(dst, pitch, &mapClip, pGfx, 2, 2, iconX, iconY, pPal);

    // Blinking cursor outline
    const int32_t cx = vx + (sCurX - camX) * cellPx;
    const int32_t cy = vy + (sCurY - camY) * cellPx;
    if (cx >= vx && cx + cellPx <= vx + MAP_VIEW_W && cy >= vy && cy + cellPx <= vy + MAP_VIEW_H)
    {
        const uint16_t curCol = (blinkPhase < 128U) ? UI_INK_LIGHT : UI_INK_GOLD;
        const VitaUiClip *const c = VitaUi_FullClip();
        VitaUiArt_FillRect(dst, pitch, c, cx, cy, cellPx, 1, curCol);
        VitaUiArt_FillRect(dst, pitch, c, cx, cy + cellPx - 1, cellPx, 1, curCol);
        VitaUiArt_FillRect(dst, pitch, c, cx, cy, 1, cellPx, curCol);
        VitaUiArt_FillRect(dst, pitch, c, cx + cellPx - 1, cy, 1, cellPx, curCol);
    }
}

void VitaUiMap_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase, const uint32_t animTick)
{
    const VitaUiClip *const clip = VitaUi_FullClip();

    // Left bevelled Map frame
    VitaUiPlate_Draw(dst, pitch, clip, MAP_PANEL_X, MAP_PANEL_Y, 174, 180, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 13);

    const uint16_t sec = GetRegionMapSecIdAt((u16)(sCurX + 1), (u16)(sCurY + 2));
    uint8_t gbaSecName[28];
    char secAscii[28];
    if (sec < MAPSEC_NONE)
    {
        GetMapNameGeneric(gbaSecName, sec);
        VitaUiArt_GbaToAscii(gbaSecName, secAscii, sizeof(secAscii));
    }
    else
    {
        snprintf(secAscii, sizeof(secAscii), "HOENN SEA");
    }

    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, secAscii, MAP_PANEL_X + 5, MAP_PANEL_Y + 1, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    VitaUiArt_DrawSpriteCentred(
        dst, pitch, clip,
        sZoom2x ? sArtZoomOut : sArtZoomIn,
        2, 2, MAP_PANEL_X + 168, MAP_PANEL_Y + 7, gVitaUiArtPal565);

    DrawHoennViewport(dst, pitch, blinkPhase);

    // Lower-left urban / landmark / Fly status box
    const int32_t infoY = MAP_PANEL_Y + 14 + MAP_VIEW_H + 4;
    char coordLine[40];
    snprintf(coordLine, sizeof(coordLine), "SEC (%d,%d)  ZOOM %s", (int32_t)sCurX, (int32_t)sCurY, sZoom2x ? "2X" : "1X");
    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, coordLine, MAP_PANEL_X + 6, infoY, UI_INK_MUTED, VITA_UI_NO_SHADOW);

    if (sFlyMode)
    {
        const bool canFly = VitaGame_CanFlyToMapSec(sec);

        char destBuf[36];
        snprintf(destBuf, sizeof(destBuf), "DEST: %s", secAscii);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, destBuf, MAP_PANEL_X + 6, infoY + 14, canFly ? UI_INK_BLUE : UI_INK_MUTED, VITA_UI_NO_SHADOW);

        const char *const flyTip = canFly ? "LANDING AVAILABLE" : "CANNOT FLY HERE";
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, flyTip, MAP_PANEL_X + 6, infoY + 28, canFly ? UI_INK_DARK : UI_INK_MUTED, VITA_UI_NO_SHADOW);

        VitaUiPlate_Draw(
            dst, pitch, clip,
            MAP_PANEL_X + 4, 183, 166, 20, 3,
            canFly ? kPlateBlueMove : kPlateSageDark,
            canFly ? VITA_PLATE_FOCUS : VITA_PLATE_DISABLED,
            blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL,
            canFly ? "A: FLY TO DESTINATION" : "UNVISITED / NO LANDING",
            MAP_PANEL_X + 4, 183, 166, 20, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
    else
    {
        const bool visited = VitaGame_CanFlyToMapSec(sec);
        const char *statusStr = visited ? "LANDMARK VISITED (FLY OK)" : "SELECT: 2X ZOOM  TRI: WILD";
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, statusStr, MAP_PANEL_X + 6, infoY + 13, UI_INK_DARK, VITA_UI_NO_SHADOW);
        char curLoc[36];
        uint8_t curNameGba[24];
        char curNameAsc[24];
        GetMapNameGeneric(curNameGba, CorrectSpecialMapSecId(gMapHeader.regionMapSectionId));
        VitaUiArt_GbaToAscii(curNameGba, curNameAsc, sizeof(curNameAsc));
        snprintf(curLoc, sizeof(curLoc), "NOW: %s", curNameAsc);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, curLoc, MAP_PANEL_X + 6, infoY + 26, UI_INK_BLUE, VITA_UI_NO_SHADOW);
    }

    // Right panel: Wild Habitat Encounters (respecting seen/caught flags)
    static const char *const sTabNames[WILD_TAB_COUNT] = { "LAND", "SURF", "ROCK", "FISH" };
    for (uint8_t t = 0; t < WILD_TAB_COUNT; t++)
    {
        const bool sel = (sWildTab == t);
        VitaUiPlate_Draw(
            dst, pitch, clip,
            182 + (int32_t)t * 33, 26, 32, 14, 2,
            sel ? kPlateSage : kPlateSageDark,
            sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL,
            blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL_NARROW, sTabNames[t],
            182 + (int32_t)t * 33, 26, 32, 14, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    VitaUiPlate_Draw(dst, pitch, clip, 182, 42, 132, 164, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);

    WildEntry wild[MAX_WILD_SPECIES];
    const int32_t wildCount = GatherWildForSection(sec, sWildTab, wild);
    if (wildCount == 0)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "NO WILD DATA", 248, 114, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        return;
    }

    const int32_t maxRows = 6;
    const uint8_t iconFrame = (uint8_t)((animTick >> 4) & 1U);
    for (int32_t r = 0; r < maxRows && (sWildScroll + r) < wildCount; r++)
    {
        const WildEntry *const we = &wild[sWildScroll + r];
        const int32_t ry = 46 + r * 26;
        const uint16_t natDex = SpeciesToNationalPokedexNum(we->species);
        const bool seen = (natDex != 0) && (GetSetPokedexFlag(natDex, FLAG_GET_SEEN) != 0);
        const bool caught = (natDex != 0) && (GetSetPokedexFlag(natDex, FLAG_GET_CAUGHT) != 0);

        VitaUiArt_FillRect(dst, pitch, clip, 186, ry + 24, 124, 1, UI_BORDER_SOFT);

        if (seen)
        {
            VitaUiArt_DrawMonIcon(dst, pitch, clip, we->species, iconFrame, 186, ry - 4, 1);
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gSpeciesNames[we->species], 218, ry + 1, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }
        else
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "???", 218, ry + 1, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            VitaUiArt_FillRect(dst, pitch, clip, 192, ry + 4, 16, 16, UI_BORDER_SOFT);
            VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "?", 200, ry + 5, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }

        char lvBuf[28];
        if (sWildTab == WILD_TAB_FISH)
        {
            static const char *const sRods[3] = { "OLD", "GOOD", "SUPR" };
            snprintf(lvBuf, sizeof(lvBuf), "%s L%u-%u", sRods[we->rodTier % 3U], we->minLv, we->maxLv);
        }
        else
        {
            snprintf(lvBuf, sizeof(lvBuf), "LV %u-%u", we->minLv, we->maxLv);
        }
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, lvBuf, 218, ry + 12, UI_INK_MUTED, VITA_UI_NO_SHADOW);

        if (caught)
            VitaUi_DrawMiniPokeball(dst, pitch, 300, ry + 6, true, false);
        else if (seen)
            VitaUi_DrawArt4bpp(dst, pitch, sArtTinyX, 8, 8, 300, ry + 6, gVitaUiArtPal565);
    }
}
