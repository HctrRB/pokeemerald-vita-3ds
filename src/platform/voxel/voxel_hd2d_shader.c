#include "voxel_hd2d_shader.h"
#include "../vita/vita_platform.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * Texture coordinates are worked out in the vertex shaders and read in the fragment shaders as
 * the varyings themselves: the SGX fetches such a texel before the fragment shader runs, while a
 * coordinate computed per pixel (a scale or a tap offset) makes the read dependent and stalls it.
 * Everything here is linear in the quad's coordinates, so moving it to the vertices changes no
 * pixel. The translator ignores precision qualifiers and runs every shader in half precision
 * (vglUseLowPrecision in MainThread_Boot), so the qualifiers below are only GLSL ES syntax.
 */

static bool sTriedInit = false;
static bool sShaderReady = false;

// The unit quad every post pass draws (DrawQuad): position and texture coordinate alike.
static const float kQuad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
static GLuint sQuadVbo = 0;
static void DrawQuad(void);

// 4-tap box: downsample fallback, wide blur and the Kawase passes.
static GLuint sCopyProg = 0;
static GLint sCopyScene = -1, sCopyUvScale = -1, sCopyTexel = -1;

// The bloom's blur in one pass (HIGH): 8 bilinear taps (VoxelHd2dShader_Kawase8).
static GLuint sKawaseProg = 0;
static GLint sKawaseScene = -1, sKawaseUvScale = -1, sKawaseInner = -1, sKawaseOuter = -1;

// One bilinear tap: the 2x2 downsample, with the night / fade overlay (uOverlay) applied.
static GLuint sTapProg = 0;
static GLint sTapScene = -1, sTapUvScale = -1, sTapOverlay = -1;

static GLuint sBrightProg = 0;
static GLint sBUScene = -1, sBUThresh = -1, sBUUvScale = -1, sBUGain = -1;

// Light shafts (VoxelHd2dShader_Shafts). Their costly part - the two hashed layers of beams, the
// profile and the breathing - depends on one screen coordinate only, a, the distance across the
// beams. Each frame it is evaluated once per texel of a SHAFT_LUT_W x 1 target (one per sun /
// moon pass) by the original per-pixel code, then a full-screen quad reads that row at a.
#define SHAFT_PROGRAMS 2
#define SHAFT_LUT_W 512u
// A full row of 32x32 tiles, not a single texel row: every row holds the same profile (the shader
// ignores y) and the quad reads the middle one. A 1-pixel-high render target drew nothing on the
// Vita (the shafts vanished at every hour).
#define SHAFT_LUT_H 32u
// The profile is drawn only over the two middle rows, the pair the shafts' bilinear read at v = 0.5
// blends (it is the middle pair whichever way vitaGL orients the rows). The other rows are never
// read: shading all 32 ran the costly per-pixel shaft code 16 times for nothing.
#define SHAFT_LUT_DRAW_Y (SHAFT_LUT_H / 2u - 1u)
#define SHAFT_LUT_DRAW_H 2u
#define SHAFT_LUTS 2u
typedef struct
{
    GLuint prog;
    GLint uvScale, aRange, shift, breath;
} ShaftLutProgram;
static ShaftLutProgram sShaftLut[SHAFT_PROGRAMS];   // 1 and 2 layers
static GLuint sShaftProg = 0;                       // the full-screen pass
static GLint sShaftLutTexU = -1, sShaftLutMap = -1, sShaftLutV = -1, sShaftDust = -1, sShaftLen = -1, sShaftCol = -1;
static GLuint sShaftLutTex[SHAFT_LUTS], sShaftLutFbo[SHAFT_LUTS];
static bool sShaftReady = false;

// The composite: the field program leaves out the 3D battle terms.
typedef struct
{
    GLuint prog;
    GLint scene, blur, bloom;
    GLint sceneUv, blurUv, bloomUv, dofRamp;
    GLint post, dofAlpha, battle, overlay;
    GLint tint0, tint1, curve;
} CompositeProgram;
static CompositeProgram sCompField, sCompBattle;

// HIGH's bloom source drawn as a band of the half-resolution target (VoxelHd2dShader_BrightPassBox4,
// VoxelHd2dShader_Kawase8Band).
static GLuint sBright4Prog = 0;
static GLint sB4Scene = -1, sB4UvScale = -1, sB4Texel = -1, sB4Overlay = -1, sB4Thresh = -1, sB4Gain = -1;
static GLuint sKawaseBandProg = 0;
static GLint sKBScene = -1, sKBOrigin = -1, sKBUvScale = -1, sKBInner = -1, sKBOuter = -1, sKBClamp = -1;
static bool sBandReady = false;

// The bright pass read straight from the full-resolution scene: at a quarter-size pixel, the 4
// bilinear taps one texel around its centre are the 4x4 box it got through the downsample, then
// the overlay the downsample applies and the same luminance ramp as kBrightFs.
static const char kBright4Fs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform vec4 uOverlay;\n"
    "uniform float uThreshold;\n"
    "uniform float uGain;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vT0).rgb + texture2D(uScene, vT1).rgb;\n"
    "    c += texture2D(uScene, vT2).rgb + texture2D(uScene, vT3).rgb;\n"
    "    c = mix(c * 0.25, uOverlay.rgb, uOverlay.a);\n"
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "    float k = clamp((l - uThreshold) / max(1.0 - uThreshold, 0.05), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(c * (k * (0.75 + 0.25 * k) * uGain), 1.0);\n"
    "}\n";

// The 8-tap bloom blur of kKawaseFs over a band of a larger texture. Each tap is clamped to the
// band's texel centres, which is what CLAMP_TO_EDGE did at the edges of the bloom's own target.
static const char kKawaseBandVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uOrigin;\n"
    "uniform vec2 uUvScale;\n"
    "uniform vec2 uInner;\n"
    "uniform vec2 uOuter;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vT4;\n"
    "varying vec2 vT5;\n"
    "varying vec2 vT6;\n"
    "varying vec2 vT7;\n"
    "void main() {\n"
    "    vec2 uv = uOrigin + aUv * uUvScale;\n"
    "    vT0 = uv + vec2(-uInner.x, -uInner.y);\n"
    "    vT1 = uv + vec2(uInner.x, -uInner.y);\n"
    "    vT2 = uv + vec2(-uInner.x, uInner.y);\n"
    "    vT3 = uv + vec2(uInner.x, uInner.y);\n"
    "    vT4 = uv + vec2(-uOuter.x, -uOuter.y);\n"
    "    vT5 = uv + vec2(uOuter.x, -uOuter.y);\n"
    "    vT6 = uv + vec2(-uOuter.x, uOuter.y);\n"
    "    vT7 = uv + vec2(uOuter.x, uOuter.y);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kKawaseBandFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform vec4 uClamp;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vT4;\n"
    "varying vec2 vT5;\n"
    "varying vec2 vT6;\n"
    "varying vec2 vT7;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, clamp(vT0, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT1, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT2, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT3, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT4, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT5, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT6, uClamp.xy, uClamp.zw)).rgb;\n"
    "    c += texture2D(uScene, clamp(vT7, uClamp.xy, uClamp.zw)).rgb;\n"
    "    gl_FragColor = vec4(c * 0.125, 1.0);\n"
    "}\n";

// HIGH's packed composite (VoxelHd2dShader_Pack, VoxelHd2dShader_CompositePacked*).
typedef struct
{
    GLuint prog;
    GLint src, bloom, uvScale, texel, bloomUv, dofRamp, dofAlpha, overlay, mix;
} PackProgram;
static PackProgram sPack;

