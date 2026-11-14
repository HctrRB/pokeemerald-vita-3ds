// POKéNAV and FLY of the new UI (design/stitch/pokenav_*, vuelo). The Hoenn map component (D8):
// the GBA region map x3 with the game's 28x15 MAPSEC grid, cursor, area highlight, player head,
// x2 zoom. POKéNAV tabs: HOENN MAP (encounters of the area, ENCOUNTERS popup with filters),
// MATCH CALL, CONDITION (spider chart) and RIBBONS. FLY: the same map, X flies to the area.

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/vita_controls.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "sound.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"

// =====================================================================================
// Map component
// =====================================================================================
#define MAP_X 20
#define MAP_Y 64
#define MAP_W 672
#define MAP_H 360
#define GRID_W 28
#define GRID_H 15
#define POP_ROWS 6

static int32_t sCurX = 4, sCurY = 11;
static bool sZoom;
static uint8_t sFlyable[MAPSEC_NONE + 1];

static void MapEnter(bool fly)
{
    int32_t x, y;
    if (UiGame_PlayerMapPos(&x, &y)) { sCurX = x; sCurY = y; }
    sZoom = false;
    if (fly)
        for (uint16_t s = 0; s < MAPSEC_NONE; s++)
            sFlyable[s] = UiGame_CanFlyTo(s);
}

static int32_t CellPx(void) { return sZoom ? 48 : 24; }

static void MapOrigin(int32_t *ox, int32_t *oy)
{
    if (!sZoom) { *ox = 0; *oy = 0; return; }
    int32_t x = sCurX * 48 + 24 - MAP_W / 2, y = sCurY * 48 + 24 - MAP_H / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > GRID_W * 48 - MAP_W) x = GRID_W * 48 - MAP_W;
    if (y > GRID_H * 48 - MAP_H) y = GRID_H * 48 - MAP_H;
    *ox = x;
    *oy = y;
}

// D-pad moves the cursor (skipping nothing, as the game), a tap puts it on the cell
static bool MapUpdate(const UiInput *in)
{
    const int32_t bx = sCurX, by = sCurY;
    if (in->repeat & VITA_BTN_LEFT && sCurX > 0) sCurX--;
    if (in->repeat & VITA_BTN_RIGHT && sCurX < GRID_W - 1) sCurX++;
    if (in->repeat & VITA_BTN_UP && sCurY > 0) sCurY--;
    if (in->repeat & VITA_BTN_DOWN && sCurY < GRID_H - 1) sCurY++;
    if (UiHit(in, MAP_X, MAP_Y, MAP_W, MAP_H))
    {
        int32_t ox, oy;
        MapOrigin(&ox, &oy);
        sCurX = (in->tx - MAP_X + ox) / CellPx();
        sCurY = (in->ty - MAP_Y + oy) / CellPx();
        if (sCurX >= GRID_W) sCurX = GRID_W - 1;
        if (sCurY >= GRID_H) sCurY = GRID_H - 1;
    }
    if (sCurX != bx || sCurY != by) { PlaySE(SE_SELECT); return true; }
    return false;
}

static uint16_t MapSec(void) { return UiGame_MapSecAt(sCurX, sCurY); }

static void MapDraw(bool fly, uint32_t frame)
{
    UiDraw_Rect(MAP_X - 3, MAP_Y - 3, MAP_W + 6, MAP_H + 6, UI_COL_LINE_DARK);
    int32_t ox, oy;
    MapOrigin(&ox, &oy);
    const int32_t c = CellPx();
    UiDraw_PushClip(MAP_X, MAP_Y, MAP_W, MAP_H);
    UiDraw_Sprite(UI_SPR_HOENN_MAP, MAP_X - ox, MAP_Y - oy, sZoom ? 6 : 3);
    const uint16_t sec = MapSec();
    for (int32_t y = 0; y < GRID_H; y++)
        for (int32_t x = 0; x < GRID_W; x++)
        {
            const uint16_t s = UiGame_MapSecAt(x, y);
            const int32_t px = MAP_X + x * c - ox, py = MAP_Y + y * c - oy;
            if (s != MAPSEC_NONE && s == sec)
                UiDraw_Rect(px, py, c, c, UI_RGBA(246, 207, 87, 90));
            if (fly && s < MAPSEC_NONE && sFlyable[s])
                UiDraw_Frame(px + 2, py + 2, c - 4, c - 4, 2, UI_RGBA(63, 183, 107, 255));
        }
    int32_t hx, hy;
    if (UiGame_PlayerMapPos(&hx, &hy))
        UiDraw_Sprite(UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY, MAP_X + hx * c - ox + c / 2 - 16, MAP_Y + hy * c - oy + c / 2 - 16, 2);
    // Cursor: blinking white frame
    if ((frame / 16) & 1)
        UiDraw_Frame(MAP_X + sCurX * c - ox, MAP_Y + sCurY * c - oy, c, c, 3, UI_COL_ON_FRAME);
    else
        UiDraw_Frame(MAP_X + sCurX * c - ox, MAP_Y + sCurY * c - oy, c, c, 3, UI_COL_SELECT);
    UiDraw_PopClip();
}

