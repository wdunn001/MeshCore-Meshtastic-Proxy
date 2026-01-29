#ifndef MESHTASTIC_HANDLER_H
#define MESHTASTIC_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "meshcore_handler.h"  // Includes MeshtasticHeader and MeshCorePacket definitions

// Meshtastic packet flags masks
#define PACKET_FLAGS_HOP_LIMIT_MASK 0x07
#define PACKET_FLAGS_WANT_ACK_MASK 0x08
#define PACKET_FLAGS_VIA_MQTT_MASK 0x10
#define PACKET_FLAGS_HOP_START_MASK 0xE0
#define PACKET_FLAGS_HOP_START_SHIFT 5

// MeshCore payload types
#define PAYLOAD_TYPE_TXT_MSG 0x02
#define PAYLOAD_TYPE_GRP_TXT 0x05
#define PAYLOAD_TYPE_GRP_DATA 0x06
#define PAYLOAD_TYPE_RAW_CUSTOM 0x0F

// Function prototypes
bool meshtastic_parsePacket(const uint8_t* data, uint8_t len, MeshtasticHeader* header, uint8_t* payload, uint8_t* payloadLen);
bool meshtastic_convertToMeshCore(const MeshtasticHeader* header, const uint8_t* payload, uint8_t payloadLen, uint8_t* output, uint8_t* outputLen);
uint8_t meshtastic_getHopLimit(const MeshtasticHeader* header);
bool meshtastic_isBroadcast(const MeshtasticHeader* header);

#endif // MESHTASTIC_HANDLER_H
