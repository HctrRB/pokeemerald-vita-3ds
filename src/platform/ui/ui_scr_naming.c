// Naming screen of the new UI (design/stitch/teclado_nombre), replacing naming_screen.c's GBA screen
// for every template: player, PC box, caught POKéMON, nickname and Walda's words. Opened hosted
// (UiCore_OpenHosted): the UI runs the game loop until OK, then the name goes to destBuffer exactly
// as SaveInputText does (only when it has a non-space character) and the return callback is set.
// Keys: d-pad + ✕ types, ○ deletes, L / R change the page, START jumps to OK. Touch: the first tap
// selects a key, a tap on the selected key types it.

#include <stdio.h>
#include <string.h>
#include "ui_internal.h"
#include "ui_game.h"
#include "platform/ui.h"
#include "../vita/vita_platform.h"

#include "global.h"
#include "main.h"
#include "naming_screen.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "walda_phrase.h"
#include "sound.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/songs.h"
#include "constants/trainers.h"

extern const u8 *NamingScreen_SentToPCTextForUi(const u8 *name);

#define COLS 9
#define ROWS 4
#define KEY_X 276
#define KEY_Y 156
#define KEY_W 52
#define KEY_H 76
#define KEY_GAP 6
#define FN_X (KEY_X + COLS * (KEY_W + KEY_GAP))
#define FN_W 132
#define FN_COL COLS                       // cursor column of SPACE / DEL / OK
#define MAX_CHARS 15

enum { PAGE_UPPER, PAGE_LOWER, PAGE_OTHERS, PAGE_COUNT };
enum { FN_SPACE, FN_DEL, FN_OK };

typedef struct { uint8_t code; const char *label; } Key;

// Others page: the GBA's OTHERS keys that the UI fonts can show (♂ ♀ are sprites)
static const Key sOthers[] = {
    { CHAR_0, "0" }, { CHAR_0 + 1, "1" }, { CHAR_0 + 2, "2" }, { CHAR_0 + 3, "3" }, { CHAR_0 + 4, "4" },
    { CHAR_0 + 5, "5" }, { CHAR_0 + 6, "6" }, { CHAR_0 + 7, "7" }, { CHAR_0 + 8, "8" }, { CHAR_0 + 9, "9" },
    { CHAR_EXCL_MARK, "!" }, { CHAR_QUESTION_MARK, "?" }, { CHAR_MALE, NULL }, { CHAR_FEMALE, NULL },
    { CHAR_SLASH, "/" }, { CHAR_HYPHEN, "-" }, { CHAR_ELLIPSIS, "..." }, { CHAR_PERIOD, "." },
    { CHAR_COMMA, "," }, { CHAR_SGL_QUOTE_RIGHT, "'" }, { CHAR_COLON, ":" },
};
static const char *const sTabs[PAGE_COUNT] = { "ABC", "abc", "?!:" };

static uint8_t sTemplate, sMax;
static uint8_t *sDest;
static uint16_t sSpecies, sGender;
static void (*sReturn)(void);
static uint8_t sText[MAX_CHARS + 1];
static int32_t sLen, sPage, sCx, sCy;
static int32_t sTabX[PAGE_COUNT], sTabW[PAGE_COUNT];
static char sMsg[200];                    // "sent to the PC" message shown before leaving
static uint32_t sFrame;

static int32_t PageCount(int32_t page)
{
    return page == PAGE_OTHERS ? (int32_t)(sizeof(sOthers) / sizeof(sOthers[0])) : 26;
}

// GBA code and label of the key at (col, row) of the current page; false for an empty cell
static bool KeyAt(int32_t col, int32_t row, Key *out)
{
    const int32_t i = row * COLS + col;
    if (col >= COLS || i >= PageCount(sPage))
        return false;
    static char letter[2][2];
    if (sPage == PAGE_OTHERS)
    {
        *out = sOthers[i];
        return true;
    }
    char *buf = letter[i & 1];
    buf[0] = (char)((sPage == PAGE_UPPER ? 'A' : 'a') + i);
    buf[1] = 0;
    out->code = (uint8_t)((sPage == PAGE_UPPER ? CHAR_A : CHAR_a) + i);
    out->label = buf;
    return true;
}

static int32_t FnAt(int32_t row)
{
    return row == 0 ? FN_SPACE : row == 1 ? FN_DEL : FN_OK;
}

