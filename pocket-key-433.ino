#include <Arduino.h>
#include <RCSwitch.h>
#include <ssd1306.h>

#include "button.h"
#include "log.h"
#include "slot.h"

#define RADIO_RX_PIN (2)
#define RADIO_TX_PIN (10)

#define BATTERY_PIN (A0)

// Receiver on pin #2 => that is interrupt 0
#define RADIO_RX_INTERRUPT (0)

// String for start display
const char startString[] = "Pocket Key 433";

size_t currentSlotIdx = 0;

RCSwitch rcSwitch = RCSwitch();

void displayText(uint8_t xpos, uint8_t y, EFontStyle style, const char *format, ...)
{
  static char buffer[25];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  ssd1306_printFixed(xpos, y, buffer, style);
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
  ssd1306_fillScreen(0x00);

  ssd1306_setFixedFont(ssd1306xled_font8x16);
  displayText(0, 0, STYLE_BOLD, startString);

  // SLOT_eraseAll(); // uncomment to erase all slots on the storage
  size_t usedSlotCount = SLOT_loadAll();

  ssd1306_setFixedFont(ssd1306xled_font6x8);
  displayText(0, 56, STYLE_NORMAL, "Slots used: %d/%d", usedSlotCount, SLOT_COUNT_MAX);
}

void loop()
{
  bool isActionDetected = BTN_handle();
  if (isActionDetected == true)
  {
    LOG_printf("UP:%d DOWN:%d LEFT:%d RIGHT:%d",
              BTN_getAction(BTN_ID_UP), BTN_getAction(BTN_ID_DOWN),
              BTN_getAction(BTN_ID_LEFT), BTN_getAction(BTN_ID_RIGHT));
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
