// WebSerial communication layer
// Note: The firmware may send text debug output (Serial.print) which can interfere with
// the binary protocol. This parser includes robust resynchronization to handle this.

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
            
            // Clear any buffered data from previous connection
            this.readBuffer = new Uint8Array(0);

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
                        
                        // First check if respId is a valid response ID (0x81-0x85)
                        // If not, we're likely out of sync (maybe text data mixed in)
                        if (respId < 0x81 || respId > 0x85) {
                            // Not a valid response ID - try to resync
                            let found = false;
                            // Search more aggressively for valid protocol header
                            for (let i = 1; i < Math.min(this.readBuffer.length - 1, 100); i++) {
                                const nextRespId = this.readBuffer[i];
                                // Known response IDs: 0x81-0x85
                                if (nextRespId >= 0x81 && nextRespId <= 0x85) {
                                    // Found potential header - also check that next byte is reasonable
                                    if (i + 1 < this.readBuffer.length) {
                                        const nextLen = this.readBuffer[i + 1];
                                        if (nextLen <= 64) {
                                            // Looks like a valid header
                                            if (i > 1) {
                                                // Log discarded bytes for debugging
                                                const discarded = Array.from(this.readBuffer.subarray(0, i))
                                                    .map(b => String.fromCharCode(b >= 32 && b < 127 ? b : 46))
                                                    .join('');
                                                console.warn(`Resyncing: discarded ${i} bytes (${discarded.length > 0 ? `"${discarded}"` : 'non-text'})`);
                                            }
                                            this.readBuffer = this.readBuffer.subarray(i);
                                            found = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!found) {
                                // No valid header found - clear buffer to prevent infinite loop
                                const discarded = Array.from(this.readBuffer.subarray(0, Math.min(50, this.readBuffer.length)))
                                    .map(b => String.fromCharCode(b >= 32 && b < 127 ? b : 46))
                                    .join('');
                                console.warn(`No valid protocol header found, clearing ${this.readBuffer.length} bytes (first 50 chars: "${discarded}")`);
                                this.readBuffer = new Uint8Array(0);
                            }
                            break;
                        }
                        
                        // Validate length (max reasonable message size is 64 bytes)
                        if (len > 64) {
                            console.warn(`Invalid message length: ${len} (respId: 0x${respId.toString(16)}) - likely out of sync`);
                            // Try to find next valid message start by looking for known response IDs
                            let found = false;
                            // Search more aggressively
                            for (let i = 2; i < Math.min(this.readBuffer.length - 1, 100); i++) {
                                const nextRespId = this.readBuffer[i];
                                // Known response IDs: 0x81-0x85
                                if (nextRespId >= 0x81 && nextRespId <= 0x85) {
                                    if (i + 1 < this.readBuffer.length) {
                                        const nextLen = this.readBuffer[i + 1];
                                        if (nextLen <= 64) {
                                            // Log what we're discarding
                                            const discarded = Array.from(this.readBuffer.subarray(0, i))
                                                .map(b => {
                                                    const hex = b.toString(16).padStart(2, '0');
                                                    const char = (b >= 32 && b < 127) ? String.fromCharCode(b) : '.';
                                                    return `${hex}(${char})`;
                                                })
                                                .join(' ');
                                            console.warn(`Resyncing after invalid length: discarded ${i} bytes: ${discarded.substring(0, 200)}`);
                                            this.readBuffer = this.readBuffer.subarray(i);
                                            found = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!found) {
                                // No valid header found, clear buffer
                                console.warn('No valid header found after invalid length, clearing buffer');
                                this.readBuffer = new Uint8Array(0);
                            }
                            break;
                        }
                        
                        if (this.readBuffer.length < 2 + len) {
                            break; // Wait for more data
                        }
                        
                        // Extract message
                        const messageData = this.readBuffer.subarray(2, 2 + len);
                        
                        // Remove processed message from buffer
                        this.readBuffer = this.readBuffer.subarray(2 + len);
                        
                        // Handle message
                        if (this.onMessage) {
                            try {
                                this.onMessage(respId, messageData);
                            } catch (error) {
                                console.error('Error handling message:', error, 'respId:', respId, 'len:', len);
                            }
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
