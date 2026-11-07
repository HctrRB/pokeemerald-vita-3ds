#ifndef GUARD_VITA_CORNER_ART_H
#define GUARD_VITA_CORNER_ART_H

#include <stdint.h>
#include <stdbool.h>

#define VITA_CORNER_PATTERN_DIM 64
#define VITA_CORNER_TRIM_DIM    32

#define VITA_CORNER_FRAME_DARK   2
#define VITA_CORNER_FRAME_GOLD   4
#define VITA_CORNER_FRAME_SHADOW 6
#define VITA_CORNER_FELT_LINE    2
#define VITA_CORNER_FELT_BAND    6

void VitaCornerArt_Felt(uint16_t *pattern, uint16_t *trim, uint16_t table);
void VitaCornerArt_Wall(uint16_t *pattern, uint16_t *trim);
void VitaCornerArt_RenderBackdrop(uint16_t screen);

#endif // GUARD_VITA_CORNER_ART_H
