#ifndef VOXEL_UNSWIZZLE_H
#define VOXEL_UNSWIZZLE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute 8x8 Morton tiled texel index (PICA200 texture format)
static inline uint32_t CtrVideo_Texel(uint32_t x, uint32_t y, uint32_t width)
{
    uint32_t morton = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1)
                    | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
    return (((y >> 3) * (width >> 3)) + (x >> 3)) * 64 + morton;
}

// Convert GBA BGR555 to RGBA5551 (exact match for GL_UNSIGNED_SHORT_5_5_5_1)
static inline uint16_t CtrVideo_RGBA5551(uint16_t color)
{
    return ((color & 31) << 11) | ((color & 0x3e0) << 1)
         | ((color >> 9) & 0x3e) | 1;
}

// GBA OBJ tile addressing helper
static inline uint32_t CtrVideo_ObjTile(uint32_t base, uint32_t x, uint32_t y,
                                        uint32_t width, bool color256, bool mapping1d)
{
    uint32_t units = color256 ? 2 : 1;
    if (color256)
        base &= ~1u;
    return (base + y * (mapping1d ? (width >> 3) * units : 32) + x * units) & 1023;
}

// Fast unswizzle from 8x8 Morton tiled memory to OpenGL linear row-major memory.
// Flips Y so that row 0 in 3DS memory (top) maps to OpenGL V = 1.0 (top).
static inline void Voxel_UnswizzleMortonToLinear(const uint16_t *src, uint16_t *dst,
                                                uint32_t width, uint32_t height)
{
    const uint32_t tilesPerRow = width >> 3;

    for (uint32_t y = 0; y < height; y++)
    {
        const uint32_t glY = height - 1 - y;
        const uint32_t tileY = y >> 3;
        const uint32_t mortonY = ((y & 1) << 1) | ((y & 2) << 2) | ((y & 4) << 3);
        const uint32_t rowBase = (tileY * tilesPerRow) << 6;
        uint16_t *const dstRow = &dst[glY * width];

        for (uint32_t tileX = 0; tileX < tilesPerRow; tileX++)
        {
            const uint16_t *const tileSrc = &src[rowBase + (tileX << 6) + mortonY];
            uint16_t *const tileDst = &dstRow[tileX << 3];
            tileDst[0] = tileSrc[0];
            tileDst[1] = tileSrc[1];
            tileDst[2] = tileSrc[4];
            tileDst[3] = tileSrc[5];
            tileDst[4] = tileSrc[16];
            tileDst[5] = tileSrc[17];
            tileDst[6] = tileSrc[20];
            tileDst[7] = tileSrc[21];
        }
    }
}

// Fast unswizzle of a specific vertical scanline span [startY, startY + numY).
// Used to update only animated tile rows (water waves, flowers) without unswizzling the entire atlas.
static inline void Voxel_UnswizzleMortonToLinearRows(const uint16_t *src, uint16_t *dst,
                                                    uint32_t width, uint32_t height,
                                                    uint32_t startY, uint32_t numY)
{
    const uint32_t tilesPerRow = width >> 3;
    const uint32_t endY = startY + numY;

    for (uint32_t y = startY; y < endY; y++)
    {
        const uint32_t glY = height - 1 - y;
        const uint32_t tileY = y >> 3;
        const uint32_t mortonY = ((y & 1) << 1) | ((y & 2) << 2) | ((y & 4) << 3);
        const uint32_t rowBase = (tileY * tilesPerRow) << 6;
        uint16_t *const dstRow = &dst[glY * width];

        for (uint32_t tileX = 0; tileX < tilesPerRow; tileX++)
        {
            const uint16_t *const tileSrc = &src[rowBase + (tileX << 6) + mortonY];
            uint16_t *const tileDst = &dstRow[tileX << 3];
            tileDst[0] = tileSrc[0];
            tileDst[1] = tileSrc[1];
            tileDst[2] = tileSrc[4];
            tileDst[3] = tileSrc[5];
            tileDst[4] = tileSrc[16];
            tileDst[5] = tileSrc[17];
            tileDst[6] = tileSrc[20];
            tileDst[7] = tileSrc[21];
        }
    }
}

// Fast unswizzle of one rectangle [x0, x0 + w) x [y0, y0 + h); x0 and w are multiples of 8.
// Used for single sprite slots, so a changed frame does not unswizzle its whole row of slots.
static inline void Voxel_UnswizzleMortonToLinearRect(const uint16_t *src, uint16_t *dst,
                                                    uint32_t width, uint32_t height,
                                                    uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
    const uint32_t tilesPerRow = width >> 3;
    const uint32_t firstTile = x0 >> 3;
    const uint32_t endTile = (x0 + w) >> 3;

    for (uint32_t y = y0; y < y0 + h; y++)
    {
        const uint32_t glY = height - 1 - y;
        const uint32_t tileY = y >> 3;
        const uint32_t mortonY = ((y & 1) << 1) | ((y & 2) << 2) | ((y & 4) << 3);
        const uint32_t rowBase = (tileY * tilesPerRow) << 6;
        uint16_t *const dstRow = &dst[glY * width];

        for (uint32_t tileX = firstTile; tileX < endTile; tileX++)
        {
            const uint16_t *const tileSrc = &src[rowBase + (tileX << 6) + mortonY];
            uint16_t *const tileDst = &dstRow[tileX << 3];
            tileDst[0] = tileSrc[0];
            tileDst[1] = tileSrc[1];
            tileDst[2] = tileSrc[4];
            tileDst[3] = tileSrc[5];
            tileDst[4] = tileSrc[16];
            tileDst[5] = tileSrc[17];
            tileDst[6] = tileSrc[20];
            tileDst[7] = tileSrc[21];
        }
    }
}

