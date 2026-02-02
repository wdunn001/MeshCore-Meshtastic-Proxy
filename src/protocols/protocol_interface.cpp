#include "protocol_interface.h"
#include "meshcore/protocol_meshcore.h"
#include "meshtastic/protocol_meshtastic.h"

/**
 * Protocol Interface Implementation
 * 
 * Provides access to protocol implementations based on ProtocolId.
 */

ProtocolInterfaceImpl* protocol_interface_get(ProtocolId id) {
    switch (id) {
        case PROTOCOL_MESHCORE:
            return meshcore_getProtocolInterface();
        case PROTOCOL_MESHTASTIC:
            return meshtastic_getProtocolInterface();
        default:
            return nullptr;
    }
}

void protocol_interface_initState(ProtocolId id, ProtocolRuntimeState* state) {
    if (state == nullptr) {
        return;
    }
    
    ProtocolInterfaceImpl* iface = protocol_interface_get(id);
    if (iface != nullptr && iface->initState != nullptr) {
        iface->initState(state);
    }
}

void protocol_interface_cleanupState(ProtocolRuntimeState* state) {
    if (state == nullptr) {
        return;
    }
    
    ProtocolInterfaceImpl* iface = protocol_interface_get(state->id);
    if (iface != nullptr && iface->cleanupState != nullptr) {
        iface->cleanupState(state);
    }
}
