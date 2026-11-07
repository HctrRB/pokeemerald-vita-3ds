#include "vita_ui_internal.h"

#include "global.h"
#include "3ds_bottom_art.h"
#include "data.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "region_map.h"
#include "sound.h"
#include "wild_encounter.h"
#include "constants/characters.h"
#include "constants/pokedex.h"
#include "event_data.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/species.h"

#include <stdio.h>
#include <string.h>

extern const u8 *const gMonFootprintTable[];

enum DexTab
{
    DEX_TAB_INFO = 0,
    DEX_TAB_AREA,
    DEX_TAB_CRY,
    DEX_TAB_SIZE,
    DEX_TAB_COUNT
};

enum DexSortOrder
{
    DEX_SORT_NUMBER = 0,
    DEX_SORT_AZ,
    DEX_SORT_HEAVIEST,
    DEX_SORT_LIGHTEST,
    DEX_SORT_TALLEST,
    DEX_SORT_SMALLEST,
    DEX_SORT_COUNT
};

typedef struct
{
    uint16_t dexNum;
    uint16_t species;
    uint16_t natDex;
    bool seen;
    bool caught;
} DexRow;

static bool sNationalMode = false;
static bool sDetailOpen = false;
static bool sSearchOpen = false;
static uint8_t sDetailTab = DEX_TAB_INFO;
static int32_t sCursor = 0;
static int32_t sScroll = 0;

// Search filter state
static uint8_t sSearchField = 0;
static uint8_t sFilterLetter = 0; // 0 = ANY, 1..26 = A..Z
static uint8_t sFilterType1 = 0;  // 0 = ANY, 1..NUMBER_OF_MON_TYPES = type + 1
static uint8_t sFilterOrder = DEX_SORT_NUMBER;
static uint32_t sCryAnimFrames = 0;

static DexRow sDexRows[NATIONAL_DEX_COUNT];
static int32_t sDexRowCount = 0;

static bool MatchesSearchFilter(const uint16_t species, const bool seen)
{
    if (sFilterLetter > 0)
    {
        if (!seen)
            return false;
        char nameAsc[16];
        VitaUiArt_GbaToAscii(gSpeciesNames[species], nameAsc, sizeof(nameAsc));
        const char want = (char)('A' + sFilterLetter - 1);
        if (nameAsc[0] != want)
            return false;
    }
    if (sFilterType1 > 0)
    {
        if (!seen)
            return false;
        const uint8_t t = (uint8_t)(sFilterType1 - 1U);
        if (gSpeciesInfo[species].types[0] != t && gSpeciesInfo[species].types[1] != t)
            return false;
    }
    return true;
}

static int32_t CompareDexRows(const DexRow *a, const DexRow *b)
{
    if (sFilterOrder == DEX_SORT_NUMBER)
        return (int32_t)a->dexNum - (int32_t)b->dexNum;

    if (!a->seen && !b->seen)
        return (int32_t)a->dexNum - (int32_t)b->dexNum;
    if (!a->seen)
        return 1;
    if (!b->seen)
        return -1;

    if (sFilterOrder == DEX_SORT_AZ)
    {
        char na[16], nb[16];
        VitaUiArt_GbaToAscii(gSpeciesNames[a->species], na, sizeof(na));
        VitaUiArt_GbaToAscii(gSpeciesNames[b->species], nb, sizeof(nb));
        return strcmp(na, nb);
    }

    const uint16_t wa = gPokedexEntries[a->natDex].weight;
    const uint16_t wb = gPokedexEntries[b->natDex].weight;
    const uint16_t ha = gPokedexEntries[a->natDex].height;
    const uint16_t hb = gPokedexEntries[b->natDex].height;

    switch (sFilterOrder)
    {
    case DEX_SORT_HEAVIEST: return (int32_t)wb - (int32_t)wa;
    case DEX_SORT_LIGHTEST: return (int32_t)wa - (int32_t)wb;
    case DEX_SORT_TALLEST:  return (int32_t)hb - (int32_t)ha;
    case DEX_SORT_SMALLEST: return (int32_t)ha - (int32_t)hb;
    default:                return (int32_t)a->dexNum - (int32_t)b->dexNum;
    }
}

