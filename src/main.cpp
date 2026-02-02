#include <Arduino.h>
#include <string.h>
#include "config.h"  // Generic config (PROTOCOL_SWITCH_INTERVAL_MS_DEFAULT)
#include "protocols/protocol_state.h"
#include "radio/radio_interface.h"
#include "protocols/protocol_interface.h"
#include "protocols/protocol_manager.h"
#include "protocols/canonical_packet.h"
#include "platforms/platform_interface.h"
#include "usb_comm.h"

// Global state variables (accessible from usb_comm.cpp)
ProtocolId rx_protocol = (ProtocolId)0;      // Currently listening protocol
ProtocolId tx_protocols[PROTOCOL_COUNT];         // Protocols to transmit to (relay targets)
uint8_t tx_protocol_count = 0;                   // Number of active transmit protocols
unsigned long lastProtocolSwitch = 0;
uint16_t protocolSwitchIntervalMs = PROTOCOL_SWITCH_INTERVAL_MS_DEFAULT;
bool autoSwitchEnabled = true;
static ProtocolId lastConfiguredProtocol = PROTOCOL_COUNT; // Track last configured protocol to avoid spam
uint8_t desiredProtocolMode = 2; // 0=MeshCore, 1=Meshtastic, 2=Auto-Switch

// Legacy ProtocolState for USB comm compatibility
ProtocolState currentProtocol = LISTENING_MESHCORE;

// Protocol runtime state objects (one per protocol) - accessible from usb_comm.cpp
ProtocolRuntimeState protocolStates[PROTOCOL_COUNT];

// Protocol configurations are now stored in protocolStates[].config
// Legacy variables removed - access via protocolStates[id].config.frequencyHz/bandwidth

// Packet buffers - use single shared buffer to save RAM
uint8_t rxBuffer[255];
uint8_t txBuffer[255];
uint8_t convertedBuffer[255];

// Helper to safely increment counters with overflow protection
#define SAFE_INCREMENT(counter) do { \
    if (counter < 2147483647U) { \
        counter++; \
    } else { \
        counter = 2147483647U; \
        static bool overflowLogged = false; \
        if (!overflowLogged) { \
            usbComm.sendDebugLog("ERR: Counter overflow - capped at max"); \
            overflowLogged = true; \
        } \
    } \
} while(0)

// Interrupt flag
volatile bool packetReceived = false;

void onPacketReceived() {
    packetReceived = true;
}

void configureProtocol(ProtocolId protocol) {
    // Skip if already configured for this protocol (prevents spam)
    if (protocol == lastConfiguredProtocol) {
        return;
    }
    
    ProtocolInterfaceImpl* iface = protocol_interface_get(protocol);
    ProtocolConfig* config = protocol_manager_getConfig(protocol);
    
    if (iface && config) {
        // Configure radio - no debug log here (logged at protocol switch level instead)
        iface->configure(config);
        protocolStates[protocol].isActive = true;
        lastConfiguredProtocol = protocol; // Remember what we configured
        
        // Debug: Log when Meshtastic is configured (to help diagnose reception issues)
        if (protocol == PROTOCOL_MESHTASTIC) {
            char debugMsg[100];
            snprintf(debugMsg, sizeof(debugMsg), "Meshtastic RX: %.3f MHz SF=%d BW=%d Sync=0x%02X", 
                     config->frequencyHz / 1000000.0, config->spreadingFactor, config->bandwidth, config->syncWord);
            usbComm.sendDebugLog(debugMsg);
        }
    }
}

void update_tx_protocols(ProtocolId rx_protocol) {
    // With canonical format, we can relay to all other protocols
    tx_protocol_count = 0;
    for (ProtocolId id = (ProtocolId)0; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        if (id != rx_protocol) {
            tx_protocols[tx_protocol_count++] = id;
        }
    }
}

void set_tx_protocols(uint8_t bitmask) {
    // Set transmit protocols from bitmask (bit 0 = MeshCore, bit 1 = Meshtastic)
    tx_protocol_count = 0;
    for (ProtocolId id = (ProtocolId)0; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        uint8_t bit = (uint8_t)id;
        if ((bitmask >> bit) & 0x01) {
            // Don't allow transmitting to the same protocol we're listening to
            if (id != rx_protocol) {
                tx_protocols[tx_protocol_count++] = id;
            }
        }
    }
}

