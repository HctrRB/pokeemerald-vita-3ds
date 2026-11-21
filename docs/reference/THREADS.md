# Threads and shared data

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

The Vita gives an application three user cores (Cortex-A9). The port uses all three. Most
regressions in this project have been races between Core 0 and Core 1, so read this before
touching anything that crosses threads.

## Threads

| Thread | Core | Created in | Work |
|---|---|---|---|
| `MainThread` | 0 | `sdl2.c` (`main`: `MainThread_Boot`, then `MainThread_Run`) | Input, presentation (vitaGL, voxel renderer, 2D overlay, the new UI's draw list), `vglSwapBuffers`. The GXM display-queue driver thread also stays on Core 0: moving it caused a GPU crash |
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
| Game state: `gMain`, tasks, the party, flags, `gObjectEvents`, `gSprites`, battle structs, save blocks | Core 1 only | nobody else | Core 0 and Core 2 never call game code nor read its writable state: they read the game frame below. The new UI reads game state on Core 1 (`Ui_Tick`) and hands Core 0 only a draw list |
| The game frame `GameFrame` (`include/platform/game_frame.h`, `vita_game_frame.c`): field, weather, fade, overlay, battle, entities | Core 1, `GameFrame_CaptureAndPublish()` in VBlank | Core 0, `GameFrame_Get()` (inline) | 4-slot ring; Core 0 pins one per rendered frame (`GameFrame_Pin` in `MainThread_Run`) and Core 1 never writes the pinned nor the latest slot. See "The game frame" |
| The 3D Fly cinematic `VoxelFlySnapshot` (`voxel_fly_game.c`) | Core 1, `VoxelFlyGame_Tick()` in VBlank | Core 0, `VoxelFly_GetSnapshot()`; Core 0 reports back the warp it drew (`VoxelFly_NotifyFrameDrawn`) | Its own `VOXEL_FLY_SNAP_SLOTS` ring, index published atomically. Platform data, not game state, so it lives outside the game frame and `tools/core_boundary` does not concern it |
| The current map's grid: `gMapHeader`, `gBackupMapLayout` | Core 1 (`CB2_LoadMap`, `DoMapLoadLoop`, connection transitions) | Core 0 (`voxel_world.c`), the chunk worker | The loader calls `GameFrame_BeginMapMutation()` (sets `sMapMutationLock`, bumps the map epoch); the lock is cleared when the next frame is published. Core 0 builds from the grid, and launches the chunk worker, only while `GameFrame_MapSettled()`: no change under way and the pinned frame is of that map |
| Emulated VRAM, OAM, palettes, registers | Core 1 | Draw workers (2D), Core 0 (overlay pass) | 2D: Core 1 is blocked while the frame is captured line by line. Overlay pass: read only; layers it hides are masked in each line's captured copy (`gFastDrawDispcntClear`, `gFastDrawBldcntClear`), never in the live registers |
| Extended sprite palettes `gPlttExt[256]`, `gOamExt[128]` | Core 1 (VBlank: `TransferPlttBuffer`, `LoadOam`) | Draw workers (2D, latched once per `DrawFrame`), Core 1 (`RefreshSlot` reads `gPlttBufferUnfaded` in VBlank) | Same as `PLTT`/`OAM`; zero-cost skip when `gPlttExtInUse == 0` |
| 2D overlay backdrop colour | Core 0 | Draw code | `gFastDrawBackdropOverride` instead of `PLTT[0]`, so Core 1 palette writes cannot leak into the chroma key |
| Audio queue | Core 1 | SDL audio thread | SDL queue |
| Building pages | `VoxelIoWorker` | Core 0 | `sBuildingIoMutex`, held only to swap requests, never across `fread` |
| Day/night clock `sClock` (`vita_daynight.c`) and frame state `VitaDayNightState` | Clock: Core 1 (`VitaDayNight_Time`, berries, the RTC) and Core 0 (`VitaDayNight_UpdateFrame`) both advance it through `VitaDayNight_Update`; state: Core 0 only | Core 0 (every day/night effect) | The state is built from one relaxed read of `timeMs` per frame and never written from Core 1 |
| New UI draw list `UiDrawList` (`src/platform/ui/ui_draw.c`) | Core 1, `Ui_Tick` | Core 0, `Ui_Render` | Triple buffer, index exchanged with `__atomic_exchange_n` |
| New UI run-time sprites (`ui_sprites.c`): cell cache and staging ring | Core 1 decodes into the ring (head) | Core 0 writes the cells into the UI texture (tail) | Single-producer / single-consumer ring with release / acquire counters; the head is published before the list that uses it. A cell is reused only after `UI_SPR_IDLE_TICKS` ticks without being drawn. The cell area (`UiAssets_CellsY`, `UiAssets_CellCount`) is set when the atlas loads, before the game thread starts, and never changes |
| Voxel world state a chunk build reads or writes: map instances, lighting caches, building caches, mesh builder window, `sScratch`, `sChunkStaging`, atlas maps | Core 0, and `VoxelChunkWorker` while a job runs | Core 0, Core 1 (entity snapshot reads instances, relief, decks) | A window, not a lock: see "The chunk worker" |
| Game boot snapshot (`gamedata`, `gamebss` in `vita_reset.c`) | Core 1 (`SoftReset` in `sdl2.c`) | Core 0, Core 1, Core 2 | Two-way atomic barrier (`sResetInProgress` / `sCore0PausedForReset`): Core 0 calls `VoxelRenderer_OnReset()` (`ChunkWorker_Join()`) and parks before Core 1 overwrites `gamedata`/`gamebss` |

## Rules

1. Core 0 never writes game state. Input reaches the game through the emulated key register.
2. Core 0 and Core 2 see the game only through the game frame and the two read-only channels
   listed in `tools/core_boundary/allow.txt` (the emulated GBA display; the map grid under
   `sMapMutationLock`). Anything new the renderer needs from the game is captured on Core 1 by
   the module that needs it (its `*_Capture` function) and read from `GameFrame_Get()`.
3. Code that changes the map layout calls `GameFrame_BeginMapMutation()` first.
4. Don't pin or re-prioritise SDK threads (GXM, audio).
5. A new worker thread gets its own file handles and buffers; no shared `FILE *`.
6. New Core 0 code that touches the voxel world outside the safe points of
   `VoxelRenderer_RenderFrame` calls `EnsureWorkerIdle()` (or `ChunkWorker_Join()`) first.
7. Functions both cores need come in two forms: the one Core 0 calls reads the game frame
   (`VoxelWorld_IsMapAvailable`), the one Core 1 calls asks the game (`VoxelWorld_FieldShownLive`).
   The game thread never reads the game frame.

## The game frame

The game ran into Core 0 for years through small reads that looked harmless: Mirage Island's
layout check, ported from the single-threaded Emerald 3DS, called `IsMirageIslandPresent()` from
Core 0 every 3D frame near Pacifidlog. `GetMonData` decrypts a Pokemon in place, so when Core 1
read the same Pokemon at that moment its checksum failed and it became a Bad Egg (logs/sesion 50).
The fix is a boundary, not a patch:

- **One contract.** At each VBlank, Core 1 captures what the other cores may know into a
  `GameFrame` (`GameFrame_CaptureAndPublish`): each module fills its own part -
  `VoxelWorld_CaptureField` (map, player tile, layout swaps, weather, fade),
  `VoxelBattle_Capture` (flags, HP, the move's type, shadows), `VoxelEntities_Capture` (objects,
  every sprite by id with its palette, tile tag, wild / weather / cloud bits, doors),
  `CaptureOverlay` and `VitaStage_CaptureFrame` (what the 2D layer shows, palettes). With the 3D
  field off only the 2D frame's part is captured.
- **One pin per rendered frame.** `MainThread_Run` pins the latest frame at the top of each
  iteration and again before each rendered frame; every pass of that frame reads the same game
  frame. `GameFrame_Get()` is a pointer load (Core 0 asks it per sprite).
- **Proved on every build.** `tools/core_boundary/check_core_boundary.py` walks the linked ELF
  from each thread's entry (`MainThread_Run`, `DrawWorkerMain`, `VoxelChunkWorkerMain`,
  `VoxelBuildingIoWorkerMain`): direct branches decoded from the instructions, taken addresses,
  function pointers in the port's tables, constant pointers to game state (`gWeatherPtr`). Any
  call into `game_combined.o`'s code or access to its `gamedata`/`gamebss` not in `allow.txt`
  fails the build (the eboot depends on it). From `DoMain` it checks the other side: no path of
  the game thread reaches Core 0's pin. Platform code is built with `-fno-section-anchors` so
  every static table is addressed by its own symbol. `main` is split: `MainThread_Boot` (before
  the game thread exists, may set up game state) is not checked, `MainThread_Run` is.
