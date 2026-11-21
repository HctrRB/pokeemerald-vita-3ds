// POKéDEX of the new UI (design/stitch/pokedex, pokedex_detalle). List: HOENN / NATIONAL tabs,
// entry card (sprite, species, type, status, height / weight, footprint, flavour), SELECT orders
// the list, triangle plays the cry, X opens the detail of a caught POKéMON (INFO / CRY / SIZE).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "event_data.h"
#include "pokedex.h"
#include "pokemon.h"
#include "sound.h"
#include "constants/pokedex.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/trainers.h"

#define LIST_X 424
#define LIST_W 512
#define LIST_Y 106
#define ROW_H 40
#define ROWS 9

enum { ORDER_NUMBER, ORDER_AZ, ORDER_HEAVIEST, ORDER_LIGHTEST, ORDER_TALLEST, ORDER_SMALLEST, ORDER_COUNT };
static const char *const sOrderNames[ORDER_COUNT] = { UI_N("NUMERICAL"), UI_N("A TO Z"), UI_N("HEAVIEST"), UI_N("LIGHTEST"), UI_N("TALLEST"), UI_N("SMALLEST") };
static const char *const sModeTabs[2] = { UI_N("HOENN"), UI_N("NATIONAL") };

static int32_t sMode, sOrder;
static UiList sList;
static uint16_t sNat[NATIONAL_DEX_COUNT];   // national numbers in list order
static int32_t sCount;
static int32_t sTabX[2], sTabW[2];
static UiMenu sMenu;
static uint32_t sFrame;

// ---------- Data ----------
static bool Seen(uint16_t nat) { return GetSetPokedexFlag(nat, FLAG_GET_SEEN) != 0; }
static bool Caught(uint16_t nat) { return GetSetPokedexFlag(nat, FLAG_GET_CAUGHT) != 0; }

static void Name(uint16_t nat, char *out, size_t cap)
{
    u8 buf[POKEMON_NAME_LENGTH + 1];
    GetSpeciesName(buf, NationalPokedexNumToSpecies(nat));
    UiGame_Text(buf, out, cap);
}

// Sort keys of the list (names converted once per sort)
typedef struct { uint16_t nat; char name[12]; } SortKey;
static SortKey sSort[NATIONAL_DEX_COUNT];

static int32_t Compare(uint16_t a, uint16_t b);

static int SortCmp(const void *pa, const void *pb)
{
    const SortKey *a = pa, *b = pb;
    const int32_t c = sOrder == ORDER_AZ ? strcmp(a->name, b->name) : Compare(a->nat, b->nat);
    return c != 0 ? c : (int)a->nat - (int)b->nat;   // stable on the number
}

static int32_t Compare(uint16_t a, uint16_t b)
{
    switch (sOrder)
    {
    case ORDER_HEAVIEST: return (int32_t)gPokedexEntries[b].weight - (int32_t)gPokedexEntries[a].weight;
    case ORDER_LIGHTEST: return (int32_t)gPokedexEntries[a].weight - (int32_t)gPokedexEntries[b].weight;
    case ORDER_TALLEST: return (int32_t)gPokedexEntries[b].height - (int32_t)gPokedexEntries[a].height;
    case ORDER_SMALLEST: return (int32_t)gPokedexEntries[a].height - (int32_t)gPokedexEntries[b].height;
    default: return 0;
    }
}

static void BuildList(void)
{
    sCount = 0;
    const int32_t n = sMode == 0 ? HOENN_DEX_COUNT : NATIONAL_DEX_COUNT;
    for (int32_t i = 1; i <= n; i++)
    {
        const uint16_t nat = sMode == 0 ? SpeciesToNationalPokedexNum(HoennPokedexNumToSpecies((u16)i)) : (uint16_t)i;
        // Other orders only list caught POKéMON, as the game's search
        if (sOrder != ORDER_NUMBER && !Caught(nat))
            continue;
        sNat[sCount++] = nat;
    }
    if (sOrder != ORDER_NUMBER)
    {
        for (int32_t i = 0; i < sCount; i++)
        {
            sSort[i].nat = sNat[i];
            if (sOrder == ORDER_AZ)
                Name(sNat[i], sSort[i].name, sizeof(sSort[i].name));
        }
        qsort(sSort, (size_t)sCount, sizeof(sSort[0]), SortCmp);
        for (int32_t i = 0; i < sCount; i++)
            sNat[i] = sSort[i].nat;
    }
    UiList_Reset(&sList, sCount, ROWS);
}

static int32_t ListNumber(int32_t i)
{
    return sMode == 0 ? NationalToHoennOrder(sNat[i]) : sNat[i];
}

// ---------- Entry card ----------
static void HeightWeight(uint16_t nat, char *h, size_t hc, char *w, size_t wc)
{
#ifdef UI_METRIC_UNITS
    // Metric, as the European releases: the game stores decimetres and hectograms. The catalog
    // writes the decimal separator ("%u,%u m" in Spanish).
    const uint16_t dm = gPokedexEntries[nat].height, hg = gPokedexEntries[nat].weight;
    snprintf(h, hc, UI_TR("%u.%u m"), (unsigned)(dm / 10), (unsigned)(dm % 10));
    snprintf(w, wc, UI_TR("%u.%u kg"), (unsigned)(hg / 10), (unsigned)(hg % 10));
#else
    // Imperial units as the English game: decimetres -> feet / inches, hectograms -> lbs
    const uint32_t inches = (uint32_t)gPokedexEntries[nat].height * 10000 / 254;
    const uint32_t in = (inches + 5) / 10;
    snprintf(h, hc, "%u'%02u\"", (unsigned)(in / 12), (unsigned)(in % 12));
    const uint32_t lbs10 = (uint32_t)gPokedexEntries[nat].weight * 100000 / 4536;
    snprintf(w, wc, UI_TR("%u.%u lbs"), (unsigned)((lbs10 + 5) / 100), (unsigned)(((lbs10 + 5) / 10) % 10));
#endif
}

