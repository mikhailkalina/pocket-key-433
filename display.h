#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace Display
{
  /**
   * @brief Font style identifiers
   */
  enum class Style
  {
    Normal,
    Bold,
    Italic,
  };

  /**
   * @brief Font size identifiers
   */
  enum class Size
  {
    Bits_8,  // 8 bit height
    Bits_16, // 16 bit height
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
   * @param yPos Vertical position, pixels
   * @param format Formatted string
   * @param ... Parameters for formatted string
   */
  void printf(uint8_t xPos, uint8_t yPos, const char *format, ...);

  /**
   * @brief Clear the screen
   */
  void clear();
} // namespace Display
