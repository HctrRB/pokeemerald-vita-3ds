# User interface

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

The port's menus, dialogue boxes, battle HUD and the scenes it replaces are the new UI
(`src/platform/ui/`), drawn natively over the 3D world, with six menu themes (GBA by default). It is the only
menu system: the old 3DS-style Vita menu (`vita_ui*.c`) was removed on 2026-11-13. Where the new UI
has no screen, or cannot open one, the game's own GBA screen is used:

- START in link and Union rooms, the Battle Pike and the Battle Pyramid opens the game's START
  menu (`Ui_OwnsFieldStart`): its entries there (REST, RETIRE, link cards) are not the hub's.
- The battle bag in the Battle Pyramid, and any battle bag or party screen the UI cannot open,
  fall back to the game's (`battle_controller_player.c`).
- Decoration shops (`MART_TYPE_DECOR` / `MART_TYPE_DECOR2`) are BUY-only in the new UI, as in the game.
- OPTION on the title menu opens the new options screen hosted (`Ui_OpenOptionsHosted`).
- If the UI atlas cannot be loaded, every menu is the game's.

Button mapping, fast forward, always-run and the save path are in
`src/platform/vita/vita_controls.c`. Holding the 4-button soft-reset chord (`A + B + Start + Select`, mapped to
`X + O + Start + Select` by default) bypasses `Ui_OwnsFieldStart()` and the Hub opening on `START` so soft reset
works in the overworld as well as on the title screen and in battle (`sMaskSoftResetChord` in `sdl2.c` masks the
chord after a reset until at least one button is released).

## New UI (`src/platform/ui/`)

Core 1 (the game thread) runs `Ui_Tick` between game frames and builds a draw list; Core 0 draws
it in one call over one `UI_TEX_W`×`UI_TEX_H` RGBA8888 texture (fixed atlas `assets/ui2/ui_atlas.bin` in rows
0–1023 + 256 64×64 cells in rows 1024–2047 for sprites decoded at run time). See
[THREADS.md](THREADS.md) and [RENDERING.md](RENDERING.md).

| Files | Role |
|---|---|
| `ui_core.c`, `ui_draw.c`, `ui_render.c`, `ui_assets.c`, `ui_sprites.c` | Screen stack, input, draw list, texture, atlas, sprite cache |
| `ui_theme.c` | Six menu themes (`GBA`, `RED`, `EMERALD`, `CRYSTAL`, `TROPICAL`, `ROYAL`; order of `UI_THEME_*`): colour tokens, box geometry, battle HUD tokens and baked font face IDs |
| `ui_widgets.c`, `ui_kit.c` | Boxes, rows, buttons, lists, popups, toasts |
| `ui_scr_*.c` | Screens: START hub, Pokédex, party, bag, PokéNav, trainer card, options, Nuzlocke, mart, PC boxes, player's PC (items, mailbox, decorations), naming keyboard, Easy Chat, berry choice, forget a move, choose a Pokémon |
| `ui_contest.c` | Contest appeal choice |
| `ui_mirror.c`, `ui_sign.c` | Dialogue, YES/NO, multichoice and money boxes (including 2D evolution, egg-hatch and trade dialogue mirrors), plus zone signs |
| `ui_battle_hud.c`, `ui_game_battle.c` | Battle HUD (plates, party balls, messages, commands, moves, targets, level up) and the battle bag / party |
| `ui_game*.c` | Game data and actions used by the screens |
| `ui_stall.c` | Diagnostic: writes `ui_stall.log` when the UI stops updating for more than 100 ms |

### Opening modes (`ui_core.c`)

- **Field overlay (`Open`)**: opened from the overworld with START / Square (`LockPlayerFieldControls`).
- **Script-driven (`UiCore_OpenFromScript`)**: opened by field script tasks (Poké Mart and decoration shops, PC boxes, player's PC, Day Care / tutor party choice, forget a move); the game task waits for `!Ui_IsOpen()`.
- **Hosted full-screen (`UiCore_OpenHosted` / `CB2_UiHost`)**: replaces a GBA `MainCallback2` scene (naming screen, Easy Chat, Berry Blender / Berry Crush choice, title-menu options) while keeping GBA sprites and tasks alive and running scene resets before opening.

### Themes (`ui_theme.c`)

Selected in `OPTIONS > GENERAL > DISPLAY > MENU COLOR` (extra `"ui_theme"`):

| Theme | Characteristics | Fonts |
|---|---|---|
| `GBA` (default) | Gen 3 look: translucent teal frame, GBA bevel and shadow | VT323 (antialiased) |
| `RED` | Chamfered dark-slate corners, diagonal frame stripes, red highlight bar | Saira Condensed |
| `EMERALD` | Warm paper panels, double bevel, dark teal frame | Press Start 2P + VT323 |
| `CRYSTAL` | Cool ice panels, glossy pill highlights | M PLUS Rounded 1c |
| `TROPICAL` | Warm sand panels, coral/amber pills, heavy dark outlines | Fredoka |
| `ROYAL` | Deep emerald-and-gold forest aesthetic, chamfered selection bar | Exo 2 |

Design mocks: [design/stitch/](../../design/stitch/README.md) (`?theme=` previews each theme). The atlas
is built by `tools/ui2/build_atlas.py`.

## Game features behind the UI

| File | Role |
|---|---|
| `vita_extras.c/h` | QoL switches stored in `settings.txt`; `VitaExtras_GetFast(FAST_EXTRA_*)` is an O(1) cache read by game code |
| `vita_daynight.c/h` | Accelerated clock (1×–1000×) and day/night light |
| `vita_encounters.c/h` | Time-of-day wild encounter tables for 20 routes |
| `vita_nuzlocke.c` | Nuzlocke rules and tracking |
| `src/follower_mon.c` | HGSS follower Pokémon (`assets/followers/`) |
