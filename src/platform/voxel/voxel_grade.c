#include <stdint.h>
#include <stdbool.h>

#include "3ds_video.h"
#include "voxel_grade.h"

/*
 * Each channel pushed away from the colour's luma by this much. The GBA art
 * is pastel, made for a dim, unlit screen; lit and shaded in 3D it read flat
 * and washed out. 1.30 was chosen against captures of 1.0, 1.30 and 1.50:
 * richer greens and reds without the lit ground turning garish. Contrast is
 * left alone, so the shadows do not deepen.
 */
#define VOXEL_GRADE_SATURATION 1.30f

/* Graded RGBA5551, alpha set, for every BGR555 colour: 64 KiB of heap. */
static uint16_t sTable[1u << 15];
static bool sReady;
static bool sReliefEnabled = true;

void VoxelGrade_SetReliefEnabled(const bool enabled)
{
    sReliefEnabled = enabled;
}

static uint32_t Channel(float value)
{
    int32_t c = (int32_t)(value + 0.5f);

    return c < 0 ? 0u : c > 31 ? 31u : (uint32_t)c;
}

void VoxelGrade_Init(void)
{
    if (sReady)
        return;
    for (uint32_t bgr = 0; bgr < (1u << 15); ++bgr)
    {
        float r = (float)(bgr & 31), g = (float)((bgr >> 5) & 31), b = (float)(bgr >> 10);
        float y = 0.299f * r + 0.587f * g + 0.114f * b;
        uint32_t gr = Channel(y + (r - y) * VOXEL_GRADE_SATURATION);
        uint32_t gg = Channel(y + (g - y) * VOXEL_GRADE_SATURATION);
        uint32_t gb = Channel(y + (b - y) * VOXEL_GRADE_SATURATION);

        sTable[bgr] = CtrVideo_RGBA5551((uint16_t)(gr | gg << 5 | gb << 10));
    }
    sReady = true;
}

uint16_t VoxelGrade_RGBA5551(uint16_t bgr15)
{
    return sReady ? sTable[bgr15 & 0x7FFF] : CtrVideo_RGBA5551(bgr15);
}

void VoxelGrade_Texels(uint16_t *texels, uint32_t count)
{
    if (!sReady)
        return;
    for (uint32_t i = 0; i < count; ++i)
    {
        uint16_t t = texels[i];
        uint32_t bgr = ((t >> 11) & 31) | ((t >> 6) & 31) << 5 | ((t >> 1) & 31) << 10;

        texels[i] = (uint16_t)((sTable[bgr] & ~1u) | (t & 1u));
    }
}

void VoxelGrade_Brighten(uint16_t *texels, uint32_t count, float factor)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        uint16_t t = texels[i];
        uint32_t r = Channel((float)((t >> 11) & 31) * factor);
        uint32_t g = Channel((float)((t >> 6) & 31) * factor);
        uint32_t b = Channel((float)((t >> 1) & 31) * factor);

        texels[i] = (uint16_t)(r << 11 | g << 6 | b << 1 | (t & 1u));
    }
}

void VoxelGrade_ApplyTileRelief16x16(uint16_t *dest, uint32_t stride, uint32_t height,
                                     uint32_t baseX, uint32_t baseY)
{
    if (!sReliefEnabled)
        return;
    int16_t lum[16][16];
    for (uint32_t y = 0; y < 16u; ++y)
    {
        uint16_t *const row = dest + (height - 1u - (baseY + y)) * stride + baseX;
        for (uint32_t x = 0; x < 16u; ++x)
        {
            const uint16_t t = row[x];
            if ((t & 1u) == 0u)
            {
                lum[y][x] = -1;
                continue;
            }
            const int16_t r = (int16_t)((t >> 11) & 31u);
            const int16_t g = (int16_t)((t >> 6) & 31u);
            const int16_t b = (int16_t)((t >> 1) & 31u);
            lum[y][x] = (int16_t)(r * 2 + g * 4 + b); // [0..217]
        }
    }

    for (uint32_t y = 0; y < 16u; ++y)
    {
        const uint32_t ym = (y > 0u) ? (y - 1u) : 0u;
        const uint32_t yp = (y < 15u) ? (y + 1u) : 15u;
        uint16_t *const row = dest + (height - 1u - (baseY + y)) * stride + baseX;
        for (uint32_t x = 0; x < 16u; ++x)
        {
            const uint16_t t = row[x];
            const int16_t cLum = lum[y][x];
            if ((t & 1u) == 0u || cLum < 0)
                continue;

            const uint32_t xm = (x > 0u) ? (x - 1u) : 0u;
            const uint32_t xp = (x < 15u) ? (x + 1u) : 15u;

            // Clamp missing/cutout neighbors to center luma so transparent edges don't halo
            const int16_t lNW = (lum[ym][xm] >= 0) ? lum[ym][xm] : cLum;
            const int16_t lSE = (lum[yp][xp] >= 0) ? lum[yp][xp] : cLum;
            const int16_t lN  = (lum[ym][x]  >= 0) ? lum[ym][x]  : cLum;
            const int16_t lS  = (lum[yp][x]  >= 0) ? lum[yp][x]  : cLum;
            const int16_t lW  = (lum[y][xm]  >= 0) ? lum[y][xm]  : cLum;
            const int16_t lE  = (lum[y][xp]  >= 0) ? lum[y][xp]  : cLum;

            const int32_t dir = (int32_t)lNW - (int32_t)lSE;
            const int32_t avg = ((int32_t)lN + (int32_t)lS + (int32_t)lW + (int32_t)lE) >> 2;
            const int32_t cav = (int32_t)cLum - avg;
            int32_t delta = (dir + cav) >> 2;
            if (delta < -5) delta = -5;
            if (delta > 5)  delta = 5;
            if (delta == 0)
                continue;

            int32_t r = (int32_t)((t >> 11) & 31u) + delta;
            int32_t g = (int32_t)((t >> 6) & 31u) + delta;
            int32_t b = (int32_t)((t >> 1) & 31u) + delta;
            r = (r < 0) ? 0 : (r > 31) ? 31 : r;
            g = (g < 0) ? 0 : (g > 31) ? 31 : g;
            b = (b < 0) ? 0 : (b > 31) ? 31 : b;
            row[x] = (uint16_t)((r << 11) | (g << 6) | (b << 1) | (t & 1u));
        }
    }
}

