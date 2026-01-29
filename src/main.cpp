#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "sx1276.h"
#include "meshcore_handler.h"
#include "meshtastic_handler.h"
#include "usb_comm.h"

// Protocol state enum (must match usb_comm.cpp)
enum ProtocolState {
    LISTENING_MESHCORE,
    LISTENING_MESHTASTIC
};

// Global state variables (accessible from usb_comm.cpp)
ProtocolState currentProtocol = LISTENING_MESHCORE;
unsigned long lastProtocolSwitch = 0;

// Packet buffers
uint8_t rxBuffer[MAX_MESHCORE_PACKET_SIZE];
uint8_t txBuffer[MAX_MESHTASTIC_PACKET_SIZE];
uint8_t convertedBuffer[MAX_MESHCORE_PACKET_SIZE];

// Statistics
// Using uint32_t (max 4,294,967,295) - add overflow protection
uint32_t meshcoreRxCount = 0;
uint32_t meshtasticRxCount = 0;
uint32_t meshcoreTxCount = 0;
uint32_t meshtasticTxCount = 0;
uint32_t conversionErrors = 0;

// Helper to safely increment counters with overflow protection
#define SAFE_INCREMENT(counter) do { \
    if (counter < 4294967295U) counter++; \
    else { \
        /* Overflow protection - log and reset */ \
        char msg[50]; \
        snprintf(msg, sizeof(msg), "Counter overflow detected!"); \
        usbComm.sendDebugLog(msg); \
        counter = 0; \
    } \
} while(0)

// Interrupt flag
volatile bool packetReceived = false;

void onPacketReceived() {
    packetReceived = true;
}

void configureForMeshCore() {
    sx1276_setMode(MODE_STDBY);
    delay(10);
    sx1276_setFrequency(DEFAULT_FREQUENCY_HZ);
    sx1276_setBandwidth(MESHCORE_BW);
    sx1276_setSpreadingFactor(MESHCORE_SF);
    sx1276_setCodingRate(MESHCORE_CR);
    sx1276_setSyncWord(MESHCORE_SYNC_WORD);
    sx1276_setPreambleLength(MESHCORE_PREAMBLE);
    sx1276_setHeaderMode(true); // Implicit header mode
    sx1276_setInvertIQ(false); // MeshCore uses normal IQ
    sx1276_setCrc(true); // Enable CRC
    delay(10);
    sx1276_setMode(MODE_RX_CONTINUOUS);
}

void configureForMeshtastic() {
    sx1276_setMode(MODE_STDBY);
    delay(10);
    sx1276_setFrequency(MESHTASTIC_FREQUENCY_HZ);
    sx1276_setBandwidth(MESHTASTIC_BW);
    sx1276_setSpreadingFactor(MESHTASTIC_SF);
    sx1276_setCodingRate(MESHTASTIC_CR);
    sx1276_setSyncWord(MESHTASTIC_SYNC_WORD);
    sx1276_setPreambleLength(MESHTASTIC_PREAMBLE);
    sx1276_setHeaderMode(true); // Implicit header mode
    sx1276_setInvertIQ(true); // Meshtastic uses inverted IQ
    sx1276_setCrc(true); // Enable CRC
    delay(10);
    sx1276_setMode(MODE_RX_CONTINUOUS);
}

bool receivePacket(uint8_t* buffer, uint8_t* len) {
    // Check if packet was received
    uint8_t irqFlags = sx1276_readRegister(REG_IRQ_FLAGS);
    if (!(irqFlags & IRQ_RX_DONE_MASK)) {
        return false;
    }
    
    // Get packet length
    *len = sx1276_getPacketLength();
    if (*len == 0 || *len > MAX_MESHCORE_PACKET_SIZE) {
        sx1276_clearIrqFlags();
        sx1276_setMode(MODE_RX_CONTINUOUS);
        return false;
    }
    
    // Read packet from FIFO
    sx1276_readFifo(buffer, *len);
    sx1276_clearIrqFlags();
    
    return true;
}

