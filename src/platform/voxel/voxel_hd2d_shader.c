#include "voxel_hd2d_shader.h"
#include "../vita/vita_platform.h"
#include <stdio.h>
#include <string.h>

static bool sTriedInit = false;
static bool sShaderReady = false;

static GLuint sCopyProg = 0;
static GLint sCopyScene = -1, sCopyUvScale = -1, sCopyTexel = -1;

static GLuint sBrightProg = 0;
static GLint sBUScene = -1, sBUThresh = -1, sBUUvScale = -1;

static GLuint sCompProg = 0;
static GLint sCUScene = -1, sCUBlur = -1, sCUBloom = -1;
static GLint sCUSceneUv = -1, sCUBlurUv = -1, sCUBloomUv = -1;
static GLint sCUDof = -1, sCUPost = -1, sCUGrade = -1, sCUBump = -1;
static GLint sCUShadowTint = -1, sCUHighTint = -1;

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
    "varying vec2 vUv;\n"
    "void main() {\n"
    "    vec3 c = texture2D(uScene, vUv * uUvScale).rgb;\n"
    "    float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
    "    float k = clamp((l - uThreshold) / max(1.0 - uThreshold, 0.05), 0.0, 1.0);\n"
    "    gl_FragColor = vec4(c * (k * (0.5 + 0.5 * k)), 1.0);\n"
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
        sShaderReady = true;
        VITA_LOG("[HD2D SHADER] Bright-Pass + Uber-Composite compiled successfully.\n");
    }
#endif
    return sShaderReady;
}

bool VoxelHd2dShader_Ready(void)
{
    return sShaderReady;
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

void VoxelHd2dShader_BrightPass(const GLuint sceneTex, const float uMax, const float vMax, const float threshold)
{
    if (!sShaderReady) return;
    static const float quad[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    glUseProgram(sBrightProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(sBUScene, 0);
    glUniform1f(sBUThresh, threshold);
    glUniform2f(sBUUvScale, uMax, vMax);

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
                                const float shadowTint[3], const float highTint[3])
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
