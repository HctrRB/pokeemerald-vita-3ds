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
 * on the console and compare logs (docs/guides/DEBUGGING.md).
 */
#ifndef VOXEL_TUNING_H
#define VOXEL_TUNING_H

/* ---- Procedural camera frustum & chunk reach (tiles) ---------------------------------- */

#define VOXEL_VIEW_REACH_MAX 48.0f    /* max horizontal reach from target (within VOXEL_WORLD_REACH=56) */
#define VOXEL_VIEW_SLACK_TILES 6.0f   /* safety margin around projected frustum XZ bounds (tiles) */
#define VOXEL_FRUSTUM_PLANE_SLACK 3.5f /* outward plane slack in world tiles (prevents edge pop-out) */
#define VOXEL_VIEW_NEAR_MIN 1.0f      /* dynamic near plane floor (normal follow camera) */
#define VOXEL_VIEW_NEAR_MAX 3.5f      /* dynamic near plane ceiling (high aerial cameras) */
#define VOXEL_VIEW_FAR_MIN 64.0f      /* dynamic far plane floor */
#define VOXEL_VIEW_FAR_MAX 200.0f     /* dynamic far plane ceiling */
#define VOXEL_VIEW_FAR_MARGIN 16.0f   /* extra depth margin past farthest ground corner */

/* ---- Follow camera & 3D Fly cinematic (tiles / degrees / seconds) --------------------- */

#define VOXEL_FOLLOW_TAU 0.09f        /* exponential follow time constant (identical at 30 & 60 FPS) */
#define VOXEL_FOLLOW_DT_MAX 0.10f     /* max frame dt fed to camera smoothing (clamps lag spikes) */
#define VOXEL_FLY_PITCH 62.0f         /* aerial pullback camera pitch (degrees) */
#define VOXEL_FLY_DISTANCE 24.0f      /* aerial pullback horizontal distance (tiles) */
#define VOXEL_FLY_RISE 22.0f          /* aerial vertical lift at peak climb (tiles) */
#define VOXEL_FLY_LEAD 8.0f           /* forward lead along flight heading (tiles) */

/* ---- Chunk builds and pre-warming (RENDERING.md, step 5) ------------------------------ */

#define VOXEL_CHUNK_WORKER 1          /* 0 builds every chunk on Core 0, as before the worker */
#define VOXEL_BUILD_TAIL_MS 6.0       /* passes after the build loop: entities, draw, overlay, swap */
#define VOXEL_BUILD_COST_INIT_MS 8.0  /* first estimate of one build, refined as chunks are built */
#define VOXEL_HOLE_BUILDS_PER_FRAME 1 /* visible holes filled on Core 0 per frame (Core 2 takes 4-chunk batches) */
#define VOXEL_PREFETCH_MAX 48         /* unbuilt chunks outside the view considered per frame */
#define VOXEL_PREFETCH_LOOKAHEAD 12.0f /* base tiles ahead of the player */
#define VOXEL_PREFETCH_SPEED_GAIN 45.0f /* extra lookahead tiles per tile/frame of movement speed */
#define VOXEL_PREFETCH_LOOKAHEAD_MAX 22.0f /* cap on speed-scaled prewarm lookahead (tiles) */
#define VOXEL_FAST_MOVE_SPEED 0.16f   /* speed (tiles/frame) above which prewarm widens to 2 chunks */
#define VOXEL_EPOCH_HASH_BUDGET 8     /* max non-urgent epoch hash checks per calm frame (sun ticks) */
#define VOXEL_VIEW_AHEAD_TILES 8.0f   /* camera advance for which chunks are pre-warmed first */
#define VOXEL_MOVE_THRESHOLD 0.02f    /* tiles per frame under which the player counts as still */
#define VOXEL_STALE_MAX 32            /* dated visible chunks considered per frame */

/* ---- Atlas, VBOs and textures rewritten at run time (RENDERING.md) -------------------- */

#define VOXEL_ATLAS_EXTEND_BUDGET_MS 4.0      /* atlas extension work per frame on Vita */
#define VOXEL_ATLAS_EXTEND_STEP_METATILES 64u /* metatiles composed per extension step */
#define VOXEL_NEIGHBOR_MARGIN_TILES 8         /* border margin for direct-neighbor proactive atlas extend */
#define VOXEL_VITAGL_PURGE_FRAMES 4u          /* vitaGL FRAME_PURGE_FREQ: frames a drawn VBO stays in GPU flight */

/* ---- Map crossings -------------------------------------------------------------------- */

#define VOXEL_CROSSING_STEP 16.0f     /* the player moving further in one frame is a re-base, not a walk */

/* ---- Colour grade and haze ------------------------------------------------------------ */

#define VOXEL_GRADE_SHADE_LOW 0.62f
#define VOXEL_HAZE_START 1.05f
#define VOXEL_HAZE_RAMP 0.60f
#define VOXEL_HAZE_MAX 0.14f
#define VOXEL_HAZE_COLOUR 0xFFFAE6D2u
#define VOXEL_TREE_BRIGHTNESS 1.12f

/* ---- Night building windows (Pass_BuildingGlow, VoxelBuildings_ReadGlow) -------------- */

#define VOXEL_WINDOW_GLOW_R 1.00f             /* warm indoor window light tint (amber-gold) */
#define VOXEL_WINDOW_GLOW_G 0.80f
#define VOXEL_WINDOW_GLOW_B 0.38f
#define VOXEL_WINDOW_GLOW_STRENGTH_STD  0.90f /* additive alpha scale in normal Voxel mode */
#define VOXEL_WINDOW_GLOW_STRENGTH_HD2D 0.68f /* lower scale in HD-2D for a soft bloom halo */

/* ---- Night lamps (Pass_NightLamps, UpdatePointLights, VoxelLamps_EmitChunk) ----------- */

#define VOXEL_LAMP_R 1.00f                  /* lamp light: Bravely Emerald / Emerald 3DS amber (255,191,107) */
#define VOXEL_LAMP_G 0.75f
#define VOXEL_LAMP_B 0.42f
#define VOXEL_LAMP_GLASS_GAIN 1.00f         /* additive glass: bright enough to cross the bloom threshold */
#define VOXEL_LAMP_POOL_GAIN 0.80f          /* pool on the ground: dst * (1 + gain * falloff * amber) */
#define VOXEL_LAMP_POOL_LIFT 0.02f          /* the pool's lift off the ground it lies on (plus a depth offset) */
#define VOXEL_LAMP_HALO_LIFT 0.30f          /* halo pulled toward the camera so the lantern does not cut it */
#define VOXEL_LAMP_LIGHT_RADIUS 3.5f        /* reach of a lamp's light on characters (lamps stand close together) */
#define VOXEL_LAMP_LIGHT_GAIN 0.85f         /* light added to characters at the lamp, before the night veil */
#define VOXEL_LAMP_HALO_MAX 64              /* halos drawn per frame */

/* ---- Fog and sand banks --------------------------------------------------------------- */

#define VOXEL_FOG_DIM 64
#define VOXEL_FOG_SPAN_X 24.0f
#define VOXEL_FOG_SOUTH 12.0f
#define VOXEL_FOG_NORTH 44.0f
#define VOXEL_FOG_PERIOD_X 18.0f
#define VOXEL_FOG_PERIOD_Z 12.0f
#define VOXEL_FOG_GRID 12
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
