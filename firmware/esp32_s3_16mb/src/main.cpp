#include <Arduino.h>
#include "config.h"
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Relay Active LOW Logic
#define RELAY_ON      LOW
#define RELAY_OFF     HIGH

// RainMaker Provisioning Details
const char *service_name = PROV_SERVICE_NAME;
const char *pop          = PROV_POP;

// Offline Fallback AP Details
const char *offline_ap_ssid = OFFLINE_AP_SSID;
const char *offline_ap_pass = OFFLINE_AP_PASS;

// NeoPixel Driver
Adafruit_NeoPixel pixel(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// Non-Volatile State Storage
Preferences prefs;

// Local Web Server on Port 80
WebServer server(HTTP_PORT);

// RainMaker Devices
static LightBulb          *light1      = NULL;
static LightBulb          *light2      = NULL;
static Device             *fan         = NULL;
static Switch             *appl        = NULL;
static TemperatureSensor  *temp_sensor = NULL;

// Persisted States (Initialized from NVS)
bool state_light1 = false;
bool state_light2 = false;
bool state_fan    = false;
bool state_appl   = false;

// 4-Channel AC Optocoupler Wall Switch Tracking
const uint8_t SWITCH_PINS[4]             = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN};
int           last_switch_reading[4]     = {HIGH, HIGH, HIGH, HIGH};
int           stable_switch_state[4]     = {HIGH, HIGH, HIGH, HIGH};
unsigned long last_switch_debounce_time[4] = {0, 0, 0, 0};

// Telemetry Timing
unsigned long last_telemetry_time = 0;

// Network Watchdog Flags
bool is_sta_connected       = false;
bool is_offline_ap_active   = false;
unsigned long disconnect_start_time = 0;
unsigned long last_reconnect_attempt = 0;

void setLED(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

// Forward Declarations
void setRelayState(int channel, bool newState, const char* source);
void toggleRelay(int channel, const char* source);
void handlePhysicalSwitches();

// Synchronize all physical states to RainMaker Cloud
void syncStatesToCloud() {
    if (light1) light1->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_light1);
    if (light2) light2->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_light2);
    if (fan)    fan->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_fan);
    if (appl)   appl->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_appl);
    if (temp_sensor) {
        temp_sensor->updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, temperatureRead());
        temp_sensor->updateAndReportParam("WiFi Signal (dBm)", is_sta_connected ? WiFi.RSSI() : 0);
    }
    Serial.printf("[STATE SYNC] L1:%d | L2:%d | Fan:%d | Appl:%d reported to Cloud.\n",
                  state_light1, state_light2, state_fan, state_appl);
}