typedef struct
{
    GLuint prog;
    GLint scene, pack, sceneUv, packUv, vig;
    GLint tint0, tint1, luma, curve, edge;
} PackedCompositeProgram;
static PackedCompositeProgram sCompPacked;        // field: scene + pack, graded, grid vignette
static PackedCompositeProgram sCompPackedReads;   // bench: the same reads, no grade
static PackedCompositeProgram sCompPackedBattle;  // 3D battle: pack only, per-pixel vignette, edge
static bool sPackReady = false;

// The field composite is drawn as a grid of COMP_GRID_X x COMP_GRID_Y quads so the vignette, a
// function of the screen position only, comes from the vertices: one multiply per pixel instead of
// the distance, clamp and scale. The cells are wide (60 x 60 px), not the screen-spanning slivers
// the SGX sets up again in every tile they touch. Linear between vertices, it departs from the
// per-pixel vignette by about 0.1 % of brightness.
#define COMP_GRID_X 16u
#define COMP_GRID_Y 9u
#define COMP_GRID_VERTS ((COMP_GRID_X + 1u) * (COMP_GRID_Y + 1u))
#define COMP_GRID_INDICES (COMP_GRID_X * COMP_GRID_Y * 6u)
static float sGridVerts[COMP_GRID_VERTS * 2u];
static uint16_t sGridIndices[COMP_GRID_INDICES];
static GLuint sGridVbo = 0, sGridIbo = 0;

// The pack pass (half resolution): the 4-tap DoF blur of the downsample, the DoF weight from its
// two ramps, the overlay and the bloom, folded into what the full-resolution scene keeps (alpha)
// and what is added to it (rgb). From the composite it replaces:
//   c = mix(scene, overlay, overlay.a); c = mix(c, blur, w) * dim; c += bloom * k
//   = scene * (1 - overlay.a) * (1 - w) * dim + [overlay * overlay.a * (1 - w) + blur * w] * dim + bloom * k
// uMix: the weight's floor (back blur in a 3D battle), the dim (1 - back dim), the bloom amount.
static const char kPackVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uUvScale;\n"
    "uniform vec2 uTexel;\n"
    "uniform vec2 uBloomUv;\n"
    "uniform vec3 uDofRamp;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vBloom;\n"
    "varying vec2 vDof;\n"
    "void main() {\n"
    "    vec2 uv = aUv * uUvScale;\n"
    "    vT0 = uv + vec2(-uTexel.x, -uTexel.y);\n"
    "    vT1 = uv + vec2(uTexel.x, -uTexel.y);\n"
    "    vT2 = uv + vec2(-uTexel.x, uTexel.y);\n"
    "    vT3 = uv + vec2(uTexel.x, uTexel.y);\n"
    "    vBloom = aUv * uBloomUv;\n"
    "    vDof = vec2((aUv.y - uDofRamp.x) * uDofRamp.z, (uDofRamp.y - aUv.y) * uDofRamp.z);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kPackFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uSrc;\n"
    "uniform sampler2D uBloom;\n"
    "uniform vec2 uDofAlpha;\n"
    "uniform vec4 uOverlay;\n"
    "uniform vec3 uMix;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vBloom;\n"
    "varying vec2 vDof;\n"
    "void main() {\n"
    "    vec3 b = texture2D(uSrc, vT0).rgb + texture2D(uSrc, vT1).rgb;\n"
    "    b += texture2D(uSrc, vT2).rgb + texture2D(uSrc, vT3).rgb;\n"
    "    vec2 t = clamp(vDof, 0.0, 1.0);\n"
    "    float w = max(max(smoothstep(0.0, 1.0, t.x) * uDofAlpha.x, smoothstep(0.0, 1.0, t.y) * uDofAlpha.y), uMix.x);\n"
    "    float keep = (1.0 - w) * uMix.y;\n"
    "    vec3 add = uOverlay.rgb * (uOverlay.a * keep) + b * (0.25 * w * uMix.y);\n"
    "    add += texture2D(uBloom, vBloom).rgb * uMix.z;\n"
    "    gl_FragColor = vec4(add, (1.0 - uOverlay.a) * keep);\n"
    "}\n";

// Field composite, drawn over the grid: uVig scales the vignette worked out per vertex.
static const char kCompGridVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uSceneUv;\n"
    "uniform vec2 uPackUv;\n"
    "uniform float uVig;\n"
    "varying vec2 vScene;\n"
    "varying vec2 vPack;\n"
    "varying float vVig;\n"
    "void main() {\n"
    "    vScene = aUv * uSceneUv;\n"
    "    vPack = aUv * uPackUv;\n"
    "    vec2 d = (aUv - 0.5) * vec2(1.7647, 1.0);\n"
    "    vVig = 1.0 - clamp(dot(d, d) - 0.12, 0.0, 0.80) * uVig;\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

// The grade of COMP_FS_GRADE without the bloom (already in the pack) and without the vignette.
#define PACKED_FS_GRADE \
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n" \
    "    float tl = clamp(l * 1.6667 - 0.250005, 0.0, 1.0);\n" \
    "    c = clamp(c * (uTint0 + tl * uTint1) + uLuma * l, 0.0, 1.0);\n" \
    "    c *= uCurve.x + c * (uCurve.y + c * uCurve.z);\n"

#define PACKED_FS_GRADE_UNIFORMS \
    "uniform vec3 uTint0;\n" \
    "uniform vec3 uTint1;\n" \
    "uniform float uLuma;\n" \
    "uniform vec3 uCurve;\n"

static const char kCompPackedFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uPack;\n"
    PACKED_FS_GRADE_UNIFORMS
    "varying vec2 vScene;\n"
    "varying vec2 vPack;\n"
    "varying float vVig;\n"
    "void main() {\n"
    "    vec4 p = texture2D(uPack, vPack);\n"
    "    vec3 c = texture2D(uScene, vScene).rgb * p.a + p.rgb;\n"
    PACKED_FS_GRADE
    "    gl_FragColor = vec4(c * vVig, 1.0);\n"
    "}\n";

static const char kCompPackedReadsFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uPack;\n"
    "varying vec2 vScene;\n"
    "varying vec2 vPack;\n"
    "void main() {\n"
    "    vec4 p = texture2D(uPack, vPack);\n"
    "    gl_FragColor = vec4(texture2D(uScene, vScene).rgb * p.a + p.rgb, 1.0);\n"
    "}\n";

// 3D battle: the scene's weight is 0 (the backdrop is blurred whole), so only the pack is read;
// the per-pixel vignette stays, as the low-HP edge needs the same distance.
static const char kCompPackedBattleVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uPackUv;\n"
    "varying vec2 vPack;\n"
    "varying vec2 vVig;\n"
    "void main() {\n"
    "    vPack = aUv * uPackUv;\n"
    "    vVig = (aUv - 0.5) * vec2(1.7647, 1.0);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kCompPackedBattleFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uPack;\n"
    PACKED_FS_GRADE_UNIFORMS
    "uniform vec2 uEdge;\n"
    "varying vec2 vPack;\n"
    "varying vec2 vVig;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uPack, vPack).rgb;\n"
    PACKED_FS_GRADE
    "    float vd = dot(vVig, vVig);\n"
    "    c *= 1.0 - clamp(vd - 0.12, 0.0, 0.80) * uEdge.x;\n"
    "    float edge = clamp(vd - 0.16, 0.0, 0.85) * uEdge.y;\n"
    "    c = mix(c, c * vec3(1.0, 0.35, 0.30) + vec3(0.24, 0.0, 0.02), edge);\n"
    "    gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
    "}\n";

