// Easy Chat of the new UI (design/stitch/correo): every type of the game, from mail, the profile and
// the interviews to the BARD's song and the quiz lady's screens. Hosted: DoEasyChatScreen hands it
// over and the UI runs the game loop until the screen ends, then sets the exit callback. Words,
// unlocked groups, the checks on OK / CANCEL / DEL ALL, the rules of each type and the effects of
// saving come from easy_chat.c itself (EasyChat_Ui*).
//
// Two levels, as in the game: the MESSAGE (word slots + its buttons) and the WORDS (groups or A-Z on
// the left, words on the right). L / R switch GROUPS / A-Z (the GBA's SELECT). The rules by type:
// - the BARD's song: no word can be deleted and each new word asks at once if the song is right;
// - the quiz lady: a fourth button goes to the other screen of the pair (QUIZ / ANSWER); the quiz
//   question being answered can only be read (✕ goes to the answer).
// The layout follows the message: a 2x5 message takes more height and the word lists get less.

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

extern bool8 EasyChat_UiBegin(u8 type, u16 *words, u8 displayedPersonType);
extern u8 EasyChat_UiColumns(void);
extern u8 EasyChat_UiMaxWords(void);
extern u16 *EasyChat_UiPhrase(void);
extern const u8 *EasyChat_UiTitle(void);
extern void EasyChat_UiInstructions(const u8 **a, const u8 **b);
extern void EasyChat_UiConfirmText(const u8 **a, const u8 **b);
extern u8 EasyChat_UiGroupCount(void);
extern u8 EasyChat_UiGroupId(u8 index);
extern const u8 *EasyChat_UiGroupName(u8 groupId);
extern u16 EasyChat_UiSelectGroup(bool8 alphabet, u16 group);
extern u16 EasyChat_UiWordAt(u16 index);
extern u16 EasyChat_UiAlphabetCount(u8 letter);
extern u8 EasyChat_UiTryConfirm(const u8 **t1, const u8 **t2);
extern u8 EasyChat_UiTryQuit(const u8 **t1, const u8 **t2);
extern u8 EasyChat_UiTryDeleteAll(const u8 **t1, const u8 **t2);
extern bool8 EasyChat_UiCanDeleteWord(void);
extern bool8 EasyChat_UiConfirmsEachWord(void);
extern void EasyChat_UiRevertLyrics(const u8 **t1, const u8 **t2);
extern u8 EasyChat_UiQuizTarget(void);
extern bool8 EasyChat_UiReadOnly(void);
extern bool8 EasyChat_UiQuizSwitch(void);
extern void EasyChat_UiFinish(bool8 confirmed);

enum { AT_MSG, AT_GROUPS, AT_WORDS };
enum { ASK_NONE, ASK_CONFIRM, ASK_QUIT, ASK_DELALL, ASK_LYRICS, ASK_MSG };

#define MSG_X 16
#define MSG_Y 60
#define MSG_W 928
#define BTN_W 160
#define BTN_PITCH 50
#define GROUP_W 300
#define WORD_COLS 3
#define ROW_H 40
#define LOW_BOTTOM 492             // the word panels end over the footer
#define LIST_TOP 52                // rows start under the panel's head
#define MAX_BUTTONS 4

// The message's buttons, by type (DEL ALL, CANCEL, OK, and the quiz lady's QUIZ / ANSWER)
typedef struct
{
    const char *label;
    uint8_t color;
    void (*press)(void);
} EcButton;

static void (*sExit)(void);
static int32_t sAt, sSlot, sBtn;          // sBtn >= 0: the cursor is on a button of the message
static bool sAlpha, sReadOnly;
static UiList sGroups;
static int32_t sGroupIds[EC_NUM_GROUPS + EC_NUM_ALPHABET_GROUPS];
static int32_t sGroupCount, sGroup;
static int32_t sWordCount, sWordSel, sWordFirst;
static int32_t sAsk;
static UiMenu sYesNo;
static char sAskText[2][120];
static int32_t sTabX[2], sTabW[2];
static EcButton sBtns[MAX_BUTTONS];
static int32_t sBtnCount;
// Layout of this type's message (Layout)
static int32_t sSlotPitch, sMsgH, sLowY, sListRows;

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

