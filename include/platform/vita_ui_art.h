#ifndef GUARD_VITA_UI_ART_H
#define GUARD_VITA_UI_ART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/vita_ui_plate.h"

enum VitaUiFontId
{
    VITA_FONT_SMALL        = 0,
    VITA_FONT_NORMAL       = 1,
    VITA_FONT_SMALL_NARROW = 2,
    VITA_FONT_NARROW       = 3,
    VITA_FONT_COUNT        = 4
};

enum
{
    VITA_UI_NO_SHADOW = 0xFFFFFFFFU
};

void VitaUiArt_Init(void);
void VitaUiArt_Shutdown(void);
void VitaUiArt_Prewarm(void);
void VitaUiArt_PrewarmPartyPics(void);

// Charset conversion (supports UTF-8 Spanish accents/ñ/¡/¿ <-> GBA charmap)
void VitaUiArt_AsciiToGba(const char *ascii, uint8_t *outGba, size_t cap);
void VitaUiArt_GbaToAscii(const uint8_t *gba, char *outAscii, size_t cap);

// Pixel-exact decomp font metrics & rendering (pass VITA_UI_NO_SHADOW for clean unshadowed text)
int32_t VitaUiArt_TextY(int32_t py, int32_t ph, enum VitaUiFontId fontId);
int32_t VitaUiArt_StrWidthGba(enum VitaUiFontId fontId, const uint8_t *gbaStr);
int32_t VitaUiArt_StrWidthAscii(enum VitaUiFontId fontId, const char *asciiStr);

int32_t VitaUiArt_DrawStrGba(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const uint8_t *gbaStr,
    int32_t x,
    int32_t y,
    uint16_t fg565,
    uint32_t shadow565OrNone);

int32_t VitaUiArt_DrawStrAscii(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t x,
    int32_t y,
    uint16_t fg565,
    uint32_t shadow565OrNone);

int32_t VitaUiArt_DrawStrAsciiInPlate(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t px,
    int32_t py,
    int32_t pw,
    int32_t ph,
    uint16_t fg565,
    uint32_t shadow565OrNone);

int32_t VitaUiArt_DrawStrAsciiInPlateEx(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t px,
    int32_t py,
    int32_t pw,
    int32_t ph,
    char align,
    int32_t pad,
    uint16_t fg565,
    uint32_t shadow565OrNone);

int32_t VitaUiArt_DrawStrGbaInPlate(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const uint8_t *gbaStr,
    int32_t px,
    int32_t py,
    int32_t pw,
    int32_t ph,
    uint16_t fg565,
    uint32_t shadow565OrNone);

int32_t VitaUiArt_DrawStrGbaInPlateEx(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const uint8_t *gbaStr,
    int32_t px,
    int32_t py,
    int32_t pw,
    int32_t ph,
    char align,
    int32_t pad,
    uint16_t fg565,
    uint32_t shadow565OrNone);

void VitaUiArt_DrawStrAsciiRight(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t rightX,
    int32_t y,
    uint16_t fg565,
    uint32_t shadow565OrNone);

void VitaUiArt_DrawStrAsciiCenter(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t centerX,
    int32_t y,
    uint16_t fg565,
    uint32_t shadow565OrNone);

// Super-sampled smooth outlined text (k in quarter-pixels: 4 = 1.0x, 5 = 1.25x, 6 = 1.5x, 7 = 1.75x)
int32_t VitaUiArt_SmoothWidthGba(enum VitaUiFontId fontId, const uint8_t *gbaStr, int32_t scaleQuarter);
void VitaUiArt_DrawSmoothGba(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const uint8_t *gbaStr,
    int32_t x,
    int32_t y,
    int32_t scaleQuarter,
    VitaUiRgb fg,
    VitaUiRgb outline,
    bool dropShadow);

void VitaUiArt_DrawSmoothAscii(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    enum VitaUiFontId fontId,
    const char *asciiStr,
    int32_t x,
    int32_t y,
    int32_t scaleQuarter,
    VitaUiRgb fg,
    VitaUiRgb outline,
    bool dropShadow);

// Clipped primitives
void VitaUiArt_FillRect(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    uint16_t col565);

void VitaUiArt_DrawTiles4bpp(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    const uint8_t *tiles,
    int32_t tilesW,
    int32_t tilesH,
    int32_t x,
    int32_t y,
    const uint16_t *pal565);

void VitaUiArt_DrawSpriteCentred(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    const uint8_t *tiles,
    int32_t tilesW,
    int32_t tilesH,
    int32_t cx,
    int32_t cy,
    const uint16_t *pal565);

void VitaUiArt_DrawTiles4bppBgrPal(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    const uint8_t *tiles,
    int32_t tilesW,
    int32_t tilesH,
    int32_t x,
    int32_t y,
    const uint16_t *palBgr555);

// Cached zero-LZ77-per-frame game sprites & badges
void VitaUiArt_DrawMonFrontPic(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t species,
    bool shiny,
    int32_t x,
    int32_t y,
    int32_t targetSize,
    bool silhouette);

void VitaUiArt_DrawMonIcon(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t species,
    uint8_t frame,
    int32_t x,
    int32_t y,
    int32_t scale);

void VitaUiArt_DrawMonIconCentred(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t species,
    uint8_t frame,
    int32_t cx,
    int32_t cy);

void VitaUiArt_DrawItemIcon(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t itemId,
    int32_t x,
    int32_t y,
    bool shadowOnly);

void VitaUiArt_DrawItemIconCentred(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t itemId,
    int32_t cx,
    int32_t cy);

void VitaUiArt_DrawItemIconCentredEx(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t itemId,
    int32_t cx,
    int32_t cy,
    bool grey);

void VitaUiArt_DrawDockIcon(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    int32_t section,
    bool enabled,
    int32_t cx,
    int32_t cy);

void VitaUiArt_DrawBagPic(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    bool female,
    uint8_t pocketFrame,
    int32_t x,
    int32_t y);

void VitaUiArt_DrawTrainerFrontPic(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint8_t gender,
    int32_t x,
    int32_t y);

void VitaUiArt_DrawTypeBadge(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint8_t moveType,
    int32_t x,
    int32_t y);

void VitaUiArt_DrawCategoryBadge(
    uint16_t *dst,
    size_t pitch,
    const VitaUiClip *clip,
    uint16_t moveId,
    int32_t x,
    int32_t y);

VitaUiRgb VitaUiArt_GetTypeRgb(uint8_t moveType);

#endif // GUARD_VITA_UI_ART_H
