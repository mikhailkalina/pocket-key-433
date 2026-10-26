#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace Battery
{
  // Highest charge level, level 0 means empty battery
  constexpr uint8_t levelMax = 4;

  /**
   * @brief Initialize battery voltage and charging status readings
   */
  void initialize();

  /**
   * @brief Process battery readings periodically
   * Should be called from the main loop
   */
  void process();

  /**
   * @brief Return filtered battery voltage
   *
   * @return Battery voltage, millivolts
   */
  uint16_t getVoltage();

  /**
   * @brief Return battery charge level with hysteresis applied
   *
   * @return Charge level from 0 (empty) to levelMax (full)
   */
  uint8_t getLevel();

  /**
   * @brief Return debounced battery charging status
   *
   * @return true if battery is charging, false otherwise
   */
  bool isCharging();

  /**
   * @brief Return the unfiltered D8 charger signal for sleep decisions
   *
   * @return true while the active-LOW charger status pin is LOW
   */
  bool isChargeSignalActive();
} // namespace Battery
