// Battle HUD of the new UI (docs/VITA_UI.md, design/stitch/combate). The game keeps
// the battle: its controllers read the buttons, its sprites (POKéMON, trainers, move animations,
// weather) and transitions are drawn as before. This file mirrors the GBA HUD from the game state
// on Core 1 and asks the GBA frame to leave out its own HUD: BG0 (text box and menus), BG1 while
// the level-up box shows (Ui_BattleDispcntClear, Core 0) and the healthbox / status summary
// sprites (gUiHideBattleHudSprites, sprite.c). Taps move the game's cursors and press A.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_controls.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "main.h"
#include "util.h"
#include "constants/rgb.h"
#include "menu_specialized.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "safari_zone.h"
#include "sprite.h"
#include "text.h"
#include "constants/battle.h"
#include "constants/battle_script_commands.h"
#include "constants/moves.h"
#include "constants/pokemon.h"

// Game hooks
extern u8 BattlePlayer_UiState(u8 battler);            // battle_controller_player.c
extern bool8 BattleSafari_ChoosingAction(u8 battler);  // battle_controller_safari.c
extern s32 gUiBattleBar[MAX_BATTLERS_COUNT][2];        // battle_interface.c (MoveBattleBar)
extern s32 gUiBattleBarMax[MAX_BATTLERS_COUNT][2];
extern u32 gUiBattleBarTick[MAX_BATTLERS_COUNT][2];
extern bool8 gUiBattleYesNo;                           // battle_script_commands.c
extern u8 gUiLevelUpPage;
extern bool8 gUiArenaJudging;                          // battle_arena.c
extern bool8 gUiHideBattleHudSprites;                  // sprite.c
extern const u8 *gUiPrinterStart[WINDOWS_MAX];         // text.c
extern u32 gUiPrinterSerial;
extern u32 gUiPrinterWinSerial[WINDOWS_MAX];
extern const struct TextPrinter *GetTextPrinterForUi(u8 windowId);
extern void UiMirror_VisibleLines(const u8 *start, const u8 *end, char out[2][160]);

enum { ST_NONE, ST_ACTION, ST_MOVE, ST_TARGET, ST_REORDER, ST_SAFARI };

#define BOX_Y 381                      // top of the bottom box (GBA y 112 at x3.4)
#define FOE_W 300
#define FOE_H 68
#define OWN_W 300
#define OWN_H 100
#define CMP_W 270                      // doubles, player's side
#define CMP_H 66

// Colours: the menu theme's (gUiTheme, ui_theme.c); these few are the same in every theme
#define C_WHITE       UI_RGBA(255, 255, 255, 255)
#define TH            gUiTheme
#define PILL          (TH->selRadius > 0)

static bool sActive;
static uint16_t sDispClear;            // Core 1 writes, Core 0 reads
static bool sWasInBattle;
static u32 sBattleSerial;              // gUiPrinterSerial when the battle began
static uint32_t sFrame;
// Party status balls strip (PartyBalls)
static float sBallsT[2];               // 0 hidden .. 1 in place
static bool sBallsSettled[2], sBallsLeaving[2];
static int32_t sBallsPrevAbs[2];
static int8_t sBallsState[2][PARTY_SIZE];

uint16_t Ui_BattleDispcntClear(void)
{
    return __atomic_load_n(&sDispClear, __ATOMIC_RELAXED);
}

bool UiBattle_Active(void)
{
    return sActive;
}

// Every tick (Core 1, before the screens): whether the HUD replaces the GBA one
void UiBattle_Tick(void)
{
    const bool inBattle = gMain.inBattle && gMain.callback2 == BattleMainCB2;
    if (inBattle && !sWasInBattle)
    {
        sBattleSerial = gUiPrinterSerial;
        memset(gUiBattleBarTick, 0, sizeof(gUiBattleBarTick));
        gUiBattleYesNo = FALSE;                    // the evolution scene uses the same box
        gUiLevelUpPage = 0;
        memset(sBallsT, 0, sizeof(sBallsT));
        memset(sBallsSettled, 0, sizeof(sBallsSettled));
        memset(sBallsLeaving, 0, sizeof(sBallsLeaving));
    }
    sWasInBattle = inBattle;
    // Link and recorded battles (VS windows, standby texts), WALLY's catching tutorial (its own
    // controller moves the GBA cursor) and the Battle Arena judgment window keep the GBA HUD
    sActive = Ui_IsEnabled() && inBattle
           && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED | BATTLE_TYPE_WALLY_TUTORIAL))
           && !gUiArenaJudging;
    gUiHideBattleHudSprites = sActive;
    const uint16_t clear = sActive ? (uint16_t)(DISPCNT_BG0_ON | (gUiLevelUpPage ? DISPCNT_BG1_ON : 0)) : 0;
    __atomic_store_n(&sDispClear, clear, __ATOMIC_RELAXED);
}

