#include <stdint.h>
#include "platform/vita_corner_art.h"
#include <math.h>
#include <string.h>

typedef struct { float r, g, b; } RgbColor;

static inline RgbColor GbaToRgb(const uint16_t r, const uint16_t g, const uint16_t b)
{
    return (RgbColor){ (float)r / 31.0f, (float)g / 31.0f, (float)b / 31.0f };
}

static inline RgbColor Shade(const RgbColor c, const float s)
{
    return (RgbColor){ c.r * s, c.g * s, c.b * s };
}

static inline uint16_t Pack565(const RgbColor c)
{
    const uint16_t r = (uint16_t)(c.r * 31.0f + 0.5f) & 31;
    const uint16_t g = (uint16_t)(c.g * 63.0f + 0.5f) & 63;
    const uint16_t b = (uint16_t)(c.b * 31.0f + 0.5f) & 31;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static inline float Around(const int32_t x, const int32_t target, const int32_t dim)
{
    int32_t d = (x - target) % dim;
    if (d > dim / 2) d -= dim;
    if (d < -dim / 2) d += dim;
    return (float)d;
}

static inline void Put(uint16_t *texture, const uint32_t dim, const uint32_t x, const uint32_t y, const RgbColor c)
{
    texture[(y % dim) * dim + (x % dim)] = Pack565(c);
}

static void Bulb(uint16_t *trim, const uint32_t x, const uint32_t y, const bool lit)
{
    const RgbColor gold = GbaToRgb(31, 26, 8);
    const RgbColor goldDark = GbaToRgb(24, 16, 4);
    const RgbColor shine = GbaToRgb(31, 31, 20);

    for (uint32_t j = 0; j < 4; ++j)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            const bool inner = (i >= 1 && i <= 2 && j >= 1 && j <= 2);
            if ((i == 0 || i == 3) && (j == 0 || j == 3))
                continue;
            Put(trim, VITA_CORNER_TRIM_DIM, x + i, y + j, !lit ? goldDark : inner ? shine : gold);
        }
    }
}

static void Coin(uint16_t *pattern, const int32_t cx, const int32_t cy)
{
    const RgbColor gold = GbaToRgb(30, 24, 8);
    const RgbColor dark = GbaToRgb(20, 14, 4);

    for (int32_t y = -3; y <= 3; ++y)
    {
        for (int32_t x = -3; x <= 3; ++x)
        {
            const float d = sqrtf((float)(x * x + y * y));
            if (d < 3.2f)
            {
                Put(pattern, VITA_CORNER_PATTERN_DIM, (uint32_t)(cx + x), (uint32_t)(cy + y), d > 2.0f ? dark : gold);
            }
        }
    }
}

void VitaCornerArt_Felt(uint16_t *pattern, uint16_t *trim, const uint16_t table)
{
    static const int32_t balls[2][2] = {{32, 16}, {0, 48}};
    const uint32_t dim = VITA_CORNER_PATTERN_DIM;
    const uint32_t trimDim = VITA_CORNER_TRIM_DIM;
    const RgbColor base = GbaToRgb(table & 31, (table >> 5) & 31, (table >> 10) & 31);
    const RgbColor dark = Shade(base, 0.82f);
    const RgbColor lid = Shade(base, 0.92f);
    const RgbColor gold = GbaToRgb(30, 24, 8);
    const RgbColor goldDark = GbaToRgb(22, 15, 4);

    for (uint32_t y = 0; y < dim; ++y)
    {
        for (uint32_t x = 0; x < dim; ++x)
        {
            RgbColor c = base;
            for (uint32_t i = 0; i < 2; ++i)
            {
                const float ox = Around((int32_t)x, balls[i][0], (int32_t)dim) + 0.5f;
                const float oy = Around((int32_t)y, balls[i][1], (int32_t)dim) + 0.5f;
                const float d = sqrtf(ox * ox + oy * oy);

                if (d >= 11.5f) continue;
                if (d > 10.0f || (fabsf(oy) < 1.2f && d > 3.5f) || (d > 2.0f && d < 3.5f))
                    c = dark;
                else if (oy < 0.0f)
                    c = lid;
            }
            Put(pattern, dim, x, y, c);
        }
    }

    for (uint32_t y = 0; y < trimDim; ++y)
        for (uint32_t x = 0; x < trimDim; ++x)
            Put(trim, trimDim, x, y, Shade(base, 0.66f));

    for (uint32_t i = 0; i < trimDim; ++i)
    {
        Put(trim, trimDim, i, 0, i % 4 < 2 ? gold : goldDark);
        Put(trim, trimDim, 0, i, i % 4 < 2 ? gold : goldDark);
    }
    Put(trim, trimDim, VITA_CORNER_FELT_LINE, VITA_CORNER_FELT_LINE, Shade(base, 0.74f));
}

void VitaCornerArt_Wall(uint16_t *pattern, uint16_t *trim)
{
    const uint32_t dim = VITA_CORNER_PATTERN_DIM;
    const uint32_t trimDim = VITA_CORNER_TRIM_DIM;
    const RgbColor wall = GbaToRgb(3, 9, 9);
    const RgbColor wallLight = GbaToRgb(4, 11, 11);
    const RgbColor seam = GbaToRgb(7, 16, 15);
    const RgbColor red = GbaToRgb(18, 4, 6);

    for (uint32_t y = 0; y < dim; ++y)
    {
        for (uint32_t x = 0; x < dim; ++x)
        {
            const uint32_t u = x + y, v = x + dim - y;
            const RgbColor c = ((u / 32 + v / 32) & 1) ? wallLight : wall;
            Put(pattern, dim, x, y, (u % 32 == 0 || v % 32 == 0) ? seam : c);
        }
    }

    for (int32_t cy = 0; cy < (int32_t)dim; cy += 16)
        for (int32_t cx = (cy % 32) ? 0 : 16; cx < (int32_t)dim; cx += 32)
            Coin(pattern, cx, cy);

    for (uint32_t y = 0; y < trimDim; ++y)
        for (uint32_t x = 0; x < trimDim; ++x)
            Put(trim, trimDim, x, y, red);

    Bulb(trim, 1, 1, true);
    Bulb(trim, 17, 1, false);
    Bulb(trim, 1, 17, false);
    Put(trim, trimDim, VITA_CORNER_FRAME_DARK, VITA_CORNER_FRAME_DARK, GbaToRgb(24, 16, 4));
    Put(trim, trimDim, VITA_CORNER_FRAME_GOLD, VITA_CORNER_FRAME_GOLD, GbaToRgb(31, 26, 8));
    Put(trim, trimDim, VITA_CORNER_FRAME_SHADOW, VITA_CORNER_FRAME_SHADOW, GbaToRgb(2, 4, 4));
}
