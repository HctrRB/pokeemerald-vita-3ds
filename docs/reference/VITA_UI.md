# User interface

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

The port's menus, dialogue boxes, battle HUD and the scenes it replaces are the new UI
(`src/platform/ui/`), drawn natively over the 3D world, with six menu themes (GBA by default). It is the only
menu system: the old 3DS-style Vita menu (`vita_ui*.c`) was removed on 2026-11-13. Where the new UI
has no screen, or cannot open one, the game's own GBA screen is used:

- START in link and Union rooms and the Battle Pike opens the game's START menu
  (`Ui_OwnsFieldStart`): its entries there (link cards...) are not the hub's. In the Battle Pyramid
  the hub is the START menu, with the Pyramid's entries (see "Battle Pyramid").
- Any battle bag or party screen the UI cannot open falls back to the game's
  (`battle_controller_player.c`).
- The link half-party choice (Cable Club, Union Room) stays the game's.
- Decoration shops (`MART_TYPE_DECOR` / `MART_TYPE_DECOR2`) are BUY-only in the new UI, as in the game.
- The title's main menu (CONTINUE, NEW GAME, OPTION, MYSTERY GIFT, `main_menu.c`) stays the
  game's, with the start-up scenes; its OPTION opens the new options screen hosted
  (`Ui_OpenOptionsHosted`).
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
| `ui_scr_*.c` | Screens: START hub, Pokédex, party, bag, PokéNav, trainer card, options, Nuzlocke, mart, PC boxes, player's PC (items, mailbox, decorations), naming keyboard, Easy Chat, berry choice, forget a move, choose a Pokémon, Pokéblock case (`ui_scr_pokeblock.c`) |
| `ui_mirror.c`, `ui_sign.c` | Dialogue, YES/NO, multichoice, the field's list menus (`ListMenu_GetFieldListForUi`: scrollable multichoice, shop and frontier lists) and the info boxes (`sInfoBoxes`: money, coins) (including 2D evolution, egg-hatch and trade dialogue mirrors), plus zone signs |
| `ui_battle_hud.c`, `ui_game_battle.c` | Battle HUD (plates, party balls, messages, commands, moves, targets, level up) and the battle bag / party |
| `ui_game*.c` | Game data and actions used by the screens |
| `ui_stall.c` | Diagnostic: writes `ui_stall.log` when the UI stops updating for more than 100 ms |

### Opening modes (`ui_core.c`)

- **Field overlay (`Open`)**: opened from the overworld with START / Square (`LockPlayerFieldControls`).
- **Script-driven (`UiCore_OpenFromScript`)**: opened by field script tasks (Poké Mart and decoration shops, PC boxes, player's PC, the "choose a POKéMON" kinds of `ui_scr_choose.c` — tutor, Day Care, generic, contest entry, Move Relearner, the Battle Pyramid's held items, the Battle Frontier's half party — each a row of its `sKinds` table, the Battle Pyramid lobby's full bag, `Ui_OpenPyramidBagToss`, and Mr. Stone's PokéNav tutorial, `Ui_OpenPokenavTutorial` from `OpenPokenavForTutorial`, on MATCH CALL until a call is made); the game task waits for `!Ui_IsOpen()`.
- **Hosted full-screen (`UiCore_OpenHosted` / `CB2_UiHost`)**: replaces a GBA scene run from `gMain.callback2` (naming screen, Easy Chat — every type; the rules of the Bard's song and the Quiz Lady's screens come from `easy_chat.c`'s `EasyChat_Ui*` —, the wall maps of Hoenn, `Ui_OpenTownMap` from `FieldShowRegionMap`, as the PokéNav's map only, Berry Blender / Berry Crush choice, title-menu options, the Pokéblock case of the Safari feeder and the Contest Lady, forget a move — also with no new move for the Move Deleter —, the Move Relearner, and the bag's CHOOSE mode for the bags scripts open, `Ui_OpenBagChoice` from `GoToBagMenu`, whose rules stay in `item_menu.c`'s `ItemMenu_*ForUi`) while keeping GBA sprites and tasks alive and running scene resets before opening.
- **To a GBA scene and back (`UiCore_CloseToScene`, `UiCore_CloseToSceneInWindow`, `UiCore_ReopenAfterField`)**: screens on the field hand over to a GBA scene (the Pokéblock eating scene, the berry tag of the bag's CHECK TAG, the recorded battle of the trainer card's RECORD page); the whole stack of screens is kept and opens again as it was when the field comes back (each screen's `resume` refreshes its data). In a window (feeding from the PokéNav CONDITION tab) the scene is drawn in `UI_SCENE_WIN_*` (`GameFrameOverlay.gbaWindow`, `RenderGameTexture`) inside the theme's frame.
- **Battle menu (`UiGame_OpenBattleMenu`)**: opened over the battle (bag — the PYRAMID BAG in the Battle Pyramid —, party, the Safari battle's Pokéblock case, and the Pokédex page after a catch, `Ui_OpenCaughtDexPage` from `Cmd_displaydexinfo`); the battle waits for `!Ui_IsOpen()`.

### Themes (`ui_theme.c`)

Each theme also names the GBA text window frame (`UiTheme.gbaWindowFrame`) that the full-screen GBA
scenes use with it: `text_window.c` reads `TextWindow_PlayerFrameType`, which returns
`Ui_GbaWindowFrame` while the UI is enabled, so the save's OPTIONS frame is never rewritten.

In the GBA theme every screen is a GBA page. The screens with their own GBA layout draw it
(`UiW_GbaTopBar`, `UiW_GbaHelpBadge`...); the others get it from the kit: `UiKit_HeaderAs` draws the
GBA top bar with the screen's section (backdrop and icon), its tab in the pager (L / R on the
pager take the neighbouring tabs through `UiKit_TabHit`; one tab is the page's title) and turns
the footer guides and the header value into the bar's help badges (`UiW_GbaPageStart`,
`UiW_GbaPageFlush`, reset and flushed by the core around each build). Badges and value always fit
the bar: their labels shrink, then only the keys show; titles that do not fit in PS2P use VT323.

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

### Pokéblock case (`ui_scr_pokeblock.c`)

Replaces `pokeblock.c`'s screen in its four modes (`PBLOCK_CASE_*`), laid out as the bag: from the
bag's USE (pushed over it) or SELECT (`Ui_OpenPokeblockCaseField`), in a Safari battle
(`Ui_OpenPokeblockCaseBattle`, a battle menu), and for the Safari feeder and the Contest Lady
(`Ui_OpenPokeblockCase`, hosted, from `OpenPokeblockCase`). Every action calls `pokeblock.c`'s
`PokeblockCase_*ForUi`, the GBA case's own code, so both leave the same game state. Sprites come
from the game's graphics at run time (`UiSpr_Pokeblock` with each colour's palette,
`UiSpr_PokeblockCase`). USE on the field opens the FEED step, drawn with the PokéNav CONDITION
screen's pieces (`UiCond_*` in `ui_scr_nav.c`) plus what the block would add
(`UiGame_PokeblockGains`); YES closes the UI and plays the GBA eating scene (`pokeblock_feed.c`),
then the field comes back and the FEED step reopens with the result (`CB2_AfterFeedScene`).

### Box summary (`ui_scr_pc.c`, `ui_scr_party.c`)

The PC's SUMMARY pushes the party screen as the summary only (`UiParty_PushSummary`): ↑ / ↓ go
through the POKéMON, ○ goes back to the boxes on the one looked at last (`UiParty_LastViewed`). The
summary readers of `ui_game.c` (`UiGame_Mon`, `UiGame_Summary`, `UiGame_Move`,
`UiGame_MonStatusIcon`, `UiGame_SwapMoves`) read a view: the party, or the current box's POKéMON
(`UiGame_ViewBox`, copies made with `BoxMonToMon` as the GBA summary of a box, empty slots left
out); a move reorder is written back to the box. The UI's close and reset return the view to the
party (`UiGame_ViewParty`).

### Easy Chat (`ui_scr_easychat.c`)

Every type of `easy_chat.c`, laid out from the type's columns and rows (a 2x5 message, the mail and
the quiz question, shows 9 words, as `PrintCurrentPhrase`) with the buttons in a table. The rules of
each type are the game's (`EasyChat_Ui*`): the Bard's song deletes no word and asks at once whether
the song is right; the Quiz Lady's screens get a fourth button to the other screen of the pair
(`EasyChat_UiQuizSwitch`), and the quiz question being answered can only be read.

