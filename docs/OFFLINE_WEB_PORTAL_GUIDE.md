# 📶 Offline Web Portal Guide (`192.168.4.1`)

When internet connectivity or mobile hotspots are disconnected for > 10 seconds, the ESP32 automatically activates its **Fallback Offline Web Portal**.

---

## 📱 Connecting to the Offline Portal

1. On your smartphone, tablet, or laptop, open Wi-Fi settings.
2. Connect to the Wi-Fi Access Point:
   - **Network Name (SSID):** `SmartHome_Offline`
   - **Password:** `12345678`
3. If Android or iOS prompts: *"This network has no internet access. Stay connected?"*, select **Yes / Keep Connection**.
4. Open your web browser (Chrome, Safari, Firefox) and navigate to:
   👉 **`http://192.168.4.1/`**  *(or `http://smarthome.local/`)*

> [!TIP]
> If the page does not load, temporarily toggle OFF cellular/mobile data on your phone. Some mobile OS versions automatically route HTTP traffic over 4G/5G when a Wi-Fi connection lacks internet.

---

## 🌟 Dashboard Features & Controls

### 1. Appliance Toggles
* Tap any card (`Light 1`, `Light 2`, `Fan`, `Appliance`) to instantly toggle its state.
* The UI dynamically renders custom glowing SVG icons and changes card background color to show real-time state.

### 2. Inline Channel Renaming
* Tap the pencil icon (**✏️**) next to any channel name.
* Type a custom name (e.g. *"Balcony Lamp"*, *"Geyser"*, *"Bedroom Fan"*) and tap Save.
* The name is saved instantly to ESP32 Flash memory (`Preferences` NVS) and persists through reboots.

### 3. Master Controls
* **Turn All ON:** Energizes all 4 relay channels simultaneously.
* **Turn All OFF:** De-energizes all 4 relay channels simultaneously.

### 4. Live Climate & System Telemetry (ESP32-WROOM)
* **Ambient Room Temperature:** Displayed in °C from DHT11.
* **Ambient Humidity:** Displayed in % RH from DHT11.
* **ESP32 Silicon Temperature:** Internal core temperature monitoring.
* **Wi-Fi Signal Strength:** Live RSSI in -dBm and percentage quality.

---

## 🔄 Automatic Cloud Re-Connection
You do not need to manually disable the offline portal. The ESP32 continuously scans in the background for your home Wi-Fi or hotspot. As soon as your router or hotspot comes back online, the ESP32 terminates the AP and reconnects to RainMaker cloud automatically.