// ---------------------------------------------------------------- game state

static bool HealthboxVisible(u8 b)
{
    if (b >= gBattlersCount || (gAbsentBattlerFlags & gBitTable[b]))
        return false;
    const u8 id = gHealthboxSpriteIds[b];
    return id < MAX_SPRITES && gSprites[id].inUse && !gSprites[id].invisible;
}

static bool BarFresh(u8 b, int which)
{
    const u32 t = gUiBattleBarTick[b][which];
    return t != 0 && gMain.vblankCounter1 - t < 60;
}

static int32_t ShownHp(u8 b)
{
    return BarFresh(b, HEALTH_BAR) ? gUiBattleBar[b][HEALTH_BAR] : gBattleMons[b].hp;
}

// EXP into the level, 0..100
static int32_t ExpPercent(u8 b)
{
    if (BarFresh(b, EXP_BAR) && gUiBattleBarMax[b][EXP_BAR] > 0)
        return gUiBattleBar[b][EXP_BAR] * 100 / gUiBattleBarMax[b][EXP_BAR];
    struct Pokemon *mon = &gPlayerParty[gBattlerPartyIndexes[b]];
    const u16 species = GetMonData(mon, MON_DATA_SPECIES);
    const u8 level = GetMonData(mon, MON_DATA_LEVEL);
    if (species == SPECIES_NONE || level >= MAX_LEVEL)
        return 0;
    const u8 rate = gSpeciesInfo[species].growthRate;
    const u32 lo = gExperienceTables[rate][level], hi = gExperienceTables[rate][level + 1];
    const u32 exp = GetMonData(mon, MON_DATA_EXP);
    return hi > lo ? (int32_t)((exp - lo) * 100 / (hi - lo)) : 0;
}

static void HpColours(int32_t pct, uint32_t *top, uint32_t *bottom)
{
    if (pct > 50) { *top = UI_RGBA(110, 231, 183, 255); *bottom = UI_RGBA(52, 211, 153, 255); }
    else if (pct > 20) { *top = UI_RGBA(253, 230, 138, 255); *bottom = UI_RGBA(254, 214, 91, 255); }
    else { *top = UI_RGBA(245, 156, 156, 255); *bottom = UI_RGBA(232, 82, 82, 255); }
}

static int StatusIndex(u32 status1)
{
    if (status1 & STATUS1_SLEEP) return 2;
    if (status1 & STATUS1_PSN_ANY) return 0;
    if (status1 & STATUS1_BURN) return 4;
    if (status1 & STATUS1_FREEZE) return 3;
    if (status1 & STATUS1_PARALYSIS) return 1;
    return -1;
}

// ---------------------------------------------------------------- plates

static void Bar(int32_t x, int32_t y, int32_t w, int32_t h, int32_t pct, uint32_t top, uint32_t bottom)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    UiDraw_Rect(x, y, w, h, TH->gauge);
    int32_t fill = (w - 4) * pct / 100;
    if (pct > 0 && fill < 1) fill = 1;
    UiDraw_RectV(x + 2, y + 2, fill, h - 4, top, bottom);
}

static void PlateBox(int32_t x, int32_t y, int32_t w, int32_t h, bool lit)
{
    UiW_ThemeBox(x, y, w, h, 3, TH->plateEdge, TH->plateTop, TH->plateBot, C_WHITE, TH->plateShade, 4, false,
                 lit ? TH->gold : TH->plateRing);
}

