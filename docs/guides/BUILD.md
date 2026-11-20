# Building

Type: guide · Audience: developers · Status: current
Verified against: `55868979`

`build_all.ps1` (Windows) or `./build_all.sh` (in MSYS2) installs everything below and runs every
step; users should follow [BUILD_YOUR_OWN.md](BUILD_YOUR_OWN.md). This page is the detail behind
it, for development.

The repository's path must not contain spaces or accents: `make` and the decomp's rules cannot
handle them (`build_all.ps1` refuses such a folder).

## Requirements (Windows)

1. [MSYS2](https://www.msys2.org) in `C:\msys64`. In the **MSYS** shell:
   ```sh
   pacman -Syu
   pacman -S --needed make git cmake tar bzip2 xz wget curl python patch pkgconf diffutils \
       mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-pkgconf \
       mingw-w64-x86_64-python mingw-w64-x86_64-python-pillow
   ```
   The generators run with MINGW64's Python and Pillow; no Windows Python is needed.
2. VitaSDK 2026.08 with the vitaGL and SDL2 packages (MSYS shell; keep the two `export` lines in
   `~/.bash_profile`):
   ```sh
   export VITASDK=/usr/local/vitasdk PATH=/usr/local/vitasdk/bin:$PATH VITASDK_CHANNEL=2026.08
   export TAR_OPTIONS=--force-local
   mkdir -p /c/vdpm-tmp && export TMPDIR=$(cygpath -m /c/vdpm-tmp)
   git clone --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm && (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
   VDPM_NONINTERACTIVE=1 vdpm install vitaGL sdl2
   ```
   The base VitaSDK has none of the libraries the game links: `vitaGL` brings vitaShaRK,
   libmathneon, SceShaccCgExt and taihen with it. Without them the build stops at
   `vitaGL.h: No such file or directory`.
   Check with `arm-vita-eabi-gcc --version` and `vdpm list`.

MSYS2 pitfalls: the Windows-form `TMPDIR` avoids `vdpm-channel: cannot open file`, and
`TAR_OPTIONS=--force-local` stops tar reading `C:/…` as a host.

## Steps

From the repository root:

0. **Game files** (once): this repository does not carry the game's graphics, sounds or binary
   data. `./fetch_assets.sh <your Emerald dump>` in the MINGW64 shell (or
   `fetch_assets.ps1 -Rom <dump>` from PowerShell) checks the dump, downloads them and
   generates the files made from them. Generators run there must read only downloaded sources
   (PNG, `.bin`, `.pal`): the conversion in step 2 has not run yet on a new copy.
1. **Host tools** (once), MINGW64 shell:
   ```sh
   make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"
   ```
2. **Game assets** (once, and after editing graphics, maps, sound or JSON):
   ```sh
   make -f vita/assets.mk -j8
   ```
3. **VPK**, from PowerShell:
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File .\build_vita.ps1
   ```
   or from the MSYS shell: `./build_vita.sh`.

Output: `build-vita/pokeemerald.vpk`, plus `eboot.bin`, the ELF `build-vita/pokeemerald` (keep it:
crash dumps are decoded against it), `build-vita/core_boundary_report.txt` and
`build-vita/doc_check_report.txt` (steps 7 and 8 below). `build_vita.sh` drops a CMake cache created in
another checkout path; for a clean rebuild delete `build-vita/`.

## CMake options

| Option | Default | Effect |
|---|---|---|
| `VITA_PARALLEL_DRAW` | ON | Draw GBA frames on the worker threads. Also enables the 288-px widescreen lines |
| `VITA_LOGGING` | OFF | `VITA_LOG` writes `ux0:data/POKEVITA/debug.log` |
| `VITA_TELEMETRY` | OFF | Write `perf_frames.csv`, `perf_summary.txt` and `ui_stall.log` to `ux0:data/POKEVITA/` |
| `VITA_DRAW_SELFCHECK` | OFF | Testing: compare parallel and sequential drawing periodically |

## What CMakeLists.txt does

1. Collects every `src/**/*.c` and `gflib/*.c` (glob).
2. **Preprocesses** each game file with `arm-vita-eabi-gcc -E`, then `tools/preproc` (charmap
   strings, `INCBIN`), into `build-vita/generated/`. Some files get extra forced includes here
   (`src/intro.c` gets `src/platform/vita/compat/ctr_gba_stage.h`).
3. Compiles both sides with `-O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -fno-strict-aliasing
   -fwrapv -fno-builtin -fcommon` (`COMMON_COMPILE_OPTIONS`). The decomp relies on type punning and
   signed overflow, hence `-fno-strict-aliasing` and `-fwrapv`. The platform side adds
   `-Wall -Wextra` and `-fno-section-anchors` (every static table addressed by its own symbol, for
   step 7). CMake's Release flags put `-O3` first; the project's own `-O2` comes later and wins.
4. Assembles `data/*.s` and the songs into one `game_data.o` (`.rodata` renamed to `.data`
   because it holds pointers).
5. Links the game side into `game_combined.o` with its `.data`/`.bss` renamed (`gamedata`/`gamebss`), so all game
   state sits between known symbols and `vita_reset.c` can restore a power-on copy for resets. `vita_reset.c` places
   a 4-byte sentinel in `.noinit` (after `gamebss`) so `__stop_gamebss` stays strictly below the RW segment's
   `p_vaddr + p_memsz` and its relocations are preserved in `.sce.rel` by `vita-elf-create`.
6. Links through `vita/vita_link.sh` (pads the code segment for `vita-elf-create`).
7. Checks the thread boundary on the linked ELF (`tools/core_boundary/check_core_boundary.py
   --strict`, target `core_boundary`): no path from Core 0 or Core 2 may reach the game side's code
   or writable state except as `tools/core_boundary/allow.txt` declares, and the game thread may not
   read Core 0's pinned game frame ([THREADS.md](../reference/THREADS.md)). `eboot.bin` depends on its stamp
   (`core_boundary.ok`): a violation fails the build, and the report says which path.
8. Checks the documentation (`tools/doc_check/check_docs.py --strict`, target `doc_check`): every
   file, symbol and link a reference document or guide cites exists, none cites a line number, and
   a guide and its translation stay in step. Errors there fail the build; plans and reports only
   get warnings (`build-vita/doc_check_report.txt`). Names that are not in the repository (files
   on the console, build outputs, SDK fields, external generators) are listed in
   `tools/doc_check/allow.txt`.
   Symbols are looked up only in files git tracks, so private or ignored files cannot hide a
   symbol that a public clone lacks.
9. `vita_create_self(... UNSAFE)` (the app writes `ux0:data/`), then `vita-mksfoex` and
   `vita-pack-vpk` called directly (`vita_create_vpk` breaks on paths with spaces).
10. Packs `vita/sce_sys/` (LiveArea), `assets/voxel/` → `voxel/`, `assets/ui/` → `ui/`,
   `assets/ui2/` → `ui2/`, and `assets/followers/{mon,text}/*.bin` packed by
   `tools/pack_followers.py` into `followers/followers.pak` (PKF1 container).

Title ID `PKEM00001`. LiveArea PNGs must be 8-bit indexed; `vita/make_livearea.sh` converts
full-colour art (needs `mingw-w64-x86_64-imagemagick`).

## Installing and testing

- **PS Vita**: install the VPK with VitaShell (HENkaku/Ensō, *Enable Unsafe Homebrew*).
  Disable overclock plugins for this title; the game sets its own clocks.
- **Vita3K**: the current build shows a black screen there, so test on hardware. Even when it
  runs, it is no guide to performance (it runs the ARM code much faster than the console):
  measure on hardware with the telemetry ([DEBUGGING.md](DEBUGGING.md)).

## Warnings

The build prints no warnings; keep it that way. The flags are split in `CMakeLists.txt`:
the decompiled game keeps the leniency it was written for (`-Wno-implicit-function-declaration`
and friends), while `src/platform/` builds with `-Wall -Wextra` (only unused parameters, sign
comparisons and missing field initialisers are left out). The preprocessing step passes
`-Wno-trigraphs`, since `??` in game text is never meant as one.