static void RebuildDexList(void)
{
    if (!IsNationalPokedexEnabled())
        sNationalMode = false;

    const uint16_t total = sNationalMode ? NATIONAL_DEX_COUNT : HOENN_DEX_COUNT;
    sDexRowCount = 0;

    for (uint16_t d = 1; d <= total; d++)
    {
        const uint16_t species = sNationalMode ? NationalPokedexNumToSpecies(d) : HoennPokedexNumToSpecies(d);
        if (species == SPECIES_NONE || species >= NUM_SPECIES)
            continue;
        const uint16_t natDex = sNationalMode ? d : HoennToNationalOrder(d);
        const bool seen = (natDex != 0) && (GetSetPokedexFlag(natDex, FLAG_GET_SEEN) != 0);
        const bool caught = (natDex != 0) && (GetSetPokedexFlag(natDex, FLAG_GET_CAUGHT) != 0);
        if (!MatchesSearchFilter(species, seen))
            continue;

        DexRow *const r = &sDexRows[sDexRowCount++];
        r->dexNum = d;
        r->species = species;
        r->natDex = natDex;
        r->seen = seen;
        r->caught = caught;
    }

    if (sFilterOrder != DEX_SORT_NUMBER && sDexRowCount > 1)
    {
        for (int32_t i = 1; i < sDexRowCount; i++)
        {
            const DexRow key = sDexRows[i];
            int32_t j = i - 1;
            while (j >= 0 && CompareDexRows(&sDexRows[j], &key) > 0)
            {
                sDexRows[j + 1] = sDexRows[j];
                j--;
            }
            sDexRows[j + 1] = key;
        }
    }

    if (sCursor >= sDexRowCount)
        sCursor = (sDexRowCount > 0) ? (sDexRowCount - 1) : 0;
    if (sScroll > sCursor)
        sScroll = sCursor;
}

void VitaUiPokedex_Reset(void)
{
    sDetailOpen = false;
    sSearchOpen = false;
    sCryAnimFrames = 0;
    RebuildDexList();
}

static void DrawFootprint16x16(
    uint16_t *dst,
    const size_t pitch,
    const uint16_t species,
    const int32_t x,
    const int32_t y,
    const uint16_t ink565)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return;
    const uint8_t *const fp = gMonFootprintTable[species];
    if (fp == NULL)
        return;

    const VitaUiClip *const clip = VitaUi_FullClip();
    for (int32_t ty = 0; ty < 2; ty++)
    {
        for (int32_t tx = 0; tx < 2; tx++)
        {
            const uint8_t *const tile = &fp[(ty * 2 + tx) * 8];
            for (int32_t py = 0; py < 8; py++)
            {
                const uint8_t bits = tile[py];
                const int32_t sy = y + ty * 8 + py;
                if (sy < clip->y0 || sy >= clip->y1)
                    continue;
                for (int32_t px = 0; px < 8; px++)
                {
                    if (((bits >> px) & 1U) == 0U)
                        continue;
                    const int32_t sx = x + tx * 8 + px;
                    if (sx >= clip->x0 && sx < clip->x1)
                        dst[(size_t)sy * pitch + (size_t)sx] = ink565;
                }
            }
        }
    }
}

static bool SpeciesAppearsInSection(const uint16_t species, const uint16_t mapSec)
{
    for (uint32_t i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED); i++)
    {
        const struct MapHeader *const mh = Overworld_GetMapHeaderByGroupAndId(
            gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum);
        if (mh == NULL || CorrectSpecialMapSecId(mh->regionMapSectionId) != mapSec)
            continue;

        if (gWildMonHeaders[i].landMonsInfo != NULL)
        {
            for (int32_t s = 0; s < LAND_WILD_COUNT; s++)
                if (gWildMonHeaders[i].landMonsInfo->wildPokemon[s].species == species)
                    return true;
        }
        if (gWildMonHeaders[i].waterMonsInfo != NULL)
        {
            for (int32_t s = 0; s < WATER_WILD_COUNT; s++)
                if (gWildMonHeaders[i].waterMonsInfo->wildPokemon[s].species == species)
                    return true;
        }
        if (gWildMonHeaders[i].fishingMonsInfo != NULL)
        {
            for (int32_t s = 0; s < FISH_WILD_COUNT; s++)
                if (gWildMonHeaders[i].fishingMonsInfo->wildPokemon[s].species == species)
                    return true;
        }
    }
    return false;
}

