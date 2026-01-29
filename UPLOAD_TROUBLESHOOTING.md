# Upload Troubleshooting Guide

## The Problem: "Programmer is not responding"

The LoRa32u4-II uses the ATMega32u4's built-in bootloader (avr109 protocol). This bootloader requires **manual activation** by double-pressing the RESET button.

## Step-by-Step Upload Process

### Method 1: Manual Reset (Most Reliable)

1. **Connect the board** via USB to your computer
2. **Close any serial monitors** or programs using the COM port
3. **Double-press the RESET button** quickly (within 1 second)
   - First press: Board resets
   - Second press (within 1 second): Enters bootloader mode
4. **Wait 2 seconds** after the double-press
5. **Immediately run the upload command**:
   ```powershell
   pio run --target upload
   ```
   - You have about **8 seconds** to complete the upload after entering bootloader mode

### Method 2: Using upload.bat Script

1. Double-click `upload.bat`
2. When prompted, **double-press RESET** button
3. Wait 2 seconds
4. Press any key to continue
5. Upload will start automatically

### Method 3: PlatformIO Extension

1. Open PlatformIO sidebar in VS Code/Cursor
2. **Before clicking Upload**: Double-press RESET button
3. Wait 2 seconds
4. Click "PROJECT TASKS" → "lora32u4II" → "General" → "Upload"
5. Upload must complete within 8 seconds

## Common Issues and Solutions

### Issue: "butterfly_recv(): programmer is not responding"

**Cause**: Bootloader is not active

**Solutions**:
1. Double-press RESET button RIGHT BEFORE uploading
2. Make sure you're uploading within 8 seconds of double-press
3. Try unplugging and replugging USB cable, then double-press reset
4. Close any serial monitors or terminal programs using the COM port

### Issue: Upload starts but fails partway through

**Solutions**:
1. Try slower upload speed (already set to 19200)
2. Use a shorter USB cable (long cables can cause timing issues)
3. Try a different USB port (prefer USB 2.0 ports)
4. Close other programs that might be using USB resources

### Issue: Wrong COM port detected

**Solutions**:
1. Check Device Manager for the correct COM port
2. Manually specify port:
   ```powershell
   pio run --target upload --upload-port COM51
   ```
   (Replace COM51 with your actual port)
3. List available ports:
   ```powershell
   pio device list
   ```

### Issue: Port disappears after reset

**This is normal!** The COM port will:
- Disappear when you press RESET
- Reappear after entering bootloader mode
- Disappear again during upload
- Reappear when firmware starts

## Alternative: Use Arduino IDE

If PlatformIO continues to have issues, you can use Arduino IDE:

1. Install Arduino IDE
2. Install board support: Tools → Board → Boards Manager → Search "LoRa32u4"
3. Select: Tools → Board → LoRa32u4-II
4. Select COM port
5. Double-press RESET button
6. Click Upload

## Bootloader Timing

The bootloader mode is active for approximately **8 seconds** after double-pressing RESET. You must:
- Double-press RESET
- Wait 2 seconds (for port to stabilize)
- Start upload within remaining ~6 seconds
- Complete upload before bootloader times out

## Still Having Issues?

1. **Check USB cable**: Use a data-capable USB cable (not charge-only)
2. **Try different USB port**: Some USB 3.0 ports have compatibility issues
3. **Update drivers**: Install latest CH340/CP2102 drivers if needed
4. **Check board**: Verify board LEDs indicate power/reset activity
5. **Manual bootloader entry**: Some boards may need triple-press or hold RESET

## Verification

After successful upload, you should see:
- Serial output starting with "MeshCore-Meshtastic Proxy Starting..."
- "SX1276 initialized successfully"
- "Proxy ready. Listening for packets..."

If you see this output, the upload was successful!
