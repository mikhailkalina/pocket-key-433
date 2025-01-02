#include <Arduino.h>
#include <RCSwitch.h>

#include "button.h"
#include "display.h"
#include "log.h"
#include "menu.h"
#include "slot.h"

namespace
{
  namespace FwVersion
  {
    constexpr uint8_t major = 0;
    constexpr uint8_t minor = 1;
  } // namespace FwVersion

  namespace MainRadio
  {
    constexpr uint8_t rxPin = 2;
    constexpr uint8_t txPin = 10;
    // Receiver on pin #2 => that is interrupt 0
    constexpr uint8_t rxInterrupt = 0;
  } // namespace MainRadio

  namespace MainMenu
  {
    constexpr uint8_t pageItemCount = 5;
    constexpr Display::Line displayLines[] = {
        Display::Line::Line_1,
        Display::Line::Line_2,
        Display::Line::Line_3,
        Display::Line::Line_4,
        Display::Line::Line_5,
    };
    static_assert(sizeof(displayLines) / sizeof(*displayLines) == pageItemCount);

    // String for root menu header
    const char rootHeader[] = "Pocket Key";
  } // namespace Menu

  // Menu item's functionality callback prototypes
  Menu::FunctionState slotEmulateCallback(Menu::Action action, int param);
  Menu::FunctionState slotSearchCallback(Menu::Action action, int param);
  Menu::FunctionState systemCallback(Menu::Action action, int param);

  namespace MenuItem
  {
    // Menu item definitions
    extern Menu::Item slotRoot;
    extern Menu::Item slotList[];
    extern Menu::Item slotEmulate;
    extern Menu::Item slotSearch;
    extern Menu::Item slotName;
    extern Menu::Item settings;
    extern Menu::Item system;

    // Root menu
    Menu::Item slotRoot = {"Slots", nullptr, &settings, slotList};
    Menu::Item settings = {"Settings", &slotRoot, nullptr, &system};

    // Slots list menu
    Menu::Item slotList[Slot::slotsCount] = {0};

    // Slot item menu
    Menu::Item slotEmulate = {"Emulate", nullptr, &slotSearch, nullptr, slotEmulateCallback};
    Menu::Item slotSearch = {"Search", &slotEmulate, &slotName, nullptr, slotSearchCallback};
    Menu::Item slotName = {"Name", &slotSearch, nullptr, nullptr};

    // Settings menu
    Menu::Item system = {"System", nullptr, nullptr, nullptr, systemCallback};
  } // namespace MenuItem

  const Menu::Item *pCurrentMenu = &MenuItem::slotRoot;
  uint8_t selectedSlotIdx = Slot::invalidIdx;

  RCSwitch rcSwitch = RCSwitch();

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
      else if (buttonEvent == Button::Event::HoldStart)
      {
        menuAction = Menu::Action::Set;
      }
      break;

