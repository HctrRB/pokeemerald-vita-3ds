#ifndef VOXEL_HD2D_SHADER_H
#define VOXEL_HD2D_SHADER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef PLATFORM_VITA
#include <vitaGL.h>
#else
#include <GL/gl.h>
#endif

// Initializes the vitashark Bright-Pass + Uber-Composite HD-2D shaders.
// Returns false (safe fallback to fixed-function) if shader compilation is unavailable.
bool VoxelHd2dShader_Init(void);
bool VoxelHd2dShader_Ready(void);
bool VoxelHd2dShader_ShaftsReady(void);

// Runs the 2x2 box-filtered 480x272 downsample into sScratchFbo (eliminates roof/wall banding).
void VoxelHd2dShader_CopyBox2x2(GLuint sceneTex, float uMax, float vMax, float texelX, float texelY);

// The half-size downsample: one tap per pixel with the texture's own filter (with GL_LINEAR, the
// exact 2x2 average of the 4-tap box above for one fetch), then the night / fade overlay
// (rgb, alpha) the world scene left to the post pass.
void VoxelHd2dShader_Downsample(GLuint sceneTex, float uMax, float vMax, const float overlay[4]);

// Runs the 240x136 luminance bright-pass into the currently bound bloom FBO.
void VoxelHd2dShader_BrightPass(GLuint sceneTex, float uMax, float vMax, float threshold, float gain);

// Procedural light shafts, in two steps per frame and pass (0: sun, 1: moon). First, before the
// bloom target is bound: VoxelHd2dShader_BeginShaftLut binds the pass's own small target and
// VoxelHd2dShader_ShaftLut evaluates its beam profile into it. Then, in the 240x136 bloom target,
// VoxelHd2dShader_Shafts draws the pass over the screen (additive blending is the caller's). The
// direction, time, aspect and motion must be the same in both calls.
void VoxelHd2dShader_BeginShaftLut(uint32_t pass);
void VoxelHd2dShader_ShaftLut(uint32_t pass, float dirX, float dirY, float timeSec,
                              float aspect, float layers, float motion);
void VoxelHd2dShader_Shafts(uint32_t pass, float dirX, float dirY, float lenExp, float timeSec,
                            float aspect, float motion,
                            float colR, float colG, float colB);

// Runs the single-pass 960x544 Uber-Composite (DoF + Battle Dim + Bloom + Reference Split-Tone/S-Curve + Vignette).
void VoxelHd2dShader_Composite(GLuint sceneTex, GLuint blurTex, GLuint bloomTex,
                                float uSceneMax, float vSceneMax,
                                float uBlurMax, float vBlurMax,
                                float uBloomMax, float vBloomMax,
                                float yTop, float yBot, float topAlpha, float botAlpha,
                                float bloomAmt, float gradeAmt, float curveAmt, float vignetteAmt,
                                float saturation, float backBlur, float backDim,
                                float bumpOffsetX, float bumpOffsetY, float bumpStrength,
                                const float shadowTint[3], const float highTint[3], const float overlay[4]);

#endif // VOXEL_HD2D_SHADER_H
