#include <stdint.h>
#include "vita_ui_internal.h"

#include "global.h"
#include "3ds_bottom_art.h"
#include "battle_setup.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "data.h"
#include "event_data.h"
#include "gym_leader_rematch.h"
#include "match_call.h"
#include "overworld.h"
#include "pokenav.h"
#include "pokemon.h"
#include "region_map.h"
#include "party_menu.h"
#include "sound.h"
#include "string_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern const u8 *const gRibbonDescriptionPointers[][2];

enum PokenavSubTab
{
    PNAV_TAB_CONDITION = 0,
    PNAV_TAB_MATCH_CALL,
    PNAV_TAB_RIBBONS,
    PNAV_TAB_COUNT
};

typedef struct
{
    bool isSpecial;
    uint16_t id; // MC_HEADER_* cuando isSpecial, sino rematchIdx
    uint8_t mapSec;
    bool wantsRematch;
} NavCaller;

typedef struct
{
    uint8_t ribbonId;
    const char *name;
} NavRibbonItem;

static uint8_t sNavTab = PNAV_TAB_CONDITION;
static int32_t sMonIdx = 0;
static int32_t sCallCursor = 0;
static int32_t sCallScroll = 0;
static bool sCallDialogOpen = false;
static uint8_t sCallTextBuf[384];
static uint8_t sCallerNameBuf[32];
static int32_t sRibbonCursor = 0;
static int32_t sRibbonScroll = 0;

