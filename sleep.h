#pragma once

namespace Sleep
{
  /**
   * @brief Enter power-down sleep until a button or charger pin change wakes the AVR
   *
   * Returns immediately if a button is held or the active-LOW charger signal is LOW.
   */
  void untilWakeSource();
} // namespace Sleep
