# 🔌 ESP32-S3 DevKitC-1 Hardware Wiring Guide

## 📋 Pin Interconnection Architecture

```
                             +-----------------------------------+
                             | MB-102 Breadboard / HLK-10M05 PS  |
                             +-----------------------------------+
                                       |                 |
                                     +5.0V              GND
                                       |                 |
                                       |                 +-----------------+
                                       |                                   |
                                       v                                   v
                         +---------------------------+       +---------------------------+
                         | 4-Channel 5V Relay Module |       | ESP32-S3 DevKitC-1 Board  |
                         +---------------------------+       +---------------------------+
                         | VCC  <-- +5V Supply Rail  |       | 3V3  (Outputs 3.3V)       |
                         | GND  <-- Common Ground    |       | GND  <-- Common Ground    |
                         | IN1  <-- HV1              |       | GPIO 10 --> LV1 (Light 1) |
                         | IN2  <-- HV2              |       | GPIO 11 --> LV2 (Light 2) |
                         | IN3  <-- HV3              |       | GPIO 12 --> LV3 (Fan)     |
                         | IN4  <-- HV4              |       | GPIO 13 --> LV4 (Appliance|
                         +---------------------------+       | GPIO 0  --> BOOT Button   |
                                       ^                     | GPIO 48 --> Onboard RGB   |
                                       |                     +---------------------------+
                             +-------------------+                         |
                             | Level Converter   |                         |
                             +-------------------+                         |
                             | HV  <-- +5V Rail  |                         |
                             | GND <-- Ground    |                         |
                             | LV  <-------------+ (3.3V from ESP32)       |
                             | GND <-------------+ (Ground from ESP32)     |
                             +-------------------+
```

---

## 📌 Signal Channel Routing Table

| Appliance | ESP32-S3 GPIO | Shifter LV | Shifter HV | Relay Input Pin | Relay Logic |
| :--- | :---: | :---: | :---: | :---: | :--- |
| 💡 **Light 1** | `GPIO 10` | `LV1` | `HV1` | `IN1` | Active LOW (LOW = Relay ON) |
| 💡 **Light 2** | `GPIO 11` | `LV2` | `HV2` | `IN2` | Active LOW (LOW = Relay ON) |
| 🌀 **Fan** | `GPIO 12` | `LV3` | `HV3` | `IN3` | Active LOW (LOW = Relay ON) |
| 🔌 **Appliance** | `GPIO 13` | `LV4` | `HV4` | `IN4` | Active LOW (LOW = Relay ON) |
| 🔲 **Wall Switch 1**| `GPIO 1` | - | - | Opto `OUT1` | Active LOW |
| 🔲 **Wall Switch 2**| `GPIO 2` | - | - | Opto `OUT2` | Active LOW |
| 🔲 **Wall Switch 3**| `GPIO 41` | - | - | Opto `OUT3` | Active LOW |
| 🔲 **Wall Switch 4**| `GPIO 42` | - | - | Opto `OUT4` | Active LOW |
| 🌈 **Status RGB LED**| `GPIO 48` | - | - | Onboard WS2812 | Green=Cloud, Blue=Offline AP |
| 🔘 **Manual Switch** | `GPIO 0` | - | - | Onboard BOOT | Pull-up LOW (Toggles Light 1) |
