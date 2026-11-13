# Notices

This repository combines the pret decompilation, code from two earlier ports, original PS Vita
code and a few third-party components. Each part keeps its own terms. This file lists them;
[LICENSE](LICENSE) is the MIT licence and covers only the parts named below as MIT.

## Game content

Pokémon Emerald's code, data, graphics, audio and text are © Nintendo, Creatures Inc. and
GAME FREAK inc. The decompiled form of that material in this tree (`src/` outside
`src/platform/`, `include/`, `asm/`, `data/`, `graphics/`, `sound/`, `constants/`) is not licensed
by this project. A built VPK contains it: do not distribute builds.

## Upstream decompilation

- **pret/pokeemerald** — <https://github.com/pret/pokeemerald>. The repository does not state a
  licence; this project does not relicense it.

## Code derived from other ports

| Component | Source | Licence | Used in |
|---|---|---|---|
| **pokeemerald-multiplatform** | <https://github.com/gradenGnostic/pokeemerald-multiplatform> | MIT ([LICENSE](LICENSE)) | The SDL2 platform layer this port starts from (`src/platform/sdl2.c`, `gba_fast_draw*.c`, `widescreen.c`, `bios.c`, `dma.c`, `cgb_audio.c`, `mods/`) and the original voxel renderer logic (map instances, metatile classification, atlas composition, follow camera) |
| **Emerald 3DS** | <https://github.com/ZallaxDev/emerald-3ds>, v0.5.0, commit `bcae4b7` | **CC BY-NC-SA 4.0** (<https://creativecommons.org/licenses/by-nc-sa/4.0/>) | Logic and data ported to vitaGL and the Vita: the voxel overworld (`src/platform/voxel/`: buildings, relief, trees, signposts, lighting and shadows, weather, entities, 3D battle), the Emerald 3DS user interface and its features (`src/platform/vita/vita_ui*.c`, `vita_battle_ui.c`, `vita_world_ui.c`, `vita_shop_ui.c`, `vita_corner_art.c`, `vita_stage.c`, `vita_daynight.c`, `vita_encounters.c`, `vita_extras.c`, `vita_nuzlocke.c`, `3ds_bottom_art.h`), the follower Pokémon (`src/follower_mon.c`), the scene classification headers (`src/platform/vita/compat/`) and the voxel data generators with their hand-made inputs (`voxelgen/`, copied from `3ds_port/scripts` and `3ds_port/assets/voxel`) |
| **pokeemerald-rogue-vita** (reference) | <https://github.com/MinZe25/pokeemerald-rogue-vita> | — | Consulted as a reference for a few low-level techniques: resetting the game from a boot copy of its data, the link padding value, the DC-blocking audio filter and two renderer optimisations |

Emerald 3DS states (`LICENSE-PORT.md`, `NOTICE.md` of that repository) that its own port code is
CC BY-NC-SA 4.0 since v0.5.0, and that its voxel code in turn derives from
pokeemerald-multiplatform (MIT). Consequences for this repository:

- Files derived from Emerald 3DS are distributed under **CC BY-NC-SA 4.0**: attribution to
  "Zallax and Emerald 3DS contributors", **non-commercial use only**, and modified versions must
  be shared under the same licence.
- The MIT grant in [LICENSE](LICENSE) does **not** apply to those files.
- Each derived file should name its origin in its header comment. Until every header is
  updated, the list above is authoritative.

## Original PS Vita code

Code written for this port that is not derived from Emerald 3DS (the VitaSDK/vitaGL backend,
build scripts, telemetry, the multicore pipeline) is offered under the MIT licence of
[LICENSE](LICENSE), like the multiplatform port it extends.

## Third-party libraries and tools

| Component | Licence | Use |
|---|---|---|
| VitaSDK toolchain, newlib, libraries | various open source | Build and runtime |
| vitaGL (Rinnegatamante) | LGPL-2.1 | OpenGL on SceGxm (linked) |
| SDL2, SDL2_image (+ libpng, libjpeg, libwebp, zlib) | zlib and others | Audio device, timing, threads |
| cJSON (Dave Gamble), in `src/platform/mods/` | MIT | JSON parsing |
| nlohmann/json (Niels Lohmann), in `tools/jsonproc/` | MIT | Host tool |
| pret host tools (`tools/preproc`, `gbagfx`, `aif2pcm`, `mid2agb`, `mapjson`, `jsonproc`, `scaninc`) | from pret | Asset conversion at build time |

## Follower Pokémon assets

The overworld sprites and messages in `assets/followers/` come from pret/pokeheartgold (commit
`3291d88a7e3e264b503024ead80f4d8b55b835cd`) and Poké Corpus by abcboy101 (commit
`5d1da078d70dea5c9feaaba7575626eab4f10a08`), through the forks listed in
`assets/followers/sources.txt`. They are Nintendo, Creatures and GAME FREAK's work and are not
relicensed.
