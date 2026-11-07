#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitaGL.h>

#include "vita_stage.h"
#include "vita_platform.h"
#include "global.h"
#include "main.h"
#include "gpu_regs.h"
#include "palette.h"
#include "3ds_bottom_art.h"

#define LEAVES_LEFT 56
#define LEAVES_RIGHT 56
#define LEAVES_PATCH 8
#define LEAVES_ROWS 256
#define LEAVES_LAYER (1 + (LEAVES_LEFT + LEAVES_PATCH + LEAVES_RIGHT) * LEAVES_ROWS)
#define LEAVES_BYTES (8 + 4 * LEAVES_LAYER)

static void (*sStageCallback)(void) = NULL;
static void (*sCentredCallback)(void) = NULL;
static uint32_t sCentredScreen = 0;

static uint8_t *sLeavesData = NULL;
static bool sLeavesTried = false;
static GLuint sLeavesTex[4] = {0, 0, 0, 0};
static uint32_t sLeavesPalHash[4] = {0, 0, 0, 0};
static uint16_t sLeavesPixels[128 * LEAVES_ROWS];

void VitaStage_Init(void)
{
    sStageCallback = NULL;
    sCentredCallback = NULL;
    sCentredScreen = 0;
}

void VitaStage_SetVBlankCallback(void (*callback)(void))
{
    sStageCallback = callback;
    SetVBlankCallback(callback);
}

void VitaCentred_SetVBlankCallback(void (*callback)(void), uint32_t screen)
{
    sCentredCallback = callback;
    sCentredScreen = screen;
    SetVBlankCallback(callback);
}

void CtrCentredMainMenu_SetVBlankCallback(void (*callback)(void))
{
    VitaCentred_SetVBlankCallback(callback, 1);
}

void CtrCentredNaming_SetVBlankCallback(void (*callback)(void))
{
    VitaCentred_SetVBlankCallback(callback, 2);
}

void CtrCentredClock_SetVBlankCallback(void (*callback)(void))
{
    VitaCentred_SetVBlankCallback(callback, 3);
}

void CtrCentred_SetVBlankCallback(void (*callback)(void))
{
    VitaCentred_SetVBlankCallback(callback, 0);
}

bool VitaStage_IsStageActive(void)
{
    return (sStageCallback != NULL && gMain.vblankCallback == sStageCallback);
}

uint32_t VitaStage_GetCentredScreen(void)
{
    if (sCentredCallback != NULL && gMain.vblankCallback == sCentredCallback)
        return sCentredScreen;
    return 0;
}

static bool IsLeavesScene(void)
{
    if (!VitaStage_IsStageActive())
        return false;

    // Emerald 3DS: In scene_1 (leaves falling), BG0..3 CNT registers are precisely configured
    for (uint32_t bg = 0; bg < 4; ++bg)
    {
        const uint16_t expected = (uint16_t)(bg | ((16 + 2 * bg) << 8) | 0x8000);
        if (GetGpuReg((uint16_t)(REG_OFFSET_BG0CNT + bg * 2)) != expected)
            return false;
    }
    return true;
}

int32_t VitaStage_GetActiveKind(void)
{
    if (VitaStage_IsStageActive())
    {
        if (IsLeavesScene())
            return VITA_STAGE_INTRO_LEAVES;

        return VITA_STAGE_INTRO_BIKE;
    }

    return VITA_STAGE_NONE;
}

void VitaStage_UpdateSceneState(void)
{
    // Check if stage callback was cleared
    if (gMain.vblankCallback != sStageCallback)
        sStageCallback = NULL;
    if (gMain.vblankCallback != sCentredCallback)
    {
        sCentredCallback = NULL;
        sCentredScreen = 0;
    }
}

bool VitaStage_LeavesLoad(void)
{
    if (sLeavesTried)
        return sLeavesData != NULL;

    sLeavesTried = true;
    size_t size = 0;
    sLeavesData = VitaPlatform_LoadAsset("ui/intro_margins.bin", &size);
    if (!sLeavesData || size != LEAVES_BYTES || memcmp(sLeavesData, "EM3DLVS1", 8) != 0)
    {
        if (sLeavesData)
        {
            free(sLeavesData);
            sLeavesData = NULL;
        }
        return false;
    }
    return true;
}

void VitaStage_LeavesFree(void)
{
    for (int32_t bg = 0; bg < 4; ++bg)
    {
        if (sLeavesTex[bg] != 0)
        {
            glDeleteTextures(1, &sLeavesTex[bg]);
            sLeavesTex[bg] = 0;
        }
        sLeavesPalHash[bg] = 0;
    }
    if (sLeavesData)
    {
        free(sLeavesData);
        sLeavesData = NULL;
    }
    sLeavesTried = false;
}