// Embedded Minimal Responsive HTML Dashboard with PWA Support
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>Smart Home // Minimal Control</title>

  <!-- PWA & Mobile Fullscreen Optimization -->
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="apple-mobile-web-app-title" content="Smart Home">
  <meta name="theme-color" content="#090a0f">
  <meta name="application-name" content="Smart Home">

  <!-- Minimal Embedded App Icon (100% Offline Data URI) -->
  <link rel="icon" type="image/svg+xml" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'%3E%3Crect width='512' height='512' rx='128' fill='%23090a0f'/%3E%3Ccircle cx='256' cy='256' r='180' fill='none' stroke='rgba(16,185,129,0.15)' stroke-width='8'/%3E%3Cpath d='M256 120 L380 230 L350 230 L350 370 L280 370 L280 290 L232 290 L232 370 L162 370 L162 230 L132 230 Z' fill='none' stroke='%2310b981' stroke-width='24' stroke-linejoin='round'/%3E%3Ccircle cx='256' cy='200' r='16' fill='%2338bdf8'/%3E%3Cpath d='M256 165 v-35 M225 185 l-25 -20 M287 185 l25 -20' stroke='%2338bdf8' stroke-width='14' stroke-linecap='round'/%3E%3C/svg%3E">
  <link rel="apple-touch-icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'%3E%3Crect width='512' height='512' rx='128' fill='%23090a0f'/%3E%3Ccircle cx='256' cy='256' r='180' fill='none' stroke='rgba(16,185,129,0.15)' stroke-width='8'/%3E%3Cpath d='M256 120 L380 230 L350 230 L350 370 L280 370 L280 290 L232 290 L232 370 L162 370 L162 230 L132 230 Z' fill='none' stroke='%2310b981' stroke-width='24' stroke-linejoin='round'/%3E%3Ccircle cx='256' cy='200' r='16' fill='%2338bdf8'/%3E%3Cpath d='M256 165 v-35 M225 185 l-25 -20 M287 185 l25 -20' stroke='%2338bdf8' stroke-width='14' stroke-linecap='round'/%3E%3C/svg%3E">
  <link rel="manifest" href="/manifest.json">

  <style>
    :root {
      --bg: #090a0f;
      --surface: rgba(255, 255, 255, 0.035);
      --surface-hover: rgba(255, 255, 255, 0.06);
      --surface-active: rgba(16, 185, 129, 0.08);
      --surface-active-border: rgba(16, 185, 129, 0.35);
      --border: rgba(255, 255, 255, 0.07);
      --text: #f8fafc;
      --text-secondary: #94a3b8;
      --text-muted: #64748b;
      --emerald: #10b981;
      --emerald-glow: rgba(16, 185, 129, 0.25);
      --cyan: #38bdf8;
      --radius-lg: 20px;
      --radius-md: 14px;
      --radius-pill: 999px;
      --font: -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", Roboto, sans-serif;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
    }

    body {
      background-color: var(--bg);
      color: var(--text);
      font-family: var(--font);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      /* Full-Screen Safe-Area padding for Notched Phones & Dynamic Island */
      padding-top: calc(env(safe-area-inset-top, 0px) + 1.75rem);
      padding-bottom: calc(env(safe-area-inset-bottom, 0px) + 2.5rem);
      padding-left: calc(env(safe-area-inset-left, 0px) + 1.25rem);
      padding-right: calc(env(safe-area-inset-right, 0px) + 1.25rem);
      position: relative;
      overflow-x: hidden;
      -webkit-font-smoothing: antialiased;
    }

    /* Ambient atmospheric background lighting */
    body::before {
      content: "";
      position: fixed;
      top: -150px;
      left: 50%;
      transform: translateX(-50%);
      width: 500px;
      height: 350px;
      background: radial-gradient(ellipse, rgba(56, 189, 248, 0.06), transparent 70%);
      pointer-events: none;
      z-index: 0;
    }

    .app-shell {
      width: 100%;
      max-width: 460px;
      display: flex;
      flex-direction: column;
      gap: 1.5rem;
      position: relative;
      z-index: 1;
    }

    /* Minimal Floating Header */
    header {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      padding: 0 0.25rem;
    }

    .brand-title {
      display: flex;
      flex-direction: column;
      gap: 0.2rem;
    }

    .brand-title h1 {
      font-size: 1.65rem;
      font-weight: 700;
      letter-spacing: -0.03em;
      color: var(--text);
    }

    .brand-subtitle {
      font-size: 0.8rem;
      color: var(--text-muted);
      font-weight: 500;
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .live-dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--emerald);
      box-shadow: 0 0 8px var(--emerald);
      animation: pulse-dot 2.5s infinite ease-in-out;
    }

    @keyframes pulse-dot {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.4; transform: scale(0.85); }
    }

    /* Header Action Pill Cluster */
    .header-actions {
      display: flex;
      flex-direction: column;
      align-items: flex-end;
      gap: 0.5rem;
    }

    /* Minimal Telemetry Pill Group */
    .telemetry-pills {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-pill);
      padding: 0.35rem 0.75rem;
      backdrop-filter: blur(16px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.25);
    }

    .telemetry-item {
      display: flex;
      align-items: center;
      gap: 5px;
      font-size: 0.75rem;
      font-weight: 600;
      color: var(--text-secondary);
    }

    .telemetry-item svg {
      width: 13px;
      height: 13px;
      stroke: currentColor;
      fill: none;
      stroke-width: 2;
    }

    .telemetry-divider {
      width: 1px;
      height: 10px;
      background: var(--border);
    }

    /* Sleek "Add to Home Screen" Pill Button */
    .install-pill {
      background: rgba(56, 189, 248, 0.1);
      border: 1px solid rgba(56, 189, 248, 0.35);
      border-radius: var(--radius-pill);
      padding: 0.35rem 0.75rem;
      color: var(--cyan);
      font-family: var(--font);
      font-size: 0.72rem;
      font-weight: 600;
      display: inline-flex;
      align-items: center;
      gap: 5px;
      cursor: pointer;
      user-select: none;
      transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
      box-shadow: 0 4px 14px rgba(56, 189, 248, 0.15);
    }

    .install-pill:hover {
      background: rgba(56, 189, 248, 0.18);
      border-color: var(--cyan);
      transform: translateY(-1px);
    }

    .install-pill:active {
      transform: scale(0.96);
    }

    .install-pill svg {
      width: 13px;
      height: 13px;
      stroke: currentColor;
      fill: none;
      stroke-width: 2.2;
      stroke-linecap: round;
      stroke-linejoin: round;
    }

    /* Cards Grid */
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 1rem;
    }

    @media (max-width: 360px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }

    /* Minimalist Interactive Appliance Card */
    .device-card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-lg);
      padding: 1.35rem 1.15rem;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 160px;
      cursor: pointer;
      user-select: none;
      position: relative;
      backdrop-filter: blur(20px);
      transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.2);
    }

    .device-card:hover {
      background: var(--surface-hover);
      border-color: rgba(255, 255, 255, 0.12);
      transform: translateY(-2px);
    }

    .device-card:active {
      transform: scale(0.975);
    }

    /* Active Glowing State */
    .device-card.active {
      background: radial-gradient(ellipse at 15% 15%, rgba(16, 185, 129, 0.16), var(--surface) 80%);
      border-color: var(--surface-active-border);
      box-shadow: 0 12px 32px -4px var(--emerald-glow), 0 4px 16px rgba(0, 0, 0, 0.4);
    }

    .card-top {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
    }

    /* Minimal Icon Badge */
    .icon-badge {
      width: 44px;
      height: 44px;
      border-radius: var(--radius-md);
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.06);
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--text-muted);
      transition: all 0.3s ease;
    }

    .device-card.active .icon-badge {
      background: rgba(16, 185, 129, 0.15);
      border-color: rgba(16, 185, 129, 0.3);
      color: var(--emerald);
      box-shadow: 0 0 16px rgba(16, 185, 129, 0.3);
    }

    .icon-badge svg {
      width: 22px;
      height: 22px;
      fill: none;
      stroke: currentColor;
      stroke-width: 1.8;
      stroke-linecap: round;
      stroke-linejoin: round;
    }

    /* Minimal Toggle Switch */
    .pill-switch {
      width: 38px;
      height: 22px;
      border-radius: var(--radius-pill);
      background: rgba(255, 255, 255, 0.08);
      position: relative;
      transition: background 0.25s ease;
    }

    .pill-thumb {
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background: #94a3b8;
      position: absolute;
      top: 3px;
      left: 3px;
      transition: transform 0.25s cubic-bezier(0.16, 1, 0.3, 1), background 0.25s;
    }

    .device-card.active .pill-switch {
      background: var(--emerald);
    }

    .device-card.active .pill-thumb {
      transform: translateX(16px);
      background: #ffffff;
      box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
    }

    /* Card Details */
    .card-meta {
      display: flex;
      flex-direction: column;
      gap: 0.15rem;
      margin-top: 1rem;
    }

    .device-name {
      font-size: 1.05rem;
      font-weight: 600;
      color: var(--text);
      letter-spacing: -0.01em;
    }

    .device-status {
      font-size: 0.75rem;
      font-weight: 500;
      color: var(--text-muted);
      transition: color 0.2s;
    }

    .device-card.active .device-status {
      color: var(--emerald);
      font-weight: 600;
    }

    /* Minimal Quick Controls */
    .quick-bar {
      display: flex;
      gap: 0.75rem;
      margin-top: 0.25rem;
    }

    .quick-btn {
      flex: 1;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      padding: 0.85rem;
      color: var(--text-secondary);
      font-family: var(--font);
      font-size: 0.82rem;
      font-weight: 600;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 0.45rem;
      cursor: pointer;
      user-select: none;
      backdrop-filter: blur(16px);
      transition: all 0.2s ease;
    }

    .quick-btn:hover {
      background: var(--surface-hover);
      color: var(--text);
      border-color: rgba(255, 255, 255, 0.15);
    }

    .quick-btn:active {
      transform: scale(0.97);
    }

    .quick-btn.off:hover {
      border-color: rgba(239, 68, 68, 0.4);
      color: #f87171;
    }

    .quick-btn.on:hover {
      border-color: rgba(16, 185, 129, 0.4);
      color: #34d399;
    }

    .quick-btn svg {
      width: 15px;
      height: 15px;
      stroke: currentColor;
      fill: none;
      stroke-width: 2;
    }

    /* Minimal Footer */
    footer {
      text-align: center;
      margin-top: 0.75rem;
      font-size: 0.72rem;
      color: var(--text-muted);
      letter-spacing: 0.02em;
    }

    /* iOS Add-To-Home-Screen Modal Sheet */
    .ios-modal-backdrop {
      position: fixed;
      inset: 0;
      background: rgba(0, 0, 0, 0.65);
      backdrop-filter: blur(8px);
      z-index: 100;
      display: none;
      align-items: flex-end;
      justify-content: center;
      animation: fadeIn 0.25s ease-out;
    }

    .ios-modal-backdrop.open {
      display: flex;
    }

    .ios-modal {
      width: 100%;
      max-width: 440px;
      background: #131722;
      border: 1px solid rgba(255, 255, 255, 0.12);
      border-top-left-radius: 24px;
      border-top-right-radius: 24px;
      padding: 1.5rem 1.25rem calc(env(safe-area-inset-bottom, 0px) + 1.5rem);
      display: flex;
      flex-direction: column;
      gap: 1.25rem;
      box-shadow: 0 -8px 40px rgba(0, 0, 0, 0.6);
      animation: slideUp 0.3s cubic-bezier(0.16, 1, 0.3, 1);
    }

    @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
    @keyframes slideUp { from { transform: translateY(100%); } to { transform: translateY(0); } }

    .modal-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .modal-header h3 {
      font-size: 1.1rem;
      font-weight: 700;
      color: var(--text);
    }

    .close-btn {
      background: rgba(255, 255, 255, 0.08);
      border: none;
      width: 28px;
      height: 28px;
      border-radius: 50%;
      color: var(--text-secondary);
      font-size: 1.1rem;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
    }

    .step-list {
      display: flex;
      flex-direction: column;
      gap: 0.85rem;
    }

    .step-item {
      display: flex;
      align-items: center;
      gap: 0.85rem;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      padding: 0.75rem 1rem;
      font-size: 0.85rem;
      color: var(--text-secondary);
    }

    .step-num {
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: var(--cyan);
      color: #090a0f;
      font-size: 0.75rem;
      font-weight: 800;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-shrink: 0;
    }

    .step-item svg {
      width: 18px;
      height: 18px;
      stroke: var(--cyan);
      fill: none;
      stroke-width: 2;
      flex-shrink: 0;
    }
  </style>