bool transmitPacket(const uint8_t* data, uint8_t len) {
    if (len == 0 || len > MAX_MESHTASTIC_PACKET_SIZE) {
        char errorMsg[60];
        snprintf(errorMsg, sizeof(errorMsg), "TX error: Invalid packet length %d (max %d)", 
                 len, MAX_MESHTASTIC_PACKET_SIZE);
        usbComm.sendError(errorMsg);
        return false;
    }
    
    // Ensure we're in standby mode
    sx1276_setMode(MODE_STDBY);
    delay(20); // Allow mode change to settle
    
    // Set power to maximum for transmission
    sx1276_setPower(17); // Max power (17 dBm)
    
    // Ensure CRC is enabled for transmission
    sx1276_setCrc(true);
    
    // Write packet to FIFO (this also sets payload length and FIFO pointer)
    sx1276_writeFifo((uint8_t*)data, len);
    
    // Clear all IRQ flags before starting TX
    sx1276_clearIrqFlags();
    
    // Configure DIO0 for TX_DONE interrupt (bit 6-7 = 00 means DIO0 = TxDone)
    sx1276_writeRegister(REG_DIO_MAPPING_1, 0x00);
    
    // Start transmission
    sx1276_setMode(MODE_TX);
    delay(5); // Small delay to allow mode change
    
    // Verify we're in TX mode
    uint8_t opMode = sx1276_readRegister(REG_OP_MODE);
    if ((opMode & 0x07) != MODE_TX) {
        // Failed to enter TX mode
        sx1276_setMode(MODE_STDBY);
        delay(10);
        char errorMsg[50];
        snprintf(errorMsg, sizeof(errorMsg), "TX error: Failed to enter TX mode (mode=0x%02X)", opMode);
        usbComm.sendError(errorMsg);
        return false;
    }
    
    // Wait for transmission to complete (with timeout)
    // TX typically takes 10-100ms depending on packet size and SF
    unsigned long startTime = millis();
    uint8_t irqFlags = 0;
    while (true) {
        irqFlags = sx1276_readRegister(REG_IRQ_FLAGS);
        if (irqFlags & IRQ_TX_DONE_MASK) {
            break; // Transmission complete
        }
        if (millis() - startTime > 5000) {
            // Timeout - return to standby
            sx1276_setMode(MODE_STDBY);
            delay(10);
            char errorMsg[50];
            snprintf(errorMsg, sizeof(errorMsg), "TX error: Timeout waiting for TX_DONE (len=%d)", len);
            usbComm.sendError(errorMsg);
            return false;
        }
        delay(1); // Small delay in loop
    }
    
    // Clear flags and return to standby
    sx1276_clearIrqFlags();
    sx1276_setMode(MODE_STDBY);
    delay(10); // Small delay before switching modes
    
    return true;
}

void handleMeshCorePacket(uint8_t* data, uint8_t len) {
    SAFE_INCREMENT(meshcoreRxCount);
    
    MeshCorePacket meshcorePacket;
    if (!meshcore_parsePacket(data, len, &meshcorePacket)) {
        SAFE_INCREMENT(conversionErrors);
        // Send descriptive error message
        char errorMsg[80];
        if (len > 0) {
            uint8_t header = data[0];
            uint8_t routeType = header & 0x03;
            uint8_t payloadType = (header >> 2) & 0x0F;
            uint8_t version = (header >> 6) & 0x03;
            uint8_t pathLenPos = 1;
            if (routeType == 0x00 || routeType == 0x03) {
                pathLenPos = 5; // Has transport codes
            }
            uint8_t pathLen = (pathLenPos < len) ? data[pathLenPos] : 0;
            if (pathLen > MAX_MESHCORE_PATH_SIZE) {
                snprintf(errorMsg, sizeof(errorMsg), "MeshCore parse error: Invalid path length %d (max %d)", 
                         pathLen, MAX_MESHCORE_PATH_SIZE);
            } else if (len < 2) {
                snprintf(errorMsg, sizeof(errorMsg), "MeshCore parse error: Packet too short (%d bytes)", len);
            } else {
                snprintf(errorMsg, sizeof(errorMsg), "MeshCore parse error: Invalid packet format (len=%d hdr=0x%02X)", 
                         len, header);
            }
            usbComm.sendError(errorMsg);
            // Also send as debug log with more details
            char debugMsg[100];
            snprintf(debugMsg, sizeof(debugMsg), "MeshCore parse fail: len=%d hdr=0x%02X rt=%d pt=%d v=%d plen=%d", 
                     len, header, routeType, payloadType, version, pathLen);
            usbComm.sendDebugLog(debugMsg);
        } else {
            usbComm.sendError("MeshCore parse error: Empty packet");
        }
        return;
    }
    
    // Convert to Meshtastic format
    uint8_t convertedLen = 0;
    if (!meshcore_convertToMeshtastic(&meshcorePacket, txBuffer, &convertedLen)) {
        SAFE_INCREMENT(conversionErrors);
        char errorMsg[60];
        snprintf(errorMsg, sizeof(errorMsg), "MeshCore->Meshtastic conversion error: Payload too large (%d bytes)", 
                 meshcorePacket.payload_len);
        usbComm.sendError(errorMsg);
        return;
    }
    
    // Switch to Meshtastic configuration
    configureForMeshtastic();
    delay(50); // Allow radio to settle
    
    // Transmit as Meshtastic packet
    if (transmitPacket(txBuffer, convertedLen)) {
        SAFE_INCREMENT(meshtasticTxCount);
        digitalWrite(LED_PIN, HIGH);
        delay(10);
        digitalWrite(LED_PIN, LOW);
    }
    
    // Switch back to MeshCore listening
    configureForMeshCore();
}

