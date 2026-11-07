#ifndef GUARD_FRAMEDRAW_H
#define GUARD_FRAMEDRAW_H

#include "global.h"
#include "platform/widescreen.h"

#ifdef WIDESCREEN_SUPPORTED
// Frame lines are FRAME_LINE_WIDTH pixels; the GBA screen starts at x = FRAME_LINE_X0.
#define FRAME_LINE_WIDTH WIDESCREEN_WIDTH
#define FRAME_LINE_X0 WIDESCREEN_MARGIN

// How the frame being drawn uses the widescreen margins (set by Widescreen_PrepareDraw).
struct FastDrawWide
{
    bool8 output;    // lines are drawn and shown 288 px wide (else only the middle 240 px)
    bool8 extend;    // some layers fill the margins this frame
    u8 bgSpan[4];    // how each text BG fills the margins: WIDE_BG_*
    bool8 extendObj; // sprites fill the margins
    u16 *bgMaps[4];  // RAM tilemaps (GBA layout) drawn instead of the BG's VRAM map, or NULL
    u8 bgMapBase[4]; // ...only while the BG still uses this screen base block
    u8 bgMapSize[4]; // BGCNT size of bgMaps: 1 = 64x32 tiles, 3 = 64x64
    u16 *bgFillMaps[4]; // WIDE_BG_FILL: a 32x32 map of the fill tile, drawn in the margins
};
extern struct FastDrawWide gFastDrawWide;
#else
#define FRAME_LINE_WIDTH DISPLAY_WIDTH
#define FRAME_LINE_X0 0
#endif

// pixels: DISPLAY_HEIGHT lines of FRAME_LINE_WIDTH pixels.
void DrawFrame(uint16_t *pixels);
// Explicit backdrop color override for UI chromaKey overlay (Pass 6): when >= 0, DrawLine uses
// this exact color directly without reading PLTT[0] or applying brightness/blending.
extern int32_t gFastDrawBackdropOverride;
// When true, DrawSprites only renders sprites with bits set in gScreenOam (coordOffsetEnabled == false)
extern bool8 gFastDrawScreenObjOnly;
// Bits cleared from DISPCNT / BLDCNT in every line the next DrawFrame draws. They are applied to
// each line's captured registers, never to the live ones (which the game thread owns): pass 6
// hides the map layers of the GBA frame it overlays on the 3D world this way.
extern u16 gFastDrawDispcntClear;
extern u16 gFastDrawBldcntClear;
// When TRUE, the next DrawFrame draws only the GBA's 240 px (nothing in the widescreen margins):
// an overlay shown centred does not pay for margins nobody sees.
extern bool8 gFastDrawCentredOnly;
// Runs a frame's per-scanline hardware events (VCount/HBlank interrupts and HBlank DMAs) in
// order, exactly as DrawFrame does, without drawing anything.
void RunFrameLineEvents(void);
// Direct event dispatch: triggers audio mixing (VCount match) and VBlank prep in < 0.001 ms without a 160-line loop
void GbaEngine_StepDirectHardwareEvents(void);
#if defined(VITA_PARALLEL_DRAW)
// DrawFrame, also writing each finished line into a BGR565 texture of FRAME_LINE_WIDTH pixels
// (pitch in bytes). pixels still receives the frame in the GBA's BGR555.
void DrawFrameToTexture(uint16_t *pixels, void *texture, int pitch);
#endif
#endif
