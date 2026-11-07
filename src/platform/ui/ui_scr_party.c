// POKéMON of the new UI (design/stitch/equipo, equipo_movimientos, equipo_objetivo, equipo_mt).
// Party screen: cards on the left, summary on the right with the INFO / SKILLS / MOVES / CONTEST
// tabs; X opens the popup (field moves, SWITCH, ITEM), SHARE HP and SWITCH pick a second card.
// Target screen ("choose a POKéMON"), opened from the bag: USE an item, GIVE it, or teach a TM.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "item.h"
#include "mail.h"
#include "sound.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/species.h"

#define CARD_X 22
#define CARD_Y 66
#define CARD_W 308
#define CARD_H 70

static uint32_t sFrame;

// ---------- Party cards (shared) ----------
static void DrawCard(int32_t slot, int32_t y, bool on, const char *tag, bool tagOk)
{
    UiMon m;
    UiGame_Mon(slot, &m);
    const int32_t x = CARD_X;
    UiW_Row(x, y, CARD_W, CARD_H - 2, on, slot);
    const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
    const uint8_t frame = (uint8_t)((sFrame / (on ? 8 : 16)) & 1);
    UiSpr_MonIcon(m.species, frame, x + 8, y + 14 - (frame ? 3 : 0), 1);
    if (m.item != ITEM_NONE)
        UiDraw_Sprite(ItemIsMail(m.item) ? UI_SPR_HOLD_MAIL : UI_SPR_HOLD_ITEM, x + 30, y + 40, 2);
    const int32_t nw = UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 52, y, m.name, ink);
    if (!m.egg)
        UiKit_GenderShiny(x + 58 + nw, y + 6, m.gender, m.shiny);
    if (m.egg)
    {
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 52, y + 32, "EGG", on ? ink : UI_COL_INK_SOFT);
        if (tag)
            UiW_Chip(x + CARD_W - 16 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, tag) - 16, y + 38, tag, false);
        return;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "Lv%u", (unsigned)m.level);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + CARD_W - 12 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y, buf, ink);
    int32_t bx = x + 52;
    const int32_t st = UiGame_MonStatusIcon(slot);
    if (st >= 0)
    {
        UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), bx, y + 42, 2);
        bx += 70;
    }
    const int32_t pct = m.maxHp ? (int32_t)m.hp * 100 / m.maxHp : 0;
    const uint32_t top = pct > 50 ? UI_RGBA(110, 219, 151, 255) : pct > 20 ? UI_RGBA(245, 209, 98, 255) : UI_RGBA(240, 120, 104, 255);
    const uint32_t bot = pct > 50 ? UI_COL_HP_HIGH : pct > 20 ? UI_COL_HP_MID : UI_COL_HP_LOW;
    if (tag)
    {
        UiW_Bar(bx, y + 46, x + CARD_W - 116 - bx, 10, pct, top, bot);
        UiW_Chip(x + CARD_W - 12 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, tag) - 16, y + 38, tag, tagOk);
    }
    else
    {
        UiW_Bar(bx, y + 46, x + CARD_W - 112 - bx, 10, pct, top, bot);
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)m.hp, (unsigned)m.maxHp);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + CARD_W - 12 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 32, buf, ink);
    }
}

static void Hearts(int32_t x, int32_t y, int32_t n, bool jam)
{
    for (int32_t k = 0; k < 8; k++)
    {
        const bool f = k < n;
        UiDraw_Rect(x + k * 12, y, 10, 10, f ? (jam ? UI_RGBA(17, 17, 17, 255) : UI_RGBA(142, 34, 25, 255)) : UI_COL_PAPER2);
        UiDraw_Rect(x + k * 12 + 1, y + 1, 8, 8, f ? (jam ? UI_RGBA(58, 58, 58, 255) : UI_RGBA(217, 72, 59, 255)) : UI_COL_PAPER2);
    }
}

static void Label(int32_t x, int32_t y, const char *k)
{
    UiText_Draw(UI_FACE_PS2P, 16, x, y + 8, k, UI_COL_INK_SOFT);
}

