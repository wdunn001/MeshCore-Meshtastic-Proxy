#include <Arduino.h>
#include <SPI.h>
#include "../../platforms/platform_interface.h"  // Platform interface for pin definitions
#include "sx1262_direct.h"

// SPI object is defined in platform-specific platform.cpp files
// Since this file is only compiled for platforms that support SX1262 (RAK4631),
// the SPI object from platforms/rak4631/platform.cpp will be available at link time.
// SPI.h provides the SPIClass type, and platform.cpp provides the SPI instance.
// For nRF52 (RAK4631), SPI is defined in platforms/rak4631/platform.cpp, so we need extern declaration
extern SPIClass SPI;

// Static variables to cache pin numbers from platform interface
static int8_t pin_nss = -1;
static int8_t pin_busy = -1;
static int8_t pin_dio1 = -1;
static int8_t pin_power_en = -1;
static uint32_t spi_freq = 8000000;

// SX1262 Direct SPI Implementation
// Similar to sx1276_direct but uses SX1262 command-based protocol

// State tracking
static uint8_t current_mode = 0x01; // STANDBY
static uint8_t last_packet_length = 0;
static int16_t last_rssi = 0;
static int8_t last_snr = 0;
static volatile bool packet_received_flag = false;
static void (*user_interrupt_handler)() = nullptr;

// Wait for BUSY pin to go low (SX1262 requirement)
static void waitForBusy() {
    if (pin_busy >= 0) {
        while (digitalRead(pin_busy) == HIGH) {
            delayMicroseconds(1);
        }
    }
}

// Send SPI command and wait for BUSY
static void sx1262_sendCommand(uint8_t cmd, uint8_t* data = nullptr, uint8_t dataLen = 0) {
    waitForBusy();
    
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    
    SPI.transfer(cmd);
    for (uint8_t i = 0; i < dataLen; i++) {
        SPI.transfer(data[i]);
    }
    
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
    
    waitForBusy();
}

// Read response from SPI command
static void sx1262_readCommand(uint8_t cmd, uint8_t* data, uint8_t dataLen) {
    waitForBusy();
    
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    
    SPI.transfer(cmd);
    for (uint8_t i = 0; i < dataLen; i++) {
        data[i] = SPI.transfer(0x00);
    }
    
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
    
    waitForBusy();
}

// Write register (16-bit address)
static void sx1262_writeReg(uint16_t address, uint8_t* data, uint8_t len) {
    uint8_t cmdData[3 + len];
    cmdData[0] = (address >> 8) & 0xFF; // Address MSB
    cmdData[1] = address & 0xFF;         // Address LSB
    cmdData[2] = len;                     // Data length
    for (uint8_t i = 0; i < len; i++) {
        cmdData[3 + i] = data[i];
    }
    sx1262_sendCommand(CMD_WRITE_REGISTER, cmdData, 3 + len);
}

// Read register (16-bit address)
static void sx1262_readReg(uint16_t address, uint8_t* data, uint8_t len) {
    uint8_t cmdData[3];
    cmdData[0] = (address >> 8) & 0xFF; // Address MSB
    cmdData[1] = address & 0xFF;         // Address LSB
    cmdData[2] = len;                     // Data length
    
    waitForBusy();
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    
    SPI.transfer(CMD_READ_REGISTER);
    SPI.transfer(cmdData[0]);
    SPI.transfer(cmdData[1]);
    SPI.transfer(cmdData[2]);
    for (uint8_t i = 0; i < len; i++) {
        data[i] = SPI.transfer(0x00);
    }
    
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
    
    waitForBusy();
}

// Map SX1276 bandwidth codes to SX1262 bandwidth values
static uint8_t bw_code_to_sx1262(uint8_t bw_code) {
    // SX1276 codes: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz, 
    //               5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
    // SX1262: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz,
    //         5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
    // Mapping is the same!
    if (bw_code > 9) bw_code = 7; // Default to 125kHz
    return bw_code;
}

