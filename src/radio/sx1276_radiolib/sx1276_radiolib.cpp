// Include Arduino and SPI FIRST
#include <Arduino.h>
#include <SPI.h>

// For nRF52 (RAK4631), SPI is defined in platform.cpp, so declare it extern
// For AVR (LoRa32u4II), SPI is defined globally by Arduino core
#ifdef RAK4631_BOARD
extern SPIClass SPI;
#endif

// Undefine any MODE_* macros that might be defined elsewhere (from radio_interface.h)
// This must be done BEFORE including RadioLib to avoid conflicts
#ifdef MODE_SLEEP
#undef MODE_SLEEP
#endif
#ifdef MODE_STDBY
#undef MODE_STDBY
#endif
#ifdef MODE_TX
#undef MODE_TX
#endif
#ifdef MODE_RX_CONTINUOUS
#undef MODE_RX_CONTINUOUS
#endif

// Now include RadioLib (after SPI is defined and MODE_* macros are undefined)
#include <RadioLib.h>

// Then include our headers (which may reference RadioLib types)
#include "sx1276_radiolib.h"
#include "../../platforms/platform_interface.h"
// Note: We use numeric constants (0x00, 0x01, 0x03, 0x05) in switch statements
// to avoid conflicts with RadioLib's Module::MODE_* enum

// RadioLib module instance
static Module* radioModule = nullptr;
static SX1276* radio = nullptr;

// Interrupt handler
static void (*interruptHandler)() = nullptr;

// Buffer for pending transmission
static uint8_t* pendingTxData = nullptr;
static uint8_t pendingTxLen = 0;

// Interrupt service routine
static void sx1276_isr() {
    if (interruptHandler != nullptr) {
        interruptHandler();
    }
}

bool sx1276_radiolib_init() {
    // Get pin numbers from platform interface
    int8_t pin_nss = platform_getRadioNssPin();
    int8_t pin_reset = platform_getRadioResetPin();
    int8_t pin_dio0 = platform_getRadioDio0Pin();
    uint32_t spi_freq = platform_getSpiFrequency();
    
    // Validate required pins
    if (pin_nss < 0 || pin_dio0 < 0) {
        return false;
    }
    
    // Initialize SPI pins
    pinMode(pin_nss, OUTPUT);
    if (pin_reset >= 0) {
        pinMode(pin_reset, OUTPUT);
    }
    pinMode(pin_dio0, INPUT);
    
    digitalWrite(pin_nss, HIGH);
    SPI.begin();
    
    // Create RadioLib module
    radioModule = new Module(pin_nss, pin_dio0, pin_reset, -1, SPI, SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    
    // Create SX1276 instance
    radio = new SX1276(radioModule);
    
    // Initialize radio
    int state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        return false;
    }
    
    // Set DIO0 interrupt mapping (TX_DONE and RX_DONE)
    // Use setPacketReceivedAction which handles interrupt setup internally
    radio->setPacketReceivedAction(sx1276_isr);
    radio->setPacketSentAction(sx1276_isr);
    
    return true;
}

uint32_t sx1276_radiolib_getMinFrequency() {
    return SX1276_MIN_FREQUENCY_HZ;
}

uint32_t sx1276_radiolib_getMaxFrequency() {
    return SX1276_MAX_FREQUENCY_HZ;
}

void sx1276_radiolib_setFrequency(uint32_t freq_hz) {
    if (radio != nullptr) {
        radio->setFrequency((float)freq_hz / 1000000.0);
    }
}

void sx1276_radiolib_setPower(uint8_t power) {
    if (radio != nullptr) {
        radio->setOutputPower(power);
    }
}

void sx1276_radiolib_setPreambleLength(uint16_t length) {
    if (radio != nullptr) {
        radio->setPreambleLength(length);
    }
}

void sx1276_radiolib_setCrc(bool enable) {
    if (radio != nullptr) {
        radio->setCRC(enable ? 2 : 0); // 0 = disabled, 2 = enabled
    }
}

void sx1276_radiolib_setSyncWord(uint8_t syncWord) {
    if (radio != nullptr) {
        radio->setSyncWord(syncWord);
    }
}

