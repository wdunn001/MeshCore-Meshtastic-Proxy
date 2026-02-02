/**
 * Platform Implementation for LoRa32u4II
 * 
 * This file implements platform-specific functionality for LoRa32u4II.
 * This file is only compiled for LoRa32u4II builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../platform_interface.h"
#include "config.h"
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
    return RADIO_NSS_PIN;
}

int8_t platform_getRadioResetPin() {
    return RADIO_RESET_PIN;
}

int8_t platform_getRadioDio0Pin() {
    return RADIO_DIO0_PIN;
}

int8_t platform_getRadioDio1Pin() {
    return RADIO_DIO1_PIN;
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

// SX126x-specific configuration - not used by SX1276
float platform_getTcxoVoltage() {
    return 0.0f;  // SX1276 doesn't use TCXO voltage setting
}

bool platform_useDio2AsRfSwitch() {
    return false;  // SX1276 doesn't use DIO2 as RF switch
}

bool platform_useRegulatorLDO() {
    return false;  // Not applicable for SX1276
}
