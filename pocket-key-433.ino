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
  extern Menu::Item slotEmulateMenu;
  extern Menu::Item slotDeleteMenu;
  extern Menu::Item displayMenu;
  extern Menu::Item systemMenu;
  extern Menu::Item brightnessMenu;
  extern Menu::Item timeoutMenu;
  extern Menu::Item fwVersionMenu;

  // Root menu
  Menu::Item slotRootMenu = {"Slots", nullptr, &settingsMenu, slotListMenu};
  Menu::Item settingsMenu = {"Settings", &slotRootMenu, nullptr, &displayMenu};

  // Slots list menu
  Menu::Item slotListMenu[Slot::slotsCount] = {0};

  // Slot item menu
  Menu::Item slotNameMenu = {"Name", nullptr, &slotSearchMenu, nullptr};
  Menu::Item slotSearchMenu = {"Search", &slotNameMenu, &slotEmulateMenu, nullptr};
  Menu::Item slotEmulateMenu = {"Emulate", &slotSearchMenu, nullptr, nullptr};

  // Settings menu
  Menu::Item displayMenu = {"Display", nullptr, &systemMenu, &brightnessMenu};
  Menu::Item systemMenu = {"System", &displayMenu, nullptr, &fwVersionMenu};

  // Display menu
  Menu::Item brightnessMenu = {"Brightness", nullptr, &timeoutMenu, nullptr};
  Menu::Item timeoutMenu = {"Timeout", &brightnessMenu, nullptr, nullptr};

  // System menu
  Menu::Item fwVersionMenu = {"FW version", nullptr, nullptr, nullptr};

  uint8_t selectedSlotIdx = Slot::invalidIdx;
  bool isSearchActive = false;

  RCSwitch rcSwitch = RCSwitch();
} // namespace

Menu::Action getMenuAction(Button::Id buttonId)
{
  Menu::Action menuAction = Menu::Action::None;
  Button::Event buttonEvent = Button::getEvent(buttonId);

  switch (buttonId)
  {
  case Button::Id::Up:
    if (buttonEvent == Button::Event::PressStart ||
        buttonEvent == Button::Event::HoldStart ||
        buttonEvent == Button::Event::HoldContinue)
    {
      menuAction = Menu::Action::Prev;
    }
    break;

  case Button::Id::Down:
    if (buttonEvent == Button::Event::PressStart ||
        buttonEvent == Button::Event::HoldStart ||
        buttonEvent == Button::Event::HoldContinue)
    {
      menuAction = Menu::Action::Next;
    }
    break;

  case Button::Id::Left:
    if (buttonEvent == Button::Event::PressEnd)
    {
      menuAction = Menu::Action::Back;
    }
    else if (buttonEvent == Button::Event::HoldStart)
    {
      menuAction = Menu::Action::Exit;
    }
    break;

  case Button::Id::Right:
    if (buttonEvent == Button::Event::PressEnd)
    {
      menuAction = Menu::Action::Enter;
    }
    break;

  default:
    break;
  }

  return menuAction;
}

void initializeRadio()
{
  pinMode(radioRxPin, INPUT);
  pinMode(radioTxPin, OUTPUT);

  // Setup transmitter on TX pin
  rcSwitch.enableTransmit(radioTxPin);
}

void slotItemCallback(Menu::Action action, int param)
{
  if (action == Menu::Action::Enter)
  {
    selectedSlotIdx = param;
  }
}

void slotSearchCallback(Menu::Action action, int param)
{
  if (action == Menu::Action::Enter)
  {
    Display::clear();
    Display::setStyle(Display::Style::Bold);
    Display::setSize(Display::Size::Bits_16);
    Display::printf(0, 0, "Searching...    ");
    Display::printf(0, 16, "Please wait");

    isSearchActive = true;
    // Enable receiver interrupt on RX pin
    rcSwitch.enableReceive(radioRxInterrupt);
  }
  else if (action == Menu::Action::Exit)
  {
    if (isSearchActive == true)
    {
      isSearchActive = false;
      // Enable receiver interrupt
      rcSwitch.disableReceive();
    }
    else
    {
      // New signal was found and set - save
      Slot::save(selectedSlotIdx);
    }

    Display::clear();
  }
}

