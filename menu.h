#pragma once

namespace Menu
{
    /**
     * @brief Menu action identifiers
     */
    enum class Action
    {
        None,
        Prev,
        Next,
        Enter,
        Back,
        Exit,
    };

    // Menu entered function prototype
    typedef void (*Callback)(Action action, int param);

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
