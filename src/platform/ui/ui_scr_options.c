// OPTIONS of the new UI (design/stitch/opciones): the real settings of the current Vita UI
// (vita_ui_options.c): GENERAL / WORLD / BATTLE / MODES / CHEATS tabs, group headers, value with
// left / right (or a tap), EXECUTE actions, button remapping and quit. Settings are saved on exit.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "platform/ui.h"
#include "ui_game.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"
#include "platform/vita_controls.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "m4a.h"
#include "platform.h"
#include "sound.h"
#include "constants/items.h"
#include "constants/songs.h"

enum { ROW_CORE, ROW_EXTRA, ROW_REMAP, ROW_QUIT };
typedef struct { uint8_t kind; uint16_t sub; const char *group, *label; const VitaExtra *ex; } OptRow;

#define LIST_X 28
#define LIST_W 904
#define LIST_Y 72
#define ROW_H 40
#define ROWS 9
#define GBA_LIST_X 26
#define GBA_LIST_W 892
#define GBA_LIST_Y 104
#define GBA_ROW_H 41
#define GBA_ROWS 8
#define MAX_ROWS 40

static int32_t OptVisRows(void)
{
    return gUiTheme->gbaFrame ? GBA_ROWS : ROWS;
}

static const char *const sTabs[VITA_TAB_COUNT] = { "GENERAL", "GRAPHICS", "WORLD", "BATTLE", "MODES", "CHEATS" };
static const char *const sCoreNames[6] = { "TEXT SPEED", "BATTLE SCENE", "BATTLE STYLE", "SOUND OUTPUT", "MASTER VOLUME", "BUTTON MODE" };
static const char *const sActions[VITA_ACTION_COUNT] = { "A BUTTON", "B BUTTON", "L SHOULDER", "R SHOULDER", "START", "SELECT", "FAST FORWARD", "QUICK OPTIONS" };
static const char *const sButtons[16] = { "UP", "DOWN", "LEFT", "RIGHT", "CROSS", "CIRCLE", "SQUARE", "TRIANGLE", "L", "R", "START", "SELECT", "R-STICK UP", "R-STICK DOWN", "R-STICK LEFT", "R-STICK RIGHT" };
static const uint8_t sDefaultButtons[VITA_ACTION_COUNT] = { 4, 5, 8, 9, 10, 11, 7, 6 };

static int32_t sTab;
static UiList sList[VITA_TAB_COUNT];
static OptRow sRows[MAX_ROWS];
static int32_t sCount;
static int32_t sTabX[VITA_TAB_COUNT], sTabW[VITA_TAB_COUNT];
// Remap window: 0 closed, 1 list, 2 waiting for a button
static int32_t sRemap, sRemapWaitFor;
static UiList sRemapList;
static bool sQuit;

