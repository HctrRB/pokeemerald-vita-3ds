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

// Light shafts (VoxelHd2dShader_Shafts). Their costly part - the two hashed layers of beams, the
// profile and the breathing - depends on one screen coordinate only, a, the distance across the
// beams. Each frame it is evaluated once per texel of a SHAFT_LUT_W x 1 target (one per sun /
// moon pass) by the original per-pixel code, then a full-screen quad reads that row at a.
#define SHAFT_PROGRAMS 2
#define SHAFT_LUT_W 512u
#define SHAFT_LUTS 2u
typedef struct
{
    GLuint prog;
    GLint uvScale, aRange, shift, t;
} ShaftLutProgram;
static ShaftLutProgram sShaftLut[SHAFT_PROGRAMS];   // 1 and 2 layers
static GLuint sShaftProg = 0;                       // the full-screen pass
static GLint sShaftLutTexU = -1, sShaftLutMap = -1, sShaftT = -1, sShaftLen = -1, sShaftCol = -1;
static GLuint sShaftLutTex[SHAFT_LUTS], sShaftLutFbo[SHAFT_LUTS];
static bool sShaftReady = false;

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

// Procedural HD-2D light shafts at quarter-res (adapted from bravely-emerald kShaftFs), in two steps.
// 1. The beam profile clamp(acc * 0.78) across the beams, one texel per sample of a: the original
//    per-pixel code (same hashes, same half precision, so the same beams), stored over two 8-bit
//    channels (value = r + g / 255) so the row keeps more than 8 bits through the bilinear read.
#define SHAFT_LUT_FS_HEAD \
    "precision mediump float;\n" \
    "uniform vec2 uARange;\n" \
    "uniform vec2 uShift;\n" \
    "uniform float uT;\n" \
    "varying vec2 vT0;\n" \
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n" \
    "float Layer(float pos, float fi, float speed, float weight) {\n" \
    "    float cell = floor(pos);\n" \
    "    float fr = pos - cell;\n" \
    "    float h1 = hh(vec2(cell, fi));\n" \
    "    float h2 = hh(vec2(cell + 7.0, fi * 3.0));\n" \
    "    float width = 0.06 + 0.14 * h2;\n" \
    "    float prof = step(0.40, h1) * (1.0 - smoothstep(0.0, width, abs(fr - (0.25 + 0.5 * h1))));\n" \
    "    float breath = 0.55 + 0.45 * sin(uT * speed + h1 * 6.2831);\n" \
    "    return prof * breath * weight;\n" \
    "}\n" \
    "void main() {\n" \
    "    float a = mix(uARange.x, uARange.y, vT0.x);\n"

#define SHAFT_LUT_FS_TAIL \
    "    float v = clamp(acc * 0.78, 0.0, 1.0);\n" \
    "    float hi = floor(v * 255.0) / 255.0;\n" \
    "    gl_FragColor = vec4(hi, (v - hi) * 255.0, 0.0, 1.0);\n" \
    "}\n"

// Layer i: pos = a * (1.1 + 0.55 i) + shift_i, speed 0.40 + 0.21 i, weight 1 - 0.07 i.
static const char kShaftLutFs1[] = SHAFT_LUT_FS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, 0.0, 0.40, 1.0);\n"
    SHAFT_LUT_FS_TAIL;
static const char kShaftLutFs2[] = SHAFT_LUT_FS_HEAD
    "    float acc = Layer(a * 1.1 + uShift.x, 0.0, 0.40, 1.0) + Layer(a * 1.65 + uShift.y, 1.0, 0.61, 0.93);\n"
    SHAFT_LUT_FS_TAIL;

// 2. The full-screen pass: the profile read at a (linear in screen position, so a varying: a
//    non-dependent read), the fall towards the bottom, then the dust motes where there is a beam.
static const char kShaftVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "uniform vec3 uLutMap;\n"
    "varying vec2 vUv;\n"
    "varying vec2 vLut;\n"
    "void main() {\n"
    "    vLut = vec2(aUv.x * uLutMap.x + aUv.y * uLutMap.y + uLutMap.z, 0.5);\n"
    "    vUv = aUv;\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kShaftFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uLut;\n"
    "uniform float uT;\n"
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

