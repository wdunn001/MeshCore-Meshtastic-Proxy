#ifndef PROTOCOL_INTERFACE_H
#define PROTOCOL_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol_manager.h"
#include "canonical_packet.h"

/**
 * Protocol Interface
 * 
 * Standard interface that all protocols must implement.
 * Provides packet handling, conversion, and state management.
 */

// Protocol statistics
typedef struct {
    uint32_t rxCount;
    uint32_t txCount;
    uint32_t parseErrors;
    uint32_t conversionErrors;
} ProtocolStats;

// Protocol runtime state (runtime state for a protocol instance)
typedef struct {
    ProtocolId id;
    ProtocolStats stats;
    ProtocolConfig config;
    bool isActive;
} ProtocolRuntimeState;

// Forward declarations
struct ProtocolInterfaceImpl;

// Protocol interface structure - each protocol provides an implementation
typedef struct ProtocolInterfaceImpl {
    ProtocolId id;
    const char* name;
    
    // Configuration
    void (*configure)(const ProtocolConfig* config);
    uint8_t (*getMaxPacketSize)();
    
    // Packet handling
    bool (*parsePacket)(const uint8_t* data, uint8_t len, void* packet);
    bool (*handlePacket)(const uint8_t* data, uint8_t len, ProtocolRuntimeState* state, uint8_t* output, uint8_t* outputLen);
    
    // Conversion to/from canonical format
    // Convert FROM this protocol TO canonical format (when receiving)
    bool (*convertToCanonical)(const uint8_t* data, uint8_t len, CanonicalPacket* canonical);
    
    // Convert FROM canonical format TO this protocol (when transmitting)
    bool (*convertFromCanonical)(const CanonicalPacket* canonical, uint8_t* output, uint8_t* outputLen);
    
    // State management
    void (*initState)(ProtocolRuntimeState* state);
    void (*cleanupState)(ProtocolRuntimeState* state);
    
    // Statistics
    void (*updateStats)(ProtocolRuntimeState* state, bool rx, bool tx, bool parseError, bool convError);
    
    // Test packet generation
    uint8_t (*generateTestPacket)(uint8_t* buffer, uint8_t* len);
} ProtocolInterfaceImpl;

// Protocol interface API
ProtocolInterfaceImpl* protocol_interface_get(ProtocolId id);
void protocol_interface_initState(ProtocolId id, ProtocolRuntimeState* state);
void protocol_interface_cleanupState(ProtocolRuntimeState* state);

#endif // PROTOCOL_INTERFACE_H
