// Main application logic for MeshCore-Meshtastic Proxy

class App {
    constructor() {
        this.state = {
            connected: false,
            meshcoreFreq: 910525000,
            meshtasticFreq: 906875000,
            currentProtocol: 0,
            meshcoreRx: 0,
            meshtasticRx: 0,
            meshcoreTx: 0,
            meshtasticTx: 0,
            conversionErrors: 0,
            lastPacket: null,
            meshcoreLastRxTime: null,
            meshtasticLastRxTime: null,
            lastActivityTime: null,
            protocolSwitches: 0,
            lastProtocol: null,
            connectionStartTime: null
        };
        
        this.statsInterval = null;
        this.statusUpdateInterval = null;
    }

    init() {
        // Setup event listeners
        this.setupEventListeners();
        
        // Setup serial message handler
        window.serialComm.onMessage = (respId, data) => this.handleMessage(respId, data);
        window.serialComm.onError = (error) => this.handleError(error);
        
        // Initial UI state
        window.UI.updateConnectionStatus(false);
        window.UI.enableButtons(false);
    }

    setupEventListeners() {
        document.getElementById('connectBtn').addEventListener('click', () => this.connect());
        document.getElementById('disconnectBtn').addEventListener('click', () => this.disconnect());
        document.getElementById('resetStatsBtn').addEventListener('click', () => this.resetStats());
        document.getElementById('testMeshCoreBtn').addEventListener('click', () => this.sendTestMessage(0));
        document.getElementById('testMeshtasticBtn').addEventListener('click', () => this.sendTestMessage(1));
        document.getElementById('testBothBtn').addEventListener('click', () => this.sendTestMessage(2));
    }

    async connect() {
        window.UI.addLogEntry('Connecting to proxy...', 'info');
        const success = await window.serialComm.connect();
        
        if (success) {
            this.state.connected = true;
            this.state.connectionStartTime = Date.now();
            this.state.lastProtocol = null;
            this.state.protocolSwitches = 0;
            window.UI.updateConnectionStatus(true);
            window.UI.enableButtons(true);
            window.UI.addLogEntry('Connected successfully', 'success');
            
            // Request device info
            await window.serialComm.getInfo();
            
            // Start stats polling
            this.startStatsPolling();
            this.startStatusUpdates();
        } else {
            window.UI.addLogEntry('Connection failed', 'error');
        }
    }

    async disconnect() {
        this.stopStatsPolling();
        this.stopStatusUpdates();
        
        this.state.connected = false;
        this.state.connectionStartTime = null;
        
        try {
            await window.serialComm.disconnect();
        } catch (error) {
            console.error('Error during disconnect:', error);
        }
        
        window.UI.updateConnectionStatus(false);
        window.UI.enableButtons(false);
        window.UI.addLogEntry('Disconnected', 'info');
    }

    startStatsPolling() {
        let infoRequestCount = 0;
        // Poll stats every second, request info every 5 seconds to track protocol switches
        this.statsInterval = setInterval(() => {
            if (this.state.connected) {
                window.serialComm.getStats();
                // Request info every 5 seconds to update current protocol status
                if (++infoRequestCount >= 5) {
                    window.serialComm.getInfo();
                    infoRequestCount = 0;
                }
            }
        }, 1000);
    }

    stopStatsPolling() {
        if (this.statsInterval) {
            clearInterval(this.statsInterval);
            this.statsInterval = null;
        }
    }

    startStatusUpdates() {
        // Update status every 500ms for real-time feedback
        this.statusUpdateInterval = setInterval(() => {
            if (this.state.connected) {
                this.updateStatus();
            }
        }, 500);
    }

    stopStatusUpdates() {
        if (this.statusUpdateInterval) {
            clearInterval(this.statusUpdateInterval);
            this.statusUpdateInterval = null;
        }
    }

