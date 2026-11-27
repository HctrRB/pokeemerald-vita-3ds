# Debugging and profiling

Type: guide · Audience: developers · Status: current
Verified against: `55868979`

## Logs

- `VITA_LOG(...)` (`src/platform/vita/vita_platform.h`) writes to
  `ux0:data/POKEVITA/debug.log` through a 32 KB buffered stream. It compiles to nothing unless
  the build is configured with `-DVITA_LOGGING=ON`.
- Do not log every frame: writes to the memory card can stall for hundreds of milliseconds.
- `printf` is not used in production code.

## UI stall log (`ui_stall.c`)

Built only with `-DVITA_TELEMETRY=ON`. With a screen of the new UI open, `ux0:data/POKEVITA/ui_stall.log` gets a line (200 at most per
session) whenever Core 1 did not tick the UI for more than 100 ms, Core 0 did not render for more
than 100 ms, or Core 0 kept drawing the same list while Core 1 was publishing new ones. Each line
gives the screen and the ages of the other core, so it says which core held the frame.

It opens, writes and closes the file on the spot, from Core 0 when the render side logs: on a
slow card that write can itself last hundreds of milliseconds and chain further stall lines.
Read a run of consecutive Core 0 gaps with that in mind.

## Per-frame telemetry (`vita_perf.c/h`)

Built only with `-DVITA_TELEMETRY=ON` (off by default, so normal builds write no telemetry).
Timestamps cost ~40 ns (`sceKernelGetProcessTimeWide`); results are buffered
in RAM and written with 32 KB buffers.

| File | Content |
|---|---|
| `ux0:data/POKEVITA/perf_summary.txt` | Every 120 3D frames (or 5 s): render FPS against the real target, logic Hz, Core 0 / Core 1 load, frame-time p50/p95/p99 and frames over budget, `HOLES`, `WORKER` (chunks built on Core 2, dropped, frames that overlapped a job, Core 0 wait), `PACING`, `UI SPRITE CELLS` (since boot: most run-time sprite cells drawn at once, how many there are, sprites skipped because none was free), events, average/min/max per stage, sub-timers, `PER CHUNK` and `LIGHT PER CHUNK` costs, atlas cost per upload, free memory, session totals. `[MEMORY …]` lines at boot and after each map warm-up |
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
| New UI | `ui2_render_ms` (Core 0 Pass 7 draw and cell uploads), `ui2_tick_ms` (Core 1 `Ui_Tick` draw-list build), `ui2_quads` (quads in the list) |
| Entities (inside `entities_ms`) | `ent_emit_ms` (`VoxelEntities_Emit`, including `ent_decode_ms`), `ent_decode_ms` (sprite frames decoded into the atlas), `ent_atlas_ms` (`WriteSpriteAtlasSlots`), `ent_pack_ms` (vertex packing, shadow and reflection copies), `ent_lights_ms` (`LightCards`) |
| Scene | `hd2d_level` (`VOXEL_HD2D_*` drawn), `dof_on`, `fast_forward`, `ui_open`, `map_group`, `map_num`, `indoor`, `weather` (`VoxelWeatherClass`), `daylight` (0 night, 1 day), `lamps` (night lamps drawn), `in_battle`, `faded`: what the last 3D frame showed, so one session's log can be split by zone (`VitaPerf_SetScene`) |
| GPU (probe builds) | `gpu_world_ms, gpu_down_ms, gpu_blur_ms, gpu_bloom_ms, gpu_kawase_ms, gpu_final_ms, gpu_total_ms, gxm_scenes`; 0 in other builds. See "Measuring build" below |
| Bench (probe builds) | `bench_run` (0 outside a run), `bench_step`, `bench_level` (level imposed, -1 none), `probe_mask` (bit of the `VitaPerfPass` switched off), `bench_measured` (1 = frame to use) |

| Marker | Where |
|---|---|
| `VitaPerf_BeginFrame` / `VitaPerf_EndFrame` | `sdl2.c` presentation loop (`VitaPerf_EndFrame` gets the target FPS) |
| `VitaPerf_MarkAtlasAnim*`, `MarkCull*`, `MarkBuild*`, `MarkDraw3D*` | `VoxelRenderer_RenderFrame` |
| `VitaPerf_SubBegin` / `VitaPerf_SubEnd`, `VitaPerf_AddCount`, `VitaPerf_MarkEvent` | `BuildChunk` phases, atlas animation, map shift, entities, live digest, and New UI render/tick (`VITA_PERF_T_UI2_RENDER`, `VITA_PERF_T_UI2_TICK`) |
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

## Measuring build (GPU time per render target, automatic bench)

