// NUZLOCKE of the new UI (design/stitch/nuzlocke): AREAS (state of each Hoenn area, X cycles it as
// the current tracker does), SQUAD (party alive / fallen), RULES; the next leader's level cap.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "pokemon.h"
#include "region_map.h"
#include "sound.h"
#include "constants/region_map_sections.h"
#include "constants/songs.h"
#include "constants/species.h"

#define LIST_X 28
#define LIST_W 560
#define LIST_Y 106
#define ROW_H 40
#define ROWS 9

static const char *const sTabs[4] = { "AREAS", "SQUAD", "RULES", "SETTINGS" };
static const char *const sRules[4][2] = {
    { "One encounter per area", "Only the first wild POKéMON of each area can be caught." },
    { "Fainted = released", "A POKéMON that faints counts as fallen and can't be used." },
    { "Level cap", "The next GYM LEADER's highest level (option LEVEL CAP)." },
    { "Nicknames", "Give every POKéMON a nickname to bond with it." },
};
static int32_t sTab, sTabX[4], sTabW[4];
static UiList sList[4];
// SETTINGS: the CHALLENGE options of the MODES tab and a tracker reset
static const VitaExtra *sSettings[8];
static int32_t sSettingCount;
static bool sResetArmed;

static void BuildSettings(void)
{
    sSettingCount = 0;
    const unsigned n = VitaExtras_GetTabExtraCount(VITA_TAB_MODES);
    for (unsigned i = 0; i < n && sSettingCount < 8; i++)
    {
        const VitaExtra *ex = VitaExtras_GetTabExtra(VITA_TAB_MODES, i);
        if (ex != NULL && ex->group != NULL && strcmp(ex->group, "CHALLENGE") == 0)
            sSettings[sSettingCount++] = ex;
    }
}
static uint16_t sAreas[KANTO_MAPSEC_START];
static int32_t sAreaCount;

static void BuildAreas(void)
{
    // The current area first, then every Hoenn area that has a state
    const uint16_t cur = CorrectSpecialMapSecId(gMapHeader.regionMapSectionId);
    sAreaCount = 0;
    if (cur < KANTO_MAPSEC_START)
        sAreas[sAreaCount++] = cur;
    for (uint16_t s = 0; s < KANTO_MAPSEC_START; s++)
        if (s != cur && VitaNuzlocke_GetArea(s) != VITA_NUZ_AREA_NONE)
            sAreas[sAreaCount++] = s;
}

static int32_t Count(void)
{
    return sTab == 0 ? sAreaCount : sTab == 1 ? UiGame_PartyCount() : sTab == 2 ? 4 : sSettingCount + 1;
}

static void Enter(void)
{
    BuildAreas();
    BuildSettings();
    sResetArmed = false;
    for (int32_t t = 0; t < 4; t++) { sTab = t; UiList_Reset(&sList[t], Count(), ROWS); }
    sTab = 0;   // first tab
}

static bool Fallen(int32_t slot)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    return GetMonData(mon, MON_DATA_HP) == 0 || VitaNuzlocke_IsFallen(GetMonData(mon, MON_DATA_PERSONALITY), GetMonData(mon, MON_DATA_OT_ID),
                                                                      GetMonData(mon, MON_DATA_SPECIES) == SPECIES_SHEDINJA);
}

static bool Update(const UiInput *in)
{
    if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); return false; }
    int32_t t = UiKit_TabHit(in, sTabX, sTabW, 4);
    if (in->pressed & VITA_BTN_L) t = (sTab + 3) % 4;
    if (in->pressed & VITA_BTN_R) t = (sTab + 1) % 4;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); UiList_Reset(&sList[sTab], Count(), ROWS); return true; }
    const int32_t a = UiList_Update(&sList[sTab], in, LIST_X, LIST_Y, LIST_W, ROW_H, false);
    if (sTab == 3)
    {
        const int32_t sel = sList[3].sel;
        const int32_t dir = (in->repeat & VITA_BTN_LEFT) ? -1 : (in->repeat & VITA_BTN_RIGHT) ? 1 : (a >= 0 ? 1 : 0);
        if (dir != 0 && sel < sSettingCount)
        {
            VitaExtras_Step(sSettings[sel], dir);
            VitaExtras_Save();
            PlaySE(SE_SELECT);
        }
        else if (a == sSettingCount)
        {
            // Reset of the area / fallen tracker: X twice
            if (!sResetArmed) { sResetArmed = true; PlaySE(SE_SELECT); UiKit_Toast("Press X again to reset the tracker."); }
            else { VitaNuzlocke_Reset(); BuildAreas(); sResetArmed = false; PlaySE(SE_SUCCESS); UiKit_Toast("Tracker reset."); }
        }
        return true;
    }
    if (a >= 0 && sTab == 0)
    {
        const enum VitaNuzAreaStatus next = (enum VitaNuzAreaStatus)(((uint32_t)VitaNuzlocke_GetArea(sAreas[a]) + 1U) & 3U);
        VitaNuzlocke_SetArea(sAreas[a], next);
        PlaySE(SE_SELECT);
    }
    return true;
}

static void State(int32_t x, int32_t y, const char *label, uint32_t top, uint32_t bot)
{
    UiDraw_RectV(x, y + 4, 140, 32, top, bot);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 70 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, label) / 2, y + 4, label, UI_COL_ON_FRAME);
}

