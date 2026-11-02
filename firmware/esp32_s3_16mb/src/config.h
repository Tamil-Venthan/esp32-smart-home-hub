#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PIN DEFINITIONS (ESP32-S3 DevKitC-1)
// ==========================================

// --- 4-Channel 5V Relay Control Pins ---
#define RELAY1_PIN       10  // Channel 1 (Light 1)
#define RELAY2_PIN       11  // Channel 2 (Light 2)
#define RELAY3_PIN       12  // Channel 3 (Fan)
#define RELAY4_PIN       13  // Channel 4 (Appliance)
#define NUM_RELAYS       4

// Standard 5V relay modules are Active LOW (0 = ON, 1 = OFF)
#define RELAY_ACTIVE_LEVEL   LOW
#define RELAY_INACTIVE_LEVEL HIGH

// --- 4-Channel AC Optocoupler Wall Switch Input Pins ---
#define SWITCH1_PIN      1   // Wall Switch 1 (Syncs with Relay 1)
#define SWITCH2_PIN      2   // Wall Switch 2 (Syncs with Relay 2)
#define SWITCH3_PIN      41  // Wall Switch 3 (Syncs with Relay 3)
#define SWITCH4_PIN      42  // Wall Switch 4 (Syncs with Relay 4)
#define DEBOUNCE_DELAY_MS 80 // Anti-bounce & AC zero-crossing filter delay

// --- Onboard Controls ---
#define RGB_LED_PIN      48  // ESP32-S3 DevKit WS2812 Data Pin
#define BOOT_BTN_PIN     0   // Onboard BOOT Button
#define NUM_LEDS         1

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
#define PROV_SERVICE_NAME    "PROV_SMARTHOME"
#define PROV_POP             "smart123"
#define OFFLINE_AP_SSID      "SmartHome_Offline"
#define OFFLINE_AP_PASS      "12345678"
#define MDNS_HOSTNAME        "smarthome"
#define HTTP_PORT            80

#endif // CONFIG_H