`build_vita.ps1 -Probe` (or `VITA_BUILD_PROBE=1 ./build_vita.sh`) builds `build-vita-probe/pokeemerald.vpk`
with `VITA_GPU_PROBE` (CMake option, implies `VITA_TELEMETRY`). Its numbers are for comparing passes
and builds with each other, never with a normal build: the probes take away the overlap of CPU and GPU,
so its frame rate is lower.

- **GPU per render target.** vitaGL has no GPU timers. `VitaPerf_GpuScene` is called right after each
  `glBindFramebuffer` of the 3D frame and before the swap: it runs `glFinish` and charges the wait to the
  scene that just ended (`gpu_*_ms`). That is the GPU work the CPU submission did not already cover;
  the GPU starts each scene idle because the previous probe waited for it. Only at render-target
  changes: vitaGL's `glFinish` ends the GXM scene, so a probe in mid-scene would split it and lose its
  depth. Without the post pass the whole frame is `gpu_world_ms`.
- **Automatic bench.** Standing still in the 3D field for `BENCH_STILL_FRAMES` frames starts a run
  (`[BENCH]` lines in `perf_summary.txt`): HIGH unpaced (`BENCH_STEP_UNPACED`), HIGH, then HIGH with
  one `VitaPerfPass` switched off at a time (`VitaPerf_PassSkipped`), then MED, LOW and OFF. The
  unpaced step (`VitaPerf_BenchUnpaced`) runs without the probes (its `gpu_*` columns are 0), with
  the swap not waiting for vsync and the presentation loop not waiting for the next 60 Hz tick: the
  CPU is far ahead, so its `frame_ms` is the real pipelined GPU frame, the number to compare with
  16.7 ms (the picture may tear during that step). The world masks split the base geometry (ground,
  cutouts, tree cores, tree edges, sway, buildings); `worldpixels` draws the world scene through an
  8×8 scissor, leaving its vertex and tiling cost. OFF comes last because switching between OFF
  and LOW turns the relief on or off, which rebuilds the atlas and the chunks (a ~0.7 s freeze). The
  level is imposed through `VitaPerf_BenchLevel`; the saved option is not touched and comes back at
  the end. Each step waits for `BENCH_SETTLE_FRAMES` calm frames and keeps `BENCH_MEASURE_FRAMES`
  frames with `bench_measured = 1`. A thin bar across the top of the screen shows the run's
  progress (`VitaPerf_BenchProgress`); when it disappears the run is over. Moving, a menu, a fade, a
  battle, Fly (any frame without the field passes) or fast forward aborts the run; the player has
  to move before another one starts. The picture changes during a run (levels, effects missing):
  that is the bench at work.
- **Session.** Play one session through the spots to measure (town at night with lamps, rain, midday,
  fog, desert, an interior), stand still at each until the bar is gone, and copy
  `perf_frames.csv` and `perf_summary.txt` to `logs/<session>/`. The scene columns split it by zone.
  Keep the clock at normal speed: an accelerated day/night cycle changes the light within a run, and
  moves the sun constantly, which relights the visible chunks (`S` events) and re-tints the sprite
  palettes.

Cost of one pass at one spot: the median of `gpu_total_ms` (or `gpu_world_ms`) over the measured
frames of step "HIGH without X", minus the same at step HIGH of the same run:

```sh
awk -F, 'NR==1 {for (i=1;i<=NF;i++) c[$i]=i; next} $c["bench_measured"]==1 {
  k=$c["bench_run"]" step "$c["bench_step"]" lvl "$c["bench_level"]" mask "$c["probe_mask"];
  n[k]++; g[k]+=$c["gpu_total_ms"]; w[k]+=$c["gpu_world_ms"]}
  END {for (k in n) printf "%s: gpu %.2f world %.2f ms (n=%d)\n", k, g[k]/n[k], w[k]/n[k], n[k]}' \
  logs/<session>/perf_frames.csv | sort -n
```

## Thread violations (`thread_violations.log`)

Always compiled in. The game's in-place mutators (the Pokémon data cipher, `FlagSet`, `FlagClear`,
`VarSet`) check that they run on the game thread (`GAME_THREAD_ONLY`, a stack-range test). A call
from any other thread writes one line per call site to `ux0:data/POKEVITA/thread_violations.log`
with the caller's address: resolve it with `arm-vita-eabi-addr2line -f -e build-vita/pokeemerald
<lr>` against the ELF of that build. The file is absent when all is well; any line in it is a path
from another core into the game that `tools/core_boundary` did not see (a callback set at run time).

## Thread boundary report

Every build writes `build-vita/core_boundary_report.txt`: for each thread, the functions reached and
each violation with the path that leads to it ([BUILD.md](BUILD.md), step 7).

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