static int32_t Rows(void)
{
    const int32_t cols = EasyChat_UiColumns();
    return (EasyChat_UiMaxWords() + cols - 1) / cols;
}

static int32_t SlotW(void)
{
    return (MSG_W - 32 - BTN_W - 16) / EasyChat_UiColumns();
}

static void SlotPos(int32_t i, int32_t *x, int32_t *y)
{
    *x = MSG_X + 16 + (i % EasyChat_UiColumns()) * SlotW();
    *y = MSG_Y + 16 + (i / EasyChat_UiColumns()) * sSlotPitch;
}

// The message panel holds its rows and its buttons; the word panels take what is left
static void Layout(void)
{
    const int32_t rows = Rows();
    sSlotPitch = rows <= 3 ? 52 : 40;
    const int32_t words = 32 + rows * sSlotPitch - 8;
    const int32_t buttons = 32 + sBtnCount * BTN_PITCH - 8;
    sMsgH = words > buttons ? words : buttons;
    if (sMsgH < 184)
        sMsgH = 184;
    sLowY = MSG_Y + sMsgH + 8;
    sListRows = (LOW_BOTTOM - sLowY - LIST_TOP + 12) / ROW_H;
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
        sYesNo.list.sel = kind == ASK_CONFIRM || kind == ASK_LYRICS ? 0 : 1;
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

static void TryDeleteAll(void)
{
    const u8 *a, *b;
    Ask(EasyChat_UiTryDeleteAll(&a, &b) == 1 ? ASK_DELALL : ASK_MSG, a, b);
    PlaySE(SE_SELECT);
}

// The other screen of the quiz lady's pair: the same screen, entered again on the new type
static void QuizSwitch(void)
{
    PlaySE(SE_SELECT);
    if (EasyChat_UiQuizSwitch())
        UiNav_Replace(&gUiEasyChatScreen);
    else
        UiNav_Close(Finish);
}

static void BuildButtons(void)
{
    sBtnCount = 0;
    if (sReadOnly)
        return;                                       // the quiz question: ✕ / ○ only
    sBtns[sBtnCount++] = (EcButton){ "DEL ALL", UI_BTN_RED, TryDeleteAll };
    sBtns[sBtnCount++] = (EcButton){ "CANCEL", UI_BTN_AMBER, TryQuit };
    sBtns[sBtnCount++] = (EcButton){ "OK", UI_BTN_GREEN, TryOk };
    const u8 quiz = EasyChat_UiQuizTarget();
    if (quiz == EASY_CHAT_TYPE_QUIZ_QUESTION || quiz == EASY_CHAT_TYPE_QUIZ_SET_QUESTION)
        sBtns[sBtnCount++] = (EcButton){ "QUIZ", UI_BTN_AMBER, QuizSwitch };
    else if (quiz == EASY_CHAT_TYPE_QUIZ_SET_ANSWER)
        sBtns[sBtnCount++] = (EcButton){ "ANSWER", UI_BTN_AMBER, QuizSwitch };
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
    UiList_Reset(&sGroups, sGroupCount, sListRows);
}

static void SelectGroup(int32_t index)
{
    sGroup = index;
    sWordCount = sGroupCount > 0 ? EasyChat_UiSelectGroup(sAlpha, (u16)sGroupIds[index]) : 0;
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
    if (!Ui_IsEnabled() || Ui_IsOpen() || !EasyChat_UiBegin(type, words, personType))
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
    sReadOnly = EasyChat_UiReadOnly();
    BuildButtons();
    Layout();
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
    sAt = AT_MSG;
    PlaySE(SE_SELECT);
    if (EasyChat_UiConfirmsEachWord())
    {
        const u8 *a, *b;                              // the BARD: "The song... is as shown. Okay?"
        EasyChat_UiConfirmText(&a, &b);
        Ask(ASK_LYRICS, a, b);
        return;
    }
    for (int32_t i = 1; i <= EasyChat_UiMaxWords(); i++)       // next empty slot, as the game does
    {
        const int32_t j = (sSlot + i) % EasyChat_UiMaxWords();
        if (p[j] == EC_EMPTY_WORD) { sSlot = j; break; }
    }
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
    {
        if (kind == ASK_LYRICS)
        {
            const u8 *a, *b;                          // the old song again
            EasyChat_UiRevertLyrics(&a, &b);
            Ask(ASK_MSG, a, b);
        }
        return true;
    }
    if (kind == ASK_CONFIRM || kind == ASK_LYRICS) End(TRUE);
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
    if (b >= 0 && b < sBtnCount)
        sBtns[b].press();
}

static void ButtonPos(int32_t b, int32_t *x, int32_t *y)
{
    *x = MSG_X + MSG_W - 16 - BTN_W;
    *y = MSG_Y + 16 + b * BTN_PITCH;
}

// The quiz question being answered: read it, then ✕ to the answer (or ○ to quit)
static bool UpdateReadOnly(const UiInput *in)
{
    if ((in->pressed & VITA_BTN_CROSS) || in->tap) { QuizSwitch(); return true; }
    if (in->pressed & VITA_BTN_CIRCLE) TryQuit();
    return true;
}

static bool Update(const UiInput *in)
{
    if (sAsk != ASK_NONE)
        return UpdateAsk(in);
    if (sReadOnly)
        return UpdateReadOnly(in);
    const int32_t max = EasyChat_UiMaxWords(), cols = EasyChat_UiColumns(), rows = Rows();
    const int32_t slotW = SlotW(), visWords = WORD_COLS * sListRows;

    if (in->pressed & (VITA_BTN_L | VITA_BTN_R)) { SetAlpha(!sAlpha); return true; }
    const int32_t t = UiKit_TabHit(in, sTabX, sTabW, 2);
    if (t >= 0) { SetAlpha(t == 1); return true; }
    if (in->pressed & VITA_BTN_START) { TryOk(); return true; }

    // Touch
    if (in->tap)
    {
        for (int32_t i = 0; i < max; i++)
        {
            int32_t x, y;
            SlotPos(i, &x, &y);
            if (!UiHit(in, x, y, slotW - 8, sSlotPitch - 8)) continue;
            if (sAt == AT_MSG && sBtn < 0 && sSlot == i) sAt = AT_GROUPS;
            else { sAt = AT_MSG; sBtn = -1; sSlot = i; }
            PlaySE(SE_SELECT);
            return true;
        }
        for (int32_t b = 0; b < sBtnCount; b++)
        {
            int32_t x, y;
            ButtonPos(b, &x, &y);
            if (UiHit(in, x, y, BTN_W, 42)) { sAt = AT_MSG; sBtn = b; PressButton(b); return true; }
        }
        for (int32_t r = 0; r < sListRows && sGroups.first + r < sGroupCount; r++)
            if (UiHit(in, 28, sLowY + LIST_TOP + r * ROW_H, GROUP_W - 24, ROW_H))
            {
                const int32_t g = sGroups.first + r;
                if (sAt == AT_GROUPS && sGroups.sel == g) sAt = AT_WORDS;
                else { sAt = AT_GROUPS; sGroups.sel = g; SelectGroup(g); }
                PlaySE(SE_SELECT);
                return true;
            }
        const int32_t ww = (MSG_W - GROUP_W - 12 - 24) / WORD_COLS;
        for (int32_t k = 0; k < visWords; k++)
        {
            const int32_t i = sWordFirst + k;
            if (i >= sWordCount) break;
            if (!UiHit(in, GROUP_W + 40 + (k % WORD_COLS) * ww, sLowY + LIST_TOP + (k / WORD_COLS) * ROW_H, ww, ROW_H)) continue;
            if (sAt == AT_WORDS && sWordSel == i) PutWord(EasyChat_UiWordAt((u16)i));
            else { sAt = AT_WORDS; sWordSel = i; PlaySE(SE_SELECT); }
            return true;
        }
        return true;
    }

    const int32_t dx = (in->repeat & VITA_BTN_RIGHT) ? 1 : (in->repeat & VITA_BTN_LEFT) ? -1 : 0;
    const int32_t dy = (in->repeat & VITA_BTN_DOWN) ? 1 : (in->repeat & VITA_BTN_UP) ? -1 : 0;
    if ((in->pressed & VITA_BTN_TRIANGLE) && sBtn < 0)
    {
        if (EasyChat_UiCanDeleteWord())
        {
            EasyChat_UiPhrase()[sSlot] = EC_EMPTY_WORD;       // delete the word (the GBA's DEL)
            PlaySE(SE_SELECT);
        }
        else
            PlaySE(SE_FAILURE);
        return true;
    }
    switch (sAt)
    {
    case AT_MSG:
        if (in->pressed & VITA_BTN_CIRCLE) { TryQuit(); return true; }
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
                else if (dy) sBtn = (sBtn + dy + sBtnCount) % sBtnCount;
            }
            else
            {
                const int32_t c = sSlot % cols, r = sSlot / cols;
                if (dx > 0 && c == cols - 1) sBtn = r < sBtnCount ? r : sBtnCount - 1;
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
        const int32_t a = UiList_Update(&sGroups, in, 28, sLowY + LIST_TOP, GROUP_W - 24, ROW_H, true);
        if (sGroups.sel != before) SelectGroup(sGroups.sel);
        if ((a >= 0 || (in->pressed & VITA_BTN_RIGHT)) && sWordCount > 0) { sAt = AT_WORDS; PlaySE(SE_SELECT); }
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
            if (sWordSel >= sWordFirst + visWords) sWordFirst = (sWordSel / WORD_COLS - sListRows + 1) * WORD_COLS;
            PlaySE(SE_SELECT);
        }
        return true;
    }
}

