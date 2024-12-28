#pragma once

#include "button.h"

// Menu function prototype
typedef void (*MenuFunction)(int param);

/**
 * @brief Navigate user through the menu according to the buttons actions
 * 
 * @param buttonId Button that triggered the action
 */
void MENU_navigate(ButtonId buttonId);

/**
 * @brief Draw menu
 */
void MENU_draw();