// =====================================================================================
// Party screen
// =====================================================================================
enum { TAB_INFO, TAB_SKILLS, TAB_MOVES, TAB_CONTEST, TAB_COUNT };
static const char *const sTabs[TAB_COUNT] = { "INFO", "SKILLS", "MOVES", "CONTEST" };
enum { MODE_BROWSE, MODE_SWITCH, MODE_SHARE, MODE_MOVES };
enum { P_SWITCH = 1000, P_ITEM, P_CANCEL, P_GIVE, P_TAKE, P_SHIFT };   // above every MOVE_* id

static int32_t sTab, sMode, sFrom, sMoveSel, sMoveSwap = -1;
static UiList sParty;
static UiMenu sPop;
static int32_t sTabX[TAB_COUNT], sTabW[TAB_COUNT];
static uint8_t sFmSlot;
static uint16_t sFmMove;

static void PartyEnter(void)
{
    sParty.sel = sParty.first = 0;
    sTab = TAB_INFO;
    sMode = MODE_BROWSE;
    sPop.open = false;
    UiList_Reset(&sParty, UiGame_PartyCount(), 6);
}

static void PartyResume(void)
{
    UiList_Reset(&sParty, UiGame_PartyCount(), 6);
}

// Field move once the UI is closed (the move's own field effect runs in the overworld)
static void FieldMoveAfter(void)
{
    uint8_t fail = 0;
    if (!VitaGame_TryUseFieldMove(sFmSlot, sFmMove, &fail))
    {
        PlaySE(SE_FAILURE);
        UiCore_OpenScreen(&gUiPartyScreen);
        UiKit_Toast(fail == 1 ? "You need the right BADGE to use it." : "It can't be used here.");
    }
}

static void OpenPopup(void)
{
    const int32_t slot = sParty.sel;
    UiMon m;
    UiGame_Mon(slot, &m);
    UiMenu_Open(&sPop, m.name, 348, CARD_Y + (slot - sParty.first) * CARD_H, 260);
    if (UiGame_InBattleMenu())
    {
        UiMenu_Add(&sPop, "SHIFT", P_SHIFT);
        UiMenu_Add(&sPop, "SUMMARY", P_CANCEL);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    for (int32_t i = 0; i < 4; i++)
    {
        const uint16_t mv = UiGame_FieldMove(slot, i);
        if (mv != MOVE_NONE)
        {
            char name[16];
            UiGame_MoveName(mv, name, sizeof(name));
            UiMenu_Add(&sPop, name, mv);
        }
    }
    if (UiGame_PartyCount() > 1)
        UiMenu_Add(&sPop, "SWITCH", P_SWITCH);
    if (!m.egg)
        UiMenu_Add(&sPop, "ITEM", P_ITEM);
    UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
}

static void PopupChoice(int32_t id)
{
    const int32_t slot = sParty.sel;
    UiMon m;
    UiGame_Mon(slot, &m);
    if (id == P_SHIFT)
    {
        const char *why = UiGame_BattleSwitch(slot);
        if (why) { PlaySE(SE_FAILURE); UiKit_Toast("%s", why); }
        else UiNav_Close(NULL);
        return;
    }
    if (id == P_SWITCH) { sMode = MODE_SWITCH; sFrom = slot; UiKit_Toast("Move %s to where?", m.name); return; }
    if (id == P_ITEM)
    {
        UiMenu_Open(&sPop, "ITEM", 348, CARD_Y + (slot - sParty.first) * CARD_H, 260);
        UiMenu_Add(&sPop, "GIVE", P_GIVE);
        if (m.item != ITEM_NONE)
            UiMenu_Add(&sPop, "TAKE", P_TAKE);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    if (id == P_GIVE) { UiBag_SetGiveTo(slot); UiNav_Push(&gUiBagScreen); return; }
    if (id == P_TAKE)
    {
        uint16_t taken;
        char name[24];
        if (UiGame_TakeItem(slot, &taken))
        {
            UiGame_ItemName(taken, name, sizeof(name));
            UiKit_Toast("Received the %s from %s.", name, m.name);
        }
        else
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast("The BAG is full.");
        }
        return;
    }
    if (id == MOVE_SOFT_BOILED || id == MOVE_MILK_DRINK)
    {
        sMode = MODE_SHARE;
        sFrom = slot;
        UiKit_Toast("Use on which POKéMON?");
        return;
    }
    if (id == MOVE_FLY)
    {
        uint8_t fail = 0;
        if (!VitaGame_TryUseFieldMove((uint8_t)slot, MOVE_FLY, &fail))
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast(fail == 1 ? "You need the right BADGE to use it." : "You can't fly from here.");
            return;
        }
        UiFly_Set((uint8_t)slot);
        UiNav_Push(&gUiFlyScreen);
        return;
    }
    if (id > 0 && id < P_SWITCH)
    {
        sFmSlot = (uint8_t)slot;
        sFmMove = (uint16_t)id;
        UiNav_Close(FieldMoveAfter);
    }
}