// mine: the player's side (HP numbers, EXP). compact: doubles, player's side.
static void Plate(u8 b, int32_t x, int32_t y, bool mine, bool compact, bool turn, bool target)
{
    const int32_t w = compact ? CMP_W : mine ? OWN_W : FOE_W;
    const int32_t h = compact ? CMP_H : mine ? OWN_H : FOE_H;
    PlateBox(x, y, w, h, turn || target);
    if (turn)
        UiW_Arrow(x - 22, y + 24, TH->gold);
    char name[24], buf[24];
    UiGame_Text(gBattleMons[b].nickname, name, sizeof(name));
    const u16 species = gBattleMons[b].species;
    // Name in the body face (as tall as the level) for legibility; the small title face only when a
    // long nickname would run into the level
    const int32_t nameRoom = w - 12 - 12 - 100;
    int32_t cx = x + 12 + (UiText_Width(UI_FACE_VT323, UI_BODY_PX, name) <= nameRoom
                           ? UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y + 1, name, UI_COL_INK)
                           : UiText_Draw(UI_FACE_PS2P, 16, x + 12, y + 10, name, UI_COL_INK));
    const u8 gender = GetGenderFromSpeciesAndPersonality(species, gBattleMons[b].personality);
    // Gender at x1 (14 px), centred on the 16 px name
    if (gender == MON_MALE) { UiDraw_Sprite(UI_SPR_GENDER_MALE, cx + 6, y + 11, 1); cx += 22; }
    else if (gender == MON_FEMALE) { UiDraw_Sprite(UI_SPR_GENDER_FEMALE, cx + 6, y + 11, 1); cx += 22; }
    // Wild foe already caught: the healthbox ball
    if (!mine && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        UiDraw_Sprite(UI_SPR_BATTLE_CAUGHT, cx + 4, y + 10, 2);
    snprintf(buf, sizeof(buf), "%u", (unsigned)gBattleMons[b].level);
    const int32_t lvW = UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 12 - lvW, y + 1, buf, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 14 - lvW - UiText_Width(UI_FACE_VT323, UI_BODY_PX, "Lv"), y + 1, "Lv", TH->lvInk);

    if (mine && (gBattleTypeFlags & BATTLE_TYPE_SAFARI))
    {
        snprintf(buf, sizeof(buf), "SAFARI BALLS  %u", (unsigned)gNumSafariBalls);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y + 40, buf, UI_COL_INK);
        return;
    }
    const int32_t max = gBattleMons[b].maxHP ? gBattleMons[b].maxHP : 1;
    const int32_t hp = ShownHp(b), pct = hp * 100 / max;
    uint32_t top, bottom;
    HpColours(pct, &top, &bottom);
    const int st = StatusIndex(gBattleMons[b].status1);
    const int32_t rowY = y + 34;
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, rowY - 8, "HP", TH->hpInk);
    if (compact)
    {
        Bar(x + 46, rowY + 2, 120, 12, pct, top, bottom);
        snprintf(buf, sizeof(buf), "%d/%d", (int)hp, (int)max);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 12 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), rowY - 8, buf, UI_COL_INK);
        if (st >= 0)
            UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), x + 46 + 120 + 6, rowY - 2, 1);
        Bar(x + 12, y + h - 10, w - 24, 6, ExpPercent(b), UI_RGBA(124, 192, 242, 255), UI_RGBA(47, 125, 194, 255));
        return;
    }
    Bar(x + 46, rowY + 2, 168, 12, pct, top, bottom);
    if (st >= 0)
        UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), x + w - 12 - 64, rowY - 2, 2);
    if (mine)
    {
        snprintf(buf, sizeof(buf), "%d/%d", (int)hp, (int)max);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 12 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 50, buf, UI_COL_INK);
        Bar(x + 12, y + h - 14, w - 24, 8, ExpPercent(b), UI_RGBA(124, 192, 242, 255), UI_RGBA(47, 125, 194, 255));
    }
}

// ---------------------------------------------------------------- party status balls

// The GBA's status summary balls (trainer battles: start and when the foe sends out the next one).
// Ball states come from its own sprites (left out of the frame by sprite.c); the strip slides on
// its own so it does not follow the balls' staggered entry and bounce.

