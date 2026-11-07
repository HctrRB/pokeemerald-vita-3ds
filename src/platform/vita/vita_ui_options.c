#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"

#include "global.h"
#include "m4a.h"
#include "platform.h"
#include "sound.h"
#include "constants/songs.h"

#include <stdio.h>
#include <string.h>

enum OptRowKind
{
    OPT_KIND_CORE = 0,
    OPT_KIND_EXTRA,
    OPT_KIND_REMAP_OPEN,
    OPT_KIND_QUIT
};

typedef struct
{
    enum OptRowKind kind;
    const char *group;
    const char *label;
    uint16_t subId;
    const VitaExtra *extra;
} OptRow;

static uint8_t sTab = VITA_TAB_GENERAL;
static int32_t sCursor[VITA_TAB_COUNT] = { 0 };
static int32_t sScroll[VITA_TAB_COUNT] = { 0 };

// Interactive button remapper modal
static bool sRemapOpen = false;
static int32_t sRemapCursor = 0;
static int32_t sRemapWaitingAction = -1;
static bool sQuitConfirmOpen = false;

static const uint8_t sDefaultButtonBits[VITA_ACTION_COUNT] = { 4, 5, 8, 9, 10, 11, 7, 6 };
static const char *const sActionLabels[VITA_ACTION_COUNT] = {
    "A BUTTON", "B BUTTON", "L SHOULDER", "R SHOULDER",
    "START", "SELECT", "FAST FORWARD", "QUICK OPTIONS"
};
static const char *const sPhysBtnNames[16] = {
    "UP", "DOWN", "LEFT", "RIGHT",
    "CROSS", "CIRCLE", "SQUARE", "TRIANGLE",
    "L", "R", "START", "SELECT", "RS UP", "RS DOWN", "RS LEFT", "RS RIGHT"
};

static int32_t BuildTabRows(const uint8_t tab, OptRow *out, const int32_t maxRows)
{
    int32_t n = 0;
    if (tab == VITA_TAB_GENERAL)
    {
        static const char *const sCoreNames[6] = {
            "TEXT SPEED", "BATTLE SCENE", "BATTLE STYLE", "SOUND OUTPUT",
            "MASTER VOLUME", "BUTTON MODE"
        };
        for (uint16_t i = 0; i < 6 && n < maxRows; i++)
        {
            out[n].kind = OPT_KIND_CORE;
            out[n].group = (i < 3) ? "GAMEPLAY" : ((i < 5) ? "AUDIO" : "INTERFACE");
            out[n].label = sCoreNames[i];
            out[n].subId = i;
            out[n].extra = NULL;
            n++;
        }
        out[n].kind = OPT_KIND_REMAP_OPEN;
        out[n].group = "CONTROLS";
        out[n].label = "BUTTON REMAPPING";
        out[n].subId = 0;
        out[n].extra = NULL;
        n++;
    }

    const uint32_t extCount = VitaExtras_GetTabExtraCount(tab);
    for (uint32_t i = 0; i < extCount && n < maxRows; i++)
    {
        const VitaExtra *const ex = VitaExtras_GetTabExtra(tab, i);
        if (ex == NULL)
            continue;
        out[n].kind = OPT_KIND_EXTRA;
        out[n].group = ex->group;
        out[n].label = ex->name;
        out[n].subId = (uint16_t)i;
        out[n].extra = ex;
        n++;
    }

    if (tab == VITA_TAB_MODES && n < maxRows)
    {
        out[n].kind = OPT_KIND_QUIT;
        out[n].group = "SYSTEM";
        out[n].label = "QUIT TO LIVEAREA";
        out[n].subId = 0;
        out[n].extra = NULL;
        n++;
    }

    return n;
}

static void FormatCoreValue(const uint16_t coreId, char *out, const size_t cap)
{
    static const char *const sTxtSpd[3] = { "SLOW", "MID", "FAST" };
    switch (coreId)
    {
    case 0: snprintf(out, cap, "%s", sTxtSpd[gSaveBlock2Ptr->optionsTextSpeed % 3U]); break;
    case 1: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsBattleSceneOff ? "OFF" : "ON"); break;
    case 2: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsBattleStyle ? "SET" : "SHIFT"); break;
    case 3: snprintf(out, cap, "%s", gSaveBlock2Ptr->optionsSound ? "STEREO" : "MONO"); break;
    case 4: snprintf(out, cap, "%u0%%", (uint32_t)Platform_GetSetting(PLATFORM_SETTING_VOLUME)); break;
    case 5:
    {
        static const char *const sBtnMode[3] = { "NORMAL", "LR", "L=A" };
        snprintf(out, cap, "%s", sBtnMode[gSaveBlock2Ptr->optionsButtonMode % 3U]);
        break;
    }
    default: out[0] = '\0'; break;
    }
}