void slotEmulateCallback(Menu::Action action, int param)
{
  if (action == Menu::Action::Enter)
  {
    const Slot::Signal &txSignal = Slot::getSignal(selectedSlotIdx);
    if (txSignal == Slot::signalInvalid)
    {
      Display::setStyle(Display::Style::Bold);
      Display::setSize(Display::Size::Bits_16);
      Display::printf(0, 0, "No signal saved!");
      Display::printf(0, 16, "Go to search menu");
      Display::printf(0, 56, "<EXIT               >");
    }
    else
    {
      Display::setStyle(Display::Style::Bold);
      Display::setSize(Display::Size::Bits_16);
      Display::printf(0, 0, "Sending...");
      Display::printf(0, 16, " Protocol: %02u", txSignal.protocol);
      Display::printf(0, 24, " Value: 0x%08X", txSignal.value);
      Display::printf(0, 32, " Bits: %2u", txSignal.bitLength);

      rcSwitch.setProtocol(txSignal.protocol);
      rcSwitch.send(txSignal.value, txSignal.bitLength);

      Display::setStyle(Display::Style::Bold);
      Display::setSize(Display::Size::Bits_16);
      Display::printf(0, 0, "Sending OK");
      Display::printf(0, 56, "<EXIT         REPEAT>");
    }
  }
  else if (action == Menu::Action::Exit)
  {
    Display::clear();
  }
}

void setupSlotItemsMenu()
{
  for (uint8_t slotIdx = 0; slotIdx < Slot::slotsCount; slotIdx++)
  {
    Menu::Item &itemMenu = slotListMenu[slotIdx];
    itemMenu.text = Slot::getName(slotIdx);
    itemMenu.prev = slotIdx > 0 ? &slotListMenu[slotIdx - 1] : nullptr;
    itemMenu.next = slotIdx < Slot::slotsCount - 1 ? &slotListMenu[slotIdx + 1] : nullptr;
    itemMenu.child = &slotNameMenu;
    itemMenu.callback = slotItemCallback;
    itemMenu.param = slotIdx;
  }

  slotSearchMenu.callback = slotSearchCallback;
  slotEmulateMenu.callback = slotEmulateCallback;
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
    Log::printf("UP:%u DOWN:%u LEFT:%u RIGHT:%u",
                Button::getState(Button::Id::Up), Button::getState(Button::Id::Down),
                Button::getState(Button::Id::Left), Button::getState(Button::Id::Right));

    Menu::Action menuAction = getMenuAction(buttonId);
    if (menuAction != Menu::Action::None)
    {
      Menu::navigate(menuAction);
    }
  }

  if (rcSwitch.available())
  {
    Slot::Signal rxSignal;
    rxSignal.protocol = rcSwitch.getReceivedProtocol();
    rxSignal.value = rcSwitch.getReceivedValue();
    rxSignal.bitLength = rcSwitch.getReceivedBitlength();

    Log::printf("Rx %02u: %u/%u", rxSignal.protocol, rxSignal.value, rxSignal.bitLength);

    if (isSearchActive == true)
    {
      isSearchActive = false;
      // Enable receiver interrupt
      rcSwitch.disableReceive();

      Slot::setSignal(selectedSlotIdx, rxSignal);

      Display::clear();
      Display::setStyle(Display::Style::Bold);
      Display::setSize(Display::Size::Bits_16);
      Display::printf(0, 0, "Found:");
      Display::printf(0, 16, " Protocol: %02u", rxSignal.protocol);
      Display::printf(0, 24, " Value: 0x%08X", rxSignal.value);
      Display::printf(0, 32, " Bits: %2u", rxSignal.bitLength);
      Display::printf(0, 56, "<EXIT&SAVE    REPEAT>");
    }

    rcSwitch.resetAvailable();
  }
}