bool VitaUiPokedex_Update(const VitaUiInput *in)
{
    bool changed = false;
    if (sCryAnimFrames > 0)
    {
        sCryAnimFrames--;
        changed = true;
    }

    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);

    if (sSearchOpen)
    {
        if (in->newButtons & VITA_BTN_UP)
        {
            sSearchField = (sSearchField == 0) ? 3U : (uint8_t)(sSearchField - 1U);
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sSearchField = (uint8_t)((sSearchField + 1U) % 4U);
            PlaySE(SE_SELECT);
            return true;
        }
        const int32_t dir = (in->newButtons & VITA_BTN_RIGHT) ? 1 : ((in->newButtons & VITA_BTN_LEFT) ? -1 : 0);
        if (dir != 0)
        {
            if (sSearchField == 0)
                sFilterLetter = (uint8_t)((27 + (int32_t)sFilterLetter + dir) % 27);
            else if (sSearchField == 1)
                sFilterType1 = (uint8_t)(((int32_t)NUMBER_OF_MON_TYPES + 1 + (int32_t)sFilterType1 + dir) % ((int32_t)NUMBER_OF_MON_TYPES + 1));
            else if (sSearchField == 2)
                sFilterOrder = (uint8_t)(((int32_t)DEX_SORT_COUNT + (int32_t)sFilterOrder + dir) % (int32_t)DEX_SORT_COUNT);
            PlaySE(SE_SELECT);
            RebuildDexList();
            return true;
        }
        if ((in->newButtons & aBtn) && sSearchField == 3)
        {
            sFilterLetter = 0;
            sFilterType1 = 0;
            sFilterOrder = DEX_SORT_NUMBER;
            RebuildDexList();
            PlaySE(SE_SELECT);
            return true;
        }
        if ((in->newButtons & bBtn) || (in->newButtons & aBtn))
        {
            sSearchOpen = false;
            PlaySE(SE_SELECT);
            return true;
        }
        return changed;
    }

    if (sDetailOpen)
    {
        if (in->newButtons & VITA_BTN_LEFT)
        {
            sDetailTab = (sDetailTab == 0) ? (DEX_TAB_COUNT - 1) : (uint8_t)(sDetailTab - 1U);
            PlaySE(SE_SELECT);
            changed = true;
        }
        if (in->newButtons & VITA_BTN_RIGHT)
        {
            sDetailTab = (uint8_t)((sDetailTab + 1U) % DEX_TAB_COUNT);
            PlaySE(SE_SELECT);
            changed = true;
        }
        for (uint8_t t = 0; t < DEX_TAB_COUNT; t++)
        {
            if (VitaUi_HitRect(in, 128 + (int32_t)t * 46, 26, 44, 15))
            {
                sDetailTab = t;
                PlaySE(SE_SELECT);
                changed = true;
            }
        }
        if ((in->newButtons & aBtn) && sDexRowCount > 0 && sDexRows[sCursor].seen)
        {
            PlayCry_Normal(sDexRows[sCursor].species, 0);
            sCryAnimFrames = 48;
            changed = true;
        }
        if ((in->newButtons & bBtn) || VitaUi_HitRect(in, 8, 188, 112, 18))
        {
            sDetailOpen = false;
            PlaySE(SE_SELECT);
            return true;
        }
        return changed;
    }

    // List navigation
    int32_t step = 0;
    if (in->newButtons & VITA_BTN_UP)    step = -1;
    if (in->newButtons & VITA_BTN_DOWN)  step =  1;
    if (in->newButtons & VITA_BTN_LEFT)  step = -7;
    if (in->newButtons & VITA_BTN_RIGHT) step =  7;

    if (step != 0 && sDexRowCount > 0)
    {
        sCursor += step;
        if (sCursor < 0) sCursor = 0;
        if (sCursor >= sDexRowCount) sCursor = sDexRowCount - 1;
        if (sCursor < sScroll) sScroll = sCursor;
        if (sCursor >= sScroll + 7) sScroll = sCursor - 6;
        PlaySE(SE_SELECT);
        changed = true;
    }

    // Touch row selection on right list
    for (int32_t r = 0; r < 7; r++)
    {
        if (VitaUi_HitRect(in, 130, 44 + r * 22, 184, 21) && (sScroll + r) < sDexRowCount)
        {
            if (sCursor == sScroll + r && sDexRows[sCursor].seen)
                sDetailOpen = true;
            else
                sCursor = sScroll + r;
            PlaySE(SE_SELECT);
            changed = true;
        }
    }

    // National / Hoenn toggle (SELECT or top-right pill)
    if (((in->newButtons & VITA_BTN_SELECT) || VitaUi_HitRect(in, 130, 26, 90, 15)) && IsNationalPokedexEnabled())
    {
        sNationalMode = !sNationalMode;
        sCursor = 0;
        sScroll = 0;
        RebuildDexList();
        PlaySE(SE_SELECT);
        changed = true;
    }

    // Search modal button (TRIANGLE or SEARCH pill)
    if ((in->newButtons & VITA_BTN_TRIANGLE) || VitaUi_HitRect(in, 224, 26, 90, 15))
    {
        sSearchOpen = true;
        PlaySE(SE_SELECT);
        return true;
    }

    if ((in->newButtons & aBtn) && sDexRowCount > 0 && sDexRows[sCursor].seen)
    {
        sDetailOpen = true;
        sDetailTab = DEX_TAB_INFO;
        PlaySE(SE_SELECT);
        return true;
    }

    return changed;
}

