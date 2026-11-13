# Assets

Three kinds of data end up in the VPK.

## 1. Game data from the decompilation

`graphics/`, `sound/`, `data/` and `src/data/` hold the game's graphics, audio, maps and tables
as extracted by pret. They are converted at build time; no ROM is read.

| Source | Tool | Output |
|---|---|---|
| `graphics/**/*.png`, `*.pal` (downloaded by `fetch_assets`, listed in `tools/assets/manifest.txt`) | `gbagfx` | `.4bpp`, `.8bpp`, `.gbapal`, `.lz` next to the source |
| `sound/**/*.aif`, `sound/songs/midi/*.mid` | `aif2pcm`, `mid2agb` | `.bin` samples, `.s` songs |
| `data/maps/**/*.json`, `src/data/**/*.json` | `mapjson`, `jsonproc` | `.inc` / `.h` included by the code |

Run by `make -f vita/assets.mk`, which reuses the decomp's `*_rules.mk` files. The outputs are
git-ignored.

## 2. Port data (`assets/`)

| File | Packed as | Read by | Format | Generator |
|---|---|---|---|---|
| `assets/voxel/buildings.bin` | `app0:voxel/` | `voxel_building.c` | Building models and texture pages | `voxelgen/scripts/gen_voxel_buildings.py` |
| `assets/voxel/relief.bin` | `app0:voxel/` | `voxel_relief.c` | Cliffs, stairs, sea rocks per layout | `voxelgen/scripts/gen_voxel_relief.py` + `voxelgen/assets/voxel/relief_fixes.json` |
| `assets/voxel/regions.bin` | `app0:voxel/` | `voxel_regions.c` | Cell roles, 4 bits per cell | `voxelgen/scripts/gen_voxel_regions.py` |
| `assets/voxel/signposts.bin` | `app0:voxel/` | `voxel_sign.c` | Signpost / lamp pixel masks | `voxelgen/scripts/gen_voxel_sign_masks.py` |
| `assets/voxel/trees.rgba5551` | `app0:voxel/` | `voxel_renderer.c` | Tree atlas | `voxelgen/scripts/gen_voxel_trees.py` |
| `assets/ui/menu.bin` (`EM3DMNU1`) | `app0:ui/` | `vita_ui_plate.c` | Menu backdrop | `tools/ui_gen/gen_menu_art.py` |
| `assets/ui/battle.bin` (`EM3DBTA1`) | `app0:ui/` | `vita_ui_plate.c` | Battle UI art | `tools/ui_gen/gen_battle_art.py` |
| `assets/ui/intro_margins.bin` (`EM3DLVS1`) | `app0:ui/` | `vita_stage.c` | Leaves margins of the intro | `tools/ui_gen/gen_intro_margins.py` (reads `graphics/intro/scene_1`; run by `build_vita.sh`, not committed) |
| `assets/followers/` | `app0:followers/` | `src/follower_mon.c` | HGSS follower sprites (FMN1) and messages | Sources in `assets/followers/sources.txt` |

The voxel data files are made by Emerald 3DS's generators, copied into `voxelgen/`
(CC BY-NC-SA 4.0). They read the decompilation in this tree, not a ROM, and reproduce the
shipped files byte for byte. `voxelgen/regen.sh` regenerates them into `assets/voxel/`; see
[voxelgen/README.md](../voxelgen/README.md). Terrain faults are corrected as data in
`voxelgen/assets/voxel/relief_fixes.json`.

## 3. Generated C tables

| File | Content | Generator |
|---|---|---|
| `src/platform/vita/3ds_bottom_art.h` | Dock and UI sprites (4bpp tiles + palettes) | Emerald 3DS `scripts/gen_bottom_art.py` (not in this repository) |

## Regenerating the UI art

Each script takes `--output`:

```sh
python tools/ui_gen/gen_menu_art.py     --output assets/ui/menu.bin
python tools/ui_gen/gen_battle_art.py   --output assets/ui/battle.bin
python tools/ui_gen/gen_intro_margins.py --output assets/ui/intro_margins.bin
```

`gen_intro_margins.py` reads `graphics/intro/scene_1`; run the asset step first.