static void BuildMessage(void)
{
    char buf[96];
    const int32_t max = EasyChat_UiMaxWords(), slotW = SlotW(), slotH = sSlotPitch - 8;
    UiW_Panel(MSG_X, MSG_Y, MSG_W, sMsgH);
    const u16 *p = EasyChat_UiPhrase();
    for (int32_t i = 0; i < max; i++)
    {
        int32_t x, y;
        SlotPos(i, &x, &y);
        const bool on = !sReadOnly && sAt == AT_MSG && sBtn < 0 && i == sSlot;
        const bool target = !sReadOnly && sAt != AT_MSG && i == sSlot;
        UiW_Row(x, y, slotW - 8, slotH, on, 0);
        if (target)
            UiDraw_Frame(x, y, slotW - 8, slotH, 3, UI_COL_SELECT);
        if (sReadOnly && p[i] == EC_EMPTY_WORD)
            continue;                                 // the question shows its words only
        WordText(p[i], buf, sizeof(buf));
        const uint32_t ink = p[i] == EC_EMPTY_WORD ? UI_COL_INK_SOFT : on ? UI_COL_SELECT_INK : UI_COL_INK;
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + (slotW - 8 - UiText_Width(UI_FACE_VT323, UI_BODY_PX, buf)) / 2,
                    y + (slotH - UI_BODY_PX) / 2, buf, ink);
    }
    for (int32_t b = 0; b < sBtnCount; b++)
    {
        int32_t x, y;
        ButtonPos(b, &x, &y);
        if (sAt == AT_MSG && sBtn == b)
            UiDraw_Rect(x - 4, y - 4, BTN_W + 8, 48, UI_COL_SELECT);
        UiW_Button(x, y, BTN_W, sBtns[b].label, sBtns[b].color, false);
    }
}