// =====================================================================================
// POKéNAV
// =====================================================================================
enum { NAV_MAP, NAV_CALL, NAV_COND, NAV_RIB, NAV_COUNT };
static const char *const sTabs[NAV_COUNT] = { "HOENN MAP", "MATCH CALL", "CONDITION", "RIBBONS" };
static const char *const sMethods[UI_WILD_COUNT] = { "GRASS", "SURF", "FISH", "ROCK" };
static const char *const sTimes[3] = { "MORNING", "DAY", "NIGHT" };
static const char *const sRods[4] = { "ALL", "OLD", "GOOD", "SUPER" };

static int32_t sTab, sTabX[NAV_COUNT], sTabW[NAV_COUNT];
static uint32_t sFrame;
static int32_t sMethod, sPhase, sRod;
static UiWild sWild[24];
static int32_t sWildN;
static bool sEncOpen;
static UiList sEncList;
static UiList sCallList, sMonList;
static const UiCaller *sCallers;
static int32_t sCallerN;
static bool sCallOpen;
static char sCallText[400];
static int32_t sRibSel;
// POKEBLOCK popup of the CONDITION tab
static bool sFeedOpen;
static UiList sFeedList;
static UiBlock sBlocks[40];
static int32_t sBlockN;
#define FEED_X 300
#define FEED_Y 80
#define FEED_W 360
#define FEED_ROWS 8

static void RefreshWild(void)
{
    sWildN = UiGame_Wild(MapSec(), sMethod, sPhase, sMethod == UI_WILD_FISH ? sRod : 0, sWild, 24);
    UiList_Reset(&sEncList, sWildN, POP_ROWS);
}

static void NavEnter(void)
{
    sTab = NAV_MAP;
    MapEnter(false);
    sPhase = UiGame_CurrentTimePhase();
    sEncOpen = sCallOpen = sFeedOpen = false;
    sCallerN = UiGame_Callers(&sCallers);
    UiList_Reset(&sCallList, sCallerN, 9);
    UiList_Reset(&sMonList, UiGame_PartyCount(), 6);
    RefreshWild();
}

static void Filter(char *buf, size_t cap)
{
    if (sMethod == UI_WILD_GRASS && UiGame_TimedEncounters()) snprintf(buf, cap, "GRASS · %s", sTimes[sPhase]);
    else if (sMethod == UI_WILD_FISH) snprintf(buf, cap, "FISH · %s", sRod ? sRods[sRod] : "ALL RODS");
    else snprintf(buf, cap, "%s", sMethods[sMethod]);
}

// ENCOUNTERS popup (design/stitch/pokenav_mapa #encuentros): method sub-tabs, time of day (grass)
// or rod (fishing) sub-tabs, one row per POKéMON with its rod and levels
#define POP_X 122
#define POP_Y 48
#define POP_W 716
#define POP_H 448
static int32_t sSubX[2][4], sSubW[2][4];

static void SubTabs(int32_t row, int32_t x, int32_t y, const char *const *labels, int32_t n, int32_t cur)
{
    for (int32_t i = 0; i < n; i++)
    {
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, labels[i]) + 24;
        sSubX[row][i] = x;
        sSubW[row][i] = w;
        if (i == cur)
        {
            UiDraw_Rect(x, y, w, 30, gUiTheme->chipEdge);
            UiDraw_RectV(x + 2, y + 2, w - 4, 26, gUiTheme->chipTop, gUiTheme->chipBot);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y - 1, labels[i], gUiTheme->chipInk);
        }
        else
        {
            UiDraw_Rect(x, y, w, 30, gUiTheme->paperShade);
            UiDraw_Rect(x + 2, y + 2, w - 4, 26, UI_COL_PAPER1);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y - 1, labels[i], UI_COL_INK_SOFT);
        }
        x += w + 2;
    }
}

