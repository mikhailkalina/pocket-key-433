#pragma once

#include <stdint.h>

namespace Settings
{
  // Off, 10..60 s in 10 s steps, then 2..5 min in 1 min steps.
  constexpr uint8_t sleepOptionCount = 11;
  constexpr uint8_t sleepOptionDefault = 6; // 60 seconds

  /**
   * @brief Load the saved sleep option or initialize EEPROM with the default
   */
  void initialize();

  /**
   * @brief Return the selected sleep option index
   *
   * @return Index from 0 (Off) to sleepOptionCount - 1
   */
  uint8_t getSleepOption();

  /**
   * @brief Return the selected inactivity timeout
   *
   * @return Timeout in seconds, or 0 when automatic sleep is disabled
   */
  uint16_t getSleepSeconds();

  /**
   * @brief Save a new sleep option to EEPROM when it changes
   *
   * @param option Index from 0 (Off) to sleepOptionCount - 1
   */
  void setSleepOption(uint8_t option);
} // namespace Settings