</head>
<body>

  <div class="app-shell">
    <!-- Clean Minimal Header -->
    <header>
      <div class="brand-title">
        <h1>Living Space</h1>
        <div class="brand-subtitle">
          <span class="live-dot"></span>
          <span id="network-mode">Offline Portal • ESP32</span>
        </div>
      </div>

      <div class="header-actions">
        <!-- Compact Floating Telemetry Pills -->
        <div class="telemetry-pills">
          <div class="telemetry-item" title="Internal SoC Temperature">
            <!-- Temp Thermometer Icon -->
            <svg viewBox="0 0 24 24"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>
            <span id="chip-temp">--.-°C</span>
          </div>
          <div class="telemetry-divider"></div>
          <div class="telemetry-item" title="Wi-Fi Signal Strength">
            <!-- Wi-Fi Waves Icon -->
            <svg viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0M1.42 9a16 16 0 0 1 21.16 0M8.53 16.11a6 6 0 0 1 6.95 0M12 20h.01"/></svg>
            <span id="wifi-rssi">-- dBm</span>
          </div>
        </div>

        <!-- Sleek Install App / Fullscreen Pill -->
        <button id="install-btn" class="install-pill" onclick="triggerInstall()" style="display: none;">
          <svg viewBox="0 0 24 24"><path d="M12 3v13M7 11l5 5 5-5M4 20h16"/></svg>
          <span>Install App</span>
        </button>
      </div>
    </header>

    <!-- 4 Appliance Control Cards -->
    <main class="grid">
      <!-- Device 1: Light 1 -->
      <div class="device-card" id="card-1" onclick="toggleRelay(1)">
        <div class="card-top">
          <div class="icon-badge">
            <!-- Lightbulb Icon -->
            <svg viewBox="0 0 24 24"><path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-7 7c0 2.5 1.5 4.5 3 5.5v1.5a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V14.5c1.5-1 3-3 3-5.5a7 7 0 0 0-7-7z"/></svg>
          </div>
          <div class="pill-switch">
            <div class="pill-thumb"></div>
          </div>
        </div>
        <div class="card-meta">
          <span class="device-name">Light 1</span>
          <span class="device-status" id="status-1">Off</span>
        </div>
      </div>

      <!-- Device 2: Light 2 -->
      <div class="device-card" id="card-2" onclick="toggleRelay(2)">
        <div class="card-top">
          <div class="icon-badge">
            <!-- Pendant Lamp Icon -->
            <svg viewBox="0 0 24 24"><path d="M12 2v6M6 14h12l-2-6H8l-2 6zM10 18h4M9 22h6"/></svg>
          </div>
          <div class="pill-switch">
            <div class="pill-thumb"></div>
          </div>
        </div>
        <div class="card-meta">
          <span class="device-name">Light 2</span>
          <span class="device-status" id="status-2">Off</span>
        </div>
      </div>

      <!-- Device 3: Fan -->
      <div class="device-card" id="card-3" onclick="toggleRelay(3)">
        <div class="card-top">
          <div class="icon-badge">
            <!-- Airflow Fan Icon -->
            <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M12 9c1.5-3 5-3 5-1s-3 3-5 4M15 12c3 1.5 3 5 1 5s-3-3-4-5M12 15c-1.5 3-5 3-5 1s3-3 5-4M9 12c-3-1.5-3-5-1-5s3 3 4 5"/></svg>
          </div>
          <div class="pill-switch">
            <div class="pill-thumb"></div>
          </div>
        </div>
        <div class="card-meta">
          <span class="device-name">Ceiling Fan</span>
          <span class="device-status" id="status-3">Off</span>
        </div>
      </div>

      <!-- Device 4: Appliance -->
      <div class="device-card" id="card-4" onclick="toggleRelay(4)">
        <div class="card-top">
          <div class="icon-badge">
            <!-- Power Socket Icon -->
            <svg viewBox="0 0 24 24"><rect x="3" y="3" width="18" height="18" rx="4"/><path d="M9 10v4M15 10v4"/></svg>
          </div>
          <div class="pill-switch">
            <div class="pill-thumb"></div>
          </div>
        </div>
        <div class="card-meta">
          <span class="device-name">Appliance</span>
          <span class="device-status" id="status-4">Off</span>
        </div>
      </div>
    </main>

    <!-- Minimalist Master Quick Bar -->
    <div class="quick-bar">
      <button class="quick-btn off" onclick="masterControl(false)">
        <svg viewBox="0 0 24 24"><path d="M18.36 6.64a9 9 0 1 1-12.73 0M12 2v10"/></svg>
        Turn All Off
      </button>
      <button class="quick-btn on" onclick="masterControl(true)">
        <svg viewBox="0 0 24 24"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>
        Turn All On
      </button>
    </div>

    <!-- Minimalist Clean Footer -->
    <footer>
      <span>192.168.4.1 • Fullscreen Smart Home Terminal</span>
    </footer>
  </div>

  <!-- iOS Add to Home Screen Instructions Modal -->
  <div id="ios-modal-backdrop" class="ios-modal-backdrop" onclick="closeIosModal(event)">
    <div class="ios-modal" onclick="event.stopPropagation()">
      <div class="modal-header">
        <h3>Install on Home Screen</h3>
        <button class="close-btn" onclick="closeIosModal()">&times;</button>
      </div>
      <div class="step-list">
        <div class="step-item">
          <span class="step-num">1</span>
          <span>Tap the <strong>Share</strong> button on your Safari bottom bar</span>
          <svg viewBox="0 0 24 24"><path d="M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8M16 6l-4-4-4 4M12 2v13"/></svg>
        </div>
        <div class="step-item">
          <span class="step-num">2</span>
          <span>Scroll down and tap <strong>Add to Home Screen</strong></span>
          <svg viewBox="0 0 24 24"><rect x="3" y="3" width="18" height="18" rx="4"/><path d="M12 8v8M8 12h8"/></svg>
        </div>
        <div class="step-item">
          <span class="step-num">3</span>
          <span>Launch from your home screen for pure <strong>full-screen</strong> control!</span>
        </div>
      </div>
    </div>
  </div>

  <script>
    const state = {
      r1: false,
      r2: false,
      r3: false,
      r4: false,
      temp: 39.4,
      rssi: -58,
      isSimulated: false
    };

    let deferredPrompt = null;
    const isStandalone = window.matchMedia('(display-mode: standalone)').matches || window.navigator.standalone === true;
    const isIOS = /iPhone|iPad|iPod/.test(navigator.userAgent) && !window.MSStream;

    // Check PWA / Add-To-Home-Screen Support
    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();
      deferredPrompt = e;
      if (!isStandalone) {
        document.getElementById('install-btn').style.display = 'inline-flex';
      }
    });

    // Check iOS Safari when not standalone
    if (isIOS && !isStandalone) {
      document.getElementById('install-btn').style.display = 'inline-flex';
    }

    function triggerInstall() {
      haptic();
      if (deferredPrompt) {
        // Android / Desktop Chrome PWA prompt
        deferredPrompt.prompt();
        deferredPrompt.userChoice.then(() => {
          deferredPrompt = null;
          document.getElementById('install-btn').style.display = 'none';
        });
      } else if (isIOS) {
        // iOS Safari instruction sheet
        document.getElementById('ios-modal-backdrop').classList.add('open');
      } else {
        alert('To install in full screen: Open browser settings menu and tap "Add to Home Screen"');
      }
    }

    function closeIosModal() {
      document.getElementById('ios-modal-backdrop').classList.remove('open');
    }

    function haptic() {
      if ('vibrate' in navigator) {
        try { navigator.vibrate(15); } catch (e) {}
      }
    }

    function render(data) {
      // 1. Relays
      for (let i = 1; i <= 4; i++) {
        const active = data['r' + i] === true;
        const card = document.getElementById('card-' + i);
        const label = document.getElementById('status-' + i);
        if (card && label) {
          if (active) {
            card.classList.add('active');
            label.textContent = 'On';
          } else {
            card.classList.remove('active');
            label.textContent = 'Off';
          }
        }
      }

      // 2. Chip Temp
      if (typeof data.temp !== 'undefined') {
        document.getElementById('chip-temp').textContent = parseFloat(data.temp).toFixed(1) + '°C';
      }

      // 3. Wi-Fi Signal
      if (typeof data.rssi !== 'undefined') {
        const rssi = parseInt(data.rssi);
        document.getElementById('wifi-rssi').textContent = (rssi === 0) ? 'AP Mode' : (rssi + ' dBm');
      }

      // 4. Mode
      if (data.mode) {
        document.getElementById('network-mode').textContent = isStandalone ? (data.mode + ' • Fullscreen') : data.mode;
      }
    }

    function toggleRelay(id) {
      haptic();
      state['r' + id] = !state['r' + id];
      render(state);

      if (state.isSimulated) return;

      fetch('/toggle?relay=' + id)
        .then(res => res.json())
        .then(data => {
          Object.assign(state, data);
          render(state);
        })
        .catch(err => console.warn('Network delayed:', err));
    }

    function masterControl(turnOn) {
      haptic();
      for (let i = 1; i <= 4; i++) {
        state['r' + i] = turnOn;
      }
      render(state);

      if (state.isSimulated) return;

      fetch('/master?state=' + (turnOn ? 1 : 0))
        .then(res => res.json())
        .then(data => {
          Object.assign(state, data);
          render(state);
        })
        .catch(() => {
          for (let i = 1; i <= 4; i++) {
            fetch('/toggle?relay=' + i).catch(() => {});
          }
        });
    }

    function fetchStatus() {
      fetch('/status', { cache: 'no-store' })
        .then(res => {
          if (!res.ok) throw new Error('ERR');
          return res.json();
        })
        .then(data => {
          Object.assign(state, data);
          state.isSimulated = false;
          render(state);
        })
        .catch(() => {
          if (!state.isSimulated) {
            state.isSimulated = true;
            document.getElementById('network-mode').textContent = isStandalone ? 'Demo • Fullscreen App' : 'Demo Mode (Browser Preview)';
          }
          state.temp = 38.5 + (Math.sin(Date.now() / 3000) * 0.8);
          render(state);
        });
    }

    fetchStatus();
    setInterval(fetchStatus, 2000);
  </script>
