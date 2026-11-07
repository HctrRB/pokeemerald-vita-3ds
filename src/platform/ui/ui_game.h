#ifndef UI_GAME_H
#define UI_GAME_H

// Game data and actions for the new UI's screens (ui_game.c). Core 1 only.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// GBA string (EOS-terminated) -> UTF-8 in the Latin-1 subset of the UI fonts. Returns the length.
size_t UiGame_Text(const uint8_t *src, char *dst, size_t cap);

uint32_t UiGame_Money(void);
void UiGame_Clock(char *out, size_t cap);        // "HH:MM" of the game clock (vita_daynight)
bool UiGame_IsNight(void);
unsigned UiGame_ClockRate(void);                 // 1 = real time, N = accelerated
void UiGame_ZoneName(char *out, size_t cap);
void UiGame_PlayerName(char *out, size_t cap);
bool UiGame_PlayerIsMale(void);
uint16_t UiGame_RegisteredItem(void);            // ITEM_NONE when none or no longer in the bag
void UiGame_ItemName(uint16_t itemId, char *out, size_t cap);
uint16_t UiGame_RepelSteps(void);
bool UiGame_InSafari(void);
uint8_t UiGame_SafariBalls(void);
bool UiGame_HasFlag(uint16_t flag);
uint8_t UiGame_BadgeCount(void);
uint16_t UiGame_DexOwned(void);
void UiGame_PlayTime(char *out, size_t cap);

typedef struct
{
    uint16_t species;          // SPECIES_EGG for eggs
    bool egg, shiny;
    uint8_t level, gender;     // gender: MON_MALE / MON_FEMALE / MON_GENDERLESS
    uint16_t hp, maxHp, item;
    uint32_t status;
    char name[24];
} UiMon;

int32_t UiGame_PartyCount(void);
bool UiGame_Mon(int32_t slot, UiMon *out);

bool UiGame_Save(void);
uint8_t UiGame_SaveSlot(void);                   // 1..3
bool UiGame_SaveOverwritesOtherGame(void);

// ---------- Bag (ui_game_items.c) ----------
typedef struct { uint16_t item, qty; } UiBagRow;
enum { UI_SORT_NUMBER, UI_SORT_AZ, UI_SORT_MOST, UI_SORT_LEAST, UI_SORT_COUNT };
// pocket: ITEMS_POCKET .. KEYITEMS_POCKET (constants/item.h). The Battle Pyramid bag shows in ITEMS.
int32_t UiGame_PocketItems(int32_t pocket, UiBagRow *rows, int32_t max);
void UiGame_SortPocket(int32_t pocket, int32_t mode);
void UiGame_ItemDescription(uint16_t itemId, char *out, size_t cap);
int32_t UiGame_ItemPocket(uint16_t itemId);
bool UiGame_ItemIsTmHm(uint16_t itemId);
uint16_t UiGame_ItemMove(uint16_t itemId);
bool UiGame_CanToss(uint16_t itemId);
bool UiGame_CanGive(uint16_t itemId);
bool UiGame_CanRegister(uint16_t itemId);
void UiGame_Toss(uint16_t itemId, uint16_t n);
void UiGame_Register(uint16_t itemId);    // toggles: registering it again removes it

typedef enum { UI_USE_DONE, UI_USE_FAIL, UI_USE_CLOSE, UI_USE_TARGET } UiUseResult;
// USE from the bag. DONE/FAIL: msg says what happened. CLOSE: close the UI and run afterClose in
// the field (bike, rods, escape rope...). TARGET: open the party to choose who gets it.
UiUseResult UiGame_UseFromBag(uint16_t itemId, char *msg, size_t cap, void (**afterClose)(void));

typedef struct { int16_t before[6], after[6]; uint8_t levelBefore, levelAfter; } UiStatDelta;
typedef enum { UI_ITEM_FAIL, UI_ITEM_DONE, UI_ITEM_DELTA, UI_ITEM_CHOOSE_MOVE, UI_ITEM_FORGET_MOVE, UI_ITEM_CLOSE } UiItemResult;
// Item (medicine, vitamin, Rare Candy, stone, PP item, TM/HM) on a party POKéMON.
// DELTA: show the stats before / after. CHOOSE_MOVE: PP items, then UiGame_ItemOnMove.
// FORGET_MOVE: TM with 4 moves, then UiGame_ReplaceMove. CLOSE: evolution, run afterClose.
UiItemResult UiGame_ItemOnMon(int32_t slot, uint16_t item, UiStatDelta *delta, char *msg, size_t cap, void (**afterClose)(void));
bool UiGame_ItemOnMove(int32_t slot, uint16_t item, int32_t moveIdx);
bool UiGame_ReplaceMove(int32_t slot, uint16_t item, int32_t moveIdx);    // false: HM moves stay
bool UiGame_CanLearnTm(int32_t slot, uint16_t item, bool *known);
void UiGame_GiveItem(int32_t slot, uint16_t item, uint16_t *previous);    // previous goes back to the bag
bool UiGame_TakeItem(int32_t slot, uint16_t *taken);
void UiGame_MoveInfo(int32_t slot, int32_t moveIdx, uint16_t *move, uint8_t *pp, uint8_t *maxPp);
void UiGame_MoveName(uint16_t move, char *out, size_t cap);

