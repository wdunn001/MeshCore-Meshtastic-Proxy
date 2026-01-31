#include "protocol_manager.h"
#include "meshcore/meshcore_handler.h"
#include "meshcore/config.h"
#include "meshtastic/meshtastic_handler.h"
#include "meshtastic/config.h"
#include <Arduino.h>

// Protocol configurations (runtime modifiable)
// Note: Radio configuration is handled by protocol implementations via radio_interface
static ProtocolConfig protocolConfigs[PROTOCOL_COUNT];

void protocol_manager_init() {
    // Initialize MeshCore config with defaults
    protocolConfigs[PROTOCOL_MESHCORE].frequencyHz = MESHCORE_DEFAULT_FREQUENCY_HZ;
    protocolConfigs[PROTOCOL_MESHCORE].bandwidth = MESHCORE_BW;
    protocolConfigs[PROTOCOL_MESHCORE].spreadingFactor = MESHCORE_SF;
    protocolConfigs[PROTOCOL_MESHCORE].codingRate = MESHCORE_CR;
    protocolConfigs[PROTOCOL_MESHCORE].syncWord = MESHCORE_SYNC_WORD;
    protocolConfigs[PROTOCOL_MESHCORE].preambleLength = MESHCORE_PREAMBLE;
    protocolConfigs[PROTOCOL_MESHCORE].implicitHeader = true;
    protocolConfigs[PROTOCOL_MESHCORE].invertIQ = false;
    protocolConfigs[PROTOCOL_MESHCORE].crcEnabled = true;
    
    // Initialize Meshtastic config with defaults
    protocolConfigs[PROTOCOL_MESHTASTIC].frequencyHz = MESHTASTIC_DEFAULT_FREQUENCY_HZ;
    protocolConfigs[PROTOCOL_MESHTASTIC].bandwidth = MESHTASTIC_BW;
    protocolConfigs[PROTOCOL_MESHTASTIC].spreadingFactor = MESHTASTIC_SF;
    protocolConfigs[PROTOCOL_MESHTASTIC].codingRate = MESHTASTIC_CR;
    protocolConfigs[PROTOCOL_MESHTASTIC].syncWord = MESHTASTIC_SYNC_WORD;
    protocolConfigs[PROTOCOL_MESHTASTIC].preambleLength = MESHTASTIC_PREAMBLE;
    protocolConfigs[PROTOCOL_MESHTASTIC].implicitHeader = true;
    protocolConfigs[PROTOCOL_MESHTASTIC].invertIQ = true;
    protocolConfigs[PROTOCOL_MESHTASTIC].crcEnabled = true;
}

void protocol_manager_configure(ProtocolId protocol, const ProtocolConfig* config) {
    if (protocol >= PROTOCOL_COUNT || config == nullptr) {
        return;
    }
    protocolConfigs[protocol] = *config;
}

ProtocolInterface* protocol_manager_getInterface(ProtocolId protocol) {
    // Deprecated: Use protocol_interface_get() instead
    // This function is kept for backward compatibility but returns nullptr
    // Radio configuration is handled by protocol implementations via radio_interface
    (void)protocol;
    return nullptr;
}

ProtocolConfig* protocol_manager_getConfig(ProtocolId protocol) {
    if (protocol >= PROTOCOL_COUNT) {
        return nullptr;
    }
    return &protocolConfigs[protocol];
}

void protocol_manager_setFrequency(ProtocolId protocol, uint32_t freqHz) {
    if (protocol >= PROTOCOL_COUNT) {
        return;
    }
    protocolConfigs[protocol].frequencyHz = freqHz;
}

void protocol_manager_setBandwidth(ProtocolId protocol, uint8_t bw) {
    if (protocol >= PROTOCOL_COUNT) {
        return;
    }
    protocolConfigs[protocol].bandwidth = bw;
}
