/* General tileset trees: ground trunk and a north-leaning cutout crown. */
#ifndef CTR_VOXEL_TREE_H
#define CTR_VOXEL_TREE_H

#include <stdint.h>
#include "voxel_mesh_builder.h"

#define VOXEL_TREE_TEXTURE_DIM 128u
#define VOXEL_TREE_TEXTURE_PATH "voxel/trees.rgba5551"

/* A small tree is one cell: its trunk, with the whole crown standing on it. */
#define VOXEL_TREE_SMALL 4
/* Long grass (Route 119, 120): its own floor and a tuft standing on it, a
 * card leant back as a small tree's crown is. */
#define VOXEL_TREE_GRASS 5

/* General IDs, plus the secondary tileset's own trees (Dewford's small one):
 * -1 for other art, the 2x2 quadrant (row*2+col) of a large tree, or
 * VOXEL_TREE_SMALL or VOXEL_TREE_GRASS. secondaryTileset is the map's, as in
 * VoxelMapInstance. */
int32_t VoxelTree_Part(const void *secondaryTileset, int32_t metatileId);
/* The same at a world cell: this map's, the next map's or the border's. */
int32_t VoxelTree_PartAt(int32_t x, int32_t y);
/* Remove the old canopy from the cell above a tree, leaving its ground. */
int32_t VoxelTree_GroundMetatile(const void *secondaryTileset, int32_t metatileId);
/* Does the tree pass lay a tree part's ground? Not for a trunk in water
 * (Mossdeep's shallows): the ground pass lays the water, animated, as
 * VoxelTree_GroundMetatile names it, and the tree its roots over it. */
bool VoxelTree_LaysGround(const void *secondaryTileset, int32_t metatileId);

/* Appended after the ordinary terrain; these vertices use the tree texture. */
void VoxelTree_EmitInstance(VoxelBuilder *builder, const VoxelMapInstance *inst,
                            int32_t x0, int32_t y0, int32_t x1, int32_t y1);
void VoxelTree_EmitBorder(VoxelBuilder *builder, int32_t x0, int32_t y0, int32_t x1, int32_t y1);

#endif