// Unswizzle with explicit source height and target power-of-two height for OpenGL textures.
// Fills unused rows at the bottom with zeros and aligns row 0 to top (V = 1.0).
static inline void Voxel_UnswizzleMortonToLinearEx(const uint16_t *src, uint16_t *dst,
                                                  uint32_t width, uint32_t srcHeight, uint32_t dstHeight)
{
    memset(dst, 0, width * dstHeight * sizeof(uint16_t));
    const uint32_t tilesPerRow = width >> 3;

    for (uint32_t y = 0; y < srcHeight; y++)
    {
        const uint32_t glY = dstHeight - 1 - y;
        const uint32_t tileY = y >> 3;
        const uint32_t mortonY = ((y & 1) << 1) | ((y & 2) << 2) | ((y & 4) << 3);
        const uint32_t rowBase = (tileY * tilesPerRow) << 6;
        uint16_t *const dstRow = &dst[glY * width];

        for (uint32_t tileX = 0; tileX < tilesPerRow; tileX++)
        {
            const uint16_t *const tileSrc = &src[rowBase + (tileX << 6) + mortonY];
            uint16_t *const tileDst = &dstRow[tileX << 3];
            tileDst[0] = tileSrc[0];
            tileDst[1] = tileSrc[1];
            tileDst[2] = tileSrc[4];
            tileDst[3] = tileSrc[5];
            tileDst[4] = tileSrc[16];
            tileDst[5] = tileSrc[17];
            tileDst[6] = tileSrc[20];
            tileDst[7] = tileSrc[21];
        }
    }
}

// Fast swizzle from linear row-major memory to 8x8 Morton tiled memory
static inline void Voxel_UnswizzleLinearToMorton(const uint16_t *src, uint16_t *dst,
                                                uint32_t width, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            dst[CtrVideo_Texel(x, y, width)] = src[y * width + x];
        }
    }
}

// Unswizzle 8-bit Morton window-glass glow mask into linear RGBA4444 (white RGB, alpha = mask >> 4)
// and optionally populate a 32x32 UV occupancy bitmask (indexed by GL v-row gy in 0..31, bit gx in 0..31).
static inline void Voxel_UnswizzleMortonGlowToRgba4444(const uint8_t *const src, uint16_t *const dst,
                                                       const uint32_t width, const uint32_t srcHeight,
                                                       const uint32_t dstHeight, uint32_t glowGridOut[32])
{
    memset(dst, 0, (size_t)width * dstHeight * sizeof(uint16_t));
    if (glowGridOut != NULL)
        memset(glowGridOut, 0, 32u * sizeof(uint32_t));
    const uint32_t tilesPerRow = width >> 3;

    for (uint32_t y = 0; y < srcHeight; y++)
    {
        const uint32_t glY = dstHeight - 1u - y;
        const uint32_t gy = (glY * 32u) / dstHeight;
        const uint32_t tileY = y >> 3;
        const uint32_t mortonY = ((y & 1u) << 1) | ((y & 2u) << 2) | ((y & 4u) << 3);
        const uint32_t rowBase = (tileY * tilesPerRow) << 6;
        uint16_t *const dstRow = &dst[glY * width];
        uint32_t rowBits = 0u;

        for (uint32_t tileX = 0; tileX < tilesPerRow; tileX++)
        {
            const uint8_t *const tileSrc = &src[rowBase + (tileX << 6) + mortonY];
            const uint32_t baseX = tileX << 3;
            uint16_t *const tileDst = &dstRow[baseX];
            const uint8_t s0 = tileSrc[0],  s1 = tileSrc[1],  s2 = tileSrc[4],  s3 = tileSrc[5];
            const uint8_t s4 = tileSrc[16], s5 = tileSrc[17], s6 = tileSrc[20], s7 = tileSrc[21];
            tileDst[0] = s0 ? (uint16_t)(0xFFF0u | (s0 >> 4)) : 0u;
            tileDst[1] = s1 ? (uint16_t)(0xFFF0u | (s1 >> 4)) : 0u;
            tileDst[2] = s2 ? (uint16_t)(0xFFF0u | (s2 >> 4)) : 0u;
            tileDst[3] = s3 ? (uint16_t)(0xFFF0u | (s3 >> 4)) : 0u;
            tileDst[4] = s4 ? (uint16_t)(0xFFF0u | (s4 >> 4)) : 0u;
            tileDst[5] = s5 ? (uint16_t)(0xFFF0u | (s5 >> 4)) : 0u;
            tileDst[6] = s6 ? (uint16_t)(0xFFF0u | (s6 >> 4)) : 0u;
            tileDst[7] = s7 ? (uint16_t)(0xFFF0u | (s7 >> 4)) : 0u;
            if (glowGridOut != NULL && (s0 | s1 | s2 | s3 | s4 | s5 | s6 | s7) != 0u)
            {
                if (s0) rowBits |= 1u << (((baseX + 0u) * 32u) / width);
                if (s1) rowBits |= 1u << (((baseX + 1u) * 32u) / width);
                if (s2) rowBits |= 1u << (((baseX + 2u) * 32u) / width);
                if (s3) rowBits |= 1u << (((baseX + 3u) * 32u) / width);
                if (s4) rowBits |= 1u << (((baseX + 4u) * 32u) / width);
                if (s5) rowBits |= 1u << (((baseX + 5u) * 32u) / width);
                if (s6) rowBits |= 1u << (((baseX + 6u) * 32u) / width);
                if (s7) rowBits |= 1u << (((baseX + 7u) * 32u) / width);
            }
        }
        if (glowGridOut != NULL && rowBits != 0u)
            glowGridOut[gy] |= rowBits;
    }
}

#ifdef __cplusplus
}
#endif

#endif // VOXEL_UNSWIZZLE_H
