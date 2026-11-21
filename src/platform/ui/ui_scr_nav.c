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
#include "main.h"
#include "overworld.h"
#include "script.h"
#include "platform/ui.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"

// =====================================================================================
// Map component
// =====================================================================================
#define MAP_X 20
#define MAP_Y 64
#define MAP_W 672
#define MAP_H 360
#define GBA_MAP_X 29
#define GBA_MAP_Y 73
#define GBA_MAP_W 560
#define GBA_MAP_H 300
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

static void MapBounds(int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    if (gUiTheme->gbaFrame) { *x = GBA_MAP_X; *y = GBA_MAP_Y; *w = GBA_MAP_W; *h = GBA_MAP_H; }
    else                    { *x = MAP_X;     *y = MAP_Y;     *w = MAP_W;     *h = MAP_H; }
}

static int32_t CellPx(void)
{
    return gUiTheme->gbaFrame ? (sZoom ? 40 : 20) : (sZoom ? 48 : 24);
}

static void MapOrigin(int32_t *ox, int32_t *oy)
{
    if (!sZoom) { *ox = 0; *oy = 0; return; }
    int32_t mx, my, mw, mh;
    MapBounds(&mx, &my, &mw, &mh);
    const int32_t c = CellPx();
    int32_t x = sCurX * c + c / 2 - mw / 2, y = sCurY * c + c / 2 - mh / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > GRID_W * c - mw) x = GRID_W * c - mw;
    if (y > GRID_H * c - mh) y = GRID_H * c - mh;
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
    int32_t mx, my, mw, mh;
    MapBounds(&mx, &my, &mw, &mh);
    if (UiHit(in, mx, my, mw, mh))
    {
        int32_t ox, oy;
        MapOrigin(&ox, &oy);
        const int32_t c = CellPx();
        sCurX = (in->tx - mx + ox) / c;
        sCurY = (in->ty - my + oy) / c;
        if (sCurX >= GRID_W) sCurX = GRID_W - 1;
        if (sCurY >= GRID_H) sCurY = GRID_H - 1;
    }
    if (sCurX != bx || sCurY != by) { PlaySE(SE_SELECT); return true; }
    return false;
}

static uint16_t MapSec(void) { return UiGame_MapSecAt(sCurX, sCurY); }

static void MapDraw(bool fly, uint32_t frame)
{
    int32_t mx, my, mw, mh;
    MapBounds(&mx, &my, &mw, &mh);
    UiDraw_Rect(mx - 3, my - 3, mw + 6, mh + 6, gUiTheme->gbaFrame ? UI_RGBA(41, 49, 49, 255) : UI_COL_LINE_DARK);
    int32_t ox, oy;
    MapOrigin(&ox, &oy);
    const int32_t c = CellPx();
    UiDraw_PushClip(mx, my, mw, mh);
    if (gUiTheme->gbaFrame)
    {
        uint16_t su = 0, sv = 0, sw = 0, sh = 0;
        if (UiAssets_Sprite(UI_SPR_HOENN_MAP, &su, &sv, &sw, &sh))
            UiDraw_Quad(mx - ox, my - oy, GRID_W * c, GRID_H * c, su, sv, su + sw, sv + sh, 0xFFFFFFFFu);
    }
    else
        UiDraw_Sprite(UI_SPR_HOENN_MAP, mx - ox, my - oy, sZoom ? 6 : 3);
    const uint16_t sec = MapSec();
    for (int32_t y = 0; y < GRID_H; y++)
        for (int32_t x = 0; x < GRID_W; x++)
        {
            const uint16_t s = UiGame_MapSecAt(x, y);
            const int32_t px = mx + x * c - ox, py = my + y * c - oy;
            if (s != MAPSEC_NONE && s == sec)
            {
                UiDraw_Rect(px, py, c, c, gUiTheme->gbaFrame ? UI_RGBA(255, 230, 57, 88) : UI_RGBA(246, 207, 87, 90));
                if (gUiTheme->gbaFrame)
                    UiDraw_Frame(px, py, c, c, 1, UI_RGBA(255, 230, 57, 220));
            }
            if (fly && s < MAPSEC_NONE && sFlyable[s])
            {
                if (gUiTheme->gbaFrame)
                {
                    UiDraw_Frame(px + c / 2 - 5, py + c / 2 - 5, 10, 10, 2, UI_RGBA(255, 255, 255, 255));
                    UiDraw_Rect(px + c / 2 - 3, py + c / 2 - 3, 6, 6, UI_RGBA(90, 213, 131, 255));
                }
                else
                    UiDraw_Frame(px + 2, py + 2, c - 4, c - 4, 2, UI_RGBA(63, 183, 107, 255));
            }
        }
    int32_t hx, hy;
    if (UiGame_PlayerMapPos(&hx, &hy))
        UiDraw_Sprite(UiGame_PlayerIsMale() ? UI_SPR_HEAD_BRENDAN : UI_SPR_HEAD_MAY, mx + hx * c - ox + c / 2 - 16, my + hy * c - oy + c / 2 - 16, 2);
    // Cursor: blinking red/white frame (GBA) or theme focus
    if (gUiTheme->gbaFrame)
    {
        const uint32_t curCol = ((frame / 16) & 1) ? UI_RGBA(230, 8, 8, 255) : UI_RGBA(255, 255, 255, 255);
        UiDraw_Frame(mx + sCurX * c - ox - 1, my + sCurY * c - oy - 1, c + 2, c + 2, 2, UI_RGBA(255, 255, 255, 255));
        UiDraw_Frame(mx + sCurX * c - ox - 3, my + sCurY * c - oy - 3, c + 6, c + 6, 3, curCol);
    }
    else if ((frame / 16) & 1)
        UiDraw_Frame(mx + sCurX * c - ox, my + sCurY * c - oy, c, c, 3, UI_COL_ON_FRAME);
    else
        UiDraw_Frame(mx + sCurX * c - ox, my + sCurY * c - oy, c, c, 3, UI_COL_SELECT);
    UiDraw_PopClip();
}

// =====================================================================================
// POKéNAV
// =====================================================================================
enum { NAV_MAP, NAV_CALL, NAV_COND, NAV_RIB, NAV_COUNT };
static const char *const sTabs[NAV_COUNT] = { UI_N("HOENN MAP"), UI_N("MATCH CALL"), UI_N("CONDITION"), UI_N("RIBBONS") };
static const char *const sMethods[UI_WILD_COUNT] = { UI_N("GRASS"), UI_N("SURF"), UI_N("FISH"), UI_N("ROCK") };
static const char *const sTimes[3] = { UI_N("MORNING"), UI_N("DAY"), UI_N("NIGHT") };
static const char *const sRods[4] = { UI_N("ALL"), UI_N("OLD"), UI_N("GOOD"), UI_N("SUPER") };

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

