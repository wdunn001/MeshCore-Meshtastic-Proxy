#include "usb_comm.h"
#include "config.h"
#include "protocols/protocol_state.h"
#include "protocols/protocol_manager.h"
#include "protocols/protocol_interface.h"
#include "radio/radio_interface.h"
#include <Arduino.h>
#include <string.h>

// Forward declarations from main.cpp
extern ProtocolId rx_protocol;      // Currently listening protocol
extern ProtocolId tx_protocols[];  // Protocols to transmit to
extern uint8_t tx_protocol_count;   // Number of transmit protocols
extern uint16_t protocolSwitchIntervalMs; // Configurable switch interval
extern bool autoSwitchEnabled; // Auto-switching enabled flag
// desiredProtocolMode always equals rx_protocol since auto-switch is disabled
// Kept for web interface compatibility (0=MeshCore, 1=Meshtastic)
extern uint8_t desiredProtocolMode;
extern ProtocolRuntimeState protocolStates[];  // Protocol runtime state objects
extern bool radioInitialized; // Track if radio initialized successfully

// Forward declarations
void sendTestMessage(ProtocolId protocol);
void setProtocol(ProtocolState protocol);
void configureProtocol(ProtocolId protocol);
void set_rx_protocol(ProtocolId protocol);
void set_tx_protocols(uint8_t bitmask);

USBComm usbComm;

void USBComm::init() {
    // Serial is initialized in main.cpp setup()
}

void USBComm::process() {
    uint8_t cmd;
    uint8_t data[64];
    uint8_t len;
    
    // Read and process up to 3 commands per loop iteration
    for (int i = 0; i < 3; i++) {
        if (!readCommand(&cmd, data, &len)) {
            break; // No more commands available
        }
        handleCommand(cmd, data, len);
    }
}

bool USBComm::readCommand(uint8_t* cmd, uint8_t* data, uint8_t* len) {
    // Non-blocking read: check if header is available
    if (Serial.available() < 2) {
        return false; // Not enough data for header
    }
    
    // Peek at the first byte to check if it's a valid command
    uint8_t peekCmd = Serial.peek();
    if (peekCmd < 0x01 || peekCmd > 0x0A) {
        // Invalid command ID - discard this byte and try to resync
        Serial.read(); // Discard invalid byte
        return false; // Return false to try again on next call
    }
    
    *cmd = Serial.read();
    *len = Serial.read();
    
    // Validate command ID (should be 0x01-0x0A) - double check after reading
    if (*cmd < 0x01 || *cmd > 0x0A) {
        // Invalid command ID - we've already consumed both bytes, return false to resync
        return false;
    }
    
    // Validate length
    if (*len > 64) {
        // Invalid length - discard both bytes and return false
        return false;
    }
    
    // Read payload if present
    if (*len > 0) {
        // Wait for payload data with timeout
        uint8_t bytesRead = 0;
        uint32_t startTime = millis();
        while (bytesRead < *len && (millis() - startTime) < 100) {
            if (Serial.available() > 0) {
                data[bytesRead++] = Serial.read();
            } else {
                delay(1); // Small delay to allow more data to arrive
            }
        }
        
        if (bytesRead < *len) {
            // Timeout - incomplete command, discard what we read
            return false;
        }
    }
    
    return true;
}

