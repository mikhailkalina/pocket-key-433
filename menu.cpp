#include <Arduino.h>

#include "menu.h"
#include "display.h"

#define MENU_ROW_COUNT (5)
#define MENU_ROW_PIXELS (8)

/**
 * @brief Menu item structure
 */
struct MenuItem
{
    const MenuItem *parent;
    const MenuItem *prev;
    const MenuItem *next;
    const MenuItem *child;
    MenuFunction func;
    int param;
    const char *text;
};

// String for start display
const char startString[] = "Pocket Key 433";

extern const MenuItem slotsMenu;
extern const MenuItem settingsMenu;
extern const MenuItem infoMenu;

extern const MenuItem slot1Menu;
extern const MenuItem slot2Menu;
extern const MenuItem slot3Menu;
extern const MenuItem slot4Menu;
extern const MenuItem slot5Menu;
extern const MenuItem slot6Menu;
extern const MenuItem slot7Menu;
extern const MenuItem slot8Menu;
extern const MenuItem slot9Menu;
extern const MenuItem slot10Menu;

const MenuItem slotsMenu = {NULL, NULL, &settingsMenu, &slot1Menu, NULL, 0, "Slots"};
const MenuItem settingsMenu = {NULL, &slotsMenu, &infoMenu, NULL, NULL, 0, "Settings"};
const MenuItem infoMenu = {NULL, &settingsMenu, NULL, NULL, NULL, 0, "Info"};

const MenuItem slot1Menu = {&slotsMenu, NULL, &slot2Menu, NULL, NULL, 0, "Slot 1"};
const MenuItem slot2Menu = {&slotsMenu, &slot1Menu, &slot3Menu, NULL, NULL, 0, "Slot 2"};
const MenuItem slot3Menu = {&slotsMenu, &slot2Menu, &slot4Menu, NULL, NULL, 0, "Slot 3"};
const MenuItem slot4Menu = {&slotsMenu, &slot3Menu, &slot5Menu, NULL, NULL, 0, "Slot 4"};
const MenuItem slot5Menu = {&slotsMenu, &slot4Menu, &slot6Menu, NULL, NULL, 0, "Slot 5"};
const MenuItem slot6Menu = {&slotsMenu, &slot5Menu, &slot7Menu, NULL, NULL, 0, "Slot 6"};
const MenuItem slot7Menu = {&slotsMenu, &slot6Menu, &slot8Menu, NULL, NULL, 0, "Slot 7"};
const MenuItem slot8Menu = {&slotsMenu, &slot7Menu, &slot9Menu, NULL, NULL, 0, "Slot 8"};
const MenuItem slot9Menu = {&slotsMenu, &slot8Menu, &slot10Menu, NULL, NULL, 0, "Slot 9"};
const MenuItem slot10Menu = {&slotsMenu, &slot9Menu, NULL, NULL, NULL, 0, "Slot 10"};

const MenuItem *pCurrentMenu = &slotsMenu;

/**
 * @brief Navigate user through the menu according to the buttons actions
 * 
 * @param buttonId Button that triggered the action
 */
void MENU_navigate(ButtonId buttonId)
{
  const MenuItem *pNewMenu = pCurrentMenu;

  ButtonAction action = BTN_getAction(buttonId);
  if (action == BTN_ACTION_CLICK)
  {
    if (buttonId == BTN_ID_LEFT && pCurrentMenu->parent != NULL)
    {
      pNewMenu = pCurrentMenu->parent;
    }
    if (buttonId == BTN_ID_UP && pCurrentMenu->prev != NULL)
    {
      pNewMenu = pCurrentMenu->prev;
    }
    if (buttonId == BTN_ID_DOWN && pCurrentMenu->next != NULL)
    {
      pNewMenu = pCurrentMenu->next;
    }
    if (buttonId == BTN_ID_RIGHT && pCurrentMenu->child != NULL)
    {
      pNewMenu = pCurrentMenu->child;
    }
  }

  if (pCurrentMenu != pNewMenu)
  {
    pCurrentMenu = pNewMenu;
    MENU_draw();
  }
}

/**
 * @brief Draw menu
 */
void MENU_draw()
{
  // Show parent header text
  const char *headerText = pCurrentMenu->parent != NULL ? pCurrentMenu->parent->text : startString;
  DISP_printf(0, 0, STYLE_BOLD, TEXT_SIZE_16, headerText);

  size_t prevItemCount = 0;
  const MenuItem *pItem = pCurrentMenu->prev;
  while (pItem != NULL)
  {
    prevItemCount++;
    pItem = pItem->prev;
  }

  size_t nextItemCount = 0;
  pItem = pCurrentMenu->next;
  while (pItem != NULL)
  {
    nextItemCount++;
    pItem = pItem->next;
  }

  // Show navigation info
  size_t menuItemIndex = prevItemCount;
  size_t totalItemCount = prevItemCount + 1 + nextItemCount;
  DISP_printf(0, 56, STYLE_NORMAL, TEXT_SIZE_8, "%d/%d", menuItemIndex + 1, totalItemCount);

  // Find the first row item
  size_t rowOffset = menuItemIndex % MENU_ROW_COUNT;
  pItem = pCurrentMenu;
  while (rowOffset > 0)
  {
    pItem = pItem->prev;
    rowOffset--;
  }

  // Fill not empty rows
  while (rowOffset < MENU_ROW_COUNT && pItem != NULL)
  {
    uint8_t yPos = 16 + rowOffset * MENU_ROW_PIXELS;

    if (pItem == pCurrentMenu)
    {
      DISP_printf(0, yPos, STYLE_BOLD, TEXT_SIZE_8, ">%s", pItem->text);
    }
    else
    {
      DISP_printf(0, yPos, STYLE_NORMAL, TEXT_SIZE_8, " %s", pItem->text);
    }

    pItem = pItem->next;
    rowOffset++;
  }

  // Clear empty rows if any
  while (rowOffset < MENU_ROW_COUNT)
  {
    uint8_t yPos = 16 + rowOffset * MENU_ROW_PIXELS;

    DISP_printf(0, yPos, STYLE_NORMAL, TEXT_SIZE_8, "");

    rowOffset++;
  }
}