// One line of help per option, shown in the bar under the list
static const char *const sCoreHelp[6] = {
    "How fast the dialogue text is printed.",
    "Show the move animations in battle.",
    "SHIFT offers a switch when the foe sends out a POKEMON.",
    "Mono or stereo sound.",
    "Volume of music and sound effects.",
    "L and R as page keys, or L working as the A button.",
};
static const struct { const char *key, *help; } sExtraHelp[] = {
    { "speed", "Game speed while fast forward is active." },
    { "ff_button", "Hold the button, or press it once to toggle." },
    { "filter", "How the 2D screens are scaled to the Vita display." },
    { "ui_theme", "Colour and style of the menus." },
    { "voxel_3d", "Draw the overworld as a 3D voxel diorama." },
    { "voxel_blur", "Blur the far background. Bloom and light shafts stay on." },
    { "voxel_bloom", "Glow of bright parts of the world (0 = off). Needs HD-2D MED or HIGH." },
    { "voxel_sky_bloom", "Sun and moon sky glow (5 = normal). Needs HD-2D MED or HIGH." },
    { "voxel_shafts", "Sun and moon light shafts (5 = normal). Needs HD-2D MED or HIGH." },
    { "voxel_fps", "Frame rate of the 3D world. 60 uses more battery." },
    { "voxel_hd2d", "LOW: sway, relief, particles. MED/HIGH: + bloom, shafts, grade." },
    { "voxel_fov", "Camera tilt: 35 is the classic view, 25 looks more from the side." },
    { "day_night", "Light changes with the clock." },
    { "timed_encounters", "Some POKEMON appear only by day or by night." },
    { "clock_speed", "How fast the in-game clock runs." },
    { "visible_wild", "Wild POKEMON roam the grass and water; touch one to battle." },
    { "follower", "The first POKEMON of the party walks behind you." },
    { "run_indoors", "Allow running inside buildings." },
    { "auto_run", "Run without holding B; hold B to walk." },
    { "field_hms", "Use field moves from the bag without the POKEMON menu." },
    { "repel_prompt", "Ask to use another REPEL when one wears off." },
    { "voxel_battle", "Fight on a 3D voxel arena." },
    { "battle_speed", "Speed of battle animations and text." },
    { "move_split", "Physical or special by move, as in later games." },
    { "exp_share", "Every POKEMON in the party gains EXP." },
    { "exp_catch", "Catching a POKEMON gives EXP." },
    { "ivs_evs", "Show IVs and EVs in the POKEMON summary." },
    { "nuzlocke", "One catch per area; fainted POKEMON are lost." },
    { "level_cap", "Limit levels to the next GYM LEADER's ace." },
    { "save_slot", "Which save file is used." },
    { "infinite_tms", "TMs are not used up." },
    { "trade_evo", "Trade evolutions happen at level 40." },
    { "fast_eggs", "EGGS hatch much sooner." },
    { "shiny_odds", "Chance that a wild POKEMON is shiny." },
    { "encounter_rate", "How often wild POKEMON appear." },
    { "always_catch", "Every POKE BALL catches." },
    { "give_money", "Set your money to $999,999." },
    { "give_pocket", "Pocket of the item to give." },
    { "give_item", "Item to give." },
    { "give_count", "How many to give." },
    { "give_now", "Put the chosen items in the BAG." },
    { "dex_hoenn", "Mark every HOENN POKEMON as caught." },
    { "dex_national", "Unlock and complete the NATIONAL POKEDEX." },
    { "max_contest", "Max the contest stats of the party." },
};

static const char *RowHelp(const OptRow *r)
{
    if (r->kind == ROW_REMAP) return "Choose the Vita button of each action.";
    if (r->kind == ROW_QUIT) return "Save the settings and quit to the LiveArea.";
    if (r->kind == ROW_CORE) return sCoreHelp[r->sub];
    for (size_t i = 0; i < sizeof(sExtraHelp) / sizeof(sExtraHelp[0]); i++)
        if (strcmp(sExtraHelp[i].key, r->ex->key) == 0)
            return sExtraHelp[i].help;
    return r->ex->count == 0 ? "Press X to run it." : "Left / right or X to change the value.";
}

static void BuildRows(void)
{
    sCount = 0;
    if (sTab == VITA_TAB_GENERAL)
    {
        for (uint16_t i = 0; i < 6; i++)
            sRows[sCount++] = (OptRow){ ROW_CORE, i, i < 3 ? "GAMEPLAY" : i < 5 ? "AUDIO" : "INTERFACE", sCoreNames[i], NULL };
        sRows[sCount++] = (OptRow){ ROW_REMAP, 0, "CONTROLS", "BUTTON REMAPPING", NULL };
    }
    const unsigned n = VitaExtras_GetTabExtraCount((uint8_t)sTab);
    for (unsigned i = 0; i < n && sCount < MAX_ROWS - 1; i++)
    {
        const VitaExtra *ex = VitaExtras_GetTabExtra((uint8_t)sTab, i);
        if (ex != NULL)
            sRows[sCount++] = (OptRow){ ROW_EXTRA, (uint16_t)i, ex->group, ex->name, ex };
    }
    if (sTab == VITA_TAB_MODES)
        sRows[sCount++] = (OptRow){ ROW_QUIT, 0, "SYSTEM", "QUIT TO LIVEAREA", NULL };
    UiList_Reset(&sList[sTab], sCount, OptVisRows());
}

