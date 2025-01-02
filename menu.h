#pragma once

namespace Menu
{
    /**
     * @brief Menu actions to process
     */
    enum class Action
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
    enum class FunctionState
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
        const char *text;
        const Item *prev;
        const Item *next;
        const Item *child;
        Callback callback;
        int param;
        const Item *parent;
    };

    /**
     * @brief Process menu action
     *
     * @param pItem Current menu item
     * @param action New action
     * @return New menu item if selected, nullptr otherwise
     */
    const Item *process(const Item *pItem, Action action);
} // namespace Menu
