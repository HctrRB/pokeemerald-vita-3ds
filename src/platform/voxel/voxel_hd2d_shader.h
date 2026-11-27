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

// The bloom's blur in one pass: 8 taps at +-1.25 and +-4.25 texels (texelX/Y: one texel of
// sceneTex in texture coordinates). False when the program is not available.
bool VoxelHd2dShader_Kawase8(GLuint sceneTex, float uMax, float vMax, float texelX, float texelY);

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
// direction, clock and aspect must be the same in both calls.
//
// The shafts' animation at one moment, wrapped on the CPU: the shaders run in half precision, so
// they never see the raw clock. Filled once per frame and pass by VoxelHd2dShader_ShaftClockAt.
typedef struct
{
    float shift[4];     // the layers' shifts: fractions (0..1), then whole-cell bases (0..15)
    float breath[2];    // the layers' breathing phases, radians
    float dust;         // the dust motes' twinkle phase, radians
} VoxelHd2dShaftClock;

void VoxelHd2dShader_ShaftClockAt(double timeSec, float motion, VoxelHd2dShaftClock *out);
void VoxelHd2dShader_BeginShaftLut(uint32_t pass);
void VoxelHd2dShader_ShaftLut(uint32_t pass, float dirX, float dirY, const VoxelHd2dShaftClock *clock,
                              float aspect, float layers);
void VoxelHd2dShader_Shafts(uint32_t pass, float dirX, float dirY, float lenExp,
                            const VoxelHd2dShaftClock *clock, float aspect,
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

// HIGH's packed composite. Everything of the composite that is not full resolution - the DoF
// blur and its weight, the night / fade overlay, the bloom, the battle's back blur and dim - is
// worked out at half resolution by VoxelHd2dShader_Pack into one RGBA8 image: alpha = the
// weight the full-resolution scene keeps, rgb = what is added to it. The full-resolution pass is
// then scene * alpha + rgb followed by the grade (VoxelHd2dShader_CompositePacked); in a 3D
// battle the scene's weight is 0 and only the packed image is read
// (VoxelHd2dShader_CompositePackedBattle).
typedef struct
{
    GLuint srcTex;           // the half-resolution image the DoF blur is taken from
    float srcU, srcV;        // its extent in texture coordinates
    float texelX, texelY;    // one blur step in srcTex coordinates
    GLuint bloomTex;         // the blurred bloom (any texture when bloomAmt is 0)
    float bloomU, bloomV;
    float bloomAmt;
    float uvTop, uvBot;      // where the DoF bands start, in texture y (1 = top of the screen)
    float topAlpha, botAlpha;
    float minBlur;           // floor of the DoF weight: 1 in a 3D battle (the backdrop blurred whole)
    float dim;               // 1 - the battle's back dim; 1 on the field
    float overlay[4];        // night / fade layer (rgb, alpha)
} VoxelHd2dPack;

// The grade of the composite (split-tone, saturation, S-curve, vignette), as the caller decides it.
typedef struct
{
    float gradeAmt, saturation, curveAmt, vignetteAmt;
    float shadowTint[3], highTint[3];
} VoxelHd2dGrade;

// What the field composite draws. READS and COPY exist for the measuring build's bench only: the
// composite's two reads without the grade, and a single read of the scene.
typedef enum
{
    VOXEL_HD2D_COMP_FULL,
    VOXEL_HD2D_COMP_READS,
    VOXEL_HD2D_COMP_COPY,
} VoxelHd2dCompVariant;

bool VoxelHd2dShader_PackReady(void);
// Into the bound half-resolution target, over its viewport.
void VoxelHd2dShader_Pack(const VoxelHd2dPack *pack);
// Into the bound full-resolution target: scene * pack.a + pack.rgb, graded.
void VoxelHd2dShader_CompositePacked(GLuint sceneTex, float sceneU, float sceneV,
                                     GLuint packTex, float packU, float packV,
                                     const VoxelHd2dGrade *grade, VoxelHd2dCompVariant variant);
// The 3D battle's: the packed image alone, graded, with the low-HP edge (0 = none).
void VoxelHd2dShader_CompositePackedBattle(GLuint packTex, float packU, float packV,
                                           const VoxelHd2dGrade *grade, float lowHpEdge);

#endif // VOXEL_HD2D_SHADER_H