static void Finish(void)
{
    void (*cb)(void) = sReturn;
    if (cb != NULL)
        SetMainCallback2(cb);
}

static void Save(void)
{
    // SaveInputText: only a name with a non-space character replaces destBuffer
    for (int32_t i = 0; i < sLen; i++)
    {
        if (sText[i] != CHAR_SPACE && sText[i] != EOS)
        {
            memcpy(sDest, sText, (size_t)sLen);
            sDest[sLen] = EOS;
            return;
        }
    }
}

static void Ok(void)
{
    PlaySE(SE_SELECT);
    Save();
    if (sTemplate == NAMING_SCREEN_CAUGHT_MON)
    {
        const u8 *msg = NamingScreen_SentToPCTextForUi(sDest);
        if (msg != NULL)
        {
            UiGame_Text(msg, sMsg, sizeof(sMsg));
            return;                                   // shown until ✕ / tap
        }
    }
    UiNav_Close(Finish);
}

static void Type(uint8_t code)
{
    if (sLen >= sMax)
        return;
    sText[sLen++] = code;
    sText[sLen] = EOS;
    PlaySE(SE_SELECT);
    if (sLen == sMax)
    {
        sCx = FN_COL;                                 // full: the cursor goes to OK, as in the game
        sCy = 2;
    }
}

static void Delete(void)
{
    if (sLen == 0)
        return;
    sText[--sLen] = EOS;
    PlaySE(SE_BALL);
}

static void Activate(void)
{
    if (sCx == FN_COL)
    {
        const int32_t f = FnAt(sCy);
        if (f == FN_SPACE) Type(CHAR_SPACE);
        else if (f == FN_DEL) Delete();
        else Ok();
        return;
    }
    Key k;
    if (KeyAt(sCx, sCy, &k))
        Type(k.code);
}

static void SetPage(int32_t page)
{
    sPage = (page + PAGE_COUNT) % PAGE_COUNT;
    PlaySE(SE_SELECT);
    Key k;
    if (sCx != FN_COL && !KeyAt(sCx, sCy, &k))
        sCx = 0, sCy = 0;
}

bool Ui_OpenNaming(uint8_t templateNum, uint8_t *destBuffer, uint16_t species, uint16_t gender, uint32_t personality, void (*returnCallback)(void))
{
    (void)personality;
    if (!Ui_IsEnabled() || Ui_IsOpen() || destBuffer == NULL)
        return false;
    sTemplate = templateNum;
    sMax = templateNum == NAMING_SCREEN_PLAYER ? PLAYER_NAME_LENGTH
         : templateNum == NAMING_SCREEN_BOX ? BOX_NAME_LENGTH
         : templateNum == NAMING_SCREEN_WALDA ? WALDA_PHRASE_LENGTH : POKEMON_NAME_LENGTH;
    if (sMax > MAX_CHARS)
        sMax = MAX_CHARS;
    sDest = destBuffer;
    sSpecies = species;
    sGender = gender;
    sReturn = returnCallback;
    memset(sText, EOS, sizeof(sText));
    sLen = 0;
    // Only Walda's words start from the current text (copyExistingString)
    if (templateNum == NAMING_SCREEN_WALDA)
        while (sLen < sMax && destBuffer[sLen] != EOS)
            sText[sLen] = destBuffer[sLen], sLen++;
    UiCore_OpenHosted(&gUiNamingScreen, NULL);
    return true;
}

static void Enter(void)
{
    sPage = PAGE_UPPER;
    sCx = sCy = 0;
    sMsg[0] = 0;
}

