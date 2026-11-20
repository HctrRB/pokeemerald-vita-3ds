// POKéMON of the new UI (design/stitch/equipo, equipo_movimientos, equipo_objetivo, equipo_mt).
// Party screen: cards on the left, summary on the right with the INFO / SKILLS / MOVES / CONTEST
// tabs; X opens the popup (field moves, SWITCH, ITEM), SHARE HP and SWITCH pick a second card.
// Target screen ("choose a POKéMON"), opened from the bag: USE an item, GIVE it, or teach a TM.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "platform/vita_controls.h"
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
enum { P_SWITCH = 1000, P_ITEM, P_MAIL, P_MAIL_READ, P_MAIL_TAKE, P_MAIL_TO_PC, P_MAIL_TO_BAG, P_CANCEL, P_GIVE, P_TAKE, P_SHIFT, P_SUMMARY };   // above every MOVE_* id

static int32_t sTab, sMode, sFrom, sMoveSel, sMoveSwap = -1;
static bool sSummaryOpen;
static UiList sParty;
static UiMenu sPop;
static int32_t sTabX[TAB_COUNT], sTabW[TAB_COUNT];
static uint8_t sFmSlot;
static uint16_t sFmMove;

// Summary only (UiParty_PushSummary, the PC's SUMMARY): the summary of the POKéMON UiGame_View*
// reads, ↑ / ↓ through them, ○ back to the screen under it. No party actions.
static bool sViewOnly;
static int32_t sViewStart = -1;     // the slot to open on, once (-1: the normal party screen)
static int32_t sLastViewed;

static void PartyEnter(void)
{
    sParty.sel = sParty.first = 0;
    sTab = TAB_INFO;
    sMode = MODE_BROWSE;
    sSummaryOpen = false;
    sMoveSwap = -1;
    sPop.open = false;
    sViewOnly = sViewStart >= 0;
    if (sViewOnly)
    {
        sParty.sel = sViewStart;
        sSummaryOpen = true;
    }
    sViewStart = -1;
    UiList_Reset(&sParty, UiGame_ViewCount(), 6);
}

static void PartyResume(void)
{
    UiList_Reset(&sParty, UiGame_ViewCount(), 6);
}

// Leaving the summary only: what was looked at last (as gLastViewedMonIndex), and the party again
static bool LeaveViewOnly(void)
{
    sLastViewed = UiGame_ViewPosition(sParty.sel);
    UiGame_ViewParty();
    sViewOnly = false;
    return false;
}

void UiParty_PushSummary(int32_t slot)
{
    sViewStart = slot < 0 ? 0 : slot;
    UiNav_Push(&gUiPartyScreen);
}

