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
ProtocolId rx_protocol = PROTOCOL_MESHCORE;      // Currently listening protocol
ProtocolId tx_protocols[PROTOCOL_COUNT];         // Protocols to transmit to (relay targets)
uint8_t tx_protocol_count = 0;                   // Number of active transmit protocols
unsigned long lastProtocolSwitch = 0;
uint16_t protocolSwitchIntervalMs = PROTOCOL_SWITCH_INTERVAL_MS_DEFAULT;
bool autoSwitchEnabled = true;
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
    ProtocolInterfaceImpl* iface = protocol_interface_get(protocol);
    ProtocolConfig* config = protocol_manager_getConfig(protocol);
    
    if (iface && config) {
        iface->configure(config);
        protocolStates[protocol].isActive = true;
    }
}

void update_tx_protocols(ProtocolId rx_protocol) {
    // With canonical format, we can relay to all other protocols
    tx_protocol_count = 0;
    for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        if (id != rx_protocol) {
            tx_protocols[tx_protocol_count++] = id;
        }
    }
}

void set_tx_protocols(uint8_t bitmask) {
    // Set transmit protocols from bitmask (bit 0 = MeshCore, bit 1 = Meshtastic)
    tx_protocol_count = 0;
    for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
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
    // Set the listen protocol and update transmit protocols
    rx_protocol = protocol;
    currentProtocol = (ProtocolState)protocol;
    
    // Update transmit protocol list (exclude listen protocol)
    update_tx_protocols(rx_protocol);
    
    // Configure radio for new listen protocol
    configureProtocol(protocol);
    lastProtocolSwitch = millis();
}

bool receivePacket(uint8_t* buffer, uint8_t* len) {
    if (!radio_isPacketReceived()) {
        return false;
    }
    
    *len = radio_getPacketLength();
    ProtocolInterfaceImpl* currentIface = protocol_interface_get(rx_protocol);
    
    if (*len == 0 || (currentIface && *len > currentIface->getMaxPacketSize())) {
        radio_clearIrqFlags();
        radio_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    
    radio_readFifo(buffer, *len);
    radio_clearIrqFlags();
    
    return true;
}

bool transmitPacket(ProtocolId protocol, const uint8_t* data, uint8_t len) {
    if (len == 0 || len > 255) {
        return false;
    }
    
    // Configure radio for target protocol
    configureProtocol(protocol);
    delay(20);
    
    radio_setPower(platform_getMaxTxPower());
    radio_setCrc(true);
    
    radio_writeFifo((uint8_t*)data, len);
    radio_clearIrqFlags();
    radio_setMode(MODE_TX);
    delay(5);
    
    // Wait for transmission to complete
    unsigned long startTime = millis();
    while (millis() - startTime < 200) {
        delay(1);
    }
    if (millis() - startTime > 5000) {
        radio_setMode(MODE_STDBY);
        delay(10);
        return false;
    }
    
    radio_clearIrqFlags();
    radio_setMode(MODE_STDBY);
    delay(10);
    
    return true;
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
        char errorMsg[40];
        snprintf(errorMsg, sizeof(errorMsg), "ERR: %s parse fail len=%d", iface->name, len);
        usbComm.sendDebugLog(errorMsg);
        return;
    }
    
    // Filter MQTT packets
    if (canonical.viaMqtt) {
        return;  // Silently drop MQTT packets
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
    for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
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
    delay(2000);
    
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
    for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
        protocol_interface_initState(id, &protocolStates[id]);
    }
    
    // Initialize listen protocol (default to first available protocol)
    rx_protocol = PROTOCOL_MESHCORE;  // Default to first protocol
    currentProtocol = (ProtocolState)rx_protocol;
    update_tx_protocols(rx_protocol);
    
    // Protocol configs are stored in protocolStates[].config and can be accessed
    // via protocol_manager_getConfig() or directly from protocolStates[]
    
    protocolSwitchIntervalMs = PROTOCOL_SWITCH_INTERVAL_MS_DEFAULT;
    autoSwitchEnabled = true;
    desiredProtocolMode = 2;
    
    // Configure radio for listen protocol
    configureProtocol(rx_protocol);
    lastProtocolSwitch = millis();
    
    // Print protocol information dynamically
    Serial.println("Proxy ready. Listening for packets...");
    Serial.println("Configured protocols:");
    for (ProtocolId id = PROTOCOL_MESHCORE; id < PROTOCOL_COUNT; id = (ProtocolId)(id + 1)) {
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
    
    if (!autoSwitchEnabled || protocolSwitchIntervalMs == 0) {
        // Manual mode - enforce desired protocol
        ProtocolId desiredId = (desiredProtocolMode == 0) ? PROTOCOL_MESHCORE : 
                               (desiredProtocolMode == 1) ? PROTOCOL_MESHTASTIC : rx_protocol;
        
        if (rx_protocol != desiredId) {
            setProtocol((ProtocolState)desiredId);
        }
    } else {
        // Auto-switch mode - rotate through protocols
        switchProtocol();
    }
    
    if (packetReceived) {
        packetReceived = false;
        
        uint8_t packetLen = 0;
        if (receivePacket(rxBuffer, &packetLen)) {
            int16_t rssi = radio_getRssi();
            int8_t snr = radio_getSnr();
            
            usbComm.sendRxPacket(rx_protocol, rssi, snr, rxBuffer, packetLen);
            handlePacket(rx_protocol, rxBuffer, packetLen);
            
            radio_setMode(MODE_RX_CONTINUOUS);
        } else {
            configureProtocol(rx_protocol);
        }
    }
    
    if (radio_isPacketReceived()) {
        packetReceived = true;
    }
    
    static unsigned long lastStatsPrint = 0;
    if (millis() - lastStatsPrint > 10000) {
        printStatistics();
        lastStatsPrint = millis();
    }
    
    delay(1);
}
