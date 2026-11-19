#ifndef VOXEL_LAMPS_H
#define VOXEL_LAMPS_H

#include <stdint.h>

#include "voxel_mesh_builder.h"
#include "voxel_world.h"

/*
 * The world's night lights: the street lamps found among the signs (VoxelSign_EmitNight) and a
 * few fixed lights that are part of a building model (Lilycove's beacon). Built with each chunk
 * on the chunk worker: their geometry goes into the chunk's VBO and their centres into the chunk
 * slot, for the halo and the light on characters drawn at night.
 */

/* Lights a chunk keeps (an 8x8 chunk of Rustboro holds at most a few lamps). */
#define VOXEL_LAMP_CHUNK_LIGHTS 8

/*
 * The night lights of one instance's cells in [x0, x1) x [y0, y1): lantern glass into `glass`,
 * the pools of light into `pool`, and each light's centre, in the builders' coordinates, into
 * `lights` (up to lightCapacity). Both builders must have the instance's stretch set
 * (VoxelBuilder_SetStretch). Returns how many lights were written.
 */
uint32_t VoxelLamps_EmitChunk(VoxelBuilder *glass, VoxelBuilder *pool, const VoxelMapInstance *inst,
                              int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                              float (*lights)[3], uint32_t lightCapacity);

#endif // VOXEL_LAMPS_H