static void BuildLists(void)
{
    char buf[96];
    const int32_t lowH = LOW_BOTTOM - sLowY, visWords = WORD_COLS * sListRows;
    UiW_Panel(16, sLowY, GROUP_W, lowH);
    UiW_PanelHead(16, sLowY, GROUP_W, sAlpha ? "A-Z" : "GROUPS", NULL);
    for (int32_t r = 0; r < sListRows && sGroups.first + r < sGroupCount; r++)
    {
        const int32_t g = sGroups.first + r, y = sLowY + LIST_TOP + r * ROW_H;
        const bool on = g == sGroups.sel;
        UiW_Row(28, y, GROUP_W - 24, ROW_H, on && sAt == AT_GROUPS, g);
        if (on && sAt != AT_GROUPS)
            UiDraw_Frame(28, y, GROUP_W - 24, ROW_H - 2, 2, UI_COL_SELECT);
        GroupLabel(g, buf, sizeof(buf));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, 56, y + 2, buf, on && sAt == AT_GROUPS ? UI_COL_SELECT_INK : UI_COL_INK);
    }
    UiW_ScrollBar(16 + GROUP_W - 12, sLowY + LIST_TOP, sListRows * ROW_H, sGroups.first, sListRows, sGroupCount);

    const int32_t wx = 16 + GROUP_W + 12, ww = (MSG_W - GROUP_W - 12 - 24) / WORD_COLS;
    UiW_Panel(wx, sLowY, MSG_W - GROUP_W - 12, lowH);
    if (sGroupCount > 0)
        GroupLabel(sGroup, buf, sizeof(buf));
    else
        buf[0] = 0;
    char right[24];
    snprintf(right, sizeof(right), "%d words", (int)sWordCount);
    UiW_PanelHead(wx, sLowY, MSG_W - GROUP_W - 12, buf, right);
    for (int32_t k = 0; k < visWords; k++)
    {
        const int32_t i = sWordFirst + k;
        if (i >= sWordCount) break;
        const int32_t x = wx + 12 + (k % WORD_COLS) * ww, y = sLowY + LIST_TOP + (k / WORD_COLS) * ROW_H;
        const bool on = sAt == AT_WORDS && i == sWordSel;
        UiW_Row(x, y, ww - 4, ROW_H, on, k / WORD_COLS);
        WordText(EasyChat_UiWordAt((u16)i), buf, sizeof(buf));
        UiText_Draw(UI_FACE_VT323, UI_BODY_PX, x + 24, y + 2, buf, on ? UI_COL_SELECT_INK : UI_COL_INK);
    }
    UiW_ScrollBar(wx + MSG_W - GROUP_W - 12 - 12, sLowY + LIST_TOP, sListRows * ROW_H, sWordFirst / WORD_COLS, sListRows,
                  (sWordCount + WORD_COLS - 1) / WORD_COLS);
}

