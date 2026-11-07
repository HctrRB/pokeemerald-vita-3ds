# Voxel engine

The 3D overworld ("diorama") is drawn from the live GBA map state: the game still runs its
2D field logic, and `src/platform/voxel/` turns the current map and its neighbours into 3D
geometry every frame. The logic comes from pokeemerald-multiplatform's voxel renderer and
Emerald 3DS (see [NOTICE.md](../NOTICE.md)); the GPU code is written for vitaGL.

## Modules

| File | Role |
|---|---|
| `voxel_world.c/h` | Logical map view: map instances (current map + connected maps) with their offsets, layout swaps, tile/metatile queries. `VoxelWorld_IsMapAvailable` decides when the 3D path runs |
| `voxel_camera.c/h` | Follow camera, pure state and math (no GL). Shift on map changes |
| `voxel_atlas.c/h` | Metatile atlas: composes GBA tiles into texture pages; tracks animated metatiles per page |
| `voxel_mesh_builder.c/h` | CPU geometry for one chunk: ground rows, cliffs, seam skirts, water |
| `voxel_relief.c/h` | Terrain relief read from `relief.bin`: cliffs, stairs, sea rocks |
| `voxel_regions.c/h` | Precomputed role of each cell (4 bits per cell), from `regions.bin` |
| `voxel_building.c/h` | Buildings modelled from their own drawing (`buildings.bin`), with a texture page LRU |
| `voxel_tree.c/h` | General-tileset trees: trunk on the ground, cutout crown (`trees.rgba5551`) |
| `voxel_sign.c/h` | Signposts and street lamps with pixel-masked geometry (`signposts.bin`) |
| `voxel_lighting.c/h` | Sun from the in-game clock, baked into vertex colours; cast shadows; face shading |
| `voxel_grade.c/h` | Colour grade (slightly more saturated GBA art) |
| `voxel_entities.c/h` | Player, NPC and follower billboards from the entity snapshot; cast shadows |
| `voxel_sprite_slots.c/h` | Slots of the sprite atlas used by the billboards |
| `voxel_battle.c/h` | The 3D battle stage built from the map the battle started on |
| `voxel_swoop.c/h` | The transition between the voxel field and 2D screens |
| `voxel_arena.c/h` | Fixed memory blocks claimed once (no fragmentation) |
| `3ds_field_view.c/h` | The part of the overworld shown, in metatiles, for map objects |
| `voxel_tuning.h` | Tuning values: chunk area, build budgets, pacing, atmosphere, DoF |
| `voxel_renderer.c/h` | Everything GPU-side: chunk cache, I/O worker, atlas upload, passes, weather, DoF, overlay. See [RENDERING.md](RENDERING.md) |
| `3ds_*.h`, `port_*.h`, `voxel_file.h`, `voxel_unswizzle.h` | Compatibility shims kept from the 3DS code (logging, platform, file access, Morton unswizzle for 3DS-format pages) |

## Chunks

- The world is split into chunks of `CHUNK_SIZE` = 8×8 metatiles. Up to `MAX_CHUNKS` = 64
  chunk slots are cached, each with its own GL vertex buffer; the geometry lives only there.
- A build has three phases: `PrepareChunkJob` (Core 0: the chunk's window and the atlas slot of
  every map it touches), `ComputeChunkJob` (geometry and lighting into `sChunkStaging`, no GL)
  and `CommitChunkJob` (Core 0: building pages, slot, VBO upload).
- Per frame: frustum-cull candidate chunks and draw the built ones. Visible holes are built on
  Core 0; every other build (chunks about to enter the view, dated chunks, pre-warming ahead of
  the movement) goes to `VoxelChunkWorker` on Core 2, one per frame. See
  [RENDERING.md](RENDERING.md) for the policy and [THREADS.md](THREADS.md) for the worker.
- A chunk is rebuilt when its content hash changes (scripts changing tiles, doors, the sun
  moving). The hash is compared once per world epoch (`sWorldEpoch`, bumped on those changes), so
  chunks off screen at the time are checked when they come back into view.
- Chunks used in the current or previous frame are never evicted.
- Lighting is most of a build (rays and contact AO); a chunk costs ~6 ms on average and up to
  ~35 ms on mountains (logs/sesion 27).

## Atlas

- Metatile pages are composed by `VoxelAtlas_JobStep` into `VOXEL_ATLAS_SLOTS` = 5 LRU slots,
  so neighbouring maps with other tilesets keep their textures. A slot used by a chunk on screen
  is marked used every frame, so the LRU never recomposes it for another tileset while visible.
- When the current map changes during play (a crossing), its slot is extended over several frames
  (`AtlasExtend_*`, 1.5 ms a frame): the slot keeps serving meanwhile, since an extension keeps
  every metatile where it was, and its tile animation waits until the end. Warps extend at once.
- Animated tiles (water, flowers, shores) are re-read from BG VRAM and only the dirty rows of
  the active slot are uploaded, through a spare texture per page (RENDERING.md, "Textures
  rewritten at run time"). Other slots must not be refreshed from VRAM: VRAM
  holds only the current map's tileset (refreshing them corrupted neighbouring terrain).

## Map changes

Crossing into a connected map: Core 1 rebuilds the layout under `sMapMutationLock`; Core 0
keeps drawing the previous instances and snapshot, then `VoxelCamera_Shift` re-bases the camera
and `VoxelWorld_GetInstanceOffset` aligns entities that are still in the old map's
coordinates. Cached chunks are not rebuilt nor re-uploaded: `ShiftChunkCoordinates` moves their
grid position and their `drawOffsetX/Z`, a translation applied when they are drawn. Chunk
geometry lives only in the VBOs; a single staging buffer (`sChunkStaging`) holds the chunk being
built. NPCs of connected maps are spawned ahead of the border
(`event_object_movement.c`, `TrySpawnObjectEvents`). See [THREADS.md](THREADS.md).

## Data files

All under `assets/voxel/`, packed into the VPK as `app0:voxel/`. Their generators are not in
this repository; see [ASSETS.md](ASSETS.md).

## Tuning values

Everything that changes how the 3D world looks or how its work is paced is in
[`src/platform/voxel/voxel_tuning.h`](../src/platform/voxel/voxel_tuning.h): the chunk area around
the camera, build budgets and pre-warming, the chunk worker switch, atlas and texture pacing,
colour grade and haze, fog, weather, motes and the depth-of-field bands. Capacities that size
buffers (chunk vertex caps, slot counts, FBO sizes) stay next to their structures in
`voxel_renderer.c`, and structured data (weather kinds, fog banks, light tables) stays with the
code that reads it.
