#include "menu.h"

#include <stdint.h>

using namespace Menu;

namespace
{
    // Menu item's functionality was active on the previous processing
    bool isFunctionActive = false;

    const Item *navigate(const Item *pItem, Action action)
    {
        const Item *pNewItem = nullptr;

        switch (action)
        {
        case Action::Prev:
            pNewItem = pItem->prev;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pItem->parent;
            }
            break;

        case Action::Next:
            pNewItem = pItem->next;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pItem->parent;
            }
            break;

        case Action::Enter:
            pNewItem = pItem->child;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pItem;
            }
            break;

        case Action::Back:
            pNewItem = pItem->parent;
            break;

        case Action::Exit:
            pNewItem = pItem;
            break;

        default:
            break;
        }

        return pNewItem;
    }
} // namespace

/**
 * @brief Process menu action
 *
 * @param pItem Current menu item
 * @param action New action
 * @return New menu item if selected, current item if its functionality finished, nullptr otherwise
 */
const Item *Menu::process(const Item *pItem, Action action)
{
    const Item *pNewItem = nullptr;

    if (pItem != nullptr)
    {
        FunctionState functionState = FunctionState::Inactive;

        if (pItem->callback != nullptr)
        {
            // Process item's functionality
            functionState = pItem->callback(action, pItem->param);
        }

        if (functionState == FunctionState::Active)
        {
            isFunctionActive = true;
        }
        else if (isFunctionActive == true)
        {
            // Functionality just finished - stay on the item instead of navigating with the same action
            isFunctionActive = false;
            pNewItem = pItem;
        }
        else
        {
            // Navigate through the menu according to the action
            pNewItem = navigate(pItem, action);
        }
    }

    return pNewItem;
}