    default:
      break;
    }

    return menuAction;
  }

  /**
   * @brief Draw specified menu item
   * Set menu item as current
   *
   * @param pDrawItem Menu item to draw
   */
  void drawMenu(const Menu::Item *pDrawItem)
  {
    if (pDrawItem != nullptr)
    {
      // Show parent header text
      const char *headerText = pDrawItem->parent != nullptr ? pDrawItem->parent->text : MainMenu::rootHeader;
      Display::printf(0, Display::Line::Header, "%-16.16s", headerText);

      // Count previous items
      uint8_t prevItemCount = 0;
      const Menu::Item *pItem = pDrawItem->prev;
      while (pItem != nullptr)
      {
        prevItemCount++;
        pItem = pItem->prev;
      }

      // Count next items
      uint8_t nextItemCount = 0;
      pItem = pDrawItem->next;
      while (pItem != nullptr)
      {
        nextItemCount++;
        pItem = pItem->next;
      }

      // Show navigation info
      uint8_t itemIdx = prevItemCount;
      uint8_t itemsCount = prevItemCount + 1 + nextItemCount;
      Display::printf(0, Display::Line::Navigation, "<BACK   %2u/%-2u  ENTER>", itemIdx + 1, itemsCount);

      // Find the first item on current page
      uint8_t itemOffset = itemIdx % MainMenu::pageItemCount;
      pItem = pDrawItem;
      while (itemOffset > 0)
      {
        pItem = pItem->prev;
        itemOffset--;
      }

      // Fill menu items on current page
      while (itemOffset < MainMenu::pageItemCount)
      {
        Display::Line line = MainMenu::displayLines[itemOffset];

        if (pItem == pDrawItem)
        {
          Display::setStyle(Display::Style::Bold);
          Display::printf(0, line, ">%-20.20s", pItem->text);
        }
        else
        {
          Display::printf(0, line, " %-20.20s", pItem ? pItem->text : "");
        }

        pItem = pItem ? pItem->next : nullptr;
        itemOffset++;
      }
    }
  }

  Menu::FunctionState slotItemCallback(Menu::Action action, int param)
  {
    Menu::FunctionState functionState = Menu::FunctionState::Inactive;

    if (action == Menu::Action::Enter)
    {
      selectedSlotIdx = param;
    }

    return functionState;
  }

  Menu::FunctionState slotEmulateCallback(Menu::Action action, int param)
  {
    enum class State
    {
      Disabled,
      NoSignal,
      SignalOpened,
    };

    static State state = State::Disabled;
    static Slot::Signal txSignal = Slot::signalInvalid;

    // Handle new action
    switch (action)
    {
    case Menu::Action::Exit:
      if (state != State::Disabled)
      {
        Display::clear();
        txSignal = Slot::signalInvalid;
        // Switch to disabled state
        state = State::Disabled;
      }
      break;

    case Menu::Action::Enter:
      if (state == State::Disabled)
      {
        // Update display
        Display::clear();
        txSignal = Slot::getSignal(selectedSlotIdx);
        if (txSignal == Slot::signalInvalid)
        {
          Display::printf(0, Display::Line::Header, "No signal");
          Display::printf(0, Display::Line::Line_1, "Go to search menu");
          Display::printf(0, Display::Line::Navigation, "<<EXIT               ");
          // Switch to no signal state
          state = State::NoSignal;
        }
        else
        {
          Display::printf(0, Display::Line::Header, "Signal");
          Display::printf(0, Display::Line::Line_1, " Protocol: %02u", txSignal.protocol);
          Display::printf(0, Display::Line::Line_2, " Value: 0x%08X", txSignal.value);
          Display::printf(0, Display::Line::Line_3, " Bits: %2u", txSignal.bitLength);
          Display::printf(0, Display::Line::Navigation, "<<EXIT         SEND>>");
          // Switch to signal opened state
          state = State::SignalOpened;
        }
      }
      break;

    case Menu::Action::Set:
      if (state == State::SignalOpened)
      {
        // Update display
        Display::printf(0, Display::Line::Header, "Sending...");
        Display::printf(0, Display::Line::Navigation, "<<EXIT               ");
        // Send signal
        rcSwitch.setProtocol(txSignal.protocol);
        rcSwitch.send(txSignal.value, txSignal.bitLength);
        // Update display
        Display::printf(0, Display::Line::Header, "Sending OK");
        Display::printf(0, Display::Line::Navigation, "<<EXIT         SEND>>");
      }
      break;

    default:
      break;
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  Menu::FunctionState slotSearchCallback(Menu::Action action, int param)
  {
    enum class State
    {
      Disabled,
      Searching,
      Found,
      Saved,
    };

    static State state = State::Disabled;
    static Slot::Signal rxSignal = Slot::signalInvalid;

    // Handle new action
    switch (action)
    {
    case Menu::Action::Exit:
      if (state != State::Disabled)
      {
        Display::clear();
        // Enable receiver interrupt
        rcSwitch.disableReceive();
        // Switch to disabled state
        state = State::Disabled;
      }
      break;

    case Menu::Action::Enter:
      if (state == State::Disabled || state == State::Found || state == State::Saved)
      {
        // Update display
        Display::clear();
        Display::printf(0, Display::Line::Header, "Searching...");
        Display::printf(0, Display::Line::Line_1, "Please wait");
        Display::printf(0, Display::Line::Navigation, "<<EXIT");
        // Reset previous found signal if any
        rcSwitch.resetAvailable();
        // Enable receiver interrupt on RX pin
        rcSwitch.enableReceive(MainRadio::rxInterrupt);
        // Switch to searching state
        state = State::Searching;
      }
      break;

    case Menu::Action::Set:
      if (state == State::Found)
      {
        // Set signal to current selected slot
        Slot::setSignal(selectedSlotIdx, rxSignal);
        // New signal was found - save to EEPROM
        Slot::save(selectedSlotIdx);
        // Update display
        Display::printf(0, Display::Line::Navigation, "<<EXIT        REPEAT>");
        // Switch to saved state
        state = State::Saved;
      }
      break;

    default:
      break;
    }

    if (state == State::Searching)
    {
      bool isSignalFound = rcSwitch.available();
      if (isSignalFound == true)
      {
        // Disable receiver interrupt
        rcSwitch.disableReceive();

        // Get signal parameters
        rxSignal.protocol = rcSwitch.getReceivedProtocol();
        rxSignal.value = rcSwitch.getReceivedValue();
        rxSignal.bitLength = rcSwitch.getReceivedBitlength();

        Log::printf("Rx %02u: %u/%u", rxSignal.protocol, rxSignal.value, rxSignal.bitLength);

        // Update display
        Display::clear();
        Display::printf(0, Display::Line::Header, "Found:");
        Display::printf(0, Display::Line::Line_1, " Protocol: %02u", rxSignal.protocol);
        Display::printf(0, Display::Line::Line_2, " Value: 0x%08X", rxSignal.value);
        Display::printf(0, Display::Line::Line_3, " Bits: %2u", rxSignal.bitLength);
        Display::printf(0, Display::Line::Navigation, "<<EXIT REPEAT>/SAVE>>");

        // Switch to saved state
        state = State::Found;
      }
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  Menu::FunctionState systemCallback(Menu::Action action, int param)
  {
    enum class State
    {
      Disabled,
      Info,
    };

    static State state = State::Disabled;

    // Handle new action
    switch (action)
    {
    case Menu::Action::Exit:
      if (state != State::Disabled)
      {
        Display::clear();
        // Switch to disabled state
        state = State::Disabled;
      }
      break;

    case Menu::Action::Enter:
      if (state == State::Disabled)
      {
        // Update display
        Display::clear();
        Display::printf(0, Display::Line::Header, "System info");
        Display::printf(0, Display::Line::Line_1, "FW version: %d.%d", FwVersion::major, FwVersion::minor);
        Display::printf(0, Display::Line::Navigation, "<<EXIT");
        // Switch to info state
        state = State::Info;
      }
      break;

    default:
      break;
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  void setupSlotItemMenu()
  {
    for (uint8_t slotIdx = 0; slotIdx < Slot::slotsCount; slotIdx++)
    {
      Menu::Item &itemMenu = MenuItem::slotList[slotIdx];
      itemMenu.text = Slot::getName(slotIdx);
      itemMenu.prev = (slotIdx == 0) ? nullptr : &MenuItem::slotList[slotIdx - 1];
      itemMenu.next = (slotIdx == Slot::slotsCount - 1) ? nullptr : &MenuItem::slotList[slotIdx + 1];
      itemMenu.child = &MenuItem::slotEmulate;
      itemMenu.callback = slotItemCallback;
      itemMenu.param = slotIdx;
    }
  }

  void initializeRadio()
  {
    pinMode(MainRadio::rxPin, INPUT);
    pinMode(MainRadio::txPin, OUTPUT);

    // Setup transmitter on TX pin
    rcSwitch.enableTransmit(MainRadio::txPin);
  }
} // namespace

void setup()
{
  Serial.begin(115200);

  Log::printf("FW version: %d.%d", FwVersion::major, FwVersion::minor);

  initializeRadio();

  // Initialize buttons
  Button::initialize();

  // Initialize display
  Display::initialize();

  // Slot::eraseAll(); // uncomment to erase all slots on the storage
  Slot::loadAll();
  setupSlotItemMenu();

  // Draw current menu initially
  drawMenu(pCurrentMenu);
}

void loop()
{
  Button::Id buttonId = Button::process();
  if (buttonId != Button::Id::None)
  {
    Log::printf("UP:%u DOWN:%u LEFT:%u RIGHT:%u",
                Button::getState(Button::Id::Up), Button::getState(Button::Id::Down),
                Button::getState(Button::Id::Left), Button::getState(Button::Id::Right));
  }

  // Get menu action according to the button event
  Menu::Action menuAction = getMenuAction(buttonId);

  const Menu::Item *pNewMenu = Menu::process(pCurrentMenu, menuAction);
  if (pNewMenu != nullptr)
  {
    if (pNewMenu != pCurrentMenu)
    {
      pCurrentMenu = pNewMenu;
    }

    // Draw new menu
    drawMenu(pCurrentMenu);
  }
}
