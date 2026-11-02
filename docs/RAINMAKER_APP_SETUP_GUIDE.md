# 📱 ESP RainMaker Cloud & Voice Assistant Setup Guide

This guide details pairing your smartphone with the ESP32 Hub and configuring Alexa & Google Assistant voice controls.

---

## 📲 Step 1: Install the ESP RainMaker App
Download the official **ESP RainMaker** application:
* **Android:** [Google Play Store](https://play.google.com/store/apps/details?id=com.espressif.rainmaker)
* **iOS (Apple):** [Apple App Store](https://apps.apple.com/app/esp-rainmaker/id1497491540)

Open the app and create a free account (supports sign in with Google or Apple ID).

---

## 📷 Step 2: Add Device & Wi-Fi Provisioning
1. Power on your ESP32 Hub.
2. In the RainMaker app, tap **Add Device** (`+` icon).
3. The app will search for nearby Bluetooth devices (`PROV_SMARTHOME` or `PROV_0F5B70`).
4. When prompted for Proof of Possession (PoP), enter: **`smart123`**
5. Select your home **2.4 GHz Wi-Fi network** and enter the Wi-Fi password.
6. The app provisions credentials over Bluetooth. Once connected, your 4 appliances and climate monitor appear instantly in the app dashboard!

---

## 🗣️ Step 3: Google Home Voice Assistant Integration
1. Open the **Google Home** app on your phone.
2. Tap **Devices** -> **+ Add** -> **Works with Google**.
3. Search for **ESP RainMaker** in the search bar.
4. Sign in with your RainMaker credentials to authorize Google.
5. All devices (`Light 1`, `Light 2`, `Fan`, `Appliance`) will be imported into Google Home.
6. Assign them to rooms (e.g., "Living Room", "Bedroom").

### Voice Commands:
* *"Hey Google, turn on Light 1."*
* *"Hey Google, turn off the Fan."*
* *"Hey Google, is Light 2 on?"*

---

## 🗣️ Step 4: Amazon Alexa Voice Integration
1. Open the **Amazon Alexa** app.
2. Go to **More** -> **Skills & Games**.
3. Search for **ESP RainMaker** and tap **Enable to Use**.
4. Log in with your RainMaker credentials.
5. Tap **Discover Devices**.

### Voice Commands:
* *"Alexa, turn on Light 2."*
* *"Alexa, turn off everything in Living Room."*

---

## ⏰ Step 5: Schedules & Scenes
* **On-Device RTC Schedules:** In the RainMaker app, tap any device and select **Schedules**. Set daily ON/OFF triggers. These timers run locally on the ESP32 internal clock and trigger even if internet goes down!
* **Scenes:** Create combined scenes (e.g., *"Goodnight"* = Turn off Light 1, Light 2, and Appliance, leave Fan ON).
