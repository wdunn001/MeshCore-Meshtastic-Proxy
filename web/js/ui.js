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
        // Legacy function - elements may not exist if using Chart.js statistics
        const element = document.getElementById('meshcoreRx');
        if (element) {
            element.textContent = count.toLocaleString();
        }
    },

    updateMeshtasticRx(count) {
        // Legacy function - elements may not exist if using Chart.js statistics
        const element = document.getElementById('meshtasticRx');
        if (element) {
            element.textContent = count.toLocaleString();
        }
    },

    updateMeshCoreTx(count) {
        // Legacy function - elements may not exist if using Chart.js statistics
        const element = document.getElementById('meshcoreTx');
        if (element) {
            element.textContent = count.toLocaleString();
        }
    },

    updateMeshtasticTx(count) {
        // Legacy function - elements may not exist if using Chart.js statistics
        const element = document.getElementById('meshtasticTx');
        if (element) {
            element.textContent = count.toLocaleString();
        }
    },

    updateConversionErrors(count) {
        // Legacy function - elements may not exist if using Chart.js statistics
        const element = document.getElementById('conversionErrors');
        if (element) {
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

    updateDeviceStatus(activity, uptime, protocolSwitches, lastActivity, platform) {
        const platformEl = document.getElementById('devicePlatform');
        const activityEl = document.getElementById('deviceActivity');
        const uptimeEl = document.getElementById('deviceUptime');
        const switchesEl = document.getElementById('protocolSwitches');
        const lastActivityEl = document.getElementById('lastActivity');
        
        if (platformEl) platformEl.textContent = platform || '--';
        if (activityEl) activityEl.textContent = activity || 'Idle';
        if (uptimeEl) uptimeEl.textContent = uptime || '--';
        if (switchesEl) switchesEl.textContent = (protocolSwitches || 0).toLocaleString();
        if (lastActivityEl) lastActivityEl.textContent = lastActivity || '--';
    },

    updateSwitchInterval(intervalMs) {
        // Auto-switch removed - this function kept for compatibility but does nothing
        return;
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
        // Auto-switch removed - this function kept for compatibility but does nothing
        // Protocol mode UI elements removed from HTML
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
        // DEPRECATED: This function is kept for compatibility but should not be used
        // Use updateTransmitProtocolsFromListen() in app.js instead
        // This function is only here in case it's called from somewhere else
        console.warn('updateTransmitProtocols() called - this may reset checkbox states. Use updateTransmitProtocolsFromListen() instead.');
        
        // Update visibility based on listen protocol
        // Count visible options
        let visibleCount = 0;
        let visibleCheckbox = null;
        
        for (let i = 0; i < ProtocolRegistry.PROTOCOL_COUNT; i++) {
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
        }
        
        // Update display text based on current checkbox states
        this.updateTransmitProtocolsDisplay();
    },

    updateTransmitProtocolsDisplay() {
        // Update display text based on current checkbox states
        const transmitProtocolsDisplay = document.getElementById('transmitProtocolsDisplay');
        if (!transmitProtocolsDisplay) return;
        
        const protocols = [];
        for (let i = 0; i < ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox && checkbox.checked) {
                protocols.push(ProtocolRegistry.getName(i));
            }
        }
        
        transmitProtocolsDisplay.textContent = `Current: ${protocols.join(', ') || 'None'}`;
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
        // protocolSelect removed - auto-switch removed from UI
        document.getElementById('listenProtocolSelect').disabled = !connected;
        // Update transmit protocol checkboxes dynamically
        for (let i = 0; i < ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const checkbox = document.getElementById(`transmitProtocol${i}`);
            if (checkbox) {
                checkbox.disabled = !connected;
            }
        }
        // Auto-switch removed - interval input removed from UI
        document.getElementById('meshcoreFreqInput').disabled = !connected;
        document.getElementById('meshcoreBwSelect').disabled = !connected;
        document.getElementById('meshtasticFreqInput').disabled = !connected;
        document.getElementById('meshtasticBwSelect').disabled = !connected;
    }
};

// Make UI available globally
window.UI = UI;
