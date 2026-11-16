#include "voxel_pointlights.h"
#include <math.h>
#include <stddef.h>

static VoxelPointLight sLights[VOXEL_POINTLIGHT_MAX];
static uint32_t sLightCount = 0;

void VoxelPointLights_Clear(void)
{
    sLightCount = 0;
}

void VoxelPointLights_Add(const float x, const float y, const float z,
                          const float r, const float g, const float b,
                          const float radius, const float intensity, const uint32_t seed)
{
    if (sLightCount >= VOXEL_POINTLIGHT_MAX || radius <= 0.05f || intensity <= 0.001f)
        return;

    VoxelPointLight *const l = &sLights[sLightCount++];
    l->x = x;
    l->y = y;
    l->z = z;
    l->r = r;
    l->g = g;
    l->b = b;
    l->radius = radius;
    l->invRadiusSq = 1.0f / (radius * radius);
    l->intensity = intensity;
    l->curR = r * intensity;
    l->curG = g * intensity;
    l->curB = b * intensity;
    l->seed = seed;
}

void VoxelPointLights_BeginFrame(const float seconds)
{
    for (uint32_t i = 0; i < sLightCount; ++i)
    {
        VoxelPointLight *const l = &sLights[i];
        const float phase = (float)(l->seed & 0xFFu) * 0.02454369f;
        const float speed = 3.2f + (float)((l->seed >> 8) & 0xFu) * 0.18f;
        const float wave = sinf(seconds * speed + phase) * 0.06f
                         + sinf(seconds * (speed * 1.73f) + phase * 1.7f) * 0.04f;
        const float scale = l->intensity * (1.0f + wave);
        l->curR = l->r * scale;
        l->curG = l->g * scale;
        l->curB = l->b * scale;
    }
}

uint32_t VoxelPointLights_Count(void)
{
    return sLightCount;
}

const VoxelPointLight *VoxelPointLights_Get(const uint32_t index)
{
    return (index < sLightCount) ? &sLights[index] : NULL;
}

void VoxelPointLights_SampleAdd(const float x, const float y, const float z,
                                float *const addR, float *const addG, float *const addB)
{
    float ar = 0.0f, ag = 0.0f, ab = 0.0f;
    for (uint32_t i = 0; i < sLightCount; ++i)
    {
        const VoxelPointLight *const l = &sLights[i];
        const float dx = x - l->x;
        const float dy = y - l->y;
        const float dz = z - l->z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        const float t = 1.0f - d2 * l->invRadiusSq;
        if (t <= 0.0f)
            continue;
        const float falloff = t * t;
        ar += l->curR * falloff;
        ag += l->curG * falloff;
        ab += l->curB * falloff;
    }
    *addR = ar;
    *addG = ag;
    *addB = ab;
}