static void StepCoreOption(const uint16_t coreId, const int32_t dir)
{
    switch (coreId)
    {
    case 0:
        gSaveBlock2Ptr->optionsTextSpeed = (u8)((3 + (int32_t)gSaveBlock2Ptr->optionsTextSpeed + dir) % 3);
        break;
    case 1:
        gSaveBlock2Ptr->optionsBattleSceneOff ^= 1U;
        break;
    case 2:
        gSaveBlock2Ptr->optionsBattleStyle ^= 1U;
        break;
    case 3:
        gSaveBlock2Ptr->optionsSound ^= 1U;
        SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound);
        break;
    case 4:
    {
        int32_t v = (int32_t)Platform_GetSetting(PLATFORM_SETTING_VOLUME) + dir;
        if (v < 0) v = 10;
        if (v > 10) v = 0;
        Platform_SetSetting(PLATFORM_SETTING_VOLUME, (u8)v);
        break;
    }
    case 5:
        gSaveBlock2Ptr->optionsButtonMode = (u8)((3 + (int32_t)gSaveBlock2Ptr->optionsButtonMode + dir) % 3);
        break;
    default:
        break;
    }
}

void VitaUiOptions_Reset(void)
{
    sRemapOpen = false;
    sRemapWaitingAction = -1;
    sQuitConfirmOpen = false;
}

bool VitaUiOptions_HasActiveModal(void)
{
    return sRemapOpen || sQuitConfirmOpen;
}