// Modes: the START menu's PokéNav; a wall map of Hoenn (FieldShowRegionMap: the map only, to look
// at); Mr. Stone's tutorial in Rustboro (OpenPokenavForTutorial: MATCH CALL only, no way out before
// a call, as POKENAV_MODE_FORCE_CALL_READY). The tab of a fixed mode can't change.
enum { NAV_MODE_NORMAL, NAV_MODE_TOWN_MAP, NAV_MODE_TUTORIAL };
static uint8_t sNavMode, sNavModeNext;
static bool sTutorialCalled;

static bool Locked(void)
{
    return sNavMode != NAV_MODE_NORMAL;
}

static void NavEnter(void)
{
    sNavMode = sNavModeNext;           // set by the entry point that opened it, once
    sNavModeNext = NAV_MODE_NORMAL;
    sTutorialCalled = false;
    sTab = sNavMode == NAV_MODE_TUTORIAL ? NAV_CALL : NAV_MAP;
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
    if (sMethod == UI_WILD_GRASS && UiGame_TimedEncounters()) snprintf(buf, cap, UI_TR("GRASS · %s"), UiLocale_Lookup(sTimes[sPhase]));
    else if (sMethod == UI_WILD_FISH) snprintf(buf, cap, UI_TR("FISH · %s"), sRod ? UiLocale_Lookup(sRods[sRod]) : UI_TR("ALL RODS"));
    else snprintf(buf, cap, "%s", UiLocale_Lookup(sMethods[sMethod]));
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
        const int32_t w = UiText_Width(UI_FACE_VT323, UI_BODY_PX, UiLocale_Lookup(labels[i])) + 24;
        sSubX[row][i] = x;
        sSubW[row][i] = w;
        if (i == cur)
        {
            UiDraw_Rect(x, y, w, 30, gUiTheme->chipEdge);
            UiDraw_RectV(x + 2, y + 2, w - 4, 26, gUiTheme->chipTop, gUiTheme->chipBot);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y - 1, UiLocale_Lookup(labels[i]), gUiTheme->chipInk);
        }
        else
        {
            UiDraw_Rect(x, y, w, 30, gUiTheme->paperShade);
            UiDraw_Rect(x + 2, y + 2, w - 4, 26, UI_COL_PAPER1);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y - 1, UiLocale_Lookup(labels[i]), UI_COL_INK_SOFT);
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
    if ((in->pressed & VITA_BTN_CIRCLE) || (in->tap && !UiHit(in, POP_X, POP_Y, POP_W, POP_H))) { PlaySE(SE_SELECT); sEncOpen = false; return true; }
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
    snprintf(buf, sizeof(buf), UI_TR("%d POKéMON"), (int)sWildN);
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
        if (sWild[i].rod) UiW_Chip(POP_X + 420, y + 10, UiLocale_Lookup(sRods[sWild[i].rod]), false);
        snprintf(buf, sizeof(buf), UI_TR("Lv%u-%u"), (unsigned)sWild[i].minLv, (unsigned)sWild[i].maxLv);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + POP_W - 36 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 6, buf, ink);
    }
    if (sWildN == 0)
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, POP_X + 40, POP_Y + 140, UI_TR("NO WILD DATA FOR THIS FILTER"), UI_COL_INK_SOFT);
    UiW_ScrollBar(POP_X + POP_W - 25, POP_Y + 134, POP_ROWS * 44, sEncList.first, POP_ROWS, sWildN);
    // Footer strip with the button guides
    const int32_t fy = POP_Y + POP_H - 46;
    UiDraw_Rect(POP_X + 3, fy, POP_W - 6, 2, gUiTheme->paperShade);
    UiDraw_Rect(POP_X + 3, fy + 2, POP_W - 6, 41, gUiTheme->paper1);
    int32_t fx = POP_X + 20;
    UiW_Glyph(fx, fy + 10, UI_GLYPH_TRIANGLE);
    fx += 30 + UiText_Draw(UI_FACE_PS2P, 16, fx + 30, fy + 15, UI_TR("Method"), UI_COL_INK) + 24;
    fx += UiText_Draw(UI_FACE_PS2P, 16, fx, fy + 15, UI_TR("<> Time / rod"), UI_COL_INK) + 24;
    UiW_Glyph(fx, fy + 10, UI_GLYPH_CIRCLE);
    UiText_Draw(UI_FACE_PS2P, 16, fx + 30, fy + 15, UI_TR("Close"), UI_COL_INK);
}

static void Call(void)
{
    if (sCallerN == 0) return;
    UiGame_CallMessage(&sCallers[sCallList.sel], sCallText, sizeof(sCallText));
    sCallOpen = true;
    sTutorialCalled = true;            // POKENAV_MODE_FORCE_CALL_EXIT: the tutorial can end
    PlaySE(SE_POKENAV_CALL);
}

static void OpenFeed(void)
{
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    if (m.egg) { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("An EGG can't eat POKéBLOCKS.")); return; }
    uint8_t c[6];
    UiGame_Condition(sMonList.sel, c);
    if (c[5] >= 255) { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("%s won't eat any more..."), m.name); return; }
    sBlockN = UiGame_Pokeblocks(sBlocks, 40);
    if (sBlockN == 0) { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("There are no POKéBLOCKS in the case.")); return; }
    sFeedOpen = true;
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
    // OpenFeed refused an EGG and a full sheen. The game's eating scene in a window over the
    // PokéNav, then the PokéNav again as it was (the block's effect is applied on return)
    PlaySE(SE_SELECT);
    sFeedOpen = false;
    UiPokeblock_FeedFromCondition((uint8_t)sMonList.sel, sBlocks[a].slot);
}

static void DrawFeed(void)
{
    char buf[16];
    UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
    UiW_Window(FEED_X, FEED_Y, FEED_W, 56 + FEED_ROWS * 40, UI_TR("POKéBLOCK CASE"));
    UiDraw_PushClip(FEED_X + 8, FEED_Y + 44, FEED_W - 16, FEED_ROWS * 40);
    for (int32_t r = 0; r < FEED_ROWS && sFeedList.first + r < sBlockN; r++)
    {
        const int32_t i = sFeedList.first + r, y = FEED_Y + 44 + r * 40;
        const bool on = i == sFeedList.sel;
        UiW_Row(FEED_X + 8, y, FEED_W - 28, 40, on, i);
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, FEED_X + 24, y + 4, sBlocks[i].name, ink);
        snprintf(buf, sizeof(buf), UI_TR("Lv%u F%u"), (unsigned)sBlocks[i].level, (unsigned)sBlocks[i].feel);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, FEED_X + FEED_W - 40 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 4, buf, on ? ink : UI_COL_INK_SOFT);
    }
    UiDraw_PopClip();
    UiW_ScrollBar(FEED_X + FEED_W - 17, FEED_Y + 44, FEED_ROWS * 40, sFeedList.first, FEED_ROWS, sBlockN);
}

