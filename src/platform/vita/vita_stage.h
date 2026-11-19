#ifndef GUARD_VITA_STAGE_H
#define GUARD_VITA_STAGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum VitaStageKind
{
    VITA_STAGE_NONE = 0,
    VITA_STAGE_INTRO_LEAVES,
    VITA_STAGE_INTRO_BIKE
};

void VitaStage_SetVBlankCallback(void (*callback)(void));

bool VitaStage_IsStageActive(void);
int32_t VitaStage_GetActiveKind(void);

// Evaluates the current scene to determine which stage mode applies

// Renders widescreen extensions in vitaGL for the active stage
void VitaStage_RenderWidescreenMargins(float vx0, float vx1, float vy0, float vy1);

/**
 * @brief Draws the intro leaves scene's continuation art beside the GBA picture (widescreen on).
 *
 * Each BG's strip from assets/ui/intro_margins.bin is drawn back to front, cut to the margin
 * width and following that BG's vertical scroll, as Emerald 3DS does. Main thread, after the
 * frame quad. Leaves the projection set to the screen in pixels.
 *
 * @param pictureX0 Screen x where the GBA picture starts (the margin width).
 * @param pictureX1 Screen x where it ends.
 * @param scale     Screen pixels per GBA pixel.
 * @return true when the leaves scene is up and its art was drawn.
 */
bool VitaStage_RenderLeavesMargins(float pictureX0, float pictureX1, float scale);

#ifdef __cplusplus
}
#endif

#endif // GUARD_VITA_STAGE_H