static const char kCopyVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uUvScale;\n"
    "uniform vec2 uTexel;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "void main() {\n"
    "    vec2 uv = aUv * uUvScale;\n"
    "    vT0 = uv + vec2(-uTexel.x, -uTexel.y);\n"
    "    vT1 = uv + vec2(uTexel.x, -uTexel.y);\n"
    "    vT2 = uv + vec2(-uTexel.x, uTexel.y);\n"
    "    vT3 = uv + vec2(uTexel.x, uTexel.y);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kCopyFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vT0).rgb;\n"
    "    c += texture2D(uScene, vT1).rgb;\n"
    "    c += texture2D(uScene, vT2).rgb;\n"
    "    c += texture2D(uScene, vT3).rgb;\n"
    "    gl_FragColor = vec4(c * 0.25, 1.0);\n"
    "}\n";

// The two Kawase passes the bloom used to take (4 diagonal taps at 1.5 texels, then 4 at 2.75)
// sample, together, a 4x4 grid at +-1.25 and +-4.25 texels. One pass keeps its 4 inner and 4 outer
// corners at equal weights: the same reach, one render target less.
static const char kKawaseVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uUvScale;\n"
    "uniform vec2 uInner;\n"
    "uniform vec2 uOuter;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vT4;\n"
    "varying vec2 vT5;\n"
    "varying vec2 vT6;\n"
    "varying vec2 vT7;\n"
    "void main() {\n"
    "    vec2 uv = aUv * uUvScale;\n"
    "    vT0 = uv + vec2(-uInner.x, -uInner.y);\n"
    "    vT1 = uv + vec2(uInner.x, -uInner.y);\n"
    "    vT2 = uv + vec2(-uInner.x, uInner.y);\n"
    "    vT3 = uv + vec2(uInner.x, uInner.y);\n"
    "    vT4 = uv + vec2(-uOuter.x, -uOuter.y);\n"
    "    vT5 = uv + vec2(uOuter.x, -uOuter.y);\n"
    "    vT6 = uv + vec2(-uOuter.x, uOuter.y);\n"
    "    vT7 = uv + vec2(uOuter.x, uOuter.y);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kKawaseFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "varying vec2 vT0;\n"
    "varying vec2 vT1;\n"
    "varying vec2 vT2;\n"
    "varying vec2 vT3;\n"
    "varying vec2 vT4;\n"
    "varying vec2 vT5;\n"
    "varying vec2 vT6;\n"
    "varying vec2 vT7;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vT0).rgb + texture2D(uScene, vT1).rgb;\n"
    "    c += texture2D(uScene, vT2).rgb + texture2D(uScene, vT3).rgb;\n"
    "    c += texture2D(uScene, vT4).rgb + texture2D(uScene, vT5).rgb;\n"
    "    c += texture2D(uScene, vT6).rgb + texture2D(uScene, vT7).rgb;\n"
    "    gl_FragColor = vec4(c * 0.125, 1.0);\n"
    "}\n";

static const char kTapVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uUvScale;\n"
    "varying vec2 vT0;\n"
    "void main() {\n"
    "    vT0 = aUv * uUvScale;\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kTapFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform vec4 uOverlay;\n"
    "varying vec2 vT0;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(mix(texture2D(uScene, vT0).rgb, uOverlay.rgb, uOverlay.a), 1.0);\n"
    "}\n";

static const char kBrightFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform float uThreshold;\n"
    "uniform float uGain;\n"
    "varying vec2 vT0;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vT0).rgb;\n"
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "    float k = clamp((l - uThreshold) / max(1.0 - uThreshold, 0.05), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(c * (k * (0.75 + 0.25 * k) * uGain), 1.0);\n"
    "}\n";

// Procedural HD-2D light shafts at quarter-res (adapted from bravely-emerald kShaftFs), in two steps.
// 1. The beam profile clamp(acc * 0.78) across the beams, one texel per sample of a: the original
//    per-pixel code (same hashes, same half precision), stored over two 8-bit
//    channels (value = r + g / 255) so the row keeps more than 8 bits through the bilinear read.
//    The time terms come wrapped from the CPU (VoxelHd2dShaftClock): a layer's position is
//    a * scale + fraction, small in magnitude, and its beam index (cell + base) mod SHAFT_CELLS,
//    so the pattern repeats every SHAFT_CELLS cells and the wrap of the base is seamless.
#define SHAFT_LUT_FS_HEAD \
    "precision mediump float;\n" \
    "uniform vec2 uARange;\n" \
    "uniform vec4 uShift;\n" \
    "uniform vec2 uBreath;\n" \
    "varying vec2 vT0;\n" \
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n" \
    "float Layer(float pos, float base, float fi, float phase, float weight) {\n" \
    "    float cell = floor(pos);\n" \
    "    float fr = pos - cell;\n" \
    "    float id = mod(cell + base, 16.0);\n" \
    "    float h1 = hh(vec2(id, fi));\n" \
    "    float h2 = hh(vec2(id + 7.0, fi * 3.0));\n" \
    "    float width = 0.06 + 0.14 * h2;\n" \
    "    float prof = step(0.40, h1) * (1.0 - smoothstep(0.0, width, abs(fr - (0.25 + 0.5 * h1))));\n" \
    "    float breath = 0.55 + 0.45 * sin(phase + h1 * 6.2831);\n" \
    "    return prof * breath * weight;\n" \
    "}\n" \
    "void main() {\n" \
    "    float a = mix(uARange.x, uARange.y, vT0.x);\n"

#define SHAFT_LUT_FS_TAIL \
    "    float v = clamp(acc * 0.78, 0.0, 1.0);\n" \
    "    float hi = floor(v * 255.0) / 255.0;\n" \
    "    gl_FragColor = vec4(hi, (v - hi) * 255.0, 0.0, 1.0);\n" \
    "}\n"

// Layer i: pos = a * (1.1 + 0.55 i) + shift_i, weight 1 - 0.07 i (speeds: VoxelHd2dShader_ShaftClockAt).
static const char kShaftLutFs1[] = SHAFT_LUT_FS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, uShift.z, 0.0, uBreath.x, 1.0);\n"
    SHAFT_LUT_FS_TAIL;
static const char kShaftLutFs2[] = SHAFT_LUT_FS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, uShift.z, 0.0, uBreath.x, 1.0)"
    " + Layer(a * 1.65 + uShift.y, uShift.w, 1.0, uBreath.y, 0.93);\n"
    SHAFT_LUT_FS_TAIL;

// 2. The full-screen pass: the profile read at a (linear in screen position, so a varying: a
//    non-dependent read), the fall towards the bottom, then the dust motes where there is a beam.
static const char kShaftVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec3 uLutMap;\n"
    "uniform float uLutV;\n"
    "varying vec2 vUv;\n"
    "varying vec2 vLut;\n"
    "void main() {\n"
    "    vLut = vec2(aUv.x * uLutMap.x + aUv.y * uLutMap.y + uLutMap.z, uLutV);\n"
    "    vUv = aUv;\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kShaftFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uLut;\n"
    "uniform float uDust;\n"
    "uniform float uLen;\n"
    "uniform vec3 uCol;\n"
    "varying vec2 vUv;\n"
    "varying vec2 vLut;\n"
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n"
    "void main() {\n"
    "    vec2 lut = texture2D(uLut, vLut).rg;\n"
    "    float beam = (lut.r + lut.g * (1.0 / 255.0)) * pow(clamp(vUv.y, 0.0, 1.0), uLen);\n"
    "    float dust = 0.0;\n"
    "    if (beam > 0.0) {\n"
    "        float cellHash = hh(floor(vUv * vec2(120.0, 68.0)));\n"
    "        float mote = 0.5 + 0.5 * sin(uDust + cellHash * 6.2831);\n"
    "        dust = step(0.96, cellHash) * mote * beam * 1.1;\n"
    "    }\n"
    "    gl_FragColor = vec4(uCol * (beam + dust), 1.0);\n"
    "}\n";

