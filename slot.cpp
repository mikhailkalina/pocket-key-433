#include "slot.h"

#include <stdint.h>

#include <CRC.h>
#include <EEPROM.h>

#include "log.h"

using namespace Slot;

namespace
{
    constexpr uint8_t nameLengthMax = 10;

    /**
     * @brief Slot item structure
     */
    struct SlotItem
    {
        char name[nameLengthMax + 1]; // 10 chars + 1 end of line
        bool isFilled;
        Signal signal;
    };

    // Slot item size + CRC size
    constexpr uint8_t slotEepromSize = sizeof(SlotItem) + sizeof(uint8_t);

    // Storage slot list
    SlotItem slotList[itemsCount] = {0};
} // namespace

/**
 * @brief Return signal for specified slot
 *
 * @param slotIdx Slot identifier
 * @return Slot signal
 */
const Signal &Slot::getSignal(uint8_t slotIdx)
{
    const SlotItem &slot = slotList[slotIdx]; // slot to get signal

    return slot.signal;
}

/**
 * @brief Set new signal to specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal New slot signal
 */
void Slot::setSignal(uint8_t slotIdx, const Signal &signal)
{
    SlotItem &slot = slotList[slotIdx]; // slot to set signal

    if (slot.isFilled == false)
    {
        Log::printf("Fill %s: %u %lu/%u", slot.name, signal.protocol, signal.value, signal.bitLength);

        slot.signal = signal;
        slot.isFilled = true;
    }
}

/**
 * @brief Return current slot name
 *
 * @param slotIdx Slot identifier
 * @return Slot name (null-terminated string)
 */
const char *Slot::getName(uint8_t slotIdx)
{
    const SlotItem &slot = slotList[slotIdx]; // slot to get name

    return slot.name;
}

/**
 * @brief Set new slot name
 *
 * @param slotIdx Slot identifier
 * @param name New slot name (null-terminated string)
 */
void Slot::setName(uint8_t slotIdx, const char *name)
{
    SlotItem &slot = slotList[slotIdx]; // slot to set name

    // Set name to default
    snprintf(slot.name, sizeof(slot.name), "%s", name);
}

/**
 * @brief Reset slot to default
 *
 * @param slotIdx
 */
void Slot::reset(uint8_t slotIdx)
{
    SlotItem &slot = slotList[slotIdx]; // slot to reset

    // Reset name to default
    snprintf(slot.name, sizeof(slot.name), "Slot %02d", slotIdx + 1);

    Log::printf("Reset %s", slot.name);

    // Empty the slot
    slot.isFilled = false;
    // Invalidate the signal
    slot.signal = signalInvalid;
}

/**
 * @brief Save slot to the storage
 *
 * @param slotIdx Slot identifier
 */
void Slot::save(uint8_t slotIdx)
{
    const SlotItem &slot = slotList[slotIdx]; // slot to save
    uint8_t crc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
    int slotAddress = slotIdx * slotEepromSize;
    int crc8Address = slotAddress + sizeof(SlotItem);

    Log::printf("Save %s", slot.name);

    EEPROM.put(slotAddress, slot);
    EEPROM.put(crc8Address, crc8);
}

/**
 * @brief Load all slots from the storage
 *
 * @return Number of filled slots
 */
uint8_t Slot::loadAll()
{
    uint8_t filledCount = 0;

    for (uint8_t slotIdx = 0; slotIdx < itemsCount; slotIdx++)
    {
        SlotItem &slot = slotList[slotIdx]; // slot to load
        uint8_t crc8 = 0;
        int slotAddress = slotIdx * slotEepromSize;
        int crc8Address = slotAddress + sizeof(SlotItem);

        EEPROM.get(slotAddress, slot);
        EEPROM.get(crc8Address, crc8);

        uint8_t calcCrc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
        if (calcCrc8 == crc8)
        {
            // Slot is valid on EEPROM
            if (slot.isFilled == true)
            {
                Log::printf("Load %s: %u %lu/%u", slot.name, slot.signal.protocol, slot.signal.value, slot.signal.bitLength);
                filledCount++;
            }
            else
            {
                Log::printf("%s is empty", slot.name);
            }
        }
        else
        {
            // Reset slot if it isn't valid
            reset(slotIdx);
            // Save to the EEPROM
            save(slotIdx);
        }
    }

    return filledCount;
}

/**
 * @brief Erase all slots on the storage
 */
void Slot::eraseAll()
{
    const int itemsSize = itemsCount * (sizeof(SlotItem) + sizeof(uint8_t)); // Slot item size + CRC size
    for (int idx = 0; idx < itemsSize; idx++)
    {
        // Erase storage with 0xFF
        EEPROM.write(idx, 0xFF);
    }
}