static bool PartyUpdate(const UiInput *in)
{
    sFrame++;
    if (sPop.open)
    {
        const int32_t id = UiMenu_Update(&sPop, in);
        if (id >= 0 && id != P_CANCEL)
            PopupChoice(id);
        return true;
    }
    int32_t tab = UiKit_TabHit(in, sTabX, sTabW, TAB_COUNT);
    if (in->pressed & VITA_BTN_L) tab = (sTab + TAB_COUNT - 1) % TAB_COUNT;
    if (in->pressed & VITA_BTN_R) tab = (sTab + 1) % TAB_COUNT;
    if (tab >= 0 && tab != sTab)
    {
        sTab = tab;
        if (sMode == MODE_MOVES && tab < TAB_MOVES) sMode = MODE_BROWSE;
        PlaySE(SE_SELECT);
        return true;
    }

    if (sMode == MODE_MOVES)
    {
        // Moves of the selected POKéMON: up / down, square reorders, circle back to the party
        if (in->pressed & VITA_BTN_CIRCLE) { PlaySE(SE_SELECT); sMode = MODE_BROWSE; sMoveSwap = -1; return true; }
        UiMove mv;
        int32_t n = 0;
        for (int32_t i = 0; i < 4; i++) { UiGame_Move(sParty.sel, i, &mv); if (mv.move) n = i + 1; }
        const int32_t before = sMoveSel;
        if (in->repeat & VITA_BTN_UP) sMoveSel = (sMoveSel + n - 1) % n;
        if (in->repeat & VITA_BTN_DOWN) sMoveSel = (sMoveSel + 1) % n;
        for (int32_t i = 0; i < n; i++)
            if (UiHit(in, 360, 74 + i * 52, 572, 52)) sMoveSel = i;
        if (sMoveSel != before) PlaySE(SE_SELECT);
        if ((in->pressed & (VITA_BTN_SQUARE | VITA_BTN_CROSS)) && !UiGame_InBattleMenu())
        {
            PlaySE(SE_SELECT);
            if (sMoveSwap < 0) sMoveSwap = sMoveSel;
            else
            {
                if (sMoveSwap != sMoveSel) UiGame_SwapMoves(sParty.sel, sMoveSwap, sMoveSel);
                sMoveSwap = -1;
            }
        }
        return true;
    }

    if (in->pressed & VITA_BTN_CIRCLE)
    {
        PlaySE(SE_SELECT);
        if (sMode != MODE_BROWSE) { sMode = MODE_BROWSE; UiKit_ClearToast(); return true; }
        if (UiGame_InBattleMenu() && !UiGame_BattlePartyCanCancel())
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast("Choose a POKéMON to send out.");
            return true;
        }
        return false;
    }
    const int32_t a = UiList_Update(&sParty, in, CARD_X, CARD_Y, CARD_W, CARD_H, true);
    if (a < 0)
        return true;
    PlaySE(SE_SELECT);
    if (sMode == MODE_SWITCH)
    {
        if (a != sFrom) UiGame_SwapPartyMons(sFrom, a);
        sMode = MODE_BROWSE;
        UiKit_ClearToast();
    }
    else if (sMode == MODE_SHARE)
    {
        char msg[64];
        if (!UiGame_ShareHp(sFrom, a, msg, sizeof(msg))) PlaySE(SE_FAILURE);
        UiKit_Toast("%s", msg);
        sMode = MODE_BROWSE;
    }
    else if (sTab >= TAB_MOVES)
    {
        UiMon m;
        UiGame_Mon(a, &m);
        if (!m.egg) { sMode = MODE_MOVES; sMoveSel = 0; sMoveSwap = -1; }
    }
    else
        OpenPopup();
    return true;
}