static int32_t SubHit(const UiInput *in, int32_t row, int32_t n, int32_t y)
{
    for (int32_t i = 0; i < n; i++)
        if (UiHit(in, sSubX[row][i], y, sSubW[row][i], 30))
            return i;
    return -1;
}

static bool UpdateEncPopup(const UiInput *in)
{
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); sEncOpen = false; return true; }
    int32_t m = sMethod;
    if (in->pressed & VITA_BTN_TRIANGLE) m = (sMethod + 1) % UI_WILD_COUNT;
    const int32_t hm = SubHit(in, 0, UI_WILD_COUNT, POP_Y + 54);
    if (hm >= 0) m = hm;
    if (m != sMethod) { sMethod = m; PlaySE(SE_SELECT); RefreshWild(); }
    const bool timed = sMethod == UI_WILD_GRASS && UiGame_TimedEncounters();
    int32_t d = (in->repeat & VITA_BTN_LEFT) ? -1 : (in->repeat & VITA_BTN_RIGHT) ? 1 : 0;
    const int32_t h2 = SubHit(in, 1, timed ? 3 : 4, POP_Y + 92);
    if (h2 >= 0 && timed) { sPhase = h2; d = 0; PlaySE(SE_SELECT); RefreshWild(); }
    if (h2 >= 0 && sMethod == UI_WILD_FISH) { sRod = h2; d = 0; PlaySE(SE_SELECT); RefreshWild(); }
    if (d && timed) { sPhase = (sPhase + d + 3) % 3; PlaySE(SE_SELECT); RefreshWild(); }
    if (d && sMethod == UI_WILD_FISH) { sRod = (sRod + d + 4) % 4; PlaySE(SE_SELECT); RefreshWild(); }
    UiList_Update(&sEncList, in, POP_X + 16, POP_Y + 134, POP_W - 32, 44, false);
    return true;
}

static void DrawEncPopup(void)
{
    char buf[48], name[24];
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    UiGame_MapSecName(MapSec(), name, sizeof(name));
    UiW_Window(POP_X, POP_Y, POP_W, POP_H, name);
    snprintf(buf, sizeof(buf), "%d POKéMON", (int)sWildN);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + POP_W - 20 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), POP_Y + 4, buf, UI_COL_ON_FRAME);
    SubTabs(0, POP_X + 16, POP_Y + 54, sMethods, UI_WILD_COUNT, sMethod);
    if (sMethod == UI_WILD_GRASS && UiGame_TimedEncounters()) SubTabs(1, POP_X + 16, POP_Y + 92, sTimes, 3, sPhase);
    if (sMethod == UI_WILD_FISH) SubTabs(1, POP_X + 16, POP_Y + 92, sRods, 4, sRod);
    for (int32_t r = 0; r < POP_ROWS && sEncList.first + r < sWildN; r++)
    {
        const int32_t i = sEncList.first + r, y = POP_Y + 134 + r * 44;
        const bool on = i == sEncList.sel;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiW_Row(POP_X + 16, y, POP_W - 32, 44, on, i);
        UiSpr_MonIcon(sWild[i].species, 0, POP_X + 44, y + 4, 1);
        UiGame_SpeciesName(sWild[i].species, name, sizeof(name));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 84, y + 6, name, ink);
        if (sWild[i].rod) UiW_Chip(POP_X + 420, y + 10, sRods[sWild[i].rod], false);
        snprintf(buf, sizeof(buf), "Lv%u-%u", (unsigned)sWild[i].minLv, (unsigned)sWild[i].maxLv);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + POP_W - 36 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 6, buf, ink);
    }
    if (sWildN == 0)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 40, POP_Y + 140, "NO WILD DATA FOR THIS FILTER", UI_COL_INK_SOFT);
    UiW_ScrollBar(POP_X + POP_W - 25, POP_Y + 134, POP_ROWS * 44, sEncList.first, POP_ROWS, sWildN);
    // Footer strip with the button guides
    const int32_t fy = POP_Y + POP_H - 46;
    UiDraw_Rect(POP_X + 3, fy, POP_W - 6, 2, gUiTheme->paperShade);
    UiDraw_Rect(POP_X + 3, fy + 2, POP_W - 6, 41, gUiTheme->paper1);
    int32_t fx = POP_X + 20;
    UiW_Glyph(fx, fy + 10, UI_GLYPH_TRIANGLE);
    fx += 30 + UiText_Draw(UI_FACE_PS2P, 16, fx + 30, fy + 15, "Method", UI_COL_INK) + 24;
    fx += UiText_Draw(UI_FACE_PS2P, 16, fx, fy + 15, "<> Time / rod", UI_COL_INK) + 24;
    UiW_Glyph(fx, fy + 10, UI_GLYPH_CIRCLE);
    UiText_Draw(UI_FACE_PS2P, 16, fx + 30, fy + 15, "Close", UI_COL_INK);
}

