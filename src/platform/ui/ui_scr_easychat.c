// Easy Chat of the new UI (design/stitch/correo): mail, profile, battle messages, interviews,
// trendy and good sayings, the questionnaire... (every type but the BARD's song and the quiz
// screens, which keep the GBA screen). Hosted: DoEasyChatScreen hands it over and the UI runs the
// game loop until the screen ends, then sets the exit callback. Words, unlocked groups, the checks
// on OK / CANCEL and the effects of saving come from easy_chat.c itself (EasyChat_Ui*).
//
// Two levels, as in the game: the MESSAGE (word slots + DEL ALL / CANCEL / OK) and the WORDS
// (groups or A-Z on the left, words on the right). L / R switch GROUPS / A-Z (the GBA's SELECT).

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "easy_chat.h"
#include "main.h"
#include "sound.h"
#include "constants/easy_chat.h"
#include "constants/songs.h"

extern bool8 EasyChat_UiSupported(u8 type);
extern bool8 EasyChat_UiBegin(u8 type, u16 *words, u8 displayedPersonType);
extern u8 EasyChat_UiColumns(void);
extern u8 EasyChat_UiMaxWords(void);
extern u16 *EasyChat_UiPhrase(void);
extern const u8 *EasyChat_UiTitle(void);
extern void EasyChat_UiInstructions(const u8 **a, const u8 **b);
extern void EasyChat_UiDeleteAllText(const u8 **a, const u8 **b);
extern u8 EasyChat_UiGroupCount(void);
extern u8 EasyChat_UiGroupId(u8 index);
extern const u8 *EasyChat_UiGroupName(u8 groupId);
extern u16 EasyChat_UiSelectGroup(bool8 alphabet, u16 group);
extern u16 EasyChat_UiWordAt(u16 index);
extern u16 EasyChat_UiAlphabetCount(u8 letter);
extern u8 EasyChat_UiTryConfirm(const u8 **t1, const u8 **t2);
extern u8 EasyChat_UiTryQuit(const u8 **t1, const u8 **t2);
extern void EasyChat_UiFinish(bool8 confirmed);

enum { AT_MSG, AT_GROUPS, AT_WORDS };
enum { BTN_DELALL, BTN_CANCEL, BTN_OK, BTN_COUNT };
enum { ASK_NONE, ASK_CONFIRM, ASK_QUIT, ASK_DELALL, ASK_MSG };

#define MSG_X 16
#define MSG_Y 60
#define MSG_W 928
#define SLOT_H 44
#define BTN_W 160
#define LOW_Y 252
#define GROUP_W 300
#define WORD_COLS 3
#define WORD_ROWS 5
#define ROW_H 40

static void (*sExit)(void);
static int32_t sAt, sSlot, sBtn;          // sBtn >= 0: the cursor is on a button of the message
static bool sAlpha;
static UiList sGroups;
static int32_t sGroupIds[EC_NUM_GROUPS + EC_NUM_ALPHABET_GROUPS];
static int32_t sGroupCount, sGroup;
static int32_t sWordCount, sWordSel, sWordFirst;
static int32_t sAsk;
static UiMenu sYesNo;
static char sAskText[2][120];
static int32_t sTabX[2], sTabW[2];

static void Text(const u8 *gba, char *out, size_t cap)
{
    if (gba == NULL) { out[0] = 0; return; }
    UiGame_Text(gba, out, cap);
}

static void WordText(u16 word, char *out, size_t cap)
{
    if (word == EC_EMPTY_WORD) { snprintf(out, cap, "-----"); return; }
    u8 buf[64];
    CopyEasyChatWord(buf, word);
    UiGame_Text(buf, out, cap);
}

static void Finish(void)
{
    if (sExit != NULL)
        SetMainCallback2(sExit);
}

static void End(bool confirmed)
{
    EasyChat_UiFinish(confirmed);
    UiNav_Close(Finish);
}

