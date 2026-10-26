#include "power.h"

#include <Arduino.h>

namespace
{
  constexpr uint8_t displayPowerPin = 9;
  constexpr uint8_t receiverPowerPin = 3;
  constexpr uint8_t transmitterPowerPin = 11;

  // Display delay is measured on this device. Radio delays are provisional:
  // their required startup times depend on the exact module and power circuit.
  constexpr unsigned long displayStartupMs = 100;
  constexpr unsigned long receiverStartupMs = 10;
  constexpr unsigned long transmitterStartupMs = 2;

  /**
   * @brief Configure a supply control pin as an output without a HIGH glitch
   *
   * @param pin Digital pin to configure
   */
  void initializeOff(uint8_t pin)
  {
    digitalWrite(pin, LOW); // pre-load the output latch before enabling output
    pinMode(pin, OUTPUT);
  }
} // namespace

/**
 * @brief Configure all peripheral supply control pins as outputs, initially off
 */
void Power::initialize()
{
  initializeOff(displayPowerPin);
  initializeOff(receiverPowerPin);
  initializeOff(transmitterPowerPin);
}

/**
 * @brief Switch display power and wait for startup when enabling it
 *
 * @param enabled true to supply the display, false to turn it off
 */
void Power::setDisplay(bool enabled)
{
  digitalWrite(displayPowerPin, enabled ? HIGH : LOW);
  if (enabled)
  {
    delay(displayStartupMs);
  }
}

/**
 * @brief Switch receiver power and wait for startup when enabling it
 *
 * @param enabled true to supply the receiver, false to turn it off
 */
void Power::setReceiver(bool enabled)
{
  digitalWrite(receiverPowerPin, enabled ? HIGH : LOW);
  if (enabled)
  {
    delay(receiverStartupMs);
  }
}

/**
 * @brief Switch transmitter power and wait for startup when enabling it
 *
 * @param enabled true to supply the transmitter, false to turn it off
 */
void Power::setTransmitter(bool enabled)
{
  digitalWrite(transmitterPowerPin, enabled ? HIGH : LOW);
  if (enabled)
  {
    delay(transmitterStartupMs);
  }
}
