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

// 4-tap box: downsample fallback, wide blur and the Kawase passes.
static GLuint sCopyProg = 0;
static GLint sCopyScene = -1, sCopyUvScale = -1, sCopyTexel = -1;

// One bilinear tap: the 2x2 downsample, with the night / fade overlay (uOverlay) applied.
static GLuint sTapProg = 0;
static GLint sTapScene = -1, sTapUvScale = -1, sTapOverlay = -1;

static GLuint sBrightProg = 0;
static GLint sBUScene = -1, sBUThresh = -1, sBUUvScale = -1, sBUGain = -1;

// Light shafts, one program per layer count (a constant loop instead of one bounded by a uniform).
#define SHAFT_PROGRAMS 2
typedef struct
{
    GLuint prog;
    GLint aRange, aCenter, axis, along, shift, tv;   // vertex shader
    GLint t, len, col;                               // fragment shader
} ShaftProgram;
static ShaftProgram sShaft[SHAFT_PROGRAMS];
static bool sShaftReady = false;

// The beams vary only across their direction (VoxelHd2dShader_Shafts): the strip that carries them
// has SHAFT_SEGMENTS parallelograms side by side across the screen, ~2 per bloom pixel along that
// axis, so the profile is sampled finer than the smallest edge of a beam (5+ samples).
#define SHAFT_SEGMENTS 512u
#define SHAFT_STRIP_VERTS (2u * (SHAFT_SEGMENTS + 1u))
// Half the strip's length along the beams, in screen units: past the screen's half diagonal (0.71).
#define SHAFT_HALF_LENGTH 1.5f
static GLuint sShaftStripVbo = 0;

// The composite: the field program leaves out the 3D battle terms.
typedef struct
{
    GLuint prog;
    GLint scene, blur, bloom;
    GLint sceneUv, blurUv, bloomUv, dofRamp;
    GLint post, dofAlpha, battle, overlay;
    GLint tint0, tint1;
} CompositeProgram;
static CompositeProgram sCompField, sCompBattle;

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

// Procedural HD-2D light shafts at quarter-res (adapted from bravely-emerald kShaftFs).
// Everything costly in them - the two hashed layers of beams, their profile and their breathing -
// depends on one screen coordinate only: a, the distance across the beams (a = uv . perp). It is
// evaluated in the vertex shader of a strip of thin parallelograms laid across the screen, one
// boundary every 1/SHAFT_SEGMENTS of a's range, with the same code and the same GPU arithmetic as
// the per-pixel shader it replaces, and interpolated between them. Per pixel only the fall along
// the screen and, where there is a beam, the dust are left. aPos: (s, side), s in 0..1 across the
// beams, side -1 or +1 along them. Same function, same beams, every frame; ~1000 evaluations
// instead of one per pixel of the bloom target.
#define SHAFT_VS_HEAD \
    "attribute vec2 aPos;\n" \
    "uniform vec2 uARange;\n" \
    "uniform float uACenter;\n" \
    "uniform vec2 uAxis;\n" \
    "uniform vec2 uAlong;\n" \
    "uniform vec2 uShift;\n" \
    "uniform float uTv;\n" \
    "varying vec2 vUv;\n" \
    "varying float vAcc;\n" \
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n" \
    "float Layer(float pos, float fi, float speed, float weight) {\n" \
    "    float cell = floor(pos);\n" \
    "    float fr = pos - cell;\n" \
    "    float h1 = hh(vec2(cell, fi));\n" \
    "    float h2 = hh(vec2(cell + 7.0, fi * 3.0));\n" \
    "    float width = 0.06 + 0.14 * h2;\n" \
    "    float prof = step(0.40, h1) * (1.0 - smoothstep(0.0, width, abs(fr - (0.25 + 0.5 * h1))));\n" \
    "    float breath = 0.55 + 0.45 * sin(uTv * speed + h1 * 6.2831);\n" \
    "    return prof * breath * weight;\n" \
    "}\n" \
    "void main() {\n" \
    "    float a = mix(uARange.x, uARange.y, aPos.x);\n" \
    "    vec2 uv = vec2(0.5) + (a - uACenter) * uAxis + aPos.y * uAlong;\n"

#define SHAFT_VS_TAIL \
    "    vAcc = clamp(acc * 0.78, 0.0, 1.0);\n" \
    "    vUv = uv;\n" \
    "    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);\n" \
    "}\n"

// Layer i: pos = a * (1.1 + 0.55 i) + shift_i, speed 0.40 + 0.21 i, weight 1 - 0.07 i.
static const char kShaftVs1[] = SHAFT_VS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, 0.0, 0.40, 1.0);\n"
    SHAFT_VS_TAIL;
