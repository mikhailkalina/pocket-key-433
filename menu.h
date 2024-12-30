#pragma once

#include <stdint.h>

namespace Menu
{
    // Menu entered function prototype
    typedef void (*Callback)(uint8_t param);

    /**
     * @brief Menu item action handler structure
     */
    struct ActionHandler
    {
        Callback callback;
        uint8_t param;
    };

    /**
     * @brief Menu item structure
     */
    struct Item
    {
        const char *text;
        const Item *prev;
        const Item *next;
        const Item *child;
        const Item *parent;
        ActionHandler enterHandler;
        ActionHandler exitHandler;
    };

    /**
     * @brief Menu action identifiers
     */
    enum class Action
    {
        None,
        Prev,
        Next,
        Enter,
        Exit,
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
