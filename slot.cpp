#include "slot.h"

#include <stdint.h>

#include <CRC.h>
#include <EEPROM.h>

#include "log.h"

// #define LOG_DEBUG // Uncomment to enable log printing

using namespace Slot;

namespace
{
#pragma pack(push, 1)
    /**
     * @brief Slot item structure
     */
    struct SlotItem
    {
        char name[nameLengthMax + 1]; // + 1 for end of line
        Signal signal;
    };
#pragma pack(pop)

    // Slot item size + CRC size
    constexpr uint8_t slotEepromSize = sizeof(SlotItem) + sizeof(uint8_t);

    // Storage slot list
    SlotItem slotList[slotsCount] = {0};
} // namespace

/**
 * @brief Return signal for specified slot
 *
 * @param slotIdx Slot identifier
 * @return Slot signal
 */
const Signal &Slot::getSignal(uint8_t slotIdx)
{
    if (slotIdx < slotsCount)
    {
        const Signal &signal = slotList[slotIdx].signal;

#ifdef LOG_DEBUG
        Log::printf("Get slot[%u] signal: %u %lu/%u", slotIdx, signal.protocol, signal.value, signal.bitLength);
#endif // LOG_DEBUG

        return signal;
    }

#ifdef LOG_DEBUG
    Log::printf("Get slot[%u] signal: invalid", slotIdx);
#endif // LOG_DEBUG

    return signalInvalid;
}

/**
 * @brief Set new signal to specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal New slot signal
 */
void Slot::setSignal(uint8_t slotIdx, const Signal &signal)
{
    if (slotIdx < slotsCount)
    {
        SlotItem &slot = slotList[slotIdx]; // slot to set signal

#ifdef LOG_DEBUG
        Log::printf("Set slot[%u] signal: %u %lu/%u", slotIdx, signal.protocol, signal.value, signal.bitLength);
#endif // LOG_DEBUG

        slot.signal = signal;
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
    const char *name = nullptr;

    if (slotIdx < slotsCount)
    {
        name = slotList[slotIdx].name;

#ifdef LOG_DEBUG
        Log::printf("Get slot[%u] name: %s", slotIdx, name);
#endif // LOG_DEBUG
    }

    return name;
}

/**
 * @brief Set new slot name
 *
 * @param slotIdx Slot identifier
 * @param name New slot name (null-terminated string)
 */
void Slot::setName(uint8_t slotIdx, const char *name)
{
    if (slotIdx < slotsCount)
    {
        SlotItem &slot = slotList[slotIdx]; // slot to set name

#ifdef LOG_DEBUG
        Log::printf("Set slot[%u] name: %s", slotIdx, name);
#endif // LOG_DEBUG

        // Set name to default
        snprintf(slot.name, sizeof(slot.name), "%s", name);
    }
}

/**
 * @brief Reset slot to default
 *
 * @param slotIdx
 */
void Slot::reset(uint8_t slotIdx)
{
    if (slotIdx < slotsCount)
    {
        SlotItem &slot = slotList[slotIdx]; // slot to reset

#ifdef LOG_DEBUG
        Log::printf("Reset slot[%u]", slotIdx);
#endif // LOG_DEBUG

        // Reset name to default
        snprintf(slot.name, sizeof(slot.name), "Slot %02d", slotIdx + 1);

        // Invalidate the signal
        slot.signal = signalInvalid;
    }
}

/**
 * @brief Save slot to the storage
 *
 * @param slotIdx Slot identifier
 */
void Slot::save(uint8_t slotIdx)
{
    if (slotIdx < slotsCount)
    {
        const SlotItem &slot = slotList[slotIdx]; // slot to save
        uint8_t crc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
        int slotAddress = slotIdx * slotEepromSize;
        int crc8Address = slotAddress + sizeof(SlotItem);

#ifdef LOG_DEBUG
        Log::printf("Save slot[%u]", slotIdx);
#endif // LOG_DEBUG

        EEPROM.put(slotAddress, slot);
        EEPROM.put(crc8Address, crc8);
    }
}

/**
 * @brief Load all slots from the storage
 *
 * @return Number of valid slots
 */
uint8_t Slot::loadAll()
{
    uint8_t validCount = 0;

    for (uint8_t slotIdx = 0; slotIdx < slotsCount; slotIdx++)
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
            // Check if slot is valid on EEPROM
            if (slot.signal == signalInvalid)
            {
#ifdef LOG_DEBUG
                Log::printf("Load slot[%u]: \"%s\" invalid", slotIdx, slot.name);
#endif // LOG_DEBUG
            }
            else
            {
#ifdef LOG_DEBUG
                Log::printf("Load slot[%u]: \"%s\" %u %lu/%u", slotIdx, slot.name,
                            slot.signal.protocol, slot.signal.value, slot.signal.bitLength);
#endif // LOG_DEBUG

                validCount++;
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

    return validCount;
}

/**
 * @brief Erase all slots on the storage
 */
void Slot::eraseAll()
{
    const int itemsSize = slotsCount * (sizeof(SlotItem) + sizeof(uint8_t)); // Slot item size + CRC size
    for (int idx = 0; idx < itemsSize; idx++)
    {
        // Erase storage with 0xFF
        EEPROM.write(idx, 0xFF);
    }
}