static void CoreValue(uint16_t id, char *out, size_t cap)
{
    static const char *const spd[3] = { "SLOW", "MID", "FAST" }, *const mode[3] = { "NORMAL", "LR", "L=A" };
    switch (id)
    {
    case 0: snprintf(out, cap, "%s", spd[gSaveBlock2Ptr->optionsTextSpeed % 3]); break;
    case 1: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsBattleSceneOff ? "OFF" : "ON"); break;
    case 2: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsBattleStyle ? "SET" : "SHIFT"); break;
    case 3: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsSound ? "STEREO" : "MONO"); break;
    case 4: snprintf(out, cap, "%u%%", (unsigned)Platform_GetSetting(PLATFORM_SETTING_VOLUME) * 10); break;
    default: snprintf(out, cap, "%s", mode[gSaveBlock2Ptr->optionsButtonMode % 3]); break;
    }
}

static void StepCore(uint16_t id, int32_t dir)
{
    switch (id)
    {
    case 0: gSaveBlock2Ptr->optionsTextSpeed = (u8)((3 + gSaveBlock2Ptr->optionsTextSpeed + dir) % 3); break;
    case 1: gSaveBlock2Ptr->optionsBattleSceneOff ^= 1; break;
    case 2: gSaveBlock2Ptr->optionsBattleStyle ^= 1; break;
    case 3: gSaveBlock2Ptr->optionsSound ^= 1; SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound); break;
    case 4:
    {
        int32_t v = (int32_t)Platform_GetSetting(PLATFORM_SETTING_VOLUME) + dir;
        Platform_SetSetting(PLATFORM_SETTING_VOLUME, (u8)(v < 0 ? 10 : v > 10 ? 0 : v));
        break;
    }
    default: gSaveBlock2Ptr->optionsButtonMode = (u8)((3 + gSaveBlock2Ptr->optionsButtonMode + dir) % 3); break;
    }
}

static void ExtraValue(const VitaExtra *ex, char *out, size_t cap)
{
    if (ex->text != NULL)
        UiGame_Text(ex->text(), out, cap);
    else if (ex->count == 0)
        snprintf(out, cap, "EXECUTE");
    else
    {
        const int v = VitaExtras_Get(ex->key);
        snprintf(out, cap, "%s", (v >= 0 && v < ex->count && ex->values) ? ex->values[v] : "?");
    }
}

static void ResetAfterSlotChange(void)
{
    VitaPlatform_RequestReset();
}

static void Enter(void)
{
    sTab = VITA_TAB_GENERAL;
    sRemap = 0;
    sQuit = false;
    BuildRows();
}

static void Step(const OptRow *r, int32_t dir)
{
    PlaySE(SE_SELECT);
    if (r->kind == ROW_CORE) StepCore(r->sub, dir);
    else if (r->kind == ROW_REMAP) { sRemap = 1; UiList_Reset(&sRemapList, VITA_ACTION_COUNT + 1, VITA_ACTION_COUNT + 1); }
    else if (r->kind == ROW_QUIT) sQuit = true;
    else if (r->ex != NULL)
    {
        const int prevSlot = VitaExtras_Get("save_slot");
        VitaExtras_Step(r->ex, dir);
        if (r->ex->count == 0) UiKit_Toast("Done.");
        if (strcmp(r->ex->key, "save_slot") == 0 && VitaExtras_Get("save_slot") != prevSlot)
        {
            VitaExtras_Save();
            VitaNuzlocke_LoadForSlot(VitaExtras_Get("save_slot") + 1);
            UiNav_Close(ResetAfterSlotChange);          // loads the other save slot
        }
    }
}

