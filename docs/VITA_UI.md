# User interface

The port's menus, dialogue boxes, battle HUD and the scenes it replaces are the new UI
(`src/platform/ui/`), drawn natively over the 3D world, with five menu themes. It is the only
menu system: the old 3DS-style Vita menu (`vita_ui*.c`) was removed on 2026-11-13. Where the new UI
has no screen, or cannot open one, the game's own GBA screen is used:

- START in link and Union rooms, the Battle Pike and the Battle Pyramid opens the game's START
  menu (`Ui_OwnsFieldStart`): its entries there (REST, RETIRE, link cards) are not the hub's.
- The battle bag in the Battle Pyramid, and any battle bag or party screen the UI cannot open,
  fall back to the game's (`battle_controller_player.c`).
- The mart's SELL tab falls back to the game's sell screen in marts other than the normal one.
- OPTION on the title menu opens the new options screen hosted (`Ui_OpenOptionsHosted`).
- If the UI atlas cannot be loaded, every menu is the game's.

Button mapping, fast forward, always-run and the save path are in
`src/platform/vita/vita_controls.c`.

## New UI (`src/platform/ui/`)

Core 1 (the game thread) runs `Ui_Tick` between game frames and builds a draw list; Core 0 draws
it in one call over one 1024×1024 texture (fixed atlas `assets/ui2/ui_atlas.bin` + sprites decoded
at run time). See [THREADS.md](THREADS.md) and [RENDERING.md](RENDERING.md).

| Files | Role |
|---|---|
| `ui_core.c`, `ui_draw.c`, `ui_render.c`, `ui_assets.c`, `ui_sprites.c` | Screen stack, input, draw list, texture, atlas, sprite cache |
| `ui_widgets.c`, `ui_kit.c` | Boxes, rows, buttons, lists, popups, toasts |
| `ui_scr_*.c` | Screens: START hub, Pokédex, party, bag, PokéNav, trainer card, options, Nuzlocke, mart, choose a Pokémon |
| `ui_mirror.c`, `ui_sign.c` | Dialogue, YES/NO, multichoice and money boxes, and zone signs, drawn from the game's own windows |
| `ui_battle_hud.c`, `ui_game_battle.c` | Battle HUD (plates, party balls, messages, commands, moves, targets, level up) and the battle bag / party |
| `ui_game*.c` | Game data and actions used by the screens |
| `ui_stall.c` | Diagnostic: writes `ui_stall.log` when the UI stops updating for more than 100 ms |

Design mocks: [design/stitch/](../design/stitch/README.md). The atlas is built by
`tools/ui2/build_atlas.py`.

## Game features behind the UI

| File | Role |
|---|---|
| `vita_extras.c/h` | QoL switches stored in `settings.txt`; `VitaExtras_GetFast(FAST_EXTRA_*)` is an O(1) cache read by game code |
| `vita_daynight.c/h` | Accelerated clock (1×–1000×) and day/night light |
| `vita_encounters.c/h` | Time-of-day wild encounter tables for 20 routes |
| `vita_nuzlocke.c` | Nuzlocke rules and tracking |
| `src/follower_mon.c` | HGSS follower Pokémon (`assets/followers/`) |
