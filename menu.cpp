#include "menu.h"

#include <stdint.h>

#include "display.h"

using namespace Menu;

namespace
{
    constexpr uint8_t pageItemCount = 5;
    constexpr uint8_t headerHeightPix = 16;
    constexpr uint8_t itemHeightPix = 8;
    constexpr uint8_t navInfoYPosPix = headerHeightPix + pageItemCount * itemHeightPix;

    // String for root item header
    const char rootHeader[] = "Pocket Key";

    const Item *pCurrentItem = nullptr;
} // namespace

/**
 * @brief Navigate user through the menu according to the buttons actions
 *
 * @param buttonId Button that triggered the action
 * @return Current selected menu item
 */
const Item *Menu::navigate(Action action)
{
    if (pCurrentItem != nullptr)
    {
        Item *pNewItem = pCurrentItem;

        switch (action)
        {
        case Action::Prev:
            pNewItem = pCurrentItem->prev;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pCurrentItem->parent;
            }
            break;

        case Action::Next:
            pNewItem = pCurrentItem->next;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pCurrentItem->parent;
            }
            break;

        case Action::Enter:
            pNewItem = pCurrentItem->child;
            if (pNewItem != nullptr)
            {
                // Save parent to exit properly
                pNewItem->parent = pCurrentItem;
            }
            if (pCurrentItem->enterHandler.callback != nullptr)
            {
                pCurrentItem->enterHandler.callback(pCurrentItem->enterHandler.param);
            }
            break;

        case Action::Exit:
            pNewItem = pCurrentItem->parent;
            if (pCurrentItem->exitHandler.callback != nullptr)
            {
                pCurrentItem->exitHandler.callback(pCurrentItem->exitHandler.param);
            }
            break;

        default:
            break;
        }

        if (pNewItem != nullptr && pNewItem != pCurrentItem)
        {
            draw(pNewItem);
        }
    }

    return pCurrentItem;
}

/**
 * @brief Draw specified menu item
 * Set menu item as current for navigation
 *
 * @param pItem Menu item to draw
 */
void Menu::draw(const Item *pItem)
{
    if (pItem != nullptr)
    {
        pCurrentItem = pItem;

        // Show parent header text
        const char *headerText = pCurrentItem->parent != nullptr ? pCurrentItem->parent->text : rootHeader;
        Display::setStyle(Display::Style::Bold);
        Display::setSize(Display::Size::Bits_16);
        Display::printf(0, 0, "%-16.16s", headerText);

        // Count previous items
        uint8_t prevItemCount = 0;
        pItem = pCurrentItem->prev;
        while (pItem != nullptr)
        {
            prevItemCount++;
            pItem = pItem->prev;
        }

        // Count next items
        uint8_t nextItemCount = 0;
        pItem = pCurrentItem->next;
        while (pItem != nullptr)
        {
            nextItemCount++;
            pItem = pItem->next;
        }

        // Show navigation info
        uint8_t itemIdx = prevItemCount;
        uint8_t itemsCount = prevItemCount + 1 + nextItemCount;
        Display::printf(0, navInfoYPosPix, "%2u/%-2u", itemIdx + 1, itemsCount);

        // Find the first item on current page
        uint8_t itemOffset = itemIdx % pageItemCount;
        pItem = pCurrentItem;
        while (itemOffset > 0)
        {
            pItem = pItem->prev;
            itemOffset--;
        }

        // Fill menu items on current page
        while (itemOffset < pageItemCount)
        {
            const uint8_t yPosPix = headerHeightPix + itemOffset * itemHeightPix;

            if (pItem == pCurrentItem)
            {
                Display::setStyle(Display::Style::Bold);
                Display::printf(0, yPosPix, ">%-20.20s", pItem->text);
            }
            else
            {
                Display::printf(0, yPosPix, " %-20.20s", pItem ? pItem->text : "");
            }

            pItem = pItem ? pItem->next : nullptr;
            itemOffset++;
        }
    }
}
