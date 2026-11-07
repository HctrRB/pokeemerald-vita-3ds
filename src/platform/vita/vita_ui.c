#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_daynight.h"
#include "platform/vita_extras.h"
#include "platform/vita_nuzlocke.h"
#include "platform/vita_battle_ui.h"

#include "global.h"
#include "3ds_bottom_art.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "contest_util.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "item.h"
#include "item_menu.h"
#include "load_save.h"
#include "main.h"
#include "money.h"
#include "new_game.h"
#include "overworld.h"
#include "platform.h"
#include "pokedex.h"
#include "pokemon.h"
#include "region_map.h"
#include "safari_zone.h"
#include "save.h"
#include "script.h"
#include "sound.h"
#include "trainer_card.h"
#include "constants/battle_frontier.h"
#include "constants/flags.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "party_menu.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/vars.h"

#include <stdio.h>
#include <string.h>

uint16_t gVitaUiArtPal565[16];
uint16_t gVitaUiArtPal2_565[16];

static const VitaUiClip sFullClip = { 0, 0, VITA_UI_WIDTH, VITA_UI_HEIGHT };

static bool sInitialized = false;
static bool sIsOpen = false;
static bool sStandaloneOptions = false;
static bool sShopSellActive = false;
static uint8_t sShopTaskId = 0;
static enum VitaUiSection sActiveSec = VITA_UI_SEC_MAP;

static uint32_t sPrevButtons = 0;
static bool sPrevTouched = false;
static uint32_t sAnimTick = 0;
static uint8_t sBlinkPhase = 0;

// Toast notification banner
static char sToastMsg[48];
static uint32_t sToastFrames = 0;

// Card & Save local state
static bool sCardBackFace = false;
static uint8_t sSaveState = 0; // 0: Ready, 1: Saved!

// DirtyRect double-buffered canvas
static uint16_t sCanvasCur[VITA_UI_WIDTH * VITA_UI_HEIGHT];
static uint16_t sCanvasPrev[VITA_UI_WIDTH * VITA_UI_HEIGHT];
static bool sForceFullDirty = true;

const VitaUiClip *VitaUi_FullClip(void)
{
    return &sFullClip;
}

void VitaUi_ShowToast(const char *msg)
{
    if (msg == NULL)
        return;
    snprintf(sToastMsg, sizeof(sToastMsg), "%s", msg);
    sToastFrames = 90;
    sForceFullDirty = true;
}

static bool MapScreenTouchToUi(int32_t *outX, int32_t *outY)
{
    int32_t rawX = 0;
    int32_t rawY = 0;
    if (!VitaPlatform_ReadTouch(&rawX, &rawY))
        return false;

    int32_t uiX;
    int32_t uiY;
    const bool isFullscreen = (Platform_GetSetting(PLATFORM_SETTING_FILTER) != 0
                            || Platform_GetSetting(PLATFORM_SETTING_UI_SCALE) != 0);
    if (isFullscreen)
    {
        // Fullscreen: 725.33x544 centered in 960x544 -> [117.33, 842.67] x [0, 544]
        uiX = ((int32_t)rawX - 117) * VITA_UI_WIDTH / 726;
        uiY = (int32_t)rawY * VITA_UI_HEIGHT / 544;
    }
    else
    {
        // Pixel Perfect 2x: 640x480 centered in 960x544 -> [160, 800] x [32, 512]
        uiX = ((int32_t)rawX - 160) / 2;
        uiY = ((int32_t)rawY - 32) / 2;
    }
    if (uiX < 0 || uiX >= VITA_UI_WIDTH || uiY < 0 || uiY >= VITA_UI_HEIGHT)
        return false;
    *outX = uiX;
    *outY = uiY;
    return true;
}

void VitaUi_DrawArt4bpp(
    uint16_t *dst,
    const size_t pitch,
    const uint8_t *art,
    const int32_t w,
    const int32_t h,
    const int32_t x,
    const int32_t y,
    const uint16_t *pal565)
{
    if (art == NULL || pal565 == NULL)
        return;
    const int32_t wt = (w >= 8) ? (w / 8) : ((w > 0) ? w : 1);
    const int32_t ht = (h >= 8) ? (h / 8) : ((h > 0) ? h : 1);
    VitaUiArt_DrawTiles4bpp(dst, pitch, &sFullClip, art, wt, ht, x, y, pal565);
}

void VitaUi_DrawMiniPokeball(
    uint16_t *dst,
    const size_t pitch,
    const int32_t x,
    const int32_t y,
    const bool caught,
    const bool faint)
{
    static const char *const sBallRows[7] = {
        "..###..",
        ".#####.",
        "#######",
        "###.###",
        "#######",
        ".#####.",
        "..###.."
    };
    const uint16_t topCol = faint ? UI_INK_MUTED : (caught ? UI_INK_RED : UI_BORDER_SOFT);
    const uint16_t botCol = faint ? UI_BORDER_SOFT : UI_INK_LIGHT;
    const uint16_t rimCol = UI_INK_DARK;

    for (int32_t py = 0; py < 7; py++)
    {
        const int32_t sy = y + py;
        if (sy < 0 || sy >= VITA_UI_HEIGHT)
            continue;
        for (int32_t px = 0; px < 7; px++)
        {
            const int32_t sx = x + px;
            if (sx < 0 || sx >= VITA_UI_WIDTH)
                continue;
            const char ch = sBallRows[py][px];
            if (ch == '.')
            {
                if (py == 3 && px == 3)
                    dst[(size_t)sy * pitch + (size_t)sx] = UI_INK_LIGHT;
                continue;
            }
            uint16_t col = (py < 3) ? topCol : ((py == 3) ? rimCol : botCol);
            if (py == 0 || py == 6 || px == 0 || px == 6)
                col = rimCol;
            dst[(size_t)sy * pitch + (size_t)sx] = col;
        }
    }
}

void VitaUi_DrawHpBar(
    uint16_t *dst,
    const size_t pitch,
    const int32_t x,
    const int32_t y,
    const int32_t w,
    const int32_t h,
    const uint16_t hp,
    const uint16_t maxHp)
{
    VitaUiArt_FillRect(dst, pitch, &sFullClip, x, y, w, h, UI_INK_DARK);
    if (w <= 2 || h <= 2 || maxHp == 0 || hp == 0)
        return;

    int32_t fillW = ((w - 2) * (int32_t)hp) / (int32_t)maxHp;
    if (fillW < 1) fillW = 1;
    if (fillW > w - 2) fillW = w - 2;

    const uint32_t pct = ((uint32_t)hp * 100U) / (uint32_t)maxHp;
    const uint16_t col = (pct > 50U) ? UI_HP_GREEN : ((pct > 20U) ? UI_HP_YELLOW : UI_HP_RED);
    VitaUiArt_FillRect(dst, pitch, &sFullClip, x + 1, y + 1, fillW, h - 2, col);
}

