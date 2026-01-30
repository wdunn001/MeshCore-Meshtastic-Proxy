// Main application logic for MeshCore-Meshtastic Proxy

class App {
    constructor() {
        this.state = {
            connected: false,
            meshcoreFreq: 910525000,
            meshtasticFreq: 906875000,
            currentProtocol: 0,
            switchInterval: 100,
            meshcoreBandwidth: 6,
            meshtasticBandwidth: 8,
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
            connectionStartTime: null,
            firmwareInfoLogged: false, // Track if we've logged firmware info
            controlsFormDirty: false, // Track if controls form has unsaved changes
            infoReceived: false // Track if we've received device info response
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
        
        // Form submission
        document.getElementById('controlForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.saveSettings();
        });
        
        // Mark form as dirty when controls are edited
        document.getElementById('protocolSelect').addEventListener('change', () => {
            this.state.controlsFormDirty = true;
            this.updateIntervalInputState();
        });
        document.getElementById('switchIntervalInput').addEventListener('input', () => {
            this.state.controlsFormDirty = true;
        });
        document.getElementById('switchIntervalInput').addEventListener('change', () => {
            this.state.controlsFormDirty = true;
        });
        
        // Protocol parameter change handlers
        document.getElementById('meshcoreFreqInput').addEventListener('change', () => this.saveMeshCoreParams());
        document.getElementById('meshcoreBwSelect').addEventListener('change', () => this.saveMeshCoreParams());
        document.getElementById('meshtasticFreqInput').addEventListener('change', () => this.saveMeshtasticParams());
        document.getElementById('meshtasticBwSelect').addEventListener('change', () => this.saveMeshtasticParams());
        
        // Cancel button handler
        document.getElementById('cancelSettingsBtn').addEventListener('click', () => {
            this.cancelSettings();
        });
    }

    async connect() {
        window.UI.addLogEntry('Connecting to proxy...', 'info');
        const success = await window.serialComm.connect();
        
        if (success) {
            this.state.connected = true;
            this.state.connectionStartTime = Date.now();
            this.state.lastProtocol = null;
            this.state.protocolSwitches = 0;
            this.state.firmwareInfoLogged = false; // Reset so we log firmware info on reconnect
            this.state.controlsFormDirty = false; // Reset dirty flag on new connection
            this.state.infoReceived = false; // Reset info received flag
            window.UI.updateConnectionStatus(true);
            window.UI.enableButtons(true);
            window.UI.addLogEntry('Connected successfully', 'success');
            
            // Give device a moment to initialize after connection
            await new Promise(resolve => setTimeout(resolve, 150));
            
            // Request device info with retry logic
            await this.requestDeviceInfoWithRetry();
            
            // Also request stats immediately to get packet counts
            await window.serialComm.getStats();
            
            // Update interval input state after connection
            this.updateIntervalInputState();
            
            // Start stats polling
            this.startStatsPolling();
            this.startStatusUpdates();
        } else {
            window.UI.addLogEntry('Connection failed', 'error');
        }
    }

    async requestDeviceInfoWithRetry(maxRetries = 5, retryDelay = 200) {
        const timeoutMs = 1500; // 1.5 second timeout per attempt
        
        // Try requesting info multiple times
        for (let retryCount = 0; retryCount < maxRetries; retryCount++) {
            if (retryCount > 0) {
                window.UI.addLogEntry(`Requesting device info (attempt ${retryCount + 1}/${maxRetries})...`, 'info');
                await new Promise(resolve => setTimeout(resolve, retryDelay));
            } else {
                window.UI.addLogEntry('Requesting device info...', 'info');
            }
            
            // Check if info was already received (from previous attempt)
            if (this.state.infoReceived) {
                break;
            }
            
            await window.serialComm.getInfo();
            
            // Wait a bit for response, checking periodically
            const waitStart = Date.now();
            while ((Date.now() - waitStart) < timeoutMs) {
                await new Promise(resolve => setTimeout(resolve, 100));
                if (this.state.infoReceived) {
                    break;
                }
            }
            
            if (this.state.infoReceived) {
                break;
            }
        }
        
        if (!this.state.infoReceived) {
            window.UI.addLogEntry('Warning: Device info not received immediately. Will retry via background polling.', 'error');
        }
    }

