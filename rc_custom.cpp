#include "rc_custom.h"

#include <stdbool.h>
#include <stdint.h>

#include <Arduino.h>
#include <avr/pgmspace.h>

using namespace RcCustom;

namespace
{
    // Pin configuration (set by initialize)
    uint8_t rxPin = 0;
    uint8_t rxInterrupt = 0;
    uint8_t txPin = 0;

    // AN-Motors pulse base length, microseconds
    constexpr unsigned int pulseAn = 412;
    // Maximum deviation of a received pulse from its expected length, microseconds
    constexpr unsigned int maxDelta = 200;

    // CAME pulse lengths, microseconds
    constexpr unsigned int pulseCameShort = 320;
    constexpr unsigned int pulseCameLong = 640;
    constexpr unsigned long gapCame = 11520;

    // NICE pulse lengths, microseconds
    constexpr unsigned int pulseNiceShort = 700;
    constexpr unsigned int pulseNiceLong = 1400;
    constexpr unsigned long gapNice = 25200;

    // Fixed-code bit length for CAME and NICE
    constexpr uint8_t codeLength12 = 12;
    constexpr uint32_t codeMask12 = 0xFFF;
    // Bit length of the AN-Motors fixed (identity) code part
    constexpr uint8_t codeLengthAn = 32;

    // Edge timing shared by all decoders
    volatile unsigned long prevTimeUs = 0;
    volatile unsigned int loLen = 0;
    volatile unsigned int hiLen = 0;

    // AN-Motors decoder state
    volatile uint8_t anCounter = 0;
    volatile uint32_t anCode1 = 0; // rolling part (ignored on store)
    volatile uint32_t anCode2 = 0; // fixed identity part

    // CAME decoder state
    volatile uint8_t cameCounter = 0;
    volatile uint32_t cameCode = 0;

    // NICE decoder state
    volatile uint8_t niceCounter = 0;
    volatile uint32_t niceCode = 0;

    // Decoded signal result
    volatile bool rxReceived = false;
    volatile uint32_t rxValue = 0;
    volatile uint8_t rxProtocol = 0;
    volatile uint8_t rxBitLength = 0;

    // Protocol names in flash memory
    const char nameCame[] PROGMEM = "CAME";
    const char nameNice[] PROGMEM = "NICE";
    const char nameAnMotors[] PROGMEM = "AN-Motors";

    /**
     * @brief Check a pulse length matches the expected one within tolerance
     *
     * @param base Expected pulse length
     * @param value Measured pulse length
     * @return true if value is within maxDelta of base, false otherwise
     */
    inline bool checkValue(unsigned int base, unsigned int value)
    {
        return (value == base) ||
               ((value > base) && ((value - base) < maxDelta)) ||
               ((value < base) && ((base - value) < maxDelta));
    }

    /**
     * @brief Store first decoded signal until it is read out
     *
     * @param value Decoded value
     * @param protocol Decoded protocol identifier
     * @param bitLength Decoded bit length
     */
    inline void storeResult(uint32_t value, uint8_t protocol, uint8_t bitLength)
    {
        if (rxReceived == false && value != 0)
        {
            rxValue = value;
            rxProtocol = protocol;
            rxBitLength = bitLength;
            rxReceived = true;
        }
    }

    /**
     * @brief Reset all decoder accumulators
     */
    inline void resetDecoders()
    {
        anCounter = 0;
        anCode1 = 0;
        anCode2 = 0;
        cameCounter = 0;
        cameCode = 0;
        niceCounter = 0;
        niceCode = 0;
    }

