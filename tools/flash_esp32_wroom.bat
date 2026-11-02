@echo off
setlocal enabledelayedexpansion
title ESP32-WROOM 1-Click Production Flasher
color 0A

echo ================================================================
echo     ESP32-WROOM 4-CHANNEL RELAY + DHT11 CLIMATE CONTROLLER
echo           1-Click Production Firmware Flasher Tool
echo ================================================================
echo.

:: Detect Python
set PYTHON="C:\Users\tamil\.platformio\penv\Scripts\python.exe"
if not exist %PYTHON% (
    for /f "delims=" %%i in ('where python 2^>nul') do (
        set PYTHON="%%i"
        goto :found_py
    )
)
:found_py

:: Detect esptool.py
set ESPTOOL="C:\Users\tamil\.platformio\packages\tool-esptoolpy\esptool.py"
if not exist %ESPTOOL% (
    for /f "delims=" %%i in ('where esptool.py 2^>nul') do (
        set ESPTOOL="%%i"
        goto :found_esptool
    )
)
:found_esptool

set BIN_DIR=%~dp0..\firmware\esp32_wroom_production\bin
if not exist "%BIN_DIR%\firmware.bin" (
    echo [ERROR] Firmware binary not found at: %BIN_DIR%\firmware.bin
    pause
    exit /b 1
)

:: Auto-detect COM Port
set DETECTED_PORT=
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames() | Select-Object -First 1"') do (
    set DETECTED_PORT=%%p
)
if "%DETECTED_PORT%"=="" set DETECTED_PORT=COM8

echo Available Serial Ports:
powershell -NoProfile -Command "$ports = [System.IO.Ports.SerialPort]::GetPortNames(); if ($ports) { $ports } else { 'No active COM ports detected. Plug in ESP32 via USB.' }"
echo.

set /p PORT="Enter ESP32 COM Port [Default: %DETECTED_PORT%]: "
if "%PORT%"=="" set PORT=%DETECTED_PORT%

echo.
echo [1/3] Connecting to ESP32 on %PORT%...
%PYTHON% %ESPTOOL% --chip esp32 --port %PORT% chip_id
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Could not connect to ESP32 on %PORT%. Hold BOOT button and try again!
    pause
    exit /b %errorlevel%
)

echo.
echo [2/3] Flashing Full Firmware Package...
%PYTHON% %ESPTOOL% --chip esp32 --port %PORT% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 "%BIN_DIR%\bootloader.bin" 0x8000 "%BIN_DIR%\partitions.bin" 0xe000 "%BIN_DIR%\boot_app0.bin" 0x10000 "%BIN_DIR%\firmware.bin"

if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Flashing failed!
    pause
    exit /b %errorlevel%
)

echo.
echo ================================================================
echo    SUCCESS! ESP32-WROOM Firmware Flashed Successfully!
echo ================================================================
echo.
pause
