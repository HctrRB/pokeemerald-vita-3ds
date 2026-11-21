# Assets

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

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
| `assets/followers/` | `app0:followers/followers.pak` | `src/follower_mon.c` (`vita_pak.c`) | HGSS follower sprites (FMN1) and messages packed into PKF1 container by `tools/pack_followers.py` | Sources in `assets/followers/sources.txt` |

The voxel data files are made by Emerald 3DS's Python generators (CC BY-NC-SA 4.0). They read
the decompilation, not a ROM. The generators and the terrain editor used to fix faulty maps are
kept outside this repository (several GB; the author's working copy is an untracked voxelgen/
folder of the checkout); the files in `assets/voxel/` are committed as built.
Terrain faults are corrected as data (`relief_fixes.json`, read by `gen_voxel_relief.py`), never
by hand-editing `relief.bin`.

## 3. Generated C headers

| File | Content | Generator |
|---|---|---|
| `src/platform/ui/ui_sprite_ids.h` | Fixed UI atlas sprite enum matching `assets/ui2/ui_atlas.bin` | `tools/ui2/build_atlas.py` |
| `src/platform/ui/ui_town_signs.h` | Town and city sign mottos extracted from `data/maps/*/scripts.inc` | `tools/ui2/gen_town_signs.py` |

## Regenerating the UI art

```sh
python tools/ui2/bake_theme_fonts.py                               # theme_fonts.png + .json (needs local server)
python tools/ui2/build_atlas.py                                    # assets/ui2/ui_atlas.bin + ui_sprite_ids.h
python tools/ui2/gen_town_signs.py                                 # src/platform/ui/ui_town_signs.h
python tools/ui_gen/gen_intro_margins.py --output assets/ui/intro_margins.bin
```

`build_atlas.py` reads the base pixel fonts baked by `design/stitch/_tools/font_bake.html`
(`tools/ui2/fonts/ui_fonts.*`), the theme fonts baked by `tools/ui2/bake_theme_fonts.py`
(`tools/ui2/fonts/theme_fonts.*` for `RED`, `CRYSTAL`, `TROPICAL` and `ROYAL`), and the images
`design/stitch/_tools/export_assets.py` exports to `design/stitch/_assets/`. With `--locale <code>`
it also adds that language's extra letters to every font: the languages and their letters are listed
in `tools/locales/languages.json`, and `tools/ui2/bake_locale_glyphs.py` bakes all of them from the
real fonts (headless browser, Google Fonts) into the committed `tools/ui2/fonts/locale_glyphs.*`; the
base sheets are never rebaked, so the English atlas does not change. A translated game's text and
text graphics come from the player's ROM of that language at build time (`tools/locales/localize.py`,
through `tools/locales/<code>/manifest.json.gz`), into the build folder's staging tree only. The fixed part may take up to
`MAX_FIXED_H` rows (`UI_FIXED_MAX_H` at run time). `build_all` runs
both `export_assets.py` and `build_atlas.py` on every build, because `ui_sprite_ids.h` is
committed and the atlas must always match it. `gen_intro_margins.py` reads
`graphics/intro/scene_1`; run the asset step first.
