// Battle HUD of the new UI (docs/reference/VITA_UI.md, design/stitch/combate). The game keeps
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
#include "constants/items.h"
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
#ifndef C_WHITE
#define C_WHITE       UI_RGBA(255, 255, 255, 255)
#endif
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

static void PlateGba(u8 b, int32_t x, int32_t y, bool mine, bool compact, bool turn, bool target)
{
    const int32_t w = compact ? 290 : mine ? 340 : 316;
    const int32_t h = compact ? 60  : mine ? 86  : 64;
    const bool lit = (turn || target);
    const bool safari = mine && (gBattleTypeFlags & BATTLE_TYPE_SAFARI);

    // Diagonal asymmetric tail below the cream healthbox card
    if (!mine)
    {
        const float xsBorder[4] = { (float)(x + 4), (float)(x + 250), (float)(x + 234), (float)(x + 4) };
        const float ysBorder[4] = { (float)(y + h - 3), (float)(y + h - 3), (float)(y + h + 12), (float)(y + h + 12) };
        UiDraw_Poly4(xsBorder, ysBorder, UI_RGBA(41, 49, 49, 255));
        const float xsFill[4] = { (float)(x + 7), (float)(x + 245), (float)(x + 232), (float)(x + 7) };
        const float ysFill[4] = { (float)(y + h - 1), (float)(y + h - 1), (float)(y + h + 9), (float)(y + h + 9) };
        UiDraw_Poly4(xsFill, ysFill, UI_RGBA(72, 94, 84, 255));
    }
    else if (!safari)
    {
        const float tailH = compact ? 14.0f : 18.0f;
        const float xsBorder[4] = { (float)(x + 26), (float)(x + w), (float)(x + w), (float)(x + 14) };
        const float ysBorder[4] = { (float)(y + h - 3), (float)(y + h - 3), (float)(y + h) + tailH, (float)(y + h) + tailH };
        UiDraw_Poly4(xsBorder, ysBorder, UI_RGBA(41, 49, 49, 255));
        const float xsFill[4] = { (float)(x + 28), (float)(x + w - 3), (float)(x + w - 3), (float)(x + 18) };
        const float ysFill[4] = { (float)(y + h - 1), (float)(y + h - 1), (float)(y + h) + tailH - 2.0f, (float)(y + h) + tailH - 2.0f };
        UiDraw_Poly4(xsFill, ysFill, UI_RGBA(56, 76, 66, 255));
        if (!compact)
            UiW_GbaExpBar(x + 28, y + h, w - 30, ExpPercent(b), NULL);
    }

    if (lit)
        UiW_GbaRingBox(x, y, w, h, 5, UI_RGBA(230, 148, 65, 255), UI_RGBA(255, 230, 57, 255),
                       UI_RGBA(255, 251, 230, 255), UI_RGBA(234, 226, 186, 255), C_WHITE, 3);
    else
        UiW_Bevel(x, y, w, h, 5, UI_RGBA(41, 49, 49, 255), UI_RGBA(255, 251, 230, 255), UI_RGBA(234, 226, 186, 255),
                  2, UI_RGBA(255, 255, 255, 255), UI_RGBA(200, 192, 152, 255), 3);
    if (turn)
    {
        UiW_GbaArrowH(x - 19, y + 23, true, UI_GBA_SHAD_DARK);
        UiW_GbaArrowH(x - 20, y + 22, true, UI_RGBA(255, 230, 57, 255));
    }

    char name[24], buf[24];
    // Safari: the player has no POKéMON out (gBattleMons[b] is all zero); the game's healthbox
    // shows only "SAFARI BALLS" and the balls left (battle_interface.c)
    if (safari)
    {
        UiW_GbaText(UI_FACE_PS2P, 16, x + 12, y + 10, UI_TR("SAFARI BALLS"), UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), UI_TR("Left: %u"), (unsigned)gNumSafariBalls);
        UiW_GbaText(UI_FACE_VT323, 28, x + 12, y + 42, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        return;
    }
    UiGame_Text(gBattleMons[b].nickname, name, sizeof(name));
    const u16 species = gBattleMons[b].species;
    const int32_t nameRoom = w - 24 - 84;
    const uint8_t namePx = UiText_Width(UI_FACE_VT323, 28, name) <= nameRoom ? 28 : 24;
    int32_t cx = x + 12 + UiW_GbaText(UI_FACE_PS2P, 16, x + 12, y + 10, name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    (void)namePx;
    const u8 gender = GetGenderFromSpeciesAndPersonality(species, gBattleMons[b].personality);
    if (gender == MON_MALE) { UiDraw_Sprite(UI_SPR_GENDER_MALE, cx + 6, y + 11, 1); cx += 22; }
    else if (gender == MON_FEMALE) { UiDraw_Sprite(UI_SPR_GENDER_FEMALE, cx + 6, y + 11, 1); cx += 22; }
    if (!mine && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        && GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        UiDraw_Sprite(UI_SPR_BATTLE_CAUGHT, cx + 4, y + 10, 2);

    snprintf(buf, sizeof(buf), UI_TR("Lv%u"), (unsigned)gBattleMons[b].level);
    UiW_GbaText(UI_FACE_PS2P, 16, x + w - 12 - UiText_Width(UI_FACE_PS2P, 16, buf), y + 10, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);

    const int32_t max = gBattleMons[b].maxHP ? gBattleMons[b].maxHP : 1;
    const int32_t hp = ShownHp(b);
    const int st = StatusIndex(gBattleMons[b].status1);
    const int32_t hpY = y + 34;
    if (compact)
    {
        UiW_GbaHpBar(x + 12, hpY, 152, 16, (uint16_t)hp, (uint16_t)max);
        snprintf(buf, sizeof(buf), "%d/%d", (int)hp, (int)max);
        UiW_GbaText(UI_FACE_VT323, 24, x + w - 12 - UiText_Width(UI_FACE_VT323, 24, buf), hpY - 4, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (st >= 0)
            UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), x + 170, hpY, 1);
        return;
    }
    if (!mine && st >= 0)
    {
        UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), x + 12, hpY + 1, 1);
        UiW_GbaHpBar(x + 58, hpY, w - 70, 16, (uint16_t)hp, (uint16_t)max);
    }
    else
        UiW_GbaHpBar(x + 12, hpY, w - 24, 16, (uint16_t)hp, (uint16_t)max);

    if (mine)
    {
        if (st >= 0)
            UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), x + 12, y + 57, 2);
        snprintf(buf, sizeof(buf), "%d / %d", (int)hp, (int)max);
        UiW_GbaText(UI_FACE_PS2P, 16, x + w - 12 - UiText_Width(UI_FACE_PS2P, 16, buf), y + 58, buf, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    }
}

