# 🏠 ESP32-WROOM-32 Smart Home Firmware (Production)

Production-grade firmware build for standard **ESP32-WROOM-32 (4MB Flash)** boards.

---

## 🌟 Features Included
* **ESP RainMaker Cloud:** Free remote control, Alexa & Google Home skills, on-device RTC schedules.
* **4-CH AC Optocoupler Wall Switch Sync:** 2-way sync with physical domestic switches with 120ms anti-bounce.
* **DHT11 Climate Monitoring:** Ambient temperature and humidity logging.
* **Silicon Telemetry:** Real-time ESP32 internal die temperature and Wi-Fi RSSI.
* **Fallback Offline Web Portal:** Spins AP `SmartHome_Offline` at `192.168.4.1` if Wi-Fi drops.
* **Inline Channel Renaming:** Save custom appliance labels to NVS flash via web UI.
* **Web OTA Updates:** Upgrade firmware wirelessly at `http://<IP>/update`.

---

## ⚡ Pin Mapping
* **Relays (Active LOW):** `GPIO 23` (Light 1), `GPIO 22` (Light 2), `GPIO 21` (Fan), `GPIO 19` (Appliance).
* **Wall Switches (Optocoupler):** `GPIO 32` (SW1), `GPIO 33` (SW2), `GPIO 25` (SW3), `GPIO 26` (SW4).
* **DHT11 Data:** `GPIO 4` (with 4.7kΩ pull-up).
* **Status LED:** `GPIO 2` (Onboard Blue LED).
* **BOOT Button:** `GPIO 0`.

---

## 🚀 1-Click Flashing
Double-click `flash_wroom.bat` to flash the precompiled binaries in `bin/` over USB.

## 🛠️ Building with PlatformIO
```bash
pio run
pio run --target upload
pio device monitor -b 115200
```
