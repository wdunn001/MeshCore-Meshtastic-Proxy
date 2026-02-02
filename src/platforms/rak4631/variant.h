#ifndef VARIANT_H
#define VARIANT_H

// RAK4631 variant pin definitions
// RAK4631 is based on nRF52840 with 48 pins
// Reference: https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631/Datasheet/

// Clock source configuration - RAK4631 uses external 32kHz crystal (LFXO)
#define USE_LFXO

// Pin count - nRF52840 has 48 pins
#define PINS_COUNT 48
#define NUM_DIGITAL_PINS 48
#define NUM_ANALOG_INPUTS 6

// Serial pins
#define PIN_SERIAL1_RX 15
#define PIN_SERIAL1_TX 16

// LED configuration - RAK4631 LEDs are active HIGH
#define LED_STATE_ON 1

// ============================================================================
// SX1262 LoRa module pins (from Meshtastic variant.h)
// ============================================================================
// P1.10   NSS     SPI NSS (Arduino GPIO number 42)
// P1.11   SCK     SPI CLK (Arduino GPIO number 43)
// P1.12   MOSI    SPI MOSI (Arduino GPIO number 44)
// P1.13   MISO    SPI MISO (Arduino GPIO number 45)
// P1.14   BUSY    BUSY signal (Arduino GPIO number 46)
// P1.15   DIO1    DIO1 event interrupt (Arduino GPIO number 47)
// P1.06   NRESET  NRESET manual reset of the SX1262 (Arduino GPIO number 38)
// ============================================================================

#define P_LORA_NSS 42
#define P_LORA_DIO_1 47
#define P_LORA_RESET 38       // RAK4631 HAS a reset pin!
#define P_LORA_BUSY 46
#define SX126X_POWER_EN 37

// SPI pins for LoRa module (different from default SPI!)
#define PIN_LORA_MISO 45
#define PIN_LORA_MOSI 44
#define PIN_LORA_SCK 43

// CRITICAL: TCXO voltage for SX1262 on RAK4631
// DIO3 controls the TCXO power supply at 1.8V
#define SX126X_DIO3_TCXO_VOLTAGE 1.8f

// DIO2 controls the antenna switch
#define SX126X_DIO2_AS_RF_SWITCH 1

// LED pins
#define PIN_LED1 35
#define PIN_LED2 36
#define LED_BUILTIN PIN_LED1

#endif // VARIANT_H
