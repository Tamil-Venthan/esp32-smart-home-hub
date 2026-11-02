# ⚡ Wireless Over-The-Air (OTA) Firmware Update Guide

The ESP32-WROOM hub features an embedded HTTP web OTA server allowing complete firmware upgrades wirelessly over Wi-Fi without needing a USB cable.

---

## 🛠️ Step 1: Exporting Compiled Firmware Binary (`.bin`)

If you make modifications in PlatformIO:
1. Open terminal in `firmware/esp32_wroom_production/`.
2. Compile the project:
   ```bash
   pio run
   ```
3. Locate the compiled production binary:
   👉 `.pio/build/esp32dev/firmware.bin`

---

## 🌐 Step 2: Uploading via Browser

1. Connect your PC or phone to the same Wi-Fi network as the ESP32 (or connect directly to the fallback AP `SmartHome_Offline`).
2. Open your web browser and navigate to:
   👉 **`http://<ESP32-IP>/update`**  *(or `http://192.168.4.1/update` if in AP mode)*
3. You will see the **ESP32 Web OTA Firmware Updater** interface.
4. Click **Choose File** (or "Browse") and select your new `firmware.bin` file.
5. Click **Update Firmware**.

---

## ⏳ Step 3: Flash Progress & Automatic Reboot

* The browser shows an animated progress bar as binary chunks are written to flash.
* Once the progress reaches 100%, the browser displays:  
  `Update Successful! Rebooting ESP32...`
* The ESP32 switches active boot partitions (`ota_0` -> `ota_1` or vice versa) and reboots automatically in less than 3 seconds.
* Your custom appliance names and relay states stored in NVS memory are 100% preserved!
