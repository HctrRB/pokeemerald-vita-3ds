# Documentation

Type: index · Audience: everyone · Status: current
Verified against: `55868979`

Start with [ARCHITECTURE.md](ARCHITECTURE.md), then read the document for the area you are
changing. Contribution rules are in [../CONTRIBUTING.md](../CONTRIBUTING.md).

| Document | Covers |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Layers, boot, one frame, where things are, runtime files |
| [THREADS.md](THREADS.md) | Cores, pacing, who owns which data, known races |
| [RENDERING.md](RENDERING.md) | The three presentation paths, voxel passes, GBA overlay, GPU rules |
| [SCENES_16_9.md](SCENES_16_9.md) | How GBA screens fill 960×544 |
| [VOXEL_ENGINE.md](VOXEL_ENGINE.md) | The 3D overworld modules, chunks, atlas, map changes |
| [VITA_UI.md](VITA_UI.md) | The native 16:9 touch interface (`src/platform/ui/`), its menu themes, battle HUD, and the Vita features behind it |
| [BUILD_YOUR_OWN.md](BUILD_YOUR_OWN.md) | For users: make your own VPK from your Pokémon Emerald cartridge ([Spanish](COMPILA_TU_VPK.md)) |
| [BUILD.md](BUILD.md) | Toolchain, build steps, CMake options, installing and testing |
| [ASSETS.md](ASSETS.md) | Where every data file comes from and how it is generated |
| [DEBUGGING.md](DEBUGGING.md) | Logs, per-frame telemetry, thread violations, crash dumps |

The design mocks of the new UI are in [../design/stitch/](../design/stitch/README.md).
