# Architecture

How the port is put together and what happens in one frame. Thread rules are in
[THREADS.md](THREADS.md), the GPU passes in [RENDERING.md](RENDERING.md).

## Layers

```
┌────────────────────────────────────────────────────────────────────────────┐
│ Game: pret/pokeemerald decompilation                                       │
│   src/*.c, include/, data/, sound/, asm/, graphics/                        │
│   Compiled with -DPORTABLE -DNONMATCHING -DMODERN=1 -DUBFIX -DPLATFORM_VITA│
│   ~17 game files carry small Vita hooks under #ifdef                       │
├────────────────────────────────────────────────────────────────────────────┤
│ GBA hardware emulation             src/platform/                           │
│   sdl2.c          main(), threads, 60 Hz clock, saves, config, RTC         │
│   gba_fast_draw*.c software GBA PPU (scanline renderer, 3 cores, NEON)     │
│   widescreen.c    288-px lines: what fills the margins in each scene       │
│   bios.c dma.c cgb_audio.c                                                 │
├─────────────────────────────────────┬──────────────────────────────────────┤
│ Voxel 3D world   src/platform/voxel │ Vita features      src/platform/vita │
│   world, chunks, meshes, atlas,     │   controls, intro stage, day/night,  │
│   buildings, trees, relief, signs,  │   extras, Nuzlocke, telemetry;       │
│   lighting, weather, entities,      │   the UI itself: src/platform/ui     │
│   3D battle, depth of field         │                                      │
│                                     │                                      │
├─────────────────────────────────────┴──────────────────────────────────────┤
│ VitaSDK 2026.08 · vitaGL · SDL2 (audio device, threads, timers)            │
└────────────────────────────────────────────────────────────────────────────┘
```

The game never talks to the Vita directly. It writes emulated GBA registers, VRAM, OAM and
palettes in RAM; the platform layer reads them back to draw, and supplies the services the GBA
hardware did (VBlank, DMA, timers, sound mixing, flash saves, the real-time clock).

## Boot

`main()` in [`src/platform/sdl2.c`](../src/platform/sdl2.c):

1. Pins itself to Core 0 (`VitaPlatform_PinCurrentThread`), creates `ux0:data/POKEVITA/`, sets
   the clocks (`vita_platform.c`: CPU 444, bus 222, GPU 222, GPU crossbar 166 MHz).
2. Initialises vitaGL with `vglInitWithCustomSizes` (8 MB GXM pool, 40 MB RAM, 88 MB CDRAM,
   8 MB physically contiguous) at 960×544.
3. Reads `pokeemerald.cfg` and `settings.txt`, opens the save, starts audio (48 kHz stereo).
4. `VoxelRenderer_Init()` and `Ui_Init()` decode the heavy assets once.
5. Starts the `AgbMain` thread (4 MB stack, Core 1), which runs the original game loop.
6. Enters the presentation loop.

## One frame

```
Core 1 (AgbMain)                      Core 0 (MainThread)                 Core 2
────────────────                      ───────────────────                 ──────
game logic for one GBA frame
VBlankIntrWait():
  VBlank handler, m4a mixer → audio
  publish entity snapshot
  post sFrameReadySem  ──────────────► wait for it (one post at 60 FPS,
  pace to 60.00 Hz on the                two at 30 FPS in 3D)
  audio buffer                         read input, update the native UI
                                       choose a path:
                                        · native UI → ShowNativeUi()
                                        · 3D field / battle →
                                          VoxelRenderer_RenderFrame()   ◄─ chunk computed since
                                        · else → VDraw() +                 the last swap
                                          RenderGameTexture()              (VoxelChunkWorker)
                                       vglSwapBuffers(), then hand the  ─► next chunk
                                       next chunk to the worker            DrawWorker1/2: GBA
                                                                           scanlines (2D and
                                                                           the overlay)
```

The 3D world renders at 30 FPS by default (`voxelFps`; 60 FPS is an option), the game logic and
audio always at 60 Hz, and every rendered frame shows the latest game frame. Which path is taken
is decided in `sdl2.c` from `VoxelWorld_IsMapAvailable()` (field callbacks, shop buy menu) and
`VoxelWorld_IsBattleMapAvailable()` (`BattleMainCB2`).

## Where to find things

| Topic | Code | Document |
|---|---|---|
| Thread ownership, snapshot, map lock | `voxel_entities.c`, `overworld.c`, `sdl2.c` | [THREADS.md](THREADS.md) |
| vitaGL passes, DoF, 2D overlay | `voxel/voxel_renderer.c` | [RENDERING.md](RENDERING.md) |
| 16:9 handling of GBA screens | `widescreen.c`, `vita/vita_stage.c` | [SCENES_16_9.md](SCENES_16_9.md) |
| Voxel world | `src/platform/voxel/` | [VOXEL_ENGINE.md](VOXEL_ENGINE.md) |
| User interface | `src/platform/ui/` | [VITA_UI.md](VITA_UI.md) |
| Build and packaging | `CMakeLists.txt`, `build_vita.*`, `vita/` | [BUILD.md](BUILD.md) |
| Data files | `assets/`, `tools/ui_gen/` | [ASSETS.md](ASSETS.md) |
| Logs, telemetry, crash dumps | `vita/vita_perf.c`, `vita/vita_platform.c` | [DEBUGGING.md](DEBUGGING.md) |

## Runtime files (`ux0:data/POKEVITA/`)

| File | Written by | Content |
|---|---|---|
| `saves/pokeemerald*.sav` | `sdl2.c` | Saves, 128 KB GBA flash layout; slot 1–3 |
| `pokeemerald.cfg` | `sdl2.c` `StoreConfigFile` | Platform settings (filter, widescreen, buttons, 3D on/off, resolution, FPS cap…) |
| `settings.txt` | `vita/vita_extras.c` | QoL extras and voxel options (`VitaExtras_GetFast`) |
| `debug.log` | `VITA_LOG` | Boot and error log |
| `perf_summary.txt`, `perf_frames.csv` | `vita/vita_perf.c` | Per-frame telemetry |