void USBComm::handleCommand(uint8_t cmd, uint8_t* data, uint8_t len) {
    switch (cmd) {
        case CMD_GET_INFO:
            // Immediately send info response
            sendInfo();
            break;
            
        case CMD_GET_STATS:
            sendStats();
            break;
            
        case CMD_SET_FREQUENCY:
            if (len == 4) {
                // Frequency change would require reconfiguration - for future implementation
                // Note: freq value decoded but not used yet
                (void)data; // Suppress unused parameter warning
                sendDebugLog("Freq change req");
            }
            break;
            
        case CMD_SET_PROTOCOL:
            if (len == 1) {
                // 0 = First protocol, 1 = Second protocol, 2 = Auto-Switch
                uint8_t protocol = data[0];
                if (protocol < PROTOCOL_COUNT) {
                    desiredProtocolMode = protocol;
                    // If auto-switch is disabled, switch immediately
                    if (!autoSwitchEnabled || protocolSwitchIntervalMs == 0) {
                        setProtocol((ProtocolState)protocol);
                    }
                    ProtocolInterfaceImpl* iface = protocol_interface_get((ProtocolId)protocol);
                    if (iface != nullptr) {
                        char msg[20];
                        snprintf(msg, sizeof(msg), "Mode: %s", iface->name);
                        sendDebugLog(msg);
                    }
                } else if (protocol == 2) {
                    desiredProtocolMode = 2;
                    // Enable auto-switch if interval is set
                    if (protocolSwitchIntervalMs > 0) {
                        autoSwitchEnabled = true;
                    }
                    sendDebugLog("Mode: Auto");
                } else {
                    sendDebugLog("ERR: Bad proto");
                }
            }
            break;
            
        case CMD_RESET_STATS:
            // Reset stats for all protocols dynamically
            if (protocolStates != nullptr) {
                for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
                    protocolStates[id].stats.rxCount = 0;
                    protocolStates[id].stats.txCount = 0;
                    protocolStates[id].stats.parseErrors = 0;
                    protocolStates[id].stats.conversionErrors = 0;
                }
            }
            sendDebugLog("Stats reset");
            break;
            
        case CMD_SEND_TEST:
            if (len == 1) {
                // 0 = First protocol, 1 = Second protocol, 2 = All protocols
                uint8_t protocol = data[0];
                if (protocol < PROTOCOL_COUNT) {
                    sendTestMessage((ProtocolId)protocol);
                } else if (protocol == 2) {
                    // Send test messages to all transmit protocols
                    for (uint8_t i = 0; i < tx_protocol_count; i++) {
                        sendTestMessage(tx_protocols[i]);
                        if (i < tx_protocol_count - 1) {
                            delay(200); // Small delay between transmissions
                        }
                    }
                }
            }
            break;
            
        case CMD_SET_SWITCH_INTERVAL:
            if (len == 2) {
                // 2 bytes: low byte, high byte (little-endian)
                uint16_t newInterval = data[0] | (data[1] << 8);
                // Validate range (0 = disabled/off)
                if (newInterval == 0) {
                    protocolSwitchIntervalMs = 0;
                    autoSwitchEnabled = false;
                    // When disabling auto-switch, use current rx_protocol (not desiredProtocolMode which might be 2)
                    // The rx_protocol should already be set correctly via CMD_SET_RX_PROTOCOL
                    // Just ensure it's configured properly
                    configureProtocol(rx_protocol);
                    sendDebugLog("Manual mode");
                } else if (newInterval >= PROTOCOL_SWITCH_INTERVAL_MS_MIN && 
                           newInterval <= PROTOCOL_SWITCH_INTERVAL_MS_MAX) {
                    protocolSwitchIntervalMs = newInterval;
                    autoSwitchEnabled = true;
                    // Update desiredProtocolMode to 2 (auto-switch) when enabling
                    desiredProtocolMode = 2;
                    char msg[50];
                    snprintf(msg, sizeof(msg), "Switch interval set to %d ms", newInterval);
                    sendDebugLog(msg);
                } else {
                    char msg[60];
                    snprintf(msg, sizeof(msg), "Invalid interval: %d (range: 0 or %d-%d ms)", 
                             newInterval, PROTOCOL_SWITCH_INTERVAL_MS_MIN, PROTOCOL_SWITCH_INTERVAL_MS_MAX);
                    sendDebugLog(msg);
                }
            }
            break;
            
        case CMD_SET_PROTOCOL_PARAMS:
            if (len == 6) {
                // Generic command: Set protocol params
                // 1 byte protocol ID + 4 bytes frequency (little-endian) + 1 byte bandwidth
                ProtocolId targetProtocol = (ProtocolId)data[0];
                uint32_t newFreq = (uint32_t)data[1] | ((uint32_t)data[2] << 8) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 24);
                uint8_t newBw = data[5];
                
                // Update protocol state config
                if (protocolStates != nullptr && targetProtocol < PROTOCOL_COUNT) {
                    ProtocolRuntimeState* state = &protocolStates[targetProtocol];
                    
                    // Validate frequency using radio interface capabilities
                    uint32_t minFreq = radio_getMinFrequency();
                    uint32_t maxFreq = radio_getMaxFrequency();
                    if (newFreq >= minFreq && newFreq <= maxFreq) {
                        state->config.frequencyHz = newFreq;
                        protocol_manager_setFrequency(targetProtocol, newFreq);
                        ProtocolInterfaceImpl* iface = protocol_interface_get(targetProtocol);
                        if (iface != nullptr) {
                            char msg[40];
                            snprintf(msg, sizeof(msg), "%s freq updated", iface->name);
                            sendDebugLog(msg);
                        }
                    } else {
                        sendDebugLog("ERR: Invalid freq");
                    }
                    // Validate bandwidth (0-9: 7.8kHz to 500kHz)
                    if (newBw <= 9) {
                        state->config.bandwidth = newBw;
                        protocol_manager_setBandwidth(targetProtocol, newBw);
                        ProtocolInterfaceImpl* iface = protocol_interface_get(targetProtocol);
                        if (iface != nullptr) {
                            char msg[40];
                            snprintf(msg, sizeof(msg), "%s BW updated", iface->name);
                            sendDebugLog(msg);
                        }
                    } else {
                        sendDebugLog("ERR: Invalid BW");
                    }
                    // Reconfigure if currently listening to this protocol
                    if (rx_protocol == targetProtocol) {
                        configureProtocol(targetProtocol);
                    }
                } else {
                    sendDebugLog("ERR: Invalid proto");
                }
            }
            break;
            
        case CMD_SET_RX_PROTOCOL:
            if (len == 1) {
                // Set listen protocol: 1 byte protocol ID
                ProtocolId rxProtocol = (ProtocolId)data[0];
                if (rxProtocol < PROTOCOL_COUNT) {
                    set_rx_protocol(rxProtocol);
                    // desiredProtocolMode is automatically updated in set_rx_protocol()
                    ProtocolInterfaceImpl* iface = protocol_interface_get(rxProtocol);
                    if (iface != nullptr) {
                        char msg[40];
                        snprintf(msg, sizeof(msg), "RX: %s", iface->name);
                        sendDebugLog(msg);
                    }
                } else {
                    sendDebugLog("ERR: Invalid RX proto");
                }
            }
            break;
            
        case CMD_SET_TX_PROTOCOLS:
            if (len == 1) {
                // Set transmit protocols: 1 byte bitmask (bit 0=MeshCore, bit 1=Meshtastic)
                uint8_t txBitmask = data[0];
                set_tx_protocols(txBitmask);
                
                // Build status message safely
                char msg[60];
                int pos = snprintf(msg, sizeof(msg), "TX: ");
                bool first = true;
                for (uint8_t i = 0; i < tx_protocol_count && pos < (int)(sizeof(msg) - 20); i++) {
                    ProtocolInterfaceImpl* iface = protocol_interface_get(tx_protocols[i]);
                    if (iface != nullptr) {
                        if (!first) {
                            pos += snprintf(msg + pos, sizeof(msg) - pos, ", ");
                        }
                        pos += snprintf(msg + pos, sizeof(msg) - pos, "%s", iface->name);
                        first = false;
                    }
                }
                if (tx_protocol_count == 0) {
                    snprintf(msg + pos, sizeof(msg) - pos, "None");
                }
                sendDebugLog(msg);
            }
            break;
            
        default:
            // Unknown command - silently ignore
            break;
    }
}

