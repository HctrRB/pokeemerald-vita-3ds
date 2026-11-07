#ifndef GUARD_VOXEL_RENDERER_H
#define GUARD_VOXEL_RENDERER_H

#ifdef PLATFORM_SDL2
#if defined(VOXEL_SUPPORTED) || defined(PLATFORM_VITA) || defined(NATIVE_LINUX)

#include <stdint.h>
#include <stdbool.h>

// Initialize the voxel renderer. Call after OpenGL context is created.
// Returns true on success, false on failure.
bool VoxelRenderer_Init(void);

// Render one frame of the 3D voxel world (submits GL commands; call VoxelRenderer_SwapBuffers after releasing VBlank).
// Returns true on success, false if rendering was skipped or locked.
bool VoxelRenderer_RenderFrame(void);

// Present the rendered 3D frame to the display without common-dialog overhead.
void VoxelRenderer_SwapBuffers(void);

// Shut down the voxel renderer and free resources.
void VoxelRenderer_Shutdown(void);

// Notify of animated tiles written to BG VRAM (water, waves, flowers)
void VoxelRenderer_NotifyTilesetAnimWrite(const void *dest, uint32_t bytes);

// External flag set by main() from command-line args or runtime toggle
extern bool gVoxelModeEnabled;
extern bool gVoxelFirstPersonMode;

#endif // VOXEL_SUPPORTED || PLATFORM_VITA || NATIVE_LINUX
#endif // PLATFORM_SDL2
#endif // GUARD_VOXEL_RENDERER_H