// Sprites 8x8 autenticos de las marcas de Pokemon (Circle, Square, Triangle, Heart) en 4bpp
static const uint8_t sMonMarkingTiles[8][32] = {
    // Circle Off (Tile 0)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x21, 0x00, 0x20, 0x11, 0x11, 0x02, 0x10, 0x11, 0x11, 0x01,
      0x10, 0x11, 0x11, 0x01, 0x20, 0x11, 0x11, 0x02, 0x00, 0x12, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00 },
    // Square Off (Tile 1)
    { 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x01, 0x10, 0x11, 0x11, 0x01, 0x10, 0x11, 0x11, 0x01,
      0x10, 0x11, 0x11, 0x01, 0x10, 0x11, 0x11, 0x01, 0x10, 0x11, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00 },
    // Triangle Off (Tile 2)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x12, 0x21, 0x00,
      0x00, 0x11, 0x11, 0x00, 0x20, 0x11, 0x11, 0x02, 0x10, 0x11, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00 },
    // Heart Off (Tile 3)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x20, 0x21, 0x12, 0x02, 0x10, 0x11, 0x11, 0x01,
      0x10, 0x11, 0x11, 0x01, 0x20, 0x11, 0x11, 0x02, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
    // Circle On (Tile 4)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x43, 0x00, 0x40, 0x33, 0x33, 0x04, 0x30, 0x33, 0x33, 0x03,
      0x30, 0x33, 0x33, 0x03, 0x40, 0x33, 0x33, 0x04, 0x00, 0x34, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00 },
    // Square On (Tile 9)
    { 0x00, 0x00, 0x00, 0x00, 0x30, 0x33, 0x33, 0x03, 0x30, 0x33, 0x33, 0x03, 0x30, 0x33, 0x33, 0x03,
      0x30, 0x33, 0x33, 0x03, 0x30, 0x33, 0x33, 0x03, 0x30, 0x33, 0x33, 0x03, 0x00, 0x00, 0x00, 0x00 },
    // Triangle On (Tile 18)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x30, 0x03, 0x00, 0x00, 0x34, 0x43, 0x00,
      0x00, 0x33, 0x33, 0x00, 0x40, 0x33, 0x33, 0x04, 0x30, 0x33, 0x33, 0x03, 0x00, 0x00, 0x00, 0x00 },
    // Heart On (Tile 35)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x40, 0x43, 0x34, 0x04, 0x30, 0x33, 0x33, 0x03,
      0x30, 0x33, 0x33, 0x03, 0x00, 0x33, 0x33, 0x00, 0x00, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const uint16_t sMarkPal[16] = {
    0x0000, // 0: Transparente
    0x5B6D, // 1: Marca inactiva cuerpo (RGB 90, 110, 104)
    0x3228, // 2: Marca inactiva contorno (RGB 52, 70, 64)
    0xFFFF, // 3: Marca activa cuerpo (blanco luminoso)
    0x1984, // 4: Marca activa contorno (RGB 24, 48, 38)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static NavCaller sCallers[96];
static int32_t sCallerCount = 0;

static inline uint16_t Blend565(const uint16_t bg, const uint16_t fg, const uint32_t alpha)
{
    const uint32_t rb_bg = bg & 0xF81FU;
    const uint32_t g_bg  = bg & 0x07E0U;
    const uint32_t rb_fg = fg & 0xF81FU;
    const uint32_t g_fg  = fg & 0x07E0U;

    const uint32_t rb = ((rb_fg * alpha + rb_bg * (256U - alpha)) >> 8) & 0xF81FU;
    const uint32_t g  = ((g_fg  * alpha + g_bg  * (256U - alpha)) >> 8) & 0x07E0U;

    return (uint16_t)(rb | g);
}

static void FillTriangle565Blend(
    uint16_t *const dst,
    const size_t pitch,
    int32_t x0, int32_t y0,
    int32_t x1, int32_t y1,
    int32_t x2, int32_t y2,
    const uint16_t col565,
    const uint32_t alpha)
{
    if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { int32_t t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { int32_t t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }

    if (y0 == y2 || y2 < 0 || y0 >= VITA_UI_HEIGHT)
        return;

    const int32_t dy02 = y2 - y0;
    const int32_t dy01 = y1 - y0;
    const int32_t dy12 = y2 - y1;

    for (int32_t y = y0; y <= y2; y++)
    {
        if (y < 0 || y >= VITA_UI_HEIGHT)
            continue;

        const int32_t xa = x0 + ((x2 - x0) * (y - y0)) / dy02;
        int32_t xb;
        if (y < y1 && dy01 > 0)
            xb = x0 + ((x1 - x0) * (y - y0)) / dy01;
        else if (dy12 > 0)
            xb = x1 + ((x2 - x1) * (y - y1)) / dy12;
        else
            xb = x1;

        int32_t left = (xa < xb) ? xa : xb;
        int32_t right = (xa < xb) ? xb : xa;
        if (left < 0) left = 0;
        if (right >= VITA_UI_WIDTH) right = VITA_UI_WIDTH - 1;

        uint16_t *const row = &dst[(size_t)y * pitch];
        for (int32_t x = left; x <= right; x++)
        {
            row[x] = Blend565(row[x], col565, alpha);
        }
    }
}

static void DrawLine565(
    uint16_t *const dst,
    const size_t pitch,
    int32_t x0,
    int32_t y0,
    const int32_t x1,
    const int32_t y1,
    const uint16_t col565)
{
    const int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    const int32_t sx = (x0 < x1) ? 1 : -1;
    const int32_t dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    const int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;)
    {
        if (x0 >= 0 && x0 < VITA_UI_WIDTH && y0 >= 0 && y0 < VITA_UI_HEIGHT)
            dst[(size_t)y0 * pitch + (size_t)x0] = col565;
        if (x0 == x1 && y0 == y1)
            break;
        const int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void DrawGrassPodium(uint16_t *const dst, const size_t pitch, const int32_t cx, const int32_t cy)
{
    const uint16_t grassCol = VitaUi_PackRgb565(178, 208, 182);
    const uint16_t grassBorder = VitaUi_PackRgb565(132, 168, 140);
    const int32_t rx = 34;
    const int32_t ry = 8;

    for (int32_t dy = -ry; dy <= ry; dy++)
    {
        const int32_t y = cy + dy;
        if (y < 0 || y >= VITA_UI_HEIGHT) continue;
        const float term = 1.0f - (float)(dy * dy) / (float)(ry * ry);
        if (term < 0.0f) continue;
        const int32_t dx = (int32_t)(sqrtf(term) * (float)rx);
        const int32_t left = cx - dx;
        const int32_t right = cx + dx;
        uint16_t *const row = &dst[(size_t)y * pitch];

        for (int32_t x = left; x <= right; x++)
        {
            if (x < 0 || x >= VITA_UI_WIDTH) continue;
            const bool isBorder = (x == left || x == right || dy == -ry || dy == ry);
            row[x] = isBorder ? grassBorder : grassCol;
        }
    }
}

static void RebuildMatchCallList(void)
{
    sCallerCount = 0;

    // 1. Contactos especiales de Match Call
    for (uint16_t i = 0; i < MC_HEADER_COUNT && sCallerCount < 96; i++)
    {
        if (!MatchCall_GetEnabled(i))
            continue;
        NavCaller *const c = &sCallers[sCallerCount++];
        c->isSpecial = true;
        c->id = i;
        c->mapSec = MatchCall_GetMapSec(i);
        const uint32_t remIdx = MatchCall_GetRematchTableIdx(i);
        c->wantsRematch = (remIdx < REMATCH_TABLE_ENTRIES) && (gSaveBlock1Ptr->trainerRematches[remIdx] != 0);
    }

    // 2. Entrenadores de ruta registrados
    for (uint16_t r = 0; r < REMATCH_SPECIAL_TRAINER_START && sCallerCount < 96; r++)
    {
        if (MatchCall_HasRematchId(r) || !FlagGet(TRAINER_REGISTERED_FLAGS_START + r))
            continue;
        NavCaller *const c = &sCallers[sCallerCount++];
        c->isSpecial = false;
        c->id = r;
        const struct MapHeader *const mh = Overworld_GetMapHeaderByGroupAndId(gRematchTable[r].mapGroup, gRematchTable[r].mapNum);
        c->mapSec = (mh != NULL) ? mh->regionMapSectionId : MAPSEC_NONE;
        c->wantsRematch = (gSaveBlock1Ptr->trainerRematches[r] != 0);
    }

    if (sCallCursor >= sCallerCount)
        sCallCursor = (sCallerCount > 0) ? (sCallerCount - 1) : 0;
    if (sCallScroll > sCallCursor)
        sCallScroll = sCallCursor;
}

static int32_t GatherMonRibbons(struct Pokemon *mon, NavRibbonItem outItems[32])
{
    int32_t n = 0;
    if (GetMonData(mon, MON_DATA_IS_EGG) != 0)
        return 0;

    static const char *const sContestRanks[4] = { "NORMAL", "SUPER", "HYPER", "MASTER" };
    static const char *const sContestCats[5] = { "COOL", "BEAUTY", "CUTE", "SMART", "TOUGH" };
    static char sRibbonNames[32][32];

    if (GetMonData(mon, MON_DATA_CHAMPION_RIBBON) && n < 32)
    {
        outItems[n].ribbonId = CHAMPION_RIBBON;
        outItems[n].name = "CHAMPION RIBBON";
        n++;
    }

    const int32_t rankFields[5] = {
        MON_DATA_COOL_RIBBON, MON_DATA_BEAUTY_RIBBON, MON_DATA_CUTE_RIBBON,
        MON_DATA_SMART_RIBBON, MON_DATA_TOUGH_RIBBON
    };
    const uint8_t baseIds[5] = {
        COOL_RIBBON_NORMAL, BEAUTY_RIBBON_NORMAL, CUTE_RIBBON_NORMAL,
        SMART_RIBBON_NORMAL, TOUGH_RIBBON_NORMAL
    };

    for (int32_t c = 0; c < 5; c++)
    {
        const uint8_t rank = (uint8_t)GetMonData(mon, rankFields[c]);
        for (uint8_t r = 0; r < rank && r < 4 && n < 32; r++)
        {
            outItems[n].ribbonId = (uint8_t)(baseIds[c] + r);
            snprintf(sRibbonNames[n], sizeof(sRibbonNames[0]), "%s %s", sContestCats[c], sContestRanks[r]);
            outItems[n].name = sRibbonNames[n];
            n++;
        }
    }

    if (GetMonData(mon, MON_DATA_WINNING_RIBBON) && n < 32)
    {
        outItems[n].ribbonId = WINNING_RIBBON;
        outItems[n].name = "WINNING LV.50";
        n++;
    }
    if (GetMonData(mon, MON_DATA_VICTORY_RIBBON) && n < 32)
    {
        outItems[n].ribbonId = VICTORY_RIBBON;
        outItems[n].name = "VICTORY OPEN";
        n++;
    }
    if (GetMonData(mon, MON_DATA_ARTIST_RIBBON) && n < 32)
    {
        outItems[n].ribbonId = ARTIST_RIBBON;
        outItems[n].name = "ARTIST MUSEUM";
        n++;
    }
    if (GetMonData(mon, MON_DATA_EFFORT_RIBBON) && n < 32)
    {
        outItems[n].ribbonId = EFFORT_RIBBON;
        outItems[n].name = "HARD WORKER";
        n++;
    }
    return n;
}

void VitaUiPokenav_Reset(void)
{
    sCallDialogOpen = false;
    sMonIdx = 0;
    sRibbonCursor = 0;
    sRibbonScroll = 0;
    RebuildMatchCallList();
}

bool VitaUiPokenav_HasActiveModal(void)
{
    return sCallDialogOpen;
}

bool VitaUiPokenav_Update(const VitaUiInput *in)
{
    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);

    if (sCallDialogOpen)
    {
        if ((in->newButtons & (aBtn | bBtn)) || in->touchPressed)
        {
            sCallDialogOpen = false;
            PlaySE(SE_POKENAV_HANG_UP);
            return true;
        }
        return false;
    }

    bool changed = false;

    // Conmutacion de pestañas superiores
    if (in->newButtons & VITA_BTN_SELECT)
    {
        sNavTab = (uint8_t)((sNavTab + 1U) % PNAV_TAB_COUNT);
        PlaySE(SE_SELECT);
        return true;
    }
    for (uint8_t t = 0; t < PNAV_TAB_COUNT; t++)
    {
        if (VitaUi_HitRect(in, 6 + (int32_t)t * 104, 26, 100, 15))
        {
            sNavTab = t;
            PlaySE(SE_SELECT);
            return true;
        }
    }

    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();

    if (sNavTab == PNAV_TAB_CONDITION || sNavTab == PNAV_TAB_RIBBONS)
    {
        if (partyCnt > 0)
        {
            // Ciclado de Pokémon con D-Pad o flechas tactiles
            if (in->newButtons & (VITA_BTN_UP | VITA_BTN_LEFT) || VitaUi_HitRect(in, 12, 176, 22, 18))
            {
                sMonIdx = (sMonIdx <= 0) ? (partyCnt - 1) : (sMonIdx - 1);
                sRibbonCursor = 0;
                sRibbonScroll = 0;
                PlaySE(SE_SELECT);
                changed = true;
            }
            else if (in->newButtons & (VITA_BTN_DOWN | VITA_BTN_RIGHT) || VitaUi_HitRect(in, 84, 176, 22, 18))
            {
                sMonIdx = (sMonIdx + 1) % partyCnt;
                sRibbonCursor = 0;
                sRibbonScroll = 0;
                PlaySE(SE_SELECT);
                changed = true;
            }

            // Marcas tactiles individuales en Condition
            if (sNavTab == PNAV_TAB_CONDITION)
            {
                struct Pokemon *const mon = &gPlayerParty[sMonIdx];
                uint8_t marks = (uint8_t)GetMonData(mon, MON_DATA_MARKINGS);

                for (uint8_t m = 0; m < 4; m++)
                {
                    if (VitaUi_HitRect(in, 12 + (int32_t)m * 24, 144, 20, 18))
                    {
                        marks ^= (1U << m);
                        SetMonData(mon, MON_DATA_MARKINGS, &marks);
                        PlaySE(SE_SELECT);
                        return true;
                    }
                }
                if (in->newButtons & aBtn)
                {
                    marks = (uint8_t)((marks + 1U) & 0x0FU);
                    SetMonData(mon, MON_DATA_MARKINGS, &marks);
                    PlaySE(SE_SELECT);
                    changed = true;
                }
            }
            else if (sNavTab == PNAV_TAB_RIBBONS)
            {
                // Navegacion vertical en lista de cintas
                struct Pokemon *const mon = &gPlayerParty[sMonIdx];
                NavRibbonItem rItems[32];
                const int32_t rCount = GatherMonRibbons(mon, rItems);

                if (rCount > 0)
                {
                    if ((in->newButtons & VITA_BTN_UP) && sRibbonCursor > 0)
                    {
                        sRibbonCursor--;
                        PlaySE(SE_SELECT);
                        changed = true;
                    }
                    else if ((in->newButtons & VITA_BTN_DOWN) && sRibbonCursor + 1 < rCount)
                    {
                        sRibbonCursor++;
                        PlaySE(SE_SELECT);
                        changed = true;
                    }
                    if (sRibbonCursor < sRibbonScroll) sRibbonScroll = sRibbonCursor;
                    if (sRibbonCursor >= sRibbonScroll + 5) sRibbonScroll = sRibbonCursor - 4;

                    for (int32_t r = 0; r < 5 && (sRibbonScroll + r) < rCount; r++)
                    {
                        if (VitaUi_HitRect(in, 122, 50 + r * 22, 186, 20))
                        {
                            sRibbonCursor = sRibbonScroll + r;
                            PlaySE(SE_SELECT);
                            return true;
                        }
                    }
                }
            }
        }
    }
    else if (sNavTab == PNAV_TAB_MATCH_CALL)
    {
        if (!FlagGet(FLAG_HAS_MATCH_CALL))
            return changed;

        if (sCallerCount > 0)
        {
            if ((in->newButtons & VITA_BTN_UP) && sCallCursor > 0)
            {
                sCallCursor--;
                PlaySE(SE_SELECT);
                changed = true;
            }
            else if ((in->newButtons & VITA_BTN_DOWN) && sCallCursor + 1 < sCallerCount)
            {
                sCallCursor++;
                PlaySE(SE_SELECT);
                changed = true;
            }
            if (sCallCursor < sCallScroll) sCallScroll = sCallCursor;
            if (sCallCursor >= sCallScroll + 6) sCallScroll = sCallCursor - 5;

            // Touch en la lista de Match Call
            for (int32_t r = 0; r < 6 && (sCallScroll + r) < sCallerCount; r++)
            {
                const int32_t idx = sCallScroll + r;
                if (VitaUi_HitRect(in, 12, 48 + r * 25, 288, 23))
                {
                    if (sCallCursor == idx)
                    {
                        // Iniciar llamada directamente
                        const NavCaller *const c = &sCallers[sCallCursor];
                        sCallTextBuf[0] = EOS;
                        if (c->isSpecial)
                        {
                            const uint8_t *desc = NULL;
                            const uint8_t *name = NULL;
                            MatchCall_GetNameAndDesc(c->id, &desc, &name);
                            if (name != NULL)
                                StringCopy(sCallerNameBuf, name);
                            else
                                StringCopy(sCallerNameBuf, (const u8 *)"CONTACT");
                            MatchCall_GetMessage(c->id, sCallTextBuf);
                        }
                        else
                        {
                            const uint16_t trId = (uint16_t)GetTrainerIdxByRematchIdx(c->id);
                            StringCopy(sCallerNameBuf, gTrainers[trId].trainerName);
                            if (!SelectMatchCallMessage(trId, sCallTextBuf))
                                StringCopy(sCallTextBuf, gTrainers[trId].trainerName);
                        }
                        sCallDialogOpen = true;
                        PlaySE(SE_POKENAV_CALL);
                        return true;
                    }
                    else
                    {
                        sCallCursor = idx;
                        PlaySE(SE_SELECT);
                        return true;
                    }
                }
            }

            if (in->newButtons & aBtn)
            {
                const NavCaller *const c = &sCallers[sCallCursor];
                sCallTextBuf[0] = EOS;
                if (c->isSpecial)
                {
                    const uint8_t *desc = NULL;
                    const uint8_t *name = NULL;
                    MatchCall_GetNameAndDesc(c->id, &desc, &name);
                    if (name != NULL)
                        StringCopy(sCallerNameBuf, name);
                    else
                        StringCopy(sCallerNameBuf, (const u8 *)"CONTACT");
                    MatchCall_GetMessage(c->id, sCallTextBuf);
                }
                else
                {
                    const uint16_t trId = (uint16_t)GetTrainerIdxByRematchIdx(c->id);
                    StringCopy(sCallerNameBuf, gTrainers[trId].trainerName);
                    if (!SelectMatchCallMessage(trId, sCallTextBuf))
                        StringCopy(sCallTextBuf, gTrainers[trId].trainerName);
                }
                sCallDialogOpen = true;
                PlaySE(SE_POKENAV_CALL);
                return true;
            }
        }
    }

    return changed;
}

static void RenderConditionRadar(uint16_t *dst, const size_t pitch, struct Pokemon *mon)
{
    const VitaUiClip *const clip = VitaUi_FullClip();
    const int32_t cx = 216;
    const int32_t cy = 104;
    const float maxR = 42.0f;

    const uint8_t stats[5] = {
        (uint8_t)GetMonData(mon, MON_DATA_COOL),
        (uint8_t)GetMonData(mon, MON_DATA_BEAUTY),
        (uint8_t)GetMonData(mon, MON_DATA_CUTE),
        (uint8_t)GetMonData(mon, MON_DATA_SMART),
        (uint8_t)GetMonData(mon, MON_DATA_TOUGH)
    };
    static const char *const sAxisNames[5] = { "COOL", "BEAUTY", "CUTE", "SMART", "TOUGH" };
    const uint16_t sAxisColors[5] = {
        UI_INK_RED,
        UI_INK_BLUE,
        0xE2F0, // Rosa concurso
        UI_INK_MINT,
        UI_INK_GOLD
    };

    // 1. Pentagonos concentricos de referencia (25%, 50%, 75%, 100%)
    for (int32_t step = 1; step <= 4; step++)
    {
        const float r = (maxR * (float)step) / 4.0f;
        for (int32_t i = 0; i < 5; i++)
        {
            const float a0 = -1.5707963f + (float)i * 1.2566371f;
            const float a1 = -1.5707963f + (float)((i + 1) % 5) * 1.2566371f;
            const int32_t x0 = cx + (int32_t)(cosf(a0) * r);
            const int32_t y0 = cy + (int32_t)(sinf(a0) * r);
            const int32_t x1 = cx + (int32_t)(cosf(a1) * r);
            const int32_t y1 = cy + (int32_t)(sinf(a1) * r);
            DrawLine565(dst, pitch, x0, y0, x1, y1, (step == 4) ? UI_INK_DARK : UI_BORDER_SOFT);
        }
    }

    // 2. Ejes radiales y calculo de puntos por estadistica
    int32_t outX[5], outY[5], valX[5], valY[5];
    for (int32_t i = 0; i < 5; i++)
    {
        const float ang = -1.5707963f + (float)i * 1.2566371f;
        const float cs = cosf(ang);
        const float sn = sinf(ang);
        outX[i] = cx + (int32_t)(cs * maxR);
        outY[i] = cy + (int32_t)(sn * maxR);
        DrawLine565(dst, pitch, cx, cy, outX[i], outY[i], UI_BORDER_SOFT);

        // Radio proporcional al valor real (0..255), con radio base de 3 px
        const float valR = 3.0f + 39.0f * ((float)stats[i] / 255.0f);
        valX[i] = cx + (int32_t)(cs * valR);
        valY[i] = cy + (int32_t)(sn * valR);

        // Etiquetas con nombre y valor numerico real (e.g. "COOL 0", "BEAUTY 255")
        char statStr[16];
        snprintf(statStr, sizeof(statStr), "%s %u", sAxisNames[i], (uint32_t)stats[i]);
        const int32_t lx = cx + (int32_t)(cs * (maxR + 18.0f));
        const int32_t ly = cy + (int32_t)(sn * (maxR + 12.0f)) - 5;
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, statStr, lx, ly, sAxisColors[i], VITA_UI_NO_SHADOW);
    }

    // 3. Relleno poligonal translucido esmeralda del area de condicion
    const uint16_t radarEmerald = VitaUi_PackRgb565(48, 172, 118);
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t nxt = (i + 1) % 5;
        FillTriangle565Blend(dst, pitch, cx, cy, valX[i], valY[i], valX[nxt], valY[nxt], radarEmerald, 110);
    }

    // 4. Perimetro del poligono de valores y puntos en vertices
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t nxt = (i + 1) % 5;
        DrawLine565(dst, pitch, valX[i], valY[i], valX[nxt], valY[nxt], UI_INK_BLUE);
        VitaUiArt_FillRect(dst, pitch, clip, valX[i] - 1, valY[i] - 1, 3, 3, UI_INK_GOLD);
    }

    // 5. Barra de Sheen con numeracion clara y porcentaje
    const uint8_t sheen = (uint8_t)GetMonData(mon, MON_DATA_SHEEN);
    VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "SHEEN", 126, 176, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    VitaUiArt_FillRect(dst, pitch, clip, 168, 179, 88, 6, UI_BORDER_SOFT);
    const int32_t sw = (88 * (int32_t)sheen) / 255;
    if (sw > 0)
        VitaUiArt_FillRect(dst, pitch, clip, 168, 179, sw, 6, UI_INK_GOLD);

    char sheenBuf[16];
    snprintf(sheenBuf, sizeof(sheenBuf), "%u/255", (uint32_t)sheen);
    VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, sheenBuf, 304, 176, UI_INK_GOLD, VITA_UI_NO_SHADOW);
}