void USBComm::sendResponse(uint8_t respId, uint8_t* data, uint8_t len) {
    // Always send critical responses (INFO, STATS, ERROR) - don't check buffer
    // For other responses, check buffer space to avoid blocking
    bool isCritical = (respId == RESP_INFO_REPLY || respId == RESP_STATS || respId == RESP_ERROR);
    
    if (!isCritical) {
        // Check available space for non-critical responses
        if (Serial.availableForWrite() < (2 + len)) {
            return; // Not enough space - skip this response
        }
    }
    
    // For critical responses, ensure we have space (wait if necessary)
    if (isCritical) {
        // Wait up to 50ms for buffer space to become available
        // Don't wait too long - we need to process USB commands
        uint32_t startTime = millis();
        while (Serial.availableForWrite() < (2 + len) && (millis() - startTime) < 50) {
            delay(1);
        }
        // If still no space after waiting, try to send anyway (might block briefly)
        // But don't wait forever - send what we can
    }
    
    // Write header
    Serial.write(respId);
    Serial.write(len);
    
    // Write payload if present
    if (len > 0 && data != nullptr) {
        Serial.write(data, len);
    }
    
    // For critical responses, ensure data is sent immediately
    // On nRF52 and other platforms, Serial.write() buffers data - flush() is needed to send it
    if (isCritical) {
        // Flush critical responses - but don't block forever
        // Some platforms might hang on flush() if USB is disconnected
        Serial.flush(); // Flush critical responses - brief blocking is acceptable
        // Note: If flush() blocks, at least the data is written and will be sent when USB is ready
    }
}

