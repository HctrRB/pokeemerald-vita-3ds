#ifndef GUARD_VITA_UI_INTERNAL_H
#define GUARD_VITA_UI_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/vita_ui.h"
#include "platform/vita_ui_plate.h"
#include "platform/vita_ui_art.h"
#include "vita_platform.h"

enum
{
    VITA_UI_HEADER_H  = 22,
    VITA_UI_CONTENT_Y = 23,
    VITA_UI_CONTENT_H = 186,
    VITA_UI_DOCK_Y    = 211,
    VITA_UI_DOCK_H    = 28
};

// Emerald 3DS canonical RGB themes (for SDF bevelled plates)
static const VitaUiRgb kPlateHeader     = {  28,  58,  52 };
static const VitaUiRgb kPlateDock       = {  24,  50,  46 };
static const VitaUiRgb kPlateSage       = {  88, 146, 126 };
static const VitaUiRgb kPlateSageDark   = {  52,  94,  82 };
static const VitaUiRgb kPlateCream      = { 242, 246, 241 };
static const VitaUiRgb kPlateCard       = { 250, 252, 249 };
static const VitaUiRgb kPlateCardSel    = { 218, 238, 228 };
static const VitaUiRgb kPlateDexRed     = { 196,  46,  48 };
static const VitaUiRgb kPlateDexLens    = {  48, 178, 214 };
static const VitaUiRgb kPlateBlueMove   = {  62, 128, 206 };
static const VitaUiRgb kPlateGold       = { 218, 174,  58 };
static const VitaUiRgb kPlateCrimson    = { 188,  56,  56 };
static const VitaUiRgb kPlatePopup      = {  36,  68,  60 };

// Crisp RGB565 ink & accent colors
#define UI_INK_DARK      VitaUi_PackRgb565( 28,  40,  38)
#define UI_INK_MUTED     VitaUi_PackRgb565( 88, 108, 102)
#define UI_INK_LIGHT     VitaUi_PackRgb565(245, 250, 247)
#define UI_INK_MINT      VitaUi_PackRgb565(172, 236, 206)
#define UI_INK_GOLD      VitaUi_PackRgb565(232, 182,  36)
#define UI_INK_GOLD_DARK VitaUi_PackRgb565(168, 118,   8)
#define UI_INK_RED       VitaUi_PackRgb565(214,  52,  52)
#define UI_INK_BLUE      VitaUi_PackRgb565( 44, 112, 204)
#define UI_HP_GREEN      VitaUi_PackRgb565( 56, 204,  88)
#define UI_HP_YELLOW     VitaUi_PackRgb565(232, 188,  36)
#define UI_HP_RED        VitaUi_PackRgb565(220,  58,  52)
#define UI_BORDER_SOFT   VitaUi_PackRgb565(184, 202, 194)

typedef struct
{
    uint32_t heldButtons;
    uint32_t newButtons;
    bool touched;
    bool touchPressed;
    int32_t touchX;
    int32_t touchY;
    uint8_t blinkPhase;
    uint32_t animTick;
} VitaUiInput;

// Shared state & art helpers exposed by vita_ui.c
extern uint16_t gVitaUiArtPal565[16];
extern uint16_t gVitaUiArtPal2_565[16];

const VitaUiClip *VitaUi_FullClip(void);
void VitaUi_ShowToast(const char *msg);
void VitaUi_DrawMiniPokeball(uint16_t *dst, size_t pitch, int32_t x, int32_t y, bool caught, bool faint);
void VitaUi_DrawArt4bpp(
    uint16_t *dst,
    size_t pitch,
    const uint8_t *art,
    int32_t w,
    int32_t h,
    int32_t x,
    int32_t y,
    const uint16_t *pal565);
void VitaUi_DrawHpBar(
    uint16_t *dst,
    size_t pitch,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    uint16_t hp,
    uint16_t maxHp);

static inline bool VitaUi_HitRect(
    const VitaUiInput *in,
    const int32_t x,
    const int32_t y,
    const int32_t w,
    const int32_t h)
{
    return in->touchPressed
        && in->touchX >= x && in->touchX < (x + w)
        && in->touchY >= y && in->touchY < (y + h);
}

// Section modules
void VitaUiMap_Prewarm(void);
void VitaUiMap_Reset(bool flyMode, uint8_t flyPartySlot);
bool VitaUiMap_Update(const VitaUiInput *in);
void VitaUiMap_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase, uint32_t animTick);
void VitaUiMap_BlitHoennThumbnail(uint16_t *dst, size_t pitch, int32_t dstX, int32_t dstY, int32_t dstW, int32_t dstH);

void VitaUiPokedex_Reset(void);
bool VitaUiPokedex_Update(const VitaUiInput *in);
void VitaUiPokedex_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase, uint32_t animTick);

void VitaUiParty_Reset(uint16_t pendingBagItem);
bool VitaUiParty_Update(const VitaUiInput *in);
void VitaUiParty_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase, uint32_t animTick);
bool VitaUiParty_HasActiveModal(void);

void VitaUiBag_Reset(void);
void VitaUiBag_SetShopSellMode(bool inShopSell);
bool VitaUiBag_IsShopSellMode(void);
bool VitaUiBag_Update(const VitaUiInput *in);
void VitaUiBag_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase);
bool VitaUiBag_HasActiveModal(void);

void VitaUiPokenav_Reset(void);
bool VitaUiPokenav_Update(const VitaUiInput *in);
void VitaUiPokenav_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase, uint32_t animTick);
bool VitaUiPokenav_HasActiveModal(void);

void VitaUiOptions_Reset(void);
bool VitaUiOptions_Update(const VitaUiInput *in);
void VitaUiOptions_Render(uint16_t *dst, size_t pitch, uint8_t blinkPhase);
bool VitaUiOptions_HasActiveModal(void);

#endif // GUARD_VITA_UI_INTERNAL_H