    /**
     * @brief Capture interrupt handler feeding all custom decoders
     *
     * Measures high/low pulse durations on each edge and runs the AN-Motors,
     * CAME and NICE state machines, storing the first complete frame decoded.
     */
    void captureIsr()
    {
        uint8_t state = digitalRead(rxPin);
        unsigned long now = micros();
        if (state == HIGH)
        {
            loLen = now - prevTimeUs;
        }
        else
        {
            hiLen = now - prevTimeUs;
        }
        prevTimeUs = now;

        // AN-Motors: evaluated on rising edge, 32-bit rolling + 32-bit fixed code
        if (state == HIGH)
        {
            if (checkValue(pulseAn, hiLen) && checkValue(pulseAn * 2, loLen)) // valid 1
            {
                if (anCounter < 32)
                {
                    anCode1 = (anCode1 << 1) | 1;
                }
                else if (anCounter < 64)
                {
                    anCode2 = (anCode2 << 1) | 1;
                }
                anCounter++;
            }
            else if (checkValue(pulseAn * 2, hiLen) && checkValue(pulseAn, loLen)) // valid 0
            {
                if (anCounter < 32)
                {
                    anCode1 = (anCode1 << 1);
                }
                else if (anCounter < 64)
                {
                    anCode2 = (anCode2 << 1);
                }
                anCounter++;
            }
            else
            {
                anCounter = 0;
                anCode1 = 0;
                anCode2 = 0;
            }

            if (anCounter >= 65 && anCode2 != 0xFFFFFFFFUL)
            {
                storeResult(anCode2, AnMotors, codeLengthAn);
                anCounter = 0;
                anCode1 = 0;
                anCode2 = 0;
            }
        }

        // CAME: 12-bit code, bits accumulated on falling edges
        if (state == LOW)
        {
            if (checkValue(pulseCameShort, hiLen) && checkValue(pulseCameLong, loLen)) // valid 1
            {
                cameCode = (cameCode << 1) | 1;
                cameCounter++;
            }
            else if (checkValue(pulseCameLong, hiLen) && checkValue(pulseCameShort, loLen)) // valid 0
            {
                cameCode = (cameCode << 1);
                cameCounter++;
            }
            else
            {
                cameCounter = 0;
                cameCode = 0;
            }
        }
        else if (cameCounter == codeLength12 && loLen > 1000 && (cameCode & codeMask12) != codeMask12)
        {
            storeResult(cameCode & codeMask12, Came, codeLength12);
            cameCounter = 0;
            cameCode = 0;
        }

        // NICE: 12-bit code, bits accumulated on falling edges
        if (state == LOW)
        {
            if (checkValue(pulseNiceShort, hiLen) && checkValue(pulseNiceLong, loLen)) // valid 1
            {
                niceCode = (niceCode << 1) | 1;
                niceCounter++;
            }
            else if (checkValue(pulseNiceLong, hiLen) && checkValue(pulseNiceShort, loLen)) // valid 0
            {
                niceCode = (niceCode << 1);
                niceCounter++;
            }
            else
            {
                niceCounter = 0;
                niceCode = 0;
            }
        }
        else if (niceCounter == codeLength12 && loLen > 2000 && (niceCode & codeMask12) != codeMask12)
        {
            storeResult(niceCode & codeMask12, Nice, codeLength12);
            niceCounter = 0;
            niceCode = 0;
        }
    }

    /**
     * @brief Delay for a duration that may exceed delayMicroseconds range
     *
     * @param us Delay duration, microseconds
     */
    void gapDelay(unsigned long us)
    {
        while (us > 16000UL)
        {
            delayMicroseconds(16000);
            us -= 16000UL;
        }
        delayMicroseconds((unsigned int)us);
    }

    /**
     * @brief Transmit a single AN-Motors bit
     *
     * @param bit Bit value to transmit
     */
    void sendBitAn(uint8_t bit)
    {
        if (bit == 0)
        {
            digitalWrite(txPin, HIGH);
            delayMicroseconds(pulseAn * 2);
            digitalWrite(txPin, LOW);
            delayMicroseconds(pulseAn);
        }
        else
        {
            digitalWrite(txPin, HIGH);
            delayMicroseconds(pulseAn);
            digitalWrite(txPin, LOW);
            delayMicroseconds(pulseAn * 2);
        }
    }

    /**
     * @brief Transmit an AN-Motors signal
     *
     * @param code2 Fixed (identity) code part; the rolling part is generated
     */
    void sendAnMotors(uint32_t code2)
    {
        uint32_t code1 = 0x25250000UL + (uint32_t)random(0xFFFF);
        for (uint8_t j = 0; j < 4; j++)
        {
            // Preamble of 12 short pulses
            for (uint8_t i = 0; i < 12; i++)
            {
                delayMicroseconds(pulseAn);
                digitalWrite(txPin, HIGH);
                delayMicroseconds(pulseAn);
                digitalWrite(txPin, LOW);
            }
            gapDelay((unsigned long)pulseAn * 10);

            // Rolling part
            for (int i = 32; i > 0; i--)
            {
                sendBitAn(bitRead(code1, i - 1));
            }
            // Fixed part
            for (int i = 32; i > 0; i--)
            {
                sendBitAn(bitRead(code2, i - 1));
            }
            // Battery and repeat flags
            sendBitAn(1);
            sendBitAn(1);
            gapDelay((unsigned long)pulseAn * 39);
        }
    }

