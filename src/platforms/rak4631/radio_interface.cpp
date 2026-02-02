/**
 * Radio Interface Implementation for RAK4631 Platform
 * 
 * This file implements the radio interface for RAK4631 by delegating to
 * the SX1262 direct SPI implementation.
 * 
 * This file is only compiled for RAK4631 builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../../radio/radio_interface.h"
#include "../../radio/sx1262_direct/sx1262_direct.h"

// Implement radio interface by delegating to SX1262 direct implementation
bool radio_init() { 
    return sx1262_direct_init(); 
}

uint32_t radio_getMinFrequency() {
    return sx1262_direct_getMinFrequency();
}

uint32_t radio_getMaxFrequency() {
    return sx1262_direct_getMaxFrequency();
}

void radio_setFrequency(uint32_t freq_hz) { 
    sx1262_direct_setFrequency(freq_hz); 
}

void radio_setPower(uint8_t power) { 
    sx1262_direct_setPower(power); 
}

void radio_setPreambleLength(uint16_t length) { 
    sx1262_direct_setPreambleLength(length); 
}

void radio_setCrc(bool enable) { 
    sx1262_direct_setCrc(enable); 
}

void radio_setSyncWord(uint8_t syncWord) { 
    sx1262_direct_setSyncWord(syncWord); 
}

void radio_setHeaderMode(bool implicit) { 
    sx1262_direct_setHeaderMode(implicit); 
}

void radio_setBandwidth(uint8_t bw) { 
    sx1262_direct_setBandwidth(bw); 
}

void radio_setSpreadingFactor(uint8_t sf) { 
    sx1262_direct_setSpreadingFactor(sf); 
}

void radio_setCodingRate(uint8_t cr) { 
    sx1262_direct_setCodingRate(cr); 
}

void radio_setInvertIQ(bool invert) { 
    sx1262_direct_setInvertIQ(invert); 
}

void radio_setMode(uint8_t mode) { 
    sx1262_direct_setMode(mode); 
}

void radio_writeFifo(uint8_t* data, uint8_t len) { 
    sx1262_direct_writeFifo(data, len); 
}

void radio_readFifo(uint8_t* data, uint8_t len) { 
    sx1262_direct_readFifo(data, len); 
}

int16_t radio_getRssi() { 
    return sx1262_direct_getRssi(); 
}

int8_t radio_getSnr() { 
    return sx1262_direct_getSnr(); 
}

uint8_t radio_readRegister(uint8_t reg) { 
    return sx1262_direct_readRegister(reg); 
}

void radio_writeRegister(uint8_t reg, uint8_t value) { 
    sx1262_direct_writeRegister(reg, value); 
}

void radio_attachInterrupt(void (*handler)()) { 
    sx1262_direct_attachInterrupt(handler); 
}

bool radio_isPacketReceived() { 
    return sx1262_direct_isPacketReceived(); 
}

uint8_t radio_getPacketLength() { 
    return sx1262_direct_getPacketLength(); 
}

void radio_clearIrqFlags() { 
    sx1262_direct_clearIrqFlags(); 
}

uint16_t radio_getIrqFlags() { 
    return sx1262_direct_getIrqFlags(); 
}

bool radio_hasPacketErrors() { 
    return sx1262_direct_hasPacketErrors(); 
}
