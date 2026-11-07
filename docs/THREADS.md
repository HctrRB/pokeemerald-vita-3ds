# Threads and shared data

The Vita gives an application three user cores (Cortex-A9). The port uses all three. Most
regressions in this project have been races between Core 0 and Core 1, so read this before
touching anything that crosses threads.

## Threads

| Thread | Core | Created in | Work |
|---|---|---|---|
| `MainThread` | 0 | `sdl2.c` (`main`) | Input, native UI, presentation (vitaGL, voxel renderer, 2D overlay), `vglSwapBuffers`. The GXM display-queue driver thread also stays on Core 0: moving it caused a GPU crash |
| `AgbMain` | 1 | `sdl2.c` (`main` → `DoMain`) | The original game loop, the m4a/CGB sound mixer, VBlank work, 60 Hz pacing |
| `DrawWorker1`, `DrawWorker2` | 2 | `gba_fast_draw.c` (`StartWorkers`) | Draw even/odd scanlines of a GBA frame and convert them to RGB565 with NEON |
| `VoxelIoWorker` | 2 | `voxel_renderer.c` (`VoxelRenderer_Init`) | Reads building texture pages from the VPK with its own buffered file handle |
| `VoxelChunkWorker` | 2 (low priority) | `voxel_renderer.c` (`ChunkWorker_Start`) | Computes one chunk at a time (`ComputeChunkJob`), launched after each swap; see "The chunk worker" below |

## Pacing

- **Core 1 is the master clock.** `VBlankIntrWait()` (`sdl2.c`) waits for
  `freq / 60` ticks and nudges the wait by up to ±500 µs (a small PLL) to keep the SDL audio
  queue near its target of ~19,200 bytes. Game logic and audio therefore run at 60.00 Hz
  whatever the renderer does. In fast forward it switches to a plain timer.
- **Core 0 follows.** It sleeps until the next tick (`accumulator`, `sdl2.c`). In 2D it
  waits for `sFrameReadySem`, posted by Core 1 after each game frame. In 3D it also waits for
  it: one post per rendered frame at 60 FPS, two at 30 FPS (`voxelFps`, `eglSwapInterval(2)`),
  then draws the latest game frame. Rendering on its own timer drifted against Core 1's
  audio-skewed period and drew some game frames twice and skipped others (the player stuttered
  while running). Fast forward keeps the timer.

## Who owns what

| Data | Writer | Reader | Protection |
|---|---|---|---|
| Game state: `gMain`, tasks, `gObjectEvents`, `gSprites`, save blocks | Core 1 | Core 0 (renderer, UI) | Renderer reads entities only through the snapshot below. The native UI reads game state while the game is paused (menu open) |
| Entity snapshot `VoxelEntitySnapshot sSnapshots[2]` (`voxel_entities.c`, `include/platform/voxel_snapshot.h`) | Core 1, `VoxelEntities_PublishSnapshot()` in VBlank | Core 0, `VoxelEntities_GetVisibleSnapshot()` | Double buffer; index swap between release fences, acquire fence on read |
| Map layout during a map change | Core 1 (`CB2_LoadMap`, `DoMapLoadLoop`, connection transitions) | Core 0 (`voxel_world.c`) | `volatile uint32_t sMapMutationLock` (`overworld.c:102`): set by the loader, cleared when the next snapshot is published. Core 0 keeps drawing the previous instances meanwhile |
| Emulated VRAM, OAM, palettes, registers | Core 1 | Draw workers (2D), Core 0 (overlay pass) | 2D: Core 1 is blocked while the frame is captured line by line. Overlay pass: read only; layers it hides are masked in each line's captured copy (`gFastDrawDispcntClear`, `gFastDrawBldcntClear`), never in the live registers |
| 2D overlay backdrop colour | Core 0 | Draw code | `gFastDrawBackdropOverride` instead of `PLTT[0]`, so Core 1 palette writes cannot leak into the chroma key |
| Audio queue | Core 1 | SDL audio thread | SDL queue |
| Building pages | `VoxelIoWorker` | Core 0 | `sBuildingIoMutex`, held only to swap requests, never across `fread` |
| New UI draw list `UiDrawList` (`src/platform/ui/ui_draw.c`) | Core 1, `Ui_Tick` | Core 0, `Ui_Render` | Triple buffer, index exchanged with `__atomic_exchange_n` |
| New UI run-time sprites (`ui_sprites.c`): cell cache and staging ring | Core 1 decodes into the ring (head) | Core 0 writes the cells into the UI texture (tail) | Single-producer / single-consumer ring with release / acquire counters; the head is published before the list that uses it. A cell is reused only after 16 ticks without being drawn |
| Voxel world state a chunk build reads or writes: map instances, lighting caches, building caches, mesh builder window, `sScratch`, `sChunkStaging`, atlas maps | Core 0, and `VoxelChunkWorker` while a job runs | Core 0, Core 1 (entity snapshot reads instances, relief, decks) | A window, not a lock: see "The chunk worker" |

