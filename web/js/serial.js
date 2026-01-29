// WebSerial communication layer

class SerialComm {
    constructor() {
        this.port = null;
        this.reader = null;
        this.writer = null;
        this.isConnected = false;
        this.readBuffer = new Uint8Array(0);
        this.onMessage = null;
        this.onError = null;
    }

    async connect() {
        try {
            if (!navigator.serial) {
                throw new Error('WebSerial API not supported. Please use Chrome/Edge browser.');
            }

            // Request port
            this.port = await navigator.serial.requestPort();
            
            // Open port with baud rate
            await this.port.open({ baudRate: 115200 });

            this.writer = this.port.writable.getWriter();
            this.reader = this.port.readable.getReader();
            this.isConnected = true;

            // Start reading loop
            this.readLoop();

            return true;
        } catch (error) {
            console.error('Serial connection error:', error);
            if (this.onError) {
                this.onError(error);
            }
            return false;
        }
    }

    async disconnect() {
        try {
            this.isConnected = false;
            
            if (this.reader) {
                try {
                    await this.reader.cancel();
                } catch (e) {}
                try {
                    this.reader.releaseLock();
                } catch (e) {}
                this.reader = null;
            }
            
            if (this.writer) {
                try {
                    this.writer.releaseLock();
                } catch (e) {}
                this.writer = null;
            }
            
            if (this.port) {
                try {
                    await this.port.close();
                } catch (e) {
                    console.warn('Port close error:', e);
                }
                this.port = null;
            }
        } catch (error) {
            console.error('Disconnect error:', error);
        }
    }

    async sendCommand(cmdId, data = new Uint8Array(0)) {
        if (!this.isConnected || !this.port || !this.writer) {
            return;
        }

        const message = window.Protocol.encodeCommand(cmdId, data);
        await this.writer.write(message);
    }

    async getInfo() {
        await this.sendCommand(window.Protocol.CMD_GET_INFO);
    }

    async getStats() {
        await this.sendCommand(window.Protocol.CMD_GET_STATS);
    }

    async resetStats() {
        await this.sendCommand(window.Protocol.CMD_RESET_STATS);
    }

    async sendTestMessage(protocol) {
        // protocol: 0 = MeshCore, 1 = Meshtastic, 2 = Both
        const data = new Uint8Array([protocol]);
        await this.sendCommand(window.Protocol.CMD_SEND_TEST, data);
    }

    async setSwitchInterval(intervalMs) {
        // Send interval as 2 bytes (little-endian)
        const data = new Uint8Array([
            intervalMs & 0xFF,        // Low byte
            (intervalMs >> 8) & 0xFF // High byte
        ]);
        await this.sendCommand(window.Protocol.CMD_SET_SWITCH_INTERVAL, data);
    }

    async setProtocol(protocol) {
        // protocol: 0 = MeshCore, 1 = Meshtastic
        const data = new Uint8Array([protocol]);
        await this.sendCommand(window.Protocol.CMD_SET_PROTOCOL, data);
    }

    async setMeshCoreParams(frequencyHz, bandwidth) {
        // 4 bytes frequency (little-endian) + 1 byte bandwidth
        const data = new Uint8Array([
            frequencyHz & 0xFF,
            (frequencyHz >> 8) & 0xFF,
            (frequencyHz >> 16) & 0xFF,
            (frequencyHz >> 24) & 0xFF,
            bandwidth & 0xFF
        ]);
        await this.sendCommand(window.Protocol.CMD_SET_MESHCORE_PARAMS, data);
    }

    async setMeshtasticParams(frequencyHz, bandwidth) {
        // 4 bytes frequency (little-endian) + 1 byte bandwidth
        const data = new Uint8Array([
            frequencyHz & 0xFF,
            (frequencyHz >> 8) & 0xFF,
            (frequencyHz >> 16) & 0xFF,
            (frequencyHz >> 24) & 0xFF,
            bandwidth & 0xFF
        ]);
        await this.sendCommand(window.Protocol.CMD_SET_MESHTASTIC_PARAMS, data);
    }

    async readLoop() {
        const decoder = new TextDecoder();
        
        while (this.isConnected) {
            try {
                const { value, done } = await this.reader.read();
                
                if (done) {
                    break;
                }
                
                if (value && value.length > 0) {
                    // Append to buffer
                    const newBuffer = new Uint8Array(this.readBuffer.length + value.length);
                    newBuffer.set(this.readBuffer);
                    newBuffer.set(value, this.readBuffer.length);
                    this.readBuffer = newBuffer;
                    
                    // Process complete messages
                    while (this.readBuffer.length >= 2) {
                        const respId = this.readBuffer[0];
                        const len = this.readBuffer[1];
                        
                        if (this.readBuffer.length < 2 + len) {
                            break; // Wait for more data
                        }
                        
                        // Extract message
                        const messageData = this.readBuffer.subarray(2, 2 + len);
                        
                        // Remove processed message from buffer
                        this.readBuffer = this.readBuffer.subarray(2 + len);
                        
                        // Handle message
                        if (this.onMessage) {
                            this.onMessage(respId, messageData);
                        }
                    }
                }
            } catch (error) {
                console.error('Read error:', error);
                if (this.onError) {
                    this.onError(error);
                }
                break;
            }
        }
    }
}

// Create global instance
window.serialComm = new SerialComm();