void VitaUI_Init(void)
{
    if (sInitialized)
        return;
    sInitialized = true;

    VitaExtras_Init();
    VitaNuzlocke_Init();
    VitaUiPlate_Init();
    VitaUiArt_Init();

    for (int32_t i = 0; i < 16; i++)
    {
        gVitaUiArtPal565[i] = VitaUi_Bgr555ToRgb565(sArtPalette[i]);
        gVitaUiArtPal2_565[i] = VitaUi_Bgr555ToRgb565(sArtPalette2[i]);
    }
    sForceFullDirty = true;
}

void VitaUI_Prewarm(void)
{
    VitaUI_Init();
    VitaUiPlate_Prewarm();
    VitaUiArt_Prewarm();
    VitaUiMap_Prewarm();
    VitaUiArt_PrewarmPartyPics();
}

static bool IsSectionEnabled(const enum VitaUiSection sec)
{
    if (sStandaloneOptions)
        return sec == VITA_UI_SEC_OPTIONS;

    if (gMain.inBattle)
        return (sec == VITA_UI_SEC_BAG || sec == VITA_UI_SEC_POKEMON);

    const bool inSafari = GetSafariZoneFlag() == TRUE;
    const bool inPike   = InBattlePike() == TRUE;
    const bool inPyr    = CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE;

    switch (sec)
    {
    case VITA_UI_SEC_MAP:     return !inPike && !inPyr;
    case VITA_UI_SEC_POKEDEX: return !inPyr && (FlagGet(FLAG_SYS_POKEDEX_GET) == TRUE);
    case VITA_UI_SEC_POKEMON: return (FlagGet(FLAG_SYS_POKEMON_GET) == TRUE) || CalculatePlayerPartyCount() > 0;
    case VITA_UI_SEC_BAG:     return !inPike;
    case VITA_UI_SEC_POKENAV: return !inSafari && !inPike && !inPyr && (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE);
    case VITA_UI_SEC_CARD:    return true;
    case VITA_UI_SEC_SAVE:    return !inSafari && !inPike;
    case VITA_UI_SEC_OPTIONS: return true;
    case VITA_UI_SEC_NUZLOCKE:return VitaExtras_GetFast(FAST_EXTRA_NUZLOCKE) != 0;
    default:                  return false;
    }
}

static void EnterSection(const enum VitaUiSection sec, const bool flyMode, const uint8_t flySlot, const uint16_t bagItem)
{
    sActiveSec = sec;
    sSaveState = 0;
    switch (sec)
    {
    case VITA_UI_SEC_MAP:     VitaUiMap_Reset(flyMode, flySlot); break;
    case VITA_UI_SEC_POKEDEX: VitaUiPokedex_Reset(); break;
    case VITA_UI_SEC_POKEMON:
        VitaUiArt_PrewarmPartyPics();
        VitaUiParty_Reset(bagItem);
        break;
    case VITA_UI_SEC_BAG:     VitaUiBag_Reset(); break;
    case VITA_UI_SEC_POKENAV: VitaUiPokenav_Reset(); break;
    case VITA_UI_SEC_OPTIONS: VitaUiOptions_Reset(); break;
    default:                  break;
    }
    sForceFullDirty = true;
}

bool VitaUI_IsOpen(void)
{
    return sIsOpen;
}

enum VitaUiSection VitaUI_GetSection(void)
{
    return sActiveSec;
}

bool VitaUI_IsStandaloneOptions(void)
{
    return sStandaloneOptions;
}

void VitaUI_Open(enum VitaUiSection section)
{
    VitaUI_Init();
    sStandaloneOptions = false;
    if (!IsSectionEnabled(section))
    {
        for (int32_t i = 0; i < VITA_UI_SEC_COUNT; i++)
        {
            if (IsSectionEnabled((enum VitaUiSection)i))
            {
                section = (enum VitaUiSection)i;
                break;
            }
        }
    }
    if (!sIsOpen)
    {
        sIsOpen = true;
        sPrevButtons = ~0U;
        sPrevTouched = true;
        PlaySE(SE_WIN_OPEN);
    }
    EnterSection(section, false, 0, ITEM_NONE);
}

void VitaUI_OpenStandaloneOptions(void)
{
    VitaUI_Init();
    sStandaloneOptions = true;
    sIsOpen = true;
    sPrevButtons = ~0U;
    sPrevTouched = true;
    PlaySE(SE_WIN_OPEN);
    EnterSection(VITA_UI_SEC_OPTIONS, false, 0, ITEM_NONE);
}

void VitaUI_OpenFlyMap(const uint8_t partySlot)
{
    VitaUI_Init();
    sStandaloneOptions = false;
    sIsOpen = true;
    EnterSection(VITA_UI_SEC_MAP, true, partySlot, ITEM_NONE);
}

void VitaUI_OpenPartyForBagItem(const uint16_t itemId)
{
    VitaUI_Init();
    sStandaloneOptions = false;
    sIsOpen = true;
    EnterSection(VITA_UI_SEC_POKEMON, false, 0, itemId);
}

void VitaUI_OpenShopSell(const uint8_t shopTaskId)
{
    VitaUI_Init();
    sStandaloneOptions = false;
    sShopSellActive = true;
    sShopTaskId = shopTaskId;
    sIsOpen = true;
    EnterSection(VITA_UI_SEC_BAG, false, 0, ITEM_NONE);
    VitaUiBag_SetShopSellMode(true);
}

bool VitaUI_IsShopSellActive(void)
{
    return sShopSellActive;
}

void VitaUI_Close(void)
{
    if (!sIsOpen)
        return;
    const bool wasStandalone = sStandaloneOptions;
    const bool wasShopSell = sShopSellActive;
    const uint8_t shopTaskId = sShopTaskId;

    sIsOpen = false;
    sStandaloneOptions = false;
    sShopSellActive = false;
    sShopTaskId = 0;
    VitaUiBag_SetShopSellMode(false);

    if (gMain.inBattle)
    {
        if (sActiveSec == VITA_UI_SEC_BAG && gSpecialVar_ItemId == 0)
            gSpecialVar_ItemId = ITEM_NONE;
    }

    if (wasShopSell)
    {
        extern void Shop_ReturnFrom3DsSell(u8 taskId);
        Shop_ReturnFrom3DsSell(shopTaskId);
        return;
    }

    extern void CB2_Overworld(void);
    if (!wasStandalone && gMain.callback2 == CB2_Overworld && !ArePlayerFieldControlsLocked())
    {
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
    }
}

void VitaUI_Toggle(void)
{
    if (sIsOpen)
        VitaUI_Close();
    else
        VitaUI_Open(VITA_UI_SEC_MAP);
}

