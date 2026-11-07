#ifndef GUARD_VITA_SHOP_UI_H
#define GUARD_VITA_SHOP_UI_H

#include <stdint.h>
#include <stdbool.h>

void VitaShopUI_Init(void);
bool VitaShopUI_IsOpen(void);
void VitaShopUI_Close(void);
bool VitaShopUI_Update(uint32_t keys);
bool VitaShopUI_Render(uint16_t *dst, int pitch);

#endif // GUARD_VITA_SHOP_UI_H
