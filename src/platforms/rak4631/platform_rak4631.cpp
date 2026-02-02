/**
 * Platform Implementation for RAK4631
 * 
 * This file implements platform-specific functionality for RAK4631.
 * This file is only compiled for RAK4631 builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../platform_interface.h"
#include "config.h"
#include "variant.h"  // Pin definitions
#include <Arduino.h>

void platform_init() {
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Platform-specific initialization for RAK4631
    // (e.g., USB, BLE, etc. if needed)
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
    return 22;  // RAK4631 max is 22 dBm
}

void platform_delayUs(uint16_t microseconds) {
    delayMicroseconds(microseconds);
}

uint32_t platform_getSerialBaud() {
    return SERIAL_BAUD;
}

// Radio pin configuration for SX1262 (RAK4631 uses SX1262)
int8_t platform_getRadioNssPin() {
    return P_LORA_NSS;  // Pin 42
}

int8_t platform_getRadioResetPin() {
    return P_LORA_RESET;  // Pin 38 on RAK4631
}

int8_t platform_getRadioDio0Pin() {
    return -1;  // SX1262 uses DIO1, not DIO0
}

int8_t platform_getRadioDio1Pin() {
    return P_LORA_DIO_1;  // Pin 47
}

int8_t platform_getRadioBusyPin() {
    return P_LORA_BUSY;  // Pin 46
}

int8_t platform_getRadioPowerEnablePin() {
    return SX126X_POWER_EN;  // Pin 37
}

uint32_t platform_getSpiFrequency() {
    return SPI_FREQ;  // 8 MHz for RAK4631
}

// SX126x-specific configuration for RAK4631
float platform_getTcxoVoltage() {
    return SX126X_DIO3_TCXO_VOLTAGE;  // 1.8V TCXO on RAK4631 - CRITICAL!
}

bool platform_useDio2AsRfSwitch() {
    return SX126X_DIO2_AS_RF_SWITCH;  // DIO2 controls antenna switch on RAK4631
}

bool platform_useRegulatorLDO() {
    return false;  // RAK4631 uses DC-DC regulator (more efficient)
}
