# 💾 NVS Storage & RainMaker Parameter Dictionary

This document specifies the exact memory schema, cloud data model, and local REST API contracts.

---

## 💾 1. Non-Volatile Storage (NVS) Schema

The ESP32 uses the Arduino `Preferences` abstraction over ESP-IDF NVS flash partitions to persist state across power failures and OTA reboots.

### Namespace 1: `"relays"`
Used to record the physical ON/OFF state of each relay.

| Key | Type | Default | Description | Write Trigger |
| :---: | :---: | :---: | :--- | :--- |
| `"r0"` | `bool` | `false` | Channel 1 (Light 1) | Any toggle (Cloud, Web, Switch, BOOT) |
| `"r1"` | `bool` | `false` | Channel 2 (Light 2) | Any toggle (Cloud, Web, Switch) |
| `"r2"` | `bool` | `false` | Channel 3 (Fan) | Any toggle (Cloud, Web, Switch) |
| `"r3"` | `bool` | `false` | Channel 4 (Appliance)| Any toggle (Cloud, Web, Switch) |

### Namespace 2: `"names"`
Used to store custom user-defined appliance labels.

| Key | Type | Default | Max Length | Description |
| :---: | :---: | :---: | :---: | :--- |
| `"n0"` | `String` | `"Light 1"` | 24 chars | Custom label for Channel 1 |
| `"n1"` | `String` | `"Light 2"` | 24 chars | Custom label for Channel 2 |
| `"n2"` | `String` | `"Fan"` | 24 chars | Custom label for Channel 3 |
| `"n3"` | `String` | `"Appliance"` | 24 chars | Custom label for Channel 4 |

> [!IMPORTANT]
> Always call `prefs.end()` after completing read/write sequences to ensure dirty cache pages are committed to SPI flash and to prevent flash wear.

---

## ☁️ 2. ESP RainMaker Data Model

The device registers on AWS IoT Core under a single **Node** holding individual **Devices**:

### Device 1 & 2: `Light 1` and `Light 2`
* **Type:** `ESP_RMAKER_DEVICE_LIGHTBULB`
* **Primary Parameter:** `ESP_RMAKER_DEF_POWER_NAME` (`"Power"`, Boolean, Read/Write)
* **Voice Mapping:** Treated natively as lighting by Google Home & Alexa (*"Turn on Light 1"*).

### Device 3: `Fan`
* **Type:** `ESP_RMAKER_DEVICE_FAN`
* **Primary Parameter:** `ESP_RMAKER_DEF_POWER_NAME` (`"Power"`, Boolean, Read/Write)
* **Voice Mapping:** Treated as a fan switch (*"Turn off the Fan"*).  
  *(Note: Speed slider parameter intentionally omitted to prevent user confusion with binary relay switches).*

### Device 4: `Appliance`
* **Type:** `ESP_RMAKER_DEVICE_SWITCH`
* **Primary Parameter:** `ESP_RMAKER_DEF_POWER_NAME` (`"Power"`, Boolean, Read/Write)
* **Voice Mapping:** Treated as a smart plug / outlet.

### Device 5: `Climate Monitor` (ESP32-WROOM Build)
* **Type:** `ESP_RMAKER_DEVICE_TEMP_SENSOR`
* **Parameters:**
  * `"Temperature"` (`float`, Read-Only, `°C`)
  * `"Humidity"` (`float`, Read-Only, `%`)
  * `"Chip Temp"` (`float`, Read-Only, `°C`)
  * `"WiFi Quality"` (`int`, Read-Only, `0-100%`)

---

## 🌐 3. Offline Web Portal REST API Endpoints

The embedded WebServer on port 80 exposes the following REST API used by the local web application:

### `GET /status`
Returns complete real-time JSON state:
```json
{
  "relays": [false, true, false, false],
  "names": ["Living Light", "Balcony Light", "Ceiling Fan", "Geyser"],
  "temp": 28.4,
  "humidity": 62.0,
  "chip_temp": 47.8,
  "wifi_rssi": -65,
  "wifi_quality": 70,
  "uptime": 86400
}
```

### `GET /toggle?relay=[0-3]`
Toggles the designated relay channel and returns updated JSON state.

### `GET /set?relay=[0-3]&state=[0|1]`
Explicitly drives the relay channel (`0` = OFF, `1` = ON).

### `GET /master?state=[0|1]`
Master switch command:
- `?state=1`: Switches all 4 channels ON simultaneously.
- `?state=0`: Switches all 4 channels OFF simultaneously.

### `GET /rename?ch=[0-3]&name=[URLEncodedString]`
Persists new custom label to NVS namespace `"names"` and updates the local web page immediately.

### `GET /update` & `POST /update`
Dual-OTA browser flashing interface. Accepts raw `.bin` firmware file uploads via multipart form data.
