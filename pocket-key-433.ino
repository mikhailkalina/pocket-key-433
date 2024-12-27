#include <CRC.h>
#include <EEPROM.h>
#include <RCSwitch.h>
#include <ssd1306.h>

#define RADIO_RX_PIN (2)
#define RADIO_TX_PIN (10)

#define BTN_UP_PIN (4)
#define BTN_DOWN_PIN (5)
#define BTN_LEFT_PIN (6)
#define BTN_RIGHT_PIN (7)

#define BATTERY_PIN (A0)

#define BTN_DEBOUNCE_TIME_MS (20)
#define BTN_HOLD_TIME_MS (1000)

// Receiver on pin #2 => that is interrupt 0
#define RADIO_RX_INTERRUPT (0)

#define SLOT_COUNT_MAX (20)
#define SLOT_NAME_LENGTH_MAX (10)

#define SIGNAL_PROTOCOL_INV (0)
#define SIGNAL_PROTOCOL_MIN (1)
#define SIGNAL_PROTOCOL_MAX (12)

enum ButtonId
{
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_COUNT, // Should be the last one
  BTN_NONE = BTN_COUNT,
};

enum ButtonState
{
  BTN_RELEASED,
  BTN_PRESSED,
  BTN_HOLD,
};

struct Button
{
  const uint8_t pin;
  const uint8_t pressLevel;
  ButtonState state;
  bool isPressed;
  unsigned long pressTimeMs;
};

// Radio signal structure
struct Signal
{
  unsigned int protocol;
  unsigned long value;
  unsigned int bitLength;
};

// Storage slot structure
struct Slot
{
  char name[SLOT_NAME_LENGTH_MAX + 1]; // 10 chars + 1 end of line
  bool isFilled;
  Signal signal;
};

// String for start display
const char startString[] = "Pocket Key 433";
// Invalid signal structure
const Signal signalInvalid = {
    .protocol = SIGNAL_PROTOCOL_INV,
    .value = 0,
    .bitLength = 0,
};

// User buttons list
Button buttonList[BTN_COUNT] = {
    {.pin = BTN_UP_PIN, .pressLevel = LOW},    // BTN_UP
    {.pin = BTN_DOWN_PIN, .pressLevel = LOW},  // BTN_DOWN
    {.pin = BTN_LEFT_PIN, .pressLevel = LOW},  // BTN_LEFT
    {.pin = BTN_RIGHT_PIN, .pressLevel = LOW}, // BTN_RIGHT
};

// Storage slots list
Slot slotList[SLOT_COUNT_MAX] = {0};
size_t currentSlotIdx = 0;

RCSwitch rcSwitch = RCSwitch();

void logPrintf(const char *format, ...)
{
#if 1
  static char buffer[100];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
#endif
}

void displayText(uint8_t xpos, uint8_t y, EFontStyle style, const char *format, ...)
{
  static char buffer[25];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  ssd1306_printFixed(xpos, y, buffer, style);
}

void resetSlot(int slotIdx)
{
  Slot &slot = slotList[slotIdx]; // slot to reset

  // Reset name to default
  snprintf(slot.name, sizeof(slot.name), "Slot %02d", slotIdx + 1);

  logPrintf("Reset %s", slot.name);

  // Empty the slot
  slot.isFilled = false;
  // Invalidate the signal
  slot.signal = signalInvalid;
}

void saveSlot(int slotIdx)
{
  Slot &slot = slotList[slotIdx]; // current slot
  uint8_t crc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
  int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8)); // Slot size + CRC  size
  int crc8Address = slotAddress + sizeof(Slot);

  logPrintf("Save %s", slot.name);

  EEPROM.put(slotAddress, slot);
  EEPROM.put(crc8Address, crc8);
}

