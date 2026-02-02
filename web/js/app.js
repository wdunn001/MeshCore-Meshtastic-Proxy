// Main application logic for MeshCore-Meshtastic Proxy

class App {
    constructor() {
        // Initialize protocol state arrays
        const protocolState = {};
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            protocolState[i] = {
                freq: i === 0 ? 910525000 : 906875000, // Default frequencies
                bandwidth: i === 0 ? 6 : 8, // Default bandwidths
                rx: 0,
                tx: 0,
                lastRxTime: null
            };
        }
        
        // Load stats update rate from localStorage or use default
        const savedStatsRate = localStorage.getItem('statsUpdateRate');
        const defaultStatsRate = savedStatsRate ? parseInt(savedStatsRate, 10) : 1000;
        
        this.state = {
            connected: false,
            protocols: protocolState, // Generic protocol state array
            rx_protocol: 0, // Current listen protocol
            tx_protocols: [], // Array of transmit protocol IDs
            currentProtocol: 0, // Legacy compatibility
            switchInterval: 100,
            statsUpdateRate: defaultStatsRate, // Configurable stats update rate (ms)
            conversionErrors: 0,
            lastPacket: null,
            lastActivityTime: null,
            protocolSwitches: 0,
            lastProtocol: null,
            connectionStartTime: null,
            firmwareInfoLogged: false,
            controlsFormDirty: false,
            infoReceived: false,
            platform: null // Platform name (e.g., "LoRa32u4II", "RAK4631")
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
        
        // Initialize stats update rate display
        const statsUpdateRateInput = document.getElementById('statsUpdateRateInput');
        const statsUpdateRateDisplay = document.getElementById('statsUpdateRateDisplay');
        if (statsUpdateRateInput && statsUpdateRateDisplay) {
            statsUpdateRateInput.value = this.state.statsUpdateRate;
            statsUpdateRateDisplay.textContent = `Current: ${this.state.statsUpdateRate}ms`;
        }
        
        // Initialize control visibility
        this.updateControlVisibility();
        
        // Initialize statistics charts (after ProtocolRegistry is loaded)
        // Use setTimeout to ensure ProtocolRegistry is available
        setTimeout(() => {
            if (window.Statistics && window.ProtocolRegistry) {
                window.Statistics.init();
            }
        }, 100);
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
        // protocolSelect removed - auto-switch removed from UI
        document.getElementById('listenProtocolSelect').addEventListener('change', () => {
            this.state.controlsFormDirty = true;
            this.updateTransmitProtocolsFromListen();
            // Update display after visibility changes
            window.UI.updateTransmitProtocolsDisplay();
            // Mark form as dirty since TX protocols may have been auto-checked
            this.state.controlsFormDirty = true;
        });
        // Setup transmit protocol checkbox listeners dynamically
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox) {
                checkbox.addEventListener('change', () => {
                    // Only mark dirty if checkbox is enabled (not auto-checked/disabled)
                    if (!checkbox.disabled) {
                        this.state.controlsFormDirty = true;
                        
                        // If checkbox is checked, sync RX protocol to match TX protocol
                        // This ensures the device can receive its own transmissions
                        if (checkbox.checked) {
                            const listenProtocolSelect = document.getElementById('listenProtocolSelect');
                            if (listenProtocolSelect) {
                                const currentRxProtocol = parseInt(listenProtocolSelect.value);
                                const txProtocolId = i;
                                
                                // Only change RX if it's different from the TX protocol
                                if (currentRxProtocol !== txProtocolId) {
                                    listenProtocolSelect.value = txProtocolId.toString();
                                    // Update visibility and auto-check logic
                                    this.updateTransmitProtocolsFromListen();
                                    // Update display
                                    window.UI.updateTransmitProtocolsDisplay();
                                    window.UI.updateListenProtocol(txProtocolId);
                                    
                                    // Log the sync action
                                    const protocolName = window.ProtocolRegistry.getName(txProtocolId);
                                    window.UI.addLogEntry(`RX protocol synced to ${protocolName} to match TX selection`, 'info');
                                }
                            }
                        } else {
                            // If checkbox is unchecked, check if we need to sync RX to another checked TX protocol
                            const listenProtocolSelect = document.getElementById('listenProtocolSelect');
                            if (listenProtocolSelect) {
                                const currentRxProtocol = parseInt(listenProtocolSelect.value);
                                
                                // Find the first checked TX protocol
                                let firstCheckedTx = null;
                                for (let j = 0; j < window.ProtocolRegistry.PROTOCOL_COUNT; j++) {
                                    const txCheckbox = document.getElementById(`transmitProtocol${j}`);
                                    if (txCheckbox && txCheckbox.checked && !txCheckbox.disabled) {
                                        firstCheckedTx = j;
                                        break;
                                    }
                                }
                                
                                // If RX matches the unchecked protocol and there's another checked TX, sync to it
                                if (currentRxProtocol === i && firstCheckedTx !== null && firstCheckedTx !== currentRxProtocol) {
                                    listenProtocolSelect.value = firstCheckedTx.toString();
                                    this.updateTransmitProtocolsFromListen();
                                    window.UI.updateTransmitProtocolsDisplay();
                                    window.UI.updateListenProtocol(firstCheckedTx);
                                    
                                    const protocolName = window.ProtocolRegistry.getName(firstCheckedTx);
                                    window.UI.addLogEntry(`RX protocol synced to ${protocolName} to match TX selection`, 'info');
                                }
                            }
                        }
                    }
                    // Update display when checkbox changes
                    window.UI.updateTransmitProtocolsDisplay();
                });
            }
        }
        // Removed switchIntervalInput event listeners - auto-switch removed from UI
        
        // Stats update rate input (not part of form submission, saves immediately)
        const statsUpdateRateInput = document.getElementById('statsUpdateRateInput');
        if (statsUpdateRateInput) {
            statsUpdateRateInput.value = this.state.statsUpdateRate;
            statsUpdateRateInput.addEventListener('input', () => {
                const value = parseInt(statsUpdateRateInput.value, 10);
                const minRate = 10; // USB max rate
                const maxRate = 10000;
                const clampedValue = Math.max(minRate, Math.min(maxRate, value));
                this.state.statsUpdateRate = clampedValue;
                statsUpdateRateInput.value = clampedValue;
                document.getElementById('statsUpdateRateDisplay').textContent = `Current: ${clampedValue}ms`;
                
                // Save to localStorage
                localStorage.setItem('statsUpdateRate', clampedValue.toString());
                
                // Restart stats polling with new rate if connected
                if (this.state.connected) {
                    this.stopStatsPolling();
                    this.startStatsPolling();
                }
            });
        }
        
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
            this.state.platform = null; // Reset platform - will be set when info is received
            window.UI.updateConnectionStatus(true);
            window.UI.enableButtons(true);
            window.UI.addLogEntry('Connected successfully', 'success');
            
            // Give device a moment to initialize after connection
            await new Promise(resolve => setTimeout(resolve, 150));
            
            // Request device info with retry logic
            await this.requestDeviceInfoWithRetry();
            
            // Also request stats immediately to get packet counts
            await window.serialComm.getStats();
            
            // Update control visibility after connection
            this.updateControlVisibility();
            
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
        this.state.platform = null; // Reset platform on disconnect
        
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
        // Use configurable stats update rate (default 1000ms, minimum 10ms for USB max rate)
        const statsRate = this.state.statsUpdateRate || 1000;
        const minRate = 10; // Minimum 10ms (100 updates/sec) - USB CDC can handle this
        const actualRate = Math.max(statsRate, minRate);
        
        // Calculate how many stats requests before info request (info every 5 seconds)
        const infoInterval = 5000; // Request info every 5 seconds
        const infoRequestEvery = Math.max(1, Math.floor(infoInterval / actualRate));
        
        // Poll stats at configured rate, request info every 5 seconds to update current protocol status
        this.statsInterval = setInterval(() => {
            if (this.state.connected) {
                window.serialComm.getStats();
                // Request info periodically to update current protocol status
                if (++infoRequestCount >= infoRequestEvery) {
                    window.serialComm.getInfo();
                    infoRequestCount = 0;
                }
            }
        }, actualRate);
    }

    stopStatsPolling() {
        if (this.statsInterval) {
            clearInterval(this.statsInterval);
            this.statsInterval = null;
        }
    }

    startStatusUpdates() {
        // Update status every 100ms for real-time feedback
        this.statusUpdateInterval = setInterval(() => {
            if (this.state.connected) {
                this.updateStatus();
            }
        }, 100);
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
        const protocolActivity = {};
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const proto = this.state.protocols[i];
            const recent = proto.lastRxTime && (now - proto.lastRxTime) < 30000;
            const active = proto.rx > 0 || proto.tx > 0;
            protocolActivity[i] = active && (recent || proto.rx > 0);
            
            // Update last packet times
            if (proto.lastRxTime) {
                window.UI.updateLastPacketTime(i, proto.lastRxTime);
            }
        }
        
        // Show as receiving if packets were received recently OR if there's been any activity
        window.UI.updateProtocolActivity(
            protocolActivity[0] || false,
            protocolActivity[1] || false
        );
        
        // Track protocol switches
        if (this.state.lastProtocol !== null && this.state.lastProtocol !== this.state.currentProtocol) {
            this.state.protocolSwitches++;
        }
        this.state.lastProtocol = this.state.currentProtocol;
        
        // Update device status
        // Auto-switch removed - always show listening status
        let activity = 'Idle';
        if (this.state.lastPacket && this.state.lastPacket.timestamp) {
            const timeSincePacket = now - this.state.lastPacket.timestamp;
            if (timeSincePacket < 2000) {
                activity = 'Processing Packet';
            } else if (timeSincePacket < 10000) {
                activity = 'Listening';
            } else {
                const rxProtocolName = window.ProtocolRegistry.getName(this.state.rx_protocol);
                activity = `Listening (${rxProtocolName})`;
            }
        } else {
            const rxProtocolName = window.ProtocolRegistry.getName(this.state.rx_protocol);
            activity = `Listening (${rxProtocolName})`;
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
        
        // Get platform name
        const platformName = this.state.platform || '--';
        window.UI.updateDeviceStatus(activity, uptime, this.state.protocolSwitches, lastActivity, platformName);
    }

    async resetStats() {
        await window.serialComm.resetStats();
        // Reset chart history
        if (window.Statistics) {
            window.Statistics.reset();
        }
        window.UI.addLogEntry('Statistics reset', 'info');
    }

    async sendTestMessage(protocol) {
        let protocolName;
        if (protocol === 2) {
            protocolName = 'All';
        } else if (window.ProtocolRegistry.isValidProtocolId(protocol)) {
            protocolName = window.ProtocolRegistry.getName(protocol);
        } else {
            protocolName = `Protocol ${protocol}`;
        }
        window.UI.addLogEntry(`Sending ${protocolName} test message...`, 'info');
        await window.serialComm.sendTestMessage(protocol);
    }

    updateControlVisibility() {
        // Auto-switch removed - always show listen and transmit protocol controls
        const listenProtocolGroup = document.getElementById('listenProtocolGroup');
        const transmitProtocolsGroup = document.getElementById('transmitProtocolsGroup');
        
        // Always show these groups (auto-switch removed)
        if (listenProtocolGroup) {
            listenProtocolGroup.style.display = 'block';
        }
        if (transmitProtocolsGroup) {
            transmitProtocolsGroup.style.display = 'block';
        }
        
        // Enable/disable controls based on connection state only
        const listenProtocolSelect = document.getElementById('listenProtocolSelect');
        
        if (listenProtocolSelect) {
            listenProtocolSelect.disabled = !this.state.connected;
        }
        
        // Update transmit protocol checkboxes (visibility, auto-check if only one, enable/disable)
        this.updateTransmitProtocolsFromListen();
    }

    updateTransmitProtocolsFromListen() {
        const listenProtocolSelect = document.getElementById('listenProtocolSelect');
        if (!listenProtocolSelect) return;
        
        const listenProtocol = parseInt(listenProtocolSelect.value);
        
        // Count visible TX protocol options
        let visibleCount = 0;
        let visibleCheckbox = null;
        
        // Hide/show checkboxes based on listen protocol
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            const label = checkbox ? checkbox.closest('label.checkbox-label') : null;
            
            if (checkbox && label) {
                if (listenProtocol === i) {
                    // Hide the checkbox for the listen protocol
                    label.style.display = 'none';
                    // Uncheck if it was checked (to prevent TX on same protocol as RX)
                    if (checkbox.checked) {
                        checkbox.checked = false;
                    }
                    checkbox.disabled = true;
                } else {
                    // Show the checkbox
                    label.style.display = '';
                    visibleCount++;
                    visibleCheckbox = checkbox;
                }
            }
        }
        
        // If only one TX protocol option is available, auto-check and disable it
        if (visibleCount === 1 && visibleCheckbox) {
            visibleCheckbox.checked = true;
            visibleCheckbox.disabled = true;
        } else {
            // Multiple options available - enable checkboxes and let user choose
            for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                const checkbox = document.getElementById(`transmitProtocol${i}`);
                const label = checkbox ? checkbox.closest('label.checkbox-label') : null;
                
                if (checkbox && label && label.style.display !== 'none') {
                    checkbox.disabled = !this.state.connected;
                }
            }
        }
    }

    async saveSettings() {
        const listenProtocolSelect = document.getElementById('listenProtocolSelect');
        
        // Manual mode only - set listen protocol and transmit protocols
        const listenProtocol = parseInt(listenProtocolSelect.value);
        const protocolName = window.ProtocolRegistry.getName(listenProtocol);
        window.UI.addLogEntry(`Setting RX protocol to ${protocolName}...`, 'info');
        await window.serialComm.setRxProtocol(listenProtocol);
        this.state.rx_protocol = listenProtocol;
        this.state.currentProtocol = listenProtocol;
        
        // Build transmit protocol bitmask dynamically
        let txBitmask = 0;
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox && checkbox.checked) {
                txBitmask |= (1 << i); // Set bit for protocol ID
            }
        }
        
        window.UI.addLogEntry(`Setting TX protocols (bitmask: 0x${txBitmask.toString(16).padStart(2, '0')})...`, 'info');
        await window.serialComm.setTxProtocols(txBitmask);
        
        // Ensure auto-switch is disabled
        await window.serialComm.setSwitchInterval(0);
        this.state.switchInterval = 0;
        
        // Update UI immediately
        window.UI.updateListenProtocol(listenProtocol);
        // Update visibility of transmit protocol checkboxes (hide listen protocol checkbox)
        this.updateTransmitProtocolsFromListen();
        // Update display to show current checkbox states
        window.UI.updateTransmitProtocolsDisplay();
        
        // Reset dirty flag after saving
        this.state.controlsFormDirty = false;
        window.UI.addLogEntry('Settings saved', 'info');
        
        // Request fresh info from device after a short delay to confirm settings were applied
        setTimeout(() => {
            if (this.state.connected) {
                window.serialComm.getInfo();
            }
        }, 200);
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

    async saveProtocolParams(protocolId) {
        const freqInput = document.getElementById(`protocol${protocolId}FreqInput`);
        const bwSelect = document.getElementById(`protocol${protocolId}BwSelect`);
        if (!freqInput || !bwSelect) return;
        
        const frequencyHz = Math.round(parseFloat(freqInput.value) * 1000000);
        const bandwidth = parseInt(bwSelect.value);
        const protocolName = window.ProtocolRegistry.getName(protocolId);
        
        // Note: Frequency validation is now done by the device based on radio capabilities
        if (bandwidth >= 0 && bandwidth <= 9) {
            window.UI.addLogEntry(`Setting ${protocolName}: ${(frequencyHz/1000000).toFixed(3)} MHz, BW=${bandwidth}`, 'info');
            await window.serialComm.setProtocolParams(protocolId, frequencyHz, bandwidth);
        } else {
            window.UI.addLogEntry(`Invalid ${protocolName} parameters`, 'error');
        }
    }

    // Legacy wrapper functions for backward compatibility with HTML
    async saveMeshCoreParams() {
        await this.saveProtocolParams(window.ProtocolRegistry.PROTOCOL_MESHCORE);
    }

    async saveMeshtasticParams() {
        await this.saveProtocolParams(window.ProtocolRegistry.PROTOCOL_MESHTASTIC);
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
                    
                    // Update protocol state from device info
                    // Note: protocol.js still uses meshcore/meshtastic names for backward compatibility with firmware protocol
                    this.state.protocols[0].freq = info.meshcoreFreq;
                    this.state.protocols[1].freq = info.meshtasticFreq;
                    this.state.protocols[0].bandwidth = info.meshcoreBandwidth;
                    this.state.protocols[1].bandwidth = info.meshtasticBandwidth;
                    this.state.rx_protocol = info.currentProtocol;
                    this.state.currentProtocol = info.currentProtocol; // Legacy compatibility
                    this.state.switchInterval = info.switchInterval;
                    
                    // Extract and store platform information
                    if (info.platformId !== undefined) {
                        // Platform ID: 0 = LoRa32u4II, 1 = RAK4631
                        this.state.platform = info.platformId === 1 ? 'RAK4631' : 'LoRa32u4II';
                    }
                    
                    // Auto-switch removed - always manual mode
                    const protocolMode = 0; // Always manual mode
                    const rxProtocolName = window.ProtocolRegistry.getName(info.currentProtocol);
                    const protocolModeText = `Manual (${rxProtocolName})`;
                    
                    // Single consolidated log message with all device status
                    const status = {
                        platform: this.state.platform || 'Unknown',
                        rx_protocol: rxProtocolName,
                        switchInterval: info.switchInterval,
                        desiredProtocolMode: protocolMode,
                        protocolMode: protocolModeText
                    };
                    for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                        status[`protocol${i}Freq`] = `${(this.state.protocols[i].freq / 1000000).toFixed(3)} MHz`;
                        status[`protocol${i}BW`] = this.state.protocols[i].bandwidth;
                    }
                    console.log('Device Status:', status);
                    
                    // Update device status panel with platform info
                    this.updateStatus();
                    
                    window.UI.updateFrequencies(info.meshcoreFreq, info.meshtasticFreq);
                    window.UI.updateProtocolStatus(info.currentProtocol);
                    // Only update form fields if form is not dirty
                    if (!this.state.controlsFormDirty) {
                        window.UI.updateListenProtocol(info.currentProtocol);
                        // Update visibility of transmit protocol checkboxes (hide listen protocol checkbox)
                        // Don't change checkbox states - we don't have TX protocol bitmask from device
                        this.updateTransmitProtocolsFromListen();
                        window.UI.updateProtocolParams(info.meshcoreFreq, info.meshcoreBandwidth, info.meshtasticFreq, info.meshtasticBandwidth);
                        // Update display text based on current checkbox states
                        window.UI.updateTransmitProtocolsDisplay();
                    } else {
                        // Form is dirty - just update display text to reflect current checkbox states
                        window.UI.updateTransmitProtocolsDisplay();
                    }
                    
                    // Update control visibility and enabled state
                    this.updateControlVisibility();
                    
                    // Only log firmware info once on first connection
                    if (!this.state.firmwareInfoLogged) {
                        const rxProtocolName = window.ProtocolRegistry.getName(info.currentProtocol);
                        window.UI.addLogEntry(`Device ready: Firmware v${info.fwVersionMajor}.${info.fwVersionMinor} | Mode: Manual (${rxProtocolName})`, 'success');
                        this.state.firmwareInfoLogged = true;
                    }
                } else {
                    console.warn('Failed to decode info response, data length:', data.length);
                }
                break;
                
            case window.Protocol.RESP_STATS:
                const stats = window.Protocol.decodeStats(data);
                if (stats) {
                    // Update protocol stats (protocol.js still uses meshcore/meshtastic names for backward compatibility)
                    this.state.protocols[0].rx = stats.meshcoreRx;
                    this.state.protocols[1].rx = stats.meshtasticRx;
                    this.state.protocols[0].tx = stats.meshcoreTx;
                    this.state.protocols[1].tx = stats.meshtasticTx;
                    this.state.conversionErrors = stats.conversionErrors;
                    
                    // Update statistics charts dynamically for each protocol
                    if (window.Statistics) {
                        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                            // Map protocol IDs to stats (0=MeshCore, 1=Meshtastic)
                            const rxCount = i === 0 ? stats.meshcoreRx : stats.meshtasticRx;
                            const txCount = i === 0 ? stats.meshcoreTx : stats.meshtasticTx;
                            window.Statistics.updateProtocolStats(i, rxCount, txCount);
                        }
                        window.Statistics.updateErrors(stats.conversionErrors);
                    }
                    
                    // Legacy UI updates (for backward compatibility)
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
                    if (window.ProtocolRegistry.isValidProtocolId(packet.protocol)) {
                        this.state.protocols[packet.protocol].lastRxTime = now;
                    }
                    
                    window.UI.updateLastPacket(packet.protocol, packet.rssi, packet.snr, packet.data);
                    const protocolName = window.ProtocolRegistry.getName(packet.protocol);
                    window.UI.addLogEntry(`${protocolName} packet: RSSI=${packet.rssi}dBm SNR=${packet.snr}dB Len=${packet.data.length}`, 'info');
                }
                break;
                
            case window.Protocol.RESP_DEBUG_LOG:
                const message = window.Protocol.decodeDebugLog(data);
                // Statistics messages (containing "RX:" and "TX:") should only be console logged, not added to event log
                // Event log should only contain actual events like packet received, config changes, errors, etc.
                if (message && (message.includes('RX:') || message.includes('TX:'))) {
                    console.log(`[Stats] ${message}`);
                } else {
                    // Other debug messages (errors, config changes, etc.) go to event log
                    window.UI.addLogEntry(message, 'info');
                }
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
