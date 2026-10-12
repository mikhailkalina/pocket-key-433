# Pocket Key 433
Portable device pocket size to sniff and generate 433MHz radio remote control signals.

## Features
- Receive a remote control signal and save it to one of 20 slots
- Transmit a saved signal while the button is held
- Edit slot names, up to 12 characters
- Slots are kept in EEPROM with CRC check
- Battery level and charging status indicator

Supported protocols:
- rc-switch protocols 1-12
- CAME 12-bit, NICE 12-bit, AN-Motors (fixed part only)

## Hardware
- Arduino Nano
- 433MHz OOK receiver and transmitter modules
- SSD1306 128x64 OLED display, I2C
- 4 buttons connected to GND
- Li-ion battery with a charger (e.g. TP4056) and a 5V boost converter

Pinout (arduino nano):
- radio RX: D2
- radio TX: D10
- button UP: D4
- button DOWN: D5
- button LEFT: D6
- button RIGHT: D7
- battery charging IN: D8
- battery IN: A0
- display SDA: A4
- display SCL: A5

## Controls
- UP / DOWN: select item, hold to scroll
- RIGHT: enter, hold to send the slot signal from the slots list or to save
- LEFT: back, hold to exit name editing

Menu:
- Slots - list of slots, shown on startup
  - Emulate - show the slot signal, hold RIGHT to transmit
  - Search - receive a signal, RIGHT to search again, hold RIGHT to save it to the slot
  - Edit name - UP / DOWN change the character, LEFT / RIGHT move the cursor
- Settings
  - System - battery voltage and firmware version

## Battery
- Battery voltage is measured on A0 against the 5V supply, so the board needs a stable 5V source.
- The icon in the top right corner shows 4 bars for 3.5-4.2V. The voltage is filtered (~6s)
  and has a ±40mV hysteresis between levels.
- Charging status is read from D8: LOW means charging, internal pull-up enabled, e.g. TP4056 CHRG pin.
  The bars are animated while charging.
- All values are set in `battery.cpp`.

## Build
Arduino IDE or arduino-cli, board Arduino Nano (`arduino:avr:nano`).

Arduino libraries used:
- ssd1306 by Alexey Dynda
- CRC by Rob Tillaart
- rc-switch by sui77

Debug log to serial port at 115200 baud is enabled by `LOG_DEBUG` define in `pocket-key-433.ino`.
