#define SLOT_COUNT_MAX (20)

#include "signal.h"

/**
 * @brief Return signal for specified slot
 *
 * @param slotIdx Slot identifier
 * @return Slot signal
 */
const Signal &SLOT_getSignal(int slotIdx);

/**
 * @brief Set new signal to specified slot
 *
 * @param slotIdx Slot identifier
 * @param signal New slot signal
 */
void SLOT_setSignal(int slotIdx, const Signal &signal);

/**
 * @brief Set new slot name
 *
 * @param slotIdx Slot identifier
 * @param name New slot name (null-terminated string)
 */
void SLOT_setName(int slotIdx, const char *name);

/**
 * @brief Reset slot to default
 *
 * @param slotIdx Slot identifier
 */
void SLOT_reset(int slotIdx);

/**
 * @brief Save slot to the storage
 *
 * @param slotIdx Slot identifier
 */
void SLOT_save(int slotIdx);

/**
 * @brief Load all slots from the storage
 *
 * @return Number of filled slots
 */
size_t SLOT_loadAll();

/**
 * @brief Erase all slots on the storage
 */
void SLOT_eraseAll();
