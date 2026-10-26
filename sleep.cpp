#include "sleep.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "button.h"
#include "battery.h"

// D4..D7 are PD4..PD7 / PCINT20..PCINT23; D8 is PB0 / PCINT0.
namespace
{
  constexpr uint8_t buttonMask = _BV(4) | _BV(5) | _BV(6) | _BV(7);
  constexpr uint8_t chargerMask = _BV(PCINT0);
  volatile bool wakeRequested = false;
}

/**
 * @brief Mark a change on one of the D4-D7 button pins as a wake event
 */
ISR(PCINT2_vect) { wakeRequested = true; }

/**
 * @brief Mark a change on the D8 charger status pin as a wake event
 */
ISR(PCINT0_vect) { wakeRequested = true; }

/**
 * @brief Enter power-down sleep until a button or charger pin change wakes the AVR
 *
 * Returns immediately if a button is held or the active-LOW charger signal is LOW.
 */
void Sleep::untilWakeSource()
{
  const uint8_t oldAdcsra = ADCSRA;
  ADCSRA &= ~_BV(ADEN);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  noInterrupts();
  wakeRequested = false;
  PCIFR = _BV(PCIF0) | _BV(PCIF2); // discard changes that preceded arming
  PCMSK0 |= chargerMask;
  PCMSK2 |= buttonMask;
  PCICR |= _BV(PCIE0) | _BV(PCIE2);

  while (!wakeRequested && !Button::anyPressed() && !Battery::isChargeSignalActive())
  {
    sleep_enable();
#if defined(BODS) && defined(BODSE)
    sleep_bod_disable();
#endif
    interrupts();
    sleep_cpu();
    sleep_disable();
    noInterrupts();
  }
  PCICR &= ~(_BV(PCIE0) | _BV(PCIE2));
  PCMSK0 &= ~chargerMask;
  PCMSK2 &= ~buttonMask;
  PCIFR = _BV(PCIF0) | _BV(PCIF2);
  interrupts();
  ADCSRA = oldAdcsra;
}
