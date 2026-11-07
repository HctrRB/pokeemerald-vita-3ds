#ifndef GUARD_PLATFORM_WIDESCREEN_H
#define GUARD_PLATFORM_WIDESCREEN_H

// Vita widescreen (Options -> DISPLAY -> WIDESCREEN). The renderer draws 288 px wide lines with
// the GBA's 240 px in the middle. On the field, the map layers (BG1-3) and the sprites also fill
// the 24 px on each side; in battles, the background (BG3) and the sprites. Menus fill them with
// their background pattern; text boxes and windows stay 240 px wide and centred.
#if defined(VITA_PARALLEL_DRAW)
#define WIDESCREEN_SUPPORTED
#define WIDESCREEN_MARGIN 24
#define WIDESCREEN_WIDTH (DISPLAY_WIDTH + 2 * WIDESCREEN_MARGIN)

// Presentation on the 960x544 screen: the GBA picture fills the height (x3.4), so the screen
// shows 960 / 3.4 = 282.35 of the 288 px of each line. Every widescreen frame uses this one
// mapping, so nothing is stretched: the GBA's 240 px land on screen x 72..888.
#define WIDESCREEN_SCREEN_W 960.0f
#define WIDESCREEN_SCREEN_H 544.0f
#define WIDESCREEN_SCALE (WIDESCREEN_SCREEN_H / (float)DISPLAY_HEIGHT)
#define WIDESCREEN_VISIBLE_U0 \
    (((float)WIDESCREEN_WIDTH - WIDESCREEN_SCREEN_W / WIDESCREEN_SCALE) * 0.5f / (float)WIDESCREEN_WIDTH)
#define WIDESCREEN_VISIBLE_U1 (1.0f - WIDESCREEN_VISIBLE_U0)
// Screen x where the GBA's 240 px start and end under that mapping (72 and 888).
#define WIDESCREEN_PICTURE_X0 ((WIDESCREEN_SCREEN_W - (float)DISPLAY_WIDTH * WIDESCREEN_SCALE) * 0.5f)
#define WIDESCREEN_PICTURE_X1 (WIDESCREEN_SCREEN_W - WIDESCREEN_PICTURE_X0)

bool8 Widescreen_IsOn(void);
// Pixels the field shows beyond each side of the GBA screen: game code widens its culling and
// object spawn ranges by this much.
#define WIDESCREEN_EXTRA_X (Widescreen_IsOn() ? WIDESCREEN_MARGIN : 0)

// Called by the field on every frame it runs. The renderer then draws BGs 1-3 from the field's
// tilemap buffers (64x32 tiles on Vita, see field_camera.c) instead of VRAM, which only has room
// for 32x32. It applies while the main callback that called it is still running, so other
// scenes are never affected.
void Widescreen_SetFieldFrame(void);
// Called by the battle on every frame it runs: its background and sprites fill the margins, the
// text box and menus (BG0) are stretched to the full width, and during the intro slide the
// environment's entry background (BG1/BG2) fills the margins too.
void Widescreen_SetBattleFrame(bool8 introSlide);

// How a text BG fills the margins.
enum
{
    WIDE_BG_CENTRED, // not at all
    WIDE_BG_WIDE,    // 288 px of its map: for maps drawn to scroll in a loop (field, battle background, intro)
    WIDE_BG_EDGES,   // each margin repeats the screen's edge column where it belongs to a repeating background
    WIDE_BG_AUTO,    // per line: WIDE where the map has pixels past the screen's right edge, else repeat both edges
    WIDE_BG_FILL,    // the margins repeat the map's most common tile: its background pattern
    WIDE_BG_EDGE_TILE, // each margin repeats the 8-px tile column at the screen's edge, always
                       // (a still picture of a stage scene; the GPU fades it to black)
};

// Stage scenes (intro, title, credits, cable car, legendary cutscenes) are told apart by the
// VBlank callback they install (vita_stage.c, compat/ctr_gba_stage.h). Each of their text BGs
// is classified every frame, as Emerald 3DS does: a layer that scrolls, or a still panorama
// with art in the map columns the GBA never shows, continues as WIDE; any other still layer
// repeats its edge tile (WIDE_BG_EDGE_TILE).
// TRUE when this frame is a stage whose layers are all still: the presenter then fades the
// margins to black so the repeated edges read as the picture's frame, not as content.
bool8 Widescreen_StageFadesMargins(void);

// Called by any other scene on every frame it runs, to choose how its BGs and sprites fill the
// margins. Scenes that don't call it (menus) fill the margins with their back BG's pattern
// (WIDE_BG_FILL) and keep the other BGs and the sprites centred.
void Widescreen_SetSceneFrame(u8 bg0, u8 bg1, u8 bg2, u8 bg3, bool8 sprites);

// Main thread, at the start of each drawn frame: fills gFastDrawWide (platform/framedraw.h).
void Widescreen_PrepareDraw(void);
#endif

#endif // GUARD_PLATFORM_WIDESCREEN_H