static void BuildDialog(const char *a, const char *b)
{
    UiW_DialogBox(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 112, a, UI_COL_INK);
    UiText_Draw(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 66, b, UI_COL_INK);
}

static void Build(void)
{
    static const char *const tabs[2] = { "GROUPS", "A-Z" };
    char title[48];
    Text(EasyChat_UiTitle(), title, sizeof(title));
    if (sReadOnly)
        UiKit_Header("EASY CHAT", NULL, 0, 0, NULL, NULL, 12);
    else
        UiKit_Header("EASY CHAT", tabs, 2, sAlpha ? 1 : 0, sTabX, sTabW, 12);
    if (title[0])
        UiW_HeaderValue(title, UI_COL_ON_FRAME);

    BuildMessage();
    int32_t fx = 16;
    if (sReadOnly)
    {
        // The quiz question: the game's instructions under it (read it, then A)
        const u8 *a, *b;
        char l1[120], l2[120];
        EasyChat_UiInstructions(&a, &b);
        Text(a, l1, sizeof(l1));
        Text(b, l2, sizeof(l2));
        if (sAsk == ASK_NONE)
            BuildDialog(l1, l2);
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Answer");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Quit");
    }
    else
    {
        BuildLists();
        UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Choose");
        UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Back");
        if (EasyChat_UiCanDeleteWord())
            UiW_FooterGlyph(&fx, UI_GLYPH_TRIANGLE, "Delete word");
        UiW_FooterKey(&fx, "START", "OK");
    }
    UiKit_FooterClock();

    if (sAsk != ASK_NONE)
    {
        UiDraw_Darken(1, 3);
        BuildDialog(sAskText[0], sAskText[1]);
        if (sAsk != ASK_MSG)
            UiMenu_Draw(&sYesNo);
    }
}

const UiScreen gUiEasyChatScreen = { Enter, Update, Build, NULL };
