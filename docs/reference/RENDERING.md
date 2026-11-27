# Rendering

Type: reference · Audience: developers · Status: current
Verified against: `55868979`

Everything on screen goes through vitaGL (OpenGL ES 1.x style fixed pipeline over SceGxm) at
960×544. There are two presentation paths, chosen every frame in `sdl2.c` (see
[ARCHITECTURE.md](ARCHITECTURE.md)); on both, the new UI is drawn last, on top.

| Path | When | Entry point |
|---|---|---|
| 3D | Field callbacks, shop buy menu, 3D battle | `VoxelRenderer_RenderFrame()` (`voxel/voxel_renderer.c`) |
| 2D | Every other GBA screen | `VDraw()` + `RenderGameTexture()` (`sdl2.c`) |
| New UI (over either) | Its draw list is not empty | `Ui_Render()` (`ui/ui_render.c`), called at the end of both paths |

## 2D path: the software GBA PPU

`gba_fast_draw.c` is a scanline renderer of the GBA's video hardware: text and affine
backgrounds, sprites, windows, blending, mosaic, per-line scroll effects.

- `DrawFrame()` runs each line's hardware events in order on the calling thread and captures
  the registers and palette that line sees; two workers on Core 2 draw the captured lines in
  parallel (`gba_fast_draw_b.c` and `gba_fast_draw_c.c` recompile the renderer so each has its own state).
- Lines are **288 px** wide (`FRAME_LINE_WIDTH`), with the GBA screen at x = 24. What goes into
  the margins is decided per scene by `widescreen.c`; see [SCENES_16_9.md](SCENES_16_9.md).
- Workers convert BGR555 to RGB565 with NEON. `VDraw` uploads the frame into a ring of
  `VITA_GAME_TEXTURE_RING` textures so the GPU never reads a texture being written.
- `RenderGameTexture` draws one quad: 288 px stretched to 960 with widescreen on, or the
  middle 240 px at 816×544 over a black clear with it off (the GBA-format scenes,
  [SCENES_16_9.md](SCENES_16_9.md)). The intro's leaves scene draws GPU margin
  art beside it (`vita_stage.c`). Filter: nearest or bilinear (`filter` setting). The day/night
  tint is applied as the quad colour.
- **Extended sprite palettes (`gPlttExt`)**: 16 port-only 4bpp overworld sprite palettes
  (`palette.h`, `sprite.c`) live after the 32 GBA palettes (`gPlttBufferUnfaded/Faded[PLTT_BUFFER_ALLOC]`)
  so the follower, its Poké Ball and up to 6 visible wild Pokémon keep their HGSS colours without
  competing for GBA OBJ slots 12–15. Fades and weather mirror `PALSLOT_NPC_1`. `DrawFrame()`
  latches `gPlttExt`, `gOamExt` and `gPlttExtInUse` once per frame (zero cost when `gPlttExtInUse == 0`),
  and `CtrVideo_ExtPalette()` feeds `voxel_entities.c` in 3D.

## 3D path: `VoxelRenderer_RenderFrame`

Per frame (`VoxelRenderer_RenderFrame`):

1. **Camera**: follow camera with sub-tile interpolation (`voxel_camera.c`), battle camera in
   3D battles.