static uint32_t HashPalette(void)
{
    uint32_t hash = 5381;
    for (int32_t i = 0; i < 256; i++)
        hash = ((hash << 5) + hash) + gPlttBufferFaded[i];
    return hash;
}

// Layer format of intro_margins.bin (tools/ui_gen/gen_intro_margins.py), per BG:
//   [0]                          sky colour above the art (palette index, 0xFF = none)
//   [1 .. 1 + 256 * 64)          left strip: per row, 56 margin pixels from the farthest
//                                (screen x = -56) to the one beside the picture (x = -1),
//                                then 8 patch pixels over the picture's own edge (unused here)
//   [.. + 256 * 56)              right strip: per row, 56 pixels from the picture's edge outwards
// A byte is bank << 4 | colour, a BG palette index; 0xFF and colour 0 are transparent.
// In the texture (128 x 256): x 0..55 is the left strip in screen order (55 beside the picture),
// x 64..119 the right strip (64 beside the picture). Rows are map rows 0..255.
#define LEAVES_TEX_W 128
#define LEAVES_RIGHT_TEX_X 64
#define LEAVES_TRANSPARENT 0xFF

static uint16_t LeafTexel(const uint8_t byte)
{
    if (byte == LEAVES_TRANSPARENT || (byte & 0x0F) == 0)
        return 0;
    const uint16_t bgr555 = gPlttBufferFaded[byte];
    const uint16_t r = bgr555 & 0x1F;
    const uint16_t g = (bgr555 >> 5) & 0x1F;
    const uint16_t b = (bgr555 >> 10) & 0x1F;
    return (uint16_t)((r << 11) | (g << 6) | (b << 1) | 1); // RGBA5551
}

static void UpdateLeavesTexture(int32_t bg)
{
    if (!VitaStage_LeavesLoad())
        return;

    const uint32_t palHash = HashPalette();
    if (sLeavesTex[bg] != 0 && sLeavesPalHash[bg] == palHash)
        return;
    sLeavesPalHash[bg] = palHash;

    if (sLeavesTex[bg] == 0)
    {
        glGenTextures(1, &sLeavesTex[bg]);
        glBindTexture(GL_TEXTURE_2D, sLeavesTex[bg]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, sLeavesTex[bg]);
    }

    const uint8_t *const layer = sLeavesData + 8 + bg * LEAVES_LAYER;
    const uint8_t *const leftData = layer + 1;
    const uint8_t *const rightData = leftData + LEAVES_ROWS * (LEAVES_LEFT + LEAVES_PATCH);

    for (int32_t y = 0; y < LEAVES_ROWS; ++y)
    {
        uint16_t *const row = &sLeavesPixels[y * LEAVES_TEX_W];

        memset(row, 0, LEAVES_TEX_W * sizeof(uint16_t));
        for (int32_t x = 0; x < LEAVES_LEFT; ++x)
            row[x] = LeafTexel(leftData[y * (LEAVES_LEFT + LEAVES_PATCH) + x]);
        for (int32_t x = 0; x < LEAVES_RIGHT; ++x)
            row[LEAVES_RIGHT_TEX_X + x] = LeafTexel(rightData[y * LEAVES_RIGHT + x]);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LEAVES_TEX_W, LEAVES_ROWS, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
                 sLeavesPixels);
}

// Map rows 0..255 of a leaves layer hold its art; rows 256..511 only its sky.
#define LEAVES_MAP_ROWS 512
#define LEAVES_ART_ROWS 256