static void Call(void)
{
    if (sCallerN == 0) return;
    UiGame_CallMessage(&sCallers[sCallList.sel], sCallText, sizeof(sCallText));
    sCallOpen = true;
    PlaySE(SE_POKENAV_CALL);
}

static void OpenFeed(void)
{
    sBlockN = UiGame_Pokeblocks(sBlocks, 40);
    if (sBlockN == 0) { PlaySE(SE_FAILURE); UiKit_Toast("There are no POKéBLOCKS in the case."); return; }
    sFeedOpen = true;
    sFeedList.sel = sFeedList.first = 0;
    UiList_Reset(&sFeedList, sBlockN, FEED_ROWS);
    PlaySE(SE_SELECT);
}

static void UpdateFeed(const UiInput *in)
{
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); sFeedOpen = false; return; }
    if (in->tap && !(in->tx >= FEED_X && in->tx < FEED_X + FEED_W && in->ty >= FEED_Y && in->ty < FEED_Y + 56 + FEED_ROWS * 40 + 12))
    { sFeedOpen = false; return; }
    const int32_t a = UiList_Update(&sFeedList, in, FEED_X + 8, FEED_Y + 44, FEED_W - 16, 40, true);
    if (a < 0) return;
    UiMon m;
    UiGame_Mon(sMonList.sel, &m);
    char name[24];
    snprintf(name, sizeof(name), "%s", sBlocks[a].name);
    const int32_t r = UiGame_FeedPokeblock(sMonList.sel, sBlocks[a].slot);
    sFeedOpen = false;
    if (r == -3) { PlaySE(SE_FAILURE); UiKit_Toast("An EGG can't eat POKéBLOCKS."); return; }
    if (r == -2) { PlaySE(SE_FAILURE); UiKit_Toast("%s won't eat any more...", m.name); return; }
    PlaySE(SE_USE_ITEM);
    UiKit_Toast(r > 0 ? "%s happily ate the %s!" : r < 0 ? "%s disdainfully ate the %s." : "%s ate the %s.", m.name, name);
}

static void DrawFeed(void)
{
    char buf[16];
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    UiW_Window(FEED_X, FEED_Y, FEED_W, 56 + FEED_ROWS * 40, "POKéBLOCK CASE");
    UiDraw_PushClip(FEED_X + 8, FEED_Y + 44, FEED_W - 16, FEED_ROWS * 40);
    for (int32_t r = 0; r < FEED_ROWS && sFeedList.first + r < sBlockN; r++)
    {
        const int32_t i = sFeedList.first + r, y = FEED_Y + 44 + r * 40;
        const bool on = i == sFeedList.sel;
        UiW_Row(FEED_X + 8, y, FEED_W - 28, 40, on, i);
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, FEED_X + 24, y + 4, sBlocks[i].name, ink);
        snprintf(buf, sizeof(buf), "Lv%u F%u", (unsigned)sBlocks[i].level, (unsigned)sBlocks[i].feel);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, FEED_X + FEED_W - 40 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, on ? ink : UI_COL_INK_SOFT);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(FEED_X + FEED_W - 17, FEED_Y + 44, FEED_ROWS * 40, sFeedList.first, FEED_ROWS, sBlockN);
}

