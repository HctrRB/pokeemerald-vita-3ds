#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_shop_ui.h"

#include "global.h"
#include "item.h"
#include "money.h"
#include "shop.h"
#include "sound.h"
#include "strings.h"
#include "string_util.h"
#include "constants/songs.h"

#include <stdio.h>
#include <string.h>

static bool sShopOpen = false;
static const VitaUiClip sFullClip = { 0, 0, 320, 240 };

void VitaShopUI_Init(void)
{
    sShopOpen = false;
}

bool VitaShopUI_IsOpen(void)
{
    return sShopOpen;
}

void VitaShopUI_Close(void)
{
    sShopOpen = false;
}

bool VitaShopUI_Update(uint32_t keys)
{
    if (!sShopOpen)
        return false;

    if (keys & 0x0002) // B button
    {
        sShopOpen = false;
        PlaySE(SE_SELECT);
        return true;
    }

    return true;
}

bool VitaShopUI_Render(uint16_t *dst, int32_t pitch)
{
    if (!sShopOpen)
        return false;

    // Header plate: POKéMON MART & Current Money
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 4, 308, 22, 4, kPlateSage, VITA_PLATE_NORMAL, 0, 0);
    VitaUiArt_DrawStrAscii(dst, pitch, &sFullClip, VITA_FONT_SMALL, "POKéMON MART", 14, 8, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    char moneyStr[32];
    snprintf(moneyStr, sizeof(moneyStr), "$%u", (uint32_t)GetMoney(&gSaveBlock1Ptr->money));
    VitaUiArt_DrawStrAsciiRight(dst, pitch, &sFullClip, VITA_FONT_SMALL, moneyStr, 306, 8, UI_INK_LIGHT, VITA_UI_NO_SHADOW);

    // Main item list plate
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 30, 308, 160, 6, kPlateCream, VITA_PLATE_NORMAL, 0, 0);

    // Bottom description plate
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 194, 308, 42, 4, kPlateCream, VITA_PLATE_NORMAL, 0, 0);

    return true;
}