// IRQ handler wrapper
static void sx1262_irq_handler() {
    packet_received_flag = true;
    if (user_interrupt_handler) {
        user_interrupt_handler();
    }
}

bool sx1262_direct_init() {
    // Get pin numbers from platform interface
    pin_nss = platform_getRadioNssPin();
    pin_busy = platform_getRadioBusyPin();
    pin_dio1 = platform_getRadioDio1Pin();
    pin_power_en = platform_getRadioPowerEnablePin();
    spi_freq = platform_getSpiFrequency();
    
    // Validate required pins
    if (pin_nss < 0 || pin_busy < 0 || pin_dio1 < 0) {
        return false;  // Required pins not available
    }
    
    // Initialize power enable pin (if available)
    if (pin_power_en >= 0) {
        pinMode(pin_power_en, OUTPUT);
        digitalWrite(pin_power_en, HIGH);
        delay(10); // Give SX1262 time to power up
    }
    
    // Initialize pins
    pinMode(pin_nss, OUTPUT);
    pinMode(pin_busy, INPUT);
    pinMode(pin_dio1, INPUT);
    
    digitalWrite(pin_nss, HIGH);
    
    // Initialize SPI
    SPI.begin();
    
    // Wait for BUSY to go low (chip ready)
    waitForBusy();
    
    // Set to standby mode
    uint8_t standbyMode = STANDBY_RC;
    sx1262_sendCommand(CMD_SET_STANDBY, &standbyMode, 1);
    
    // Set regulator mode to LDO (for RAK4631)
    uint8_t regMode = 0x00; // LDO mode
    sx1262_sendCommand(CMD_SET_REGULATOR_MODE, &regMode, 1);
    
    // Radio is now initialized and ready
    // Protocol-specific configuration (frequency, bandwidth, etc.) should be done
    // by the protocol layer via the radio interface functions
    
    // Configure DIO1 for RX_DONE and TX_DONE interrupts
    uint8_t dioParams[8];
    dioParams[0] = 0x00; // IRQ mask MSB (IRQ_TX_DONE | IRQ_RX_DONE)
    dioParams[1] = 0x03; // IRQ mask LSB
    dioParams[2] = 0x00; // DIO1 mask MSB
    dioParams[3] = 0x03; // DIO1 mask LSB (DIO1 = IRQ_TX_DONE | IRQ_RX_DONE)
    dioParams[4] = 0x00; // DIO2 mask MSB
    dioParams[5] = 0x00; // DIO2 mask LSB
    dioParams[6] = 0x00; // DIO3 mask MSB
    dioParams[7] = 0x00; // DIO3 mask LSB
    sx1262_sendCommand(CMD_SET_DIO_IRQ_PARAMS, dioParams, 8);
    
    // Clear any pending IRQs
    sx1262_direct_clearIrqFlags();
    
    // Set to RX continuous mode
    sx1262_direct_setMode(0x05); // RX_CONTINUOUS
    
    return true;
}

uint32_t sx1262_direct_getMinFrequency() {
    return SX1262_MIN_FREQUENCY_HZ;
}

uint32_t sx1262_direct_getMaxFrequency() {
    return SX1262_MAX_FREQUENCY_HZ;
}

void sx1262_direct_setFrequency(uint32_t freq_hz) {
    // SX1262 frequency calculation: freq = (freq_hz * 2^25) / 32e6
    uint32_t freq_reg = (uint32_t)((uint64_t)freq_hz << 25) / 32000000;
    
    uint8_t freqData[4];
    freqData[0] = (freq_reg >> 24) & 0xFF;
    freqData[1] = (freq_reg >> 16) & 0xFF;
    freqData[2] = (freq_reg >> 8) & 0xFF;
    freqData[3] = freq_reg & 0xFF;
    
    sx1262_sendCommand(CMD_SET_RF_FREQUENCY, freqData, 4);
}

