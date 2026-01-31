#ifndef MESHCORE_HANDLER_H
#define MESHCORE_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// MeshCore Packet Header bits (from Packet.h)
#define PH_ROUTE_MASK     0x03   // 2-bits
#define PH_TYPE_SHIFT     2
#define PH_TYPE_MASK      0x0F   // 4-bits
#define PH_VER_SHIFT      6
#define PH_VER_MASK       0x03   // 2-bits

#define ROUTE_TYPE_TRANSPORT_FLOOD   0x00
#define ROUTE_TYPE_FLOOD             0x01
#define ROUTE_TYPE_DIRECT            0x02
#define ROUTE_TYPE_TRANSPORT_DIRECT  0x03

#define MAX_MESHCORE_PATH_SIZE 64
#define MAX_MESHCORE_PAYLOAD_SIZE 184

// MeshCore packet structure
typedef struct {
    uint8_t header;
    uint16_t transport_codes[2];
    uint8_t path_len;
    uint8_t path[MAX_MESHCORE_PATH_SIZE];
    uint16_t payload_len;
    uint8_t payload[MAX_MESHCORE_PAYLOAD_SIZE];
} MeshCorePacket;

// Meshtastic packet header (from RadioInterface.h)
typedef struct {
    uint32_t to;        // NodeNum, little-endian
    uint32_t from;      // NodeNum, little-endian
    uint32_t id;        // PacketId, little-endian
    uint8_t flags;      // hop_limit[0:2], want_ack[3], via_mqtt[4], hop_start[5:7]
    uint8_t channel;    // channel hash
    uint8_t next_hop;   // last byte of NodeNum
    uint8_t relay_node; // last byte of NodeNum
} MeshtasticHeader;

// Function prototypes
bool meshcore_parsePacket(const uint8_t* data, uint8_t len, MeshCorePacket* packet);
bool meshcore_convertToMeshtastic(const MeshCorePacket* meshcore, uint8_t* output, uint8_t* outputLen);
uint8_t meshcore_getRouteType(uint8_t header);
uint8_t meshcore_getPayloadType(uint8_t header);
bool meshcore_hasTransportCodes(uint8_t header);

#endif // MESHCORE_HANDLER_H
