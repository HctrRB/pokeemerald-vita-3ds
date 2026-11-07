#ifndef GUARD_VOXEL_SNAPSHOT_H
#define GUARD_VOXEL_SNAPSHOT_H

#include "global.h"
#include "global.fieldmap.h"

// Double-buffered snapshot of active object events and sprites for thread-safe rendering on Core 0
typedef struct {
    struct ObjectEvent objectEvents[OBJECT_EVENTS_COUNT];
    struct Sprite sprites[OBJECT_EVENTS_COUNT];
    float playerWorldX;
    float playerWorldZ;
    int16_t mapGroup;
    int16_t mapNum;
    uint32_t frameNum;
} VoxelEntitySnapshot;

// Map mutation barrier: set during map loads (CB2_LoadMap/DoMapLoadLoop) to prevent Core 0 from reading transient layout data
extern volatile uint32_t sMapMutationLock;

void VoxelEntities_PublishSnapshot(void);
const VoxelEntitySnapshot *VoxelEntities_GetVisibleSnapshot(void);

#endif // GUARD_VOXEL_SNAPSHOT_H
