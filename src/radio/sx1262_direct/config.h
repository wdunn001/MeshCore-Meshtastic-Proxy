#ifndef SX1262_DIRECT_CONFIG_H
#define SX1262_DIRECT_CONFIG_H

// SX1262 Direct SPI Radio Configuration
// Pin definitions for SX1262 radio chip

// RAK4631 uses SX1262 - pins are defined in platform variant.h
// Include platform variant to get pin definitions
#ifdef RAK4631_BOARD
    #include "../../platforms/rak4631/variant.h"
    
    // Use platform pin definitions
    // P_LORA_NSS, P_LORA_DIO_1, P_LORA_BUSY, SX126X_POWER_EN are defined in variant.h
#else
    // SX1262 is only supported on RAK4631 platform
    // This file should not be included for other platforms
    // (platform-specific code should use src_filter to exclude sx1262_direct/*)
#endif

#endif // SX1262_DIRECT_CONFIG_H
