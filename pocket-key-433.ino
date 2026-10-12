#include <stdio.h>

#include <Arduino.h>
#include <RCSwitch.h>

#include "button.h"
#include "display.h"
#include "log.h"
#include "menu.h"
#include "rc_custom.h"
#include "slot.h"

// #define LOG_DEBUG // Uncomment to enable log printing

namespace
{
  namespace FwVersion
  {
    constexpr uint8_t major = 0;
    constexpr uint8_t minor = 6;
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
    constexpr unsigned long welcomeTimeMs = 1000;
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

    // Maximum text lengths
    constexpr uint8_t headerLengthMax = 16;
    constexpr uint8_t itemTextLengthMax = 20;

    // String for root menu header
    const char rootHeaderString[] PROGMEM = "Pocket Key";
    const char authorString[] PROGMEM = "inspired by mr drone";

    /**
     * @brief Show header text
     *
     * @param headerString Header text located in flash memory
     */
    void showHeader(const char *headerString)
    {
      Display::printString_P(0, Display::Line::Header, headerLengthMax, headerString);
    }

    /**
     * @brief Show header text centered horizontally
     *
     * @param headerString Header text located in flash memory
     */
    void showHeaderCentered(const char *headerString)
    {
      uint8_t length = strnlen_P(headerString, headerLengthMax);
      uint8_t offset = (headerLengthMax - length) / 2;
      Display::printString_P(offset, Display::Line::Header, headerLengthMax - offset, headerString);
    }

    /**
     * @brief Show system information below the header
     */
    void showSystemInfo()
    {
      Display::setStyle(Display::Style::Italic);
      Display::printString_P(0, Display::Line::Line_1, itemTextLengthMax, authorString);

      uint16_t batteryVoltage = Battery::readVoltage();
      DISPLAY_PRINTF(0, Display::Line::Line_3, "Battery: %4umV", batteryVoltage);
      DISPLAY_PRINTF(0, Display::Line::Line_4, "Firmware: v%u.%u", FwVersion::major, FwVersion::minor);
    }
  } // namespace Menu

  namespace Radio
  {
    constexpr uint8_t rxPin = 2;
    constexpr uint8_t txPin = 10;
    // Receiver on pin #2 => that is interrupt 0
    constexpr uint8_t rxInterrupt = 0;

    RCSwitch rcSwitch = RCSwitch();

    // Receiver backends. Both use the same RX interrupt, so only one is active
    // at a time; the search alternates between them to find any protocol.
    enum class RxBackend : uint8_t
    {
      RcSwitch, // rc-switch protocols 1..12
      Custom,   // CAME, NICE, AN-Motors
    };

    // Time slice spent on each backend before switching while searching
    constexpr unsigned long rxBackendSwitchMs = 400;

    RxBackend rxBackend = RxBackend::RcSwitch;
    unsigned long rxBackendStartMs = 0;

    /**
     * @brief Initialize radio
     */
    void initialize()
    {
      pinMode(rxPin, INPUT);
      pinMode(txPin, OUTPUT);

      // Setup transmitter on TX pin
      rcSwitch.enableTransmit(txPin);
      // Set transmit repetition to 1 packet per command
      rcSwitch.setRepeatTransmit(1);

      // Setup custom protocol backend on the same RX/TX pins
      RcCustom::initialize(rxPin, rxInterrupt, txPin);
    }

    /**
     * @brief Start receiving with the specified backend
     *
     * @param backend Receiver backend to activate
     */
    void startRxBackend(RxBackend backend)
    {
      if (backend == RxBackend::RcSwitch)
      {
        // Reset previous found signal if any
        rcSwitch.resetAvailable();
        // Enable receiver interrupt on RX pin
        rcSwitch.enableReceive(rxInterrupt);
      }
      else
      {
        RcCustom::resetAvailable();
        RcCustom::enableReceive();
      }

      rxBackend = backend;
      rxBackendStartMs = millis();
    }

    /**
     * @brief Stop the currently active receiver backend
     */
    void stopRxBackend()
    {
      if (rxBackend == RxBackend::RcSwitch)
      {
        rcSwitch.disableReceive();
      }
      else
      {
        RcCustom::disableReceive();
      }
    }

    /**
     * @brief Enable receiver interrupt handling
     */
    inline void enableReciever()
    {
      startRxBackend(RxBackend::RcSwitch);
    }