size_t loadSlots()
{
  size_t filledCount = 0;

  for (size_t slotIdx = 0; slotIdx < SLOT_COUNT_MAX; slotIdx++)
  {
    Slot &slot = slotList[slotIdx]; // current slot
    uint8_t crc8 = 0;
    int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8)); // Slot size + CRC size
    int crc8Address = slotAddress + sizeof(Slot);

    EEPROM.get(slotAddress, slot);
    EEPROM.get(crc8Address, crc8);

    uint8_t calcCrc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
    if (calcCrc8 == crc8)
    {
      // Slot is valid on EEPROM
      if (slot.isFilled == true)
      {
        logPrintf("%s: %u %lu/%u", slot.name, slot.signal.protocol, slot.signal.value, slot.signal.bitLength);
        filledCount++;
      }
      else
      {
        logPrintf("%s: empty", slot.name);
      }
    }
    else
    {
      // Reset slot if it isn't valid
      resetSlot(slotIdx);
      // Save to the EEPROM
      saveSlot(slotIdx);
    }
  }

  return filledCount;
}

void eraseSlots()
{
  const int storageSize = SLOT_COUNT_MAX * (sizeof(Slot) + sizeof(uint8_t)); // Slot size + CRC size
  for (int idx = 0; idx < storageSize; idx++)
  {
    // Erase storage with 0xFF
    EEPROM.write(idx, 0xFF);
  }
}

bool handleButtons()
{
  bool isStateChanged = false;
  // Get current system time
  unsigned long currentTimeMs = millis();

  for (size_t btnId = 0; btnId < BTN_COUNT; btnId++)
  {
    Button &button = buttonList[btnId]; // current button
    ButtonState buttonPrevState = button.state;

    uint8_t pinLevel = digitalRead(button.pin);
    if (pinLevel == button.pressLevel)
    {
      // Button is pressed
      if (button.isPressed == false)
      {
        // First time
        button.isPressed = true;
        button.pressTimeMs = currentTimeMs;
      }
      else if (button.state == BTN_RELEASED && button.pressTimeMs + BTN_DEBOUNCE_TIME_MS < currentTimeMs)
      {
        // Debounce time passed after press
        button.state = BTN_PRESSED;
      }
      else if (button.state == BTN_PRESSED && button.pressTimeMs + BTN_HOLD_TIME_MS < currentTimeMs)
      {
        // Hold time passed after press
        button.state = BTN_HOLD;
      }
    }
    else if (button.isPressed == true)
    {
      // Button is released
      button.isPressed = false;
      button.state = BTN_RELEASED;
    }

    if (button.state != buttonPrevState)
    {
      isStateChanged = true;
    }
  }

  return isStateChanged;
}

ButtonState getButtonState(ButtonId btnId)
{
  const Button &button = buttonList[btnId];

  return button.state;
}

void initializePins()
{
  pinMode(RADIO_RX_PIN, INPUT);
  pinMode(RADIO_TX_PIN, OUTPUT);

  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);

  pinMode(BATTERY_PIN, INPUT);
}

void setup()
{
  Serial.begin(115200);

  initializePins();

  // Enable interrupt on RX pin
  rcSwitch.enableReceive(RADIO_RX_INTERRUPT);

  // Initialize display
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);

  ssd1306_setFixedFont(ssd1306xled_font8x16);
  displayText(0, 0, STYLE_BOLD, startString);

  // eraseSlots(); // uncomment to erase all slots in the storage
  size_t filledCount = loadSlots();

  ssd1306_setFixedFont(ssd1306xled_font6x8);
  displayText(0, 56, STYLE_NORMAL, "Slots used: %d/%d", filledCount, SLOT_COUNT_MAX);
}

void loop()
{
  if (handleButtons())
  {
    logPrintf("UP:%d DOWN:%d LEFT:%d RIGHT:%d",
              getButtonState(BTN_UP), getButtonState(BTN_DOWN),
              getButtonState(BTN_LEFT), getButtonState(BTN_RIGHT));
  }

  if (rcSwitch.available())
  {
    unsigned int rxProto = rcSwitch.getReceivedProtocol();
    unsigned long rxValue = rcSwitch.getReceivedValue();
    unsigned int rxBits = rcSwitch.getReceivedBitlength();

    logPrintf("Rx %02u: %lu/%u", rxProto, rxValue, rxBits);

    rcSwitch.resetAvailable();
  }
}