static void Types(uint16_t species, int32_t x, int32_t y)
{
    const uint8_t t0 = gSpeciesInfo[species].types[0], t1 = gSpeciesInfo[species].types[1];
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + t0), x, y, 2);
    if (t1 != t0)
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + t1), x + 70, y, 2);
}

static void Kv(int32_t x, int32_t y, int32_t w, const char *k)
{
    UiW_Inset(x, y, w, 56);
    UiText_Draw(UI_FACE_PS2P, 16, x + 10, y + 6, k, UI_COL_INK_SOFT);
}

static void Well(int32_t x, int32_t y, int32_t w, int32_t h)
{
    UiW_Inset(x, y, w, h);
}

static void BuildCard(void)
{
    UiW_Panel(16, 60, 400, 432);
    if (sCount == 0)
        return;
    const uint16_t nat = sNat[sList.sel], species = NationalPokedexNumToSpecies(nat);
    const bool seen = Seen(nat), own = Caught(nat);
    char buf[200], name[16];
    snprintf(buf, sizeof(buf), UI_TR("No.%03d"), (int)ListNumber(sList.sel));
    Name(nat, name, sizeof(name));
    UiW_PanelHead(16, 60, 400, buf, seen ? name : "-----");
    Well(32, 108, 156, 156);
    if (seen)
    {
        if (own) UiSpr_MonFront(species, false, 46, 122, 2);
        else UiSpr_MonFrontTint(species, 46, 122, 2, UI_RGBA(0, 0, 0, 255));
    }
    Kv(200, 108, 200, UI_TR("SPECIES"));
    if (seen) UiGame_Text(gPokedexEntries[nat].categoryName, buf, sizeof(buf)); else snprintf(buf, sizeof(buf), "?????");
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 210, 130, buf, UI_COL_INK);
    Kv(200, 166, 200, UI_TR("TYPE"));
    if (seen) Types(species, 210, 188); else UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 210, 188, "???", UI_COL_INK);
    Kv(200, 224, 200, UI_TR("STATUS"));
    if (own) UiDraw_Sprite(UI_SPR_BALL_POKE, 210, 254, 1);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, own ? 232 : 210, 246, own ? UI_TR("CAUGHT") : seen ? UI_TR("SEEN") : UI_TR("UNKNOWN"), UI_COL_INK);
    char h[16], w[16];
    if (own) HeightWeight(nat, h, sizeof(h), w, sizeof(w)); else { snprintf(h, sizeof(h), "???"); snprintf(w, sizeof(w), "???"); }
    Kv(32, 290, 140, UI_TR("HEIGHT"));
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 42, 312, h, UI_COL_INK);
    Kv(176, 290, 152, UI_TR("WEIGHT"));
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 186, 312, w, UI_COL_INK);
    Well(336, 290, 64, 56);
    if (own) UiSpr_Footprint(species, 344, 294, 3, UI_COL_INK);
    Well(32, 360, 368, 120);
    if (own) UiGame_Text(gPokedexEntries[nat].description, buf, sizeof(buf));
    else snprintf(buf, sizeof(buf), seen ? UI_TR("Catch it to see its POKéDEX entry.") : UI_TR("Not seen yet."));
    UiW_TextWrapped(44, 364, 344, 30, buf, UI_COL_INK, 3);
}

// ---------- List screen ----------
static void Enter(void)
{
    sMenu.open = false;
    sMode = 0;
    BuildList();
}

static void Resume(void) { BuildList(); }

static uint16_t sDetailNat;
static bool sCaughtPage;   // opened by a battle after a catch (Ui_OpenCaughtDexPage): one POKéMON
static int32_t sListCryTicks;

static void DrawDexCardBox(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t bg)
{
    UiW_Bevel(x, y, w, h, 5, UI_RGBA(40, 56, 64, 255), bg, bg, 3, UI_RGBA(96, 96, 112, 255), UI_RGBA(96, 96, 112, 255), 3);
}

