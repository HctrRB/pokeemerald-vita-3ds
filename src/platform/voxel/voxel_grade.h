/*
 * The voxel world's colour grade: the art a little more saturated, applied as
 * its colours become textures - palettes, sprites, the tree texture, building
 * pages - so it costs nothing per frame. The 2D picture is not graded.
 */
#ifndef VOXEL_GRADE_H
#define VOXEL_GRADE_H

#include <stdbool.h>
#include <stdint.h>

/* Builds the table. Before any texture is made, and once. */
void VoxelGrade_Init(void);
/* A GBA BGR555 colour to the graded, opaque PICA RGBA5551 texel. */
uint16_t VoxelGrade_RGBA5551(uint16_t bgr15);
/* Grades RGBA5551 texels in place, keeping each one's alpha bit. */
void VoxelGrade_Texels(uint16_t *texels, uint32_t count);
/* Scales RGBA5551 texels' brightness in place, keeping their alpha bit. */
void VoxelGrade_Brighten(uint16_t *texels, uint32_t count, float factor);
/* Enables or disables CPU-baked tile relief during atlas composition. */
void VoxelGrade_SetReliefEnabled(bool enabled);
/* Bakes subtle NW directional relief & cavity contrast into a 16x16 atlas slot in place. */
void VoxelGrade_ApplyTileRelief16x16(uint16_t *dest, uint32_t stride, uint32_t height,
                                     uint32_t baseX, uint32_t baseY);
/* Bakes NW directional relief & cavity contrast into a sprite atlas slot rect in place. */
void VoxelGrade_ApplyRectRelief(uint16_t *buf, uint32_t stride, uint32_t width, uint32_t height);

#endif
