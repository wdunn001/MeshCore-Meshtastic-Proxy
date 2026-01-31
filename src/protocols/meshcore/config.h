#ifndef MESHCORE_CONFIG_H
#define MESHCORE_CONFIG_H

// MeshCore Protocol Configuration
// LoRa parameters for MeshCore protocol

// Default frequency (910.525 MHz - MeshCore default)
#define MESHCORE_DEFAULT_FREQUENCY_HZ 910525000

// LoRa Parameters - MeshCore (USA/Canada Recommended Preset - "Public" channel)
// Matches MeshCore's standard "Public" channel configuration for maximum compatibility
#define MESHCORE_SF 7           // Spreading Factor
#define MESHCORE_BW 6           // Bandwidth (6 = 62.5kHz) - MeshCore standard
#define MESHCORE_CR 5           // Coding Rate
#define MESHCORE_SYNC_WORD 0x12 // MeshCore sync word
#define MESHCORE_PREAMBLE 8     // Preamble length

// Packet Size Limits
#define MAX_MESHCORE_PACKET_SIZE 255

#endif // MESHCORE_CONFIG_H