</body>
</html>
)rawliteral";

// Web Handlers

// Embedded Web App Manifest for PWA Fullscreen Install
const char MANIFEST_JSON[] PROGMEM = R"rawliteral(
{
  "name": "Smart Home Hub",
  "short_name": "SmartHome",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#090a0f",
  "theme_color": "#090a0f",
  "orientation": "portrait-primary",
  "icons": [
    {
      "src": "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'%3E%3Crect width='512' height='512' rx='128' fill='%23090a0f'/%3E%3Ccircle cx='256' cy='256' r='180' fill='none' stroke='rgba(16,185,129,0.15)' stroke-width='8'/%3E%3Cpath d='M256 120 L380 230 L350 230 L350 370 L280 370 L280 290 L232 290 L232 370 L162 370 L162 230 L132 230 Z' fill='none' stroke='%2310b981' stroke-width='24' stroke-linejoin='round'/%3E%3Ccircle cx='256' cy='200' r='16' fill='%2338bdf8'/%3E%3Cpath d='M256 165 v-35 M225 185 l-25 -20 M287 185 l25 -20' stroke='%2338bdf8' stroke-width='14' stroke-linecap='round'/%3E%3C/svg%3E",
      "sizes": "192x192 512x512",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
}
)rawliteral";

void handleManifest() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "application/manifest+json", MANIFEST_JSON);
}

