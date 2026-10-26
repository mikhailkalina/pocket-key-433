# Pocket Key 433

Portable 433 MHz remote signal receiver and transmitter based on an Arduino Nano (ATmega328P).

## Features

- Receive a remote control signal and save it to one of 20 slots
- Transmit a saved signal while the button is held
- Edit slot names, up to 12 characters
- Store slots in EEPROM with a CRC check
- Show battery level and charging status
- Automatically enter `POWER_DOWN` after a configurable period of menu inactivity
- Switch the display, receiver and transmitter supplies independently

Supported protocols: rc-switch 1–12; CAME 12-bit, NICE 12-bit, and AN-Motors (fixed part only).

## Hardware and pinout

The design uses an Arduino Nano, 433 MHz OOK receiver and transmitter, 128×64 SSD1306 I²C OLED, four buttons connected to GND, and a Li-ion charger (for example TP4056) with a stable 5 V boost converter. All modules must share GND.

| Nano pin | Direction | Purpose |
| --- | --- | --- |
| D2 | Input | Receiver DATA. This is INT0, used by rc-switch and the custom protocol decoder while **Search** is active. |
| D3 | Output | Receiver supply control, active HIGH only during **Search**. |
| D4 | Input, pull-up | UP button to GND. Also a pin-change interrupt wake source. |
| D5 | Input, pull-up | DOWN button to GND. Also a pin-change interrupt wake source. |
| D6 | Input, pull-up | LEFT button to GND. Also a pin-change interrupt wake source. |
| D7 | Input, pull-up | RIGHT button to GND. Also a pin-change interrupt wake source. |
| D8 | Input, pull-up | Charger status and sleep wake source (PB0/PCINT0), active LOW. Connect to the **open-drain CHRG/status output**, not the charger supply input. LOW means charging; the internal pull-up holds HIGH otherwise. LOW wakes the device and keeps its display on. Check the pinout of your charger board. |
| D9 | Output | OLED supply control, active HIGH. On while awake; off during sleep. Intended as the control input for a future power switch. |
| D10 | Output | Transmitter DATA. Held LOW when the transmitter is off; driven with OOK pulses during transmission. |
| D11 | Output | Transmitter supply control, active HIGH only while sending a signal. |
| A0 | Analog input | Battery voltage measurement. Connect to battery positive **only when its maximum voltage stays below the Nano's ADC reference / VCC**; the firmware assumes a stable 5 V ADC reference and converts 0–5 V to millivolts without a divider. For a different supply or a divider, update `battery.cpp`. Do not connect A0 to the 5 V boost output if you want the battery voltage. |
| A4 | I²C SDA | OLED data. Released as an input before OLED power is removed. |
| A5 | I²C SCL | OLED clock. Released as an input before OLED power is removed. |

### Supply current and switching

**Do not assume D3/D9/D11 can safely supply arbitrary modules directly.** The [official Nano pinout](https://docs.arduino.cc/resources/pinouts/A000005-full-pinout.pdf) specifies **20 mA maximum per I/O pin**. The ATmega328P's [40 mA figure is an absolute maximum stress rating](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf), not an operating target. Actual output voltage also falls under load.

For scale, an [SSD1306 OLED breakout may draw about 20 mA on average](https://learn.adafruit.com/monochrome-oled-breakouts/power-requirements), already at the Nano limit. A [receiver example specifies 4.8–6.2 mA](https://micmodshop.ir/wp-content/uploads/2023/09/RXB6-DataSheet.pdf), while an [FS1000A transmitter example specifies 20–28 mA](https://www.jyctec.com/pd.jsp?id=636). These are examples, not specifications for your exact modules. Direct GPIO supply is plausible only for a measured receiver below 20 mA; the OLED is borderline and the transmitter can exceed the limit. Use external switches for D9 and D11, and preferably D3 as well. Check inrush current and the module's required supply voltage before any direct connection.

The firmware's `power.cpp` uses active-HIGH outputs, suitable as control signals for a driver or a suitably wired switch. If your high-side key is active LOW, invert the corresponding driver logic there. When switching the OLED supply, make sure SDA/SCL pull-ups do not remain tied to an unswitched 5 V rail: they can back-power the unpowered display. The firmware releases A4/A5, but external pull-ups must follow the switched rail or be isolated. The receiver and transmitter signal pins must likewise not back-power unpowered modules.

Power-on waits are centralized in `power.cpp`: **100 ms for the OLED** (measured on this device), **10 ms for RX**, and **2 ms for TX**. The radio values are provisional because the exact modules are unspecified; verify them with the installed modules, especially the first received/transmitted packet. For comparison, [NiceRF specifies 3 ms receiving start time for its SRX882S](https://www.nicerf.com/ask-modules/superheterodyne-receiver-srx882s.html), which does not establish the timing of another receiver. The [SSD1306 datasheet's 100 ms](https://www.displayfuture.com/Display/datasheet/controller/SSD1306.pdf) describes the time from the display-ON command until the panel lights up; it is not a prescribed wait before sending I²C initialization commands.

## Controls and menu

- UP / DOWN: select an item; hold to scroll
- RIGHT: enter; hold to send a slot signal or save an edit
- LEFT: back; on the **Main menu** it is the `<SLEEP` button for immediate sleep; hold to exit name editing

Menu:

- **Slots** — list of slots, shown on startup
  - **Emulate** — show the saved signal; hold RIGHT to transmit
  - **Search** — receive a signal; RIGHT to search again; hold RIGHT to save it
  - **Edit name** — UP / DOWN change a character; LEFT / RIGHT move the cursor
- **Settings**
  - **System** — battery voltage and firmware version
  - **Auto sleep** — UP / DOWN choose Off, 10, 20, 30, 40, 50, or 60 seconds, then 2, 3, 4, or 5 minutes; hold RIGHT to save, LEFT to leave

## Power saving

The inactivity timer runs **only while navigating menus and submenus**. An opened function screen, including an active scan, transmission, name editor, system screen, or sleep-setting editor, will not be interrupted by automatic sleep. The default timeout is 60 seconds; **Off** disables only the inactivity timer. On the Main menu, a short LEFT press invokes `<SLEEP` immediately even when Auto sleep is Off. While D8 is LOW (charging), neither automatic nor manual sleep is allowed; the full inactivity period starts after D8 returns HIGH. The selection is stored by the separate `settings.cpp` module in EEPROM after the 20 slot records (starting at byte 400); its versioned record migrates the previous timeout choice and can be extended for future settings.

When the timeout expires, receiver and transmitter power are off, the I²C bus is released, display power is removed, and the ATmega328P enters `POWER_DOWN`. Any button D4–D7 wakes it through `PCINT2`; D8 going LOW wakes it through `PCINT0` so the battery and charging indication becomes visible. The display is reinitialized and the current menu is redrawn. A wake button press is consumed until the buttons are released, so it does not also activate a menu item. Timer-driven `millis()` does not advance during `POWER_DOWN`. The Nano's USB interface, power LED, boost converter, charger, and any unswitched external circuits still draw current; total device consumption depends on the hardware.

## Battery

- The icon displays four bars over the 3.5–4.2 V battery range. Voltage filtering takes about 6 seconds, with ±40 mV level hysteresis.
- D8 reads charger status. The bars animate while charging.
- Calibration and thresholds are in `battery.cpp`.

## Build

Use Arduino IDE or `arduino-cli`, board `arduino:avr:nano`.

Libraries: ssd1306 by Alexey Dynda, CRC by Rob Tillaart, rc-switch by sui77.

Debug logs to serial at 115200 baud are enabled by `LOG_DEBUG` in `pocket-key-433.ino`.
