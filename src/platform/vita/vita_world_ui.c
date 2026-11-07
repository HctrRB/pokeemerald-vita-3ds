#include <stdint.h>
#include "vita_ui_internal.h"
#include "platform/vita_world_ui.h"

#include "global.h"
#include "main.h"
#include "event_data.h"
#include "field_message_box.h"
#include "map_name_popup.h"
#include "menu.h"
#include "overworld.h"
#include "script_menu.h"
#include "string_util.h"
#include "text.h"

#include <stdio.h>
#include <string.h>

static const VitaUiClip sFullClip = { 0, 0, 320, 240 };

void VitaWorldUI_Init(void)
{
}

bool VitaWorldUI_IsActive(void)
{
    if (gMain.callback2 != CB2_Overworld)
        return false;

    if (!IsFieldMessageBoxHidden())
        return true;

    return false;
}

bool VitaWorldUI_Render(uint16_t *dst, int32_t pitch)
{
    if (!VitaWorldUI_IsActive())
        return false;

    // Draw dialogue box plate (matching Emerald 3DS bottom text plate)
    // 308x56 plate at bottom
    VitaUiPlate_Draw(dst, pitch, &sFullClip, 6, 178, 308, 56, 4, kPlateCream, VITA_PLATE_NORMAL, 0, 0);

    // Render dialogue text from gStringVar4 / buffer
    if (gStringVar4[0] != 0 && gStringVar4[0] != EOS)
    {
        VitaUiArt_DrawStrGba(dst, pitch, &sFullClip, VITA_FONT_NORMAL, gStringVar4, 16, 186, UI_INK_DARK, VITA_UI_NO_SHADOW);
    }

    return true;
}
