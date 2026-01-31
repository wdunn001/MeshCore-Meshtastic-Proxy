#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Radio Interface
 * 
 * This is the common API that all radio implementations must support.
 * Main application code uses these functions, and platform-specific
 * implementations provide the actual functionality.
 * 
 * Each platform (SX1276, SX1262, etc.) implements these functions
 * according to their hardware capabilities.
 */

// ============================================================================
// Common Constants (shared across all platforms)
// ============================================================================

// Operating Modes (common across all platforms)
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

// Note: Platform-specific mode constants (like MODE_LONG_RANGE_MODE for SX1276)
// are defined in their respective radio implementation headers

// ============================================================================
// Radio Interface API
// ============================================================================
// All platforms must implement these functions. The interface layer routes
// calls to the appropriate platform-specific implementation.

/**
 * Initialize the radio hardware
 * @return true if initialization successful, false otherwise
 */
bool radio_init();

/**
 * Get minimum supported frequency
 * @return Minimum frequency in Hz
 */
uint32_t radio_getMinFrequency();

/**
 * Get maximum supported frequency
 * @return Maximum frequency in Hz
 */
uint32_t radio_getMaxFrequency();

/**
 * Configure radio frequency
 * @param freq_hz Frequency in Hz
 */
void radio_setFrequency(uint32_t freq_hz);

/**
 * Set transmit power
 * @param power Power level (platform-specific range, typically 0-22 dBm)
 */
void radio_setPower(uint8_t power);

/**
 * Set preamble length
 * @param length Preamble length in symbols
 */
void radio_setPreambleLength(uint16_t length);

/**
 * Enable or disable CRC
 * @param enable true to enable CRC, false to disable
 */
void radio_setCrc(bool enable);

/**
 * Set sync word
 * @param syncWord Sync word value (8-bit)
 */
void radio_setSyncWord(uint8_t syncWord);

/**
 * Set header mode
 * @param implicit true for implicit header mode, false for explicit
 */
void radio_setHeaderMode(bool implicit);

/**
 * Set bandwidth
 * @param bw Bandwidth code (platform-specific encoding)
 */
void radio_setBandwidth(uint8_t bw);

/**
 * Set spreading factor
 * @param sf Spreading factor (typically 6-12)
 */
void radio_setSpreadingFactor(uint8_t sf);

/**
 * Set coding rate
 * @param cr Coding rate (typically 5-8)
 */
void radio_setCodingRate(uint8_t cr);

/**
 * Set IQ inversion
 * @param invert true to invert IQ, false for normal
 */
void radio_setInvertIQ(bool invert);

/**
 * Set operating mode
 * @param mode One of MODE_SLEEP, MODE_STDBY, MODE_TX, MODE_RX_CONTINUOUS
 */
void radio_setMode(uint8_t mode);

/**
 * Write data to FIFO for transmission
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 */
void radio_writeFifo(uint8_t* data, uint8_t len);

/**
 * Read data from FIFO after reception
 * @param data Pointer to buffer to store received data
 * @param len Maximum number of bytes to read
 */
void radio_readFifo(uint8_t* data, uint8_t len);

/**
 * Get received signal strength indicator
 * @return RSSI in dBm
 */
int16_t radio_getRssi();

/**
 * Get signal-to-noise ratio
 * @return SNR in dB
 */
int8_t radio_getSnr();

/**
 * Read a register value (platform-specific, may not be meaningful for all platforms)
 * @param reg Register address
 * @return Register value
 */
uint8_t radio_readRegister(uint8_t reg);

/**
 * Write a register value (platform-specific, may not be meaningful for all platforms)
 * @param reg Register address
 * @param value Value to write
 */
void radio_writeRegister(uint8_t reg, uint8_t value);

/**
 * Attach interrupt handler for packet events
 * @param handler Function pointer to interrupt handler
 */
void radio_attachInterrupt(void (*handler)());

/**
 * Check if a packet has been received
 * @return true if packet received, false otherwise
 */
bool radio_isPacketReceived();

/**
 * Get length of received packet
 * @return Packet length in bytes
 */
uint8_t radio_getPacketLength();

/**
 * Clear interrupt flags
 */
void radio_clearIrqFlags();

// ============================================================================
// Compatibility Macros (for legacy code using sx1276_* naming)
// ============================================================================
// These map old function names to the new radio_* interface
// This allows existing code to work without modification

#define sx1276_init radio_init
#define sx1276_setFrequency radio_setFrequency
#define sx1276_setPower radio_setPower
#define sx1276_setPreambleLength radio_setPreambleLength
#define sx1276_setCrc radio_setCrc
#define sx1276_setSyncWord radio_setSyncWord
#define sx1276_setHeaderMode radio_setHeaderMode
#define sx1276_setBandwidth radio_setBandwidth
#define sx1276_setSpreadingFactor radio_setSpreadingFactor
#define sx1276_setCodingRate radio_setCodingRate
#define sx1276_setInvertIQ radio_setInvertIQ
#define sx1276_setMode radio_setMode
#define sx1276_writeFifo radio_writeFifo
#define sx1276_readFifo radio_readFifo
#define sx1276_getRssi radio_getRssi
#define sx1276_getSnr radio_getSnr
#define sx1276_readRegister radio_readRegister
#define sx1276_writeRegister radio_writeRegister
#define sx1276_attachInterrupt radio_attachInterrupt
#define sx1276_isPacketReceived radio_isPacketReceived
#define sx1276_getPacketLength radio_getPacketLength
#define sx1276_clearIrqFlags radio_clearIrqFlags

// ============================================================================
// Platform-specific Constants
// ============================================================================
// Register definitions and platform-specific mode constants are defined in
// their respective radio implementation headers:
//   - SX1276: src/radio/sx1276_direct/sx1276_direct.h
//   - SX1262: src/radio/sx1262_direct/sx1262_direct.h
// 
// Include the appropriate radio header if you need platform-specific constants.

#endif // RADIO_INTERFACE_H
