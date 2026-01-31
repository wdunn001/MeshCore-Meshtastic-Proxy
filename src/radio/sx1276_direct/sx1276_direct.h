#ifndef SX1276_DIRECT_H
#define SX1276_DIRECT_H

#include <stdint.h>
#include <stdbool.h>

// SX1276 Register Definitions (ATmega-specific)
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

// Operating Modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

// IRQ Flags
#define IRQ_TX_DONE_MASK         0x08
#define IRQ_RX_DONE_MASK         0x40

// Frequency Range Constants (SX1276: 137-1020 MHz)
#define SX1276_MIN_FREQUENCY_HZ  137000000UL
#define SX1276_MAX_FREQUENCY_HZ 1020000000UL

// Function Prototypes (internal implementation)
bool sx1276_direct_init();
uint32_t sx1276_direct_getMinFrequency();
uint32_t sx1276_direct_getMaxFrequency();
void sx1276_direct_setFrequency(uint32_t freq_hz);
void sx1276_direct_setPower(uint8_t power);
void sx1276_direct_setPreambleLength(uint16_t length);
void sx1276_direct_setCrc(bool enable);
void sx1276_direct_setSyncWord(uint8_t syncWord);
void sx1276_direct_setHeaderMode(bool implicit);
void sx1276_direct_setBandwidth(uint8_t bw);
void sx1276_direct_setSpreadingFactor(uint8_t sf);
void sx1276_direct_setCodingRate(uint8_t cr);
void sx1276_direct_setInvertIQ(bool invert);
void sx1276_direct_setMode(uint8_t mode);
void sx1276_direct_writeFifo(uint8_t* data, uint8_t len);
void sx1276_direct_readFifo(uint8_t* data, uint8_t len);
int16_t sx1276_direct_getRssi();
int8_t sx1276_direct_getSnr();
uint8_t sx1276_direct_readRegister(uint8_t reg);
void sx1276_direct_writeRegister(uint8_t reg, uint8_t value);
void sx1276_direct_attachInterrupt(void (*handler)());
bool sx1276_direct_isPacketReceived();
uint8_t sx1276_direct_getPacketLength();
void sx1276_direct_clearIrqFlags();

#endif // SX1276_DIRECT_H