### Frontier Pass pages (`ui_scr_card.c`)

With `FLAG_SYS_FRONTIER_PASS` the trainer card adds the pass's MAP (the map composed from the
pass's own graphics, `UiSpr_FrontierMap`, each facility marked and described from
`FrontierPass_LandmarkForUi`) and RECORD (the recorded battle, played by
`FrontierPass_PlayRecordForUi` through `UiCore_CloseToScene`; the card opens again on that page).

### Battle Pyramid (`ui_scr_hub.c`, `ui_scr_bag.c`, `ui_scr_choose.c`)

The hub is the START menu on the Pyramid's floors with the game's entries (`BuildBattlePyramidStartMenu`):
POKéMON, BAG, the trainer card, REST (the save, then `SoftResetInBattlePyramid` once it is safe),
RETIRE (`gText_BattlePyramidConfirmRetire`, then the `BattlePyramid_Retire` script) and OPTION; the
floor (`StartMenu_PyramidFloorForUi`) takes the REPEL capsule. The bag there is the PYRAMID BAG
(`UiGame_PocketItems`, `UiGame_BagRemove`), in the field and in battle. Leaving with a full bag, the
lobby's script opens the PYRAMID BAG with TOSS only (`UiGame_UsePyramidBag` off the floors) and the
party with TAKE ITEM / TOSS for the held items (party_menu.c's `PartyMenu_TakeHeldItemForUi` and the
toss pair).

### Battle Frontier half party (`ui_scr_choose.c`)

`ChoosePartyForBattleFrontier` (the Frontier facilities and the Battle Tents) opens the choose screen
in its half-party kind: choosing an able POKéMON enters it or takes it out of the order, CONFIRM (or
START) runs the game's checks and ○ asks the facility's cancel question. Everything goes through
party_menu.c (`PartyMenu_HalfParty*ForUi`, `gSelectedOrderFromParty`), so VAR_RESULT and the order
are what the GBA screen leaves.

## Game features behind the UI

| File | Role |
|---|---|
| `vita_extras.c/h` | QoL switches stored in `settings.txt`; `VitaExtras_GetFast(FAST_EXTRA_*)` is an O(1) cache read by game code |
| `vita_daynight.c/h` | Accelerated clock (1×–1000×) and day/night light |
| `vita_encounters.c/h` | Time-of-day wild encounter tables for 20 routes |
| `vita_nuzlocke.c` | Nuzlocke rules and tracking |
| `src/follower_mon.c` | HGSS follower Pokémon (`assets/followers/`) |
