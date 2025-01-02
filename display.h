#pragma once

#include <stdbool.h>
#include <stdint.h>

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
   *
   * @param xPos Horisontal position, pixels
   * @param line Vertical line identifier
   * @param format Formatted string
   * @param ... Parameters for formatted string
   */
  void printf(uint8_t xPos, Line line, const char *format, ...);

  /**
   * @brief Clear the screen
   */
  void clear();
} // namespace Display
