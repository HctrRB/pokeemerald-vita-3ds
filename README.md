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

Under active development; full validation on hardware is the next milestone. Known open issues:

- Intro and title screen margins at 16:9 are broken (sprites cut at the GBA edge, streaks
  beside Rayquaza); a replacement is designed in [docs/SCENES_16_9.md](docs/SCENES_16_9.md).
- The shop buy/sell flow over the 3D world still has visual glitches.
- Occasional frame drops in dense areas of the 3D overworld.

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

## Build

Step-by-step guide for users: [docs/BUILD_YOUR_OWN.md](docs/BUILD_YOUR_OWN.md)
(en español: [docs/COMPILA_TU_VPK.md](docs/COMPILA_TU_VPK.md)). In short, with MSYS2, VitaSDK,
Git and Python 3 + Pillow installed:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\fetch_assets.ps1 -Rom "C:\path\to\emerald.gba"   # once
```

```sh
make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"   # once, MINGW64 shell
make -f vita/assets.mk -j8                                      # once, and after asset edits
```

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_vita.ps1   # produces build-vita\pokeemerald.vpk
```

Install the VPK with VitaShell (title ID `PKEM00001`, *Enable Unsafe Homebrew* on). Vita3K runs
it for functional testing; measure performance on hardware.

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

- [docs/README.md](docs/README.md): index of the technical documentation.
- [CONTRIBUTING.md](CONTRIBUTING.md): code standards and workflow.

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