static void DrawPokedexPreviewPodium(uint16_t *const dst, const size_t pitch, const VitaUiClip *const clip)
{
    const int32_t x0 = 15;
    const int32_t x1 = 114;
    const int32_t y0 = 51;
    const int32_t y1 = 130;

    const uint16_t colSkyEven   = VitaUi_PackRgb565(216, 242, 218);
    const uint16_t colSkyOdd    = VitaUi_PackRgb565(228, 248, 230);
    const uint16_t colHorizon   = VitaUi_PackRgb565(168, 212, 172);
    const uint16_t colFloorEven = VitaUi_PackRgb565(198, 234, 202);
    const uint16_t colFloorOdd  = VitaUi_PackRgb565(208, 240, 212);

    const uint16_t colRimTop    = VitaUi_PackRgb565(164, 228, 94);
    const uint16_t colRimBot    = VitaUi_PackRgb565(134, 194, 72);
    const uint16_t colGrass     = VitaUi_PackRgb565(82, 182, 98);
    const uint16_t colGrassDark = VitaUi_PackRgb565(52, 142, 70);
    const uint16_t colGrassLit  = VitaUi_PackRgb565(118, 206, 128);

    const int32_t cx = 65;
    const int32_t cy = 114;

    for (int32_t y = y0; y <= y1; ++y)
    {
        if (clip != NULL && (y < clip->y0 || y >= clip->y1))
            continue;

        uint16_t *const row = &dst[(size_t)y * pitch];
        const int32_t dy = y - cy;
        const int32_t dySqOuter = dy * dy * 1936; // 44^2
        const int32_t dySqInner = dy * dy * 1600; // 40^2

        for (int32_t x = x0; x <= x1; ++x)
        {
            if (clip != NULL && (x < clip->x0 || x >= clip->x1))
                continue;

            // Preserve rounded corner pixels of enclosing cream plate
            if ((x == x0 && (y == y0 || y == y1)) || (x == x1 && (y == y0 || y == y1)))
                continue;

            const int32_t dx = x - cx;
            // Outer ellipse: rx=44, ry=13 -> rx^2=1936, ry^2=169, rx^2 * ry^2 = 327184
            const int32_t dOuter = dx * dx * 169 + dySqOuter;

            if (dOuter <= 327184)
            {
                // Inner ellipse: rx=40, ry=10 -> rx^2=1600, ry^2=100, rx^2 * ry^2 = 160000
                const int32_t dInner = dx * dx * 100 + dySqInner;
                if (dInner <= 160000)
                {
                    uint16_t col = colGrass;
                    if (((x * 5 + y * 11) % 17) == 0)
                        col = colGrassDark;
                    else if (((x * 7 + y * 13) % 23) == 0)
                        col = colGrassLit;
                    row[x] = col;
                }
                else
                {
                    row[x] = (dy > 3) ? colRimBot : colRimTop;
                }
            }
            else
            {
                if (y < 68)
                    row[x] = ((y & 1) == 0) ? colSkyEven : colSkyOdd;
                else if (y == 68)
                    row[x] = colHorizon;
                else
                    row[x] = ((y & 1) == 0) ? colFloorEven : colFloorOdd;
            }
        }
    }
}

