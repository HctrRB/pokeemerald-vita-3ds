# GBA screens at 16:9

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

The Vita screen is 960×544. A GBA frame (240×160) scaled to the full height (×3.4) is 816 px
wide, so 72 screen pixels — about **21 GBA pixels** — are missing on each side. This document
describes how the port fills them.

## How the margins are filled

Two independent pieces:

**`widescreen.c` (CPU, from pokeemerald-multiplatform).** Every GBA frame is drawn into lines of
288 px (24 extra on each side). At the start of each `DrawFrame`, `Widescreen_PrepareDraw()`
fills `gFastDrawWide` with a policy per background layer:

| Policy | Margin content |
|---|---|
| `WIDE_BG_CENTRED` | Nothing |
| `WIDE_BG_WIDE` | The layer's real map continues |
| `WIDE_BG_EDGES` | The screen-edge pixel column repeated |
| `WIDE_BG_AUTO` | `WIDE` where the map has art past the edge, else `EDGES` |
| `WIDE_BG_FILL` | The layer's most common visible tile |

Scenes that declare a policy (each frame, while their `gMain.callback2` is current):

| Scene | Call | Policy |
|---|---|---|
| Field (`overworld.c`) | `Widescreen_SetFieldFrame` | BG1-3 `WIDE` from a 64-tile ring kept by `field_camera.c`; sprites extended |
| Battle (`battle_main.c`) | `Widescreen_SetBattleFrame` | BG1-3 `WIDE`; BG0 text box rebuilt stretched to the edges; sprites extended |
| Stage scenes: intro, title, credits, cable car, legendary cutscenes | Forced include `compat/ctr_gba_stage.h` (CMake) | Per layer, every frame (`PrepareStageFrame`, after Emerald 3DS `RecordScroll`): a layer panning horizontally (more than 4 px over 32 frames) or scrolled line by line by a scanline effect → `WIDE`; a still panorama behind scrolling scenery → `WIDE`; any other still layer → `WIDE_BG_EDGE_TILE`. Sprites extended. All-still frames get a GPU fade to black; the leaves scene draws its continuation art (`VitaStage_RenderLeavesMargins`) instead of repeated edges |
| GBA-format scenes (below) | `GameFrameOverlay.gbaFormat` | No widescreen: centred lines, the 816×544 quad, black side bars |
| Everything else | — | Back layer `FILL`, other layers and sprites centred |

Widescreen is on (`Widescreen_IsOn()` returns `TRUE`) except for the GBA-format scenes; the GBA
options screen's DISPLAY page keeps only VOLUME. Every widescreen frame shows the 288-px line with
one mapping: `WIDESCREEN_VISIBLE_U0..U1` (columns 2.8–285.2) at a uniform ×3.4, so the GBA's 240 px
sit at screen x 72–888.

**GBA-format scenes.** The full-screen GBA mini-games and screens are shown in the GBA's own
format, 816×544 with black side bars (decided 2026-11-19): Berry Blender, Berry Crush, Pokémon
Jump, Dodrio Berry Picking, slot machine, roulette, the contest, its results and painting, the
Pokéblock eating scene, the Battle Dome tree, the Battle Factory screens, the berry tag, the Union
Room chat, the system screens (save failed, clock reset, clear save, berry fix), and the other
full-screen GBA screens: the title's main menu and Birch's speech, the Hall of Fame (also from the
PC), the diploma, the wall clock, the starter choice, Mystery Gift, Mystery Event and the e-Reader,
the GBA trainer card of the link rooms and the link trade menu. Each scene's
file answers for its own callbacks (`*_IsGbaFormatForUi`); Core 1 publishes the result in the game
frame (`GameFrameOverlay.gbaFormat`, `vita_game_frame.c`), and Core 0 turns widescreen off for that
frame in `Widescreen_PrepareDraw`, so `RenderGameTexture` draws the centred quad over a black clear.
The flag is latched (`GbaFormatLatched`): it stays on from the scene's start until a 16:9 context is
back (field, battle, stage scenes, evolution / egg / trade), so loading and closing steps between
them never switch the format for a frame.
Stage scenes, evolution, egg hatch, trade, the field and battles keep 16:9.

**`vita_stage.c` (GPU, from Emerald 3DS).** Records the VBlank callback of stage scenes; Core 1
puts whether a stage (and the leaves scene) is on screen in the game frame
(`VitaStage_CaptureFrame`), and Core 0 reads it (`VitaStage_IsStageActive`) to draw the pre-made leaf art (`assets/ui/intro_margins.bin`) beside
the intro's leaves scene.

## Known problems

- Title validated on hardware. The Game Freak / leaves scene uses the Emerald 3DS continuation
  art and can still be improved. Bike, legendary cutscenes and credits not yet checked.
- On the title screen the Rayquaza layer keeps its own `EDGES` policy; check it on hardware.
- Field-move banners are still stretched from 240 to 960 px (18 % distortion).

## Pass 6 (the GBA layer over the 3D world)

Centred overlays draw and upload only the GBA's 240 px (`gFastDrawCentredOnly`), and the layers the
overlay hides are masked in each line's captured registers (`gFastDrawDispcntClear`), never in the
live `REG_DISPCNT`. See [RENDERING.md](RENDERING.md).

## Not done

- Centred screens still pick their back layer's fill by searching for the most common visible tile
  every frame (`MostCommonMapEntry`). A per-screen fill table, as Emerald 3DS has, would replace it.
- `widescreen.c` and `vita_stage.c` are still two files.
