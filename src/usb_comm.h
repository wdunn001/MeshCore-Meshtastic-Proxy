#ifndef USB_COMM_H
#define USB_COMM_H

#include <stdint.h>
#include <stdbool.h>

// Command IDs
#define CMD_GET_INFO      0x01
#define CMD_GET_STATS     0x02
#define CMD_SET_FREQUENCY 0x03
#define CMD_SET_PROTOCOL  0x04
#define CMD_RESET_STATS   0x05
#define CMD_SEND_TEST     0x06
#define CMD_SET_SWITCH_INTERVAL 0x07
#define CMD_SET_PROTOCOL_PARAMS 0x08  // Generic: 1 byte protocol ID + 4 bytes freq + 1 byte bandwidth
#define CMD_SET_RX_PROTOCOL 0x09      // Set listen protocol: 1 byte protocol ID
#define CMD_SET_TX_PROTOCOLS 0x0A     // Set transmit protocols: 1 byte bitmask (bit 0=MeshCore, bit 1=Meshtastic)

// Response IDs
#define RESP_INFO_REPLY   0x81
#define RESP_STATS        0x82
#define RESP_RX_PACKET    0x83
#define RESP_ERROR        0x84
#define RESP_DEBUG_LOG    0x85

class USBComm {
public:
    void init();
    void process();
    void sendInfo();
    void sendStats();
    void sendRxPacket(uint8_t protocol, int16_t rssi, int8_t snr, uint8_t* data, uint8_t len);
    void sendDebugLog(const char* message);
    void sendError(const char* errorMessage);
    
private:
    void handleCommand(uint8_t cmd, uint8_t* data, uint8_t len);
    void sendResponse(uint8_t respId, uint8_t* data, uint8_t len);
    bool readCommand(uint8_t* cmd, uint8_t* data, uint8_t* len);
};

extern USBComm usbComm;

#endif // USB_COMM_H