static int32_t CoreChoices(uint16_t id, const char *const **out)
{
    static const char *const kSpd[3] = { "SLOW", "MID", "FAST" };
    static const char *const kOnOff[2] = { "ON", "OFF" };
    static const char *const kStyle[2] = { "SHIFT", "SET" };
    static const char *const kSound[2] = { "MONO", "STEREO" };
    static const char *const kMode[3] = { "NORMAL", "LR", "L=A" };
    switch (id)
    {
    case 0: *out = kSpd;   return gSaveBlock2Ptr->optionsTextSpeed % 3;
    case 1: *out = kOnOff; return gSaveBlock2Ptr->optionsBattleSceneOff ? 1 : 0;
    case 2: *out = kStyle; return gSaveBlock2Ptr->optionsBattleStyle ? 1 : 0;
    case 3: *out = kSound; return gSaveBlock2Ptr->optionsSound ? 1 : 0;
    case 5: *out = kMode;  return gSaveBlock2Ptr->optionsButtonMode % 3;
    default: *out = NULL;  return -1;
    }
}

static int32_t CoreChoiceCount(uint16_t id)
{
    return (id == 0 || id == 5) ? 3 : (id == 4) ? 0 : 2;
}

static int32_t RowChoices(const OptRow *row, const char *const **vals, int32_t *curOut)
{
    if (row->kind == ROW_CORE && row->sub != 4)
    {
        *curOut = CoreChoices(row->sub, vals);
        return CoreChoiceCount(row->sub);
    }
    if (row->kind == ROW_EXTRA && row->ex != NULL && row->ex->text == NULL && row->ex->count >= 2 && row->ex->count <= 4 && row->ex->values != NULL)
    {
        *vals = row->ex->values;
        *curOut = VitaExtras_Get(row->ex->key);
        return row->ex->count;
    }
    *vals = NULL;
    *curOut = -1;
    return 0;
}

static bool Update(const UiInput *in)
{
    const bool gba = gUiTheme->gbaFrame;
    if (sQuit)
    {
        if (in->pressed & VITA_BTN_CROSS) { VitaExtras_Save(); VitaPlatform_RequestQuit(); }
        if (in->pressed & VITA_BTN_CIRCLE || in->tap) { PlaySE(SE_SELECT); sQuit = false; }
        return true;
    }
    if (sRemap == 2)
    {
        if (in->tap) { PlaySE(SE_SELECT); sRemap = 1; return true; }
        // The next face / shoulder / START / SELECT button pressed becomes the action's button
        for (int32_t bit = 4; bit < 16; bit++)
            if (in->pressed & (1u << bit))
            {
                Platform_SetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + sRemapWaitFor), (u8)bit);
                PlaySE(SE_SELECT);
                sRemap = 1;
                break;
            }
        return true;
    }
    if (sRemap == 1)
    {
        if ((in->pressed & VITA_BTN_CIRCLE) || (in->tap && !UiHit(in, 200, 90, 560, 360))) { PlaySE(SE_SELECT); sRemap = 0; return true; }
        // Triangle: the action keeps no button (unbind)
        if ((in->pressed & VITA_BTN_TRIANGLE) && sRemapList.sel < VITA_ACTION_COUNT)
        {
            Platform_SetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + sRemapList.sel), VITA_BTN_UNBOUND);
            PlaySE(SE_SELECT);
            UiKit_Toast("%s has no button now.", sActions[sRemapList.sel]);
            return true;
        }
        const int32_t a = UiList_Update(&sRemapList, in, 220, 150, 520, 36, true);
        if (a == VITA_ACTION_COUNT)
        {
            for (int32_t i = 0; i < VITA_ACTION_COUNT; i++)
                Platform_SetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + i), sDefaultButtons[i]);
            PlaySE(SE_SUCCESS);
            UiKit_Toast("Buttons reset to the defaults.");
        }
        else if (a >= 0)
        {
            PlaySE(SE_SELECT);
            sRemapWaitFor = a;
            sRemap = 2;
        }
        return true;
    }
    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        VitaExtras_Save();
        return false;
    }
    if (in->pressed & VITA_BTN_START) { PlaySE(SE_SELECT); sQuit = true; return true; }
    if (in->pressed & VITA_BTN_SELECT) { PlaySE(SE_SELECT); sRemap = 1; UiList_Reset(&sRemapList, VITA_ACTION_COUNT + 1, VITA_ACTION_COUNT + 1); return true; }
    int32_t t = -1;
    if (gba)
    {
        const int32_t ph = UiW_GbaPagerHit(in, true, VITA_TAB_COUNT);
        if (ph != 0)
            t = (sTab + VITA_TAB_COUNT + ph) % VITA_TAB_COUNT;
    }
    else
        t = UiKit_TabHit(in, sTabX, sTabW, VITA_TAB_COUNT);
    if (in->pressed & VITA_BTN_L) t = (sTab + VITA_TAB_COUNT - 1) % VITA_TAB_COUNT;
    if (in->pressed & VITA_BTN_R) t = (sTab + 1) % VITA_TAB_COUNT;
    if (t >= 0 && t != sTab) { sTab = t; PlaySE(SE_SELECT); BuildRows(); return true; }
    UiList *l = &sList[sTab];
    l->rows = OptVisRows();
    const int32_t lx = gba ? GBA_LIST_X : LIST_X;
    const int32_t ly = gba ? GBA_LIST_Y : LIST_Y;
    const int32_t lw = gba ? GBA_LIST_W : LIST_W;
    const int32_t rh = gba ? GBA_ROW_H : ROW_H;
    const int32_t a = UiList_Update(l, in, lx, ly, lw, rh, false);
    if (in->repeat & VITA_BTN_LEFT) Step(&sRows[l->sel], -1);
    else if (in->repeat & VITA_BTN_RIGHT) Step(&sRows[l->sel], 1);
    else if (a >= 0) Step(&sRows[a], 1);
    return true;
}