static void Ask(int32_t kind, const u8 *t1, const u8 *t2)
{
    Text(t1, sAskText[0], sizeof(sAskText[0]));
    Text(t2, sAskText[1], sizeof(sAskText[1]));
    sAsk = kind;
    if (kind != ASK_MSG)
    {
        UiMenu_Open(&sYesNo, "EASY CHAT", 700, 330, 220);
        UiMenu_Add(&sYesNo, "YES", 1);
        UiMenu_Add(&sYesNo, "NO", UI_MENU_CANCEL);
        sYesNo.list.sel = kind == ASK_CONFIRM ? 0 : 1;
    }
}

static void TryOk(void)
{
    const u8 *a, *b;
    const u8 r = EasyChat_UiTryConfirm(&a, &b);
    Ask(r == 0 ? ASK_CONFIRM : r == 1 ? ASK_QUIT : ASK_MSG, a, b);
    PlaySE(SE_SELECT);
}

static void TryQuit(void)
{
    const u8 *a, *b;
    Ask(EasyChat_UiTryQuit(&a, &b) == 1 ? ASK_QUIT : ASK_MSG, a, b);
    PlaySE(SE_SELECT);
}

static void BuildGroups(void)
{
    sGroupCount = 0;
    if (sAlpha)
    {
        for (int32_t i = 0; i < EC_NUM_ALPHABET_GROUPS; i++)
            if (EasyChat_UiAlphabetCount((u8)i) > 0)
                sGroupIds[sGroupCount++] = i;
    }
    else
        for (int32_t i = 0; i < EasyChat_UiGroupCount(); i++)
            sGroupIds[sGroupCount++] = EasyChat_UiGroupId((u8)i);
    UiList_Reset(&sGroups, sGroupCount, 5);
}

static void SelectGroup(int32_t index)
{
    sGroup = index;
    sWordCount = EasyChat_UiSelectGroup(sAlpha, (u16)sGroupIds[index]);
    sWordSel = sWordFirst = 0;
}

static void GroupLabel(int32_t index, char *out, size_t cap)
{
    if (sAlpha)
    {
        const int32_t g = sGroupIds[index];
        if (g == 0) snprintf(out, cap, "OTHERS");
        else snprintf(out, cap, "%c", 'A' + g - 1);
    }
    else
        Text(EasyChat_UiGroupName((u8)sGroupIds[index]), out, cap);
}

bool Ui_OpenEasyChat(uint8_t type, uint16_t *words, void (*exitCallback)(void), uint8_t personType)
{
    if (!Ui_IsEnabled() || Ui_IsOpen() || !EasyChat_UiSupported(type) || !EasyChat_UiBegin(type, words, personType))
        return false;
    sExit = exitCallback;
    UiCore_OpenHosted(&gUiEasyChatScreen, NULL);
    return true;
}

static void Enter(void)
{
    sAt = AT_MSG;
    sBtn = -1;
    sAlpha = false;
    sAsk = ASK_NONE;
    u16 *p = EasyChat_UiPhrase();
    sSlot = 0;
    for (int32_t i = 0; i < EasyChat_UiMaxWords(); i++)
        if (p[i] == EC_EMPTY_WORD) { sSlot = i; break; }
    BuildGroups();
    SelectGroup(0);
}

static void SetAlpha(bool alpha)
{
    if (alpha == sAlpha)
        return;
    sAlpha = alpha;
    BuildGroups();
    SelectGroup(0);
    PlaySE(SE_SELECT);
}

static void PutWord(u16 word)
{
    u16 *p = EasyChat_UiPhrase();
    p[sSlot] = word;
    for (int32_t i = 1; i <= EasyChat_UiMaxWords(); i++)       // next empty slot, as the game does
    {
        const int32_t j = (sSlot + i) % EasyChat_UiMaxWords();
        if (p[j] == EC_EMPTY_WORD) { sSlot = j; break; }
    }
    sAt = AT_MSG;
    PlaySE(SE_SELECT);
}

static bool UpdateAsk(const UiInput *in)
{
    if (sAsk == ASK_MSG)
    {
        if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap)
            sAsk = ASK_NONE;
        return true;
    }
    const int32_t id = UiMenu_Update(&sYesNo, in);
    if (id == UI_MENU_NONE)
        return true;
    const int32_t kind = sAsk;
    sAsk = ASK_NONE;
    if (id != 1)
        return true;
    if (kind == ASK_CONFIRM) End(TRUE);
    else if (kind == ASK_QUIT) End(FALSE);
    else if (kind == ASK_DELALL)
    {
        u16 *p = EasyChat_UiPhrase();
        for (int32_t i = 0; i < EasyChat_UiMaxWords(); i++)
            p[i] = EC_EMPTY_WORD;
        sSlot = 0;
    }
    return true;
}

