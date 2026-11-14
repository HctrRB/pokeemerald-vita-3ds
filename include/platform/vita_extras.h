#ifndef VITA_EXTRAS_H
#define VITA_EXTRAS_H

#include <stdbool.h>
#include <stdint.h>

enum
{
    VITA_TAB_GENERAL = 0,   // Text, Sound, Volume, Fast-Forward, Button Remap
    VITA_TAB_WORLD,         // 3D Voxel, Day/Night, Clock Speed, HGSS Follower, Field QoL
    VITA_TAB_BATTLE,        // 3D Battles, Native HD HUD, Battle Speed, Move Split, Exp Share
    VITA_TAB_MODES,         // Nuzlocke, Level Cap, Save Slots, Scaler, Quit
    VITA_TAB_CHEATS,        // Shiny Odds, Always Catch, Infinite TMs, Money, Dex, Give Items
    VITA_TAB_COUNT,

    // Legacy aliases preserved for compatibility
    VITA_TAB_SYSTEM       = VITA_TAB_GENERAL,
    VITA_TAB_ENHANCEMENTS = VITA_TAB_WORLD
};

typedef struct
{
    uint8_t tab;                  // VITA_TAB_*
    const char *group;            // Emerald 3DS sub-group header label
    const char *name;             // ASCII label displayed in menu
    const char *key;              // Key name in settings.txt
    uint8_t count;                // Number of cyclic values (0: executable action)
    uint8_t fallback;             // Default value when absent from settings.txt
    const char *const *values;    // Value labels (e.g. "OFF", "ON")
    void (*act)(void);            // Callback upon value change or action execution
    void (*step)(int direction);  // Optional custom stepper (e.g. GiveItemStep)
    const uint8_t *(*text)(void); // Optional dynamic text in game charmap
} VitaExtra;

extern const VitaExtra gVitaExtras[];
extern const unsigned gVitaExtraCount;
extern const char *const gVitaExtrasOffOn[2];

// Lifecycle & Settings Persistence (ux0:data/POKEVITA/settings.txt)
void VitaExtras_Init(void);
void VitaExtras_Save(void);

// Fast cached extras for high-frequency gameplay queries
typedef enum
{
    FAST_EXTRA_DAY_NIGHT = 0,
    FAST_EXTRA_TIMED_ENCOUNTERS,
    FAST_EXTRA_CLOCK_SPEED,
    FAST_EXTRA_RUN_INDOORS,
    FAST_EXTRA_MOVE_SPLIT,
    FAST_EXTRA_FOLLOWER,
    FAST_EXTRA_VOXEL_3D,
    FAST_EXTRA_INFINITE_TMS,
    FAST_EXTRA_REPEL_PROMPT,
    FAST_EXTRA_IVS_EVS,
    FAST_EXTRA_EXP_SHARE,
    FAST_EXTRA_EXP_CATCH,
    FAST_EXTRA_AUTO_RUN,
    FAST_EXTRA_FIELD_HMS,
    FAST_EXTRA_VOXEL_BATTLE,
    FAST_EXTRA_BATTLE_SPEED,
    FAST_EXTRA_FAST_EGGS,
    FAST_EXTRA_NUZLOCKE,
    FAST_EXTRA_LEVEL_CAP,
    FAST_EXTRA_SHINY_ODDS,
    FAST_EXTRA_ALWAYS_CATCH,
    FAST_EXTRA_TRADE_EVO,
    FAST_EXTRA_VOXEL_RES,
    FAST_EXTRA_VOXEL_FPS,
    FAST_EXTRA_VOXEL_BLUR,
    FAST_EXTRA_UI_THEME,       // colour theme of the new UI menus (UI_THEME_*)
    FAST_EXTRA_COUNT
} FastExtraId;

// Fast O(1) query & mutation
int VitaExtras_GetFast(FastExtraId id);
int VitaExtras_Get(const char *key);
void VitaExtras_Set(const char *key, int value);
void VitaExtras_Step(const VitaExtra *extra, int direction);

// Tab querying
unsigned VitaExtras_GetTabExtraCount(uint8_t tab);
const VitaExtra *VitaExtras_GetTabExtra(uint8_t tab, unsigned index);

#endif // VITA_EXTRAS_H