static void DrawWordWrappedCallText(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t *const gbaText,
    const int32_t startX,
    const int32_t startY,
    const int32_t maxWidth)
{
    if (dst == NULL || gbaText == NULL)
        return;

    uint8_t wrapped[384];
    uint32_t wIdx = 0;
    const uint8_t *p = gbaText;
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

    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, wrapped, startX, startY, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
}

void VitaUiPokenav_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase, const uint32_t animTick)
{
    (void)animTick;
    const VitaUiClip *const clip = VitaUi_FullClip();

    static const char *const sNavTabs[PNAV_TAB_COUNT] = { "CONDITION", "MATCH CALL", "RIBBONS" };
    for (uint8_t t = 0; t < PNAV_TAB_COUNT; t++)
    {
        const bool sel = (sNavTab == t);
        const bool locked = (t == PNAV_TAB_MATCH_CALL && !FlagGet(FLAG_HAS_MATCH_CALL));
        VitaUiPlate_Draw(
            dst, pitch, clip,
            6 + (int32_t)t * 104, 26, 100, 15, 2,
            sel ? kPlateSage : kPlateSageDark,
            locked ? VITA_PLATE_DISABLED : (sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL),
            blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(
            dst, pitch, clip, VITA_FONT_SMALL, sNavTabs[t],
            6 + (int32_t)t * 104 + 50, 27, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();

    if (sNavTab == PNAV_TAB_CONDITION)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 6, 43, 106, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 15);
        VitaUiPlate_Draw(dst, pitch, clip, 116, 43, 198, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
        if (partyCnt <= 0)
            return;

        struct Pokemon *const mon = &gPlayerParty[sMonIdx];
        const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
        uint8_t nick[POKEMON_NAME_LENGTH + 2];
        GetMonNickname(mon, nick);
        VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, nick, 12, 45, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        if (isEgg)
        {
            VitaUiArt_DrawMonIcon(dst, pitch, clip, SPECIES_EGG, 0, 27, 74, 2);
            VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "EGG HAS NO\nCONTEST STATS", 215, 112, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            return;
        }

        // Podio de cesped bajo el sprite del Pokemon
        DrawGrassPodium(dst, pitch, 59, 120);

        const uint16_t sp = (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
        VitaUiArt_DrawMonFrontPic(dst, pitch, clip, sp, IsMonShiny(mon), 27, 62, 64, false);

        // 4 Marcas tactiles individuales
        const uint8_t marks = (uint8_t)GetMonData(mon, MON_DATA_MARKINGS);
        for (uint8_t m = 0; m < 4; m++)
        {
            const bool on = ((marks >> m) & 1U) != 0U;
            const int32_t bx = 12 + (int32_t)m * 24;
            const int32_t by = 144;
            VitaUiPlate_Draw(
                dst, pitch, clip,
                bx, by, 20, 18, 2,
                on ? kPlateSage : kPlateSageDark,
                on ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL,
                blinkPhase, 0);
            const uint8_t *const tile = on ? sMonMarkingTiles[m + 4] : sMonMarkingTiles[m];
            VitaUiArt_DrawTiles4bpp(dst, pitch, clip, tile, 1, 1, bx + 6, by + 5, sMarkPal);
        }

        // Selector de Pokemon con botones flecha tactiles
        VitaUiPlate_Draw(dst, pitch, clip, 12, 176, 22, 18, 2, kPlateSage, VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "<", 23, 178, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        char idxStr[16];
        snprintf(idxStr, sizeof(idxStr), "%d / %d", (int32_t)(sMonIdx + 1), (int32_t)partyCnt);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, idxStr, 59, 178, UI_INK_BLUE, VITA_UI_NO_SHADOW);

        VitaUiPlate_Draw(dst, pitch, clip, 84, 176, 22, 18, 2, kPlateSage, VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, ">", 95, 178, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        RenderConditionRadar(dst, pitch, mon);
        return;
    }

    if (sNavTab == PNAV_TAB_MATCH_CALL)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 6, 43, 308, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
        if (!FlagGet(FLAG_HAS_MATCH_CALL))
        {
            VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "MATCH CALL NOT YET INSTALLED", 160, 114, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            return;
        }

        for (int32_t r = 0; r < 6 && (sCallScroll + r) < sCallerCount; r++)
        {
            const int32_t idx = sCallScroll + r;
            const NavCaller *const c = &sCallers[idx];
            const int32_t ry = 48 + r * 25;
            const bool sel = (idx == sCallCursor);

            if (sel)
                VitaUiPlate_Draw(dst, pitch, clip, 12, ry, 288, 23, 3, kPlateCardSel, VITA_PLATE_FOCUS, blinkPhase, 0);

            if (c->isSpecial)
            {
                const uint8_t *desc = NULL;
                const uint8_t *name = NULL;
                MatchCall_GetNameAndDesc(c->id, &desc, &name);
                if (name != NULL)
                    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, name, 18, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
                if (desc != NULL)
                    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, desc, 108, ry + 4, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            }
            else
            {
                const uint16_t trId = (uint16_t)GetTrainerIdxByRematchIdx(c->id);
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, gTrainers[trId].trainerName, 18, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gTrainerClassNames[gTrainers[trId].trainerClass], 108, ry + 4, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            }

            if (c->mapSec < MAPSEC_NONE)
            {
                uint8_t secGba[24];
                GetMapNameGeneric(secGba, c->mapSec);
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, secGba, 206, ry + 4, UI_INK_BLUE, VITA_UI_NO_SHADOW);
            }

            if (c->wantsRematch)
                VitaUi_DrawMiniPokeball(dst, pitch, 286, ry + 7, true, false);
        }

        // Barra de desplazamiento scrollbar lateral
        if (sCallerCount > 6)
        {
            VitaUiArt_FillRect(dst, pitch, clip, 305, 48, 3, 150, UI_BORDER_SOFT);
            const int32_t thumbH = (150 * 6) / sCallerCount;
            const int32_t thumbY = 48 + (150 - thumbH) * sCallScroll / (sCallerCount - 6);
            VitaUiArt_FillRect(dst, pitch, clip, 305, thumbY, 3, thumbH, UI_INK_MINT);
        }

        if (sCallDialogOpen)
        {
            VitaUiPlate_Draw(dst, pitch, clip, 18, 54, 284, 136, 4, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 16);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, "POKENAV CALL", 28, 58, UI_INK_MINT, VITA_UI_NO_SHADOW);
            VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, sCallerNameBuf, 110, 58, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            VitaUiArt_FillRect(dst, pitch, clip, 28, 74, 264, 1, UI_BORDER_SOFT);

            DrawWordWrappedCallText(dst, pitch, clip, sCallTextBuf, 28, 80, 264);

            VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "[A / TOUCH: HANG UP]", 160, 172, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
        return;
    }

    // Pestaña de Ribbons
    VitaUiPlate_Draw(dst, pitch, clip, 6, 43, 106, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 15);
    VitaUiPlate_Draw(dst, pitch, clip, 116, 43, 198, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);
    if (partyCnt <= 0)
        return;

    struct Pokemon *const mon = &gPlayerParty[sMonIdx];
    uint8_t nick[POKEMON_NAME_LENGTH + 2];
    GetMonNickname(mon, nick);
    VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL, nick, 12, 45, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
    if (!isEgg)
    {
        DrawGrassPodium(dst, pitch, 59, 120);
        VitaUiArt_DrawMonFrontPic(dst, pitch, clip, (uint16_t)GetMonData(mon, MON_DATA_SPECIES), IsMonShiny(mon), 27, 62, 64, false);
    }

    NavRibbonItem rItems[32];
    const int32_t ribCnt = GatherMonRibbons(mon, rItems);
    char cntStr[24];
    snprintf(cntStr, sizeof(cntStr), "RIBBONS: %d", (int32_t)ribCnt);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, cntStr, 59, 146, UI_INK_DARK, VITA_UI_NO_SHADOW);

    // Selector de Pokemon tactil en ribbons
    VitaUiPlate_Draw(dst, pitch, clip, 12, 176, 22, 18, 2, kPlateSage, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "<", 23, 178, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    char pIdxStr[16];
    snprintf(pIdxStr, sizeof(pIdxStr), "%d / %d", (int32_t)(sMonIdx + 1), (int32_t)partyCnt);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, pIdxStr, 59, 178, UI_INK_BLUE, VITA_UI_NO_SHADOW);

    VitaUiPlate_Draw(dst, pitch, clip, 84, 176, 22, 18, 2, kPlateSage, VITA_PLATE_NORMAL, blinkPhase, 0);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, ">", 95, 178, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    if (ribCnt == 0)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "NO RIBBONS EARNED YET", 215, 114, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    }
    else
    {
        for (int32_t i = 0; i < 5 && (sRibbonScroll + i) < ribCnt; i++)
        {
            const int32_t idx = sRibbonScroll + i;
            const int32_t ry = 48 + i * 22;
            const bool sel = (idx == sRibbonCursor);

            VitaUiPlate_Draw(dst, pitch, clip, 122, ry, 186, 20, 3, sel ? kPlateCardSel : kPlateCream, sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);

            char idxBuf[8];
            snprintf(idxBuf, sizeof(idxBuf), "[%02d]", idx + 1);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, idxBuf, 126, ry + 4, UI_INK_MINT, VITA_UI_NO_SHADOW);

            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sArtStar, 2, 2, 154, ry + 10, gVitaUiArtPal2_565);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, rItems[idx].name, 164, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }

        // Panel de inspeccion de descripcion de la cinta seleccionada
        if (sRibbonCursor >= 0 && sRibbonCursor < ribCnt)
        {
            const uint8_t rId = rItems[sRibbonCursor].ribbonId;
            VitaUiArt_FillRect(dst, pitch, clip, 122, 162, 186, 1, UI_BORDER_SOFT);

            if (gRibbonDescriptionPointers[rId][0] != NULL)
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gRibbonDescriptionPointers[rId][0], 124, 168, UI_INK_DARK, VITA_UI_NO_SHADOW);
            if (gRibbonDescriptionPointers[rId][1] != NULL)
                VitaUiArt_DrawStrGba(dst, pitch, clip, VITA_FONT_SMALL_NARROW, gRibbonDescriptionPointers[rId][1], 124, 182, UI_INK_MUTED, VITA_UI_NO_SHADOW);
        }
    }
}