static bool Update(const UiInput *in)
{
    sFrame++;
    if (sMsg[0])
    {
        if ((in->pressed & (VITA_BTN_CROSS | VITA_BTN_CIRCLE)) || in->tap)
        {
            sMsg[0] = 0;
            UiNav_Close(Finish);
        }
        return true;
    }
    // ○ deletes and never leaves: the only way out is OK
    if (in->pressed & VITA_BTN_CIRCLE) { Delete(); return true; }
    if (in->pressed & VITA_BTN_START) { sCx = FN_COL; sCy = 2; PlaySE(SE_SELECT); return true; }
    if (in->pressed & VITA_BTN_L) { SetPage(sPage - 1); return true; }
    if (in->pressed & VITA_BTN_R) { SetPage(sPage + 1); return true; }
    const int32_t t = UiKit_TabHit(in, sTabX, sTabW, PAGE_COUNT);
    if (t >= 0 && t != sPage) { SetPage(t); return true; }
    if (in->pressed & VITA_BTN_CROSS) { Activate(); return true; }

    // D-pad, wrapping; the function column has three targets (OK covers rows 2 and 3)
    const int32_t dx = (in->repeat & VITA_BTN_RIGHT) ? 1 : (in->repeat & VITA_BTN_LEFT) ? -1 : 0;
    const int32_t dy = (in->repeat & VITA_BTN_DOWN) ? 1 : (in->repeat & VITA_BTN_UP) ? -1 : 0;
    if (dx || dy)
    {
        Key k;
        for (int32_t tries = 0; tries < COLS + 2; tries++)
        {
            sCx = (sCx + dx + COLS + 1) % (COLS + 1);
            sCy = (sCy + dy + ROWS) % ROWS;
            if (sCx == FN_COL && dy && sCy == 3) sCy = dy > 0 ? 0 : 2;     // OK spans two rows
            if (sCx == FN_COL || KeyAt(sCx, sCy, &k))
                break;
            if (dy && !dx) { sCy = (sCy + ROWS) % ROWS; continue; }
        }
        if (sCx == FN_COL && sCy == 3) sCy = 2;
        PlaySE(SE_SELECT);
        return true;
    }

    if (in->tap)
    {
        for (int32_t r = 0; r < ROWS; r++)
        {
            for (int32_t c = 0; c <= COLS; c++)
            {
                const int32_t x = c == FN_COL ? FN_X : KEY_X + c * (KEY_W + KEY_GAP);
                const int32_t y = KEY_Y + r * (KEY_H + KEY_GAP);
                const int32_t w = c == FN_COL ? FN_W : KEY_W;
                int32_t h = KEY_H;
                if (c == FN_COL && r == 3) continue;
                if (c == FN_COL && r == 2) h = 2 * KEY_H + KEY_GAP;
                Key k;
                if (c != FN_COL && !KeyAt(c, r, &k)) continue;
                if (!UiHit(in, x, y, w, h)) continue;
                if (sCx == c && sCy == r) Activate();
                else { sCx = c; sCy = r; PlaySE(SE_SELECT); }
                return true;
            }
        }
    }
    return true;
}

// One GBA character of the name or a key: text, or the ♂ / ♀ sprite
static void DrawCode(uint8_t code, const char *label, int32_t cx, int32_t y, uint8_t px, uint32_t colour)
{
    if (code == CHAR_MALE || code == CHAR_FEMALE)
    {
        UiDraw_Sprite(code == CHAR_MALE ? UI_SPR_GENDER_MALE : UI_SPR_GENDER_FEMALE, cx - 14, y + 6, 2);
        return;
    }
    char buf[8];
    if (label == NULL)
    {
        const uint8_t s[2] = { code, EOS };
        UiGame_Text(s, buf, sizeof(buf));
        label = buf;
    }
    UiText_Draw(UI_FACE_VT323, px, cx - UiText_Width(UI_FACE_VT323, px, label) / 2, y, label, colour);
}

