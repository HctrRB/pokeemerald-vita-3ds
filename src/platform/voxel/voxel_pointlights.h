#ifndef VOXEL_POINTLIGHTS_H
#define VOXEL_POINTLIGHTS_H

#include <stdbool.h>
#include <stdint.h>

#define VOXEL_POINTLIGHT_MAX 16

typedef struct
{
    float x, y, z;
    float r, g, b;
    float radius;
    float invRadiusSq;
    float intensity;
    float curR, curG, curB;
    bool flicker;   /* a flame (the cave lantern); electric lamps burn steady */
    uint32_t seed;  /* the flicker's phase and speed */
} VoxelPointLight;

void VoxelPointLights_Clear(void);
void VoxelPointLights_Add(float x, float y, float z,
                          float r, float g, float b,
                          float radius, float intensity, bool flicker, uint32_t seed);
void VoxelPointLights_BeginFrame(float seconds);
uint32_t VoxelPointLights_Count(void);
const VoxelPointLight *VoxelPointLights_Get(uint32_t index);

/* Adds warm/colored point-light illumination to a world-space point (e.g. an entity billboard). */
void VoxelPointLights_SampleAdd(float x, float y, float z,
                                float *addR, float *addG, float *addB);

#endif // VOXEL_POINTLIGHTS_H
