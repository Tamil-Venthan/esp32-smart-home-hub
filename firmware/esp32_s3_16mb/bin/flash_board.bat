@echo off
title ESP32-S3 Smart Home 1-Click Flash Tool
color 0A

echo ============================================================
echo      ESP32-S3 SMART HOME 1-CLICK FIRMWARE RESTORE TOOL
echo      RainMaker Cloud + Offline Fallback Web Portal
echo ============================================================
echo.

set /p PORT="Enter ESP32-S3 COM Port (e.g. COM5) [Default: COM5]: "
if "%PORT%"=="" set PORT=COM5

echo.
echo [1/4] Detecting ESP32-S3 on %PORT%...
"C:\Users\tamil\.platformio\penv\Scripts\python.exe" "C:\Users\tamil\.platformio\packages\tool-esptoolpy\esptool.py" --port %PORT% chip_id
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Could not connect to ESP32-S3 on %PORT%. Check cable or port!
    pause
    exit /b %errorlevel%
)

echo.
echo [2/4] Flashing Firmware and Partitions...
"C:\Users\tamil\.platformio\penv\Scripts\python.exe" "C:\Users\tamil\.platformio\packages\tool-esptoolpy\esptool.py" --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x00008000 partitions.bin 0x0000e000 boot_app0.bin 0x00010000 firmware.bin
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Firmware flash failed!
    pause
    exit /b %errorlevel%
)

echo.
echo [3/4] Writing Hardware DIO Bootloader (0x00000000)...
"C:\Users\tamil\.platformio\penv\Scripts\python.exe" "C:\Users\tamil\.platformio\packages\tool-esptoolpy\esptool.py" --chip esp32s3 --port %PORT% write_flash 0x00000000 bootloader_dio.bin
if %errorlevel% neq 0 (
    color 0C
    echo [ERROR] Bootloader flash failed!
    pause
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo    SUCCESS! ESP32-S3 Smart Home is Flashed and Ready!
echo ============================================================
echo.
pause