static void RenderLeftDexShell(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    VitaUiPlate_Draw(dst, pitch, clip, 6, 26, 118, 180, 5, kPlateDexRed, VITA_PLATE_NORMAL, blinkPhase, 18);

    // Iconic Pokédex cyan camera lens + 3 indicator LEDs
    VitaUiPlate_Draw(dst, pitch, clip, 11, 29, 14, 12, 3, kPlateDexLens, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUiArt_FillRect(dst, pitch, clip, 29, 31, 4, 4, UI_INK_RED);
    VitaUiArt_FillRect(dst, pitch, clip, 36, 31, 4, 4, UI_HP_YELLOW);
    VitaUiArt_FillRect(dst, pitch, clip, 43, 31, 4, 4, UI_HP_GREEN);

    const uint16_t seenCnt = sNationalMode ? GetNationalPokedexCount(FLAG_GET_SEEN) : GetHoennPokedexCount(FLAG_GET_SEEN);
    const uint16_t ownCnt  = sNationalMode ? GetNationalPokedexCount(FLAG_GET_CAUGHT) : GetHoennPokedexCount(FLAG_GET_CAUGHT);
    VitaUiPlate_Draw(dst, pitch, clip, 54, 28, 64, 14, 2, kPlateSageDark, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUi_DrawMiniPokeball(dst, pitch, 58, 31, true, false);
    char cntBuf[16];
    snprintf(cntBuf, sizeof(cntBuf), "%u / %u", ownCnt, seenCnt);
    VitaUiArt_DrawStrAsciiInPlate(dst, pitch, clip, VITA_FONT_SMALL_NARROW, cntBuf, 66, 28, 50, 14, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Cream sprite display screen with grass diorama & battle podium
    VitaUiPlate_Draw(dst, pitch, clip, 12, 48, 106, 86, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
    DrawPokedexPreviewPodium(dst, pitch, clip);

    if (sDexRowCount <= 0 || sCursor < 0 || sCursor >= sDexRowCount)
        return;

    const DexRow *const sel = &sDexRows[sCursor];
    if (sel->seen && sel->species > SPECIES_NONE && sel->species < NUM_SPECIES)
    {
        VitaUiArt_DrawMonFrontPic(dst, pitch, clip, sel->species, false, 33, 56, 64, false);
        VitaUiArt_DrawTypeBadge(dst, pitch, clip, gSpeciesInfo[sel->species].types[0], 14, 138);
        if (gSpeciesInfo[sel->species].types[1] != gSpeciesInfo[sel->species].types[0])
            VitaUiArt_DrawTypeBadge(dst, pitch, clip, gSpeciesInfo[sel->species].types[1], 48, 138);
        if (sel->caught)
            DrawFootprint16x16(dst, pitch, sel->species, 96, 136, UI_INK_LIGHT);
    }
    else
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_NORMAL, "?", 65, 82, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    }

    char numLine[32];
    if (sel->seen)
    {
        char spAsc[16];
        VitaUiArt_GbaToAscii(gSpeciesNames[sel->species], spAsc, sizeof(spAsc));
        snprintf(numLine, sizeof(numLine), "#%03u %s", sel->dexNum, spAsc);
    }
    else
    {
        snprintf(numLine, sizeof(numLine), "#%03u -----", sel->dexNum);
    }
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, numLine, 65, 158, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    if (sDetailOpen)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 12, 182, 106, 18, 3, kPlateSageDark, VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "B: BACK TO LIST", 65, 185, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
    else if (sel->seen)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "A: INSPECT ENTRY", 65, 186, UI_INK_MINT, VITA_UI_NO_SHADOW);
    }
}

static void DrawDexDescription(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t *const descGba,
    const int32_t startX,
    const int32_t startY,
    const int32_t maxWidth)
{
    if (dst == NULL || descGba == NULL)
        return;

    // Sustituye saltos rigidos de GBA y envuelve palabras dinamicamente al ancho del panel
    uint8_t wrapped[384];
    uint32_t wIdx = 0;
    const uint8_t *p = descGba;
    uint8_t lineBuf[128];
    uint32_t lineLen = 0;

    while (*p != EOS && wIdx < sizeof(wrapped) - 4)
    {
        while (*p == CHAR_SPACE || *p == CHAR_NEWLINE)
            p++;

        if (*p == EOS)
            break;

        uint8_t word[64];
        uint32_t wordLen = 0;
        while (*p != EOS && *p != CHAR_SPACE && *p != CHAR_NEWLINE && wordLen < sizeof(word) - 1)
        {
            word[wordLen++] = *p++;
        }
        word[wordLen] = EOS;

        uint8_t testLine[128];
        uint32_t testLen = 0;
        if (lineLen > 0)
        {
            for (uint32_t i = 0; i < lineLen; i++)
                testLine[testLen++] = lineBuf[i];
            testLine[testLen++] = CHAR_SPACE;
        }
        for (uint32_t i = 0; i < wordLen; i++)
            testLine[testLen++] = word[i];
        testLine[testLen] = EOS;

        const int32_t lineWidth = VitaUiArt_StrWidthGba(VITA_FONT_SMALL_NARROW, testLine);
        if (lineWidth <= maxWidth || lineLen == 0)
        {
            if (lineLen > 0)
            {
                lineBuf[lineLen++] = CHAR_SPACE;
                wrapped[wIdx++] = CHAR_SPACE;
            }
            for (uint32_t i = 0; i < wordLen; i++)
            {
                lineBuf[lineLen++] = word[i];
                wrapped[wIdx++] = word[i];
            }
        }
        else
        {
            wrapped[wIdx++] = CHAR_NEWLINE;
            lineLen = 0;
            for (uint32_t i = 0; i < wordLen; i++)
            {
                lineBuf[lineLen++] = word[i];
                wrapped[wIdx++] = word[i];
            }
        }
    }
    wrapped[wIdx] = EOS;

    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, wrapped, startX, startY, UI_INK_DARK, VITA_UI_NO_SHADOW);
}