- **Run-time tripwire.** The game's in-place mutators (the Pokemon data cipher, `FlagSet`,
  `FlagClear`, `VarSet`) call `GAME_THREAD_ONLY()` (`include/platform/game_thread.h`): a stack
  range test, no system call, that logs the caller of a violation once to
  `ux0:data/POKEVITA/thread_violations.log`.

## The chunk worker

`VoxelChunkWorker` computes a batch of chunks per launch (`ComputeChunkJob` for each; up to
`CHUNK_WORKER_BATCH`, `CHUNK_WORKER_BATCH_NORMAL` in normal play), and a batch can be aborted
cooperatively (`sAsyncAbortBatch`); see [VOXEL_ENGINE.md](VOXEL_ENGINE.md) for the three build
phases.

- **Launch**: after `vglSwapBuffers` (`ChunkWorker_Launch`), only while `GameFrame_MapSettled()`.
  The batch is chosen during the frame (`ChunkWorker_ChooseBatch`) and prepared on Core 0 at
  launch (`PrepareChunkJob` may compose an atlas slot, which uses GL).
- **Frames that overlap it**: a 3D frame polls the job (`ChunkWorker_Poll`). If it still runs
  and the current map is unchanged, the frame goes on without rebuilding the map instances,
  building chunks or sampling light for the billboards (`VoxelEntities_AllowLighting`), and
  waits in mid-frame (`EnsureWorkerIdle`) only before a map, tileset or tile change, the sun
  moving or a hole to build. A map change, warm-up or 3D battle waits at once. The night lamps are
  part of each chunk's build (`EmitChunkLamps`), so they never make Core 0 wait.
