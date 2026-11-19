#include "voxel_hd2d_shader.h"
#include "../vita/vita_platform.h"
#include <stdio.h>
#include <string.h>

static bool sTriedInit = false;
static bool sShaderReady = false;

static GLuint sCopyProg = 0;
static GLint sCopyScene = -1, sCopyUvScale = -1, sCopyTexel = -1;

static GLuint sBrightProg = 0;
static GLint sBUScene = -1, sBUThresh = -1, sBUUvScale = -1, sBUGain = -1;

static GLuint sShaftProg = 0;
static GLint sSUDir = -1, sSULen = -1, sSUTime = -1, sSUAsp = -1;
static GLint sSUCount = -1, sSUMotion = -1, sSUCol = -1;
static bool sShaftReady = false;

static GLuint sCompProg = 0;
static GLint sCUScene = -1, sCUBlur = -1, sCUBloom = -1;
static GLint sCUSceneUv = -1, sCUBlurUv = -1, sCUBloomUv = -1;
static GLint sCUDof = -1, sCUPost = -1, sCUGrade = -1, sCUBump = -1;
static GLint sCUShadowTint = -1, sCUHighTint = -1, sCUWarmKeep = -1;

static const char kFullVs[] =
    "attribute vec2 aPos;\n"
    "attribute vec2 aUv;\n"
    "varying vec2 vUv;\n"
    "void main() {\n"
    "    vUv = aUv;\n"
    "    gl_Position = vec4(aPos * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char kCopyFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform vec2 uUvScale;\n"
    "uniform vec2 uTexel;\n"
    "varying vec2 vUv;\n"
    "void main() {\n"
    "    vec2 uv = vUv * uUvScale;\n"
    "    vec3 c = texture2D(uScene, uv + vec2(-uTexel.x, -uTexel.y)).rgb;\n"
    "    c += texture2D(uScene, uv + vec2(uTexel.x, -uTexel.y)).rgb;\n"
    "    c += texture2D(uScene, uv + vec2(-uTexel.x, uTexel.y)).rgb;\n"
    "    c += texture2D(uScene, uv + vec2(uTexel.x, uTexel.y)).rgb;\n"
    "    gl_FragColor = vec4(c * 0.25, 1.0);\n"
    "}\n";

static const char kBrightFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform float uThreshold;\n"
    "uniform vec2 uUvScale;\n"
    "uniform float uGain;\n"
    "varying vec2 vUv;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vUv * uUvScale).rgb;\n"
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "    float k = clamp((l - uThreshold) / max(1.0 - uThreshold, 0.05), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(c * (k * (0.75 + 0.25 * k) * uGain), 1.0);\n"
    "}\n";

// Procedural HD-2D light shafts at quarter-res (adapted from bravely-emerald kShaftFs).
// Uses highp + small offsets + safe hash multiplier to avoid fp16 quantization stutter on SGX543.
static const char kShaftFs[] =
    "precision highp float;\n"
    "uniform vec2 uDir;\n"
    "uniform float uLen;\n"
    "uniform float uTime;\n"
    "uniform float uAsp;\n"
    "uniform float uCount;\n"
    "uniform float uMotion;\n"
    "uniform vec3 uCol;\n"
    "varying vec2 vUv;\n"
    "float hh(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 437.5453); }\n"
    "void main() {\n"
    "    vec2 d = normalize(uDir);\n"
    "    float a = dot(vec2(vUv.x * uAsp, vUv.y), vec2(d.y, -d.x));\n"
    "    float t = uTime * uMotion;\n"
    "    float acc = 0.0;\n"
    "    for (int i = 0; i < 5; i++) {\n"
    "        float fi = float(i);\n"
    "        if (fi >= uCount) break;\n"
    "        float sgn = mod(fi, 2.0) < 0.5 ? 1.0 : -1.0;\n"
    "        float pos = a * (1.1 + fi * 0.55) + fi * 1.73 + t * sgn * (0.035 + 0.016 * fi);\n"
    "        float cell = floor(pos);\n"
    "        float fr = pos - cell;\n"
    "        float h1 = hh(vec2(cell, fi));\n"
    "        float h2 = hh(vec2(cell + 7.0, fi * 3.0));\n"
    "        float width = 0.06 + 0.14 * h2;\n"
    "        float prof = step(0.40, h1) * (1.0 - smoothstep(0.0, width, abs(fr - (0.25 + 0.5 * h1))));\n"
    "        float breath = 0.55 + 0.45 * sin(t * (0.40 + 0.21 * fi) + h1 * 6.2831);\n"
    "        acc += prof * breath * (1.0 - 0.07 * fi);\n"
    "    }\n"
    "    float fall = pow(clamp(vUv.y, 0.0, 1.0), uLen);\n"
    "    float beam = clamp(acc * 0.78, 0.0, 1.0) * fall;\n"
    "    float mote = 0.5 + 0.5 * sin(t * 2.8 + hh(floor(vUv * vec2(120.0, 68.0))) * 6.2831);\n"
    "    float dust = step(0.96, hh(floor(vUv * vec2(120.0, 68.0)))) * mote * beam * 1.1;\n"
    "    gl_FragColor = vec4(uCol * (beam + dust), 1.0);\n"
    "}\n";