int32_t UiParty_LastViewed(void)
{
    return sLastViewed;
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
    const int32_t px = gUiTheme->gbaFrame ? 340 : 348;
    const int32_t py = gUiTheme->gbaFrame ? 68 + slot * 50 : CARD_Y + (slot - sParty.first) * CARD_H;
    UiMenu_Open(&sPop, m.name, px, py, 260);
    if (UiGame_InBattleMenu())
    {
        UiMenu_Add(&sPop, "SHIFT", P_SHIFT);
        UiMenu_Add(&sPop, "SUMMARY", P_SUMMARY);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    UiMenu_Add(&sPop, "SUMMARY", P_SUMMARY);
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
        UiMenu_Add(&sPop, ItemIsMail(m.item) ? "MAIL" : "ITEM", ItemIsMail(m.item) ? P_MAIL : P_ITEM);
    UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
}

static void PopupChoice(int32_t id)
{
    const int32_t slot = sParty.sel;
    UiMon m;
    UiGame_Mon(slot, &m);
    if (id == P_SUMMARY)
    {
        sSummaryOpen = true;
        sTab = TAB_SKILLS;
        sMode = MODE_BROWSE;
        sMoveSwap = -1;
        return;
    }
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
        UiMenu_Open(&sPop, "ITEM", gUiTheme->gbaFrame ? 340 : 348, gUiTheme->gbaFrame ? 68 + slot * 50 : CARD_Y + (slot - sParty.first) * CARD_H, 260);
        UiMenu_Add(&sPop, "GIVE", P_GIVE);
        if (m.item != ITEM_NONE)
            UiMenu_Add(&sPop, "TAKE", P_TAKE);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    if (id == P_MAIL)
    {
        UiMenu_Open(&sPop, "MAIL", gUiTheme->gbaFrame ? 340 : 348, gUiTheme->gbaFrame ? 68 + slot * 50 : CARD_Y + (slot - sParty.first) * CARD_H, 260);
        UiMenu_Add(&sPop, "READ", P_MAIL_READ);
        UiMenu_Add(&sPop, "TAKE", P_MAIL_TAKE);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    if (id == P_MAIL_READ)
    {
        const u8 mailId = (u8)GetMonData(&gPlayerParty[slot], MON_DATA_MAIL);
        if (mailId < MAIL_COUNT)
            UiMailRead_Open(&gSaveBlock1Ptr->mail[mailId], "PARTY");
        return;
    }
    if (id == P_MAIL_TAKE)
    {
        UiMenu_Open(&sPop, "SEND TO PC?", gUiTheme->gbaFrame ? 320 : 330, gUiTheme->gbaFrame ? 68 + slot * 50 : CARD_Y + (slot - sParty.first) * CARD_H, 300);
        UiMenu_Add(&sPop, "SEND TO PC", P_MAIL_TO_PC);
        UiMenu_Add(&sPop, "TO BAG (ERASE)", P_MAIL_TO_BAG);
        UiMenu_Add(&sPop, "CANCEL", P_CANCEL);
        return;
    }
    if (id == P_MAIL_TO_PC || id == P_MAIL_TO_BAG)
    {
        const bool toPc = (id == P_MAIL_TO_PC);
        const int32_t res = UiGame_TakeMail(slot, toPc);
        if (res > 0)
        {
            PlaySE(SE_SELECT);
            UiKit_Toast(toPc ? "The MAIL was transferred to the MAILBOX." : "The MAIL was returned to the BAG.");
        }
        else
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast(toPc ? "The PC's MAILBOX is full." : "The BAG is full.");
        }
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

static void GbaSlotRect(int32_t i, int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    *x = 14;
    *w = 556;
    if (i == 0)
    {
        *y = 60;
        *h = 82;
    }
    else
    {
        *y = 148 + (i - 1) * 63;
        *h = 58;
    }
}

static int32_t PartyMoveCount(int32_t slot)
{
    int32_t n = 0;
    for (int32_t i = 0; i < 4; i++)
    {
        UiMove mv;
        UiGame_Move(slot, i, &mv);
        if (mv.move) n = i + 1;
    }
    return n;
}

static bool PartyGbaUpdate(const UiInput *in)
{
    if (sSummaryOpen)
    {
        if (in->pressed & VITA_BTN_CIRCLE)
        {
            PlaySE(SE_SELECT);
            if (sMode == MODE_MOVES)
            {
                sMode = MODE_BROWSE;
                sMoveSwap = -1;
            }
            else if (sViewOnly)
                return LeaveViewOnly();
            else
                sSummaryOpen = false;
            return true;
        }
        int32_t nextTab = -1;
        const int32_t ph = UiW_GbaPagerHit(in, true, TAB_COUNT);
        if ((in->pressed & VITA_BTN_L) || ph < 0) nextTab = (sTab + TAB_COUNT - 1) % TAB_COUNT;
        if ((in->pressed & VITA_BTN_R) || ph > 0) nextTab = (sTab + 1) % TAB_COUNT;
        if (sMode != MODE_MOVES)
        {
            if (in->repeat & VITA_BTN_LEFT) nextTab = (sTab + TAB_COUNT - 1) % TAB_COUNT;
            if (in->repeat & VITA_BTN_RIGHT) nextTab = (sTab + 1) % TAB_COUNT;
        }
        if (nextTab >= 0 && nextTab != sTab)
        {
            sTab = nextTab;
            sMode = MODE_BROWSE;
            sMoveSwap = -1;
            PlaySE(SE_SELECT);
            return true;
        }
        // TappingKnownMoves 2x2 pills on SKILLS opens MOVES directly
        if (sTab == TAB_SKILLS)
        {
            for (int32_t k = 0; k < 4; k++)
            {
                const int32_t mx = 362 + (k & 1) * 290;
                const int32_t my = 430 + (k >> 1) * 44;
                if (UiHit(in, mx, my, 284, 40) && k < PartyMoveCount(sParty.sel))
                {
                    sTab = TAB_MOVES;
                    sMoveSel = k;
                    sMode = MODE_MOVES;
                    sMoveSwap = -1;
                    PlaySE(SE_SELECT);
                    return true;
                }
            }
        }
        if (sTab >= TAB_MOVES)
        {
            const int32_t n = PartyMoveCount(sParty.sel);
            for (int32_t k = 0; k < n; k++)
                if (UiHit(in, 362, 104 + k * 54, 574, 50))
                {
                    sMode = MODE_MOVES;
                    sMoveSel = k;
                    PlaySE(SE_SELECT);
                }
            if (sMode == MODE_MOVES && n > 0)
            {
                const int32_t before = sMoveSel;
                if (in->repeat & VITA_BTN_UP) sMoveSel = (sMoveSel + n - 1) % n;
                if (in->repeat & VITA_BTN_DOWN) sMoveSel = (sMoveSel + 1) % n;
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
            if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_SQUARE)) && n > 0)
            {
                sMode = MODE_MOVES;
                sMoveSel = 0;
                sMoveSwap = -1;
                PlaySE(SE_SELECT);
                return true;
            }
        }
        // Up / Down switches POKéMON while staying in Summary View (04_summary)
        const int32_t pCount = UiGame_ViewCount();
        if (pCount > 1)
        {
            const int32_t before = sParty.sel;
            if (in->repeat & VITA_BTN_UP) sParty.sel = (sParty.sel + pCount - 1) % pCount;
            if (in->repeat & VITA_BTN_DOWN) sParty.sel = (sParty.sel + 1) % pCount;
            if (sParty.sel != before)
            {
                sMoveSel = 0;
                sMoveSwap = -1;
                PlaySE(SE_SELECT);
            }
        }
        return true;
    }

    // Party View (02_party)
    if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 788, 468, 158, 68))
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
    if ((in->pressed & VITA_BTN_SQUARE) || UiHit(in, 582, 60, 364, 402))
    {
        PlaySE(SE_SELECT);
        sSummaryOpen = true;
        sTab = TAB_SKILLS;
        sMode = MODE_BROWSE;
        sMoveSwap = -1;
        return true;
    }
    if ((in->pressed & VITA_BTN_TRIANGLE) && !UiGame_InBattleMenu() && UiGame_PartyCount() > 1)
    {
        UiMon m;
        UiGame_Mon(sParty.sel, &m);
        PlaySE(SE_SELECT);
        sMode = MODE_SWITCH;
        sFrom = sParty.sel;
        UiKit_Toast("Move %s to where?", m.name);
        return true;
    }
    const int32_t pCount = UiGame_PartyCount();
    if (pCount <= 0)
        return true;
    const int32_t before = sParty.sel;
    if (in->repeat & VITA_BTN_UP) sParty.sel = (sParty.sel + pCount - 1) % pCount;
    if (in->repeat & VITA_BTN_DOWN) sParty.sel = (sParty.sel + 1) % pCount;
    if (in->repeat & VITA_BTN_LEFT) sParty.sel = 0;
    if ((in->repeat & VITA_BTN_RIGHT) && pCount > 1 && sParty.sel == 0) sParty.sel = 1;
    bool activate = (in->pressed & VITA_BTN_CROSS) != 0;
    int32_t hitIdx = -1;
    for (int32_t i = 0; i < pCount; i++)
    {
        int32_t sx, sy, sw, sh;
        GbaSlotRect(i, &sx, &sy, &sw, &sh);
        if (UiHit(in, sx, sy, sw, sh))
        {
            hitIdx = i;
            if (sParty.sel == i || sMode == MODE_SWITCH || sMode == MODE_SHARE)
                activate = true;
            sParty.sel = i;
        }
    }
    if (sParty.sel != before)
        PlaySE(SE_SELECT);
    if (activate)
    {
        const int32_t a = hitIdx >= 0 ? hitIdx : sParty.sel;
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
        else
            OpenPopup();
    }
    return true;
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
    if (gUiTheme->gbaFrame)
        return PartyGbaUpdate(in);
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
        if (sViewOnly)
            return LeaveViewOnly();
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
    if (sViewOnly && sTab < TAB_MOVES)
        return true;                                  // the summary only: no party actions
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

static void DrawGbaCancelBtn(int32_t x, int32_t y, int32_t w, int32_t h)
{
    UiW_Bevel(x, y, w, h, 5, UI_RGBA(24, 32, 32, 255), UI_RGBA(106, 106, 168, 255), UI_RGBA(70, 70, 118, 255),
              2, UI_RGBA(148, 148, 208, 255), UI_RGBA(44, 44, 78, 255), 3);
    UiDraw_Sprite(UI_SPR_BALL_POKE, x + 14, y + (h - 24) / 2, 2);
    UiText_DrawMid(UI_FACE_PS2P, 16, x + 46, y + 1, h, "CANCEL", UI_RGBA(24, 24, 40, 200));
    UiText_DrawMid(UI_FACE_PS2P, 16, x + 45, y, h, "CANCEL", UI_RGBA(255, 255, 255, 255));
}

