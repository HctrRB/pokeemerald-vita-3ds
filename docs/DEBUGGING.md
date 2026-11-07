# Debugging and profiling

## Logs

- `VITA_LOG(...)` (`src/platform/vita/vita_platform.h`) writes to
  `ux0:data/POKEVITA/debug.log` through a 32 KB buffered stream. It compiles to nothing unless
  the build is configured with `-DVITA_LOGGING=ON`.
- Do not log every frame: writes to the memory card can stall for hundreds of milliseconds.
- `printf` is not used in production code.

## Per-frame telemetry (`vita_perf.c/h`)

Always compiled in. Timestamps cost ~40 ns (`sceKernelGetProcessTimeWide`); results are buffered
in RAM and written with 32 KB buffers.

| File | Content |
|---|---|
| `ux0:data/POKEVITA/perf_summary.txt` | Every 120 3D frames (or 5 s): render FPS against the real target, logic Hz, Core 0 / Core 1 load, frame-time p50/p95/p99 and frames over budget, `HOLES`, `WORKER` (chunks built on Core 2, dropped, frames that overlapped a job, Core 0 wait), `PACING`, events, average/min/max per stage, sub-timers, `PER CHUNK` and `LIGHT PER CHUNK` costs, atlas cost per upload, free memory, session totals. `[MEMORY …]` lines at boot and after each map warm-up |
| `ux0:data/POKEVITA/perf_frames.csv` | One row per Core 0 frame |

CSV columns:

| Group | Columns |
|---|---|
| Frame | `frame, render3d, target_fps, frame_ms` (wall time, start to after the swap), `c0_active_ms` (sum of the named stages), `other_ms` (`frame_ms` minus the named stages) |
| Atlas animation | `atlas_ms, atlas_compose_ms, atlas_upload_ms, atlas_uploads` |
| Chunks | `cull_ms, build_ms` (Core 0 builds only), `build_ground_ms` (ground, relief and their light rays; includes `build_atlas_ext_ms`), `build_atlas_ext_ms, build_trees_ms, build_bldg_ms, build_upload_ms, light_rays, build_verts`. The phase times and counts include the chunk committed from the worker that frame |
| Other stages | `draw3d_ms, ui_scan_ms, ui_up_ms, swap_ms, entities_ms, digest_ms, shift_ms` (all the map change work of the frame: `HandleMapChange`, chunk shift, building and lighting resets) |
| Core 1 | `c1_logic_ms, c1_vblank_ms, c1_pacing_ms` (time sleeping to hold 60 Hz; Core 1 never waits for Core 0) |
| Scene | `chunks_vis, chunks_built, draw_calls, verts`; `holes`: chunks in view left wrong this frame, without geometry (the background shows) or drawn with an atlas slot recomposed since their build |
| Lighting (chunk builds) | `light_ray_ms` (marching shadow rays), `light_contact_ms` (contact AO), `light_steps` (ray points), `light_cells` (cells classified: cache misses), `light_blocks` (block bounds worked out), `light_api_ms` (all time inside the lighting entry points, rays and AO included). The times are part of the build phases above |
| Pacing | `logic_ticks`: game frames since the previous Core 0 frame (1 per 3D frame at 60 FPS, 2 at 30; the summary's `PACING` line counts the others); `world_ms`: `VoxelWorld_BuildInstances`; `atlas_slots_ms`: the current map's atlas slot (compose, extend, activate) on map or tileset changes |
| Chunk worker | `worker_built`, `worker_dropped` (results dropped because the world changed), `worker_compute_ms` (the worker's time for the chunk committed this frame), `worker_wait_ms` (Core 0 blocked waiting for it, at the frame start or in mid-frame). `build_ms` and `chunks_built` count only the builds on Core 0 |
| `events` | Letters: `M` new map (chunk cache reset), `X` map crossing (chunks shifted), `L` live tile changed, `S` sun moved (visible chunks rebuild), `W` warm-up, `F` faded (synchronous builds), `A` atlas extended in a build, `C` an atlas slot recomposed for another tileset |

| Marker | Where |
|---|---|
| `VitaPerf_BeginFrame` / `EndFrame` | `sdl2.c` presentation loop (`EndFrame` gets the target FPS) |
| `VitaPerf_MarkAtlasAnim*`, `MarkCull*`, `MarkBuild*`, `MarkDraw3D*` | `VoxelRenderer_RenderFrame` |
| `VitaPerf_SubBegin/SubEnd`, `AddCount`, `MarkEvent` | `BuildChunk` phases, atlas animation, map shift, entities, live digest |
| `VitaPerf_MarkUiScanlines*`, `MarkUiUpload*` | Pass 6 (GBA overlay) |
| `VitaPerf_MarkSwap*` | around `vglSwapBuffers` |
| `VitaPerf_Core1*` | `VBlankIntrWait` (logic, VBlank and audio, pacing) |

Draw calls and vertices are counted by a `glDrawArrays` wrapper in `voxel_renderer.c`.

Reading it: the budget is 33.3 ms at 30 FPS and 16.7 ms at 60 FPS (`target_fps`). In 3D the
remaining spikes come from `ui_scan_ms` + `ui_up_ms` (a GBA window on screen), `worker_wait_ms`
(Core 0 waiting for a long chunk on Core 2), `build_ms` (a visible hole built on Core 0) and the
crossing frame (`shift_ms`, `atlas_slots_ms`, `entities_ms`); a large `other_ms` points at work
outside the named stages. Core 1 should show `c1_logic_ms` + `c1_vblank_ms` well under 16.7 ms;
if `c1_pacing_ms` drops to zero the game logic is the limit.

Copy the files to `logs/<session>/` in the repository (git-ignored) to keep a history. Quick
aggregation by column name:

```sh
awk -F, 'NR==1 {for (i=1;i<=NF;i++) c[$i]=i; next}
  $c["worker_built"]>0 {n++; w+=$c["worker_compute_ms"]}
  $c["frame_ms"]>16.7 && $c["render3d"]==1 {slow++}
  END {print "worker chunk:", w/n, "ms;", slow, "frames over 16.7 ms"}' logs/<session>/perf_frames.csv
```

## Crash dumps

A crash leaves `ux0:data/psp2core-*.psp2dmp` (gzip'd ELF core). Decode it against the ELF of
the same build (`build-vita/pokeemerald`):

- With `vita-parse-core`: `vita-parse-core <dump> build-vita/pokeemerald`.
- By hand: take the crashed thread's PC/LR from `THREAD_REG_INFO`, subtract the module base
  from `MODULE_INFO`, then `arm-vita-eabi-addr2line -e build-vita/pokeemerald 0x81000000+offset`.

Keep the ELF of every build you test on hardware; a dump from another build decodes to wrong lines.

## Vita3K vs hardware

Vita3K runs the ARM code much faster than the console and hides GPU timing and texture races.
Use it for boot, visuals and crashes; take performance numbers from the telemetry on a real Vita.

## Checking the parallel renderer

Configure with `-DVITA_DRAW_SELFCHECK=ON`: every few frames the parallel renderer's output is
compared byte for byte with the sequential one.