static void Build(void)
{
    char buf[64];
    const char *leader = "";
    const uint8_t cap = VitaNuzlocke_GetLevelCap(VitaNuzlocke_CountBadges(), &leader);
    UiKit_Header("NUZLOCKE", sTabs, 3, sTab, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), "NEXT: %s (CAP LV.%u)", leader ? leader : "-", (unsigned)cap);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);
    UiW_Panel(16, 60, 584, 432);
    UiW_PanelHead(16, 60, 584, sTab == 0 ? "ENCOUNTERS" : sTab == 1 ? "SQUAD VITAL STATUS" : sTab == 2 ? "RULES" : "SETTINGS",
                  sTab == 0 ? "X: change state" : sTab == 3 ? "< > change" : NULL);
    const UiList *l = &sList[sTab];
    const uint32_t green[2] = { UI_RGBA(110, 219, 151, 255), UI_COL_HP_HIGH }, red[2] = { UI_RGBA(240, 120, 104, 255), UI_COL_HP_LOW };
    for (int32_t r = 0; r < ROWS && l->first + r < Count(); r++)
    {
        const int32_t i = l->first + r, y = LIST_Y + r * ROW_H;
        const bool on = i == l->sel;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        if (sTab == 0)
        {
            u8 name[32];
            GetMapNameGeneric(name, sAreas[i]);
            UiGame_Text(name, buf, sizeof(buf));
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, y + 4, buf, ink);
            const enum VitaNuzAreaStatus st = VitaNuzlocke_GetArea(sAreas[i]);
            if (st == VITA_NUZ_AREA_CAUGHT) State(LIST_X + LIST_W - 160, y, "CAUGHT", green[0], green[1]);
            else if (st == VITA_NUZ_AREA_MISSED) State(LIST_X + LIST_W - 160, y, "MISSED", red[0], red[1]);
            else UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 150, y + 4, st == VITA_NUZ_AREA_AVAILABLE ? "AVAILABLE" : "-", ink);
        }
        else if (sTab == 1)
        {
            UiMon m;
            UiGame_Mon(i, &m);
            UiSpr_MonIcon(m.species, 0, LIST_X + 24, y + 2, 1);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 64, y + 4, m.name, ink);
            if (!m.egg)
            {
                snprintf(buf, sizeof(buf), "Lv%u", (unsigned)m.level);
                UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 300, y + 4, buf, ink);
                const bool dead = Fallen(i);
                State(LIST_X + LIST_W - 160, y, dead ? "FALLEN" : "ALIVE", dead ? red[0] : green[0], dead ? red[1] : green[1]);
            }
        }
        else if (sTab == 2)
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, y + 4, sRules[i][0], ink);
        else if (i < sSettingCount)
        {
            const VitaExtra *ex = sSettings[i];
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, y + 4, ex->name, ink);
            const int v = VitaExtras_Get(ex->key);
            const char *val = (ex->values && v >= 0 && v < ex->count) ? ex->values[v] : "?";
            const int32_t vw = UiText_Width(UI_FACE_VT323, UI_BODY_PX, val);
            UiW_Capsule(LIST_X + LIST_W - 60 - vw, y + 4, vw + 24, 32);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 48 - vw, y + 4, val, UI_COL_TITLE);
        }
        else
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 28, y + 4, sResetArmed ? "RESET TRACKER? (X)" : "RESET TRACKER", on ? ink : UI_COL_HP_LOW);
    }
    UiW_Panel(612, 60, 332, 200);
    if (sTab == 0 && sAreaCount)
    {
        u8 name[32];
        GetMapNameGeneric(name, sAreas[l->sel]);
        UiGame_Text(name, buf, sizeof(buf));
        UiText_Draw(UI_FACE_PS2P, 16, 628, 76, buf, UI_COL_INK);
        UiW_TextWrapped(628, 104, 300, 30, l->sel == 0 ? "You are in this area now." : "An area you have visited.", UI_COL_INK_SOFT, 2);
    }
    else if (sTab == 1 && UiGame_PartyCount())
    {
        UiMon m;
        UiGame_Mon(l->sel, &m);
        UiText_Draw(UI_FACE_PS2P, 16, 628, 76, m.name, UI_COL_INK);
        snprintf(buf, sizeof(buf), Fallen(l->sel) ? "Fainted: it can no longer be used." : "Alive. Level cap: %u", (unsigned)cap);
        UiW_TextWrapped(628, 104, 300, 30, buf, UI_COL_INK, 3);
    }
    else if (sTab == 3)
    {
        UiText_Draw(UI_FACE_PS2P, 16, 628, 76, "SETTINGS", UI_COL_INK);
        UiW_TextWrapped(628, 104, 300, 30, l->sel < sSettingCount ? "Left / right or X to change. Saved at once." : "Clears the areas and fallen allies of this save slot.", UI_COL_INK, 4);
    }
    else if (sTab == 2)
    {
        UiText_Draw(UI_FACE_PS2P, 16, 628, 76, "RULE", UI_COL_INK);
        UiW_TextWrapped(628, 104, 300, 30, sRules[l->sel][1], UI_COL_INK, 4);
    }
    uint32_t caught = 0, missed = 0, fallen = 0;
    VitaNuzlocke_GetCounts(&caught, &missed, &fallen);
    UiW_Panel(612, 272, 332, 140);
    const char *k[3] = { "CAUGHT AREAS", "MISSED AREAS", "FALLEN ALLIES" };
    const uint32_t v[3] = { caught, missed, fallen };
    for (int32_t i = 0; i < 3; i++)
    {
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 628, 284 + i * 38, k[i], UI_COL_INK);
        snprintf(buf, sizeof(buf), "%u", (unsigned)v[i]);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 924 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), 284 + i * 38, buf, UI_COL_INK);
    }
    int32_t fx = 16;
    if (sTab == 0) UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Change state");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiKit_FooterClock();
}

const UiScreen gUiNuzlockeScreen = { Enter, Update, Build, NULL };
