#include <Arduino.h>
#include <RCSwitch.h>

#include "button.h"
#include "display.h"
#include "log.h"
#include "menu.h"
#include "slot.h"

#define RADIO_RX_PIN (2)
#define RADIO_TX_PIN (10)

#define BATTERY_PIN (A0)

// Receiver on pin #2 => that is interrupt 0
#define RADIO_RX_INTERRUPT (0)

extern const Menu::Item slotRootMenu;
extern const Menu::Item settingsMenu;
extern const Menu::Item displayMenu;
extern const Menu::Item radioMenu;
extern const Menu::Item systemMenu;
extern const Menu::Item brightnessMenu;
extern const Menu::Item contrastMenu;
extern const Menu::Item timeoutMenu;
extern const Menu::Item timeoutMenu;
extern Menu::Item slotItemsMenu[];

// Root menu
const Menu::Item slotRootMenu = {nullptr, nullptr, &settingsMenu, (const Menu::Item *)slotItemsMenu, nullptr, 0, "Slots"};
const Menu::Item settingsMenu = {nullptr, &slotRootMenu, nullptr, &displayMenu, nullptr, 0, "Settings"};

// Slots menu
Menu::Item slotItemsMenu[Slot::itemsCount] = {0};

// Settings menu
const Menu::Item displayMenu = {&settingsMenu, nullptr, &radioMenu, &brightnessMenu, nullptr, 0, "Display"};
const Menu::Item radioMenu = {&settingsMenu, &displayMenu, &systemMenu, nullptr, nullptr, 0, "Radio"};
const Menu::Item systemMenu = {&settingsMenu, &radioMenu, nullptr, nullptr, nullptr, 0, "System"};

// Display menu
const Menu::Item brightnessMenu = {&displayMenu, nullptr, &contrastMenu, nullptr, nullptr, 0, "Brightness"};
const Menu::Item contrastMenu = {&displayMenu, &brightnessMenu, &timeoutMenu, nullptr, nullptr, 0, "Contrast"};
const Menu::Item timeoutMenu = {&displayMenu, &contrastMenu, nullptr, nullptr, nullptr, 0, "Timeout"};

unsigned long rxTimeMs = 0;

RCSwitch rcSwitch = RCSwitch();

void initializePins()
{
  pinMode(RADIO_RX_PIN, INPUT);
  pinMode(RADIO_TX_PIN, OUTPUT);

  pinMode(BATTERY_PIN, INPUT);
}

void setupSlotItemsMenu()
{
  for (uint8_t idx = 0; idx < Slot::itemsCount; idx++)
  {
    Menu::Item &itemMenu = slotItemsMenu[idx];
    itemMenu.parent = &slotRootMenu;
    itemMenu.prev = idx > 0 ? &slotItemsMenu[idx - 1] : nullptr;
    itemMenu.next = idx < Slot::itemsCount - 1 ? &slotItemsMenu[idx + 1] : nullptr;
    itemMenu.child = nullptr;
    itemMenu.text = Slot::getName(idx);
  }
}

void setup()
{
  Serial.begin(115200);

  initializePins();

  // Initialize buttons
  Button::initialize();

  // Enable receiver interrupt on RX pin
  rcSwitch.enableReceive(RADIO_RX_INTERRUPT);
  // Setup transmitter on TX pin
  rcSwitch.enableTransmit(RADIO_TX_PIN);

  // Slot::eraseAll(); // uncomment to erase all slots on the storage
  Slot::loadAll();
  setupSlotItemsMenu();

  // Initialize display
  Display::initialize();

  // Draw the slot menu initially
  Menu::draw(&slotRootMenu);
}

void loop()
{
  Button::Id buttonId = Button::process();
  if (buttonId != Button::Id::None)
  {
    Log::printf("UP:%d DOWN:%d LEFT:%d RIGHT:%d",
                (int)Button::getState(Button::Id::Up), (int)Button::getState(Button::Id::Down),
                (int)Button::getState(Button::Id::Left), (int)Button::getState(Button::Id::Right));

    Menu::Action menuAction = Menu::Action::None;

    Button::Action buttonAction = Button::getAction(buttonId);
    if (buttonAction == Button::Action::PressEnd)
    {
      switch (buttonId)
      {
      case Button::Id::Left:
        menuAction = Menu::Action::Exit;
        break;

      case Button::Id::Up:
        menuAction = Menu::Action::Prev;
        break;

      case Button::Id::Down:
        menuAction = Menu::Action::Next;
        break;

      case Button::Id::Right:
        menuAction = Menu::Action::Enter;
        break;

      default:
        break;
      }
    }

    if (menuAction != Menu::Action::None)
    {
      Menu::navigate(menuAction);
    }
  }

  if (rcSwitch.available())
  {
    unsigned int rxProto = rcSwitch.getReceivedProtocol();
    unsigned long rxValue = rcSwitch.getReceivedValue();
    unsigned int rxBits = rcSwitch.getReceivedBitlength();

    Log::printf("Rx %02u: %lu/%u", rxProto, rxValue, rxBits);

    Display::printf(96, 56, "RX<<");
    rxTimeMs = millis();

    rcSwitch.resetAvailable();
  }
  else if (rxTimeMs > 0 && rxTimeMs + 100 < millis())
  {
    Display::printf(96, 56, "    "); // clear rx sign
    rxTimeMs = 0;
  }
}
