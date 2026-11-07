// POKéDEX of the new UI (design/stitch/pokedex, pokedex_detalle). List: HOENN / NATIONAL tabs,
// entry card (sprite, species, type, status, height / weight, footprint, flavour), SELECT orders
// the list, triangle plays the cry, X opens the detail of a caught POKéMON (INFO / CRY / SIZE).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
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
static const char *const sOrderNames[ORDER_COUNT] = { "NUMERICAL", "A TO Z", "HEAVIEST", "LIGHTEST", "TALLEST", "SMALLEST" };
static const char *const sModeTabs[2] = { "HOENN", "NATIONAL" };

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
    // Imperial units as the English game: decimetres -> feet / inches, hectograms -> lbs
    const uint32_t inches = (uint32_t)gPokedexEntries[nat].height * 10000 / 254;
    const uint32_t in = (inches + 5) / 10;
    snprintf(h, hc, "%u'%02u\"", (unsigned)(in / 12), (unsigned)(in % 12));
    const uint32_t lbs10 = (uint32_t)gPokedexEntries[nat].weight * 100000 / 4536;
    snprintf(w, wc, "%u.%u lbs", (unsigned)((lbs10 + 5) / 100), (unsigned)(((lbs10 + 5) / 10) % 10));
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
    snprintf(buf, sizeof(buf), "No.%03d", (int)ListNumber(sList.sel));
    Name(nat, name, sizeof(name));
    UiW_PanelHead(16, 60, 400, buf, seen ? name : "-----");
    Well(32, 108, 156, 156);
    if (seen)
    {
        if (own) UiSpr_MonFront(species, false, 46, 122, 2);
        else UiSpr_MonFrontTint(species, 46, 122, 2, UI_RGBA(0, 0, 0, 255));
    }
    Kv(200, 108, 200, "SPECIES");
    if (seen) UiGame_Text(gPokedexEntries[nat].categoryName, buf, sizeof(buf)); else snprintf(buf, sizeof(buf), "?????");
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 210, 130, buf, UI_COL_INK);
    Kv(200, 166, 200, "TYPE");
    if (seen) Types(species, 210, 188); else UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 210, 188, "???", UI_COL_INK);
    Kv(200, 224, 200, "STATUS");
    if (own) UiDraw_Sprite(UI_SPR_BALL_POKE, 210, 254, 1);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, own ? 232 : 210, 246, own ? "CAUGHT" : seen ? "SEEN" : "UNKNOWN", UI_COL_INK);
    char h[16], w[16];
    if (own) HeightWeight(nat, h, sizeof(h), w, sizeof(w)); else { snprintf(h, sizeof(h), "???"); snprintf(w, sizeof(w), "???"); }
    Kv(32, 290, 140, "HEIGHT");
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 42, 312, h, UI_COL_INK);
    Kv(176, 290, 152, "WEIGHT");
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 186, 312, w, UI_COL_INK);
    Well(336, 290, 64, 56);
    if (own) UiSpr_Footprint(species, 344, 294, 3, UI_COL_INK);
    Well(32, 360, 368, 120);
    if (own) UiGame_Text(gPokedexEntries[nat].description, buf, sizeof(buf));
    else snprintf(buf, sizeof(buf), seen ? "Catch it to see its POKéDEX entry." : "Not seen yet.");
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
static bool Update(const UiInput *in)
{
    sFrame++;
    if (sMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id >= 0) { sOrder = id; BuildList(); UiKit_Toast("Order: %s", sOrderNames[sOrder]); }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    const int32_t modes = IsNationalPokedexEnabled() ? 2 : 1;
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, modes);
    if (in->pressed & (VITA_BTN_L | VITA_BTN_R)) t = (sMode + 1) % modes;
    if (t >= 0 && t != sMode) { sMode = t; PlaySE(SE_SELECT); BuildList(); return true; }
    if (in->pressed & VITA_BTN_SELECT)
    {
        PlaySE(SE_SELECT);
        UiMenu_Open(&sMenu, "ORDER", 600, 120, 300);
        for (int32_t i = 0; i < ORDER_COUNT; i++) UiMenu_Add(&sMenu, sOrderNames[i], i);
        return true;
    }
    if (in->pressed & VITA_BTN_TRIANGLE && sCount && Seen(sNat[sList.sel]))
        PlayCry_Normal(NationalPokedexNumToSpecies(sNat[sList.sel]), 0);
    if (in->repeat & (VITA_BTN_LEFT | VITA_BTN_RIGHT))
    {
        // Left / right: a page of 9 up or down
        sList.sel += (in->repeat & VITA_BTN_RIGHT) ? ROWS : -ROWS;
        if (sList.sel < 0) sList.sel = 0;
        if (sList.sel >= sCount) sList.sel = sCount - 1;
        sList.first = sList.sel - sList.sel % ROWS;
        UiList_Reset(&sList, sCount, ROWS);
        PlaySE(SE_SELECT);
    }
    const int32_t a = UiList_Update(&sList, in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
    if (a >= 0)
    {
        if (Caught(sNat[a])) { PlaySE(SE_SELECT); sDetailNat = sNat[a]; UiNav_Push(&gUiDexDetailScreen); }
        else { PlaySE(SE_FAILURE); UiKit_Toast("Catch it to see its entry."); }
    }
    return true;
}