static bool InitShaftLut(ShaftLutProgram *const p, const char *const fsSrc)
{
    p->prog = LinkProgram(kTapVs, fsSrc);
    if (p->prog == 0)
        return false;
    p->uvScale = glGetUniformLocation(p->prog, "uUvScale");   // kTapVs
    p->aRange = glGetUniformLocation(p->prog, "uARange");
    p->shift  = glGetUniformLocation(p->prog, "uShift");
    p->t      = glGetUniformLocation(p->prog, "uT");
    return true;
}

// The shafts' profiles: one SHAFT_LUT_W x 1 target per pass, filtered (read between samples of a).
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHAFT_LUT_W, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenFramebuffers(1, &sShaftLutFbo[i]);
        if (sShaftLutFbo[i] == 0)
            return false;
        glBindFramebuffer(GL_FRAMEBUFFER, sShaftLutFbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sShaftLutTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
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
    sShaftProg = LinkProgram(kShaftVs, kShaftFs);
    if (sShaftProg && InitShaftLut(&sShaftLut[0], kShaftLutFs1) && InitShaftLut(&sShaftLut[1], kShaftLutFs2)
     && InitShaftTarget())
    {
        sShaftLutTexU = glGetUniformLocation(sShaftProg, "uLut");
        sShaftLutMap  = glGetUniformLocation(sShaftProg, "uLutMap");
        sShaftT       = glGetUniformLocation(sShaftProg, "uT");
        sShaftLen     = glGetUniformLocation(sShaftProg, "uLen");
        sShaftCol     = glGetUniformLocation(sShaftProg, "uCol");
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

// The frame of one shafts pass: a = uv . perp over the screen (uv in 0..1), its range from the
// corners, and the layers' shifts. False when the direction has no beams to draw.
typedef struct
{
    float perpX, perpY;
    float aMin, aMax;
    float shift0, shift1;
    float t;
} ShaftFrame;

static bool ShaftFrameOf(const float dirX, const float dirY, const float timeSec, const float aspect,
                         const float motion, ShaftFrame *const out)
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
    if (out->aMax - out->aMin < 1e-6f)
        return false;
    out->t = timeSec * motion;
    // Layer i: pos = a * (1.1 + 0.55 i) + 1.73 i + t * sign_i * (0.035 + 0.016 i), sign +1 then -1.
    out->shift0 = out->t * 0.035f;
    out->shift1 = 1.73f - out->t * 0.051f;
    return true;
}

void VoxelHd2dShader_BeginShaftLut(const uint32_t pass)
{
    if (pass >= SHAFT_LUTS)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, sShaftLutFbo[pass]);
    glViewport(0, 0, SHAFT_LUT_W, 1);
}

void VoxelHd2dShader_ShaftLut(const uint32_t pass, const float dirX, const float dirY, const float timeSec,
                              const float aspect, const float layers, const float motion)
{
    ShaftFrame fr;
    if (!sShaftReady || pass >= SHAFT_LUTS || !ShaftFrameOf(dirX, dirY, timeSec, aspect, motion, &fr))
        return;
    const ShaftLutProgram *const p = &sShaftLut[(layers >= 1.5f) ? 1 : 0];

    // The quad's x runs across a's range: texel i holds a at (i + 0.5) / W of it.
    glUseProgram(p->prog);
    glUniform2f(p->aRange, fr.aMin, fr.aMax);
    glUniform2f(p->shift, fr.shift0, fr.shift1);
    glUniform1f(p->t, fr.t);
    glUniform2f(p->uvScale, 1.0f, 1.0f);   // kTapVs: vT0 = aUv
    DrawQuad();
    glUseProgram(0);
}

void VoxelHd2dShader_Shafts(const uint32_t pass, const float dirX, const float dirY, const float lenExp,
                            const float timeSec, const float aspect, const float motion,
                            const float colR, const float colG, const float colB)
{
    ShaftFrame fr;
    if (!sShaftReady || pass >= SHAFT_LUTS || !ShaftFrameOf(dirX, dirY, timeSec, aspect, motion, &fr))
        return;
    // The profile's texture coordinate: (a - aMin) / (aMax - aMin), a = uv . perp.
    const float inv = 1.0f / (fr.aMax - fr.aMin);

    glUseProgram(sShaftProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sShaftLutTex[pass]);
    glUniform1i(sShaftLutTexU, 0);
    glUniform3f(sShaftLutMap, fr.perpX * inv, fr.perpY * inv, -fr.aMin * inv);
    glUniform1f(sShaftT, fr.t);
    glUniform1f(sShaftLen, lenExp);
    glUniform3f(sShaftCol, colR, colG, colB);
    DrawQuad();
    glUseProgram(0);
}