bool VitaUI_OnStartMenu(void)
{
    VitaUI_Open(VITA_UI_SEC_MAP);
    return true;
}

void VitaUI_Invalidate(void)
{
    sForceFullDirty = true;
}

bool VitaUI_IsRunAlwaysActive(void)
{
    return (VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0) && (FlagGet(FLAG_SYS_B_DASH) == TRUE);
}

static bool HasSubscreenModal(void)
{
    if (sActiveSec == VITA_UI_SEC_POKEMON && VitaUiParty_HasActiveModal())
        return true;
    if (sActiveSec == VITA_UI_SEC_BAG && VitaUiBag_HasActiveModal())
        return true;
    if (sActiveSec == VITA_UI_SEC_POKENAV && VitaUiPokenav_HasActiveModal())
        return true;
    if (sActiveSec == VITA_UI_SEC_OPTIONS && VitaUiOptions_HasActiveModal())
        return true;
    return false;
}

static void StepDockSection(const int32_t dir)
{
    if (sStandaloneOptions)
        return;
    int32_t cur = (int32_t)sActiveSec;
    for (int32_t tries = 0; tries < VITA_UI_SEC_COUNT; tries++)
    {
        cur = (VITA_UI_SEC_COUNT + cur + dir) % VITA_UI_SEC_COUNT;
        if (IsSectionEnabled((enum VitaUiSection)cur))
        {
            PlaySE(SE_SELECT);
            EnterSection((enum VitaUiSection)cur, false, 0, ITEM_NONE);
            return;
        }
    }
}

static uint8_t CountTrainerStars(void)
{
    uint8_t stars = 0;
    if (GetGameStat(GAME_STAT_ENTERED_HOF) > 0)
        stars++;
    if (HasAllHoennMons())
        stars++;
    if (CountPlayerMuseumPaintings() >= 5)
        stars++;
    if (FlagGet(FLAG_SYS_FRONTIER_PASS) && gSaveBlock2Ptr->frontier.cardBattlePoints > 0)
        stars++;
    return stars;
}

static bool UpdateCardSaveOrNuzlocke(const VitaUiInput *in)
{
    const uint32_t aBtn = VitaUI_ActionButton(VITA_ACTION_A);

    if (sActiveSec == VITA_UI_SEC_CARD)
    {
        if ((in->newButtons & aBtn) || VitaUi_HitRect(in, 236, 30, 74, 16))
        {
            sCardBackFace = !sCardBackFace;
            PlaySE(SE_SELECT);
            return true;
        }
        return false;
    }

    if (sActiveSec == VITA_UI_SEC_SAVE)
    {
        if ((in->newButtons & aBtn) || VitaUi_HitRect(in, 64, 168, 192, 26))
        {
            IncrementGameStat(GAME_STAT_SAVED_GAME);
            const u8 saveType = gDifferentSaveFile ? SAVE_OVERWRITE_DIFFERENT_FILE : SAVE_NORMAL;
            if (TrySavingData(saveType) == SAVE_STATUS_OK)
            {
                Platform_StoreSaveFile();
                VitaNuzlocke_SaveForSlot((int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE));
                gDifferentSaveFile = FALSE;
                sSaveState = 1;
                PlaySE(SE_SAVE);
                VitaUi_ShowToast("GAME SAVED SAFELY!");
            }
            else
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("SAVE FAILED!");
            }
            return true;
        }
        return false;
    }

    if (sActiveSec == VITA_UI_SEC_NUZLOCKE)
    {
        // A cycles current area encounter status manually if needed (Available -> Caught -> Missed -> None)
        if ((in->newButtons & aBtn) || VitaUi_HitRect(in, 10, 44, 146, 40))
        {
            const uint16_t sec = CorrectSpecialMapSecId(gMapHeader.regionMapSectionId);
            const enum VitaNuzAreaStatus next = (enum VitaNuzAreaStatus)(((uint32_t)VitaNuzlocke_GetArea(sec) + 1U) & 3U);
            VitaNuzlocke_SetArea(sec, next);
            PlaySE(SE_SELECT);
            return true;
        }
    }
    return false;
}

