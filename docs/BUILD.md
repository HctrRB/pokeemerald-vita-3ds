# Building

## Requirements (Windows)

1. [MSYS2](https://www.msys2.org) in `C:\msys64`. In the **MSYS** shell:
   ```sh
   pacman -Syu
   pacman -S --needed make git cmake tar bzip2 xz wget curl python patch pkgconf diffutils \
       mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-pkgconf
   ```
2. VitaSDK 2026.08 with the SDL2 packages (MSYS shell; keep the two `export` lines in
   `~/.bash_profile`):
   ```sh
   export VITASDK=/usr/local/vitasdk PATH=/usr/local/vitasdk/bin:$PATH VITASDK_CHANNEL=2026.08
   export TAR_OPTIONS=--force-local
   mkdir -p /c/vdpm-tmp && export TMPDIR=$(cygpath -m /c/vdpm-tmp)
   git clone --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm && (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
   VDPM_NONINTERACTIVE=1 vdpm install sdl2 sdl2_image
   ```
   vitaGL comes with VitaSDK. Check with `arm-vita-eabi-gcc --version`.

MSYS2 pitfalls: the Windows-form `TMPDIR` avoids `vdpm-channel: cannot open file`;
`TAR_OPTIONS=--force-local` stops tar reading `C:/…` as a host; without Developer Mode pacman
cannot create symlinks (copy the target instead, e.g. `cp lib/libpng16.a lib/libpng.a` in
`$VITASDK/arm-vita-eabi`).

## Steps

From the repository root:

0. **Game files** (once): this repository does not carry the game's graphics, sounds or binary
   data. `fetch_assets.ps1 -Rom <your Emerald dump>` (or `./fetch_assets.sh <dump>`) checks the
   dump, downloads them and generates the files made from them; see
   [BUILD_YOUR_OWN.md](BUILD_YOUR_OWN.md).
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

Output: `build-vita/pokeemerald.vpk`, plus `eboot.bin` and the ELF `build-vita/pokeemerald`
(keep it: crash dumps are decoded against it). `build_vita.sh` drops a CMake cache created in
another checkout path; for a clean rebuild delete `build-vita/`.

## CMake options

| Option | Default | Effect |
|---|---|---|
| `VITA_PARALLEL_DRAW` | ON | Draw GBA frames on the worker threads. Also enables the 288-px widescreen lines |
| `VITA_LOGGING` | OFF | `VITA_LOG` writes `ux0:data/POKEVITA/debug.log` |
| `VITA_DRAW_SELFCHECK` | OFF | Testing: compare parallel and sequential drawing periodically |

## What CMakeLists.txt does

1. Collects every `src/**/*.c` and `gflib/*.c` (glob).
2. **Preprocesses** each game file with `arm-vita-eabi-gcc -E`, then `tools/preproc` (charmap
   strings, `INCBIN`), into `build-vita/generated/`. Some files get extra forced includes here
   (`src/intro.c` gets `src/platform/vita/compat/ctr_gba_stage.h`).
3. Compiles with `-O2 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -fno-strict-aliasing -fwrapv`.
   The decomp relies on type punning and signed overflow, hence the last two flags.
4. Assembles `data/*.s` and the songs into one `game_data.o` (`.rodata` renamed to `.data`
   because it holds pointers).
5. Links the game side into `game_combined.o` with its `.data`/`.bss` renamed, so all game
   state sits between known symbols and `vita_reset.c` can restore a power-on copy for resets.
6. Links through `vita/vita_link.sh` (pads the code segment for `vita-elf-create`).
7. `vita_create_self(... UNSAFE)` (the app writes `ux0:data/`), then `vita-mksfoex` and
   `vita-pack-vpk` called directly (`vita_create_vpk` breaks on paths with spaces).
8. Packs `vita/sce_sys/` (LiveArea), `assets/voxel/` → `voxel/`, `assets/ui/` → `ui/`,
   `assets/followers/` → `followers/`.

Title ID `PKEM00001`. LiveArea PNGs must be 8-bit indexed; `vita/make_livearea.sh` converts
full-colour art (needs `mingw-w64-x86_64-imagemagick`).

## Installing and testing

- **PS Vita**: install the VPK with VitaShell (HENkaku/Ensō, *Enable Unsafe Homebrew*).
  Disable overclock plugins for this title; the game sets its own clocks.
- **Vita3K**: good for boot, visuals, input and crashes; not for performance (it runs the ARM
  code much faster than the console). Measure on hardware with the telemetry
  ([DEBUGGING.md](DEBUGGING.md)).

## Warnings

The build prints no warnings; keep it that way. The flags are split in `CMakeLists.txt`:
the decompiled game keeps the leniency it was written for (`-Wno-implicit-function-declaration`
and friends), while `src/platform/` builds with `-Wall -Wextra` (only unused parameters, sign
comparisons and missing field initialisers are left out). The preprocessing step passes
`-Wno-trigraphs`, since `??` in game text is never meant as one.