- **Commit**: at the start of the first frame that finds the job done, before the chunk pass
  (`ChunkWorker_Commit`). The result is dropped if `sWorldEpoch` or the generation of an atlas
  slot it used changed meanwhile.
- **Shared lookup caches**: while a job runs, `VoxelWorld_FreezeMemos` stops the world lookup
  memos from being written. Core 0's billboards may look up at the same time (Core 1 no longer
  does: its capture, `VoxelEntities_Capture`, reads only instance 0's origin, in
  `VoxelWorld_GetPlayerWorldCoordsLive`), so the caches tolerate a half-written entry: the relief cell and cut memos write the index
  before the key and check a found index against its cell; the world memos publish an entry
  before the count; the building placement bands and cell grids rebuild in place, never free
  their memory while running (`CELL_GRID_CAP`) and keep every lookup inside the slot, the
  layout's placements and the model, so a lookup during a rebuild can only answer wrong once.
  The building row indexes (`FrontAt`) are used by Core 0 alone.

## Known risks (to fix)

- **Overlay pass reads VRAM while Core 1 writes it.** Text can tear for a frame while a window
  is being printed.
- **The chunk worker shares Core 2 with the draw workers.** A job can run while the GBA overlay
  is drawn: its scanlines went from 3.5 to 5 ms on average in logs/sesion 27. No Core 2 thread
  has a priority of its own (`VitaPlatform_PinCurrentThread` sets affinity only; the chunk worker
  sets `SDL_THREAD_PRIORITY_NORMAL`). Giving it a priority below `DrawWorker1/2` would remove it.
- **The extras initialise lazily.** `VitaExtras_GetFast` calls `VitaExtras_Init` (which may write
  `settings.txt` and `pokeemerald.cfg`) when it has not run, and both cores call `VitaExtras_GetFast`. It
  does not happen today, since `MainThread_Boot` initialises them first; the lazy branch is a
  latent race.
- **Core 1 reads one Core 0 value.** When the player object has no sprite,
  `VoxelWorld_GetPlayerWorldCoordsLive` (Core 1) adds instance 0's origin, which Core 0 writes
  in `VoxelWorld_BuildInstances`. A torn read can only misplace that fallback position for a
  frame.
