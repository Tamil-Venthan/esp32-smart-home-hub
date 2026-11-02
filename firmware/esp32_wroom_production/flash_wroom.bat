@echo off
setlocal enabledelayedexpansion
title ESP32-WROOM 1-Click Production Flasher
color 0A

echo ================================================================
echo     ESP32-WROOM 4-CHANNEL RELAY + DHT11 CLIMATE CONTROLLER
echo           1-Click Production Firmware Flasher Tool
echo ================================================================
echo.

:: Detect Python environment
set PYTHON="%USERPROFILE%\.platformio\penv\Scripts\python.exe"
if not exist %PYTHON% (
    set PYTHON=python
)

:: Locate esptool.py
set ESPTOOL="%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py"
if not exist %ESPTOOL% (
    for /f "delims=" %%i in ('where esptool.py 2^>nul') do set ESPTOOL="%%i"
)

:: Locate binaries
set BIN_DIR=%~dp0bin
if not exist "%BIN_DIR%\firmware.bin" (
    echo [ERROR] Cannot find firmware binaries in "%BIN_DIR%"!
    echo Please make sure firmware.bin, bootloader.bin, partitions.bin, and boot_app0.bin exist in bin\
    pause
    exit /b 1
)

:: Detect COM Port via PowerShell
set DETECTED_PORT=
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames() | Select-Object -First 1"') do (
    set DETECTED_PORT=%%p
)

if "%DETECTED_PORT%"=="" (
    set DEFAULT_PORT=COM8
) else (
    set DEFAULT_PORT=%DETECTED_PORT%
)

echo Available COM Ports:
powershell -NoProfile -Command "$ports = [System.IO.Ports.SerialPort]::GetPortNames(); if ($ports) { $ports } else { 'No active COM ports detected. (Plug in your ESP32 via USB cable)' }"
echo.
set /p PORT="Enter ESP32 COM Port [Default: %DEFAULT_PORT%]: "
if "%PORT%"=="" set PORT=%DEFAULT_PORT%

echo.
echo ----------------------------------------------------------------
echo Target Port:   %PORT%
echo Target Chip:   ESP32-WROOM (ESP32)
echo Flash Speed:   921600 baud (DIO @ 40MHz)
echo Flashing files:
echo   - 0x1000  : bootloader.bin
echo   - 0x8000  : partitions.bin
echo   - 0xe000  : boot_app0.bin
echo   - 0x10000 : firmware.bin
echo ----------------------------------------------------------------
echo.
echo Connecting and flashing ESP32 on %PORT%...
echo (If flashing fails to start, hold down the BOOT button on your ESP32!)
echo.

%PYTHON% %ESPTOOL% --chip esp32 --port %PORT% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 "%BIN_DIR%\bootloader.bin" 0x8000 "%BIN_DIR%\partitions.bin" 0xe000 "%BIN_DIR%\boot_app0.bin" 0x10000 "%BIN_DIR%\firmware.bin"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================================================
    echo    SUCCESS! ESP32-WROOM Flashed and Booted Successfully!
    echo ================================================================
    echo.
    echo [Hardware Pin Configuration]
    echo   - Relays:      GPIO 23 (Light 1), GPIO 22 (Light 2), GPIO 21 (Fan), GPIO 19 (Appliance)
    echo   - Switches:    GPIO 32 (SW1), GPIO 33 (SW2), GPIO 25 (SW3), GPIO 26 (SW4)
    echo   - DHT11 Temp:  GPIO 4
    echo   - Status LED:  GPIO 2
    echo   - BOOT Button: GPIO 0 (Long press ^> 3s to reset Wi-Fi provisioning)
    echo.
    echo [Next Steps for New ESP32]
    echo   1. Open ESP RainMaker mobile app on iPhone or Android.
    echo   2. Tap '+' -^> Add Device -^> Scan QR Code (open rainmaker_qr.html).
    echo   3. Or connect via Bluetooth (Name: PROV_ESP32_WROOM, PoP: wroom123).
    echo   4. If Wi-Fi is unavailable, connect to 'SmartHome_Offline' AP (Pass: 12345678)
    echo      and visit http://192.168.4.1/ or http://smarthome.local/
    echo ================================================================
) else (
    echo.
    echo ================================================================
    echo [ERROR] Flashing failed!
    echo 1. Check that the ESP32 is plugged in securely.
    echo 2. Verify the correct COM port in Device Manager.
    echo 3. Hold down the physical 'BOOT' button on the board while plugging in or starting.
    echo ================================================================
)

echo.
pause
