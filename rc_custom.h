#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Decoder and transmitter for radio protocols not supported by rc-switch
 *
 * Implements its own capture interrupt handler and bit-bang transmitter for
 * fixed-code remotes (CAME, NICE, AN-Motors). Protocol identifiers continue the
 * numbering after rc-switch protocols (1..12), so a signal can be routed to the
 * proper backend by its protocol number alone.
 *
 * All three are OOK (on-off keyed) protocols where each bit is a high pulse
 * followed by a low pulse; a bit's value is encoded in the short/long ordering
 * of the two pulses. A received pulse matches an expected length when it is
 * within +/- maxDelta microseconds of it.
 *
 * Supported protocols:
 *
 * - CAME (id 64): 12-bit fixed code. Bit pulses are short = 320 us and
 *   long = 640 us; a '1' is short-high/long-low, a '0' is long-high/short-low.
 *   A frame is 12 bits terminated by an inter-frame gap of ~11520 us. The
 *   stored value is the 12-bit code (codeMask12). Transmit repeats the frame
 *   4 times.
 *
 * - NICE (id 65): 12-bit fixed code with the same short/long bit encoding as
 *   CAME but longer pulses, short = 700 us and long = 1400 us, and a larger
 *   inter-frame gap of ~25200 us. Stored value is the 12-bit code; transmit
 *   repeats the frame 4 times.
 *
 * - AN-Motors (id 66): base pulse 412 us, decoded on rising edges. A frame
 *   carries a 32-bit part followed by a 32-bit fixed identity part. Only the
 *   fixed identity part is captured and used as the stored value
 *   (bit length codeLengthAn = 32); the leading part is not retained. Transmit
 *   emits a 12-pulse preamble, the two 32-bit parts and two trailing flag bits,
 *   repeated 4 times.
 */
namespace RcCustom
{
    /**
     * @brief Custom protocol identifiers
     *
     * Values start past rc-switch protocol range (1..12) to keep them unique.
     */
    enum Protocol : uint8_t
    {
        Came = 64,
        Nice = 65,
        AnMotors = 66,
    };

    /**
     * @brief Initialize custom radio backend
     *
     * @param rxPin Receiver input pin (read inside the capture interrupt)
     * @param rxInterrupt Receiver interrupt number for the RX pin
     * @param txPin Transmitter output pin
     */
    void initialize(uint8_t rxPin, uint8_t rxInterrupt, uint8_t txPin);

    /**
     * @brief Check protocol is a custom (non rc-switch) one
     *
     * @param protocol Protocol identifier
     * @return true if protocol is handled by this module, false otherwise
     */
    bool isSupported(uint8_t protocol);

    /**
     * @brief Return protocol name located in flash memory
     *
     * @param protocol Protocol identifier
     * @return Pointer to name in flash memory, or nullptr if protocol is not custom
     */
    const char *getName_P(uint8_t protocol);

    /**
     * @brief Enable capture interrupt handling
     */
    void enableReceive();

    /**
     * @brief Disable capture interrupt handling
     */
    void disableReceive();

    /**
     * @brief Check a signal has been decoded
     *
     * @return true if a signal is available, false otherwise
     */
    bool available();

    /**
     * @brief Reset decoded signal availability
     */
    void resetAvailable();

    /**
     * @brief Copy decoded signal (valid only when available() is true)
     *
     * @param value Decoded signal value
     * @param protocol Decoded signal protocol identifier
     * @param bitLength Decoded signal bit length
     */
    void getReceived(uint32_t &value, uint8_t &protocol, uint8_t &bitLength);

    /**
     * @brief Transmit a custom protocol signal
     *
     * @param value Signal value to transmit
     * @param protocol Protocol identifier (must be a custom one)
     * @param bitLength Signal bit length (unused for fixed-length protocols)
     */
    void send(uint32_t value, uint8_t protocol, uint8_t bitLength);
} // namespace RcCustom
