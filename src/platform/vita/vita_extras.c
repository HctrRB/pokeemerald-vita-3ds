#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/vita_extras.h"
#include "platform/vita_daynight.h"
#include "platform/vita_nuzlocke.h"
#include "vita_platform.h"
#include "platform.h"
#include "global.h"
#include "pokedex.h"
#include "pokemon.h"
#include "item.h"
#include "money.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/pokedex.h"
#include "constants/characters.h"
#include "event_data.h"
#include "sound.h"
#include "constants/songs.h"

#define MAX_SETTINGS_ENTRIES 112
#define SETTINGS_FILENAME "settings.txt"

typedef struct
{
    char key[32];
    int32_t value;
} SettingEntry;

static SettingEntry sSettings[MAX_SETTINGS_ENTRIES];
static uint32_t sSettingCount = 0;
static bool sInitialized = false;
static int32_t sFastSettings[FAST_EXTRA_COUNT];

static inline int32_t LookupFastId(const char *key)
{
    if (key == NULL || key[0] == '\0')
        return -1;
    switch (key[0])
    {
    case 'a':
        if (strcmp(key, "auto_run") == 0) return FAST_EXTRA_AUTO_RUN;
        if (strcmp(key, "always_catch") == 0) return FAST_EXTRA_ALWAYS_CATCH;
        break;
    case 'b':
        if (strcmp(key, "battle_speed") == 0) return FAST_EXTRA_BATTLE_SPEED;
        break;
    case 'c':
        if (strcmp(key, "clock_speed") == 0) return FAST_EXTRA_CLOCK_SPEED;
        break;
    case 'd':
        if (strcmp(key, "day_night") == 0) return FAST_EXTRA_DAY_NIGHT;
        break;
    case 'e':
        if (strcmp(key, "exp_share") == 0) return FAST_EXTRA_EXP_SHARE;
        if (strcmp(key, "exp_catch") == 0) return FAST_EXTRA_EXP_CATCH;
        break;
    case 'f':
        if (strcmp(key, "follower") == 0) return FAST_EXTRA_FOLLOWER;
        if (strcmp(key, "field_hms") == 0) return FAST_EXTRA_FIELD_HMS;
        if (strcmp(key, "fast_eggs") == 0) return FAST_EXTRA_FAST_EGGS;
        break;
    case 'i':
        if (strcmp(key, "infinite_tms") == 0) return FAST_EXTRA_INFINITE_TMS;
        if (strcmp(key, "ivs_evs") == 0) return FAST_EXTRA_IVS_EVS;
        break;
    case 'l':
        if (strcmp(key, "level_cap") == 0) return FAST_EXTRA_LEVEL_CAP;
        break;
    case 'm':
        if (strcmp(key, "move_split") == 0) return FAST_EXTRA_MOVE_SPLIT;
        break;
    case 'n':
        if (strcmp(key, "nuzlocke") == 0) return FAST_EXTRA_NUZLOCKE;
        break;
    case 'r':
        if (strcmp(key, "run_indoors") == 0) return FAST_EXTRA_RUN_INDOORS;
        if (strcmp(key, "repel_prompt") == 0) return FAST_EXTRA_REPEL_PROMPT;
        break;
    case 's':
        if (strcmp(key, "shiny_odds") == 0) return FAST_EXTRA_SHINY_ODDS;
        break;
    case 't':
        if (strcmp(key, "timed_encounters") == 0) return FAST_EXTRA_TIMED_ENCOUNTERS;
        if (strcmp(key, "trade_evo") == 0) return FAST_EXTRA_TRADE_EVO;
        break;
    case 'u':
        if (strcmp(key, "ui_v2") == 0) return FAST_EXTRA_UI_V2;
        if (strcmp(key, "ui_theme") == 0) return FAST_EXTRA_UI_THEME;
        break;
    case 'v':
        if (strcmp(key, "voxel_3d") == 0) return FAST_EXTRA_VOXEL_3D;
        if (strcmp(key, "voxel_battle") == 0) return FAST_EXTRA_VOXEL_BATTLE;
        if (strcmp(key, "voxel_res") == 0) return FAST_EXTRA_VOXEL_RES;
        if (strcmp(key, "voxel_fps") == 0) return FAST_EXTRA_VOXEL_FPS;
        if (strcmp(key, "voxel_blur") == 0) return FAST_EXTRA_VOXEL_BLUR;
        break;
    default:
        break;
    }
    return -1;
}

