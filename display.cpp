#include "display.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

    /**
     * @brief Return vertical pixel mask with the specified rows set
     *
     * @param top Top row, bit N is pixel row N
     * @param bottom Bottom row
     * @return Pixel mask
     */
    constexpr uint16_t pixelRows(uint8_t top, uint8_t bottom)
    {
        return (uint16_t)((2UL << bottom) - (1UL << top));
    }

    // Battery indicator in the top right corner, 2 display pages high:
    // body outline columns 0..14 with bars inside, tip column 15
    constexpr uint8_t batteryWidthPix = 16;
    constexpr uint8_t batteryHeightPix = 16;
    constexpr uint8_t batteryXPix = 128 - batteryWidthPix;
    constexpr uint8_t batteryBodyWidthPix = 15;
    constexpr uint8_t batteryBarOffsetPix = 2; // First bar column, after outline and gap
    constexpr uint8_t batteryBarWidthPix = 2;
    constexpr uint8_t batteryBarStepPix = 3; // Bar width and gap after it
    static_assert(batteryBarOffsetPix + batteryBarsMax * batteryBarStepPix + 1 == batteryBodyWidthPix,
                  "Battery bars do not fit the body");

    // Pixel rows of battery indicator columns, aligned with the header font capital letters
    constexpr uint16_t batteryOutlineRows = pixelRows(3, 12);
    constexpr uint16_t batteryInsideRows = pixelRows(3, 3) | pixelRows(12, 12);
    constexpr uint16_t batteryBarRows = pixelRows(5, 10);
    constexpr uint16_t batteryTipRows = pixelRows(6, 9);

    // Current battery indicator bars, batteryBarsHidden if indicator is not shown
    constexpr uint8_t batteryBarsHidden = 0xFF;
    uint8_t batteryBars = batteryBarsHidden;
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
void Display::setInverted(bool isInverted, bool isPermanent)
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

namespace
{
    /**
     * @brief Draw text buffer to the specified position of the display
     *
     * @param charOffset Horisontal position offset from the left, characters
     * @param line Vertical line identifier
     */
    void drawBuffer(uint8_t charOffset, Line line)
    {
        uint8_t xPos = 0;
        uint8_t yPos = lineOffsets[(uint8_t)line];

        if (line == Line::Header)
        {
            Display::setSize(Display::Size::Font_8x16);
            xPos = headerOffsetXPix;
        }
        else if (line == Line::Navigation)
        {
            xPos = navOffsetXPix;
        }
        else
        {
            xPos = linesOffsetXPix;
        }

        // Character offset is calculated after header font size is applied
        xPos += charOffset * charWidthPix;

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
     * @brief Draw battery indicator with current bars
     */
    void drawBattery()
    {
        // Top display page bytes followed by bottom page bytes
        uint8_t bitmap[batteryWidthPix * batteryHeightPix / 8];

        for (uint8_t x = 0; x < batteryWidthPix; x++)
        {
            uint16_t rows = batteryInsideRows;

            if (x == 0 || x == batteryBodyWidthPix - 1)
            {
                rows = batteryOutlineRows;
            }
            else if (x >= batteryBodyWidthPix)
            {
                rows = batteryTipRows;
            }
            else if (x >= batteryBarOffsetPix)
            {
                uint8_t barX = x - batteryBarOffsetPix;
                if (barX / batteryBarStepPix < batteryBars && barX % batteryBarStepPix < batteryBarWidthPix)
                {
                    rows |= batteryBarRows;
                }
            }

            bitmap[x] = (uint8_t)rows;
            bitmap[batteryWidthPix + x] = (uint8_t)(rows >> 8);
        }

        ssd1306_drawBuffer(batteryXPix, 0, batteryWidthPix, batteryHeightPix, bitmap);
    }
} // namespace

/**
 * @brief Print formatted string to the specified position of the display
 *
 * @param charOffset Horisontal position offset from the left, characters
 * @param line Vertical line identifier
 * @param format Formatted string located in flash memory
 * @param ... Parameters for formatted string
 */
void Display::printf_P(uint8_t charOffset, Line line, const char *format, ...)
{
    if (charOffset >= lengthMax || line >= Line::Count)
    {
        return;
    }

    va_list args;

    va_start(args, format);
    vsnprintf_P(buffer, lengthMax + 1, format, args);
    va_end(args);

    drawBuffer(charOffset, line);
}

/**
 * @brief Print string to the specified position of the display
 * String is truncated and padded with spaces to the specified width
 *
 * @param charOffset Horisontal position offset from the left, characters
 * @param line Vertical line identifier
 * @param width Width of printed string, characters
 * @param string String located in RAM
 */
void Display::printString(uint8_t charOffset, Line line, uint8_t width, const char *string)
{
    if (charOffset >= lengthMax || line >= Line::Count)
    {
        return;
    }

    // Copy string truncated and padded with spaces to the specified width
    width = (width < lengthMax) ? width : lengthMax;
    uint8_t length = strnlen(string, width);
    memcpy(buffer, string, length);
    memset(&buffer[length], ' ', width - length);
    buffer[width] = '\0';

    drawBuffer(charOffset, line);
}

/**
 * @brief Print string to the specified position of the display
 * String is truncated and padded with spaces to the specified width
 *
 * @param charOffset Horisontal position offset from the left, characters
 * @param line Vertical line identifier
 * @param width Width of printed string, characters
 * @param string String located in flash memory
 */
void Display::printString_P(uint8_t charOffset, Line line, uint8_t width, const char *string)
{
    if (charOffset >= lengthMax || line >= Line::Count)
    {
        return;
    }

    // Copy string from flash memory truncated and padded with spaces to the specified width
    width = (width < lengthMax) ? width : lengthMax;
    uint8_t length = strnlen_P(string, width);
    memcpy_P(buffer, string, length);
    memset(&buffer[length], ' ', width - length);
    buffer[width] = '\0';

    drawBuffer(charOffset, line);
}

/**
 * @brief Clear the screen
 * Battery indicator is restored if shown
 */
void Display::clear()
{
    ssd1306_clearScreen();

    if (batteryBars != batteryBarsHidden)
    {
        drawBattery();
    }
}

/**
 * @brief Show battery indicator in the top right corner of the screen
 * Header text must not exceed 13 characters to leave room for the indicator
 *
 * @param bars Number of filled bars, up to batteryBarsMax
 */
void Display::showBattery(uint8_t bars)
{
    if (bars > batteryBarsMax)
    {
        bars = batteryBarsMax;
    }

    // Redraw on change only
    if (bars != batteryBars)
    {
        batteryBars = bars;
        drawBattery();
    }
}
