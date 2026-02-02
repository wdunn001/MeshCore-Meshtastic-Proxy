/*
 * platform.cpp
 * Platform-specific definitions for RAK4631 (nRF52840)
 * This file provides SPI and pin mapping definitions that nRF52 needs
 * Only compiled for RAK4631 builds
 */

#ifdef RAK4631_BOARD

#include <Arduino.h>
#include <SPI.h>
#include "variant.h"

// Pin mapping array: maps Arduino pin number to nRF52840 GPIO pin
// nRF52840 has 48 GPIO pins total
// Format: direct GPIO pin numbers (0-47)
const uint32_t g_ADigitalPinMap[] = {
  // P0.00 - P0.31 (pins 0-31)
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  // P1.00 - P1.15 (pins 32-47)
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
};

// SPI object definition for nRF52 - using LoRa SPI pins!
// SPIClass constructor: SPIClass(NRF_SPIM_Type *p_spi, uint8_t uc_pinMISO, uint8_t uc_pinSCK, uint8_t uc_pinMOSI)
// RAK4631 LoRa SPI pins: MISO=45, SCK=43, MOSI=44 (from Meshtastic variant.h)
// Using NRF_SPIM0 as the SPI peripheral
SPIClass SPI(NRF_SPIM0, PIN_LORA_MISO, PIN_LORA_SCK, PIN_LORA_MOSI);

#endif // RAK4631_BOARD
