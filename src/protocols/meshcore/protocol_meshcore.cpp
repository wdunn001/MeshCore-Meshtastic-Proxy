#include "protocol_meshcore.h"
#include "../protocol_manager.h"
#include "../canonical_packet.h"
#include "../../radio/radio_interface.h"
#include "../../usb_comm.h"
#include <Arduino.h>
#include <string.h>

extern USBComm usbComm;

// MeshCore packet handler implementation
static bool meshcore_handlePacket(const uint8_t* data, uint8_t len, ProtocolRuntimeState* state, uint8_t* output, uint8_t* outputLen) {
    if (state == nullptr || output == nullptr || outputLen == nullptr) {
        return false;
    }
    
    // Parse MeshCore packet
    MeshCorePacket meshcorePacket;
    if (!meshcore_parsePacket(data, len, &meshcorePacket)) {
        state->stats.parseErrors++;
        if (len > 0) {
            char errorMsg[40];
            snprintf(errorMsg, sizeof(errorMsg), "ERR: MC parse fail len=%d", len);
            usbComm.sendDebugLog(errorMsg);
        } else {
            usbComm.sendDebugLog("ERR: MC empty");
        }
        return false;
    }
    
    state->stats.rxCount++;
    
    // Convert to Meshtastic format
    if (!meshcore_convertToMeshtastic(&meshcorePacket, output, outputLen)) {
        state->stats.conversionErrors++;
        usbComm.sendDebugLog("ERR: MC->MT conv fail");
        return false;
    }
    
    return true;
}

// MeshCore configuration
static void meshcore_configure(const ProtocolConfig* config) {
    radio_setMode(MODE_STDBY);
    delay(10);
    radio_setFrequency(config->frequencyHz);
    radio_setBandwidth(config->bandwidth);
    radio_setSpreadingFactor(config->spreadingFactor);
    radio_setCodingRate(config->codingRate);
    radio_setSyncWord(config->syncWord);
    radio_setPreambleLength(config->preambleLength);
    radio_setHeaderMode(config->implicitHeader);
    radio_setInvertIQ(config->invertIQ);
    radio_setCrc(config->crcEnabled);
    delay(10);
    radio_setMode(MODE_RX_CONTINUOUS);
}

// Get max packet size
static uint8_t meshcore_getMaxPacketSize() {
    return MAX_MESHCORE_PACKET_SIZE;
}

// Parse packet (wrapper for handler)
static bool meshcore_parsePacketWrapper(const uint8_t* data, uint8_t len, void* packet) {
    if (packet == nullptr) {
        return false;
    }
    return meshcore_parsePacket(data, len, (MeshCorePacket*)packet);
}

// Convert MeshCore packet to canonical format
static bool meshcore_convertToCanonical(const uint8_t* data, uint8_t len, CanonicalPacket* canonical) {
    if (data == nullptr || canonical == nullptr) {
        return false;
    }
    
    // Parse MeshCore packet
    MeshCorePacket meshcorePacket;
    if (!meshcore_parsePacket(data, len, &meshcorePacket)) {
        return false;
    }
    
    canonical_packet_init(canonical);
    
    // Extract route type
    uint8_t routeType = meshcore_getRouteType(meshcorePacket.header);
    switch (routeType) {
        case ROUTE_TYPE_FLOOD:
            canonical->routeType = CANONICAL_ROUTE_FLOOD;
            break;
        case ROUTE_TYPE_DIRECT:
            canonical->routeType = CANONICAL_ROUTE_DIRECT;
            break;
        case ROUTE_TYPE_TRANSPORT_DIRECT:
            canonical->routeType = CANONICAL_ROUTE_TRANSPORT_DIRECT;
            break;
        default:
            canonical->routeType = CANONICAL_ROUTE_BROADCAST;
            break;
    }
    
    // Extract payload type
    uint8_t payloadType = meshcore_getPayloadType(meshcorePacket.header);
    switch (payloadType) {
        case 0x02:
            canonical->messageType = CANONICAL_MSG_TEXT;
            break;
        case 0x05:
            canonical->messageType = CANONICAL_MSG_GROUP_TEXT;
            break;
        case 0x06:
            canonical->messageType = CANONICAL_MSG_GROUP_DATA;
            break;
        case 0x0F:
            canonical->messageType = CANONICAL_MSG_RAW;
            break;
        default:
            canonical->messageType = CANONICAL_MSG_DATA;
            break;
    }
    
    // Extract version
    canonical->version = (meshcorePacket.header >> PH_VER_SHIFT) & PH_VER_MASK;
    
    // Copy path
    canonical->pathLength = meshcorePacket.path_len;
    if (canonical->pathLength > 0 && canonical->pathLength <= CANONICAL_MAX_PATH) {
        memcpy(canonical->path, meshcorePacket.path, meshcorePacket.path_len);
    }
    
    // Copy payload
    canonical->payloadLength = meshcorePacket.payload_len;
    if (canonical->payloadLength > 0 && canonical->payloadLength <= CANONICAL_MAX_PAYLOAD) {
        memcpy(canonical->payload, meshcorePacket.payload, meshcorePacket.payload_len);
    }
    
    // Set default addressing (MeshCore doesn't have explicit addresses)
    canonical->sourceAddress = 0;
    canonical->destinationAddress = 0xFFFFFFFF;  // Broadcast
    canonical->packetId = 0;
    canonical->hopLimit = 3;  // Default
    
    return true;
}

