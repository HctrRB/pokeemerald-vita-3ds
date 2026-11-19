// PC boxes of the new UI (design/stitch/pc_cajas), replacing pokemon_storage_system.c's screen when
// a mode is picked in the PC menu: WITHDRAW, DEPOSIT, MOVE POKéMON or MOVE ITEMS. Opened over the
// world from the PC menu task (UiCore_OpenFromScript); closing goes back to the PC menu.
//
// The box is the game's own wallpaper (UiSpr_Wallpaper, x2) with the 6 x 5 grid on it; the party on
// the left, the POKéMON under the cursor on the right. The rules are the GBA's:
// - the last POKéMON able to battle can't be picked up, stored or released;
// - an EGG can't be released, a POKéMON holding MAIL can't be released or stored in a box;
// - releasing the last POKéMON that knows a needed field move (SURF, DIVE; STRENGTH and ROCK SMASH
//   inside the POKéMON LEAGUE) makes it come back (RunCanReleaseMon);
// - ○ asks "Continue BOX operations?" and is refused while holding a POKéMON or an item.
// Not here yet: SUMMARY (the summary screen of a boxed POKéMON).

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "battle_main.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "item.h"
#include "mail.h"
#include "main.h"
#include "naming_screen.h"
#include "overworld.h"
#include "palette.h"
#include "constants/rgb.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "sound.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/moves.h"
#include "constants/songs.h"

extern void PokeStorage_ReturnToPcMenuForUi(void);
extern void PokeStorage_FieldReturnAfterNamingForUi(void);
extern u8 PokeStorage_GetBoxWallpaperForUi(u8 boxId);
extern void PokeStorage_SetBoxWallpaperForUi(u8 boxId, u8 wallpaperId);
extern s16 CompactPartySlots(void);
extern void BoxMonAtToMon(u8 boxId, u8 boxPosition, struct Pokemon *dst);
extern void SetBoxMonAt(u8 boxId, u8 boxPosition, struct BoxPokemon *src);
extern void ZeroBoxMonAt(u8 boxId, u8 boxPosition);
extern struct BoxPokemon *GetBoxedMonPtr(u8 boxId, u8 boxPosition);
extern u8 *GetBoxNamePtr(u8 boxId);
extern u8 StorageGetCurrentBox(void);
extern s16 GetFirstFreeBoxSpot(u8 boxId);
extern u8 CountPartyAliveNonEggMonsExcept(u8 slotToIgnore);

enum { MODE_WITHDRAW, MODE_DEPOSIT, MODE_MOVE, MODE_ITEMS };   // = OPTION_* of the PC menu
enum { AREA_BOX, AREA_PARTY, AREA_PLATE };
enum { ACT_MOVE, ACT_SUMMARY, ACT_WITHDRAW, ACT_STORE, ACT_MARK, ACT_RELEASE, ACT_TAKE, ACT_GIVE, ACT_BAG, ACT_INFO };
enum { BOXM_JUMP = 20, BOXM_WALLPAPER, BOXM_NAME };
enum { POP_NONE, POP_ACTIONS, POP_BOXMENU, POP_BOXLIST, POP_SETS, POP_WALLS, POP_MARKS, POP_RELEASE, POP_EXIT, POP_GIVE, POP_MSG };

#define WALL_X 212
#define WALL_Y 70
#define GRID_X (WALL_X + 16)
#define GRID_Y (WALL_Y + 54)
#define CELL_W 48
#define CELL_H 46
#define PARTY_X 24
#define PARTY_Y 96
#define SLOT_H 54
#define BOXES TOTAL_BOXES_COUNT

static int32_t sMode, sArea, sPos, sPartyPos;
static bool sQuick;
static bool sHolding;            // a POKéMON on the cursor (sHeld)
static struct Pokemon sHeld;
static uint16_t sHeldItem;       // MOVE ITEMS: an item on the cursor
static int32_t sPop;
static UiMenu sMenu;
static UiList sBoxList, sGiveList;
static int32_t sBoxListStore;    // the box list is the STORE target (DEPOSIT)
static int32_t sWallSet, sWallPreview;
static int32_t sMarkSel;
static uint8_t sMarks;
static char sMsgs[4][96];        // messages shown one after another (✕ / tap)
static int32_t sMsgCount, sMsgAt;
static void (*sMsgDone)(void);
static uint16_t sGiveItems[64];
static int32_t sGiveCount;
static uint32_t sFrame;

// ---------- Data ----------

static uint8_t Box(void) { return StorageGetCurrentBox(); }

static struct BoxPokemon *BoxMon(int32_t pos) { return GetBoxedMonPtr(Box(), (u8)pos); }

static bool BoxHas(int32_t pos) { return GetBoxMonData(BoxMon(pos), MON_DATA_SPECIES) != SPECIES_NONE; }

static int32_t PartyCount(void) { return CalculatePlayerPartyCount(); }

static int32_t BoxCount(uint8_t box)
{
    int32_t n = 0;
    for (int32_t i = 0; i < IN_BOX_COUNT; i++)
        if (GetBoxMonData(GetBoxedMonPtr(box, (u8)i), MON_DATA_SPECIES) != SPECIES_NONE)
            n++;
    return n;
}

// The POKéMON under the cursor (or held); NULL when the slot is empty
static struct BoxPokemon *CursorMon(void)
{
    if (sArea == AREA_PARTY)
        return sPartyPos < PartyCount() ? &gPlayerParty[sPartyPos].box : NULL;
    if (sArea == AREA_BOX)
        return BoxHas(sPos) ? BoxMon(sPos) : NULL;
    return NULL;
}

static void MonName(struct BoxPokemon *bm, char *out, size_t cap)
{
    u8 name[POKEMON_NAME_LENGTH + 1];
    GetBoxMonData(bm, MON_DATA_NICKNAME, name);
    name[POKEMON_NAME_LENGTH] = EOS;
    UiGame_Text(name, out, cap);
}

// ---------- Messages ----------

static void Msg(void (*done)(void), const char *a, const char *b, const char *c, const char *d)
{
    const char *m[4] = { a, b, c, d };
    sMsgCount = 0;
    for (int32_t i = 0; i < 4; i++)
        if (m[i])
            snprintf(sMsgs[sMsgCount++], sizeof(sMsgs[0]), "%s", m[i]);
    sMsgAt = 0;
    sMsgDone = done;
    sPop = POP_MSG;
}

static void Fail(const char *text)
{
    PlaySE(SE_FAILURE);
    Msg(NULL, text, NULL, NULL, NULL);
}

// ---------- Moving POKéMON ----------

// Picks up the POKéMON under the cursor (MOVE). False with the reason already shown.
static bool PickUp(void)
{
    if (sArea == AREA_PARTY)
    {
        if (sPartyPos >= PartyCount())
            return false;
        if (CountPartyAliveNonEggMonsExcept((u8)sPartyPos) == 0)
        {
            Fail("That's your last POKéMON!");
            return false;
        }
        sHeld = gPlayerParty[sPartyPos];
        ZeroMonData(&gPlayerParty[sPartyPos]);
        CompactPartySlots();
        CalculatePlayerPartyCount();
    }
    else
    {
        if (!BoxHas(sPos))
            return false;
        BoxMonAtToMon(Box(), (u8)sPos, &sHeld);
        ZeroBoxMonAt(Box(), (u8)sPos);
    }
    sHolding = true;
    PlaySE(SE_SELECT);
    return true;
}

