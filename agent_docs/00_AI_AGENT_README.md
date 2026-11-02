# 🤖 AI Agent Master Entrypoint & Instruction Manual
> **Project:** ESP32 Hybrid Smart Home Automation Hub (Cloud + Offline Fallback)  
> **Target Audience:** Future AI Agents (Antigravity, Claude, ChatGPT, Cursor, Copilot) & Human Engineers  
> **Repository Purpose:** Production IoT Firmware, Custom PCB Hardware, and Cloud Architecture

---

## 🎯 Purpose of This Document
If you are an AI agent analyzing or extending this codebase, **START HERE**.  
This file defines the operating principles, safety constraints, hardware mappings, and architectural contracts governing this repository.

---

## 📂 Repository Layout for AI Agents

```
ESP32_Home_Automation_Repository/
├── agent_docs/                        <-- AI Agent Knowledge Base & Blueprints
│   ├── 00_AI_AGENT_README.md          <-- [You Are Here] Master AI guide
│   ├── 01_SYSTEM_ARCHITECTURE.md      <-- Dual-mode Cloud + Offline architecture
│   ├── 02_PINOUT_AND_HARDWARE_MAP.md  <-- Pin allocations, strapping pins & constraints
│   ├── 03_AC_OPTOCOUPLER_LOGIC.md     <-- 230V AC optocoupler zero-crossing & 2-way sync
│   ├── 04_POWER_SUPPLY_SPECIFICATION.md <-- HLK-10M05 dual 5V/3.3V isolated power board
│   ├── 05_NVS_AND_PARAM_DICTIONARY.md <-- NVS flash keys & RainMaker parameter schema
│   ├── 06_FIRMWARE_STATE_MACHINE.md   <-- State transitions (Boot, WiFi, AP, Cloud, OTA)
│   ├── 07_TROUBLESHOOTING_MATRIX.md   <-- Known issues, root causes, and verified fixes
│   └── 08_AGENT_PROMPT_COOKBOOK.md    <-- Tested task prompt recipes for future extensions
│
├── firmware/
│   ├── esp32_wroom_production/        <-- ESP32-WROOM-32 (4MB Flash, DHT11, Optocoupler, OTA)
│   │   ├── src/config.h               <-- Master pin definitions & timings
│   │   ├── src/main.cpp               <-- Unified firmware logic (~44KB)
│   │   ├── platformio.ini             <-- PlatformIO build flags & dependencies
│   │   ├── partitions_wroom_4MB.csv   <-- Dual-OTA 4MB partition table
│   │   ├── bin/                       <-- Ready-to-flash factory binaries
│   │   └── flash_wroom.bat            <-- 1-Click production flasher
│   │
│   └── esp32_s3_16mb/                 <-- ESP32-S3 DevKitC-1 (16MB Flash, WS2812 RGB LED)
│       ├── src/config.h               <-- S3 pin definitions
│       ├── src/main.cpp               <-- Unified firmware logic
│       ├── platformio.ini             <-- S3 PlatformIO configuration
│       ├── partitions_rainmaker_16MB.csv <-- 16MB partition map
│       ├── bootloader_dio.bin         <-- DIO mode bootloader
│       └── bin/                       <-- Precompiled S3 binaries & restore tool
│
├── hardware/
│   ├── 5v_power_supply_module/        <-- 220V AC to 5V/3.3V Isolated Power Supply KiCad Project
│   │   ├── 5V_Power_Supply.kicad_pcb  <-- KiCad PCB Layout (Creepage slots, wide AC tracks)
│   │   ├── 5V_Power_Supply.kicad_sch  <-- KiCad Schematic
│   │   ├── 5V_Power_Supply_Schematic.pdf
│   │   ├── BOM.csv                    <-- Sourced bill of materials (LCSC / Mouser)
│   │   └── production/                <-- Fabrication zip (Gerber, Drill, CPL, BOM)
│   │
│   ├── optocoupler_4ch_module/        <-- 4-Channel 230V AC PC814 Wall Switch Sensor Board
│   │   ├── Optocoupler_4CH.kicad_pcb  <-- KiCad PCB Layout
│   │   ├── Optocoupler_4CH.kicad_sch  <-- KiCad Schematic
│   │   ├── production/                <-- Gerbers, JLCPCB CPL & BOM
│   │   └── renders/                   <-- Visual PCB renders
│   │
│   └── wiring_and_pinouts/            <-- Interconnection diagrams & mains safety guides
│
├── docs/                              <-- User manual, RainMaker cloud guide, Web UI manual
└── tools/                             <-- Automated batch flashing utilities
```

