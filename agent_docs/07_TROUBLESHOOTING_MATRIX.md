# 🩺 Troubleshooting & Diagnostics Matrix for AI Agents

When assisting a user with hardware or firmware bugs, check this matrix before suggesting code modifications.

---

## 🔍 Diagnostic Matrix

| # | Observed Symptom | Probable Root Cause | Verified Engineering Solution |
| :-: | :--- | :--- | :--- |
| **1** | `rst:0x10 (RTCWDT_RTC_RESET)` or `Brownout detector was triggered` | Relays powered from the ESP32's onboard 3.3V rail, or inadequate 5V power supply capacity. | Power relays from dedicated 5V rail on HLK-10M05 module or external MB-102 power supply. Verify common GND is connected. Ensure 1000µF bulk capacitor is installed across the 5V rail. |
| **2** | ESP32-S3 fails to boot after flashing (`rst:0x3 (SW_RESET), boot:0x28 (SPI_FAST_FLASH_BOOT)` loop) | Flash mode mismatch: Octal SPI vs Dual I/O (DIO) mode bootloader configuration. | Flash `bootloader_dio.bin` at offset `0x00000000`. Set `board_build.flash_mode = dio` in `platformio.ini`. |
| **3** | Relays chatter or double-click when flipping a physical domestic wall switch | 1. 10µF zero-crossing smoothing capacitor missing on PC814 optocoupler board.<br>2. `DEBOUNCE_DELAY_MS` set too low. | Verify 10µF ceramic capacitor is installed across each channel's optocoupler output. Ensure `DEBOUNCE_DELAY_MS >= 100ms` in `config.h`. |
| **4** | DHT11 returns `NaN` or constant read errors | 1. Reading sensor faster than DHT11 response time (1 Hz max).<br>2. Missing 4.7kΩ pull-up resistor on single-bus data line. | Ensure `DHT_READ_INTERVAL >= 2500ms`. Verify 4.7kΩ pull-up resistor between `DHT_PIN` and `+3.3V`. |
| **5** | Mobile phone connects to `SmartHome_Offline` AP but `192.168.4.1` fails to load | Mobile OS (Android/iOS) detects "Wi-Fi has no internet access" and silently redirects HTTP requests to cellular 4G/5G data. | Instruct user to temporarily disable mobile data on phone, or tap "Keep Wi-Fi Connection" when prompted by Android/iOS. Access via `http://smarthome.local` or direct IP `http://192.168.4.1`. |
| **6** | ESP RainMaker device stays offline in mobile app | ESP32 connected to 5GHz Wi-Fi network or router client isolation is active. | ESP32-WROOM only supports **2.4 GHz Wi-Fi**. Connect smartphone to a dedicated 2.4 GHz SSID during provisioning. Ensure AP client isolation is disabled in router settings. |
| **7** | ESP32 fails to flash over USB serial (`Failed to connect to ESP32: No serial data received`) | 1. Missing CP210x or CH340 USB driver.<br>2. Auto-reset circuit failed to pull GPIO 0 LOW. | Install CP210x driver from `CP210x_Windows_Driver`. Hold down `BOOT` button on ESP32 while plugging in USB cable or executing flash script. |
| **8** | ESP32 boots into bootloader download mode automatically on power up | An external sensor or relay line is pulling `GPIO 0` LOW or `GPIO 12` HIGH during power up. | Check pinning in `agent_docs/02_PINOUT_AND_HARDWARE_MAP.md`. Verify no external loads hold strapping pins LOW at boot. |
| **9** | Web OTA upload fails midway through flashing | Dual-OTA partition boundaries exceeded, or partition table mismatch. | Verify compiled binary size is under 1.8MB (`ota_0` / `ota_1` partition limit). Use `partitions_wroom_4MB.csv`. |