static void PressButton(int32_t b)
{
    if (b == BTN_OK) TryOk();
    else if (b == BTN_CANCEL) TryQuit();
    else
    {
        const u8 *a, *c;
        EasyChat_UiDeleteAllText(&a, &c);
        Ask(ASK_DELALL, a, c);
        PlaySE(SE_SELECT);
    }
}

static bool Update(const UiInput *in)
{
    if (sAsk != ASK_NONE)
        return UpdateAsk(in);
    const int32_t max = EasyChat_UiMaxWords(), cols = EasyChat_UiColumns(), rows = (max + cols - 1) / cols;
    const int32_t slotW = (MSG_W - 32 - BTN_W - 16) / cols;

    if (in->pressed & (VITA_BTN_L | VITA_BTN_R)) { SetAlpha(!sAlpha); return true; }
    const int32_t t = UiKit_TabHit(in, sTabX, sTabW, 2);
    if (t >= 0) { SetAlpha(t == 1); return true; }
    if (in->pressed & VITA_BTN_START) { TryOk(); return true; }

    // Touch
    if (in->tap)
    {
        for (int32_t i = 0; i < max; i++)
        {
            const int32_t x = MSG_X + 16 + (i % cols) * slotW, y = MSG_Y + 16 + (i / cols) * (SLOT_H + 8);
            if (!UiHit(in, x, y, slotW - 8, SLOT_H)) continue;
            if (sAt == AT_MSG && sBtn < 0 && sSlot == i) sAt = AT_GROUPS;
            else { sAt = AT_MSG; sBtn = -1; sSlot = i; }
            PlaySE(SE_SELECT);
            return true;
        }
        for (int32_t b = 0; b < BTN_COUNT; b++)
            if (UiHit(in, MSG_X + MSG_W - 16 - BTN_W, MSG_Y + 16 + b * 50, BTN_W, 42)) { sAt = AT_MSG; sBtn = b; PressButton(b); return true; }
        for (int32_t r = 0; r < 5 && sGroups.first + r < sGroupCount; r++)
            if (UiHit(in, 28, LOW_Y + 52 + r * ROW_H, GROUP_W - 24, ROW_H))
            {
                const int32_t g = sGroups.first + r;
                if (sAt == AT_GROUPS && sGroups.sel == g) sAt = AT_WORDS;
                else { sAt = AT_GROUPS; sGroups.sel = g; SelectGroup(g); }
                PlaySE(SE_SELECT);
                return true;
            }
        const int32_t ww = (MSG_W - GROUP_W - 12 - 24) / WORD_COLS;
        for (int32_t k = 0; k < WORD_COLS * WORD_ROWS; k++)
        {
            const int32_t i = sWordFirst + k;
            if (i >= sWordCount) break;
            if (!UiHit(in, GROUP_W + 28 + (k % WORD_COLS) * ww, LOW_Y + 52 + (k / WORD_COLS) * ROW_H, ww, ROW_H)) continue;
            if (sAt == AT_WORDS && sWordSel == i) PutWord(EasyChat_UiWordAt((u16)i));
            else { sAt = AT_WORDS; sWordSel = i; PlaySE(SE_SELECT); }
            return true;
        }
        return true;
    }

    const int32_t dx = (in->repeat & VITA_BTN_RIGHT) ? 1 : (in->repeat & VITA_BTN_LEFT) ? -1 : 0;
    const int32_t dy = (in->repeat & VITA_BTN_DOWN) ? 1 : (in->repeat & VITA_BTN_UP) ? -1 : 0;
    switch (sAt)
    {
    case AT_MSG:
        if (in->pressed & VITA_BTN_CIRCLE) { TryQuit(); return true; }
        if (in->pressed & VITA_BTN_TRIANGLE && sBtn < 0)
        {
            EasyChat_UiPhrase()[sSlot] = EC_EMPTY_WORD;               // delete the word (the GBA's DEL)
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->pressed & VITA_BTN_CROSS)
        {
            if (sBtn >= 0) PressButton(sBtn);
            else { sAt = AT_GROUPS; PlaySE(SE_SELECT); }
            return true;
        }
        if (dx || dy)
        {
            if (sBtn >= 0)
            {
                if (dx < 0) { sBtn = -1; sSlot = (sSlot / cols) * cols + cols - 1; if (sSlot >= max) sSlot = max - 1; }
                else if (dy) sBtn = (sBtn + dy + BTN_COUNT) % BTN_COUNT;
            }
            else
            {
                const int32_t c = sSlot % cols, r = sSlot / cols;
                if (dx > 0 && c == cols - 1) sBtn = r < BTN_COUNT ? r : BTN_COUNT - 1;
                else if (dx) sSlot = r * cols + (c + dx + cols) % cols;
                else sSlot = ((r + dy + rows) % rows) * cols + c;
                if (sSlot >= max) sSlot = max - 1;
            }
            PlaySE(SE_SELECT);
        }
        return true;
    case AT_GROUPS:
    {
        if (in->pressed & VITA_BTN_CIRCLE) { sAt = AT_MSG; PlaySE(SE_SELECT); return true; }
        const int32_t before = sGroups.sel;
        const int32_t a = UiList_Update(&sGroups, in, 28, LOW_Y + 52, GROUP_W - 24, ROW_H, true);
        if (sGroups.sel != before) SelectGroup(sGroups.sel);
        if (a >= 0 || (in->pressed & VITA_BTN_RIGHT)) { sAt = AT_WORDS; PlaySE(SE_SELECT); }
        return true;
    }
    default:
        if (in->pressed & VITA_BTN_CIRCLE) { sAt = AT_GROUPS; PlaySE(SE_SELECT); return true; }
        if (in->pressed & VITA_BTN_CROSS) { if (sWordCount) PutWord(EasyChat_UiWordAt((u16)sWordSel)); return true; }
        if (dx || dy)
        {
            if (dx < 0 && sWordSel % WORD_COLS == 0) { sAt = AT_GROUPS; PlaySE(SE_SELECT); return true; }
            int32_t n = sWordSel + dx + dy * WORD_COLS;
            if (n < 0) n = 0;
            if (n >= sWordCount) n = sWordCount - 1;
            sWordSel = n;
            if (sWordSel < sWordFirst) sWordFirst = (sWordSel / WORD_COLS) * WORD_COLS;
            if (sWordSel >= sWordFirst + WORD_COLS * WORD_ROWS) sWordFirst = (sWordSel / WORD_COLS - WORD_ROWS + 1) * WORD_COLS;
            PlaySE(SE_SELECT);
        }
        return true;
    }
}

