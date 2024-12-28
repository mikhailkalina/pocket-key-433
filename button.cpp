#include <Arduino.h>

#include "button.h"

#define BTN_UP_PIN (4)
#define BTN_DOWN_PIN (5)
#define BTN_LEFT_PIN (6)
#define BTN_RIGHT_PIN (7)

#define BTN_DEBOUNCE_TIME_MS (20)
#define BTN_HOLD_TIME_MS (1000)

/**
 * @brief Button structure
 */
struct Button
{
    const uint8_t pin;
    const uint8_t activeLevel;
    ButtonState state;
    ButtonAction action;
    bool isActive;
    unsigned long activeTimeMs;
};

// Button list
static Button buttonList[BTN_ID_COUNT] = {
    {.pin = BTN_UP_PIN, .activeLevel = LOW},    // BTN_UP
    {.pin = BTN_DOWN_PIN, .activeLevel = LOW},  // BTN_DOWN
    {.pin = BTN_LEFT_PIN, .activeLevel = LOW},  // BTN_LEFT
    {.pin = BTN_RIGHT_PIN, .activeLevel = LOW}, // BTN_RIGHT
};

/**
 * @brief Initialize buttons
 */
void BTN_initialize()
{
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
    pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
    pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
}

/**
 * @brief Handle buttons changes 
 * 
 * @return true if new button action detected, false otherwise
 */
bool BTN_handle()
{
    bool isActionDetected = false;
    // Get current system time
    unsigned long currentTimeMs = millis();

    for (size_t btnId = 0; btnId < BTN_ID_COUNT; btnId++)
    {
        Button &button = buttonList[btnId]; // current button
        button.action = BTN_ACTION_NONE;    // No action by default

        uint8_t buttonPinLevel = digitalRead(button.pin);
        if (buttonPinLevel == button.activeLevel)
        {
            // Button is active
            if (button.isActive == false)
            {
                // First time detect active level
                button.isActive = true;
                button.activeTimeMs = currentTimeMs;
            }
            else if (button.state == BTN_STATE_RELEASED &&
                     currentTimeMs > button.activeTimeMs + BTN_DEBOUNCE_TIME_MS)
            {
                // Debounce time passed after press
                button.state = BTN_STATE_PRESSED;
                button.action = BTN_ACTION_PRESS;
            }
            else if (button.state == BTN_STATE_PRESSED &&
                     currentTimeMs > button.activeTimeMs + BTN_HOLD_TIME_MS)
            {
                // Hold time passed after press
                button.state = BTN_STATE_HOLD;
                button.action = BTN_ACTION_LONG_PRESS;
            }
        }
        else if (button.isActive == true)
        {
            button.isActive = false;
            if (button.state != BTN_STATE_RELEASED)
            {
                // Determine action type according to state
                button.action = (button.state == BTN_STATE_PRESSED) ? BTN_ACTION_CLICK : BTN_ACTION_HOLD_END;
                // Button is released
                button.state = BTN_STATE_RELEASED;
            }
        }

        if (button.action != BTN_ACTION_NONE)
        {
            isActionDetected = true;
        }
    }

    return isActionDetected;
}

/**
 * @brief Return current state for specified button
 * 
 * @param btnId Button identifier
 * @return Button state
 */
ButtonState BTN_getState(ButtonId btnId)
{
    const Button &button = buttonList[btnId];

    return button.state;
}

/**
 * @brief Return detected action for specified button
 * 
 * @param btnId Button identifier
 * @return Button action
 */
ButtonAction BTN_getAction(ButtonId btnId)
{
    const Button &button = buttonList[btnId];

    return button.action;
}