static void SyncFastExtras(void)
{
    for (uint32_t i = 0; i < FAST_EXTRA_COUNT; i++)
        sFastSettings[i] = 0;

    for (uint32_t i = 0; i < sSettingCount; i++)
    {
        const int32_t fastId = LookupFastId(sSettings[i].key);
        if (fastId >= 0)
            sFastSettings[fastId] = sSettings[i].value;
    }
}

int32_t VitaExtras_GetFast(FastExtraId id)
{
    if (!sInitialized)
        VitaExtras_Init();
    if ((uint32_t)id < FAST_EXTRA_COUNT)
        return sFastSettings[id];
    return 0;
}

const char *const gVitaExtrasOffOn[2] = { "OFF", "ON" };
static const char *const sSpeedValues[] = { "2X", "3X", "4X", "5X", NULL };
static const char *const sFFButtonValues[] = { "HOLD", "TOGGLE", NULL };
static const char *const sFilterValues[] = { "NEAREST", "BILINEAR", "SHARP", NULL };
// Order = UI_THEME_* (src/platform/ui/ui_internal.h); RED is the default
static const char *const sUiThemeValues[] = { "RED", "EMERALD", "CRYSTAL", "TROPICAL", "ROYAL", NULL };
static const char *const sVoxelFpsValues[] = { "30", "60", NULL };
static const char *const sSaveSlotValues[] = { "SLOT 1", "SLOT 2", "SLOT 3", NULL };
static const char *const sDayNightValues[] = { "OFF", "AUTO", NULL };
static const char *const sClockSpeedValues[] = { "1X", "6X", "12X", "24X", "60X", "120X", "1000X", NULL };
static const char *const sBattleSpeedValues[] = { "1X", "1.5X", "2X", "3X", "4X", NULL };
static const char *const sLevelCapValues[] = { "OFF", "GYM CAP", "STRICT", NULL };
static const char *const sShinyOddsValues[] = { "1/8192", "1/4096", "1/1024", "1/256", "1/64", "ALWAYS", NULL };
static const char *const sActionSetValues[] = { "EXECUTE", NULL };
static const char *const sGivePockets[] = { "ITEMS", "POKE BALLS", "TMS & HMS", "BERRIES", "KEY ITEMS", NULL };
static const char *const sGiveCounts[] = { "1", "5", "10", "50", "99", NULL };
static const u8 sGiveCountValues[] = { 1, 5, 10, 50, 99 };

static void ActionCompleteHoennDex(void)
{
    for (u16 hoenn = 1; hoenn <= HOENN_DEX_COUNT; hoenn++)
    {
        const u16 national = HoennToNationalOrder(hoenn);
        GetSetPokedexFlag(national, FLAG_SET_SEEN);
        GetSetPokedexFlag(national, FLAG_SET_CAUGHT);
    }
    PlaySE(SE_SUCCESS);
}

static void ActionCompleteNationalDex(void)
{
    EnableNationalPokedex();
    for (u16 national = 1; national <= NATIONAL_DEX_COUNT; national++)
    {
        GetSetPokedexFlag(national, FLAG_SET_SEEN);
        GetSetPokedexFlag(national, FLAG_SET_CAUGHT);
    }
    PlaySE(SE_SUCCESS);
}

static void ActionInfiniteMoney(void)
{
    SetMoney(&gSaveBlock1Ptr->money, 999999);
    PlaySE(SE_SUCCESS);
}

static bool8 GiveItemUsable(const u16 item, const u8 pocket)
{
    u8 name[ITEM_NAME_LENGTH + 1];
    if (item == ITEM_NONE || item >= ITEMS_COUNT || GetPocketByItemId(item) != pocket)
        return FALSE;
    CopyItemName(item, name);
    return name[0] != CHAR_QUESTION_MARK && name[0] != EOS;
}

static u8 GivePocket(void)
{
    const int32_t pocket = VitaExtras_Get("give_pocket");
    return POCKET_ITEMS + (pocket >= 0 && pocket < 5 ? pocket : 0);
}

static u16 GiveItem(void)
{
    u16 item = (u16)VitaExtras_Get("give_item");
    const u8 pocket = GivePocket();
    if (GiveItemUsable(item, pocket))
        return item;
    for (item = 1; item < ITEMS_COUNT; item++)
    {
        if (GiveItemUsable(item, pocket))
            return item;
    }
    return ITEM_NONE;
}

static void GiveItemStep(int32_t direction)
{
    u16 item = GiveItem();
    const u8 pocket = GivePocket();
    if (item == ITEM_NONE)
        return;
    for (u16 tries = 0; tries < ITEMS_COUNT; tries++)
    {
        item = direction < 0 ? (item <= 1 ? ITEMS_COUNT - 1 : item - 1) : (item + 1 >= ITEMS_COUNT ? 1 : item + 1);
        if (GiveItemUsable(item, pocket))
            break;
    }
    VitaExtras_Set("give_item", item);
}