bool VitaUI_Update(const uint32_t buttons)
{
    if (!sIsOpen)
        return false;

    sAnimTick++;
    const uint8_t prevBlink = sBlinkPhase;
    sBlinkPhase = (uint8_t)((sAnimTick * 4U) & 0xFFU);
    bool changed = ((sBlinkPhase ^ prevBlink) & 0x80U) != 0U;

    if (sToastFrames > 0)
    {
        sToastFrames--;
        if (sToastFrames == 0)
            changed = true;
    }

    int32_t tx = -1;
    int32_t ty = -1;
    const bool touched = MapScreenTouchToUi(&tx, &ty);

    VitaUiInput in;
    in.heldButtons = buttons;
    in.newButtons = buttons & ~sPrevButtons;
    in.touched = touched;
    in.touchPressed = touched && !sPrevTouched;
    in.touchX = tx;
    in.touchY = ty;
    in.blinkPhase = sBlinkPhase;
    in.animTick = sAnimTick;

    sPrevButtons = buttons;
    sPrevTouched = touched;

    // Header hitboxes (strictly uiY < VITA_UI_HEADER_H = 22)
    if (in.touchPressed && in.touchY < VITA_UI_HEADER_H)
    {
        // Registered item shortcut [Y] on header left
        if (in.touchX < 84 && !sStandaloneOptions)
        {
            if (gSaveBlock1Ptr->registeredItem != ITEM_NONE && CheckBagHasItem(gSaveBlock1Ptr->registeredItem, 1))
            {
                PlaySE(SE_SELECT);
                VitaUI_Close();
                UseRegisteredKeyItemOnField();
                return true;
            }
            PlaySE(SE_FAILURE);
            VitaUi_ShowToast("NO REGISTERED KEY ITEM");
            return true;
        }
        // RUN shoe plate on header right
        if (in.touchX >= 256 && !sStandaloneOptions)
        {
            if (FlagGet(FLAG_SYS_B_DASH))
            {
                const int32_t nextRun = (VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) == 0) ? 1 : 0;
                VitaExtras_Set("auto_run", nextRun);
                PlaySE(SE_SELECT);
                changed = true;
            }
            else
            {
                PlaySE(SE_FAILURE);
                VitaUi_ShowToast("RUNNING SHOES NOT OBTAINED");
            }
            return true;
        }
    }

    // L / R shoulder section switching & Dock tap (unless a modal is open, in battle, or in shop sell)
    if (!HasSubscreenModal() && !sStandaloneOptions && !gMain.inBattle && !sShopSellActive)
    {
        const uint32_t lBtn = VitaUI_ActionButton(VITA_ACTION_L);
        const uint32_t rBtn = VitaUI_ActionButton(VITA_ACTION_R);
        if (in.newButtons & lBtn)
        {
            StepDockSection(-1);
            return true;
        }
        if (in.newButtons & rBtn)
        {
            StepDockSection(1);
            return true;
        }

        // Dock touch hitboxes
        if (in.touchPressed && in.touchY >= VITA_UI_DOCK_Y)
        {
            // Contextual Safari RETIRE pill on dock right
            if (GetSafariZoneFlag() == TRUE && in.touchX >= 264)
            {
                PlaySE(SE_SELECT);
                VitaUI_Close();
                SafariZoneRetirePrompt();
                return true;
            }
            const int32_t dockCount = (VitaExtras_GetFast(FAST_EXTRA_NUZLOCKE) != 0) ? 9 : 8;
            const int32_t slotW = (GetSafariZoneFlag() == TRUE ? 256 : 312) / dockCount;
            const int32_t idx = (in.touchX - 4) / slotW;
            if (idx >= 0 && idx < dockCount)
            {
                const enum VitaUiSection target = (enum VitaUiSection)idx;
                if (IsSectionEnabled(target))
                {
                    PlaySE(SE_SELECT);
                    EnterSection(target, false, 0, ITEM_NONE);
                }
                else
                {
                    PlaySE(SE_FAILURE);
                }
                return true;
            }
        }
    }
    else if (!HasSubscreenModal() && (gMain.inBattle || sShopSellActive))
    {
        if (in.touchPressed && in.touchY >= VITA_UI_DOCK_Y)
        {
            PlaySE(SE_WIN_OPEN);
            VitaUI_Close();
            return true;
        }
    }

    // Delegate to active section
    bool secChanged = false;
    switch (sActiveSec)
    {
    case VITA_UI_SEC_MAP:     secChanged = VitaUiMap_Update(&in); break;
    case VITA_UI_SEC_POKEDEX: secChanged = VitaUiPokedex_Update(&in); break;
    case VITA_UI_SEC_POKEMON: secChanged = VitaUiParty_Update(&in); break;
    case VITA_UI_SEC_BAG:     secChanged = VitaUiBag_Update(&in); break;
    case VITA_UI_SEC_POKENAV: secChanged = VitaUiPokenav_Update(&in); break;
    case VITA_UI_SEC_OPTIONS: secChanged = VitaUiOptions_Update(&in); break;
    default:                  secChanged = UpdateCardSaveOrNuzlocke(&in); break;
    }

    // Close VitaUI on B when at root level
    const uint32_t bBtn = VitaUI_ActionButton(VITA_ACTION_B);
    if (!secChanged && !HasSubscreenModal() && (in.newButtons & bBtn))
    {
        PlaySE(SE_WIN_OPEN);
        VitaUI_Close();
        return true;
    }

    if (changed || secChanged)
        sForceFullDirty = true;
    return changed || secChanged;
}

static void RenderHeaderBar(uint16_t *dst, const size_t pitch)
{
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 0, 0, VITA_UI_WIDTH, VITA_UI_HEADER_H, 0, kPlateHeader, VITA_PLATE_NORMAL, sBlinkPhase, 0);

    // 1. Registered item shortcut [SELECT] with real 24x24 icon or clean SEL badge
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 3, 2, 78, 18, 3, kPlateSageDark, VITA_PLATE_NORMAL, sBlinkPhase, 0);
    const uint16_t regItem = sStandaloneOptions ? ITEM_NONE : gSaveBlock1Ptr->registeredItem;
    if (regItem != ITEM_NONE)
    {
        VitaUiArt_DrawItemIconCentred(dst, pitch, &sFullClip, regItem, 15, 11);
        uint8_t nmGba[20];
        CopyItemName(regItem, nmGba);
        char nmAsc[14];
        VitaUiArt_GbaToAscii(nmGba, nmAsc, sizeof(nmAsc));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, nmAsc, 28, 5, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
    else
    {
        VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 4, 20, 14, 2, kPlateSage, VITA_PLATE_NORMAL, sBlinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "SEL", 6, 4, 20, 14, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "NO ITEM", 30, 5, UI_INK_MINT, VITA_UI_NO_SHADOW);
    }

    // 2. Repel step counter / Safari counter
    if (!sStandaloneOptions && GetSafariZoneFlag() == TRUE)
    {
        char safBuf[24];
        snprintf(safBuf, sizeof(safBuf), "BALL:%u", (uint32_t)gNumSafariBalls);
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, safBuf, 86, 4, UI_INK_GOLD, VITA_UI_NO_SHADOW);
    }
    else if (!sStandaloneOptions)
    {
        const uint16_t repelSteps = VarGet(VAR_REPEL_STEP_COUNT);
        if (repelSteps > 0)
        {
            char rpBuf[20];
            snprintf(rpBuf, sizeof(rpBuf), "RP:%u", repelSteps);
            VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, rpBuf, 88, 4, UI_INK_MINT, VITA_UI_NO_SHADOW);
        }
    }

    // 3. Accelerated Day/Night Clock HH:MM *Nx + Sun/Moon icon
    uint32_t dnSec = 12U * 3600U;
    uint32_t dnRate = 1U;
    VitaDayNight_Time(&dnSec, &dnRate);
    const uint32_t hh = (dnSec / 3600U) % 24U;
    const uint32_t mm = (dnSec / 60U) % 60U;
    const bool isNight = (hh < 6U || hh >= 20U);

    VitaUiArt_DrawSpriteCentred(dst, pitch, &sFullClip, isNight ? sArtMoon : sArtSun, 2, 2, 148, 11, gVitaUiArtPal2_565);
    char clkBuf[28];
    if (dnRate > 1U)
        snprintf(clkBuf, sizeof(clkBuf), "%02u:%02u *%ux", (uint32_t)hh, (uint32_t)mm, (uint32_t)dnRate);
    else
        snprintf(clkBuf, sizeof(clkBuf), "%02u:%02u", (uint32_t)hh, (uint32_t)mm);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, clkBuf, 157, 4, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // 4. RUN toggle plate with authentic sArtShoeOn / sArtShoeOff
    const bool hasShoes = !sStandaloneOptions && (FlagGet(FLAG_SYS_B_DASH) == TRUE);
    const bool runOn = hasShoes && (VitaExtras_GetFast(FAST_EXTRA_AUTO_RUN) != 0);
    VitaUiPlate_Draw(
        dst, pitch, &sFullClip, 258, 2, 59, 18, 3,
        runOn ? kPlateSage : kPlateSageDark,
        hasShoes ? (runOn ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL) : VITA_PLATE_DISABLED,
        sBlinkPhase, 0);
    if (hasShoes)
    {
        VitaUiArt_DrawSpriteCentred(dst, pitch, &sFullClip, runOn ? sArtShoeOn : sArtShoeOff, 3, 3, 270, 11, gVitaUiArtPal565);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, runOn ? "RUN ON" : "RUN", 280, 2, 37, 18, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        if (runOn)
        {
            VitaUiArt_FillRect(dst, pitch, &sFullClip, 311, 4, 2, 2, 0x2FE7);
        }
    }
    else
    {
        static uint16_t sGreyPal[16];
        static bool sGreyInit = false;
        if (!sGreyInit)
        {
            for (int32_t i = 0; i < 16; i++)
            {
                const uint16_t c = gVitaUiArtPal565[i];
                const uint8_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
                const uint8_t lum = (uint8_t)((r * 77 + (g >> 1) * 150 + b * 29) >> 8);
                sGreyPal[i] = (uint16_t)((lum << 11) | (((lum << 1) | (lum >> 4)) << 5) | lum);
            }
            sGreyInit = true;
        }
        VitaUiArt_DrawSpriteCentred(dst, pitch, &sFullClip, sArtShoeOff, 3, 3, 270, 11, sGreyPal);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "RUN", 280, 2, 37, 18, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    }
}

