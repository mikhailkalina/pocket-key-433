#include <Arduino.h>
#include <RCSwitch.h>
#include <ssd1306.h>

#include "button.h"
#include "log.h"
#include "menu.h"
#include "slot.h"

#define RADIO_RX_PIN (2)
#define RADIO_TX_PIN (10)

#define BATTERY_PIN (A0)

// Receiver on pin #2 => that is interrupt 0
#define RADIO_RX_INTERRUPT (0)

#define MENU_ROW_COUNT (5)
#define MENU_ROW_PIXELS (8)

enum TextSize
{
  TEXT_8,
  TEXT_16,
};

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

// String for start display
const char startString[] = "Pocket Key 433";

size_t currentSlotIdx = 0;

RCSwitch rcSwitch = RCSwitch();

void displayText(uint8_t xpos, uint8_t y, EFontStyle style, TextSize size, const char *format, ...)
{
  static char buffer[21 + 1];
  va_list args;
  const size_t maxLength = size == TEXT_8 ? 21 : 16;

  va_start(args, format);
  vsnprintf(buffer, maxLength + 1, format, args);
  va_end(args);

  // Clear the rest of the buffer
  for (size_t idx = strlen(buffer); idx < maxLength; idx++)
  {
    buffer[idx] = ' ';
  }
  buffer[maxLength] = '\0'; // end of line

  ssd1306_printFixed(xpos, y, buffer, style);
}

void NavigateMenu(ButtonId actionBtnId)
{
  const MenuItem *pNewMenu = pCurrentMenu;

  ButtonAction action = BTN_getAction(actionBtnId);
  if (action == BTN_ACTION_CLICK)
  {
    if (actionBtnId == BTN_ID_LEFT && pCurrentMenu->parent != NULL)
    {
      pNewMenu = pCurrentMenu->parent;
    }
    if (actionBtnId == BTN_ID_UP && pCurrentMenu->prev != NULL)
    {
      pNewMenu = pCurrentMenu->prev;
    }
    if (actionBtnId == BTN_ID_DOWN && pCurrentMenu->next != NULL)
    {
      pNewMenu = pCurrentMenu->next;
    }
    if (actionBtnId == BTN_ID_RIGHT && pCurrentMenu->child != NULL)
    {
      pNewMenu = pCurrentMenu->child;
    }
  }

  if (pCurrentMenu != pNewMenu)
  {
    pCurrentMenu = pNewMenu;
    ShowMenu(pCurrentMenu);
  }
}

void ShowMenu(const MenuItem *pMenuItem)
{
  if (pMenuItem == NULL)
  {
    return;
  }

  // Show parent header text
  const char *headerText = pMenuItem->parent != NULL ? pMenuItem->parent->text : startString;
  ssd1306_setFixedFont(ssd1306xled_font8x16);
  displayText(0, 0, STYLE_BOLD, TEXT_16, headerText);

  size_t prevItemCount = 0;
  const MenuItem *pItem = pMenuItem->prev;
  while (pItem != NULL)
  {
    prevItemCount++;
    pItem = pItem->prev;
  }

  size_t nextItemCount = 0;
  pItem = pMenuItem->next;
  while (pItem != NULL)
  {
    nextItemCount++;
    pItem = pItem->next;
  }

  // Show navigation info
  size_t menuItemIndex = prevItemCount;
  size_t totalItemCount = prevItemCount + 1 + nextItemCount;
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  displayText(0, 56, STYLE_NORMAL, TEXT_8, "%d/%d", menuItemIndex + 1, totalItemCount);

  // Find the first row item
  size_t rowOffset = menuItemIndex % MENU_ROW_COUNT;
  pItem = pMenuItem;
  while (rowOffset > 0)
  {
    pItem = pItem->prev;
    rowOffset--;
  }

  // Fill not empty rows
  while (rowOffset < MENU_ROW_COUNT && pItem != NULL)
  {
    uint8_t yPos = 16 + rowOffset * MENU_ROW_PIXELS;

    if (pItem == pMenuItem)
    {
      displayText(0, yPos, STYLE_BOLD, TEXT_8, ">%s", pItem->text);
    }
    else
    {
      displayText(0, yPos, STYLE_NORMAL, TEXT_8, " %s", pItem->text);
    }

    pItem = pItem->next;
    rowOffset++;
  }

  // Clear empty rows if any
  while (rowOffset < MENU_ROW_COUNT)
  {
    uint8_t yPos = 16 + rowOffset * MENU_ROW_PIXELS;

    displayText(0, yPos, STYLE_NORMAL, TEXT_8, "");

    rowOffset++;
  }
}

void initializePins()
{
  pinMode(RADIO_RX_PIN, INPUT);
  pinMode(RADIO_TX_PIN, OUTPUT);

  pinMode(BATTERY_PIN, INPUT);
}

void setup()
{
  Serial.begin(115200);

  initializePins();
  BTN_initialize();

  // Enable interrupt on RX pin
  rcSwitch.enableReceive(RADIO_RX_INTERRUPT);

  // Initialize display
  ssd1306_128x64_i2c_init();
  ssd1306_clearScreen();

  // SLOT_eraseAll(); // uncomment to erase all slots on the storage
  size_t usedSlotCount = SLOT_loadAll();

  ShowMenu(pCurrentMenu);
}

void loop()
{
  ButtonId actionBtnId = BTN_handle();
  if (actionBtnId != BTN_ID_NONE)
  {
    LOG_printf("UP:%d DOWN:%d LEFT:%d RIGHT:%d",
               BTN_getAction(BTN_ID_UP), BTN_getAction(BTN_ID_DOWN),
               BTN_getAction(BTN_ID_LEFT), BTN_getAction(BTN_ID_RIGHT));

    NavigateMenu(actionBtnId);
  }

  if (rcSwitch.available())
  {
    unsigned int rxProto = rcSwitch.getReceivedProtocol();
    unsigned long rxValue = rcSwitch.getReceivedValue();
    unsigned int rxBits = rcSwitch.getReceivedBitlength();

    LOG_printf("Rx %02u: %lu/%u", rxProto, rxValue, rxBits);

    rcSwitch.resetAvailable();
  }
}
