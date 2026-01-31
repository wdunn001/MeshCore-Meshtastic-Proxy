#ifndef MESHTASTIC_CONFIG_H
#define MESHTASTIC_CONFIG_H

// Meshtastic Protocol Configuration
// LoRa parameters for Meshtastic protocol

// Default frequency (906.875 MHz - Meshtastic frequency)
#define MESHTASTIC_DEFAULT_FREQUENCY_HZ 906875000

// LoRa Parameters - Meshtastic
#define MESHTASTIC_SF 7           // Spreading Factor
#define MESHTASTIC_BW 8           // Bandwidth (8 = 250kHz) - Meshtastic standard
#define MESHTASTIC_CR 5           // Coding Rate
#define MESHTASTIC_SYNC_WORD 0x2B // Meshtastic sync word
#define MESHTASTIC_PREAMBLE 16    // Preamble length (Meshtastic requirement)

// Packet Size Limits
#define MAX_MESHTASTIC_PACKET_SIZE 255
#define MESHTASTIC_HEADER_SIZE 16
#define MAX_MESHTASTIC_PAYLOAD_SIZE 237

#endif // MESHTASTIC_CONFIG_H
