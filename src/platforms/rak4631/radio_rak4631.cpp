/**
 * Radio Implementation for RAK4631 Platform
 * 
 * This file implements the radio interface for RAK4631 by delegating to
 * the SX1262 RadioLib implementation.
 * 
 * This file is only compiled for RAK4631 builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../../radio/radio_interface.h"
#include "../../radio/sx1262_radiolib/sx1262_radiolib.h"

// Implement radio interface by delegating to SX1262 RadioLib implementation
bool radio_init() { 
    return sx1262_radiolib_init(); 
}

uint32_t radio_getMinFrequency() {
    return sx1262_radiolib_getMinFrequency();
}

uint32_t radio_getMaxFrequency() {
    return sx1262_radiolib_getMaxFrequency();
}

void radio_setFrequency(uint32_t freq_hz) { 
    sx1262_radiolib_setFrequency(freq_hz); 
}

void radio_setPower(uint8_t power) { 
    sx1262_radiolib_setPower(power); 
}

void radio_setPreambleLength(uint16_t length) { 
    sx1262_radiolib_setPreambleLength(length); 
}

void radio_setCrc(bool enable) { 
    sx1262_radiolib_setCrc(enable); 
}

void radio_setSyncWord(uint8_t syncWord) { 
    sx1262_radiolib_setSyncWord(syncWord); 
}

void radio_setHeaderMode(bool implicit) { 
    sx1262_radiolib_setHeaderMode(implicit); 
}

void radio_setBandwidth(uint8_t bw) { 
    sx1262_radiolib_setBandwidth(bw); 
}

void radio_setSpreadingFactor(uint8_t sf) { 
    sx1262_radiolib_setSpreadingFactor(sf); 
}

void radio_setCodingRate(uint8_t cr) { 
    sx1262_radiolib_setCodingRate(cr); 
}

void radio_setInvertIQ(bool invert) { 
    sx1262_radiolib_setInvertIQ(invert); 
}

void radio_setMode(uint8_t mode) { 
    sx1262_radiolib_setMode(mode); 
}

void radio_writeFifo(uint8_t* data, uint8_t len) { 
    sx1262_radiolib_writeFifo(data, len); 
}

void radio_readFifo(uint8_t* data, uint8_t len) { 
    sx1262_radiolib_readFifo(data, len); 
}

int16_t radio_getRssi() { 
    return sx1262_radiolib_getRssi(); 
}

int8_t radio_getSnr() { 
    return sx1262_radiolib_getSnr(); 
}

uint8_t radio_readRegister(uint8_t reg) { 
    return sx1262_radiolib_readRegister(reg); 
}

void radio_writeRegister(uint8_t reg, uint8_t value) { 
    sx1262_radiolib_writeRegister(reg, value); 
}

void radio_attachInterrupt(void (*handler)()) { 
    sx1262_radiolib_attachInterrupt(handler); 
}

bool radio_isPacketReceived() { 
    return sx1262_radiolib_isPacketReceived(); 
}

uint8_t radio_getPacketLength() { 
    return sx1262_radiolib_getPacketLength(); 
}

void radio_clearIrqFlags() { 
    sx1262_radiolib_clearIrqFlags(); 
}

uint16_t radio_getIrqFlags() { 
    return sx1262_radiolib_getIrqFlags(); 
}

bool radio_hasPacketErrors() { 
    return sx1262_radiolib_hasPacketErrors(); 
}
