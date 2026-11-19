#ifndef GUARD_VOXEL_SNAPSHOT_H
#define GUARD_VOXEL_SNAPSHOT_H

#include "global.h"
#include "global.fieldmap.h"
#include "sprite.h"

// Lock-free 4-slot snapshot of active object events, sprites, and OAM matrices for thread-safe rendering on Core 0
typedef struct {
    struct ObjectEvent objectEvents[OBJECT_EVENTS_COUNT];
    struct Sprite sprites[OBJECT_EVENTS_COUNT];
    struct OamMatrix oamMatrices[OAM_MATRIX_COUNT];
    u8 extPalettes[OBJECT_EVENTS_COUNT];
    float playerWorldX;
    float playerWorldZ;
    int16_t mapGroup;
    int16_t mapNum;
    uint32_t frameNum;
} VoxelEntitySnapshot;

// Map mutation barrier: set during map loads (CB2_LoadMap/DoMapLoadLoop) to prevent Core 0 from reading transient layout data
extern volatile uint32_t sMapMutationLock;

void VoxelEntities_PublishSnapshot(void);
void VoxelEntities_BeginFrameSnapshot(void);
void VoxelEntities_EndFrameSnapshot(void);
const VoxelEntitySnapshot *VoxelEntities_GetVisibleSnapshot(void);

#ifdef VOXEL_SUPPORTED
void VoxelFlyGame_OnWarpLoaded(void);
#endif

#endif // GUARD_VOXEL_SNAPSHOT_H
