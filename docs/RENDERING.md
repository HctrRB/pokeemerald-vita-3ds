# Rendering

Everything on screen goes through vitaGL (OpenGL ES 1.x style fixed pipeline over SceGxm) at
960×544. There are three presentation paths, chosen every frame in `sdl2.c` (see
[ARCHITECTURE.md](ARCHITECTURE.md)).

| Path | When | Entry point |
|---|---|---|
| Native UI | The dock / a native screen is open | `ShowNativeUi()` (`sdl2.c`) |
| 3D | Field callbacks, shop buy menu, 3D battle | `VoxelRenderer_RenderFrame()` (`voxel/voxel_renderer.c`) |
| 2D | Every other GBA screen | `VDraw()` + `RenderGameTexture()` (`sdl2.c`) |

## 2D path: the software GBA PPU

`gba_fast_draw.c` is a scanline renderer of the GBA's video hardware: text and affine
backgrounds, sprites, windows, blending, mosaic, per-line scroll effects.

- `DrawFrame()` runs each line's hardware events in order on the calling thread and captures
  the registers and palette that line sees; two workers on Core 2 draw the captured lines in
  parallel (`gba_fast_draw_b.c` and `_c.c` recompile the renderer so each has its own state).
- Lines are **288 px** wide (`FRAME_LINE_WIDTH`), with the GBA screen at x = 24. What goes into
  the margins is decided per scene by `widescreen.c`; see [SCENES_16_9.md](SCENES_16_9.md).
- Workers convert BGR555 to RGB565 with NEON. `VDraw` uploads the frame into a ring of
  `VITA_GAME_TEXTURE_RING` textures so the GPU never reads a texture being written.
- `RenderGameTexture` draws one quad: 288 px stretched to 960 with widescreen on, or the
  middle 240 px at 816×544 (pillarboxed) with it off. The intro's leaves scene draws GPU margin
  art beside it (`vita_stage.c`). Filter: nearest or bilinear (`filter` setting). The day/night
  tint is applied as the quad colour.

## 3D path: `VoxelRenderer_RenderFrame`

Per frame (`VoxelRenderer_RenderFrame`):

1. **Camera**: follow camera with sub-tile interpolation (`voxel_camera.c`), battle camera in
   3D battles.
2. **Atlas**: metatile atlas pages composed on demand in 5 LRU slots (`voxel_atlas.c`);
   animated tiles (water, flowers, shores) recomposed on the CPU when the game animates them and
   uploaded through a spare texture per page (see "Textures rewritten at run time" below).
3. **Lighting**: sun position from the clock (`voxel_lighting.c`); chunks whose lighting hash
   changed are rebuilt progressively.
