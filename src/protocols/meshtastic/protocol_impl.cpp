#include "protocol_impl.h"
#include "../protocol_manager.h"
#include "../canonical_packet.h"
#include "../../radio/radio_interface.h"
#include "../../usb_comm.h"
#include <Arduino.h>
#include <string.h>

extern USBComm usbComm;

// Meshtastic packet handler implementation
static bool meshtastic_handlePacket(const uint8_t* data, uint8_t len, ProtocolRuntimeState* state, uint8_t* output, uint8_t* outputLen) {
    if (state == nullptr || output == nullptr || outputLen == nullptr) {
        return false;
    }
    
    // Parse Meshtastic packet
    MeshtasticHeader header;
    uint8_t payload[255];
    uint8_t payloadLen = 0;
    
    if (!meshtastic_parsePacket(data, len, &header, payload, &payloadLen)) {
        state->stats.parseErrors++;
        char errorMsg[40];
        if (len < MESHTASTIC_HEADER_SIZE) {
            snprintf(errorMsg, sizeof(errorMsg), "ERR: MT too short %d<%d", len, MESHTASTIC_HEADER_SIZE);
        } else {
            snprintf(errorMsg, sizeof(errorMsg), "ERR: MT parse fail len=%d", len);
        }
        usbComm.sendDebugLog(errorMsg);
        return false;
    }
    
    // Filter out MQTT-originated packets
    if (meshtastic_isViaMqtt(&header)) {
        return false;  // Silently drop MQTT packets
    }
    
    state->stats.rxCount++;
    
    // Convert to MeshCore format
    if (!meshtastic_convertToMeshCore(&header, payload, payloadLen, output, outputLen)) {
        state->stats.conversionErrors++;
        usbComm.sendDebugLog("ERR: MT->MC conv fail");
        return false;
    }
    
    return true;
}

