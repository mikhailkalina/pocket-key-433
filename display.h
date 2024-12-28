#pragma once

#include <ssd1306.h>

/**
 * @brief Text size identifiers
 */
enum TextSize
{
  TEXT_SIZE_8,
  TEXT_SIZE_16,
  TEXT_SIZE_COUNT,
};

/**
 * @brief Initialize display
 */
void DISP_initialize();

/**
 * @brief Print formatted string to the specified position of the display
 * The rest of the row will be automatically cleaned
 * 
 * @param xPos Horisontal position, pixels
 * @param yPos Vertical position, pixels
 * @param style Normal/bold text style
 * @param size Size of the text
 * @param format Formatted string
 * @param ... Parameters for formatted string
 */
void DISP_printf(uint8_t xPos, uint8_t yPos, EFontStyle style, TextSize size, const char *format, ...);
