#include "display.h"

#include <stdbool.h>
#include <stdint.h>

#include <ssd1306.h>

using namespace Display;

namespace
{
    constexpr uint8_t textSize8LengthMax = 21 + 1;
    constexpr uint8_t textSize16LengthMax = 21 + 1;

    // Current font style
    Style stylePermanent = Style::Normal;
    Style styleInUse = stylePermanent;
    // Current font size
    Size sizePermanent = Size::Bits_8;
    Size sizeInUse = sizePermanent;

    // ssd1306 display font style
    EFontStyle fontStyle = STYLE_NORMAL;

    // Local buffer for text string
    char buffer[textSize8LengthMax];
    // Maximum length of the text
    uint8_t lengthMax = textSize8LengthMax;
} // namespace

/**
 * @brief Initialize display
 */
void Display::initialize()
{
    ssd1306_128x64_i2c_init();
    ssd1306_clearScreen();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
}

/**
 * @brief Set font style for printed text
 *
 * @param style New font style
 * @param isPermanent true if set style is permament, false if temporary
 */
void Display::setStyle(Style style, bool isPermanent)
{
    if (styleInUse != style)
    {
        styleInUse = style;
        switch (styleInUse)
        {
        case Style::Normal:
            fontStyle = STYLE_NORMAL;
            break;

        case Style::Bold:
            fontStyle = STYLE_BOLD;
            break;

        case Style::Italic:
            fontStyle = STYLE_ITALIC;
            break;

        default:
            styleInUse = Style::Normal;
            fontStyle = STYLE_NORMAL;
            break;
        }
    }

    if (isPermanent == true && stylePermanent != style)
    {
        stylePermanent = style;
    }
}

/**
 * @brief Set font size for printed text
 *
 * @param size New font size
 * @param isPermanent true if set size is permament, false if temporary
 */
void Display::setSize(Size size, bool isPermanent)
{
    if (sizeInUse != size)
    {
        sizeInUse = size;
        switch (sizeInUse)
        {
        case Size::Bits_8:
            ssd1306_setFixedFont(ssd1306xled_font6x8);
            lengthMax = textSize8LengthMax;
            break;

        case Size::Bits_16:
            ssd1306_setFixedFont(ssd1306xled_font8x16);
            lengthMax = textSize16LengthMax;
            break;

        default:
            sizeInUse = Size::Bits_8;
            ssd1306_setFixedFont(ssd1306xled_font6x8);
            lengthMax = textSize8LengthMax;
            break;
        }
    }

    if (isPermanent == true && sizePermanent != size)
    {
        sizePermanent = size;
    }
}

/**
 * @brief Print formatted string to the specified position of the display
 *
 * @param xPos Horisontal position, pixels
 * @param yPos Vertical position, pixels
 * @param style Style of the printed text
 * @param size Size of the printed text
 * @param format Formatted string
 * @param ... Parameters for formatted string
 */
void Display::printf(uint8_t xPos, uint8_t yPos, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, lengthMax, format, args);
    va_end(args);

    ssd1306_printFixed(xPos, yPos, buffer, fontStyle);

    if (styleInUse != stylePermanent)
    {
        // Return permament style back in use
        setStyle(stylePermanent);
    }

    if (sizeInUse != sizePermanent)
    {
        // Return permament size back in use
        setSize(sizePermanent);
    }
}
