#ifndef SX1276_DIRECT_CONFIG_H
#define SX1276_DIRECT_CONFIG_H

// SX1276 Direct SPI Radio Configuration
// Pin definitions for SX1276 radio chip

// ATmega32u4 Pin Definitions (LoRa32u4 II)
// Correct pins per BSFrance LoRa32u4 II board specification:
// NSS = pin 8, RST = pin 4, DIO0 = pin 7, DIO1 = pin 5
#define SX1276_SS    8     // SPI Slave Select (NSS)
#define SX1276_RESET 4     // Reset pin
#define SX1276_RST   4     // Reset pin (alias for compatibility)
#define SX1276_DIO0  7     // DIO0 interrupt pin
#define SX1276_DIO1  5     // DIO1 interrupt pin (must be manually wired for versions < 1.3)

#endif // SX1276_DIRECT_CONFIG_H
