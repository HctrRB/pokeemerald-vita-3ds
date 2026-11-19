# Architecture

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

How the port is put together and what happens in one frame. Thread rules are in
[THREADS.md](THREADS.md), the GPU passes in [RENDERING.md](RENDERING.md).

## Layers

```
┌────────────────────────────────────────────────────────────────────────────┐
│ Game side: linked into game_combined.o, runs on Core 1 only                │
│   pret/pokeemerald decompilation: src/*.c, include/, data/, sound/, asm/,  │
│   graphics/, compiled with -DPORTABLE -DNONMATCHING -DMODERN=1 -DUBFIX     │
│   -DPLATFORM_VITA; some game files carry small Vita hooks under #ifdef     │
│   GBA hardware emulated in RAM: src/platform/bios.c, dma.c, cgb_audio.c    │
├────────────────────────────────────────────────────────────────────────────┤
│ Platform side: src/platform/{sdl2,widescreen,gba_fast_draw*}.c, vita/,      │
│ voxel/, ui/                                                                │
│   sdl2.c          main() (MainThread_Boot / MainThread_Run), the game      │
│                   thread, the 60 Hz clock, saves, config, RTC              │
│   gba_fast_draw*.c software GBA PPU (scanline renderer, 3 cores, NEON)     │
│   widescreen.c    288-px lines: what fills the margins in each scene       │
├─────────────────────────────────────┬──────────────────────────────────────┤
│ Voxel 3D world   src/platform/voxel │ Vita features      src/platform/vita │
│   world, chunks, meshes, atlas,     │   controls, stage scenes, day/night, │
│   buildings, trees, relief, signs,  │   extras, Nuzlocke, telemetry, the   │
│   lamps, lighting, weather,         │   game frame and the game-thread     │
│   entities, 3D battle, 3D Fly, HD-2D│   tripwire; the UI: src/platform/ui  │
├─────────────────────────────────────┴──────────────────────────────────────┤
│ VitaSDK · vitaGL · SDL2 (audio device, threads, timers)                    │
└────────────────────────────────────────────────────────────────────────────┘
```

The game never talks to the Vita directly. It writes emulated GBA registers, VRAM, OAM and
palettes in RAM; the platform reads them back to draw, and supplies the services the GBA hardware
did (VBlank, DMA, timers, sound mixing, flash saves, the real-time clock).

**The game side / platform side split** is made by `CMakeLists.txt` (`PLATFORM_SIDE_REGEX`):
every `src/**/*.c` that does not match it is game side, including `src/platform/bios.c`, `dma.c`
and `cgb_audio.c`. The game side is linked first into `game_combined.o`, whose `.data`/`.bss`
become `gamedata`/`gamebss` so that `vita_reset.c` can restore a power-on copy of them. The same
split is what `tools/core_boundary` checks: Core 0 and Core 2 may not reach the game side's code
or writable state except as [THREADS.md](THREADS.md) allows.

## Boot

`main()` in [`src/platform/sdl2.c`](../../src/platform/sdl2.c) pins itself to Core 0
(`VitaPlatform_PinCurrentThread`) and runs `MainThread_Boot`, before the game thread exists (the
only time Core 0 may set up game state):

1. `VitaPlatform_Init`: creates `ux0:data/POKEVITA/` and `saves/`, sets the CPU, bus, GPU and GPU
   crossbar clocks, reads the extras (`VitaExtras_Init`, `settings.txt`). Then `VitaPerf_Init` and
   `ModManager_Init`.
2. Reads `pokeemerald.cfg` and the save file of the chosen slot.
3. Initialises vitaGL with `vglInitWithCustomSizes` at 960×544 (pool sizes: the call in
   `MainThread_Boot`).
4. `VoxelRenderer_Init()`, the platform settings, the GBA frame textures and `Ui_Init()` decode the
   heavy assets once.
5. Starts audio (48 kHz stereo, `cgb_audio_init` and the SDL device).
6. Takes the power-on snapshot of the game's memory (`VitaReset_TakeBootSnapshot`) and sets the
   game's clock.

Then `main()` creates the game thread `AgbMain` (`DoMain`, stack `GAME_THREAD_STACK_SIZE`, Core 1),
which registers itself (`GameThread_Register`) and runs the original game loop, and enters
`MainThread_Run`, Core 0's steady state.

## One frame

```
Core 1 (AgbMain)                      Core 0 (MainThread_Run)             Core 2
────────────────                      ───────────────────────             ──────
game logic for one GBA frame
VBlankIntrWait():
  line events, VBlank handler,
  m4a mixer → audio
  Ui_Tick(): new UI draw list
  VoxelFlyGame_Tick()
  GameFrame_CaptureAndPublish()
  post sFrameReadySem  ──────────────► wait for it (one post at 60 FPS,
  pace to 60.00 Hz on the                two at 30 FPS in 3D)
  audio buffer                         GameFrame_Pin(): the frame every
                                         decision below reads
                                       read input
                                       choose a path:
                                        · 3D field / battle →
                                          VoxelRenderer_RenderFrame()   ◄─ chunks computed since
                                        · else → VDraw() +                 the last swap
                                          RenderGameTexture()              (VoxelChunkWorker)
                                       Ui_Render(): the new UI on top
                                       swap, then hand the next chunks  ─► next batch
                                       to the worker                       DrawWorker1/2: GBA
                                                                           scanlines (2D and
                                                                           the overlay)
```

The game logic and audio always run at 60 Hz; the 3D world renders at the framerate chosen in
OPTIONS > GRAPHICS, and every rendered frame shows the latest game frame. The path is chosen from
the pinned game frame (`VoxelWorld_IsMapAvailable`, `VoxelWorld_IsBattleMapAvailable`), which Core 1
computes from the field callbacks, the shop's buy menu and `BattleMainCB2`.

## Where to find things

| Topic | Code | Document |
|---|---|---|
| Thread ownership, the game frame, the map lock, the boundary check | `src/platform/vita/vita_game_frame.c`, `include/platform/game_frame.h`, `overworld.c`, `sdl2.c`, `tools/core_boundary/` | [THREADS.md](THREADS.md) |
| vitaGL passes, HD-2D, 2D overlay | `voxel/voxel_renderer.c`, `voxel/voxel_hd2d_shader.c` | [RENDERING.md](RENDERING.md) |
| 16:9 handling of GBA screens | `widescreen.c`, `src/platform/vita/vita_stage.c` | [SCENES_16_9.md](SCENES_16_9.md) |
| Voxel world | `src/platform/voxel/` | [VOXEL_ENGINE.md](VOXEL_ENGINE.md) |
| User interface | `src/platform/ui/` | [VITA_UI.md](VITA_UI.md) |
| Build and packaging | `CMakeLists.txt`, `build_vita.*`, `vita/` | [BUILD.md](../guides/BUILD.md) |
| Data files | `assets/`, `tools/ui2/`, `tools/ui_gen/gen_intro_margins.py` | [ASSETS.md](ASSETS.md) |
| Logs, telemetry, crash dumps | `src/platform/vita/vita_perf.c`, `src/platform/vita/vita_platform.c` | [DEBUGGING.md](../guides/DEBUGGING.md) |

## Runtime files

What the game reads and writes on the memory card, and which build writes each file:
[RUNTIME_FILES.md](RUNTIME_FILES.md).
