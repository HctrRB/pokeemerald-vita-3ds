# Native UI (Emerald 3DS style)

The port has two interfaces, chosen with the option **NEW UI** (OPTIONS > MODES):

- **The new UI** (`src/platform/ui/`), described in the next section: screens, dialogue boxes and
  the battle HUD drawn over the 3D world, with the look of Emerald 3DS.
- **The original Vita UI** (`src/platform/vita/vita_ui*.c`), described after it: a canvas modelled
  on Emerald 3DS's bottom screen. It is used when NEW UI is off, and still for the Battle Pyramid
  bag and the PC.

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

## Original Vita UI

The START menu and most of the game's own menus are replaced by a native interface modelled on
Emerald 3DS's bottom screen, drawn on the Vita's single screen with touch and buttons.

### Lifecycle

```
START (field) ──► VitaUI_Open(VITA_UI_SEC_MAP)      game paused: AgbMain stops being stepped
every frame   ──► VitaUI_Update(buttons)            input, touch, navigation, actions
              ──► ShowNativeUi()                    VitaUI_RenderDirty → glTexSubImage2D of dirty rows
START / B     ──► VitaUI_Close()                    game resumes
```

- Opened from `sdl2.c` when `gMain.callback2 == CB2_Overworld` (START, or the remappable
  `VITA_ACTION_MENU` button for Options). It is also opened by the game for the bag and party in
  battle (`PlayerHandleChooseItem`, `PlayerHandleChoosePokemon`), for selling in shops
  (`VitaUI_OpenShopSell`), for the Fly map (`VitaUI_OpenFlyMap`) and from the main menu
  (`VitaUI_OpenStandaloneOptions`).
- While open, the main loop still ticks audio and VBlank (`TickMenuAudioAndVBlank`) so music
  keeps playing.
- Canvas: `VITA_UI_WIDTH`×`VITA_UI_HEIGHT` = 320×240 RGB565, drawn on the CPU. Only rows that
  changed are uploaded; the GPU scales it to 640×480 or 725×544 (`uiScale` setting).
- `VitaUI_Prewarm()` at boot decodes the Hoenn map, trainer pictures, type art, dock icons and
  plates so the first opening does not stall.

## Sections (`enum VitaUiSection`, `include/platform/vita_ui.h`)

| Section | File | Content |
|---|---|---|
| `MAP` | `vita_ui_map.c` | Hoenn map with cursor, zone encounters (day/night, surf, fishing, rock smash), Fly destinations |
| `POKEDEX` | `vita_ui_pokedex.c` | 386 entries, two panels, cries, footprints, search |
| `POKEMON` | `vita_ui_party.c` | Party, actions popup, summary with stats/IVs/EVs, moves |
| `BAG` | `vita_ui_bag.c` | Pockets, item use, registered item |
| `POKENAV` | `vita_ui_pokenav.c` | Condition (radar, sheen, markings), Match Call, Ribbons |
| `CARD` | `vita_ui.c` | Trainer card with badges and flip |
| `SAVE` | `vita_ui.c` | Save from the dock |
| `OPTIONS` | `vita_ui_options.c` | Five categories of options and QoL extras, button remapper |
| `NUZLOCKE` | `vita_ui.c` (rules in `vita_nuzlocke.c`) | Nuzlocke tracker, shown when the extra is on |

Each section is enabled by `IsSectionAvailable` (`vita_ui.c:246`): for example the Pokédex
needs `FLAG_SYS_POKEDEX_GET`, and some sections are hidden in the Battle Pike, Pyramid or
Safari Zone.

## Drawing helpers

| File | Role |
|---|---|
| `vita_ui_plate.c/h` | Bevelled "glass" plates rasterised from signed-distance shapes, cached |
| `vita_ui_art.c/h` | Text (GBA font to the canvas), sprites, icons, Pokémon pictures, colour conversion |
| `vita_ui_internal.h` | Shared theme colours and helpers between the section files |
| `3ds_bottom_art.h`, `hoenn_map_baked.h` | Generated art tables |
| `assets/ui/menu.bin`, `battle.bin` | Procedural backgrounds and battle art (see [ASSETS.md](ASSETS.md)) |

## In-battle and in-world UI

| File | Role |
|---|---|
| `vita_battle_ui.c/h` | Touch action deck: FIGHT, moves with type/category/PP/effectiveness, quick ball, speed, targets, level-up panel |
| `vita_world_ui.c` | HD dialogue plate and signs in the field |
| `vita_shop_ui.c` | Shop header (money) over the GBA buy menu |
| `vita_corner_art.c` | Procedural art for the casino and contests |

## Game features behind the UI

| File | Role |
|---|---|
| `vita_extras.c/h` | QoL switches stored in `settings.txt`; `VitaExtras_GetFast(FAST_EXTRA_*)` is an O(1) cache read by game code |
| `vita_daynight.c/h` | Accelerated clock (1×–1000×) and day/night light |
| `vita_encounters.c/h` | Time-of-day wild encounter tables for 20 routes |
| `vita_nuzlocke.c` | Nuzlocke rules and tracking |
| `src/follower_mon.c` | HGSS follower Pokémon (`assets/followers/`) |

## Adding a section

1. Add the value to `enum VitaUiSection` before `VITA_UI_SEC_COUNT`.
2. Create `vita_ui_<name>.c` with an update function (input → state) and a draw function
   (state → canvas), using the plate and art helpers.
3. Dispatch both from `VitaUI_Update` / `VitaUI_Render` in `vita_ui.c` and add the dock icon.
4. Call `VitaUI_Invalidate()` when the state changes outside the update function so the canvas is redrawn.