    /**
     * @brief Transmit a CAME signal
     *
     * @param code 12-bit code to transmit
     */
    void sendCame(uint32_t code)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            digitalWrite(txPin, HIGH);
            delayMicroseconds(pulseCameShort);
            digitalWrite(txPin, LOW);
            for (int i = codeLength12; i > 0; i--)
            {
                if (bitRead(code, i - 1))
                {
                    digitalWrite(txPin, LOW);
                    delayMicroseconds(pulseCameLong);
                    digitalWrite(txPin, HIGH);
                    delayMicroseconds(pulseCameShort);
                }
                else
                {
                    digitalWrite(txPin, LOW);
                    delayMicroseconds(pulseCameShort);
                    digitalWrite(txPin, HIGH);
                    delayMicroseconds(pulseCameLong);
                }
            }
            digitalWrite(txPin, LOW);
            gapDelay(gapCame);
        }
    }

    /**
     * @brief Transmit a NICE signal
     *
     * @param code 12-bit code to transmit
     */
    void sendNice(uint32_t code)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            digitalWrite(txPin, HIGH);
            delayMicroseconds(pulseNiceShort);
            digitalWrite(txPin, LOW);
            for (int i = codeLength12; i > 0; i--)
            {
                if (bitRead(code, i - 1))
                {
                    digitalWrite(txPin, LOW);
                    delayMicroseconds(pulseNiceLong);
                    digitalWrite(txPin, HIGH);
                    delayMicroseconds(pulseNiceShort);
                }
                else
                {
                    digitalWrite(txPin, LOW);
                    delayMicroseconds(pulseNiceShort);
                    digitalWrite(txPin, HIGH);
                    delayMicroseconds(pulseNiceLong);
                }
            }
            digitalWrite(txPin, LOW);
            gapDelay(gapNice);
        }
    }
} // namespace

/**
 * @brief Initialize custom radio backend
 *
 * @param rxPinArg Receiver input pin (read inside the capture interrupt)
 * @param rxInterruptArg Receiver interrupt number for the RX pin
 * @param txPinArg Transmitter output pin
 */
void RcCustom::initialize(uint8_t rxPinArg, uint8_t rxInterruptArg, uint8_t txPinArg)
{
    rxPin = rxPinArg;
    rxInterrupt = rxInterruptArg;
    txPin = txPinArg;

    // Vary the AN-Motors rolling part between transmissions
    randomSeed(micros());
}

/**
 * @brief Check protocol is a custom (non rc-switch) one
 *
 * @param protocol Protocol identifier
 * @return true if protocol is handled by this module, false otherwise
 */
bool RcCustom::isSupported(uint8_t protocol)
{
    return (protocol == Came || protocol == Nice || protocol == AnMotors);
}

/**
 * @brief Return protocol name located in flash memory
 *
 * @param protocol Protocol identifier
 * @return Pointer to name in flash memory, or nullptr if protocol is not custom
 */
const char *RcCustom::getName_P(uint8_t protocol)
{
    switch (protocol)
    {
    case Came:
        return nameCame;
    case Nice:
        return nameNice;
    case AnMotors:
        return nameAnMotors;
    default:
        return nullptr;
    }
}

/**
 * @brief Enable capture interrupt handling
 */
void RcCustom::enableReceive()
{
    resetDecoders();
    rxReceived = false;
    prevTimeUs = micros();
    attachInterrupt(rxInterrupt, captureIsr, CHANGE);
}

/**
 * @brief Disable capture interrupt handling
 */
void RcCustom::disableReceive()
{
    detachInterrupt(rxInterrupt);
}

/**
 * @brief Check a signal has been decoded
 *
 * @return true if a signal is available, false otherwise
 */
bool RcCustom::available()
{
    return rxReceived;
}

/**
 * @brief Reset decoded signal availability
 */
void RcCustom::resetAvailable()
{
    rxReceived = false;
}

/**
 * @brief Copy decoded signal (valid only when available() is true)
 *
 * @param value Decoded signal value
 * @param protocol Decoded signal protocol identifier
 * @param bitLength Decoded signal bit length
 */
void RcCustom::getReceived(uint32_t &value, uint8_t &protocol, uint8_t &bitLength)
{
    value = rxValue;
    protocol = rxProtocol;
    bitLength = rxBitLength;
}

/**
 * @brief Transmit a custom protocol signal
 *
 * @param value Signal value to transmit
 * @param protocol Protocol identifier (must be a custom one)
 * @param bitLength Signal bit length (unused for fixed-length protocols)
 */
void RcCustom::send(uint32_t value, uint8_t protocol, uint8_t /* bitLength */)
{
    switch (protocol)
    {
    case Came:
        sendCame(value);
        break;
    case Nice:
        sendNice(value);
        break;
    case AnMotors:
        sendAnMotors(value);
        break;
    default:
        break;
    }
}
