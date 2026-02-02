/**
 * Radio Interface Implementation for LoRa32u4II Platform
 * 
 * This file implements the radio interface for LoRa32u4II by delegating to
 * the SX1276 direct SPI implementation.
 * 
 * This file is only compiled for LoRa32u4II builds (excluded for other platforms
 * via platformio.ini src_filter).
 */

#include "../../radio/radio_interface.h"
#include "../../radio/sx1276_direct/sx1276_direct.h"

// Implement radio interface by delegating to SX1276 direct implementation
bool radio_init() { 
    return sx1276_direct_init(); 
}

uint32_t radio_getMinFrequency() {
    return sx1276_direct_getMinFrequency();
}

uint32_t radio_getMaxFrequency() {
    return sx1276_direct_getMaxFrequency();
}

void radio_setFrequency(uint32_t freq_hz) { 
    sx1276_direct_setFrequency(freq_hz); 
}

void radio_setPower(uint8_t power) { 
    sx1276_direct_setPower(power); 
}

void radio_setPreambleLength(uint16_t length) { 
    sx1276_direct_setPreambleLength(length); 
}

void radio_setCrc(bool enable) { 
    sx1276_direct_setCrc(enable); 
}

void radio_setSyncWord(uint8_t syncWord) { 
    sx1276_direct_setSyncWord(syncWord); 
}

void radio_setHeaderMode(bool implicit) { 
    sx1276_direct_setHeaderMode(implicit); 
}

void radio_setBandwidth(uint8_t bw) { 
    sx1276_direct_setBandwidth(bw); 
}

void radio_setSpreadingFactor(uint8_t sf) { 
    sx1276_direct_setSpreadingFactor(sf); 
}

void radio_setCodingRate(uint8_t cr) { 
    sx1276_direct_setCodingRate(cr); 
}

void radio_setInvertIQ(bool invert) { 
    sx1276_direct_setInvertIQ(invert); 
}

void radio_setMode(uint8_t mode) { 
    sx1276_direct_setMode(mode); 
}

void radio_writeFifo(uint8_t* data, uint8_t len) { 
    sx1276_direct_writeFifo(data, len); 
}

void radio_readFifo(uint8_t* data, uint8_t len) { 
    sx1276_direct_readFifo(data, len); 
}

int16_t radio_getRssi() { 
    return sx1276_direct_getRssi(); 
}

int8_t radio_getSnr() { 
    return sx1276_direct_getSnr(); 
}

uint8_t radio_readRegister(uint8_t reg) { 
    return sx1276_direct_readRegister(reg); 
}

void radio_writeRegister(uint8_t reg, uint8_t value) { 
    sx1276_direct_writeRegister(reg, value); 
}

void radio_attachInterrupt(void (*handler)()) { 
    sx1276_direct_attachInterrupt(handler); 
}

bool radio_isPacketReceived() { 
    return sx1276_direct_isPacketReceived(); 
}

uint8_t radio_getPacketLength() { 
    return sx1276_direct_getPacketLength(); 
}

void radio_clearIrqFlags() { 
    sx1276_direct_clearIrqFlags(); 
}

uint16_t radio_getIrqFlags() { 
    return sx1276_direct_getIrqFlags(); 
}

bool radio_hasPacketErrors() { 
    return sx1276_direct_hasPacketErrors(); 
}
