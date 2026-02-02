#ifndef LORA32U4II_CONFIG_H
#define LORA32U4II_CONFIG_H

// LoRa32u4II Platform Configuration
// Platform-specific settings for LoRa32u4II (ATmega32u4)

// SPI Settings - ATmega32u4 at 8MHz needs slower SPI
#define SPI_FREQ 1000000   // 1 MHz SPI clock (matches reference implementation for LoRa32u4 II)

// Serial Configuration
#define SERIAL_BAUD 115200

// LED pin
#define LED_PIN 13

// Radio pin definitions for SX1276 (LoRa32u4II uses SX1276)
// Per BSFrance LoRa32u4 II board specification:
// NSS = pin 8, RST = pin 4, DIO0 = pin 7, DIO1 = pin 5
#define RADIO_NSS_PIN    8     // SPI Slave Select (NSS)
#define RADIO_RESET_PIN  4     // Reset pin
#define RADIO_DIO0_PIN   7     // DIO0 interrupt pin
#define RADIO_DIO1_PIN   5     // DIO1 interrupt pin (must be manually wired for versions < 1.3)

#endif // LORA32U4II_CONFIG_H
