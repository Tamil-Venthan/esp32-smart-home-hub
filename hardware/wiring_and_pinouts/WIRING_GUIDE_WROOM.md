# 🔌 ESP32-WROOM Hardware Wiring & Interconnection Guide

## 📋 Pin Interconnection Diagram

```
                             +-----------------------------------+
                             | HLK-10M05 Dual-Rail Power Board   |
                             +-----------------------------------+
                                       |                 |
                                     +5.0V             +3.3V
                                       │                 │
                                       │    Common GND   │
                                       ├────────┬────────┘
                                       │        │
                   ┌───────────────────┘        └───────────────────┐
                   ▼                                                ▼
     +---------------------------+                    +---------------------------+
     | 4-Channel 5V Relay Board  |                    | ESP32-WROOM DevKit V1     |
     +---------------------------+                    +---------------------------+
     | VCC  <-- +5V Rail         |                    | 3V3  <-- +3.3V Rail       |
     | GND  <-- Common Ground    |                    | GND  <-- Common Ground    |
     | IN1  <---------------------------------------- | GPIO 23 (Relay 1 - Light) |
     | IN2  <---------------------------------------- | GPIO 22 (Relay 2 - Light) |
     | IN3  <---------------------------------------- | GPIO 21 (Relay 3 - Fan)   |
     | IN4  <---------------------------------------- | GPIO 19 (Relay 4 - App)   |
     +---------------------------+                    | GPIO 4  <-- DHT11 Data    |
                                                      | GPIO 2  --> Status LED    |
                                                      | GPIO 0  <-- BOOT Button   |
     +---------------------------+                    +---------------------------+
     | 4-CH PC814 Optocoupler Bd |                              ▲ ▲ ▲ ▲
     +---------------------------+                              │ │ │ │
     | 3V3  <-- +3.3V Rail       |                              │ │ │ │
     | GND  <-- Common Ground    |                              │ │ │ │
     | OUT1 ────────────────────────────────────────────────────┘ │ │ │ (GPIO 32)
     | OUT2 ──────────────────────────────────────────────────────┘ │ │ (GPIO 33)
     | OUT3 ────────────────────────────────────────────────────────┘ │ (GPIO 25)
     | OUT4 ──────────────────────────────────────────────────────────┘ (GPIO 26)
     | J1 (SW1-SW4) <-- 230V Wall Switches                                       |
     | J1 (AC_N)    <-- AC Mains Neutral Wire                                    |
     +---------------------------------------------------------------------------+
```

---

## 📌 Pin Assignment Table

| Peripheral | ESP32-WROOM Pin | Destination Header / Module | Operating Voltage | Logic Level |
| :--- | :---: | :--- | :---: | :--- |
| **Relay 1 (Light 1)** | `GPIO 23` | Relay Board `IN1` | 5V Coil / 3.3V In | Active LOW (LOW = Relay ON) |
| **Relay 2 (Light 2)** | `GPIO 22` | Relay Board `IN2` | 5V Coil / 3.3V In | Active LOW (LOW = Relay ON) |
| **Relay 3 (Fan)** | `GPIO 21` | Relay Board `IN3` | 5V Coil / 3.3V In | Active LOW (LOW = Relay ON) |
| **Relay 4 (Appliance)** | `GPIO 19` | Relay Board `IN4` | 5V Coil / 3.3V In | Active LOW (LOW = Relay ON) |
| **Wall Switch 1** | `GPIO 32` | Optocoupler Board `OUT1` | 3.3V Logic | Active LOW (Switch ON = LOW) |
| **Wall Switch 2** | `GPIO 33` | Optocoupler Board `OUT2` | 3.3V Logic | Active LOW (Switch ON = LOW) |
| **Wall Switch 3** | `GPIO 25` | Optocoupler Board `OUT3` | 3.3V Logic | Active LOW (Switch ON = LOW) |
| **Wall Switch 4** | `GPIO 26` | Optocoupler Board `OUT4` | 3.3V Logic | Active LOW (Switch ON = LOW) |
| **DHT11 Sensor** | `GPIO 4` | DHT11 Signal Pin | 3.3V (4.7kΩ Pull-up)| 1-Wire Bi-directional |
| **Status LED** | `GPIO 2` | Onboard Blue LED | 3.3V | HIGH = ON, LOW = OFF |
| **BOOT Button** | `GPIO 0` | Onboard Push Button | 3.3V Internal Pull-up| LOW = Pressed |

---

## ⚡ DHT11 Wiring Details
- **`Pin 1 (VCC)`**: Connect to **`+3.3V`** clean logic rail.
- **`Pin 2 (DATA)`**: Connect to ESP32 **`GPIO 4`**. Connect a **4.7kΩ to 10kΩ pull-up resistor** between `DATA` and `+3.3V`.
- **`Pin 3 (NC)`**: No connection / leave floating.
- **`Pin 4 (GND)`**: Connect to common **`GND`**.
