@echo off
setlocal enabledelayedexpansion
title ESP32-S3 1-Click Production Flasher
color 0A

echo ================================================================
echo     ESP32-S3 DEVKITC-1 (16MB) SMART HOME CONTROLLER
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

set BIN_DIR=%~dp0..\firmware\esp32_s3_16mb\bin
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
if "%DETECTED_PORT%"=="" set DETECTED_PORT=COM5

echo Available Serial Ports:
powershell -NoProfile -Command "$ports = [System.IO.Ports.SerialPort]::GetPortNames(); if ($ports) { $ports } else { 'No active COM ports detected. Plug in ESP32-S3 via USB.' }"
echo.

set /p PORT="Enter ESP32-S3 COM Port [Default: %DETECTED_PORT%]: "
if "%PORT%"=="" set PORT=%DETECTED_PORT%

echo.
echo [1/3] Connecting to ESP32-S3 on %PORT%...
%PYTHON% %ESPTOOL% --chip esp32s3 --port %PORT% chip_id
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Could not connect to ESP32-S3 on %PORT%. Hold BOOT button and try again!
    pause
    exit /b %errorlevel%
)

echo.
echo [2/3] Flashing Firmware, Partitions, and Boot Data...
%PYTHON% %ESPTOOL% --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x00008000 "%BIN_DIR%\partitions.bin" 0x0000e000 "%BIN_DIR%\boot_app0.bin" 0x00010000 "%BIN_DIR%\firmware.bin"
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Firmware flash failed!
    pause
    exit /b %errorlevel%
)

echo.
echo [3/3] Writing Custom DIO Bootloader (0x00000000)...
%PYTHON% %ESPTOOL% --chip esp32s3 --port %PORT% write_flash 0x00000000 "%BIN_DIR%\bootloader_dio.bin"
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Bootloader flash failed!
    pause
    exit /b %errorlevel%
)

echo.
echo ================================================================
echo    SUCCESS! ESP32-S3 Firmware Flashed Successfully!
echo ================================================================
echo.
pause