// Puts the held POKéMON down (swapping with the one there)
static void PutDown(void)
{
    if (sArea == AREA_PARTY)
    {
        const int32_t n = PartyCount();
        if (sPartyPos < n)
        {
            struct Pokemon tmp = gPlayerParty[sPartyPos];
            gPlayerParty[sPartyPos] = sHeld;
            sHeld = tmp;
            PlaySE(SE_SELECT);
            return;                                    // still holding the swapped one
        }
        gPlayerParty[n] = sHeld;
        CalculatePlayerPartyCount();
        sPartyPos = n;
    }
    else if (sArea == AREA_BOX)
    {
        if (ItemIsMail((u16)GetMonData(&sHeld, MON_DATA_HELD_ITEM)))
        {
            Fail("MAIL can't be stored!");
            return;
        }
        if (BoxHas(sPos))
        {
            struct Pokemon tmp;
            BoxMonAtToMon(Box(), (u8)sPos, &tmp);
            SetBoxMonAt(Box(), (u8)sPos, &sHeld.box);
            sHeld = tmp;
            PlaySE(SE_SELECT);
            return;
        }
        SetBoxMonAt(Box(), (u8)sPos, &sHeld.box);
    }
    else
        return;
    sHolding = false;
    PlaySE(SE_SELECT);
}

static void Withdraw(void)
{
    if (PartyCount() >= PARTY_SIZE)
    {
        Fail("Your party's full!");
        return;
    }
    char name[16];
    MonName(BoxMon(sPos), name, sizeof(name));
    BoxMonAtToMon(Box(), (u8)sPos, &gPlayerParty[PartyCount()]);
    ZeroBoxMonAt(Box(), (u8)sPos);
    CalculatePlayerPartyCount();
    PlaySE(SE_SELECT);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s was taken out of the BOX.", name);
    Msg(NULL, buf, NULL, NULL, NULL);
}

static void Store(uint8_t box)
{
    struct Pokemon *mon = &gPlayerParty[sPartyPos];
    const s16 spot = GetFirstFreeBoxSpot(box);
    if (spot < 0)
    {
        Fail("The BOX is full.");
        return;
    }
    char name[16], buf[64];
    MonName(&mon->box, name, sizeof(name));
    SetBoxMonAt(box, (u8)spot, &mon->box);
    ZeroMonData(mon);
    CompactPartySlots();
    CalculatePlayerPartyCount();
    if (sPartyPos >= PartyCount())
        sPartyPos = PartyCount() - 1;
    PlaySE(SE_SELECT);
    snprintf(buf, sizeof(buf), "%s was deposited.", name);
    Msg(NULL, buf, NULL, NULL, NULL);
}

// ---------- Release (RunCanReleaseMon) ----------

static bool KnowsAny(struct BoxPokemon *bm, const uint16_t *moves, int32_t n, uint32_t *mask)
{
    *mask = 0;
    for (int32_t i = 0; i < n; i++)
        for (int32_t m = 0; m < MAX_MON_MOVES; m++)
            if (GetBoxMonData(bm, MON_DATA_MOVE1 + m) == moves[i])
                *mask |= 1u << i;
    return *mask != 0;
}

static bool CanRelease(struct BoxPokemon *self)
{
    // At least three POKéMON in all (AtLeastThreeUsableMons)
    int32_t count = 0;
    for (int32_t i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_SANITY_HAS_SPECIES))
            count++;
    for (int32_t b = 0; b < BOXES && count < 3; b++)
        for (int32_t p = 0; p < IN_BOX_COUNT && count < 3; p++)
            if (CheckBoxMonSanityAt((u8)b, (u8)p))
                count++;
    if (count < 3)
        return false;
    // Field moves that must stay with someone (sRestrictedReleaseMoves)
    uint16_t moves[4];
    int32_t n = 0;
    moves[n++] = MOVE_SURF;
    moves[n++] = MOVE_DIVE;
    const s8 g = gSaveBlock1Ptr->location.mapGroup, m = gSaveBlock1Ptr->location.mapNum;
    if ((g == MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F) && m == MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F))
     || (g == MAP_GROUP(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F) && m == MAP_NUM(MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F)))
    {
        moves[n++] = MOVE_STRENGTH;
        moves[n++] = MOVE_ROCK_SMASH;
    }
    uint32_t need, has;
    if (!KnowsAny(self, moves, n, &need))
        return true;
    for (int32_t i = 0; i < PARTY_SIZE && need; i++)
        if (&gPlayerParty[i].box != self && KnowsAny(&gPlayerParty[i].box, moves, n, &has))
            need &= ~has;
    for (int32_t b = 0; b < BOXES && need; b++)
        for (int32_t p = 0; p < IN_BOX_COUNT && need; p++)
        {
            struct BoxPokemon *bm = GetBoxedMonPtr((u8)b, (u8)p);
            if (bm != self && GetBoxMonData(bm, MON_DATA_SPECIES) != SPECIES_NONE && KnowsAny(bm, moves, n, &has))
                need &= ~has;
        }
    return need == 0;
}

// Checks before the YES / NO (egg, mail, last POKéMON)
static bool ReleaseAllowed(void)
{
    struct BoxPokemon *bm = CursorMon();
    if (bm == NULL)
        return false;
    if (GetBoxMonData(bm, MON_DATA_IS_EGG)) { Fail("You can't release an EGG."); return false; }
    if (ItemIsMail((u16)GetBoxMonData(bm, MON_DATA_HELD_ITEM))) { Fail("Please remove the MAIL."); return false; }
    if (sArea == AREA_PARTY && CountPartyAliveNonEggMonsExcept((u8)sPartyPos) == 0) { Fail("That's your last POKéMON!"); return false; }
    return true;
}

static void Release(void)
{
    struct BoxPokemon *bm = CursorMon();
    char name[16], a[64], b[64];
    MonName(bm, name, sizeof(name));
    snprintf(a, sizeof(a), "%s was released.", name);
    if (!CanRelease(bm))
    {
        // It comes back: the GBA's "… … … … !" sequence
        snprintf(b, sizeof(b), "%s came back!", name);
        Msg(NULL, a, "… … … … !", b, "Was it worried about you?");
        return;
    }
    if (sArea == AREA_PARTY)
    {
        ZeroMonData(&gPlayerParty[sPartyPos]);
        CompactPartySlots();
        CalculatePlayerPartyCount();
        if (sPartyPos >= PartyCount())
            sPartyPos = PartyCount() - 1;
    }
    else
        ZeroBoxMonAt(Box(), (u8)sPos);
    PlaySE(SE_SELECT);
    snprintf(b, sizeof(b), "Bye-bye, %s!", name);
    Msg(NULL, a, b, NULL, NULL);
}

// ---------- Items (MOVE ITEMS) ----------

static uint16_t HeldItemOf(struct BoxPokemon *bm)
{
    return bm ? (uint16_t)GetBoxMonData(bm, MON_DATA_HELD_ITEM) : ITEM_NONE;
}

