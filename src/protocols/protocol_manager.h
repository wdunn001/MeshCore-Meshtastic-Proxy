#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Protocol Manager Interface
 * 
 * Manages protocol enumeration, configuration, and switching.
 * Each protocol provides its own configuration implementation.
 */

// Protocol enumeration
typedef enum {
    PROTOCOL_MESHCORE = 0,
    PROTOCOL_MESHTASTIC = 1,
    PROTOCOL_COUNT = 2
} ProtocolId;

// Protocol configuration structure
typedef struct {
    uint32_t frequencyHz;
    uint8_t bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t syncWord;
    uint16_t preambleLength;
    bool implicitHeader;
    bool invertIQ;
    bool crcEnabled;
} ProtocolConfig;

// Protocol interface - each protocol implements these functions
typedef struct {
    ProtocolId id;
    const char* name;
    void (*configure)(const ProtocolConfig* config);
    uint8_t (*getMaxPacketSize)();
    bool (*parsePacket)(const uint8_t* data, uint8_t len, void* packet);
    bool (*convertToOther)(const void* packet, uint8_t* output, uint8_t* outputLen);
} ProtocolInterface;

// Protocol Manager API
void protocol_manager_init();
void protocol_manager_configure(ProtocolId protocol, const ProtocolConfig* config);
ProtocolInterface* protocol_manager_getInterface(ProtocolId protocol);
ProtocolConfig* protocol_manager_getConfig(ProtocolId protocol);
void protocol_manager_setFrequency(ProtocolId protocol, uint32_t freqHz);
void protocol_manager_setBandwidth(ProtocolId protocol, uint8_t bw);

#endif // PROTOCOL_MANAGER_H