// Meshtastic configuration
static void meshtastic_configure(const ProtocolConfig* config) {
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
static uint8_t meshtastic_getMaxPacketSize() {
    return MAX_MESHTASTIC_PACKET_SIZE;
}

// Parse packet (wrapper for handler)
static bool meshtastic_parsePacketWrapper(const uint8_t* data, uint8_t len, void* packet) {
    if (packet == nullptr) {
        return false;
    }
    MeshtasticHeader* header = (MeshtasticHeader*)packet;
    uint8_t payload[255];
    uint8_t payloadLen = 0;
    return meshtastic_parsePacket(data, len, header, payload, &payloadLen);
}

// Convert Meshtastic packet to canonical format
static bool meshtastic_convertToCanonical(const uint8_t* data, uint8_t len, CanonicalPacket* canonical) {
    if (data == nullptr || canonical == nullptr) {
        return false;
    }
    
    // Parse Meshtastic packet
    MeshtasticHeader header;
    uint8_t payload[255];
    uint8_t payloadLen = 0;
    
    if (!meshtastic_parsePacket(data, len, &header, payload, &payloadLen)) {
        return false;
    }
    
    // Filter MQTT packets
    if (meshtastic_isViaMqtt(&header)) {
        return false;
    }
    
    canonical_packet_init(canonical);
    
    // Extract addressing
    canonical->sourceAddress = header.from;
    canonical->destinationAddress = header.to;
    canonical->packetId = header.id;
    
    // Extract routing info
    canonical->hopLimit = meshtastic_getHopLimit(&header);
    canonical->wantAck = (header.flags & PACKET_FLAGS_WANT_ACK_MASK) != 0;
    canonical->viaMqtt = meshtastic_isViaMqtt(&header);
    canonical->channel = header.channel;
    
    // Determine route type from destination
    if (meshtastic_isBroadcast(&header)) {
        canonical->routeType = CANONICAL_ROUTE_BROADCAST;
    } else {
        canonical->routeType = CANONICAL_ROUTE_DIRECT;
    }
    
    // Copy payload (Meshtastic payload is typically protobuf-encoded)
    canonical->payloadLength = payloadLen;
    if (canonical->payloadLength > 0 && canonical->payloadLength <= CANONICAL_MAX_PAYLOAD) {
        memcpy(canonical->payload, payload, payloadLen);
    }
    
    // Default message type (Meshtastic uses protobuf, so we can't easily determine type)
    canonical->messageType = CANONICAL_MSG_DATA;
    canonical->version = 1;
    
    return true;
}

// Convert canonical format to Meshtastic packet
static bool meshtastic_convertFromCanonical(const CanonicalPacket* canonical, uint8_t* output, uint8_t* outputLen) {
    if (canonical == nullptr || output == nullptr || outputLen == nullptr) {
        return false;
    }
    
    if (!canonical_packet_isValid(canonical)) {
        return false;
    }
    
    // Build Meshtastic header
    MeshtasticHeader* header = (MeshtasticHeader*)output;
    header->to = canonical->destinationAddress;
    header->from = canonical->sourceAddress;
    header->id = canonical->packetId;
    
    // Build flags
    header->flags = canonical->hopLimit & PACKET_FLAGS_HOP_LIMIT_MASK;
    if (canonical->wantAck) {
        header->flags |= PACKET_FLAGS_WANT_ACK_MASK;
    }
    if (canonical->viaMqtt) {
        header->flags |= PACKET_FLAGS_VIA_MQTT_MASK;
    }
    
    header->channel = canonical->channel;
    header->next_hop = 0;
    header->relay_node = 0;
    
    // Copy payload
    uint8_t offset = MESHTASTIC_HEADER_SIZE;
    if (canonical->payloadLength > 0 && canonical->payloadLength <= MAX_MESHTASTIC_PAYLOAD_SIZE) {
        memcpy(&output[offset], canonical->payload, canonical->payloadLength);
        offset += canonical->payloadLength;
    }
    
    *outputLen = offset;
    return true;
}

// Initialize state
static void meshtastic_initState(ProtocolRuntimeState* state) {
    if (state == nullptr) {
        return;
    }
    state->id = PROTOCOL_MESHTASTIC;
    state->stats.rxCount = 0;
    state->stats.txCount = 0;
    state->stats.parseErrors = 0;
    state->stats.conversionErrors = 0;
    state->isActive = false;
    
    // Initialize config from protocol manager defaults
    ProtocolConfig* config = protocol_manager_getConfig(PROTOCOL_MESHTASTIC);
    if (config != nullptr) {
        state->config = *config;
    }
}

// Cleanup state
static void meshtastic_cleanupState(ProtocolRuntimeState* state) {
    // Nothing to cleanup for Meshtastic
    (void)state;
}

// Update statistics
static void meshtastic_updateStats(ProtocolRuntimeState* state, bool rx, bool tx, bool parseError, bool convError) {
    if (state == nullptr) {
        return;
    }
    if (rx) state->stats.rxCount++;
    if (tx) state->stats.txCount++;
    if (parseError) state->stats.parseErrors++;
    if (convError) state->stats.conversionErrors++;
}

// Generate test packet
static uint8_t meshtastic_generateTestPacket(uint8_t* buffer, uint8_t* len) {
    const char* testMsg = "Meshtastic Test";
    uint8_t msgLen = strlen(testMsg);
    
    MeshtasticHeader* header = (MeshtasticHeader*)buffer;
    header->to = 0xFFFFFFFF;
    header->from = 0x00000001;
    header->id = 0x00000001;
    header->flags = 0x03;
    header->channel = 0;
    header->next_hop = 0;
    header->relay_node = 0;
    
    memcpy(&buffer[MESHTASTIC_HEADER_SIZE], testMsg, msgLen);
    *len = MESHTASTIC_HEADER_SIZE + msgLen;
    return *len;
}

// Meshtastic protocol interface implementation
static ProtocolInterfaceImpl meshtasticInterface = {
    .id = PROTOCOL_MESHTASTIC,
    .name = "Meshtastic",
    .configure = meshtastic_configure,
    .getMaxPacketSize = meshtastic_getMaxPacketSize,
    .parsePacket = meshtastic_parsePacketWrapper,
    .handlePacket = meshtastic_handlePacket,
    .convertToCanonical = meshtastic_convertToCanonical,
    .convertFromCanonical = meshtastic_convertFromCanonical,
    .initState = meshtastic_initState,
    .cleanupState = meshtastic_cleanupState,
    .updateStats = meshtastic_updateStats,
    .generateTestPacket = meshtastic_generateTestPacket
};

ProtocolInterfaceImpl* meshtastic_getProtocolInterface() {
    return &meshtasticInterface;
}