static void PartyBalls(int32_t xPlayer, int32_t yPlayer, int32_t xFoe, int32_t yFoe)
{
    const u16 start = GetSpriteTileStartByTag(TAG_STATUS_SUMMARY_BALLS_TILE);
    int8_t state[2][PARTY_SIZE];
    int16_t px[2][PARTY_SIZE];
    int32_t n[2] = { 0, 0 }, absSum[2] = { 0, 0 };
    if (start != 0xFFFF)
        for (int32_t i = 0; i < MAX_SPRITES; i++)
        {
            const struct Sprite *s = &gSprites[i];
            if (!s->inUse || s->invisible || !s->usingSheet || s->sheetTileStart != start)
                continue;
            const int side = s->data[2] ? 1 : 0;
            if (n[side] >= PARTY_SIZE)
                continue;
            state[side][n[side]] = (int8_t)((s->oam.tileNum - start) & 3);
            px[side][n[side]] = s->x;
            absSum[side] += s->x2 < 0 ? -s->x2 : s->x2;
            n[side]++;
        }
    for (int side = 0; side < 2; side++)
    {
        float target = 0.0f;
        if (n[side] > 0)
        {
            // left to right by the GBA x
            for (int32_t a = 0; a < n[side]; a++)
                for (int32_t c = a + 1; c < n[side]; c++)
                    if (px[side][c] < px[side][a])
                    {
                        const int16_t tx = px[side][a]; px[side][a] = px[side][c]; px[side][c] = tx;
                        const int8_t ts = state[side][a]; state[side][a] = state[side][c]; state[side][c] = ts;
                    }
            for (int32_t i = 0; i < PARTY_SIZE; i++)
                sBallsState[side][i] = i < n[side] ? state[side][i] : 1;   // missing slot: empty
            const int32_t absAvg = absSum[side] / n[side];
            if (absAvg <= 8)
                sBallsSettled[side] = true;
            // Settled balls moving away again: the GBA is hiding them
            if (sBallsSettled[side] && absAvg > sBallsPrevAbs[side] && absAvg > 16)
                sBallsLeaving[side] = true;
            sBallsPrevAbs[side] = absAvg;
            target = sBallsLeaving[side] ? 0.0f : 1.0f;
        }
        else
        {
            sBallsSettled[side] = sBallsLeaving[side] = false;
            sBallsPrevAbs[side] = 0;
        }
        const float step = 1.0f / 14.0f;
        if (sBallsT[side] < target) { sBallsT[side] += step; if (sBallsT[side] > target) sBallsT[side] = target; }
        else if (sBallsT[side] > target) { sBallsT[side] -= step; if (sBallsT[side] < target) sBallsT[side] = target; }
        if (sBallsT[side] <= 0.0f)
            continue;
        const float t = sBallsT[side], e = 1.0f - (1.0f - t) * (1.0f - t);   // ease out
        const int32_t w = 6 * 22 + 16;
        const int32_t away = (int32_t)((1.0f - e) * (float)(w + 40));
        const int32_t x = side ? xFoe - away : xPlayer + away, y = side ? yFoe : yPlayer;
        UiW_ThemeBox(x, y, w, 30, 2, TH->darkEdge, TH->darkTop, TH->darkBot, TH->darkHi, TH->darkBot, 3, PILL, 0);
        for (int32_t i = 0; i < PARTY_SIZE; i++)
            UiDraw_Sprite((uint16_t)(UI_SPR_BATTLE_BALL_FIRST + sBallsState[side][i]), x + 10 + i * 22, y + 7, 2);
    }
}

// ---------------------------------------------------------------- bottom box

static void CommandButton(int32_t x, int32_t y, int32_t w, int32_t h, const char *label, int32_t i, bool on)
{
    UiW_ThemeBox(x, y, w, h, 3, TH->cmdEdge, TH->cmdTop[i], TH->cmdBot[i], UI_RGBA(255, 255, 255, 102), UI_RGBA(0, 0, 0, 89),
                 3, PILL, on ? TH->gold : 0);
    if (on)
        UiW_Arrow(x + (PILL ? 18 : 10), y + h / 2 - 6, TH->cmdInk == C_WHITE ? TH->gold : TH->cmdInk);
    UiText_DrawMid(UI_FACE_PS2P, 16, x + (PILL ? 38 : 30), y, h, label, TH->cmdInk);
}