static bool NavUpdate(const UiInput *in)
{
    sFrame++;
    if (sEncOpen) return UpdateEncPopup(in);
    if (sFeedOpen) { UpdateFeed(in); return true; }
    if (sCallOpen)
    {
        if (in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE) || in->tap) { PlaySE(SE_SELECT); sCallOpen = false; }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, NAV_COUNT);
    if (in->pressed & VITA_BTN_L) t = (sTab + NAV_COUNT - 1) % NAV_COUNT;
    if (in->pressed & VITA_BTN_R) t = (sTab + 1) % NAV_COUNT;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); return true; }
    switch (sTab)
    {
    case NAV_MAP:
        if (MapUpdate(in)) RefreshWild();
        if (in->pressed & VITA_BTN_CROSS) { sZoom = !sZoom; PlaySE(SE_SELECT); }
        if (in->pressed & VITA_BTN_TRIANGLE) { sMethod = (sMethod + 1) % UI_WILD_COUNT; PlaySE(SE_SELECT); RefreshWild(); }
        if (in->pressed & VITA_BTN_SQUARE) { sEncOpen = true; PlaySE(SE_SELECT); UiList_Reset(&sEncList, sWildN, POP_ROWS); }
        break;
    case NAV_CALL:
        if (in->repeat & (VITA_BTN_LEFT | VITA_BTN_RIGHT))
        {
            // Left / right: a page of 9, as the Pokédex
            sCallList.sel += (in->repeat & VITA_BTN_RIGHT) ? 9 : -9;
            if (sCallList.sel >= sCallerN) sCallList.sel = sCallerN - 1;
            if (sCallList.sel < 0) sCallList.sel = 0;
            sCallList.first = sCallList.sel - sCallList.sel % 9;
            UiList_Reset(&sCallList, sCallerN, 9);
            PlaySE(SE_SELECT);
        }
        if (UiList_Update(&sCallList, in, 28, 106, 416, 40, false) >= 0 || UiHit(in, 496, 424, 208, 40)) Call();
        if (((in->pressed & VITA_BTN_TRIANGLE) || UiHit(in, 720, 424, 208, 40)) && sCallerN)
        {
            int32_t x, y;
            if (UiGame_MapSecPos(sCallers[sCallList.sel].mapSec, &x, &y)) { sCurX = x; sCurY = y; sTab = NAV_MAP; RefreshWild(); PlaySE(SE_SELECT); }
        }
        break;
    default:
    {
        const int32_t before = sMonList.sel;
        UiList_Update(&sMonList, in, 22, 66, 288, 44, true);
        if (sMonList.sel != before) sRibSel = 0;
        if (sTab == NAV_COND && ((in->pressed & VITA_BTN_SQUARE) || UiHit(in, 676, 424, 252, 40)))
        {
            if (UiGame_HasPokeblockCase()) OpenFeed();
            else { PlaySE(SE_FAILURE); UiKit_Toast("You don't have a POKéBLOCK CASE yet."); }
        }
        if (sTab == NAV_RIB)
        {
            uint8_t ids[32];
            const int32_t n = UiGame_Ribbons(sMonList.sel, ids, 32);
            const int32_t b = sRibSel;
            if (in->repeat & VITA_BTN_LEFT && sRibSel > 0) sRibSel--;
            if (in->repeat & VITA_BTN_RIGHT && sRibSel < n - 1) sRibSel++;
            for (int32_t i = 0; i < n && i < 16; i++)
                if (UiHit(in, 348 + (i % 8) * 72, 110 + (i / 8) * 72, 64, 64)) sRibSel = i;
            if (sRibSel != b) PlaySE(SE_SELECT);
        }
        break;
    }
    }
    return true;
}

static void PartyList(void)
{
    UiW_Panel(16, 60, 300, 432);
    for (int32_t i = 0; i < UiGame_PartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = 66 + i * 44;
        const bool on = i == sMonList.sel;
        UiW_Row(22, y, 288, 44, on, i);
        UiSpr_MonIcon(m.species, 0, 30, y + 4, 1);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 68, y + 6, m.name, on ? UI_COL_SELECT_INK : UI_COL_INK);
    }
}

static void BuildMapTab(void)
{
    char buf[48], name[24];
    MapDraw(false, sFrame);
    UiW_Panel(704, 60, 240, 56);
    UiGame_MapSecName(MapSec(), name, sizeof(name));
    UiText_Draw(UI_FACE_PS2P, 16, 720, 80, name, UI_COL_INK);
    UiW_Panel(704, 128, 240, 364);
    Filter(buf, sizeof(buf));
    UiW_PanelHead(704, 128, 240, buf, NULL);
    for (int32_t i = 0; i < sWildN && i < 7; i++)
    {
        const int32_t y = 174 + i * 44;
        UiW_Row(710, y, 228, 44, false, i);
        UiSpr_MonIcon(sWild[i].species, 0, 716, y + 4, 1);
        UiGame_SpeciesName(sWild[i].species, name, sizeof(name));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 752, y + 6, name, UI_COL_INK);
    }
    if (sWildN == 0)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 720, 180, "NO WILD DATA", UI_COL_INK_SOFT);
}