void handleMeshtasticPacket(uint8_t* data, uint8_t len) {
    SAFE_INCREMENT(meshtasticRxCount);
    
    MeshtasticHeader header;
    uint8_t payload[MAX_MESHTASTIC_PAYLOAD_SIZE];
    uint8_t payloadLen = 0;
    
    if (!meshtastic_parsePacket(data, len, &header, payload, &payloadLen)) {
        SAFE_INCREMENT(conversionErrors);
        char errorMsg[60];
        if (len < MESHTASTIC_HEADER_SIZE) {
            snprintf(errorMsg, sizeof(errorMsg), "Meshtastic parse error: Packet too short (%d bytes, need %d)", 
                     len, MESHTASTIC_HEADER_SIZE);
        } else {
            snprintf(errorMsg, sizeof(errorMsg), "Meshtastic parse error: Invalid packet format (%d bytes)", len);
        }
        usbComm.sendError(errorMsg);
        return;
    }
    
    // Convert to MeshCore format
    uint8_t convertedLen = 0;
    if (!meshtastic_convertToMeshCore(&header, payload, payloadLen, convertedBuffer, &convertedLen)) {
        SAFE_INCREMENT(conversionErrors);
        char errorMsg[60];
        snprintf(errorMsg, sizeof(errorMsg), "Meshtastic->MeshCore conversion error: Payload too large (%d bytes)", 
                 payloadLen);
        usbComm.sendError(errorMsg);
        return;
    }
    
    // Switch to MeshCore configuration
    configureForMeshCore();
    delay(50); // Allow radio to settle
    
    // Transmit as MeshCore packet
    if (transmitPacket(convertedBuffer, convertedLen)) {
        SAFE_INCREMENT(meshcoreTxCount);
        digitalWrite(LED_PIN, HIGH);
        delay(10);
        digitalWrite(LED_PIN, LOW);
    }
    
    // Switch back to Meshtastic listening
    configureForMeshtastic();
}

void switchProtocol() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastProtocolSwitch >= PROTOCOL_SWITCH_INTERVAL_MS) {
        if (currentProtocol == LISTENING_MESHCORE) {
            currentProtocol = LISTENING_MESHTASTIC;
            configureForMeshtastic();
        } else {
            currentProtocol = LISTENING_MESHCORE;
            configureForMeshCore();
        }
        lastProtocolSwitch = currentTime;
    }
}

void printStatistics() {
    Serial.print("MeshCore RX: ");
    Serial.print(meshcoreRxCount);
    Serial.print(" | Meshtastic RX: ");
    Serial.print(meshtasticRxCount);
    Serial.print(" | MeshCore TX: ");
    Serial.print(meshcoreTxCount);
    Serial.print(" | Meshtastic TX: ");
    Serial.print(meshtasticTxCount);
    Serial.print(" | Errors: ");
    Serial.println(conversionErrors);
}

// Generate a test MeshCore packet (broadcast text message)
uint8_t generateMeshCoreTestPacket(uint8_t* buffer, uint8_t* len) {
    const char* testMsg = "MeshCore Test";
    uint8_t msgLen = strlen(testMsg);
    
    uint8_t i = 0;
    
    // Header: FLOOD (0x01) | TXT_MSG (0x02 << 2) | VER_1 (0x00 << 6)
    buffer[i++] = 0x01 | (0x02 << 2) | (0x00 << 6);
    
    // No transport codes for FLOOD
    // Path length: 0 for broadcast
    buffer[i++] = 0;
    
    // Payload: text message
    memcpy(&buffer[i], testMsg, msgLen);
    i += msgLen;
    
    *len = i;
    return i;
}