static void Build(void)
{
    static const char *const tabs[2] = { "GROUPS", "A-Z" };
    char buf[96], title[48];
    Text(EasyChat_UiTitle(), title, sizeof(title));
    UiKit_Header("EASY CHAT", tabs, 2, sAlpha ? 1 : 0, sTabX, sTabW, 12);
    if (title[0])
        UiW_HeaderValue(title, UI_COL_ON_FRAME);

    // Message
    const int32_t max = EasyChat_UiMaxWords(), cols = EasyChat_UiColumns(), rows = (max + cols - 1) / cols;
    const int32_t slotW = (MSG_W - 32 - BTN_W - 16) / cols;
    const int32_t msgH = 32 + (rows > 3 ? rows : 3) * (SLOT_H + 8) - 8;
    UiW_Panel(MSG_X, MSG_Y, MSG_W, msgH > 184 ? msgH : 184);
    const u16 *p = EasyChat_UiPhrase();
    for (int32_t i = 0; i < max; i++)
    {
        const int32_t x = MSG_X + 16 + (i % cols) * slotW, y = MSG_Y + 16 + (i / cols) * (SLOT_H + 8);
        const bool on = sAt == AT_MSG && sBtn < 0 && i == sSlot;
        const bool target = sAt != AT_MSG && i == sSlot;
        UiW_Row(x, y, slotW - 8, SLOT_H, on, 0);
        if (target)
            UiDraw_Frame(x, y, slotW - 8, SLOT_H, 3, UI_COL_SELECT);
        WordText(p[i], buf, sizeof(buf));
        const uint32_t ink = p[i] == EC_EMPTY_WORD ? UI_COL_INK_SOFT : on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + (slotW - 8 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf)) / 2, y + 4, buf, ink);
    }
    static const char *const btn[BTN_COUNT] = { "DEL ALL", "CANCEL", "OK" };
    static const uint8_t col[BTN_COUNT] = { UI_BTN_RED, UI_BTN_AMBER, UI_BTN_GREEN };
    for (int32_t b = 0; b < BTN_COUNT; b++)
    {
        const int32_t x = MSG_X + MSG_W - 16 - BTN_W, y = MSG_Y + 16 + b * 50;
        if (sAt == AT_MSG && sBtn == b)
            UiDraw_Rect(x - 4, y - 4, BTN_W + 8, 48, UI_COL_SELECT);
        UiW_Button(x, y, BTN_W, btn[b], col[b], false);
    }

    // Groups (or letters) and words
    UiW_Panel(16, LOW_Y, GROUP_W, 240);
    UiW_PanelHead(16, LOW_Y, GROUP_W, sAlpha ? "A-Z" : "GROUPS", NULL);
    for (int32_t r = 0; r < 5 && sGroups.first + r < sGroupCount; r++)
    {
        const int32_t g = sGroups.first + r, y = LOW_Y + 52 + r * ROW_H;
        const bool on = g == sGroups.sel;
        UiW_Row(28, y, GROUP_W - 24, ROW_H, on && sAt == AT_GROUPS, g);
        if (on && sAt != AT_GROUPS)
            UiDraw_Frame(28, y, GROUP_W - 24, ROW_H - 2, 2, UI_COL_SELECT);
        GroupLabel(g, buf, sizeof(buf));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 56, y + 2, buf, on && sAt == AT_GROUPS ? UI_COL_SELECT_INK : UI_COL_INK);
    }
    UiW_ScrollBar(16 + GROUP_W - 12, LOW_Y + 52, 5 * ROW_H, sGroups.first, 5, sGroupCount);

    const int32_t wx = 16 + GROUP_W + 12, ww = (MSG_W - GROUP_W - 12 - 24) / WORD_COLS;
    UiW_Panel(wx, LOW_Y, MSG_W - GROUP_W - 12, 240);
    GroupLabel(sGroup, buf, sizeof(buf));
    char right[24];
    snprintf(right, sizeof(right), "%d words", (int)sWordCount);
    UiW_PanelHead(wx, LOW_Y, MSG_W - GROUP_W - 12, buf, right);
    for (int32_t k = 0; k < WORD_COLS * WORD_ROWS; k++)
    {
        const int32_t i = sWordFirst + k;
        if (i >= sWordCount) break;
        const int32_t x = wx + 12 + (k % WORD_COLS) * ww, y = LOW_Y + 52 + (k / WORD_COLS) * ROW_H;
        const bool on = sAt == AT_WORDS && i == sWordSel;
        UiW_Row(x, y, ww - 4, ROW_H, on, k / WORD_COLS);
        WordText(EasyChat_UiWordAt((u16)i), buf, sizeof(buf));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 24, y + 2, buf, on ? UI_COL_SELECT_INK : UI_COL_INK);
    }
    UiW_ScrollBar(wx + MSG_W - GROUP_W - 12 - 12, LOW_Y + 52, WORD_ROWS * ROW_H, sWordFirst / WORD_COLS, WORD_ROWS,
                  (sWordCount + WORD_COLS - 1) / WORD_COLS);

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Choose");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
    UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Delete word");
    UiW_FooterKey(&fx, "START", "OK");
    UiKit_FooterClock();

    if (sAsk != ASK_NONE)
    {
        UiDraw_Darken(1, 3);
        UiW_DialogBox(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 112, sAskText[0], UI_COL_INK);
        UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 66, sAskText[1], UI_COL_INK);
        if (sAsk != ASK_MSG)
            UiMenu_Draw(&sYesNo);
    }
}

const UiScreen gUiEasyChatScreen = { Enter, Update, Build, NULL };
