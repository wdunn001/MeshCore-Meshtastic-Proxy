// UI update functions for MeshCore-Meshtastic Proxy

const UI = {
    updateConnectionStatus(connected) {
        const statusIndicator = document.querySelector('.status-indicator');
        const statusText = document.getElementById('statusText');
        
        if (connected) {
            statusIndicator.classList.remove('disconnected');
            statusIndicator.classList.add('connected');
            statusText.textContent = 'Connected';
        } else {
            statusIndicator.classList.remove('connected');
            statusIndicator.classList.add('disconnected');
            statusText.textContent = 'Disconnected';
        }
    },

    updateMeshCoreRx(count) {
        document.getElementById('meshcoreRx').textContent = count.toLocaleString();
    },

    updateMeshtasticRx(count) {
        document.getElementById('meshtasticRx').textContent = count.toLocaleString();
    },

    updateMeshCoreTx(count) {
        document.getElementById('meshcoreTx').textContent = count.toLocaleString();
    },

    updateMeshtasticTx(count) {
        document.getElementById('meshtasticTx').textContent = count.toLocaleString();
    },

    updateConversionErrors(count) {
        const element = document.getElementById('conversionErrors');
        const statItem = element.closest('.stat-item');
        element.textContent = count.toLocaleString();
        if (count > 0) {
            element.style.color = '#dc3545';
            if (statItem) {
                statItem.classList.add('error-stat');
            }
        } else {
            element.style.color = '#28a745';
            if (statItem) {
                statItem.classList.remove('error-stat');
            }
        }
    },

    updateProtocolStatus(protocol) {
        const meshcoreIndicator = document.getElementById('meshcoreIndicator');
        const meshtasticIndicator = document.getElementById('meshtasticIndicator');
        const meshcoreListening = document.getElementById('meshcoreListening');
        const meshtasticListening = document.getElementById('meshtasticListening');
        
        // Update which protocol is currently listening
        if (protocol === 0) {
            meshcoreIndicator.classList.add('active');
            meshtasticIndicator.classList.remove('active');
            meshcoreListening.textContent = 'Yes';
            meshtasticListening.textContent = 'No';
        } else {
            meshcoreIndicator.classList.remove('active');
            meshtasticIndicator.classList.add('active');
            meshcoreListening.textContent = 'No';
            meshtasticListening.textContent = 'Yes';
        }
    },

    updateProtocolActivity(meshcoreActive, meshtasticActive) {
        const meshcoreBadge = document.getElementById('meshcoreStatusBadge');
        const meshtasticBadge = document.getElementById('meshtasticStatusBadge');
        const meshcoreIndicator = document.getElementById('meshcoreIndicator');
        const meshtasticIndicator = document.getElementById('meshtasticIndicator');
        
        // Update MeshCore status
        if (meshcoreActive) {
            meshcoreBadge.textContent = 'Active';
            meshcoreBadge.className = 'status-badge receiving';
            meshcoreIndicator.classList.add('receiving');
        } else {
            meshcoreBadge.textContent = 'Inactive';
            meshcoreBadge.className = 'status-badge inactive';
            meshcoreIndicator.classList.remove('receiving');
        }
        
        // Update Meshtastic status
        if (meshtasticActive) {
            meshtasticBadge.textContent = 'Active';
            meshtasticBadge.className = 'status-badge receiving';
            meshtasticIndicator.classList.add('receiving');
        } else {
            meshtasticBadge.textContent = 'Inactive';
            meshtasticBadge.className = 'status-badge inactive';
            meshtasticIndicator.classList.remove('receiving');
        }
    },

    updateLastPacketTime(protocol, timestamp) {
        const element = protocol === 0 ? document.getElementById('meshcoreLastRx') : document.getElementById('meshtasticLastRx');
        if (element && timestamp) {
            const now = Date.now();
            const elapsed = Math.floor((now - timestamp) / 1000);
            if (elapsed < 60) {
                element.textContent = `${elapsed}s ago`;
            } else if (elapsed < 3600) {
                element.textContent = `${Math.floor(elapsed / 60)}m ago`;
            } else {
                element.textContent = new Date(timestamp).toLocaleTimeString();
            }
        }
    },

    updateDeviceStatus(activity, uptime, protocolSwitches, lastActivity) {
        const activityEl = document.getElementById('deviceActivity');
        const uptimeEl = document.getElementById('deviceUptime');
        const switchesEl = document.getElementById('protocolSwitches');
        const lastActivityEl = document.getElementById('lastActivity');
        
        if (activityEl) activityEl.textContent = activity || 'Idle';
        if (uptimeEl) uptimeEl.textContent = uptime || '--';
        if (switchesEl) switchesEl.textContent = (protocolSwitches || 0).toLocaleString();
        if (lastActivityEl) lastActivityEl.textContent = lastActivity || '--';
    },

    updateSwitchInterval(intervalMs) {
        const input = document.getElementById('switchIntervalInput');
        const display = document.getElementById('switchIntervalDisplay');
        
        if (input) {
            // Clamp value to min/max range
            const clampedValue = Math.max(50, Math.min(1000, intervalMs));
            input.value = clampedValue.toString();
        }
        if (display) {
            const intervalText = intervalMs === 0 ? 'Off (Manual)' : `${intervalMs}ms`;
            display.textContent = `Current: ${intervalText}`;
        }
    },

    updateProtocolSelect(desiredProtocolMode, switchInterval) {
        const protocolSelect = document.getElementById('protocolSelect');
        const protocolDisplay = document.getElementById('protocolDisplay');
        
        if (protocolSelect && protocolDisplay) {
            // Map: 0=MeshCore, 1=Meshtastic, 2=Auto-Switch -> 0=Manual, 2=Auto-Switch
            const modeValue = desiredProtocolMode === 2 ? 2 : 0;
            protocolSelect.value = modeValue.toString();
            
            if (desiredProtocolMode === 2) {
                // Auto-switch mode - show interval
                protocolDisplay.textContent = `Current: Auto-Switch (${switchInterval}ms)`;
            } else {
                // Manual mode
                protocolDisplay.textContent = `Current: Manual`;
            }
        }
    },

    updateListenProtocol(currentProtocol) {
        const listenProtocolSelect = document.getElementById('listenProtocolSelect');
        const listenProtocolDisplay = document.getElementById('listenProtocolDisplay');
        
        if (listenProtocolSelect) {
            listenProtocolSelect.value = currentProtocol.toString();
        }
        if (listenProtocolDisplay) {
            const protocolName = currentProtocol === 0 ? 'MeshCore' : 'Meshtastic';
            listenProtocolDisplay.textContent = `Current: ${protocolName}`;
        }
    },

    updateTransmitProtocols(listenProtocol) {
        const transmitProtocolsDisplay = document.getElementById('transmitProtocolsDisplay');
        
        // Transmit protocols are all except the listen protocol
        const protocols = [];
        for (let i = 0; i < ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox) {
                checkbox.checked = (listenProtocol !== i);
                if (listenProtocol !== i) {
                    protocols.push(ProtocolRegistry.getName(i));
                }
            }
        }
        
        if (transmitProtocolsDisplay) {
            transmitProtocolsDisplay.textContent = `Current: ${protocols.join(', ') || 'None'}`;
        }
    },

    updateProtocolParams(meshcoreFreq, meshcoreBw, meshtasticFreq, meshtasticBw) {
        const meshcoreFreqInput = document.getElementById('meshcoreFreqInput');
        const meshcoreBwSelect = document.getElementById('meshcoreBwSelect');
        const meshtasticFreqInput = document.getElementById('meshtasticFreqInput');
        const meshtasticBwSelect = document.getElementById('meshtasticBwSelect');
        
        // Only update if form is not dirty (checked by caller)
        if (meshcoreFreqInput) meshcoreFreqInput.value = (meshcoreFreq / 1000000).toFixed(3);
        if (meshcoreBwSelect) meshcoreBwSelect.value = meshcoreBw.toString();
        if (meshtasticFreqInput) meshtasticFreqInput.value = (meshtasticFreq / 1000000).toFixed(3);
        if (meshtasticBwSelect) meshtasticBwSelect.value = meshtasticBw.toString();
    },

    updateFrequencies(meshcoreFreq, meshtasticFreq) {
        document.getElementById('meshcoreFreq').textContent = (meshcoreFreq / 1000000).toFixed(3) + ' MHz';
        document.getElementById('meshtasticFreq').textContent = (meshtasticFreq / 1000000).toFixed(3) + ' MHz';
    },

    updateLastPacket(protocol, rssi, snr, data) {
        const protocolName = protocol === 0 ? 'MeshCore' : 'Meshtastic';
        const element = document.getElementById('lastPacket');
        
        let hex = '';
        let ascii = '';
        for (let i = 0; i < Math.min(data.length, 32); i++) {
            hex += data[i].toString(16).padStart(2, '0').toUpperCase() + ' ';
            ascii += (data[i] >= 32 && data[i] < 127) ? String.fromCharCode(data[i]) : '.';
        }
        
        element.innerHTML = `
            <div><strong>Protocol:</strong> ${protocolName}</div>
            <div><strong>RSSI:</strong> ${rssi} dBm | <strong>SNR:</strong> ${snr} dB</div>
            <div><strong>Length:</strong> ${data.length} bytes</div>
            <div class="packet-hex" style="margin-top: 10px;">
                <div class="bytes">${hex}</div>
                <div class="ascii">${ascii}</div>
            </div>
        `;
    },

    addLogEntry(message, type = 'info') {
        const log = document.getElementById('eventLog');
        const entry = document.createElement('div');
        
        // Auto-detect error type if message starts with "ERR:"
        if (message.startsWith('ERR:') && type === 'info') {
            type = 'error';
        }
        
        entry.className = `log-entry ${type}`;
        const timestamp = new Date().toLocaleTimeString();
        entry.textContent = `[${timestamp}] ${message}`;
        log.insertBefore(entry, log.firstChild);
        
        // Keep only last 100 entries
        while (log.children.length > 100) {
            log.removeChild(log.lastChild);
        }
    },

    enableButtons(connected) {
        document.getElementById('connectBtn').disabled = connected;
        document.getElementById('disconnectBtn').disabled = !connected;
        document.getElementById('resetStatsBtn').disabled = !connected;
        document.getElementById('testMeshCoreBtn').disabled = !connected;
        document.getElementById('testMeshtasticBtn').disabled = !connected;
        document.getElementById('testBothBtn').disabled = !connected;
        document.getElementById('saveSettingsBtn').disabled = !connected;
        document.getElementById('cancelSettingsBtn').disabled = !connected;
        document.getElementById('protocolSelect').disabled = !connected;
        document.getElementById('listenProtocolSelect').disabled = !connected;
        // Update transmit protocol checkboxes dynamically
        for (let i = 0; i < ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox) {
                checkbox.disabled = !connected;
            }
        }
        // Interval input and other controls enabled state is controlled by protocol mode
        const protocolSelect = document.getElementById('protocolSelect');
        const intervalInput = document.getElementById('switchIntervalInput');
        if (intervalInput && protocolSelect) {
            const isAutoSwitch = parseInt(protocolSelect.value) === 2;
            intervalInput.disabled = !connected || !isAutoSwitch;
        }
        document.getElementById('meshcoreFreqInput').disabled = !connected;
        document.getElementById('meshcoreBwSelect').disabled = !connected;
        document.getElementById('meshtasticFreqInput').disabled = !connected;
        document.getElementById('meshtasticBwSelect').disabled = !connected;
    }
};

// Make UI available globally
window.UI = UI;
