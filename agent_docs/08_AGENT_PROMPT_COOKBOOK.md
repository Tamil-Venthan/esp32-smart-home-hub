# 📖 AI Agent Prompt Cookbook & Extension Recipes

Use these pre-engineered prompt templates when prompting an AI coding agent to extend or modify this project.

---

## 🍳 Recipe 1: Adding an I2C SSD1306 0.96" OLED Display

### Prompt to AI Agent:
```text
I want to add an I2C SSD1306 128x64 OLED display to our ESP32-WROOM smart home hub.
Context & Constraints:
1. Refer to 'agent_docs/02_PINOUT_AND_HARDWARE_MAP.md'. Allocate free I2C pins (GPIO 18 for SDA, GPIO 17 for SCL).
2. Wire the SSD1306 VCC to the clean +3.3V logic rail and GND to common ground.
3. Use the Adafruit_SSD1306 and Adafruit_GFX libraries in platformio.ini.
4. Display real-time status:
   - Top Bar: Wi-Fi RSSI indicator and Cloud/AP status icon.
   - Middle: Live Room Temp (°C) and Humidity (% RH) from DHT11.
   - Bottom: Status of the 4 relays ([L1:ON] [L2:OFF] [FAN:ON] [APP:OFF]).
5. Update display every 1000ms using non-blocking millis() in loop(). Do NOT use delay().
```

---

## 🍳 Recipe 2: Adding a PZEM-004T AC Power & Energy Meter

### Prompt to AI Agent:
```text
I want to integrate a PZEM-004T v3.0 AC energy monitor into our ESP32-WROOM system.
Context & Constraints:
1. Refer to 'agent_docs/02_PINOUT_AND_HARDWARE_MAP.md'. Connect PZEM-004T UART to HardwareSerial(2) using GPIO 16 (RX2) and GPIO 17 (TX2).
2. Read Voltage (V), Current (A), Active Power (W), Frequency (Hz), and Energy (kWh).
3. Add a new device 'Energy Meter' to ESP RainMaker and expose these parameters.
4. Expose the electrical metrics in the local offline WebServer at GET /status.
5. Ensure sensor polling occurs every 2000ms in a non-blocking task so it does not block relay switching.
```

---

## 🍳 Recipe 3: Expanding from 4 Channels to 8 Channels

### Prompt to AI Agent:
```text
We want to expand the relay and wall switch channels from 4 to 8 on ESP32-WROOM.
Context & Constraints:
1. Refer to 'agent_docs/02_PINOUT_AND_HARDWARE_MAP.md' to ensure no strapping pins (0, 2, 12, 15) or flash pins (6-11) are assigned.
2. Update NUM_RELAYS to 8 in config.h.
3. Update NVS Preferences keys to persist r0-r7 and n0-n7.
4. Extend the embedded HTML PWA in main.cpp to render 8 responsive cards instead of 4.
5. Verify current budget from 'agent_docs/04_POWER_SUPPLY_SPECIFICATION.md' — 8 relays draw ~640mA total, well within our 2.0A HLK-10M05 module rating.
```

---

## 🍳 Recipe 4: Adding Home Assistant Local MQTT Alongside RainMaker

### Prompt to AI Agent:
```text
I want the hub to support local Home Assistant MQTT Auto-Discovery simultaneously with ESP RainMaker Cloud.
Context & Constraints:
1. Read 'agent_docs/01_SYSTEM_ARCHITECTURE.md'. Maintain dual connectivity.
2. Include PubSubClient in platformio.ini.
3. Connect to local MQTT broker when in online Wi-Fi mode.
4. Publish Home Assistant MQTT discovery topics: homeassistant/switch/smarthome_ch1/config.
5. Keep state synchronized: a state change via RainMaker, physical switch, or local web must publish updated state to both RainMaker and MQTT.
```