bool VitaUiOptions_Update(const VitaUiInput *in)
{
    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);

    if (sQuitConfirmOpen)
    {
        if (in->newButtons & aBtn)
        {
            VitaPlatform_RequestQuit();
            return true;
        }
        if ((in->newButtons & bBtn) || in->touchPressed)
        {
            sQuitConfirmOpen = false;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    if (sRemapOpen)
    {
        if (sRemapWaitingAction >= 0)
        {
            // Capture physical non-DPAD button bit (4..11)
            for (int32_t bit = 4; bit < 12; bit++)
            {
                if ((in->newButtons & (1U << bit)) != 0U)
                {
                    Platform_SetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + sRemapWaitingAction), (u8)bit);
                    sRemapWaitingAction = -1;
                    PlaySE(SE_SELECT);
                    return true;
                }
            }
            return false;
        }

        const int32_t totalRemapRows = VITA_ACTION_COUNT + 1; // +1 = RESET DEFAULTS
        if (in->newButtons & VITA_BTN_UP)
        {
            sRemapCursor = (sRemapCursor <= 0) ? (totalRemapRows - 1) : (sRemapCursor - 1);
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & VITA_BTN_DOWN)
        {
            sRemapCursor = (sRemapCursor + 1) % totalRemapRows;
            PlaySE(SE_SELECT);
            return true;
        }
        if (in->newButtons & aBtn)
        {
            if (sRemapCursor == VITA_ACTION_COUNT)
            {
                for (int32_t a = 0; a < VITA_ACTION_COUNT; a++)
                    Platform_SetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + a), sDefaultButtonBits[a]);
                PlaySE(SE_SUCCESS);
            }
            else
            {
                sRemapWaitingAction = sRemapCursor;
                PlaySE(SE_SELECT);
            }
            return true;
        }
        if (in->newButtons & bBtn)
        {
            sRemapOpen = false;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    bool changed = false;
    // Category tab switching via SELECT or tapping category pills
    if (in->newButtons & VITA_BTN_SELECT)
    {
        sTab = (uint8_t)((sTab + 1U) % VITA_TAB_COUNT);
        PlaySE(SE_SELECT);
        changed = true;
    }
    for (uint8_t t = 0; t < VITA_TAB_COUNT; t++)
    {
        if (VitaUi_HitRect(in, 6 + (int32_t)t * 62, 26, 60, 15))
        {
            sTab = t;
            PlaySE(SE_SELECT);
            changed = true;
        }
    }

    OptRow rows[24];
    const int32_t rowCount = BuildTabRows(sTab, rows, 24);
    if (rowCount <= 0)
        return changed;

    int32_t cur = sCursor[sTab];
    if (cur >= rowCount) cur = rowCount - 1;

    if ((in->newButtons & VITA_BTN_UP) && cur > 0)
    {
        cur--;
        PlaySE(SE_SELECT);
        changed = true;
    }
    else if ((in->newButtons & VITA_BTN_DOWN) && cur + 1 < rowCount)
    {
        cur++;
        PlaySE(SE_SELECT);
        changed = true;
    }
    sCursor[sTab] = cur;
    if (cur < sScroll[sTab]) sScroll[sTab] = cur;
    if (cur >= sScroll[sTab] + 6) sScroll[sTab] = cur - 5;

    // Touch row hit
    int32_t stepDir = 0;
    if (in->newButtons & VITA_BTN_RIGHT) stepDir =  1;
    if (in->newButtons & VITA_BTN_LEFT)  stepDir = -1;

    for (int32_t r = 0; r < 6 && (sScroll[sTab] + r) < rowCount; r++)
    {
        if (VitaUi_HitRect(in, 10, 48 + r * 25, 300, 23))
        {
            sCursor[sTab] = sScroll[sTab] + r;
            cur = sCursor[sTab];
            stepDir = 1;
            changed = true;
        }
    }

    if ((in->newButtons & aBtn) && stepDir == 0)
        stepDir = 1;

    if (stepDir != 0)
    {
        const OptRow *const sel = &rows[cur];
        if (sel->kind == OPT_KIND_CORE)
        {
            StepCoreOption(sel->subId, stepDir);
            PlaySE(SE_SELECT);
        }
        else if (sel->kind == OPT_KIND_REMAP_OPEN)
        {
            sRemapOpen = true;
            sRemapCursor = 0;
            sRemapWaitingAction = -1;
            PlaySE(SE_SELECT);
        }
        else if (sel->kind == OPT_KIND_QUIT)
        {
            sQuitConfirmOpen = true;
            PlaySE(SE_SELECT);
        }
        else if (sel->kind == OPT_KIND_EXTRA && sel->extra != NULL)
        {
            const int32_t prevSaveSlot = VitaExtras_Get("save_slot");
            VitaExtras_Step(sel->extra, stepDir);
            PlaySE(SE_SELECT);
            if (strcmp(sel->extra->key, "save_slot") == 0 && VitaExtras_Get("save_slot") != prevSaveSlot)
            {
                const int32_t newSlot = VitaExtras_Get("save_slot") + 1;
                VitaNuzlocke_LoadForSlot(newSlot);
                VitaUI_Close();
                VitaPlatform_RequestReset();
                return true;
            }
        }
        changed = true;
    }

    return changed;
}

