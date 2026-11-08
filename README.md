# Pokémon Emerald Vita 3DS

Unofficial native PS Vita port of Pokémon Emerald, built from the
[pret/pokeemerald](https://github.com/pret/pokeemerald) decompilation. It is not an emulator:
the game's C code is compiled for the Vita's ARM CPU, and a platform layer supplies what the GBA
hardware did. On top of it, the port brings the features of
[Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds) to the Vita with vitaGL:

- a 3D voxel overworld built live from the game's maps, with sun, shadows, day/night and weather;
- 3D battles with a new HUD (plates, party balls, commands, moves and targets, touch);
- a new touch interface (option NEW UI) over the 3D world: START hub, Pokédex, party, bag,
  PokéNav, trainer card, options, shop, dialogue boxes, zone signs;
- HGSS follower Pokémon, an accelerated clock, time-based encounters and optional QoL extras
  (physical/special split, infinite TMs, running indoors, IV/EV pages, Nuzlocke tracker…).

> **AI-assisted project.** The code, build scripts and documentation were written with an AI
> coding assistant (Claude Code, by Anthropic) under the supervision of HctrRB, who sets the
> direction, reviews the changes and tests every build on a real PS Vita.

> **Legal notice.** Pokémon and Pokémon Emerald are © Nintendo, Creatures Inc. and GAME FREAK inc.
> This project is not affiliated with or endorsed by them. This repository does not contain the
> game's graphics, sounds or binary data, nor any ROM: to build, you check a dump of **your own
> cartridge** and the game files are downloaded from the public decompilation
> ([docs/BUILD_YOUR_OWN.md](docs/BUILD_YOUR_OWN.md)). Like every decompilation, the tree still
> holds the game's code and text in decompiled form. A built VPK contains Nintendo's data:
> **do not distribute builds**.

## Status

> **Work in progress, in active testing.** Expect bugs, crashes and unfinished screens. If you
> find one, please [open an issue](../../issues/new/choose) with the bug report template: what
> you did, what happened, and the log files from `ux0:data/POKEVITA/`.
> 60fps with drops to 55 at 333/166.

Known open issues:

- Intro and title screen margins at 16:9 are broken (sprites cut at the GBA edge, streaks
  beside Rayquaza); a replacement is designed in [docs/SCENES_16_9.md](docs/SCENES_16_9.md).
- The shop buy/sell flow over the 3D world still has visual glitches.
- Occasional frame drops in dense areas of the 3D overworld.
- Missing some UI screens, give feedback.
- Terrain generation bugs (came from Emerald 3DS)
- Menu can freeze sometimes
- Evolutions and some screens are not tested.

- Give feedback!!

## Repository layout

```
CMakeLists.txt, build_vita.ps1/.sh   Vita build (preprocess → compile → link → SELF → VPK)
make_tools.mk, *_rules.mk, vita/     host tools, asset rules, link wrapper, LiveArea art
src/, include/, asm/, data/,         the pokeemerald decompilation (game code and text data;
constants/                           graphics/, sound/ and data binaries come from fetch_assets)
fetch_assets.sh/.ps1, tools/assets/  game files download (checks your ROM, then the manifest)
src/platform/                        GBA hardware emulation and main loop
src/platform/voxel/                  3D voxel world
src/platform/vita/                   native UI and Vita-specific features
assets/                              port data packed into the VPK (voxel, UI art, followers)
tools/                               pret host tools and the UI art generators (tools/ui_gen)
docs/                                technical documentation
```

## Build your own VPK

No builds are distributed: you make your own from your Pokémon Emerald cartridge. Full guide,
with every command and the common errors: [docs/BUILD_YOUR_OWN.md](docs/BUILD_YOUR_OWN.md)
(en español: [docs/COMPILA_TU_VPK.md](docs/COMPILA_TU_VPK.md)).

**One step (Windows).** With [MSYS2](https://www.msys2.org) installed in `C:\msys64` and the
repository cloned, one script installs what is missing (MSYS2 packages, VitaSDK, Python with
Pillow), checks your dump, gets the game files, builds everything and leaves `EmeraldVita.vpk`
in the repository folder:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1 -Rom "C:\path\to\emerald.gba"
```

The ROM is only needed the first time; after `git pull`, run `.\build_all.ps1` again to rebuild.
The steps it runs, one by one:

1. **Dump your cartridge** to a `.gba` file (any language of Pokémon Emerald, 16 MB).
2. **Install the tools** (Windows, once): [MSYS2](https://www.msys2.org) in `C:\msys64`,
   [VitaSDK](https://vitasdk.org) with SDL2, [Git](https://git-scm.com) and
   [Python 3](https://www.python.org) with Pillow (`pip install pillow`). Exact commands:
   [docs/BUILD.md](docs/BUILD.md#requirements-windows).
3. **Get the repository**:
   ```powershell
   git clone https://github.com/HctrRB/pokeemerald-vita-3ds.git
   cd pokeemerald-vita-3ds
   ```
4. **Get the game files** (once). The script checks that your dump is Pokémon Emerald, then
   downloads the decompilation's graphics, sounds and data and checks each file
   ([tools/assets/manifest.txt](tools/assets/manifest.txt)). Nothing is copied from the ROM.
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File .\fetch_assets.ps1 -Rom "C:\path\to\emerald.gba"
   ```
5. **Build the host tools and convert the game files** (once), in the *MSYS2 MINGW64* shell:
   ```sh
   make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"
   make -f vita/assets.mk -j8
   ```
6. **Build the VPK**, in PowerShell:
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File .\build_vita.ps1
   ```
   The result is `build-vita\pokeemerald.vpk`.
7. **Install it** with VitaShell (*Enable unsafe homebrew* on in HENkaku settings). The game
   appears as **Emerald Vita** (title ID `PKEM00001`); saves and settings live in
   `ux0:data/POKEVITA/`.

Do not share the VPK or the downloaded files: they contain Nintendo's game data. Vita3K runs the
VPK for functional testing; measure performance on hardware.

## Controls

| Button | Action |
|---|---|
| Cross / Circle | A / B |
| D-pad or left stick | Move |
| L / R | L / R |
| START | Open or close the touch menu (field) |
| SELECT | Registered item |
| Triangle | Fast forward |
| Square | Options |
| Touch | Menu, battle action deck |

Buttons can be remapped in Options. Settings and saves live in `ux0:data/POKEVITA/`
(see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#runtime-files-ux0datapokevita)).

## Documentation

| Document | Covers |
|---|---|
| [docs/BUILD_YOUR_OWN.md](docs/BUILD_YOUR_OWN.md) ([español](docs/COMPILA_TU_VPK.md)) | Make your own VPK from your cartridge, step by step |
| [docs/BUILD.md](docs/BUILD.md) | Toolchain, build steps, CMake options, installing and testing |
| [docs/README.md](docs/README.md) | Index of the technical documentation |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, boot, one frame, where things are, runtime files |
| [docs/THREADS.md](docs/THREADS.md) | Cores, pacing, who owns which data |
| [docs/RENDERING.md](docs/RENDERING.md) | Presentation paths, voxel passes, GBA overlay, GPU rules |
| [docs/VITA_UI.md](docs/VITA_UI.md) | The new UI and the original Vita UI |
| [docs/VOXEL_ENGINE.md](docs/VOXEL_ENGINE.md) | The 3D overworld |
| [docs/SCENES_16_9.md](docs/SCENES_16_9.md) | How GBA screens fill 16:9 |
| [docs/ASSETS.md](docs/ASSETS.md) | Where every data file comes from |
| [docs/DEBUGGING.md](docs/DEBUGGING.md) | Logs, per-frame telemetry, crash dumps |
| [design/stitch/README.md](design/stitch/README.md) | Design mocks of the new UI (Spanish) |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Code standards and workflow |
| [NOTICE.md](NOTICE.md) | Licences of the ported code and credits |

## Credits

- **pret** and contributors: the pokeemerald decompilation and its tools.
- **Zallax** and the Emerald 3DS contributors: [Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds),
  the source of most of what this port adds: the voxel world, the 3D battles, the followers, the
  extras and the look of the interface.
- **gradenGnostic** and contributors:
  [pokeemerald-multiplatform](https://github.com/gradenGnostic/pokeemerald-multiplatform), the SDL2
  platform layer and voxel renderer this port starts from.
- **Rinnegatamante** (vitaGL), the **VitaSDK**, **SDL** and **Vita3K** teams, **Dave Gamble**
  (cJSON) and **Niels Lohmann** (nlohmann/json).
- **HctrRB**: project lead, supervision and hardware testing.

## Legal

- Pokémon, Pokémon Emerald and all related names, characters, graphics, music and text are
  trademarks and copyrights of Nintendo, Creatures Inc. and GAME FREAK inc. This is a
  non-commercial fan project.
- This repository does not include the game's graphics, sounds or binary data, nor any ROM.
  `fetch_assets` checks a dump of the user's own cartridge and downloads those files from the
  public pret/pokeemerald decompilation (via pokeemerald-multiplatform); they remain the property
  of their owners. The decompiled code and text in this tree come from the same decompilation.
  Builds contain that material: don't upload, share or sell them. Use this project only with a
  legitimately owned copy of Pokémon Emerald.
- Code licences differ by origin: MIT for the multiplatform layer and original Vita code
  ([LICENSE](LICENSE)), **CC BY-NC-SA 4.0** for everything derived from Emerald 3DS. The full list
  is in [NOTICE.md](NOTICE.md).
- Provided as is, without warranty. Rights holders can request changes or removal by opening an
  issue.