static void DrawGbaPartySlot(int32_t slot, bool sel, bool mark, const char *tag, bool tagOk, bool partnerDim)
{
    int32_t x, y, w, h;
    GbaSlotRect(slot, &x, &y, &w, &h);
    const bool lead = (slot == 0);
    const uint32_t edge = mark ? UI_RGBA(245, 209, 98, 255) : UI_RGBA(24, 32, 32, 255);
    const uint32_t top = sel ? UI_RGBA(72, 184, 160, 255) : lead ? UI_RGBA(72, 158, 168, 255) : UI_RGBA(59, 132, 164, 255);
    const uint32_t bot = sel ? UI_RGBA(42, 136, 112, 255) : lead ? UI_RGBA(48, 114, 128, 255) : UI_RGBA(40, 98, 128, 255);
    const uint32_t hi  = sel ? UI_RGBA(158, 240, 222, 255) : UI_RGBA(120, 205, 216, 255);
    if (sel)
        UiW_GbaRingBox(x, y, w, h, 5, UI_RGBA(255, 90, 32, 255), UI_RGBA(255, 230, 57, 255), top, bot, hi, 3);
    else
        UiW_Bevel(x, y, w, h, 5, edge, top, bot, 2, hi, UI_RGBA(24, 58, 72, 255), 3);

    UiMon m;
    if (!UiGame_Mon(slot, &m))
        return;
    const uint8_t frame = (uint8_t)((sFrame / (sel ? 8 : 16)) & 1);
    if (sel)
    {
        UiW_GbaArrowH(x + 11, y + (h - 12) / 2 + 1, true, UI_GBA_SHAD_DARK);
        UiW_GbaArrowH(x + 10, y + (h - 12) / 2, true, UI_RGBA(230, 8, 8, 255));
    }
    else
        UiDraw_Sprite(UI_SPR_BALL_POKE, x + 10, y + (h - 12) / 2, 1);

    if (lead)
        UiSpr_MonIcon(m.species, frame, x + 28, y + 8 - (frame ? 3 : 0), 2);
    else
        UiSpr_MonIcon(m.species, frame, x + 32, y + 12 - (frame ? 2 : 0), 1);

    const int32_t textX = lead ? x + 102 : x + 74;
    const int32_t maxNameW = (x + 324) - textX - (m.egg ? 0 : (m.shiny ? 38 : 22));
    UiDraw_PushClip(textX, y + (lead ? 8 : 4), maxNameW, 30);
    const int32_t nameW = UiW_GbaText(UI_FACE_PS2P, 16, textX, y + (lead ? 14 : 9), m.name, C_WHITE, UI_GBA_SHAD_DARK);
    UiDraw_PopClip();
    if (!m.egg)
        UiKit_GenderShiny(textX + (nameW > maxNameW ? maxNameW : nameW) + 6, y + (lead ? 8 : 3), m.gender, m.shiny);

    if (m.egg)
    {
        UiW_GbaText(UI_FACE_VT323, 28, textX, y + (lead ? 42 : 28), "EGG", UI_RGBA(216, 248, 240, 255), UI_GBA_SHAD_DARK);
        if (tag)
            UiW_Chip(x + w - 20 - UiText_Width(UI_FACE_VT323, 32, tag), y + (h - 26) / 2, tag, tagOk);
    }
    else
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Lv%u", (unsigned)m.level);
        const int32_t lvY = lead ? y + 46 : y + 32;
        const int32_t lvW = UiW_GbaText(UI_FACE_VT323, 24, textX, lvY, buf, UI_RGBA(255, 230, 57, 255), UI_GBA_SHAD_DARK);
        int32_t bx = textX + lvW + 10;
        const int32_t st = UiGame_MonStatusIcon(slot);
        if (st >= 0)
        {
            UiDraw_Sprite((uint16_t)(UI_SPR_STATUS_FIRST + st), bx, lvY + 4, 2);
            bx += 68;
        }
        if (m.item != ITEM_NONE)
        {
            UiDraw_Sprite(ItemIsMail(m.item) ? UI_SPR_HOLD_MAIL : UI_SPR_HOLD_ITEM, bx, lvY + 6, 2);
            if (lead)
            {
                char iname[24];
                UiGame_ItemName(m.item, iname, sizeof(iname));
                UiW_GbaText(UI_FACE_VT323, 24, bx + 20, lvY, iname, UI_RGBA(216, 248, 240, 255), UI_GBA_SHAD_DARK);
            }
        }
        if (tag)
        {
            UiW_Bar(x + 332, y + 12, 206, 12, m.maxHp ? (int32_t)m.hp * 100 / m.maxHp : 0, UI_RGBA(115, 255, 172, 255), UI_RGBA(90, 213, 131, 255));
            UiW_Chip(x + w - 16 - UiText_Width(UI_FACE_VT323, 32, tag) - 16, y + (lead ? 42 : 28), tag, tagOk);
        }
        else
        {
            UiW_GbaHpBar(x + 332, y + (lead ? 14 : 10), 208, 16, m.hp, m.maxHp);
            snprintf(buf, sizeof(buf), "%u / %u", (unsigned)m.hp, (unsigned)m.maxHp);
            UiW_GbaText(UI_FACE_VT323, lead ? 32 : 28, x + w - 16 - UiText_Width(UI_FACE_VT323, lead ? 32 : 28, buf),
                        y + (lead ? 40 : 28), buf, C_WHITE, UI_GBA_SHAD_DARK);
        }
    }
    if (partnerDim)
        UiDraw_Rect(x, y, w, h, UI_RGBA(0, 0, 0, 72));
}

