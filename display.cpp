#include <Arduino.h>
#include <ssd1306.h>

#include "display.h"

#define TEXT_SIZE_8_LENGTH_MAX (21)
#define TEXT_SIZE_16_LENGTH_MAX (16)

// Local buffer for text string
static char buffer[TEXT_SIZE_8_LENGTH_MAX + 1];

/**
 * @brief Initialize display
 */
void DISP_initialize()
{
    ssd1306_128x64_i2c_init();
    ssd1306_clearScreen();
}

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
void DISP_printf(uint8_t xPos, uint8_t yPos, EFontStyle style, TextSize size, const char *format, ...)
{
    uint8_t lengthMax = 0;

    switch (size)
    {
    case TEXT_SIZE_8:
        ssd1306_setFixedFont(ssd1306xled_font6x8);
        lengthMax = TEXT_SIZE_8_LENGTH_MAX;
        break;

    case TEXT_SIZE_16:
        ssd1306_setFixedFont(ssd1306xled_font8x16);
        lengthMax = TEXT_SIZE_16_LENGTH_MAX;
        break;

    default:
        ssd1306_setFixedFont(ssd1306xled_font6x8);
        lengthMax = TEXT_SIZE_8_LENGTH_MAX;
        break;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, lengthMax + 1, format, args);
    va_end(args);

    // Clear the rest of the buffer
    for (size_t idx = strlen(buffer); idx < lengthMax; idx++)
    {
        buffer[idx] = ' ';
    }
    buffer[lengthMax] = '\0'; // end of line

    ssd1306_printFixed(xPos, yPos, buffer, style);
}