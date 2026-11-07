/*
 * The 3D battle: the voxel world drawn as a battle's scenery.
 *
 * The battle's own picture - its Pokemon, the healthboxes, the text box,
 * the move animations - stays the game's, composed over the world where the
 * GBA draws its scenery on BG3 (3ds_video.c, RenderBattleWorld). This module
 * is the game's side of it: where in the map the battle stands, and where
 * the battlers' feet are on the screen.
 */
#ifndef CTR_VOXEL_BATTLE_H
#define CTR_VOXEL_BATTLE_H

#include <stdint.h>
#include <stdbool.h>

#define VOXEL_BATTLE_PITCH 30.0f
#define VOXEL_BATTLE_DISTANCE 7.0f
#define VOXEL_BATTLE_FOV 35.0f

bool VoxelBattle_GameInBattle(void);
bool VoxelBattle_IsLink(void);

void VoxelBattle_BeginStage(void);
bool VoxelBattle_StepStage(uint32_t cells, uint32_t candidates, float *targetX, float *targetZ,
                           float *ground);

typedef struct
{
    float x, y, rx, ry;
} VoxelBattleShadow;

uint32_t VoxelBattle_Shadows(VoxelBattleShadow *out, uint32_t max);

#endif // CTR_VOXEL_BATTLE_H