static void BuildInfo(int32_t slot)
{
    UiSummary s;
    UiMon m;
    UiGame_Mon(slot, &m);
    char buf[64];
    // Boxes: sprite, identity, then one box per data row
    UiW_Inset(364, 76, 140, 140);
    UiSpr_MonFront(m.species, m.shiny, 370, 82, 2);
    UiW_Inset(512, 76, 416, 140);
    UiText_Draw(UI_FACE_PS2P, 16, 520, 84, m.name, UI_COL_INK);
    if (m.egg)
    {
        UiGame_Summary(slot, &s);
        const char *t = s.eggCycles <= 5 ? "It's making sounds inside. It's going to hatch soon!"
                      : s.eggCycles <= 10 ? "It moves around inside sometimes. It must be close to hatching."
                      : s.eggCycles <= 40 ? "What will hatch from this? It doesn't seem close to hatching."
                                          : "It looks like this EGG will take a long time to hatch.";
        UiW_TextWrapped(520, 110, 410, 30, t, UI_COL_INK, 4);
        return;
    }
    UiGame_Summary(slot, &s);
    UiKit_GenderShiny(520 + UiText_Width(UI_FACE_PS2P, 16, m.name) + 10, 80, m.gender, m.shiny);
    snprintf(buf, sizeof(buf), "No.%03u %s", (unsigned)s.dexNo, s.species);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 520, 104, buf, UI_COL_INK);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[0]), 520, 144, 2);
    if (s.types[1] != s.types[0])
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[1]), 592, 144, 2);
    static const char *const keys[6] = { "OT / ID", "NATURE", "ABILITY", "", "MET", "ITEM" };
    for (int32_t r = 0; r < 6; r++)
    {
        const int32_t y = 226 + r * 32;
        if (r != 3)
            UiW_Inset(360, y - 1, 572, 31);
        if (keys[r][0]) Label(372, y, keys[r]);
        buf[0] = 0;
        if (r == 0) snprintf(buf, sizeof(buf), "%s / %05u", s.ot, (unsigned)s.otId);
        if (r == 1) snprintf(buf, sizeof(buf), "%s", s.nature);
        if (r == 2) snprintf(buf, sizeof(buf), "%s", s.ability);
        if (r == 4) snprintf(buf, sizeof(buf), "%s Lv%u", s.metAt, (unsigned)s.metLevel);
        if (r == 5) { if (s.item) UiGame_ItemName(s.item, buf, sizeof(buf)); else snprintf(buf, sizeof(buf), "NONE"); }
        if (r == 3)
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 520, y - 4, s.abilityDesc, UI_COL_INK_SOFT);
        else
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 520, y, buf, UI_COL_INK);
    }
    UiW_Inset(360, 417, 572, 31);
    Label(364, 418, "EXP NEXT");
    snprintf(buf, sizeof(buf), "%u", (unsigned)s.expToNext);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 520, 418, buf, UI_COL_INK);
    UiW_Bar(640, 432, 280, 10, s.expPct, UI_RGBA(124, 192, 242, 255), UI_COL_EXP);
}