static const u8 *GiveItemText(void)
{
    static u8 sName[ITEM_NAME_LENGTH + 1];
    const u16 item = GiveItem();
    if (item == ITEM_NONE)
        sName[0] = EOS;
    else
        CopyItemName(item, sName);
    return sName;
}

static void GiveItemsNow(void)
{
    const u16 item = GiveItem();
    const int32_t count = VitaExtras_Get("give_count");
    u16 quantity = sGiveCountValues[count >= 0 && count < 5 ? count : 0];
    if (GetPocketByItemId(item) == POCKET_KEY_ITEMS)
        quantity = 1;
    if (item != ITEM_NONE && CheckBagHasSpace(item, quantity) && AddBagItem(item, quantity))
        PlaySE(SE_SUCCESS);
    else
        PlaySE(SE_FAILURE);
}

static void ActionMaxContestStats(void)
{
    const int32_t count = (int32_t)CalculatePlayerPartyCount();
    if (count <= 0)
    {
        PlaySE(SE_FAILURE);
        return;
    }
    const uint8_t maxStat = 255;
    for (int32_t i = 0; i < count; i++)
    {
        struct Pokemon *const mon = &gPlayerParty[i];
        if (GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        SetMonData(mon, MON_DATA_COOL, &maxStat);
        SetMonData(mon, MON_DATA_BEAUTY, &maxStat);
        SetMonData(mon, MON_DATA_CUTE, &maxStat);
        SetMonData(mon, MON_DATA_SMART, &maxStat);
        SetMonData(mon, MON_DATA_TOUGH, &maxStat);
        SetMonData(mon, MON_DATA_SHEEN, &maxStat);
    }
    PlaySE(SE_SUCCESS);
}

const VitaExtra gVitaExtras[] = {
    // ---- [GENERAL] ----
    { VITA_TAB_GENERAL, "PACING",     "FAST FWD SPEED",    "speed",            4, 1, sSpeedValues,       NULL, NULL, NULL },
    { VITA_TAB_GENERAL, "PACING",     "FF BUTTON MODE",    "ff_button",        2, 0, sFFButtonValues,    NULL, NULL, NULL },
    { VITA_TAB_GENERAL, "DISPLAY",    "2D SCALER",         "filter",           3, 0, sFilterValues,      NULL, NULL, NULL },
    { VITA_TAB_GENERAL, "DISPLAY",    "MENU COLOR",        "ui_theme",         5, 0, sUiThemeValues,     NULL, NULL, NULL },

    // ---- [WORLD] ----
    { VITA_TAB_WORLD,   "RENDERER",   "3D VOXEL DIORAMA",  "voxel_3d",         2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "RENDERER",   "DIORAMA BLUR (DOF)","voxel_blur",       2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "RENDERER",   "TARGET FRAMERATE",  "voxel_fps",        2, 0, sVoxelFpsValues,    NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "TIME & SKY", "DAY / NIGHT CYCLE", "day_night",        2, 1, sDayNightValues,    NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "TIME & SKY", "TIMED ENCOUNTERS",  "timed_encounters", 2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "TIME & SKY", "CLOCK SPEED",       "clock_speed",      7, 0, sClockSpeedValues,  NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "COMPANION",  "HGSS FOLLOWER",     "follower",         2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "EXPLORATION","RUN INDOORS",       "run_indoors",      2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "EXPLORATION","AUTO RUN (NO B)",   "auto_run",         2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "EXPLORATION","BAG HM SHORTCUTS",  "field_hms",        2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_WORLD,   "EXPLORATION","REPEL PROMPT",      "repel_prompt",     2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },

    // ---- [BATTLE] ----
    { VITA_TAB_BATTLE,  "ARENA",      "3D VOXEL BATTLE",   "voxel_battle",     2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_BATTLE,  "ARENA",      "BATTLE SPEED",      "battle_speed",     5, 0, sBattleSpeedValues, NULL, NULL, NULL },
    { VITA_TAB_BATTLE,  "MECHANICS",  "PHYS/SPEC SPLIT",   "move_split",       2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_BATTLE,  "MECHANICS",  "PARTY EXP SHARE",   "exp_share",        2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_BATTLE,  "MECHANICS",  "EXP ON CATCH",      "exp_catch",        2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_BATTLE,  "INSPECTION", "IVS & EVS STATS",   "ivs_evs",          2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },

    // ---- [MODES] ----
    { VITA_TAB_MODES,   "CHALLENGE",  "NUZLOCKE MODE",     "nuzlocke",         2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_MODES,   "CHALLENGE",  "LEVEL CAP",         "level_cap",        3, 0, sLevelCapValues,    NULL, NULL, NULL },
    { VITA_TAB_MODES,   "PROFILES",   "ACTIVE SAVE SLOT",  "save_slot",        3, 0, sSaveSlotValues,    NULL, NULL, NULL },
    { VITA_TAB_MODES,   "PREVIEW",    "NEW UI (TEST)",     "ui_v2",            2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },

    // ---- [CHEATS] ----
    { VITA_TAB_CHEATS,  "CONVENIENCE","INFINITE TMS",      "infinite_tms",     2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "CONVENIENCE","TRADE EVO LV40",    "trade_evo",        2, 1, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "CONVENIENCE","FAST HATCH EGGS",   "fast_eggs",        2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "ENCOUNTERS", "SHINY ODDS",        "shiny_odds",       6, 0, sShinyOddsValues,   NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "ENCOUNTERS", "ALWAYS CATCH",      "always_catch",     2, 0, gVitaExtrasOffOn,   NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "REWARDS",    "MAX MONEY ($999K)", "give_money",       0, 0, sActionSetValues,   ActionInfiniteMoney,     NULL, NULL },
    { VITA_TAB_CHEATS,  "ITEM SPAWN", "GIVE POCKET",       "give_pocket",      5, 0, sGivePockets,       NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "ITEM SPAWN", "GIVE ITEM",         "give_item",        0, 0, NULL,               NULL, GiveItemStep, GiveItemText },
    { VITA_TAB_CHEATS,  "ITEM SPAWN", "GIVE COUNT",        "give_count",       5, 0, sGiveCounts,        NULL, NULL, NULL },
    { VITA_TAB_CHEATS,  "ITEM SPAWN", "DELIVER ITEM",      "give_now",         0, 0, sActionSetValues,   GiveItemsNow,            NULL, NULL },
    { VITA_TAB_CHEATS,  "POKEDEX",    "COMPLETE HOENN DEX","dex_hoenn",        0, 0, sActionSetValues,   ActionCompleteHoennDex,  NULL, NULL },
    { VITA_TAB_CHEATS,  "POKEDEX",    "UNLOCK NATIONAL",   "dex_national",     0, 0, sActionSetValues,   ActionCompleteNationalDex, NULL, NULL },
    { VITA_TAB_CHEATS,  "CONTESTS",   "MAX CONTEST STATS", "max_contest",      0, 0, sActionSetValues,   ActionMaxContestStats,     NULL, NULL },
};

const uint32_t gVitaExtraCount = sizeof(gVitaExtras) / sizeof(gVitaExtras[0]);

static int32_t FindSettingIndex(const char *key, const bool createIfMissing)
{
    for (uint32_t i = 0; i < sSettingCount; i++)
    {
        if (strcmp(sSettings[i].key, key) == 0)
            return (int32_t)i;
    }
    if (createIfMissing && sSettingCount < MAX_SETTINGS_ENTRIES)
    {
        const int32_t idx = (int32_t)sSettingCount++;
        strncpy(sSettings[idx].key, key, sizeof(sSettings[idx].key) - 1);
        sSettings[idx].key[sizeof(sSettings[idx].key) - 1] = '\0';
        sSettings[idx].value = 0;
        return idx;
    }
    return -1;
}

static void SyncPlatformSetting(const char *key, const int32_t value)
{
    if (strcmp(key, "auto_run") == 0) Platform_SetSetting(PLATFORM_SETTING_AUTO_RUN, (u8)value);
    else if (strcmp(key, "field_hms") == 0) Platform_SetSetting(PLATFORM_SETTING_HM_SHORTCUTS, (u8)value);
    else if (strcmp(key, "ivs_evs") == 0) Platform_SetSetting(PLATFORM_SETTING_STATS_INFO, (u8)value);
    else if (strcmp(key, "exp_share") == 0) Platform_SetSetting(PLATFORM_SETTING_EXP_SHARE, (u8)value);
    else if (strcmp(key, "voxel_3d") == 0) Platform_SetSetting(PLATFORM_SETTING_VOXEL_3D, (u8)value);
    else if (strcmp(key, "voxel_fps") == 0) Platform_SetSetting(PLATFORM_SETTING_VOXEL_FPS, (u8)value);
    else if (strcmp(key, "filter") == 0)
    {
        Platform_SetSetting(PLATFORM_SETTING_FILTER, (u8)value);
        Platform_SetSetting(PLATFORM_SETTING_UI_SCALE, (u8)(value != 0 ? 1 : 0));
    }
    else if (strcmp(key, "speed") == 0) Platform_SetSetting(PLATFORM_SETTING_FAST_FORWARD_SPEED, (u8)(value + 2));
    else if (strcmp(key, "ff_button") == 0) Platform_SetSetting(PLATFORM_SETTING_FAST_FORWARD_TOGGLE, (u8)value);
    else if (strcmp(key, "save_slot") == 0) Platform_SetSetting(PLATFORM_SETTING_SAVE_FILE, (u8)(value + 1));
}

void VitaExtras_Init(void)
{
    if (sInitialized)
        return;
    sInitialized = true;
    sSettingCount = 0;

    for (uint32_t i = 0; i < gVitaExtraCount; i++)
    {
        const int32_t idx = FindSettingIndex(gVitaExtras[i].key, true);
        if (idx >= 0)
            sSettings[idx].value = gVitaExtras[i].fallback;
    }

    char path[256];
    VitaPlatform_BuildPath(path, sizeof(path), SETTINGS_FILENAME);
    FILE *const f = fopen(path, "r");
    if (f == NULL)
    {
        VitaExtras_Save();
        SyncFastExtras();
        for (uint32_t i = 0; i < sSettingCount; i++)
            SyncPlatformSetting(sSettings[i].key, sSettings[i].value);
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char key[32];
        int32_t val = 0;
        if (sscanf(line, "%31[^=]=%d", key, &val) == 2)
        {
            const int32_t idx = FindSettingIndex(key, true);
            if (idx >= 0)
                sSettings[idx].value = val;
        }
    }
    fclose(f);

    SyncFastExtras();
    for (uint32_t i = 0; i < sSettingCount; i++)
        SyncPlatformSetting(sSettings[i].key, sSettings[i].value);
}

void VitaExtras_Save(void)
{
    char path[256];
    VitaPlatform_BuildPath(path, sizeof(path), SETTINGS_FILENAME);
    FILE *const f = fopen(path, "w");
    if (f == NULL)
        return;

    for (uint32_t i = 0; i < sSettingCount; i++)
        fprintf(f, "%s=%d\n", sSettings[i].key, sSettings[i].value);
    fclose(f);
}

int32_t VitaExtras_Get(const char *key)
{
    if (!sInitialized)
        VitaExtras_Init();

    const int32_t fastId = LookupFastId(key);
    if (fastId >= 0)
        return sFastSettings[fastId];

    const int32_t idx = FindSettingIndex(key, false);
    if (idx >= 0)
        return sSettings[idx].value;

    for (uint32_t i = 0; i < gVitaExtraCount; i++)
    {
        if (strcmp(gVitaExtras[i].key, key) == 0)
            return gVitaExtras[i].fallback;
    }
    return 0;
}

void VitaExtras_Set(const char *key, int32_t value)
{
    if (!sInitialized)
        VitaExtras_Init();

    const int32_t idx = FindSettingIndex(key, true);
    if (idx < 0 || sSettings[idx].value == value)
        return;

    const bool isClockOrDayNight = (strcmp(key, "clock_speed") == 0 || strcmp(key, "day_night") == 0);
    if (isClockOrDayNight)
        VitaDayNight_Update();
    sSettings[idx].value = value;
    const int32_t fastId = LookupFastId(key);
    if (fastId >= 0)
        sFastSettings[fastId] = value;
    if (isClockOrDayNight)
        VitaDayNight_Update();
    VitaExtras_Save();
    SyncPlatformSetting(key, value);
}

void VitaExtras_Step(const VitaExtra *extra, int32_t direction)
{
    if (extra == NULL)
        return;

    if (extra->step != NULL)
    {
        extra->step(direction);
        return;
    }

    if (extra->count == 0)
    {
        if (extra->act != NULL)
            extra->act();
        return;
    }

    const int32_t current = VitaExtras_Get(extra->key);
    int32_t next = (current + direction) % extra->count;
    if (next < 0)
        next += extra->count;

    VitaExtras_Set(extra->key, next);
    if (extra->act != NULL)
        extra->act();
}

uint32_t VitaExtras_GetTabExtraCount(uint8_t tab)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < gVitaExtraCount; i++)
    {
        if (gVitaExtras[i].tab == tab)
            count++;
    }
    return count;
}

const VitaExtra *VitaExtras_GetTabExtra(uint8_t tab, uint32_t index)
{
    uint32_t current = 0;
    for (uint32_t i = 0; i < gVitaExtraCount; i++)
    {
        if (gVitaExtras[i].tab == tab)
        {
            if (current == index)
                return &gVitaExtras[i];
            current++;
        }
    }
    return NULL;
}
