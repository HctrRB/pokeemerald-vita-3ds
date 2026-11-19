/*
 * voxel_fly_game.c - Core 1 owner & Core 0 snapshot bridge for the 3D Fly cinematic.
 */
#include "voxel_fly_game.h"

#include "global.h"
#include "field_effect.h"
#include "overworld.h"
#include "region_map.h"
#include "constants/field_effects.h"
#include "constants/region_map_sections.h"
#include "platform.h"
#include "voxel_world.h"

/* Core 1 state */
static VoxelFly sFly;
static bool sFlyAsked;
static uint32_t sWarpVersion = 1u;
static uint32_t sFlyStartWarpVersion = 0u;

/* Core 1 -> Core 0 triple-buffered snapshot (safe at 30 FPS where Core 1 ticks twice per 3D frame) */
#define VOXEL_FLY_SNAP_SLOTS 3
static VoxelFlySnapshot sSnapshots[VOXEL_FLY_SNAP_SLOTS];
static volatile int32_t sSnapIdx = 0;

/* Core 0 -> Core 1 atomic feedback: last warpVersion drawn in 3D */
static volatile uint32_t sDrawnWarpVersion = 0u;

static bool VoxelFieldReady(void)
{
    return Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D) != 0
        && VoxelWorld_FieldShownLive();   // Core 1: the game's own state
}

static void FlyDirection(float *dirX, float *dirZ)
{
    const struct MapHeader *to = GetDestinationWarpMapHeader();
    const u8 from = gMapHeader.regionMapSectionId;
    const u8 dest = (to != NULL) ? to->regionMapSectionId : MAPSEC_NONE;

    *dirX = 0.0f;
    *dirZ = -1.0f;
    if (from >= MAPSEC_NONE || dest >= MAPSEC_NONE)
        return;

    /* Region-map cells: x east, y south -> voxel world's +X and +Z. */
    *dirX = ((float)gRegionMapEntries[dest].x + (float)gRegionMapEntries[dest].width * 0.5f)
          - ((float)gRegionMapEntries[from].x + (float)gRegionMapEntries[from].width * 0.5f);
    *dirZ = ((float)gRegionMapEntries[dest].y + (float)gRegionMapEntries[dest].height * 0.5f)
          - ((float)gRegionMapEntries[from].y + (float)gRegionMapEntries[from].height * 0.5f);
}

static void PublishSnapshot(void)
{
    const int32_t cur = __atomic_load_n(&sSnapIdx, __ATOMIC_RELAXED);
    const int32_t next = (cur + 1 >= VOXEL_FLY_SNAP_SLOTS) ? 0 : (cur + 1);
    VoxelFlySnapshot *const dst = &sSnapshots[next];

    dst->active = VoxelFly_Active(&sFly);
    dst->warpVersion = sWarpVersion;
    VoxelFly_Frame(&sFly, &dst->frame);
    __atomic_store_n(&sSnapIdx, next, __ATOMIC_RELEASE);
}

bool8 Port_FlyHover(void)
{
    if (!FieldEffectActiveListContains(FLDEFF_USE_FLY)
     || FieldEffectActiveListContains(FLDEFF_FLY_IN))
        return FALSE;

    if (!sFlyAsked)
    {
        float dirX, dirZ;

        sFlyAsked = true;
        if (VoxelFly_Active(&sFly) || !VoxelFieldReady())
            return FALSE;

        FlyDirection(&dirX, &dirZ);
        sFlyStartWarpVersion = sWarpVersion;
        VoxelFly_Begin(&sFly, dirX, dirZ);
        PublishSnapshot();
    }
    return VoxelFly_Hovers(&sFly) ? TRUE : FALSE;
}

bool8 Port_FlyBusy(void)
{
    return VoxelFly_Busy(&sFly) ? TRUE : FALSE;
}

void VoxelFlyGame_OnWarpLoaded(void)
{
    sWarpVersion++;
    if (sWarpVersion == 0u)
        sWarpVersion = 1u;
}

void VoxelFlyGame_Tick(void)
{
    if (!FieldEffectActiveListContains(FLDEFF_USE_FLY))
        sFlyAsked = false;

    if (VoxelFly_Active(&sFly))
    {
        const bool voxelEnabled = (Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D) != 0);
        const bool voxelAvail = voxelEnabled && VoxelWorld_FieldShownLive();   // Core 1: the game's own state

        if (!voxelEnabled || (VoxelFly_Hovers(&sFly) && !voxelAvail))
        {
            VoxelFly_Cancel(&sFly);
        }
        else
        {
            const bool warped = (sWarpVersion != sFlyStartWarpVersion);
            const bool gone = warped || !voxelAvail;
            const uint32_t drawnWarp = __atomic_load_n(&sDrawnWarpVersion, __ATOMIC_ACQUIRE);
            const bool fieldShown = warped && voxelAvail && (drawnWarp == sWarpVersion);

            VoxelFly_Step(&sFly, fieldShown, gone);
        }
    }

    PublishSnapshot();
}

void VoxelFlyGame_OnReset(void)
{
    VoxelFly_Cancel(&sFly);
    sFlyAsked = false;
    sWarpVersion = 1u;
    sFlyStartWarpVersion = 0u;
    __atomic_store_n(&sDrawnWarpVersion, 0u, __ATOMIC_RELEASE);
    PublishSnapshot();
}

const VoxelFlySnapshot *VoxelFly_GetSnapshot(void)
{
    const int32_t idx = __atomic_load_n(&sSnapIdx, __ATOMIC_ACQUIRE);
    return &sSnapshots[idx];
}

void VoxelFly_NotifyFrameDrawn(uint32_t warpVersion)
{
    if (warpVersion != 0u)
        __atomic_store_n(&sDrawnWarpVersion, warpVersion, __ATOMIC_RELEASE);
}