// Convert canonical format to MeshCore packet
static bool meshcore_convertFromCanonical(const CanonicalPacket* canonical, uint8_t* output, uint8_t* outputLen) {
    if (canonical == nullptr || output == nullptr || outputLen == nullptr) {
        return false;
    }
    
    if (!canonical_packet_isValid(canonical)) {
        return false;
    }
    
    // Build MeshCore packet
    uint8_t i = 0;
    
    // Header: route type | payload type << 2 | version << 6
    uint8_t routeType = 0x01;  // Default to FLOOD
    switch (canonical->routeType) {
        case CANONICAL_ROUTE_FLOOD:
            routeType = 0x01;
            break;
        case CANONICAL_ROUTE_DIRECT:
            routeType = 0x02;
            break;
        case CANONICAL_ROUTE_TRANSPORT_DIRECT:
            routeType = 0x03;
            break;
        default:
            routeType = 0x01;
            break;
    }
    
    uint8_t payloadType = 0x02;  // Default to TEXT_MSG
    switch (canonical->messageType) {
        case CANONICAL_MSG_TEXT:
            payloadType = 0x02;
            break;
        case CANONICAL_MSG_GROUP_TEXT:
            payloadType = 0x05;
            break;
        case CANONICAL_MSG_GROUP_DATA:
            payloadType = 0x06;
            break;
        case CANONICAL_MSG_RAW:
            payloadType = 0x0F;
            break;
        default:
            payloadType = 0x02;
            break;
    }
    
    uint8_t version = canonical->version & 0x03;
    output[i++] = routeType | (payloadType << PH_TYPE_SHIFT) | (version << PH_VER_SHIFT);
    
    // Path length
    output[i++] = canonical->pathLength;
    
    // Path (if present)
    if (canonical->pathLength > 0) {
        memcpy(&output[i], canonical->path, canonical->pathLength);
        i += canonical->pathLength;
    }
    
    // Payload
    if (canonical->payloadLength > 0) {
        memcpy(&output[i], canonical->payload, canonical->payloadLength);
        i += canonical->payloadLength;
    }
    
    *outputLen = i;
    return true;
}

// Initialize state
static void meshcore_initState(ProtocolRuntimeState* state) {
    if (state == nullptr) {
        return;
    }
    state->id = PROTOCOL_MESHCORE;
    state->stats.rxCount = 0;
    state->stats.txCount = 0;
    state->stats.parseErrors = 0;
    state->stats.conversionErrors = 0;
    state->isActive = false;
    
    // Initialize config from protocol manager defaults
    ProtocolConfig* config = protocol_manager_getConfig(PROTOCOL_MESHCORE);
    if (config != nullptr) {
        state->config = *config;
    }
}

// Cleanup state
static void meshcore_cleanupState(ProtocolRuntimeState* state) {
    // Nothing to cleanup for MeshCore
    (void)state;
}

// Update statistics
static void meshcore_updateStats(ProtocolRuntimeState* state, bool rx, bool tx, bool parseError, bool convError) {
    if (state == nullptr) {
        return;
    }
    if (rx) state->stats.rxCount++;
    if (tx) state->stats.txCount++;
    if (parseError) state->stats.parseErrors++;
    if (convError) state->stats.conversionErrors++;
}

// Generate test packet
static uint8_t meshcore_generateTestPacket(uint8_t* buffer, uint8_t* len) {
    const char* testMsg = "MeshCore Test";
    uint8_t msgLen = strlen(testMsg);
    
    uint8_t i = 0;
    buffer[i++] = 0x01 | (0x02 << 2) | (0x00 << 6);
    buffer[i++] = 0;
    memcpy(&buffer[i], testMsg, msgLen);
    i += msgLen;
    
    *len = i;
    return i;
}

// MeshCore protocol interface implementation
static ProtocolInterfaceImpl meshcoreInterface = {
    .id = PROTOCOL_MESHCORE,
    .name = "MeshCore",
    .configure = meshcore_configure,
    .getMaxPacketSize = meshcore_getMaxPacketSize,
    .parsePacket = meshcore_parsePacketWrapper,
    .handlePacket = meshcore_handlePacket,
    .convertToCanonical = meshcore_convertToCanonical,
    .convertFromCanonical = meshcore_convertFromCanonical,
    .initState = meshcore_initState,
    .cleanupState = meshcore_cleanupState,
    .updateStats = meshcore_updateStats,
    .generateTestPacket = meshcore_generateTestPacket
};

ProtocolInterfaceImpl* meshcore_getProtocolInterface() {
    return &meshcoreInterface;
}