    updateStatus() {
        const now = Date.now();
        
        // Update protocol activity based on recent packet activity (within last 30 seconds)
        const meshcoreRecent = this.state.meshcoreLastRxTime && (now - this.state.meshcoreLastRxTime) < 30000;
        const meshtasticRecent = this.state.meshtasticLastRxTime && (now - this.state.meshtasticLastRxTime) < 30000;
        const meshcoreActive = this.state.meshcoreRx > 0 || this.state.meshcoreTx > 0;
        const meshtasticActive = this.state.meshtasticRx > 0 || this.state.meshtasticTx > 0;
        
        // Show as receiving if packets were received recently OR if there's been any activity
        window.UI.updateProtocolActivity(
            meshcoreActive && (meshcoreRecent || this.state.meshcoreRx > 0), 
            meshtasticActive && (meshtasticRecent || this.state.meshtasticRx > 0)
        );
        
        // Update last packet times
        if (this.state.meshcoreLastRxTime) {
            window.UI.updateLastPacketTime(0, this.state.meshcoreLastRxTime);
        }
        if (this.state.meshtasticLastRxTime) {
            window.UI.updateLastPacketTime(1, this.state.meshtasticLastRxTime);
        }
        
        // Track protocol switches
        if (this.state.lastProtocol !== null && this.state.lastProtocol !== this.state.currentProtocol) {
            this.state.protocolSwitches++;
        }
        this.state.lastProtocol = this.state.currentProtocol;
        
        // Update device status
        let activity = 'Idle';
        if (this.state.lastPacket && this.state.lastPacket.timestamp) {
            const timeSincePacket = now - this.state.lastPacket.timestamp;
            if (timeSincePacket < 2000) {
                activity = 'Processing Packet';
            } else if (timeSincePacket < 10000) {
                activity = 'Listening';
            } else {
                activity = this.state.currentProtocol === 0 ? 'Listening (MeshCore)' : 'Listening (Meshtastic)';
            }
        } else {
            activity = this.state.currentProtocol === 0 ? 'Listening (MeshCore)' : 'Listening (Meshtastic)';
        }
        
        let uptime = '--';
        if (this.state.connectionStartTime) {
            const elapsed = Math.floor((now - this.state.connectionStartTime) / 1000);
            const hours = Math.floor(elapsed / 3600);
            const minutes = Math.floor((elapsed % 3600) / 60);
            const seconds = elapsed % 60;
            if (hours > 0) {
                uptime = `${hours}h ${minutes}m`;
            } else if (minutes > 0) {
                uptime = `${minutes}m ${seconds}s`;
            } else {
                uptime = `${seconds}s`;
            }
        }
        
        let lastActivity = '--';
        if (this.state.lastActivityTime) {
            const elapsed = Math.floor((now - this.state.lastActivityTime) / 1000);
            if (elapsed < 60) {
                lastActivity = `${elapsed}s ago`;
            } else {
                lastActivity = new Date(this.state.lastActivityTime).toLocaleTimeString();
            }
        }
        
        window.UI.updateDeviceStatus(activity, uptime, this.state.protocolSwitches, lastActivity);
    }

    async resetStats() {
        await window.serialComm.resetStats();
        window.UI.addLogEntry('Statistics reset', 'info');
    }

    async sendTestMessage(protocol) {
        const protocolName = protocol === 0 ? 'MeshCore' : (protocol === 1 ? 'Meshtastic' : 'Both');
        window.UI.addLogEntry(`Sending ${protocolName} test message...`, 'info');
        await window.serialComm.sendTestMessage(protocol);
    }

    handleMessage(respId, data) {
        switch (respId) {
            case window.Protocol.RESP_INFO_REPLY:
                const info = window.Protocol.decodeInfoReply(data);
                if (info) {
                    this.state.meshcoreFreq = info.meshcoreFreq;
                    this.state.meshtasticFreq = info.meshtasticFreq;
                    this.state.currentProtocol = info.currentProtocol;
                    window.UI.updateFrequencies(info.meshcoreFreq, info.meshtasticFreq);
                    window.UI.updateProtocolStatus(info.currentProtocol);
                    window.UI.addLogEntry(`Firmware v${info.fwVersionMajor}.${info.fwVersionMinor} | Switch Interval: ${info.switchInterval}ms`, 'info');
                }
                break;
                
            case window.Protocol.RESP_STATS:
                const stats = window.Protocol.decodeStats(data);
                if (stats) {
                    this.state.meshcoreRx = stats.meshcoreRx;
                    this.state.meshtasticRx = stats.meshtasticRx;
                    this.state.meshcoreTx = stats.meshcoreTx;
                    this.state.meshtasticTx = stats.meshtasticTx;
                    this.state.conversionErrors = stats.conversionErrors;
                    
                    window.UI.updateMeshCoreRx(stats.meshcoreRx);
                    window.UI.updateMeshtasticRx(stats.meshtasticRx);
                    window.UI.updateMeshCoreTx(stats.meshcoreTx);
                    window.UI.updateMeshtasticTx(stats.meshtasticTx);
                    window.UI.updateConversionErrors(stats.conversionErrors);
                }
                break;
                
            case window.Protocol.RESP_RX_PACKET:
                const packet = window.Protocol.decodeRxPacket(data);
                if (packet) {
                    const now = Date.now();
                    packet.timestamp = now;
                    this.state.lastPacket = packet;
                    this.state.lastActivityTime = now;
                    
                    // Update last RX time for the protocol
                    if (packet.protocol === 0) {
                        this.state.meshcoreLastRxTime = now;
                    } else {
                        this.state.meshtasticLastRxTime = now;
                    }
                    
                    window.UI.updateLastPacket(packet.protocol, packet.rssi, packet.snr, packet.data);
                    const protocolName = packet.protocol === 0 ? 'MeshCore' : 'Meshtastic';
                    window.UI.addLogEntry(`${protocolName} packet: RSSI=${packet.rssi}dBm SNR=${packet.snr}dB Len=${packet.data.length}`, 'info');
                }
                break;
                
            case window.Protocol.RESP_DEBUG_LOG:
                const message = window.Protocol.decodeDebugLog(data);
                window.UI.addLogEntry(message, 'info');
                break;
                
            case window.Protocol.RESP_ERROR:
                window.UI.addLogEntry('Error from device', 'error');
                break;
        }
    }

    handleError(error) {
        window.UI.addLogEntry(`Error: ${error.message}`, 'error');
        if (this.state.connected) {
            this.disconnect();
        }
    }
}

// Initialize app when DOM is ready
let app;
document.addEventListener('DOMContentLoaded', () => {
    app = new App();
    app.init();
    window.app = app;
});