    /**
     * @brief Disable receiver interrupt handling
     */
    inline void disableReciever()
    {
      stopRxBackend();
    }

    /**
     * @brief Read received signal
     *
     * Alternates between receiver backends when no signal is found, so a single
     * search detects both rc-switch and custom protocols.
     *
     * @param signal Signal to read
     * @return true if signal was read, false if no signal received
     */
    bool readSignal(Slot::Signal &signal)
    {
      // Read signal data with disabled interrupts, as it is updated by the receiver interrupt
      noInterrupts();
      bool result = false;
      if (rxBackend == RxBackend::RcSwitch)
      {
        result = rcSwitch.available();
        if (result == true)
        {
          signal.protocol = rcSwitch.getReceivedProtocol();
          signal.value = rcSwitch.getReceivedValue();
          signal.bitLength = rcSwitch.getReceivedBitlength();
        }
      }
      else
      {
        result = RcCustom::available();
        if (result == true)
        {
          RcCustom::getReceived(signal.value, signal.protocol, signal.bitLength);
        }
      }
      interrupts();

      // Switch backend when the current one found nothing in its time slice
      if (result == false && (millis() - rxBackendStartMs >= rxBackendSwitchMs))
      {
        stopRxBackend();
        startRxBackend((rxBackend == RxBackend::RcSwitch) ? RxBackend::Custom
                                                          : RxBackend::RcSwitch);
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
      if (RcCustom::isSupported(signal.protocol) == true)
      {
        RcCustom::send(signal.value, signal.protocol, signal.bitLength);
      }
      else
      {
        rcSwitch.setProtocol(signal.protocol);
        rcSwitch.send(signal.value, signal.bitLength);
      }
    }
  } // namespace Radio

  // Menu item's functionality callback prototypes
  Menu::FunctionState slotItemCallback(Menu::Action action, int param);
  Menu::FunctionState slotEmulateCallback(Menu::Action action, int param);
  Menu::FunctionState slotSearchCallback(Menu::Action action, int param);
  Menu::FunctionState slotEditNameCallback(Menu::Action action, int param);
  Menu::FunctionState systemCallback(Menu::Action action, int param);

  namespace MenuItem
  {
    const char allowedChars[] PROGMEM = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    constexpr uint8_t allowedCharsCount = sizeof(allowedChars) - 1;

    // Menu item texts
    const char slotRootText[] PROGMEM = "Slots";
    const char settingsText[] PROGMEM = "Settings";
    const char slotEmulateText[] PROGMEM = "Emulate";
    const char slotSearchText[] PROGMEM = "Search";
    const char slotEditNameText[] PROGMEM = "Edit name";
    const char systemText[] PROGMEM = "System";

    // Menu item definitions
    extern Menu::Item slotRoot;
    extern Menu::Item slotList[];
    extern Menu::Item slotEmulate;
    extern Menu::Item slotSearch;
    extern Menu::Item slotEditName;
    extern Menu::Item settings;
    extern Menu::Item system;

    // Menu item fields: text, prev, next, child, callback, param, parent, isTextInRam

    // Root menu
    Menu::Item slotRoot = {slotRootText, nullptr, &settings, slotList, nullptr, 0, nullptr, false};
    Menu::Item settings = {settingsText, &slotRoot, nullptr, &system, nullptr, 0, nullptr, false};

    // Slots list menu (filled by setupSlots)
    Menu::Item slotList[Slot::slotsCount] = {};
    char slotNameList[Slot::slotsCount][Slot::nameLengthMax + 1] = {0};

    // Slot item menu
    Menu::Item slotEmulate = {slotEmulateText, nullptr, &slotSearch, nullptr, slotEmulateCallback, 0, nullptr, false};
    Menu::Item slotSearch = {slotSearchText, &slotEmulate, &slotEditName, nullptr, slotSearchCallback, 0, nullptr, false};
    Menu::Item slotEditName = {slotEditNameText, &slotSearch, nullptr, nullptr, slotEditNameCallback, 0, nullptr, false};

    // Settings menu
    Menu::Item system = {systemText, nullptr, nullptr, nullptr, systemCallback, 0, nullptr, false};

