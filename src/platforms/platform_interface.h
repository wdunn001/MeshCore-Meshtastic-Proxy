#ifndef PLATFORM_INTERFACE_H
#define PLATFORM_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Platform Interface
 * 
 * Provides platform-specific functionality like LED control, initialization,
 * and platform-specific settings. Each platform provides its own implementation.
 */

// Platform initialization
void platform_init();

// LED control
void platform_setLed(bool on);
void platform_blinkLed(uint16_t durationMs);

// Platform-specific power management
uint8_t platform_getMaxTxPower();  // Returns max TX power in dBm for this platform

// Platform-specific delays/timing
void platform_delayUs(uint16_t microseconds);  // Platform-optimized microsecond delay

// Platform-specific serial/USB configuration
uint32_t platform_getSerialBaud();  // Returns serial baud rate for this platform

// Radio pin configuration (platform knows which pins to use for radio)
// Returns pin numbers for radio interface
int8_t platform_getRadioNssPin();           // SPI chip select (NSS) pin
int8_t platform_getRadioResetPin();        // Reset pin (-1 if not used)
int8_t platform_getRadioDio0Pin();         // DIO0 interrupt pin
int8_t platform_getRadioDio1Pin();         // DIO1 interrupt pin (-1 if not used)
int8_t platform_getRadioBusyPin();         // Busy pin for SX1262 (-1 if not used)
int8_t platform_getRadioPowerEnablePin();  // Power enable pin (-1 if not used)

// SPI configuration
uint32_t platform_getSpiFrequency();  // Returns SPI frequency in Hz for this platform

#endif // PLATFORM_INTERFACE_H
