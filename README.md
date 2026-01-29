# MeshCore-Meshtastic Proxy

A bidirectional proxy device that bridges MeshCore and Meshtastic mesh networks, allowing nodes from both protocols to communicate with each other.

## Hardware Requirements

### Device: LoRa32u4-II-Lora-Atmega32u4-SX1276

- **MCU**: ATMega32u4 @ 8MHz
- **Radio**: SX1276 LoRa module
- **Antenna**: 915MHz (US region)
- **Vendor**: [BSFrance](https://bsfrance.fr/lora-long-range/1345-LoRa32u4-II-Lora-LiPo-Atmega32u4-SX1276-HPD13-868MHZ-EU-Antenna.html)

### Pin Connections

The LoRa32u4-II board has the following pin mappings:

- **SPI Interface**:
  - NSS (CS): Pin 8
  - MOSI: Pin 16
  - MISO: Pin 14
  - SCK: Pin 15

- **SX1276 Control Pins**:
  - RST: Pin 4
  - DIO0: Pin 7
  - DIO1: Pin 5 (must be manually wired for board versions < 1.3)

- **LED**: Pin 13 (built-in LED)

### Important Note on DIO1

For LoRa32u4-II board versions **before 1.3**, DIO1 must be manually wired from the SX1276 module to GPIO pin 5. Check the label on your PCB - version 1.3 has DIO1 already wired on the PCB.

## Frequency Configuration

The device defaults to **853MHz**, but this proxy configures it to **910.525 MHz** to match MeshCore's default frequency. Meshtastic uses **906.875 MHz**.

- **MeshCore Frequency**: 910.525 MHz
- **Meshtastic Frequency**: 906.875 MHz

## Protocol Details

### MeshCore Packet Format

Wire format (as defined in `Packet.cpp`):
```
- Header byte (bits: route_type[0:1], payload_type[2:5], version[6:7])
- Optional: 4 bytes transport codes (if route type has transport)
  - transport_codes[0] (2 bytes, little-endian)
  - transport_codes[1] (2 bytes, little-endian)
- Path length byte (path_len)
- Path data (variable, max 64 bytes)
- Payload data (variable, max 184 bytes)
```

**LoRa Parameters**:
- Spreading Factor: 7
- Bandwidth: 125 kHz
- Coding Rate: 5
- Sync Word: 0x12
- Preamble: 8 bytes

### Meshtastic Packet Format

Wire format (as defined in `RadioInterface.cpp`):
```
- PacketHeader (16 bytes):
  - to (NodeNum, 4 bytes, little-endian)
  - from (NodeNum, 4 bytes, little-endian)
  - id (PacketId, 4 bytes, little-endian)
  - flags (1 byte: hop_limit[0:2], want_ack[3], via_mqtt[4], hop_start[5:7])
  - channel (1 byte: channel hash)
  - next_hop (1 byte: last byte of NodeNum)
  - relay_node (1 byte: last byte of NodeNum)
- Payload: encrypted.bytes (protobuf encoded, max 237 bytes)
```

**LoRa Parameters**:
- Spreading Factor: 7
- Bandwidth: 125 kHz
- Coding Rate: 5
- Sync Word: 0x2B
- Preamble: 16 bytes

## How It Works

The proxy operates using **time-slicing** to handle both protocols with a single radio:

1. **Time-Sliced Reception**: Alternates between listening for MeshCore packets (sync word 0x12) and Meshtastic packets (sync word 0x2B) every 100ms
2. **Packet Detection**: Uses sync word and packet structure to identify the protocol
3. **Protocol Conversion**: Converts packet format between MeshCore and Meshtastic
4. **Retransmission**: Immediately retransmits converted packets with appropriate LoRa parameters

### Conversion Process

**MeshCore → Meshtastic**:
- Extracts payload from MeshCore packet
- Creates Meshtastic header with broadcast destination (0xFFFFFFFF)
- Wraps MeshCore payload as Meshtastic encrypted payload
- Retransmits with Meshtastic LoRa parameters

**Meshtastic → MeshCore**:
- Parses Meshtastic 16-byte header
- Extracts protobuf payload
- Creates MeshCore packet with FLOOD route type
- Uses RAW_CUSTOM payload type to preserve protobuf data
- Retransmits with MeshCore LoRa parameters

## Building and Uploading

### Prerequisites

1. Install [PlatformIO](https://platformio.org/)
2. Connect the LoRa32u4-II board via USB
3. Ensure DIO1 is wired to pin 5 (if board version < 1.3)

### Build and Upload

```bash
# Navigate to project directory
cd MeshcoreMeshstaticProxy

# Build the project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Working with Multiple Projects

The workspace file `MeshcoreMeshstaticProxy.code-workspace` includes both this project and the ELRSDongle project. Open this workspace file in VS Code/Cursor to work with multiple PlatformIO projects simultaneously.

## Web Interface

The proxy includes a web-based control interface similar to the ELRSDongle project.

### Setup Web Interface

```bash
cd web
npm install
npm run dev
```

Then open `http://localhost:8001` in Chrome/Edge browser.

### Features

- Real-time statistics monitoring
- Protocol status display
- Last received packet viewer
- Event log
- Statistics reset

See `web/README_WEB.md` for detailed web interface documentation.

### PlatformIO Configuration

The project is configured for the `lora32u4II` board:
- Platform: `atmelavr`
- Framework: `arduino`
- MCU: `atmega32u4` @ 8MHz
- Upload speed: 19200 baud

## Usage

1. **Power on** the LoRa32u4-II board
2. **Connect via USB** to view serial output (115200 baud)
3. The proxy will automatically start listening for packets
4. **LED indicator**: Blinks briefly when a packet is received and retransmitted

### Serial Output

The proxy prints statistics every 10 seconds:
```
MeshCore RX: 5 | Meshtastic RX: 3 | MeshCore TX: 3 | Meshtastic TX: 5 | Errors: 0
```

- **MeshCore RX**: Packets received from MeshCore nodes
- **Meshtastic RX**: Packets received from Meshtastic nodes
- **MeshCore TX**: Packets transmitted as MeshCore format
- **Meshtastic TX**: Packets transmitted as Meshtastic format
- **Errors**: Conversion or transmission errors

## Configuration

Edit `src/config.h` to adjust:

- **Frequencies**: `DEFAULT_FREQUENCY_HZ` (MeshCore) and `MESHTASTIC_FREQUENCY_HZ`
- **LoRa Parameters**: Spreading Factor, Bandwidth, Coding Rate, Sync Words
- **Time-Slicing**: `PROTOCOL_SWITCH_INTERVAL_MS` (default: 100ms)

## Limitations

1. **Single Radio**: Cannot receive both protocols simultaneously - requires time-slicing
2. **Packet Loss**: Time-slicing may cause missed packets if both protocols transmit simultaneously
3. **Protocol Conversion**: Some metadata may be lost during conversion (e.g., exact routing paths)
4. **Memory Constraints**: ATMega32u4 has limited RAM (2.5KB) - buffer sizes are optimized
5. **Frequency Switching**: The proxy switches between 910.525 MHz (MeshCore) and 906.875 MHz (Meshtastic) - ensure both frequencies are within your antenna's range

## Troubleshooting

### No Packets Received

1. **Check antenna**: Ensure 915MHz antenna is properly connected
2. **Verify frequency**: Confirm MeshCore nodes are using 910.525 MHz
3. **Check sync words**: Ensure nodes are using correct sync words (0x12 for MeshCore, 0x2B for Meshtastic)
4. **DIO1 wiring**: Verify DIO1 is connected to pin 5 (for versions < 1.3)

### Conversion Errors

- Check serial output for error count
- Verify packet sizes are within limits (255 bytes max)
- Ensure LoRa parameters match between proxy and nodes

### Radio Not Initializing

- Check SPI connections (NSS, MOSI, MISO, SCK)
- Verify reset pin (pin 4) is connected
- Check SX1276 version register returns 0x12

## Project Structure

```
MeshcoreMeshstaticProxy/
├── platformio.ini                    # PlatformIO configuration
├── MeshcoreMeshstaticProxy.code-workspace  # VS Code workspace
├── README.md                         # This file
├── src/
│   ├── main.cpp                      # Main proxy loop
│   ├── config.h                      # Configuration constants
│   ├── sx1276.h                      # SX1276 driver header
│   ├── sx1276.cpp                    # SX1276 driver implementation
│   ├── meshcore_handler.h            # MeshCore packet handler
│   ├── meshcore_handler.cpp
│   ├── meshtastic_handler.h          # Meshtastic packet handler
│   ├── meshtastic_handler.cpp
│   ├── usb_comm.h                    # USB communication header
│   └── usb_comm.cpp                  # USB communication implementation
└── web/                               # Web interface
    ├── index.html                    # Main HTML page
    ├── package.json                  # Node.js dependencies
    ├── vite.config.js                # Vite configuration
    ├── css/
    │   └── styles.css               # Stylesheet
    └── js/
        ├── main.js                   # Entry point
        ├── protocol.js               # Protocol definitions
        ├── serial.js                 # WebSerial communication
        ├── ui.js                     # UI update functions
        └── app.js                    # Main application logic
```

## References

- [MeshCore Source Code](https://github.com/meshtastic/MeshCore)
- [Meshtastic Firmware](https://github.com/meshtastic/firmware)
- [LoRa32u4-II Board Documentation](https://docs.platformio.org/en/latest/boards/atmelavr/lora32u4II.html)
- [SX1276 Datasheet](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276)

## License

This project is provided as-is for bridging MeshCore and Meshtastic networks.