static void DrawModalsGba(void)
{
    if (sRemap)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(200, 100, 560, 400, sRemap == 2 ? "PRESS A BUTTON" : "BUTTON REMAPPING");
        UiText_Draw(UI_FACE_PS2P, 16, 220, 482, "X: change  TRIANGLE: no button", UI_COL_INK_SOFT);
        for (int32_t i = 0; i <= VITA_ACTION_COUNT; i++)
        {
            const int32_t y = 150 + i * 36;
            const bool on = i == sRemapList.sel;
            UiW_Row(220, y, 520, 36, on, i);
            const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
            if (i == VITA_ACTION_COUNT) { UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 248, y + 2, "RESET TO DEFAULTS", ink); continue; }
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 248, y + 2, sActions[i], ink);
            const uint8_t bit = (uint8_t)Platform_GetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + i));
            const char *b = (sRemap == 2 && i == sRemapWaitFor) ? "PRESS..." : bit < 16 ? sButtons[bit] : "NONE";
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 720 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, b), y + 2, b, ink);
        }
    }
    if (sQuit)
    {
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiW_Window(280, 200, 400, 140, "QUIT");
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 300, 250, "Quit to the LiveArea?", UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 300, 286, "X: quit   O: cancel", UI_COL_INK_SOFT);
    }
}

