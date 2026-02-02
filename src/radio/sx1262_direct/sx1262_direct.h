#ifndef SX1262_DIRECT_H
#define SX1262_DIRECT_H

#include <stdint.h>
#include <stdbool.h>

// SX1262 Command Definitions (direct SPI implementation)
// SX1262 uses command-based SPI protocol (not register-based like SX1276)

// SPI Commands
#define CMD_SET_SLEEP               0x84
#define CMD_SET_STANDBY             0x80
#define CMD_SET_FS                  0xC1
#define CMD_SET_TX                  0x83
#define CMD_SET_RX                  0x82
#define CMD_SET_RX_DUTY_CYCLE       0x94
#define CMD_SET_CAD                 0xC5
#define CMD_SET_TX_CONTINUOUS_WAVE  0xD1
#define CMD_SET_TX_INFINITE_PREAMBLE 0xD2
#define CMD_SET_REGULATOR_MODE      0x96
#define CMD_CALIBRATE               0x89
#define CMD_CALIBRATE_IMAGE         0x98
#define CMD_SET_PA_CONFIG           0x95
#define CMD_SET_RX_TX_FALLBACK_MODE 0x93
#define CMD_WRITE_REGISTER          0x0D
#define CMD_READ_REGISTER           0x1D
#define CMD_WRITE_BUFFER            0x0E
#define CMD_READ_BUFFER             0x1E
#define CMD_SET_DIO_IRQ_PARAMS      0x08
#define CMD_GET_IRQ_STATUS          0x12
#define CMD_CLEAR_IRQ_STATUS        0x02
#define CMD_SET_DIO2_AS_RF_SWITCH_CTRL 0x9D
#define CMD_SET_DIO3_AS_TCXO_CTRL   0x97
#define CMD_SET_RF_FREQUENCY        0x86
#define CMD_GET_PACKET_STATUS       0x14
#define CMD_GET_RX_BUFFER_STATUS    0x13

// Register Addresses (for WriteRegister/ReadRegister commands)
#define REG_LORA_SYNC_WORD_MSB      0x0740
#define REG_LORA_SYNC_WORD_LSB      0x0741
#define REG_RX_GAIN                 0x08AC
#define REG_TX_MODULATION           0x0889
#define REG_RX_MODULATION           0x0889
#define REG_LORA_CONFIG_1            0x0706
#define REG_LORA_CONFIG_2            0x0707
#define REG_LORA_CONFIG_3            0x0920
#define REG_RANDOM_NUMBER_GEN       0x0819
#define REG_IQ_POLARITY              0x0736

// Operating Modes (for SetStandby command)
#define STANDBY_RC                  0x00
#define STANDBY_XOSC                0x01

// IRQ Masks
#define IRQ_TX_DONE                 0x01
#define IRQ_RX_DONE                 0x02
#define IRQ_PREAMBLE_DETECTED       0x04
#define IRQ_SYNC_WORD_VALID         0x08
#define IRQ_HEADER_VALID            0x10
#define IRQ_HEADER_ERROR            0x20
#define IRQ_CRC_ERROR               0x40
#define IRQ_CAD_DONE                0x80
#define IRQ_CAD_DETECTED            0x100
#define IRQ_TIMEOUT                 0x200
#define IRQ_RX_TX_TIMEOUT           0x400
#define IRQ_PREAMBLE_ERROR          0x800
#define IRQ_ALL                     0xFFFF

// Frequency Range Constants (SX1262: 150-960 MHz)
#define SX1262_MIN_FREQUENCY_HZ  150000000UL
#define SX1262_MAX_FREQUENCY_HZ  960000000UL

// Function Prototypes (matching sx1276_direct.h API)
bool sx1262_direct_init();
uint32_t sx1262_direct_getMinFrequency();
uint32_t sx1262_direct_getMaxFrequency();
void sx1262_direct_setFrequency(uint32_t freq_hz);
void sx1262_direct_setPower(uint8_t power);
void sx1262_direct_setPreambleLength(uint16_t length);
void sx1262_direct_setCrc(bool enable);
void sx1262_direct_setSyncWord(uint8_t syncWord);
void sx1262_direct_setHeaderMode(bool implicit);
void sx1262_direct_setBandwidth(uint8_t bw);
void sx1262_direct_setSpreadingFactor(uint8_t sf);
void sx1262_direct_setCodingRate(uint8_t cr);
void sx1262_direct_setInvertIQ(bool invert);
void sx1262_direct_setMode(uint8_t mode);
void sx1262_direct_writeFifo(uint8_t* data, uint8_t len);
void sx1262_direct_readFifo(uint8_t* data, uint8_t len);
int16_t sx1262_direct_getRssi();
int8_t sx1262_direct_getSnr();
uint8_t sx1262_direct_readRegister(uint8_t reg);
void sx1262_direct_writeRegister(uint8_t reg, uint8_t value);
void sx1262_direct_attachInterrupt(void (*handler)());

// Get IRQ status flags (returns 16-bit value: MSB in [0], LSB in [1])
uint16_t sx1262_direct_getIrqFlags();

// Check if received packet has CRC or header errors
bool sx1262_direct_hasPacketErrors();
bool sx1262_direct_isPacketReceived();
uint8_t sx1262_direct_getPacketLength();
void sx1262_direct_clearIrqFlags();

#endif // SX1262_DIRECT_H
