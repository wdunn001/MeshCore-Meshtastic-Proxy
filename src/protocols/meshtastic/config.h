#ifndef MESHTASTIC_CONFIG_H
#define MESHTASTIC_CONFIG_H

// Meshtastic Protocol Configuration
// LoRa parameters for Meshtastic protocol
// Configured for LongFast preset (US region default):
// - SF=11, BW=250kHz, CR=5
// - Frequency slot 20 = 906.875 MHz (for US region with LongFast preset)

// Default frequency (906.875 MHz - Meshtastic frequency slot 20 for LongFast preset)
#define MESHTASTIC_DEFAULT_FREQUENCY_HZ 906875000

// LoRa Parameters - Meshtastic LongFast Preset
#define MESHTASTIC_SF 11          // Spreading Factor (LongFast uses SF=11, not SF=7!)
#define MESHTASTIC_BW 8           // Bandwidth (8 = 250kHz) - LongFast preset
#define MESHTASTIC_CR 5           // Coding Rate (LongFast preset)
// NOTE: Hardware limitations:
// 1. Sync word filtering is hardware-level and cannot be disabled.
//    Meshtastic uses sync word 0x2B (base) but hashes it with channel name.
//    Using 0x2B matches Meshtastic's base sync word for maximum compatibility.
// 2. Radio can only listen on ONE frequency at a time.
//    To listen to ALL Meshtastic messages across different frequencies/channels,
//    use auto-switch mode to rotate through frequencies quickly.
//    Or configure multiple protocol instances with different frequencies.
#define MESHTASTIC_SYNC_WORD 0x2B // Meshtastic base sync word (matches most Meshtastic traffic)
#define MESHTASTIC_PREAMBLE 16    // Preamble length (Meshtastic requirement)

// Packet Size Limits
#define MAX_MESHTASTIC_PACKET_SIZE 255
#define MESHTASTIC_HEADER_SIZE 16
#define MAX_MESHTASTIC_PAYLOAD_SIZE 237

#endif // MESHTASTIC_CONFIG_H
