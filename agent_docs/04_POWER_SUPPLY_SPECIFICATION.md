# ⚡ Power Supply Specification & Safety Architecture

## 🔌 Design Overview

Reliable ESP32 smart home systems fail most frequently due to **inadequate power supply architecture** (brownout resets, relay coil back-EMF, and lightning/grid surges). This custom dual-rail power supply resolves all three issues.

```
                  ┌────────────────────────────────────────────────────────┐
                  │ 220V AC Mains Input (100V - 240V, 50/60Hz)             │
                  └──────────┬─────────────────────────────┬───────────────┘
                             │                             │
                      [ 2A Fuse 5x20mm ]             [ 14D471K MOV ]
                             │                      (Surge Clamping)
                      [ 10D-9 NTC ]                        │
                   (Inrush Current Limiter)                │
                             │                             │
                  ┌──────────┴─────────────────────────────┴───────────────┐
                  │ Hi-Link HLK-10M05 (10W Isolated AC-DC Switching Module)│
                  │ >6.3mm Creepage Distance + 2.0mm PCB Milled Air Slot   │
                  └──────────┬─────────────────────────────┬───────────────┘
                             │                             │
                           +5.0V                          GND
                             │                             │
                   [ 1000µF Low-ESR Cap ]                  │
              (Absorbs Relay Inrush Kickback)              │
                             │                             │
          ┌──────────────────┴──────────────────┐          │
          │                                     │          │
          ▼                                     ▼          │
  [ +5V High-Current Rail ]            [ AMS1117-3.3 LDO ] │
  • Powers 4–10 Relay Coils            • 1.0A Max Output   │
  • Capacity: 2000mA                   • Ripple Rejection  │
                                                │          │
                                                ▼          ▼
                                     [ +3.3V Clean Logic Rail ]
                                     • Powers ESP32 Microcontroller
                                     • Powers DHT11 & I2C Sensors
```

---

## 📐 Electrical Specifications

| Parameter | Specification | Purpose / Safety Margin |
| :--- | :--- | :--- |
| **Input AC Voltage** | `100V – 240V AC (50/60Hz)` | Universal worldwide grid compatibility |
| **Primary Module** | Hi-Link **HLK-10M05** | Isolated SMPS (10 Watts, 5V DC @ 2.0A) |
| **Primary Output Rail** | **`+5.0V DC` @ 2.0A max** | High-current rail dedicated to relay coils |
| **Secondary Output Rail**| **`+3.3V DC` @ 1.0A max** | Regulated via onboard AMS1117-3.3 for CPU & sensors |
| **Mains Fuse** | `2.0A 250V Slow-Blow (5x20mm)` | Overcurrent & short-circuit protection |
| **Surge Suppressor (MOV)**| `14D471K` Varistor | Clamps grid voltage spikes & lightning surges at ~710V |
| **Inrush Suppressor (NTC)**| `10D-9` Thermistor | Suppresses high capacitive charging current at turn-on |
| **Bulk Decoupling** | `1000µF 16V Low-ESR` | Prevents voltage sag when all 4 relay coils energize |
| **Creepage Air Gap** | `2.0mm Physical Slot` | >6.3mm effective clearance (meets IEC 60950-1 isolation) |

---

## ⚡ Current Budget Analysis

| Component | Nominal Current | Peak Inrush Current | Rail |
| :--- | :---: | :---: | :---: |
| **ESP32-WROOM-32 (Wi-Fi TX Burst)** | 160mA | 380mA (5ms burst) | +3.3V |
| **DHT11 Sensor** | 1mA | 2.5mA | +3.3V |
| **PC814 Logic Pull-Ups (4 CH)** | 1.3mA | 1.3mA | +3.3V |
| **4x 5V Relay Coils (Simultaneous)** | 300mA | 420mA | +5.0V |
| **Onboard Power Indicator LEDs** | 10mA | 10mA | +5V / +3.3V |
| **Total Worst-Case Consumption** | **~472mA** | **~813mA** | - |
| **Power Supply Rated Capacity** | **2000mA** | **2500mA** | **> 200% Headroom!** |

> [!NOTE]
> Even with all 4 relays active and the ESP32 transmitting at full +20dBm Wi-Fi power, the system utilizes **less than 45%** of the HLK-10M05 module's rated capacity. This guarantees cool operation and prevents thermal degradation.

---

## 🛠️ PCB Layout & Creepage Safety Rules for KiCad

1. **Creepage Slot:** An unbroken **2.0mm wide routing slot** on the `Edge.Cuts` layer physically separates the high-voltage AC mains pads from the low-voltage DC planes.
2. **AC Trace Width:** High-voltage AC mains copper traces are routed with a minimum width of **2.0mm (80 mil)** with rounded 45° bends to withstand continuous 2A current without localized heating.
3. **Ground Plane Separation:** The ground copper pour (`GND`) is strictly confined to the low-voltage DC section of the PCB. **No copper pour is permitted beneath the AC input terminal, fuse, MOV, or primary side of the HLK-10M05.**
