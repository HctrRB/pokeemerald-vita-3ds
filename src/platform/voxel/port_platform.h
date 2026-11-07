#ifndef GUARD_PORT_PLATFORM_H
#define GUARD_PORT_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct MapLayout *const gMapLayouts[];

static inline const void *Port_ResolveAssetPointer(const void *ptr)
{
    return ptr;
}

static inline u32 Port_GetAssetSizeExact(const void *ptr)
{
    (void)ptr;
    return 16384u;
}

static inline const struct MapLayout *Port_GetMapLayoutById(u16 layoutId)
{
    if (layoutId >= 1 && layoutId <= 441)
        return gMapLayouts[layoutId - 1];
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // GUARD_PORT_PLATFORM_H