static void DrawDexDescBox(int32_t x, int32_t y, int32_t w, int32_t h, const char *desc, uint32_t frame)
{
    UiW_GbaBox(x, y, w, h, 5, UI_RGBA(40, 56, 64, 255), UI_RGBA(248, 248, 248, 255), UI_RGBA(248, 248, 248, 255), UI_RGBA(216, 216, 216, 255), 3);
    UiDraw_Rect(x + 6, y + h - 5, w - 12, 3, UI_RGBA(184, 184, 184, 255));
    // Left & right amber tri-tone rails (GBA Pokedex entry frame)
    UiDraw_Rect(x + 3, y + 6, 5, h - 12, UI_RGBA(248, 112, 0, 255));
    UiDraw_Rect(x + 8, y + 6, 5, h - 12, UI_RGBA(248, 184, 0, 255));
    UiDraw_Rect(x + 13, y + 6, 4, h - 12, UI_RGBA(248, 232, 0, 255));
    UiDraw_Rect(x + 17, y + 6, 2, h - 12, UI_RGBA(40, 56, 64, 255));

    UiDraw_Rect(x + w - 19, y + 6, 2, h - 12, UI_RGBA(40, 56, 64, 255));
    UiDraw_Rect(x + w - 17, y + 6, 4, h - 12, UI_RGBA(248, 232, 0, 255));
    UiDraw_Rect(x + w - 13, y + 6, 5, h - 12, UI_RGBA(248, 184, 0, 255));
    UiDraw_Rect(x + w - 8, y + 6, 5, h - 12, UI_RGBA(248, 112, 0, 255));

    UiW_GbaTextWrapped(UI_FACE_VT323, 28, x + 28, y + 14, w - 58, 28, desc, UI_RGBA(40, 56, 64, 255), UI_RGBA(216, 216, 216, 255), 5);
    UiW_GbaArrowV(x + w - 34, y + h - 18 + (int32_t)((frame / 16) & 1) * 2, true, UI_RGBA(230, 8, 8, 255));
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sListCryTicks > 0)
        sListCryTicks--;
    const bool gba = gUiTheme->gbaFrame;
    if (sMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id >= 0) { sOrder = id; BuildList(); UiKit_Toast(UI_TR("Order: %s"), UiLocale_Lookup(sOrderNames[sOrder])); }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    const int32_t modes = IsNationalPokedexEnabled() ? 2 : 1;
    int32_t t = -1;
    if (gba && modes > 1)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, modes);
        if (ph != 0)
            t = (sMode + modes + ph) % modes;
    }
    else if (!gba)
        t = UiKit_TabHit(in, sTabX, sTabW, modes);
    if (in->pressed & VITA_BTN_L) t = (sMode + modes - 1) % modes;
    if (in->pressed & VITA_BTN_R) t = (sMode + 1) % modes;
    if (t >= 0 && t != sMode) { sMode = t; PlaySE(SE_SELECT); BuildList(); return true; }
    if ((in->pressed & VITA_BTN_SELECT) || (gba && UiHit(in, 26, 222, 478, 34)))
    {
        PlaySE(SE_SELECT);
        UiMenu_Open(&sMenu, UI_TR("ORDER"), 600, 120, 300);
        for (int32_t i = 0; i < ORDER_COUNT; i++) UiMenu_Add(&sMenu, UiLocale_Lookup(sOrderNames[i]), i);
        return true;
    }
    const bool playCryHit = gba && UiHit(in, 94, 292, 112, 42);
    if (((in->pressed & VITA_BTN_TRIANGLE) || playCryHit) && sCount && Seen(sNat[sList.sel]))
    {
        PlayCry_Normal(NationalPokedexNumToSpecies(sNat[sList.sel]), 0);
        sListCryTicks = 48;
    }
    if (sCount > 0 && (in->repeat & (VITA_BTN_LEFT | VITA_BTN_RIGHT)))
    {
        // Left / right: a page of 9 up or down
        sList.sel += (in->repeat & VITA_BTN_RIGHT) ? ROWS : -ROWS;
        if (sList.sel < 0) sList.sel = 0;
        if (sList.sel >= sCount) sList.sel = sCount - 1;
        sList.first = sList.sel - sList.sel % ROWS;
        UiList_Reset(&sList, sCount, ROWS);
        PlaySE(SE_SELECT);
    }
    const int32_t lx = gba ? 544 : LIST_X;
    const int32_t ly = gba ? 114 : LIST_Y;
    const int32_t lw = gba ? 374 : LIST_W;
    const int32_t rh = gba ? 45  : ROW_H;
    const int32_t a = UiList_Update(&sList, in, lx, ly, lw, rh, false);
    if (a >= 0)
    {
        if (Caught(sNat[a])) { PlaySE(SE_SELECT); sDetailNat = sNat[a]; sCaughtPage = false; UiNav_Push(&gUiDexDetailScreen); }
        else { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("Catch it to see its entry.")); }
    }
    return true;
}