static void SetHeldItem(struct BoxPokemon *bm, uint16_t item)
{
    SetBoxMonData(bm, MON_DATA_HELD_ITEM, &item);
    if (sArea == AREA_PARTY)
        SetMonData(&gPlayerParty[sPartyPos], MON_DATA_HELD_ITEM, &item);
}

// ✕ with an item on the cursor: give it, or swap it with the one held
static void PlaceItem(void)
{
    struct BoxPokemon *bm = CursorMon();
    if (bm == NULL || GetBoxMonData(bm, MON_DATA_IS_EGG))
    {
        PlaySE(SE_FAILURE);
        return;
    }
    const uint16_t had = HeldItemOf(bm);
    if (ItemIsMail(had))
    {
        Fail("Please remove the MAIL.");
        return;
    }
    SetHeldItem(bm, sHeldItem);
    sHeldItem = had;
    PlaySE(SE_SELECT);
}

static void BuildGiveList(void)
{
    static const int32_t pockets[3] = { ITEMS_POCKET, BALLS_POCKET, BERRIES_POCKET };
    sGiveCount = 0;
    for (int32_t p = 0; p < 3; p++)
    {
        UiBagRow rows[48];
        const int32_t n = UiGame_PocketItems(pockets[p], rows, 48);
        for (int32_t i = 0; i < n && sGiveCount < 64; i++)
            if (UiGame_CanGive(rows[i].item) && !ItemIsMail(rows[i].item))
                sGiveItems[sGiveCount++] = rows[i].item;
    }
    UiList_Reset(&sGiveList, sGiveCount, 6);
}

// ---------- Popups ----------

static void OpenActions(void)
{
    struct BoxPokemon *bm = CursorMon();
    if (bm == NULL)
        return;
    if (sMode == MODE_ITEMS && GetBoxMonData(bm, MON_DATA_IS_EGG))
    {
        Fail("EGGs can't hold items.");
        return;
    }
    char name[16];
    MonName(bm, name, sizeof(name));
    const int32_t x = sArea == AREA_PARTY ? 196 : 548;
    UiMenu_Open(&sMenu, name, x, 110, 260);
    if (sMode == MODE_ITEMS)
    {
        const uint16_t it = HeldItemOf(bm);
        if (it != ITEM_NONE)
        {
            if (!ItemIsMail(it)) UiMenu_Add(&sMenu, "MOVE", ACT_MOVE);
            if (!ItemIsMail(it)) UiMenu_Add(&sMenu, "BAG", ACT_BAG);
            UiMenu_Add(&sMenu, "INFO", ACT_INFO);
        }
        else
            UiMenu_Add(&sMenu, "GIVE", ACT_GIVE);
    }
    else
    {
        if (sMode == MODE_MOVE) UiMenu_Add(&sMenu, "MOVE", ACT_MOVE);
        if (sArea == AREA_BOX && sMode != MODE_DEPOSIT) UiMenu_Add(&sMenu, "WITHDRAW", ACT_WITHDRAW);
        if (sArea == AREA_PARTY && sMode != MODE_WITHDRAW) UiMenu_Add(&sMenu, "STORE", ACT_STORE);
        UiMenu_Add(&sMenu, "MARK", ACT_MARK);
        UiMenu_Add(&sMenu, "RELEASE", ACT_RELEASE);
    }
    UiMenu_Add(&sMenu, "CANCEL", UI_MENU_CANCEL);
    sPop = POP_ACTIONS;
    PlaySE(SE_SELECT);
}

static void OpenBoxList(bool store)
{
    sBoxListStore = store;
    UiList_Reset(&sBoxList, BOXES, 7);
    sBoxList.sel = store ? Box() : Box();
    sPop = POP_BOXLIST;
}

static void OpenSets(void)
{
    UiMenu_Open(&sMenu, "WALLPAPER", 236, 120, 300);
    UiMenu_Add(&sMenu, "SCENERY 1", 0);
    UiMenu_Add(&sMenu, "SCENERY 2", 1);
    UiMenu_Add(&sMenu, "SCENERY 3", 2);
    UiMenu_Add(&sMenu, "ETCETERA", 3);
    if (IsWaldaWallpaperUnlocked())
        UiMenu_Add(&sMenu, "FRIENDS", 4);
    UiMenu_Add(&sMenu, "CANCEL", UI_MENU_CANCEL);
    sPop = POP_SETS;
}

static const char *const sWallNames[] = { "FOREST", "CITY", "DESERT", "SAVANNA", "CRAG", "VOLCANO", "SNOW", "CAVE",
    "BEACH", "SEAFLOOR", "RIVER", "SKY", "POLKA-DOT", "POKéCENTER", "MACHINE", "PLAIN", "FRIENDS" };

static void OpenWalls(int32_t set)
{
    sWallSet = set;
    if (set == 4)
    {
        PokeStorage_SetBoxWallpaperForUi(Box(), 16);   // WALLPAPER_FRIENDS (its pattern is chosen at Walda's)
        sPop = POP_NONE;
        PlaySE(SE_SELECT);
        return;
    }
    UiMenu_Open(&sMenu, "WALLPAPER", 236, 120, 300);
    for (int32_t i = 0; i < 4; i++)
        UiMenu_Add(&sMenu, sWallNames[set * 4 + i], set * 4 + i);
    UiMenu_Add(&sMenu, "CANCEL", UI_MENU_CANCEL);
    sWallPreview = set * 4;
    sPop = POP_WALLS;
}

// ---------- Leaving ----------

static void AfterClose(void)
{
    PokeStorage_ReturnToPcMenuForUi();
}

static void StartBoxNaming(void)
{
    // As the GBA's NAME: the naming screen, then the field comes back and the boxes reopen
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);       // the map must not show behind the keyboard
    CleanupOverworldWindowsAndTilemaps();
    gFieldCallback = PokeStorage_FieldReturnAfterNamingForUi;
    DoNamingScreen(NAMING_SCREEN_BOX, GetBoxNamePtr(Box()), 0, 0, 0, CB2_ReturnToField);
}

// ---------- Screen ----------

bool Ui_OpenPc(uint8_t option)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || option > MODE_ITEMS || !__atomic_load_n(&gUiVoxelFrame, __ATOMIC_RELAXED))
        return false;
    sMode = option;
    UiCore_OpenFromScript(&gUiPcScreen, AfterClose);
    return true;
}

static void Enter(void)
{
    sArea = sMode == MODE_DEPOSIT ? AREA_PARTY : AREA_BOX;
    sPos = 0;
    sPartyPos = 0;
    sQuick = false;
    sHolding = false;
    sHeldItem = ITEM_NONE;
    sPop = POP_NONE;
    sMenu.open = false;
}

static void ChangeBox(int32_t d)
{
    gPokemonStoragePtr->currentBox = (u8)((Box() + BOXES + d) % BOXES);
    PlaySE(SE_SELECT);
}

