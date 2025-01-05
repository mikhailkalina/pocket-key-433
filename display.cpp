#include "display.h"

#include <stdbool.h>
#include <stdint.h>

#include <ssd1306.h>

using namespace Display;

namespace
{
    constexpr uint8_t headerOffsetXPix = 0;
    constexpr uint8_t linesOffsetXPix = 4;
    constexpr uint8_t navOffsetXPix = 1;

    constexpr uint8_t textSize6x8CharWidthPix = 6;
    constexpr uint8_t textSize6x8LengthMax = 21;
    constexpr uint8_t textSize8x16CharWidthPix = 8;
    constexpr uint8_t textSize8x16LengthMax = 16;

    constexpr uint8_t lineOffsets[] = {0, 16, 24, 32, 40, 48, 56};
    static_assert(sizeof(lineOffsets) / sizeof(*lineOffsets) == static_cast<uint8_t>(Line::Count));

    // Current inverted mode
    bool isInvertedPermanent = false;
    bool isInvertedInUse = isInvertedPermanent;
    // Current font style
    Style stylePermanent = Style::Normal;
    Style styleInUse = stylePermanent;
    // Current font size
    Size sizePermanent = Size::Font_6x8;
    Size sizeInUse = sizePermanent;

    // ssd1306 display font style
    EFontStyle fontStyle = STYLE_NORMAL;

    // Local buffer for text string
    char buffer[textSize6x8LengthMax + 1];
    // Character width, pixels
    uint8_t charWidthPix = textSize6x8CharWidthPix;
    // Maximum length of the text
    uint8_t lengthMax = textSize6x8LengthMax;
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
 * @brief Set inverted mode
 *
 * @param isInverted true for inverted mode, false for normal
 * @param isPermanent true if set mode is permament, false if temporary
 */
void Display::setInverted(bool isInverted, bool isPermanent = false)
{
    if (isInvertedInUse != isInverted)
    {
        isInvertedInUse = isInverted;
        if (isInvertedInUse == true)
        {
            ssd1306_negativeMode();
        }
        else
        {
            ssd1306_positiveMode();
        }
    }

    if (isPermanent == true && isInvertedPermanent != isInverted)
    {
        isInvertedPermanent = isInverted;
    }
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
        case Size::Font_6x8:
            ssd1306_setFixedFont(ssd1306xled_font6x8);
            charWidthPix = textSize6x8CharWidthPix;
            lengthMax = textSize6x8LengthMax;
            break;

        case Size::Font_8x16:
            ssd1306_setFixedFont(ssd1306xled_font8x16);
            charWidthPix = textSize8x16CharWidthPix;
            lengthMax = textSize8x16LengthMax;
            break;

        default:
            sizeInUse = Size::Font_6x8;
            ssd1306_setFixedFont(ssd1306xled_font6x8);
            charWidthPix = textSize6x8CharWidthPix;
            lengthMax = textSize6x8LengthMax;
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
 * @param charOffset Horisontal position offset from the left, characters
 * @param line Vertical line identifier
 * @param format Formatted string
 * @param ... Parameters for formatted string
 */
void Display::printf(uint8_t charOffset, Line line, const char *format, ...)
{
    if (charOffset >= lengthMax || line >= Line::Count)
    {
        return;
    }

    va_list args;

    va_start(args, format);
    vsnprintf(buffer, lengthMax + 1, format, args);
    va_end(args);

    uint8_t xPos = charOffset * charWidthPix;
    uint8_t yPos = lineOffsets[(uint8_t)line];

    if (line == Line::Header)
    {
        Display::setStyle(Display::Style::Bold);
        Display::setSize(Display::Size::Font_8x16);
        xPos += headerOffsetXPix;
    }
    else if (line == Line::Navigation)
    {
        xPos += navOffsetXPix;
    }
    else
    {
        xPos += linesOffsetXPix;
    }

    ssd1306_printFixed(xPos, yPos, buffer, fontStyle);

    if (isInvertedInUse != isInvertedPermanent)
    {
        // Return permanent inverted mode in use
        setInverted(isInvertedPermanent);
    }

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

/**
 * @brief Clear the screen
 */
void Display::clear()
{
    ssd1306_clearScreen();
}
