#ifndef GUARD_PLATFORM_VITA_PAK_H
#define GUARD_PLATFORM_VITA_PAK_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Checks whether a follower asset exists inside the packaged PKF1 container.
 * Accepts paths with or without a leading "followers/" prefix.
 */
bool VitaPak_FollowerExists(const char *path);

/**
 * Loads a follower asset from the packaged PKF1 container into a newly
 * malloc'd buffer with an extra trailing NUL byte (matching VitaData_Load).
 * Returns NULL if the container is unavailable or the entry is not found.
 */
void *VitaPak_LoadFollower(const char *path, uint32_t *outSize);

/**
 * Releases the open container file handle and cached table of contents.
 */
void VitaPak_Shutdown(void);

#endif // GUARD_PLATFORM_VITA_PAK_H