static bool Hit(int32_t tx, int32_t ty, int32_t x, int32_t y, int32_t w, int32_t h)
{
    return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

static void Commands(u8 b, bool safari, bool tap, int32_t tx, int32_t ty)
{
    static const char *const kLabels[2][4] = { { "FIGHT", "BAG", "POKéMON", "RUN" }, { "BALL", "POKéBLOCK", "GO NEAR", "RUN" } };
    const int32_t x0 = UI_SCREEN_W - 16 - 400, y0 = BOX_Y + 18, cw = 195, ch = 56;
    const u8 cursor = gActionSelectionCursor[b] & 3;
    for (int32_t i = 0; i < 4; i++)
    {
        const int32_t x = x0 + (i & 1) * (cw + 10), y = y0 + (i >> 1) * (ch + 10);
        CommandButton(x, y, cw, ch, kLabels[safari ? 1 : 0][i], i, i == cursor);
        if (tap && Hit(tx, ty, x, y, cw, ch))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gActionSelectionCursor[b] = (u8)i;
        }
    }
}

static void PromptText(u8 b, bool safari)
{
    char name[24];
    if (safari) UiGame_PlayerName(name, sizeof(name));
    else UiGame_Text(gBattleMons[b].nickname, name, sizeof(name));
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 22, "What will", UI_COL_INK);
    const int32_t w = UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 72, name, TH->em);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28 + w, BOX_Y + 72, " do?", UI_COL_INK);
}

static void Moves(u8 b, int st, bool tap, int32_t tx, int32_t ty)
{
    const struct ChooseMoveStruct *mi = (const struct ChooseMoveStruct *)(&gBattleBufferA[b][4]);
    const int32_t x0 = 16, y0 = BOX_Y + 18, cw = 295, ch = 56;
    const int cursor = st == ST_REORDER ? gMultiUsePlayerCursor : gMoveSelectionCursor[b];
    const int from = st == ST_REORDER ? gMoveSelectionCursor[b] : -1;
    char name[24];
    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
    {
        const int32_t x = x0 + (i & 1) * (cw + 10), y = y0 + (i >> 1) * (ch + 10);
        const bool on = i == cursor && st != ST_TARGET ? true : i == cursor;
        if (on)
            UiW_ThemeBox(x, y, cw, ch, 2, TH->selEdge, TH->selTop, TH->selBot, UI_RGBA(255, 255, 255, 150), UI_RGBA(0, 0, 0, 40), 0,
                         PILL, i == from ? TH->gold : 0);
        else
            UiW_ThemeBox(x, y, cw, ch, 2, TH->dlgEdge, TH->dlgTop, TH->dlgBot, C_WHITE, TH->dlgShade, 0, PILL,
                         i == from ? TH->gold : 0);
        if (on)
            UiW_Arrow(x + (PILL ? 16 : 10), y + ch / 2 - 6, TH->selInk);
        if (mi->moves[i] != MOVE_NONE)
            UiGame_MoveName(mi->moves[i], name, sizeof(name));
        else
            snprintf(name, sizeof(name), "-");
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + (PILL ? 36 : 30), y + 6, name, on ? TH->selInk : UI_COL_INK);
        if (tap && st == ST_MOVE && mi->moves[i] != MOVE_NONE && Hit(tx, ty, x, y, cw, ch))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gMoveSelectionCursor[b] = (u8)i;
        }
    }
    // PP and TYPE of the selected move (GBA B_WIN_PP / B_WIN_MOVE_TYPE); the target in doubles
    const int32_t px = UI_SCREEN_W - 16 - 312, py = BOX_Y + 18;
    UiW_ThemeBox(px, py, 312, 131, 2, TH->darkEdge, TH->darkTop, TH->darkBot, TH->darkHi, TH->darkBot, 0, false, 0);
    const int sel = cursor & 3;
    const u16 move = mi->moves[sel];
    if (st == ST_TARGET)
    {
        UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 14, "TARGET", UI_COL_ON_SOFT);
        const u8 t = gMultiUsePlayerCursor;
        if (t < gBattlersCount)
        {
            UiGame_Text(gBattleMons[t].nickname, name, sizeof(name));
            UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, px + 16, py + 40, name, TH->gold);
            UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 98, GetBattlerSide(t) == B_SIDE_PLAYER ? (t == b ? "SELF" : "ALLY") : "FOE", UI_COL_TITLE);
        }
        return;
    }
    if (move == MOVE_NONE)
        return;
    char buf[16];
    UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 18, "PP", UI_COL_ON_SOFT);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)mi->currentPp[sel], (unsigned)mi->maxPp[sel]);
    const u8 pp = mi->currentPp[sel], maxPp = mi->maxPp[sel] ? mi->maxPp[sel] : 1;
    const uint32_t ppc = pp == 0 ? UI_RGBA(240, 122, 110, 255) : pp * 4 <= maxPp ? UI_RGBA(245, 158, 43, 255)
                       : pp * 2 <= maxPp ? UI_RGBA(245, 209, 98, 255) : UI_COL_ON_FRAME;
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, px + 312 - 16 - UiText_Width(UI_FACE_VT323, UI_DIALOG_PX, buf), py + 4, buf, ppc);
    UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 66, "TYPE", UI_COL_ON_SOFT);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gBattleMoves[move].type), px + 312 - 16 - 64, py + 58, 2);
    if (st == ST_REORDER)
        UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 100, "MOVE WHERE?", UI_COL_TITLE);
}

