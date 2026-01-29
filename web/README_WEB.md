# MeshCore-Meshtastic Proxy Web Interface

A Progressive Web App (PWA) for monitoring and controlling the MeshCore-Meshtastic proxy device via WebSerial API.

## Features

### Real-time Statistics
- **Packet Counts**: Monitor RX/TX counts for both MeshCore and Meshtastic protocols
- **Conversion Errors**: Highlighted in red with pulsing animation when errors occur
- **Auto-refresh**: Statistics update every second automatically

### Protocol Status Indicators
- **Current Listening Protocol**: Shows which protocol is actively listening (green border)
- **Activity Status**: Separate badges for MeshCore and Meshtastic showing if packets have been received
  - "Active" (blue) - Protocol has received/transmitted packets
  - "Inactive" (gray) - No activity detected
- **Last Packet Timestamps**: Shows when last packet was received on each protocol
- **Frequency Display**: Shows configured frequencies for each protocol

### Device Status Panel
- **Current Activity**: Real-time status (Idle, Listening, Processing Packet)
- **Uptime**: Connection duration since last connect
- **Protocol Switches**: Count of protocol switches (time-slicing)
- **Last Activity**: Time since last packet activity

### Test Message Transmission
- **Test MeshCore TX**: Sends test message on MeshCore frequency (910.525 MHz)
- **Test Meshtastic TX**: Sends test message on Meshtastic frequency (906.875 MHz)
- **Test Both TX**: Sends test messages on both protocols sequentially
- **Debug Output**: Event log shows transmission status and frequency information
- **LED Indicator**: Device LED blinks when test messages are transmitted

### Packet Viewer
- **Last Received Packet**: Hex dump with protocol identification
- **Signal Quality**: RSSI and SNR values for each packet
- **Packet Details**: Length, protocol type, and timestamp

### Event Log
- **Real-time Activity**: Timestamped log of all proxy events
- **Color-coded**: Info (gray), Success (green), Error (red)
- **Auto-scroll**: Newest entries appear at top
- **Limited History**: Keeps last 100 entries

### WebSerial Communication
- **Direct USB Connection**: No drivers needed (uses WebSerial API)
- **Automatic Polling**: Requests statistics and protocol info automatically
- **Error Handling**: Graceful disconnection and reconnection support

## Requirements

- **Browser**: Chrome/Edge (WebSerial API support required)
- **Node.js**: For development server (optional, can serve static files)

## Setup

1. Install dependencies:
```bash
cd web
npm install
```

2. Start development server:
```bash
npm run dev
```

Or use the batch file:
```bash
serve.bat
```

3. Open browser to `http://localhost:8001`

## Usage

1. **Connect to Proxy**:
   - Click "Connect Proxy" button
   - Select the COM port for your LoRa32u4-II device from the browser's port selector
   - Connection status indicator will turn green when connected

2. **Monitor Activity**:
   - Statistics update automatically every second
   - Protocol status shows which protocol is currently listening
   - Device status panel shows real-time activity

3. **Send Test Messages**:
   - Click "Test MeshCore TX" to transmit on 910.525 MHz
   - Click "Test Meshtastic TX" to transmit on 906.875 MHz
   - Click "Test Both TX" to test both protocols
   - Check event log for transmission status
   - Use spectrum analyzer to verify transmission at correct frequencies

4. **View Packets**:
   - Last received packet appears automatically in packet viewer
   - Event log shows details of each received packet (RSSI, SNR, length)

5. **Debug Issues**:
   - Check conversion errors (highlighted in red if > 0)
   - Review event log for detailed activity
   - Monitor protocol switches to understand time-slicing behavior
   - Verify test message transmission success/failure

## Understanding the Status Indicators

### Protocol Status Panel
- **Green Border**: Protocol currently being listened to (time-slicing)
- **Blue Badge "Active"**: Protocol has received/transmitted packets
- **Gray Badge "Inactive"**: No packet activity detected
- **"Listening: Yes/No"**: Shows which protocol is in the current time slice

### Time-Slicing Behavior
The device alternates between protocols every 100ms:
- **MeshCore**: Listens for 100ms, then switches
- **Meshtastic**: Listens for 100ms, then switches
- This means packets may be missed if they arrive during the other protocol's time slice
- The "Protocol Switches" counter shows how many times it has switched

## Building for Production

```bash
npm run build
```

Output will be in the `dist/` directory.

## Troubleshooting

### Connection Issues
- **"WebSerial API not supported"**: Use Chrome or Edge browser
- **Port not found**: Ensure device is connected and drivers are installed
- **Connection fails**: Try unplugging and replugging USB cable

### No Statistics Updating
- **Check connection**: Ensure "Connected" status is shown
- **Verify firmware**: Make sure latest firmware is uploaded
- **Check serial port**: Ensure no other program is using the COM port

### Test Messages Not Working
- **Check event log**: Look for "TX success" or "TX failed" messages
- **Verify frequencies**: Test messages use 910.525 MHz (MeshCore) and 906.875 MHz (Meshtastic)
- **Check LED**: Device LED should blink when test message is sent
- **Spectrum analyzer**: Look at specific frequencies, not just "915 MHz"

### Conversion Errors
- **Red highlighting**: Errors are automatically highlighted when count > 0
- **Check packet sizes**: Ensure packets are within 255 byte limit
- **Verify LoRa parameters**: Sync words and parameters must match nodes
- **Review event log**: Look for specific error messages
