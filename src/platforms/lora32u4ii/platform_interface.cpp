/**
 * Platform Interface Implementation for LoRa32u4II
 * 
 * This file is only compiled for LoRa32u4II builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../platform_interface.h"
#include "config.h"
#include "../../radio/sx1276_direct/config.h"  // Radio pin definitions
#include <Arduino.h>

void platform_init() {
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Platform-specific initialization for LoRa32u4II
    // (e.g., USB, etc. if needed)
}

void platform_setLed(bool on) {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void platform_blinkLed(uint16_t durationMs) {
    platform_setLed(true);
    delay(durationMs);
    platform_setLed(false);
}

uint8_t platform_getMaxTxPower() {
    return 17;  // LoRa32u4II max is 17 dBm
}

void platform_delayUs(uint16_t microseconds) {
    delayMicroseconds(microseconds);
}

uint32_t platform_getSerialBaud() {
    return SERIAL_BAUD;
}

// Radio pin configuration for SX1276 (LoRa32u4II uses SX1276)
int8_t platform_getRadioNssPin() {
    return SX1276_SS;  // Pin 8
}

int8_t platform_getRadioResetPin() {
    return SX1276_RESET;  // Pin 4
}

int8_t platform_getRadioDio0Pin() {
    return SX1276_DIO0;  // Pin 7
}

int8_t platform_getRadioDio1Pin() {
    return SX1276_DIO1;  // Pin 5
}

int8_t platform_getRadioBusyPin() {
    return -1;  // SX1276 doesn't have a BUSY pin
}

int8_t platform_getRadioPowerEnablePin() {
    return -1;  // SX1276 doesn't have a power enable pin
}

uint32_t platform_getSpiFrequency() {
    return SPI_FREQ;  // 1 MHz for LoRa32u4II
}
