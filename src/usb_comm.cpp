#include "usb_comm.h"
#include "config.h"
#include <Arduino.h>
#include <string.h>

// Forward declarations from main.cpp
extern uint32_t meshcoreRxCount;
extern uint32_t meshtasticRxCount;
extern uint32_t meshcoreTxCount;
extern uint32_t meshtasticTxCount;
extern uint32_t conversionErrors;

// Protocol state enum (must match main.cpp)
enum ProtocolState {
    LISTENING_MESHCORE,
    LISTENING_MESHTASTIC
};
extern ProtocolState currentProtocol;

// Forward declaration for test message function
void sendTestMessage(uint8_t protocol);

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
    
    *cmd = Serial.read();
    *len = Serial.read();
    
    // Validate length
    if (*len > 64) {
        // Invalid length - consume remaining bytes to resync
        while (Serial.available() > 0 && Serial.peek() != *cmd) {
            Serial.read();
        }
        return false;
    }
    
    // Read payload if present
    if (*len > 0) {
        uint8_t bytesRead = 0;
        uint32_t startTime = millis();
        while (bytesRead < *len && (millis() - startTime) < 100) {
            if (Serial.available() > 0) {
                data[bytesRead++] = Serial.read();
            }
        }
        
        if (bytesRead < *len) {
            // Timeout - incomplete command
            return false;
        }
    }
    
    return true;
}

void USBComm::handleCommand(uint8_t cmd, uint8_t* data, uint8_t len) {
    switch (cmd) {
        case CMD_GET_INFO:
            sendInfo();
            break;
            
        case CMD_GET_STATS:
            sendStats();
            break;
            
        case CMD_SET_FREQUENCY:
            if (len == 4) {
                uint32_t freq = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | 
                               ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
                // Frequency change would require reconfiguration - for future implementation
                char msg[50];
                snprintf(msg, sizeof(msg), "Frequency change requested: %lu Hz", freq);
                sendDebugLog(msg);
            }
            break;
            
        case CMD_SET_PROTOCOL:
            if (len == 1) {
                // 0 = MeshCore, 1 = Meshtastic
                // Protocol switching is automatic, but could be forced here
                sendDebugLog("Protocol switching is automatic");
            }
            break;
            
        case CMD_RESET_STATS:
            meshcoreRxCount = 0;
            meshtasticRxCount = 0;
            meshcoreTxCount = 0;
            meshtasticTxCount = 0;
            conversionErrors = 0;
            sendDebugLog("Statistics reset - all counters cleared");
            break;
            
        case CMD_SEND_TEST:
            if (len == 1) {
                // 0 = MeshCore, 1 = Meshtastic, 2 = Both
                uint8_t protocol = data[0];
                if (protocol == 0 || protocol == 2) {
                    sendTestMessage(0); // MeshCore
                }
                if (protocol == 1 || protocol == 2) {
                    delay(200); // Small delay between transmissions
                    sendTestMessage(1); // Meshtastic
                }
            }
            break;
            
        default:
            // Unknown command - silently ignore
            break;
    }
}

void USBComm::sendResponse(uint8_t respId, uint8_t* data, uint8_t len) {
    // Check available space in serial buffer
    if (Serial.availableForWrite() < (2 + len)) {
        return; // Not enough space - skip this response to prevent blocking
    }
    
    // Write header
    Serial.write(respId);
    Serial.write(len);
    
    // Write payload if present
    if (len > 0 && data != nullptr) {
        Serial.write(data, len);
    }
}

void USBComm::sendInfo() {
    uint8_t info[12];
    info[0] = 0x01; // Firmware version major
    info[1] = 0x00; // Firmware version minor
    info[2] = (uint8_t)(DEFAULT_FREQUENCY_HZ & 0xFF);
    info[3] = (uint8_t)((DEFAULT_FREQUENCY_HZ >> 8) & 0xFF);
    info[4] = (uint8_t)((DEFAULT_FREQUENCY_HZ >> 16) & 0xFF);
    info[5] = (uint8_t)((DEFAULT_FREQUENCY_HZ >> 24) & 0xFF);
    info[6] = (uint8_t)(MESHTASTIC_FREQUENCY_HZ & 0xFF);
    info[7] = (uint8_t)((MESHTASTIC_FREQUENCY_HZ >> 8) & 0xFF);
    info[8] = (uint8_t)((MESHTASTIC_FREQUENCY_HZ >> 16) & 0xFF);
    info[9] = (uint8_t)((MESHTASTIC_FREQUENCY_HZ >> 24) & 0xFF);
    info[10] = PROTOCOL_SWITCH_INTERVAL_MS;
    info[11] = currentProtocol; // 0 = MeshCore, 1 = Meshtastic
    
    sendResponse(RESP_INFO_REPLY, info, 12);
}

void USBComm::sendStats() {
    uint8_t stats[20];
    
    // MeshCore RX count (4 bytes)
    stats[0] = (uint8_t)(meshcoreRxCount & 0xFF);
    stats[1] = (uint8_t)((meshcoreRxCount >> 8) & 0xFF);
    stats[2] = (uint8_t)((meshcoreRxCount >> 16) & 0xFF);
    stats[3] = (uint8_t)((meshcoreRxCount >> 24) & 0xFF);
    
    // Meshtastic RX count (4 bytes)
    stats[4] = (uint8_t)(meshtasticRxCount & 0xFF);
    stats[5] = (uint8_t)((meshtasticRxCount >> 8) & 0xFF);
    stats[6] = (uint8_t)((meshtasticRxCount >> 16) & 0xFF);
    stats[7] = (uint8_t)((meshtasticRxCount >> 24) & 0xFF);
    
    // MeshCore TX count (4 bytes)
    stats[8] = (uint8_t)(meshcoreTxCount & 0xFF);
    stats[9] = (uint8_t)((meshcoreTxCount >> 8) & 0xFF);
    stats[10] = (uint8_t)((meshcoreTxCount >> 16) & 0xFF);
    stats[11] = (uint8_t)((meshcoreTxCount >> 24) & 0xFF);
    
    // Meshtastic TX count (4 bytes)
    stats[12] = (uint8_t)(meshtasticTxCount & 0xFF);
    stats[13] = (uint8_t)((meshtasticTxCount >> 8) & 0xFF);
    stats[14] = (uint8_t)((meshtasticTxCount >> 16) & 0xFF);
    stats[15] = (uint8_t)((meshtasticTxCount >> 24) & 0xFF);
    
    // Conversion errors (4 bytes)
    stats[16] = (uint8_t)(conversionErrors & 0xFF);
    stats[17] = (uint8_t)((conversionErrors >> 8) & 0xFF);
    stats[18] = (uint8_t)((conversionErrors >> 16) & 0xFF);
    stats[19] = (uint8_t)((conversionErrors >> 24) & 0xFF);
    
    sendResponse(RESP_STATS, stats, 20);
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

void USBComm::sendError(const char* message) {
    // Error messages are higher priority - try to send even if buffer is getting full
    if (Serial.availableForWrite() < 20) {
        return; // Skip if buffer is almost full
    }
    uint8_t len = strlen(message);
    if (len > 64) len = 64;
    sendResponse(RESP_ERROR, (uint8_t*)message, len);
}