static bool NavUpdate(const UiInput *in)
{
    sFrame++;
    const bool gba = gUiTheme->gbaFrame;
    if (sEncOpen) return UpdateEncPopup(in);
    if (sFeedOpen) { UpdateFeed(in); return true; }
    if (sCallOpen)
    {
        if (in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE) || in->tap) { PlaySE(SE_SELECT); sCallOpen = false; }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        if (sNavMode == NAV_MODE_TUTORIAL && !sTutorialCalled)
        {
            PlaySE(SE_FAILURE);                       // call Mr. Stone first
            return true;
        }
        PlaySE(SE_SELECT);
        return false;
    }
    int32_t t = -1;
    if (Locked())
        ;                                             // a fixed tab
    else if (gba)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, NAV_COUNT);
        if (ph != 0)
            t = (sTab + NAV_COUNT + ph) % NAV_COUNT;
    }
    else
        t = UiKit_TabHit(in, sTabX, sTabW, NAV_COUNT);
    if (!Locked() && (in->pressed & VITA_BTN_L)) t = (sTab + NAV_COUNT - 1) % NAV_COUNT;
    if (!Locked() && (in->pressed & VITA_BTN_R)) t = (sTab + 1) % NAV_COUNT;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); return true; }
    switch (sTab)
    {
    case NAV_MAP:
        if (MapUpdate(in)) RefreshWild();
        if (in->pressed & VITA_BTN_CROSS) { sZoom = !sZoom; PlaySE(SE_SELECT); }
        if (Locked())
            break;                                    // a wall map: only the map
        if (in->pressed & VITA_BTN_TRIANGLE) { sMethod = (sMethod + 1) % UI_WILD_COUNT; PlaySE(SE_SELECT); RefreshWild(); }
        if ((in->pressed & VITA_BTN_SQUARE) || (gba ? UiHit(in, 616, 178, 330, 296) : UiHit(in, 704, 128, 240, 364)))
        {
            sEncOpen = true;
            PlaySE(SE_SELECT);
            UiList_Reset(&sEncList, sWildN, POP_ROWS);
        }
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
        if (UiList_Update(&sCallList, in, 28, gba ? 108 : 106, gba ? 412 : 416, gba ? 39 : 40, false) >= 0
            || UiHit(in, gba ? 492 : 496, gba ? 420 : 424, 208, 44))
            Call();
        if (!Locked() && ((in->pressed & VITA_BTN_TRIANGLE) || UiHit(in, gba ? 720 : 720, gba ? 420 : 424, 208, 44)) && sCallerN)
        {
            int32_t x, y;
            if (UiGame_MapSecPos(sCallers[sCallList.sel].mapSec, &x, &y)) { sCurX = x; sCurY = y; sTab = NAV_MAP; RefreshWild(); PlaySE(SE_SELECT); }
        }
        break;
    default:
    {
        const int32_t before = sMonList.sel;
        UiList_Update(&sMonList, in, 22, gba ? 108 : 66, gba ? 276 : 288, gba ? 58 : 44, true);
        if (sMonList.sel != before) sRibSel = 0;
        if (sTab == NAV_COND && ((in->pressed & (VITA_BTN_SQUARE | VITA_BTN_CROSS)) || UiHit(in, 676, gba ? 420 : 424, 252, 44)))
        {
            if (UiGame_HasPokeblockCase()) OpenFeed();
            else { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("You don't have a POKéBLOCK CASE yet.")); }
        }
        if (sTab == NAV_RIB)
        {
            uint8_t ids[32];
            const int32_t n = UiGame_Ribbons(sMonList.sel, ids, 32);
            const int32_t b = sRibSel;
            if (in->repeat & VITA_BTN_LEFT && sRibSel > 0) sRibSel--;
            if (in->repeat & VITA_BTN_RIGHT && sRibSel < n - 1) sRibSel++;
            const int32_t rx0 = gba ? 340 : 348, ry0 = gba ? 114 : 110;
            for (int32_t i = 0; i < n && i < 16; i++)
                if (UiHit(in, rx0 + (i % 8) * 72, ry0 + (i / 8) * 72, 64, 64)) sRibSel = i;
            if (sRibSel != b) PlaySE(SE_SELECT);
        }
        break;
    }
    }
    return true;
}

static void DrawGbaBtn(int32_t x, int32_t y, int32_t w, int32_t h, const char *label, bool green)
{
    const uint32_t top = green ? UI_RGBA(68, 178, 114, 255) : UI_RGBA(232, 168, 56, 255);
    const uint32_t bot = green ? UI_RGBA(40, 134, 80, 255)  : UI_RGBA(192, 124, 32, 255);
    const uint32_t hi  = green ? UI_RGBA(136, 232, 174, 255) : UI_RGBA(255, 224, 140, 255);
    const uint32_t sh  = green ? UI_RGBA(24, 84, 52, 255)   : UI_RGBA(120, 76, 16, 255);
    UiW_Bevel(x, y, w, h, 4, UI_RGBA(41, 49, 49, 255), top, bot, 2, hi, sh, 3);
    UiW_GbaTextCentred(UI_FACE_PS2P, 16, x, y, w, h, label, C_WHITE, UI_GBA_SHAD_DARK);
}