static void BuildGbaInspectPanel(int32_t slot)
{
    UiW_GbaWin(582, 60, 364, 402, UI_GBA_WIN_DEFAULT);
    UiMon m;
    UiSummary s;
    if (!UiGame_Mon(slot, &m))
        return;
    UiGame_Summary(slot, &s);

    char buf[48];
    snprintf(buf, sizeof(buf), "No.%03u", (unsigned)s.dexNo);
    UiW_GbaSectionStrip(592, 70, 344, 26, UI_GBA_TAG_BLUE, "PROFILE", m.egg ? "EGG" : buf);

    // Visor (592, 100, 82, 82)
    UiW_GbaBox(592, 100, 82, 82, 4, UI_RGBA(40, 48, 48, 255), UI_RGBA(230, 236, 244, 255), UI_RGBA(230, 236, 244, 255), UI_GBA_SHAD_LIGHT, 0);
    for (int32_t g = 10; g < 78; g += 12)
    {
        UiDraw_Rect(594 + g, 102, 1, 78, UI_RGBA(180, 196, 212, 90));
        UiDraw_Rect(594, 102 + g, 78, 1, UI_RGBA(180, 196, 212, 90));
    }
    UiSpr_MonFront(m.species, m.shiny, 601, 109, 1);

    const int32_t maxNameW = 204;
    UiDraw_PushClip(684, 98, maxNameW, 30);
    const int32_t nw = UiW_GbaText(UI_FACE_PS2P, 16, 684, 102, m.name, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    UiDraw_PopClip();
    if (m.egg)
    {
        const char *eggHint = s.eggCycles <= 5 ? "Almost ready to hatch!"
                            : s.eggCycles <= 10 ? "Moves around inside."
                                                : "Will take time to hatch.";
        UiW_GbaText(UI_FACE_VT323, 28, 684, 134, eggHint, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        return;
    }
    UiKit_GenderShiny(684 + (nw > maxNameW ? maxNameW : nw) + 6, 98, m.gender, m.shiny);
    UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[0]), 684, 126, 2);
    if (s.types[1] != s.types[0])
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[1]), 754, 126, 2);
    snprintf(buf, sizeof(buf), "%s · Lv%u", s.nature, (unsigned)m.level);
    UiW_GbaText(UI_FACE_VT323, 24, 684, 160, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);

    // Ability & Item
    UiW_GbaSectionStrip(592, 190, 344, 26, UI_GBA_TAG_TEAL, "ABILITY & ITEM", s.ability);
    UiW_GbaBox(592, 220, 344, 66, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiDraw_PushClip(596, 222, 336, 62);
    if (s.item != ITEM_NONE)
    {
        char iname[24];
        UiGame_ItemName(s.item, iname, sizeof(iname));
        UiSpr_ItemIcon(s.item, 598, 222, 1);
        snprintf(buf, sizeof(buf), "Item: %s", iname);
        UiW_GbaText(UI_FACE_VT323, 24, 624, 223, buf, UI_RGBA(32, 104, 168, 255), UI_GBA_SHAD_LIGHT);
    }
    else
        UiW_GbaText(UI_FACE_VT323, 24, 600, 223, "Item: NONE", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    UiW_GbaTextWrapped(UI_FACE_VT323, 24, 600, 243, 330, 20, s.abilityDesc, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
    UiDraw_PopClip();

    // Known Moves 2x2
    const int32_t mvCount = PartyMoveCount(slot);
    snprintf(buf, sizeof(buf), "%d / 4", (int)mvCount);
    UiW_GbaSectionStrip(592, 292, 344, 26, UI_GBA_TAG_MAGENTA, "KNOWN MOVES", buf);
    for (int32_t k = 0; k < 4; k++)
    {
        const int32_t mx = 592 + (k & 1) * 176;
        const int32_t my = 322 + (k >> 1) * 65;
        UiW_GbaBox(mx, my, 168, 60, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        UiMove mv;
        UiGame_Move(slot, k, &mv);
        if (!mv.move)
        {
            UiW_GbaTextCentred(UI_FACE_VT323, 24, mx, my, 168, 60, "---", UI_RGBA(140, 148, 140, 255), UI_GBA_SHAD_LIGHT);
            continue;
        }
        UiDraw_PushClip(mx + 6, my + 2, 156, 56);
        UiW_GbaText(UI_FACE_VT323, 24, mx + 8, my + 4, mv.name, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + mv.type), mx + 8, my + 32, 1);
        snprintf(buf, sizeof(buf), "%u/%u", (unsigned)mv.pp, (unsigned)mv.maxPp);
        UiW_GbaText(UI_FACE_VT323, 24, mx + 160 - UiText_Width(UI_FACE_VT323, 24, buf), my + 30, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiDraw_PopClip();
    }
}

static void BuildGbaSummary(void)
{
    static const char *const kPageNames[TAB_COUNT] = { "POKéMON INFO", "POKéMON SKILLS", "BATTLE MOVES", "CONTEST MOVES" };
    UiW_GbaTopBar(UI_GBA_BG_SUMMARY, UI_SPR_BALL_POKE, "POKéMON SUMMARY", TAB_COUNT, sTab, false, kPageNames[sTab]);
    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "BACK", false);
    if (sMode == MODE_MOVES)
    {
        UiW_GbaHelpBadge(&rx, "X", "SWAP", false);
        UiW_GbaHelpBadge(&rx, "UD", "MOVE", false);
    }
    else
    {
        UiW_GbaHelpBadge(&rx, "LR", "PAGE", false);
        UiW_GbaHelpBadge(&rx, "UD", "MON", false);
    }

    UiMon m;
    UiSummary s;
    if (!UiGame_Mon(sParty.sel, &m))
        return;
    UiGame_Summary(sParty.sel, &s);

    char buf[96], lvBuf[16];
    snprintf(buf, sizeof(buf), "No.%03u %s", (unsigned)s.dexNo, m.name);
    snprintf(lvBuf, sizeof(lvBuf), m.egg ? "EGG" : "Lv%u", (unsigned)m.level);

    // Left Summary Card (14, 60, 326, 472)
    UiW_GbaWin(14, 60, 326, 472, UI_GBA_WIN_DEFAULT);
    UiW_GbaSectionStrip(24, 70, 306, 28, UI_GBA_TAG_SLATE, buf, lvBuf);

    // Visor (24, 104, 306, 152)
    UiW_GbaBox(24, 104, 306, 152, 4, UI_RGBA(40, 48, 48, 255), UI_RGBA(230, 236, 248, 255), UI_RGBA(216, 224, 240, 255), UI_GBA_SHAD_LIGHT, 0);
    for (int32_t gx = 26; gx < 328; gx += 16)
        UiDraw_Rect(gx, 106, 1, 148, UI_RGBA(180, 196, 224, 90));
    for (int32_t gy = 106; gy < 254; gy += 16)
        UiDraw_Rect(26, gy, 302, 1, UI_RGBA(180, 196, 224, 90));
    UiDraw_Sprite(UI_SPR_BALL_POKE, 34, 112, 2);
    if (!m.egg)
        UiKit_GenderShiny(62, 110, m.gender, m.shiny);
    UiSpr_MonFront(m.species, m.shiny, 24 + (306 - 128) / 2, 104 + 12, 2);
    if (!m.egg)
    {
        UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[0]), 32, 218, 2);
        if (s.types[1] != s.types[0])
            UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + s.types[1]), 100, 218, 2);
        for (uint8_t k = 0; k < 4; k++)
            UiW_GbaMark(254 + k * 18, 232, k, UI_RGBA(112, 120, 136, 255));
    }

    if (!m.egg)
    {
        UiW_GbaHpBar(24, 266, 198, 16, m.hp, m.maxHp);
        snprintf(buf, sizeof(buf), "%u / %u", (unsigned)m.hp, (unsigned)m.maxHp);
        UiW_GbaTextMid(UI_FACE_VT323, 28, 330 - UiText_Width(UI_FACE_VT323, 28, buf), 262, 24, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "NEXT %u", (unsigned)s.expToNext);
        UiW_GbaExpBar(24, 290, 306, s.expPct, buf);
    }

    UiW_GbaSectionStrip(24, 316, 306, 26, UI_GBA_TAG_BLUE, "TRAINER MEMO", m.egg ? "DAYCARE" : s.ot);
    UiW_GbaBox(24, 348, 306, 172, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiDraw_PushClip(32, 354, 290, 160);
    if (m.egg)
    {
        const char *t = s.eggCycles <= 5 ? "It's making sounds inside. It's going to hatch soon!"
                      : s.eggCycles <= 10 ? "It moves around inside sometimes. Must be close to hatching."
                      : s.eggCycles <= 40 ? "What will hatch from this? Doesn't seem close to hatching."
                                          : "It looks like this EGG will take a long time to hatch.";
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 34, 358, 286, 28, t, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 5);
    }
    else
    {
        const int32_t nw = UiW_GbaText(UI_FACE_VT323, 28, 34, 356, s.nature, UI_RGBA(208, 40, 40, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaText(UI_FACE_VT323, 28, 34 + nw + 6, 356, "nature.", UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "Met at %s at Lv.%u.", s.metAt, (unsigned)s.metLevel);
        UiW_GbaTextWrapped(UI_FACE_VT323, 28, 34, 388, 286, 26, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 3);
        snprintf(buf, sizeof(buf), "OT: %s   IDNo. %05u", s.ot, (unsigned)s.otId);
        UiW_GbaText(UI_FACE_VT323, 24, 34, 482, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }
    UiDraw_PopClip();

    // Right Data Sheet (352, 60, 594, 472)
    UiW_GbaWin(352, 60, 594, 472, UI_GBA_WIN_DEFAULT);
    if (m.egg)
    {
        UiW_GbaSectionStrip(362, 70, 574, 28, UI_GBA_TAG_BLUE, "POKéMON EGG", "UNKNOWN");
        UiW_GbaBox(362, 104, 574, 416, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        UiW_GbaTextCentred(UI_FACE_VT323, 32, 362, 260, 574, 40, "Stats and moves are unknown while inside the EGG.", UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        return;
    }

    if (sTab == TAB_SKILLS)
    {
        static const char *const kStatNames[6] = { "HP", "ATTACK", "DEFENSE", "SP. ATK", "SP. DEF", "SPEED" };
        static const uint32_t kBarCols[6] = {
            UI_RGBA(90, 213, 131, 255),  UI_RGBA(240, 120, 104, 255), UI_RGBA(255, 164, 98, 255),
            UI_RGBA(88, 184, 248, 255),  UI_RGBA(255, 230, 57, 255),  UI_RGBA(216, 112, 192, 255),
        };
        if (s.natureUp > 0 && s.natureDown > 0)
            snprintf(buf, sizeof(buf), "+%s / -%s", kStatNames[s.natureUp], kStatNames[s.natureDown]);
        else
            snprintf(buf, sizeof(buf), "%s (NEUTRAL)", s.nature);
        UiW_GbaSectionStrip(362, 70, 574, 28, UI_GBA_TAG_GREEN, "STATS & NATURE", buf);

        UiW_GbaBox(362, 104, 574, 218, 4, UI_RGBA(40, 48, 48, 255), C_WHITE, C_WHITE, 0, 0);
        for (int32_t i = 0; i < 6; i++)
        {
            const int32_t ry = 106 + i * 36;
            const bool up = (i == s.natureUp), dn = (i == s.natureDown);
            UiDraw_Rect(364, ry, 136, 36, UI_RGBA(200, 216, 120, 255));
            UiDraw_Rect(500, ry, 2, 36, UI_RGBA(136, 152, 64, 255));
            UiDraw_Rect(502, ry, 432, 36, (i & 1) ? UI_RGBA(241, 242, 228, 255) : UI_RGBA(250, 248, 236, 255));
            if (i < 5)
                UiDraw_Rect(364, ry + 35, 570, 1, UI_RGBA(150, 172, 156, 220));
            const uint32_t nameCol = up ? UI_RGBA(208, 40, 40, 255) : dn ? UI_RGBA(32, 104, 168, 255) : UI_RGBA(40, 48, 24, 255);
            UiW_GbaTextMid(UI_FACE_PS2P, 16, 372, ry, 36, kStatNames[i], nameCol, UI_RGBA(232, 240, 176, 255));
            if (i == 0) snprintf(buf, sizeof(buf), "%u/%u", (unsigned)s.hp, (unsigned)s.stats[0]);
            else snprintf(buf, sizeof(buf), "%u%s", (unsigned)s.stats[i], up ? "+" : dn ? "-" : "");
            UiW_GbaTextMid(UI_FACE_VT323, 28, 566 - UiText_Width(UI_FACE_VT323, 28, buf), ry, 36, buf, nameCol, UI_GBA_SHAD_LIGHT);

            // Stat gauge bar
            const int32_t barW = s.showIvEv ? 196 : 340;
            int32_t pct = (int32_t)s.stats[i] * 100 / 350;
            if (pct > 100) pct = 100;
            UiW_Bar(578, ry + 12, barW, 12, pct, kBarCols[i], kBarCols[i]);
            if (s.showIvEv)
            {
                UiW_GbaBox(784, ry + 6, 70, 24, 3, UI_RGBA(184, 192, 176, 255), UI_RGBA(232, 236, 224, 255), UI_RGBA(232, 236, 224, 255), 0, 0);
                snprintf(buf, sizeof(buf), "EV %u", (unsigned)s.evs[i]);
                UiW_GbaTextCentred(UI_FACE_VT323, 24, 784, ry + 6, 70, 24, buf, UI_RGBA(64, 72, 64, 255), UI_GBA_SHAD_LIGHT);

                const bool perf = (s.ivs[i] == 31);
                UiW_GbaBox(858, ry + 6, 68, 24, 3, perf ? UI_RGBA(200, 148, 24, 255) : UI_RGBA(184, 192, 176, 255),
                           perf ? UI_RGBA(255, 246, 216, 255) : UI_RGBA(232, 236, 224, 255),
                           perf ? UI_RGBA(255, 246, 216, 255) : UI_RGBA(232, 236, 224, 255), 0, 0);
                snprintf(buf, sizeof(buf), "IV %u", (unsigned)s.ivs[i]);
                UiW_GbaTextCentred(UI_FACE_VT323, 24, 858, ry + 6, 68, 24, buf, perf ? UI_RGBA(168, 108, 0, 255) : UI_RGBA(64, 72, 64, 255), UI_GBA_SHAD_LIGHT);
            }
        }

        // Middle ITEM & ABILITY
        UiW_GbaSectionStrip(362, 328, 282, 24, UI_GBA_TAG_GOLD, "ITEM", NULL);
        UiW_GbaBox(362, 354, 282, 42, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        if (s.item != ITEM_NONE)
        {
            UiSpr_ItemIcon(s.item, 370, 362, 1);
            UiGame_ItemName(s.item, buf, sizeof(buf));
            UiW_GbaTextMid(UI_FACE_VT323, 28, 400, 354, 42, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        }
        else
            UiW_GbaTextMid(UI_FACE_VT323, 28, 374, 354, 42, "NONE", UI_RGBA(120, 128, 120, 255), UI_GBA_SHAD_LIGHT);

        UiW_GbaSectionStrip(654, 328, 282, 24, UI_GBA_TAG_TEAL, "ABILITY", s.ability);
        UiW_GbaBox(654, 354, 282, 42, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        UiDraw_PushClip(660, 356, 270, 38);
        UiW_GbaTextMid(UI_FACE_VT323, 24, 662, 354, 42, s.abilityDesc, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        UiDraw_PopClip();

        // Bottom 2x2 KNOWN MOVES pills
        UiW_GbaSectionStrip(362, 402, 574, 24, UI_GBA_TAG_MAGENTA, "KNOWN MOVES", "TAP MOVE FOR DETAILS");
        for (int32_t k = 0; k < 4; k++)
        {
            const int32_t mx = 362 + (k & 1) * 290;
            const int32_t my = 430 + (k >> 1) * 44;
            UiW_GbaBox(mx, my, 284, 40, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
            UiMove mv;
            UiGame_Move(sParty.sel, k, &mv);
            if (!mv.move)
            {
                UiW_GbaTextCentred(UI_FACE_VT323, 24, mx, my, 284, 40, "---", UI_RGBA(140, 148, 140, 255), UI_GBA_SHAD_LIGHT);
                continue;
            }
            UiDraw_Sprite((uint16_t)(UI_SPR_TYPE_FIRST + mv.type), mx + 8, my + 4, 2);
            UiDraw_PushClip(mx + 76, my + 2, 134, 36);
            UiW_GbaTextMid(UI_FACE_VT323, 28, mx + 76, my, 40, mv.name, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            UiDraw_PopClip();
            snprintf(buf, sizeof(buf), "%u/%u", (unsigned)mv.pp, (unsigned)mv.maxPp);
            UiW_GbaTextMid(UI_FACE_VT323, 24, mx + 276 - UiText_Width(UI_FACE_VT323, 24, buf), my, 40, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        }
    }
    else if (sTab == TAB_INFO)
    {
        snprintf(buf, sizeof(buf), "No.%03u", (unsigned)s.dexNo);
        UiW_GbaSectionStrip(362, 70, 574, 28, UI_GBA_TAG_BLUE, "POKéMON INFO", buf);
        static const char *const kInfoKeys[5] = { "DEX NO.", "SPECIES", "OT / ID", "NATURE", "HELD ITEM" };
        UiW_GbaBox(362, 104, 574, 202, 4, UI_RGBA(40, 48, 48, 255), C_WHITE, C_WHITE, 0, 0);
        for (int32_t r = 0; r < 5; r++)
        {
            const int32_t ry = 106 + r * 40;
            UiDraw_Rect(364, ry, 156, 40, UI_RGBA(200, 216, 120, 255));
            UiDraw_Rect(520, ry, 2, 40, UI_RGBA(136, 152, 64, 255));
            UiDraw_Rect(522, ry, 412, 40, (r & 1) ? UI_RGBA(241, 242, 228, 255) : UI_RGBA(250, 248, 236, 255));
            if (r < 4)
                UiDraw_Rect(364, ry + 39, 570, 1, UI_RGBA(150, 172, 156, 220));
            UiW_GbaTextMid(UI_FACE_PS2P, 16, 374, ry, 40, kInfoKeys[r], UI_RGBA(40, 48, 24, 255), UI_RGBA(232, 240, 176, 255));
            if (r == 0) snprintf(buf, sizeof(buf), "No.%03u", (unsigned)s.dexNo);
            else if (r == 1) snprintf(buf, sizeof(buf), "%s", s.species);
            else if (r == 2) snprintf(buf, sizeof(buf), "%s / %05u", s.ot, (unsigned)s.otId);
            else if (r == 3) snprintf(buf, sizeof(buf), "%s", s.nature);
            else if (s.item != ITEM_NONE) UiGame_ItemName(s.item, buf, sizeof(buf));
            else snprintf(buf, sizeof(buf), "NONE");
            UiW_GbaTextMid(UI_FACE_VT323, 32, 534, ry, 40, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        }

        UiW_GbaSectionStrip(362, 316, 574, 26, UI_GBA_TAG_GOLD, "EXPERIENCE", NULL);
        UiW_GbaBox(362, 348, 574, 172, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        snprintf(buf, sizeof(buf), "EXP. POINTS: %u", (unsigned)s.exp);
        UiW_GbaText(UI_FACE_VT323, 32, 380, 366, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
        snprintf(buf, sizeof(buf), "TO NEXT LV.: %u", (unsigned)s.expToNext);
        UiW_GbaText(UI_FACE_VT323, 32, 380, 406, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
        UiW_GbaExpBar(380, 456, 536, s.expPct, NULL);
    }
    else
    {
        const bool contest = (sTab == TAB_CONTEST);
        const bool focus = (sMode == MODE_MOVES);
        UiW_GbaSectionStrip(362, 70, 574, 28, UI_GBA_TAG_MAGENTA, contest ? "CONTEST MOVES" : "BATTLE MOVES",
                            UiGame_InBattleMenu() ? "TAP TO INSPECT" : "X / SQ REORDER");
        UiMove sel;
        memset(&sel, 0, sizeof(sel));
        for (int32_t i = 0; i < 4; i++)
        {
            UiMove mv;
            UiGame_Move(sParty.sel, i, &mv);
            const int32_t ry = 104 + i * 54;
            const bool on = focus ? (i == sMoveSel) : (i == sMoveSel && mv.move != 0);
            if (on)
            {
                const uint32_t edge = (focus && i == sMoveSwap) ? UI_RGBA(216, 128, 16, 255) : UI_RGBA(230, 148, 65, 255);
                UiW_GbaBox(362, ry, 574, 50, 4, edge, UI_RGBA(255, 246, 205, 255), UI_RGBA(255, 246, 205, 255), C_WHITE, 0);
                UiW_GbaArrowH(373, ry + 20, true, UI_GBA_SHAD_LIGHT);
                UiW_GbaArrowH(372, ry + 19, true, UI_RGBA(230, 8, 8, 255));
            }
            else
            {
                const uint32_t rowBg = (i & 1) ? UI_RGBA(241, 242, 228, 255) : UI_RGBA(250, 248, 236, 255);
                UiW_GbaBox(362, ry, 574, 50, 4, UI_RGBA(150, 172, 156, 255), rowBg, rowBg, UI_GBA_SHAD_LIGHT, 0);
            }
            if (!mv.move)
            {
                UiW_GbaTextMid(UI_FACE_VT323, 32, 400, ry, 50, "---", UI_RGBA(140, 148, 140, 255), UI_GBA_SHAD_LIGHT);
                continue;
            }
            if (i == sMoveSel)
                sel = mv;
            UiDraw_Sprite((uint16_t)(contest ? UI_SPR_CONTEST_FIRST + mv.contestCategory : UI_SPR_TYPE_FIRST + mv.type), 390, ry + 9, 2);
            UiW_GbaTextMid(UI_FACE_VT323, 32, 468, ry, 50, mv.name, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
            if (contest)
                Hearts(822, ry + 20, mv.appeal, false);
            else
            {
                snprintf(buf, sizeof(buf), "PP %u/%u", (unsigned)mv.pp, (unsigned)mv.maxPp);
                UiW_GbaTextMid(UI_FACE_VT323, 28, 922 - UiText_Width(UI_FACE_VT323, 28, buf), ry, 50, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
            }
        }
        if (sel.move)
        {
            static const char *const cats[3] = { "PHYSICAL", "SPECIAL", "STATUS" };
            if (contest)
                snprintf(buf, sizeof(buf), "APPEAL %u · JAM %u", (unsigned)sel.appeal, (unsigned)sel.jam);
            else
                snprintf(buf, sizeof(buf), "PWR %u · ACC %u · %s", (unsigned)sel.power, (unsigned)sel.accuracy, cats[sel.category]);
            UiW_GbaSectionStrip(362, 326, 574, 26, UI_GBA_TAG_TEAL, "MOVE DATA", buf);
            UiW_GbaBox(362, 356, 574, 164, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
            UiW_GbaTextWrapped(UI_FACE_VT323, 32, 376, 368, 546, 30, contest ? sel.contestDesc : sel.desc, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 4);
        }
    }
}

static void PartyBuildGba(void)
{
    if (sSummaryOpen)
    {
        BuildGbaSummary();
        if (sPop.open)
            UiMenu_Draw(&sPop);
        return;
    }

    const int32_t n = UiGame_PartyCount();
    UiW_GbaTopBar(UI_GBA_BG_PARTY, UI_SPR_BALL_POKE, "POKéMON PARTY", 0, 0, false, NULL);
    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "BACK", false);
    UiW_GbaHelpBadge(&rx, "SQ", "SUMMARY", false);
    if (!UiGame_InBattleMenu() && n > 1)
        UiW_GbaHelpBadge(&rx, "TRI", "SWITCH", false);
    UiW_GbaHelpBadge(&rx, "X", "OPTIONS", false);

    for (int32_t i = 0; i < 6; i++)
    {
        if (i >= n)
        {
            int32_t x, y, w, h;
            GbaSlotRect(i, &x, &y, &w, &h);
            UiW_Bevel(x, y, w, h, 5, UI_RGBA(16, 24, 28, 255), UI_RGBA(31, 51, 56, 255), UI_RGBA(24, 40, 44, 255), 0, 0, 0, 0);
            continue;
        }
        const bool on = (i == sParty.sel);
        const bool mark = ((sMode == MODE_SWITCH || sMode == MODE_SHARE) && i == sFrom);
        const bool inBattle = UiGame_InBattleMenu() && UiGame_MonIsInBattle(i);
        const bool partner = UiGame_InBattleMenu() && UiGame_IsMultiPartner(i);
        DrawGbaPartySlot(i, on, mark, inBattle ? "IN BATTLE" : partner ? "PARTNER" : NULL, !partner, partner);
    }

    BuildGbaInspectPanel(sParty.sel);

    UiW_GbaMsg(14, 468, 762, 68, false, false);
    const char *msg = sMode == MODE_SWITCH ? "Move POKéMON to which slot?"
                    : sMode == MODE_SHARE  ? "Share HP with which POKéMON?"
                                           : "Choose a POKéMON, or tap PROFILE for Summary.";
    UiW_GbaTextMid(UI_FACE_VT323, 32, 34, 468, 68, msg, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    DrawGbaCancelBtn(788, 468, 158, 68);
    if (sPop.open)
        UiMenu_Draw(&sPop);
}

static void PartyBuild(void)
{
    if (gUiTheme->gbaFrame)
    {
        PartyBuildGba();
        return;
    }
    UiKit_Header(sViewOnly ? "SUMMARY" : "POKéMON", sTabs, TAB_COUNT, sTab, sTabX, sTabW, 12);
    UiW_Panel(16, 60, 320, 432);
    // Six cards: the party, or a window of the box being looked at (the list scrolls)
    const int32_t n = UiGame_ViewCount();
    for (int32_t i = sParty.first; i < n && i < sParty.first + 6; i++)
    {
        const int32_t y = CARD_Y + (i - sParty.first) * CARD_H;
        const bool on = i == sParty.sel;
        const bool inBattle = UiGame_InBattleMenu() && UiGame_MonIsInBattle(i);
        const bool partner = UiGame_InBattleMenu() && UiGame_IsMultiPartner(i);
        DrawCard(i, y, on && sMode != MODE_MOVES, inBattle ? "IN BATTLE" : partner ? "PARTNER" : NULL, !partner);
        if (partner)
            UiDraw_Rect(CARD_X, y, CARD_W, CARD_H - 2, UI_RGBA(0, 0, 0, 70));   // the partner's: greyed
        if ((sMode == MODE_SWITCH || sMode == MODE_SHARE) && i == sFrom)
            UiDraw_Frame(CARD_X, y, CARD_W, CARD_H - 2, 3, UI_COL_SELECT);
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
        if (sTab >= TAB_MOVES || !sViewOnly)
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
            if (id == 1)
            {
                uint16_t prev = ITEM_NONE;
                if (UiGame_GiveItem(slot, sItem, &prev))
                {
                    PlaySE(SE_SELECT);
                    UiKit_Toast("Swapped. The old item went back to the BAG.");
                    UiNav_Pop();
                }
                else
                {
                    PlaySE(SE_FAILURE);
                    UiKit_Toast(ItemIsMail(prev) ? "Please remove the MAIL." : "The BAG is full. The POKéMON's item couldn't be taken.");
                }
            }
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
    int32_t a = -1;
    if (gUiTheme->gbaFrame)
    {
        if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 788, 468, 158, 68))
        {
            PlaySE(SE_SELECT);
            return false;
        }
        const int32_t pCount = UiGame_PartyCount();
        if (pCount <= 0)
            return true;
        const int32_t before = sTList.sel;
        if (in->repeat & VITA_BTN_UP) sTList.sel = (sTList.sel + pCount - 1) % pCount;
        if (in->repeat & VITA_BTN_DOWN) sTList.sel = (sTList.sel + 1) % pCount;
        if (in->repeat & VITA_BTN_LEFT) sTList.sel = 0;
        if ((in->repeat & VITA_BTN_RIGHT) && pCount > 1 && sTList.sel == 0) sTList.sel = 1;
        if (sTList.sel != before) PlaySE(SE_SELECT);
        if (in->pressed & VITA_BTN_CROSS)
            a = sTList.sel;
        for (int32_t i = 0; i < pCount; i++)
        {
            int32_t sx, sy, sw, sh;
            GbaSlotRect(i, &sx, &sy, &sw, &sh);
            if (UiHit(in, sx, sy, sw, sh))
            {
                sTList.sel = i;
                a = i;
                break;
            }
        }
    }
    else
    {
        // Circle or the BACK button: back to the bag without using / giving anything
        if ((in->pressed & VITA_BTN_CIRCLE) || UiHit(in, 364, 436, 200, 40))
        {
            PlaySE(SE_SELECT);
            return false;
        }
        a = UiList_Update(&sTList, in, CARD_X, CARD_Y, CARD_W, CARD_H, true);
    }
    if (a < 0)
        return true;
    UiMon m;
    UiGame_Mon(a, &m);
    char item[24];
    UiGame_ItemName(sItem, item, sizeof(item));
    if (sTMode == UI_TARGET_GIVE)
    {
        if (m.egg) { PlaySE(SE_FAILURE); UiKit_Toast("An EGG can't hold items."); return true; }
        if (ItemIsMail(sItem))
        {
            if (m.item != ITEM_NONE)
            {
                PlaySE(SE_FAILURE);
                UiKit_Toast("Can't transfer MAIL to a POKéMON holding an item.");
                return true;
            }
            void (*after)(void) = NULL;
            if (UiGame_GiveMailBegin(a, sItem, &after))
            {
                PlaySE(SE_SELECT);
                UiNav_Close(after);
            }
            else
            {
                PlaySE(SE_FAILURE);
                UiKit_Toast("No room for MAIL.");
            }
            return true;
        }
        if (m.item != ITEM_NONE)
        {
            if (ItemIsMail(m.item))
            {
                PlaySE(SE_FAILURE);
                UiKit_Toast("Please remove the MAIL.");
                return true;
            }
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
        uint16_t prev = ITEM_NONE;
        if (!UiGame_GiveItem(a, sItem, &prev))
        {
            PlaySE(SE_FAILURE);
            UiKit_Toast("The BAG is full. The POKéMON's item couldn't be taken.");
            return true;
        }
        PlaySE(SE_SELECT);
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

static void DrawTargetDeltaModal(void)
{
    static const char *const names[6] = { "HP", "ATTACK", "DEFENSE", "SP. ATK", "SP. DEF", "SPEED" };
    char buf[96];
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

static void TargetBuildGba(void)
{
    char item[24], buf[128];
    UiGame_ItemName(sItem, item, sizeof(item));
    const int32_t n = UiGame_PartyCount();
    UiW_GbaTopBar(UI_GBA_BG_PARTY, UI_SPR_BALL_POKE, "CHOOSE A POKéMON", 0, 0, false, NULL);
    int32_t rx = UI_SCREEN_W - 14;
    UiW_GbaHelpBadge(&rx, "O", "BACK", false);
    UiW_GbaHelpBadge(&rx, "X", sTMode == UI_TARGET_GIVE ? "GIVE" : sTMode == UI_TARGET_TM ? "TEACH" : "USE", false);

    for (int32_t i = 0; i < 6; i++)
    {
        if (i >= n)
        {
            int32_t x, y, w, h;
            GbaSlotRect(i, &x, &y, &w, &h);
            UiW_Bevel(x, y, w, h, 5, UI_RGBA(16, 24, 28, 255), UI_RGBA(31, 51, 56, 255), UI_RGBA(24, 40, 44, 255), 0, 0, 0, 0);
            continue;
        }
        const char *tag = NULL;
        bool ok = true;
        if (sTMode == UI_TARGET_TM)
        {
            bool known = false;
            ok = UiGame_CanLearnTm(i, sItem, &known);
            tag = known ? "LEARNED" : ok ? "ABLE" : "NO";
        }
        DrawGbaPartySlot(i, i == sTList.sel, false, tag, ok, false);
    }

    UiW_GbaWin(582, 60, 364, 402, UI_GBA_WIN_DEFAULT);
    const char *modeLabel = sTMode == UI_TARGET_GIVE ? "GIVE ITEM"
                          : sTMode == UI_TARGET_TM   ? "TM / HM"
                          : sTMode == UI_TARGET_BATTLE ? "BATTLE ITEM"
                                                     : "USE ITEM";
    UiW_GbaSectionStrip(592, 70, 344, 26, UI_GBA_TAG_GOLD, modeLabel, item);

    UiW_GbaBox(592, 102, 62, 62, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiSpr_ItemIcon(sItem, 599, 109, 2);
    UiW_GbaText(UI_FACE_PS2P, 16, 666, 106, item, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    if (sTMode == UI_TARGET_TM)
    {
        char mv[16];
        UiGame_MoveName(UiGame_ItemMove(sItem), mv, sizeof(mv));
        UiW_GbaText(UI_FACE_VT323, 28, 666, 134, mv, UI_RGBA(32, 104, 168, 255), UI_GBA_SHAD_LIGHT);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Select target slot");
        UiW_GbaText(UI_FACE_VT323, 24, 666, 136, buf, UI_RGBA(82, 88, 100, 255), UI_GBA_SHAD_LIGHT);
    }

    UiW_GbaBox(592, 172, 344, 154, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
    UiGame_ItemDescription(sItem, buf, sizeof(buf));
    UiW_GbaTextWrapped(UI_FACE_VT323, 32, 604, 182, 320, 28, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 5);

    UiMon tm;
    if (UiGame_Mon(sTList.sel, &tm))
    {
        UiW_GbaSectionStrip(592, 336, 344, 26, UI_GBA_TAG_TEAL, "TARGET POKéMON", tm.name);
        UiW_GbaBox(592, 366, 344, 84, 4, UI_RGBA(150, 172, 156, 255), UI_RGBA(244, 245, 236, 255), UI_RGBA(244, 245, 236, 255), UI_GBA_SHAD_LIGHT, 0);
        if (sTMode == UI_TARGET_GIVE)
        {
            if (tm.item != ITEM_NONE)
            {
                char old[24];
                UiGame_ItemName(tm.item, old, sizeof(old));
                snprintf(buf, sizeof(buf), "Currently holding: %s", old);
            }
            else
                snprintf(buf, sizeof(buf), "Not holding any item.");
            UiW_GbaTextWrapped(UI_FACE_VT323, 28, 604, 378, 320, 28, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
        }
        else if (sTMode == UI_TARGET_TM)
        {
            bool known = false;
            const bool able = UiGame_CanLearnTm(sTList.sel, sItem, &known);
            UiW_GbaTextWrapped(UI_FACE_VT323, 28, 604, 378, 320, 28,
                               known ? "Already knows this move." : able ? "Compatible! Can learn this move." : "Incompatible with this move.",
                               able ? UI_RGBA(24, 128, 72, 255) : UI_RGBA(176, 40, 40, 255), UI_GBA_SHAD_LIGHT, 2);
        }
        else
        {
            snprintf(buf, sizeof(buf), "HP %u / %u  (Lv%u)", (unsigned)tm.hp, (unsigned)tm.maxHp, (unsigned)tm.level);
            UiW_GbaTextWrapped(UI_FACE_VT323, 28, 604, 378, 320, 28, buf, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT, 2);
        }
    }

    UiW_GbaMsg(14, 468, 762, 68, false, false);
    const char *hint = sTMode == UI_TARGET_GIVE ? "Give item to which POKéMON?"
                     : sTMode == UI_TARGET_TM   ? "Teach move to which POKéMON?"
                                                : "Use item on which POKéMON?";
    UiW_GbaTextMid(UI_FACE_VT323, 32, 34, 468, 68, hint, UI_RGBA(41, 49, 49, 255), UI_GBA_SHAD_LIGHT);
    DrawGbaCancelBtn(788, 468, 158, 68);
    if (sMoveMenu.open)
        UiMenu_Draw(&sMoveMenu);
    if (sDeltaOpen)
        DrawTargetDeltaModal();
}

static void TargetBuild(void)
{
    if (gUiTheme->gbaFrame)
    {
        TargetBuildGba();
        return;
    }
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
        DrawTargetDeltaModal();
}

static void TargetResume(void)
{
    UiList_Reset(&sTList, UiGame_PartyCount(), PARTY_SIZE);
}

const UiScreen gUiTargetScreen = { TargetEnter, TargetUpdate, TargetBuild, TargetResume };