void VitaUiOptions_Render(uint16_t *dst, const size_t pitch, const uint8_t blinkPhase)
{
    const VitaUiClip *const clip = VitaUi_FullClip();

    static const char *const sTabTitles[VITA_TAB_COUNT] = {
        "GENERAL", "WORLD", "BATTLE", "MODES", "CHEATS"
    };
    for (uint8_t t = 0; t < VITA_TAB_COUNT; t++)
    {
        const bool sel = (sTab == t);
        VitaUiPlate_Draw(
            dst, pitch, clip,
            6 + (int32_t)t * 62, 26, 60, 15, 2,
            sel ? kPlateSage : kPlateSageDark,
            sel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(
            dst, pitch, clip, VITA_FONT_SMALL_NARROW, sTabTitles[t],
            6 + (int32_t)t * 62, 26, 60, 15, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    VitaUiPlate_Draw(dst, pitch, clip, 6, 43, 308, 163, 4, kPlateCream, VITA_PLATE_NORMAL, blinkPhase, 0);

    OptRow rows[24];
    const int32_t rowCount = BuildTabRows(sTab, rows, 24);
    for (int32_t r = 0; r < 6 && (sScroll[sTab] + r) < rowCount; r++)
    {
        const int32_t idx = sScroll[sTab] + r;
        const OptRow *const row = &rows[idx];
        const int32_t ry = 47 + r * 25;
        const bool isSel = (idx == sCursor[sTab]);

        VitaUiPlate_Draw(
            dst, pitch, clip, 12, ry, 296, 22, 3,
            isSel ? kPlateCardSel : kPlateCard,
            isSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);

        if (row->group != NULL)
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, row->group, 18, ry + 1, UI_INK_MUTED, VITA_UI_NO_SHADOW);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, row->label, 68, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }
        else
        {
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL, row->label, 18, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
        }

        // Value pill on right
        char valBuf[28] = "";
        bool isOnPill = false;
        if (row->kind == OPT_KIND_CORE)
        {
            FormatCoreValue(row->subId, valBuf, sizeof(valBuf));
            isOnPill = (strcmp(valBuf, "ON") == 0 || strcmp(valBuf, "STEREO") == 0 || strcmp(valBuf, "FAST") == 0);
        }
        else if (row->kind == OPT_KIND_REMAP_OPEN)
        {
            snprintf(valBuf, sizeof(valBuf), "CONFIGURE");
            isOnPill = true;
        }
        else if (row->kind == OPT_KIND_QUIT)
        {
            snprintf(valBuf, sizeof(valBuf), "EXIT APP");
        }
        else if (row->kind == OPT_KIND_EXTRA && row->extra != NULL)
        {
            if (row->extra->text != NULL)
            {
                const uint8_t *const dyn = row->extra->text();
                VitaUiArt_GbaToAscii(dyn, valBuf, sizeof(valBuf));
                isOnPill = true;
            }
            else if (row->extra->values != NULL)
            {
                const int32_t v = (row->extra->count > 0) ? (VitaExtras_Get(row->extra->key) % row->extra->count) : 0;
                snprintf(valBuf, sizeof(valBuf), "%s", row->extra->values[v]);
                isOnPill = (strcmp(valBuf, "ON") == 0 || strcmp(valBuf, "AUTO") == 0 || strcmp(valBuf, "EXECUTE") == 0);
            }
        }

        VitaUiPlate_Draw(
            dst, pitch, clip, 224, ry + 3, 78, 16, 2,
            row->kind == OPT_KIND_QUIT ? kPlateCrimson : (isOnPill ? kPlateSage : kPlateSageDark),
            VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, valBuf, 263, ry + 4, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }

    if (sRemapOpen)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 24, 34, 272, 170, 5, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 16);
        VitaUiArt_DrawStrAsciiCenter(
            dst, pitch, clip, VITA_FONT_SMALL,
            sRemapWaitingAction >= 0 ? "PRESS NEW PS VITA BUTTON..." : "PHYSICAL BUTTON REMAPPING",
            160, 36, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        for (int32_t a = 0; a < VITA_ACTION_COUNT; a++)
        {
            const int32_t col = a / 4;
            const int32_t row = a % 4;
            const int32_t bx = 32 + col * 130;
            const int32_t by = 56 + row * 28;
            const bool aSel = (sRemapCursor == a);
            const uint8_t bit = Platform_GetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + a));

            VitaUiPlate_Draw(
                dst, pitch, clip, bx, by, 124, 24, 3,
                (sRemapWaitingAction == a) ? kPlateGold : (aSel ? kPlateSage : kPlateSageDark),
                aSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
            VitaUiArt_DrawStrAscii(dst, pitch, clip, VITA_FONT_SMALL_NARROW, sActionLabels[a], bx + 5, by + 2, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
            VitaUiArt_DrawStrAsciiRight(dst, pitch, clip, VITA_FONT_SMALL_NARROW, (bit < 16 ? sPhysBtnNames[bit] : "NONE"), bx + 119, by + 11, UI_INK_MINT, VITA_UI_NO_SHADOW);
        }

        const bool defSel = (sRemapCursor == VITA_ACTION_COUNT);
        VitaUiPlate_Draw(
            dst, pitch, clip, 80, 172, 160, 22, 3,
            defSel ? kPlateSage : kPlateSageDark,
            defSel ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL, blinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "RESET TO DEFAULTS", 160, 176, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
    else if (sQuitConfirmOpen)
    {
        VitaUiPlate_Draw(dst, pitch, clip, 56, 78, 208, 76, 5, kPlatePopup, VITA_PLATE_FOCUS, blinkPhase, 16);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "EXIT TO PS VITA LIVEAREA?", 160, 80, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL_NARROW, "UNSAVED PROGRESS WILL BE LOST.", 160, 104, UI_INK_MINT, VITA_UI_NO_SHADOW);
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, clip, VITA_FONT_SMALL, "A: EXIT NOW     B: CANCEL", 160, 128, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
}