// The composite's vertex shader: the three texture coordinates, the two DoF ramps before their
// clamp (linear in y) and the vignette vector (linear in x and y).
static const char kCompVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec2 uSceneUv;\n"
    "uniform vec2 uBlurUv;\n"
    "uniform vec2 uBloomUv;\n"
    "uniform vec3 uDofRamp;\n"
    "varying vec2 vScene;\n"
    "varying vec2 vBlur;\n"
    "varying vec2 vBloom;\n"
    "varying vec2 vDof;\n"
    "varying vec2 vVig;\n"
    "void main() {\n"
    "    vScene = aUv * uSceneUv;\n"
    "    vBlur = aUv * uBlurUv;\n"
    "    vBloom = aUv * uBloomUv;\n"
    "    vDof = vec2((aUv.y - uDofRamp.x) * uDofRamp.z, (uDofRamp.y - aUv.y) * uDofRamp.z);\n"
    "    vVig = (aUv - 0.5) * vec2(1.7647, 1.0);\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

// DoF tilt-shift + Bloom + Split-Tone + S-Curve + Vignette in one pass, so the whole image
// (blurred bands included) gets the grade and the vignette. The split-tone and saturation
//   mix(c, mix(vec3(l), c * mix(shadowTint, highTint, tl), saturation), grade)
// reduce to c * (uTint0 + tl * uTint1) + uPost.y * l, every factor fixed for the frame
// (VoxelHd2dShader_Composite). The S-curve c + k * (c*c*(3 - 2c) - c) is the cubic
// c * (uCurve.x + c * (uCurve.y + c * uCurve.z)) with uCurve = (1 - k, 3k, -2k): three
// multiply-adds instead of five operations. uPost: bloom, that luma factor, unused, vignette. uDofAlpha:
// top and bottom band alpha. uOverlay: the night / fade layer the world scene leaves to this
// pass. The field program needs no final clamp: after the clamped grade, the S-curve and the
// vignette keep every channel in [0, 1]. The battle variant adds uBattle: back blur,
// 1 - back dim, low-HP edge (which can push past 1, so it keeps its clamp).
#define COMP_FS_HEAD \
    "precision mediump float;\n" \
    "uniform sampler2D uScene;\n" \
    "uniform sampler2D uBlur;\n" \
    "uniform sampler2D uBloom;\n" \
    "uniform vec4 uPost;\n" \
    "uniform vec2 uDofAlpha;\n" \
    "uniform vec4 uOverlay;\n" \
    "uniform vec3 uTint0;\n" \
    "uniform vec3 uTint1;\n" \
    "uniform vec3 uCurve;\n" \
    "varying vec2 vScene;\n" \
    "varying vec2 vBlur;\n" \
    "varying vec2 vBloom;\n" \
    "varying vec2 vDof;\n" \
    "varying vec2 vVig;\n"

#define COMP_FS_DOF \
    "    vec3 c = mix(texture2D(uScene, vScene).rgb, uOverlay.rgb, uOverlay.a);\n" \
    "    vec3 b = texture2D(uBlur, vBlur).rgb;\n" \
    "    vec2 t = clamp(vDof, 0.0, 1.0);\n" \
    "    float dofW = max(smoothstep(0.0, 1.0, t.x) * uDofAlpha.x, smoothstep(0.0, 1.0, t.y) * uDofAlpha.y);\n"

#define COMP_FS_GRADE \
    "    c += texture2D(uBloom, vBloom).rgb * uPost.x;\n" \
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n" \
    "    float tl = clamp(l * 1.6667 - 0.250005, 0.0, 1.0);\n" \
    "    c = clamp(c * (uTint0 + tl * uTint1) + uPost.y * l, 0.0, 1.0);\n" \
    "    c *= uCurve.x + c * (uCurve.y + c * uCurve.z);\n" \
    "    float vd = dot(vVig, vVig);\n" \
    "    c *= 1.0 - clamp(vd - 0.12, 0.0, 0.80) * uPost.w;\n"

static const char kCompFieldFs[] = COMP_FS_HEAD
    "void main() {\n"
    COMP_FS_DOF
    "    c = mix(c, b, dofW);\n"
    COMP_FS_GRADE
    "    gl_FragColor = vec4(c, 1.0);\n"
    "}\n";

static const char kCompBattleFs[] = COMP_FS_HEAD
    "uniform vec3 uBattle;\n"
    "void main() {\n"
    COMP_FS_DOF
    "    c = mix(c, b, max(dofW, uBattle.x)) * uBattle.y;\n"
    COMP_FS_GRADE
    "    float edge = clamp(vd - 0.16, 0.0, 0.85) * uBattle.z;\n"
    "    c = mix(c, c * vec3(1.0, 0.35, 0.30) + vec3(0.24, 0.0, 0.02), edge);\n"
    "    gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);\n"
    "}\n";