static void Message(bool tap, int32_t ty)
{
    if (gUiPrinterWinSerial[0] <= sBattleSerial)
        return;                                    // nothing printed in this battle yet
    const struct TextPrinter *tp = GetTextPrinterForUi(B_WIN_MSG);
    const u8 *start = gUiPrinterStart[B_WIN_MSG];
    const u8 *end = tp->active ? tp->printerTemplate.currentChar : NULL;
    if (end != NULL && (start == NULL || end < start || end - start > 1024))
        end = NULL;
    char lines[2][160];
    UiMirror_VisibleLines(start, end, lines);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 22, lines[0], UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 72, lines[1], UI_COL_INK);
    const bool waiting = tp->active && tp->state == RENDER_STATE_WAIT;
    if (waiting && ((sFrame / 16) & 1))
        UiDraw_Tri(UI_SCREEN_W - 44, UI_SCREEN_H - 34, UI_SCREEN_W - 28, UI_SCREEN_H - 34, UI_SCREEN_W - 36, UI_SCREEN_H - 24, TH->em);
    if (waiting && tap && ty >= BOX_Y)
        UiMirror_Inject(A_BUTTON);
}

static void YesNo(bool tap, int32_t tx, int32_t ty)
{
    const int32_t w = 180, x = UI_SCREEN_W - 16 - w, y = BOX_Y - 12 - 100;
    const int cursor = gBattleCommunication[CURSOR_POSITION] ? 1 : 0;
    UiW_DialogBox(x, y, w, 100);
    static const char *const opt[2] = { "YES", "NO" };
    for (int32_t i = 0; i < 2; i++)
    {
        UiW_Row(x + 8, y + 8 + i * 42, w - 16, 42, cursor == i, i);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 36, y + 12 + i * 42, opt[i], cursor == i ? UI_COL_SELECT_INK : UI_COL_INK);
        if (tap && Hit(tx, ty, x + 8, y + 8 + i * 42, w - 16, 42))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gBattleCommunication[CURSOR_POSITION] = (u8)i;
        }
    }
}

// Level-up pop-up (GBA B_WIN_LEVEL_UP_BOX): above the player's plates, page 1 the gains, page 2
// the new values
static void LevelUp(int32_t platesTop)
{
    static const char *const kNames[NUM_STATS] = { "HP", "ATK", "DEF", "SPE", "SP.A", "SP.D" };
    static const u8 kOrder[NUM_STATS] = { STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };
    u16 curr[NUM_STATS];
    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], curr);
    const u16 *before = gBattleResources->beforeLvlUp->stats;
    const int32_t w = 300, h = 140, x = UI_SCREEN_W - 16 - w, y = platesTop - 14 - h;
    UiW_ThemeBox(x, y, w, h, 3, TH->darkEdge, TH->darkTop, TH->darkBot, TH->darkHi, TH->darkBot, 4, false, TH->plateRing);
    UiText_Draw(UI_FACE_PS2P, 16, x + 14, y + 12, gUiLevelUpPage == 2 ? "NEW STATS" : "LEVEL UP!", UI_COL_TITLE);
    char buf[12];
    for (int32_t k = 0; k < NUM_STATS; k++)
    {
        const u8 s = kOrder[k];
        const int32_t cx = x + 14 + (k & 1) * 142, cy = y + 36 + (k >> 1) * 32;
        UiDraw_Rect(cx, cy + 31, 128, 1, TH->darkHi);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx, cy, kNames[s], UI_COL_ON_SOFT);
        if (gUiLevelUpPage == 2)
            snprintf(buf, sizeof(buf), "%u", (unsigned)curr[s]);
        else
            snprintf(buf, sizeof(buf), "+%d", (int)curr[s] - (int)before[s]);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx + 128 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), cy, buf,
                    gUiLevelUpPage == 2 ? UI_COL_ON_FRAME : UI_RGBA(110, 231, 183, 255));
    }
}

