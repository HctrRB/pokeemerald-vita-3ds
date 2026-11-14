# Build your own VPK

> **Work in progress, in active testing.** Please report the bugs you find (see the end of this guide).

This repository does not contain the game's graphics, sounds or binary data, and it does not
distribute builds. You make your own VPK from your own copy of the game:

1. You need a dump of **your Pokémon Emerald cartridge** (`.gba`, any language). The script only
   reads it to check that it is Pokémon Emerald; nothing is copied from it, stored or uploaded.
2. The script then downloads the game files of the
   [pret/pokeemerald](https://github.com/pret/pokeemerald) decompilation (through
   [pokeemerald-multiplatform](https://github.com/gradenGnostic/pokeemerald-multiplatform), the
   version this port is built on) and checks every file against `tools/assets/manifest.txt`.
3. It builds the VPK, and you install it on your PS Vita.

Do not share the VPK or the downloaded files: they contain Nintendo's game data.

Spanish version: [COMPILA_TU_VPK.md](COMPILA_TU_VPK.md).

## What you need

- A Windows 10 or 11 PC with an internet connection and about 5 GB free.
- Your Pokémon Emerald dump (`.gba`, 16 MB).
- A PS Vita with HENkaku or Ensō and VitaShell.

Everything else (MSYS2, VitaSDK, Python, Git) is installed by the script.

## 1. Get the repository

Either clone it with Git:

```powershell
git clone https://github.com/HctrRB/pokeemerald-vita-3ds.git C:\Emerald\pokeemerald-vita-3ds
```

or, without Git, use **Code > Download ZIP** on GitHub and extract it.

Put it in a folder whose path has **no spaces or accents**, for example
`C:\Emerald\pokeemerald-vita-3ds`. The build tools cannot handle them (the script checks and
stops), so `C:\Users\Your Name\Downloads\...` will not work.

## 2. Build

Open PowerShell in the repository folder (in Explorer, right-click inside the folder >
*Open in Terminal*; on Windows 10, *File > Open Windows PowerShell*) and run:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1
```

A window asks for your `.gba`. You can also give it on the command line:
`.\build_all.ps1 -Rom "C:\path\to\emerald.gba"` (any path, spaces included).

The script:

1. installs MSYS2 in `C:\msys64` if it is missing, and the packages the build needs;
2. installs VitaSDK with SDL2 in MSYS2 (`/usr/local/vitasdk`) if it is missing;
3. checks your dump and downloads the game files (about 9,000 files);
4. builds the tools, converts the game files and builds the VPK.

The first run downloads 1–2 GB and can take 20–40 minutes; later runs take a few minutes. The
result is **`EmeraldVita.vpk`** in the repository folder. Every step that is already done is
skipped, so you can run the script again after a failure.

**Updating:** `git pull` (or download the ZIP again into the same folder) and run
`.\build_all.ps1` again. The ROM is only asked for the first time.

## 3. Install

1. Copy `EmeraldVita.vpk` to the Vita (USB or FTP with VitaShell).
2. In VitaShell, press X on the VPK and install it (*Enable unsafe homebrew* must be on in
   HENkaku settings).
3. The game shows up as **Emerald Vita**. Saves and settings live in `ux0:data/POKEVITA/`.

Your own LiveArea art can go in `vita/sce_sys_custom/` with the same file names as
`vita/sce_sys/` (8-bit indexed PNGs); it is used instead of the neutral images.

## Common errors

| Message | Fix |
|---|---|
| `This folder's path has spaces or special characters` | Move the repository folder to a simple path such as `C:\Emerald\pokeemerald-vita-3ds`. |
| `Could not download MSYS2` / `MSYS2 did not install` | Install MSYS2 by hand from [msys2.org](https://www.msys2.org) in `C:\msys64`, then run the script again. |
| `not a Pokémon Emerald dump` | The file is not a Pokémon Emerald ROM, or it is damaged. It must be 16 MB. |
| `VitaSDK did not install` | Usually a download that failed: run the script again. Otherwise see [BUILD.md](BUILD.md#requirements-windows). |
| `running scripts is disabled on this system` | Start it exactly as shown above (`powershell.exe -ExecutionPolicy Bypass -File ...`). |

## Building by hand

The script runs these steps; you can run them yourself in the *MSYS2 MINGW64* shell, from the
repository folder (`cd /c/Emerald/pokeemerald-vita-3ds`):

1. Packages and VitaSDK, once (the exact commands and pitfalls are in
   [BUILD.md](BUILD.md#requirements-windows)).
2. The game files, once:
   ```sh
   ./fetch_assets.sh /c/path/to/emerald.gba
   ```
3. The tools and the converted game files:
   ```sh
   make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"
   make -f vita/assets.mk -j8
   ```
4. The VPK: `./build_vita.sh` → `build-vita/pokeemerald.vpk`.

## Reporting bugs

This port is a **work in progress in active testing**. When something goes wrong, open an issue
on GitHub with the *Bug report* template and include:

- what you were doing and what happened (a photo of the screen helps);
- the files `perf_summary.txt`, `perf_frames.csv` and any `*.log` from `ux0:data/POKEVITA/`;
- for a crash, the `.psp2dmp` file from `ux0:data/` (see [DEBUGGING.md](DEBUGGING.md));
- the commit you built (`git log -1 --oneline`);
- if the build failed, the last lines the script printed.
