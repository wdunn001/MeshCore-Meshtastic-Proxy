#include "sx1276_direct.h"
#include "../../platforms/platform_interface.h"  // Platform interface for pin definitions
#include <SPI.h>
#include <avr/interrupt.h>
#include <Arduino.h>

// Static variables to cache pin numbers from platform interface
static int8_t pin_nss = -1;
static int8_t pin_reset = -1;
static int8_t pin_dio0 = -1;
static uint32_t spi_freq = 1000000;

// SX1276 SPI Communication (internal helpers)
static uint8_t sx1276_readReg(uint8_t reg) {
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    SPI.transfer(reg & 0x7F); // Read: bit 7 = 0
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
    return value;
}

static void sx1276_writeReg(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    SPI.transfer(reg | 0x80); // Write: bit 7 = 1
    SPI.transfer(value);
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
}

bool sx1276_direct_init() {
    // Get pin numbers from platform interface
    pin_nss = platform_getRadioNssPin();
    pin_reset = platform_getRadioResetPin();
    pin_dio0 = platform_getRadioDio0Pin();
    spi_freq = platform_getSpiFrequency();
    
    // Validate required pins
    if (pin_nss < 0 || pin_dio0 < 0) {
        return false;  // Required pins not available
    }
    
    // Initialize SPI pins
    pinMode(pin_nss, OUTPUT);
    if (pin_reset >= 0) {
        pinMode(pin_reset, OUTPUT);
    }
    pinMode(pin_dio0, INPUT);
    
    digitalWrite(pin_nss, HIGH);
    
    // Initialize SPI
    SPI.begin();
    
    // Reset SX1276 (if reset pin available)
    if (pin_reset >= 0) {
        digitalWrite(pin_reset, LOW);
        delay(10);
        digitalWrite(pin_reset, HIGH);
        delay(10);
    }
    
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
    
    // Radio is now initialized and ready
    // Protocol-specific configuration (frequency, bandwidth, etc.) should be done
    // by the protocol layer via the radio interface functions
    
    // Set default IQ inversion (protocols will override as needed)
    sx1276_direct_setInvertIQ(false);
    
    // Configure FIFO addresses
    sx1276_writeReg(REG_FIFO_TX_BASE_ADDR, 0x00);
    sx1276_writeReg(REG_FIFO_RX_BASE_ADDR, 0x00);
    
    // CRITICAL: Configure LNA for maximum sensitivity
    // LNA_GAIN_1 (max gain) + LNA_BOOST_ON (150% current)
    sx1276_writeReg(REG_LNA, LNA_GAIN_1 | LNA_BOOST_ON);
    
    // CRITICAL: Enable AGC Auto in MODEM_CONFIG_3
    // This allows the radio to automatically adjust gain for optimal reception
    sx1276_writeReg(REG_MODEM_CONFIG_3, AGC_AUTO_ON);
    
    // Configure DIO0 mapping for RX_DONE (will be remapped for TX)
    // DIO0: 00 = RxDone (in RX mode), TxDone (in TX mode)
    sx1276_writeReg(REG_DIO_MAPPING_1, 0x00);
    
    // Clear all IRQ flags
    sx1276_writeReg(REG_IRQ_FLAGS, 0xFF);
    
    return true;
}

uint32_t sx1276_direct_getMinFrequency() {
    return SX1276_MIN_FREQUENCY_HZ;
}

uint32_t sx1276_direct_getMaxFrequency() {
    return SX1276_MAX_FREQUENCY_HZ;
}

