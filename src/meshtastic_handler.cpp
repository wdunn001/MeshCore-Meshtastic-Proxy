#include "meshtastic_handler.h"
#include "meshcore_handler.h"  // For PH_TYPE_SHIFT and PH_VER_SHIFT
#include "config.h"
#include <string.h>

bool meshtastic_parsePacket(const uint8_t* data, uint8_t len, MeshtasticHeader* header, uint8_t* payload, uint8_t* payloadLen) {
    if (data == NULL || header == NULL || payload == NULL || payloadLen == NULL) {
        return false;
    }
    
    if (len < MESHTASTIC_HEADER_SIZE) {
        return false; // Packet too short
    }
    
    // Parse header (little-endian)
    memcpy(&header->to, &data[0], 4);
    memcpy(&header->from, &data[4], 4);
    memcpy(&header->id, &data[8], 4);
    header->flags = data[12];
    header->channel = data[13];
    header->next_hop = data[14];
    header->relay_node = data[15];
    
    // Extract payload
    uint8_t payloadSize = len - MESHTASTIC_HEADER_SIZE;
    if (payloadSize > MAX_MESHTASTIC_PAYLOAD_SIZE) {
        return false; // Payload too large
    }
    
    memcpy(payload, &data[MESHTASTIC_HEADER_SIZE], payloadSize);
    *payloadLen = payloadSize;
    
    return true;
}

uint8_t meshtastic_getHopLimit(const MeshtasticHeader* header) {
    return header->flags & PACKET_FLAGS_HOP_LIMIT_MASK;
}

bool meshtastic_isBroadcast(const MeshtasticHeader* header) {
    return header->to == 0xFFFFFFFF;
}

bool meshtastic_convertToMeshCore(const MeshtasticHeader* header, const uint8_t* payload, uint8_t payloadLen, uint8_t* output, uint8_t* outputLen) {
    if (header == NULL || payload == NULL || output == NULL || outputLen == NULL) {
        return false;
    }
    
    // Check if payload fits in MeshCore packet
    if (payloadLen > 184) {
        return false; // Payload too large for MeshCore
    }
    
    uint8_t i = 0;
    
    // Build MeshCore header byte
    // Route type: FLOOD (0x01) for broadcast, DIRECT (0x02) for unicast
    uint8_t routeType = meshtastic_isBroadcast(header) ? 0x01 : 0x02;
    
    // Payload type: Use RAW_CUSTOM (0x0F) to preserve Meshtastic protobuf data
    uint8_t payloadType = PAYLOAD_TYPE_RAW_CUSTOM;
    
    // Version: Use VER_1 (0x00)
    uint8_t version = 0x00;
    
    // Construct header byte
    uint8_t meshcoreHeader = routeType | (payloadType << PH_TYPE_SHIFT) | (version << PH_VER_SHIFT);
    output[i++] = meshcoreHeader;
    
    // No transport codes for simple conversion
    // (Could be added later if needed)
    
    // Path length: Use hop limit as path approximation
    uint8_t hopLimit = meshtastic_getHopLimit(header);
    uint8_t pathLen = hopLimit;
    if (pathLen > 64) pathLen = 64; // Max path size
    
    output[i++] = pathLen;
    
    // Path data: Create simple path from Meshtastic routing info
    // Use from node ID bytes as path
    uint8_t fromBytes[4];
    memcpy(fromBytes, &header->from, 4);
    for (uint8_t j = 0; j < pathLen && j < 4; j++) {
        output[i++] = fromBytes[j];
    }
    // Pad with zeros if needed
    for (uint8_t j = 4; j < pathLen; j++) {
        output[i++] = 0;
    }
    
    // Copy payload
    memcpy(&output[i], payload, payloadLen);
    i += payloadLen;
    
    *outputLen = i;
    
    return true;
}
