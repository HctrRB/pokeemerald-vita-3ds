// PS Vita startup logo, displayed after vitaGL init and before the voxel game starts.
// Artwork is stored as RGBA8888 in app0:splash/wolff_paw.rgba; no PNG decoder needed.
// 500 ms fade-in, 700 ms hold, 600 ms fade-out; pixel art remains crisp.
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vitaGL.h>
#include <SDL2/SDL.h>
#include "vita_boot_splash.h"
#include "vita_platform.h"

#define VITA_SCREEN_W 960.0f
#define VITA_SCREEN_H 544.0f
#define LOGO_DRAW_SIZE 240.0f
#define SPLASH_FADE_IN_MS 500
#define SPLASH_HOLD_MS 700
#define SPLASH_FADE_OUT_MS 600
#define SPLASH_TOTAL_MS (SPLASH_FADE_IN_MS + SPLASH_HOLD_MS + SPLASH_FADE_OUT_MS)

void VitaBootSplash_Show(void)
{
    size_t size = 0U;
    uint8_t *data = VitaPlatform_LoadAsset("splash/wolff_paw.rgba", &size);
    if (data == NULL || size < 8U || memcmp(data, "WPAW", 4) != 0)
    {
        VITA_LOG("[BOOT] Wolff's Paw image missing or invalid\n");
        free(data);
        return;
    }
    const uint16_t width = (uint16_t)(data[4] | ((uint16_t)data[5] << 8));
    const uint16_t height = (uint16_t)(data[6] | ((uint16_t)data[7] << 8));
    if (width == 0U || height == 0U || width > 512U || height > 512U
        || size != 8U + (size_t)width * height * 4U)
    {
        VITA_LOG("[BOOT] Invalid startup image dimensions\n");
        free(data);
        return;
    }

    GLuint texture = 0U;
    glGenTextures(1, &texture);
    if (texture == 0U)
    {
        free(data);
        return;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data + 8);
    free(data);

    const float logoX = (VITA_SCREEN_W - LOGO_DRAW_SIZE) * 0.5f;
    const float logoY = (VITA_SCREEN_H - LOGO_DRAW_SIZE) * 0.5f;
    const GLfloat vertices[] = {
        logoX, logoY,
        logoX + LOGO_DRAW_SIZE, logoY,
        logoX + LOGO_DRAW_SIZE, logoY + LOGO_DRAW_SIZE,
        logoX, logoY + LOGO_DRAW_SIZE
    };
    const GLfloat texcoords[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };

    glViewport(0, 0, 960, 544);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, VITA_SCREEN_W, VITA_SCREEN_H, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

    eglSwapInterval(NULL, 1);
    const uint32_t start = SDL_GetTicks();
    while (true)
    {
        const uint32_t elapsed = SDL_GetTicks() - start;
        if (elapsed >= SPLASH_TOTAL_MS)
            break;

        float opacity = 1.0f;
        if (elapsed < SPLASH_FADE_IN_MS)
            opacity = (float)elapsed / (float)SPLASH_FADE_IN_MS;
        else if (elapsed >= SPLASH_FADE_IN_MS + SPLASH_HOLD_MS)
            opacity = (float)(SPLASH_TOTAL_MS - elapsed) / (float)SPLASH_FADE_OUT_MS;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor4f(1.0f, 1.0f, 1.0f, opacity);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        vglSwapBuffers(GL_FALSE);
        SDL_Delay(1);
    }
    // Finish on black so the subsequent game initialization never flashes the logo.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vglSwapBuffers(GL_FALSE);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texture);
}