void handleRoot() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send_P(200, "text/html", PAGE_INDEX);
}

void handleStatus() {
    String json = "{";
    json += "\"r1\":" + String(state_light1 ? "true" : "false") + ",";
    json += "\"r2\":" + String(state_light2 ? "true" : "false") + ",";
    json += "\"r3\":" + String(state_fan ? "true" : "false") + ",";
    json += "\"r4\":" + String(state_appl ? "true" : "false") + ",";
    json += "\"temp\":" + String(temperatureRead(), 1) + ",";
    json += "\"rssi\":" + String(is_sta_connected ? WiFi.RSSI() : 0) + ",";
    json += "\"mode\":\"" + String(is_sta_connected ? "Local LAN (Cloud Online)" : "Offline Fallback AP") + "\"";
    json += "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

// Centralized Relay State Controller
void setRelayState(int channel, bool newState, const char* source) {
    bool *statePtr = NULL;
    int pin = -1;
    const char *prefKey = "";
    Device *dev = NULL;
    const char *chName = "";

    switch (channel) {
        case 1:
            statePtr = &state_light1;
            pin = RELAY1_PIN;
            prefKey = "r1";
            dev = light1;
            chName = "Light 1";
            break;
        case 2:
            statePtr = &state_light2;
            pin = RELAY2_PIN;
            prefKey = "r2";
            dev = light2;
            chName = "Light 2";
            break;
        case 3:
            statePtr = &state_fan;
            pin = RELAY3_PIN;
            prefKey = "r3";
            dev = fan;
            chName = "Fan";
            break;
        case 4:
            statePtr = &state_appl;
            pin = RELAY4_PIN;
            prefKey = "r4";
            dev = appl;
            chName = "Appliance";
            break;
        default:
            return;
    }

    if (*statePtr != newState || strcmp(source, "INIT") == 0) {
        *statePtr = newState;
        digitalWrite(pin, newState ? RELAY_ON : RELAY_OFF);
        prefs.putBool(prefKey, newState);
        if (dev && strcmp(source, "CLOUD ACTION") != 0) {
            dev->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, newState);
        }
        Serial.printf("[%s] %s -> %s\n", source, chName, newState ? "ON" : "OFF");

        // Visual status pulse
        setLED(0, 150, 150);
        delay(30);
        setLED(0, is_sta_connected ? 150 : 0, is_sta_connected ? 0 : 150);
    }
}

