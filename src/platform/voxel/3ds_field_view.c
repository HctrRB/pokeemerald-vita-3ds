/*
 * The part of the overworld on screen, for the map objects (3ds_field_view.h).
 */
#include <stdint.h>
#include "3ds_field_view.h"
#include <math.h>
#include <string.h>
#include "platform.h"

#define DEG_TO_RAD (3.14159265358979323846f / 180.0f)

/*
 * The 2D field.
 */
#define FLAT_NORTH 6.0f
#define FLAT_SOUTH 9.0f
#define FLAT_HALF_WIDTH 13.0f

/*
 * The voxel camera (voxel_camera.c): a 35-degree vertical field of view on
 * the PS Vita screen (960x544), looking down at the player's pitch from a
 * horizontal distance of (8 + scale) / zoom.
 */
#define VOXEL_FOV 35.0f
#define VOXEL_ASPECT (960.0f / 544.0f)
#define VOXEL_DISTANCE 13.0f
#define VOXEL_GROUND_DROP 1.0f
#define VOXEL_PERSON_HEIGHT 2.0f
#define VOXEL_REACH_MAX 22.0f

void CtrFieldView_Flat(CtrFieldView *view)
{
    view->north = FLAT_NORTH;
    view->south = FLAT_SOUTH;
    view->farHalfWidth = FLAT_HALF_WIDTH;
    view->nearHalfWidth = FLAT_HALF_WIDTH;
    view->shiftX = view->shiftY = 0;
}

static inline float Clamp(float value, float lo, float hi)
{
    return value < lo ? lo : value > hi ? hi : value;
}

void CtrFieldView_Voxel(CtrFieldView *view, float pitchDegrees, float zoomPercent)
{
    float pitch = pitchDegrees * DEG_TO_RAD;
    float halfFov = VOXEL_FOV * 0.5f * DEG_TO_RAD;
    float tanX = tanf(halfFov) * VOXEL_ASPECT;
    float distance = VOXEL_DISTANCE * 100.0f / (zoomPercent > 1.0f ? zoomPercent : 100.0f);
    float height = tanf(pitch) * distance;
    float farHeight = height + VOXEL_GROUND_DROP;
    float nearHeight = height - VOXEL_PERSON_HEIGHT;
    float upper = pitch - halfFov, lower = pitch + halfFov;
    float farGround, nearGround;

    /* The top edge of the picture meets the ground (or reaches the bound,
     * looking at or over the horizon). */
    farGround = upper > 0.01f ? farHeight / tanf(upper) : distance + VOXEL_REACH_MAX;
    /* The bottom edge meets a person's head. */
    nearGround = nearHeight > 0.0f ? nearHeight / tanf(lower) : 0.0f;

    view->north = Clamp(farGround - distance, 0.0f, VOXEL_REACH_MAX);
    view->south = Clamp(distance - nearGround, 0.0f, VOXEL_REACH_MAX);
    /* Half the width of the picture at a point is its depth along the view
     * times the tangent of half the horizontal field of view; a card is a
     * metatile wider than the tile it stands on. */
    view->farHalfWidth = Clamp((farGround * cosf(pitch) + farHeight * sinf(pitch)) * tanX + 1.0f,
                               1.0f, VOXEL_REACH_MAX);
    view->nearHalfWidth = Clamp((nearGround * cosf(pitch) + nearHeight * sinf(pitch)) * tanX + 1.0f,
                                1.0f, VOXEL_REACH_MAX);
    view->shiftX = view->shiftY = 0;
}

float CtrFieldView_Outside(const CtrFieldView *view, int32_t dx, int32_t dy)
{
    float outside = 0.0f;
    float span = view->north + view->south;
    float t, halfWidth, side;

    dx -= view->shiftX;
    dy -= view->shiftY;
    if (dy < -view->north)
        outside = -view->north - dy;
    else if (dy > view->south)
        outside = dy - view->south;

    /* The width at this row, between the far edge's and the near edge's. */
    t = span > 0.0f ? Clamp(((float)dy + view->north) / span, 0.0f, 1.0f) : 0.0f;
    halfWidth = view->farHalfWidth + (view->nearHalfWidth - view->farHalfWidth) * t;
    side = fabsf((float)dx) - halfWidth;
    return side > outside ? side : outside;
}

float CtrFieldView_Score(const CtrFieldView *view, int32_t dx, int32_t dy)
{
    return 2.0f * CtrFieldView_Outside(view, dx, dy) + sqrtf((float)(dx * dx + dy * dy));
}

void CtrFieldView_Window(const CtrFieldView *view, int32_t margin, int32_t *west, int32_t *east, int32_t *north, int32_t *south)
{
    float halfWidth = view->farHalfWidth > view->nearHalfWidth ? view->farHalfWidth : view->nearHalfWidth;
    int32_t sideways = (int32_t)ceilf(halfWidth) + margin;

    *west = sideways - view->shiftX;
    *east = sideways + view->shiftX;
    *north = (int32_t)ceilf(view->north) + margin - view->shiftY;
    *south = (int32_t)ceilf(view->south) + margin + view->shiftY;
}

float CtrFieldView_Limit(float at, float before, float after, float low, float high, uint32_t closed)
{
    float least = low + before, most = high - after;

    /* Both ends closed and the map narrower than the view: in the middle. */
    if ((closed & CTR_FIELD_EDGE_LOW) && (closed & CTR_FIELD_EDGE_HIGH) && least > most)
        return (low + high + before - after) * 0.5f;
    if ((closed & CTR_FIELD_EDGE_LOW) && at < least)
        return least;
    if ((closed & CTR_FIELD_EDGE_HIGH) && at > most)
        return most;
    return at;
}

static CtrFieldView sView;
static uint32_t sGeneration;
static int32_t sKey = -1;
static int32_t sFlatShiftX, sFlatShiftY, sVoxelShiftX, sVoxelShiftY;

void CtrFieldView_SetFlatShift(int32_t x, int32_t y)
{
    sFlatShiftX = x;
    sFlatShiftY = y;
}

void CtrFieldView_SetVoxelShift(int32_t x, int32_t y)
{
    sVoxelShiftX = x;
    sVoxelShiftY = y;
}

const CtrFieldView *CtrFieldView_Current(void)
{
    bool voxel = Platform_GetSetting(PLATFORM_SETTING_VOXEL_3D) != 0;
    int32_t pitch = 40;
    int32_t zoom = 100;
    int32_t key = voxel ? 1 + ((pitch & 0xFF) << 1) + ((zoom & 0x3FF) << 9) : 0;
    int32_t shiftX = voxel ? sVoxelShiftX : sFlatShiftX;
    int32_t shiftY = voxel ? sVoxelShiftY : sFlatShiftY;

    if (key != sKey || shiftX != sView.shiftX || shiftY != sView.shiftY)
    {
        CtrFieldView view;

        if (voxel)
            CtrFieldView_Voxel(&view, (float)pitch, (float)zoom);
        else
            CtrFieldView_Flat(&view);
        view.shiftX = shiftX;
        view.shiftY = shiftY;
        if (sKey < 0 || memcmp(&view, &sView, sizeof(view)) != 0)
            ++sGeneration;
        sView = view;
        sKey = key;
    }
    return &sView;
}

uint32_t CtrFieldView_Generation(void)
{
    CtrFieldView_Current();
    return sGeneration;
}