void VoxelGrade_ApplyRectRelief(uint16_t *const buf, const uint32_t stride,
                                const uint32_t width, const uint32_t height)
{
    if (!sReliefEnabled || buf == NULL || width < 2u || width > 1024u || stride < width || height < 2u)
        return;

    static int16_t sLum[3][1024];
    for (uint32_t r = 0; r < 2u; ++r)
    {
        const uint16_t *const src = buf + r * stride;
        for (uint32_t x = 0; x < width; ++x)
        {
            const uint16_t t = src[x];
            if ((t & 1u) == 0u)
            {
                sLum[r][x] = -1;
                continue;
            }
            const int16_t cr = (int16_t)((t >> 11) & 31u);
            const int16_t cg = (int16_t)((t >> 6) & 31u);
            const int16_t cb = (int16_t)((t >> 1) & 31u);
            sLum[r][x] = (int16_t)(cr * 2 + cg * 4 + cb);
        }
    }

    for (uint32_t y = 0; y < height; ++y)
    {
        const uint32_t curIdx = y % 3u;
        const uint32_t prevIdx = (y > 0u) ? ((y - 1u) % 3u) : curIdx;
        const uint32_t nextIdx = (y + 1u < height) ? ((y + 1u) % 3u) : curIdx;

        if (y + 1u < height && y + 1u >= 2u)
        {
            const uint16_t *const nextSrc = buf + (y + 1u) * stride;
            for (uint32_t x = 0; x < width; ++x)
            {
                const uint16_t t = nextSrc[x];
                if ((t & 1u) == 0u)
                {
                    sLum[nextIdx][x] = -1;
                    continue;
                }
                const int16_t cr = (int16_t)((t >> 11) & 31u);
                const int16_t cg = (int16_t)((t >> 6) & 31u);
                const int16_t cb = (int16_t)((t >> 1) & 31u);
                sLum[nextIdx][x] = (int16_t)(cr * 2 + cg * 4 + cb);
            }
        }

        uint16_t *const row = buf + y * stride;
        for (uint32_t x = 0; x < width; ++x)
        {
            const uint16_t t = row[x];
            const int16_t cLum = sLum[curIdx][x];
            // Protect dark outlines (cLum < 30) from muddy artifacts
            if ((t & 1u) == 0u || cLum < 30)
                continue;

            const uint32_t xm = (x > 0u) ? (x - 1u) : 0u;
            const uint32_t xp = (x + 1u < width) ? (x + 1u) : (width - 1u);

            const int16_t lNW = (sLum[prevIdx][xm] >= 0) ? sLum[prevIdx][xm] : cLum;
            const int16_t lSE = (sLum[nextIdx][xp] >= 0) ? sLum[nextIdx][xp] : cLum;
            const int16_t lN  = (sLum[prevIdx][x]  >= 0) ? sLum[prevIdx][x]  : cLum;
            const int16_t lS  = (sLum[nextIdx][x]  >= 0) ? sLum[nextIdx][x]  : cLum;
            const int16_t lW  = (sLum[curIdx][xm]  >= 0) ? sLum[curIdx][xm]  : cLum;
            const int16_t lE  = (sLum[curIdx][xp]  >= 0) ? sLum[curIdx][xp]  : cLum;

            const int32_t dir = (int32_t)lNW - (int32_t)lSE;
            const int32_t avg = ((int32_t)lN + (int32_t)lS + (int32_t)lW + (int32_t)lE) >> 2;
            const int32_t cav = (int32_t)cLum - avg;
            int32_t factor = (dir * 5 + cav * 3) >> 2;
            if (factor < -64) factor = -64;
            if (factor > 48)  factor = 48;
            // Damp positive highlight boost on already bright/light regions to prevent garish clipping
            if (factor > 0 && cLum > 175)
                factor = (factor * 2) / 3;
            if (factor == 0)
                continue;

            const int32_t scale = 256 + factor;
            int32_t r = ((int32_t)((t >> 11) & 31u) * scale + 128) >> 8;
            int32_t g = ((int32_t)((t >> 6)  & 31u) * scale + 128) >> 8;
            int32_t b = ((int32_t)((t >> 1)  & 31u) * scale + 128) >> 8;
            r = (r < 0) ? 0 : (r > 31) ? 31 : r;
            g = (g < 0) ? 0 : (g > 31) ? 31 : g;
            b = (b < 0) ? 0 : (b > 31) ? 31 : b;
            row[x] = (uint16_t)((r << 11) | (g << 6) | (b << 1) | (t & 1u));
        }
    }
}
