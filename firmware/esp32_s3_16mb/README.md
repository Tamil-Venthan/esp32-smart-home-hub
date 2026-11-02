# 🏠 ESP32-S3 DevKitC-1 Smart Home Firmware (16MB Flash)

Production firmware build for **ESP32-S3 DevKitC-1 (N16R8, 16MB Flash)**.

---

## 🌟 Features Included
* **ESP RainMaker Cloud:** Remote worldwide control, Google Assistant & Alexa voice control.
* **Onboard WS2812 RGB LED Indicator:** Real-time visual feedback on `GPIO 48` (Green = Online, Blue = AP mode, Cyan = Action acknowledged).
* **Fallback Offline Web Portal:** Embedded dark-mode PWA served at `192.168.4.1`.
* **Power Outage State Recovery:** Restores physical relay states from NVS memory upon boot.
* **BOOT Push Button:** Local toggle on `GPIO 0`.

---

## ⚡ Pin Mapping
* **Relays (Active LOW):** `GPIO 10` (Light 1), `GPIO 11` (Light 2), `GPIO 12` (Fan), `GPIO 13` (Appliance).
* **Wall Switches (Optocoupler):** `GPIO 1` (SW1), `GPIO 2` (SW2), `GPIO 41` (SW3), `GPIO 42` (SW4).
* **RGB LED (WS2812):** `GPIO 48`.
* **BOOT Button:** `GPIO 0`.

---

## 🚀 1-Click Flashing
Double-click `bin/flash_board.bat` or run `tools/flash_esp32_s3.bat`.

## 🛠️ Building with PlatformIO
```bash
pio run
pio run --target upload
```
