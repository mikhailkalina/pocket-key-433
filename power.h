#pragma once

namespace Power
{
  // Active HIGH supply controls. Replace the pin drivers here for external keys.
  /**
   * @brief Configure all peripheral supply control pins as outputs, initially off
   */
  void initialize();

  /**
   * @brief Switch display power and wait for startup when enabling it
   *
   * @param enabled true to supply the display, false to turn it off
   */
  void setDisplay(bool enabled);

  /**
   * @brief Switch receiver power and wait for startup when enabling it
   *
   * @param enabled true to supply the receiver, false to turn it off
   */
  void setReceiver(bool enabled);

  /**
   * @brief Switch transmitter power and wait for startup when enabling it
   *
   * @param enabled true to supply the transmitter, false to turn it off
   */
  void setTransmitter(bool enabled);
} // namespace Power
