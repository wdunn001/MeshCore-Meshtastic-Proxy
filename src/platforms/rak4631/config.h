#ifndef RAK4631_CONFIG_H
#define RAK4631_CONFIG_H

// RAK4631 Platform Configuration
// Platform-specific settings for RAK4631 (nRF52840)

#include "variant.h"  // Pin definitions

// SPI Settings - RAK4631 can handle higher SPI speeds
#define SPI_FREQ 8000000   // 8 MHz SPI clock (SX1262 supports up to 8 MHz)

// Serial Configuration
#define SERIAL_BAUD 115200

// LED pin (from variant.h)
#define LED_PIN PIN_LED1

#endif // RAK4631_CONFIG_H
