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
| `assets/voxel/buildings.bin` | `app0:voxel/` | `voxel_building.c` | Building models and texture pages | Emerald 3DS `scripts/gen_voxel_buildings.py` |
| `assets/voxel/relief.bin` | `app0:voxel/` | `voxel_relief.c` | Cliffs, stairs, sea rocks per layout | Emerald 3DS `scripts/gen_voxel_relief.py` + `relief_fixes.json` |
| `assets/voxel/regions.bin` | `app0:voxel/` | `voxel_regions.c` | Cell roles, 4 bits per cell | Emerald 3DS `scripts/gen_voxel_regions.py` |
| `assets/voxel/signposts.bin` | `app0:voxel/` | `voxel_sign.c` | Signpost / lamp pixel masks | Emerald 3DS `scripts/gen_voxel_sign_masks.py` |
| `assets/voxel/trees.rgba5551` | `app0:voxel/` | `voxel_renderer.c` | Tree atlas | Emerald 3DS `scripts/gen_voxel_trees.py` |
| `assets/ui2/ui_atlas.bin` | `app0:ui2/` | `ui_assets.c` | Fixed part of the new UI's texture: baked theme fonts, white block, UI sprites | `tools/ui2/build_atlas.py` (also writes `src/platform/ui/ui_sprite_ids.h`) |
| `assets/ui/intro_margins.bin` (`EM3DLVS1`) | `app0:ui/` | `vita_stage.c` | Leaves margins of the intro | `tools/ui_gen/gen_intro_margins.py` (reads `graphics/intro/scene_1`; run by `build_vita.sh`, not committed) |
| `assets/followers/` | `app0:followers/` | `src/follower_mon.c` | HGSS follower sprites (FMN1) and messages | Sources in `assets/followers/sources.txt` |

The voxel data files are made by Emerald 3DS's Python generators (CC BY-NC-SA 4.0). They read
the decompilation, not a ROM. The generators and the terrain editor used to fix faulty maps are
kept outside this repository (several GB); the files in `assets/voxel/` are committed as built.
Terrain faults are corrected as data (`relief_fixes.json`, read by `gen_voxel_relief.py`), never
by hand-editing `relief.bin`.

## 3. Generated C tables

| File | Content | Generator |
|---|---|---|
| `src/platform/vita/3ds_bottom_art.h` | Dock and UI sprites (4bpp tiles + palettes) | Emerald 3DS `scripts/gen_bottom_art.py` (not in this repository) |

## Regenerating the UI art

```sh
python tools/ui2/build_atlas.py                                    # assets/ui2/ui_atlas.bin
python tools/ui_gen/gen_intro_margins.py --output assets/ui/intro_margins.bin
```

`build_atlas.py` reads the fonts baked by `design/stitch/_tools/font_bake.html`
(`tools/ui2/fonts/`). `gen_intro_margins.py` reads `graphics/intro/scene_1`; run the asset step
first.