static void RenderDexDetailRight(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase, const uint32_t animTick)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    static const char *const sTabs[DEX_TAB_COUNT] = { "INFO", "AREA", "CRY", "SIZE" };
    for (uint8_t t = 0; t < DEX_TAB_COUNT; t++)
    {
        const bool sel = (sDetailTab == t);
        VitaUiPlate_Draw(
            dst, pitch, clip,
            128 + (int32_t)t * 46, 26, 44, 15, 2,
            sel ? kPlateSage : kPlateSageDark,
            sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL,
            blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL_NARROW, sTabs[t],
            128 + (int32_t)t * 46, 26, 44, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    VitaUiPlate_Draw(dst, pitch, clip, 128, 43, 186, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
    if (sDexRowCount <= 0 || sCursor < 0 || sCursor >= sDexRowCount)
        return;

    const DexRow *const row = &sDexRows[sCursor];
    const struct PokedexEntry *const pe = (row->natDex <= NATIONAL_DEX_COUNT) ? &gPokedexEntries[row->natDex] : &gPokedexEntries[0];

    if (sDetailTab == DEX_TAB_INFO)
    {
        char catAsc[24];
        VitaUiArt_GbaToAscii(pe->categoryName, catAsc, sizeof(catAsc));
        char catLine[36];
        snprintf(catLine, sizeof(catLine), "%s POKEMON", catAsc);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, catLine, 136, 49, UI_INK_BLUE, VITA_UI_NO_SHADOW);

        char hwBuf[48];
        if (row->caught)
        {
            snprintf(hwBuf, sizeof(hwBuf), "HT: %u.%u m   WT: %u.%u kg",
                     pe->height / 10U, pe->height % 10U,
                     pe->weight / 10U, pe->weight % 10U);
        }
        else
        {
            snprintf(hwBuf, sizeof(hwBuf), "HT: ?.? m     WT: ?.? kg");
        }
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, hwBuf, 136, 65, UI_INK_DARK, VITA_UI_NO_SHADOW);
        VitaUiArt_FillRect(dst, pitch, clip, 136, 81, 170, 1, UI_BORDER_SOFT);

        if (row->caught && pe->description != NULL)
        {
            DrawDexDescription(dst, pitch, clip, pe->description, 136, 88, 166);
        }
        else
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "CATCH THIS POKEMON TO\nRECORD FULL DATA.", 136, 98, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
    }
    else if (sDetailTab == DEX_TAB_AREA)
    {
        const int32_t mx = 137;
        const int32_t my = 52;
        const int32_t mw = 168;
        const int32_t mh = 90;
        VitaUiMap_BlitHoennThumbnail(dst, pitch, mx, my, mw, mh);

        int32_t matches = 0;
        for (int32_t gy = 0; gy < 15; gy++)
        {
            for (int32_t gx = 0; gx < 28; gx++)
            {
                const uint16_t sec = GetRegionMapSecIdAt((u16)(gx + 1), (u16)(gy + 2));
                if (sec < MAPSEC_NONE && SpeciesAppearsInSection(row->species, sec))
                {
                    matches++;
                    if (blinkPhase < 160U)
                        VitaUiArt_FillRect(dst, pitch, clip, mx + gx * 6, my + gy * 6, 6, 6, UI_INK_RED);
                }
            }
        }
        VitaUiArt_DrawStrAsciiCenter(
            dst, pitch, clip, VITA_FONT_SMALL,
            matches > 0 ? "WILD HABITAT HIGHLIGHTED" : "AREA UNKNOWN IN HOENN",
            221, 154, matches > 0 ? UI_INK_RED : UI_INK_MUTED, VITA_UI_NO_SHADOW);
    }
    else if (sDetailTab == DEX_TAB_CRY)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 138, 56, 166, 76, 4, kPlateHeader, VITA_PLATE_NORMAL, blinkPhase, 0);
        for (int32_t i = 0; i < 38; i++)
        {
            int32_t amp = 3;
            if (sCryAnimFrames > 0)
            {
                const uint32_t h = ((uint32_t)i * 29U + animTick * 7U + row->species * 13U) & 31U;
                amp = 4 + (int32_t)(h % 26U);
            }
            const int32_t bx = 145 + i * 4;
            VitaUiArt_FillRect(dst, pitch, clip, bx, 94 - amp / 2, 2, amp, UI_INK_MINT);
        }
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "PRESS A TO PLAY CRY", 221, 152, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }
    else if (sDetailTab == DEX_TAB_SIZE)
    {
        const int32_t floorY = 148;
        VitaUiArt_FillRect(dst, pitch, clip, 144, floorY, 154, 2, UI_BORDER_SOFT);

        // Trainer silhouette on right, Pokemon scaled silhouette on left
        VitaUiArt_DrawTrainerFrontPic(dst, pitch, clip, gSaveBlock2Ptr->playerGender, 232, floorY - 64);

        int32_t monPx = 48;
        if (pe->pokemonScale > 0)
        {
            monPx = (64 * 256) / (int32_t)pe->pokemonScale;
            if (monPx < 20) monPx = 20;
            if (monPx > 64) monPx = 64;
        }
        if (row->species > SPECIES_NONE && row->species < NUM_SPECIES)
            VitaUiArt_DrawMonFrontPic(dst, pitch, clip, row->species, false, 160 + (64 - monPx) / 2, floorY - monPx, monPx, true);

        char cmpLine[40];
        snprintf(cmpLine, sizeof(cmpLine), "POKEMON: %u.%um   TRAINER: 1.5m", pe->height / 10U, pe->height % 10U);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, cmpLine, 221, 160, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }
}

