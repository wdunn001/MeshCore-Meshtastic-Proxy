#include "sx1276.h"
#include "config.h"
#include <SPI.h>
#include <avr/interrupt.h>
#include <Arduino.h>

// SX1276 SPI Communication (internal helpers)
static uint8_t sx1276_readReg(uint8_t reg) {
    SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(SX1276_SS, LOW);
    SPI.transfer(reg & 0x7F); // Read: bit 7 = 0
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(SX1276_SS, HIGH);
    SPI.endTransaction();
    return value;
}

static void sx1276_writeReg(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(SX1276_SS, LOW);
    SPI.transfer(reg | 0x80); // Write: bit 7 = 1
    SPI.transfer(value);
    digitalWrite(SX1276_SS, HIGH);
    SPI.endTransaction();
}

bool sx1276_init() {
    // Initialize SPI pins
    pinMode(SX1276_SS, OUTPUT);
    pinMode(SX1276_RESET, OUTPUT);
    pinMode(SX1276_DIO0, INPUT);
    
    digitalWrite(SX1276_SS, HIGH);
    
    // Initialize SPI
    SPI.begin();
    
    // Reset SX1276
    digitalWrite(SX1276_RESET, LOW);
    delay(10);
    digitalWrite(SX1276_RESET, HIGH);
    delay(10);
    
    // Check version register (SX1276 returns 0x12, SX1272 returns 0x11)
    uint8_t version = sx1276_readReg(REG_VERSION);
    if (version != 0x12 && version != 0x11) {
        return false; // Wrong chip or not responding
    }
    
    // Put in sleep mode
    sx1276_writeReg(REG_OP_MODE, MODE_SLEEP | MODE_LONG_RANGE_MODE);
    delay(10);
    
    // Set to standby
    sx1276_writeReg(REG_OP_MODE, MODE_STDBY | MODE_LONG_RANGE_MODE);
    delay(10);
    
    // Configure default parameters
    sx1276_setFrequency(DEFAULT_FREQUENCY_HZ);
    sx1276_setBandwidth(MESHCORE_BW);  // 125kHz
    sx1276_setSpreadingFactor(MESHCORE_SF); // SF7
    sx1276_setCodingRate(MESHCORE_CR); // CR 5
    sx1276_setPower(17); // Max power
    sx1276_setPreambleLength(MESHCORE_PREAMBLE);
    sx1276_setCrc(true);
    
    // Use implicit header mode and sync word 0x12 (MeshCore default)
    sx1276_setHeaderMode(true); // Implicit header mode
    sx1276_setSyncWord(MESHCORE_SYNC_WORD);
    
    // Configure FIFO addresses
    sx1276_writeReg(REG_FIFO_TX_BASE_ADDR, 0x00);
    sx1276_writeReg(REG_FIFO_RX_BASE_ADDR, 0x00);
    
    // Configure DIO0 for TX_DONE and RX_DONE
    // DIO0: 00 = TxDone, 01 = RxDone (default mapping)
    sx1276_writeReg(REG_DIO_MAPPING_1, 0x00);
    
    return true;
}

void sx1276_setFrequency(uint32_t freq_hz) {
    // FREQ_STEP = 32 MHz / 2^19 = 61.03515625 Hz per step
    // regFreq = freq_hz / FREQ_STEP
    const double FREQ_STEP = 61.03515625;
    uint32_t regFreq = (uint32_t)((double)freq_hz / FREQ_STEP);
    
    // Set STANDBY mode before changing frequency (required by SX1276)
    uint8_t currentMode = sx1276_readReg(REG_OP_MODE);
    if ((currentMode & 0x07) != MODE_STDBY) {
        sx1276_writeReg(REG_OP_MODE, (currentMode & 0xF8) | MODE_STDBY | MODE_LONG_RANGE_MODE);
        delay(1); // Small delay for mode change
    }
    
    // Write frequency registers (MSB, MID, LSB)
    uint8_t FRQ_MSB = (uint8_t)((regFreq >> 16) & 0xFF);
    uint8_t FRQ_MID = (uint8_t)((regFreq >> 8) & 0xFF);
    uint8_t FRQ_LSB = (uint8_t)(regFreq & 0xFF);
    
    sx1276_writeReg(REG_FRF_MSB, FRQ_MSB);
    sx1276_writeReg(REG_FRF_MID, FRQ_MID);
    sx1276_writeReg(REG_FRF_LSB, FRQ_LSB);
}

void sx1276_setPower(uint8_t power) {
    if (power > 20) power = 20;
    
    uint8_t paConfig = 0x80; // PA_BOOST
    if (power > 17) {
        paConfig |= 0x80 | (power - 5);
    } else if (power > 14) {
        paConfig |= 0x40 | (power - 2);
    } else {
        paConfig |= power;
    }
    sx1276_writeReg(REG_PA_CONFIG, paConfig);
}

