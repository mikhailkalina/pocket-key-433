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

unsigned long rxTimeMs = 0;

RCSwitch rcSwitch = RCSwitch();

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

  // Initialize buttons
  BTN_initialize();

  // Enable interrupt on RX pin
  rcSwitch.enableReceive(RADIO_RX_INTERRUPT);

  // Initialize display
  DISP_initialize();

  // SLOT_eraseAll(); // uncomment to erase all slots on the storage
  size_t usedSlotCount = SLOT_loadAll();

  MENU_draw();
}

void loop()
{
  ButtonId actionBtnId = BTN_handle();
  if (actionBtnId != BTN_ID_NONE)
  {
    LOG_printf("UP:%d DOWN:%d LEFT:%d RIGHT:%d",
               BTN_getAction(BTN_ID_UP), BTN_getAction(BTN_ID_DOWN),
               BTN_getAction(BTN_ID_LEFT), BTN_getAction(BTN_ID_RIGHT));

    MENU_navigate(actionBtnId);
  }

  if (rcSwitch.available())
  {
    unsigned int rxProto = rcSwitch.getReceivedProtocol();
    unsigned long rxValue = rcSwitch.getReceivedValue();
    unsigned int rxBits = rcSwitch.getReceivedBitlength();

    LOG_printf("Rx %02u: %lu/%u", rxProto, rxValue, rxBits);

    DISP_printf(96, 56, STYLE_NORMAL, TEXT_SIZE_8, "RX<<");
    rxTimeMs = millis();

    rcSwitch.resetAvailable();
  }
  else if (rxTimeMs > 0 && rxTimeMs + 100 < millis())
  {
    DISP_printf(96, 56, STYLE_NORMAL, TEXT_SIZE_8, "");
    rxTimeMs = 0;
  }
}
