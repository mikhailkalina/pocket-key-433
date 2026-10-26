#include "battery.h"

#include <stdbool.h>
#include <stdint.h>

#include <Arduino.h>

using namespace Battery;

namespace
{
    constexpr uint8_t voltagePin = A0;
    // Charger status output, e.g. TP4056 CHRG (open drain, active while charging)
    constexpr uint8_t chargingPin = 8;
    constexpr uint8_t chargingActiveLevel = LOW;

    constexpr uint16_t voltageRefMv = 5000; // ADC reference voltage, millivolts
    constexpr uint16_t rawValueMax = 1023;  // bits

    // Battery voltage range mapped to charge levels
    constexpr uint16_t voltageEmptyMv = 3500;
    constexpr uint16_t voltageFullMv = 4200;
    // Voltage has to cross a level boundary by this value to change the level
    constexpr uint16_t hysteresisMv = 40;

    constexpr unsigned long samplePeriodMs = 100;
    // Exponential moving average factor 1/2^filterShift, time constant ~6.4s at 100ms sampling
    constexpr uint8_t filterShift = 6;
    // Charging status has to stay changed for this time to be accepted
    constexpr unsigned long chargingDebounceMs = 500;

    /**
     * @brief Return voltage boundary between the specified level and the level below
     * Boundaries are placed halfway between levels, so the level is the nearest one to the voltage
     *
     * @param level Charge level from 1 to levelMax
     * @return Boundary voltage, millivolts
     */
    constexpr uint16_t getLevelBoundaryMv(uint8_t level)
    {
        return voltageEmptyMv + (uint32_t)(voltageFullMv - voltageEmptyMv) * (2 * level - 1) / (2 * levelMax);
    }

    static_assert(2 * hysteresisMv < getLevelBoundaryMv(2) - getLevelBoundaryMv(1),
                  "Hysteresis is too large for the level step");
    static_assert(((uint32_t)rawValueMax << filterShift) <= 0xFFFF,
                  "Filtered value does not fit 16 bits");

    // Filtered raw ADC value scaled by 2^filterShift
    uint16_t filteredValue = 0;
    uint8_t currentLevel = 0;
    bool isChargingState = false;
    unsigned long sampleTimeMs = 0;
    unsigned long chargingChangeTimeMs = 0;

    /**
     * @brief Update charge level according to the voltage
     *
     * @param voltage Battery voltage, millivolts
     * @param hysteresis Voltage to cross a level boundary by, millivolts
     */
    void updateLevel(uint16_t voltage, uint16_t hysteresis)
    {
        while (currentLevel < levelMax && voltage >= getLevelBoundaryMv(currentLevel + 1) + hysteresis)
        {
            currentLevel++;
        }

        while (currentLevel > 0 && voltage < getLevelBoundaryMv(currentLevel) - hysteresis)
        {
            currentLevel--;
        }
    }
} // namespace

/**
 * @brief Initialize battery voltage and charging status readings
 */
void Battery::initialize()
{
    pinMode(voltagePin, INPUT);
    pinMode(chargingPin, INPUT_PULLUP);

    // Start the filter from the sum of initial readings, that equals their average scaled by 2^filterShift
    filteredValue = 0;
    for (uint8_t sampleIdx = 0; sampleIdx < (1 << filterShift); sampleIdx++)
    {
        filteredValue += analogRead(voltagePin);
    }

    // Set the nearest level initially
    currentLevel = 0;
    updateLevel(getVoltage(), 0);

    isChargingState = (digitalRead(chargingPin) == chargingActiveLevel);

    sampleTimeMs = millis();
    chargingChangeTimeMs = sampleTimeMs;
}

/**
 * @brief Process battery readings periodically
 * Should be called from the main loop
 */
void Battery::process()
{
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - sampleTimeMs < samplePeriodMs)
    {
        return;
    }

    sampleTimeMs = currentTimeMs;

    // Filter voltage readings with exponential moving average
    filteredValue = filteredValue - (filteredValue >> filterShift) + analogRead(voltagePin);
    updateLevel(getVoltage(), hysteresisMv);

    bool isPinActive = (digitalRead(chargingPin) == chargingActiveLevel);
    if (isPinActive == isChargingState)
    {
        // No change - restart debounce time
        chargingChangeTimeMs = currentTimeMs;
    }
    else if (currentTimeMs - chargingChangeTimeMs >= chargingDebounceMs)
    {
        // Pin level is stable for debounce time - accept new status
        isChargingState = isPinActive;
    }
}

/**
 * @brief Return filtered battery voltage
 *
 * @return Battery voltage, millivolts
 */
uint16_t Battery::getVoltage()
{
    // Calculate voltage from filtered reading
    return (uint32_t)filteredValue * voltageRefMv / ((uint32_t)rawValueMax << filterShift);
}

/**
 * @brief Return battery charge level with hysteresis applied
 *
 * @return Charge level from 0 (empty) to levelMax (full)
 */
uint8_t Battery::getLevel()
{
    return currentLevel;
}

/**
 * @brief Return debounced battery charging status
 *
 * @return true if battery is charging, false otherwise
 */
bool Battery::isCharging()
{
    return isChargingState;
}

/**
 * @brief Return the current, unfiltered charger status on D8
 *
 * @return true when the active-LOW charger status pin is LOW
 */
bool Battery::isChargeSignalActive()
{
    return digitalRead(chargingPin) == chargingActiveLevel;
}