static void BuildGba(void)
{
    char buf[64];
    UiW_GbaTopBar(UI_GBA_BG_OPTIONS, 0, "OPTIONS", VITA_TAB_COUNT, sTab, false, sTabs[sTab]);
    UiSpr_ItemIcon(ITEM_CARD_KEY, 28, 13, 1);

    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "BACK", true);
    UiW_GbaHelpBadge(&rx, "SELECT", "REMAP", true);
    UiW_GbaHelpBadge(&rx, "X", "CHANGE", false);

    // Main options sheet (.gba-win.opt-sheet)
    UiW_GbaWin(14, 60, 932, 384, UI_GBA_WIN_DEFAULT);
    snprintf(buf, sizeof(buf), "TAB %d / %d · %s", (int)sTab + 1, VITA_TAB_COUNT, sTabs[sTab]);
    UiW_GbaSectionStrip(26, 70, 908, 28, UI_GBA_TAG_SLATE, "SYSTEM & GAMEPLAY OPTIONS", buf);

    UiW_GbaBox(GBA_LIST_X, GBA_LIST_Y, GBA_LIST_W, GBA_ROWS * GBA_ROW_H, 6, UI_RGBA(150, 172, 156, 255),
               UI_RGBA(250, 248, 236, 255), UI_RGBA(244, 244, 230, 255), C_WHITE, 0);
    UiList *const l = &sList[sTab];
    l->rows = GBA_ROWS;
    UiDraw_PushClip(GBA_LIST_X + 2, GBA_LIST_Y + 2, GBA_LIST_W - 4, GBA_ROWS * GBA_ROW_H - 4);
    for (int32_t r = 0; r < GBA_ROWS && l->first + r < sCount; r++)
    {
        const int32_t i = l->first + r, y = GBA_LIST_Y + r * GBA_ROW_H;
        const OptRow *row = &sRows[i];
        const bool on = (i == l->sel);
        if (on)
        {
            UiW_GbaBox(GBA_LIST_X + 4, y + 2, GBA_LIST_W - 8, GBA_ROW_H - 4, 4, UI_RGBA(230, 148, 65, 255),
                       UI_RGBA(255, 249, 224, 255), UI_RGBA(255, 242, 186, 255), C_WHITE, 0);
            UiW_Arrow(GBA_LIST_X + 12, y + 14, UI_RGBA(230, 8, 8, 255));
        }
        else
        {
            UiDraw_Rect(GBA_LIST_X + 2, y, GBA_LIST_W - 4, GBA_ROW_H, (i & 1) ? UI_RGBA(241, 242, 228, 255) : UI_RGBA(250, 248, 236, 255));
            if (r < GBA_ROWS - 1)
                UiDraw_Rect(GBA_LIST_X + 2, y + GBA_ROW_H - 2, GBA_LIST_W - 4, 2, UI_RGBA(228, 228, 212, 255));
        }
        UiW_GbaTextMid(UI_FACE_VT323, 28, GBA_LIST_X + 34, y, GBA_ROW_H, row->label, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);

        const char *const *vals = NULL;
        int32_t curVal = -1;
        const int32_t vc = RowChoices(row, &vals, &curVal);
        if (vc >= 2)
        {
            int32_t px = GBA_LIST_X + GBA_LIST_W - 14;
            for (int32_t j = vc - 1; j >= 0; j--)
            {
                const int32_t pw = UiText_Width(UI_FACE_VT323, 28, vals[j]) + 22;
                px -= pw;
                if (j == curVal)
                {
                    UiW_GbaBox(px, y + 5, pw, 30, 4, UI_RGBA(216, 32, 24, 255),
                               UI_RGBA(255, 240, 238, 255), UI_RGBA(252, 224, 220, 255), C_WHITE, 1);
                    UiW_GbaTextCentred(UI_FACE_VT323, 28, px, y + 5, pw, 30, vals[j], UI_RGBA(216, 32, 24, 255), UI_RGBA(248, 200, 196, 255));
                }
                else
                    UiW_GbaTextCentred(UI_FACE_VT323, 28, px, y + 5, pw, 30, vals[j], UI_RGBA(130, 138, 132, 255), UI_GBA_SHAD_LIGHT);
                px -= 8;
            }
        }
        else
        {
            if (row->kind == ROW_CORE) CoreValue(row->sub, buf, sizeof(buf));
            else if (row->kind == ROW_EXTRA) ExtraValue(row->ex, buf, sizeof(buf));
            else snprintf(buf, sizeof(buf), row->kind == ROW_REMAP ? "OPEN" : "EXECUTE");
            const int32_t pw = UiText_Width(UI_FACE_VT323, 28, buf) + 28;
            const int32_t px = GBA_LIST_X + GBA_LIST_W - 18 - pw;
            UiW_GbaBox(px, y + 5, pw, 30, 4, UI_RGBA(216, 32, 24, 255),
                       UI_RGBA(255, 240, 238, 255), UI_RGBA(252, 224, 220, 255), C_WHITE, 1);
            UiW_GbaTextCentred(UI_FACE_VT323, 28, px, y + 5, pw, 30, buf, UI_RGBA(216, 32, 24, 255), UI_RGBA(248, 200, 196, 255));
            if (on && (row->kind == ROW_CORE || (row->kind == ROW_EXTRA && row->ex != NULL && row->ex->count > 1)))
            {
                UiW_GbaArrowH(px - 18, y + 14, false, UI_RGBA(216, 32, 24, 255));
                UiW_GbaArrowH(px + pw + 6, y + 14, true, UI_RGBA(216, 32, 24, 255));
            }
        }
    }
    UiDraw_PopClip();
    UiW_ScrollBar(924, GBA_LIST_Y, GBA_ROWS * GBA_ROW_H, l->first, GBA_ROWS, sCount);

    // Bottom GBA message help box (.gba-msg)
    UiW_GbaMsg(14, 452, 932, 80, false, true);
    if (sCount > 0)
    {
        const OptRow *sel = &sRows[l->sel];
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 38, 464, 868, 28, RowHelp(sel), UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
    }
    DrawModalsGba();
}