static void BuildGba(void)
{
    char buf[220], name[16], seenBuf[36];
    const uint16_t seenTotal = sMode == 1 ? GetNationalPokedexCount(FLAG_GET_SEEN) : GetHoennPokedexCount(FLAG_GET_SEEN);
    const uint16_t ownTotal = sMode == 1 ? GetNationalPokedexCount(FLAG_GET_CAUGHT) : GetHoennPokedexCount(FLAG_GET_CAUGHT);
    snprintf(seenBuf, sizeof(seenBuf), UI_TR("SEEN %03u · OWN %03u"), (unsigned)seenTotal, (unsigned)ownTotal);

    const int32_t modes = IsNationalPokedexEnabled() ? 2 : 1;
    UiW_GbaTopBar(UI_GBA_BG_POKEDEX, 0, UI_TR("POKéDEX"), modes, sMode, false, sMode == 0 ? UI_TR("HOENN DEX") : UI_TR("NATIONAL DEX"));

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", UI_TR("BACK"), true);
    UiW_GbaHelpBadge(&rx, "SELECT", UI_TR("ORDER"), true);
    UiW_GbaHelpBadge(&rx, "TRI", UI_TR("CRY"), false);
    UiW_GbaHelpBadge(&rx, "X", UI_TR("DATA"), false);

    // Left column (14, 60, 504, 472)
    DrawDexCardBox(14, 60, 504, 210, UI_RGBA(248, 248, 248, 255));
    // 140x140 sprite well with grid (.dex-sprite-well)
    UiW_GbaBox(26, 70, 140, 140, 4, UI_RGBA(96, 96, 112, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(184, 184, 184, 255), 0);
    for (int32_t g = 16; g < 134; g += 16)
    {
        UiDraw_Rect(29 + g, 73, 1, 134, UI_RGBA(40, 56, 64, 36));
        UiDraw_Rect(29, 73 + g, 134, 1, UI_RGBA(40, 56, 64, 36));
    }

    const bool has = (sCount > 0);
    const uint16_t nat = has ? sNat[sList.sel] : 1;
    const uint16_t species = NationalPokedexNumToSpecies(nat);
    const bool seen = has && Seen(nat), own = has && Caught(nat);
    if (has && seen)
    {
        if (own) UiSpr_MonFront(species, false, 32, 76, 2);
        else UiSpr_MonFrontTint(species, 32, 76, 2, UI_RGBA(0, 0, 0, 255));
    }

    // Data column (.dex-name-banner: 178, 70, 326, 62)
    UiW_GbaBox(178, 70, 326, 62, 4, UI_RGBA(160, 160, 160, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), C_WHITE, 0);
    if (has)
    {
        if (own) UiDraw_Sprite(UI_SPR_BALL_POKE, 188, 79, 1);
        Name(nat, name, sizeof(name));
        snprintf(buf, sizeof(buf), UI_TR("No.%03d  %s"), (int)ListNumber(sList.sel), seen ? name : "-----");
        UiW_GbaText(UI_FACE_VT323, 28, own ? 210 : 188, 74, buf, UI_RGBA(40, 56, 64, 255), C_WHITE);
        if (seen)
        {
            const uint8_t t0 = gSpeciesInfo[species].types[0], t1 = gSpeciesInfo[species].types[1];
            if (t1 != t0)
            {
                UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + t0), 430, 80, 1);
                UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + t1), 466, 80, 1);
            }
            else
                UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + t0), 466, 80, 1);
            {
                // "%s POKéMON" as a whole: languages put the category on either side
                char cat[32];
                UiGame_Text(gPokedexEntries[nat].categoryName, cat, sizeof(cat));
                snprintf(buf, sizeof(buf), UI_TR("%s POKéMON"), cat);
            }
        }
        else
            snprintf(buf, sizeof(buf), UI_TR("????? POKéMON"));
        UiW_GbaText(UI_FACE_VT323, 24, 188, 102, buf, UI_RGBA(72, 80, 88, 255), C_WHITE);
    }

    // HT / WT + Footprint circle (.dex-foot-circle)
    char h[16], w[16];
    if (own) HeightWeight(nat, h, sizeof(h), w, sizeof(w));
    else { snprintf(h, sizeof(h), "???"); snprintf(w, sizeof(w), "???"); }
    UiW_GbaText(UI_FACE_VT323, 28, 178, 142, UI_TR("HT"), UI_RGBA(40, 56, 64, 255), UI_RGBA(216, 216, 216, 255));
    UiW_GbaText(UI_FACE_VT323, 28, 430 - UiText_Width(UI_FACE_VT323, 28, h), 142, h, UI_RGBA(40, 56, 64, 255), UI_RGBA(216, 216, 216, 255));
    UiDraw_Rect(178, 171, 252, 3, UI_RGBA(184, 184, 184, 255));
    UiW_GbaText(UI_FACE_VT323, 28, 178, 178, UI_TR("WT"), UI_RGBA(40, 56, 64, 255), UI_RGBA(216, 216, 216, 255));
    UiW_GbaText(UI_FACE_VT323, 28, 430 - UiText_Width(UI_FACE_VT323, 28, w), 178, w, UI_RGBA(40, 56, 64, 255), UI_RGBA(216, 216, 216, 255));
    UiDraw_Rect(178, 207, 252, 3, UI_RGBA(184, 184, 184, 255));

    UiW_GbaBox(444, 146, 60, 60, 30, UI_RGBA(160, 160, 160, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(184, 184, 184, 255), 0);
    if (own)
        UiSpr_Footprint(species, 458, 160, 2, UI_RGBA(40, 56, 64, 255));

    // Area / Order pill at bottom of info card (.dex-area-pill)
    UiW_GbaBox(26, 222, 478, 34, 4, UI_RGBA(96, 96, 112, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), C_WHITE, 0);
    snprintf(buf, sizeof(buf), UI_TR("STATUS: %s  ·  SORT: %s"), own ? UI_TR("CAUGHT") : seen ? UI_TR("SEEN") : UI_TR("UNKNOWN"), UiLocale_Lookup(sOrderNames[sOrder]));
    UiW_GbaTextMid(UI_FACE_VT323, 24, 38, 222, 34, buf, UI_RGBA(40, 56, 64, 255), C_WHITE);

    // Cry meter card (14, 278, 504, 70)
    DrawDexCardBox(14, 278, 504, 70, UI_RGBA(216, 216, 216, 255));
    UiW_GbaText(UI_FACE_VT323, 24, 28, 290, UI_TR("CRY"), UI_RGBA(40, 56, 64, 255), C_WHITE);
    UiW_GbaText(UI_FACE_VT323, 24, 28, 310, UI_TR("METER"), UI_RGBA(40, 56, 64, 255), C_WHITE);

    UiW_GbaBox(94, 292, 112, 42, 4, UI_RGBA(16, 21, 24, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(80, 100, 112, 255), 0);
    UiW_GbaKeyCap(102, 301, 24, "TRI", false);
    UiW_GbaArrowH(142, 307, true, UI_RGBA(106, 255, 255, 255));
    UiW_GbaTextMid(UI_FACE_VT323, 24, 158, 294, 38, UI_TR("PLAY"), UI_RGBA(248, 248, 248, 255), UI_GBA_SHAD_DARK);

    UiW_GbaBox(216, 290, 288, 46, 4, UI_RGBA(96, 96, 112, 255), UI_RGBA(0, 0, 40, 255), UI_RGBA(0, 0, 40, 255), 0, 0);
    static const uint8_t kWave[16] = { 20, 42, 74, 92, 82, 58, 88, 96, 66, 44, 72, 90, 56, 36, 60, 26 };
    for (int32_t b = 0; b < 16; b++)
    {
        int32_t pct = has && seen ? kWave[b] : 10;
        if (sListCryTicks > 0)
        {
            const int32_t mod = (int32_t)(((b * 37 + sFrame * 13 + nat) % 85) + 15);
            pct = mod;
        }
        else if (!own)
            pct = 12;
        const int32_t bh = 32 * pct / 100;
        UiDraw_RectV(226 + b * 17, 329 - bh, 12, bh, UI_RGBA(248, 232, 0, 255), UI_RGBA(48, 208, 72, 255));
    }

    // Bottom description box (14, 356, 504, 176)
    if (own) UiGame_Text(gPokedexEntries[nat].description, buf, sizeof(buf));
    else snprintf(buf, sizeof(buf), seen ? UI_TR("Catch it to record full POKéDEX data and area habitat.") : UI_TR("No data recorded yet."));
    DrawDexDescBox(14, 356, 504, 176, buf, sFrame);

    // Right amber list panel (532, 60, 414, 472)
    DrawDexCardBox(532, 60, 414, 472, UI_RGBA(248, 248, 248, 255));
    UiW_GbaBox(544, 70, 390, 36, 4, UI_RGBA(16, 21, 24, 255), UI_RGBA(40, 56, 64, 255), UI_RGBA(40, 56, 64, 255), 0, 0);
    UiW_GbaTextMid(UI_FACE_VT323, 28, 554, 70, 36, sMode == 0 ? UI_TR("HOENN POKéDEX") : UI_TR("NATIONAL DEX"), UI_RGBA(248, 232, 0, 255), UI_GBA_SHAD_DARK);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 926 - UiText_Width(UI_FACE_VT323, 24, seenBuf), 70, 36, seenBuf, UI_RGBA(48, 208, 72, 255), UI_GBA_SHAD_DARK);

    UiDraw_PushClip(544, 114, 374, ROWS * 45);
    for (int32_t r = 0; r < ROWS && sList.first + r < sCount; r++)
    {
        const int32_t i = sList.first + r;
        const int32_t ry = 114 + r * 45;
        const uint16_t rowNat = sNat[i];
        const bool rOn = (i == sList.sel), rSeen = Seen(rowNat), rOwn = Caught(rowNat);
        if (rOn)
        {
            UiW_GbaBox(544, ry + 1, 374, 42, 4, UI_RGBA(40, 56, 64, 255),
                       UI_RGBA(255, 238, 0, 255), UI_RGBA(248, 184, 0, 255), C_WHITE, 0);
            UiW_GbaArrowH(553, ry + 17, true, C_WHITE);
            UiW_GbaArrowH(552, ry + 16, true, UI_RGBA(230, 8, 8, 255));
        }
        else if (r < ROWS - 1)
            UiDraw_Rect(544, ry + 43, 374, 2, UI_RGBA(224, 224, 216, 255));

        snprintf(buf, sizeof(buf), UI_TR("No.%03d"), (int)ListNumber(i));
        UiW_GbaTextMid(UI_FACE_VT323, 24, 570, ry, 44, buf, rOn ? UI_RGBA(40, 56, 64, 255) : UI_RGBA(72, 80, 88, 255),
                       rOn ? C_WHITE : UI_RGBA(216, 216, 216, 255));
        if (rOwn)
            UiDraw_Sprite(UI_SPR_BALL_POKE, 640, ry + 14, 1);
        if (rSeen)
            UiSpr_MonIcon(NationalPokedexNumToSpecies(rowNat), 0, 662, ry + 6, 1);
        if (rSeen)
            Name(rowNat, name, sizeof(name));
        else
            snprintf(name, sizeof(name), "-----");
        UiW_GbaTextMid(UI_FACE_VT323, 28, 704, ry, 44, name, rOwn ? UI_RGBA(49, 49, 49, 255) : UI_RGBA(112, 120, 128, 255),
                       rOn ? C_WHITE : UI_RGBA(216, 216, 216, 255));
    }
    UiDraw_PopClip();
    UiW_ScrollBar(924, 114, ROWS * 45, sList.first, ROWS, sCount);

    if (sMenu.open)
        UiMenu_Draw(&sMenu);
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGba();
        return;
    }
    char buf[32];
    UiKit_Header(UI_TR("POKéDEX"), sModeTabs, IsNationalPokedexEnabled() ? 2 : 1, sMode, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), UI_TR("SEEN %u · OWN %u"), (unsigned)GetHoennPokedexCount(FLAG_GET_SEEN), (unsigned)GetHoennPokedexCount(FLAG_GET_CAUGHT));
    if (sMode == 1)
        snprintf(buf, sizeof(buf), UI_TR("SEEN %u · OWN %u"), (unsigned)GetNationalPokedexCount(FLAG_GET_SEEN), (unsigned)GetNationalPokedexCount(FLAG_GET_CAUGHT));
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    BuildCard();
    UiW_Panel(416 - 0 + 0, 60, 528, 432);
    UiW_PanelHead(416, 60, 528, sMode == 0 ? UI_TR("HOENN LIST") : UI_TR("NATIONAL LIST"), UiLocale_Lookup(sOrderNames[sOrder]));
    UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
    for (int32_t r = 0; r < ROWS && sList.first + r < sCount; r++)
    {
        const int32_t i = sList.first + r, y = LIST_Y + r * ROW_H;
        const uint16_t nat = sNat[i];
        const bool on = i == sList.sel, seen = Seen(nat), own = Caught(nat);
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        snprintf(buf, sizeof(buf), "%03d", (int)ListNumber(i));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, y + 4, buf, on ? ink : UI_COL_INK_SOFT);
        if (own) UiDraw_Sprite(UI_SPR_BALL_POKE, LIST_X + 84, y + 12, 1);
        if (seen) UiSpr_MonIcon(NationalPokedexNumToSpecies(nat), 0, LIST_X + 106, y + 2, 1);
        if (seen) Name(nat, buf, sizeof(buf)); else snprintf(buf, sizeof(buf), "-----");
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 146, y + 4, buf, own ? ink : UI_COL_INK_SOFT);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, sList.first, ROWS, sCount);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Details"));
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, UI_TR("Cry"));
    UiW_FooterKey(&fx, "SELECT", UI_TR("Order"));
    UiKit_FooterClock();
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
}