void set_rx_protocol(ProtocolId protocol) {
    // Set the listen protocol
    rx_protocol = protocol;
    currentProtocol = (ProtocolState)protocol;
    
    // Debug: Log RX protocol change
    ProtocolInterfaceImpl* iface = protocol_interface_get(protocol);
    const char* protocolName = iface && iface->name ? iface->name : "Unknown";
    char debugMsg[50];
    snprintf(debugMsg, sizeof(debugMsg), "RX protocol set to: %s", protocolName);
    usbComm.sendDebugLog(debugMsg);
    
    // DON'T auto-update TX protocols - user controls them explicitly via CMD_SET_TX_PROTOCOLS
    // The set_tx_protocols() function already prevents TX on the same protocol as RX
    
    // Configure radio for new listen protocol
    // Force reconfiguration by resetting lastConfiguredProtocol
    lastConfiguredProtocol = PROTOCOL_COUNT;
    configureProtocol(protocol);
    lastProtocolSwitch = millis();
}

bool receivePacket(uint8_t* buffer, uint8_t* len) {
    if (!radio_isPacketReceived()) {
        return false;
    }
    
    // Check for packet errors (CRC, header) BEFORE reading packet (like Meshtastic does)
    // This filters out noise packets that fail CRC or have header errors
    // TEMPORARILY DISABLED: May be too aggressive if transmitting devices don't use CRC
    // TODO: Make CRC checking configurable or check CRC status after reading packet
    /*
    if (radio_hasPacketErrors()) {
        // Packet has CRC or header errors - reject this corrupted/noise packet
        // Debug: Log the rejection
        ProtocolInterfaceImpl* rxIface = protocol_interface_get(rx_protocol);
        const char* rxName = rxIface && rxIface->name ? rxIface->name : "Unknown";
        char errorMsg[50];
        snprintf(errorMsg, sizeof(errorMsg), "RX %s: CRC/header error - rejected", rxName);
        usbComm.sendDebugLog(errorMsg);
        radio_clearIrqFlags();
        radio_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    */
    
    // Follow RadioLib pattern: get packet length FIRST (reads RX buffer status)
    // This gives us both the length and ensures we have valid packet data
    *len = radio_getPacketLength();
    
    // Validate length BEFORE reading - reject 255 as it usually indicates buffer corruption
    if (*len == 0 || *len == 255 || *len > 255) {
        // Invalid length - clear and return
        radio_clearIrqFlags();
        radio_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    
    // Check against protocol max packet size
    ProtocolInterfaceImpl* currentIface = protocol_interface_get(rx_protocol);
    if (currentIface && *len > currentIface->getMaxPacketSize()) {
        radio_clearIrqFlags();
        radio_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    
    // Read FIFO with the correct length
    // Note: readFifo reads RX buffer status again internally, but we trust our length
    // and readFifo will clamp to our length if RX buffer status is corrupted
    radio_readFifo(buffer, *len);
    
    // Verify the length wasn't corrupted during read
    // readFifo sets last_packet_length, so check it matches
    uint8_t verifyLen = radio_getPacketLength();
    if (verifyLen != *len && (verifyLen == 255 || verifyLen == 0)) {
        // Length was corrupted during read - reject this packet
        radio_clearIrqFlags();
        radio_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    
    // Clear IRQ flags after reading
    radio_clearIrqFlags();
    
    return true;
}

bool transmitPacket(ProtocolId protocol, const uint8_t* data, uint8_t len) {
    if (len == 0 || len > 255) {
        return false;
    }
    
    // Save current listening protocol to restore after TX
    ProtocolId savedRxProtocol = rx_protocol;
    
    // Configure radio for target protocol
    configureProtocol(protocol);
    delay(20);
    
    radio_setPower(platform_getMaxTxPower());
    radio_setCrc(true);
    
    radio_writeFifo((uint8_t*)data, len);
    radio_clearIrqFlags();
    radio_setMode(MODE_TX);
    
    // Wait for transmission to complete (use timeout - IRQ flags are platform-specific)
    // LoRa packets take: (preamble + payload) * symbol_time
    // For SF7: symbol_time ~= 1ms, so even long packets complete in <100ms
    // Use 500ms timeout to be safe
    unsigned long startTime = millis();
    while (millis() - startTime < 500) {
        delay(1);
    }
    
    radio_clearIrqFlags();
    radio_setMode(MODE_STDBY);
    delay(10);
    
    // CRITICAL: Switch back to listening protocol and RX mode
    // Reset lastConfiguredProtocol so we reconfigure even if it's the same protocol
    // This ensures we always return to RX mode after TX
    lastConfiguredProtocol = PROTOCOL_COUNT; // Force reconfiguration
    configureProtocol(savedRxProtocol);
    
    // Ensure we're in RX mode (configureProtocol should do this, but be explicit)
    radio_setMode(MODE_RX_CONTINUOUS);
    
    return true; // TX completed (timeout-based, no IRQ check needed)
}

void handlePacket(ProtocolId protocol, const uint8_t* data, uint8_t len) {
    ProtocolInterfaceImpl* iface = protocol_interface_get(protocol);
    ProtocolRuntimeState* state = &protocolStates[protocol];
    
    if (iface == nullptr || state == nullptr || iface->convertToCanonical == nullptr) {
        return;
    }
    
    // Convert received packet to canonical format
    CanonicalPacket canonical;
    if (!iface->convertToCanonical(data, len, &canonical)) {
        state->stats.parseErrors++;
        // For Meshtastic: be more lenient - if it's Meshtastic protocol, still try to forward
        if (protocol == PROTOCOL_MESHTASTIC) {
            // Meshtastic relay mode: forward even if conversion fails
            // Just copy raw bytes directly
            canonical_packet_init(&canonical);
            canonical.payloadLength = (len > CANONICAL_MAX_PAYLOAD) ? CANONICAL_MAX_PAYLOAD : len;
            memcpy(canonical.payload, data, canonical.payloadLength);
            canonical.messageType = CANONICAL_MSG_DATA;
            canonical.routeType = CANONICAL_ROUTE_BROADCAST;
            canonical.version = 1;
        } else {
            char errorMsg[60];
            snprintf(errorMsg, sizeof(errorMsg), "ERR: %s parse fail len=%d (first bytes: %02X %02X %02X)", 
                     iface->name, len, len > 0 ? data[0] : 0, len > 1 ? data[1] : 0, len > 2 ? data[2] : 0);
            usbComm.sendDebugLog(errorMsg);
            return;
        }
    }
    
    // Debug: Log successful parse (or relay for Meshtastic)
    char successMsg[50];
    snprintf(successMsg, sizeof(successMsg), "%s %s: %d bytes", iface->name, 
             protocol == PROTOCOL_MESHTASTIC ? "relay" : "parse OK", len);
    usbComm.sendDebugLog(successMsg);
    
    // Filter MQTT packets (only if we successfully parsed and detected MQTT)
    // For Meshtastic relay mode, we don't parse so we can't filter MQTT
    if (protocol != PROTOCOL_MESHTASTIC && canonical.viaMqtt) {
        return;  // Silently drop MQTT packets (only for parsed protocols)
    }
    
    state->stats.rxCount++;
    
    // Relay to all other protocols using canonical format
    for (uint8_t i = 0; i < tx_protocol_count; i++) {
        ProtocolId targetProtocol = tx_protocols[i];
        ProtocolInterfaceImpl* targetIface = protocol_interface_get(targetProtocol);
        
        if (targetIface == nullptr || targetIface->convertFromCanonical == nullptr) {
            continue;
        }
        
        // Convert from canonical format to target protocol
        uint8_t convertedLen = 0;
        if (!targetIface->convertFromCanonical(&canonical, txBuffer, &convertedLen)) {
            state->stats.conversionErrors++;
            continue;
        }
        
        // Transmit to target protocol
        if (transmitPacket(targetProtocol, txBuffer, convertedLen)) {
            ProtocolRuntimeState* targetState = &protocolStates[targetProtocol];
            if (targetState != nullptr && targetIface->updateStats != nullptr) {
                targetIface->updateStats(targetState, false, true, false, false);
            }
            platform_blinkLed(10);
        } else {
            usbComm.sendDebugLog("ERR: TX fail");
        }
    }
    
    // Switch back to listening protocol
    configureProtocol(protocol);
}

void switchProtocol() {
    if (!autoSwitchEnabled || protocolSwitchIntervalMs == 0) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastProtocolSwitch >= protocolSwitchIntervalMs) {
        // Rotate through all available protocols
        ProtocolId nextProtocol = (ProtocolId)((rx_protocol + 1) % PROTOCOL_COUNT);
        rx_protocol = nextProtocol;
        
        // Update legacy ProtocolState for USB comm compatibility
        currentProtocol = (ProtocolState)rx_protocol;
        
        // Update transmit protocol list
        update_tx_protocols(rx_protocol);
        
        // Configure radio for new listening protocol
        configureProtocol(rx_protocol);
        
        lastProtocolSwitch = currentTime;
    }
}

void setProtocol(ProtocolState protocol) {
    ProtocolId id = (ProtocolId)protocol;
    set_rx_protocol(id);
}

void printStatistics() {
    uint32_t totalConversionErrors = 0;
    uint32_t totalParseErrors = 0;
    
    // Build statistics string for debug log (sent via binary protocol, not Serial.print)
    char statsBuffer[256];
    int offset = 0;
    
    // Print statistics for each protocol dynamically
    for (ProtocolId id = (ProtocolId)0; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        ProtocolInterfaceImpl* iface = protocol_interface_get(id);
        ProtocolRuntimeState* state = &protocolStates[id];
        
        if (iface != nullptr && state != nullptr) {
            const char* name = iface->name ? iface->name : "Unknown";
            
            offset += snprintf(statsBuffer + offset, sizeof(statsBuffer) - offset, 
                "%s RX: %lu TX: %lu", name, state->stats.rxCount, state->stats.txCount);
            
            if (id < PROTOCOL_COUNT - 1) {
                offset += snprintf(statsBuffer + offset, sizeof(statsBuffer) - offset, " | ");
            }
            
            totalConversionErrors += state->stats.conversionErrors;
            totalParseErrors += state->stats.parseErrors;
        }
    }
    
    snprintf(statsBuffer + offset, sizeof(statsBuffer) - offset,
        " | Errors: %lu (Conv: %lu, Parse: %lu)",
        totalConversionErrors + totalParseErrors, totalConversionErrors, totalParseErrors);
    
    // Send via USB comm binary protocol instead of Serial.print to avoid interfering with protocol
    usbComm.sendDebugLog(statsBuffer);
}

void sendTestMessage(ProtocolId protocol) {
    ProtocolInterfaceImpl* iface = protocol_interface_get(protocol);
    if (iface == nullptr || iface->generateTestPacket == nullptr) {
        return;
    }
    
    uint8_t testBuffer[64];
    uint8_t testLen = 0;
    char debugMsg[64];
    
    iface->generateTestPacket(testBuffer, &testLen);
    
    ProtocolConfig* config = protocol_manager_getConfig(protocol);
    snprintf(debugMsg, sizeof(debugMsg), "TX %s: %d bytes @ %.3f MHz", 
             iface->name, testLen, config->frequencyHz / 1000000.0);
    usbComm.sendDebugLog(debugMsg);
    
    if (transmitPacket(protocol, testBuffer, testLen)) {
        ProtocolRuntimeState* state = &protocolStates[protocol];
        if (state != nullptr && iface->updateStats != nullptr) {
            iface->updateStats(state, false, true, false, false);
        }
        platform_blinkLed(50);
        usbComm.sendDebugLog("Test TX success");
    } else {
        usbComm.sendDebugLog("Test TX failed");
    }
    
    // Switch back to listening protocol
    configureProtocol(rx_protocol);
}

void setup() {
    // Platform-specific initialization (LED, USB Serial, etc.)
    platform_init();
    
    // Initialize USB Serial (platform-specific config)
    Serial.begin(platform_getSerialBaud());
    
    // Wait for USB Serial connection (important for Arduino Leonardo/LoRa32u4II)
    // On boards with native USB, Serial might not be immediately available
    // However, don't block forever - timeout after 3 seconds to allow operation without USB
    #ifdef ARDUINO_AVR_LEONARDO
        uint32_t serialWaitStart = millis();
        while (!Serial && (millis() - serialWaitStart < 3000)) {
            delay(10); // Wait for USB Serial to be ready (max 3 seconds)
        }
    #endif
    
    delay(2000); // Give serial monitor time to connect
    
    Serial.println("MeshCore-Meshtastic Proxy Starting...");
    
    usbComm.init();
    
    if (!radio_init()) {
        Serial.println("ERROR: Radio initialization failed!");
        usbComm.sendDebugLog("ERR: Radio init failed - check SPI connections");
        while(1) {
            platform_blinkLed(100);
        }
    }
    
    Serial.println("Radio initialized successfully");
    radio_attachInterrupt(onPacketReceived);
    
    // Initialize protocol manager
    protocol_manager_init();
    
    // Initialize protocol states dynamically
    for (ProtocolId id = (ProtocolId)0; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        protocol_interface_initState(id, &protocolStates[id]);
    }
    
    // Protocol configs are stored in protocolStates[].config and can be accessed
    // via protocol_manager_getConfig() or directly from protocolStates[]
    
    // Disable auto-switch - always listen to MeshCore (protocol 0)
    protocolSwitchIntervalMs = 0;
    autoSwitchEnabled = false;
    desiredProtocolMode = 0; // MeshCore
    
    // Set RX protocol to MeshCore (protocol 0)
    rx_protocol = (ProtocolId)0; // MeshCore
    currentProtocol = (ProtocolState)rx_protocol;
    
    // Set TX protocols to both MeshCore (0) and Meshtastic (1)
    tx_protocol_count = 0;
    tx_protocols[tx_protocol_count++] = (ProtocolId)0; // MeshCore
    tx_protocols[tx_protocol_count++] = (ProtocolId)1; // Meshtastic
    
    // Configure radio for listen protocol (MeshCore)
    configureProtocol(rx_protocol);
    lastProtocolSwitch = millis();
    
    // Print protocol information dynamically
    Serial.println("Proxy ready. Listening for packets...");
    Serial.println("Configured protocols:");
    for (ProtocolId id = (ProtocolId)0; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        ProtocolInterfaceImpl* iface = protocol_interface_get(id);
        ProtocolConfig* config = protocol_manager_getConfig(id);
        
        if (iface != nullptr && config != nullptr) {
            Serial.print("  ");
            Serial.print(iface->name);
            Serial.print(" @ ");
            Serial.print(config->frequencyHz / 1000000.0);
            Serial.println(" MHz");
        }
    }
    
    delay(1000);
    // Send test messages for all transmit protocols (relay targets)
    for (uint8_t i = 0; i < tx_protocol_count; i++) {
        sendTestMessage(tx_protocols[i]);
        delay(500);
    }
}

void loop() {
    usbComm.process();
    
    // Auto-switch disabled - user controls RX/TX protocols via USB commands
    // No automatic protocol switching - respect user's RX protocol selection
    
    // Check for packet via polling (backup to interrupt)
    static unsigned long lastDebugLog = 0;
    unsigned long now = millis();
    
    // Debug: Log current listening state every 30 seconds (reduced frequency to avoid browser issues)
    if (now - lastDebugLog > 30000) {
        lastDebugLog = now;
        ProtocolInterfaceImpl* rxIface = protocol_interface_get(rx_protocol);
        ProtocolConfig* config = protocol_manager_getConfig(rx_protocol);
        if (rxIface && config) {
            char stateMsg[100];
            snprintf(stateMsg, sizeof(stateMsg), "Listening: %s @ %.3f MHz SF=%d BW=%d Sync=0x%02X", 
                     rxIface->name, config->frequencyHz / 1000000.0, 
                     config->spreadingFactor, config->bandwidth, config->syncWord);
            usbComm.sendDebugLog(stateMsg);
        }
    }
    
    if (radio_isPacketReceived()) {
        packetReceived = true;
    }
    
    if (packetReceived) {
        packetReceived = false;
        
        uint8_t packetLen = 0;
        if (receivePacket(rxBuffer, &packetLen)) {
            // Verify packetLen was actually set (should be > 0 and <= 255)
            if (packetLen > 0 && packetLen <= 255) {
                int16_t rssi = radio_getRssi();
                int8_t snr = radio_getSnr();
                
                // Filter out noise packets (RSSI -127 dBm means no signal, just noise)
                // Also filter packets with invalid length (255 usually means buffer corruption)
                if (rssi <= -127 || packetLen == 255) {
                    // Noise or corrupted packet - ignore it
                    radio_clearIrqFlags();
                    radio_setMode(MODE_RX_CONTINUOUS);
                    return; // Skip processing this packet
                }
                
                // Debug: Log packet reception
                ProtocolInterfaceImpl* rxIface = protocol_interface_get(rx_protocol);
                const char* rxName = rxIface && rxIface->name ? rxIface->name : "Unknown";
                char debugMsg[60];
                snprintf(debugMsg, sizeof(debugMsg), "RX %s: RSSI=%d SNR=%d Len=%d", rxName, rssi, snr, packetLen);
                usbComm.sendDebugLog(debugMsg);
                
                // Send packet to web interface
                usbComm.sendRxPacket(rx_protocol, rssi, snr, rxBuffer, packetLen);
                
                // Handle packet (relay to other protocols)
                handlePacket(rx_protocol, rxBuffer, packetLen);
                
                radio_setMode(MODE_RX_CONTINUOUS);
            } else {
                // Invalid packet length - clear and reconfigure
                radio_clearIrqFlags();
                configureProtocol(rx_protocol);
            }
        } else {
            // Packet reception failed - reconfigure radio
            radio_clearIrqFlags();
            configureProtocol(rx_protocol);
        }
    }
    
    // Statistics are sent via RESP_STATS when requested by the client
    // No need to send debug log statistics periodically - they're redundant
    
    delay(1);
}
