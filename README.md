# Pokémon Emerald Vita 3DS

Unofficial native PS Vita port of Pokémon Emerald, built from the
[pret/pokeemerald](https://github.com/pret/pokeemerald) decompilation. It is not an emulator:
the game's C code is compiled for the Vita's ARM CPU, and a platform layer supplies what the GBA
hardware did. On top of it, the port brings the features of
[Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds) to the Vita with vitaGL:

- a 3D voxel overworld built live from the game's maps, with sun, shadows, day/night and weather;
- 3D battles on a stage taken from the map where the battle starts;
- a new touch interface over the 3D world that replaces the GBA menus and boxes, with five
  menu themes;
- HGSS follower Pokémon, an accelerated clock, time-based encounters and optional QoL extras.

The full list is under [Features](#features).

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

## Features

### 3D world

- Voxel overworld built live from the game's maps: sun and shadows, day/night cycle, weather,
  depth of field, water reflections, 3D buildings and trees. 30 or 60 FPS (OPTIONS > WORLD).
- 3D battles: the stage is picked from the map around the player, and the camera glides from
  the field into it.
- Wild encounters in the 3D world fade smoothly into the battle.
- HGSS follower Pokémon, an accelerated clock and time-based encounters.

### New interface

Drawn natively at 960×544 over the 3D world. Every screen works with the buttons and with touch
(first tap selects, a second tap on the selection confirms).

- **START menu**: Pokédex (list, entry, cry, size, area), party (summary pages, switch, held
  items, field moves, move reorder), bag (pockets, sort, use / give / toss / register), PokéNav
  (map, Match Call, condition, ribbons), Fly map, trainer card, Nuzlocke tracker, options, save.
- **PC boxes**: withdraw, deposit, move POKéMON and items, release, marks, jump to a box,
  wallpapers (the game's own, Walda's included) and box names.
- **Scenes the scripts open**: Poké Mart buy / sell, naming keyboard (player, rival, POKéMON,
  boxes), Easy Chat (mail, profile, battle messages, interviews, trendy saying, survey…), berry
  choice for the Berry Blender and Berry Crush, contest appeal choice, "choose a POKéMON" for
  the move tutor, the Day Care and other scripts.
- **Battle**: HUD with health plates, party balls, commands, moves with PP and type, targets in
  double battles, YES / NO, level-up stats and the Safari Zone; bag and party in battle (multi
  battles included); the Battle Pyramid bag.
- **Field**: dialogue box, YES / NO, choice lists (PC menus, NPC questions), money box, item
  obtained, PokéNav calls and zone signs with each town's motto.
- **Menu themes** (OPTIONS > GENERAL > MENU COLOR): RED (default), EMERALD, CRYSTAL, TROPICAL
  and ROYAL. Each has its own colours, box shapes, bevels and font, and applies to every screen,
  the battle HUD and the zone signs.

### Extras

Physical / special split, infinite TMs, running indoors, IV / EV pages, EXP Share and EXP
multiplier, Nuzlocke rules, field HM shortcuts, button remapping.

## Status

> **Work in progress, in active testing.** Expect bugs, crashes and unfinished screens. If you
> find one, please [open an issue](../../issues/new/choose) with the bug report template: what
> you did, what happened, and the log files from `ux0:data/POKEVITA/`. Feedback on screens
> that are missing or feel wrong is very welcome.

Performance: 60 FPS with drops to 55 at 333 / 166 MHz.

### Not yet tested on a PS Vita

These parts build and run in the design mocks, but have not been checked on the console yet:

- The menu themes on every screen, the battle HUD in each theme, and the lighter TROPICAL font.
- PC boxes: moving, withdrawing, depositing, releasing, marks, wallpapers and box names.
- Naming keyboard: new game, box names, nicknames, a POKéMON caught with a full party.
- Easy Chat in each of its uses, the berry choice and the contest appeal choice.
- Day Care and move tutor choice (the right POKéMON is taken, the screen fades back in).
- The dialogue box after PokéNav calls and YES / NO answers (it used to flicker).
- The words of the PC choice menus, larger POKéMON names in battle.
- The fade into wild battles in the 3D world.
- Evolutions and some other scenes.

### Known issues and gaps

- Still on the GBA interface: the summary of a boxed POKéMON in the PC, the Battle Pyramid bag
  inside battles and its "bag full: choose what to drop" step, and Easy Chat's Bard song and
  quiz screens.
- Some UI screens are still missing: give feedback on the ones you find.
- The menu can freeze sometimes.
- Terrain generation bugs in some maps (they come from Emerald 3DS).
- The shop buy / sell flow over the 3D world with the new UI off still has visual glitches.
- Walda's FRIENDS wallpaper may draw without its frame.
- Trainer and legendary battle transitions are the GBA ones and only partly show over the 3D
  world.
- Link and recorded battles, Wally's catching tutorial and the Battle Arena judgment keep the
  GBA battle HUD.
- Intro and title screen margins at 16:9 are broken (sprites cut at the GBA edge, streaks
  beside Rayquaza); a replacement is designed in [docs/SCENES_16_9.md](docs/SCENES_16_9.md).
- Occasional frame drops in dense areas of the 3D overworld.
- Vita3K shows a black screen; test on hardware.

## Repository layout

```
build_all.ps1/.sh                    one step from your dump to EmeraldVita.vpk
CMakeLists.txt, build_vita.ps1/.sh   Vita build (preprocess → compile → link → SELF → VPK)
make_tools.mk, *_rules.mk, vita/     host tools, asset rules, link wrapper, LiveArea art
src/, include/, asm/, data/,         the pokeemerald decompilation (game code and text data;
constants/                           graphics/, sound/ and data binaries come from fetch_assets)
fetch_assets.sh/.ps1, tools/assets/  game files download (checks your ROM, then the manifest)
src/platform/                        GBA hardware emulation and main loop
src/platform/voxel/                  3D voxel world
src/platform/ui/                     the new interface (screens, widgets, themes)
src/platform/vita/                   Vita-specific features (clock, encounters, extras, stage)
assets/                              port data packed into the VPK (voxel, UI atlas, followers)
tools/                               pret host tools, UI atlas builder (tools/ui2), generators
design/stitch/                       HTML mocks of the new interface
docs/                                technical documentation
```

## Build your own VPK

No builds are distributed: you make your own from your Pokémon Emerald cartridge. Full guide,
with every command and the common errors: [docs/BUILD_YOUR_OWN.md](docs/BUILD_YOUR_OWN.md)
(en español: [docs/COMPILA_TU_VPK.md](docs/COMPILA_TU_VPK.md)).

You need a Windows 10/11 PC, your Pokémon Emerald dump (`.gba`, any language, 16 MB) and a
PS Vita with HENkaku or Ensō. Nothing else has to be installed by hand:

1. **Get the repository** (`git clone`, or *Code > Download ZIP*) into a folder with no spaces
   or accents in its path, e.g. `C:\Emerald\pokeemerald-vita-3ds`.
2. **Run the script** in PowerShell from that folder, and choose your `.gba` in the window it
   opens:
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1
   ```
   It installs what is missing ([MSYS2](https://www.msys2.org) in `C:\msys64`, its packages,
   [VitaSDK](https://vitasdk.org)), checks your dump, downloads the game files and checks each
   one against [tools/assets/manifest.txt](tools/assets/manifest.txt) (nothing is copied from
   the ROM), and builds everything. The first run takes 20–40 minutes.
3. **Install `EmeraldVita.vpk`** with VitaShell (*Enable unsafe homebrew* on in HENkaku
   settings). The game appears as **Emerald Vita** (title ID `PKEM00001`); saves and settings
   live in `ux0:data/POKEVITA/`.

To update: `git pull` and run `.\build_all.ps1` again (the ROM is only asked for the first
time). The manual steps are in the guide and in [docs/BUILD.md](docs/BUILD.md).

Do not share the VPK or the downloaded files: they contain Nintendo's game data.

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
| [docs/VITA_UI.md](docs/VITA_UI.md) | The user interface |
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
