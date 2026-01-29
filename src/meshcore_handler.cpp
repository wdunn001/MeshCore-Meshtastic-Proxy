#include "meshcore_handler.h"
#include "config.h"
#include <string.h>

bool meshcore_hasTransportCodes(uint8_t header) {
    uint8_t routeType = header & PH_ROUTE_MASK;
    return (routeType == ROUTE_TYPE_TRANSPORT_FLOOD || routeType == ROUTE_TYPE_TRANSPORT_DIRECT);
}

uint8_t meshcore_getRouteType(uint8_t header) {
    return header & PH_ROUTE_MASK;
}

uint8_t meshcore_getPayloadType(uint8_t header) {
    return (header >> PH_TYPE_SHIFT) & PH_TYPE_MASK;
}

bool meshcore_parsePacket(const uint8_t* data, uint8_t len, MeshCorePacket* packet) {
    if (data == NULL || packet == NULL || len == 0) {
        return false;
    }
    
    uint8_t i = 0;
    
    // Read header byte
    packet->header = data[i++];
    
    // Read transport codes if present
    if (meshcore_hasTransportCodes(packet->header)) {
        if (i + 4 > len) return false;
        memcpy(&packet->transport_codes[0], &data[i], 2); i += 2;
        memcpy(&packet->transport_codes[1], &data[i], 2); i += 2;
    } else {
        packet->transport_codes[0] = 0;
        packet->transport_codes[1] = 0;
    }
    
    // Read path length
    if (i >= len) return false;
    packet->path_len = data[i++];
    
    // Validate path length
    if (packet->path_len > MAX_MESHCORE_PATH_SIZE) return false;
    
    // Read path data
    if (i + packet->path_len > len) return false;
    memcpy(packet->path, &data[i], packet->path_len);
    i += packet->path_len;
    
    // Remaining bytes are payload
    if (i >= len) return false;
    packet->payload_len = len - i;
    if (packet->payload_len > MAX_MESHCORE_PAYLOAD_SIZE) return false;
    memcpy(packet->payload, &data[i], packet->payload_len);
    
    return true;
}

bool meshcore_convertToMeshtastic(const MeshCorePacket* meshcore, uint8_t* output, uint8_t* outputLen) {
    if (meshcore == NULL || output == NULL || outputLen == NULL) {
        return false;
    }
    
    // Check if payload fits in Meshtastic packet
    uint8_t totalPayloadSize = meshcore->payload_len;
    if (totalPayloadSize > MAX_MESHTASTIC_PAYLOAD_SIZE) {
        return false; // Payload too large
    }
    
    MeshtasticHeader* header = (MeshtasticHeader*)output;
    
    // Set Meshtastic header fields
    // For broadcast, use 0xFFFFFFFF
    header->to = 0xFFFFFFFF; // Broadcast
    header->from = 0x00000001; // Proxy node ID (placeholder)
    header->id = 0x00000001; // Packet ID (placeholder, should be incremented)
    
    // Extract hop limit from MeshCore path (if available)
    uint8_t hopLimit = 3; // Default hop limit
    if (meshcore->path_len > 0) {
        // Use path length as approximation for hop limit
        hopLimit = meshcore->path_len;
        if (hopLimit > 7) hopLimit = 7; // Max 7 hops
    }
    
    header->flags = hopLimit & 0x07; // Bottom 3 bits
    header->flags |= (0 << 3); // want_ack = false
    header->flags |= (0 << 4); // via_mqtt = false
    header->flags |= (0 << 5); // hop_start = 0
    
    header->channel = 0; // Default channel
    header->next_hop = 0;
    header->relay_node = 0;
    
    // Copy payload (MeshCore payload becomes Meshtastic encrypted payload)
    memcpy(&output[MESHTASTIC_HEADER_SIZE], meshcore->payload, meshcore->payload_len);
    
    *outputLen = MESHTASTIC_HEADER_SIZE + meshcore->payload_len;
    
    return true;
}
