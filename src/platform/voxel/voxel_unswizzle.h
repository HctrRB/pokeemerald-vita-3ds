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

#ifdef __cplusplus
}
#endif

#endif // VOXEL_UNSWIZZLE_H