void sx1262_direct_setPower(uint8_t power) {
    if (power > 22) power = 22; // RAK4631 max is 22 dBm
    
    // PA config: [0]=power, [1]=rampTime (0x04 = 200us)
    uint8_t paConfig[2];
    paConfig[0] = power;
    paConfig[1] = 0x04; // Ramp time
    
    sx1262_sendCommand(CMD_SET_PA_CONFIG, paConfig, 2);
}

void sx1262_direct_setBandwidth(uint8_t bw) {
    uint8_t bw_sx1262 = bw_code_to_sx1262(bw);
    
    // Read current LoRa config
    uint8_t config[1];
    sx1262_readReg(REG_LORA_CONFIG_2, config, 1);
    
    // Set bandwidth bits (bits 4-7)
    config[0] = (config[0] & 0x0F) | (bw_sx1262 << 4);
    
    uint8_t writeData[1] = {config[0]};
    sx1262_writeReg(REG_LORA_CONFIG_2, writeData, 1);
}

void sx1262_direct_setSpreadingFactor(uint8_t sf) {
    if (sf < 6) sf = 6;
    if (sf > 12) sf = 12;
    
    // Read current LoRa config
    uint8_t config[1];
    sx1262_readReg(REG_LORA_CONFIG_2, config, 1);
    
    // Set spreading factor bits (bits 0-3)
    config[0] = (config[0] & 0xF0) | (sf - 5);
    
    uint8_t writeData[1] = {config[0]};
    sx1262_writeReg(REG_LORA_CONFIG_2, writeData, 1);
}

void sx1262_direct_setCodingRate(uint8_t cr) {
    if (cr < 5) cr = 5;
    if (cr > 8) cr = 8;
    
    // Read current LoRa config
    uint8_t config[1];
    sx1262_readReg(REG_LORA_CONFIG_1, config, 1);
    
    // Set coding rate bits (bits 1-3)
    config[0] = (config[0] & 0xF1) | ((cr - 4) << 1);
    
    uint8_t writeData[1] = {config[0]};
    sx1262_writeReg(REG_LORA_CONFIG_1, writeData, 1);
}

void sx1262_direct_setSyncWord(uint8_t syncWord) {
    uint8_t syncData[2] = {syncWord, syncWord}; // MSB and LSB (same for 8-bit sync word)
    sx1262_writeReg(REG_LORA_SYNC_WORD_MSB, syncData, 2);
}

void sx1262_direct_setPreambleLength(uint16_t length) {
    // SX1262 preamble length is set via SetTxParams command
    // For now, store it - it will be used when setting TX mode
    // Note: This is a simplified version - preamble is typically set with TX params
    // The actual preamble length is configured when entering TX mode
}

void sx1262_direct_setCrc(bool enable) {
    uint8_t config[1];
    sx1262_readReg(REG_LORA_CONFIG_1, config, 1);
    
    if (enable) {
        config[0] |= 0x20; // Enable CRC
    } else {
        config[0] &= ~0x20; // Disable CRC
    }
    
    uint8_t writeData[1] = {config[0]};
    sx1262_writeReg(REG_LORA_CONFIG_1, writeData, 1);
}

void sx1262_direct_setHeaderMode(bool implicit) {
    uint8_t config[1];
    sx1262_readReg(REG_LORA_CONFIG_1, config, 1);
    
    if (implicit) {
        config[0] |= 0x01; // Implicit header mode
    } else {
        config[0] &= ~0x01; // Explicit header mode
    }
    
    uint8_t writeData[1] = {config[0]};
    sx1262_writeReg(REG_LORA_CONFIG_1, writeData, 1);
}

void sx1262_direct_setInvertIQ(bool invert) {
    uint8_t iqData[1];
    sx1262_readReg(REG_IQ_POLARITY, iqData, 1);
    
    if (invert) {
        iqData[0] = 0x01; // Inverted IQ
    } else {
        iqData[0] = 0x00; // Normal IQ
    }
    
    sx1262_writeReg(REG_IQ_POLARITY, iqData, 1);
}