void USBComm::sendInfo() {
    // Single point for all device state reporting - reuses stack buffer
    uint8_t info[18];
    uint8_t* p = info;
    
    // Firmware version
    *p++ = 0x01; // Major
    *p++ = 0x00; // Minor
    
    // Get frequencies from protocol states dynamically
    // Use safe access with null checks and bounds checking
    uint32_t firstFreq = 0;
    uint32_t secondFreq = 0;
    uint8_t firstBw = 0;
    uint8_t secondBw = 0;
    
    // Safely access protocol states (they might not be initialized yet during early setup)
    if (protocolStates != nullptr) {
        if (PROTOCOL_COUNT > PROTOCOL_MESHCORE && PROTOCOL_MESHCORE < PROTOCOL_COUNT) {
            firstFreq = protocolStates[PROTOCOL_MESHCORE].config.frequencyHz;
            firstBw = protocolStates[PROTOCOL_MESHCORE].config.bandwidth;
        }
        if (PROTOCOL_COUNT > PROTOCOL_MESHTASTIC && PROTOCOL_MESHTASTIC < PROTOCOL_COUNT) {
            secondFreq = protocolStates[PROTOCOL_MESHTASTIC].config.frequencyHz;
            secondBw = protocolStates[PROTOCOL_MESHTASTIC].config.bandwidth;
        }
    }
    
    // First protocol frequency (4 bytes, little-endian)
    *p++ = (uint8_t)(firstFreq & 0xFF);
    *p++ = (uint8_t)((firstFreq >> 8) & 0xFF);
    *p++ = (uint8_t)((firstFreq >> 16) & 0xFF);
    *p++ = (uint8_t)((firstFreq >> 24) & 0xFF);
    
    // Second protocol frequency (4 bytes, little-endian)
    *p++ = (uint8_t)(secondFreq & 0xFF);
    *p++ = (uint8_t)((secondFreq >> 8) & 0xFF);
    *p++ = (uint8_t)((secondFreq >> 16) & 0xFF);
    *p++ = (uint8_t)((secondFreq >> 24) & 0xFF);
    
    // Switch interval (2 bytes, little-endian)
    *p++ = (uint8_t)(protocolSwitchIntervalMs & 0xFF);
    *p++ = (uint8_t)((protocolSwitchIntervalMs >> 8) & 0xFF);
    
    // Current protocol and bandwidths
    *p++ = (uint8_t)rx_protocol;
    *p++ = firstBw;
    *p++ = secondBw;
    // desiredProtocolMode always equals rx_protocol since auto-switch is disabled
    // (kept for web interface compatibility)
    *p++ = desiredProtocolMode; // 0=MeshCore, 1=Meshtastic (matches rx_protocol)
    
    // Platform ID: 0 = LoRa32u4II, 1 = RAK4631
    #ifdef RAK4631_BOARD
        *p++ = 1; // RAK4631
    #else
        *p++ = 0; // LoRa32u4II (default)
    #endif
    
    // Always send the response - this is a critical response
    sendResponse(RESP_INFO_REPLY, info, 18);
}