// 30 FPS HD-2D composite: DoF tilt-shift + Battle dim + Bloom + Split-Tone + S-Curve + Vignette in 1 pass
// so 100% of the image (including blurred bands) receives full HD-2D grading and vignette.
static const char kCompFs[] =
    "precision mediump float;\n"
    "uniform sampler2D uScene;\n"
    "uniform sampler2D uBlur;\n"
    "uniform sampler2D uBloom;\n"
    "uniform vec2 uSceneUv;\n"
    "uniform vec2 uBlurUv;\n"
    "uniform vec2 uBloomUv;\n"
    "uniform vec4 uDof;\n"
    "uniform vec4 uPost;\n"
    "uniform vec4 uGrade;\n"
    "uniform vec3 uBump;\n"
    "uniform vec3 uShadowTint;\n"
    "uniform vec3 uHighTint;\n"
    "uniform float uWarmKeep;\n"
    "varying vec2 vUv;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vUv * uSceneUv).rgb;\n"
    "    vec3 b = texture2D(uBlur, vUv * uBlurUv).rgb;\n"
    "    float tTop = clamp((vUv.y - uDof.x) * uDof.z, 0.0, 1.0);\n"
    "    float tBot = clamp((uDof.y - vUv.y) * uDof.z, 0.0, 1.0);\n"
    "    float wTop = smoothstep(0.0, 1.0, tTop) * uDof.w;\n"
    "    float wBot = smoothstep(0.0, 1.0, tBot) * uGrade.w;\n"
    "    float dofW = max(max(wTop, wBot), uGrade.y);\n"
    "    c = mix(c, b, dofW) * uGrade.z;\n"
    "    c += texture2D(uBloom, vUv * uBloomUv).rgb * uPost.x;\n"
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "    float t = clamp((l - 0.15) * 1.6667, 0.0, 1.0);\n"
    "    vec3 tint = mix(uShadowTint, uHighTint, t);\n"
    // Warm light at night (lamps, windows: red over blue in a blue night) keeps its warmth
    // instead of taking the cool split-tone: the warm-cold contrast of HD-2D nights.
    "    float warm = smoothstep(0.02, 0.16, c.r - c.b) * uWarmKeep;\n"
    "    tint = mix(tint, vec3(1.05, 0.97, 0.86), warm);\n"
    "    vec3 g = mix(vec3(l), c * tint, uGrade.x);\n"
    "    c = clamp(mix(c, g, uPost.y), 0.0, 1.0);\n"
    "    vec3 sc = c * c * (3.0 - 2.0 * c);\n"
    "    c += uPost.z * (sc - c);\n"
    "    vec2 d = (vUv - 0.5) * vec2(1.7647, 1.0);\n"
    "    float vig = 1.0 - clamp(dot(d, d) - 0.12, 0.0, 0.80) * uPost.w;\n"
    "    c *= vig;\n"
    "    float edge = clamp(dot(d, d) - 0.16, 0.0, 0.85) * uBump.z;\n"
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