    uint8_t getCharAllowedIdx(char ch)
    {
      uint8_t charIdx = allowedCharsCount - 1;
      while (charIdx > 0 && ch != (char)pgm_read_byte(&allowedChars[charIdx]))
      {
        charIdx--;
      }

      return charIdx;
    }

    /**
     * @brief Setup slot menu items with slot data
     */
    void setupSlots()
    {
      for (uint8_t slotIdx = 0; slotIdx < Slot::slotsCount; slotIdx++)
      {
        Menu::Item &itemMenu = MenuItem::slotList[slotIdx];
        itemMenu.text = slotNameList[slotIdx];
        itemMenu.isTextInRam = true;
        Slot::getName(slotIdx, slotNameList[slotIdx]);
        itemMenu.prev = (slotIdx == 0) ? nullptr : &MenuItem::slotList[slotIdx - 1];
        itemMenu.next = (slotIdx == Slot::slotsCount - 1) ? nullptr : &MenuItem::slotList[slotIdx + 1];
        itemMenu.child = &MenuItem::slotEmulate;
        itemMenu.callback = slotItemCallback;
        itemMenu.param = slotIdx;
        // Slots list is shown on startup, so set parent to get back to the root menu
        itemMenu.parent = &MenuItem::slotRoot;
      }
    }
  } // namespace MenuItem

  const Menu::Item *pCurrentMenu = &MenuItem::slotList[0];
  uint8_t selectedSlotIdx = Slot::invalidIdx;

  // Buffer large enough for the longest protocol label (e.g. "AN-Motors")
  constexpr uint8_t protocolLabelSize = 10;

  /**
   * @brief Build a human-readable protocol label
   *
   * Custom protocols are shown by name, rc-switch protocols by number.
   *
   * @param protocol Protocol identifier
   * @param label Buffer to fill (at least protocolLabelSize bytes)
   */
  void getProtocolLabel(uint8_t protocol, char *label)
  {
    const char *name = RcCustom::getName_P(protocol);
    if (name != nullptr)
    {
      strncpy_P(label, name, protocolLabelSize - 1);
      label[protocolLabelSize - 1] = '\0';
    }
    else
    {
      snprintf_P(label, protocolLabelSize, PSTR("%02u"), protocol);
    }
  }

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
      if (buttonEvent == Button::Event::PressEnd ||
          buttonEvent == Button::Event::HoldStart ||
          buttonEvent == Button::Event::HoldContinue)
      {
        menuAction = Menu::Action::Prev;
      }
      break;

