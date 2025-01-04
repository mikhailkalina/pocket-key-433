#pragma once

#include <stdint.h>

namespace Slot
{
    // Number of slot items
    static constexpr uint8_t slotsCount = 10;
    static constexpr uint8_t invalidIdx = slotsCount;
    // Maximum name length
    static constexpr uint8_t nameLengthMax = 12;

#pragma pack(push, 1)
    /**
     * @brief Radio signal structure
     */
    struct Signal
    {
        uint32_t value;
        uint8_t protocol;
        uint8_t bitLength;

        bool operator==(const Signal &other) const
        {
            return ((protocol == other.protocol) &&
                    (bitLength == other.bitLength) &&
                    (value == other.value));
        }
    };
#pragma pack(pop)

    static constexpr Signal signalInvalid = {0, 0, 0};

    /**
     * @brief Return signal from specified slot
     *
     * @param slotIdx Slot identifier
     * @param signal Object to copy current slot signal
     */
    void getSignal(uint8_t slotIdx, Signal &signal);

    /**
     * @brief Set signal to specified slot
     *
     * @param slotIdx Slot identifier
     * @param signal New slot signal
     */
    void setSignal(uint8_t slotIdx, const Signal &signal);

    /**
     * @brief Return slot name
     *
     * @param slotIdx Slot identifier
     * @param name String to copy current slot name (at least nameLengthMax + 1 size)
     */
    void getName(uint8_t slotIdx, char *name);

    /**
     * @brief Set slot name
     *
     * @param slotIdx Slot identifier
     * @param name New slot name (null-terminated string)
     */
    void setName(uint8_t slotIdx, const char *name);

    /**
     * @brief Erase all slots on the storage
     */
    void eraseStorage();
} // namespace Slot
