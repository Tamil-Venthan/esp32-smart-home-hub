#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PIN DEFINITIONS (ESP32-WROOM-32 / DevKit V1)
// ==========================================

// --- 4-Channel 5V Relay Control Pins ---
// Safe digital outputs, away from flash bus & strapping pins
#define RELAY1_PIN       23  // Channel 1 (Light 1)
#define RELAY2_PIN       22  // Channel 2 (Light 2)
#define RELAY3_PIN       21  // Channel 3 (Fan)
#define RELAY4_PIN       19  // Channel 4 (Appliance)
#define NUM_RELAYS       4

// Standard 5V relay modules are Active LOW (0 = ON, 1 = OFF)
#define RELAY_ACTIVE_LEVEL   LOW
#define RELAY_INACTIVE_LEVEL HIGH

// --- 4-Channel AC Optocoupler Wall Switch Input Pins ---
// Safe pins with internal pull-ups, zero impact on boot sequence
#define SWITCH1_PIN      32  // Wall Switch 1 (Syncs with Relay 1)
#define SWITCH2_PIN      33  // Wall Switch 2 (Syncs with Relay 2)
#define SWITCH3_PIN      25  // Wall Switch 3 (Syncs with Relay 3)
#define SWITCH4_PIN      26  // Wall Switch 4 (Syncs with Relay 4)
#define DEBOUNCE_DELAY_MS 120 // Anti-bounce & AC zero-crossing filter delay

// --- DHT11 Temperature & Humidity Sensor ---
#define DHT_PIN          4   // 1-Wire Single Bus Data Pin
#define DHT_TYPE         DHT11
#define DHT_READ_INTERVAL 3000 // Milliseconds between reads

// --- Onboard Controls ---
#define STATUS_LED_PIN   2   // Onboard Blue LED on ESP32-WROOM
#define BOOT_BTN_PIN     0   // Onboard BOOT Button

// ==========================================
// DEFAULT CHANNEL LABELS
// ==========================================
#define DEFAULT_NAME_RELAY1  "Light 1"
#define DEFAULT_NAME_RELAY2  "Light 2"
#define DEFAULT_NAME_RELAY3  "Fan"
#define DEFAULT_NAME_RELAY4  "Appliance"

// ==========================================
// NETWORK & PROVISIONING SETTINGS
// ==========================================
#define PROV_SERVICE_NAME    "PROV_0F5B70"
#define PROV_POP             "smart123"
#define OFFLINE_AP_SSID      "SmartHome_Offline"
#define OFFLINE_AP_PASS      "12345678"
#define MDNS_HOSTNAME        "smarthome"
#define HTTP_PORT            80

// Optional Direct Home Wi-Fi Credentials (bypasses app provisioning)
#define WIFI_SSID            ""
#define WIFI_PASS            ""

#endif // CONFIG_H