void sx1262_direct_setMode(uint8_t mode) {
    current_mode = mode;
    
    switch (mode) {
        case 0x00: // SLEEP
            sx1262_sendCommand(CMD_SET_SLEEP);
            break;
        case 0x01: // STDBY
            {
                uint8_t standbyMode = STANDBY_RC;
                sx1262_sendCommand(CMD_SET_STANDBY, &standbyMode, 1);
            }
            break;
        case 0x03: // TX
            // TX mode is set when transmitting (handled in writeFifo)
            {
                uint8_t standbyMode = STANDBY_RC;
                sx1262_sendCommand(CMD_SET_STANDBY, &standbyMode, 1);
            }
            break;
        case 0x05: // RX_CONTINUOUS
            {
                uint8_t rxParams[3];
                rxParams[0] = 0x00; // Timeout MSB (0 = continuous)
                rxParams[1] = 0x00; // Timeout LSB
                rxParams[2] = 0x00; // RX window mode (continuous)
                sx1262_sendCommand(CMD_SET_RX, rxParams, 3);
            }
            break;
    }
}

void sx1262_direct_writeFifo(uint8_t* data, uint8_t len) {
    // Ensure we're in standby before TX
    uint8_t standbyMode = STANDBY_RC;
    sx1262_sendCommand(CMD_SET_STANDBY, &standbyMode, 1);
    
    // Write buffer
    uint8_t writeCmd[1 + len];
    writeCmd[0] = 0x00; // Offset (start of buffer)
    for (uint8_t i = 0; i < len; i++) {
        writeCmd[1 + i] = data[i];
    }
    sx1262_sendCommand(CMD_WRITE_BUFFER, writeCmd, 1 + len);
    
    // Set TX with timeout
    uint8_t txParams[3];
    txParams[0] = 0x00; // Timeout MSB
    txParams[1] = 0x00; // Timeout LSB (0 = no timeout)
    txParams[2] = 0x00; // Ramp time
    sx1262_sendCommand(CMD_SET_TX, txParams, 3);
    
    current_mode = 0x03; // TX
}

void sx1262_direct_readFifo(uint8_t* data, uint8_t len) {
    // Get RX buffer status to get offset (we already have length from getPacketLength)
    uint8_t rxStatus[2];
    sx1262_readCommand(CMD_GET_RX_BUFFER_STATUS, rxStatus, 2);
    
    uint8_t rxPacketLen = rxStatus[0]; // Payload length from RX buffer status
    uint8_t offset = rxStatus[1];      // RX start buffer pointer
    
    // Use the length passed in (from getPacketLength) - it's already validated
    // Only use RX buffer status length if it's valid and matches (for verification)
    uint8_t packetLen = len;
    if (rxPacketLen != 0 && rxPacketLen != 255 && rxPacketLen == len) {
        // RX buffer status matches our length - good, use it
        packetLen = rxPacketLen;
    } else if (rxPacketLen == 255 || rxPacketLen == 0) {
        // RX buffer status is corrupted - trust the length parameter instead
        packetLen = len;
    } else if (rxPacketLen > len) {
        // RX buffer status says more data than expected - clamp to our length
        packetLen = len;
    }
    
    // Read buffer
    uint8_t readCmd[2];
    readCmd[0] = offset;  // Offset
    readCmd[1] = packetLen;  // Length
    
    waitForBusy();
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    
    SPI.transfer(CMD_READ_BUFFER);
    SPI.transfer(readCmd[0]);
    SPI.transfer(readCmd[1]);
    for (uint8_t i = 0; i < packetLen; i++) {
        data[i] = SPI.transfer(0x00);
    }
    
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
    
    waitForBusy();
    
    // Get RSSI and SNR from packet status
    uint8_t status[3];
    sx1262_readCommand(CMD_GET_PACKET_STATUS, status, 3);
    last_rssi = -(status[0] / 2); // RSSI in dBm
    last_snr = ((int8_t)status[1]) / 4; // SNR in dB
    
    // Save packet length BEFORE clearing IRQ flags (which resets last_packet_length)
    last_packet_length = packetLen;
    
    // Note: Don't clear IRQ flags here - let the caller do it after getting the length
    // Clearing here would reset last_packet_length to 0, making getPacketLength() fail
    
    // Restart RX if we were in RX mode
    if (current_mode == 0x05) {
        sx1262_direct_setMode(0x05);
    }
}