void toggleRelay(int channel, const char* source) {
    bool current = false;
    switch (channel) {
        case 1: current = state_light1; break;
        case 2: current = state_light2; break;
        case 3: current = state_fan; break;
        case 4: current = state_appl; break;
        default: return;
    }
    setRelayState(channel, !current, source);
}


void handleMaster() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("state")) {
        bool target = (server.arg("state").toInt() == 1);
        for (int i = 1; i <= 4; i++) {
            setRelayState(i, target, "LOCAL WEB MASTER");
        }
    }
    handleStatus();
}

void handleToggle() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("relay")) {
        int r = server.arg("relay").toInt();
        if (r >= 1 && r <= 4) {
            toggleRelay(r, "LOCAL WEB TOGGLE");
        }
    }
    handleStatus();
}

// 4-Channel AC Optocoupler Wall Switch Handler
void handlePhysicalSwitches() {
    for (int i = 0; i < 4; i++) {
        int reading = digitalRead(SWITCH_PINS[i]);
        if (reading != last_switch_reading[i]) {
            last_switch_debounce_time[i] = millis();
            last_switch_reading[i] = reading;
        }

        if ((millis() - last_switch_debounce_time[i]) > DEBOUNCE_DELAY_MS) {
            if (reading != stable_switch_state[i]) {
                stable_switch_state[i] = reading;
                toggleRelay(i + 1, "WALL SWITCH");
            }
        }
    }
}

// Provisioning & Wi-Fi Event Callback
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_INIT:
            wifi_prov_mgr_disable_auto_stop(120000);
            break;

        case ARDUINO_EVENT_PROV_START:
            Serial.println("\n**************************************************");
            Serial.println(">>> [PROVISIONING ACTIVE] WAITING FOR APP! <<<");
            Serial.printf(">>> Wi-Fi Network Name: %s\n", service_name);
            Serial.printf(">>> Password / PoP: %s\n", pop);
            Serial.println("**************************************************\n");
            setLED(0, 0, 150); // Solid Blue
            break;

        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("\n[PROVISIONING] Hotspot credentials received! Connecting...");
            setLED(150, 150, 0); // Yellow
            wifi_prov_mgr_stop_provisioning();
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            is_sta_connected = true;
            disconnect_start_time = 0;
            Serial.printf("\n[WIFI] Connected to Hotspot! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.println("[RAINMAKER] Node ONLINE in Cloud! Relays active.");

            // Initialize mDNS Responder (http://smarthome.local)
            if (MDNS.begin(MDNS_HOSTNAME)) {
                MDNS.addService("http", "tcp", HTTP_PORT);
                Serial.printf("[mDNS] Responder active at http://%s.local\n", MDNS_HOSTNAME);
            } else {
                Serial.println("[mDNS] Error setting up mDNS responder!");
            }

            // If offline fallback AP was active, turn it off
            if (is_offline_ap_active) {
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                is_offline_ap_active = false;
                Serial.println("[OFFLINE PORTAL] Closed fallback AP. Cloud mode restored.");
            }

            setLED(0, 150, 0); // Solid Green for Online
            syncStatesToCloud();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            is_sta_connected = false;
            if (disconnect_start_time == 0) {
                disconnect_start_time = millis();
            }
            Serial.println("[WIFI] Disconnected from Hotspot.");
            break;

        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[SOFT_AP] Access Point interface started.");
            break;

        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("[SOFT_AP] Access Point interface stopped.");
            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("\n[SOFT_AP] >>> Phone/Client connected to SmartHome_Offline! <<<");
            break;

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("[SOFT_AP] Phone/Client disconnected from SmartHome_Offline.");
            break;

        default:
            break;
    }
}