static void Act(int32_t id)
{
    struct BoxPokemon *bm = CursorMon();
    switch (id)
    {
    case ACT_MOVE:
        if (sMode == MODE_ITEMS)
        {
            sHeldItem = HeldItemOf(bm);
            SetHeldItem(bm, ITEM_NONE);
            PlaySE(SE_SELECT);
        }
        else
            PickUp();
        break;
    case ACT_WITHDRAW: Withdraw(); break;
    case ACT_STORE:
        if (CountPartyAliveNonEggMonsExcept((u8)sPartyPos) == 0) { Fail("That's your last POKéMON!"); break; }
        if (ItemIsMail(HeldItemOf(bm))) { Fail("MAIL can't be stored!"); break; }
        OpenBoxList(true);
        break;
    case ACT_MARK:
        sMarks = (uint8_t)GetBoxMonData(bm, MON_DATA_MARKINGS);
        sMarkSel = 0;
        sPop = POP_MARKS;
        break;
    case ACT_RELEASE:
        if (ReleaseAllowed())
        {
            UiMenu_Open(&sMenu, "RELEASE", 330, 200, 300);
            UiMenu_Add(&sMenu, "YES", 1);
            UiMenu_Add(&sMenu, "NO", UI_MENU_CANCEL);
            sMenu.list.sel = 1;
            sPop = POP_RELEASE;
        }
        break;
    case ACT_BAG:
    {
        const uint16_t it = HeldItemOf(bm);
        if (!AddBagItem(it, 1)) { Fail("The BAG is full."); break; }
        SetHeldItem(bm, ITEM_NONE);
        PlaySE(SE_SELECT);
        Msg(NULL, "Placed item in the BAG.", NULL, NULL, NULL);
        break;
    }
    case ACT_INFO:
    {
        char d[160];
        UiGame_ItemDescription(HeldItemOf(bm), d, sizeof(d));
        Msg(NULL, d, NULL, NULL, NULL);
        break;
    }
    case ACT_GIVE:
        BuildGiveList();
        if (sGiveCount == 0) { Fail("There are no items to give."); break; }
        sPop = POP_GIVE;
        break;
    }
}

static bool UpdatePopup(const UiInput *in)
{
    switch (sPop)
    {
    case POP_MSG:
        if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap)
        {
            if (++sMsgAt >= sMsgCount)
            {
                sPop = POP_NONE;
                if (sMsgDone) sMsgDone();
            }
        }
        return true;
    case POP_ACTIONS:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id != UI_MENU_CANCEL) Act(id);
        return true;
    }
    case POP_BOXMENU:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id == BOXM_JUMP) OpenBoxList(false);
        else if (id == BOXM_WALLPAPER) OpenSets();
        else if (id == BOXM_NAME) UiNav_Close(StartBoxNaming);
        return true;
    }
    case POP_SETS:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id != UI_MENU_CANCEL) OpenWalls(id);
        return true;
    }
    case POP_WALLS:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (sMenu.list.sel < 4) sWallPreview = sWallSet * 4 + sMenu.list.sel;   // live preview
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id != UI_MENU_CANCEL)
        {
            PokeStorage_SetBoxWallpaperForUi(Box(), (u8)id);
            PlaySE(SE_SELECT);
        }
        return true;
    }
    case POP_RELEASE:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id == 1) Release();
        return true;
    }
    case POP_EXIT:
    {
        const int32_t id = UiMenu_Update(&sMenu, in);
        if (id == UI_MENU_NONE) return true;
        sPop = POP_NONE;
        if (id == 2) { PlaySE(SE_PC_OFF); return false; }   // NO: back to the PC menu
        return true;
    }
    case POP_BOXLIST:
    {
        if ((in->pressed & VITA_BTN_CIRCLE) || (in->tap && !UiHit(in, 264, 70, 432, 360))) { sPop = POP_NONE; PlaySE(SE_SELECT); return true; }
        const int32_t a = UiList_Update(&sBoxList, in, 280, 120, 400, 40, true);
        if (a < 0) return true;
        sPop = POP_NONE;
        if (sBoxListStore) Store((uint8_t)a);
        else { gPokemonStoragePtr->currentBox = (u8)a; PlaySE(SE_SELECT); }
        return true;
    }
    case POP_GIVE:
    {
        if ((in->pressed & VITA_BTN_CIRCLE) || (in->tap && !UiHit(in, 284, 80, 392, 310))) { sPop = POP_NONE; PlaySE(SE_SELECT); return true; }
        const int32_t a = UiList_Update(&sGiveList, in, 300, 130, 360, 40, true);
        if (a < 0) return true;
        sPop = POP_NONE;
        const uint16_t it = sGiveItems[a];
        RemoveBagItem(it, 1);
        SetHeldItem(CursorMon(), it);
        PlaySE(SE_SELECT);
        return true;
    }
    case POP_MARKS:
    {
        if (in->pressed & VITA_BTN_CIRCLE) { sPop = POP_NONE; return true; }
        if (in->repeat & VITA_BTN_UP) sMarkSel = (sMarkSel + 5) % 6;
        if (in->repeat & VITA_BTN_DOWN) sMarkSel = (sMarkSel + 1) % 6;
        int32_t hit = -1;
        for (int32_t i = 0; i < 6; i++)
            if (UiHit(in, 360, 150 + i * 44, 240, 42)) hit = i;
        if (hit >= 0 && hit != sMarkSel) { sMarkSel = hit; return true; }
        if ((in->pressed & VITA_BTN_CROSS) || hit >= 0)
        {
            if (sMarkSel < 4) sMarks ^= (uint8_t)(1 << sMarkSel);
            else
            {
                if (sMarkSel == 4)
                {
                    struct BoxPokemon *bm = CursorMon();
                    SetBoxMonData(bm, MON_DATA_MARKINGS, &sMarks);
                    if (sArea == AREA_PARTY) SetMonData(&gPlayerParty[sPartyPos], MON_DATA_MARKINGS, &sMarks);
                }
                sPop = POP_NONE;
            }
            PlaySE(SE_SELECT);
        }
        return true;
    }
    default:
        return true;
    }
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sPop != POP_NONE)
        return UpdatePopup(in);

    if (in->pressed & VITA_BTN_CIRCLE)
    {
        if (sHolding) { Fail("You're holding a POKéMON!"); return true; }
        if (sHeldItem != ITEM_NONE) { Fail("You're holding an item!"); return true; }
        if (sHeldItem != ITEM_NONE) { Fail("Put the item down first."); return true; }
        UiMenu_Open(&sMenu, "PC", 330, 200, 300);
        UiMenu_Add(&sMenu, "YES", 1);
        UiMenu_Add(&sMenu, "NO", 2);
        sPop = POP_EXIT;
        PlaySE(SE_SELECT);
        return true;
    }
    if (in->pressed & VITA_BTN_L) { ChangeBox(-1); return true; }
    if (in->pressed & VITA_BTN_R) { ChangeBox(1); return true; }
    if (in->pressed & VITA_BTN_SELECT && sMode != MODE_ITEMS) { sQuick = !sQuick; PlaySE(SE_SELECT); return true; }
    const bool partyOk = sMode != MODE_WITHDRAW;

    // D-pad: the box grid (6 x 5), the plate above it, the party on the left
    int32_t dx = (in->repeat & VITA_BTN_RIGHT) ? 1 : (in->repeat & VITA_BTN_LEFT) ? -1 : 0;
    int32_t dy = (in->repeat & VITA_BTN_DOWN) ? 1 : (in->repeat & VITA_BTN_UP) ? -1 : 0;
    if (dx || dy)
    {
        if (sArea == AREA_BOX)
        {
            const int32_t c = sPos % 6, r = sPos / 6;
            if (dx < 0 && c == 0 && partyOk) { sArea = AREA_PARTY; sPartyPos = r < 6 ? r : 5; }
            else if (dy < 0 && r == 0) sArea = AREA_PLATE;
            else sPos = ((r + dy + 5) % 5) * 6 + (c + dx + 6) % 6;
        }
        else if (sArea == AREA_PARTY)
        {
            if (dx > 0) { sArea = AREA_BOX; sPos = (sPartyPos < 5 ? sPartyPos : 4) * 6; }
            else if (dy) sPartyPos = (sPartyPos + dy + 6) % 6;
        }
        else
        {
            if (dx) ChangeBox(dx);
            else if (dy > 0) sArea = AREA_BOX, sPos = 2;
        }
        PlaySE(SE_SELECT);
        return true;
    }

    // Touch: party slots, box cells, the plate and its L / R keys
    if (in->tap)
    {
        if (UiHit(in, WALL_X, WALL_Y + 8, 32, 32)) { ChangeBox(-1); return true; }
        if (UiHit(in, WALL_X + 288, WALL_Y + 8, 32, 32)) { ChangeBox(1); return true; }
        int32_t area = -1, pos = 0;
        if (UiHit(in, WALL_X + 36, WALL_Y, 248, 48)) area = AREA_PLATE;
        for (int32_t i = 0; i < IN_BOX_COUNT && area < 0; i++)
            if (UiHit(in, GRID_X + (i % 6) * CELL_W, GRID_Y + (i / 6) * CELL_H, CELL_W, CELL_H)) area = AREA_BOX, pos = i;
        for (int32_t i = 0; i < PARTY_SIZE && area < 0 && partyOk; i++)
            if (UiHit(in, PARTY_X, PARTY_Y + i * SLOT_H, 156, SLOT_H - 4)) area = AREA_PARTY, pos = i;
        if (area >= 0)
        {
            const bool same = area == sArea && (area == AREA_PLATE || (area == AREA_BOX ? pos == sPos : pos == sPartyPos));
            sArea = area;
            if (area == AREA_BOX) sPos = pos;
            if (area == AREA_PARTY) sPartyPos = pos;
            if (!same) { PlaySE(SE_SELECT); return true; }
        }
        else
            return true;
    }

    if ((in->pressed & VITA_BTN_CROSS) || in->tap)
    {
        if (sArea == AREA_PLATE)
        {
            UiMenu_Open(&sMenu, "BOX", 236, 120, 260);
            UiMenu_Add(&sMenu, "JUMP", BOXM_JUMP);
            UiMenu_Add(&sMenu, "WALLPAPER", BOXM_WALLPAPER);
            UiMenu_Add(&sMenu, "NAME", BOXM_NAME);
            UiMenu_Add(&sMenu, "CANCEL", UI_MENU_CANCEL);
            sPop = POP_BOXMENU;
            PlaySE(SE_SELECT);
            return true;
        }
        if (sMode == MODE_ITEMS && sHeldItem != ITEM_NONE) { PlaceItem(); return true; }
        if (sHolding) { PutDown(); return true; }
        if (sQuick && sMode == MODE_MOVE) { PickUp(); return true; }
        OpenActions();
    }
    return true;
}

