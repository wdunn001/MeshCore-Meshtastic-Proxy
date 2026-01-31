// Protocol Registry - Generic protocol management for web interface
// Maps protocol IDs to display names and manages protocol state

const ProtocolRegistry = {
    // Protocol definitions (should match firmware PROTOCOL_COUNT)
    PROTOCOL_COUNT: 2,
    
    // Protocol ID constants (should match firmware ProtocolId enum)
    PROTOCOL_MESHCORE: 0,
    PROTOCOL_MESHTASTIC: 1,
    
    // Protocol display names (can be updated from device if needed)
    protocolNames: {
        0: 'MeshCore',
        1: 'Meshtastic'
    },
    
    // Get protocol name by ID
    getName(protocolId) {
        return this.protocolNames[protocolId] || `Protocol ${protocolId}`;
    },
    
    // Get all protocol IDs
    getAllProtocolIds() {
        const ids = [];
        for (let i = 0; i < this.PROTOCOL_COUNT; i++) {
            ids.push(i);
        }
        return ids;
    },
    
    // Check if protocol ID is valid
    isValidProtocolId(id) {
        return id >= 0 && id < this.PROTOCOL_COUNT;
    }
};

// Make ProtocolRegistry available globally
window.ProtocolRegistry = ProtocolRegistry;
