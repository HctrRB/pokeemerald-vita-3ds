// Core 0 pass of the new UI: draws the latest list published by Core 1 over the frame (after the
// 3D passes or the 2D GBA frame, before the swap). Core 1 already emitted the vertices, so this
// is one bind and one draw call: one RGBA8888 texture (fonts, GBA sprites, a white block for solid
// fills and gradients, and 64x64 cells of sprites decoded at run time).
// After the first frame only new sprite cells are written, straight into the texture memory and
// never into a cell drawn in the last frames, so it never hits vitaGL's copy-on-write of textures
// in flight (docs/reference/RENDERING.md).

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ui_internal.h"
#include "platform/ui.h"
#include "../vita/vita_perf.h"

#ifdef PLATFORM_VITA
#include <vitaGL.h>
#else
#include <GL/gl.h>
#endif

#define UI_BACKDROP_COUNT 7u
#define UI_BACKDROP_TILE  16
#define UI_TOPBAR_H       52

#define UI_VBO_RING_SIZE  4u

static GLuint sTexture;
static GLuint sBackdropTex[UI_BACKDROP_COUNT];
static bool sBackdropReady;
static GLuint sUiVbo[UI_VBO_RING_SIZE];
static uint32_t sUiVboCap[UI_VBO_RING_SIZE];
static uint32_t sUiVboIdx;
static uint32_t sLastUploadedSeq;

static void EnsureBackdrops(void)
{
    if (sBackdropReady)
        return;
    uint32_t pixels[UI_BACKDROP_TILE * UI_BACKDROP_TILE];
    glGenTextures((GLsizei)UI_BACKDROP_COUNT, sBackdropTex);
    for (uint32_t i = 0; i < UI_BACKDROP_COUNT; i++)
    {
        if (sBackdropTex[i] == 0)
            return;
        UiW_GbaBgPattern((uint8_t)i, pixels);
        glBindTexture(GL_TEXTURE_2D, sBackdropTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, UI_BACKDROP_TILE, UI_BACKDROP_TILE, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
    sBackdropReady = true;
}

static bool EnsureTexture(void)
{
    if (sTexture != 0)
        return true;
    uint16_t w = 0, h = 0;
    const uint16_t *pixels = UiAssets_Pixels(&w, &h);
    if (pixels == NULL || w != UI_TEX_W || h == 0 || h > UI_DYN_Y)
        return false;
    // Whole texture: the fixed atlas on top, the run-time sprite cells below start empty
    uint32_t *full = calloc((size_t)UI_TEX_W * UI_TEX_H, sizeof(uint32_t));
    if (full == NULL)
        return false;
    for (size_t i = 0, n = (size_t)w * h; i < n; i++)
        full[i] = Ui_UnpackTexel(pixels[i]);
    glGenTextures(1, &sTexture);
    glBindTexture(GL_TEXTURE_2D, sTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, UI_TEX_W, UI_TEX_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, full);
    free(full);

    return true;
}

static void DrawBackdrop(int8_t kind)
{
    if (kind < 0 || (uint32_t)kind >= UI_BACKDROP_COUNT)
        return;
    EnsureBackdrops();
    if (!sBackdropReady || sBackdropTex[kind] == 0)
        return;
    static const float pos[8] = {
        0.0f, (float)UI_TOPBAR_H,
        (float)UI_SCREEN_W, (float)UI_TOPBAR_H,
        (float)UI_SCREEN_W, (float)UI_SCREEN_H,
        0.0f, (float)UI_SCREEN_H
    };
    const float uMax = (float)UI_SCREEN_W / (float)UI_BACKDROP_TILE;
    const float vMax = (float)(UI_SCREEN_H - UI_TOPBAR_H) / (float)UI_BACKDROP_TILE;
    const float uvs[8] = { 0.0f, 0.0f, uMax, 0.0f, uMax, vMax, 0.0f, vMax };
    glBindTexture(GL_TEXTURE_2D, sBackdropTex[kind]);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertexPointer(2, GL_FLOAT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glEnableClientState(GL_COLOR_ARRAY);
}

static void Draw(const UiDrawList *list, GLuint texture)
{
    if (list->vcount == 0)
        return;
    if (sUiVbo[0] == 0)
        glGenBuffers((GLsizei)UI_VBO_RING_SIZE, sUiVbo);
    if (sUiVbo[0] != 0)
    {
        if (list->seq != sLastUploadedSeq)
        {
            sUiVboIdx = (sUiVboIdx + 1u) & (UI_VBO_RING_SIZE - 1u);
            glBindBuffer(GL_ARRAY_BUFFER, sUiVbo[sUiVboIdx]);
            if (list->vcount > sUiVboCap[sUiVboIdx])
            {
                const uint32_t allocCap = (list->vcount + 511u) & ~511u;
                glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(allocCap * sizeof(UiGlVertex)),
                             NULL, GL_DYNAMIC_DRAW);
                sUiVboCap[sUiVboIdx] = allocCap;
            }
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            (GLsizeiptr)(list->vcount * sizeof(UiGlVertex)), list->verts);
            sLastUploadedSeq = list->seq;
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, sUiVbo[sUiVboIdx]);
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glVertexPointer(2, GL_FLOAT, sizeof(UiGlVertex), (const void *)offsetof(UiGlVertex, x));
        glTexCoordPointer(2, GL_FLOAT, sizeof(UiGlVertex), (const void *)offsetof(UiGlVertex, u));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(UiGlVertex), (const void *)offsetof(UiGlVertex, rgba));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)list->vcount);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return;
    }
    const UiGlVertex *v = list->verts;
    glBindTexture(GL_TEXTURE_2D, texture);
    glVertexPointer(2, GL_FLOAT, sizeof(UiGlVertex), &v[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(UiGlVertex), &v[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(UiGlVertex), &v[0].rgba);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)list->vcount);
}

void Ui_Render(int32_t viewWidth, int32_t viewHeight)
{
    if (!UiAssets_Ready())
        return;
    const UiDrawList *list = UiDraw_Acquire();
    UiStall_Render(list->seq);
    if (list->vcount == 0 && list->backdropKind < 0)
        return;
    const double t0 = VitaPerf_NowMs();
    if (!EnsureTexture())
        return;

    glViewport(0, 0, viewWidth, viewHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)UI_SCREEN_W, (double)UI_SCREEN_H, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Sprites Core 1 decoded for this list go into their cells first (never into a cell drawn in
    // the last frames, so it is written in place without vitaGL's copy of the texture)
    DrawBackdrop(list->backdropKind);
    glBindTexture(GL_TEXTURE_2D, sTexture);
    UiSpr_FlushUploads();
    Draw(list, sTexture);


    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);

    const double ms = VitaPerf_NowMs() - t0;
    UiCore_SetLastRenderMs((float)ms);
    VitaPerf_AddTime(VITA_PERF_T_UI2_RENDER, ms);
    VitaPerf_AddTime(VITA_PERF_T_UI2_TICK, list->tickMs);
    VitaPerf_AddCount(VITA_PERF_C_UI2_QUADS, list->vcount / 6);
}