static void BuildCallTab(void)
{
    char buf[48];
    UiW_Panel(16, 60, 452, 432);
    UiW_PanelHead(16, 60, 452, "CONTACTS", NULL);
    for (int32_t r = 0; r < 9 && sCallList.first + r < sCallerN; r++)
    {
        const int32_t i = sCallList.first + r, y = 106 + r * 40;
        const bool on = i == sCallList.sel;
        UiW_Row(28, y, 416, 40, on, i);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 56, y + 4, sCallers[i].name, on ? UI_COL_SELECT_INK : UI_COL_INK);
        if (sCallers[i].rematch)
        {
            // Compact rematch mark inside the row (the full text is in the detail panel)
            UiW_Chip(28 + 416 - 46, y + 7, "!", true);
        }
    }
    UiW_ScrollBar(28 + 428 - 9, 106, 360, sCallList.first, 9, sCallerN);
    UiW_Panel(480, 60, 464, 432);
    if (sCallerN == 0) { UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 496, 80, "No contacts yet.", UI_COL_INK_SOFT); return; }
    const UiCaller *c = &sCallers[sCallList.sel];
    // Trainer: the battle sprite in a bevelled box
    UiW_Inset(496, 76, 152, 152);
    if (c->trainerPic != 0xFFFF) UiSpr_Trainer(c->trainerPic, 508, 88, 2);
    else UiDraw_Sprite(UI_SPR_START_POKENAV, 556, 136, 1);
    UiW_Inset(660, 76, 268, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 672, 84, "NAME", UI_COL_INK_SOFT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 672, 104, c->name, UI_COL_INK);
    UiW_Inset(660, 158, 268, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 672, 166, "TYPE", UI_COL_INK_SOFT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 672, 186, c->desc, UI_COL_INK);
    UiW_Inset(496, 240, 432, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 508, 248, "AREA", UI_COL_INK_SOFT);
    UiGame_MapSecName(c->mapSec, buf, sizeof(buf));
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 508, 268, buf, UI_COL_INK);
    // Rematch: ready or not; a ready trainer waits in that area for a new battle
    UiW_Inset(496, 322, 432, 82);
    UiText_Draw(UI_FACE_PS2P, 16, 508, 330, "REMATCH", UI_COL_INK_SOFT);
    UiW_TextWrapped(508, 350, 408, 30, c->rematch ? "Ready! Meet this trainer in the area to battle again." : "Not ready yet.",
                    c->rematch ? UI_COL_HP_HIGH : UI_COL_INK_SOFT, 2);
    UiW_Button(496, 424, 208, "CALL", UI_BTN_GREEN, false);
    UiW_Button(720, 424, 208, "SHOW ON MAP", UI_BTN_AMBER, false);
}