void sx1276_direct_setFrequency(uint32_t freq_hz) {
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

void sx1276_direct_setPower(uint8_t power) {
    // SX1276 PA_CONFIG register (0x09):
    // Bit 7: PaSelect - 0=RFO, 1=PA_BOOST (use PA_BOOST for higher power)
    // Bits 6-4: MaxPower - not used when PA_BOOST selected
    // Bits 3-0: OutputPower - Pout = 17 - (15 - OutputPower) = 2 + OutputPower dBm (PA_BOOST)
    //                         Range: 2 to 17 dBm (OutputPower 0-15)
    //                         For >17 dBm, need PA_DAC register (0x4D)
    
    if (power > 17) power = 17;  // Max without PA_DAC
    if (power < 2) power = 2;    // Min with PA_BOOST
    
    // PA_BOOST mode: Pout = 2 + OutputPower, so OutputPower = power - 2
    uint8_t outputPower = power - 2;
    if (outputPower > 15) outputPower = 15;
    
    // PA_BOOST (bit 7) + OutputPower (bits 3-0)
    uint8_t paConfig = 0x80 | outputPower;
    sx1276_writeReg(REG_PA_CONFIG, paConfig);
}

void sx1276_direct_setBandwidth(uint8_t bw) {
    // BW: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz, 5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0x0F) | (bw << 4);
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

void sx1276_direct_setSpreadingFactor(uint8_t sf) {
    // SF: 6-12
    if (sf < 6) sf = 6;
    if (sf > 12) sf = 12;
    
    uint8_t config2 = sx1276_readReg(REG_MODEM_CONFIG_2);
    config2 = (config2 & 0x0F) | (sf << 4);
    sx1276_writeReg(REG_MODEM_CONFIG_2, config2);
    
    // Configure MODEM_CONFIG_3 based on spreading factor
    // Always keep AGC_AUTO_ON, add LOW_DATA_RATE_OPTIMIZE for SF11/SF12
    uint8_t config3 = AGC_AUTO_ON;  // Always enable AGC
    if (sf >= 11) {
        // Enable Low Data Rate Optimize for SF11/SF12 with low bandwidths
        // This is required for symbol times > 16ms
        config3 |= LOW_DATA_RATE_OPTIMIZE;
    }
    sx1276_writeReg(REG_MODEM_CONFIG_3, config3);
    
    // SF6 requires special detection optimize settings
    if (sf == 6) {
        // For SF6, set detection optimize to 0x05 and threshold to 0x0C
        sx1276_writeReg(REG_DETECTION_OPTIMIZE, 0x05);
        sx1276_writeReg(REG_DETECTION_THRESHOLD, 0x0C);
    } else {
        // For SF7-12, use standard settings
        sx1276_writeReg(REG_DETECTION_OPTIMIZE, 0x03);
        sx1276_writeReg(REG_DETECTION_THRESHOLD, 0x0A);
    }
}

void sx1276_direct_setCodingRate(uint8_t cr) {
    if (cr < 5) cr = 5;
    if (cr > 8) cr = 8;
    
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    config1 = (config1 & 0xF1) | ((cr - 4) << 1);
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

void sx1276_direct_setInvertIQ(bool invert) {
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

void sx1276_direct_setMode(uint8_t mode) {
    // Clear IRQ flags before mode change
    sx1276_writeReg(REG_IRQ_FLAGS, 0xFF);
    
    // Configure DIO0 mapping based on mode
    // DIO0 bits 7-6: 00=RxDone, 01=TxDone, 10=CadDone
    if (mode == MODE_TX) {
        // For TX mode, map DIO0 to TxDone (01 in bits 7-6)
        sx1276_writeReg(REG_DIO_MAPPING_1, 0x40);
    } else if (mode == MODE_RX_CONTINUOUS) {
        // For RX mode, map DIO0 to RxDone (00 in bits 7-6)
        sx1276_writeReg(REG_DIO_MAPPING_1, 0x00);
    }
    
    uint8_t targetMode = MODE_LONG_RANGE_MODE | mode;
    sx1276_writeReg(REG_OP_MODE, targetMode);
}

void sx1276_direct_writeFifo(uint8_t* data, uint8_t len) {
    sx1276_writeReg(REG_FIFO_ADDR_PTR, 0x00);
    sx1276_writeReg(REG_PAYLOAD_LENGTH, len);
    
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    SPI.transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
}

void sx1276_direct_readFifo(uint8_t* data, uint8_t len) {
    uint8_t addr = sx1276_readReg(REG_FIFO_RX_CURRENT_ADDR);
    sx1276_writeReg(REG_FIFO_ADDR_PTR, addr);
    
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(pin_nss, LOW);
    SPI.transfer(REG_FIFO & 0x7F);
    for (uint8_t i = 0; i < len; i++) {
        data[i] = SPI.transfer(0x00);
    }
    digitalWrite(pin_nss, HIGH);
    SPI.endTransaction();
}

int16_t sx1276_direct_getRssi() {
    return -164 + sx1276_readReg(REG_PKT_RSSI_VALUE);
}

int8_t sx1276_direct_getSnr() {
    return (int8_t)sx1276_readReg(REG_PKT_SNR_VALUE) / 4;
}

void sx1276_direct_setPreambleLength(uint16_t length) {
    sx1276_writeReg(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    sx1276_writeReg(REG_PREAMBLE_LSB, (uint8_t)(length));
}

void sx1276_direct_setCrc(bool enable) {
    uint8_t config2 = sx1276_readReg(REG_MODEM_CONFIG_2);
    if (enable) {
        config2 |= 0x04;
    } else {
        config2 &= ~0x04;
    }
    sx1276_writeReg(REG_MODEM_CONFIG_2, config2);
}

void sx1276_direct_setSyncWord(uint8_t syncWord) {
    sx1276_writeReg(REG_SYNC_WORD, syncWord);
}

void sx1276_direct_setHeaderMode(bool implicit) {
    // Bit 0 of REG_MODEM_CONFIG_1: 0 = explicit, 1 = implicit
    uint8_t config1 = sx1276_readReg(REG_MODEM_CONFIG_1);
    if (implicit) {
        config1 |= 0x01; // Set bit 0 for implicit header mode
    } else {
        config1 &= ~0x01; // Clear bit 0 for explicit header mode
    }
    sx1276_writeReg(REG_MODEM_CONFIG_1, config1);
}

uint8_t sx1276_direct_readRegister(uint8_t reg) {
    return sx1276_readReg(reg);
}

void sx1276_direct_writeRegister(uint8_t reg, uint8_t value) {
    sx1276_writeReg(reg, value);
}

void sx1276_direct_attachInterrupt(void (*handler)()) {
    if (pin_dio0 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_dio0), handler, RISING);
    }
}

bool sx1276_direct_isPacketReceived() {
    uint8_t irqFlags = sx1276_readReg(REG_IRQ_FLAGS);
    return (irqFlags & IRQ_RX_DONE_MASK) != 0;
}

uint8_t sx1276_direct_getPacketLength() {
    return sx1276_readReg(REG_RX_NB_BYTES);
}

void sx1276_direct_clearIrqFlags() {
    sx1276_writeReg(REG_IRQ_FLAGS, 0xFF); // Clear all flags
}

uint16_t sx1276_direct_getIrqFlags() {
    // SX1276 IRQ flags are 8-bit (unlike SX1262 which is 16-bit)
    return (uint16_t)sx1276_readReg(REG_IRQ_FLAGS);
}

bool sx1276_direct_hasPacketErrors() {
    uint16_t irqFlags = sx1276_direct_getIrqFlags();
    // Check CRC error (bit 5 = 0x20)
    if (irqFlags & IRQ_CRC_ERROR_MASK) {
        return true; // CRC error - packet corrupted
    }
    return false; // No errors detected (SX1276 doesn't have header error flag)
}
