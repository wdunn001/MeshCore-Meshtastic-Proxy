# Building and Uploading Instructions

## Option 1: Using PlatformIO Extension in VS Code/Cursor (Recommended)

1. **Open the workspace:**
   - Open `MeshcoreMeshstaticProxy.code-workspace` in VS Code/Cursor
   - Or open the `MeshcoreMeshstaticProxy` folder directly

2. **Install PlatformIO Extension (if not already installed):**
   - Press `Ctrl+Shift+X` to open Extensions
   - Search for "PlatformIO IDE"
   - Click Install

3. **Build the project:**
   - Click the PlatformIO icon in the left sidebar (ant icon)
   - Click "PROJECT TASKS" → "lora32u4II" → "General" → "Build"
   - Or use the checkmark icon (✓) in the bottom status bar
   - Or press `Ctrl+Alt+B`

4. **Upload to device:**
   - Connect your LoRa32u4-II board via USB
   - Click "PROJECT TASKS" → "lora32u4II" → "General" → "Upload"
   - Or use the arrow icon (→) in the bottom status bar
   - Or press `Ctrl+Alt+U`

5. **Monitor serial output:**
   - Click "PROJECT TASKS" → "lora32u4II" → "Monitor"
   - Or use the plug icon (🔌) in the bottom status bar
   - Or press `Ctrl+Alt+S`

## Option 2: Install PlatformIO CLI

If you prefer command line:

1. **Install Python** (if not installed):
   - Download from https://www.python.org/downloads/
   - Make sure to check "Add Python to PATH" during installation

2. **Install PlatformIO CLI:**
   ```powershell
   python -m pip install --upgrade pip
   python -m pip install platformio
   ```

3. **Add to PATH (optional):**
   - PlatformIO will be installed in: `%USERPROFILE%\.platformio\penv\Scripts\`
   - Add this to your PATH environment variable

4. **Build and upload:**
   ```powershell
   cd H:\MeshcoreMeshstaticProxy
   pio run                    # Build
   pio run --target upload    # Upload
   pio device monitor         # Monitor serial
   ```

## Troubleshooting

### Port not found
- Make sure the device is connected via USB
- Check Device Manager for COM port number
- You can specify port manually: `pio run --target upload --upload-port COM3`

### Upload fails
- Try pressing the reset button on the board right before upload
- Some boards need to be in bootloader mode (double-press reset)
- Check upload speed in `platformio.ini` (currently 19200)

### Build errors
- Make sure all source files are in `src/` directory
- Check that `platformio.ini` has correct board configuration
- Try cleaning: `pio run --target clean`

## Quick Commands Reference

```powershell
# Navigate to project
cd H:\MeshcoreMeshstaticProxy

# Build
pio run

# Upload
pio run --target upload

# Clean build
pio run --target clean

# Monitor serial
pio device monitor

# List devices
pio device list
```
