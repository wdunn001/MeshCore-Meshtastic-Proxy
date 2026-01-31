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

#endif // LORA32U4II_CONFIG_H
