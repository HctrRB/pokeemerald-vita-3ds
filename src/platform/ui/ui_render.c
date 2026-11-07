// Core 0 pass of the new UI: draws the latest list published by Core 1 over the frame (after the
// 3D passes or the 2D GBA frame, before the swap). Core 1 already emitted the vertices, so this
// is one bind and one draw call: one RGBA5551 texture (fonts, GBA sprites, a white block for solid
// fills and gradients, and 64x64 cells of sprites decoded at run time).
// After the first frame only new sprite cells are written, straight into the texture memory and
// never into a cell drawn in the last frames, so it never hits vitaGL's copy-on-write of textures
// in flight (docs/RENDERING.md).

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

static GLuint sTexture;

static bool EnsureTexture(void)
{
    if (sTexture != 0)
        return true;
    uint16_t w = 0, h = 0;
    const uint16_t *pixels = UiAssets_Pixels(&w, &h);
    if (pixels == NULL || w != UI_TEX_W || h == 0 || h > UI_DYN_Y)
        return false;
    // Whole texture: the fixed atlas on top, the run-time sprite cells below start empty
    uint16_t *full = calloc((size_t)UI_TEX_W * UI_TEX_H, sizeof(uint16_t));
    if (full == NULL)
        return false;
    memcpy(full, pixels, (size_t)w * h * sizeof(uint16_t));
    glGenTextures(1, &sTexture);
    glBindTexture(GL_TEXTURE_2D, sTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, UI_TEX_W, UI_TEX_H, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, full);
    free(full);

    return true;
}

static void Draw(const UiGlVertex *v, uint32_t count, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glVertexPointer(2, GL_FLOAT, sizeof(UiGlVertex), &v[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(UiGlVertex), &v[0].u);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(UiGlVertex), &v[0].rgba);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)count);
}

void Ui_Render(int32_t viewWidth, int32_t viewHeight)
{
    if (!UiAssets_Ready())
        return;
    const UiDrawList *list = UiDraw_Acquire();
    UiStall_Render(list->seq);
    if (list->vcount == 0)
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
    glBindTexture(GL_TEXTURE_2D, sTexture);
    UiSpr_FlushUploads();
    Draw(list->verts, list->vcount, sTexture);


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
