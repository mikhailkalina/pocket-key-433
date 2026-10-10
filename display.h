#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <avr/pgmspace.h>

namespace Display
{
  /**
   * @brief Font styles
   */
  enum class Style
  {
    Normal,
    Bold,
    Italic,
  };

  /**
   * @brief Font sizes
   */
  enum class Size
  {
    Font_6x8,  // 6 width x 8 height pixels
    Font_8x16, // 8 width x 16 height pixels
  };

  /**
   * @brief Line identifiers
   */
  enum class Line
  {
    Header,
    Line_1,
    Line_2,
    Line_3,
    Line_4,
    Line_5,
    Navigation,
    Count, // should be the last one
  };

  /**
   * @brief Initialize display
   */
  void initialize();

  /**
   * @brief Set inverted mode
   *
   * @param isInverted true for inverted mode, false for normal
   * @param isPermanent true if set mode is permament, false if temporary
   */
  void setInverted(bool isInverted, bool isPermanent = false);

  /**
   * @brief Set font style for printed text
   *
   * @param style New font style
   * @param isPermanent true if set style is permament, false if temporary
   */
  void setStyle(Style style, bool isPermanent = false);

  /**
   * @brief Set font size for printed text
   *
   * @param size New font size
   * @param isPermanent true if set size is permament, false if temporary
   */
  void setSize(Size size, bool isPermanent = false);

  /**
   * @brief Print formatted string to the specified position of the display
   * Use DISPLAY_PRINTF macro to get format string checked by compiler
   *
   * @param charOffset Horisontal position offset from the left, characters
   * @param line Vertical line identifier
   * @param format Formatted string located in flash memory
   * @param ... Parameters for formatted string
   */
  void printf_P(uint8_t charOffset, Line line, const char *format, ...);

  /**
   * @brief Check formatted string against parameters by compiler, never called
   *
   * @param format Formatted string
   * @param ... Parameters for formatted string
   */
  __attribute__((format(printf, 1, 2))) inline void checkFormat(const char *, ...) {}

  /**
   * @brief Print string to the specified position of the display
   * String is truncated and padded with spaces to the specified width
   *
   * @param charOffset Horisontal position offset from the left, characters
   * @param line Vertical line identifier
   * @param width Width of printed string, characters
   * @param string String located in RAM
   */
  void printString(uint8_t charOffset, Line line, uint8_t width, const char *string);

  /**
   * @brief Print string to the specified position of the display
   * String is truncated and padded with spaces to the specified width
   *
   * @param charOffset Horisontal position offset from the left, characters
   * @param line Vertical line identifier
   * @param width Width of printed string, characters
   * @param string String located in flash memory
   */
  void printString_P(uint8_t charOffset, Line line, uint8_t width, const char *string);

  /**
   * @brief Clear the screen
   */
  void clear();
} // namespace Display

/**
 * @brief Print formatted string to the specified position of the display
 * Format string is placed in flash memory and checked by compiler against parameters
 *
 * @param charOffset Horisontal position offset from the left, characters
 * @param line Vertical line identifier
 * @param format Formatted string literal
 * @param ... Parameters for formatted string
 */
#define DISPLAY_PRINTF(charOffset, line, format, ...)                  \
  do                                                                  \
  {                                                                   \
    if (false)                                                        \
    {                                                                 \
      Display::checkFormat(format, ##__VA_ARGS__);                    \
    }                                                                 \
    Display::printf_P(charOffset, line, PSTR(format), ##__VA_ARGS__); \
  } while (false)