    case Button::Id::Down:
      if (buttonEvent == Button::Event::PressEnd ||
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
   * @brief Print menu item text padded with spaces to the specified width
   *
   * @param line Vertical line identifier
   * @param width Width of printed text, characters
   * @param pItem Menu item to print text
   */
  void printItemText(Display::Line line, uint8_t width, const Menu::Item *pItem)
  {
    if (pItem->isTextInRam == true)
    {
      Display::printString(0, line, width, pItem->text);
    }
    else
    {
      Display::printString_P(0, line, width, pItem->text);
    }
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
      if (isRootMenu == true)
      {
        MainMenu::showHeader(MainMenu::rootHeaderString);
      }
      else
      {
        printItemText(Display::Line::Header, MainMenu::headerLengthMax, pDrawItem->parent);
      }

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
      if (isRootMenu == true)
      {
        DISPLAY_PRINTF(0, Display::Line::Navigation, "        %2u/%-2u  ENTER>", itemIdx + 1, itemsCount);
      }
      else
      {
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<BACK   %2u/%-2u  ENTER>", itemIdx + 1, itemsCount);
      }

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
        }

        if (pItem != nullptr)
        {
          printItemText(line, MainMenu::itemTextLengthMax, pItem);
        }
        else
        {
          // Clear the line
          Display::printString(0, line, MainMenu::itemTextLengthMax, "");
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
  Menu::FunctionState slotEmulateCallback(Menu::Action action, int /* param */)
  {
    enum class State : uint8_t
    {
      Disabled,
      NoSignal,
      SignalOpened,
      Sending,
    };

    static State state = State::Disabled;
    static Slot::Signal txSignal = Slot::signalInvalid;
    static uint16_t txCount = 0;

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
        Slot::getSignal(selectedSlotIdx, txSignal);
        if (txSignal == Slot::signalInvalid)
        {
          MainMenu::showHeader(PSTR("No signal saved"));
          DISPLAY_PRINTF(0, Display::Line::Line_1, "Go to search menu");
          DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT");
          // Switch to no signal state
          state = State::NoSignal;
        }
        else
        {
          MainMenu::showHeader(PSTR("Signal TX"));
          char protocolLabel[protocolLabelSize];
          getProtocolLabel(txSignal.protocol, protocolLabel);
          DISPLAY_PRINTF(0, Display::Line::Line_1, "Protocol: %s", protocolLabel);
          DISPLAY_PRINTF(0, Display::Line::Line_2, "Value: 0x%02lX", txSignal.value);
          DISPLAY_PRINTF(0, Display::Line::Line_3, "Bits: %2u", txSignal.bitLength);
          DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT          SEND>");
          // Switch to signal opened state
          state = State::SignalOpened;
        }
      }
      break;

    default:
      break;
    }

    if (state == State::SignalOpened)
    {
      Button::Event buttonEvent = Button::getEvent(Button::Id::Right);
      if (buttonEvent == Button::Event::PressStart)
      {
        // Update display
        MainMenu::showHeader(PSTR("Sending..."));
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT         SEND>>");
        // Switch to sending state
        txCount = 0;
        state = State::Sending;
      }
    }

    if (state == State::Sending)
    {
      Button::State buttonState = Button::getState(Button::Id::Right);
      if (buttonState == Button::State::Released)
      {
        // Update display
        MainMenu::showHeader(PSTR("Signal TX"));
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT          SEND>");
        // Switch back to signal opened state
        state = State::SignalOpened;
      }
      else
      {
        // Send signal to the radio
        Radio::sendSignal(txSignal);
        txCount++;
        DISPLAY_PRINTF(9, Display::Line::Navigation, "%03u", txCount);
      }
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
  Menu::FunctionState slotSearchCallback(Menu::Action action, int /* param */)
  {
    enum class State : uint8_t
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
        MainMenu::showHeader(PSTR("Searching..."));
        DISPLAY_PRINTF(0, Display::Line::Line_1, "Please wait");
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT");
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
        // Update display
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT        REPEAT>");
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
        MainMenu::showHeader(PSTR("Signal RX"));
        char protocolLabel[protocolLabelSize];
        getProtocolLabel(rxSignal.protocol, protocolLabel);
        DISPLAY_PRINTF(0, Display::Line::Line_1, "Protocol: %s", protocolLabel);
        DISPLAY_PRINTF(0, Display::Line::Line_2, "Value: 0x%02lX", rxSignal.value);
        DISPLAY_PRINTF(0, Display::Line::Line_3, "Bits: %2u", rxSignal.bitLength);
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT REPEAT>/SAVE>>");

        // Switch to saved state
        state = State::Found;
      }
    }

    Menu::FunctionState functionState = (state == State::Disabled) ? Menu::FunctionState::Inactive
                                                                   : Menu::FunctionState::Active;

    return functionState;
  }

  /**
   * @brief Slot name editing item's functionality callback
   *
   * @param action New menu action
   * @param param Menu item's parameter
   * @return Current menu item's function state
   */
  Menu::FunctionState slotEditNameCallback(Menu::Action action, int /* param */)
  {
    enum class State : uint8_t
    {
      Disabled,
      Refresh,
      WaitInput,
    };

    static State state = State::Disabled;
    static char *currentName = nullptr;
    static char newName[Slot::nameLengthMax + 1];
    static uint8_t editCharOffset;
    static uint8_t editCharAllowedIdx;

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
        MainMenu::showHeader(PSTR("Edit name"));
        // Copy current slot name
        currentName = MenuItem::slotNameList[selectedSlotIdx];
        snprintf_P(newName, sizeof(newName), PSTR("%-12s"), currentName);
        editCharOffset = 0;
        editCharAllowedIdx = MenuItem::getCharAllowedIdx(newName[editCharOffset]);
        // Switch to refresh state
        state = State::Refresh;
      }
      else if (state == State::WaitInput)
      {
        if (editCharOffset < Slot::nameLengthMax - 1)
        {
          // Shift cursor to the right
          editCharOffset++;
        }
        else
        {
          // Loop to the left
          editCharOffset = 0;
        }
        editCharAllowedIdx = MenuItem::getCharAllowedIdx(newName[editCharOffset]);
        // Switch to refresh state
        state = State::Refresh;
      }
      break;

