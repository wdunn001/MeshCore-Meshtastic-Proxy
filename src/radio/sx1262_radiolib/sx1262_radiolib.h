#ifndef SX1262_RADIOLIB_H
#define SX1262_RADIOLIB_H

#include <stdint.h>
#include <stdbool.h>

// Frequency Range Constants (SX1262: 150-960 MHz)
#define SX1262_MIN_FREQUENCY_HZ  150000000UL
#define SX1262_MAX_FREQUENCY_HZ  960000000UL

// Function Prototypes (matches sx1262_direct interface)
bool sx1262_radiolib_init();
uint32_t sx1262_radiolib_getMinFrequency();
uint32_t sx1262_radiolib_getMaxFrequency();
void sx1262_radiolib_setFrequency(uint32_t freq_hz);
void sx1262_radiolib_setPower(uint8_t power);
void sx1262_radiolib_setPreambleLength(uint16_t length);
void sx1262_radiolib_setCrc(bool enable);
void sx1262_radiolib_setSyncWord(uint8_t syncWord);
void sx1262_radiolib_setHeaderMode(bool implicit);
void sx1262_radiolib_setBandwidth(uint8_t bw);
void sx1262_radiolib_setSpreadingFactor(uint8_t sf);
void sx1262_radiolib_setCodingRate(uint8_t cr);
void sx1262_radiolib_setInvertIQ(bool invert);
void sx1262_radiolib_setMode(uint8_t mode);
void sx1262_radiolib_writeFifo(uint8_t* data, uint8_t len);
void sx1262_radiolib_readFifo(uint8_t* data, uint8_t len);
int16_t sx1262_radiolib_getRssi();
int8_t sx1262_radiolib_getSnr();
uint8_t sx1262_radiolib_readRegister(uint8_t reg);
void sx1262_radiolib_writeRegister(uint8_t reg, uint8_t value);
void sx1262_radiolib_attachInterrupt(void (*handler)());
bool sx1262_radiolib_isPacketReceived();
uint8_t sx1262_radiolib_getPacketLength();
void sx1262_radiolib_clearIrqFlags();
uint16_t sx1262_radiolib_getIrqFlags();
bool sx1262_radiolib_hasPacketErrors();

#endif // SX1262_RADIOLIB_H