void sx1276_radiolib_setHeaderMode(bool implicit) {
    if (radio != nullptr) {
        if (implicit) {
            radio->implicitHeader(0xFF); // Use max length for implicit header
        } else {
            radio->explicitHeader();
        }
    }
}

void sx1276_radiolib_setBandwidth(uint8_t bw) {
    if (radio != nullptr) {
        // Convert bandwidth code to RadioLib bandwidth
        // bw codes: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz, 5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
        float bandwidths[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
        if (bw < sizeof(bandwidths)/sizeof(bandwidths[0])) {
            radio->setBandwidth(bandwidths[bw]);
        }
    }
}

void sx1276_radiolib_setSpreadingFactor(uint8_t sf) {
    if (radio != nullptr) {
        radio->setSpreadingFactor(sf);
    }
}

void sx1276_radiolib_setCodingRate(uint8_t cr) {
    if (radio != nullptr) {
        radio->setCodingRate(cr);
    }
}

void sx1276_radiolib_setInvertIQ(bool invert) {
    if (radio != nullptr) {
        radio->invertIQ(invert);
    }
}

void sx1276_radiolib_setMode(uint8_t mode) {
    if (radio == nullptr) return;
    
    // Use numeric constants to avoid conflicts with RadioLib's MODE_* definitions
    switch (mode) {
        case 0x00: // MODE_SLEEP
            radio->sleep();
            break;
        case 0x01: // MODE_STDBY
            radio->standby();
            break;
        case 0x03: // MODE_TX
            // Start transmission with pending data
            if (pendingTxData != nullptr && pendingTxLen > 0) {
                radio->startTransmit(pendingTxData, pendingTxLen);
                pendingTxData = nullptr;
                pendingTxLen = 0;
            }
            break;
        case 0x05: // MODE_RX_CONTINUOUS
            radio->startReceive();
            break;
    }
}

void sx1276_radiolib_writeFifo(uint8_t* data, uint8_t len) {
    if (radio != nullptr && data != nullptr) {
        // Store data for transmission (will be sent when setMode(MODE_TX) is called)
        // Note: This assumes data buffer remains valid until transmission starts
        pendingTxData = data;
        pendingTxLen = len;
    }
}

void sx1276_radiolib_readFifo(uint8_t* data, uint8_t len) {
    if (radio != nullptr && data != nullptr) {
        radio->readData(data, len);
    }
}

int16_t sx1276_radiolib_getRssi() {
    if (radio != nullptr) {
        return radio->getRSSI();
    }
    return -127;
}

int8_t sx1276_radiolib_getSnr() {
    if (radio != nullptr) {
        return radio->getSNR();
    }
    return 0;
}

uint8_t sx1276_radiolib_readRegister(uint8_t reg) {
    if (radioModule != nullptr) {
        return radioModule->SPIreadRegister(reg);
    }
    return 0;
}

void sx1276_radiolib_writeRegister(uint8_t reg, uint8_t value) {
    if (radioModule != nullptr) {
        radioModule->SPIwriteRegister(reg, value);
    }
}

void sx1276_radiolib_attachInterrupt(void (*handler)()) {
    interruptHandler = handler;
}

bool sx1276_radiolib_isPacketReceived() {
    if (radio != nullptr) {
        return radio->available();
    }
    return false;
}

uint8_t sx1276_radiolib_getPacketLength() {
    if (radio != nullptr) {
        return radio->getPacketLength();
    }
    return 0;
}

void sx1276_radiolib_clearIrqFlags() {
    if (radio != nullptr) {
        radio->clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);
    }
}

uint16_t sx1276_radiolib_getIrqFlags() {
    if (radio != nullptr) {
        return radio->getIRQFlags();
    }
    return 0;
}

bool sx1276_radiolib_hasPacketErrors() {
    if (radio != nullptr) {
        uint16_t flags = radio->getIRQFlags();
        // Check for payload CRC error (SX127x doesn't have separate header CRC error flag)
        return (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) != 0;
    }
    return false;
}
