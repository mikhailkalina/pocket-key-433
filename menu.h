#pragma once

#include <stdint.h>

namespace Menu
{
    /**
     * @brief Menu actions to process
     */
    enum class Action : uint8_t
    {
        None,
        Prev,
        Next,
        Enter,
        Back,
        Exit,
        Set,
    };

    /**
     * @brief Menu item's functionality states
     */
    enum class FunctionState : uint8_t
    {
        Inactive,
        Active,
    };

    /**
     * @brief Menu item's functionality callback prototype
     *
     * @param action Menu action to proceed
     * @param param Menu item's parameter
     * @return Menu item's functionality state
     */
    typedef FunctionState (*Callback)(Action action, int param);

    /**
     * @brief Menu item structure
     */
    struct Item
    {
        const char *text; // Located in flash memory unless isTextInRam is set
        const Item *prev;
        const Item *next;
        const Item *child;
        Callback callback;
        int param;
        mutable const Item *parent; // Updated during navigation
        bool isTextInRam;
    };

    /**
     * @brief Process menu action
     *
     * @param pItem Current menu item
     * @param action New action
     * @return New menu item if selected, current item if its functionality finished, nullptr otherwise
     */
    const Item *process(const Item *pItem, Action action);
} // namespace Menu
