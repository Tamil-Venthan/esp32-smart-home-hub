# 📌 Pinout & Hardware Specification Matrix

This document is the definitive hardware reference for modifying pin allocations, adding peripheral sensors, or debugging boot conflicts.

---

## ⚡ Master Pin Mapping: ESP32-WROOM vs ESP32-S3

| Function / Peripheral | ESP32-WROOM-32 (DevKit V1) | ESP32-S3 (DevKitC-1 N16R8) | Electrical Characteristics & Logic |
| :--- | :---: | :---: | :--- |
| **Relay Channel 1 (Light 1)** | `GPIO 23` | `GPIO 10` | Digital Output, Active LOW (0 = ON, 1 = OFF) |
| **Relay Channel 2 (Light 2)** | `GPIO 22` | `GPIO 11` | Digital Output, Active LOW (0 = ON, 1 = OFF) |
| **Relay Channel 3 (Fan)** | `GPIO 21` | `GPIO 12` | Digital Output, Active LOW (0 = ON, 1 = OFF) |
| **Relay Channel 4 (Appliance)** | `GPIO 19` | `GPIO 13` | Digital Output, Active LOW (0 = ON, 1 = OFF) |
| **Wall Switch 1 (Opto CH1)** | `GPIO 32` | `GPIO 1` | Digital Input, Internal Pull-Up, Active LOW |
| **Wall Switch 2 (Opto CH2)** | `GPIO 33` | `GPIO 2` | Digital Input, Internal Pull-Up, Active LOW |
| **Wall Switch 3 (Opto CH3)** | `GPIO 25` | `GPIO 41` | Digital Input, Internal Pull-Up, Active LOW |
| **Wall Switch 4 (Opto CH4)** | `GPIO 26` | `GPIO 42` | Digital Input, Internal Pull-Up, Active LOW |
| **DHT11 Sensor (1-Wire Data)**| `GPIO 4` | *(Optional/Ext)* | Bi-directional Single-Bus with 4.7kΩ pull-up |
| **Local Status LED** | `GPIO 2` (Blue LED) | `GPIO 48` (RGB WS2812) | WROOM: HIGH=ON. S3: Single-wire NeoPixel protocol |
| **Local Manual Switch** | `GPIO 0` (BOOT) | `GPIO 0` (BOOT) | Active LOW momentary push button |

---

## ⚠️ Strapping Pins & Prohibited GPIO Constraints

### ESP32-WROOM-32 Strapping Table
| GPIO | Function at Boot | Default Required State | Agent Rule / Restriction |
| :---: | :--- | :---: | :--- |
| **GPIO 0** | Boot mode select | HIGH (via pull-up) | Used as local BOOT toggle. Do NOT hold LOW during power-on. |
| **GPIO 2** | Flashing mode select | LOW or Floating | Connected to onboard Blue LED. Safe for output. |
| **GPIO 5** | SDIO / Timing | HIGH during boot | Avoid connecting to relays that clamp state at boot. |
| **GPIO 12** | Flash Voltage (MTDI) | **LOW (3.3V flash)** | **CRITICAL:** Pulling this HIGH forces 1.8V flash, causing immediate boot crash. |
| **GPIO 15** | Silent Boot / MTDO | HIGH during boot | Output pin. Flashes debug characters during boot. |
| **GPIO 6–11**| Integrated Flash Bus | Reserved | **NEVER ASSIGN.** Internally connected to SPI Flash memory IC. |
| **GPIO 34–39**| Input-Only Pins | Input only | **NO internal pull-up/pull-down resistors.** External resistors required. |

### ESP32-S3 DevKitC-1 Strapping Table
| GPIO | Function at Boot | Agent Rule / Restriction |
| :---: | :--- | :--- |
| **GPIO 0** | Bootloader enable | Hold LOW to force serial flashing mode. |
| **GPIO 45** | VDD_SPI voltage selection | Internal pull-down. Do not bias externally. |
| **GPIO 46** | ROM message printing | Pull-down. Controls UART debug baud print. |
| **GPIO 26–32**| Octal SPI Flash / PSRAM | **NEVER ASSIGN** on N16R8 modules (connected to internal PSRAM & Flash). |

---

## 🔌 Available Free GPIOs for Future Expansion

### On ESP32-WROOM-32:
* **I2C Bus (e.g., OLED SSD1306 Display, RTC DS3231, BME280):**
  * `SDA`: Recommended `GPIO 18` or `GPIO 5`
  * `SCL`: Recommended `GPIO 17` or `GPIO 16`
* **Hardware UART (e.g., PZEM-004T Energy Meter, Nextion Touch Display):**
  * `UART2 TX`: `GPIO 17`
  * `UART2 RX`: `GPIO 16`
* **Analog ADC (e.g., CT Current Sensor, LDR Light Sensor):**
  * Recommended: `GPIO 34`, `GPIO 35`, `GPIO 36` (ADC1 channels — **Safe while Wi-Fi is active! ADC2 cannot be used when Wi-Fi is transmitting!**)

### On ESP32-S3 DevKitC-1:
* **I2C Bus:** `GPIO 8` (SDA) and `GPIO 9` (SCL)
* **SPI Bus (e.g., SD Card, TFT Display):** `GPIO 14` (CLK), `GPIO 21` (MOSI), `GPIO 47` (MISO)
* **Analog ADC (ADC1):** `GPIO 4`, `GPIO 5`, `GPIO 6`, `GPIO 7`
