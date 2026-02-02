#include "protocol_meshtastic.h"
#include "../protocol_manager.h"
#include "../canonical_packet.h"
#include "../../radio/radio_interface.h"
#include "../../usb_comm.h"
#include <Arduino.h>
#include <string.h>

extern USBComm usbComm;

// Meshtastic packet handler implementation
// ULTRA-LENIENT: Forward ANY packet bytes - no validation, no filtering
// Accepts packets of ANY length and structure - just relay everything
static bool meshtastic_handlePacket(const uint8_t* data, uint8_t len, ProtocolRuntimeState* state, uint8_t* output, uint8_t* outputLen) {
    if (state == nullptr || output == nullptr || outputLen == nullptr || data == nullptr) {
        return false;
    }
    
    // Accept packets of ANY length - no validation
    // Clamp to max size if needed, but don't reject
    uint8_t copyLen = len;
    if (copyLen > MAX_MESHTASTIC_PACKET_SIZE) {
        copyLen = MAX_MESHTASTIC_PACKET_SIZE;
    }
    
    // For relay: just copy the raw packet bytes
    // No parsing, no filtering, no validation - just forward everything
    if (copyLen > 0) {
        memcpy(output, data, copyLen);
    }
    *outputLen = copyLen;
    
    state->stats.rxCount++;
    
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
// ULTRA-LENIENT: Accept ANY packet - no validation at all
// We forward everything regardless of structure, length, or content
static bool meshtastic_parsePacketWrapper(const uint8_t* data, uint8_t len, void* packet) {
    // Accept packets of ANY length - no validation
    // Just check that we have valid pointers
    if (packet == nullptr || data == nullptr) {
        return false;
    }
    
    // Accept packets of any length (even 0 bytes)
    // No structure validation - we're just relaying raw bytes
    return true;
}

// Convert Meshtastic packet to canonical format
// ULTRA-LENIENT: Forward ANY packet bytes without ANY validation
// This allows the proxy to relay ALL Meshtastic packets regardless of:
// - Region (frequency)
// - Channel (sync word)
// - Packet structure
// - Packet length
// We just forward raw bytes - let the receiver decide if it's valid
static bool meshtastic_convertToCanonical(const uint8_t* data, uint8_t len, CanonicalPacket* canonical) {
    if (data == nullptr || canonical == nullptr) {
        return false;
    }
    
    // Accept packets of ANY length (even 0 bytes, though that's unlikely)
    // No validation - just forward everything
    if (len > 255) {
        len = 255; // Clamp to max uint8_t
    }
    
    canonical_packet_init(canonical);
    
    // For Meshtastic relay: just copy raw packet bytes as payload
    // This preserves the entire packet structure for forwarding
    canonical->payloadLength = len;
    if (canonical->payloadLength > CANONICAL_MAX_PAYLOAD) {
        canonical->payloadLength = CANONICAL_MAX_PAYLOAD; // Truncate if too large
    }
    if (canonical->payloadLength > 0) {
        memcpy(canonical->payload, data, canonical->payloadLength);
    }
    
    // Set minimal canonical fields (we don't parse, so use defaults)
    canonical->sourceAddress = 0;
    canonical->destinationAddress = 0xFFFFFFFF; // Assume broadcast
    canonical->packetId = 0;
    canonical->hopLimit = 0;
    canonical->wantAck = false;
    canonical->viaMqtt = false;
    canonical->channel = 0;
    canonical->routeType = CANONICAL_ROUTE_BROADCAST; // Assume broadcast for relay
    canonical->messageType = CANONICAL_MSG_DATA;
    canonical->version = 1;
    
    return true;
}

// Convert canonical format to Meshtastic packet
// SIMPLIFIED: Just copy raw packet bytes back (reverse of convertToCanonical)
// This preserves the original Meshtastic packet structure
static bool meshtastic_convertFromCanonical(const CanonicalPacket* canonical, uint8_t* output, uint8_t* outputLen) {
    if (canonical == nullptr || output == nullptr || outputLen == nullptr) {
        return false;
    }
    
    if (!canonical_packet_isValid(canonical)) {
        return false;
    }
    
    // For Meshtastic relay: just copy the raw packet bytes back
    // The payload contains the original Meshtastic packet
    if (canonical->payloadLength == 0 || canonical->payloadLength > MAX_MESHTASTIC_PACKET_SIZE) {
        return false;
    }
    
    memcpy(output, canonical->payload, canonical->payloadLength);
    *outputLen = canonical->payloadLength;
    
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
