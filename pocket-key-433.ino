#include <Arduino.h>
#include <RCSwitch.h>

#include "button.h"
#include "display.h"
#include "log.h"
#include "menu.h"
#include "slot.h"

namespace
{
  constexpr uint8_t radioRxPin = 2;
  constexpr uint8_t radioTxPin = 10;
  // Receiver on pin #2 => that is interrupt 0
  constexpr uint8_t radioRxInterrupt = 0;

  extern Menu::Item slotRootMenu;
  extern Menu::Item settingsMenu;
  extern Menu::Item slotListMenu[];
  extern Menu::Item slotNameMenu;
  extern Menu::Item slotSearchMenu;
  extern Menu::Item slotSaveMenu;
  extern Menu::Item slotEmulateMenu;
  extern Menu::Item slotDeleteMenu;
  extern Menu::Item displayMenu;
  extern Menu::Item systemMenu;
  extern Menu::Item brightnessMenu;
  extern Menu::Item timeoutMenu;
  extern Menu::Item fwVersionMenu;

  // Root menu
  Menu::Item slotRootMenu = {"Slots", nullptr, &settingsMenu, slotListMenu, nullptr, 0};
  Menu::Item settingsMenu = {"Settings", &slotRootMenu, nullptr, &displayMenu, nullptr, 0};

  // Slots list menu
  Menu::Item slotListMenu[Slot::itemsCount] = {0};

  // Slot item menu
  Menu::Item slotNameMenu = {"Name", nullptr, &slotSearchMenu, nullptr, nullptr, 0};
  Menu::Item slotSearchMenu = {"Search", &slotNameMenu, &slotSaveMenu, nullptr, nullptr, 0};
  Menu::Item slotSaveMenu = {"Save", &slotSearchMenu, nullptr, nullptr, nullptr, 0};
  Menu::Item slotEmulateMenu = {"Emulate", &slotNameMenu, &slotDeleteMenu, nullptr, nullptr, 0};
  Menu::Item slotDeleteMenu = {"Delete", &slotEmulateMenu, nullptr, nullptr, nullptr, 0};

  // Settings menu
  Menu::Item displayMenu = {"Display", nullptr, &systemMenu, &brightnessMenu, 0};
  Menu::Item systemMenu = {"System", &displayMenu, nullptr, &fwVersionMenu, 0};

  // Display menu
  Menu::Item brightnessMenu = {"Brightness", nullptr, &timeoutMenu, nullptr, nullptr, 0};
  Menu::Item timeoutMenu = {"Timeout", &brightnessMenu, nullptr, nullptr, nullptr, 0};

  // System menu
  Menu::Item fwVersionMenu = {"FW version", nullptr, nullptr, nullptr, nullptr, 0};

  unsigned long rxTimeMs = 0;
  uint8_t selectedSlotIdx = Slot::itemsCount;

  RCSwitch rcSwitch = RCSwitch();
} // namespace

void initializeRadio()
{
  pinMode(radioRxPin, INPUT);
  pinMode(radioTxPin, OUTPUT);

  // Enable receiver interrupt on RX pin
  rcSwitch.enableReceive(radioRxInterrupt);
  // Setup transmitter on TX pin
  rcSwitch.enableTransmit(radioTxPin);
}

void slotEnterCallback(uint8_t slotIdx)
{
  const Slot::Signal &slotSignal = Slot::getSignal(slotIdx);
  if (slotSignal == Slot::signalInvalid)
  {
    // Signal is invalid - activate search menu
    slotNameMenu.next = &slotSearchMenu;
  }
  else
  {
    // Signal is valid - activate emulate menu
    slotNameMenu.next = &slotEmulateMenu;
  }

  selectedSlotIdx = slotIdx;
}

void setupSlotItemsMenu()
{
  for (uint8_t slotIdx = 0; slotIdx < Slot::itemsCount; slotIdx++)
  {
    Menu::Item &itemMenu = slotListMenu[slotIdx];
    itemMenu.text = Slot::getName(slotIdx);
    itemMenu.prev = slotIdx > 0 ? &slotListMenu[slotIdx - 1] : nullptr;
    itemMenu.next = slotIdx < Slot::itemsCount - 1 ? &slotListMenu[slotIdx + 1] : nullptr;
    itemMenu.child = &slotNameMenu;
    itemMenu.enterHandler.callback = slotEnterCallback;
    itemMenu.enterHandler.param = slotIdx;
  }
}

void setup()
{
  Serial.begin(115200);

  initializeRadio();

  // Initialize buttons
  Button::initialize();

  // Initialize display
  Display::initialize();

  // Slot::eraseAll(); // uncomment to erase all slots on the storage
  Slot::loadAll();
  setupSlotItemsMenu();

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