// ---------------------------------------------------------------- frame

bool UiBattle_Draw(void)
{
    if (!sActive || Ui_IsOpen())
        return false;
    sFrame++;
    int32_t tx = 0, ty = 0;
    const bool tap = UiMirror_Tap(&tx, &ty);
    const bool dbl = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0;
    const bool safari = (gBattleTypeFlags & BATTLE_TYPE_SAFARI) != 0;

    // Who is choosing (player's side)
    u8 who = 0xFF;
    int st = ST_NONE;
    for (u8 pos = B_POSITION_PLAYER_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
    {
        const u8 b = GetBattlerAtPosition(pos);
        if (b >= gBattlersCount)
            continue;
        if (safari && BattleSafari_ChoosingAction(b)) { who = b; st = ST_SAFARI; break; }
        const u8 s = BattlePlayer_UiState(b);
        if (s != 0) { who = b; st = s; break; }
    }

    // Foes' plates, top left
    const u8 foes[2] = { GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT) };
    for (int32_t i = 0; i < (dbl ? 2 : 1); i++)
        if (HealthboxVisible(foes[i]))
            Plate(foes[i], 16, 16 + i * 84, false, false, false, st == ST_TARGET && gMultiUsePlayerCursor == foes[i]);
    // Own plates, right above the bottom box
    const u8 mine[2] = { GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) };
    int32_t ownTop = BOX_Y - 12 - OWN_H;
    if (dbl)
    {
        ownTop = BOX_Y - 10 - 2 * (CMP_H + 8);
        for (int32_t i = 0; i < 2; i++)
            if (HealthboxVisible(mine[i]))
                Plate(mine[i], UI_SCREEN_W - 16 - CMP_W, ownTop + i * (CMP_H + 8), true, true,
                      st != ST_NONE && who == mine[i], st == ST_TARGET && gMultiUsePlayerCursor == mine[i]);
    }
    else if (HealthboxVisible(mine[0]))
        Plate(mine[0], UI_SCREEN_W - 16 - OWN_W, ownTop, true, false, false, false);
    PartyBalls(UI_SCREEN_W - 16 - (6 * 22 + 16), ownTop - 44, 16, dbl ? 16 + 84 + FOE_H + 12 : 16 + FOE_H + 12);

    // Bottom box: full width, flush with the bottom and the sides (GBA y 112..160)
    UiW_ThemeBox(-6, BOX_Y, UI_SCREEN_W + 12, UI_SCREEN_H - BOX_Y + 8, 3, TH->dlgEdge, TH->dlgTop, TH->dlgBot, C_WHITE,
                 TH->dlgShade, 0, false, 0);
    if (TH->accent)
        UiDraw_Rect(0, BOX_Y + 3 + TH->radius, 6, UI_SCREEN_H - BOX_Y - 3 - TH->radius, TH->accent);
    if (st == ST_ACTION || st == ST_SAFARI)
    {
        PromptText(who, st == ST_SAFARI);
        Commands(who, st == ST_SAFARI, tap, tx, ty);
    }
    else if (st == ST_MOVE || st == ST_TARGET || st == ST_REORDER)
        Moves(who, st, tap, tx, ty);
    else
    {
        Message(tap && !gUiBattleYesNo, ty);
        if (gUiBattleYesNo)
            YesNo(tap, tx, ty);
    }
    if (gUiLevelUpPage)
        LevelUp(ownTop);

    // The game's fades to black (start and end of the battle) darken the HUD with the picture
    if (gPaletteFade.blendColor == RGB_BLACK && (gPaletteFade.multipurpose1 & 0xFFFF) == 0xFFFF && gPaletteFade.y > 0)
        UiDraw_Darken(gPaletteFade.y, 16);
    return true;
}