    async disconnect() {
        this.stopStatsPolling();
        this.stopStatusUpdates();
        
        this.state.connected = false;
        this.state.connectionStartTime = null;
        this.state.firmwareInfoLogged = false; // Reset for next connection
        this.state.controlsFormDirty = false; // Reset dirty flag on disconnect
        this.state.infoReceived = false; // Reset info received flag on disconnect
        
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

    updateIntervalInputState() {
        const protocolSelect = document.getElementById('protocolSelect');
        const intervalInput = document.getElementById('switchIntervalInput');
        const isAutoSwitch = parseInt(protocolSelect.value) === 2;
        
        // Only enable interval input when Auto-Switch is selected
        intervalInput.disabled = !this.state.connected || !isAutoSwitch;
    }

    async saveSettings() {
        const protocolSelect = document.getElementById('protocolSelect');
        const intervalInput = document.getElementById('switchIntervalInput');
        
        const protocol = parseInt(protocolSelect.value);
        
        // Set protocol first
        if (protocol !== 2) {
            // Manual mode - set protocol and disable auto-switch
            const protocolName = protocol === 0 ? 'MeshCore' : 'Meshtastic';
            window.UI.addLogEntry(`Setting protocol to ${protocolName}...`, 'info');
            await window.serialComm.setProtocol(protocol);
            // Set interval to 0 to disable auto-switch
            await window.serialComm.setSwitchInterval(0);
            this.state.switchInterval = 0;
        } else {
            // Auto-switch mode - use interval value
            const interval = parseInt(intervalInput.value);
            if (interval !== this.state.switchInterval) {
                window.UI.addLogEntry(`Setting switch interval to ${interval}ms...`, 'info');
                await window.serialComm.setSwitchInterval(interval);
                this.state.switchInterval = interval;
            }
            window.UI.addLogEntry('Auto-switching enabled', 'info');
        }
        
        // Reset dirty flag after saving
        this.state.controlsFormDirty = false;
        window.UI.addLogEntry('Settings saved', 'info');
    }

    cancelSettings() {
        // Reset dirty flag first so form will update when info response arrives
        this.state.controlsFormDirty = false;
        // Request fresh info to reset form to device state
        if (this.state.connected) {
            window.serialComm.getInfo();
        }
        window.UI.addLogEntry('Settings cancelled - form reset to device state', 'info');
    }

    async saveMeshCoreParams() {
        const freqInput = document.getElementById('meshcoreFreqInput');
        const bwSelect = document.getElementById('meshcoreBwSelect');
        const frequencyHz = Math.round(parseFloat(freqInput.value) * 1000000);
        const bandwidth = parseInt(bwSelect.value);
        
        if (frequencyHz >= 902000000 && frequencyHz <= 928000000 && bandwidth >= 0 && bandwidth <= 9) {
            window.UI.addLogEntry(`Setting MeshCore: ${(frequencyHz/1000000).toFixed(3)} MHz, BW=${bandwidth}`, 'info');
            await window.serialComm.setMeshCoreParams(frequencyHz, bandwidth);
        } else {
            window.UI.addLogEntry('Invalid MeshCore parameters', 'error');
        }
    }

    async saveMeshtasticParams() {
        const freqInput = document.getElementById('meshtasticFreqInput');
        const bwSelect = document.getElementById('meshtasticBwSelect');
        const frequencyHz = Math.round(parseFloat(freqInput.value) * 1000000);
        const bandwidth = parseInt(bwSelect.value);
        
        if (frequencyHz >= 902000000 && frequencyHz <= 928000000 && bandwidth >= 0 && bandwidth <= 9) {
            window.UI.addLogEntry(`Setting Meshtastic: ${(frequencyHz/1000000).toFixed(3)} MHz, BW=${bandwidth}`, 'info');
            await window.serialComm.setMeshtasticParams(frequencyHz, bandwidth);
        } else {
            window.UI.addLogEntry('Invalid Meshtastic parameters', 'error');
        }
    }

    handleMessage(respId, data) {
        switch (respId) {
            case window.Protocol.RESP_INFO_REPLY:
                const info = window.Protocol.decodeInfoReply(data);
                if (info) {
                    // Mark that we've received info
                    if (!this.state.infoReceived) {
                        console.log('Device info received successfully');
                    }
                    this.state.infoReceived = true;
                    
                    this.state.meshcoreFreq = info.meshcoreFreq;
                    this.state.meshtasticFreq = info.meshtasticFreq;
                    this.state.currentProtocol = info.currentProtocol;
                    this.state.switchInterval = info.switchInterval;
                    this.state.meshcoreBandwidth = info.meshcoreBandwidth;
                    this.state.meshtasticBandwidth = info.meshtasticBandwidth;
                    
                    // Use desiredProtocolMode from device if available, otherwise infer from switchInterval
                    const protocolMode = info.desiredProtocolMode !== undefined 
                        ? info.desiredProtocolMode 
                        : (info.switchInterval === 0 ? info.currentProtocol : 2);
                    
                    // Determine what protocol mode will be set in dropdown
                    const protocolModeText = protocolMode === 2 
                        ? `Auto-Switch (${info.switchInterval}ms)` 
                        : `Manual (${protocolMode === 0 ? 'MeshCore' : 'Meshtastic'})`;
                    
                    // Single consolidated log message with all device status
                    console.log('Device Status:', {
                        protocol: info.currentProtocol === 0 ? 'MeshCore' : 'Meshtastic',
                        switchInterval: info.switchInterval,
                        desiredProtocolMode: protocolMode,
                        protocolMode: protocolModeText,
                        meshcoreFreq: `${(info.meshcoreFreq / 1000000).toFixed(3)} MHz`,
                        meshtasticFreq: `${(info.meshtasticFreq / 1000000).toFixed(3)} MHz`,
                        meshcoreBW: info.meshcoreBandwidth,
                        meshtasticBW: info.meshtasticBandwidth
                    });
                    
                    window.UI.updateFrequencies(info.meshcoreFreq, info.meshtasticFreq);
                    window.UI.updateProtocolStatus(info.currentProtocol);
                    // Only update form fields if form is not dirty
                    if (!this.state.controlsFormDirty) {
                        window.UI.updateSwitchInterval(info.switchInterval);
                        window.UI.updateProtocolSelect(protocolMode, info.switchInterval);
                        window.UI.updateProtocolParams(info.meshcoreFreq, info.meshcoreBandwidth, info.meshtasticFreq, info.meshtasticBandwidth);
                    }
                    
                    // Update interval input enabled state based on protocol mode
                    this.updateIntervalInputState();
                    
                    // Only log firmware info once on first connection
                    if (!this.state.firmwareInfoLogged) {
                        const intervalText = info.switchInterval === 0 ? 'Off (Manual)' : `${info.switchInterval}ms`;
                        const protocolModeTextLog = protocolMode === 2 
                            ? `Auto-Switch (${info.switchInterval}ms)` 
                            : `Manual (${protocolMode === 0 ? 'MeshCore' : 'Meshtastic'})`;
                        window.UI.addLogEntry(`Device ready: Firmware v${info.fwVersionMajor}.${info.fwVersionMinor} | Mode: ${protocolModeTextLog}`, 'success');
                        this.state.firmwareInfoLogged = true;
                    }
                } else {
                    console.warn('Failed to decode info response, data length:', data.length);
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
                } else {
                    console.warn('Failed to decode stats response, data length:', data.length);
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
                const errorMsg = window.Protocol.decodeError(data);
                if (errorMsg && errorMsg.length > 0) {
                    window.UI.addLogEntry(`Error: ${errorMsg}`, 'error');
                } else {
                    window.UI.addLogEntry('Error from device (no details)', 'error');
                }
                break;
        }
    }

    handleError(error) {
        window.UI.addLogEntry(`Error: ${error.message}`, 'error');
        if (this.state.connected) {
            // Disconnect will reset the dirty flag
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
