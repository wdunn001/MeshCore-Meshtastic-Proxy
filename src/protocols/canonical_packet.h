#ifndef CANONICAL_PACKET_H
#define CANONICAL_PACKET_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Canonical Packet Format
 * 
 * Standard intermediate format for protocol conversion.
 * All protocols convert TO this format when receiving and FROM this format when transmitting.
 * This reduces conversion complexity from N*(N-1) to 2*N conversions.
 */

// Maximum sizes for canonical packet fields
#define CANONICAL_MAX_ADDRESSES 8
#define CANONICAL_MAX_PAYLOAD 255
#define CANONICAL_MAX_PATH 64

// Message types (common across protocols)
typedef enum {
    CANONICAL_MSG_TEXT = 0x01,
    CANONICAL_MSG_DATA = 0x02,
    CANONICAL_MSG_GROUP_TEXT = 0x05,
    CANONICAL_MSG_GROUP_DATA = 0x06,
    CANONICAL_MSG_RAW = 0x0F,
    CANONICAL_MSG_UNKNOWN = 0xFF
} CanonicalMessageType;

// Routing types
typedef enum {
    CANONICAL_ROUTE_BROADCAST = 0x00,
    CANONICAL_ROUTE_FLOOD = 0x01,
    CANONICAL_ROUTE_DIRECT = 0x02,
    CANONICAL_ROUTE_TRANSPORT_DIRECT = 0x03
} CanonicalRouteType;

// Canonical packet structure
typedef struct {
    // Routing information
    CanonicalRouteType routeType;
    uint8_t hopLimit;
    bool wantAck;
    bool viaMqtt;  // Filter flag - packets with this set should be dropped
    
    // Addressing
    uint32_t sourceAddress;      // Source node address
    uint32_t destinationAddress; // Destination node address (0xFFFFFFFF for broadcast)
    uint32_t packetId;           // Packet identifier
    
    // Path/routing path (for MeshCore-style routing)
    uint8_t pathLength;
    uint32_t path[CANONICAL_MAX_PATH / 4];  // Path as array of addresses
    
    // Message content
    CanonicalMessageType messageType;
    uint16_t payloadLength;
    uint8_t payload[CANONICAL_MAX_PAYLOAD];
    
    // Protocol-specific metadata
    uint8_t channel;      // Channel identifier
    uint8_t version;      // Protocol version
} CanonicalPacket;

// Canonical packet API
void canonical_packet_init(CanonicalPacket* packet);
bool canonical_packet_isBroadcast(const CanonicalPacket* packet);
bool canonical_packet_isValid(const CanonicalPacket* packet);

#endif // CANONICAL_PACKET_H
