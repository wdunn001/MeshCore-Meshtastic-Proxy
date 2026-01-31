#include "canonical_packet.h"
#include <string.h>

void canonical_packet_init(CanonicalPacket* packet) {
    if (packet == nullptr) {
        return;
    }
    
    memset(packet, 0, sizeof(CanonicalPacket));
    packet->destinationAddress = 0xFFFFFFFF;  // Default to broadcast
    packet->messageType = CANONICAL_MSG_UNKNOWN;
    packet->routeType = CANONICAL_ROUTE_BROADCAST;
}

bool canonical_packet_isBroadcast(const CanonicalPacket* packet) {
    if (packet == nullptr) {
        return false;
    }
    return packet->destinationAddress == 0xFFFFFFFF || 
           packet->routeType == CANONICAL_ROUTE_BROADCAST ||
           packet->routeType == CANONICAL_ROUTE_FLOOD;
}

bool canonical_packet_isValid(const CanonicalPacket* packet) {
    if (packet == nullptr) {
        return false;
    }
    
    // Basic validation
    if (packet->payloadLength > CANONICAL_MAX_PAYLOAD) {
        return false;
    }
    
    if (packet->pathLength > CANONICAL_MAX_PATH) {
        return false;
    }
    
    // Must have some payload or addressing information
    if (packet->payloadLength == 0 && packet->pathLength == 0) {
        return false;
    }
    
    return true;
}