static void Build(void)
{
    char buf[32];
    UiKit_Header("POKéDEX", sModeTabs, IsNationalPokedexEnabled() ? 2 : 1, sMode, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), "SEEN %u · OWN %u", (unsigned)GetHoennPokedexCount(FLAG_GET_SEEN), (unsigned)GetHoennPokedexCount(FLAG_GET_CAUGHT));
    if (sMode == 1)
        snprintf(buf, sizeof(buf), "SEEN %u · OWN %u", (unsigned)GetNationalPokedexCount(FLAG_GET_SEEN), (unsigned)GetNationalPokedexCount(FLAG_GET_CAUGHT));
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    BuildCard();
    UiW_Panel(416 - 0 + 0, 60, 528, 432);
    UiW_PanelHead(416, 60, 528, sMode == 0 ? "HOENN LIST" : "NATIONAL LIST", sOrderNames[sOrder]);
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
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Details");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Cry");
    UiW_FooterKey(&fx, "SELECT", "Order");
    UiKit_FooterClock();
    if (sMenu.open)
        UiMenu_Draw(&sMenu);
}

const UiScreen gUiDexScreen = { Enter, Update, Build, Resume };

// ---------- Detail: INFO / CRY / SIZE ----------
static const char *const sDetTabs[4] = { "INFO", "AREA", "CRY", "SIZE" };
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
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = UiKit_TabHit(in, sDetTabX, sDetTabW, 4);
    if (in->pressed & VITA_BTN_L) t = (sDetTab + 3) % 4;
    if (in->pressed & VITA_BTN_R) t = (sDetTab + 1) % 4;
    if (t >= 0 && t != sDetTab) { sDetTab = t; PlaySE(SE_SELECT); if (t == 2) Cry(); }
    if (in->pressed & VITA_BTN_TRIANGLE || (sDetTab == 2 && (in->pressed & VITA_BTN_CROSS))) Cry();
    if (in->repeat & VITA_BTN_UP) StepCaught(-1);
    if (in->repeat & VITA_BTN_DOWN) StepCaught(1);
    return true;
}

static void DetBuild(void)
{
    const uint16_t nat = sDetailNat, species = NationalPokedexNumToSpecies(nat);
    char buf[200], name[16];
    UiKit_Header("POKéDEX", sDetTabs, 4, sDetTab, sDetTabX, sDetTabW, 12);
    Name(nat, name, sizeof(name));
    snprintf(buf, sizeof(buf), "No.%03u %s", (unsigned)nat, name);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    if (sDetTab == 1)
    {
        // AREA: where it lives in the wild (Hoenn map, red areas)
        if (sAreaFor != species) { sAreaCount = UiGame_SpeciesAreas(species, sAreas, 256); sAreaFor = species; }
        static uint32_t sAreaFrame;
        UiMap_DrawArea(sAreas, ++sAreaFrame);
        UiW_Panel(704, 60, 240, 432);
        UiText_Draw(UI_FACE_PS2P, 16, 720, 80, name, UI_COL_INK);
        UiW_TextWrapped(720, 110, 210, 30, sAreaCount ? "Red areas: where it lives in the wild." : "AREA UNKNOWN", sAreaCount ? UI_COL_INK : UI_COL_INK_SOFT, 4);
        int32_t fx = 16;
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiKit_FooterClock();
        return;
    }
    UiW_Panel(16, 60, 928, 432);
    Well(32, 76, 232, 232);
    UiSpr_MonFront(species, false, 52, 96, 3);
    if (sDetTab == 0)
    {
        Kv(280, 76, 352, "NAME");
        UiText_Draw(UI_FACE_PS2P, 16, 290, 104, name, UI_COL_INK);
        Kv(280, 136, 352, "SPECIES");
        UiGame_Text(gPokedexEntries[nat].categoryName, buf, sizeof(buf));
        strncat(buf, " POKéMON", sizeof(buf) - strlen(buf) - 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 290, 158, buf, UI_COL_INK);
        Kv(280, 196, 352, "TYPE");
        Types(species, 290, 218);
        char h[16], w[16];
        HeightWeight(nat, h, sizeof(h), w, sizeof(w));
        Kv(280, 256, 352, "HEIGHT · WEIGHT");
        snprintf(buf, sizeof(buf), "%s · %s", h, w);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 290, 278, buf, UI_COL_INK);
        Kv(648, 76, 280, "FOOTPRINT");
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
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 300, 330, "X or triangle: play the cry again.", UI_COL_INK_SOFT);
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
        snprintf(buf, sizeof(buf), "Size compared with %s", name);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 296, 84, buf, UI_COL_INK_SOFT);
    }
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Cry");
    UiW_FooterKey(&fx, "UP/DOWN", "Previous / next");
    UiKit_FooterClock();
}

const UiScreen gUiDexDetailScreen = { DetEnter, DetUpdate, DetBuild, NULL };
