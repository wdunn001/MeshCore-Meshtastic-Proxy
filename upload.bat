@echo off
echo ========================================
echo MeshCore-Meshtastic Proxy Upload Helper
echo ========================================
echo.
echo INSTRUCTIONS:
echo 1. Make sure your LoRa32u4-II board is connected via USB
echo 2. Press and release the RESET button TWICE quickly (double-press)
echo 3. Wait 2 seconds after double-pressing reset
echo 4. The upload will start automatically
echo.
echo If upload fails, try:
echo - Unplug and replug the USB cable
echo - Double-press reset button again
echo - Check Device Manager for COM port
echo.
pause
echo.
echo Starting upload...
echo.

cd /d %~dp0
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" run --target upload

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Upload successful!
    echo ========================================
    echo.
    echo Opening serial monitor...
    timeout /t 2 /nobreak >nul
    "%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" device monitor
) else (
    echo.
    echo ========================================
    echo Upload failed!
    echo ========================================
    echo.
    echo Troubleshooting:
    echo 1. Make sure board is connected
    echo 2. Try double-pressing reset button
    echo 3. Check COM port in Device Manager
    echo 4. Try unplugging and replugging USB
    echo.
)

pause