static void BuildCondTab(void)
{
    static const char *const names[5] = { "COOL", "BEAUTY", "CUTE", "SMART", "TOUGH" };
    static const uint32_t cols[5][2] = { { 0xFF3A5FE0u, 0xFF2742B8u }, { 0xFFE0A04Au, 0xFFB87A2Fu }, { 0xFFC88CF0u, 0xFFA060D0u }, { 0xFF6BD07Au, 0xFF3FA050u }, { 0xFF39C8E8u, 0xFF1C9CC0u } };
    PartyList();
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    uint8_t c[6];
    UiGame_Condition(sMonList.sel, c);
    UiW_Panel(328, 60, 320, 432);
    const float cx = 488, cy = 286, R = 112;
    float ax[5], ay[5];
    for (int32_t i = 0; i < 5; i++) { const float a = -1.5707963f + i * 1.2566371f; ax[i] = cosf(a); ay[i] = sinf(a); }
    // Rings at 25 / 50 / 75 / 100 %, axes from the centre to each vertex
    for (int32_t ring = 1; ring <= 4; ring++)
        for (int32_t i = 0; i < 5; i++)
        {
            const float f = R * ring / 4, j = (float)((i + 1) % 5);
            UiDraw_Line(cx + ax[i] * f, cy + ay[i] * f, cx + ax[(int)j] * f, cy + ay[(int)j] * f, 2, UI_RGBA(209, 200, 175, 255));
        }
    for (int32_t i = 0; i < 5; i++)
        UiDraw_Line(cx, cy, cx + ax[i] * R, cy + ay[i] * R, 2, UI_RGBA(199, 188, 158, 255));
    if (!m.egg)
    {
        float px[5], py[5];
        for (int32_t i = 0; i < 5; i++)
        {
            const float v = (c[i] < 8 ? 8 : c[i]) / 255.0f * R;
            px[i] = cx + ax[i] * v;
            py[i] = cy + ay[i] * v;
        }
        for (int32_t i = 0; i < 5; i++)
            UiDraw_Tri(cx, cy, px[i], py[i], px[(i + 1) % 5], py[(i + 1) % 5], UI_RGBA(63, 183, 107, 190));
        for (int32_t i = 0; i < 5; i++)
        {
            UiDraw_Line(px[i], py[i], px[(i + 1) % 5], py[(i + 1) % 5], 2, UI_RGBA(22, 39, 28, 255));
            UiDraw_Rect((int32_t)px[i] - 4, (int32_t)py[i] - 4, 8, 8, UI_RGBA(22, 39, 28, 255));
        }
    }
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t lx = (int32_t)(cx + ax[i] * (R + 26)) - UiText_Width(UI_FACE_VT323, UI_BODY_PX, names[i]) / 2;
        const int32_t ly = (int32_t)(cy + ay[i] * (R + 26)) - 16;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, lx, ly, names[i], UI_COL_INK);
    }
    UiW_Panel(660, 60, 284, 432);
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t y = 80 + i * 48;
        UiDraw_Sprite((uint16_t)(UI_SPR_CONTEST_FIRST + i), 676, y, 2);
        UiW_Bar(752, y + 10, 128, 12, c[i] * 100 / 255, cols[i][0], cols[i][1]);
        char v[8];
        snprintf(v, sizeof(v), "%u", (unsigned)c[i]);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 928 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, v), y, v, UI_COL_INK);
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "SHEEN %u / 255", (unsigned)c[5]);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 676, 330, buf, UI_COL_INK);
    UiW_Bar(676, 366, 252, 10, c[5] * 100 / 255, UI_RGBA(124, 192, 242, 255), UI_COL_EXP);
    // Always shown; without the POKéBLOCK CASE (SLATEPORT contest hall) it only explains why
    UiW_Button(676, 424, 252, "FEED POKéBLOCK", UiGame_HasPokeblockCase() ? UI_BTN_GREEN : UI_BTN_AMBER, false);
}

static void BuildRibTab(void)
{
    PartyList();
    UiW_Panel(328, 60, 616, 432);
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    char buf[64], desc[80];
    snprintf(buf, sizeof(buf), "%s · RIBBONS", m.name);
    UiW_PanelHead(328, 60, 616, buf, NULL);
    uint8_t ids[32];
    const int32_t n = UiGame_Ribbons(sMonList.sel, ids, 32);
    for (int32_t i = 0; i < 16; i++)
    {
        const int32_t x = 348 + (i % 8) * 72, y = 110 + (i / 8) * 72;
        UiW_Inset(x, y, 64, 64);
        if (i < n) UiDraw_Sprite((uint16_t)(UI_SPR_RIBBON_FIRST + ids[i]), x, y, 2);
        if (i < n && i == sRibSel) UiDraw_Frame(x, y, 64, 64, 3, UI_COL_SELECT);
    }
    UiDraw_Rect(348, 268, 576, 2, UI_COL_PAPER2);
    if (n == 0)
    {
        snprintf(buf, sizeof(buf), "%s has no ribbons yet. Win contests to earn them.", m.name);
        UiW_TextWrapped(348, 284, 576, 30, buf, UI_COL_INK_SOFT, 3);
        return;
    }
    UiGame_RibbonText(ids[sRibSel < n ? sRibSel : 0], buf, sizeof(buf), desc, sizeof(desc));
    UiText_Draw(UI_FACE_PS2P, 16, 348, 290, buf, UI_COL_INK);
    UiW_TextWrapped(348, 316, 576, 30, desc, UI_COL_INK, 3);
}