// mine: the player's side (HP numbers, EXP). compact: doubles, player's side.
static void Plate(u8 b, int32_t x, int32_t y, bool mine, bool compact, bool turn, bool target)
{
    if (TH->gbaFrame)
    {
        PlateGba(b, x, y, mine, compact, turn, target);
        return;
    }
    const int32_t w = compact ? CMP_W : mine ? OWN_W : FOE_W;
    const int32_t h = compact ? CMP_H : mine ? OWN_H : FOE_H;
    PlateBox(x, y, w, h, turn || target);
    if (turn)
        UiW_Arrow(x - 22, y + 24, TH->gold);
    char name[24], buf[24];
    // Safari: no POKéMON out on the player's side (gBattleMons[b] is all zero), only the balls left
    if (mine && (gBattleTypeFlags & BATTLE_TYPE_SAFARI))
    {
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y + 1, UI_TR("SAFARI BALLS"), UI_COL_INK);
        snprintf(buf, sizeof(buf), UI_TR("Left: %u"), (unsigned)gNumSafariBalls);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, y + 40, buf, UI_COL_INK);
        return;
    }
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
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + w - 14 - lvW - UiText_Width(UI_FACE_VT323, UI_BODY_PX, UI_TR("Lv")), y + 1, UI_TR("Lv"), TH->lvInk);

    const int32_t max = gBattleMons[b].maxHP ? gBattleMons[b].maxHP : 1;
    const int32_t hp = ShownHp(b), pct = hp * 100 / max;
    uint32_t top, bottom;
    HpColours(pct, &top, &bottom);
    const int st = StatusIndex(gBattleMons[b].status1);
    const int32_t rowY = y + 34;
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 12, rowY - 8, UI_TR("HP"), TH->hpInk);
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