// ---------- Drawing ----------

static void DrawMarks(int32_t x, int32_t y, uint8_t marks, uint32_t on, uint32_t off)
{
    static const char *const sym[4] = { "o", "^", "#", "<3" };
    for (int32_t i = 0; i < 4; i++)
        x += UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x, y, sym[i], (marks >> i) & 1 ? on : off) + 10;
}

static void DrawDetail(void)
{
    const bool gba = gUiTheme->gbaFrame;
    const uint32_t ink = gba ? UI_RGBA(49, 49, 49, 255) : UI_COL_INK;
    const uint32_t soft = gba ? UI_RGBA(110, 120, 114, 255) : UI_COL_INK_SOFT;
    const uint32_t sh = gba ? UI_GBA_SHAD_LIGHT : 0;

    if (gba)
        UiW_GbaWin(548, 60, 396, 432, UI_GBA_WIN_DEFAULT);
    else
        UiW_Panel(548, 60, 396, 432);

    struct BoxPokemon *bm = sHolding ? &sHeld.box : CursorMon();
    if (gba)
        UiW_GbaBox(564, 76, 140, 140, 4, UI_RGBA(150, 172, 156, 255),
                   UI_RGBA(240, 243, 230, 255), UI_RGBA(228, 234, 218, 255), C_WHITE, 0);
    else
        UiW_Inset(564, 76, 140, 140);

    if (bm == NULL)
    {
        if (gba)
            UiW_GbaTextWrapped(UI_FACE_VT323, UI_BODY_PX, 716, 84, 210, 30,
                               sArea == AREA_PLATE ? "✕: BOX menu (JUMP, WALLPAPER, NAME)." : "Empty.", soft, sh, 4);
        else
            UiW_TextWrapped(716, 84, 210, 30, sArea == AREA_PLATE ? "✕: BOX menu (JUMP, WALLPAPER, NAME)." : "Empty.", soft, 4);
        return;
    }
    const bool egg = GetBoxMonData(bm, MON_DATA_IS_EGG);
    const uint16_t species = (uint16_t)GetBoxMonData(bm, MON_DATA_SPECIES_OR_EGG);
    UiSpr_MonFront(species, !egg && IsShinyOtIdPersonality(GetBoxMonData(bm, MON_DATA_OT_ID), GetBoxMonData(bm, MON_DATA_PERSONALITY)), 570, 82, 2);
    char name[16], buf[64];
    MonName(bm, name, sizeof(name));
    const int32_t nw = gba ? UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 716, 76, name, ink, sh)
                           : UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 716, 76, name, ink);
    if (!egg)
    {
        UiKit_GenderShiny(722 + nw, 82, GetBoxMonGender(bm), false);
        snprintf(buf, sizeof(buf), "Lv%u", (unsigned)GetLevelFromBoxMonExp(bm));
        if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 716, 108, buf, ink, sh);
        else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 716, 108, buf, ink);
    }
    const uint16_t it = HeldItemOf(bm);
    if (gba) UiW_GbaText(UI_FACE_PS2P, 16, 716, 156, "ITEM", soft, sh);
    else     UiText_Draw(UI_FACE_PS2P, 16, 716, 156, "ITEM", soft);
    if (it != ITEM_NONE) { UiSpr_ItemIcon(it, 716, 176, 1); UiGame_ItemName(it, buf, sizeof(buf)); }
    else snprintf(buf, sizeof(buf), "NONE");
    if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, it ? 746 : 716, 172, buf, it ? ink : soft, sh);
    else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, it ? 746 : 716, 172, buf, it ? ink : soft);
    if (egg)
        return;
    const u32 pid = GetBoxMonData(bm, MON_DATA_PERSONALITY);
    if (gba)
        UiW_GbaBox(564, 228, 364, 70, 4, UI_RGBA(150, 172, 156, 255),
                   UI_RGBA(240, 243, 230, 255), UI_RGBA(228, 234, 218, 255), C_WHITE, 0);
    else
        UiW_Inset(564, 228, 364, 70);
    if (gba) UiW_GbaText(UI_FACE_PS2P, 16, 576, 236, "NATURE", soft, sh);
    else     UiText_Draw(UI_FACE_PS2P, 16, 576, 236, "NATURE", soft);
    UiGame_Text(gNatureNamePointers[GetNatureFromPersonality(pid)], buf, sizeof(buf));
    if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 576, 256, buf, ink, sh);
    else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 576, 256, buf, ink);
    if (gba) UiW_GbaText(UI_FACE_PS2P, 16, 756, 236, "MARKS", soft, sh);
    else     UiText_Draw(UI_FACE_PS2P, 16, 756, 236, "MARKS", soft);
    DrawMarks(756, 256, (uint8_t)GetBoxMonData(bm, MON_DATA_MARKINGS), gba ? UI_RGBA(216, 32, 24, 255) : UI_COL_SELECT_INK, soft);

    if (gba)
        UiW_GbaBox(564, 306, 364, 70, 4, UI_RGBA(150, 172, 156, 255),
                   UI_RGBA(240, 243, 230, 255), UI_RGBA(228, 234, 218, 255), C_WHITE, 0);
    else
        UiW_Inset(564, 306, 364, 70);
    if (gba) UiW_GbaText(UI_FACE_PS2P, 16, 576, 314, "ABILITY", soft, sh);
    else     UiText_Draw(UI_FACE_PS2P, 16, 576, 314, "ABILITY", soft);
    UiGame_Text(gAbilityNames[UiGame_SafeAbility(GetAbilityBySpecies(species, (u8)GetBoxMonData(bm, MON_DATA_ABILITY_NUM)))], buf, sizeof(buf));
    if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 576, 334, buf, ink, sh);
    else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 576, 334, buf, ink);
    if (gba) UiW_GbaText(UI_FACE_PS2P, 16, 756, 314, "OT", soft, sh);
    else     UiText_Draw(UI_FACE_PS2P, 16, 756, 314, "OT", soft);
    u8 ot[PLAYER_NAME_LENGTH + 1];
    GetBoxMonData(bm, MON_DATA_OT_NAME, ot);
    ot[PLAYER_NAME_LENGTH] = EOS;
    UiGame_Text(ot, buf, sizeof(buf));
    if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 756, 334, buf, ink, sh);
    else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 756, 334, buf, ink);

    const char *hint = sHolding ? "✕: put it down (swaps with the one there)." :
                       sMode == MODE_ITEMS ? (sHeldItem ? "✕ on a POKéMON: give or swap the item." : "✕: item options.") :
                       sQuick ? "Quick grab: ✕ picks it up at once." : "✕: options. SELECT: quick grab.";
    if (gba)
    {
        UiW_GbaBox(564, 386, 364, 92, 4, UI_RGBA(150, 172, 156, 255),
                   UI_RGBA(240, 243, 230, 255), UI_RGBA(228, 234, 218, 255), C_WHITE, 0);
        UiW_GbaTextWrapped(UI_FACE_VT323, 26, 576, 394, 340, 26, hint, ink, sh, 3);
    }
    else
        UiW_TextWrapped(564, 392, 364, 30, hint, soft, 3);
}