static void RenderDockBar(uint16_t *dst, const size_t pitch)
{
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 0, VITA_UI_DOCK_Y, VITA_UI_WIDTH, VITA_UI_DOCK_H, 0, kPlateDock, VITA_PLATE_NORMAL, sBlinkPhase, 0);

    if (gMain.inBattle)
    {
        const char *const title = (sActiveSec == VITA_UI_SEC_BAG) ? "BATTLE BAG" : "BATTLE PARTY";
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, title, 12, VITA_UI_DOCK_Y + 7, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        VitaUiPlate_Draw(dst, pitch, &sFullClip, 220, VITA_UI_DOCK_Y + 3, 94, 20, 2, kPlateSage, VITA_PLATE_NORMAL, sBlinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "B: BACK", 220, VITA_UI_DOCK_Y + 3, 94, 20, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        return;
    }

    if (sShopSellActive)
    {
        char moneyStr[36];
        snprintf(moneyStr, sizeof(moneyStr), "MART SELL  -  MONEY: $%u", (uint32_t)GetMoney(&gSaveBlock1Ptr->money));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, moneyStr, 12, VITA_UI_DOCK_Y + 7, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

        VitaUiPlate_Draw(dst, pitch, &sFullClip, 230, VITA_UI_DOCK_Y + 3, 84, 20, 2, kPlateSage, VITA_PLATE_NORMAL, sBlinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "B: EXIT", 230, VITA_UI_DOCK_Y + 3, 84, 20, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
        return;
    }

    const bool inSafari = !sStandaloneOptions && (GetSafariZoneFlag() == TRUE);
    const int32_t count = (VitaExtras_GetFast(FAST_EXTRA_NUZLOCKE) != 0) ? 9 : 8;
    const int32_t availW = inSafari ? 256 : 312;
    const int32_t slotW = availW / count;

    for (int32_t i = 0; i < count; i++)
    {
        const enum VitaUiSection sec = (enum VitaUiSection)i;
        const bool enabled = IsSectionEnabled(sec);
        const bool active = (sActiveSec == sec);
        const int32_t bx = 4 + i * slotW;

        VitaUiPlate_Draw(
            dst, pitch, &sFullClip,
            bx, VITA_UI_DOCK_Y + 3, slotW - 2, 22, 3,
            active ? kPlateSage : kPlateSageDark,
            !enabled ? VITA_PLATE_DISABLED : (active ? VITA_PLATE_FOCUS : VITA_PLATE_NORMAL),
            sBlinkPhase, 0);

        VitaUiArt_DrawDockIcon(dst, pitch, &sFullClip, (int32_t)sec, enabled, bx + (slotW - 2) / 2, VITA_UI_DOCK_Y + 14);
    }

    if (inSafari)
    {
        VitaUiPlate_Draw(dst, pitch, &sFullClip, 262, VITA_UI_DOCK_Y + 3, 54, 22, 3, kPlateCrimson, VITA_PLATE_NORMAL, sBlinkPhase, 0);
        VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "RETIRE", 262, VITA_UI_DOCK_Y + 3, 54, 22, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
    }
}

static void RenderTrainerCard(uint16_t *dst, const size_t pitch)
{
    const uint8_t stars = CountTrainerStars();
    static const VitaUiRgb sCardTierRgb[5] = {
        { 226, 238, 232 }, // 0 stars: Emerald Mint
        { 218, 178, 134 }, // 1 star: Bronze
        { 196, 210, 222 }, // 2 stars: Copper/Silver
        { 238, 212, 118 }, // 3 stars: Gold
        { 192, 234, 244 }  // 4 stars: Platinum Crystal
    };

    VitaUiPlate_Draw(dst, pitch, &sFullClip, 10, 26, 300, 180, 6, sCardTierRgb[stars % 5U], VITA_PLATE_NORMAL, sBlinkPhase, 18);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL,
                           sCardBackFace ? "TRAINER CARD - RECORD BACK" : "HOENN LEAGUE TRAINER CARD",
                           18, 28, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Stars in header
    for (uint8_t s = 0; s < stars && s < 4; s++)
        VitaUiArt_DrawSpriteCentred(dst, pitch, &sFullClip, sArtStar, 2, 2, 192 + (int32_t)s * 10, 35, gVitaUiArtPal2_565);

    VitaUiPlate_Draw(dst, pitch, &sFullClip, 238, 28, 66, 14, 2, kPlateSage, VITA_PLATE_NORMAL, sBlinkPhase, 0);
    VitaUiArt_DrawStrAsciiInPlate(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "A: FLIP", 238, 28, 66, 14, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    if (sCardBackFace)
    {
        char line[56];
        snprintf(line, sizeof(line), "HALL OF FAME DEBUT: %u TIMES", (uint32_t)GetGameStat(GAME_STAT_ENTERED_HOF));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 22, 54, UI_INK_DARK, VITA_UI_NO_SHADOW);

        snprintf(line, sizeof(line), "POKEMON CAUGHT: %u    TRADES: %u",
                 (uint32_t)GetGameStat(GAME_STAT_POKEMON_CAPTURES), (uint32_t)GetGameStat(GAME_STAT_POKEMON_TRADES));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 22, 76, UI_INK_DARK, VITA_UI_NO_SHADOW);

        snprintf(line, sizeof(line), "POKEBLOCKS WITH FRIENDS: %u", (uint32_t)GetGameStat(GAME_STAT_POKEBLOCKS));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 22, 98, UI_INK_DARK, VITA_UI_NO_SHADOW);

        snprintf(line, sizeof(line), "CONTESTS WON: %u", (uint32_t)GetGameStat(GAME_STAT_WON_CONTEST));
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 22, 120, UI_INK_DARK, VITA_UI_NO_SHADOW);

        snprintf(line, sizeof(line), "BATTLE FRONTIER BP: %u", (uint32_t)gSaveBlock2Ptr->frontier.cardBattlePoints);
        VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 22, 142, UI_INK_BLUE, VITA_UI_NO_SHADOW);
        return;
    }

    // Front face: Trainer info on left + High-detail 64x64 Brendan/May portrait on right
    char buf[48];
    char nameAsc[16];
    VitaUiArt_GbaToAscii(gSaveBlock2Ptr->playerName, nameAsc, sizeof(nameAsc));
    const uint16_t tid = (uint16_t)((gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0]);

    snprintf(buf, sizeof(buf), "NAME: %s", nameAsc);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, buf, 20, 52, UI_INK_DARK, VITA_UI_NO_SHADOW);
    snprintf(buf, sizeof(buf), "IDNo. %05u", tid);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, buf, 134, 52, UI_INK_MUTED, VITA_UI_NO_SHADOW);

    snprintf(buf, sizeof(buf), "MONEY: $%u", (uint32_t)GetMoney(&gSaveBlock1Ptr->money));
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, buf, 20, 72, UI_INK_DARK, VITA_UI_NO_SHADOW);

    snprintf(buf, sizeof(buf), "POKEDEX: %u", (uint32_t)GetHoennPokedexCount(FLAG_GET_CAUGHT));
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, buf, 20, 92, UI_INK_DARK, VITA_UI_NO_SHADOW);

    snprintf(buf, sizeof(buf), "TIME: %u:%02u",
             (uint32_t)gSaveBlock2Ptr->playTimeHours, (uint32_t)gSaveBlock2Ptr->playTimeMinutes);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, buf, 20, 112, UI_INK_DARK, VITA_UI_NO_SHADOW);

    VitaUiPlate_Draw(dst, pitch, &sFullClip, 226, 52, 72, 74, 4, kPlateCream, VITA_PLATE_NORMAL, sBlinkPhase, 0);
    VitaUiArt_DrawTrainerFrontPic(dst, pitch, &sFullClip, gSaveBlock2Ptr->playerGender, 230, 57);

    // 8 Hoenn Gym Badges with BGR555 -> RGB565 conversion!
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 18, 142, 284, 56, 4, kPlateHeader, VITA_PLATE_NORMAL, sBlinkPhase, 0);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, "HOENN LEAGUE BADGES", 26, 146, UI_INK_MINT, VITA_UI_NO_SHADOW);

    static uint8_t sBadgesDec[0x800];
    static bool sBadgesReady = false;
    static uint16_t sBadgePals[8][16];
    static uint16_t sBadgeDim565[16];
    if (!sBadgesReady)
    {
        LZ77UnCompWram(GetHoennTrainerCardBadges_Gfx(), sBadgesDec);

        // Silueta salvia tenue 1:1 Emerald 3DS para medallas no obtenidas
        sBadgeDim565[0] = 0x0000;
        for (int32_t i = 1; i < 16; i++)
            sBadgeDim565[i] = VitaUi_PackRgb565(176, 196, 184);

        // Paletas cromaticas autenticas de las 8 medallas de Hoenn (Piedra, Puño, Dinamo, Calor, Alma, Pluma, Mente, Lluvia)
        // El sprite 4bpp original utiliza los indices: 1=reflejo blanco, 2=color base claro, 3=color principal, 4/15=sombra/relieve
        static const uint16_t sHoennBadgeColors[8][4] = {
            { 0xFFFF, 0xCE79, 0x8C71, 0x4A49 }, // Piedra (Granito / Gris roca)
            { 0xFFFF, 0x5D3F, 0x2A18, 0x18C8 }, // Puño (Azul marino / Lucha)
            { 0xFFFF, 0xFFE0, 0xFE40, 0x33B9 }, // Dinamo (Amarillo electrico / Oro)
            { 0xFFFF, 0xFD60, 0xF940, 0x9800 }, // Calor (Naranja fuego / Carmesi)
            { 0xFFFF, 0xFC08, 0xB800, 0x5800 }, // Alma (Rojo vivo / Yin-Yang)
            { 0xFFFF, 0x87FF, 0x2C7A, 0x1290 }, // Pluma (Celeste cielo / Turquesa)
            { 0xFFFF, 0xFDC4, 0xC216, 0x690F }, // Mente (Ambar psiquico / Amatista)
            { 0xFFFF, 0x7E1F, 0x235F, 0x1134 }  // Lluvia (Azul hielo / Zafiro marino)
        };

        for (int32_t b = 0; b < 8; b++)
        {
            sBadgePals[b][0] = 0x0000; // Transparente
            sBadgePals[b][1] = sHoennBadgeColors[b][0];
            sBadgePals[b][2] = sHoennBadgeColors[b][1];
            sBadgePals[b][3] = sHoennBadgeColors[b][2];
            sBadgePals[b][4] = sHoennBadgeColors[b][3];
            sBadgePals[b][15] = sHoennBadgeColors[b][3];
        }
        sBadgesReady = true;
    }

    for (int32_t b = 0; b < 8; b++)
    {
        const int32_t bx = 28 + b * 34;
        const int32_t by = 164;
        const bool earned = FlagGet(FLAG_BADGE01_GET + b) == TRUE;

        // La imagen descomprimida mide 16x2 tiles (128x16 px).
        // Cada medalla b se compone de los cuadrantes (2b, 2b+1) en la fila superior
        // y (2b+16, 2b+17) en la fila inferior.
        uint8_t badgeTiles[128];
        memcpy(&badgeTiles[0],  &sBadgesDec[(2 * b) * 32],      32);
        memcpy(&badgeTiles[32], &sBadgesDec[(2 * b + 1) * 32],  32);
        memcpy(&badgeTiles[64], &sBadgesDec[(2 * b + 16) * 32], 32);
        memcpy(&badgeTiles[96], &sBadgesDec[(2 * b + 17) * 32], 32);

        VitaUiArt_DrawTiles4bpp(
            dst, pitch, &sFullClip,
            badgeTiles, 2, 2, bx, by,
            earned ? sBadgePals[b] : sBadgeDim565);
    }
}