bool VoxelHd2dShader_Init(void)
{
    if (sTriedInit)
        return sShaderReady;
    sTriedInit = true;

#ifdef PLATFORM_VITA
    sCopyProg   = LinkProgram(kFullVs, kCopyFs);
    sBrightProg = LinkProgram(kFullVs, kBrightFs);
    sCompProg   = LinkProgram(kFullVs, kCompFs);
    if (sCopyProg && sBrightProg && sCompProg)
    {
        sCopyScene   = glGetUniformLocation(sCopyProg, "uScene");
        sCopyUvScale = glGetUniformLocation(sCopyProg, "uUvScale");
        sCopyTexel   = glGetUniformLocation(sCopyProg, "uTexel");

        sBUScene   = glGetUniformLocation(sBrightProg, "uScene");
        sBUThresh  = glGetUniformLocation(sBrightProg, "uThreshold");
        sBUUvScale = glGetUniformLocation(sBrightProg, "uUvScale");
        sBUGain    = glGetUniformLocation(sBrightProg, "uGain");

        sCUScene      = glGetUniformLocation(sCompProg, "uScene");
        sCUBlur       = glGetUniformLocation(sCompProg, "uBlur");
        sCUBloom      = glGetUniformLocation(sCompProg, "uBloom");
        sCUSceneUv    = glGetUniformLocation(sCompProg, "uSceneUv");
        sCUBlurUv     = glGetUniformLocation(sCompProg, "uBlurUv");
        sCUBloomUv    = glGetUniformLocation(sCompProg, "uBloomUv");
        sCUDof        = glGetUniformLocation(sCompProg, "uDof");
        sCUPost       = glGetUniformLocation(sCompProg, "uPost");
        sCUGrade      = glGetUniformLocation(sCompProg, "uGrade");
        sCUBump       = glGetUniformLocation(sCompProg, "uBump");
        sCUShadowTint = glGetUniformLocation(sCompProg, "uShadowTint");
        sCUHighTint   = glGetUniformLocation(sCompProg, "uHighTint");
        sCUWarmKeep   = glGetUniformLocation(sCompProg, "uWarmKeep");
        sShaderReady = true;
        VITA_LOG("[HD2D SHADER] Bright-Pass + Uber-Composite compiled successfully.\n");
    }
    sShaftProg = LinkProgram(kFullVs, kShaftFs);
    if (sShaftProg)
    {
        sSUDir    = glGetUniformLocation(sShaftProg, "uDir");
        sSULen    = glGetUniformLocation(sShaftProg, "uLen");
        sSUTime   = glGetUniformLocation(sShaftProg, "uTime");
        sSUAsp    = glGetUniformLocation(sShaftProg, "uAsp");
        sSUCount  = glGetUniformLocation(sShaftProg, "uCount");
        sSUMotion = glGetUniformLocation(sShaftProg, "uMotion");
        sSUCol    = glGetUniformLocation(sShaftProg, "uCol");
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

void VoxelHd2dShader_CopyBox2x2(const GLuint sceneTex, const float uMax, const float vMax,
                                const float texelX, const float texelY)
{
    if (!sShaderReady) return;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glUseProgram(sCopyProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sCopyScene, 0);
    glUniform2f(sCopyUvScale, uMax, vMax);
    glUniform2f(sCopyTexel, texelX, texelY);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void VoxelHd2dShader_BrightPass(const GLuint sceneTex, const float uMax, const float vMax, const float threshold, const float gain)
{
    if (!sShaderReady) return;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glUseProgram(sBrightProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sBUScene, 0);
    glUniform1f(sBUThresh, threshold);
    glUniform2f(sBUUvScale, uMax, vMax);
    glUniform1f(sBUGain, gain);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
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
                                const float shadowTint[3], const float highTint[3], const float warmKeep)
{
    if (!sShaderReady) return;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glUseProgram(sCompProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sCUScene, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex);
    glUniform1i(sCUBlur, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, bloomTex);
    glUniform1i(sCUBloom, 2);

    (void)bumpOffsetX; (void)bumpOffsetY;
    const float invSlope = (yTop < 0.98f) ? 2.8f : 0.0f;
    glUniform2f(sCUSceneUv, uSceneMax, vSceneMax);
    glUniform2f(sCUBlurUv,  uBlurMax,  vBlurMax);
    glUniform2f(sCUBloomUv, uBloomMax, vBloomMax);
    glUniform4f(sCUDof,     yTop, yBot, invSlope, topAlpha);
    glUniform4f(sCUPost,    bloomAmt, gradeAmt, curveAmt, vignetteAmt);
    glUniform4f(sCUGrade,   saturation, backBlur, 1.0f - backDim, botAlpha);
    glUniform3f(sCUBump,    0.0f, 0.0f, bumpStrength);
    glUniform3fv(sCUShadowTint, 1, shadowTint);
    glUniform3fv(sCUHighTint,   1, highTint);
    glUniform1f(sCUWarmKeep,    warmKeep);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

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
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glUseProgram(sShaftProg);
    glUniform2f(sSUDir, dirX, dirY);
    glUniform1f(sSULen, lenExp);
    glUniform1f(sSUTime, timeSec);
    glUniform1f(sSUAsp, aspect);
    glUniform1f(sSUCount, layers);
    glUniform1f(sSUMotion, motion);
    glUniform3f(sSUCol, colR, colG, colB);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}