// RainMaker Cloud Callback
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool powerState = val.val.b;
        int ch = 0;
        if (strcmp(device_name, "Light 1") == 0) ch = 1;
        else if (strcmp(device_name, "Light 2") == 0) ch = 2;
        else if (strcmp(device_name, "Fan") == 0) ch = 3;
        else if (strcmp(device_name, "Appliance") == 0) ch = 4;

        if (ch > 0) {
            setRelayState(ch, powerState, "CLOUD ACTION");
            param->updateAndReport(val);
        }
    }
}

void setup() {
    // Disable hardware brownout detector so temporary voltage drops on wires do not reset the chip
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    Serial.setTxTimeoutMs(0); // Prevent Serial.print from blocking when running on mobile charger
    delay(200);

    Serial.println("\n==================================================");
    Serial.println("   ESP32-S3 SMART HOME: CLOUD + OFFLINE PORTAL");
    Serial.println("   RainMaker Cloud + Physical Switch Sync");
    Serial.println("==================================================");

    // Initialize Non-Volatile State Storage
    prefs.begin("smarthome", false);
    state_light1 = prefs.getBool("r1", false);
    state_light2 = prefs.getBool("r2", false);
    state_fan    = prefs.getBool("r3", false);
    state_appl   = prefs.getBool("r4", false);

    // Initialize Relays to their exact stored physical states (write state BEFORE pinMode to prevent startup glitch/chatter)
    digitalWrite(RELAY1_PIN, state_light1 ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY2_PIN, state_light2 ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY3_PIN, state_fan ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY4_PIN, state_appl ? RELAY_ON : RELAY_OFF);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(RELAY4_PIN, OUTPUT);

    // Initialize 4-Channel AC Optocoupler Wall Switch Pins
    for (int i = 0; i < 4; i++) {
        pinMode(SWITCH_PINS[i], INPUT_PULLUP);
        last_switch_reading[i] = digitalRead(SWITCH_PINS[i]);
        stable_switch_state[i] = last_switch_reading[i];
    }
    Serial.printf("[SWITCHES] Optocoupler inputs initialized on Pins %d, %d, %d, %d.\n",
                  SWITCH_PINS[0], SWITCH_PINS[1], SWITCH_PINS[2], SWITCH_PINS[3]);

    // Initialize BOOT Button
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

    // Initialize Status RGB LED
    pixel.begin();
    pixel.setBrightness(40);
    setLED(0, 0, 150); // Blue until Wi-Fi connects

    // Register Wi-Fi Provisioning Events
    WiFi.onEvent(sysProvEvent);
    disconnect_start_time = millis();

    // Initialize RainMaker Node
    Node my_node = RMaker.initNode("ESP32 Smart Home");

    // 1. Light 1 (Relay 1)
    light1 = new LightBulb("Light 1", NULL, state_light1);
    light1->addCb(write_callback);
    my_node.addDevice(*light1);

    // 2. Light 2 (Relay 2)
    light2 = new LightBulb("Light 2", NULL, state_light2);
    light2->addCb(write_callback);
    my_node.addDevice(*light2);

    // 3. Fan (Relay 3) - Custom Fan Device with ONLY Power ON/OFF (no speed slider)
    fan = new Device("Fan", ESP_RMAKER_DEVICE_FAN);
    fan->addNameParam();
    fan->addPowerParam(state_fan);
    fan->assignPrimaryParam(fan->getParamByName(ESP_RMAKER_DEF_POWER_NAME));
    fan->addCb(write_callback);
    my_node.addDevice(*fan);

    // 4. Appliance (Relay 4)
    appl = new Switch("Appliance", NULL, state_appl);
    appl->addCb(write_callback);
    my_node.addDevice(*appl);

    // 5. Diagnostics (Internal Temperature & Wi-Fi RSSI)
    temp_sensor = new TemperatureSensor("ESP32 Diagnostics", NULL, temperatureRead());
    Param wifi_param("WiFi Signal (dBm)", "esp.param.wifi-rssi", value(0), PROP_FLAG_READ);
    temp_sensor->addParam(wifi_param);
    my_node.addDevice(*temp_sensor);

    // Enable Advanced RainMaker Services
    RMaker.enableOTA(OTA_USING_TOPICS);
    RMaker.enableTZService();
    RMaker.enableSchedule();
    RMaker.enableScenes();
    RMaker.enableSystemService(SYSTEM_SERV_FLAGS_ALL, 2, 2, 2);

    // Start RainMaker Engine
    RMaker.start();

    // Start Provisioning via Bluetooth (BLE)
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                            WIFI_PROV_SECURITY_1, pop, service_name);

    // Start Immediate Local Offline Wi-Fi Access Point (SmartHome_Offline)
    WiFi.mode(WIFI_AP_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); // Cut peak current by ~60% to prevent voltage collapse on wire power
    WiFi.setSleep(false);              // Keep radio current steady, avoiding sharp di/dt voltage dips
    WiFi.softAP(offline_ap_ssid, offline_ap_pass);
    is_offline_ap_active = true;

    // Start mDNS Responder (http://smarthome.local)
    MDNS.begin(MDNS_HOSTNAME);
    MDNS.addService("http", "tcp", HTTP_PORT);

    Serial.println("\n--------------------------------------------------");
    Serial.printf(">>> [OFFLINE AP ACTIVE] SSID: %s\n", offline_ap_ssid);
    Serial.printf(">>> [OFFLINE PASSWORD]: %s\n", offline_ap_pass);
    Serial.printf(">>> Dashboard IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf(">>> Dashboard URL: http://%s.local or http://%s\n", MDNS_HOSTNAME, WiFi.softAPIP().toString().c_str());
    Serial.println("--------------------------------------------------\n");

    // Configure Web Server Routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/manifest.json", HTTP_GET, handleManifest);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/toggle", HTTP_GET, handleToggle);
    server.on("/master", HTTP_GET, handleMaster);

    server.on("/set", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        if (server.hasArg("relay") && server.hasArg("state")) {
            int r = server.arg("relay").toInt();
            bool st = (server.arg("state").toInt() == 1);
            if (r >= 1 && r <= 4) {
                setRelayState(r, st, "LOCAL API");
            }
        }
        handleStatus();
    });

    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found");
    });

    server.begin();
    Serial.println("[WEB SERVER] Local Web Server started on port 80.");

    // Initial state report to RainMaker Cloud
    syncStatesToCloud();
}

