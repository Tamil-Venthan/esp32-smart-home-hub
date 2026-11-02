# 🛠️ Flashing Tools & USB Driver Guide

This directory provides 1-click automated batch scripts for restoring and flashing production firmware binaries to fresh boards without requiring PlatformIO or VS Code.

---

## ⚡ Flashing Scripts
* **`flash_esp32_wroom.bat`**: Auto-detects serial COM ports and flashes the ESP32-WROOM (4MB) with bootloader, partitions, boot_app0, and production firmware.
* **`flash_esp32_s3.bat`**: Auto-detects serial COM ports, writes 16MB partitions, firmware, and custom DIO mode bootloader at `0x00000000`.

---

## 🔌 USB Driver Installation
If your computer fails to recognize the ESP32 when plugged in via USB:
1. Most ESP32 dev boards use the **Silicon Labs CP2102/CP2104** or **WCH CH340** USB-to-UART bridge.
2. For CP210x: Check `CP210x_Windows_Driver/` in the root workspace or download the official VCP driver from [Silicon Labs CP210x Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).
3. In Windows Device Manager, ensure a valid `Silicon Labs CP210x USB to UART Bridge (COMx)` appears under "Ports (COM & LPT)".

---

## 💻 Manual Flashing with `esptool.py` (Command Line)

### ESP32-WROOM-32 (4MB):
```bash
esptool.py --chip esp32 --port COM8 --baud 921600 write_flash -z \
  0x1000 firmware/esp32_wroom_production/bin/bootloader.bin \
  0x8000 firmware/esp32_wroom_production/bin/partitions.bin \
  0xe000 firmware/esp32_wroom_production/bin/boot_app0.bin \
  0x10000 firmware/esp32_wroom_production/bin/firmware.bin
```

### ESP32-S3 (16MB):
```bash
esptool.py --chip esp32s3 --port COM5 --baud 921600 write_flash \
  0x8000 firmware/esp32_s3_16mb/bin/partitions.bin \
  0xe000 firmware/esp32_s3_16mb/bin/boot_app0.bin \
  0x10000 firmware/esp32_s3_16mb/bin/firmware.bin

# Write DIO Bootloader at 0x0:
esptool.py --chip esp32s3 --port COM5 write_flash \
  0x00000000 firmware/esp32_s3_16mb/bin/bootloader_dio.bin
```
