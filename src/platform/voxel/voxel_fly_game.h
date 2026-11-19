/*
 * voxel_fly_game.h - Core 1 owner and Core 0 double-buffered snapshot bridge for the 3D Fly cinematic.
 *
 * Threading (docs/reference/THREADS.md):
 * - Core 1 owns VoxelFly, answers Port_FlyHover() / Port_FlyBusy(), advances VoxelFly_Step()
 *   at exact 60 Hz game ticks (VoxelFlyGame_Tick), increments warpVersion in LoadMapFromWarp(),
 *   and publishes an atomic double-buffered VoxelFlySnapshot.
 * - Core 0 reads VoxelFly_GetSnapshot() locklessly and reports the warpVersion it last drew
 *   via VoxelFly_NotifyFrameDrawn().
 */
#ifndef VOXEL_FLY_GAME_H
#define VOXEL_FLY_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "voxel_fly.h"

typedef struct
{
    bool active;
    uint32_t warpVersion;
    VoxelFlyFrame frame;
} VoxelFlySnapshot;

/* Core 1: called from LoadMapFromWarp() whenever a warp loads a map (including same-map warps). */
void VoxelFlyGame_OnWarpLoaded(void);

/* Core 1: called once per 60 Hz game tick (right before GameFrame_CaptureAndPublish). */
void VoxelFlyGame_Tick(void);

/* Core 1 (during SoftReset barrier): resets Fly state and publishes a clean inactive snapshot. */
void VoxelFlyGame_OnReset(void);

/* Core 0: lockless read of the latest Fly snapshot published by Core 1. */
const VoxelFlySnapshot *VoxelFly_GetSnapshot(void);

/* Core 0: called after VoxelRenderer_RenderFrame draws a 3D frame for `warpVersion`. */
void VoxelFly_NotifyFrameDrawn(uint32_t warpVersion);

#endif /* VOXEL_FLY_GAME_H */