4. **Culling** (`Frame_SelectChunks`): a chunk is a candidate when it overlaps the tile window
   around the camera target (36 tiles north, 12 south, 12–18 to the sides), then must pass the
   frustum test (`Frustum_ChunkInView`, `zFar = 62`). A building is emitted only by the chunk
   holding its north-west cell, so each chunk records how far its building models reach past its
   own cells and how tall they are (`bldWest/East/North/South/Top`, measured from the vertices
   on the worker); both tests use that extent, and chunks up to 3 west and 4 north of the window
   are kept for their models. Those extra chunks are drawn only when already built: they are
   never built or pre-warmed from there. Without this, wide buildings (Rustboro's Devon Corp.)
   vanished at the screen's west edge while still in view.
5. **Chunk builds**: in three phases (`PrepareChunkJob`, `ComputeChunkJob`, `CommitChunkJob`).
   Visible holes, and chunks drawn with an outdated atlas, are built on Core 0 in the frame:
   up to two whatever the budget, more if they fit it (`ChunkBuildFits`: the estimated build
   cost plus `VOXEL_BUILD_TAIL_MS` must stay inside 16.7 ms at 60 FPS or 33.3 ms at 30 FPS).
   Every other build goes to the chunk worker on Core 2, one per frame: first a chunk the view
   will reach within 8 tiles of movement (`Frustum_ChunkInViewAhead`), then the dated chunk
   (sun moved, tile changed) nearest to the player, then the unbuilt chunk nearest to a point
   12 tiles ahead of the movement. It is launched after the swap and committed at the start of
   the first frame that finds it done; a frame that starts while it still runs does not wait
   for it (see THREADS.md). While an atlas slot is being extended after a crossing, nothing is
   handed to the worker: each of those frames waits for it before touching the atlas. Without
   the worker those builds run on Core 0 when the frame has room. Warm-up and faded frames
   build on Core 0 without limit. Chunks drawn this frame or the previous one are never evicted.
6. **Passes**, each batched into few draw calls:

| Pass | Content | Notes |
|---|---|---|
| 1A / 1B | Ground and relief, then cliff cutouts | Opaque first for early-Z on the SGX543; cutouts with `GL_ALPHA_TEST` |
| 2 | Trees and tall grass | Tree atlas |
| 3 | Buildings and indoor furniture | Building pages in an LRU cache, alpha test only on pages that need it |
| 4 | Cast and decal shadows | One colour (`0xFF281408`) |
| 4.5 | Reflections on water | |
| 5 | Entity billboards, player silhouette behind roofs | `glDepthFunc(GL_GREATER)` for the silhouette |
| 5.1 | Day/night modulation | Skipped when it would be a no-op |
| 5.2 | Street lamps and light pools | |
| 5.4–5.8 | Fog / sand banks, dust motes, weather particles | |
| 5.9 | 2D atmosphere: sun rays, moonlight, fades, lightning, cave gloom | |
| DoF | Tilt-shift "diorama" blur | Scene rendered to a 960×544 FBO, downscaled to 1/4 (240×136) and composited as blurred top/bottom bands. Off indoors, during fades or with `voxel_blur` off |
| 6 | GBA overlay | See below |

In the code each step is a `Frame_*` function (`Frame_FollowCamera`, `Frame_UpdateAtlas`,
`Frame_AnimateTiles`, `Frame_TrackWorldChanges`, `Frame_ReadAtmosphere`, `Frame_SelectChunks`,
`Frame_EmitEntities`) and each pass a `Pass_*` function (`Pass_Ground` … `Pass_Overlay2D`).
They share the frame state through a `VoxelFrame`; `VoxelRenderer_RenderFrame` only calls them
in order. The builds left after the visibility loop go through a `VoxelBuildQueue`
(`BuildQueue_SendToWorker`, `BuildQueue_BuildStale`, `BuildQueue_Prewarm`).

## Pass 6: the GBA overlay over 3D

Text boxes, menus, the battle interface and field-move banners are still drawn by the game
through the GBA PPU. When `VoxelRenderer_HasActiveUi()` sees one:

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
  An earlier per-frame `vglBufferData` path crashed the GPU.
- Never upload into a texture the GPU drew in the last frames through `glTexSubImage2D`: vitaGL
  copies all of it first. Use rings or spares, or write in place on purpose (see below).

## Textures rewritten at run time

vitaGL's `glTexSubImage2D`, on a texture drawn in the last 4 swaps (`FRAME_PURGE_FREQ`),
allocates a new texture and copies the old one into it, reading it back from CDRAM: ~3.3 ms for a
512×256 page whatever the size of the update (measured in `logs/SESION 17`), and a 256 KB
allocation freed later by the garbage collector. Textures the renderer rewrites therefore keep
spare copies and only write into a copy that has not been drawn for `VOXEL_TEX_IDLE_FRAMES` (5)
frames, which vitaGL fills directly:

- Animated atlas pages 0–1 of the active slot: one spare each (`sAtlasSpares`), rows tracked
  as stale per copy; the spare is brought up to date and swapped in when idle long enough, so an
  animation step can show up to a few frames late.
- Sprite atlas of the billboards: **not** a spare. `VoxelEntities_Emit` gives billboards the
  texture coordinates of the frames it decoded that same frame, so a spare swapped in later showed
  late or wrong frames. Only the 64-px bands of the slots re-decoded this frame
  (`VoxelEntities_TakeDirtySlots`) are unswizzled and written straight into the texture memory
  (`WriteSpriteAtlasSlots`, `vglGetTexDataPointer`), as vitaGL's own direct path does; the GPU
  may still read it for the previous frame, so a billboard can show its new frame one frame early.
- GBA overlay (pass 6): not converted nor uploaded when its image hash is unchanged.
- New UI texture (`src/platform/ui/`, 1024x1024 RGBA5551): the fixed atlas on top (fonts, GBA
  sprites of `tools/ui2/build_atlas.py`) is uploaded once; the 64x64 cells below receive sprites
  decoded at run time (Pokémon icons and front sprites, item icons, footprints). Each new cell is
  written in place (`vglGetTexDataPointer`) before the draw, and only cells no list drew in the
  last 16 ticks are reused, so the GPU never reads a cell being written.
