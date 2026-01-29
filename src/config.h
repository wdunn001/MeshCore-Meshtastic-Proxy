#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// SX1276 Pin Definitions (LoRa32u4 II)
// Correct pins per BSFrance LoRa32u4 II board specification:
// NSS = pin 8, RST = pin 4, DIO0 = pin 7, DIO1 = pin 5
#define SX1276_SS    8     // SPI Slave Select (NSS)
#define SX1276_RESET 4     // Reset pin
#define SX1276_RST   4     // Reset pin (alias for compatibility)
#define SX1276_DIO0  7     // DIO0 interrupt pin
#define SX1276_DIO1  5     // DIO1 interrupt pin (must be manually wired for versions < 1.3)

// SPI Settings
#define SPI_FREQ     1000000   // 1 MHz SPI clock (matches reference implementation for LoRa32u4 II)

// Frequency Configuration
#define DEFAULT_FREQUENCY_HZ 910525000  // 910.525 MHz (MeshCore default)
#define MESHTASTIC_FREQUENCY_HZ 906875000  // 906.875 MHz (Meshtastic frequency)

// LoRa Parameters - MeshCore (USA/Canada Recommended Preset - "Public" channel)
// Matches MeshCore's standard "Public" channel configuration for maximum compatibility
#define MESHCORE_SF 7           // Spreading Factor
#define MESHCORE_BW 6           // Bandwidth (6 = 62.5kHz) - MeshCore standard
#define MESHCORE_CR 5           // Coding Rate
#define MESHCORE_SYNC_WORD 0x12 // MeshCore sync word
#define MESHCORE_PREAMBLE 8     // Preamble length

// LoRa Parameters - Meshtastic
#define MESHTASTIC_SF 7           // Spreading Factor
#define MESHTASTIC_BW 8           // Bandwidth (8 = 250kHz) - Meshtastic standard
#define MESHTASTIC_CR 5           // Coding Rate
#define MESHTASTIC_SYNC_WORD 0x2B // Meshtastic sync word
#define MESHTASTIC_PREAMBLE 16    // Preamble length (Meshtastic requirement)

// Packet Size Limits
#define MAX_MESHCORE_PACKET_SIZE 255
#define MAX_MESHTASTIC_PACKET_SIZE 255
#define MESHTASTIC_HEADER_SIZE 16
#define MAX_MESHTASTIC_PAYLOAD_SIZE 237

// Time-slicing Configuration
#define PROTOCOL_SWITCH_INTERVAL_MS 100  // Switch between protocols every 100ms

// Serial Configuration
#define SERIAL_BAUD 115200

// LED Pin (if available)
#define LED_PIN 13

#endif // CONFIG_H