2. **Atlas**: metatile atlas pages composed on demand in `VOXEL_ATLAS_SLOTS` LRU slots (`voxel_atlas.c`);
   animated tiles (water, flowers, shores) recomposed on the CPU when the game animates them and
   written straight into the page texture via `WriteAtlasRowsDirect` (see "Textures rewritten at
   run time" below), keeping the active map and visible neighbours in sync on the exact same frame.
3. **Lighting**: sun position from the clock (`voxel_lighting.c`); chunks whose lighting hash
   changed are rebuilt progressively.
4. **Culling** (`ComputeViewAndFrustum`, `Frame_SelectChunks`): the frustum's corner rays are
   projected onto the ground, clamped to `VOXEL_VIEW_REACH_MAX` around the camera target and
   widened by `VOXEL_VIEW_SLACK_TILES`; a chunk is a candidate when it overlaps that area, then
   must pass the frustum test (`Frustum_ChunkInView`). Near and far planes follow the camera each
   frame (`VOXEL_VIEW_NEAR_*`, `VOXEL_VIEW_FAR_*`), so high aerial poses keep their depth precision. A building is emitted only by the chunk
   holding its north-west cell, so each chunk records how far its building models reach past its
   own cells and how tall they are (`bldWest/East/North/South/Top`, measured from the vertices
   on the worker); both tests use that extent, and chunks up to 3 west and 4 north of the window
   are kept for their models. Those extra chunks are drawn only when already built: they are
   never built or pre-warmed from there. Without this, wide buildings (Rustboro's Devon Corp.)
   vanished at the screen's west edge while still in view.
5. **Chunk builds**: in three phases (`PrepareChunkJob`, `ComputeChunkJob`, `CommitChunkJob`).
   Visible holes, and chunks drawn with an outdated atlas, are built on Core 0 in the frame:
   `VOXEL_HOLE_BUILDS_PER_FRAME` whatever the budget, more if they fit it (`ChunkBuildFits`: the
   estimated build cost plus `VOXEL_BUILD_TAIL_MS` must stay inside the frame time).
   Every other build goes to the chunk worker on Core 2 in batches (`ChunkWorker_ChooseBatch`):
   first chunks the view will reach within `VOXEL_VIEW_AHEAD_TILES` of movement
   (`Frustum_ChunkInViewAhead`), then the dated chunks (sun moved, tile changed) nearest to the
   player, then the unbuilt chunks nearest to a point `VOXEL_PREFETCH_LOOKAHEAD` tiles ahead of the
   movement. A batch is launched after the swap and committed at the start of
   the first frame that finds it done; a frame that starts while it still runs does not wait
   for it (see THREADS.md). While an atlas slot is being extended after a crossing, nothing is
   handed to the worker: each of those frames waits for it before touching the atlas. Without
   the worker those builds run on Core 0 when the frame has room. Warm-up and faded frames
   build on Core 0 without limit. Chunks drawn this frame or the previous one are never evicted.
6. **Passes**, each batched into few draw calls:

| Pass | Content | Notes |
|---|---|---|
| 1A / 1B | Ground and relief, then cliff cutouts | Opaque first for early-Z on the SGX543; cutouts with `GL_ALPHA_TEST` |
| 2 | Trees, tall grass, ash grass & flowers | Tree atlas (`sTreeTex`) partitioned into static opaque tree cores/ground (`2A-0`, `GL_ALPHA_TEST` off for SGX543 HSR), static alpha-tested border strips (`2A-1`), and GLSL wind-sway & player-bend foliage (`2B`, standing tall grass, ash grass and animated GBA flower frames; all trees stay static) |
| 3 | Buildings and indoor furniture | Building pages in an LRU cache, alpha test only on pages that need it |
| 4 | Cast and decal shadows | One colour (`0xFF281408`) |
| 4.5 | Reflections on water | |
| 5 | Entity billboards, player silhouette behind roofs | `glDepthFunc(GL_GREATER)` for the silhouette |
| 5.1 | Day/night modulation | Skipped when it would be a no-op |
| 5.2 | Night lamps: pools, glass, halos | Built with each chunk on Core 2 (`voxel_lamps.c`, `EmitChunkLamps`): static glass and pool spans in the chunk VBO plus each light's centre in the slot. Drawn from the VBOs with the frame's `nightGlow` as a constant colour (two draws per lamp chunk, one for all halos). The same lights tint the characters (`UpdatePointLights`, `LightCards`): their lit cards are drawn again after the night veil, additive (`Pass_EntityLamps`), since a card's own colour is already near white. Cheaper at lower HD-2D levels (`kLampLevels`) |
| 5.4–5.8 | Fog / sand banks, dust motes / fireflies, surface FX & weather particles | Fog/sand banks conform to a cached world-snapped terrain grid (`SmoothedFogWorldY`), motes/fireflies and surface FX (`DrawSurfaceFx`) anchor to `SurfaceWorldY(wx, wz)` |
| 5.9 | 2D atmosphere: sun rays, moonlight, fades, lightning, cave gloom | With the HD-2D post pass the moonlight / fade layer is applied by its downsample and composite instead of a full-screen blended quad (`OverlayInPost`) |
| DoF / Bloom | HD-2D post-processing (DoF, Kawase Bloom, Composite) | Scene rendered to a 960×544 RGBA8 FBO, downscaled to 1/4 (240×136) for `smoothstep` tilt-shift DoF + Kawase bloom (`voxel_hd2d_shader.c`; HIGH blurs it in one 8-tap pass, `VoxelHd2dShader_Kawase8`), split-tone & vignette. Runs with OPTIONS > GRAPHICS > HD-2D at MED or HIGH (bloom, sky bloom, shafts, grade) or with DEPTH OF FIELD on (bands only); off during fades. LOW keeps the world effects (relief, sway, particles) without this pass |
| 6 | GBA overlay | See below |

In the code each step is a `Frame_*` function (`Frame_FollowCamera`, `Frame_UpdateAtlas`,
`Frame_AnimateTiles`, `Frame_TrackWorldChanges`, `Frame_ReadAtmosphere`, `Frame_SelectChunks`,
`Frame_EmitEntities`) and each pass a `Pass_*` function (`Pass_Ground` … `Pass_Overlay2D`).
They share the frame state through a `VoxelFrame`; `VoxelRenderer_RenderFrame` only calls them
in order. The builds left after the visibility loop go through a `VoxelBuildQueue`
(`BuildQueue_SendToWorker`, `BuildQueue_BuildStale`, `BuildQueue_Prewarm`).

## The day/night cycle

One schedule drives every day/night effect: `VITA_DAYNIGHT_DAWN_MIN`, `VITA_DAYNIGHT_SUNRISE_MIN`,
`VITA_DAYNIGHT_MORNING_MIN`, `VITA_DAYNIGHT_NOON_START_MIN`, `VITA_DAYNIGHT_NOON_END_MIN`,
`VITA_DAYNIGHT_DUSK_MIN`, `VITA_DAYNIGHT_SUNSET_MIN` and `VITA_DAYNIGHT_NIGHT_MIN`
(`include/platform/vita_daynight.h`). Once per presented frame, `VitaDayNight_UpdateFrame` (Core 0;
`Frame_Begin` in 3D, `RenderGameTexture` in 2D) reads the clock once and builds a
`VitaDayNightState`: the tint, daylight and glow (`VitaDayNight_At`, keyed on the schedule), the
night layer (`moonlight`), the shadow sun (`sun`, in 15-minute steps, since each step relights the
visible chunks), the sun and moon on screen (`sunPath`, `moonPath`), the moon shafts' visibility and
the HD-2D grade's day / dusk / night weights. The sun follows one arc (`SunAngle`) for the shadows and
the screen: up at sunrise, flat at its highest between the noon marks, down at nightfall, when the
moon rises; the moon sets at sunrise. Windows, street lamps, lit characters and fireflies follow the
glow; the shafts, sky bloom and motes the daylight and the paths. No pass works out the time on its
own. With the option off or indoors the state is a fixed early afternoon. The wild encounter phases
(`VitaEncounters_Phase`) use the same marks: morning from sunrise to the noon start, day until
nightfall, night until sunrise.

The animation clock (`VoxelFrame::clock`, seconds, frozen while a menu holds the HD-2D backdrop) is
a double. `Frame_DeriveClock` turns it, once per frame, into what the passes read: the foliage
sway phases (`uSwayA`, `uSwayB`) and the shafts' wrapped shifts and phases
(`VoxelHd2dShader_ShaftClockAt`). Every shader runs in half precision, so no shader gets the raw
clock. Fog, weather and Fly wisp offsets wrap their clock products in double; slow terms (motes,
glints, lamps) read the float copy `seconds`.

## Pass 6: the GBA overlay over 3D

Field dialogues, YES/NO, multichoice, the money box, zone signs, the menus and the battle HUD
are drawn by the new UI (`src/platform/ui/`). Pass 6 is skipped on the field whenever
`Ui_MirrorCovers()` is true, and runs only for unmirrored GBA windows, field-move / HM banners,
and 3D battles (where it draws the Pokémon and trainer sprites, move animations and weather while
`Ui_BattleDispcntClear()` and `gUiHideBattleHudSprites` hide the GBA HUD). When
`VoxelRenderer_HasActiveUi()` sees an active GBA layer:

1. The map layers are hidden through `gFastDrawDispcntClear` (field: BG1-3; shop: BG2-3; battle:
   BG3), applied to each line's captured registers; the live registers are never written.
   Outside battles the overlay is centred, so `gFastDrawCentredOnly` limits drawing to the GBA's
   240 px.
2. `DrawFrame()` renders the GBA frame with the backdrop set to the chroma key `0x7C1F`
   (`gFastDrawBackdropOverride`).
3. The frame is converted to RGBA4444: chroma key → transparent, semi-transparent weather keeps
   the GBA's blend alpha.
4. It is uploaded (two textures used in turn, so vitaGL never copies the one drawn last frame;
   the conversion skips pairs of transparent pixels) and drawn as one quad: the full 288-px line in 3D battles (uniform ×3.4
   mapping), only the 240 px columns otherwise (field text at 816×544, field-move banners
   stretched to 960).

While visible it costs ~5 ms of scanlines and ~2 ms of conversion and upload per frame, and up
to ~13 + 5 ms on large screens (logs/sesion 27); it is most of the slow frames left in 3D. The
scanlines are drawn by `DrawWorker1/2` on Core 2, which they now share with the chunk worker.
When no window is open the pass is skipped entirely. With the new UI's battle HUD, BG0 (and BG1
while the level-up box shows) and the healthbox sprites are left out: the HUD is drawn by the new
UI (`Ui_BattleDispcntClear`, `gUiHideBattleHudSprites`). The new UI replaces the GBA menus, so
this pass is not being optimised.

## GPU rules learned on hardware

- Draw opaque geometry with alpha test and blending off so the SGX543 can reject hidden pixels
  early.
- Avoid fullscreen blended quads; bake fog and tint into vertex colours where possible.
- Chunk geometry lives in one GL buffer per chunk slot, filled once with `glBufferData(NULL)` +
  `glBufferSubData` when the chunk is built (`UploadChunkVbo`). Per-frame dynamic
  geometry (weather, billboards, overlays) streams from client arrays (`sBatchGlVerts`).
  An earlier path that re-uploaded a vitaGL buffer every frame crashed the GPU.
- Never upload into a texture the GPU drew in the last frames through `glTexSubImage2D`: vitaGL
  copies all of it first. Use rings or spares, or write in place on purpose (see below).
- GLSL: work out texture coordinates in the vertex shader and sample the varyings as they are
  (`voxel_hd2d_shader.c`); a coordinate computed in the fragment shader makes the read dependent.
  Loops get constant bounds (one program per variant), and values that only depend on uniforms are
  computed on the CPU.
- A procedural effect that only varies along one screen axis is evaluated once per sample of that
  axis into a small 1D target, then read from a full-screen quad: the light shafts
  (`VoxelHd2dShader_ShaftLut`, `VoxelHd2dShader_Shafts`) depend only on the distance across the
  beams, so 512 evaluations per pass replace one per pixel of the bloom target. The profile's
  target is a full tile row high (a 1-pixel-high target drew nothing), but the draw covers only
  the two rows the shafts read (`SHAFT_LUT_DRAW_Y`): shade only what is read. Screen-spanning
  thin geometry is not the way: the SGX sets every triangle up again in each tile it touches, and
  diagonal strips touch many (measured in logs/SESION 54).
- GLSL precision qualifiers are ignored: vitaGL's translator defines `highp`/`mediump`/`lowp` as
  nothing and, with `vglUseLowPrecision` on (`MainThread_Boot`), turns every `float` into `half`.

## Textures rewritten at run time

vitaGL's `glTexSubImage2D`, on a texture drawn in the last 4 swaps (`FRAME_PURGE_FREQ`),
allocates a new texture and copies the old one into it, reading it back from CDRAM: ~3.3 ms for a
512×256 page whatever the size of the update (measured in `logs/SESION 17`), and a 256 KB
allocation freed later by the garbage collector. Textures rewritten at run time therefore either
avoid `glTexSubImage2D` via direct `vglGetTexDataPointer` writes or use double-buffered textures:

- Animated atlas pages 0–1 (active slot and visible neighbouring slots with the same primary
  tileset): written straight into texture memory (`WriteAtlasRowsDirect`, `vglGetTexDataPointer`)
  on the same frame so sea/flower animations stay lock-stepped across zone borders without
  `glTexSubImage2D` GC copies. Neighbouring slots accumulate primary dirty masks in
  `sSlotPendingPrimary` so they catch up immediately when re-entering view. The sea, shores and
  flowers cycle through a fixed set of frames, so each animated metatile is composed once per
  frame of the cycle and then copied from a cache keyed by the content of its tiles
  (`ComposeAnimated` in `voxel_atlas.c`; metatiles with a cut plan are always composed).
- Sprite atlas of the billboards: **not** a spare. `VoxelEntities_Emit` gives billboards the
  texture coordinates of the frames it decoded that same frame, so a spare swapped in later showed
  late or wrong frames. Only the 64×64 slots re-decoded this frame
  (`VoxelEntities_TakeDirtySlots`) are unswizzled, given their relief and written straight into the texture memory
  (`WriteSpriteAtlasSlots`, `vglGetTexDataPointer`), as vitaGL's own direct path does; the GPU
  may still read it for the previous frame, so a billboard can show its new frame one frame early.
- GBA overlay (pass 6): not converted nor uploaded when its image hash is unchanged.
- New UI texture (`src/platform/ui/`, `UI_TEX_W`×`UI_TEX_H` RGBA8888, unpacked from the atlas's 16-bit texels): the fixed atlas on top (rows 0–1023,
  `assets/ui2/ui_atlas.bin` v3 built by `tools/ui2/build_atlas.py`, holding every theme's
  baked fonts and fixed GBA sprites) is uploaded once; the 256 64×64 cells below (rows 1024–2047)
  receive sprites decoded at run time (Pokémon icons and front sprites, trainers, item icons,
  footprints, decorations, PC box wallpapers, mail stationery and zone sign plates). Each new
  cell is written in place (`vglGetTexDataPointer`) before the draw, and only cells no list drew
  in the last 16 ticks are reused, so the GPU never reads a cell being written.
