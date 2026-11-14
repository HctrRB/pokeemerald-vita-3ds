/*
 * voxel_tuning.h - the voxel renderer's tuning values in one place.
 *
 * Thread: read by Core 0 (renderer) and the chunk worker (Core 2); compile-time only.
 * Origin: values from Emerald 3DS (ctr_voxel.c, CC BY-NC-SA 4.0, see NOTICE.md) and from the
 * Vita performance work.
 *
 * Only knobs that change how the 3D world looks or how its work is paced live here. Capacities
 * that size buffers and data layouts (chunk vertex caps, slot counts, FBO sizes) stay next to
 * the structures they size in voxel_renderer.c. Changing a value here changes behaviour: test
 * on the console and compare logs (docs/DEBUGGING.md).
 */
#ifndef VOXEL_TUNING_H
#define VOXEL_TUNING_H

/* ---- Chunks considered around the camera target (tiles) ------------------------------- */

#define VOXEL_VIEW_NORTH_TILES 36.0f  /* candidate rows reach this far north of the target */
#define VOXEL_VIEW_SOUTH_TILES 12.0f  /* and this far south */
#define VOXEL_VIEW_FAR_ROW_TILES 8.0f /* rows further north than this use the far margins */
#define VOXEL_VIEW_FAR_LEFT 18.0f     /* lateral margins of the far rows (and indoors) */
#define VOXEL_VIEW_FAR_RIGHT 16.0f
#define VOXEL_VIEW_NEAR_LEFT 14.0f    /* lateral margins of the near rows */
#define VOXEL_VIEW_NEAR_RIGHT 12.0f

/* ---- Chunk builds and pre-warming (RENDERING.md, step 5) ------------------------------ */

#define VOXEL_CHUNK_WORKER 1          /* 0 builds every chunk on Core 0, as before the worker */
#define VOXEL_BUILD_TAIL_MS 6.0       /* passes after the build loop: entities, draw, overlay, swap */
#define VOXEL_BUILD_COST_INIT_MS 8.0  /* first estimate of one build, refined as chunks are built */
#define VOXEL_HOLE_BUILDS_PER_FRAME 2 /* visible holes filled per frame regardless of the budget */
#define VOXEL_PREFETCH_MAX 32         /* unbuilt chunks outside the view considered per frame */
#define VOXEL_PREFETCH_LOOKAHEAD 12.0f /* tiles ahead of the player that pre-warming aims at */
#define VOXEL_VIEW_AHEAD_TILES 8.0f   /* camera advance for which chunks are pre-warmed first */
#define VOXEL_MOVE_THRESHOLD 0.02f    /* tiles per frame under which the player counts as still */
#define VOXEL_STALE_MAX 32            /* dated visible chunks considered per frame */

/* ---- Atlas and textures rewritten at run time (RENDERING.md) -------------------------- */

#define VOXEL_TEX_IDLE_FRAMES 5u      /* frames a spare texture rests before vitaGL writes it directly */
#define VOXEL_ATLAS_EXTEND_BUDGET_MS 1.5      /* atlas extension work per frame on crossings */
#define VOXEL_ATLAS_EXTEND_STEP_METATILES 16u /* metatiles composed per extension step */

/* ---- Map crossings -------------------------------------------------------------------- */

#define VOXEL_CROSSING_STEP 16.0f     /* the player moving further in one frame is a re-base, not a walk */

/* ---- Colour grade and haze ------------------------------------------------------------ */

#define VOXEL_GRADE_SHADE_LOW 0.62f
#define VOXEL_HAZE_START 1.05f
#define VOXEL_HAZE_RAMP 0.60f
#define VOXEL_HAZE_MAX 0.14f
#define VOXEL_HAZE_COLOUR 0xFFFAE6D2u
#define VOXEL_TREE_BRIGHTNESS 1.12f

/* ---- Fog and sand banks --------------------------------------------------------------- */

#define VOXEL_FOG_DIM 64
#define VOXEL_FOG_SPAN_X 24.0f
#define VOXEL_FOG_SOUTH 12.0f
#define VOXEL_FOG_NORTH 36.0f
#define VOXEL_FOG_PERIOD_X 18.0f
#define VOXEL_FOG_PERIOD_Z 12.0f
#define VOXEL_FOG_GRID 8
#define VOXEL_FOG_BANK_VERTICES (VOXEL_FOG_GRID * VOXEL_FOG_GRID * 6)

/* ---- Weather particles ---------------------------------------------------------------- */

#define VOXEL_WEATHER_COUNT 256
#define VOXEL_WEATHER_SPAN_X 24.0f
#define VOXEL_WEATHER_SPAN_Y 8.5f
#define VOXEL_WEATHER_SPAN_Z 28.0f
#define VOXEL_WEATHER_LOW -0.4f
#define VOXEL_WEATHER_NORTH 20.0f
#define VOXEL_WEATHER_FADE_XZ 3.0f
#define VOXEL_WEATHER_FADE_Y 1.2f

/* ---- Dust motes ----------------------------------------------------------------------- */

#define VOXEL_MOTE_COUNT 32
#define VOXEL_MOTE_SPAN_X 22.0f
#define VOXEL_MOTE_SPAN_Z 18.0f
#define VOXEL_MOTE_ALPHA 0.85f
#define VOXEL_MOTE_RADIUS 0.062f

/* ---- Depth of field: blurred bands at 544 rows ---------------------------------------- */

#define VOXEL_DOF_TOP_ROWS 150.0f     /* rows of the top band (a diorama: only the edges) */
#define VOXEL_DOF_BOTTOM_ROWS 84.0f   /* rows of the bottom band */
#define VOXEL_DOF_TOP_ALPHA 0.60f     /* blur strength at the screen edge */
#define VOXEL_DOF_BOTTOM_ALPHA 0.50f

/* ---- Sun rays ------------------------------------------------------------------------- */

#define VOXEL_RAYS_OPACITY 0.75f      /* on the shafts' alpha (25 % more transparent) */

#endif /* VOXEL_TUNING_H */
