#include "slot.h"

#include <stdint.h>
#include <stdio.h>

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
    constexpr uint8_t slotStorageSize = sizeof(SlotItem) + sizeof(uint8_t);
    static_assert(slotStorageSize == 20);

    /**
     * @brief Save slot item to the storage
     *
     * @param slotIdx Slot identifier
     * @param item Slot item to save
     */
    void save(uint8_t slotIdx, const SlotItem &item)
    {
        int slotAddress = slotIdx * slotStorageSize;
        int crc8Address = slotAddress + sizeof(SlotItem);
        uint8_t crc8 = calcCRC8((const uint8_t *)&item, sizeof(item));

#ifdef LOG_DEBUG
        Log::printf("Save slot[%u]: \"%s\" %02u 0x%02lX/%u", slotIdx, item.name,
                    item.signal.protocol, item.signal.value, item.signal.bitLength);
#endif // LOG_DEBUG

        EEPROM.put(slotAddress, item);
        EEPROM.put(crc8Address, crc8);
    }

    /**
     * @brief Reset slot item to default values
     *
     * @param slotIdx Slot identifier
     * @param item Slot item to save
     */
    void reset(uint8_t slotIdx, SlotItem &item)
    {
        // Reset name to default
        snprintf(item.name, sizeof(item.name), "Slot %02d", slotIdx + 1);

        // Invalidate the signal
        item.signal = signalInvalid;

#ifdef LOG_DEBUG
        Log::printf("Reset slot[%u]", slotIdx);
#endif // LOG_DEBUG

        // Save to the storage
        save(slotIdx, item);
    }

    /**
     * @brief Load slot item from the storage
     *
     * @param slotIdx Slot identifier
     * @param item Slot item to load
     */
    void load(uint8_t slotIdx, SlotItem &item)
    {
        int slotAddress = slotIdx * slotStorageSize;
        int crc8Address = slotAddress + sizeof(SlotItem);
        uint8_t crc8 = 0;

        EEPROM.get(slotAddress, item);
        EEPROM.get(crc8Address, crc8);

        uint8_t calcCrc8 = calcCRC8((const uint8_t *)&item, sizeof(item));
        if (calcCrc8 != crc8)
        {
            // Reset slot if it isn't valid
            reset(slotIdx, item);
        }

#ifdef LOG_DEBUG
        Log::printf("Load slot[%u]: \"%s\" %02u 0x%02lX/%u", slotIdx, item.name,
                    item.signal.protocol, item.signal.value, item.signal.bitLength);
#endif // LOG_DEBUG
    }
} // namespace

/**
 * @brief Return signal from specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal Object to copy current slot signal
 */
void Slot::getSignal(uint8_t slotIdx, Signal &signal)
{
    if (slotIdx < slotsCount)
    {
        // Load current slot item
        SlotItem item;
        load(slotIdx, item);

        // Copy slot signal
        signal = item.signal;
    }
    else
    {
        // Set invalid signal
        signal = signalInvalid;
    }
}

/**
 * @brief Set signal to specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal New slot signal
 */
void Slot::setSignal(uint8_t slotIdx, const Signal &signal)
{
    if (slotIdx < slotsCount)
    {
        // Load current slot item
        SlotItem item;
        load(slotIdx, item);

        // Copy new signal and save updated item
        item.signal = signal;
        save(slotIdx, item);
    }
}

/**
 * @brief Return slot name
 *
 * @param slotIdx Slot identifier
 * @param name String to copy current slot name (at least nameLengthMax + 1 size)
 */
void Slot::getName(uint8_t slotIdx, char *name)
{
    if (slotIdx < slotsCount)
    {
        // Load current slot item
        SlotItem item;
        load(slotIdx, item);

        // Copy slot name
        snprintf(name, sizeof(item.name), "%s", item.name);
    }
    else
    {
        // Set empty name
        name[0] = '\0';
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
        // Load current slot item
        SlotItem item;
        load(slotIdx, item);

        // Copy new name and save updated item
        snprintf(item.name, sizeof(item.name), "%s", name);
        save(slotIdx, item);
    }
}

/**
 * @brief Erase all slots on the storage
 */
void Slot::eraseStorage()
{
    const int storageSize = slotsCount * slotStorageSize;
    for (int idx = 0; idx < storageSize; idx++)
    {
        // Erase storage with 0xFF
        EEPROM.write(idx, 0xFF);
    }
}