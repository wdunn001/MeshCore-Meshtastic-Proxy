// Protocol definitions and message encoding/decoding for MeshCore-Meshtastic Proxy

const Protocol = {
    // Command IDs
    CMD_GET_INFO: 0x01,
    CMD_GET_STATS: 0x02,
    CMD_SET_FREQUENCY: 0x03,
    CMD_SET_PROTOCOL: 0x04,
    CMD_RESET_STATS: 0x05,
    CMD_SEND_TEST: 0x06,

    // Response IDs
    RESP_INFO_REPLY: 0x81,
    RESP_STATS: 0x82,
    RESP_RX_PACKET: 0x83,
    RESP_ERROR: 0x84,
    RESP_DEBUG_LOG: 0x85,

    // Encode a command message
    encodeCommand(cmdId, data = new Uint8Array(0)) {
        const buffer = new Uint8Array(2 + data.length);
        buffer[0] = cmdId;
        buffer[1] = data.length;
        buffer.set(data, 2);
        return buffer;
    },

    // Decode INFO_REPLY response
    decodeInfoReply(data) {
        if (data.length < 12) return null;
        return {
            fwVersionMajor: data[0],
            fwVersionMinor: data[1],
            meshcoreFreq: data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24),
            meshtasticFreq: data[6] | (data[7] << 8) | (data[8] << 16) | (data[9] << 24),
            switchInterval: data[10],
            currentProtocol: data[11] // 0 = MeshCore, 1 = Meshtastic
        };
    },

    // Decode STATS response
    decodeStats(data) {
        if (data.length < 20) return null;
        
        return {
            meshcoreRx: data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24),
            meshtasticRx: data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24),
            meshcoreTx: data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24),
            meshtasticTx: data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24),
            conversionErrors: data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24)
        };
    },

    // Decode RX_PACKET response
    decodeRxPacket(data) {
        if (data.length < 5) return null;
        
        const protocol = data[0]; // 0 = MeshCore, 1 = Meshtastic
        // RSSI is signed 16-bit (int16_t), convert from unsigned to signed
        let rssi = data[1] | (data[2] << 8);
        if (rssi > 32767) rssi = rssi - 65536; // Convert unsigned to signed
        // Convert signed 8-bit SNR
        const snr = (data[3] > 127) ? data[3] - 256 : data[3];
        const len = data[4];
        
        if (data.length < 5 + len) return null;
        
        const packetData = new Uint8Array(len);
        packetData.set(data.subarray(5, 5 + len));
        
        return {
            protocol: protocol,
            rssi: rssi,
            snr: snr,
            data: packetData
        };
    },

    // Decode DEBUG_LOG response
    decodeDebugLog(data) {
        const decoder = new TextDecoder();
        return decoder.decode(data);
    }
};

// Make Protocol available globally
window.Protocol = Protocol;