static void DrawLeavesQuad(const float x0, const float x1, const float y0, const float y1,
                           const float u0, const float u1, const float v0, const float v1)
{
    const float verts[8] = {x0, y0, x1, y0, x1, y1, x0, y1};
    const float uvs[8] = {u0, v0, u1, v0, u1, v1, u0, v1};

    glVertexPointer(2, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void DrawLeavesSky(const uint8_t sky, const float x0, const float x1, const float y0, const float y1)
{
    const uint16_t bgr555 = gPlttBufferFaded[sky];
    const float verts[8] = {x0, y0, x1, y0, x1, y1, x0, y1};

    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColor4f((float)(bgr555 & 0x1F) / 31.0f, (float)((bgr555 >> 5) & 0x1F) / 31.0f,
              (float)((bgr555 >> 10) & 0x1F) / 31.0f, 1.0f);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

// One layer's strips beside the picture, in screen pixels, following the layer's vertical scroll
// as Emerald 3DS does (3ds_video.c DrawLeavesMargins).
static void DrawLeavesLayer(const int32_t bg, const float pictureX0, const float pictureX1, const float scale)
{
    const float marginGba = pictureX0 / scale;
    const float leftU0 = ((float)LEAVES_LEFT - marginGba) / (float)LEAVES_TEX_W;
    const float leftU1 = (float)LEAVES_LEFT / (float)LEAVES_TEX_W;
    const float rightU0 = (float)LEAVES_RIGHT_TEX_X / (float)LEAVES_TEX_W;
    const float rightU1 = ((float)LEAVES_RIGHT_TEX_X + marginGba) / (float)LEAVES_TEX_W;
    const float screenW = pictureX0 + pictureX1;
    const uint8_t sky = sLeavesData[8 + bg * LEAVES_LAYER];
    const int32_t scrollY = *(volatile uint16_t *)(REG_ADDR_BG0VOFS + bg * 4) & (LEAVES_MAP_ROWS - 1);
    int32_t a = 0;

    glBindTexture(GL_TEXTURE_2D, sLeavesTex[bg]);
    while (a < DISPLAY_HEIGHT)
    {
        const int32_t row = (a + scrollY) & (LEAVES_MAP_ROWS - 1);
        const int32_t rowsLeft = (row < LEAVES_ART_ROWS ? LEAVES_ART_ROWS : LEAVES_MAP_ROWS) - row;
        const int32_t b = (a + rowsLeft > DISPLAY_HEIGHT) ? DISPLAY_HEIGHT : a + rowsLeft;
        const float y0 = (float)a * scale;
        const float y1 = (float)b * scale;

        if (row < LEAVES_ART_ROWS)
        {
            const float v0 = (float)row / (float)LEAVES_ART_ROWS;
            const float v1 = (float)(row + (b - a)) / (float)LEAVES_ART_ROWS;

            DrawLeavesQuad(0.0f, pictureX0, y0, y1, leftU0, leftU1, v0, v1);
            DrawLeavesQuad(pictureX1, screenW, y0, y1, rightU0, rightU1, v0, v1);
        }
        else if (sky != LEAVES_TRANSPARENT)
        {
            DrawLeavesSky(sky, 0.0f, pictureX0, y0, y1);
            DrawLeavesSky(sky, pictureX1, screenW, y0, y1);
        }
        a = b;
    }
}

bool VitaStage_RenderLeavesMargins(const float pictureX0, const float pictureX1, const float scale)
{
    if (!IsLeavesScene() || !VitaStage_LeavesLoad())
        return false;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, pictureX0 + pictureX1, (double)DISPLAY_HEIGHT * scale, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    for (int32_t bg = 3; bg >= 0; --bg) // back to front, as the scene's priorities are 3..0
    {
        UpdateLeavesTexture(bg);
        if (sLeavesTex[bg] != 0)
            DrawLeavesLayer(bg, pictureX0, pictureX1, scale);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_ALPHA_TEST);
    return true;
}

void VitaStage_RenderWidescreenMargins(float vx0, float vx1, float vy0, float vy1)
{
    const int32_t stageKind = VitaStage_GetActiveKind();
    if (stageKind == VITA_STAGE_NONE)
        return;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 960.0, 544.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (stageKind == VITA_STAGE_INTRO_LEAVES)
    {
        // Render the 4 parallax leaves layers in the margins with alpha fade to black at edges
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        for (int32_t bg = 3; bg >= 0; --bg)
        {
            UpdateLeavesTexture(bg);
            if (sLeavesTex[bg] == 0)
                continue;

            glBindTexture(GL_TEXTURE_2D, sLeavesTex[bg]);

            // Left margin quad [0.0 .. vx0]
            const float uLeft0 = 0.0f;
            const float uLeft1 = (float)LEAVES_LEFT / 128.0f;
            const float leftVerts[8] = {
                0.0f, vy0,
                vx0,  vy0,
                vx0,  vy1,
                0.0f, vy1
            };
            const float leftUvs[8] = {
                uLeft0, 0.0f,
                uLeft1, 0.0f,
                uLeft1, 1.0f,
                uLeft0, 1.0f
            };
            // Alpha: 0.0 at outer screen edge, 1.0 at GBA boundary
            const float leftColors[16] = {
                1.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f
            };

            glVertexPointer(2, GL_FLOAT, 0, leftVerts);
            glTexCoordPointer(2, GL_FLOAT, 0, leftUvs);
            glColorPointer(4, GL_FLOAT, 0, leftColors);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            // Right margin quad [vx1 .. 960.0]
            const float uRight0 = 64.0f / 128.0f;
            const float uRight1 = (float)(64 + LEAVES_RIGHT) / 128.0f;
            const float rightVerts[8] = {
                vx1,    vy0,
                960.0f, vy0,
                960.0f, vy1,
                vx1,    vy1
            };
            const float rightUvs[8] = {
                uRight0, 0.0f,
                uRight1, 0.0f,
                uRight1, 1.0f,
                uRight0, 1.0f
            };
            const float rightColors[16] = {
                1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f
            };

            glVertexPointer(2, GL_FLOAT, 0, rightVerts);
            glTexCoordPointer(2, GL_FLOAT, 0, rightUvs);
            glColorPointer(4, GL_FLOAT, 0, rightColors);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_ALPHA_TEST);
    }
}
