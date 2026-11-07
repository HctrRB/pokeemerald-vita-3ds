#ifndef VOXEL_FILE_H
#define VOXEL_FILE_H

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline FILE *VoxelFile_Open(const char *relPath)
{
    char buf[512];
    FILE *f = NULL;

    if (!relPath || !relPath[0])
        return NULL;

    // Skip any leading "romfs:/" or "romfs:" if present
    const char *cleanPath = relPath;
    if (strncmp(cleanPath, "romfs:/", 7) == 0)
        cleanPath += 7;
    else if (strncmp(cleanPath, "romfs:", 6) == 0)
        cleanPath += 6;

    // 1. Packaged application directory on PS Vita (app0:...) - primary asset location
    snprintf(buf, sizeof(buf), "app0:%s", cleanPath);
    f = fopen(buf, "rb");
    if (f) return f;

    // 2. User override directory on PS Vita (ux0:data/POKEVITA/...)
    snprintf(buf, sizeof(buf), "ux0:data/POKEVITA/%s", cleanPath);
    f = fopen(buf, "rb");
    if (f) return f;

    // 3. Assets directory (desktop / local build testing)
    snprintf(buf, sizeof(buf), "assets/%s", cleanPath);
    f = fopen(buf, "rb");
    if (f) return f;

    // 4. Direct relative path
    f = fopen(cleanPath, "rb");
    if (f) return f;

    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // VOXEL_FILE_H