static const char kShaftVs2[] = SHAFT_VS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, 0.0, 0.40, 1.0) + Layer(a * 1.65 + uShift.y, 1.0, 0.61, 0.93);\n"
    SHAFT_VS_TAIL;

// The fall towards the bottom of the screen, then the dust motes, only where there is a beam
// (they are multiplied by it: zero elsewhere).
static const char kShaftFs[] =
    "precision mediump float;\n"
    "uniform float uT;\n"
    "uniform float uLen;\n"
    "uniform vec3 uCol;\n"
    "varying vec2 vUv;\n"
    "varying float vAcc;\n"
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n"
    "void main() {\n"
    "    float beam = vAcc * pow(clamp(vUv.y, 0.0, 1.0), uLen);\n"
    "    float dust = 0.0;\n"
    "    if (beam > 0.0) {\n"
    "        float cellHash = hh(floor(vUv * vec2(120.0, 68.0)));\n"
    "        float mote = 0.5 + 0.5 * sin(uT * 2.8 + cellHash * 6.2831);\n"
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
// (VoxelHd2dShader_Composite). uPost: bloom, that luma factor, S-curve, vignette. uDofAlpha:
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
    "    float tl = clamp((l - 0.15) * 1.6667, 0.0, 1.0);\n" \
    "    c = clamp(c * (uTint0 + tl * uTint1) + uPost.y * l, 0.0, 1.0);\n" \
    "    vec3 sc = c * c * (3.0 - 2.0 * c);\n" \
    "    c += uPost.z * (sc - c);\n" \
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
    return true;
}

static bool InitShafts(ShaftProgram *const p, const char *const vsSrc)
{
    p->prog = LinkProgram(vsSrc, kShaftFs);
    if (p->prog == 0)
        return false;
    p->aRange  = glGetUniformLocation(p->prog, "uARange");
    p->aCenter = glGetUniformLocation(p->prog, "uACenter");
    p->axis    = glGetUniformLocation(p->prog, "uAxis");
    p->along   = glGetUniformLocation(p->prog, "uAlong");
    p->shift   = glGetUniformLocation(p->prog, "uShift");
    p->tv      = glGetUniformLocation(p->prog, "uTv");
    p->t       = glGetUniformLocation(p->prog, "uT");
    p->len     = glGetUniformLocation(p->prog, "uLen");
    p->col     = glGetUniformLocation(p->prog, "uCol");
    return true;
}

