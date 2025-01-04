#include <Arduino.h>
#include <RCSwitch.h>

#include "button.h"
#include "display.h"
#include "log.h"
#include "menu.h"
#include "slot.h"

// #define LOG_DEBUG // Uncomment to enable log printing

namespace
{
  namespace FwVersion
  {
    constexpr uint8_t major = 0;
    constexpr uint8_t minor = 3;
  } // namespace FwVersion

  namespace Battery
  {
    constexpr uint8_t inputPin = A0;

    constexpr uint16_t voltageMax = 5000;  // millivolts
    constexpr uint16_t rawValueMax = 1023; // bits

    /**
     * @brief Initialize battery voltage readings
     */
    void initialize()
    {
      pinMode(inputPin, INPUT);
    }

    /**
     * @brief Read battery voltage
     *
     * @return Current battery voltage
     */
    uint16_t readVoltage()
    {
      uint16_t rawValue = analogRead(inputPin);

      // Calculate voltage from raw reading
      uint16_t voltage = (uint32_t)rawValue * voltageMax / rawValueMax;

      return voltage;
    }
  } // namespace Battery

  namespace MainMenu
  {
    constexpr uint8_t headerOffsetXPix = 0;
    constexpr uint8_t linesOffsetXPix = 4;
    constexpr uint8_t navOffsetXPix = 1;

    constexpr unsigned long welcomeTimeMs = 3000;
    constexpr unsigned long systemInfoUpdatePeriodMs = 1000;

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
    const char rootHeaderString[] = "Pocket Key";
    const char authorString[] = "Inspired by mr drone";

    /**
     * @brief Show welcome screen
     */
    void showSystemInfo(const char *headerString)
    {
      Display::printf(headerOffsetXPix, Display::Line::Header, "%-16.16s", headerString);
      Display::printf(linesOffsetXPix, Display::Line::Line_1, authorString);

      uint16_t batteryVoltage = Battery::readVoltage();
      Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_3, "Battery: %4umV", batteryVoltage);
      Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_4, "Firmware: v%u.%u", FwVersion::major, FwVersion::minor);
    }
  } // namespace Menu

  namespace Radio
  {
    constexpr uint8_t rxPin = 2;
    constexpr uint8_t txPin = 10;
    // Receiver on pin #2 => that is interrupt 0
    constexpr uint8_t rxInterrupt = 0;

    RCSwitch rcSwitch = RCSwitch();

    /**
     * @brief Initialize radio
     */
    void initialize()
    {
      pinMode(rxPin, INPUT);
      pinMode(txPin, OUTPUT);

      // Setup transmitter on TX pin
      rcSwitch.enableTransmit(txPin);
    }

    /**
     * @brief Enable receiver interrupt handling
     */
    inline void enableReciever()
    {
      // Reset previous found signal if any
      rcSwitch.resetAvailable();

      // Enable receiver interrupt on RX pin
      rcSwitch.enableReceive(rxInterrupt);
    }

    /**
     * @brief Disable receiver interrupt handling
     */
    inline void disableReciever()
    {
      rcSwitch.disableReceive();
    }

    /**
     * @brief Read received signal
     *
     * @param signal Signal to read
     * @return true if signal was read, false if no signal received
     */
    bool readSignal(Slot::Signal &signal)
    {
      bool result = rcSwitch.available();
      if (result == true)
      {
        signal.protocol = rcSwitch.getReceivedProtocol();
        signal.value = rcSwitch.getReceivedValue();
        signal.bitLength = rcSwitch.getReceivedBitlength();
      }

      return result;
    }

    /**
     * @brief Send signal to transmit
     *
     * @param signal Signal to transmit
     */
    void sendSignal(const Slot::Signal &signal)
    {
      rcSwitch.setProtocol(signal.protocol);
      rcSwitch.send(signal.value, signal.bitLength);
    }
  } // namespace Radio

  // Menu item's functionality callback prototypes
  Menu::FunctionState slotItemCallback(Menu::Action action, int param);
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

    /**
     * @brief Setup slot menu items with slot data
     */
    void setupSlots()
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
  } // namespace MenuItem

  const Menu::Item *pCurrentMenu = &MenuItem::slotRoot;
  uint8_t selectedSlotIdx = Slot::invalidIdx;

  /**
   * @brief Return menu action according to the button events
   *
   * @param buttonId Button identifier caused the event
   * @return Current menu action
   */
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
      bool isRootMenu = (pDrawItem->parent == nullptr);

      // Show parent header text
      const char *headerText = isRootMenu ? MainMenu::rootHeaderString : pDrawItem->parent->text;
      Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", headerText);

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
      Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation,
                      isRootMenu ? "        %2u/%-2u  ENTER>" : "<BACK   %2u/%-2u  ENTER>",
                      itemIdx + 1, itemsCount);

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
          Display::setInverted(true);
          Display::printf(MainMenu::linesOffsetXPix, line, "%-20.20s", pItem->text);
        }
        else
        {
          Display::printf(MainMenu::linesOffsetXPix, line, "%-20.20s", pItem ? pItem->text : "");
        }

        pItem = pItem ? pItem->next : nullptr;
        itemOffset++;
      }
    }
  }

  /**
   * @brief Slot selection menu item's functionality callback
   *
   * @param action New menu action
   * @param param Menu item's parameter
   * @return Current menu item's function state
   */
  Menu::FunctionState slotItemCallback(Menu::Action action, int param)
  {
    Menu::FunctionState functionState = Menu::FunctionState::Inactive;

    if (action == Menu::Action::Enter)
    {
      selectedSlotIdx = param;
    }

    return functionState;
  }

  /**
   * @brief Slot emulation menu item's functionality callback
   *
   * @param action New menu action
   * @param param Menu item's parameter
   * @return Current menu item's function state
   */
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
          Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "No signal saved");
          Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_1, "Go to search menu");
          Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT");
          // Switch to no signal state
          state = State::NoSignal;
        }
        else
        {
          Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "Signal TX");
          Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_1, "Protocol: %02u", txSignal.protocol);
          Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_2, "Value: 0x%08X", txSignal.value);
          Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_3, "Bits: %2u", txSignal.bitLength);
          Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT         SEND>>");
          // Switch to signal opened state
          state = State::SignalOpened;
        }
      }
      break;

    case Menu::Action::Set:
      if (state == State::SignalOpened)
      {
        // Update display
        Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "Sending...");
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "%-21.21s", "");
        // Send signal to the radio
        Radio::sendSignal(txSignal);
        // Update display
        Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "Signal TX");
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT         SEND>>");
      }
      break;

    default:
      break;
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  /**
   * @brief Slot searching menu item's functionality callback
   *
   * @param action New menu action
   * @param param Menu item's parameter
   * @return Current menu item's function state
   */
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
        // Disable receiver
        Radio::disableReciever();
        // Switch to disabled state
        state = State::Disabled;
      }
      break;

    case Menu::Action::Enter:
      if (state == State::Disabled || state == State::Found || state == State::Saved)
      {
        // Update display
        Display::clear();
        Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "Searching...");
        Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_1, "Please wait");
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT");
        // Enable radio receiver
        Radio::enableReciever();
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
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT        REPEAT>");
        // Switch to saved state
        state = State::Saved;
      }
      break;

    default:
      break;
    }

    if (state == State::Searching)
    {
      bool isSignalRead = Radio::readSignal(rxSignal);
      if (isSignalRead == true)
      {
        Radio::disableReciever();

#ifdef LOG_DEBUG
        Log::printf("Rx %02u: %u/%u", rxSignal.protocol, rxSignal.value, rxSignal.bitLength);
#endif // LOG_DEBUG

        // Update display
        Display::clear();
        Display::printf(MainMenu::headerOffsetXPix, Display::Line::Header, "%-16.16s", "Signal RX");
        Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_1, "Protocol: %02u", rxSignal.protocol);
        Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_2, "Value: 0x%08X", rxSignal.value);
        Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_3, "Bits: %2u", rxSignal.bitLength);
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT REPEAT>/SAVE>>");

        // Switch to saved state
        state = State::Found;
      }
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  /**
   * @brief System information menu item's functionality callback
   *
   * @param action New menu action
   * @param param Menu item's parameter
   * @return Current menu item's function state
   */
  Menu::FunctionState systemCallback(Menu::Action action, int param)
  {
    enum class State
    {
      Disabled,
      ShowInfo,
    };

    static State state = State::Disabled;
    static unsigned long lastUpdateTimeMs = 0;

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
        MainMenu::showSystemInfo("System info");
        Display::printf(MainMenu::navOffsetXPix, Display::Line::Navigation, "<<EXIT");
        lastUpdateTimeMs = millis();
        // Switch to show info state
        state = State::ShowInfo;
      }
      break;

    default:
      break;
    }

    if (state == State::ShowInfo)
    {
      unsigned long currentTimeMs = millis();
      if (currentTimeMs > lastUpdateTimeMs + MainMenu::systemInfoUpdatePeriodMs)
      {
        uint16_t batteryVoltage = Battery::readVoltage();
        Display::printf(MainMenu::linesOffsetXPix, Display::Line::Line_3, "Battery: %4umV", batteryVoltage);
        lastUpdateTimeMs = currentTimeMs;
      }
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }
} // namespace

