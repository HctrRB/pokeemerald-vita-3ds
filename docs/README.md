# Documentation

Type: index · Audience: everyone · Status: current
Verified against: `55868979`

Start with [reference/ARCHITECTURE.md](reference/ARCHITECTURE.md), then read the document for the
area you are changing. Contribution rules are in [../CONTRIBUTING.md](../CONTRIBUTING.md).

## Guides: how to do one thing

| Document | For | Covers |
|---|---|---|
| [guides/BUILD_YOUR_OWN.md](guides/BUILD_YOUR_OWN.md) | Players | Make your own VPK from your Pokémon Emerald cartridge ([español](guides/BUILD_YOUR_OWN.es.md)) |
| [guides/BUILD.md](guides/BUILD.md) | Developers | Toolchain, build steps, CMake options, the build-time checks, installing and testing |
| [guides/DEBUGGING.md](guides/DEBUGGING.md) | Developers | Logs, per-frame telemetry, thread violations, crash dumps |
| [guides/ADDING_A_LANGUAGE.md](guides/ADDING_A_LANGUAGE.md) | Developers, translators | The registry, interface catalog, game text manifest and layout patches of a language, and how to build one |
| [guides/UI_HARDWARE_TESTS.md](guides/UI_HARDWARE_TESTS.md) | Testers | What to check on a Vita for each screen of the new UI, what is already checked and what is not |

## Reference: how the port works today

| Document | Covers |
|---|---|
| [reference/ARCHITECTURE.md](reference/ARCHITECTURE.md) | Game side and platform side, boot, one frame, where things are |
| [reference/THREADS.md](reference/THREADS.md) | Cores, pacing, the game frame, who owns which data, the chunk worker, known risks |
| [reference/RENDERING.md](reference/RENDERING.md) | The presentation paths, voxel passes, GBA overlay, GPU rules |
| [reference/VOXEL_ENGINE.md](reference/VOXEL_ENGINE.md) | The 3D overworld modules, chunks, atlas, map changes |
| [reference/VITA_UI.md](reference/VITA_UI.md) | The native 16:9 touch interface (`src/platform/ui/`), its menu themes, battle HUD, and the Vita features behind it |
| [reference/SCENES_16_9.md](reference/SCENES_16_9.md) | How GBA screens fill 960×544 |
| [reference/ASSETS.md](reference/ASSETS.md) | Where every data file comes from and how it is generated |
| [reference/RUNTIME_FILES.md](reference/RUNTIME_FILES.md) | Files the game reads and writes on the memory card, and which build writes them |

The design mocks of the new UI are in [../design/stitch/](../design/stitch/README.md).

## Rules for documentation

- **English**, for every document. The players' build guide is also kept in Spanish
  ([guides/BUILD_YOUR_OWN.es.md](guides/BUILD_YOUR_OWN.es.md)); both change in the same commit.
- **Reference documents describe the code as it is today** and are the only technical source of
  truth. They cite symbols and constant names, never line numbers or copied values (a number copied
  into a document goes stale; a name can be looked up).
- **A commit that changes documented behaviour updates the document in the same commit.**
- Every document here starts with two lines: `Type · Audience · Status` and
  `Verified against: <commit>`.
- The build checks it (`tools/doc_check`): a reference document or guide that cites a missing file,
  symbol or link, or a line number, fails the build ([guides/BUILD.md](guides/BUILD.md), step 8).