const UiScreen gUiDexScreen = { Enter, Update, Build, Resume };

// ---------- Detail: INFO / CRY / SIZE ----------
static const char *const sDetTabs[4] = { UI_N("INFO"), UI_N("AREA"), UI_N("CRY"), UI_N("SIZE") };
static int32_t sDetTab, sDetTabX[4], sDetTabW[4], sCryTicks;
static uint8_t sAreas[256];
static int32_t sAreaCount;
static uint16_t sAreaFor;

static void DetEnter(void)
{
    sDetTab = 0;
    sAreaFor = 0;
}

// Previous / next caught POKéMON of the list
static void StepCaught(int32_t d)
{
    for (int32_t k = 1; k < sCount; k++)
    {
        const int32_t i = (sList.sel + d * k + sCount * 4) % sCount;
        if (Caught(sNat[i])) { sList.sel = i; sDetailNat = sNat[i]; PlaySE(SE_SELECT); return; }
    }
}

static void Cry(void)
{
    PlayCry_Normal(NationalPokedexNumToSpecies(sDetailNat), 0);
    sCryTicks = 60;
}

static bool DetUpdate(const UiInput *in)
{
    if (sCryTicks > 0) sCryTicks--;
    // After a catch: A or B closes the page and the battle goes on (the GBA caught page)
    if (sCaughtPage && (in->pressed & (VITA_BTN_CIRCLE | (sDetTab != 2 ? VITA_BTN_CROSS : 0))))
    {
        PlaySE(SE_SELECT);
        sCaughtPage = false;
        UiNav_Close(NULL);
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = -1;
    if (gUiTheme->gbaFrame)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, 4);
        if (ph != 0)
            t = (sDetTab + 4 + ph) % 4;
    }
    else
        t = UiKit_TabHit(in, sDetTabX, sDetTabW, 4);
    if (in->pressed & VITA_BTN_L) t = (sDetTab + 3) % 4;
    if (in->pressed & VITA_BTN_R) t = (sDetTab + 1) % 4;
    if (t >= 0 && t != sDetTab) { sDetTab = t; PlaySE(SE_SELECT); if (t == 2) Cry(); }
    if (in->pressed & VITA_BTN_TRIANGLE || (sDetTab == 2 && (in->pressed & VITA_BTN_CROSS))) Cry();
    if (!sCaughtPage && (in->repeat & VITA_BTN_UP)) StepCaught(-1);
    if (!sCaughtPage && (in->repeat & VITA_BTN_DOWN)) StepCaught(1);
    return true;
}

