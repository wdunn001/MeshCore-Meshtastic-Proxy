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
#include "sx1262_radiolib.h"
#include "../../platforms/platform_interface.h"
// Note: We use numeric constants (0x00, 0x01, 0x03, 0x05) in switch statements
// to avoid conflicts with RadioLib's Module::MODE_* enum

// RadioLib module instance
static Module* radioModule = nullptr;
static SX1262* radio = nullptr;

// Interrupt handler
static void (*interruptHandler)() = nullptr;

// Buffer for pending transmission
static uint8_t* pendingTxData = nullptr;
static uint8_t pendingTxLen = 0;

// Interrupt service routine
static void sx1262_isr() {
    if (interruptHandler != nullptr) {
        interruptHandler();
    }
}

bool sx1262_radiolib_init() {
    // Get pin numbers from platform interface
    int8_t pin_nss = platform_getRadioNssPin();
    int8_t pin_reset = platform_getRadioResetPin();
    int8_t pin_dio1 = platform_getRadioDio1Pin();
    int8_t pin_busy = platform_getRadioBusyPin();
    int8_t pin_power_en = platform_getRadioPowerEnablePin();
    uint32_t spi_freq = platform_getSpiFrequency();
    
    // Validate required pins
    if (pin_nss < 0 || pin_dio1 < 0 || pin_busy < 0) {
        // Invalid pin configuration - this is a platform configuration issue
        return false;
    }
    
    // CRITICAL: Initialize SPI FIRST before any pin operations
    // On nRF52, SPI.begin() must be called before configuring pins
    SPI.begin();
    
    // Initialize SPI pins
    pinMode(pin_nss, OUTPUT);
    digitalWrite(pin_nss, HIGH); // Set NSS high (inactive)
    
    pinMode(pin_dio1, INPUT);
    pinMode(pin_busy, INPUT);
    
    // Enable power first (if power enable pin exists)
    if (pin_power_en >= 0) {
        pinMode(pin_power_en, OUTPUT);
        digitalWrite(pin_power_en, LOW); // Start with power off
        delay(10);
        digitalWrite(pin_power_en, HIGH); // Enable power
        delay(100); // SX1262 needs ~100ms after power-on to be ready
    }
    
    // Handle reset pin (if available)
    if (pin_reset >= 0) {
        pinMode(pin_reset, OUTPUT);
        // Reset radio: pull reset low, wait, then high
        digitalWrite(pin_reset, LOW);
        delay(10);
        digitalWrite(pin_reset, HIGH);
        delay(100); // Give radio time to stabilize after reset
    }
    
    // CRITICAL: Wait for BUSY pin to go LOW before SPI operations
    // SX1262 BUSY pin is HIGH when radio is busy, LOW when ready
    // RadioLib's Module class should handle this, but we ensure it's ready
    uint32_t busyWaitStart = millis();
    while (digitalRead(pin_busy) == HIGH && (millis() - busyWaitStart) < 500) {
        delay(1); // Wait for BUSY to go LOW (max 500ms timeout)
    }
    
    // If BUSY is still HIGH after timeout, radio might not be responding
    if (digitalRead(pin_busy) == HIGH) {
        // Radio not responding - cleanup and fail
        return false;
    }
    
    // Create RadioLib module
    // SX1262 uses DIO1 for interrupts, BUSY pin for busy indication
    // Note: RadioLib Module constructor accepts -1 for reset pin (no reset)
    radioModule = new Module(pin_nss, pin_dio1, pin_reset, pin_busy, SPI, SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    
    if (radioModule == nullptr) {
        // Failed to allocate module
        return false;
    }
    
    // Create SX1262 instance
    radio = new SX1262(radioModule);
    
    if (radio == nullptr) {
        // Failed to allocate radio
        delete radioModule;
        radioModule = nullptr;
        return false;
    }
    
    // Get platform-specific SX126x configuration
    float tcxoVoltage = platform_getTcxoVoltage();      // Critical for RAK4631 (1.8V)
    bool useRegulatorLDO = platform_useRegulatorLDO();  // False = DC-DC (more efficient)
    bool useDio2AsRfSwitch = platform_useDio2AsRfSwitch();
    
    // Initialize radio with default LoRa parameters
    // RadioLib's begin() can fail if SPI communication fails or radio doesn't respond
    // SX1262 begin() signature: begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, 
    //   uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10, 
    //   uint16_t preambleLength = 8, float tcxoVoltage = 0.0, bool useRegulatorLDO = false)
    // CRITICAL: Pass tcxoVoltage for boards with TCXO (like RAK4631 which needs 1.8V on DIO3)
    int state = radio->begin(
        434.0,                                 // freq (will be reconfigured by protocol)
        125.0,                                 // bw (will be reconfigured by protocol)
        9,                                     // sf (will be reconfigured by protocol)
        7,                                     // cr (will be reconfigured by protocol)
        RADIOLIB_SX126X_SYNC_WORD_PRIVATE,     // syncWord (will be reconfigured by protocol)
        10,                                    // power (will be reconfigured by protocol)
        8,                                     // preambleLength (will be reconfigured by protocol)
        tcxoVoltage,                           // TCXO voltage - CRITICAL for RAK4631!
        useRegulatorLDO                        // Use DC-DC regulator (false) or LDO (true)
    );
    
    if (state != RADIOLIB_ERR_NONE) {
        // Radio initialization failed - could be SPI communication issue or hardware problem
        // Clean up allocated objects
        delete radio;
        radio = nullptr;
        delete radioModule;
        radioModule = nullptr;
        return false;
    }
    
    // Configure DIO2 as RF switch control if platform requires it
    // This is CRITICAL for RAK4631 - DIO2 controls the antenna switch
    if (useDio2AsRfSwitch) {
        radio->setDio2AsRfSwitch(true);
    }
    
    // Set DIO1 interrupt mapping (TX_DONE and RX_DONE)
    // Use setPacketReceivedAction and setPacketSentAction which handle interrupt setup internally
    radio->setPacketReceivedAction(sx1262_isr);
    radio->setPacketSentAction(sx1262_isr);
    
    return true;
}

uint32_t sx1262_radiolib_getMinFrequency() {
    return SX1262_MIN_FREQUENCY_HZ;
}

uint32_t sx1262_radiolib_getMaxFrequency() {
    return SX1262_MAX_FREQUENCY_HZ;
}

void sx1262_radiolib_setFrequency(uint32_t freq_hz) {
    if (radio != nullptr) {
        radio->setFrequency((float)freq_hz / 1000000.0);
    }
}

void sx1262_radiolib_setPower(uint8_t power) {
    if (radio != nullptr) {
        radio->setOutputPower(power);
    }
}

void sx1262_radiolib_setPreambleLength(uint16_t length) {
    if (radio != nullptr) {
        radio->setPreambleLength(length);
    }
}

void sx1262_radiolib_setCrc(bool enable) {
    if (radio != nullptr) {
        radio->setCRC(enable ? 2 : 0); // 0 = disabled, 2 = enabled
    }
}

void sx1262_radiolib_setSyncWord(uint8_t syncWord) {
    if (radio != nullptr) {
        radio->setSyncWord(syncWord);
    }
}

void sx1262_radiolib_setHeaderMode(bool implicit) {
    if (radio != nullptr) {
        if (implicit) {
            radio->implicitHeader(0xFF); // Use max length for implicit header
        } else {
            radio->explicitHeader();
        }
    }
}

void sx1262_radiolib_setBandwidth(uint8_t bw) {
    if (radio != nullptr) {
        // Convert bandwidth code to RadioLib bandwidth
        // bw codes: 0=7.8kHz, 1=10.4kHz, 2=15.6kHz, 3=20.8kHz, 4=31.25kHz, 5=41.7kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
        float bandwidths[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
        if (bw < sizeof(bandwidths)/sizeof(bandwidths[0])) {
            radio->setBandwidth(bandwidths[bw]);
        }
    }
}

void sx1262_radiolib_setSpreadingFactor(uint8_t sf) {
    if (radio != nullptr) {
        radio->setSpreadingFactor(sf);
    }
}

void sx1262_radiolib_setCodingRate(uint8_t cr) {
    if (radio != nullptr) {
        radio->setCodingRate(cr);
    }
}

void sx1262_radiolib_setInvertIQ(bool invert) {
    if (radio != nullptr) {
        radio->invertIQ(invert);
    }
}

void sx1262_radiolib_setMode(uint8_t mode) {
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

void sx1262_radiolib_writeFifo(uint8_t* data, uint8_t len) {
    if (radio != nullptr && data != nullptr) {
        // Store data for transmission (will be sent when setMode(MODE_TX) is called)
        // Note: This assumes data buffer remains valid until transmission starts
        pendingTxData = data;
        pendingTxLen = len;
    }
}

void sx1262_radiolib_readFifo(uint8_t* data, uint8_t len) {
    if (radio != nullptr && data != nullptr) {
        radio->readData(data, len);
    }
}

int16_t sx1262_radiolib_getRssi() {
    if (radio != nullptr) {
        return radio->getRSSI();
    }
    return -127;
}

int8_t sx1262_radiolib_getSnr() {
    if (radio != nullptr) {
        return radio->getSNR();
    }
    return 0;
}

uint8_t sx1262_radiolib_readRegister(uint8_t reg) {
    if (radioModule != nullptr) {
        // SX1262 uses 16-bit register addresses, but we'll use the low byte
        return radioModule->SPIreadRegister(reg);
    }
    return 0;
}

void sx1262_radiolib_writeRegister(uint8_t reg, uint8_t value) {
    if (radioModule != nullptr) {
        // SX1262 uses 16-bit register addresses, but we'll use the low byte
        radioModule->SPIwriteRegister(reg, value);
    }
}

void sx1262_radiolib_attachInterrupt(void (*handler)()) {
    interruptHandler = handler;
}

bool sx1262_radiolib_isPacketReceived() {
    if (radio != nullptr) {
        return radio->available();
    }
    return false;
}

uint8_t sx1262_radiolib_getPacketLength() {
    if (radio != nullptr) {
        return radio->getPacketLength();
    }
    return 0;
}

void sx1262_radiolib_clearIrqFlags() {
    if (radio != nullptr) {
        radio->clearIrqFlags(RADIOLIB_SX126X_IRQ_ALL);
    }
}

uint16_t sx1262_radiolib_getIrqFlags() {
    if (radio != nullptr) {
        return (uint16_t)radio->getIrqFlags();
    }
    return 0;
}

bool sx1262_radiolib_hasPacketErrors() {
    if (radio != nullptr) {
        // Check for CRC error or header error using RadioLib's getIrqFlags
        uint32_t flags = radio->getIrqFlags();
        return (flags & (RADIOLIB_SX126X_IRQ_CRC_ERR | RADIOLIB_SX126X_IRQ_HEADER_ERR)) != 0;
    }
    return false;
}