static void BuildSkills(int32_t slot)
{
    UiSummary s;
    UiGame_Summary(slot, &s);
    static const char *const names[6] = { "HP", "ATTACK", "DEFENSE", "SP. ATK", "SP. DEF", "SPEED" };
    char buf[24];
    Label(364, 76, "STAT");
    Label(640, 76, "VALUE");
    if (s.showIvEv) { Label(760, 76, "IV"); Label(850, 76, "EV"); }
    for (int32_t i = 0; i < 6; i++)
    {
        const int32_t y = 112 + i * 46;
        UiW_Row(360, y, 572, 44, false, i);
        const uint32_t col = i == s.natureUp ? UI_COL_HP_LOW : i == s.natureDown ? UI_COL_EXP : UI_COL_INK;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 380, y + 4, names[i], col);
        if (i == 0) snprintf(buf, sizeof(buf), "%u/%u", (unsigned)s.hp, (unsigned)s.stats[0]);
        else snprintf(buf, sizeof(buf), "%u", (unsigned)s.stats[i]);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 640, y + 4, buf, col);
        if (s.showIvEv)
        {
            snprintf(buf, sizeof(buf), "%u", (unsigned)s.ivs[i]);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 760, y + 4, buf, UI_COL_INK_SOFT);
            snprintf(buf, sizeof(buf), "%u", (unsigned)s.evs[i]);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 850, y + 4, buf, UI_COL_INK_SOFT);
        }
    }
    UiW_Inset(360, 394, 572, 82);
    snprintf(buf, sizeof(buf), "NATURE %s", s.nature);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 376, 400, buf, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 376, 434, "Red: raised  Blue: lowered", UI_COL_INK_SOFT);
}

static void BuildMoves(int32_t slot, bool contest)
{
    static const char *const cats[3] = { "PHYSICAL", "SPECIAL", "STATUS" };
    char buf[32];
    const bool focus = sMode == MODE_MOVES;
    UiMove sel;
    memset(&sel, 0, sizeof(sel));
    for (int32_t i = 0; i < 4; i++)
    {
        UiMove mv;
        UiGame_Move(slot, i, &mv);
        const int32_t y = 74 + i * 52;
        const bool on = focus && i == sMoveSel;
        UiW_Row(360, y, 572, 50, on, i);
        if (focus && i == sMoveSwap)
            UiDraw_Frame(360, y, 572, 50, 3, UI_COL_SELECT);
        if (!mv.move)
        {
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 400, y + 8, "-", UI_COL_INK_SOFT);
            continue;
        }
        if ((focus ? sMoveSel : 0) == i)
            sel = mv;
        const uint32_t ink = on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiDraw_Sprite((uint16_t)(contest ? UI_SPR_CONTEST_FIRST + mv.contestCategory : UI_SPR_TYPE_FIRST + mv.type), 384, y + 9, 2);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 460, y + 8, mv.name, ink);
        if (contest)
            Hearts(820, y + 20, mv.appeal, false);
        else
        {
            snprintf(buf, sizeof(buf), "PP %u/%u", (unsigned)mv.pp, (unsigned)mv.maxPp);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 916 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf), y + 8, buf, ink);
        }
    }
    if (!sel.move)
        return;
    const int32_t y = 292;
    // Data and description in bevelled boxes
    UiW_Inset(360, y - 8, 572, 48);
    UiW_Inset(360, y + 48, 572, 132);
    if (contest)
    {
        Label(376, y, "APPEAL");
        Hearts(520, y + 12, sel.appeal, false);
        Label(700, y, "JAM");
        Hearts(800, y + 12, sel.jam, true);
        UiW_TextWrapped(376, y + 56, 540, 30, sel.contestDesc, UI_COL_INK, 4);
    }
    else
    {
        Label(376, y, "POWER");
        snprintf(buf, sizeof(buf), "%s", sel.power > 1 ? "" : "-");
        if (sel.power > 1) snprintf(buf, sizeof(buf), "%u", (unsigned)sel.power);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 470, y, buf, UI_COL_INK);
        Label(560, y, "ACC");
        if (sel.accuracy) snprintf(buf, sizeof(buf), "%u", (unsigned)sel.accuracy); else snprintf(buf, sizeof(buf), "-");
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 630, y, buf, UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 740, y, cats[sel.category], UI_COL_INK_SOFT);
        UiW_TextWrapped(376, y + 56, 540, 30, sel.desc, UI_COL_INK, 4);
    }
}