int16_t sx1262_direct_getRssi() {
    uint8_t status[3];
    sx1262_readCommand(CMD_GET_PACKET_STATUS, status, 3);
    return -(status[0] / 2); // RSSI in dBm
}

int8_t sx1262_direct_getSnr() {
    uint8_t status[3];
    sx1262_readCommand(CMD_GET_PACKET_STATUS, status, 3);
    return ((int8_t)status[1]) / 4; // SNR in dB
}

uint8_t sx1262_direct_getPacketLength() {
    // Always read RX buffer status directly (like RadioLib does)
    // Don't rely on cached value as it may be stale
    uint8_t rxStatus[2];
    sx1262_readCommand(CMD_GET_RX_BUFFER_STATUS, rxStatus, 2);
    uint8_t packetLen = rxStatus[0]; // Payload length
    
    // Update cache for consistency
    last_packet_length = packetLen;
    
    return packetLen;
}

bool sx1262_direct_isPacketReceived() {
    if (packet_received_flag) {
        return true;
    }
    
    // Check IRQ status
    uint8_t irqStatus[2];
    sx1262_readCommand(CMD_GET_IRQ_STATUS, irqStatus, 2);
    return (irqStatus[1] & IRQ_RX_DONE) != 0;
}

void sx1262_direct_clearIrqFlags() {
    uint8_t clearIrq[2] = {0xFF, 0xFF}; // Clear all IRQs
    sx1262_sendCommand(CMD_CLEAR_IRQ_STATUS, clearIrq, 2);
    packet_received_flag = false;
    last_packet_length = 0; // Reset cached length after clearing IRQ
}

void sx1262_direct_attachInterrupt(void (*handler)()) {
    user_interrupt_handler = handler;
    if (pin_dio1 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_dio1), sx1262_irq_handler, RISING);
    }
}

uint16_t sx1262_direct_getIrqFlags() {
    uint8_t irqStatus[2];
    sx1262_readCommand(CMD_GET_IRQ_STATUS, irqStatus, 2);
    // Return 16-bit IRQ status: MSB in [0], LSB in [1]
    return ((uint16_t)irqStatus[0] << 8) | irqStatus[1];
}

bool sx1262_direct_hasPacketErrors() {
    uint16_t irqFlags = sx1262_direct_getIrqFlags();
    // Check CRC error (bit 6 in LSB = 0x0040) and header error (bit 5 in LSB = 0x0020)
    // IRQ flags: [MSB][LSB], errors are in LSB byte
    if (irqFlags & 0x0040) { // IRQ_CRC_ERROR
        return true; // CRC error - packet corrupted
    }
    if (irqFlags & 0x0020) { // IRQ_HEADER_ERROR
        return true; // Header error - packet corrupted
    }
    return false; // No errors detected
}

uint8_t sx1262_direct_readRegister(uint8_t reg) {
    // Compatibility function - SX1262 uses 16-bit addresses
    uint8_t data[1];
    sx1262_readReg((uint16_t)reg, data, 1);
    return data[0];
}

void sx1262_direct_writeRegister(uint8_t reg, uint8_t value) {
    // Compatibility function - SX1262 uses 16-bit addresses
    uint8_t data[1] = {value};
    sx1262_writeReg((uint16_t)reg, data, 1);
}