// CONDITION's party list, radar and attributes are shared with the POKéBLOCK CASE's FEED step
// (ui_scr_pokeblock.c): sel is the party slot shown, gain (NULL: none) what a block would add.
void UiCond_PartyListGba(int32_t sel)
{
    UiW_GbaWin(14, 60, 296, 414, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(26, 72, 272, 28, UI_GBA_TAG_BLUE, UI_TR("PARTY"), NULL);
    for (int32_t i = 0; i < UiGame_PartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = 108 + i * 58;
        const bool on = (i == sel);
        if (on)
        {
            UiW_GbaBox(22, y + 1, 276, 52, 4, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(29, y + 22, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(28, y + 21, true, UI_RGBA(230, 8, 8, 255));
        }
        else
            UiW_GbaBox(22, y + 1, 276, 52, 4, UI_RGBA(184, 190, 176, 255),
                       UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        UiSpr_MonIcon(m.species, 0, 42, y + 11, 1);
        UiW_GbaTextMid(UI_FACE_VT323, 28, 82, y, 54, m.name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }
}

// GBA non-linear condition curve (src/menu_specialized.c: sConditionToLineLength, max 35)
static const uint8_t sCondLen[256] = {
     4,  5,  6,  7,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 13,
    13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 17,
    17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19,
    19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21,
    21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 33, 33, 33, 33, 33, 33, 33, 33,
    33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34, 34,
    34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 35
};

void UiCond_Radar(int32_t sel, float cx, float cy, float R, bool gba, const int16_t *gain)
{
    static const char *const names[5] = { UI_N("COOL"), UI_N("BEAUTY"), UI_N("CUTE"), UI_N("SMART"), UI_N("TOUGH") };
    UiMon m;
    if (!UiGame_Mon(sel, &m)) return;
    uint8_t c[6];
    UiGame_Condition(sel, c);
    float ax[5], ay[5];
    for (int32_t i = 0; i < 5; i++) { const float a = -1.5707963f + i * 1.2566371f; ax[i] = cosf(a); ay[i] = sinf(a); }
    for (int32_t ring = 1; ring <= 4; ring++)
        for (int32_t i = 0; i < 5; i++)
        {
            const float f = R * ring / 4.0f;
            const int32_t j = (i + 1) % 5;
            UiDraw_Line(cx + ax[i] * f, cy + ay[i] * f, cx + ax[j] * f, cy + ay[j] * f, 2, gUiTheme->paperShade);
        }
    for (int32_t i = 0; i < 5; i++)
        UiDraw_Line(cx, cy, cx + ax[i] * R, cy + ay[i] * R, 2, gUiTheme->paperShade);
    if (!m.egg)
    {
        float px[5], py[5];
        for (int32_t i = 0; i < 5; i++)
        {
            const float v = (float)sCondLen[c[i]] / 35.0f * R;
            px[i] = cx + ax[i] * v;
            py[i] = cy + ay[i] * v;
        }
        const uint32_t fill = (gUiTheme->winTop & 0x00FFFFFFu) | 0xB8000000u;
        for (int32_t i = 0; i < 5; i++)
            UiDraw_Tri(cx, cy, px[i], py[i], px[(i + 1) % 5], py[(i + 1) % 5], fill);
        for (int32_t i = 0; i < 5; i++)
        {
            UiDraw_Line(px[i], py[i], px[(i + 1) % 5], py[(i + 1) % 5], 2, gUiTheme->ink);
            UiDraw_Rect((int32_t)px[i] - 4, (int32_t)py[i] - 4, 8, 8, gUiTheme->ink);
        }
        if (gain != NULL)
        {
            // What the block would add: the new outline in the theme's selection colour
            float gx[5], gy[5];
            for (int32_t i = 0; i < 5; i++)
            {
                int32_t nv = c[i] + gain[i];
                if (nv < 0) nv = 0;
                if (nv > 255) nv = 255;
                const float v = (float)sCondLen[nv] / 35.0f * R;
                gx[i] = cx + ax[i] * v;
                gy[i] = cy + ay[i] * v;
            }
            for (int32_t i = 0; i < 5; i++)
                UiDraw_Line(gx[i], gy[i], gx[(i + 1) % 5], gy[(i + 1) % 5], 3, gUiTheme->selEdge);
        }
    }
    else
    {
        const int32_t ew = UiText_Width(UI_FACE_PS2P, 16, UI_TR("EGG"));
        UiText_Draw(UI_FACE_PS2P, 16, (int32_t)cx - ew / 2, (int32_t)cy - 8, UI_TR("EGG"), UI_COL_INK_SOFT);
    }
    for (int32_t i = 0; i < 5; i++)
    {
        char num[8];
        if (m.egg) snprintf(num, sizeof(num), "-");
        else snprintf(num, sizeof(num), "%u", (unsigned)c[i]);
        const float dist = R + 32.0f;
        const int32_t tx = (int32_t)(cx + ax[i] * dist);
        const int32_t ty = (int32_t)(cy + ay[i] * dist) - 22;
        const int32_t lw = UiText_Width(UI_FACE_VT323, 24, UiLocale_Lookup(names[i]));
        const int32_t nw = UiText_Width(UI_FACE_PS2P, 12, num);
        if (gba)
        {
            UiW_GbaText(UI_FACE_VT323, 24, tx - lw / 2, ty, UiLocale_Lookup(names[i]), UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            UiW_GbaText(UI_FACE_PS2P, 12, tx - nw / 2, ty + 24, num, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        }
        else
        {
            UiText_Draw(UI_FACE_VT323, 24, tx - lw / 2, ty, UiLocale_Lookup(names[i]), UI_COL_INK);
            UiText_Draw(UI_FACE_PS2P, 12, tx - nw / 2, ty + 24, num, UI_COL_INK_SOFT);
        }
    }
}

void UiCond_Attrs(int32_t sel, bool gba, const int16_t *gain, int16_t sheenGain, const char *button, bool buttonOn)
{
    static const uint32_t cols[5][2] = { { 0xFF3A5FE0u, 0xFF2742B8u }, { 0xFFE0A04Au, 0xFFB87A2Fu }, { 0xFFC88CF0u, 0xFFA060D0u }, { 0xFF6BD07Au, 0xFF3FA050u }, { 0xFF39C8E8u, 0xFF1C9CC0u } };
    UiMon m;
    if (!UiGame_Mon(sel, &m)) return;
    uint8_t c[6];
    UiGame_Condition(sel, c);
    const int32_t y0 = gba ? 108 : 106;
    const int32_t step = gba ? 44 : 42;
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t y = y0 + i * step;
        const int32_t pct = m.egg ? 0 : (c[i] == 0 ? 0 : (c[i] * 100 + 254) / 255);
        UiDraw_Sprite((uint16_t)(UI_SPR_CONTEST_FIRST + i), 674, y + 4, 2);
        // With a block to feed: a shorter bar and what it adds, in the theme's selection colour
        UiW_Bar(750, y + 12, gain != NULL ? 88 : 128, 14, pct, cols[i][0], cols[i][1]);
        if (gain != NULL && gain[i] != 0 && !m.egg)
        {
            char g[8];
            snprintf(g, sizeof(g), "%+d", (int)gain[i]);
            if (gba)
                UiW_GbaTextMid(UI_FACE_VT323, 24, 844, y, 38, g, gUiTheme->selEdge, UI_GBA_SHAD_LIGHT);
            else
                UiText_DrawMid(UI_FACE_VT323, 24, 844, y, 38, g, gUiTheme->selEdge);
        }
        char v[8];
        if (m.egg) snprintf(v, sizeof(v), "-");
        else snprintf(v, sizeof(v), "%u", (unsigned)c[i]);
        if (gba)
            UiW_GbaTextMid(UI_FACE_VT323, 28, 932 - UiText_Width(UI_FACE_VT323, 28, v), y, 38, v, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        else
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 928 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, v), y, v, UI_COL_INK);
    }
    char buf[64];
    const uint8_t sparkles = m.egg ? 0 : (c[5] == 255 ? 10 : c[5] / 26);
    if (m.egg) snprintf(buf, sizeof(buf), UI_TR("SHEEN -"));
    else if (gain != NULL) snprintf(buf, sizeof(buf), c[5] >= 255 ? UI_TR("SHEEN %u / 255  MAX") : UI_TR("SHEEN %u / 255  %+d"), (unsigned)c[5], (int)sheenGain);
    else snprintf(buf, sizeof(buf), UI_TR("SHEEN %u / 255  *%u/10"), (unsigned)c[5], (unsigned)sparkles);
    const int32_t sy = gba ? 336 : 326;
    const int32_t sby = gba ? 370 : 362;
    const int32_t spct = m.egg ? 0 : (c[5] == 0 ? 0 : (c[5] * 100 + 254) / 255);
    if (gba)
        UiW_GbaText(UI_FACE_VT323, 28, 676, sy, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    else
        UiText_Draw(UI_FACE_VT323, 26, 676, sy, buf, UI_COL_INK);
    UiW_Bar(676, sby, 252, 12, spct, UI_RGBA(124, 192, 242, 255), UI_COL_EXP);
    if (gba)
        DrawGbaBtn(676, 420, 252, 42, button, buttonOn);
    else
        UiW_Button(676, 424, 252, button, buttonOn ? UI_BTN_GREEN : UI_BTN_AMBER, false);
}

static void DrawCondAttrs(bool gba)
{
    UiMon m;
    uint8_t c[6];
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    UiGame_Condition(sMonList.sel, c);
    UiCond_Attrs(sMonList.sel, gba, NULL, 0, UI_TR("FEED POKéBLOCK"), UiGame_HasPokeblockCase() && !m.egg && c[5] < 255);
}

static void BuildMapTabGba(void)
{
    char buf[64], secName[32];
    const uint16_t sec = MapSec();
    UiGame_MapSecName(sec, secName, sizeof(secName));

    // Left dark-well Hoenn map card (14, 60, 590, 414)
    UiW_GbaWin(14, 60, 590, 414, UI_GBA_WIN_DARK);
    MapDraw(false, sFrame);

    // Bottom radar strip inside map card (.pnav-map-strip)
    UiW_Bevel(29, 383, 560, 78, 4, UI_RGBA(16, 24, 28, 255), UI_RGBA(27, 44, 50, 255), UI_RGBA(27, 44, 50, 255),
              2, UI_RGBA(72, 120, 136, 255), UI_RGBA(16, 24, 28, 255), 0);

    snprintf(buf, sizeof(buf), UI_TR("GRID %02d-%02d · %s"), (int)sCurX, (int)sCurY, secName);
    UiW_GbaText(UI_FACE_PS2P, 16, 41, 396, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
    snprintf(buf, sizeof(buf), sZoom ? UI_TR("ZOOM: x2 DETAIL") : UI_TR("ZOOM: FULL HOENN"));
    UiW_GbaText(UI_FACE_VT323, 24, 41, 426, buf, UI_RGBA(115, 255, 172, 255), UI_GBA_SHAD_DARK);
    int32_t readyCount = 0;
    for (int32_t i = 0; i < sCallerN; i++)
        if (sCallers[i].rematch && sCallers[i].mapSec == sec)
            readyCount++;
    snprintf(buf, sizeof(buf), UI_TR("%d MATCH READY"), (int)readyCount);
    UiW_GbaText(UI_FACE_VT323, 24, 576 - UiText_Width(UI_FACE_VT323, 24, buf), 426, buf, UI_RGBA(184, 212, 224, 255), UI_GBA_SHAD_DARK);

    // Right column (616, 60, 330, 414)
    UiW_GbaWin(616, 60, 330, 108, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(628, 72, 306, 26, UI_GBA_TAG_BLUE, secName, (sec < MAPSEC_NONE && UiGame_CanFlyTo(sec)) ? UI_TR("FLY OK") : UI_TR("AREA"));
    snprintf(buf, sizeof(buf), UI_TR("Call Ready: %d Trainers"), (int)readyCount);
    UiW_GbaText(UI_FACE_VT323, 24, 628, 104, buf, UI_RGBA(23, 94, 71, 255), UI_GBA_SHAD_LIGHT);
    UiW_GbaText(UI_FACE_VT323, 24, 628, 130, UI_TR("Tap map or D-pad to scan"), UI_RGBA(96, 104, 96, 255), UI_GBA_SHAD_LIGHT);

    UiW_GbaWin(616, 178, 330, 296, UI_GBA_WIN_DEFAULT);
    Filter(buf, sizeof(buf));
    UiW_GbaSectionStrip(628, 190, 306, 26, UI_GBA_TAG_TEAL, UI_TR("ENCOUNTERS"), buf);
    for (int32_t i = 0; i < sWildN && i < 5; i++)
    {
        const int32_t ry = 224 + i * 48;
        const bool hi = (i == 0);
        UiW_GbaBox(628, ry, 306, 44, 4, hi ? UI_RGBA(230, 148, 65, 255) : UI_RGBA(184, 190, 176, 255),
                   hi ? UI_RGBA(255, 246, 205, 255) : UI_RGBA(244, 245, 236, 255),
                   hi ? UI_RGBA(255, 246, 205, 255) : UI_RGBA(244, 245, 236, 255), C_WHITE, 0);
        UiDraw_Sprite(UI_SPR_BALL_POKE, 636, ry + 14, 1);
        UiSpr_MonIcon(sWild[i].species, 0, 656, ry + 6, 1);
        UiGame_SpeciesName(sWild[i].species, secName, sizeof(secName));
        UiW_GbaTextMid(UI_FACE_VT323, 24, 692, ry, 44, secName, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (sWild[i].minLv == sWild[i].maxLv)
            snprintf(buf, sizeof(buf), UI_TR("Lv%u"), (unsigned)sWild[i].minLv);
        else
            snprintf(buf, sizeof(buf), UI_TR("Lv%u-%u"), (unsigned)sWild[i].minLv, (unsigned)sWild[i].maxLv);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 926 - UiText_Width(UI_FACE_VT323, 24, buf), ry, 44, buf, UI_RGBA(23, 94, 71, 255), UI_GBA_SHAD_LIGHT);
    }
    if (sWildN == 0)
        UiW_GbaTextCentred(UI_FACE_VT323, 28, 628, 250, 306, 100, UI_TR("NO WILD POKéMON"), UI_RGBA(112, 120, 112, 255), UI_GBA_SHAD_LIGHT);
}

static void BuildCallTabGba(void)
{
    char buf[48];
    UiW_GbaWin(14, 60, 452, 414, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(26, 72, 428, 28, UI_GBA_TAG_BLUE, UI_TR("MATCH CALL"), UI_TR("CONTACTS"));
    for (int32_t r = 0; r < 9 && sCallList.first + r < sCallerN; r++)
    {
        const int32_t i = sCallList.first + r, y = 108 + r * 39;
        const bool on = (i == sCallList.sel);
        if (on)
        {
            UiW_GbaBox(28, y + 1, 412, 36, 4, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
            UiW_GbaArrowH(35, y + 14, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(34, y + 13, true, UI_RGBA(230, 8, 8, 255));
        }
        else
        {
            const uint32_t rBg = (r & 1) ? UI_RGBA(241, 242, 228, 255) : UI_RGBA(250, 248, 236, 255);
            UiW_GbaBox(28, y + 1, 412, 36, 4, UI_RGBA(210, 214, 200, 255), rBg, rBg, UI_GBA_SHAD_LIGHT, 0);
        }
        UiW_GbaTextMid(UI_FACE_VT323, 28, 52, y, 38, sCallers[i].name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (sCallers[i].rematch)
            UiW_Chip(400, y + 5, "!", true);
    }
    UiW_ScrollBar(448, 108, 351, sCallList.first, 9, sCallerN);

    UiW_GbaWin(478, 60, 468, 414, UI_GBA_WIN_DEFAULT);
    if (sCallerN == 0)
    {
        UiW_GbaText(UI_FACE_VT323, 28, 496, 84, UI_TR("No contacts registered yet."), UI_RGBA(112, 120, 112, 255), UI_GBA_SHAD_LIGHT);
        return;
    }
    const UiCaller *c = &sCallers[sCallList.sel];
    UiW_GbaBox(492, 74, 152, 152, 4, UI_RGBA(40, 48, 48, 255), UI_RGBA(232, 238, 240, 255), UI_RGBA(232, 238, 240, 255), UI_GBA_SHAD_LIGHT, 0);
    if (c->trainerPic != 0xFFFF) UiSpr_Trainer(c->trainerPic, 504, 86, 2);
    else UiDraw_Sprite(UI_SPR_START_POKENAV, 552, 134, 1);

    UiW_GbaSectionStrip(656, 74, 276, 24, UI_GBA_TAG_BLUE, UI_TR("TRAINER"), NULL);
    UiW_GbaBox(656, 100, 276, 46, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiW_GbaTextMid(UI_FACE_VT323, 28, 666, 100, 46, c->name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    UiW_GbaSectionStrip(656, 152, 276, 24, UI_GBA_TAG_SLATE, UI_TR("CLASS"), NULL);
    UiW_GbaBox(656, 178, 276, 48, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiW_GbaTextMid(UI_FACE_VT323, 24, 666, 178, 48, c->desc, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    UiW_GbaSectionStrip(492, 234, 440, 24, UI_GBA_TAG_TEAL, UI_TR("AREA"), NULL);
    UiW_GbaBox(492, 260, 440, 42, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiGame_MapSecName(c->mapSec, buf, sizeof(buf));
    UiW_GbaTextMid(UI_FACE_VT323, 28, 504, 260, 42, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    UiW_GbaSectionStrip(492, 308, 440, 24, UI_GBA_TAG_GREEN, UI_TR("REMATCH"), NULL);
    UiW_GbaBox(492, 334, 440, 78, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiW_GbaTextWrapped(UI_FACE_VT323, 28, 506, 344, 412, 26,
                       c->rematch ? UI_TR("Ready! Meet this trainer in that area to battle again.") : UI_TR("Not ready for a rematch yet."),
                       c->rematch ? UI_RGBA(32, 156, 8, 255) : UI_RGBA(96, 104, 96, 255), UI_GBA_SHAD_LIGHT, 2);

    DrawGbaBtn(492, 420, 208, 42, UI_TR("CALL"), true);
    DrawGbaBtn(720, 420, 208, 42, UI_TR("SHOW ON MAP"), false);
}

static void BuildCondTabGba(void)
{
    UiCond_PartyListGba(sMonList.sel);
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;

    UiW_GbaWin(322, 60, 326, 414, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(334, 72, 302, 28, UI_GBA_TAG_TEAL, UI_TR("CONDITION"), m.name);
    UiCond_Radar(sMonList.sel, 485, 290, 92.0f, true, NULL);

    UiW_GbaWin(660, 60, 286, 414, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(672, 72, 262, 28, UI_GBA_TAG_GOLD, UI_TR("ATTRIBUTES"), NULL);
    DrawCondAttrs(true);
}

static void BuildRibTabGba(void)
{
    UiCond_PartyListGba(sMonList.sel);
    UiW_GbaWin(322, 60, 624, 414, UI_GBA_WIN_DEFAULT);
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m))
        return;
    char buf[64], desc[80];
    UiW_GbaSectionStrip(334, 72, 600, 28, UI_GBA_TAG_MAGENTA, UI_TR("RIBBONS"), m.name);
    uint8_t ids[32];
    const int32_t n = UiGame_Ribbons(sMonList.sel, ids, 32);
    for (int32_t i = 0; i < 16; i++)
    {
        const int32_t x = 340 + (i % 8) * 72, y = 114 + (i / 8) * 72;
        const bool rOn = (i < n && i == sRibSel);
        UiW_GbaBox(x, y, 64, 64, 4, rOn ? UI_RGBA(230, 8, 8, 255) : UI_RGBA(184, 190, 176, 255),
                   rOn ? UI_RGBA(255, 246, 205, 255) : UI_RGBA(244, 245, 236, 255),
                   rOn ? UI_RGBA(255, 246, 205, 255) : UI_RGBA(244, 245, 236, 255), C_WHITE, 0);
        if (i < n) UiDraw_Sprite((uint16_t)(UI_SPR_RIBBON_FIRST + ids[i]), x, y, 2);
    }
    UiW_GbaBox(338, 272, 592, 188, 4, UI_RGBA(184, 190, 176, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiDraw_PushClip(352, 278, 564, 174);
    if (n == 0)
    {
        snprintf(buf, sizeof(buf), UI_TR("%s has no ribbons yet. Win contests to earn them."), m.name);
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 356, 288, 556, 28, buf, UI_RGBA(112, 120, 112, 255), UI_GBA_SHAD_LIGHT, 4);
    }
    else
    {
        UiGame_RibbonText(ids[sRibSel < n ? sRibSel : 0], buf, sizeof(buf), desc, sizeof(desc));
        UiW_GbaText(UI_FACE_PS2P, 16, 356, 288, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 356, 320, 556, 28, desc, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT, 4);
    }
    UiDraw_PopClip();
}

static void NavBuildGba(void)
{
    if (Locked())
        UiW_GbaTopBar(UI_GBA_BG_POKENAV, UI_SPR_START_POKENAV, sNavMode == NAV_MODE_TOWN_MAP ? UI_TR("HOENN MAP") : UI_TR("POKéNAV"), 0, 0, false, NULL);
    else
        UiW_GbaTopBar(UI_GBA_BG_POKENAV, UI_SPR_START_POKENAV, UI_TR("POKéNAV"), NAV_COUNT, sTab, false, UiLocale_Lookup(sTabs[sTab]));
    int32_t rx = UI_SCREEN_W - 14;
    if (sNavMode != NAV_MODE_TUTORIAL || sTutorialCalled)
        UiW_GbaHelpBadge(&rx, "O", UI_TR("BACK"), true);
    switch (sTab)
    {
    case NAV_MAP:
        if (!Locked())
        {
            UiW_GbaHelpBadge(&rx, "SQ", UI_TR("LIST"), false);
            UiW_GbaHelpBadge(&rx, "TRI", UI_TR("METHOD"), false);
        }
        UiW_GbaHelpBadge(&rx, "X", UI_TR("ZOOM"), false);
        BuildMapTabGba();
        break;
    case NAV_CALL:
        if (!Locked())
            UiW_GbaHelpBadge(&rx, "TRI", UI_TR("MAP"), false);
        UiW_GbaHelpBadge(&rx, "X", UI_TR("CALL"), false);
        BuildCallTabGba();
        break;
    case NAV_COND:
        UiW_GbaHelpBadge(&rx, UI_TR("X/SQ"), UI_TR("FEED"), false);
        BuildCondTabGba();
        break;
    default:
        UiW_GbaHelpBadge(&rx, "<>", UI_TR("RIBBON"), false);
        BuildRibTabGba();
        break;
    }
    if (sEncOpen) DrawEncPopup();
    if (sFeedOpen) DrawFeed();
    if (sCallOpen)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(120, 150, 720, 240, sCallers[sCallList.sel].name);
        UiW_TextWrapped(144, 200, 672, 30, sCallText, UI_COL_INK, 6);
    }
}

void UiCond_PartyList(int32_t sel)
{
    UiW_Panel(16, 60, 300, 432);
    for (int32_t i = 0; i < UiGame_PartyCount(); i++)
    {
        UiMon m;
        UiGame_Mon(i, &m);
        const int32_t y = 66 + i * 44;
        const bool on = i == sel;
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
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 720, 180, UI_TR("NO WILD DATA"), UI_COL_INK_SOFT);
}

static void BuildCallTab(void)
{
    char buf[48];
    UiW_Panel(16, 60, 452, 432);
    UiW_PanelHead(16, 60, 452, UI_TR("CONTACTS"), NULL);
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
    if (sCallerN == 0) { UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 496, 80, UI_TR("No contacts yet."), UI_COL_INK_SOFT); return; }
    const UiCaller *c = &sCallers[sCallList.sel];
    // Trainer: the battle sprite in a bevelled box
    UiW_Inset(496, 76, 152, 152);
    if (c->trainerPic != 0xFFFF) UiSpr_Trainer(c->trainerPic, 508, 88, 2);
    else UiDraw_Sprite(UI_SPR_START_POKENAV, 556, 136, 1);
    UiW_Inset(660, 76, 268, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 672, 84, UI_TR("NAME"), UI_COL_INK_SOFT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 672, 104, c->name, UI_COL_INK);
    UiW_Inset(660, 158, 268, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 672, 166, UI_TR("TYPE"), UI_COL_INK_SOFT);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 672, 186, c->desc, UI_COL_INK);
    UiW_Inset(496, 240, 432, 70);
    UiText_Draw(UI_FACE_PS2P, 16, 508, 248, UI_TR("AREA"), UI_COL_INK_SOFT);
    UiGame_MapSecName(c->mapSec, buf, sizeof(buf));
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 508, 268, buf, UI_COL_INK);
    // Rematch: ready or not; a ready trainer waits in that area for a new battle
    UiW_Inset(496, 322, 432, 82);
    UiText_Draw(UI_FACE_PS2P, 16, 508, 330, UI_TR("REMATCH"), UI_COL_INK_SOFT);
    UiW_TextWrapped(508, 350, 408, 30, c->rematch ? UI_TR("Ready! Meet this trainer in the area to battle again.") : UI_TR("Not ready yet."),
                    c->rematch ? UI_COL_HP_HIGH : UI_COL_INK_SOFT, 2);
    UiW_Button(496, 424, 208, UI_TR("CALL"), UI_BTN_GREEN, false);
    UiW_Button(720, 424, 208, UI_TR("SHOW ON MAP"), UI_BTN_AMBER, false);
}

static void BuildCondTab(void)
{
    UiCond_PartyList(sMonList.sel);
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    char buf[64];
    snprintf(buf, sizeof(buf), UI_TR("%s · CONDITION"), m.name);
    UiW_Panel(328, 60, 320, 432);
    UiW_PanelHead(328, 60, 320, buf, NULL);
    UiCond_Radar(sMonList.sel, 488, 300, 92.0f, false, NULL);

    UiW_Panel(660, 60, 284, 432);
    UiW_PanelHead(660, 60, 284, UI_TR("ATTRIBUTES"), NULL);
    DrawCondAttrs(false);
}

static void BuildRibTab(void)
{
    UiCond_PartyList(sMonList.sel);
    UiW_Panel(328, 60, 616, 432);
    UiMon m;
    if (!UiGame_Mon(sMonList.sel, &m)) return;
    char buf[64], desc[80];
    snprintf(buf, sizeof(buf), UI_TR("%s · RIBBONS"), m.name);
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
        snprintf(buf, sizeof(buf), UI_TR("%s has no ribbons yet. Win contests to earn them."), m.name);
        UiW_TextWrapped(348, 284, 576, 30, buf, UI_COL_INK_SOFT, 3);
        return;
    }
    UiGame_RibbonText(ids[sRibSel < n ? sRibSel : 0], buf, sizeof(buf), desc, sizeof(desc));
    UiText_Draw(UI_FACE_PS2P, 16, 348, 290, buf, UI_COL_INK);
    UiW_TextWrapped(348, 316, 576, 30, desc, UI_COL_INK, 3);
}

static void NavBuild(void)
{
    if (gUiTheme->gbaFrame)
    {
        NavBuildGba();
        return;
    }
    if (Locked())
        UiKit_Header(sNavMode == NAV_MODE_TOWN_MAP ? UI_TR("HOENN MAP") : UI_TR("POKéNAV"), &sTabs[sTab], 1, 0, sTabX, sTabW, 10);
    else
        UiKit_Header(UI_TR("POKéNAV"), sTabs, NAV_COUNT, sTab, sTabX, sTabW, 10);
    int32_t fx = 16;
    switch (sTab)
    {
    case NAV_MAP:
        BuildMapTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Zoom"));
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        if (!Locked())
        {
            UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, UI_TR("Method"));
            UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, UI_TR("Encounters"));
        }
        break;
    case NAV_CALL:
        BuildCallTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Call"));
        if (sNavMode != NAV_MODE_TUTORIAL || sTutorialCalled)
            UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        if (!Locked())
            UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, UI_TR("Show on map"));
        break;
    case NAV_COND:
        BuildCondTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Feed"));
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        break;
    default:
        BuildRibTab();
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Back"));
        UiW_FooterKey(&fx, "<>", UI_TR("Ribbon"));
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

// A wall map (field_specials.c FieldShowRegionMap): hosted over the black the script faded to; on
// leaving, the field comes back and the script goes on (as the GBA region map's callback)
static void ExitTownMap(void)
{
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

bool Ui_OpenTownMap(void)
{
    if (!Ui_IsEnabled() || Ui_IsOpen())
        return false;
    sNavModeNext = NAV_MODE_TOWN_MAP;
    UiCore_OpenHosted(&gUiNavScreen, ExitTownMap);
    return true;
}

// Mr. Stone's tutorial (pokenav.c OpenPokenavForTutorial): over the field, the script waits
static void AfterTutorial(void)
{
    ScriptContext_Enable();
}

bool Ui_OpenPokenavTutorial(void)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || gMain.callback2 != CB2_Overworld)
        return false;
    sNavModeNext = NAV_MODE_TUTORIAL;
    UiCore_OpenFromScript(&gUiNavScreen, AfterTutorial);
    return true;
}

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
    const int32_t prevX = sCurX, prevY = sCurY;
    MapUpdate(in);
    const bool gba = gUiTheme->gbaFrame;
    const bool tapSame = in->tap && ((sCurX == prevX && sCurY == prevY && UiHit(in, gba ? 26 : 24, gba ? 68 : 68, gba ? 560 : 660, gba ? 320 : 360))
                                     || UiHit(in, gba ? 616 : 704, 60, gba ? 330 : 240, 420));
    if ((in->pressed & VITA_BTN_CROSS) || tapSame)
    {
        const uint16_t sec = MapSec();
        if (sec < MAPSEC_NONE && sFlyable[sec]) { PlaySE(SE_SELECT); sFlySec = sec; UiNav_Close(FlyAfter); }
        else if (in->pressed & VITA_BTN_CROSS) { PlaySE(SE_FAILURE); UiKit_Toast(UI_TR("You can't fly there.")); }
    }
    return true;
}

static void FlyBuild(void)
{
    int32_t n = 0;
    for (uint16_t s = 0; s < MAPSEC_NONE; s++) n += sFlyable[s];
    char buf[48], name[24];
    const uint16_t sec = MapSec();
    UiGame_MapSecName(sec, name, sizeof(name));

    if (gUiTheme->gbaFrame)
    {
        snprintf(buf, sizeof(buf), UI_TR("%d DESTINATIONS"), (int)n);
        UiW_GbaTopBar(UI_GBA_BG_POKENAV, UI_SPR_START_POKENAV, UI_TR("FLY"), 1, 0, true, buf);
        int32_t rx = UI_SCREEN_W - 14;
        UiW_GbaHelpBadge(&rx, "O", UI_TR("CANCEL"), true);
        UiW_GbaHelpBadge(&rx, "X", UI_TR("FLY"), false);

        UiW_GbaWin(14, 60, 590, 472, UI_GBA_WIN_DARK);
        MapDraw(true, sFrame);
        UiDraw_Frame(29, 383, 560, 134, 2, UI_RGBA(41, 49, 49, 255));
        UiDraw_Rect(31, 385, 556, 130, UI_RGBA(27, 44, 50, 255));
        UiText_Draw(UI_FACE_PS2P, 16, 41, 396, name, UI_RGBA(255, 230, 57, 255));
        UiText_Draw(UI_FACE_VT323, 28, 41, 428, sec < MAPSEC_NONE && sFlyable[sec] ? UI_TR("Valid destination - press X to FLY!") : UI_TR("Move cursor to a green square to FLY."),
                    sec < MAPSEC_NONE && sFlyable[sec] ? UI_RGBA(115, 255, 172, 255) : UI_RGBA(184, 212, 224, 255));

        UiW_GbaWin(616, 60, 330, 472, UI_GBA_WIN_DEFAULT);
        UiW_GbaSectionStrip(628, 72, 306, 26, UI_GBA_TAG_BLUE, UI_TR("DESTINATION"), name);
        UiMon m;
        UiGame_Mon(sFlySlot, &m);
        UiDraw_Frame(628, 112, 306, 108, 2, UI_RGBA(184, 190, 176, 255));
        UiDraw_Rect(630, 114, 302, 104, UI_RGBA(244, 245, 236, 255));
        UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 8) & 1), 640, 134, 1);
        snprintf(buf, sizeof(buf), UI_TR("%s will use FLY"), m.name);
        UiW_TextWrapped(684, 128, 236, 28, buf, UI_RGBA(49, 49, 49, 255), 3);
        UiW_GbaSectionStrip(628, 236, 306, 26, UI_GBA_TAG_GREEN, UI_TR("MAP GUIDE"), NULL);
        UiW_TextWrapped(636, 276, 290, 28, UI_TR("Tap anywhere on the Hoenn map or use D-Pad. Green squares are cities and towns unlocked for FLY."), UI_RGBA(72, 80, 72, 255), 5);
        return;
    }

    static const char *const tab[1] = { UI_N("HOENN") };
    int32_t tx[1], tw[1];
    UiKit_Header(UI_TR("FLY"), tab, 1, 0, tx, tw, 12);
    snprintf(buf, sizeof(buf), UI_TR("Destinations: %d"), (int)n);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    MapDraw(true, sFrame);
    UiW_Panel(704, 60, 240, 120);
    UiText_Draw(UI_FACE_PS2P, 16, 720, 80, name, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 720, 110, sec < MAPSEC_NONE && sFlyable[sec] ? UI_TR("You can fly here.") : "-", UI_COL_INK_SOFT);
    UiW_Panel(704, 192, 240, 200);
    UiMon m;
    UiGame_Mon(sFlySlot, &m);
    UiSpr_MonIcon(m.species, (uint8_t)((sFrame / 8) & 1), 720, 210, 1);
    snprintf(buf, sizeof(buf), UI_TR("%s will FLY"), m.name);
    UiW_TextWrapped(760, 210, 176, 30, buf, UI_COL_INK, 2);
    UiW_TextWrapped(720, 280, 216, 30, UI_TR("Green frames: towns you can fly to."), UI_COL_INK_SOFT, 3);
    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, UI_TR("Fly here"));
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, UI_TR("Cancel"));
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