## Rules

1. Core 0 never writes game state. Input reaches the game through the emulated key register.
2. Anything the renderer needs from `gObjectEvents` / `gSprites` goes into the snapshot,
   computed on Core 1 (the player's world position is already there).
3. Code that changes the map layout sets `sMapMutationLock` first.
4. Don't pin or re-prioritise SDK threads (GXM, audio).
5. A new worker thread gets its own file handles and buffers; no shared `FILE *`.
6. New Core 0 code that touches the voxel world outside the safe points of
   `VoxelRenderer_RenderFrame` calls `EnsureWorkerIdle()` (or `ChunkWorker_Join()`) first.

## The chunk worker

`VoxelChunkWorker` computes one chunk at a time (`ComputeChunkJob`); see
[VOXEL_ENGINE.md](VOXEL_ENGINE.md) for the three build phases.

- **Launch**: after `vglSwapBuffers` (`ChunkWorker_Launch`), never while `sMapMutationLock` is
  set. The chunk is chosen during the frame (`ChunkWorker_Choose`) and prepared on Core 0 at
  launch (`PrepareChunkJob` may compose an atlas slot, which uses GL).
- **Frames that overlap it**: a 3D frame polls the job (`ChunkWorker_Poll`). If it still runs
  and the current map is unchanged, the frame goes on without rebuilding the map instances,
  building chunks or sampling light for the billboards (`VoxelEntities_AllowLighting`), and
  waits in mid-frame (`EnsureWorkerIdle`) only before a map, tileset or tile change, the sun
  moving, a hole to build or lamps to rebuild. A map change, warm-up or 3D battle waits at once.
- **Commit**: at the start of the first frame that finds the job done, before the chunk pass
  (`ChunkWorker_Commit`). The result is dropped if `sWorldEpoch` or the generation of an atlas
  slot it used changed meanwhile.
- **Shared lookup caches**: while a job runs, `VoxelWorld_FreezeMemos` stops the world lookup
  memos from being written. Core 1's entity snapshot (`VoxelRelief_ShiftAt`,
  `VoxelBuildings_DeckAt`, `VoxelWorld_*`) and Core 0's billboards may look up at the same time,
  so the caches tolerate a half-written entry: the relief cell and cut memos write the index
  before the key and check a found index against its cell; the world memos publish an entry
  before the count; the building placement bands and cell grids rebuild in place, never free
  their memory while running (`CELL_GRID_CAP`) and keep every lookup inside the slot, the
  layout's placements and the model, so a lookup during a rebuild can only answer wrong once.
  The building row indexes (`FrontAt`) are used by Core 0 alone.

## Known risks (to fix)

- **Snapshot reuse at 30 FPS.** With two buffers, Core 1 publishes twice per 3D frame, so the
  second publish writes into the buffer Core 0 may still be reading. A third buffer, or a
  sequence counter checked after reading, removes the race.
- **Overlay pass reads VRAM while Core 1 writes it.** Text can tear for a frame while a window
  is being printed.
- **The chunk worker shares Core 2 with the draw workers.** A job can run while the GBA overlay
  is drawn: its scanlines went from 3.5 to 5 ms on average in logs/sesion 27. Giving the chunk
  worker a priority below `DrawWorker1/2` would remove it.