static void DetBuildGba(void)
{
    const uint16_t nat = sDetailNat, species = NationalPokedexNumToSpecies(nat);
    char buf[220], name[16];
    Name(nat, name, sizeof(name));
    UiW_GbaTopBar(UI_GBA_BG_POKEDEX, 0, UI_TR("POKéDEX"), 4, sDetTab, false, UiLocale_Lookup(sDetTabs[sDetTab]));

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", sCaughtPage ? UI_TR("CLOSE") : UI_TR("BACK"), true);
    UiW_GbaHelpBadge(&rx, "TRI", UI_TR("CRY"), false);
    if (!sCaughtPage)
        UiW_GbaHelpBadge(&rx, "UD", UI_TR("STEP"), false);

    if (sDetTab == 1)
    {
        if (sAreaFor != species) { sAreaCount = UiGame_SpeciesAreas(species, sAreas, 256); sAreaFor = species; }
        static uint32_t sAreaFrame;
        UiMap_DrawArea(sAreas, ++sAreaFrame);
        DrawDexCardBox(704, 60, 242, 472, UI_RGBA(248, 248, 248, 255));
        UiW_GbaSectionStrip(716, 72, 218, 28, UI_GBA_TAG_GREEN, name, UI_TR("HABITAT"));
        snprintf(buf, sizeof(buf), UI_TR("No.%03u"), (unsigned)nat);
        UiW_GbaText(UI_FACE_PS2P, 16, 718, 110, buf, UI_RGBA(72, 80, 88, 255), C_WHITE);
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 718, 148, 214, 28, sAreaCount ? UI_TR("Red zones show where this wild POKéMON appears.") : UI_TR("AREA UNKNOWN"),
                           sAreaCount ? UI_RGBA(40, 56, 64, 255) : UI_RGBA(112, 120, 128, 255), C_WHITE, 5);
        return;
    }

    DrawDexCardBox(14, 60, 932, 472, UI_RGBA(248, 248, 248, 255));
    // Left 232x232 grid well
    UiW_GbaBox(32, 78, 232, 232, 4, UI_RGBA(96, 96, 112, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(184, 184, 184, 255), 0);
    for (int32_t g = 16; g < 226; g += 16)
    {
        UiDraw_Rect(35 + g, 81, 1, 226, UI_RGBA(40, 56, 64, 36));
        UiDraw_Rect(35, 81 + g, 226, 1, UI_RGBA(40, 56, 64, 36));
    }
    UiSpr_MonFront(species, false, 52, 98, 3);

    if (sDetTab == 0)
    {
        UiW_GbaBox(280, 78, 354, 64, 4, UI_RGBA(160, 160, 160, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), C_WHITE, 0);
        snprintf(buf, sizeof(buf), UI_TR("No.%03u  %s"), (unsigned)nat, name);
        UiW_GbaText(UI_FACE_PS2P, 16, 294, 88, buf, UI_RGBA(40, 56, 64, 255), C_WHITE);
        {
            // "%s POKéMON" as a whole: languages put the category on either side
            char cat[32];
            UiGame_Text(gPokedexEntries[nat].categoryName, cat, sizeof(cat));
            snprintf(buf, sizeof(buf), UI_TR("%s POKéMON"), cat);
        }
        UiW_GbaText(UI_FACE_VT323, 28, 294, 110, buf, UI_RGBA(72, 80, 88, 255), C_WHITE);

        UiW_GbaSectionStrip(280, 150, 354, 26, UI_GBA_TAG_GREEN, UI_TR("TYPE"), NULL);
        Types(species, 290, 184);

        char h[16], w[16];
        HeightWeight(nat, h, sizeof(h), w, sizeof(w));
        UiW_GbaSectionStrip(280, 226, 354, 26, UI_GBA_TAG_TEAL, UI_TR("SIZE & WEIGHT"), NULL);
        snprintf(buf, sizeof(buf), UI_TR("HT %s   ·   WT %s"), h, w);
        UiW_GbaText(UI_FACE_VT323, 32, 290, 260, buf, UI_RGBA(40, 56, 64, 255), C_WHITE);

        UiW_GbaBox(654, 78, 272, 232, 4, UI_RGBA(160, 160, 160, 255), UI_RGBA(236, 236, 230, 255), UI_RGBA(236, 236, 230, 255), C_WHITE, 0);
        UiW_GbaSectionStrip(664, 88, 252, 26, UI_GBA_TAG_SLATE, UI_TR("FOOTPRINT"), NULL);
        UiW_GbaBox(742, 136, 96, 96, 48, UI_RGBA(160, 160, 160, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(216, 216, 216, 255), UI_RGBA(184, 184, 184, 255), 0);
        UiSpr_Footprint(species, 766, 160, 3, UI_RGBA(40, 56, 64, 255));

        UiGame_Text(gPokedexEntries[nat].description, buf, sizeof(buf));
        DrawDexDescBox(32, 326, 896, 188, buf, sFrame);
    }
    else if (sDetTab == 2)
    {
        UiW_GbaSectionStrip(280, 78, 648, 28, UI_GBA_TAG_BLUE, UI_TR("POKéMON CRY ANALYZER"), name);
        UiW_GbaBox(280, 114, 648, 196, 4, UI_RGBA(96, 96, 112, 255), UI_RGBA(0, 0, 40, 255), UI_RGBA(0, 0, 40, 255), 0, 0);
        for (int32_t x = 0; x < 42; x++)
        {
            const int32_t phase = (int32_t)((x * 37 + nat * 13) % 100);
            const int32_t amp = sCryTicks > 0 ? 12 + phase * sCryTicks * 3 / 60 : 8;
            const int32_t bh = amp > 166 ? 166 : amp;
            UiDraw_RectV(300 + x * 15, 212 - bh / 2, 10, bh, UI_RGBA(248, 232, 0, 255), UI_RGBA(48, 208, 72, 255));
        }
        DrawDexDescBox(32, 326, 896, 188, UI_TR("Press X or TRIANGLE to play this POKéMON's cry again."), sFrame);
    }
    else
    {
        UiW_GbaSectionStrip(280, 78, 648, 28, UI_GBA_TAG_GOLD, UI_TR("HEIGHT COMPARISON"), NULL);
        UiW_GbaBox(280, 114, 648, 398, 4, UI_RGBA(160, 160, 160, 255), UI_RGBA(232, 236, 230, 255), UI_RGBA(232, 236, 230, 255), C_WHITE, 0);
        const int32_t ps = gPokedexEntries[nat].pokemonScale ? gPokedexEntries[nat].pokemonScale : 256;
        const int32_t ts = gPokedexEntries[nat].trainerScale ? gPokedexEntries[nat].trainerScale : 256;
        int32_t ms = 192 * 256 / ps, tsz = 192 * 256 / ts;
        if (ms > 340) ms = 340;
        if (tsz > 340) tsz = 340;
        UiSpr_TrainerSized(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 420 - tsz / 2, 480 - tsz, tsz, tsz);
        UiSpr_MonFrontSized(species, 760 - ms / 2, 480 - ms, ms, ms);
        UiGame_PlayerName(name, sizeof(name));
        snprintf(buf, sizeof(buf), UI_TR("Scale comparison with %s"), name);
        UiW_GbaText(UI_FACE_VT323, 28, 300, 126, buf, UI_RGBA(72, 80, 88, 255), C_WHITE);
    }
}

static void DetBuild(void)
{
    if (gUiTheme->gbaFrame)
    {
        DetBuildGba();
        return;
    }
    const uint16_t nat = sDetailNat, species = NationalPokedexNumToSpecies(nat);
    char buf[200], name[16];
    UiKit_Header(UI_TR("POKéDEX"), sDetTabs, 4, sDetTab, sDetTabX, sDetTabW, 12);
    Name(nat, name, sizeof(name));
    snprintf(buf, sizeof(buf), UI_TR("No.%03u %s"), (unsigned)nat, name);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    if (sDetTab == 1)
    {
        // AREA: where it lives in the wild (Hoenn map, red areas)
        if (sAreaFor != species) { sAreaCount = UiGame_SpeciesAreas(species, sAreas, 256); sAreaFor = species; }
        static uint32_t sAreaFrame;
        UiMap_DrawArea(sAreas, ++sAreaFrame);
        UiW_Panel(704, 60, 240, 432);
        UiText_Draw(UI_FACE_PS2P, 16, 720, 80, name, UI_COL_INK);
        UiW_TextWrapped(720, 110, 210, 30, sAreaCount ? UI_TR("Red areas: where it lives in the wild.") : UI_TR("AREA UNKNOWN"), sAreaCount ? UI_COL_INK : UI_COL_INK_SOFT, 4);
        int32_t fx = 16;
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        UiKit_FooterClock();
        return;
    }
    UiW_Panel(16, 60, 928, 432);
    Well(32, 76, 232, 232);
    UiSpr_MonFront(species, false, 52, 96, 3);
    if (sDetTab == 0)
    {
        Kv(280, 76, 352, UI_TR("NAME"));
        UiText_Draw(UI_FACE_PS2P, 16, 290, 104, name, UI_COL_INK);
        Kv(280, 136, 352, UI_TR("SPECIES"));
        {
            // "%s POKéMON" as a whole: languages put the category on either side
            char cat[32];
            UiGame_Text(gPokedexEntries[nat].categoryName, cat, sizeof(cat));
            snprintf(buf, sizeof(buf), UI_TR("%s POKéMON"), cat);
        }
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 290, 158, buf, UI_COL_INK);
        Kv(280, 196, 352, UI_TR("TYPE"));
        Types(species, 290, 218);
        char h[16], w[16];
        HeightWeight(nat, h, sizeof(h), w, sizeof(w));
        Kv(280, 256, 352, UI_TR("HEIGHT · WEIGHT"));
        snprintf(buf, sizeof(buf), "%s · %s", h, w);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 290, 278, buf, UI_COL_INK);
        Kv(648, 76, 280, UI_TR("FOOTPRINT"));
        Well(840, 82, 80, 44);
        UiSpr_Footprint(species, 856, 86, 2, UI_COL_INK);
        Well(32, 324, 896, 156);
        UiGame_Text(gPokedexEntries[nat].description, buf, sizeof(buf));
        UiW_TextWrapped(48, 332, 864, 30, buf, UI_COL_INK, 4);
    }
    else if (sDetTab == 2)
    {
        // Cry meter: bars that jump while the cry plays
        UiW_PanelDark(280, 76, 648, 232);
        for (int32_t x = 0; x < 48; x++)
        {
            const int32_t phase = (int32_t)((x * 37 + nat * 13) % 100);
            const int32_t amp = sCryTicks > 0 ? 10 + phase * sCryTicks * 3 / 60 : 6;
            const int32_t h = amp > 200 ? 200 : amp;
            UiDraw_RectV(300 + x * 13, 192 - h / 2, 9, h, UI_RGBA(110, 219, 151, 255), UI_RGBA(47, 158, 88, 255));
        }
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 300, 330, UI_TR("X or triangle: play the cry again."), UI_COL_INK_SOFT);
    }
    else
    {
        // Size compared with the player, with the Pokédex scales (256 = sprite at its size)
        Well(280, 76, 648, 404);
        const int32_t ps = gPokedexEntries[nat].pokemonScale ? gPokedexEntries[nat].pokemonScale : 256;
        const int32_t ts = gPokedexEntries[nat].trainerScale ? gPokedexEntries[nat].trainerScale : 256;
        int32_t ms = 192 * 256 / ps, tsz = 192 * 256 / ts;
        if (ms > 360) ms = 360;
        if (tsz > 360) tsz = 360;
        UiSpr_TrainerSized(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 400 - tsz / 2, 460 - tsz, tsz, tsz);
        UiSpr_MonFrontSized(species, 760 - ms / 2, 460 - ms, ms, ms);
        UiGame_PlayerName(name, sizeof(name));
        snprintf(buf, sizeof(buf), UI_TR("Size compared with %s"), name);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 296, 84, buf, UI_COL_INK_SOFT);
    }
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, sCaughtPage ? UI_TR("Close") : UI_TR("Back"));
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, UI_TR("Cry"));
    if (!sCaughtPage)
        UiW_FooterKey(&fx, UI_TR("UP/DOWN"), UI_TR("Previous / next"));
    UiKit_FooterClock();
}

const UiScreen gUiDexDetailScreen = { DetEnter, DetUpdate, DetBuild, NULL };

// After catching a new species (battle_script_commands.c Cmd_displaydexinfo): its POKéDEX page over
// the battle as a battle menu, with its cry, instead of the GBA caught page. Core 1.
bool Ui_OpenCaughtDexPage(uint16_t nationalNum)
{
    if (!Ui_IsEnabled() || Ui_IsOpen())
        return false;
    sDetailNat = nationalNum;
    sCaughtPage = true;
    if (!UiGame_OpenBattleMenu(&gUiDexDetailScreen))
    {
        sCaughtPage = false;
        return false;
    }
    PlayCry_Normal(NationalPokedexNumToSpecies(nationalNum), 0);
    return true;
}
