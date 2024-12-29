#pragma once

namespace Menu
{
    // Menu entered function prototype
    typedef void (*Callback)(int param);

    /**
     * @brief Menu item structure
     */
    struct Item
    {
        const Item *parent;
        const Item *prev;
        const Item *next;
        const Item *child;
        Callback callback;
        int param;
        const char *text;
    };

    /**
     * @brief Menu action identifiers
     */
    enum class Action
    {
        None,
        Exit,
        Prev,
        Next,
        Enter,
    };

    /**
     * @brief Navigate user through the menu according to the action
     *
     * @param action New action
     * @return Current selected menu item
     */
    const Item *navigate(Action action);

    /**
     * @brief Draw specified menu item
     * Set menu item as current for navigation
     *
     * @param pItem Menu item to draw
     */
    void draw(const Item *pItem);
} // namespace Menu