void VitaUiPokedex_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase, const uint32_t animTick)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    RenderLeftDexShell(dst, pitch, blinkPhase);

    if (sDetailOpen)
    {
        RenderDexDetailRight(dst, pitch, blinkPhase, animTick);
        return;
    }

    // Right list header pills
    const bool natAvail = IsNationalPokedexEnabled();
    VitaUiPlate_Draw(
        dst, pitch, clip, 128, 26, 92, 15, 2,
        sNationalMode ? kPlateBlueMove : kPlateSage,
        natAvail ? VITA_PLATE_NORMAL : VITA_PLATE_DISABLED, blinkPhase, 0);
    VitaUiArt_DrawStrAsciiInPlate(
        dst, pitch, clip, VITA_FONT_SMALL_NARROW,
        sNationalMode ? "NATIONAL (SEL)" : (natAvail ? "HOENN (SEL)" : "HOENN DEX"),
        128, 26, 92, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    VitaUiPlate_Draw(dst, pitch, clip, 224, 26, 90, 15, 2, kPlateSageDark, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUiArt_DrawStrAsciiInPlate(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "SEARCH (TRI)", 224, 26, 90, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    VitaUiPlate_Draw(dst, pitch, clip, 128, 43, 186, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);

    const uint8_t iconFrame = (uint8_t)((animTick >> 4) & 1U);
    for (int32_t r = 0; r < 7 && (sScroll + r) < sDexRowCount; r++)
    {
        const int32_t idx = sScroll + r;
        const DexRow *const row = &sDexRows[idx];
        const int32_t ry = 46 + r * 22;
        const bool isSel = (idx == sCursor);

        if (isSel)
            VitaUiPlate_Draw(dst, pitch, clip, 132, ry, 178, 20, 3, kPlateCardSel, VITA_PLATE_FOCUS, blinkPhase, 0);

        char numStr[12];
        snprintf(numStr, sizeof(numStr), "%03u", row->dexNum);
        VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, numStr, 136, ry + 3, UI_INK_MUTED, VITA_UI_NO_SHADOW);

        if (row->seen)
        {
            VitaUiArt_DrawMonIconCentred(dst, pitch, clip, row->species, isSel ? iconFrame : 0U, 172, ry + 10);
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, gSpeciesNames[row->species], 188, ry + 3, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }
        else
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "---------", 188, ry + 3, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }

        if (row->caught)
            VitaUi_DrawMiniPokeball(dst, pitch, 296, ry + 6, true, false);
        else if (row->seen)
            VitaUi_DrawArt4bpp(dst, pitch, sArtTinyX, 8, 8, 296, ry + 6, gVitaUiArtPal565);
    }

    if (sSearchOpen)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 134, 56, 174, 128, 4, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 16);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "POKEDEX FILTER & SORT", 221, 58, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        char l0[32], l1[32], l2[32];
        if (sFilterLetter == 0)
            snprintf(l0, sizeof(l0), "NAME: ANY");
        else
            snprintf(l0, sizeof(l0), "NAME: %c...", (char)('A' + sFilterLetter - 1));

        static const char *const sOrders[DEX_SORT_COUNT] = {
            "NUMBER", "A TO Z", "HEAVIEST", "LIGHTEST", "TALLEST", "SMALLEST"
        };
        snprintf(l1, sizeof(l1), "TYPE: %s", (sFilterType1 == 0) ? "ANY" : "");
        snprintf(l2, sizeof(l2), "ORDER: %s", sOrders[sFilterOrder]);

        const char *rows[4] = { l0, l1, l2, "RESET ALL FILTERS" };
        for (uint8_t f = 0; f < 4; f++)
        {
            const int32_t fy = 80 + (int32_t)f * 24;
            const bool fSel = (sSearchField == f);
            VitaUiPlate_Draw(
                dst, pitch, clip, 142, fy, 158, 20, 3,
                fSel ? kPlateSage : kPlateSageDark,
                fSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, rows[f], 148, fy + 3, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            if (f == 1 && sFilterType1 > 0)
                VitaUiArt_DrawTypeBadge(dst, pitch, clip, (uint8_t)(sFilterType1 - 1U), 258, fy + 4);
        }
    }
}
