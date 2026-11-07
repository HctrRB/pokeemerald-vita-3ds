#include <stdio.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>

#include "vita_input.h"
#include "vita_platform.h"

// Analog stick deflection thresholds (0-255, center 128)
static const uint8_t STICK_DEADZONE_LOW  = 64U;
static const uint8_t STICK_DEADZONE_HIGH = 192U;

// Touch screen dimensions
static const float TOUCH_PANEL_MAX_X = 1920.0f;
static const float TOUCH_PANEL_MAX_Y = 1088.0f;
static const float SCREEN_WIDTH_F    = 960.0f;
static const float SCREEN_HEIGHT_F   = 544.0f;

// GBA centered viewport on Vita display (816x544, offset 72px)
static const int32_t GBA_VIEWPORT_OFFSET_X = 72;
static const int32_t GBA_VIEWPORT_WIDTH    = 816;
static const int32_t GBA_VIEWPORT_HEIGHT   = 544;
static const int32_t GBA_NATIVE_WIDTH      = 240;
static const int32_t GBA_NATIVE_HEIGHT     = 160;

// Internal persistent state
static VitaInputState sCurrentState;
static uint32_t sPreviousButtons = 0U;

bool VitaInput_Init(void)
{
    // Configure sampling mode for both analog sticks
    const int32_t ctrlResult = sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    if (ctrlResult < 0)
    {
        VITA_LOG("[INPUT] sceCtrlSetSamplingMode failed: 0x%08X\n", ctrlResult);
        return false;
    }

    // Start sampling the front capacitive touch panel
    const int32_t touchResult = sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    if (touchResult < 0)
    {
        VITA_LOG("[INPUT] sceTouchSetSamplingState failed: 0x%08X\n", touchResult);
        return false;
    }

    memset(&sCurrentState, 0, sizeof(sCurrentState));
    sPreviousButtons = 0U;

    VITA_LOG("[INPUT] Initialized controller and front touch panel.\n");
    return true;
}

static uint32_t DecodeButtons(const SceCtrlData *const pad)
{
    uint32_t buttons = VITA_BTN_NONE;

    if (pad->buttons & SCE_CTRL_CROSS)    buttons |= VITA_BTN_A;
    if (pad->buttons & SCE_CTRL_CIRCLE)   buttons |= VITA_BTN_B;
    if (pad->buttons & SCE_CTRL_SELECT)   buttons |= VITA_BTN_SELECT;
    if (pad->buttons & SCE_CTRL_START)    buttons |= VITA_BTN_START;
    if (pad->buttons & SCE_CTRL_SQUARE)   buttons |= VITA_BTN_SQUARE;
    if (pad->buttons & SCE_CTRL_TRIANGLE) buttons |= VITA_BTN_TRIANGLE;
    if (pad->buttons & SCE_CTRL_LTRIGGER) buttons |= VITA_BTN_L;
    if (pad->buttons & SCE_CTRL_RTRIGGER) buttons |= VITA_BTN_R;

    // Map D-pad and left analog stick
    if ((pad->buttons & SCE_CTRL_UP)    || pad->ly < STICK_DEADZONE_LOW)  buttons |= VITA_BTN_UP;
    if ((pad->buttons & SCE_CTRL_DOWN)  || pad->ly > STICK_DEADZONE_HIGH) buttons |= VITA_BTN_DOWN;
    if ((pad->buttons & SCE_CTRL_LEFT)  || pad->lx < STICK_DEADZONE_LOW)  buttons |= VITA_BTN_LEFT;
    if ((pad->buttons & SCE_CTRL_RIGHT) || pad->lx > STICK_DEADZONE_HIGH) buttons |= VITA_BTN_RIGHT;

    if (pad->ry < STICK_DEADZONE_LOW)  buttons |= VITA_BTN_RS_UP;
    if (pad->ry > STICK_DEADZONE_HIGH) buttons |= VITA_BTN_RS_DOWN;
    if (pad->rx < STICK_DEADZONE_LOW)  buttons |= VITA_BTN_RS_LEFT;
    if (pad->rx > STICK_DEADZONE_HIGH) buttons |= VITA_BTN_RS_RIGHT;

    // Hardware safety: a genuine GBA d-pad never reports opposing directions simultaneously
    if ((buttons & (VITA_BTN_UP | VITA_BTN_DOWN)) == (VITA_BTN_UP | VITA_BTN_DOWN))
    {
        buttons &= ~(VITA_BTN_UP | VITA_BTN_DOWN);
    }
    if ((buttons & (VITA_BTN_LEFT | VITA_BTN_RIGHT)) == (VITA_BTN_LEFT | VITA_BTN_RIGHT))
    {
        buttons &= ~(VITA_BTN_LEFT | VITA_BTN_RIGHT);
    }

    return buttons;
}