static void Build(void)
{
    char buf[64];
    UiKit_Header("NAME", sTabs, PAGE_COUNT, sPage, sTabX, sTabW, 12);
    snprintf(buf, sizeof(buf), "%d / %d", (int)sLen, (int)sMax);
    UiW_HeaderValue(buf, UI_COL_ON_FRAME);

    // Who is named
    UiW_Panel(16, 60, 244, 432);
    const char *head = sTemplate == NAMING_SCREEN_PLAYER ? "YOUR NAME" : sTemplate == NAMING_SCREEN_BOX ? "BOX NAME"
                     : sTemplate == NAMING_SCREEN_WALDA ? "WORDS" : "NICKNAME";
    UiText_Draw(UI_FACE_PS2P, 16, 32, 78, head, UI_COL_INK);
    UiW_Inset(42, 112, 192, 192);
    if (sTemplate == NAMING_SCREEN_CAUGHT_MON || sTemplate == NAMING_SCREEN_NICKNAME)
        UiSpr_MonFront(sSpecies, false, 74, 144, 2);
    else if (sTemplate == NAMING_SCREEN_PLAYER)
        UiSpr_Trainer(UiGame_PlayerIsMale() ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY, 74, 144, 2);
    else
        UiDraw_Sprite(UI_SPR_BALL, 106, 176, 4);
    char q[64];
    if (sTemplate == NAMING_SCREEN_CAUGHT_MON || sTemplate == NAMING_SCREEN_NICKNAME)
    {
        UiGame_SpeciesName(sSpecies, buf, sizeof(buf));
        snprintf(q, sizeof(q), "%s's nickname?", buf);
        if (sGender == MON_MALE || sGender == MON_FEMALE)
            UiKit_GenderShiny(200, 80, sGender == MON_MALE ? 0x00 : 0xFE, false);
    }
    else
        snprintf(q, sizeof(q), "%s", sTemplate == NAMING_SCREEN_PLAYER ? "Your name?" : sTemplate == NAMING_SCREEN_BOX ? "The BOX's name?" : "Tell him the words.");
    UiW_TextWrapped(32, 318, 212, 30, q, UI_COL_INK, 3);
    if (sTemplate != NAMING_SCREEN_WALDA)
        UiW_TextWrapped(32, 414, 212, 30, sTemplate == NAMING_SCREEN_PLAYER ? "Empty: the name stays as it was." : "Empty: no change.", UI_COL_INK_SOFT, 2);

    // The name, one slot per character
    UiW_Panel(272, 60, 672, 84);
    const int32_t slotW = 44, nameX = 272 + (672 - sMax * slotW) / 2;
    for (int32_t i = 0; i < sMax; i++)
    {
        const int32_t x = nameX + i * slotW;
        const bool cur = i == sLen && ((sFrame / 16) & 1) == 0;
        UiDraw_Rect(x + 4, 128, slotW - 8, 3, cur ? UI_COL_SELECT : gUiTheme->paperShade);
        if (i < sLen && sText[i] != CHAR_SPACE)
            DrawCode(sText[i], NULL, x + slotW / 2, 82, UI_DIALOG_PX, UI_COL_INK);
    }

    // Keyboard
    UiW_Panel(264, 148, 680, 344);
    for (int32_t r = 0; r < ROWS; r++)
    {
        for (int32_t c = 0; c < COLS; c++)
        {
            Key k;
            if (!KeyAt(c, r, &k))
                continue;
            const int32_t x = KEY_X + c * (KEY_W + KEY_GAP), y = KEY_Y + r * (KEY_H + KEY_GAP);
            const bool on = sCx == c && sCy == r;
            UiW_Row(x, y, KEY_W, KEY_H, on, 0);
            DrawCode(k.code, k.label, x + KEY_W / 2, y + (KEY_H - 32) / 2 - 2, UI_BODY_PX, on ? UI_COL_SELECT_INK : UI_COL_INK);
        }
    }
    static const char *const fn[3] = { "SPACE", "DEL", "OK" };
    static const uint8_t colours[3] = { UI_BTN_GREEN, UI_BTN_RED, UI_BTN_GREEN };
    for (int32_t f = 0; f < 3; f++)
    {
        const int32_t y = KEY_Y + f * (KEY_H + KEY_GAP);
        const int32_t h = f == FN_OK ? 2 * KEY_H + KEY_GAP : KEY_H;
        const bool on = sCx == FN_COL && FnAt(sCy) == f;
        if (on)
            UiDraw_Rect(FN_X - 4, y - 4, FN_W + 8, h + 8, UI_COL_SELECT);
        UiW_Button(FN_X, y + (h - 40) / 2, FN_W, fn[f], f == FN_SPACE ? UI_BTN_AMBER : colours[f], false);
    }

    int32_t fx = 16;
    UiW_FooterGlyph(&fx, UI_GLYPH_CROSS, "Type");
    UiW_FooterGlyph(&fx, UI_GLYPH_CIRCLE, "Delete");
    UiW_FooterKey(&fx, "START", "Go to OK");
    UiKit_FooterClock();

    if (sMsg[0])
    {
        UiDraw_Darken(1, 2);
        UiW_DialogBox(48, UI_SCREEN_H - 24 - 124, UI_SCREEN_W - 96, 124);
        UiW_TextWrappedF(UI_FACE_VT323, UI_DIALOG_PX, 76, UI_SCREEN_H - 24 - 112, UI_SCREEN_W - 152, 46, sMsg, UI_COL_INK, 2);
    }
}

const UiScreen gUiNamingScreen = { Enter, Update, Build, NULL };
