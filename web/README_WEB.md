# MeshCore-Meshtastic Proxy Web Interface

A Progressive Web App (PWA) for monitoring and controlling the MeshCore-Meshtastic proxy device via WebSerial API.

## Features

- **Real-time Statistics**: Monitor packet counts for both protocols
- **Protocol Status**: See which protocol is currently being listened to
- **Packet Viewer**: View last received packet with hex dump
- **Event Log**: Real-time log of proxy activity
- **WebSerial Communication**: Direct USB serial communication with the proxy

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

1. Click "Connect Proxy" button
2. Select the COM port for your LoRa32u4-II device
3. The interface will automatically start polling statistics
4. View real-time packet flow and protocol status

## Building for Production

```bash
npm run build
```

Output will be in the `dist/` directory.