void sx1276_setBandwidth(uint8_t bw) {
    // BW: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz, 5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0x0F) | (bw << 4);
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

void sx1276_setSpreadingFactor(uint8_t sf) {
    // SF: 6-12
    if (sf < 6) sf = 6;
    if (sf > 12) sf = 12;
    
    uint8_t config2 = sx1276_readReg(REG_MODEM_CONFIG_2);
    config2 = (config2 & 0x0F) | (sf << 4);
    sx1276_writeReg(REG_MODEM_CONFIG_2, config2);
    
    // Enable LNA boost for SF6
    if (sf == 6) {
        uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
        config1 |= 0x08;
        sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
    }
}

void sx1276_setCodingRate(uint8_t cr) {
    if (cr < 5) cr = 5;
    if (cr > 8) cr = 8;
    
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0xF1) | ((cr - 4) << 1);
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

void sx1276_setInvertIQ(bool invert) {
    uint8_t invertIQ = sx1276_readReg(REG_INVERT_IQ);
    if (invert) {
        invertIQ |= 0x40; // Set bit 6
        invertIQ |= 0x01;  // Set bit 0
    } else {
        invertIQ &= ~0x40; // Clear bit 6
        invertIQ &= ~0x01;  // Clear bit 0
    }
    sx1276_writeReg(REG_INVERT_IQ, invertIQ);
}

void sx1276_setMode(uint8_t mode) {
    uint8_t targetMode = MODE_LONG_RANGE_MODE | mode;
    sx1276_writeReg(REG_OP_MODE, targetMode);
}

void sx1276_writeFifo(uint8_t* data, uint8_t len) {
    sx1276_writeReg(REG_FIFO_ADDR_PTR, 0x00);
    sx1276_writeReg(REG_PAYLOAD_LENGTH, len);
    
    SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(SX1276_SS, LOW);
    SPI.transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(SX1276_SS, HIGH);
    SPI.endTransaction();
}

void sx1276_readFifo(uint8_t* data, uint8_t len) {
    uint8_t addr = sx1276_readReg(REG_FIFO_RX_CURRENT_ADDR);
    sx1276_writeReg(REG_FIFO_ADDR_PTR, addr);
    
    SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(SX1276_SS, LOW);
    SPI.transfer(REG_FIFO & 0x7F);
    for (uint8_t i = 0; i < len; i++) {
        data[i] = SPI.transfer(0x00);
    }
    digitalWrite(SX1276_SS, HIGH);
    SPI.endTransaction();
}

int16_t sx1276_getRssi() {
    return -164 + sx1276_readReg(REG_PKT_RSSI_VALUE);
}

int8_t sx1276_getSnr() {
    return (int8_t)sx1276_readReg(REG_PKT_SNR_VALUE) / 4;
}

void sx1276_setPreambleLength(uint16_t length) {
    sx1276_writeReg(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    sx1276_writeReg(REG_PREAMBLE_LSB, (uint8_t)(length));
}

void sx1276_setCrc(bool enable) {
    uint8_t config2 = sx1276_readReg(REG_MODEM_CONFIG_2);
    if (enable) {
        config2 |= 0x04;
    } else {
        config2 &= ~0x04;
    }
    sx1276_writeReg(REG_MODEM_CONFIG_2, config2);
}

void sx1276_setSyncWord(uint8_t syncWord) {
    sx1276_writeReg(REG_SYNC_WORD, syncWord);
}

void sx1276_setHeaderMode(bool implicit) {
    // Bit 0 of REG_MODEM_CONFIG_1: 0 = explicit, 1 = implicit
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    if (implicit) {
        config1 |= 0x01; // Set bit 0 for implicit header mode
    } else {
        config1 &= ~0x01; // Clear bit 0 for explicit header mode
    }
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

uint8_t sx1276_readRegister(uint8_t reg) {
    return sx1276_readReg(reg);
}

void sx1276_writeRegister(uint8_t reg, uint8_t value) {
    sx1276_writeReg(reg, value);
}

void sx1276_attachInterrupt(void (*handler)()) {
    attachInterrupt(digitalPinToInterrupt(SX1276_DIO0), handler, RISING);
}

bool sx1276_isPacketReceived() {
    uint8_t irqFlags = sx1276_readReg(REG_IRQ_FLAGS);
    return (irqFlags & IRQ_RX_DONE_MASK) != 0;
}

uint8_t sx1276_getPacketLength() {
    return sx1276_readReg(REG_RX_NB_BYTES);
}

void sx1276_clearIrqFlags() {
    sx1276_writeReg(REG_IRQ_FLAGS, 0xFF); // Clear all flags
}
