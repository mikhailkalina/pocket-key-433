#include <CRC.h>
#include <EEPROM.h>
#include <RCSwitch.h>
#include <ssd1306.h>

#define RC_RX_PIN (2)
#define RC_TX_PIN (10)

#define BTN_UP_PIN (4)
#define BTN_DOWN_PIN (5)
#define BTN_LEFT_PIN (6)
#define BTN_RIGHT_PIN (7)

// Receiver on pin #2 => that is interrupt 0
#define RC_RX_INTERRUPT (0)

#define SLOT_COUNT_MAX (20)
#define SLOT_NAME_LENGTH_MAX (10)

#define SIGNAL_PROTOCOL_INV (0)
#define SIGNAL_PROTOCOL_MIN (1)
#define SIGNAL_PROTOCOL_MAX (12)

// Radio signal structure
struct Signal {
  unsigned int protocol;
  unsigned long value;
  unsigned int bitLength;
};

// Storage slot structure
struct Slot {
  char name[SLOT_NAME_LENGTH_MAX + 1];  // 10 chars + 1 end of line
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

// Storage slots list
Slot slotList[SLOT_COUNT_MAX];
int currentSlotIdx = 0;

RCSwitch rcSwitch = RCSwitch();

void logPrintf(const char *format, ...) {
#if 1
  static char buffer[100];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
#endif
}

void resetSlot(int slotIdx) {
  Slot &slot = slotList[slotIdx];  // slot to reset

  // Reset name to default
  snprintf(slot.name, sizeof(slot.name), "Slot %02d", slotIdx);
  // Empty the slot
  slot.isFilled = false;
  // Invalidate the signal
  slot.signal = signalInvalid;
}

void saveSlotToEEPROM(int slotIdx) {
    Slot &slot = slotList[slotIdx];  // current slot
    uint8_t crc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
    int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8));  // Slot size + CRC  size
    int crc8Address = slotAddress + sizeof(Slot);

    EEPROM.put(slotAddress, slot);
    EEPROM.put(crc8Address, crc8);
}

void loadSlotsFromEEPROM() {
  for (size_t slotIdx = 0; slotIdx < SLOT_COUNT_MAX; slotIdx++) {
    Slot &slot = slotList[slotIdx];  // current slot
    uint8_t crc8 = 0;
    int slotAddress = slotIdx * (sizeof(Slot) + sizeof(crc8));  // Slot size + CRC  size
    int crc8Address = slotAddress + sizeof(Slot);

    EEPROM.get(slotAddress, slot);
    EEPROM.get(crc8Address, crc8);

    uint8_t calcCrc8 = calcCRC8((const uint8_t *)&slot, sizeof(slot));
    if (calcCrc8 == crc8) {
      // Slot is valid on EEPROM
      logPrintf("Slot %02d is %s", slotIdx, slot.isFilled ? "filled" : "empty");
    } else {
      // Reset slot if it isn't valid
      resetSlot(slotIdx);
      // Save to the EEPROM
      saveSlotToEEPROM(slotIdx);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Enable interrupt on RX pin
  rcSwitch.enableReceive(RC_RX_INTERRUPT);

  // Initialize display
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  ssd1306_printFixed(0, 8, startString, STYLE_BOLD);
  
  loadSlotsFromEEPROM();
}

void loop() {
  if (rcSwitch.available()) {

    unsigned int rxProto = rcSwitch.getReceivedProtocol();
    unsigned long rxValue = rcSwitch.getReceivedValue();
    unsigned int rxBits = rcSwitch.getReceivedBitlength();

    logPrintf("Rx %02u: %lu/%u", rxProto, rxValue, rxBits);

    rcSwitch.resetAvailable();
  }
}