static void CommandsGba(u8 b, int32_t by, int32_t bh, bool safari, bool tap, int32_t tx, int32_t ty)
{
    static const char *const kLabels[2][4] = { { UI_N("FIGHT"), UI_N("BAG"), UI_N("POKéMON"), UI_N("RUN") }, { UI_N("BALL"), UI_N("POKéBLOCK"), UI_N("GO NEAR"), UI_N("RUN") } };
    static const uint32_t kCols[4] = {
        UI_RGBA(230, 8, 8, 255), UI_RGBA(224, 138, 42, 255), UI_RGBA(32, 156, 8, 255), UI_RGBA(72, 104, 184, 255)
    };
    UiW_GbaWin(578, by, 376, bh, UI_GBA_WIN_LAVENDER);
    const int32_t x0 = 594, y0 = by + 15, cw = 168, ch = (bh - 38) / 2;
    const u8 cursor = gActionSelectionCursor[b] & 3;
    for (int32_t i = 0; i < 4; i++)
    {
        const int32_t bx = x0 + (i & 1) * (cw + 8);
        const int32_t bcy = y0 + (i >> 1) * (ch + 8);
        const bool on = (i == cursor);
        if (on)
            UiW_GbaRingBox(bx, bcy, cw, ch, 4, UI_RGBA(255, 90, 32, 255), UI_RGBA(255, 230, 57, 255),
                           UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
        else
            UiW_GbaBox(bx, bcy, cw, ch, 4, UI_RGBA(200, 198, 216, 255), UI_RGBA(244, 245, 250, 255), UI_RGBA(244, 245, 250, 255), C_WHITE, 0);
        UiDraw_Rect(bx + 8, bcy + (ch - 26) / 2, 6, 26, kCols[i]);
        if (on)
        {
            UiW_GbaArrowH(bx + 19, bcy + (ch - 12) / 2 + 1, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(bx + 18, bcy + (ch - 12) / 2, true, UI_RGBA(230, 8, 8, 255));
        }
        UiW_GbaTextMid(UI_FACE_VT323, 28, bx + (on ? 32 : 20), bcy, ch, UiLocale_Lookup(kLabels[safari ? 1 : 0][i]), UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (!safari && i == 1)
            UiSpr_ItemIcon(ITEM_POTION, bx + cw - 30, bcy + (ch - 24) / 2, 1);
        else if (i == 2 && !safari)
            UiDraw_Sprite(UI_SPR_BALL_POKE, bx + cw - 28, bcy + (ch - 20) / 2, 1);

        if (tap && Hit(tx, ty, bx, bcy, cw, ch))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gActionSelectionCursor[b] = (u8)i;
        }
    }
}

static void PromptTextGba(u8 b, int32_t by, int32_t bh, bool safari)
{
    UiW_GbaMsg(6, by, 566, bh, true, true);
    char name[24];
    if (safari) UiGame_PlayerName(name, sizeof(name));
    else UiGame_Text(gBattleMons[b].nickname, name, sizeof(name));
    const int32_t ty = by + (bh - 36) / 2;
    const int32_t w1 = UiW_GbaText(UI_FACE_VT323, 36, 36, ty, UI_TR("What will "), C_WHITE, UI_GBA_SHAD_DARK);
    const int32_t w2 = UiW_GbaText(UI_FACE_VT323, 36, 36 + w1, ty, name, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
    UiW_GbaText(UI_FACE_VT323, 36, 36 + w1 + w2, ty, UI_TR(" do?"), C_WHITE, UI_GBA_SHAD_DARK);
}

static void TacPill(int32_t *px, int32_t y, const char *txt, bool stab)
{
    const int32_t w = UiText_Width(UI_FACE_VT323, 24, txt) + 14;
    UiW_GbaBox(*px, y, w, 28, 3, stab ? UI_RGBA(32, 156, 8, 255) : UI_RGBA(176, 180, 196, 255),
               stab ? UI_RGBA(226, 248, 232, 255) : UI_RGBA(238, 240, 246, 255),
               stab ? UI_RGBA(226, 248, 232, 255) : UI_RGBA(238, 240, 246, 255), C_WHITE, 0);
    UiW_GbaTextCentred(UI_FACE_VT323, 24, *px, y, w, 28, txt,
                       stab ? UI_RGBA(23, 94, 71, 255) : UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    *px += w + 6;
}

static void MovesGba(u8 b, int32_t by, int32_t bh, int st, bool tap, int32_t tx, int32_t ty)
{
    const struct ChooseMoveStruct *mi = (const struct ChooseMoveStruct *)(&gBattleBufferA[b][4]);
    UiW_GbaWin(6, by, 598, bh, UI_GBA_WIN_LAVENDER);
    const int32_t x0 = 22, y0 = by + 15, cw = 279, ch = (bh - 38) / 2;
    const int cursor = st == ST_REORDER ? gMultiUsePlayerCursor : gMoveSelectionCursor[b];
    const int from = st == ST_REORDER ? gMoveSelectionCursor[b] : -1;
    char name[24], buf[32];

    for (int32_t i = 0; i < MAX_MON_MOVES; i++)
    {
        const int32_t mx = x0 + (i & 1) * (cw + 8);
        const int32_t my = y0 + (i >> 1) * (ch + 8);
        const bool on = (i == cursor);
        if (on)
            UiW_GbaRingBox(mx, my, cw, ch, 4, (i == from) ? UI_RGBA(216, 128, 16, 255) : UI_RGBA(255, 90, 32, 255),
                           UI_RGBA(255, 230, 57, 255), UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
        else
            UiW_GbaBox(mx, my, cw, ch, 4, (i == from) ? UI_RGBA(216, 128, 16, 255) : UI_RGBA(200, 198, 216, 255),
                       UI_RGBA(244, 245, 250, 255), UI_RGBA(244, 245, 250, 255), C_WHITE, 0);
        if (on)
        {
            UiW_GbaArrowH(mx + 11, my + (ch - 12) / 2 + 1, true, UI_GBA_SHAD_LIGHT);
            UiW_GbaArrowH(mx + 10, my + (ch - 12) / 2, true, UI_RGBA(230, 8, 8, 255));
        }

        const u16 move = mi->moves[i] < MOVES_COUNT ? mi->moves[i] : MOVE_NONE;
        if (move != MOVE_NONE)
            UiGame_MoveName(move, name, sizeof(name));
        else
            snprintf(name, sizeof(name), "-");
        UiW_GbaTextMid(UI_FACE_VT323, 28, mx + (on ? 24 : 12), my, ch, name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        if (move != MOVE_NONE)
            UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gBattleMoves[move].type), mx + cw - 72, my + (ch - 32) / 2, 2);

        if (tap && st == ST_MOVE && move != MOVE_NONE && Hit(tx, ty, mx, my, cw, ch))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gMoveSelectionCursor[b] = (u8)i;
        }
    }

    // Right tactical PP / Type / PWR / ACC / STAB card
    UiW_GbaWin(610, by, 344, bh, UI_GBA_WIN_LAVENDER);
    const int sel = cursor & 3;
    const u16 move = mi->moves[sel] < MOVES_COUNT ? mi->moves[sel] : MOVE_NONE;
    if (st == ST_TARGET)
    {
        UiW_GbaSectionStrip(622, by + 12, 320, 28, UI_GBA_TAG_SLATE, UI_TR("TARGET"), NULL);
        const u8 t = gMultiUsePlayerCursor;
        if (t < gBattlersCount)
        {
            UiGame_Text(gBattleMons[t].nickname, name, sizeof(name));
            UiW_GbaText(UI_FACE_VT323, 32, 622, by + 48, name, UI_RGBA(49, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            UiW_GbaText(UI_FACE_PS2P, 16, 622, by + 98,
                        GetBattlerSide(t) == B_SIDE_PLAYER ? (t == b ? UI_TR("SELF") : UI_TR("ALLY")) : UI_TR("FOE"), UI_RGBA(32, 104, 168, 255), UI_GBA_SHAD_LIGHT);
        }
        return;
    }
    if (move == MOVE_NONE)
        return;

    snprintf(buf, sizeof(buf), UI_TR("PP %u / %u"), (unsigned)mi->currentPp[sel], (unsigned)mi->maxPp[sel]);
    UiW_GbaSectionStrip(622, by + 12, 320, 28, UI_GBA_TAG_SLATE, buf, NULL);
    const u8 mType = gBattleMoves[move].type;
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + mType), 622 + 320 - 38, by + 18, 1);

    int32_t px = 622;
    const int32_t py = by + 50;
    const u8 pwr = gBattleMoves[move].power, acc = gBattleMoves[move].accuracy;
    if (pwr > 1) snprintf(buf, sizeof(buf), UI_TR("PWR %u"), (unsigned)pwr); else snprintf(buf, sizeof(buf), UI_TR("PWR -"));
    TacPill(&px, py, buf, false);
    if (acc) snprintf(buf, sizeof(buf), UI_TR("ACC %u"), (unsigned)acc); else snprintf(buf, sizeof(buf), UI_TR("ACC -"));
    TacPill(&px, py, buf, false);
    const char *cat = pwr == 0 ? UI_TR("STAT") : IS_MOVE_PHYSICAL(move, mType) ? UI_TR("PHYS") : UI_TR("SPEC");
    TacPill(&px, py, cat, false);
    if (pwr > 0 && IS_BATTLER_OF_TYPE(b, mType))
        TacPill(&px, py, UI_TR("1.5x STAB"), true);

    const int32_t botY = by + bh - 40;
    const char *note = st == ST_REORDER ? UI_TR("MOVE WHERE?") : pwr == 0 ? UI_TR("STATUS MOVE") : UI_TR("READY TO USE");
    UiW_GbaTextMid(UI_FACE_VT323, 24, 622, botY, 26, note, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    int32_t rx = 942;
    const int32_t backRight = rx;
    UiW_GbaHelpBadgeY(&rx, botY + 1, "O", UI_TR("BACK"), true);
    if (tap && Hit(tx, ty, rx, botY - 2, backRight - rx, 32))
        UiMirror_Inject(B_BUTTON);
}

static void LevelUpGba(int32_t platesTop)
{
    static const char *const kNames[NUM_STATS] = { UI_N("HP"), UI_N("ATK"), UI_N("DEF"), UI_N("SPE"), UI_N("SP.A"), UI_N("SP.D") };
    static const u8 kOrder[NUM_STATS] = { STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };
    u16 curr[NUM_STATS];
    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], curr);
    const u16 *before = gBattleResources->beforeLvlUp->stats;
    const int32_t w = 316, h = 152, x = UI_SCREEN_W - 16 - w, y = platesTop - 12 - h;
    UiW_GbaWin(x, y, w, h, UI_GBA_WIN_DARK);
    UiW_GbaSectionStrip(x + 12, y + 10, w - 24, 26, UI_GBA_TAG_GREEN,
                        gUiLevelUpPage == 2 ? UI_TR("NEW STATS") : UI_TR("LEVEL UP!"), NULL);
    char buf[12];
    for (int32_t k = 0; k < NUM_STATS; k++)
    {
        const u8 s = kOrder[k];
        const int32_t cx = x + 16 + (k & 1) * 146, cy = y + 42 + (k >> 1) * 34;
        UiDraw_Rect(cx, cy + 32, 134, 1, UI_RGBA(64, 92, 82, 255));
        UiText_Draw(UI_FACE_VT323, 28, cx, cy, UiLocale_Lookup(kNames[s]), UI_RGBA(216, 248, 240, 255));
        if (gUiLevelUpPage == 2)
            snprintf(buf, sizeof(buf), "%u", (unsigned)curr[s]);
        else
            snprintf(buf, sizeof(buf), "+%d", (int)curr[s] - (int)before[s]);
        UiText_Draw(UI_FACE_VT323, 28, cx + 134 - UiText_Width(UI_FACE_VT323, 28, buf), cy, buf,
                    gUiLevelUpPage == 2 ? UI_RGBA(255, 255, 255, 255) : UI_RGBA(115, 255, 172, 255));
    }
}

static void Commands(u8 b, bool safari, bool tap, int32_t tx, int32_t ty)
{
    static const char *const kLabels[2][4] = { { UI_N("FIGHT"), UI_N("BAG"), UI_N("POKéMON"), UI_N("RUN") }, { UI_N("BALL"), UI_N("POKéBLOCK"), UI_N("GO NEAR"), UI_N("RUN") } };
    const int32_t x0 = UI_SCREEN_W - 16 - 400, y0 = BOX_Y + 18, cw = 195, ch = 56;
    const u8 cursor = gActionSelectionCursor[b] & 3;
    for (int32_t i = 0; i < 4; i++)
    {
        const int32_t x = x0 + (i & 1) * (cw + 10), y = y0 + (i >> 1) * (ch + 10);
        CommandButton(x, y, cw, ch, UiLocale_Lookup(kLabels[safari ? 1 : 0][i]), i, i == cursor);
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
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 22, UI_TR("What will"), UI_COL_INK);
    const int32_t w = UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28, BOX_Y + 72, name, TH->em);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 28 + w, BOX_Y + 72, UI_TR(" do?"), UI_COL_INK);
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
        const u16 move = mi->moves[i] < MOVES_COUNT ? mi->moves[i] : MOVE_NONE;
        if (move != MOVE_NONE)
            UiGame_MoveName(move, name, sizeof(name));
        else
            snprintf(name, sizeof(name), "-");
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, x + (PILL ? 36 : 30), y + 6, name, on ? TH->selInk : UI_COL_INK);
        if (tap && st == ST_MOVE && move != MOVE_NONE && Hit(tx, ty, x, y, cw, ch))
        {
            if (i == cursor) UiMirror_Inject(A_BUTTON);
            else gMoveSelectionCursor[b] = (u8)i;
        }
    }
    // PP and TYPE of the selected move (GBA B_WIN_PP / B_WIN_MOVE_TYPE); the target in doubles
    const int32_t px = UI_SCREEN_W - 16 - 312, py = BOX_Y + 18;
    UiW_ThemeBox(px, py, 312, 131, 2, TH->darkEdge, TH->darkTop, TH->darkBot, TH->darkHi, TH->darkBot, 0, false, 0);
    const int sel = cursor & 3;
    const u16 move = mi->moves[sel] < MOVES_COUNT ? mi->moves[sel] : MOVE_NONE;
    if (st == ST_TARGET)
    {
        UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 14, UI_TR("TARGET"), UI_COL_ON_SOFT);
        const u8 t = gMultiUsePlayerCursor;
        if (t < gBattlersCount)
        {
            UiGame_Text(gBattleMons[t].nickname, name, sizeof(name));
            UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, px + 16, py + 40, name, TH->gold);
            UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 98, GetBattlerSide(t) == B_SIDE_PLAYER ? (t == b ? UI_TR("SELF") : UI_TR("ALLY")) : UI_TR("FOE"), UI_COL_TITLE);
        }
        UiW_Capsule(px + 216, py + 92, 84, 30);
        UiText_DrawCentred(UI_FACE_PS2P, 12, px + 216, py + 92, 84, 30, UI_TR("BACK"), UI_COL_ON_FRAME);
        if (tap && Hit(tx, ty, px + 216, py + 92, 84, 30))
            UiMirror_Inject(B_BUTTON);
        return;
    }
    if (move == MOVE_NONE)
    {
        UiW_Capsule(px + 216, py + 92, 84, 30);
        UiText_DrawCentred(UI_FACE_PS2P, 12, px + 216, py + 92, 84, 30, UI_TR("BACK"), UI_COL_ON_FRAME);
        if (tap && Hit(tx, ty, px + 216, py + 92, 84, 30))
            UiMirror_Inject(B_BUTTON);
        return;
    }
    char buf[16];
    UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 18, UI_TR("PP"), UI_COL_ON_SOFT);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)mi->currentPp[sel], (unsigned)mi->maxPp[sel]);
    const u8 pp = mi->currentPp[sel], maxPp = mi->maxPp[sel] ? mi->maxPp[sel] : 1;
    const uint32_t ppc = pp == 0 ? UI_RGBA(240, 122, 110, 255) : pp * 4 <= maxPp ? UI_RGBA(245, 158, 43, 255)
                       : pp * 2 <= maxPp ? UI_RGBA(245, 209, 98, 255) : UI_COL_ON_FRAME;
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, px + 312 - 16 - UiText_Width(UI_FACE_VT323, UI_DIALOG_PX, buf), py + 4, buf, ppc);
    UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 66, UI_TR("TYPE"), UI_COL_ON_SOFT);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + gBattleMoves[move].type), px + 312 - 16 - 64, py + 54, 2);
    if (st == ST_REORDER)
        UiText_Draw(UI_FACE_PS2P, 16, px + 16, py + 100, UI_TR("MOVE WHERE?"), UI_COL_TITLE);
    else
    {
        UiW_Capsule(px + 216, py + 94, 84, 28);
        UiText_DrawCentred(UI_FACE_PS2P, 12, px + 216, py + 94, 84, 28, UI_TR("BACK"), UI_COL_ON_FRAME);
        if (tap && Hit(tx, ty, px + 216, py + 94, 84, 28))
            UiMirror_Inject(B_BUTTON);
    }
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
    static const char *const opt[2] = { UI_N("YES"), UI_N("NO") };
    for (int32_t i = 0; i < 2; i++)
    {
        UiW_Row(x + 8, y + 8 + i * 42, w - 16, 42, cursor == i, i);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 36, y + 12 + i * 42, UiLocale_Lookup(opt[i]), cursor == i ? UI_COL_SELECT_INK : UI_COL_INK);
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
    static const char *const kNames[NUM_STATS] = { UI_N("HP"), UI_N("ATK"), UI_N("DEF"), UI_N("SPE"), UI_N("SP.A"), UI_N("SP.D") };
    static const u8 kOrder[NUM_STATS] = { STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };
    u16 curr[NUM_STATS];
    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], curr);
    const u16 *before = gBattleResources->beforeLvlUp->stats;
    const int32_t w = 300, h = 140, x = UI_SCREEN_W - 16 - w, y = platesTop - 14 - h;
    UiW_ThemeBox(x, y, w, h, 3, TH->darkEdge, TH->darkTop, TH->darkBot, TH->darkHi, TH->darkBot, 4, false, TH->plateRing);
    UiText_Draw(UI_FACE_PS2P, 16, x + 14, y + 12, gUiLevelUpPage == 2 ? UI_TR("NEW STATS") : UI_TR("LEVEL UP!"), UI_COL_TITLE);
    char buf[12];
    for (int32_t k = 0; k < NUM_STATS; k++)
    {
        const u8 s = kOrder[k];
        const int32_t cx = x + 14 + (k & 1) * 142, cy = y + 36 + (k >> 1) * 32;
        UiDraw_Rect(cx, cy + 31, 128, 1, TH->darkHi);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, cx, cy, UiLocale_Lookup(kNames[s]), UI_COL_ON_SOFT);
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
        {
            const int32_t fw = TH->gbaFrame ? (dbl ? 290 : 326) : FOE_W;
            const int32_t fh = TH->gbaFrame ? 74 : FOE_H;
            Plate(foes[i], 16, 16 + i * 84, false, false, false, st == ST_TARGET && gMultiUsePlayerCursor == foes[i]);
            if (st == ST_TARGET && tap && Hit(tx, ty, 16, 16 + i * 84, fw, fh))
            {
                if (gMultiUsePlayerCursor == foes[i]) UiMirror_Inject(A_BUTTON);
                else gMultiUsePlayerCursor = foes[i];
            }
        }
    // Own plates, right above the bottom box
    const u8 mine[2] = { GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) };
    int32_t ownTop = TH->gbaFrame ? BOX_Y - 12 - 104 : BOX_Y - 12 - OWN_H;
    if (dbl)
    {
        ownTop = TH->gbaFrame ? BOX_Y - 10 - 2 * 78 : BOX_Y - 10 - 2 * (CMP_H + 8);
        for (int32_t i = 0; i < 2; i++)
            if (HealthboxVisible(mine[i]))
            {
                const int32_t mw = TH->gbaFrame ? 290 : CMP_W;
                const int32_t mh = TH->gbaFrame ? 72 : CMP_H;
                const int32_t mx = UI_SCREEN_W - 16 - mw;
                const int32_t my = ownTop + i * (TH->gbaFrame ? 78 : CMP_H + 8);
                Plate(mine[i], mx, my, true, true,
                      st != ST_NONE && who == mine[i], st == ST_TARGET && gMultiUsePlayerCursor == mine[i]);
                if (st == ST_TARGET && tap && Hit(tx, ty, mx, my, mw, mh))
                {
                    if (gMultiUsePlayerCursor == mine[i]) UiMirror_Inject(A_BUTTON);
                    else gMultiUsePlayerCursor = mine[i];
                }
            }
    }
    else if (HealthboxVisible(mine[0]))
        Plate(mine[0], UI_SCREEN_W - 16 - (TH->gbaFrame ? 340 : OWN_W), ownTop, true, false,
              st != ST_NONE && who == mine[0], false);
    PartyBalls(UI_SCREEN_W - 16 - (6 * 22 + 16), ownTop - 44, 16, dbl ? 16 + 84 + FOE_H + 12 : 16 + FOE_H + 12);

    // Bottom box: full width, flush with the bottom and the sides (GBA y 112..160)
    if (TH->gbaFrame)
    {
        UiDraw_Rect(0, BOX_Y, UI_SCREEN_W, 3, UI_RGBA(16, 21, 24, 255));
        UiDraw_Rect(0, BOX_Y + 3, UI_SCREEN_W, UI_SCREEN_H - BOX_Y - 3, UI_RGBA(28, 36, 40, 255));
        const int32_t by = BOX_Y + 8, bh = UI_SCREEN_H - by - 6;
        if (st == ST_ACTION || st == ST_SAFARI)
        {
            PromptTextGba(who, by, bh, st == ST_SAFARI);
            CommandsGba(who, by, bh, st == ST_SAFARI, tap, tx, ty);
        }
        else if (st == ST_MOVE || st == ST_TARGET || st == ST_REORDER)
            MovesGba(who, by, bh, st, tap, tx, ty);
        else
        {
            const struct TextPrinter *tp = GetTextPrinterForUi(B_WIN_MSG);
            const bool waiting = (gUiPrinterWinSerial[0] > sBattleSerial) && tp->active && tp->state == RENDER_STATE_WAIT;
            UiW_GbaMsg(6, by, UI_SCREEN_W - 12, bh, true, waiting);
            if (gUiPrinterWinSerial[0] > sBattleSerial)
            {
                const u8 *start = gUiPrinterStart[B_WIN_MSG];
                const u8 *end = tp->active ? tp->printerTemplate.currentChar : NULL;
                if (end != NULL && (start == NULL || end < start || end - start > 1024))
                    end = NULL;
                char lines[2][160];
                UiMirror_VisibleLines(start, end, lines);
                UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 34, by + 22, lines[0], UI_RGBA(255, 255, 255, 255));
                UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 34, by + 72, lines[1], UI_RGBA(255, 255, 255, 255));
                if (waiting && !gUiBattleYesNo && tap && ty >= BOX_Y)
                    UiMirror_Inject(A_BUTTON);
            }
            if (gUiBattleYesNo)
            {
                const int32_t w = 180, x = UI_SCREEN_W - 16 - w, y = BOX_Y - 12 - 108;
                const int cursor = gBattleCommunication[CURSOR_POSITION] ? 1 : 0;
                UiW_GbaWin(x, y, w, 108, UI_GBA_WIN_DEFAULT);
                static const char *const opt[2] = { UI_N("YES"), UI_N("NO") };
                for (int32_t i = 0; i < 2; i++)
                {
                    const int32_t ry = y + 12 + i * 42;
                    if (cursor == i)
                    {
                        UiDraw_Rect(x + 10, ry, w - 20, 40, UI_RGBA(255, 246, 205, 255));
                        UiDraw_Frame(x + 10, ry, w - 20, 40, 2, UI_RGBA(230, 148, 65, 255));
                        UiW_Arrow(x + 18, ry + 14, UI_RGBA(230, 8, 8, 255));
                    }
                    UiText_DrawMid(UI_FACE_VT323, 32, x + 42, ry, 40, UiLocale_Lookup(opt[i]), UI_RGBA(49, 49, 49, 255));
                    if (tap && Hit(tx, ty, x + 10, ry, w - 20, 40))
                    {
                        if (i == cursor) UiMirror_Inject(A_BUTTON);
                        else gBattleCommunication[CURSOR_POSITION] = (u8)i;
                    }
                }
            }
        }
        if (gUiLevelUpPage)
        {
            LevelUpGba(ownTop);
            if (tap && !gUiBattleYesNo)
                UiMirror_Inject(A_BUTTON);
        }
    }
    else
    {
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
        {
            LevelUp(ownTop);
            if (tap && !gUiBattleYesNo)
                UiMirror_Inject(A_BUTTON);
        }
    }

    // The game's fades to black (start and end of the battle) darken the HUD with the picture
    if (gPaletteFade.blendColor == RGB_BLACK && (gPaletteFade.multipurpose1 & 0xFFFF) == 0xFFFF && gPaletteFade.y > 0)
        UiDraw_Darken(gPaletteFade.y, 16);
    return true;
}