static void Build(void)
{
    if (gUiTheme->gbaFrame)
    {
        BuildGba();
        return;
    }
    char buf[48];
    UiKit_Header("OPTIONS", sTabs, VITA_TAB_COUNT, sTab, sTabX, sTabW, 12);
    UiW_Panel(16, 60, 928, 376);
    const UiList *l = &sList[sTab];
    UiDraw_PushClip(LIST_X, LIST_Y, LIST_W, ROWS * ROW_H);
    const char *prevGroup = l->first > 0 ? sRows[l->first - 1].group : NULL;
    for (int32_t r = 0; r < ROWS && l->first + r < sCount; r++)
    {
        const int32_t i = l->first + r, y = LIST_Y + r * ROW_H;
        const OptRow *row = &sRows[i];
        const bool on = i == l->sel;
        UiW_Row(LIST_X, y, LIST_W - 12, ROW_H, on, i);
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        // Group name on the first row of each group
        if (row->group && (prevGroup == NULL || strcmp(prevGroup, row->group) != 0))
            UiText_Draw(UI_FACE_PS2P, 16, LIST_X + 28, y + 12, row->group, UI_COL_MONEY);
        prevGroup = row->group;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + 220, y + 4, row->label, ink);
        if (row->kind == ROW_CORE) CoreValue(row->sub, buf, sizeof(buf));
        else if (row->kind == ROW_EXTRA) ExtraValue(row->ex, buf, sizeof(buf));
        else snprintf(buf, sizeof(buf), row->kind == ROW_REMAP ? "OPEN" : "EXECUTE");
        const int32_t vw = UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf);
        const int32_t vx = LIST_X + LIST_W - 60 - vw;
        UiW_Capsule(vx - 12, y + 4, vw + 24, 32);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, vx, y + 4, buf, UI_COL_TITLE);
        if (on && (row->kind == ROW_CORE || (row->kind == ROW_EXTRA && row->ex->count > 1)))
        {
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, vx - 40, y + 4, "<", ink);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, LIST_X + LIST_W - 40, y + 4, ">", ink);
        }
    }
    UiDraw_PopClip();
    UiW_ScrollBar(LIST_X + LIST_W - 9, LIST_Y, ROWS * ROW_H, l->first, ROWS, sCount);
    UiW_PanelDark(16, 444, 928, 48);
    const OptRow *sel = &sRows[l->sel];
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 32, 450, RowHelp(sel), UI_COL_ON_FRAME);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Change");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Save");
    UiW_FooterKey(&fx, "SELECT", "Remap");
    UiW_FooterKey(&fx, "START", "Quit");
    UiKit_FooterClock();
    DrawModalsGba();
}

const UiScreen gUiOptionsScreen = { Enter, Update, Build, NULL };

bool Ui_OpenOptionsHosted(void (*after)(void))
{
    if (!Ui_IsEnabled() || Ui_IsOpen())
        return false;
    UiCore_OpenHosted(&gUiOptionsScreen, after);
    return true;
}
