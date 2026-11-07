#ifndef GUARD_VITA_WORLD_UI_H
#define GUARD_VITA_WORLD_UI_H

#include <stdint.h>
#include <stdbool.h>

void VitaWorldUI_Init(void);
bool VitaWorldUI_IsActive(void);
bool VitaWorldUI_Render(uint16_t *dst, int pitch);

#endif // GUARD_VITA_WORLD_UI_H
