// Statistics module with Chart.js integration
// Provides combined RX/TX graphs with all protocols as different lines

import { Chart, registerables } from 'chart.js';
Chart.register(...registerables);

const Statistics = {
    charts: {}, // Store chart instances (rxChart, txChart, errorsChart)
    history: {}, // Store historical data for charts
    maxHistoryPoints: 20, // Keep last 20 data points for charts
    protocolColors: [
        '#0066cc', // Blue (MeshCore)
        '#28a745', // Green (Meshtastic)
        '#ffc107', // Yellow
        '#dc3545', // Red
        '#17a2b8', // Cyan
        '#6f42c1'  // Purple
    ],
    
    // Helper to convert hex to rgba
    hexToRgba(hex, alpha) {
        const r = parseInt(hex.slice(1, 3), 16);
        const g = parseInt(hex.slice(3, 5), 16);
        const b = parseInt(hex.slice(5, 7), 16);
        return `rgba(${r}, ${g}, ${b}, ${alpha})`;
    },
    
    init() {
        // Initialize history storage for each protocol
        this.currentValues = {};
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            this.history[i] = {
                rx: [],
                tx: [],
                timestamps: []
            };
            this.currentValues[i] = { rx: 0, tx: 0 };
        }
        
        // Create combined charts
        this.createCharts();
    },
    
    createCharts() {
        const statsContainer = document.getElementById('statsContainer');
        if (!statsContainer) return;
        
        // Clear existing content
        statsContainer.innerHTML = '';
        
        // Create datasets for each protocol
        const rxDatasets = [];
        const txDatasets = [];
        
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            const protocolName = window.ProtocolRegistry.getName(i);
            const color = this.protocolColors[i % this.protocolColors.length];
            
            rxDatasets.push({
                label: protocolName,
                data: [],
                borderColor: color,
                backgroundColor: this.hexToRgba(color, 0.1),
                borderWidth: 2,
                fill: false,
                tension: 0.4,
                pointRadius: 0
            });
            
            txDatasets.push({
                label: protocolName,
                data: [],
                borderColor: color,
                backgroundColor: this.hexToRgba(color, 0.1),
                borderWidth: 2,
                fill: false,
                tension: 0.4,
                pointRadius: 0
            });
        }
        
        // Create RX graph section
        const rxSection = document.createElement('div');
        rxSection.className = 'protocol-stats-section';
        rxSection.innerHTML = `
            <div class="kpi-card">
                <div class="kpi-header">
                    <label>RX Packets (All Protocols)</label>
                    <div class="kpi-protocol-values" id="rxProtocolValues"></div>
                </div>
                <div class="kpi-chart-wrapper">
                    <canvas id="chartRx" class="kpi-chart"></canvas>
                </div>
            </div>
        `;
        statsContainer.appendChild(rxSection);
        
        // Create TX graph section
        const txSection = document.createElement('div');
        txSection.className = 'protocol-stats-section';
        txSection.innerHTML = `
            <div class="kpi-card">
                <div class="kpi-header">
                    <label>TX Packets (All Protocols)</label>
                    <div class="kpi-protocol-values" id="txProtocolValues"></div>
                </div>
                <div class="kpi-chart-wrapper">
                    <canvas id="chartTx" class="kpi-chart"></canvas>
                </div>
            </div>
        `;
        statsContainer.appendChild(txSection);
        
        // Create RX chart with all protocols
        const rxCtx = document.getElementById('chartRx');
        if (rxCtx) {
            this.charts.rx = new Chart(rxCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: rxDatasets
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { 
                            display: true,
                            position: 'bottom',
                            labels: {
                                boxWidth: 12,
                                padding: 8,
                                font: { size: 10 },
                                usePointStyle: true
                            }
                        },
                        tooltip: { 
                            enabled: true,
                            mode: 'index',
                            intersect: false
                        }
                    },
                    scales: {
                        x: { display: false },
                        y: { 
                            display: false,
                            beginAtZero: true
                        }
                    },
                    elements: {
                        point: { radius: 0 }
                    },
                    interaction: {
                        mode: 'index',
                        intersect: false
                    }
                }
            });
        }
        
        // Create TX chart with all protocols
        const txCtx = document.getElementById('chartTx');
        if (txCtx) {
            this.charts.tx = new Chart(txCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: txDatasets
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { 
                            display: true,
                            position: 'bottom',
                            labels: {
                                boxWidth: 12,
                                padding: 8,
                                font: { size: 10 },
                                usePointStyle: true
                            }
                        },
                        tooltip: { 
                            enabled: true,
                            mode: 'index',
                            intersect: false
                        }
                    },
                    scales: {
                        x: { display: false },
                        y: { 
                            display: false,
                            beginAtZero: true
                        }
                    },
                    elements: {
                        point: { radius: 0 }
                    },
                    interaction: {
                        mode: 'index',
                        intersect: false
                    }
                }
            });
        }
        
        // Add errors card
        const errorsSection = document.createElement('div');
        errorsSection.className = 'protocol-stats-section';
        errorsSection.innerHTML = `
            <div class="kpi-card error-card">
                <div class="kpi-header">
                    <div style="display: flex; align-items: center; gap: 8px;">
                        <label>Errors</label>
                        <button class="expand-graph-btn" id="expandErrorsBtn" title="Show/Hide Error Graph">
                            <svg width="16" height="16" viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg">
                                <path d="M8 3L8 13M3 8L13 8" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                            </svg>
                        </button>
                    </div>
                    <div class="kpi-value error-value" id="statErrors">0</div>
                </div>
                <div class="kpi-chart-wrapper errors-chart-wrapper" id="errorsChartWrapper" style="display: none;">
                    <canvas id="chartErrors" class="kpi-chart"></canvas>
                </div>
            </div>
        `;
        statsContainer.appendChild(errorsSection);
        
        // Add click handler for expand/collapse button
        const expandBtn = document.getElementById('expandErrorsBtn');
        const errorsChartWrapper = document.getElementById('errorsChartWrapper');
        if (expandBtn && errorsChartWrapper) {
            expandBtn.addEventListener('click', () => {
                const isHidden = errorsChartWrapper.style.display === 'none';
                errorsChartWrapper.style.display = isHidden ? 'block' : 'none';
                // Update button icon (rotate or change)
                const svg = expandBtn.querySelector('svg');
                if (svg) {
                    if (isHidden) {
                        // Show minus icon when expanded
                        svg.innerHTML = '<path d="M3 8L13 8" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>';
                    } else {
                        // Show plus icon when collapsed
                        svg.innerHTML = '<path d="M8 3L8 13M3 8L13 8" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>';
                    }
                }
                // Resize chart when shown
                if (isHidden && this.charts.errors) {
                    setTimeout(() => {
                        this.charts.errors.resize();
                    }, 100);
                }
            });
        }
        
        // Create errors chart
        const errorsCtx = document.getElementById('chartErrors');
        if (errorsCtx) {
            this.charts.errors = new Chart(errorsCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Errors',
                        data: [],
                        borderColor: '#dc3545',
                        backgroundColor: 'rgba(220, 53, 69, 0.1)',
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        pointRadius: 0
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: false },
                        tooltip: { enabled: false }
                    },
                    scales: {
                        x: { display: false },
                        y: { 
                            display: false,
                            beginAtZero: true
                        }
                    },
                    elements: {
                        point: { radius: 0 }
                    }
                }
            });
        }
        
        this.errorsHistory = [];
    },
    
    updateProtocolStats(protocolId, rxCount, txCount) {
        // Update current values for this protocol (store latest value)
        if (!this.currentValues) {
            this.currentValues = {};
        }
        this.currentValues[protocolId] = { rx: rxCount, tx: txCount };
        
        // Update history for this protocol
        const now = Date.now();
        
        // Check if we need to add a new timestamp point (synchronize across all protocols)
        let shouldAddNewPoint = false;
        if (!this.history[0] || !this.history[0].timestamps.length) {
            shouldAddNewPoint = true;
        } else {
            const lastTimestamp = this.history[0].timestamps[this.history[0].timestamps.length - 1];
            if (now - lastTimestamp > 500) { // Add new point every 500ms
                shouldAddNewPoint = true;
            }
        }
        
        if (shouldAddNewPoint) {
            // Add new synchronized timestamp point for all protocols
            for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                const currentRx = this.currentValues[i] ? this.currentValues[i].rx : 0;
                const currentTx = this.currentValues[i] ? this.currentValues[i].tx : 0;
                
                this.history[i].rx.push(currentRx);
                this.history[i].tx.push(currentTx);
                this.history[i].timestamps.push(now);
                
                // Keep only last N points
                if (this.history[i].rx.length > this.maxHistoryPoints) {
                    this.history[i].rx.shift();
                    this.history[i].tx.shift();
                    this.history[i].timestamps.shift();
                }
            }
            
            // Update combined RX chart (all protocols in one chart)
            const rxChart = this.charts.rx;
            if (rxChart) {
                rxChart.data.labels = this.history[0].timestamps.map(() => '');
                for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                    if (rxChart.data.datasets[i]) {
                        rxChart.data.datasets[i].data = this.history[i].rx;
                    }
                }
                rxChart.update('none');
            }
            
            // Update combined TX chart (all protocols in one chart)
            const txChart = this.charts.tx;
            if (txChart) {
                txChart.data.labels = this.history[0].timestamps.map(() => '');
                for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                    if (txChart.data.datasets[i]) {
                        txChart.data.datasets[i].data = this.history[i].tx;
                    }
                }
                txChart.update('none');
            }
        } else {
            // Update the last point with current values
            const lastIdx = this.history[protocolId].rx.length - 1;
            if (lastIdx >= 0) {
                this.history[protocolId].rx[lastIdx] = rxCount;
                this.history[protocolId].tx[lastIdx] = txCount;
                
                // Update charts immediately
                const rxChart = this.charts.rx;
                if (rxChart && rxChart.data.datasets[protocolId]) {
                    rxChart.data.datasets[protocolId].data[lastIdx] = rxCount;
                    rxChart.update('none');
                }
                
                const txChart = this.charts.tx;
                if (txChart && txChart.data.datasets[protocolId]) {
                    txChart.data.datasets[protocolId].data[lastIdx] = txCount;
                    txChart.update('none');
                }
            }
        }
        
        // Update protocol values display
        this.updateProtocolValuesDisplay();
    },
    
    updateProtocolValuesDisplay() {
        // Update RX values display
        const rxValuesEl = document.getElementById('rxProtocolValues');
        if (rxValuesEl) {
            const values = [];
            for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                const protocolName = window.ProtocolRegistry.getName(i);
                const currentRx = this.currentValues[i] ? this.currentValues[i].rx : 0;
                values.push(`${protocolName}: ${currentRx.toLocaleString()}`);
            }
            rxValuesEl.textContent = values.join(' | ');
            rxValuesEl.className = 'kpi-protocol-values';
        }
        
        // Update TX values display
        const txValuesEl = document.getElementById('txProtocolValues');
        if (txValuesEl) {
            const values = [];
            for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
                const protocolName = window.ProtocolRegistry.getName(i);
                const currentTx = this.currentValues[i] ? this.currentValues[i].tx : 0;
                values.push(`${protocolName}: ${currentTx.toLocaleString()}`);
            }
            txValuesEl.textContent = values.join(' | ');
            txValuesEl.className = 'kpi-protocol-values';
        }
    },
    
    updateErrors(errorCount) {
        const errorsElement = document.getElementById('statErrors');
        if (errorsElement) {
            errorsElement.textContent = errorCount.toLocaleString();
            if (errorCount > 0) {
                errorsElement.classList.add('error-value');
            } else {
                errorsElement.classList.remove('error-value');
            }
        }
        
        // Toggle red background on error card based on error count
        const errorCard = errorsElement?.closest('.error-card');
        if (errorCard) {
            if (errorCount > 0) {
                errorCard.classList.add('has-errors');
            } else {
                errorCard.classList.remove('has-errors');
            }
        }
        
        const errorsChart = this.charts.errors;
        if (errorsChart) {
            this.errorsHistory.push(errorCount);
            
            // Keep only last N points
            if (this.errorsHistory.length > this.maxHistoryPoints) {
                this.errorsHistory.shift();
            }
            
            errorsChart.data.labels = this.errorsHistory.map(() => '');
            errorsChart.data.datasets[0].data = this.errorsHistory;
            errorsChart.update('none');
        }
    },
    
    reset() {
        // Reset all charts and history
        this.currentValues = {};
        for (let i = 0; i < window.ProtocolRegistry.PROTOCOL_COUNT; i++) {
            this.history[i] = {
                rx: [],
                tx: [],
                timestamps: []
            };
            this.currentValues[i] = { rx: 0, tx: 0 };
            
            // Clear datasets in combined charts
            if (this.charts.rx && this.charts.rx.data.datasets[i]) {
                this.charts.rx.data.datasets[i].data = [];
            }
            if (this.charts.tx && this.charts.tx.data.datasets[i]) {
                this.charts.tx.data.datasets[i].data = [];
            }
        }
        
        // Update combined charts
        if (this.charts.rx) {
            this.charts.rx.data.labels = [];
            this.charts.rx.update();
        }
        if (this.charts.tx) {
            this.charts.tx.data.labels = [];
            this.charts.tx.update();
        }
        
        // Reset errors
        this.errorsHistory = [];
        const errorsChart = this.charts.errors;
        if (errorsChart) {
            errorsChart.data.datasets[0].data = [];
            errorsChart.data.labels = [];
            errorsChart.update();
        }
        
        // Update display
        this.updateProtocolValuesDisplay();
    }
};

// Make Statistics available globally
window.Statistics = Statistics;

export default Statistics;