static void PartyBuild(void)
{
    UiKit_Header("POKéMON", sTabs, TAB_COUNT, sTab, sTabX, sTabW, 12);
    UiW_Panel(16, 60, 320, 432);
    const int32_t n = UiGame_PartyCount();
    for (int32_t i = 0; i < n; i++)
    {
        const bool on = i == sParty.sel;
        const bool inBattle = UiGame_InBattleMenu() && UiGame_MonIsInBattle(i);
        DrawCard(i, CARD_Y + i * CARD_H, on && sMode != MODE_MOVES, inBattle ? "IN BATTLE" : NULL, true);
        if ((sMode == MODE_SWITCH || sMode == MODE_SHARE) && i == sFrom)
            UiDraw_Frame(CARD_X, CARD_Y + i * CARD_H, CARD_W, CARD_H - 2, 3, UI_COL_SELECT);
    }
    UiW_Panel(348, 60, 596, 432);
    UiMon m;
    if (UiGame_Mon(sParty.sel, &m))
    {
        if (sTab == TAB_INFO || m.egg) BuildInfo(sParty.sel);
        else if (sTab == TAB_SKILLS) BuildSkills(sParty.sel);
        else BuildMoves(sParty.sel, sTab == TAB_CONTEST);
    }
    int32_t fx = 16;
    if (sMode == MODE_MOVES)
    {
        if (!UiGame_InBattleMenu())
            UiW_FooterGlyph(&fx, UI_GLYPH_SQUARE, sMoveSwap < 0 ? "Reorder" : "Swap here");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Party");
    }
    else
    {
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sTab >= TAB_MOVES ? "Moves" : sMode == MODE_BROWSE ? "Options" : "Choose");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, sMode == MODE_BROWSE ? "Back" : "Cancel");
    }
    UiKit_FooterClock();
    if (sPop.open)
        UiMenu_Draw(&sPop);
}

const UiScreen gUiPartyScreen = { PartyEnter, PartyUpdate, PartyBuild, PartyResume };

// =====================================================================================
// Target screen: choose a POKéMON for an item from the bag
// =====================================================================================
static int32_t sTMode;
static uint16_t sItem;
static UiList sTList;
static UiMenu sMoveMenu;
static bool sDeltaOpen;
static UiStatDelta sDelta;
static int32_t sMenuKind;      // 0 PP item, 1 forget a move, 2 swap held item

void UiTarget_Set(int32_t mode, uint16_t itemId)
{
    sTMode = mode;
    sItem = itemId;
}

static void TargetEnter(void)
{
    sTList.sel = sTList.first = 0;      // the first POKéMON is selected
    sMoveMenu.open = false;
    sDeltaOpen = false;
    UiList_Reset(&sTList, UiGame_PartyCount(), 6);
}

static void MovesMenu(int32_t slot, const char *title, bool dontLearn)
{
    UiMenu_Open(&sMoveMenu, title, 348, 150, 420);
    for (int32_t i = 0; i < 4; i++)
    {
        UiMove mv;
        UiGame_Move(slot, i, &mv);
        if (!mv.move) continue;
        char buf[24];
        snprintf(buf, sizeof(buf), "%s %u/%u", mv.name, (unsigned)mv.pp, (unsigned)mv.maxPp);
        UiMenu_Add(&sMoveMenu, buf, i);
    }
    if (dontLearn)
        UiMenu_Add(&sMoveMenu, "DON'T LEARN", 9);
}

static void AfterUse(void)
{
    if (!CheckBagHasItem(sItem, 1))
        UiNav_Pop();
}