static void RenderSaveScreen(uint16_t *dst, const size_t pitch)
{
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 16, 28, 288, 134, 5, kPlateCream, VITA_PLATE_NORMAL, sBlinkPhase, 18);

    uint8_t mapGba[28];
    char mapAsc[28];
    GetMapNameGeneric(mapGba, CorrectSpecialMapSecId(gMapHeader.regionMapSectionId));
    VitaUiArt_GbaToAscii(mapGba, mapAsc, sizeof(mapAsc));
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, mapAsc, 24, 30, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    char slotLbl[20];
    snprintf(slotLbl, sizeof(slotLbl), "SLOT %u", (uint32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE));
    VitaUiArt_DrawStrAsciiRight(dst, pitch, &sFullClip, VITA_FONT_SMALL, slotLbl, 296, 30, UI_INK_MINT, VITA_UI_NO_SHADOW);

    char nameAsc[16];
    VitaUiArt_GbaToAscii(gSaveBlock2Ptr->playerName, nameAsc, sizeof(nameAsc));
    char line[48];
    snprintf(line, sizeof(line), "PLAYER: %s     BADGES: %u", nameAsc, (uint32_t)VitaNuzlocke_CountBadges());
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 26, 54, UI_INK_DARK, VITA_UI_NO_SHADOW);

    snprintf(line, sizeof(line), "POKEDEX: %u     PLAY TIME: %u:%02u",
             (uint32_t)GetHoennPokedexCount(FLAG_GET_CAUGHT),
             (uint32_t)gSaveBlock2Ptr->playTimeHours,
             (uint32_t)gSaveBlock2Ptr->playTimeMinutes);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, line, 26, 72, UI_INK_DARK, VITA_UI_NO_SHADOW);

    // 6 Party Pokemon Icons preview row
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 26, 94, 268, 38, 4, kPlateCardSel, VITA_PLATE_NORMAL, sBlinkPhase, 0);
    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();
    const uint8_t iconFrame = (uint8_t)((sAnimTick >> 4) & 1U);
    for (int32_t i = 0; i < PARTY_SIZE; i++)
    {
        const int32_t ix = 34 + i * 43;
        if (i < partyCnt)
        {
            const bool isEgg = GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG) != 0;
            const uint16_t sp = isEgg ? SPECIES_EGG : (uint16_t)GetMonData(&gPlayerParty[i], MON_DATA_SPECIES);
            VitaUiArt_DrawMonIcon(dst, pitch, &sFullClip, sp, iconFrame, ix, 96, 1);
        }
        else
        {
            VitaUi_DrawMiniPokeball(dst, pitch, ix + 12, 110, false, true);
        }
    }

    if (gDifferentSaveFile)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW,
                                     "WARNING: OVERWRITING DIFFERENT SAVE!", 160, 140, UI_INK_RED, VITA_UI_NO_SHADOW);
    }
    else if (sSaveState == 1)
    {
        VitaUiArt_DrawStrAsciiCenter(dst, pitch, &sFullClip, VITA_FONT_SMALL,
                                     "SAVED TO UX0:DATA/POKEVITA/!", 160, 140, UI_HP_GREEN, VITA_UI_NO_SHADOW);
    }

    VitaUiPlate_Draw(dst, pitch, &sFullClip, 64, 168, 192, 28, 4, kPlateSage, VITA_PLATE_FOCUS, sBlinkPhase, 0);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, &sFullClip, VITA_FONT_NORMAL, "A: SAVE ADVENTURE", 160, 174, UI_INK_LIGHT, VITA_UI_NO_SHADOW);
}

