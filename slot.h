#pragma once

#include <stdint.h>

namespace Slot
{
    // Number of slot items
    static constexpr uint8_t slotsCount = 10;
    static constexpr uint8_t invalidIdx = slotsCount;
    // Maximum name length
    static constexpr uint8_t nameLengthMax = 11;

#pragma pack(push, 1)
    /**
     * @brief Radio signal structure
     */
    struct Signal
    {
        uint8_t protocol;
        uint8_t bitLength;
        uint32_t value;

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
     * @brief Return signal for specified slot
     *
     * @param slotIdx Slot identifier
     * @return Slot signal
     */
    const Signal &getSignal(uint8_t slotIdx);

    /**
     * @brief Set new signal to specified slot
     *
     * @param slotIdx Slot identifier
     * @param signal New slot signal
     */
    void setSignal(uint8_t slotIdx, const Signal &signal);

    /**
     * @brief Return current slot name
     *
     * @param slotIdx Slot identifier
     * @return Slot name (null-terminated string)
     */
    const char *getName(uint8_t slotIdx);

    /**
     * @brief Set new slot name
     *
     * @param slotIdx Slot identifier
     * @param name New slot name (null-terminated string)
     */
    void setName(uint8_t slotIdx, const char *name);

    /**
     * @brief Reset slot to default
     *
     * @param slotIdx Slot identifier
     */
    void reset(uint8_t slotIdx);

    /**
     * @brief Save slot to the storage
     *
     * @param slotIdx Slot identifier
     */
    void save(uint8_t slotIdx);

    /**
     * @brief Load all slots from the storage
     *
     * @return Number of valid slots
     */
    uint8_t loadAll();

    /**
     * @brief Erase all slots on the storage
     */
    void eraseAll();
} // namespace Slot
