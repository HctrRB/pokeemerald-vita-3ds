# Build your own VPK

This repository does not contain the game's graphics, sounds or binary data, and it does not
distribute builds. You make your own VPK from your own copy of the game:

1. You need a dump of **your Pokémon Emerald cartridge** (`.gba`, any language). The script only
   reads it to check that it is Pokémon Emerald; nothing is copied from it, stored or uploaded.
2. The script then downloads the game files of the
   [pret/pokeemerald](https://github.com/pret/pokeemerald) decompilation (through
   [pokeemerald-multiplatform](https://github.com/gradenGnostic/pokeemerald-multiplatform), the
   version this port is built on) and checks every file against `tools/assets/manifest.txt`.
3. You build the VPK and install it on your PS Vita.

Do not share the VPK or the downloaded files: they contain Nintendo's game data.

Spanish version: [COMPILA_TU_VPK.md](COMPILA_TU_VPK.md).

## 1. Install the tools (Windows, once)

1. **MSYS2** in `C:\msys64` ([msys2.org](https://www.msys2.org)). Open *MSYS2 MSYS* and run:
   ```sh
   pacman -Syu
   pacman -S --needed make git cmake tar bzip2 xz wget curl python patch pkgconf diffutils \
       mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-pkgconf
   ```
2. **VitaSDK** with SDL2, in the same shell (also add the two `export` lines to `~/.bash_profile`):
   ```sh
   export VITASDK=/usr/local/vitasdk PATH=/usr/local/vitasdk/bin:$PATH VITASDK_CHANNEL=2026.08
   export TAR_OPTIONS=--force-local
   mkdir -p /c/vdpm-tmp && export TMPDIR=$(cygpath -m /c/vdpm-tmp)
   git clone --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm && (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
   VDPM_NONINTERACTIVE=1 vdpm install sdl2 sdl2_image
   ```
3. **Python 3 for Windows** ([python.org](https://www.python.org), tick *Add to PATH*) and Pillow,
   from PowerShell:
   ```powershell
   pip install pillow
   ```
4. **Git for Windows** ([git-scm.com](https://git-scm.com)) to download the repository.

Problems with MSYS2 or VitaSDK: see [BUILD.md](BUILD.md#requirements-windows).

## 2. Get the repository

```powershell
git clone https://github.com/HctrRB/pokeemerald-vita-3ds.git
cd pokeemerald-vita-3ds
```

## 3. Get the game files (once)

Put your dump anywhere, then from the repository folder in PowerShell:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\fetch_assets.ps1 -Rom "C:\path\to\emerald.gba"
```

It checks the dump, downloads about 9,000 files and generates the port's files made from them
(the interface atlas and the design mock images). It takes a few minutes the first time.

## 4. Build

In *MSYS2 MINGW64*, from the repository folder (`cd /c/path/to/pokeemerald-vita-3ds`):

```sh
make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"   # once
make -f vita/assets.mk -j8                                      # once
```

Then in PowerShell:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_vita.ps1
```

The result is `build-vita\pokeemerald.vpk`.

## 5. Install

1. Copy `pokeemerald.vpk` to the Vita (USB or FTP with VitaShell).
2. In VitaShell, press X on the VPK and install it (*Enable unsafe homebrew* must be on in
   HENkaku settings).
3. The game shows up as **Emerald Vita**. Saves and settings live in `ux0:data/POKEVITA/`.

Your own LiveArea art can go in `vita/sce_sys_custom/` with the same file names as
`vita/sce_sys/` (8-bit indexed PNGs); it is used instead of the neutral images.

## Common errors

| Message | Fix |
|---|---|
| `not a Pokémon Emerald dump` | The file is not a Pokémon Emerald ROM, or it is damaged. It must be 16 MB. |
| `git is needed` | Install Git in MSYS2: `pacman -S git`. |
| `Python 3 with Pillow is needed` | Install Python for Windows and run `pip install pillow`. |
| `the game files are missing` | Run step 3 before building. |
| `convert the game graphics first` | Run `make -f vita/assets.mk -j8` (step 4). |
