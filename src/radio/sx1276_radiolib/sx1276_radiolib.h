#ifndef SX1276_RADIOLIB_H
#define SX1276_RADIOLIB_H

#include <stdint.h>
#include <stdbool.h>

// SX1276 Register Definitions (for compatibility)
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_RSSI_VALUE       0x1A
#define REG_PKT_SNR_VALUE        0x1B
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_MODEM_CONFIG_3       0x26
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_INVERT_IQ            0x33
#define REG_VERSION              0x42

// Operating Modes - DO NOT define MODE_* constants here to avoid conflicts with RadioLib's Module.h
// MODE_SLEEP (0x00), MODE_STDBY (0x01), MODE_TX (0x03), MODE_RX_CONTINUOUS (0x05) are defined in radio_interface.h
// MODE_LONG_RANGE_MODE is SX1276-specific and not used by RadioLib
#define MODE_LONG_RANGE_MODE     0x80

// IRQ Flags (for compatibility)
#define IRQ_TX_DONE_MASK         0x08
#define IRQ_RX_DONE_MASK         0x40
#define IRQ_CRC_ERROR_MASK       0x20
#define IRQ_HEADER_VALID_MASK    0x10
#define IRQ_HEADER_ERROR_MASK    0x08

// Frequency Range Constants (SX1276: 137-1020 MHz)
#define SX1276_MIN_FREQUENCY_HZ  137000000UL
#define SX1276_MAX_FREQUENCY_HZ 1020000000UL

// Function Prototypes (matches sx1276_direct interface)
bool sx1276_radiolib_init();
uint32_t sx1276_radiolib_getMinFrequency();
uint32_t sx1276_radiolib_getMaxFrequency();
void sx1276_radiolib_setFrequency(uint32_t freq_hz);
void sx1276_radiolib_setPower(uint8_t power);
void sx1276_radiolib_setPreambleLength(uint16_t length);
void sx1276_radiolib_setCrc(bool enable);
void sx1276_radiolib_setSyncWord(uint8_t syncWord);
void sx1276_radiolib_setHeaderMode(bool implicit);
void sx1276_radiolib_setBandwidth(uint8_t bw);
void sx1276_radiolib_setSpreadingFactor(uint8_t sf);
void sx1276_radiolib_setCodingRate(uint8_t cr);
void sx1276_radiolib_setInvertIQ(bool invert);
void sx1276_radiolib_setMode(uint8_t mode);
void sx1276_radiolib_writeFifo(uint8_t* data, uint8_t len);
void sx1276_radiolib_readFifo(uint8_t* data, uint8_t len);
int16_t sx1276_radiolib_getRssi();
int8_t sx1276_radiolib_getSnr();
uint8_t sx1276_radiolib_readRegister(uint8_t reg);
void sx1276_radiolib_writeRegister(uint8_t reg, uint8_t value);
void sx1276_radiolib_attachInterrupt(void (*handler)());
bool sx1276_radiolib_isPacketReceived();
uint8_t sx1276_radiolib_getPacketLength();
void sx1276_radiolib_clearIrqFlags();
uint16_t sx1276_radiolib_getIrqFlags();
bool sx1276_radiolib_hasPacketErrors();

#endif // SX1276_RADIOLIB_H