static bool TargetUpdate(const UiInput *in)
{
    sFrame++;
    char msg[80];
    const int32_t slot = sTList.sel;
    if (sDeltaOpen)
    {
        if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap)
        {
            PlaySE(SE_SELECT);
            sDeltaOpen = false;
            AfterUse();
        }
        return true;
    }
    if (sMoveMenu.open)
    {
        const int32_t id = UiMenu_Update(&sMoveMenu, in);
        if (id == UI_MENU_NONE || id == UI_MENU_CANCEL || id == 9)
            return true;
        if (sMenuKind == 2)
        {
            if (id == 1) { char it[24]; uint16_t prev; UiGame_ItemName(sItem, it, sizeof(it)); UiGame_GiveItem(slot, sItem, &prev); PlaySE(SE_SELECT); UiKit_Toast("Swapped. The old item went back to the BAG."); UiNav_Pop(); }
            return true;
        }
        if (sTMode == UI_TARGET_BATTLE)
        {
            const UiItemResult r = UiGame_BattleItemOnMon(slot, sItem, id, msg, sizeof(msg));
            if (r == UI_ITEM_CLOSE) { PlaySE(SE_USE_ITEM); UiNav_Close(NULL); }
            else { PlaySE(SE_FAILURE); UiKit_Toast("%s", msg); }
            return true;
        }
        if (sMenuKind == 0)
        {
            const bool ok = UiGame_ItemOnMove(slot, sItem, id);
            PlaySE(ok ? SE_USE_ITEM : SE_FAILURE);
            UiKit_Toast(ok ? "The move's PP went up." : "It won't have any effect.");
        }
        else if (UiGame_ReplaceMove(slot, sItem, id))
        {
            PlaySE(SE_SUCCESS);
            UiKit_Toast("1, 2, and... Poof! It learned the new move.");
        }
        else
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast("HM moves can't be forgotten now.");
        }
        AfterUse();
        return true;
    }
    // Circle or the BACK button: back to the bag without using / giving anything
    if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 364, 436, 200, 40))
    {
        PlaySE(SE_SELECT);
        return false;
    }
    const int32_t a = UiList_Update(&sTList, in, CARD_X, CARD_Y, CARD_W, CARD_H, true);
    if (a < 0)
        return true;
    UiMon m;
    UiGame_Mon(a, &m);
    char item[24];
    UiGame_ItemName(sItem, item, sizeof(item));
    if (sTMode == UI_TARGET_GIVE)
    {
        if (m.egg) { PlaySE(SE_FAILURE); UiKit_Toast("An EGG can't hold items."); return true; }
        if (m.item != ITEM_NONE)
        {
            // Already holding one: confirm the swap, as the game does
            char old[24], title[40];
            UiGame_ItemName(m.item, old, sizeof(old));
            snprintf(title, sizeof(title), "SWAP %s?", old);
            UiMenu_Open(&sMoveMenu, title, 348, 200, 420);
            UiMenu_Add(&sMoveMenu, "YES", 1);
            UiMenu_Add(&sMoveMenu, "NO", 9);
            sMenuKind = 2;
            PlaySE(SE_SELECT);
            return true;
        }
        uint16_t prev;
        UiGame_GiveItem(a, sItem, &prev);
        PlaySE(SE_SELECT);
        if (prev != ITEM_NONE)
        {
            char old[24];
            UiGame_ItemName(prev, old, sizeof(old));
            UiKit_Toast("%s now holds the %s. The %s went back to the BAG.", m.name, item, old);
        }
        else
            UiKit_Toast("%s was given the %s to hold.", m.name, item);
        return false;                                      // back to the bag
    }
    if (sTMode == UI_TARGET_BATTLE)
    {
        const UiItemResult r = UiGame_BattleItemOnMon(a, sItem, -1, msg, sizeof(msg));
        if (r == UI_ITEM_CHOOSE_MOVE) { PlaySE(SE_SELECT); sMenuKind = 0; MovesMenu(a, "WHICH MOVE?", false); }
        else if (r == UI_ITEM_CLOSE) { PlaySE(SE_USE_ITEM); UiNav_Close(NULL); }
        else { PlaySE(SE_FAILURE); UiKit_Toast("%s", msg); }
        return true;
    }
    void (*after)(void) = NULL;
    const UiItemResult r = UiGame_ItemOnMon(a, sItem, &sDelta, msg, sizeof(msg), &after);
    switch (r)
    {
    case UI_ITEM_FAIL: PlaySE(SE_FAILURE); UiKit_Toast("%s", msg); break;
    case UI_ITEM_DONE: PlaySE(SE_USE_ITEM); UiKit_Toast("%s", msg); AfterUse(); break;
    case UI_ITEM_DELTA: PlaySE(SE_USE_ITEM); sDeltaOpen = true; break;
    case UI_ITEM_CHOOSE_MOVE: PlaySE(SE_SELECT); sMenuKind = 0; MovesMenu(a, "WHICH MOVE?", false); break;
    case UI_ITEM_FORGET_MOVE: PlaySE(SE_SELECT); sMenuKind = 1; MovesMenu(a, "FORGET WHICH MOVE?", true); break;
    case UI_ITEM_CLOSE: UiNav_Close(after); break;
    }
    return true;
}

