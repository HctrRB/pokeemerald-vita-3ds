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

// Runs the 240x136 luminance bright-pass into the currently bound bloom FBO.
void VoxelHd2dShader_BrightPass(GLuint sceneTex, float uMax, float vMax, float threshold, float gain);

// Renders bravely-emerald style procedural light shafts into the currently bound 240x136 FBO.
void VoxelHd2dShader_Shafts(float dirX, float dirY, float lenExp, float timeSec,
                            float aspect, float layers, float motion,
                            float colR, float colG, float colB);

// Runs the single-pass 960x544 Uber-Composite (DoF + Battle Dim + Bloom + Reference Split-Tone/S-Curve + Vignette).
// warmKeep (0..1, the night): how much warm-lit pixels keep their warmth instead of the cool split-tone.
void VoxelHd2dShader_Composite(GLuint sceneTex, GLuint blurTex, GLuint bloomTex,
                                float uSceneMax, float vSceneMax,
                                float uBlurMax, float vBlurMax,
                                float uBloomMax, float vBloomMax,
                                float yTop, float yBot, float topAlpha, float botAlpha,
                                float bloomAmt, float gradeAmt, float curveAmt, float vignetteAmt,
                                float saturation, float backBlur, float backDim,
                                float bumpOffsetX, float bumpOffsetY, float bumpStrength,
                                const float shadowTint[3], const float highTint[3], float warmKeep);

#endif // VOXEL_HD2D_SHADER_H
