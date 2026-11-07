# Contributing

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and [docs/THREADS.md](docs/THREADS.md) first.
Every change is tested on a real PS Vita before it is considered done.

## Where code goes

| Change | Place |
|---|---|
| Vita backend, GBA hardware emulation, main loop | `src/platform/` |
| 3D world | `src/platform/voxel/` |
| Native UI and Vita features | `src/platform/vita/` |
| Public platform headers used by game code | `include/platform/` |
| Game behaviour | The decompilation (`src/*.c`), only as small hooks under `#ifdef PLATFORM_VITA` (or `VOXEL_SUPPORTED`). Keep the original code path intact in the `#else` branch |

Never reformat or restyle decompilation files; keep their diffs minimal so upstream changes stay mergeable.

## Code standards (platform code)

Language: **English** for identifiers, comments and documentation.

1. **Types**: fixed-width types (`uint32_t`, `int32_t`, `uint8_t`, `size_t`) instead of bare
   `int`/`unsigned`. `const` on read-only parameters, pointers and locals.
2. **Flow**: guard clauses and early returns; at most two levels of nesting; one responsibility
   per function, aiming at ≤ 50 lines. Hot loops in the renderers may be longer when splitting
   them costs measurable time; say so in a comment.
3. **No magic values**: named constants, enums or macros. No duplicated blocks.
4. **File header**: every file starts with a comment saying what it does, which thread runs it,
   and where it comes from (original, pokeemerald-multiplatform, Emerald 3DS) with its licence.
5. **Public functions** in headers get a `/** @brief … */` comment: what it does, parameters,
   thread, ownership of returned pointers.
6. **Vita safety**: check every `sce*` return value (negative is an error). No buffers over
   4 KB on the stack. No `printf`; use `VITA_LOG`. Exit with `sceKernelExitProcess(0)`.
7. **Threads**: follow [docs/THREADS.md](docs/THREADS.md). Core 0 never writes game state.
8. **CMake**: target properties only (`target_include_directories`, `target_link_libraries`, …).

Formatting: `src/platform/.clang-format` and `.editorconfig` (Allman braces, 4 spaces, 110
columns). Format only the lines you change; don't mix formatting with logic in one commit.

## Workflow

1. Work on a branch from `main`; one topic per branch.
2. Build with `build_vita.ps1` (see [docs/BUILD.md](docs/BUILD.md)). The build must finish with no
   new warnings.
3. Test on hardware with the smoke list below; for performance work, compare
   `perf_frames.csv` before and after ([docs/DEBUGGING.md](docs/DEBUGGING.md)).
4. Commit in small steps. Message: `type(scope): summary` (`feat`, `fix`, `perf`, `refactor`,
   `docs`, `build`, `chore`), with a body explaining why.
5. Update the documentation in `docs/` that your change makes wrong, in the same branch.

## Smoke test (hardware or Vita3K)

1. Intro (all scenes) → title → main menu → load a save.
2. Walk, run and cycle Route 101 → Oldale: no flicker or camera jumps at the border.
3. Mauville and Route 111 in a sandstorm: steady frame rate.
4. Talk to an NPC (GBA text over 3D).
5. Wild battle in 3D: FIGHT, BAG, POKÉMON, RUN.
6. START: every dock screen; save from the dock.
7. Buy and sell in a shop.
8. A field move (Cut or Surf) and Fly.
9. Quit to LiveArea.
