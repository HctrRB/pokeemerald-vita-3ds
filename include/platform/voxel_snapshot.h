#ifndef GUARD_VOXEL_SNAPSHOT_H
#define GUARD_VOXEL_SNAPSHOT_H

#include "global.h"
#include "global.fieldmap.h"
#include "sprite.h"

#define VOXEL_SNAPSHOT_DOORS 2

// The entities of one game frame, part of the game frame (include/platform/game_frame.h):
// captured on Core 1 (VoxelEntities_Capture), drawn by Core 0 from this copy alone.
typedef struct {
    struct ObjectEvent objectEvents[OBJECT_EVENTS_COUNT];
    // GetObjectEventGraphicsInfo of each active object (a follower's is built at run time)
    struct ObjectEventGraphicsInfo graphicsInfo[OBJECT_EVENTS_COUNT];
    // Every sprite, by sprite id: the objects' and the field effects'
    struct Sprite sprites[MAX_SPRITES];
    u8 extPalettes[MAX_SPRITES];        // GetSpriteExtPalette, 0xFF without one
    u16 tileTags[MAX_SPRITES];          // GetSpriteTileTagByTileStart of affine sprites, else 0xFFFF
    u64 visibleWild;                    // bit n: sprite n is a visible wild Pokemon
    u64 weatherSprites;                 // bit n: sprite n is drawn by the weather (CtrSprite_IsVoxelWeather)
    u64 cloudSprites;                   // bit n: sprite n is a cloud (CtrSprite_IsVoxelCloud)
    u64 healSprites;                    // bit n: sprite n is a ball of the Pokecenter's healing machine
    s16 spriteCoordOffsetX;             // gSpriteCoordOffsetX/Y: takes a screen sprite to the objects' frame
    s16 spriteCoordOffsetY;
    struct OamMatrix oamMatrices[OAM_MATRIX_COUNT];
    u8 playerObjectEventId;             // gPlayerAvatar.objectEventId
    // The door being animated (PortDoor_Get) and its twin (PortDoor_GetTwin)
    u8 doorCount;
    u8 doorWidth;
    s16 doorX[VOXEL_SNAPSHOT_DOORS];
    s16 doorY[VOXEL_SNAPSHOT_DOORS];
    const u8 *doorTiles;                // ROM
    const u8 *doorPalettes;             // ROM
    float playerWorldX;
    float playerWorldZ;
    int16_t mapGroup;
    int16_t mapNum;
    uint32_t frameNum;
} VoxelEntitySnapshot;

// Map mutation barrier: set during map loads (CB2_LoadMap/DoMapLoadLoop) to prevent Core 0 from
// reading transient layout data; cleared when the next game frame is published.
extern volatile uint32_t sMapMutationLock;

// Core 1: fills the entities of the game frame being captured. `keepPrevious`: the shop's buy
// menu is open, the field behind it is kept as it was.
void VoxelEntities_Capture(VoxelEntitySnapshot *snap, const VoxelEntitySnapshot *previous, bool8 keepPrevious);
// Core 0: the entities of the pinned game frame.
const VoxelEntitySnapshot *VoxelEntities_GetVisibleSnapshot(void);

#ifdef VOXEL_SUPPORTED
void VoxelFlyGame_OnWarpLoaded(void);
#endif

#endif // GUARD_VOXEL_SNAPSHOT_H
