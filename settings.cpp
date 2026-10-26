#include "settings.h"

#include <EEPROM.h>
#include <avr/pgmspace.h>

#include "slot.h"

namespace
{
  // Slots occupy the first 20 * 20 bytes. Keep application settings after them.
  constexpr int storageAddress = Slot::storageBytes;
  constexpr uint8_t magic = 0xA7;
  constexpr uint8_t version = 3;
  const uint16_t sleepSeconds[] PROGMEM = {0, 10, 20, 30, 40, 50, 60, 120, 180, 240, 300};
  static_assert(sizeof(sleepSeconds) / sizeof(*sleepSeconds) == Settings::sleepOptionCount);

  struct StoredSettings
  {
    uint8_t magic;
    uint8_t version;
    uint8_t sleepOption;
    uint8_t check;
  };

  uint8_t sleepOption = Settings::sleepOptionDefault;

  /**
   * @brief Calculate the integrity byte for a settings record
   *
   * @param data Settings record to check
   * @return XOR check byte
   */
  uint8_t calculateCheck(const StoredSettings &data)
  {
    return data.magic ^ data.version ^ data.sleepOption ^ 0x5A;
  }
} // namespace

/**
 * @brief Load the saved sleep option or initialize EEPROM with the default
 *
 * Version 2 values are migrated to the current option indices.
 */
void Settings::initialize()
{
  StoredSettings data;
  EEPROM.get(storageAddress, data);
  const bool valid = data.magic == magic && data.check == calculateCheck(data);
  if (valid && data.version == version && data.sleepOption < sleepOptionCount)
  {
    sleepOption = data.sleepOption;
  }
  else
  {
    // In version 2 the same choices used indices 0..9, without Off.
    const uint8_t initialOption = (valid && data.version == 2 && data.sleepOption < 10)
                                      ? data.sleepOption + 1 : Settings::sleepOptionDefault;
    sleepOption = 0xFF;
    Settings::setSleepOption(initialOption);
  }
}

/**
 * @brief Return the selected sleep option index
 *
 * @return Index from 0 (Off) to sleepOptionCount - 1
 */
uint8_t Settings::getSleepOption()
{
  return sleepOption;
}

/**
 * @brief Return the selected inactivity timeout
 *
 * @return Timeout in seconds, or 0 when automatic sleep is disabled
 */
uint16_t Settings::getSleepSeconds()
{
  return pgm_read_word(&sleepSeconds[sleepOption]);
}

/**
 * @brief Save a new sleep option to EEPROM when it changes
 *
 * @param option Index from 0 (Off) to sleepOptionCount - 1
 */
void Settings::setSleepOption(uint8_t option)
{
  if (option >= sleepOptionCount || option == sleepOption)
  {
    return;
  }

  sleepOption = option;
  StoredSettings data = {magic, version, sleepOption, 0};
  data.check = calculateCheck(data);
  EEPROM.put(storageAddress, data); // update semantics avoid needless writes
}