void USBComm::sendStats() {
    // Single point for all statistics reporting - reuses stack buffer
    uint8_t stats[24];
    uint8_t* p = stats;
    
    // Helper macro to pack uint32_t (little-endian)
    #define PACK_U32(val) \
        *p++ = (uint8_t)((val) & 0xFF); \
        *p++ = (uint8_t)(((val) >> 8) & 0xFF); \
        *p++ = (uint8_t)(((val) >> 16) & 0xFF); \
        *p++ = (uint8_t)(((val) >> 24) & 0xFF);
    
    // Get stats from protocol state objects dynamically
    // For backward compatibility, send stats for first two protocols
    uint32_t meshcoreRxCount = (protocolStates != nullptr && PROTOCOL_COUNT > PROTOCOL_MESHCORE) ? 
        protocolStates[PROTOCOL_MESHCORE].stats.rxCount : 0;
    uint32_t meshtasticRxCount = (protocolStates != nullptr && PROTOCOL_COUNT > PROTOCOL_MESHTASTIC) ? 
        protocolStates[PROTOCOL_MESHTASTIC].stats.rxCount : 0;
    uint32_t meshcoreTxCount = (protocolStates != nullptr && PROTOCOL_COUNT > PROTOCOL_MESHCORE) ? 
        protocolStates[PROTOCOL_MESHCORE].stats.txCount : 0;
    uint32_t meshtasticTxCount = (protocolStates != nullptr && PROTOCOL_COUNT > PROTOCOL_MESHTASTIC) ? 
        protocolStates[PROTOCOL_MESHTASTIC].stats.txCount : 0;
    
    // Aggregate errors across all protocols
    uint32_t conversionErrors = 0;
    uint32_t parseErrors = 0;
    if (protocolStates != nullptr) {
        for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
            conversionErrors += protocolStates[id].stats.conversionErrors;
            parseErrors += protocolStates[id].stats.parseErrors;
        }
    }
    
    PACK_U32(meshcoreRxCount);
    PACK_U32(meshtasticRxCount);
    PACK_U32(meshcoreTxCount);
    PACK_U32(meshtasticTxCount);
    PACK_U32(conversionErrors);
    PACK_U32(parseErrors);
    
    #undef PACK_U32
    
    sendResponse(RESP_STATS, stats, 24);
}

void USBComm::sendRxPacket(uint8_t protocol, int16_t rssi, int8_t snr, uint8_t* data, uint8_t len) {
    if (len > 56) len = 56; // Limit to fit in buffer
    
    uint8_t buffer[64];
    buffer[0] = protocol; // 0 = MeshCore, 1 = Meshtastic
    buffer[1] = (uint8_t)(rssi & 0xFF);
    buffer[2] = (uint8_t)((rssi >> 8) & 0xFF);
    buffer[3] = (int8_t)snr;
    buffer[4] = len;
    
    if (len > 0 && data != nullptr) {
        memcpy(&buffer[5], data, len);
    }
    
    sendResponse(RESP_RX_PACKET, buffer, 5 + len);
}

void USBComm::sendDebugLog(const char* message) {
    // Debug logs are low priority - only send if buffer has plenty of space
    if (Serial.availableForWrite() < 80) {
        return; // Skip debug log if buffer is getting full
    }
    uint8_t len = strlen(message);
    if (len > 64) len = 64;
    sendResponse(RESP_DEBUG_LOG, (uint8_t*)message, len);
}

void USBComm::sendError(const char* errorMessage) {
    // Error messages are higher priority - try to send even if buffer is getting full
    if (Serial.availableForWrite() < 20) {
        return; // Only skip if buffer is very full
    }
    uint8_t len = strlen(errorMessage);
    if (len > 60) len = 60; // Slightly shorter than debug log to ensure delivery
    sendResponse(RESP_ERROR, (uint8_t*)errorMessage, len);
}
