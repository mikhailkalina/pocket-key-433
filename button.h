/**
 * @brief Button identifiers
 */
enum ButtonId
{
  BTN_ID_UP,
  BTN_ID_DOWN,
  BTN_ID_LEFT,
  BTN_ID_RIGHT,
  BTN_ID_COUNT, // Should be the last one
  BTN_ID_NONE = BTN_ID_COUNT,
};

/**
 * @brief Button states
 */
enum ButtonState
{
  BTN_STATE_RELEASED,
  BTN_STATE_PRESSED,
  BTN_STATE_HOLD,
};

/**
 * @brief Button action types
 */
enum ButtonAction
{
  BTN_ACTION_NONE,
  BTN_ACTION_PRESS,
  BTN_ACTION_LONG_PRESS,
  BTN_ACTION_CLICK,
  BTN_ACTION_HOLD_END,
};

/**
 * @brief Initialize buttons
 */
void BTN_initialize();

/**
 * @brief Handle buttons changes 
 * 
 * @return true if new button action detected, false otherwise
 */
bool BTN_handle();

/**
 * @brief Return current state for specified button
 * 
 * @param btnId Button identifier
 * @return Button state
 */
ButtonState BTN_getState(ButtonId btnId);

/**
 * @brief Return detected action for specified button
 * 
 * @param btnId Button identifier
 * @return Button action
 */
ButtonAction BTN_getAction(ButtonId btnId);