static GLuint CompileShader(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    if (!sh) return 0;
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[256] = {0};
        glGetShaderInfoLog(sh, sizeof(log) - 1, NULL, log);
        VITA_LOG("[HD2D SHADER] Compile failed: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint LinkProgram(const char *vsSrc, const char *fsSrc)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aUv");
    glBindAttribLocation(prog, 2, "aCol");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[256] = {0};
        glGetProgramInfoLog(prog, sizeof(log) - 1, NULL, log);
        VITA_LOG("[HD2D SHADER] Link failed: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static bool InitComposite(CompositeProgram *const p, const char *const fsSrc)
{
    p->prog = LinkProgram(kCompVs, fsSrc);
    if (p->prog == 0)
        return false;
    p->scene      = glGetUniformLocation(p->prog, "uScene");
    p->blur       = glGetUniformLocation(p->prog, "uBlur");
    p->bloom      = glGetUniformLocation(p->prog, "uBloom");
    p->sceneUv    = glGetUniformLocation(p->prog, "uSceneUv");
    p->blurUv     = glGetUniformLocation(p->prog, "uBlurUv");
    p->bloomUv    = glGetUniformLocation(p->prog, "uBloomUv");
    p->dofRamp    = glGetUniformLocation(p->prog, "uDofRamp");
    p->post       = glGetUniformLocation(p->prog, "uPost");
    p->dofAlpha   = glGetUniformLocation(p->prog, "uDofAlpha");
    p->battle     = glGetUniformLocation(p->prog, "uBattle");   // -1 in the field program
    p->overlay    = glGetUniformLocation(p->prog, "uOverlay");
    p->tint0      = glGetUniformLocation(p->prog, "uTint0");
    p->tint1      = glGetUniformLocation(p->prog, "uTint1");
    p->curve      = glGetUniformLocation(p->prog, "uCurve");
    return true;
}

static bool InitShaftLut(ShaftLutProgram *const p, const char *const fsSrc)
{
    p->prog = LinkProgram(kTapVs, fsSrc);
    if (p->prog == 0)
        return false;
    p->uvScale = glGetUniformLocation(p->prog, "uUvScale");   // kTapVs
    p->aRange = glGetUniformLocation(p->prog, "uARange");
    p->shift  = glGetUniformLocation(p->prog, "uShift");
    p->breath = glGetUniformLocation(p->prog, "uBreath");
    return true;
}

// The shafts' profiles: one SHAFT_LUT_W x SHAFT_LUT_H target per pass, filtered (read between
// samples of a). False when vitaGL cannot render into them: the caller then keeps the 3D rays.
// A target each, not two rows of one, so nothing depends on how vitaGL orients FBO rows.
static bool InitShaftTarget(void)
{
    for (uint32_t i = 0; i < SHAFT_LUTS; ++i)
    {
        glGenTextures(1, &sShaftLutTex[i]);
        if (sShaftLutTex[i] == 0)
            return false;
        glBindTexture(GL_TEXTURE_2D, sShaftLutTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHAFT_LUT_W, SHAFT_LUT_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenFramebuffers(1, &sShaftLutFbo[i]);
        if (sShaftLutFbo[i] == 0)
            return false;
        glBindFramebuffer(GL_FRAMEBUFFER, sShaftLutFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sShaftLutTex[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            VITA_LOG("[HD2D SHADER] Shaft profile target %u incomplete\n", (unsigned)i);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

static bool InitPackedComposite(PackedCompositeProgram *const p, const char *const vsSrc, const char *const fsSrc)
{
    p->prog = LinkProgram(vsSrc, fsSrc);
    if (p->prog == 0)
        return false;
    p->scene   = glGetUniformLocation(p->prog, "uScene");    // -1 in the battle program
    p->pack    = glGetUniformLocation(p->prog, "uPack");
    p->sceneUv = glGetUniformLocation(p->prog, "uSceneUv");  // -1 in the battle program
    p->packUv  = glGetUniformLocation(p->prog, "uPackUv");
    p->vig     = glGetUniformLocation(p->prog, "uVig");      // -1 in the reads and battle programs
    p->tint0   = glGetUniformLocation(p->prog, "uTint0");    // -1 in the reads program, as below
    p->tint1   = glGetUniformLocation(p->prog, "uTint1");
    p->luma    = glGetUniformLocation(p->prog, "uLuma");
    p->curve   = glGetUniformLocation(p->prog, "uCurve");
    p->edge    = glGetUniformLocation(p->prog, "uEdge");     // battle program only
    return true;
}

// The composite grid (COMP_GRID_X x COMP_GRID_Y quads over 0..1), in static buffers. With either
// buffer missing the grid is drawn from the client arrays.
static void InitCompositeGrid(void)
{
    uint32_t v = 0;
    for (uint32_t y = 0; y <= COMP_GRID_Y; ++y)
    {
        for (uint32_t x = 0; x <= COMP_GRID_X; ++x)
        {
            sGridVerts[v++] = (float)x / (float)COMP_GRID_X;
            sGridVerts[v++] = (float)y / (float)COMP_GRID_Y;
        }
    }
    uint32_t i = 0;
    for (uint32_t y = 0; y < COMP_GRID_Y; ++y)
    {
        for (uint32_t x = 0; x < COMP_GRID_X; ++x)
        {
            const uint16_t a = (uint16_t)(y * (COMP_GRID_X + 1u) + x);
            const uint16_t b = (uint16_t)(a + 1u);
            const uint16_t c = (uint16_t)(a + COMP_GRID_X + 1u);
            const uint16_t d = (uint16_t)(c + 1u);
            sGridIndices[i++] = a;
            sGridIndices[i++] = b;
            sGridIndices[i++] = d;
            sGridIndices[i++] = a;
            sGridIndices[i++] = d;
            sGridIndices[i++] = c;
        }
    }

    glGenBuffers(1, &sGridVbo);
    glGenBuffers(1, &sGridIbo);
    if (sGridVbo == 0 || sGridIbo == 0)
    {
        if (sGridVbo != 0)
            glDeleteBuffers(1, &sGridVbo);
        if (sGridIbo != 0)
            glDeleteBuffers(1, &sGridIbo);
        sGridVbo = 0;
        sGridIbo = 0;
        VITA_LOG("[HD2D SHADER] Composite grid buffers not created, drawn from client arrays\n");
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, sGridVbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(sGridVerts), sGridVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sGridIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sizeof(sGridIndices), sGridIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// The packed composite's programs. False leaves HIGH on the unpacked composite.
static bool InitPacked(void)
{
    sPack.prog = LinkProgram(kPackVs, kPackFs);
    if (sPack.prog == 0)
        return false;
    sPack.src      = glGetUniformLocation(sPack.prog, "uSrc");
    sPack.bloom    = glGetUniformLocation(sPack.prog, "uBloom");
    sPack.uvScale  = glGetUniformLocation(sPack.prog, "uUvScale");
    sPack.texel    = glGetUniformLocation(sPack.prog, "uTexel");
    sPack.bloomUv  = glGetUniformLocation(sPack.prog, "uBloomUv");
    sPack.dofRamp  = glGetUniformLocation(sPack.prog, "uDofRamp");
    sPack.dofAlpha = glGetUniformLocation(sPack.prog, "uDofAlpha");
    sPack.overlay  = glGetUniformLocation(sPack.prog, "uOverlay");
    sPack.mix      = glGetUniformLocation(sPack.prog, "uMix");
    if (!InitPackedComposite(&sCompPacked, kCompGridVs, kCompPackedFs)
     || !InitPackedComposite(&sCompPackedReads, kCompGridVs, kCompPackedReadsFs)
     || !InitPackedComposite(&sCompPackedBattle, kCompPackedBattleVs, kCompPackedBattleFs))
        return false;
    InitCompositeGrid();
    return true;
}

bool VoxelHd2dShader_Init(void)
{
    if (sTriedInit)
        return sShaderReady;
    sTriedInit = true;

#ifdef PLATFORM_VITA
    // Filled once and never rewritten (rewriting a vitaGL buffer the GPU still reads copies it).
    glGenBuffers(1, &sQuadVbo);
    if (sQuadVbo != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, sQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(kQuad), kQuad, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        VITA_LOG("[HD2D SHADER] Quad buffer not created, post passes draw from a client array\n");
    }
    sCopyProg   = LinkProgram(kCopyVs, kCopyFs);
    sKawaseProg = LinkProgram(kKawaseVs, kKawaseFs);
    if (sKawaseProg)
    {
        sKawaseScene   = glGetUniformLocation(sKawaseProg, "uScene");
        sKawaseUvScale = glGetUniformLocation(sKawaseProg, "uUvScale");
        sKawaseInner   = glGetUniformLocation(sKawaseProg, "uInner");
        sKawaseOuter   = glGetUniformLocation(sKawaseProg, "uOuter");
    }
    sTapProg    = LinkProgram(kTapVs, kTapFs);
    sBrightProg = LinkProgram(kTapVs, kBrightFs);
    const bool compOk = InitComposite(&sCompField, kCompFieldFs) && InitComposite(&sCompBattle, kCompBattleFs);
    if (sCopyProg && sTapProg && sBrightProg && compOk)
    {
        sCopyScene   = glGetUniformLocation(sCopyProg, "uScene");
        sCopyUvScale = glGetUniformLocation(sCopyProg, "uUvScale");
        sCopyTexel   = glGetUniformLocation(sCopyProg, "uTexel");

        sTapScene    = glGetUniformLocation(sTapProg, "uScene");
        sTapUvScale  = glGetUniformLocation(sTapProg, "uUvScale");
        sTapOverlay  = glGetUniformLocation(sTapProg, "uOverlay");

        sBUScene   = glGetUniformLocation(sBrightProg, "uScene");
        sBUThresh  = glGetUniformLocation(sBrightProg, "uThreshold");
        sBUUvScale = glGetUniformLocation(sBrightProg, "uUvScale");
        sBUGain    = glGetUniformLocation(sBrightProg, "uGain");
        sShaderReady = true;
        VITA_LOG("[HD2D SHADER] Bright-Pass + Uber-Composite compiled successfully.\n");
    }
    sShaftProg = LinkProgram(kShaftVs, kShaftFs);
    if (sShaftProg && InitShaftLut(&sShaftLut[0], kShaftLutFs1) && InitShaftLut(&sShaftLut[1], kShaftLutFs2)
     && InitShaftTarget())
    {
        sShaftLutTexU = glGetUniformLocation(sShaftProg, "uLut");
        sShaftLutMap  = glGetUniformLocation(sShaftProg, "uLutMap");
        sShaftLutV    = glGetUniformLocation(sShaftProg, "uLutV");
        sShaftDust    = glGetUniformLocation(sShaftProg, "uDust");
        sShaftLen     = glGetUniformLocation(sShaftProg, "uLen");
        sShaftCol     = glGetUniformLocation(sShaftProg, "uCol");
        sShaftReady = true;
        VITA_LOG("[HD2D SHADER] Procedural light shafts compiled successfully.\n");
    }
    sPackReady = sShaderReady && InitPacked();
    sBright4Prog = LinkProgram(kCopyVs, kBright4Fs);
    sKawaseBandProg = LinkProgram(kKawaseBandVs, kKawaseBandFs);
    if (sBright4Prog && sKawaseBandProg)
    {
        sB4Scene   = glGetUniformLocation(sBright4Prog, "uScene");
        sB4UvScale = glGetUniformLocation(sBright4Prog, "uUvScale");   // kCopyVs
        sB4Texel   = glGetUniformLocation(sBright4Prog, "uTexel");     // kCopyVs
        sB4Overlay = glGetUniformLocation(sBright4Prog, "uOverlay");
        sB4Thresh  = glGetUniformLocation(sBright4Prog, "uThreshold");
        sB4Gain    = glGetUniformLocation(sBright4Prog, "uGain");
        sKBScene   = glGetUniformLocation(sKawaseBandProg, "uScene");
        sKBOrigin  = glGetUniformLocation(sKawaseBandProg, "uOrigin");
        sKBUvScale = glGetUniformLocation(sKawaseBandProg, "uUvScale");
        sKBInner   = glGetUniformLocation(sKawaseBandProg, "uInner");
        sKBOuter   = glGetUniformLocation(sKawaseBandProg, "uOuter");
        sKBClamp   = glGetUniformLocation(sKawaseBandProg, "uClamp");
        sBandReady = sShaderReady && sShaftReady;
    }
    if (!sBandReady)
        VITA_LOG("[HD2D SHADER] Band passes not available, HIGH keeps the separate bloom and shaft targets\n");
    if (!sPackReady)
        VITA_LOG("[HD2D SHADER] Packed composite not available, HIGH uses the full composite\n");
#endif
    return sShaderReady;
}

bool VoxelHd2dShader_PackReady(void)
{
    return sPackReady;
}

bool VoxelHd2dShader_BandReady(void)
{
    return sBandReady;
}

void VoxelHd2dShader_BrightPassBox4(const GLuint sceneTex, const float uMax, const float vMax,
                                    const float texelX, const float texelY, const float overlay[4],
                                    const float threshold, const float gain)
{
    if (!sBandReady)
        return;
    glUseProgram(sBright4Prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sB4Scene, 0);
    glUniform2f(sB4UvScale, uMax, vMax);
    glUniform2f(sB4Texel, texelX, texelY);
    glUniform4fv(sB4Overlay, 1, overlay);
    glUniform1f(sB4Thresh, threshold);
    glUniform1f(sB4Gain, gain);
    DrawQuad();
    glUseProgram(0);
}

bool VoxelHd2dShader_Kawase8Band(const GLuint tex, const float u0, const float v0, const float uSize, const float vSize,
                                 const float texelX, const float texelY)
{
    if (!sBandReady)
        return false;
    glUseProgram(sKawaseBandProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(sKBScene, 0);
    glUniform2f(sKBOrigin, u0, v0);
    glUniform2f(sKBUvScale, uSize, vSize);
    glUniform2f(sKBInner, 1.25f * texelX, 1.25f * texelY);
    glUniform2f(sKBOuter, 4.25f * texelX, 4.25f * texelY);
    glUniform4f(sKBClamp, u0 + 0.5f * texelX, v0 + 0.5f * texelY,
                u0 + uSize - 0.5f * texelX, v0 + vSize - 0.5f * texelY);
    DrawQuad();
    glUseProgram(0);
    return true;
}

bool VoxelHd2dShader_Ready(void)
{
    return sShaderReady;
}

bool VoxelHd2dShader_ShaftsReady(void)
{
    return sShaftReady;
}

// One full-screen quad with the current program; attribute 0 = position, 1 = texture coordinate.
// It comes from sQuadVbo, filled once at init: client arrays are copied into vitaGL's pool on
// every draw. Without the buffer (creation failed) the client array is used as before. The array
// buffer binding is put back to 0, as the fixed-function draws around these passes expect.
static void DrawQuad(void)
{
    const void *const base = (sQuadVbo != 0) ? NULL : (const void *)kQuad;

    if (sQuadVbo != 0)
        glBindBuffer(GL_ARRAY_BUFFER, sQuadVbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, base);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, base);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    if (sQuadVbo != 0)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool VoxelHd2dShader_Kawase8(const GLuint sceneTex, const float uMax, const float vMax,
                             const float texelX, const float texelY)
{
    if (!sShaderReady || sKawaseProg == 0)
        return false;

    glUseProgram(sKawaseProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sKawaseScene, 0);
    glUniform2f(sKawaseUvScale, uMax, vMax);
    glUniform2f(sKawaseInner, 1.25f * texelX, 1.25f * texelY);
    glUniform2f(sKawaseOuter, 4.25f * texelX, 4.25f * texelY);
    DrawQuad();
    glUseProgram(0);
    return true;
}

void VoxelHd2dShader_CopyBox2x2(const GLuint sceneTex, const float uMax, const float vMax,
                                const float texelX, const float texelY)
{
    if (!sShaderReady) return;

    glUseProgram(sCopyProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sCopyScene, 0);
    glUniform2f(sCopyUvScale, uMax, vMax);
    glUniform2f(sCopyTexel, texelX, texelY);
    DrawQuad();
    glUseProgram(0);
}

void VoxelHd2dShader_Downsample(const GLuint sceneTex, const float uMax, const float vMax, const float overlay[4])
{
    if (!sShaderReady) return;

    glUseProgram(sTapProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sTapScene, 0);
    glUniform2f(sTapUvScale, uMax, vMax);
    glUniform4fv(sTapOverlay, 1, overlay);
    DrawQuad();
    glUseProgram(0);
}

void VoxelHd2dShader_BrightPass(const GLuint sceneTex, const float uMax, const float vMax, const float threshold, const float gain)
{
    if (!sShaderReady) return;

    glUseProgram(sBrightProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sBUScene, 0);
    glUniform1f(sBUThresh, threshold);
    glUniform2f(sBUUvScale, uMax, vMax);
    glUniform1f(sBUGain, gain);
    DrawQuad();
    glUseProgram(0);
}

// The grade's per-frame factors (COMP_FS_HEAD): c * (tint0 + tl * tint1) + luma * l, and the
// S-curve's cubic.
typedef struct
{
    float tint0[3], tint1[3];
    float luma;
    float curve[3];
} GradeFactors;

static void GradeFactorsOf(const float gradeAmt, const float saturation, const float curveAmt,
                           const float shadowTint[3], const float highTint[3], GradeFactors *const out)
{
    const float keep = 1.0f - gradeAmt;
    const float chroma = gradeAmt * saturation;
    for (int32_t k = 0; k < 3; ++k)
    {
        out->tint0[k] = keep + chroma * shadowTint[k];
        out->tint1[k] = chroma * (highTint[k] - shadowTint[k]);
    }
    out->luma = gradeAmt * (1.0f - saturation);
    out->curve[0] = 1.0f - curveAmt;
    out->curve[1] = 3.0f * curveAmt;
    out->curve[2] = -2.0f * curveAmt;
}

void VoxelHd2dShader_Composite(const GLuint sceneTex, const GLuint blurTex, const GLuint bloomTex,
                                const float uSceneMax, const float vSceneMax,
                                const float uBlurMax, const float vBlurMax,
                                const float uBloomMax, const float vBloomMax,
                                const float yTop, const float yBot, const float topAlpha, const float botAlpha,
                                const float bloomAmt, const float gradeAmt, const float curveAmt, const float vignetteAmt,
                                const float saturation, const float backBlur, const float backDim,
                                const float bumpOffsetX, const float bumpOffsetY, const float bumpStrength,
                                const float shadowTint[3], const float highTint[3], const float overlay[4])
{
    if (!sShaderReady) return;
    (void)bumpOffsetX; (void)bumpOffsetY;

    GradeFactors gf;
    GradeFactorsOf(gradeAmt, saturation, curveAmt, shadowTint, highTint, &gf);

    // The field never has the battle terms: they would cost every pixel for a no-op.
    const bool battle = (backBlur > 0.0f || backDim > 0.0f || bumpStrength > 0.0f);
    const CompositeProgram *const p = battle ? &sCompBattle : &sCompField;
    const float invSlope = (yTop < 0.98f) ? 2.8f : 0.0f;

    glUseProgram(p->prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(p->scene, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex);
    glUniform1i(p->blur, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, bloomTex);
    glUniform1i(p->bloom, 2);

    glUniform2f(p->sceneUv, uSceneMax, vSceneMax);
    glUniform2f(p->blurUv,  uBlurMax,  vBlurMax);
    glUniform2f(p->bloomUv, uBloomMax, vBloomMax);
    glUniform3f(p->dofRamp, yTop, yBot, invSlope);
    glUniform4f(p->post,     bloomAmt, gf.luma, 0.0f, vignetteAmt);
    glUniform3fv(p->curve,   1, gf.curve);
    glUniform2f(p->dofAlpha, topAlpha, botAlpha);
    glUniform4fv(p->overlay, 1, overlay);
    if (battle)
        glUniform3f(p->battle, backBlur, 1.0f - backDim, bumpStrength);
    glUniform3fv(p->tint0, 1, gf.tint0);
    glUniform3fv(p->tint1, 1, gf.tint1);
    DrawQuad();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

void VoxelHd2dShader_Pack(const VoxelHd2dPack *const pk)
{
    if (!sPackReady || pk == NULL)
        return;
    // The DoF ramps start at the band edges and reach full blur over 1 / 2.8 of the screen; no
    // top band when it would start at the very top (as the full composite).
    const float invSlope = (pk->uvTop < 0.98f) ? 2.8f : 0.0f;

    glUseProgram(sPack.prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pk->srcTex);
    glUniform1i(sPack.src, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pk->bloomTex);
    glUniform1i(sPack.bloom, 1);
    glUniform2f(sPack.uvScale, pk->srcU, pk->srcV);
    glUniform2f(sPack.texel, pk->texelX, pk->texelY);
    glUniform2f(sPack.bloomUv, pk->bloomU, pk->bloomV);
    glUniform3f(sPack.dofRamp, pk->uvTop, pk->uvBot, invSlope);
    glUniform2f(sPack.dofAlpha, pk->topAlpha, pk->botAlpha);
    glUniform4fv(sPack.overlay, 1, pk->overlay);
    glUniform3f(sPack.mix, pk->minBlur, pk->dim, pk->bloomAmt);
    DrawQuad();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

static void SetPackedGrade(const PackedCompositeProgram *const p, const VoxelHd2dGrade *const g)
{
    GradeFactors gf;
    GradeFactorsOf(g->gradeAmt, g->saturation, g->curveAmt, g->shadowTint, g->highTint, &gf);
    glUniform3fv(p->tint0, 1, gf.tint0);
    glUniform3fv(p->tint1, 1, gf.tint1);
    glUniform1f(p->luma, gf.luma);
    glUniform3fv(p->curve, 1, gf.curve);
}

// The composite grid with the current program; attribute 0 = position, 1 = texture coordinate.
// The array and index buffer bindings are put back to 0.
static void DrawCompositeGrid(void)
{
    const bool buffers = (sGridVbo != 0 && sGridIbo != 0);
    const void *const verts = buffers ? NULL : (const void *)sGridVerts;
    const void *const indices = buffers ? NULL : (const void *)sGridIndices;

    if (buffers)
    {
        glBindBuffer(GL_ARRAY_BUFFER, sGridVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sGridIbo);
    }
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glDrawElements(GL_TRIANGLES, (GLsizei)COMP_GRID_INDICES, GL_UNSIGNED_SHORT, indices);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    if (buffers)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VoxelHd2dShader_CompositePacked(const GLuint sceneTex, const float sceneU, const float sceneV,
                                     const GLuint packTex, const float packU, const float packV,
                                     const VoxelHd2dGrade *const grade, const VoxelHd2dCompVariant variant)
{
    if (!sPackReady || grade == NULL)
        return;
    if (variant == VOXEL_HD2D_COMP_COPY)
    {
        static const float kNoOverlay[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        VoxelHd2dShader_Downsample(sceneTex, sceneU, sceneV, kNoOverlay);
        return;
    }
    const PackedCompositeProgram *const p = (variant == VOXEL_HD2D_COMP_READS) ? &sCompPackedReads : &sCompPacked;

    glUseProgram(p->prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(p->scene, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, packTex);
    glUniform1i(p->pack, 1);
    glUniform2f(p->sceneUv, sceneU, sceneV);
    glUniform2f(p->packUv, packU, packV);
    if (p == &sCompPacked)
    {
        glUniform1f(p->vig, grade->vignetteAmt);
        SetPackedGrade(p, grade);
    }
    DrawCompositeGrid();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

void VoxelHd2dShader_CompositePackedBattle(const GLuint packTex, const float packU, const float packV,
                                           const VoxelHd2dGrade *const grade, const float lowHpEdge)
{
    if (!sPackReady || grade == NULL)
        return;
    const PackedCompositeProgram *const p = &sCompPackedBattle;

    glUseProgram(p->prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, packTex);
    glUniform1i(p->pack, 0);
    glUniform2f(p->packUv, packU, packV);
    glUniform2f(p->edge, grade->vignetteAmt, lowHpEdge);
    SetPackedGrade(p, grade);
    DrawQuad();
    glUseProgram(0);
}

// The beam index period of SHAFT_LUT_FS_HEAD (mod(cell + base, 16.0)).
#define SHAFT_CELLS 16.0
#define SHAFT_TWO_PI 6.283185307179586

// x reduced to [0, period), in double: the products of a long session's clock keep their
// fractions, and the wrapped value fits half precision in the shader.
static double ShaftWrap(const double x, const double period)
{
    return x - floor(x / period) * period;
}

void VoxelHd2dShader_ShaftClockAt(const double timeSec, const float motion, VoxelHd2dShaftClock *const out)
{
    const double t = timeSec * (double)motion;
    // Layer i: pos = a * (1.1 + 0.55 i) + 1.73 i + t * sign_i * (0.035 + 0.016 i), sign +1 then -1;
    // breathing speed 0.40 + 0.21 i; dust twinkle 2.8.
    const double s0 = ShaftWrap(t * 0.035, SHAFT_CELLS);
    const double s1 = ShaftWrap(1.73 - t * 0.051, SHAFT_CELLS);
    const double base0 = floor(s0);
    const double base1 = floor(s1);
    out->shift[0] = (float)(s0 - base0);
    out->shift[1] = (float)(s1 - base1);
    out->shift[2] = (float)base0;
    out->shift[3] = (float)base1;
    out->breath[0] = (float)ShaftWrap(t * 0.40, SHAFT_TWO_PI);
    out->breath[1] = (float)ShaftWrap(t * 0.61, SHAFT_TWO_PI);
    out->dust = (float)ShaftWrap(t * 2.8, SHAFT_TWO_PI);
}

// The frame of one shafts pass: a = uv . perp over the screen (uv in 0..1) and its range from the
// corners. False when the direction has no beams to draw.
typedef struct
{
    float perpX, perpY;
    float aMin, aMax;
} ShaftFrame;

static bool ShaftFrameOf(const float dirX, const float dirY, const float aspect, ShaftFrame *const out)
{
    // normalize(uDir) of the original shader, once.
    const float dirLen = sqrtf(dirX * dirX + dirY * dirY);
    if (dirLen < 1e-6f)
        return false;
    const float dx = dirX / dirLen;
    const float dy = dirY / dirLen;
    out->perpX = aspect * dy;
    out->perpY = -dx;
    const float corners[4] = { 0.0f, out->perpX, out->perpY, out->perpX + out->perpY };
    out->aMin = corners[0];
    out->aMax = corners[0];
    for (int32_t k = 1; k < 4; ++k)
    {
        out->aMin = fminf(out->aMin, corners[k]);
        out->aMax = fmaxf(out->aMax, corners[k]);
    }
    return out->aMax - out->aMin >= 1e-6f;
}

void VoxelHd2dShader_BeginShaftLut(const uint32_t pass)
{
    if (pass >= SHAFT_LUTS)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, sShaftLutFbo[pass]);
    glViewport(0, (GLint)SHAFT_LUT_DRAW_Y, SHAFT_LUT_W, SHAFT_LUT_DRAW_H);
}

void VoxelHd2dShader_ShaftLut(const uint32_t pass, const float dirX, const float dirY,
                              const VoxelHd2dShaftClock *const clock, const float aspect, const float layers)
{
    ShaftFrame fr;
    if (!sShaftReady || pass >= SHAFT_LUTS || clock == NULL || !ShaftFrameOf(dirX, dirY, aspect, &fr))
        return;
    const ShaftLutProgram *const p = &sShaftLut[(layers >= 1.5f) ? 1 : 0];

    // The quad's x runs across a's range: texel i holds a at (i + 0.5) / W of it.
    glUseProgram(p->prog);
    glUniform2f(p->aRange, fr.aMin, fr.aMax);
    glUniform4fv(p->shift, 1, clock->shift);
    glUniform2fv(p->breath, 1, clock->breath);
    glUniform2f(p->uvScale, 1.0f, 1.0f);   // kTapVs: vT0 = aUv
    DrawQuad();
    glUseProgram(0);
}

void VoxelHd2dShader_ShaftSourceOwn(const uint32_t pass, VoxelHd2dShaftSource *const out)
{
    if (out == NULL)
        return;
    out->tex = (pass < SHAFT_LUTS) ? sShaftLutTex[pass] : 0;
    out->uScale = 1.0f;
    out->v = 0.5f;   // the boundary of the two middle rows (SHAFT_LUT_DRAW_Y)
}

// The profile of a pass as 2 rows in a band of the bound target, which must be at least
// SHAFT_LUT_W + 2 texels wide: the same texels 0..W-1 as its own target (the same a at each), and
// two more past the end of a's range so the bilinear read of the last one does not take a texel
// it never wrote (its own target clamped there).
#define SHAFT_BAND_ROW(pass) (7 + 16 * (int32_t)(pass))
#define SHAFT_BAND_DRAW_W (SHAFT_LUT_W + 2u)

void VoxelHd2dShader_ShaftLutBand(const uint32_t pass, const float dirX, const float dirY,
                                  const VoxelHd2dShaftClock *const clock, const float aspect, const float layers,
                                  const GLuint tex, const int32_t texW, const int32_t texH, const int32_t bandY,
                                  VoxelHd2dShaftSource *const out)
{
    ShaftFrame fr;
    if (out != NULL)
        out->tex = 0;
    if (!sShaftReady || pass >= SHAFT_LUTS || clock == NULL || out == NULL || texW < (int32_t)SHAFT_BAND_DRAW_W
     || !ShaftFrameOf(dirX, dirY, aspect, &fr))
        return;
    const ShaftLutProgram *const p = &sShaftLut[(layers >= 1.5f) ? 1 : 0];
    const int32_t rowY = bandY + SHAFT_BAND_ROW(pass);
    const float aEnd = fr.aMin + (fr.aMax - fr.aMin) * ((float)SHAFT_BAND_DRAW_W / (float)SHAFT_LUT_W);

    glViewport(0, rowY, (GLsizei)SHAFT_BAND_DRAW_W, (GLsizei)SHAFT_LUT_DRAW_H);
    glUseProgram(p->prog);
    glUniform2f(p->aRange, fr.aMin, aEnd);
    glUniform4fv(p->shift, 1, clock->shift);
    glUniform2fv(p->breath, 1, clock->breath);
    glUniform2f(p->uvScale, 1.0f, 1.0f);   // kTapVs: vT0 = aUv
    DrawQuad();
    glUseProgram(0);

    out->tex = tex;
    out->uScale = (float)SHAFT_LUT_W / (float)texW;
    out->v = (float)(rowY + 1) / (float)texH;   // the boundary of the two rows drawn
}

void VoxelHd2dShader_Shafts(const uint32_t pass, const float dirX, const float dirY, const float lenExp,
                            const VoxelHd2dShaftClock *const clock, const float aspect,
                            const float colR, const float colG, const float colB,
                            const VoxelHd2dShaftSource *const src)
{
    ShaftFrame fr;
    if (!sShaftReady || pass >= SHAFT_LUTS || clock == NULL || src == NULL || src->tex == 0
     || !ShaftFrameOf(dirX, dirY, aspect, &fr))
        return;
    // The profile's texture coordinate: (a - aMin) / (aMax - aMin) across its SHAFT_LUT_W texels,
    // a = uv . perp; scaled when the profile sits in a wider texture.
    const float inv = src->uScale / (fr.aMax - fr.aMin);

    glUseProgram(sShaftProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src->tex);
    glUniform1i(sShaftLutTexU, 0);
    glUniform3f(sShaftLutMap, fr.perpX * inv, fr.perpY * inv, -fr.aMin * inv);
    glUniform1f(sShaftLutV, src->v);
    glUniform1f(sShaftDust, clock->dust);
    glUniform1f(sShaftLen, lenExp);
    glUniform3f(sShaftCol, colR, colG, colB);
    DrawQuad();
    glUseProgram(0);
}
