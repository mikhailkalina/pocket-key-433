#include <Arduino.h>
#include <CRC.h>
#include <EEPROM.h>

#include "log.h"
#include "slot.h"

#define SLOT_NAME_LENGTH_MAX (10)

/**
 * @brief Storage slot structure
 */
struct Slot
{
    char name[SLOT_NAME_LENGTH_MAX + 1]; // 10 chars + 1 end of line
    bool isFilled;
    Signal signal;
};

// Storage slot list
Slot slotList[SLOT_COUNT_MAX] = {0};

/**
 * @brief Return signal for specified slot
 *
 * @param slotIdx Slot identifier
 * @return Slot signal
 */
const Signal &SLOT_getSignal(int slotIdx)
{
    const Slot &slot = slotList[slotIdx]; // slot to get signal

    return slot.signal;
}

/**
 * @brief Set new signal to specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal New slot signal
 */
void SLOT_setSignal(int slotIdx, const Signal &signal)
{
    Slot &slot = slotList[slotIdx]; // slot to set signal

    if (slot.isFilled == false)
    {
        LOG_printf("Fill %s: %u %lu/%u", slot.name, signal.protocol, signal.value, signal.bitLength);

        slot.signal = signal;
        slot.isFilled = true;
    }
}

/**
 * @brief Set new slot name
 *
 * @param slotIdx Slot identifier
 * @param name New slot name (null-terminated string)
 */
void SLOT_setName(int slotIdx, const char *name)
{
    Slot &slot = slotList[slotIdx]; // slot to set name

    // Set name to default
    snprintf(slot.name, sizeof(slot.name), "%s", name);
}

/**
 * @brief Reset slot to default
 *
 * @param slotIdx
 */
void SLOT_reset(int slotIdx)
{
    Slot &slot = slotList[slotIdx]; // slot to reset

    // Reset name to default
    snprintf(slot.name, sizeof(slot.name), "Slot %02d", slotIdx + 1);

    LOG_printf("Reset %s", slot.name);
    
    // Empty the slot
    slot.isFilled = false;
    // Invalidate the signal
    slot.signal = SGNL_getInvalid();
}

/**
 * @brief Save slot to the storage
 *
 * @param slotIdx Slot identifier
 */
void SLOT_save(int slotIdx)
{
    const Slot &slot = slotList[slotIdx]; // slot to save
    uint8_t crc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
    int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8)); // Slot size + CRC  size
    int crc8Address = slotAddress + sizeof(Slot);

    LOG_printf("Save %s", slot.name);

    EEPROM.put(slotAddress, slot);
    EEPROM.put(crc8Address, crc8);
}

/**
 * @brief Load all slots from the storage
 *
 * @return Number of filled slots
 */
size_t SLOT_loadAll()
{
    size_t filledCount = 0;

    for (size_t slotIdx = 0; slotIdx < SLOT_COUNT_MAX; slotIdx++)
    {
        Slot &slot = slotList[slotIdx]; // slot to load
        uint8_t crc8 = 0;
        int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8)); // Slot size + CRC size
        int crc8Address = slotAddress + sizeof(Slot);

        EEPROM.get(slotAddress, slot);
        EEPROM.get(crc8Address, crc8);

        uint8_t calcCrc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
        if (calcCrc8 == crc8)
        {
            // Slot is valid on EEPROM
            if (slot.isFilled == true)
            {
                LOG_printf("Load %s: %u %lu/%u", slot.name, slot.signal.protocol, slot.signal.value, slot.signal.bitLength);
                filledCount++;
            }
            else
            {
                LOG_printf("%s is empty", slot.name);
            }
        }
        else
        {
            // Reset slot if it isn't valid
            SLOT_reset(slotIdx);
            // Save to the EEPROM
            SLOT_save(slotIdx);
        }
    }

    return filledCount;
}

/**
 * @brief Erase all slots on the storage
 */
void SLOT_eraseAll()
{
    const int storageSize = SLOT_COUNT_MAX * (sizeof(Slot) + sizeof(uint8_t)); // Slot size + CRC size
    for (int idx = 0; idx < storageSize; idx++)
    {
        // Erase storage with 0xFF
        EEPROM.write(idx, 0xFF);
    }
}