void setup()
{
  // Initialize serial port for logs
  Serial.begin(115200);

#ifdef LOG_DEBUG
  // Log FW version info
  Log::printf("Firmware: v%d.%d", FwVersion::major, FwVersion::minor);
#endif // LOG_DEBUG

  // Initialize battery voltage readings
  Battery::initialize();

#ifdef LOG_DEBUG
  // Log battery info
  uint16_t batteryVoltage = Battery::readVoltage();
  Log::printf("Battery: %4u", batteryVoltage);
#endif // LOG_DEBUG

  // Initialize display
  Display::initialize();

  // Show welcome screen
  MainMenu::showSystemInfo(MainMenu::rootHeaderString);
  unsigned long welcomeEndTimeMs = millis() + MainMenu::welcomeTimeMs;

  // Initialize buttons
  Button::initialize();

  // Initialize radio
  Radio::initialize();

  // Erase all slots on the EEPROM
  // Slot::eraseAll();

  // Load all slot data from the EEPROM
  Slot::loadAll();

  // Setup slot menu items with slot data
  MenuItem::setupSlots();

  // Wait until welcome screen time ends
  while (millis() < welcomeEndTimeMs)
  {
    delay(1);
  }

  // Draw current menu initially
  drawMenu(pCurrentMenu);
}

void loop()
{
  Button::Id buttonId = Button::process();
  if (buttonId != Button::Id::None)
  {
#ifdef LOG_DEBUG
    Log::printf("UP:%u DOWN:%u LEFT:%u RIGHT:%u",
                Button::getState(Button::Id::Up), Button::getState(Button::Id::Down),
                Button::getState(Button::Id::Left), Button::getState(Button::Id::Right));
#endif // LOG_DEBUG
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
