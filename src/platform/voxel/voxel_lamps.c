#include "voxel_lamps.h"

#include <stddef.h>

#include "constants/layouts.h"
#include "voxel_relief.h"
#include "voxel_sign.h"

/* A light that belongs to a building model rather than to a sign: where its lamp sits on the
 * map's cell (x, y), as an offset into the cell and a height above its ground, in tiles. */
typedef struct
{
    uint16_t layout;
    uint16_t x, y;
    float dx, dz, height;
} FixedLight;

static const FixedLight sFixedLights[] = {
    /* Lilycove's beacon (lilycove_beacon, voxelgen/scripts/voxel_building_specs.py): a lathe
     * stood 25 px into its 1x2 rect at (61, 33), the yellow lamp about 20 px up its profile
     * (BEACON), on the point's back row. */
    { LAYOUT_LILYCOVE_CITY, 61, 34, 0.5f, 0.5625f, 1.25f },
};

static uint32_t EmitFixedLights(const VoxelBuilder *place, const VoxelMapInstance *inst,
                                int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                float (*lights)[3], uint32_t lightCapacity)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < sizeof(sFixedLights) / sizeof(sFixedLights[0]) && count < lightCapacity; ++i)
    {
        const FixedLight *const light = &sFixedLights[i];
        const int32_t wx = inst->originX + light->x, wz = inst->originY + light->y;
        if (light->layout != inst->layoutId || wx < x0 || wx >= x1 || wz < y0 || wz >= y1)
            continue;
        /* Placed as the builder places what stands on that cell (its relief and the map's base). */
        VoxelBuilder at = *place;
        at.base = VoxelRelief_Base(inst);
        at.lift = VoxelRelief_CellLift(inst, wx, wz);
        at.shift = VoxelRelief_CellShift(inst, wx, wz);
        float x = (float)wx + light->dx, y = light->height, z = (float)wz + light->dz;
        VoxelBuilder_Place(&at, &x, &y, &z);
        lights[count][0] = x;
        lights[count][1] = y;
        lights[count][2] = z;
        ++count;
    }
    return count;
}

uint32_t VoxelLamps_EmitChunk(VoxelBuilder *glass, VoxelBuilder *pool, const VoxelMapInstance *inst,
                              int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                              float (*lights)[3], uint32_t lightCapacity)
{
    uint32_t count;

    if (glass == NULL || pool == NULL || inst == NULL || inst->indoor)
        return 0;
    count = VoxelSign_EmitNight(glass, pool, inst, x0, y0, x1, y1, lights, lightCapacity);
    return count + EmitFixedLights(glass, inst, x0, y0, x1, y1, lights + count, lightCapacity - count);
}