// ---------- Summary (ui_game.c) ----------
typedef struct
{
    uint16_t dexNo, otId, item, hp, stats[6];
    uint8_t types[2], metLevel, level, expPct, ivs[6], evs[6], eggCycles;
    int8_t natureUp, natureDown;          // display index (1 ATK .. 5 SPEED), 0 = neutral
    uint32_t exp, expToNext;
    bool showIvEv;
    char species[16], ot[12], nature[12], ability[16], abilityDesc[64], metAt[24];
} UiSummary;
typedef struct
{
    uint16_t move, power;
    uint8_t type, accuracy, pp, maxPp, category, contestCategory, appeal, jam;   // category: 0 phys, 1 spec, 2 status
    char name[16], desc[96], contestDesc[96];
} UiMove;
int32_t UiGame_MonStatusIcon(int32_t slot);   // row of UI_SPR_STATUS_FIRST, or -1
void UiGame_Summary(int32_t slot, UiSummary *s);
void UiGame_Move(int32_t slot, int32_t idx, UiMove *m);
void UiGame_SwapMoves(int32_t slot, int32_t a, int32_t b);
void UiGame_SwapPartyMons(int32_t a, int32_t b);
uint16_t UiGame_FieldMove(int32_t slot, int32_t idx);      // MOVE_NONE if not a field move
bool UiGame_ShareHp(int32_t from, int32_t to, char *msg, size_t cap);

// ---------- Map, PokéNav (ui_game_nav.c) ----------
uint16_t UiGame_MapSecAt(int32_t gx, int32_t gy);              // 28x15 grid of the region map
void UiGame_MapSecName(uint16_t sec, char *out, size_t cap);
bool UiGame_PlayerMapPos(int32_t *gx, int32_t *gy);
uint16_t UiGame_PlayerMapSec(void);
bool UiGame_MapSecPos(uint16_t sec, int32_t *gx, int32_t *gy);
bool UiGame_CanFlyTo(uint16_t sec);
enum { UI_WILD_GRASS, UI_WILD_SURF, UI_WILD_FISH, UI_WILD_ROCK, UI_WILD_COUNT };
typedef struct { uint16_t species; uint8_t minLv, maxLv, rod; } UiWild;     // rod 1 OLD, 2 GOOD, 3 SUPER
bool UiGame_TimedEncounters(void);
int32_t UiGame_CurrentTimePhase(void);                           // 0 MORNING, 1 DAY, 2 NIGHT
// phase: time of day (grass, with TIMED ENCOUNTERS on); rod: 0 all, 1..3 one rod (fishing)
int32_t UiGame_Wild(uint16_t sec, int32_t method, int32_t phase, int32_t rod, UiWild *out, int32_t max);
void UiGame_SpeciesName(uint16_t species, char *out, size_t cap);
int32_t UiGame_SpeciesAreas(uint16_t species, uint8_t *out, int32_t count);   // wild areas of a species
typedef struct { bool special, rematch; uint16_t id, mapSec, trainerPic; char name[24], desc[32]; } UiCaller;   // trainerPic 0xFFFF: none
int32_t UiGame_Callers(const UiCaller **out);
void UiGame_CallMessage(const UiCaller *c, char *out, size_t cap);
void UiGame_Condition(int32_t slot, uint8_t out[6]);           // COOL BEAUTY CUTE SMART TOUGH SHEEN
int32_t UiGame_Ribbons(int32_t slot, uint8_t *ids, int32_t max);
void UiGame_RibbonText(uint8_t id, char *name, size_t nc, char *desc, size_t dc);

// POKEBLOCKS fed from the CONDITION tab
typedef struct { uint8_t slot, color, level, feel; char name[24]; } UiBlock;
bool UiGame_HasPokeblockCase(void);
int32_t UiGame_Pokeblocks(UiBlock *out, int32_t max);
int32_t UiGame_FeedPokeblock(int32_t slot, int32_t pkblSlot);   // 1 loved, 0 ate, -1 disliked, -2 sheen max, -3 egg

// Battle BAG / POKEMON (ui_game_battle.c)
typedef enum { UI_BUSE_FAIL, UI_BUSE_CLOSE, UI_BUSE_TARGET } UiBattleUse;
bool UiGame_InBattleMenu(void);
void UiGame_BattleMenuEnd(void);       // ui_core.c, when the UI closes
bool UiGame_MonIsInBattle(int32_t slot);
const char *UiGame_BattleSwitch(int32_t slot);
bool UiGame_BattlePartyCanCancel(void);
bool UiGame_BattleItemUsable(uint16_t item);
UiBattleUse UiGame_BattleUse(uint16_t item, char *msg, size_t cap);
UiItemResult UiGame_BattleItemOnMon(int32_t slot, uint16_t item, int32_t moveIdx, char *msg, size_t cap);

#endif // UI_GAME_H
