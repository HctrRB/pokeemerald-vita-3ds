#ifndef GUARD_VITA_INPUT_H
#define GUARD_VITA_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "vita_platform.h"

#define VITA_BTN_NONE 0U
#define VITA_BTN_A VITA_BTN_CROSS
#define VITA_BTN_B VITA_BTN_CIRCLE

/**
 * @brief Structure holding unified input state for both physical pad and touchscreen.
 */
typedef struct {
    uint32_t buttonsPressed;
    uint32_t buttonsHeld;
    uint32_t buttonsReleased;
    uint8_t  leftStickX;
    uint8_t  leftStickY;
    uint8_t  rightStickX;
    uint8_t  rightStickY;
    bool     touchActive;
    int32_t  touchScreenX; // 0..959
    int32_t  touchScreenY; // 0..543
    int32_t  touchGbaX;    // 0..239
    int32_t  touchGbaY;    // 0..159
} VitaInputState;

/**
 * @brief Initializes the PS Vita control and touch hardware.
 * Enables analog sampling mode and starts front touch panel sampling.
 * @return True on success, false on error.
 */
bool VitaInput_Init(void);

/**
 * @brief Polls pad and touch states, updating internal edge triggers.
 * @param outState Pointer to destination state structure (may be NULL).
 */
void VitaInput_Poll(VitaInputState *const outState);

/**
 * @brief Reads the current raw button bitmask.
 * @return Bitmask of currently held buttons.
 */
uint32_t VitaInput_GetButtonsHeld(void);

/**
 * @brief Checks if front touch is currently active and retrieves screen coordinates.
 * @param outX Destination pointer for X coordinate (0..959).
 * @param outY Destination pointer for Y coordinate (0..543).
 * @return True if a touch report is active, false otherwise.
 */
bool VitaInput_ReadTouchScreen(int32_t *const outX, int32_t *const outY);

/**
 * @brief Checks if front touch is active within the GBA viewport and maps to 240x160.
 * @param outGbaX Destination pointer for GBA X coordinate (0..239).
 * @param outGbaY Destination pointer for GBA Y coordinate (0..159).
 * @return True if touch is within the active GBA screen, false otherwise.
 */
bool VitaInput_ReadTouchGBA(int32_t *const outGbaX, int32_t *const outGbaY);

#endif // GUARD_VITA_INPUT_H