static void TargetBuild(void)
{
    static const char *const tab[1] = { "CHOOSE A POKéMON" };
    int32_t tx[1], tw[1];
    UiKit_Header("POKéMON", tab, 1, 0, tx, tw, 12);
    char item[24], buf[96];
    UiGame_ItemName(sItem, item, sizeof(item));
    snprintf(buf, sizeof(buf), "%s · %s", sTMode == UI_TARGET_GIVE ? "GIVE" : sTMode == UI_TARGET_TM ? "TEACH" : "USE", item);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    UiW_Panel(16, 60, 320, 432);
    const int32_t n = UiGame_PartyCount();
    for (int32_t i = 0; i < n; i++)
    {
        const char *tag = NULL;
        bool ok = false;
        if (sTMode == UI_TARGET_TM)
        {
            bool known = false;
            ok = UiGame_CanLearnTm(i, sItem, &known);
            tag = known ? "LEARNED" : ok ? "ABLE" : "NOT ABLE";
        }
        DrawCard(i, CARD_Y + i * CARD_H, i == sTList.sel, tag, ok);
    }

    UiW_Panel(348, 60, 596, 432);
    UiW_Inset(364, 76, 56, 56);
    UiSpr_ItemIcon(sItem, 368, 80, 2);
    UiText_Draw(UI_FACE_PS2P, 16, 436, 84, item, UI_COL_INK);
    if (sTMode == UI_TARGET_TM)
    {
        char mv[16];
        UiGame_MoveName(UiGame_ItemMove(sItem), mv, sizeof(mv));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 436, 104, mv, UI_COL_INK);
    }
    UiGame_ItemDescription(sItem, buf, sizeof(buf));
    UiW_TextWrapped(364, 150, 564, 30, buf, UI_COL_INK, 4);
    UiDraw_Rect(364, 280, 564, 2, UI_COL_PAPER2);
    const char *hint = sTMode == UI_TARGET_GIVE ? "Choose who will hold it. Its held item goes back to the BAG."
                     : sTMode == UI_TARGET_TM ? "Choose who will learn the move."
                                              : "Choose the POKéMON to use it on.";
    UiW_TextWrapped(364, 296, 564, 30, hint, UI_COL_INK_SOFT, 3);
    UiW_Button(364, 436, 200, "BACK", UI_BTN_RED, false);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, sTMode == UI_TARGET_GIVE ? "Give" : sTMode == UI_TARGET_TM ? "Teach" : "Use");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back to the bag");
    UiKit_FooterClock();
    if (sMoveMenu.open)
        UiMenu_Draw(&sMoveMenu);
    if (sDeltaOpen)
    {
        static const char *const names[6] = { "HP", "ATTACK", "DEFENSE", "SP. ATK", "SP. DEF", "SPEED" };
        UiDraw_Rect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COL_DIM);
        UiMon m;
        UiGame_Mon(sTList.sel, &m);
        snprintf(buf, sizeof(buf), "%s Lv%u", m.name, (unsigned)sDelta.levelAfter);
        UiW_Window(260, 100, 440, 300, buf);
        for (int32_t i = 0; i < 6; i++)
        {
            const int32_t y = 148 + i * 40;
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 284, y, names[i], UI_COL_INK);
            snprintf(buf, sizeof(buf), "%d -> %d", sDelta.before[i], sDelta.after[i]);
            UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 460, y, buf, UI_COL_INK);
            if (sDelta.after[i] != sDelta.before[i])
            {
                snprintf(buf, sizeof(buf), "+%d", sDelta.after[i] - sDelta.before[i]);
                UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 620, y, buf, UI_COL_HP_HIGH);
            }
        }
    }
}

const UiScreen gUiTargetScreen = { TargetEnter, TargetUpdate, TargetBuild, NULL };