    case Menu::Action::Back:
      if (state == State::WaitInput)
      {
        if (editCharOffset > 0)
        {
          // Shift cursor to the left
          editCharOffset--;
        }
        else
        {
          // Loop to the right
          editCharOffset = Slot::nameLengthMax - 1;
        }
        editCharAllowedIdx = MenuItem::getCharAllowedIdx(newName[editCharOffset]);
        // Switch to refresh state
        state = State::Refresh;
      }
      break;

    case Menu::Action::Prev:
      if (state == State::WaitInput)
      {
        if (editCharAllowedIdx > 0)
        {
          editCharAllowedIdx--;
        }
        else
        {
          editCharAllowedIdx = MenuItem::allowedCharsCount - 1;
        }
        // Switch to refresh state
        state = State::Refresh;
      }
      break;

    case Menu::Action::Next:
      if (state == State::WaitInput)
      {
        if (editCharAllowedIdx < MenuItem::allowedCharsCount - 1)
        {
          editCharAllowedIdx++;
        }
        else
        {
          editCharAllowedIdx = 0;
        }
        // Switch to refresh state
        state = State::Refresh;
      }
      break;

    case Menu::Action::Set:
      if (state == State::WaitInput)
      {
        bool isNotEqual = (strncmp(newName, currentName, strlen(newName)) != 0);
        if (isNotEqual == true)
        {
          // Copy new slot name
          snprintf_P(currentName, sizeof(MenuItem::slotNameList[0]), PSTR("%-12s"), newName);
          // Save new slot name on the storage
          Slot::setName(selectedSlotIdx, newName);
          DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT               ");
        }
      }
      break;

    default:
      break;
    }

    if (state == State::Refresh)
    {
      // Update name
      newName[editCharOffset] = pgm_read_byte(&MenuItem::allowedChars[editCharAllowedIdx]);
      Display::setSize(Display::Size::Font_8x16, true);
      DISPLAY_PRINTF(0, Display::Line::Line_2, "%s", newName);
      Display::setInverted(true);
      DISPLAY_PRINTF(editCharOffset, Display::Line::Line_2, "%c", newName[editCharOffset]);
      Display::setSize(Display::Size::Font_6x8, true);

      bool isEqual = (strncmp(newName, currentName, strlen(newName)) == 0);
      if (isEqual == true)
      {
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT               ");
      }
      else
      {
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT         SAVE>>");
      }

      // Switch to wait input state
      state = State::WaitInput;
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
  Menu::FunctionState systemCallback(Menu::Action action, int /* param */)
  {
    enum class State : uint8_t
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
        MainMenu::showHeader(PSTR("System info"));
        MainMenu::showSystemInfo();
        DISPLAY_PRINTF(0, Display::Line::Navigation, "<<EXIT");
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
      if (currentTimeMs - lastUpdateTimeMs > MainMenu::systemInfoUpdatePeriodMs)
      {
        lastUpdateTimeMs = currentTimeMs;

        uint16_t batteryVoltage = Battery::readVoltage();
        DISPLAY_PRINTF(0, Display::Line::Line_3, "Battery: %4umV", batteryVoltage);
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
  MainMenu::showHeaderCentered(MainMenu::rootHeaderString);
  MainMenu::showSystemInfo();
  unsigned long welcomeStartTimeMs = millis();

  // Initialize buttons
  Button::initialize();

  // Initialize radio
  Radio::initialize();

  // Erase all slots on the storage
  // Slot::eraseStorage();

  // Setup slot menu items with slot data
  MenuItem::setupSlots();

  // Wait until welcome screen time ends
  while (millis() - welcomeStartTimeMs < MainMenu::welcomeTimeMs)
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
    // Enum values are cast to unsigned int, as 1-byte enum is passed through variadic arguments as is
    Log::printf("UP:%u DOWN:%u LEFT:%u RIGHT:%u",
                static_cast<unsigned int>(Button::getState(Button::Id::Up)),
                static_cast<unsigned int>(Button::getState(Button::Id::Down)),
                static_cast<unsigned int>(Button::getState(Button::Id::Left)),
                static_cast<unsigned int>(Button::getState(Button::Id::Right)));
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