static void RenderNuzlockeScreen(uint16_t *dst, const size_t pitch)
{
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 26, 150, 180, 4, kPlateCream, VITA_PLATE_NORMAL, sBlinkPhase, 16);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, "NUZLOCKE TRACKER", 12, 28, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Current area encounter status
    const uint16_t sec = CorrectSpecialMapSecId(gMapHeader.regionMapSectionId);
    uint8_t secGba[24];
    char secAsc[24];
    GetMapNameGeneric(secGba, sec);
    VitaUiArt_GbaToAscii(secGba, secAsc, sizeof(secAsc));
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, secAsc, 14, 48, UI_INK_BLUE, VITA_UI_NO_SHADOW);

    static const char *const sAreaLbl[4] = { "OPEN ENCOUNTER", "AVAILABLE", "CAUGHT!", "MISSED / FLED" };
    const enum VitaNuzAreaStatus st = VitaNuzlocke_GetArea(sec);
    VitaUiPlate_Draw(
        dst, pitch, &sFullClip, 12, 62, 138, 22, 3,
        st == VITA_NUZ_AREA_CAUGHT ? kPlateSage : (st == VITA_NUZ_AREA_MISSED ? kPlateCrimson : kPlateBlueMove),
        VITA_PLATE_FOCUS, sBlinkPhase, 0);
    VitaUiArt_DrawStrAsciiCenter(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, sAreaLbl[st & 3U], 81, 66, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Next Gym Level Cap
    const char *leader = NULL;
    const uint8_t cap = VitaNuzlocke_GetLevelCap(VitaNuzlocke_CountBadges(), &leader);
    char capBuf[36];
    snprintf(capBuf, sizeof(capBuf), "NEXT: %s (CAP LV.%u)", leader, cap);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, capBuf, 14, 96, UI_INK_DARK, VITA_UI_NO_SHADOW);

    // Counts
    uint32_t caughtCnt = 0, missedCnt = 0, fallenCnt = 0;
    VitaNuzlocke_GetCounts(&caughtCnt, &missedCnt, &fallenCnt);
    snprintf(capBuf, sizeof(capBuf), "CAUGHT AREAS: %u", (uint32_t)caughtCnt);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, capBuf, 14, 120, UI_HP_GREEN, VITA_UI_NO_SHADOW);
    snprintf(capBuf, sizeof(capBuf), "MISSED AREAS: %u", (uint32_t)missedCnt);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, capBuf, 14, 140, UI_INK_MUTED, VITA_UI_NO_SHADOW);
    snprintf(capBuf, sizeof(capBuf), "FALLEN ALLIES: %u", (uint32_t)fallenCnt);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, capBuf, 14, 160, UI_INK_RED, VITA_UI_NO_SHADOW);

    // Right panel: Party Nuzlocke alive / fallen roster
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 160, 26, 154, 180, 4, kPlateCream, VITA_PLATE_NORMAL, sBlinkPhase, 16);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, "SQUAD VITAL STATUS", 168, 28, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    const int32_t partyCnt = (int32_t)CalculatePlayerPartyCount();
    for (int32_t i = 0; i < partyCnt; i++)
    {
        struct Pokemon *const mon = &gPlayerParty[i];
        const int32_t ry = 46 + i * 26;
        const bool isEgg = GetMonData(mon, MON_DATA_IS_EGG) != 0;
        const uint16_t sp = isEgg ? SPECIES_EGG : (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
        const bool fallen = !isEgg && (GetMonData(mon, MON_DATA_HP) == 0
            || VitaNuzlocke_IsFallen(GetMonData(mon, MON_DATA_PERSONALITY), GetMonData(mon, MON_DATA_OT_ID), sp == SPECIES_SHEDINJA));

        VitaUiArt_DrawMonIcon(dst, pitch, &sFullClip, sp, 0, 164, ry - 4, 1);
        uint8_t nick[POKEMON_NAME_LENGTH + 2];
        GetMonNickname(mon, nick);
        VitaUiArt_DrawStrGba(dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW, nick, 196, ry + 4, UI_INK_DARK, VITA_UI_NO_SHADOW);
        VitaUiArt_DrawStrAsciiRight(
            dst, pitch, &sFullClip, VITA_FONT_SMALL_NARROW,
            isEgg ? "EGG" : (fallen ? "FALLEN" : "ALIVE"),
            308, ry + 4, fallen ? UI_INK_RED : UI_HP_GREEN, VITA_UI_NO_SHADOW);
    }
}