void loop() {
    // 1. Handle Local Web Server Clients
    server.handleClient();

    // 2. Handle 4-Channel Physical Wall Switches (Optocouplers)
    handlePhysicalSwitches();

    // 3. Periodic Telemetry Reporting (Every 30s to Cloud)
    if (millis() - last_telemetry_time > 30000) {
        last_telemetry_time = millis();
        if (temp_sensor && is_sta_connected) {
            float current_temp = temperatureRead();
            int current_rssi = WiFi.RSSI();
            temp_sensor->updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, current_temp);
            temp_sensor->updateAndReportParam("WiFi Signal (dBm)", current_rssi);
            Serial.printf("[TELEMETRY] Temp: %.1f C | RSSI: %d dBm reported to Cloud.\n", current_temp, current_rssi);
        }
    }

    // 4. Offline Fallback Watchdog: If disconnected from Hotspot for > 10s, start SmartHome_Offline AP
    if (!is_sta_connected) {
        if (disconnect_start_time > 0 && (millis() - disconnect_start_time > 10000)) {
            if (!is_offline_ap_active) {
                Serial.println("\n[WIFI WATCHDOG] Hotspot offline > 10s. Activating SmartHome_Offline AP...");
                WiFi.mode(WIFI_AP_STA);
                WiFi.softAP(offline_ap_ssid, offline_ap_pass);
                is_offline_ap_active = true;

                // Enable mDNS in SoftAP mode too
                MDNS.begin(MDNS_HOSTNAME);
                MDNS.addService("http", "tcp", HTTP_PORT);

                setLED(0, 0, 150); // Solid Blue for Offline AP
                Serial.println("--------------------------------------------------");
                Serial.printf(">>> [OFFLINE AP ACTIVE] SSID: %s\n", offline_ap_ssid);
                Serial.printf(">>> [OFFLINE PASSWORD]: %s\n", offline_ap_pass);
                Serial.printf(">>> Dashboard IP: %s\n", WiFi.softAPIP().toString().c_str());
                Serial.printf(">>> Dashboard URL: http://%s.local or http://%s\n", MDNS_HOSTNAME, WiFi.softAPIP().toString().c_str());
                Serial.println("--------------------------------------------------\n");
            }
        }

        // Background Hotspot Reconnect:
        // When Offline AP is active and a phone is connected, DO NOT scan/reconnect!
        // Channel scanning disrupts connected clients and causes connection failure / drops.
        if (is_offline_ap_active) {
            int numClients = WiFi.softAPgetStationNum();
            if (numClients == 0) {
                // No phones connected: periodically check if Hotspot returned (every 30s)
                if (millis() - last_reconnect_attempt > 30000) {
                    last_reconnect_attempt = millis();
                    Serial.println("[OFFLINE AP] No stations connected. Probing if Hotspot is back...");
                    WiFi.reconnect();
                }
            }
        } else {
            // Grace period before AP starts: retry every 5s
            if (millis() - last_reconnect_attempt > 5000) {
                last_reconnect_attempt = millis();
                WiFi.reconnect();
            }
        }
    }

    // 5. Physical BOOT Button Handler (GPIO 0)
    if (digitalRead(BOOT_BTN_PIN) == LOW) {
        delay(80);
        unsigned long pressStart = millis();
        while (digitalRead(BOOT_BTN_PIN) == LOW) {
            delay(50);
        }
        unsigned long pressDuration = millis() - pressStart;

        if (pressDuration > 10000) {
            Serial.println("\n[RESET] Factory Reset Triggered via Button! Clearing all data...");
            setLED(255, 0, 0);
            RMakerFactoryReset(2);
        } else if (pressDuration > 3000) {
            Serial.println("\n[RESET] Wi-Fi Reset Triggered! Re-opening Provisioning...");
            setLED(0, 0, 255);
            RMakerWiFiReset(2);
        } else {
            // Short press: Toggle Light 1 locally
            toggleRelay(1, "BOOT BUTTON");
        }
    }

    delay(10);
}