// The shafts' strip, once: (s, side) per vertex, a triangle strip of SHAFT_SEGMENTS quads.
static bool InitShaftStrip(void)
{
    static float verts[SHAFT_STRIP_VERTS * 2u];   // static: 8 KB, kept off the stack
    for (uint32_t i = 0; i <= SHAFT_SEGMENTS; ++i)
    {
        const float sCoord = (float)i / (float)SHAFT_SEGMENTS;
        verts[i * 4u + 0u] = sCoord;
        verts[i * 4u + 1u] = -1.0f;
        verts[i * 4u + 2u] = sCoord;
        verts[i * 4u + 3u] = 1.0f;
    }
    (void)glGetError();   // only this upload's error below, not an earlier one
    glGenBuffers(1, &sShaftStripVbo);
    if (sShaftStripVbo == 0)
        return false;
    glBindBuffer(GL_ARRAY_BUFFER, sShaftStripVbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return glGetError() == GL_NO_ERROR;
}

bool VoxelHd2dShader_Init(void)
{
    if (sTriedInit)
        return sShaderReady;
    sTriedInit = true;

#ifdef PLATFORM_VITA
    sCopyProg   = LinkProgram(kCopyVs, kCopyFs);
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
    if (InitShafts(&sShaft[0], kShaftVs1) && InitShafts(&sShaft[1], kShaftVs2) && InitShaftStrip())
    {
        sShaftReady = true;
        VITA_LOG("[HD2D SHADER] Procedural light shafts compiled successfully.\n");
    }
#endif
    return sShaderReady;
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
static void DrawQuad(void)
{
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
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

    // The grade's per-frame factors (COMP_FS_HEAD).
    const float keep = 1.0f - gradeAmt;
    const float chroma = gradeAmt * saturation;
    float tint0[3], tint1[3];
    for (int32_t k = 0; k < 3; ++k)
    {
        tint0[k] = keep + chroma * shadowTint[k];
        tint1[k] = chroma * (highTint[k] - shadowTint[k]);
    }
    const float lumaAmt = gradeAmt * (1.0f - saturation);

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
    glUniform4f(p->post,     bloomAmt, lumaAmt, curveAmt, vignetteAmt);
    glUniform2f(p->dofAlpha, topAlpha, botAlpha);
    glUniform4fv(p->overlay, 1, overlay);
    if (battle)
        glUniform3f(p->battle, backBlur, 1.0f - backDim, bumpStrength);
    glUniform3fv(p->tint0, 1, tint0);
    glUniform3fv(p->tint1, 1, tint1);
    DrawQuad();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

void VoxelHd2dShader_Shafts(const float dirX, const float dirY, const float lenExp, const float timeSec,
                            const float aspect, const float layers, const float motion,
                            const float colR, const float colG, const float colB)
{
    if (!sShaftReady) return;

    // normalize(uDir) of the original shader, once; a zero direction has no beams to draw.
    const float dirLen = sqrtf(dirX * dirX + dirY * dirY);
    if (dirLen < 1e-6f)
        return;
    const float dx = dirX / dirLen;
    const float dy = dirY / dirLen;
    const float t = timeSec * motion;
    // Layer i: pos = a * (1.1 + 0.55 i) + 1.73 i + t * sign_i * (0.035 + 0.016 i), sign +1 then -1.
    const float shift0 = t * 0.035f;
    const float shift1 = 1.73f - t * 0.051f;
    const ShaftProgram *const p = &sShaft[(layers >= 1.5f) ? 1 : 0];

    // a = uv . perp over the screen (uv in 0..1): its range from the corners, and the strip's
    // frame: uv = 0.5 + (a - a(centre)) * perp / |perp|^2 + side * along, along perpendicular to perp.
    const float perpX = aspect * dy;
    const float perpY = -dx;
    const float perpLen2 = perpX * perpX + perpY * perpY;
    if (perpLen2 < 1e-12f)
        return;
    const float corners[4] = { 0.0f, perpX, perpY, perpX + perpY };
    float aMin = corners[0], aMax = corners[0];
    for (int32_t k = 1; k < 4; ++k)
    {
        aMin = fminf(aMin, corners[k]);
        aMax = fmaxf(aMax, corners[k]);
    }
    const float perpLen = sqrtf(perpLen2);
    const float alongX = -perpY / perpLen * SHAFT_HALF_LENGTH;
    const float alongY = perpX / perpLen * SHAFT_HALF_LENGTH;

    glUseProgram(p->prog);
    glUniform2f(p->aRange, aMin, aMax);
    glUniform1f(p->aCenter, 0.5f * (perpX + perpY));
    glUniform2f(p->axis, perpX / perpLen2, perpY / perpLen2);
    glUniform2f(p->along, alongX, alongY);
    glUniform2f(p->shift, shift0, shift1);
    glUniform1f(p->tv, t);
    glUniform1f(p->t, t);
    glUniform1f(p->len, lenExp);
    glUniform3f(p->col, colR, colG, colB);

    glBindBuffer(GL_ARRAY_BUFFER, sShaftStripVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)SHAFT_STRIP_VERTS);
    glDisableVertexAttribArray(0);
    // The rest of the post pass draws from client arrays.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