static void Build(void)
{
    static const char *const tabs[4][1] = { { "WITHDRAW" }, { "DEPOSIT" }, { "MOVE POKéMON" }, { "MOVE ITEMS" } };
    const bool gba = gUiTheme->gbaFrame;
    const uint32_t ink = gba ? UI_RGBA(49, 49, 49, 255) : UI_COL_INK;
    const uint32_t soft = gba ? UI_RGBA(110, 120, 114, 255) : UI_COL_INK_SOFT;
    const uint32_t sh = gba ? UI_GBA_SHAD_LIGHT : 0;

    if (gba)
    {
        UiW_GbaTopBar(UI_GBA_BG_OPTIONS, UI_SPR_BALL, tabs[sMode][0], 0, 0, false, NULL);
        int32_t rx = UI_SCREEN_W - 14;
        UiW_GbaHelpBadge(&rx, "O", "EXIT", true);
        if (sMode != MODE_ITEMS)
            UiW_GbaHelpBadge(&rx, "SELECT", sQuick ? "MENU" : "QUICK", true);
        UiW_GbaHelpBadge(&rx, "X", sHolding ? "PLACE" : "OPTIONS", false);
    }
    else
    {
        int32_t tx[1], tw[1];
        UiKit_Header("PC", tabs[sMode], 1, 0, tx, tw, 12);
        UiW_HeaderValue(sMode == MODE_ITEMS ? "ITEMS" : sQuick ? "CURSOR: QUICK GRAB" : "CURSOR: MENU", UI_COL_ON_FRAME);
    }

    // Party
    char buf[64];
    if (gba)
        UiW_GbaWin(16, 60, 180, 432, UI_GBA_WIN_DEFAULT);
    else
        UiW_Panel(16, 60, 180, 432);
    snprintf(buf, sizeof(buf), "PARTY %d/6", (int)PartyCount());
    if (gba)
        UiW_GbaSectionStrip(24, 68, 164, 24, UI_GBA_TAG_SLATE, buf, NULL);
    else
        UiText_Draw(UI_FACE_PS2P, 16, 30, 72, buf, UI_COL_INK);

    const bool partyOk = sMode != MODE_WITHDRAW;
    for (int32_t i = 0; i < PARTY_SIZE; i++)
    {
        const int32_t y = PARTY_Y + i * SLOT_H;
        const bool on = sArea == AREA_PARTY && sPartyPos == i;
        if (gba)
        {
            if (on)
                UiW_GbaBox(PARTY_X, y, 156, SLOT_H - 4, 4, UI_RGBA(230, 148, 65, 255),
                           UI_RGBA(255, 249, 224, 255), UI_RGBA(255, 242, 186, 255), C_WHITE, 1);
            else
                UiW_GbaBox(PARTY_X, y, 156, SLOT_H - 4, 4, UI_RGBA(150, 172, 156, 255),
                           UI_RGBA(248, 248, 236, 255), UI_RGBA(236, 238, 224, 255), C_WHITE, 0);
        }
        else
            UiW_Row(PARTY_X, y, 156, SLOT_H - 4, on, i);
        if (i < PartyCount())
        {
            struct Pokemon *mon = &gPlayerParty[i];
            const uint8_t f = (uint8_t)((sFrame / (on ? 8 : 16)) & 1);
            UiSpr_MonIcon((uint16_t)GetMonData(mon, MON_DATA_SPECIES_OR_EGG), f, PARTY_X + 30, y + 8 - (f ? 2 : 0), 1);
            if (sMode == MODE_ITEMS && GetMonData(mon, MON_DATA_HELD_ITEM))
                UiSpr_ItemIcon((uint16_t)GetMonData(mon, MON_DATA_HELD_ITEM), PARTY_X + 62, y + 22, 1);
            if (!GetMonData(mon, MON_DATA_IS_EGG))
            {
                snprintf(buf, sizeof(buf), "Lv%u", (unsigned)GetMonData(mon, MON_DATA_LEVEL));
                if (gba) UiW_GbaTextMid(UI_FACE_VT323, UI_BODY_PX, PARTY_X + 92, y, SLOT_H - 4, buf, on ? UI_RGBA(168, 84, 8, 255) : ink, sh);
                else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, PARTY_X + 92, y + 6, buf, on ? UI_COL_SELECT_INK : ink);
            }
        }
        else
        {
            if (gba) UiW_GbaTextMid(UI_FACE_VT323, UI_BODY_PX, PARTY_X + 40, y, SLOT_H - 4, "-----", soft, sh);
            else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, PARTY_X + 40, y + 6, "-----", soft);
        }
    }
    if (!partyOk)
        UiDraw_Rect(16, 60, 180, 432, UI_RGBA(0, 0, 0, 70));

    // Box: drop shadow behind wallpaper, the GBA wallpaper itself, the plate, the grid
    UiDraw_Rect(WALL_X + 38, WALL_Y + 6, 244, 42, UI_RGBA(0, 0, 0, 110));
    UiDraw_Rect(WALL_X + 2, WALL_Y + 56, 316, 236, UI_RGBA(0, 0, 0, 110));
    const uint8_t wall = sPop == POP_WALLS ? (uint8_t)sWallPreview : PokeStorage_GetBoxWallpaperForUi(Box());
    UiSpr_Wallpaper(wall, WALL_X, WALL_Y, 2);

    UiGame_Text(GetBoxNamePtr(Box()), buf, sizeof(buf));
    if (sArea == AREA_PLATE)
    {
        if (gba)
            UiW_GbaBox(WALL_X + 36, WALL_Y + 2, 248, 44, 5, UI_RGBA(255, 230, 57, 255),
                       UI_RGBA(255, 255, 255, 36), UI_RGBA(255, 255, 255, 16), 0, 0);
        else
            UiDraw_Frame(WALL_X + 36, WALL_Y + 2, 248, 44, 3, UI_COL_SELECT);
    }
    UiW_GbaTextCentred(UI_FACE_PS2P, 16, WALL_X + 36, WALL_Y + 2, 248, 44, buf, C_WHITE, UI_RGBA(0, 0, 0, 180));

    if (gba)
    {
        UiW_GbaBox(WALL_X, WALL_Y + 8, 30, 30, 4, UI_RGBA(24, 32, 32, 255),
                   UI_RGBA(56, 108, 138, 255), UI_RGBA(34, 72, 98, 255), UI_RGBA(128, 186, 214, 255), 2);
        UiW_GbaTextCentred(UI_FACE_PS2P, 14, WALL_X, WALL_Y + 8, 30, 30, "L", C_WHITE, UI_RGBA(16, 24, 28, 200));
        UiW_GbaBox(WALL_X + 290, WALL_Y + 8, 30, 30, 4, UI_RGBA(24, 32, 32, 255),
                   UI_RGBA(56, 108, 138, 255), UI_RGBA(34, 72, 98, 255), UI_RGBA(128, 186, 214, 255), 2);
        UiW_GbaTextCentred(UI_FACE_PS2P, 14, WALL_X + 290, WALL_Y + 8, 30, 30, "R", C_WHITE, UI_RGBA(16, 24, 28, 200));
    }
    else
    {
        UiW_Capsule(WALL_X, WALL_Y + 8, 30, 30);
        UiText_DrawCentred(UI_FACE_PS2P, 16, WALL_X, WALL_Y + 8, 30, 30, "L", UI_COL_ON_FRAME);
        UiW_Capsule(WALL_X + 290, WALL_Y + 8, 30, 30);
        UiText_DrawCentred(UI_FACE_PS2P, 16, WALL_X + 290, WALL_Y + 8, 30, 30, "R", UI_COL_ON_FRAME);
    }

    for (int32_t i = 0; i < IN_BOX_COUNT; i++)
    {
        const int32_t x = GRID_X + (i % 6) * CELL_W, y = GRID_Y + (i / 6) * CELL_H;
        const bool on = sArea == AREA_BOX && sPos == i;
        if (on)
        {
            const uint32_t selCol = sQuick ? UI_RGBA(47, 122, 92, 255) : gUiTheme->selEdge;
            UiW_GbaBox(x, y, CELL_W, CELL_H, 5, selCol,
                       UI_RGBA(255, 247, 214, 150), UI_RGBA(255, 240, 184, 150), C_WHITE, 1);
            // Downward pointer triangle (.cell.on::after)
            UiDraw_Rect(x + 17, y - 9, 14, 3, selCol);
            UiDraw_Rect(x + 19, y - 6, 10, 3, selCol);
            UiDraw_Rect(x + 21, y - 3, 6, 3, selCol);
        }
        struct BoxPokemon *bm = BoxMon(i);
        if (GetBoxMonData(bm, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        const uint8_t f = (uint8_t)(on ? (sFrame / 8) & 1 : 0);
        UiSpr_MonIcon((uint16_t)GetBoxMonData(bm, MON_DATA_SPECIES_OR_EGG), f, x + 8, y + 6 - (f ? 2 : 0), 1);
        if (sMode == MODE_ITEMS && GetBoxMonData(bm, MON_DATA_HELD_ITEM))
            UiSpr_ItemIcon((uint16_t)GetBoxMonData(bm, MON_DATA_HELD_ITEM), x + 26, y + 22, 1);
    }
    snprintf(buf, sizeof(buf), "%d/30", (int)BoxCount(Box()));
    if (gba)
    {
        UiW_GbaBox(WALL_X + 40, WALL_Y + 296, 100, 32, 5, UI_RGBA(40, 48, 48, 255),
                   UI_RGBA(248, 249, 244, 255), UI_RGBA(232, 236, 224, 255), C_WHITE, 2);
        UiW_GbaTextCentred(UI_FACE_VT323, UI_BODY_PX, WALL_X + 40, WALL_Y + 296, 100, 32, buf, ink, sh);
        UiW_GbaBox(WALL_X + 148, WALL_Y + 296, 172, 32, 5, UI_RGBA(40, 48, 48, 255),
                   UI_RGBA(248, 249, 244, 255), UI_RGBA(232, 236, 224, 255), C_WHITE, 2);
        UiW_GbaTextCentred(UI_FACE_VT323, UI_BODY_PX, WALL_X + 148, WALL_Y + 296, 172, 32, sWallNames[wall < 17 ? wall : 0], ink, sh);
    }
    else
    {
        UiW_Capsule(WALL_X + 40, WALL_Y + 296, 100, 32);
        UiText_DrawCentred(UI_FACE_VT323, UI_BODY_PX, WALL_X + 40, WALL_Y + 296, 100, 32, buf, UI_COL_ON_FRAME);
        UiW_Capsule(WALL_X + 148, WALL_Y + 296, 172, 32);
        UiText_DrawCentred(UI_FACE_VT323, UI_BODY_PX, WALL_X + 148, WALL_Y + 296, 172, 32, sWallNames[wall < 17 ? wall : 0], UI_COL_ON_FRAME);
    }

    // What the cursor holds
    if (sHolding)
    {
        const int32_t hx = sArea == AREA_PARTY ? PARTY_X + 100 : sArea == AREA_BOX ? GRID_X + (sPos % 6) * CELL_W + 20 : WALL_X + 140;
        const int32_t hy = sArea == AREA_PARTY ? PARTY_Y + sPartyPos * SLOT_H - 26 : sArea == AREA_BOX ? GRID_Y + (sPos / 6) * CELL_H - 30 : WALL_Y + 20;
        UiSpr_MonIcon((uint16_t)GetMonData(&sHeld, MON_DATA_SPECIES_OR_EGG), (uint8_t)((sFrame / 8) & 1), hx, hy, 1);
    }
    if (sHeldItem != ITEM_NONE)
    {
        const int32_t hx = sArea == AREA_PARTY ? PARTY_X + 120 : GRID_X + (sPos % 6) * CELL_W + 26;
        const int32_t hy = sArea == AREA_PARTY ? PARTY_Y + sPartyPos * SLOT_H - 16 : GRID_Y + (sPos / 6) * CELL_H - 18;
        UiSpr_ItemIcon(sHeldItem, hx, hy, 1);
    }

    DrawDetail();

    if (!gba)
    {
        int32_t fx = 16;
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sHolding ? "Put down" : "Options");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Exit");
        if (sMode != MODE_ITEMS)
            UiW_FooterKey(&fx, "SELECT", "Quick grab");
        UiKit_FooterClock();
    }

    // Popups
    if (sPop == POP_ACTIONS || sPop == POP_BOXMENU || sPop == POP_SETS || sPop == POP_WALLS || sPop == POP_RELEASE || sPop == POP_EXIT)
    {
        if (sPop == POP_RELEASE || sPop == POP_EXIT)
        {
            UiDraw_Darken(1, 3);
            if (gba)
            {
                UiW_GbaMsg(48, UI_SCREEN_H - 24 - 124 - 40, UI_SCREEN_W - 96, 90, false, false);
                UiW_GbaText(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 124 - 22,
                            sPop == POP_RELEASE ? "Release this POKéMON?" : "Continue BOX operations?", ink, sh);
            }
            else
            {
                UiW_DialogBox(48, UI_SCREEN_H - 24 - 124 - 40, UI_SCREEN_W - 96, 90);
                UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 124 - 22, sPop == POP_RELEASE ? "Release this POKéMON?" : "Continue BOX operations?", UI_COL_INK);
            }
        }
        UiMenu_Draw(&sMenu);
    }
    else if (sPop == POP_BOXLIST)
    {
        UiDraw_Darken(1, 3);
        if (gba)
        {
            UiW_GbaWin(264, 70, 432, 360, UI_GBA_WIN_DEFAULT);
            UiW_GbaSectionStrip(276, 78, 408, 28, UI_GBA_TAG_SLATE, sBoxListStore ? "DEPOSIT IN WHICH BOX?" : "JUMP TO WHICH BOX?", NULL);
        }
        else
            UiW_Window(264, 70, 432, 360, sBoxListStore ? "DEPOSIT IN WHICH BOX?" : "JUMP TO WHICH BOX?");
        for (int32_t r = 0; r < 7 && sBoxList.first + r < BOXES; r++)
        {
            const int32_t b = sBoxList.first + r, y = 120 + r * 40;
            const bool on = b == sBoxList.sel;
            UiW_Row(280, y, 400, 40, on, b);
            UiGame_Text(GetBoxNamePtr((u8)b), buf, sizeof(buf));
            if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 308, y + 2, buf, on ? UI_RGBA(168, 84, 8, 255) : ink, sh);
            else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 308, y + 2, buf, on ? UI_COL_SELECT_INK : ink);
            char n[16];
            snprintf(n, sizeof(n), "%d/30", (int)BoxCount((u8)b));
            if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 600, y + 2, n, on ? UI_RGBA(168, 84, 8, 255) : soft, sh);
            else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 600, y + 2, n, on ? UI_COL_SELECT_INK : soft);
        }
        UiW_ScrollBar(684, 120, 280, sBoxList.first, 7, BOXES);
    }
    else if (sPop == POP_GIVE)
    {
        UiDraw_Darken(1, 3);
        if (gba)
        {
            UiW_GbaWin(284, 80, 392, 310, UI_GBA_WIN_DEFAULT);
            UiW_GbaSectionStrip(296, 88, 368, 28, UI_GBA_TAG_SLATE, "GIVE WHICH ITEM?", NULL);
        }
        else
            UiW_Window(284, 80, 392, 310, "GIVE WHICH ITEM?");
        for (int32_t r = 0; r < 6 && sGiveList.first + r < sGiveCount; r++)
        {
            const int32_t i = sGiveList.first + r, y = 130 + r * 40;
            const bool on = i == sGiveList.sel;
            UiW_Row(300, y, 360, 40, on, i);
            UiSpr_ItemIcon(sGiveItems[i], 328, y + 8, 1);
            UiGame_ItemName(sGiveItems[i], buf, sizeof(buf));
            if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 360, y + 2, buf, on ? UI_RGBA(168, 84, 8, 255) : ink, sh);
            else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 360, y + 2, buf, on ? UI_COL_SELECT_INK : ink);
        }
        UiW_ScrollBar(664, 130, 240, sGiveList.first, 6, sGiveCount);
    }
    else if (sPop == POP_MARKS)
    {
        static const char *const labels[6] = { "o  CIRCLE", "^  TRIANGLE", "#  SQUARE", "<3 HEART", "OK", "CANCEL" };
        UiDraw_Darken(1, 3);
        if (gba)
        {
            UiW_GbaWin(344, 100, 272, 330, UI_GBA_WIN_DEFAULT);
            UiW_GbaSectionStrip(356, 108, 248, 28, UI_GBA_TAG_SLATE, "MARK", NULL);
        }
        else
            UiW_Window(344, 100, 272, 330, "MARK");
        for (int32_t i = 0; i < 6; i++)
        {
            const int32_t y = 150 + i * 44;
            const bool on = i == sMarkSel;
            UiW_Row(360, y, 240, 42, on, i);
            if (gba) UiW_GbaText(UI_FACE_VT323, UI_BODY_PX, 388, y + 4, labels[i], on ? UI_RGBA(168, 84, 8, 255) : ink, sh);
            else     UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 388, y + 4, labels[i], on ? UI_COL_SELECT_INK : ink);
            if (i < 4 && ((sMarks >> i) & 1))
                UiText_Draw(UI_FACE_PS2P, 16, 568, y + 12, "*", on ? UI_COL_SELECT_INK : gUiTheme->selEdge);
        }
    }
    else if (sPop == POP_MSG)
    {
        UiDraw_Darken(1, 3);
        if (gba)
        {
            UiW_GbaMsg(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124, false, true);
            UiW_GbaTextWrapped(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 112, UI_SCREEN_W - 152, 46, sMsgs[sMsgAt], ink, sh, 2);
        }
        else
        {
            UiW_DialogBox(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124);
            UiW_TextWrappedF(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 112, UI_SCREEN_W - 152, 46, sMsgs[sMsgAt], UI_COL_INK, 2);
        }
    }
}

const UiScreen gUiPcScreen = { Enter, Update, Build, NULL };
