#ifndef VARIANT_H
#define VARIANT_H

// RAK4631 variant pin definitions
// RAK4631 is based on nRF52840 with 48 pins

// Clock source configuration - RAK4631 uses external 32kHz crystal (LFXO)
// This is required for proper BLE and timing functionality
#define USE_LFXO

// Pin count - nRF52840 has 48 pins
#define PINS_COUNT 48
#define NUM_DIGITAL_PINS 48
#define NUM_ANALOG_INPUTS 6

// Serial pins - Serial1 uses hardware UART pins
// On nRF52840 Feather/RAK4631, Serial1 typically uses pins 0 and 1
#define PIN_SERIAL1_RX 0
#define PIN_SERIAL1_TX 1

// LED configuration
// LED_STATE_ON defines the logic level for LED on state (0 = LOW, 1 = HIGH)
// RAK4631 LEDs are typically active LOW
#define LED_STATE_ON 0

// LoRa module pins
#define P_LORA_NSS 42
#define P_LORA_DIO_1 47
#define P_LORA_RESET -1  // No reset pin, handled internally
#define P_LORA_BUSY 46
#define SX126X_POWER_EN 37

// LED pins
#define PIN_LED1 35
#define PIN_LED2 36
#define LED_BUILTIN PIN_LED1

#endif // VARIANT_H