void VitaInput_Poll(VitaInputState *const outState)
{
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));

    if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0)
    {
        const uint32_t currentButtons = DecodeButtons(&pad);
        sCurrentState.buttonsHeld     = currentButtons;
        sCurrentState.buttonsPressed  = currentButtons & ~sPreviousButtons;
        sCurrentState.buttonsReleased = ~currentButtons & sPreviousButtons;
        sPreviousButtons              = currentButtons;

        sCurrentState.leftStickX  = pad.lx;
        sCurrentState.leftStickY  = pad.ly;
        sCurrentState.rightStickX = pad.rx;
        sCurrentState.rightStickY = pad.ry;
    }

    // Poll front touch screen
    SceTouchData touch;
    memset(&touch, 0, sizeof(touch));
    sCurrentState.touchActive = false;

    if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) > 0 && touch.reportNum > 0)
    {
        sCurrentState.touchActive = true;
        sCurrentState.touchScreenX = (int32_t)((float)touch.report[0].x * (SCREEN_WIDTH_F / TOUCH_PANEL_MAX_X));
        sCurrentState.touchScreenY = (int32_t)((float)touch.report[0].y * (SCREEN_HEIGHT_F / TOUCH_PANEL_MAX_Y));

        // Map to GBA coords if inside the active viewport
        const int32_t sx = sCurrentState.touchScreenX;
        const int32_t sy = sCurrentState.touchScreenY;

        if (sx >= GBA_VIEWPORT_OFFSET_X && sx < (GBA_VIEWPORT_OFFSET_X + GBA_VIEWPORT_WIDTH) &&
            sy >= 0 && sy < GBA_VIEWPORT_HEIGHT)
        {
            sCurrentState.touchGbaX = (sx - GBA_VIEWPORT_OFFSET_X) * GBA_NATIVE_WIDTH / GBA_VIEWPORT_WIDTH;
            sCurrentState.touchGbaY = sy * GBA_NATIVE_HEIGHT / GBA_VIEWPORT_HEIGHT;
        }
        else
        {
            sCurrentState.touchGbaX = -1;
            sCurrentState.touchGbaY = -1;
        }
    }

    if (outState != NULL)
    {
        *outState = sCurrentState;
    }
}

uint32_t VitaInput_GetButtonsHeld(void)
{
    return sCurrentState.buttonsHeld;
}

bool VitaInput_ReadTouchScreen(int32_t *const outX, int32_t *const outY)
{
    if (!sCurrentState.touchActive)
    {
        return false;
    }

    if (outX != NULL) *outX = sCurrentState.touchScreenX;
    if (outY != NULL) *outY = sCurrentState.touchScreenY;
    return true;
}

bool VitaInput_ReadTouchGBA(int32_t *const outGbaX, int32_t *const outGbaY)
{
    if (!sCurrentState.touchActive || sCurrentState.touchGbaX < 0 || sCurrentState.touchGbaY < 0)
    {
        return false;
    }

    if (outGbaX != NULL) *outGbaX = sCurrentState.touchGbaX;
    if (outGbaY != NULL) *outGbaY = sCurrentState.touchGbaY;
    return true;
}