void VitaUI_Render(uint16_t *dest, const size_t pitchPixels)
{
    VitaUI_Init();
    VitaUiPlate_BlitMenuBackdrop(dest, pitchPixels, &sFullClip);
    RenderHeaderBar(dest, pitchPixels);

    switch (sActiveSec)
    {
    case VITA_UI_SEC_MAP:      VitaUiMap_Render(dest, pitchPixels, sBlinkPhase, sAnimTick); break;
    case VITA_UI_SEC_POKEDEX:  VitaUiPokedex_Render(dest, pitchPixels, sBlinkPhase, sAnimTick); break;
    case VITA_UI_SEC_POKEMON:  VitaUiParty_Render(dest, pitchPixels, sBlinkPhase, sAnimTick); break;
    case VITA_UI_SEC_BAG:      VitaUiBag_Render(dest, pitchPixels, sBlinkPhase); break;
    case VITA_UI_SEC_POKENAV:  VitaUiPokenav_Render(dest, pitchPixels, sBlinkPhase, sAnimTick); break;
    case VITA_UI_SEC_CARD:     RenderTrainerCard(dest, pitchPixels); break;
    case VITA_UI_SEC_SAVE:     RenderSaveScreen(dest, pitchPixels); break;
    case VITA_UI_SEC_OPTIONS:  VitaUiOptions_Render(dest, pitchPixels, sBlinkPhase); break;
    case VITA_UI_SEC_NUZLOCKE: RenderNuzlockeScreen(dest, pitchPixels); break;
    default:                   break;
    }

    RenderDockBar(dest, pitchPixels);

    if (sToastFrames > 0)
    {
        VitaUiPlate_Draw(dest, pitchPixels, &sFullClip, 44, 186, 232, 20, 4, kPlatePopup, VITA_PLATE_FOCUS, sBlinkPhase, 0);
        VitaUiArt_DrawStrAsciiCenter(dest, pitchPixels, &sFullClip, VITA_FONT_SMALL, sToastMsg, 160, 189, UI_INK_MINT, VITA_UI_NO_SHADOW);
    }
}

bool VitaUI_RenderDirty(const uint16_t **outCanvas, int32_t *outY0, int32_t *outY1)
{
    VitaUI_Init();
    *outCanvas = sCanvasCur;
    if (!sForceFullDirty && sToastFrames == 0)
    {
        *outY0 = 0;
        *outY1 = 0;
        return false;
    }

    VitaUI_Render(sCanvasCur, VITA_UI_WIDTH);

    if (sForceFullDirty)
    {
        sForceFullDirty = false;
        memcpy(sCanvasPrev, sCanvasCur, sizeof(sCanvasCur));
        *outY0 = 0;
        *outY1 = VITA_UI_HEIGHT;
        return true;
    }

    int32_t firstRow = VITA_UI_HEIGHT;
    int32_t lastRow = 0;
    for (int32_t y = 0; y < VITA_UI_HEIGHT; y++)
    {
        const size_t off = (size_t)y * VITA_UI_WIDTH;
        if (memcmp(&sCanvasCur[off], &sCanvasPrev[off], VITA_UI_WIDTH * sizeof(uint16_t)) != 0)
        {
            if (y < firstRow) firstRow = y;
            lastRow = y + 1;
            memcpy(&sCanvasPrev[off], &sCanvasCur[off], VITA_UI_WIDTH * sizeof(uint16_t));
        }
    }

    sForceFullDirty = false;
    if (firstRow >= lastRow)
    {
        *outY0 = 0;
        *outY1 = 0;
        return false;
    }
    *outY0 = firstRow;
    *outY1 = lastRow;
    return true;
}

// ---- Platform Input & Save Path Helpers (replaces deprecated vita_menu.c) ----

uint32_t VitaUI_ActionButton(const int32_t action)
{
    if (action < 0 || action >= VITA_ACTION_COUNT)
        return 0U;
    const u8 bit = Platform_GetSetting((enum PlatformSetting)(PLATFORM_SETTING_BUTTON_FIRST + action));
    return bit < 16 ? (1U << bit) : 0U;   // VITA_BTN_UNBOUND: no button
}

uint16_t VitaUI_MapButtons(const uint32_t buttons)
{
    static const uint16_t sGbaKeys[VITA_ACTION_COUNT] = {
        A_BUTTON, B_BUTTON, L_BUTTON, R_BUTTON, START_BUTTON, SELECT_BUTTON, 0, 0
    };
    uint16_t keys = 0;
    if (buttons & VITA_BTN_UP)    keys |= DPAD_UP;
    if (buttons & VITA_BTN_DOWN)  keys |= DPAD_DOWN;
    if (buttons & VITA_BTN_LEFT)  keys |= DPAD_LEFT;
    if (buttons & VITA_BTN_RIGHT) keys |= DPAD_RIGHT;

    for (int32_t a = 0; a < VITA_ACTION_COUNT; a++)
    {
        if ((buttons & VitaUI_ActionButton(a)) != 0U)
            keys |= sGbaKeys[a];
    }
    return keys;
}

bool VitaUI_FastForwardActive(const uint32_t buttons)
{
    static bool sToggled = false;
    static bool sWasPressed = false;
    if (sIsOpen)
        return false;

    const bool pressed = (buttons & VitaUI_ActionButton(VITA_ACTION_FAST_FORWARD)) != 0U;
    if (!Platform_GetSetting(PLATFORM_SETTING_FAST_FORWARD_TOGGLE))
    {
        sToggled = false;
        sWasPressed = pressed;
        return pressed;
    }
    if (pressed && !sWasPressed)
        sToggled = !sToggled;
    sWasPressed = pressed;
    return sToggled;
}

void VitaUI_GetSavePath(char *dest, const size_t size)
{
    const int32_t file = (int32_t)Platform_GetSetting(PLATFORM_SETTING_SAVE_FILE);
    char rel[32];
    if (file <= 1)
        snprintf(rel, sizeof(rel), "saves/pokeemerald.sav");
    else
        snprintf(rel, sizeof(rel), "saves/pokeemerald_%d.sav", (int32_t)file);
    VitaPlatform_BuildPath(dest, size, rel);
}
