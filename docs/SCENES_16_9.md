# GBA screens at 16:9

The Vita screen is 960×544. A GBA frame (240×160) scaled to the full height (×3.4) is 816 px
wide, so 72 screen pixels — about **21 GBA pixels** — are missing on each side. This document
describes how the port fills them today and the planned replacement.

## Current system

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
| Field (`overworld.c:1517`) | `Widescreen_SetFieldFrame` | BG1-3 `WIDE` from a 64-tile ring kept by `field_camera.c`; sprites extended |
| Battle (`battle_main.c`) | `Widescreen_SetBattleFrame` | BG1-3 `WIDE`; BG0 text box rebuilt stretched to the edges; sprites extended |
| Stage scenes: intro, title, credits, cable car, legendary cutscenes | Forced include `compat/ctr_gba_stage.h` (CMake) | Per layer, every frame (`PrepareStageFrame`, after Emerald 3DS `RecordScroll`): a layer panning horizontally (more than 4 px over 32 frames) or scrolled line by line by a scanline effect → `WIDE`; a still panorama behind scrolling scenery → `WIDE`; any other still layer → `WIDE_BG_EDGE_TILE`. Sprites extended. All-still frames get a GPU fade to black; the leaves scene draws its continuation art (`VitaStage_RenderLeavesMargins`) instead of repeated edges |
| Everything else | — | Back layer `FILL`, other layers and sprites centred |

Widescreen is always on (`Widescreen_IsOn()` returns `TRUE`); the GBA options screen's DISPLAY
page keeps only VOLUME. Every path shows the 288-px line with one mapping:
`WIDESCREEN_VISIBLE_U0..U1` (columns 2.8–285.2) at a uniform ×3.4, so the GBA's 240 px sit at
screen x 72–888.

**`vita_stage.c` (GPU, from Emerald 3DS).** Records the VBlank callback of stage scenes
(`VitaStage_IsStageActive`) and draws the pre-made leaf art (`assets/ui/intro_margins.bin`) beside
the intro's leaves scene.

## Known problems

- Title validated on hardware. The Game Freak / leaves scene uses the Emerald 3DS continuation
  art and can still be improved. Bike, legendary cutscenes and credits not yet checked.
- On the title screen the Rayquaza layer keeps its own `EDGES` policy; check it on hardware.
- Field-move banners are still stretched from 240 to 960 px (18 % distortion).
- Centred menus still use the per-frame most-common-tile `FILL`.

## Planned replacement: Wide Scene

Approved design. Summary:

1. **One mapping**: every path shows columns 2.8–285.2 of the 288-px buffer at a uniform ×3.4.
2. **Scene classification as in Emerald 3DS**: game files are compiled with a forced include that
   renames `SetVBlankCallback`, so the platform knows the scene family (stage, centred, battle,
   transition) without editing the scene code.
3. **Per-layer stage policy decided every frame**: scrolling or panoramic layers wrap; still
   layers repeat their edge *tile* and fade to black on the GPU.
4. **Centred screens** use a per-screen fill table (after Emerald 3DS `sCentredFills`) instead of
   searching for the most common tile each frame.
5. **Widescreen always on**; the option and the unused border-art code are removed.
6. **Cheaper pass 6**: only 240 px drawn and uploaded for centred overlays, and the layer mask
   passed to the rasterizer instead of written to `REG_DISPCNT`.

`widescreen.c` and `vita_stage.c` are merged into `vita/wide_scene.c/h`.