---

## ⚡ Golden Rules for AI Agents Modifying This Project

### Rule 1: NEVER Bypass Relay Power Isolation
- **Coil Inrush:** Standard 5V relay coils draw **70mA to 80mA each**. Turning on 4 relays requires **~320mA**.
- **Rule:** Never suggest powering relays from the ESP32's onboard 3.3V regulator or micro-USB 5V line. Relays **must** be powered from the dedicated high-current 5V rail of the HLK-10M05 module or external MB-102 power supply.
- **Relay Logic:** 4-channel relay boards are **Active LOW** (`0 = RELAY_ON`, `1 = RELAY_OFF`).

### Rule 2: Observe ESP32 Strapping Pin Constraints
- **ESP32-WROOM:**
  - `GPIO 0`: Strapping pin (Bootloader vs SPI boot). Keep pulled HIGH at boot.
  - `GPIO 2`: Strapping pin. Onboard Blue LED. Must not be pulled HIGH strongly during flash download.
  - `GPIO 12`: Strapping pin (MTDI - sets flash voltage). Do not connect to relays or switches that might pull it HIGH at boot!
  - `GPIO 15`: Strapping pin (MTDO - debug logs).
  - `GPIO 6-11`: Internally connected to SPI Flash IC. **NEVER USE FOR GPIO**.
  - `GPIO 34, 35, 36, 39`: Input-only pins, NO internal pull-ups.

### Rule 3: 230V AC Wall Switch Anti-Bounce Filtering
- Mains frequency is 50Hz (20ms period, zero crossing every 10ms).
- When a physical wall switch is ON, the optocoupler emits a 100Hz pulse train if unbuffered.
- The hardware uses a **10µF smoothing capacitor** across the phototransistor collector.
- The firmware enforces a software debounce window of **80ms to 120ms** before acknowledging a toggle. Do not reduce debounce below 60ms or false double-triggering will occur on AC zero crossings.

### Rule 4: Zero Blocking in Event Loop
- ESP RainMaker, local HTTP WebServer, DNS captive portal, and DHT11 polling share the main loop.
- **Rule:** Never use `delay()` in `loop()`! All timing must rely on non-blocking `millis()` delta checks. Blocking will cause RainMaker MQTT disconnects or watch-dog timer (WDT) panics.

### Rule 5: Keep Flash Partitions Synchronized
- If editing partition maps, ensure dual OTA partitions (`ota_0` and `ota_1`) have identical sizes and match the PlatformIO partition definition.
- Precompiled binaries in `bin/` must be re-exported if `main.cpp` or partition tables are updated.

---

## 🧭 Fast AI Navigation Index
- Need to understand state synchronization and NVS? 👉 [05_NVS_AND_PARAM_DICTIONARY.md](file:///D:/ESP32%20Home%20Automation/ESP32_Home_Automation_Repository/agent_docs/05_NVS_AND_PARAM_DICTIONARY.md)
- Need to check which GPIOs are free for new sensors? 👉 [02_PINOUT_AND_HARDWARE_MAP.md](file:///D:/ESP32%20Home%20Automation/ESP32_Home_Automation_Repository/agent_docs/02_PINOUT_AND_HARDWARE_MAP.md)
- Need to diagnose why the device is bootlooping? 👉 [07_TROUBLESHOOTING_MATRIX.md](file:///D:/ESP32%20Home%20Automation/ESP32_Home_Automation_Repository/agent_docs/07_TROUBLESHOOTING_MATRIX.md)
- Need prompt templates to write new features? 👉 [08_AGENT_PROMPT_COOKBOOK.md](file:///D:/ESP32%20Home%20Automation/ESP32_Home_Automation_Repository/agent_docs/08_AGENT_PROMPT_COOKBOOK.md)
