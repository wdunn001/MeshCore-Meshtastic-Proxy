# Upload Instructions

## Quick Upload Steps

1. **Connect the board** via USB
2. **Double-press the RESET button** quickly (this puts it in bootloader mode)
3. **Wait 2 seconds** after double-pressing
4. **Run the upload command** (see below)

## Using the Batch Script (Easiest)

Double-click `upload.bat` and follow the on-screen instructions.

## Using PlatformIO CLI

```powershell
cd H:\MeshcoreMeshstaticProxy

# Method 1: Auto-detect port (recommended)
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload

# Method 2: Specify port manually
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM51
```

## Using PlatformIO Extension in VS Code/Cursor

1. Open the project in VS Code/Cursor
2. Click PlatformIO icon in sidebar
3. Click "PROJECT TASKS" → "lora32u4II" → "General" → "Upload"
4. **Before clicking Upload**: Double-press the RESET button on your board
5. Click Upload within 2-3 seconds

## Troubleshooting Upload Issues

### "Programmer is not responding"
- **Solution**: Double-press the RESET button right before uploading
- The board needs to be in bootloader mode
- You have about 8 seconds after entering bootloader mode

### "Port not found" or "COM port error"
- Check Device Manager for the COM port number
- Try unplugging and replugging the USB cable
- Make sure no other program is using the serial port (close serial monitors)

### Upload keeps failing
1. Unplug USB cable
2. Wait 5 seconds
3. Plug USB cable back in
4. Double-press RESET button
5. Wait 2 seconds
6. Try upload again

### Wrong COM port detected
- Manually specify port: `--upload-port COM51` (replace COM51 with your port)
- Check available ports: `pio device list`

## Bootloader Mode

The LoRa32u4-II uses the ATMega32u4's built-in bootloader. To enter bootloader mode:
- **Double-press RESET** button quickly (within 1 second)
- The board will enter bootloader mode for ~8 seconds
- Upload must complete within this window

## After Successful Upload

The firmware will start automatically. You should see serial output:
```
SX1276 initialized successfully
Proxy ready. Listening for packets...
MeshCore Frequency: 910.525 MHz
Meshtastic Frequency: 906.875 MHz
```

## Serial Monitor

To view serial output:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor
```

Or use PlatformIO extension: "PROJECT TASKS" → "Monitor"