static void NavBuild(void)
{
    UiKit_Header("POKéNAV", sTabs, NAV_COUNT, sTab, sTabX, sTabW, 10);
    int32_t fx = 16;
    switch (sTab)
    {
    case NAV_MAP:
        BuildMapTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Zoom");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Method");
        UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "Encounters");
        break;
    case NAV_CALL:
        BuildCallTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Call");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Show on map");
        break;
    case NAV_COND:
        BuildCondTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, "Feed");
        break;
    default:
        BuildRibTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        UiW_FooterKey(&fx, "<>", "Ribbon");
        break;
    }
    UiKit_FooterClock();
    if (sEncOpen) DrawEncPopup();
    if (sFeedOpen) DrawFeed();
    if (sCallOpen)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(120, 150, 720, 240, sCallers[sCallList.sel].name);
        UiW_TextWrapped(144, 200, 672, 30, sCallText, UI_COL_INK, 6);
    }
}

const UiScreen gUiNavScreen = { NavEnter, NavUpdate, NavBuild, NULL };

// =====================================================================================
// FLY
// =====================================================================================
static uint8_t sFlySlot;

void UiFly_Set(uint8_t partySlot)
{
    sFlySlot = partySlot;
}

static void FlyEnter(void)
{
    MapEnter(true);
}

static uint16_t sFlySec;
static void FlyAfter(void)
{
    VitaGame_ExecuteFlyToMapSec(sFlySlot, sFlySec, 0);
}

static bool FlyUpdate(const UiInput *in)
{
    sFrame++;
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    MapUpdate(in);
    if (in->pressed & VITA_BTN_CROSS)
    {
        const uint16_t sec = MapSec();
        if (sec < MAPSEC_NONE && sFlyable[sec]) { PlaySE(SE_SELECT); sFlySec = sec; UiNav_Close(FlyAfter); }
        else { PlaySE(SE_FAILURE); UiKit_Toast("You can't fly there."); }
    }
    return true;
}

static void FlyBuild(void)
{
    static const char *const tab[1] = { "HOENN" };
    int32_t tx[1], tw[1];
    UiKit_Header("FLY", tab, 1, 0, tx, tw, 12);
    int32_t n = 0;
    for (uint16_t s = 0; s < MAPSEC_NONE; s++) n += sFlyable[s];
    char buf[48], name[24];
    snprintf(buf, sizeof(buf), "Destinations: %d", (int)n);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    MapDraw(true, sFrame);
    const uint16_t sec = MapSec();
    UiW_Panel(704, 60, 240, 120);
    UiGame_MapSecName(sec, name, sizeof(name));
    UiText_Draw(UI_FACE_PS2P, 16, 720, 80, name, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 720, 110, sec < MAPSEC_NONE && sFlyable[sec] ? "You can fly here." : "-", UI_COL_INK_SOFT);
    UiW_Panel(704, 192, 240, 200);
    UiMon m;
    UiGame_Mon(sFlySlot, &m);
    UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 8) & 1), 720, 210, 1);
    snprintf(buf, sizeof(buf), "%s will FLY", m.name);
    UiW_TextWrapped(760, 210, 176, 30, buf, UI_COL_INK, 2);
    UiW_TextWrapped(720, 280, 216, 30, "Green frames: towns you can fly to.", UI_COL_INK_SOFT, 3);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Fly here");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Cancel");
    UiKit_FooterClock();
}

const UiScreen gUiFlyScreen = { FlyEnter, FlyUpdate, FlyBuild, NULL };

// Pokédex AREA: the Hoenn map with the areas where the species lives
void UiMap_DrawArea(const uint8_t *areas, uint32_t frame)
{
    UiDraw_Rect(MAP_X - 3, MAP_Y - 3, MAP_W + 6, MAP_H + 6, UI_COL_LINE_DARK);
    UiDraw_PushClip(MAP_X, MAP_Y, MAP_W, MAP_H);
    UiDraw_Sprite(UI_SPR_HOENN_MAP, MAP_X, MAP_Y, 3);
    const uint32_t a = (frame / 20) & 1 ? 150 : 110;   // gentle blink
    for (int32_t y = 0; y < GRID_H; y++)
        for (int32_t x = 0; x < GRID_W; x++)
        {
            const uint16_t s = UiGame_MapSecAt(x, y);
            if (s < MAPSEC_NONE && areas[s])
                UiDraw_Rect(MAP_X + x * 24, MAP_Y + y * 24, 24, 24, UI_RGBA(217, 72, 59, a));
        }
    int32_t hx, hy;
    if (UiGame_PlayerMapPos(&hx, &hy))
        UiDraw_Sprite(UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY, MAP_X + hx * 24 - 4, MAP_Y + hy * 24 - 4, 2);
    UiDraw_PopClip();
}