// Generate a test Meshtastic packet (broadcast message)
uint8_t generateMeshtasticTestPacket(uint8_t* buffer, uint8_t* len) {
    const char* testMsg = "Meshtastic Test";
    uint8_t msgLen = strlen(testMsg);
    
    MeshtasticHeader* header = (MeshtasticHeader*)buffer;
    
    // Broadcast header
    header->to = 0xFFFFFFFF;      // Broadcast (little-endian)
    header->from = 0x00000001;    // Proxy node ID (little-endian)
    header->id = 0x00000001;      // Packet ID (little-endian)
    header->flags = 0x03;         // Hop limit = 3, want_ack=0, via_mqtt=0, hop_start=0
    header->channel = 0;          // Default channel hash (0 for default/primary channel)
    header->next_hop = 0;
    header->relay_node = 0;
    
    // Payload: simple text message (normally this would be protobuf-encoded and encrypted)
    // For testing, we send raw text - nodes may not decode it but should see the packet
    memcpy(&buffer[MESHTASTIC_HEADER_SIZE], testMsg, msgLen);
    
    *len = MESHTASTIC_HEADER_SIZE + msgLen;
    return *len;
}

void sendTestMessage(uint8_t protocol) {
    uint8_t testBuffer[64];
    uint8_t testLen = 0;
    char debugMsg[64];
    
    if (protocol == 0) {
        // Send MeshCore test message
        generateMeshCoreTestPacket(testBuffer, &testLen);
        
        // Configure radio for MeshCore
        sx1276_setMode(MODE_STDBY);
        delay(20);
        sx1276_setFrequency(DEFAULT_FREQUENCY_HZ);
        sx1276_setBandwidth(MESHCORE_BW);
        sx1276_setSpreadingFactor(MESHCORE_SF);
        sx1276_setCodingRate(MESHCORE_CR);
        sx1276_setSyncWord(MESHCORE_SYNC_WORD);
        sx1276_setPreambleLength(MESHCORE_PREAMBLE);
        sx1276_setHeaderMode(true); // Implicit header mode
        sx1276_setInvertIQ(false); // MeshCore uses normal IQ
        sx1276_setCrc(true); // Enable CRC
        delay(20);
        
        snprintf(debugMsg, sizeof(debugMsg), "TX MeshCore: %d bytes @ %.3f MHz", testLen, DEFAULT_FREQUENCY_HZ / 1000000.0);
        usbComm.sendDebugLog(debugMsg);
        
        // Verify radio is configured correctly before TX
        uint8_t syncWord = sx1276_readRegister(REG_SYNC_WORD);
        uint8_t opMode = sx1276_readRegister(REG_OP_MODE);
        char verifyMsg[80];
        snprintf(verifyMsg, sizeof(verifyMsg), "Pre-TX: sync=0x%02X mode=0x%02X", syncWord, opMode);
        usbComm.sendDebugLog(verifyMsg);
        
        if (transmitPacket(testBuffer, testLen)) {
            SAFE_INCREMENT(meshcoreTxCount);
            digitalWrite(LED_PIN, HIGH);
            delay(50);
            digitalWrite(LED_PIN, LOW);
            usbComm.sendDebugLog("MeshCore test TX success");
        } else {
            usbComm.sendDebugLog("MeshCore test TX failed - check radio config");
        }
        
        // Return to listening mode
        configureForMeshCore();
    } else {
        // Send Meshtastic test message
        generateMeshtasticTestPacket(testBuffer, &testLen);
        
        // Configure radio for Meshtastic
        sx1276_setMode(MODE_STDBY);
        delay(20);
        sx1276_setFrequency(MESHTASTIC_FREQUENCY_HZ);
        sx1276_setBandwidth(MESHTASTIC_BW);
        sx1276_setSpreadingFactor(MESHTASTIC_SF);
        sx1276_setCodingRate(MESHTASTIC_CR);
        sx1276_setSyncWord(MESHTASTIC_SYNC_WORD);
        sx1276_setPreambleLength(MESHTASTIC_PREAMBLE);
        sx1276_setHeaderMode(true); // Implicit header mode
        sx1276_setInvertIQ(true); // Meshtastic uses inverted IQ
        sx1276_setCrc(true); // Enable CRC
        delay(20);
        
        snprintf(debugMsg, sizeof(debugMsg), "TX Meshtastic: %d bytes @ %.3f MHz", testLen, MESHTASTIC_FREQUENCY_HZ / 1000000.0);
        usbComm.sendDebugLog(debugMsg);
        
        // Verify radio is configured correctly before TX
        uint8_t syncWord = sx1276_readRegister(REG_SYNC_WORD);
        uint8_t opMode = sx1276_readRegister(REG_OP_MODE);
        uint8_t invertIQ = sx1276_readRegister(REG_INVERT_IQ);
        char verifyMsg[80];
        snprintf(verifyMsg, sizeof(verifyMsg), "Pre-TX: sync=0x%02X mode=0x%02X IQ=0x%02X", syncWord, opMode, invertIQ);
        usbComm.sendDebugLog(verifyMsg);
        
        if (transmitPacket(testBuffer, testLen)) {
            SAFE_INCREMENT(meshtasticTxCount);
            digitalWrite(LED_PIN, HIGH);
            delay(50);
            digitalWrite(LED_PIN, LOW);
            usbComm.sendDebugLog("Meshtastic test TX success");
        } else {
            usbComm.sendDebugLog("Meshtastic test TX failed - check radio config");
        }
        
        // Return to listening mode
        configureForMeshtastic();
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(2000); // Wait for serial to initialize
    
    Serial.println("MeshCore-Meshtastic Proxy Starting...");
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize USB communication
    usbComm.init();
    
    // Initialize SX1276
    if (!sx1276_init()) {
        Serial.println("ERROR: SX1276 initialization failed!");
        while(1) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
    
    Serial.println("SX1276 initialized successfully");
    
    // Attach interrupt
    sx1276_attachInterrupt(onPacketReceived);
    
    // Start listening for MeshCore packets
    configureForMeshCore();
    currentProtocol = LISTENING_MESHCORE;
    lastProtocolSwitch = millis();
    
    Serial.println("Proxy ready. Listening for packets...");
    Serial.print("MeshCore Frequency: ");
    Serial.print(DEFAULT_FREQUENCY_HZ / 1000000.0);
    Serial.println(" MHz");
    Serial.print("Meshtastic Frequency: ");
    Serial.print(MESHTASTIC_FREQUENCY_HZ / 1000000.0);
    Serial.println(" MHz");
    
    // Send startup test messages after a short delay
    delay(1000);
    sendTestMessage(0); // MeshCore
    delay(500);
    sendTestMessage(1); // Meshtastic
}

void loop() {
    // Process USB commands
    usbComm.process();
    
    // Time-slice between protocols
    switchProtocol();
    
    // Check for received packet
    if (packetReceived) {
        packetReceived = false;
        
        uint8_t packetLen = 0;
        if (receivePacket(rxBuffer, &packetLen)) {
            // Get RSSI and SNR before processing
            int16_t rssi = sx1276_getRssi();
            int8_t snr = sx1276_getSnr();
            
            if (currentProtocol == LISTENING_MESHCORE) {
                // Send packet info to USB before conversion
                usbComm.sendRxPacket(0, rssi, snr, rxBuffer, packetLen);
                handleMeshCorePacket(rxBuffer, packetLen);
            } else {
                // Send packet info to USB before conversion
                usbComm.sendRxPacket(1, rssi, snr, rxBuffer, packetLen);
                handleMeshtasticPacket(rxBuffer, packetLen);
            }
        } else {
            // Restart receive mode if packet read failed
            if (currentProtocol == LISTENING_MESHCORE) {
                configureForMeshCore();
            } else {
                configureForMeshtastic();
            }
        }
    }
    
    // Also check IRQ flags directly (in case interrupt didn't fire)
    if (sx1276_isPacketReceived()) {
        packetReceived = true;
    }
    
    // Print statistics every 10 seconds
    static unsigned long lastStatsPrint = 0;
    if (millis() - lastStatsPrint > 10000) {
        printStatistics();
        lastStatsPrint = millis();
    }
    
    delay(1); // Small delay to prevent tight loop
}
