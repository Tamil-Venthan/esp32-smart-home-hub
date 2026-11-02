#include <Arduino.h>
#include "config.h"
#include "RMaker.h"
#include <esp_rmaker_common_events.h>
#include "WiFi.h"
#include "WiFiProv.h"
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <Update.h>

// Prevent Arduino framework from prematurely releasing Bluetooth memory on classic ESP32
extern "C" bool btInUse() {
    return true;
}

// Relay Active LOW Logic
#define RELAY_ON      LOW
#define RELAY_OFF     HIGH

// RainMaker Provisioning Details
const char *service_name = PROV_SERVICE_NAME;
const char *pop          = PROV_POP;

// Offline Fallback AP Details
const char *offline_ap_ssid = OFFLINE_AP_SSID;
const char *offline_ap_pass = OFFLINE_AP_PASS;

// DHT11 Sensor Driver
DHT dht(DHT_PIN, DHT_TYPE);
float current_temp     = 25.0;
float current_humidity = 50.0;
unsigned long last_dht_read_time = 0;

// Non-Volatile State Storage
Preferences prefs;

// Local Web Server on Port 80
WebServer server(HTTP_PORT);

// RainMaker Devices
static LightBulb          *light1      = NULL;
static LightBulb          *light2      = NULL;
static Fan                *fan         = NULL;
static Switch             *appl        = NULL;
static TemperatureSensor  *temp_sensor = NULL;

// Persisted States (Initialized from NVS)
bool state_light1 = false;
bool state_light2 = false;
bool state_fan    = false;
bool state_appl   = false;

// Dynamic Device Names (Persisted in NVS)
String name_light1 = DEFAULT_NAME_RELAY1;
String name_light2 = DEFAULT_NAME_RELAY2;
String name_fan    = DEFAULT_NAME_RELAY3;
String name_appl   = DEFAULT_NAME_RELAY4;

// 4-Channel AC Optocoupler Wall Switch Tracking
const uint8_t SWITCH_PINS[4]             = {SWITCH1_PIN, SWITCH2_PIN, SWITCH3_PIN, SWITCH4_PIN};
int           last_switch_reading[4]     = {HIGH, HIGH, HIGH, HIGH};
int           stable_switch_state[4]     = {HIGH, HIGH, HIGH, HIGH};
unsigned long last_switch_debounce_time[4] = {0, 0, 0, 0};

// Telemetry & LED Timing
unsigned long last_telemetry_time = 0;
unsigned long last_led_blink_time = 0;
bool          led_blink_state     = false;

// Network Watchdog Flags
bool is_sta_connected       = false;
bool is_offline_ap_active   = false;
unsigned long disconnect_start_time = 0;
unsigned long last_reconnect_attempt = 0;

unsigned long led_pulse_until = 0;

// Onboard Blue LED Controller
void updateStatusLED() {
    unsigned long now = millis();
    if (led_pulse_until > 0) {
        if (now < led_pulse_until) {
            return; // Hold pulse state momentarily
        }
        led_pulse_until = 0;
    }

    if (is_sta_connected) {
        // Solid ON when connected to Cloud
        digitalWrite(STATUS_LED_PIN, HIGH);
    } else if (is_offline_ap_active) {
        // Slow blink (1000ms period) when in Offline AP mode
        if (now - last_led_blink_time >= 500) {
            last_led_blink_time = now;
            led_blink_state = !led_blink_state;
            digitalWrite(STATUS_LED_PIN, led_blink_state ? HIGH : LOW);
        }
    } else {
        // Fast blink (200ms period) while connecting / provisioning
        if (now - last_led_blink_time >= 200) {
            last_led_blink_time = now;
            led_blink_state = !led_blink_state;
            digitalWrite(STATUS_LED_PIN, led_blink_state ? HIGH : LOW);
        }
    }
}

void pulseLED() {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    led_pulse_until = millis() + 40;
}

// Forward Declarations
void setRelayState(int channel, bool newState, const char* source);
void toggleRelay(int channel, const char* source);
void handlePhysicalSwitches();

// Synchronize physical states & DHT11 readings to RainMaker Cloud
void syncStatesToCloud() {
    if (light1) {
        light1->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, name_light1.c_str());
        light1->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_light1);
    }
    if (light2) {
        light2->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, name_light2.c_str());
        light2->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_light2);
    }
    if (fan) {
        fan->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, name_fan.c_str());
        fan->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_fan);
    }
    if (appl) {
        appl->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, name_appl.c_str());
        appl->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state_appl);
    }
    if (temp_sensor) {
        temp_sensor->updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, current_temp);
        temp_sensor->updateAndReportParam("Humidity", current_humidity);
    }
    Serial.printf("[STATE SYNC] L1(%s):%d | L2(%s):%d | Fan(%s):%d | Appl(%s):%d | Temp:%.1fC | Hum:%.1f%%\n",
                  name_light1.c_str(), state_light1,
                  name_light2.c_str(), state_light2,
                  name_fan.c_str(), state_fan,
                  name_appl.c_str(), state_appl,
                  current_temp, current_humidity);
}

// Embedded Minimal Responsive HTML Dashboard with DHT11 Climate Tiles & PWA Support
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>Smart Home // WROOM Hub</title>
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="theme-color" content="#090a0f">
  <link rel="manifest" href="/manifest.json">
  <style>
    :root {
      --bg: #090a0f;
      --surface: rgba(255, 255, 255, 0.04);
      --surface-hover: rgba(255, 255, 255, 0.07);
      --surface-active: rgba(16, 185, 129, 0.09);
      --border: rgba(255, 255, 255, 0.08);
      --text: #f8fafc;
      --text-sec: #94a3b8;
      --text-muted: #64748b;
      --emerald: #10b981;
      --cyan: #38bdf8;
      --orange: #f97316;
      --radius-lg: 20px;
      --radius-md: 14px;
      --radius-pill: 999px;
      --font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      background-color: var(--bg);
      color: var(--text);
      font-family: var(--font);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 1.5rem 1rem 3rem 1rem;
      overflow-x: hidden;
    }
    .container { width: 100%; max-width: 440px; display: flex; flex-direction: column; gap: 1.25rem; }
    header { display: flex; justify-content: space-between; align-items: center; padding: 0.5rem 0.25rem; }
    .badge {
      display: inline-flex; align-items: center; gap: 0.5rem;
      padding: 0.35rem 0.85rem; border-radius: var(--radius-pill);
      background: var(--surface); border: 1px solid var(--border);
      font-size: 0.75rem; font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em;
    }
    .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--emerald); box-shadow: 0 0 10px var(--emerald); }
    .dot.offline { background: var(--cyan); box-shadow: 0 0 10px var(--cyan); }
    
    /* Climate Grid */
    .climate-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 0.85rem; }
    .climate-card {
      background: var(--surface); border: 1px solid var(--border);
      border-radius: var(--radius-md); padding: 1rem; display: flex; flex-direction: column; gap: 0.35rem;
    }
    .climate-label { font-size: 0.75rem; color: var(--text-sec); font-weight: 500; display: flex; align-items: center; gap: 0.4rem; }
    .climate-val { font-size: 1.4rem; font-weight: 700; font-family: monospace; color: var(--text); }
    .climate-val.temp { color: var(--orange); }
    .climate-val.hum { color: var(--cyan); }
    .climate-val.chip { color: #f59e0b; }
    .climate-val.wifi { color: var(--emerald); font-size: 1.05rem; }

    /* Relays Grid */
    .relay-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 0.85rem; }
    .relay-card {
      background: var(--surface); border: 1px solid var(--border);
      border-radius: var(--radius-lg); padding: 1.25rem 1rem;
      display: flex; flex-direction: column; align-items: flex-start;
      gap: 0.75rem; cursor: pointer; transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
      user-select: none;
    }
    .relay-card:active { transform: scale(0.97); }
    .relay-card.active {
      background: var(--surface-active); border-color: rgba(16, 185, 129, 0.4);
      box-shadow: 0 0 25px rgba(16, 185, 129, 0.15);
    }
    .card-icon {
      width: 44px; height: 44px; border-radius: 12px;
      display: flex; align-items: center; justify-content: center;
      background: rgba(255, 255, 255, 0.05); font-size: 1.3rem; transition: background 0.2s;
    }
    .relay-card.active .card-icon { background: rgba(16, 185, 129, 0.2); }
    .card-info { display: flex; flex-direction: column; gap: 0.2rem; width: 100%; }
    .card-title { font-size: 0.95rem; font-weight: 600; color: var(--text); }
    .card-status { font-size: 0.75rem; color: var(--text-muted); font-weight: 500; text-transform: uppercase; }
    .relay-card.active .card-status { color: var(--emerald); }

    /* Master Buttons */
    .master-row { display: grid; grid-template-columns: 1fr 1fr; gap: 0.85rem; margin-top: 0.25rem; }
    .btn-master {
      background: var(--surface); border: 1px solid var(--border);
      color: var(--text); padding: 0.85rem; border-radius: var(--radius-md);
      font-size: 0.85rem; font-weight: 600; cursor: pointer; transition: all 0.15s;
    }
    .btn-master:active { transform: scale(0.96); background: var(--surface-hover); }
    footer { text-align: center; font-size: 0.75rem; color: var(--text-muted); margin-top: 1rem; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div>
        <h1 style="font-size: 1.3rem; font-weight: 700;">Smart Home Hub</h1>
        <p style="font-size: 0.75rem; color: var(--text-sec);">ESP32-WROOM Controller</p>
      </div>
      <div class="badge">
        <span class="dot" id="net-dot"></span>
        <span id="net-label">Connecting...</span>
      </div>
    </header>

    <!-- System Telemetry Cards: Room Climate + ESP32 Metrics -->
    <div class="climate-grid">
      <div class="climate-card">
        <div class="climate-label">🌡️ Room Temp</div>
        <div class="climate-val temp" id="val-temp">--.- &deg;C</div>
      </div>
      <div class="climate-card">
        <div class="climate-label">💧 Room Hum</div>
        <div class="climate-val hum" id="val-hum">-- %</div>
      </div>
      <div class="climate-card">
        <div class="climate-label">⚡ Chip Temp</div>
        <div class="climate-val chip" id="val-chip">--.- &deg;C</div>
      </div>
      <div class="climate-card">
        <div class="climate-label">📶 Wi-Fi Signal</div>
        <div class="climate-val wifi" id="val-wifi">-- dBm</div>
      </div>
    </div>

    <!-- 4 Appliance Relay Cards with Inline Rename -->
    <div class="relay-grid">
      <div class="relay-card" id="card-1" onclick="toggle(1)">
        <div style="display:flex; justify-content:space-between; width:100%; align-items:center;">
          <div class="card-icon">💡</div>
          <span style="font-size:0.8rem; opacity:0.6; cursor:pointer; padding:2px 6px;" onclick="event.stopPropagation(); renameChannel(1)" title="Rename">✏️</span>
        </div>
        <div class="card-info">
          <div class="card-title" id="title-1">Light 1</div>
          <div class="card-status" id="stat-1">OFF</div>
        </div>
      </div>

      <div class="relay-card" id="card-2" onclick="toggle(2)">
        <div style="display:flex; justify-content:space-between; width:100%; align-items:center;">
          <div class="card-icon">💡</div>
          <span style="font-size:0.8rem; opacity:0.6; cursor:pointer; padding:2px 6px;" onclick="event.stopPropagation(); renameChannel(2)" title="Rename">✏️</span>
        </div>
        <div class="card-info">
          <div class="card-title" id="title-2">Light 2</div>
          <div class="card-status" id="stat-2">OFF</div>
        </div>
      </div>

      <div class="relay-card" id="card-3" onclick="toggle(3)">
        <div style="display:flex; justify-content:space-between; width:100%; align-items:center;">
          <div class="card-icon">🌀</div>
          <span style="font-size:0.8rem; opacity:0.6; cursor:pointer; padding:2px 6px;" onclick="event.stopPropagation(); renameChannel(3)" title="Rename">✏️</span>
        </div>
        <div class="card-info">
          <div class="card-title" id="title-3">Fan</div>
          <div class="card-status" id="stat-3">OFF</div>
        </div>
      </div>

      <div class="relay-card" id="card-4" onclick="toggle(4)">
        <div style="display:flex; justify-content:space-between; width:100%; align-items:center;">
          <div class="card-icon">🔌</div>
          <span style="font-size:0.8rem; opacity:0.6; cursor:pointer; padding:2px 6px;" onclick="event.stopPropagation(); renameChannel(4)" title="Rename">✏️</span>
        </div>
        <div class="card-info">
          <div class="card-title" id="title-4">Appliance</div>
          <div class="card-status" id="stat-4">OFF</div>
        </div>
      </div>
    </div>

    <!-- Master Controls -->
    <div class="master-row">
      <button class="btn-master" onclick="master(1)">⚡ All ON</button>
      <button class="btn-master" onclick="master(0)">🌙 All OFF</button>
    </div>

    <!-- Hardware & OTA Status Bar -->
    <div style="background:var(--surface); border:1px solid var(--border); border-radius:var(--radius-md); padding:0.85rem 1rem; display:flex; justify-content:space-between; align-items:center; font-size:0.8rem;">
      <span style="color:var(--text-sec);">ESP32-WROOM &bull; Dual OTA</span>
      <a href="/update" style="color:var(--cyan); text-decoration:none; font-weight:600;">⚡ Wireless Update &rarr;</a>
    </div>

    <footer>ESP32-WROOM-32 &bull; DHT11 &bull; 4CH Relays &bull; Fallback Portal</footer>
  </div>

  <script>
    function updateUI(data) {
      document.getElementById('net-label').textContent = data.mode || 'Online';
      const isOnline = (data.mode && data.mode.includes('Cloud'));
      document.getElementById('net-dot').className = isOnline ? 'dot' : 'dot offline';
      
      document.getElementById('val-temp').innerHTML = data.temp !== undefined ? (data.temp.toFixed(1) + ' &deg;C') : '--.- &deg;C';
      document.getElementById('val-hum').innerHTML = data.humidity !== undefined ? (data.humidity.toFixed(0) + ' %') : '-- %';
      document.getElementById('val-chip').innerHTML = data.chip_temp !== undefined ? (data.chip_temp.toFixed(1) + ' &deg;C') : '--.- &deg;C';
      if (data.rssi && data.rssi !== 0) {
        document.getElementById('val-wifi').innerHTML = data.wifi_qual + '% <span style="font-size:0.75rem;color:var(--text-sec);font-weight:normal;">(' + data.rssi + 'dBm)</span>';
      } else {
        document.getElementById('val-wifi').innerHTML = '<span style="font-size:0.85rem;color:var(--cyan);">AP Mode</span>';
      }

      if (data.n1) document.getElementById('title-1').textContent = data.n1;
      if (data.n2) document.getElementById('title-2').textContent = data.n2;
      if (data.n3) document.getElementById('title-3').textContent = data.n3;
      if (data.n4) document.getElementById('title-4').textContent = data.n4;

      [1, 2, 3, 4].forEach(ch => {
        const state = data['r' + ch];
        const card = document.getElementById('card-' + ch);
        const stat = document.getElementById('stat-' + ch);
        if (state) {
          card.classList.add('active');
          stat.textContent = 'ON';
        } else {
          card.classList.remove('active');
          stat.textContent = 'OFF';
        }
      });
    }

    function fetchStatus() {
      fetch('/status')
        .then(res => res.json())
        .then(data => updateUI(data))
        .catch(err => console.log('Polling...', err));
    }

    function toggle(ch) {
      fetch('/toggle?relay=' + ch)
        .then(res => res.json())
        .then(data => updateUI(data));
    }

    function master(state) {
      fetch('/master?state=' + state)
        .then(res => res.json())
        .then(d => updateUI(d));
    }

    function renameChannel(ch) {
      const cur = document.getElementById('title-' + ch).textContent;
      const n = prompt('Enter custom name for Channel ' + ch + ':', cur);
      if (n && n.trim() !== '' && n.trim() !== cur) {
        fetch('/rename?ch=' + ch + '&name=' + encodeURIComponent(n.trim()))
          .then(res => res.json())
          .then(d => updateUI(d));
      }
    }

    fetchStatus();
    setInterval(fetchStatus, 2500);
  </script>
</body>
</html>
)rawliteral";

// Embedded Web App Manifest for PWA Fullscreen Install
const char MANIFEST_JSON[] PROGMEM = R"rawliteral(
{
  "name": "Smart Home WROOM Hub",
  "short_name": "SmartHome",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#090a0f",
  "theme_color": "#090a0f"
}
)rawliteral";

// Embedded Wireless OTA Update Page
const char PAGE_UPDATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Smart Home // OTA Update</title>
  <style>
    body { background:#090a0f; color:#f8fafc; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif; display:flex; justify-content:center; align-items:center; min-height:100vh; margin:0; padding:1.5rem 1rem; box-sizing:border-box; }
    .card { background:rgba(255,255,255,0.04); border:1px solid rgba(255,255,255,0.08); border-radius:18px; padding:1.75rem 1.25rem; max-width:380px; width:100%; box-shadow:0 10px 40px rgba(0,0,0,0.5); }
    h2 { font-size:1.2rem; margin-top:0; color:#fff; display:flex; align-items:center; gap:0.5rem; }
    p { font-size:0.8rem; color:#94a3b8; line-height:1.5; margin-bottom:1.25rem; }
    input[type=file] { width:100%; padding:0.85rem; border-radius:10px; border:1px dashed rgba(255,255,255,0.2); background:rgba(0,0,0,0.3); color:#f8fafc; font-size:0.85rem; margin-bottom:1.25rem; box-sizing:border-box; }
    button { width:100%; padding:0.85rem; border-radius:10px; border:none; background:#10b981; color:#fff; font-weight:700; font-size:0.95rem; cursor:pointer; }
    button:active { transform:scale(0.98); }
    .back { display:block; text-align:center; margin-top:1.25rem; font-size:0.85rem; color:#38bdf8; text-decoration:none; }
  </style>
</head>
<body>
  <div class="card">
    <h2>⚡ Wireless Firmware Update</h2>
    <p>Select your compiled <code>firmware.bin</code> file to flash the ESP32 wirelessly over the local network without USB cables.</p>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update" accept=".bin" required>
      <button type="submit">Flash &amp; Reboot ESP32</button>
    </form>
    <a href="/" class="back">&larr; Return to Dashboard</a>
  </div>
</body>
</html>
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
    float chip_temp = temperatureRead();
    int rssi = is_sta_connected ? WiFi.RSSI() : 0;
    int wifi_qual = 0;
    if (is_sta_connected) {
        if (rssi <= -100) wifi_qual = 0;
        else if (rssi >= -50) wifi_qual = 100;
        else wifi_qual = 2 * (rssi + 100);
    }

    String json = "{";
    json += "\"r1\":" + String(state_light1 ? "true" : "false") + ",";
    json += "\"r2\":" + String(state_light2 ? "true" : "false") + ",";
    json += "\"r3\":" + String(state_fan ? "true" : "false") + ",";
    json += "\"r4\":" + String(state_appl ? "true" : "false") + ",";
    json += "\"n1\":\"" + name_light1 + "\",";
    json += "\"n2\":\"" + name_light2 + "\",";
    json += "\"n3\":\"" + name_fan + "\",";
    json += "\"n4\":\"" + name_appl + "\",";
    json += "\"temp\":" + String(current_temp, 1) + ",";
    json += "\"humidity\":" + String(current_humidity, 1) + ",";
    json += "\"chip_temp\":" + String(chip_temp, 1) + ",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"wifi_qual\":" + String(wifi_qual) + ",";
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
            chName = name_light1.c_str();
            break;
        case 2:
            statePtr = &state_light2;
            pin = RELAY2_PIN;
            prefKey = "r2";
            dev = light2;
            chName = name_light2.c_str();
            break;
        case 3:
            statePtr = &state_fan;
            pin = RELAY3_PIN;
            prefKey = "r3";
            dev = fan;
            chName = name_fan.c_str();
            break;
        case 4:
            statePtr = &state_appl;
            pin = RELAY4_PIN;
            prefKey = "r4";
            dev = appl;
            chName = name_appl.c_str();
            break;
        default:
            return;
    }

    bool is_init = (strcmp(source, "INIT") == 0);
    if (*statePtr != newState || is_init) {
        bool changed = (*statePtr != newState);
        *statePtr = newState;
        digitalWrite(pin, newState ? RELAY_ON : RELAY_OFF);
        if (changed && !is_init) {
            prefs.putBool(prefKey, newState);
        }
        if (dev && strcmp(source, "CLOUD ACTION") != 0) {
            dev->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, newState);
        }
        Serial.printf("[%s] %s (GPIO %d) -> %s\n", source, chName, pin, newState ? "ON" : "OFF");
        pulseLED();
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

// RainMaker Cloud & Claim Event Callback
static void rmaker_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == RMAKER_COMMON_EVENT) {
        if (event_id == RMAKER_MQTT_EVENT_CONNECTED) {
            Serial.println("\n[RAINMAKER CLOUD] >>> MQTT CONNECTED TO ESP CLOUD! <<<");
        } else if (event_id == RMAKER_MQTT_EVENT_DISCONNECTED) {
            Serial.println("\n[RAINMAKER CLOUD] >>> MQTT Disconnected from ESP Cloud! <<<");
        }
    } else if (event_base == RMAKER_EVENT) {
        if (event_id == RMAKER_EVENT_CLAIM_STARTED) {
            Serial.println("\n[CLAIM] >>> Claiming Started... <<<");
        } else if (event_id == RMAKER_EVENT_CLAIM_SUCCESSFUL) {
            Serial.println("\n[CLAIM] >>> Claiming SUCCESSFUL! <<<");
        } else if (event_id == RMAKER_EVENT_CLAIM_FAILED) {
            Serial.println("\n[CLAIM] >>> Claiming FAILED! <<<");
        }
    }
}

// Provisioning & Wi-Fi Event Callback
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_INIT:
            wifi_prov_mgr_disable_auto_stop(10000);
            break;

        case ARDUINO_EVENT_PROV_START:
            Serial.println("\n**************************************************");
            Serial.println(">>> [PROVISIONING ACTIVE] WAITING FOR APP! <<<");
            Serial.printf(">>> BLE Device Name: %s\n", service_name);
            Serial.printf(">>> Password / PoP: %s\n", pop);
            Serial.println("**************************************************\n");
            WiFiProv.printQR(service_name, pop, "ble");
            break;

        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("\n[PROVISIONING] Hotspot credentials received! Connecting...");
            wifi_prov_mgr_stop_provisioning();
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            is_sta_connected = true;
            disconnect_start_time = 0;
            Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WIFI] Gateway: %s | DNS: %s\n", WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
            {
                IPAddress cloudIP;
                if (WiFi.hostByName("mqtt.rainmaker.espressif.com", cloudIP)) {
                    Serial.printf("[DNS TEST] SUCCESS: mqtt.rainmaker.espressif.com resolved to %s\n", cloudIP.toString().c_str());
                } else {
                    Serial.println("[DNS TEST] FAILED: Cannot resolve mqtt.rainmaker.espressif.com! Hotspot might have no internet access.");
                }
                char *nid = esp_rmaker_get_node_id();
                Serial.printf("[RAINMAKER] Node ID: %s\n", nid ? nid : "UNKNOWN");
            }
            Serial.println("[RAINMAKER] Waiting for Cloud MQTT connection...");

            // Initialize mDNS Responder (http://smarthome.local)
            if (MDNS.begin(MDNS_HOSTNAME)) {
                MDNS.addService("http", "tcp", HTTP_PORT);
                Serial.printf("[mDNS] Responder active at http://%s.local\n", MDNS_HOSTNAME);
            }

            // If offline fallback AP was active, turn it off
            if (is_offline_ap_active) {
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                is_offline_ap_active = false;
                Serial.println("[OFFLINE PORTAL] Closed fallback AP. Cloud mode restored.");
            }

            syncStatesToCloud();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            is_sta_connected = false;
            if (disconnect_start_time == 0) {
                disconnect_start_time = millis();
            }
            Serial.println("[WIFI] Disconnected from AP.");
            break;

        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[SOFT_AP] Access Point interface started.");
            break;

        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("[SOFT_AP] Access Point interface stopped.");
            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("\n[SOFT_AP] >>> Client connected to SmartHome_Offline! <<<");
            break;

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("[SOFT_AP] Client disconnected from SmartHome_Offline.");
            break;

        default:
            break;
    }
}

// RainMaker Cloud Callback
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();
    Serial.printf("[CLOUD WRITE] Device: '%s' | Param: '%s'\n",
                  device_name ? device_name : "NULL", param_name ? param_name : "NULL");

    int ch = 0;
    if (light1 && device->getDeviceHandle() == light1->getDeviceHandle()) ch = 1;
    else if (light2 && device->getDeviceHandle() == light2->getDeviceHandle()) ch = 2;
    else if (fan && device->getDeviceHandle() == fan->getDeviceHandle()) ch = 3;
    else if (appl && device->getDeviceHandle() == appl->getDeviceHandle()) ch = 4;

    if (ch == 0 && device_name) {
        if (name_light1.equalsIgnoreCase(device_name) || strcmp(device_name, "Light 1") == 0) ch = 1;
        else if (name_light2.equalsIgnoreCase(device_name) || strcmp(device_name, "Light 2") == 0) ch = 2;
        else if (name_fan.equalsIgnoreCase(device_name) || strcmp(device_name, "Fan") == 0) ch = 3;
        else if (name_appl.equalsIgnoreCase(device_name) || strcmp(device_name, "Appliance") == 0) ch = 4;
    }

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool powerState = val.val.b;
        if (ch > 0) {
            setRelayState(ch, powerState, "CLOUD ACTION");
            param->updateAndReport(val);
        }
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SPEED_NAME) == 0) {
        int speed = val.val.i;
        if (ch == 3) {
            setRelayState(3, speed > 0, "CLOUD ACTION (SPEED)");
            param->updateAndReport(val);
        }
    } else if (strcmp(param_name, ESP_RMAKER_DEF_NAME_PARAM) == 0) {
        const char *newName = val.val.s;
        if (newName && strlen(newName) > 0 && ch >= 1 && ch <= 4) {
            if (ch == 1) { name_light1 = newName; prefs.putString("n1", name_light1); }
            else if (ch == 2) { name_light2 = newName; prefs.putString("n2", name_light2); }
            else if (ch == 3) { name_fan = newName; prefs.putString("n3", name_fan); }
            else if (ch == 4) { name_appl = newName; prefs.putString("n4", name_appl); }
            param->updateAndReport(val);
            Serial.printf("[RENAME CLOUD] Channel %d renamed to '%s'\n", ch, newName);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("   ESP32-WROOM SMART HOME: 4CH + DHT11 CLIMATE");
    Serial.println("   RainMaker Cloud + Offline Portal + Wall Switches");
    Serial.println("==================================================");

    // Initialize Onboard Status Blue LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // Initialize Non-Volatile State Storage
    prefs.begin("smarthome", false);
    state_light1 = prefs.getBool("r1", false);
    state_light2 = prefs.getBool("r2", false);
    state_fan    = prefs.getBool("r3", false);
    state_appl   = prefs.getBool("r4", false);

    name_light1  = prefs.getString("n1", DEFAULT_NAME_RELAY1);
    name_light2  = prefs.getString("n2", DEFAULT_NAME_RELAY2);
    name_fan     = prefs.getString("n3", DEFAULT_NAME_RELAY3);
    name_appl    = prefs.getString("n4", DEFAULT_NAME_RELAY4);

    // Initialize Relays to their exact stored physical states
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(RELAY4_PIN, OUTPUT);
    digitalWrite(RELAY1_PIN, state_light1 ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY2_PIN, state_light2 ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY3_PIN, state_fan ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY4_PIN, state_appl ? RELAY_ON : RELAY_OFF);

    // Initialize 4-Channel AC Optocoupler Wall Switch Pins
    for (int i = 0; i < 4; i++) {
        pinMode(SWITCH_PINS[i], INPUT_PULLUP);
        last_switch_reading[i] = digitalRead(SWITCH_PINS[i]);
        stable_switch_state[i] = last_switch_reading[i];
    }
    Serial.printf("[SWITCHES] Optocoupler inputs initialized on Pins %d, %d, %d, %d.\n",
                  SWITCH_PINS[0], SWITCH_PINS[1], SWITCH_PINS[2], SWITCH_PINS[3]);

    // Initialize DHT11 Sensor
    dht.begin();
    float init_t = dht.readTemperature();
    float init_h = dht.readHumidity();
    if (!isnan(init_t) && !isnan(init_h)) {
        current_temp = init_t;
        current_humidity = init_h;
        Serial.printf("[DHT11] Initial read: %.1f C | %.1f %%\n", current_temp, current_humidity);
    } else {
        Serial.println("[DHT11] Warning: Initial read failed, using defaults until next cycle.");
    }

    // Initialize BOOT Button
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

    // Register Wi-Fi Provisioning Events
    WiFi.onEvent(sysProvEvent);
    disconnect_start_time = 0;

    // Initialize RainMaker Node
    Node my_node = RMaker.initNode("ESP32 Smart Home");

    // 1. Light 1 (Relay 1)
    light1 = new LightBulb("Light 1", NULL, state_light1);
    light1->addNameParam();
    light1->addCb(write_callback);
    my_node.addDevice(*light1);

    // 2. Light 2 (Relay 2)
    light2 = new LightBulb("Light 2", NULL, state_light2);
    light2->addNameParam();
    light2->addCb(write_callback);
    my_node.addDevice(*light2);

    // 3. Fan (Relay 3)
    fan = new Fan("Fan", NULL, state_fan);
    fan->addNameParam();
    fan->addCb(write_callback);
    my_node.addDevice(*fan);

    // 4. Appliance (Relay 4)
    appl = new Switch("Appliance", NULL, state_appl);
    appl->addNameParam();
    appl->addCb(write_callback);
    my_node.addDevice(*appl);

    // 5. Climate Sensor (DHT11 Ambient Temperature & Humidity)
    temp_sensor = new TemperatureSensor("Temperature", NULL, current_temp);
    Param hum_param("Humidity", "custom.param.humidity", value(current_humidity), PROP_FLAG_READ);
    hum_param.addBounds(value(0.0f), value(100.0f), value(0.1f));
    temp_sensor->addParam(hum_param);
    my_node.addDevice(*temp_sensor);

    // Enable Standard RainMaker Services
    RMaker.enableOTA(OTA_USING_TOPICS);
    RMaker.enableTZService();
    RMaker.enableSchedule();
    RMaker.enableScenes();

    // Register Cloud Connection & Claim Event Callbacks
    esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, rmaker_event_cb, NULL);
    esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, rmaker_event_cb, NULL);

    // Start RainMaker Engine
    RMaker.start();

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

    server.on("/rename", HTTP_GET, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        if (server.hasArg("ch") && server.hasArg("name")) {
            int ch = server.arg("ch").toInt();
            String newName = server.arg("name");
            newName.trim();
            if (newName.length() > 0 && ch >= 1 && ch <= 4) {
                if (ch == 1) {
                    name_light1 = newName;
                    prefs.putString("n1", newName);
                    if (light1) light1->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, newName.c_str());
                } else if (ch == 2) {
                    name_light2 = newName;
                    prefs.putString("n2", newName);
                    if (light2) light2->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, newName.c_str());
                } else if (ch == 3) {
                    name_fan = newName;
                    prefs.putString("n3", newName);
                    if (fan) fan->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, newName.c_str());
                } else if (ch == 4) {
                    name_appl = newName;
                    prefs.putString("n4", newName);
                    if (appl) appl->updateAndReportParam(ESP_RMAKER_DEF_NAME_PARAM, newName.c_str());
                }
                Serial.printf("[RENAME LOCAL] Channel %d set to '%s'\n", ch, newName.c_str());
            }
        }
        handleStatus();
    });

    // Wireless Web Over-The-Air (OTA) Firmware Upload Routes
    server.on("/update", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send_P(200, "text/html", PAGE_UPDATE);
    });
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        bool success = !Update.hasError();
        server.send(200, "text/html", success ? 
            "<html><body style='background:#090a0f;color:#10b981;font-family:sans-serif;text-align:center;padding:3rem;'><h2>✅ Update Successful!</h2><p style='color:#94a3b8;'>ESP32 is rebooting into the new firmware... Refreshing in 10 seconds.</p><script>setTimeout(function(){window.location.href='/';},10000);</script></body></html>" : 
            "<html><body style='background:#090a0f;color:#ef4444;font-family:sans-serif;text-align:center;padding:3rem;'><h2>❌ Update Failed</h2><p><a href='/update' style='color:#38bdf8;'>Try Again</a></p></body></html>");
        delay(1000);
        if (success) {
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("\n[OTA UPDATE] Starting wireless upload: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA UPDATE] Complete! Total bytes: %u\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found");
    });

    server.begin();
    Serial.println("[WEB SERVER] Local Web Server started on port 80.");

    // Start BLE Provisioning for Official ESP RainMaker Mobile App
    Serial.println("\n[PROV] Starting ESP RainMaker BLE Provisioning...");
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                            WIFI_PROV_SECURITY_1, pop, service_name, NULL, NULL, false);
    WiFiProv.printQR(service_name, pop, "ble");

    syncStatesToCloud();
}

void loop() {
    // 1. Handle Local Web Server Clients
    server.handleClient();

    // 1b. Fallback: If not connected to Wi-Fi after 120s, activate Offline AP
    if (!is_sta_connected && !is_offline_ap_active && millis() > 120000) {
        WiFi.softAP(offline_ap_ssid, offline_ap_pass);
        is_offline_ap_active = true;
        MDNS.begin(MDNS_HOSTNAME);
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.println("\n[OFFLINE PORTAL] Provisioning window idle. Started fallback AP: SmartHome_Offline (192.168.4.1)");
    }

    // 2. Handle 4-Channel Physical Wall Switches (Optocouplers)
    handlePhysicalSwitches();

    // 3. Update Status LED Blinking / Solid State
    updateStatusLED();

    // 4. Non-blocking DHT11 Sensor Reading (Every 3 seconds)
    if (millis() - last_dht_read_time >= DHT_READ_INTERVAL) {
        last_dht_read_time = millis();
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if (!isnan(t) && !isnan(h)) {
            current_temp = t;
            current_humidity = h;
        }
    }

    // 5. Periodic Telemetry Reporting (Every 30s to Cloud)
    if (millis() - last_telemetry_time > 30000) {
        last_telemetry_time = millis();
        if (temp_sensor && is_sta_connected) {
            temp_sensor->updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, current_temp);
            temp_sensor->updateAndReportParam("Humidity", current_humidity);
            Serial.printf("[TELEMETRY] Temp: %.1f C | Hum: %.1f %% reported to Cloud.\n",
                          current_temp, current_humidity);
        }
    }

    // 6. Background Reconnect Probing (if Wi-Fi credentials are saved)
    if (!is_sta_connected) {
        String saved_ssid = prefs.getString("wifi_ssid", "");
        String saved_pass = prefs.getString("wifi_pass", "");
        if (strlen(WIFI_SSID) > 0) {
            saved_ssid = WIFI_SSID;
            saved_pass = WIFI_PASS;
        }
        if (saved_ssid.length() > 0) {
            if (millis() - last_reconnect_attempt > 15000) {
                last_reconnect_attempt = millis();
                Serial.printf("[WIFI] Reconnecting to %s...\n", saved_ssid.c_str());
                WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());
            }
        }
    }

    // 7. Physical BOOT Button Handler (GPIO 0)
    if (digitalRead(BOOT_BTN_PIN) == LOW) {
        delay(80);
        unsigned long pressStart = millis();
        while (digitalRead(BOOT_BTN_PIN) == LOW) {
            delay(50);
        }
        unsigned long pressDuration = millis() - pressStart;

        if (pressDuration > 10000) {
            Serial.println("\n[RESET] Factory Reset Triggered via Button! Clearing all data...");
            RMakerFactoryReset(2);
        } else if (pressDuration > 3000) {
            Serial.println("\n[RESET] Wi-Fi Reset Triggered! Re-opening Provisioning...");
            RMakerWiFiReset(2);
        } else {
            // Short press: Toggle Light 1 locally
            toggleRelay(1, "BOOT BUTTON");
        }
    }

    // 8. Serial Command Interface for Diagnostics & Instant Testing
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '1') toggleRelay(1, "SERIAL MANUAL");
        else if (c == '2') toggleRelay(2, "SERIAL MANUAL");
        else if (c == '3') toggleRelay(3, "SERIAL MANUAL");
        else if (c == '4') toggleRelay(4, "SERIAL MANUAL");
        else if (c == 's' || c == 'S') {
            Serial.println("\n===== [SYSTEM DIAGNOSTICS] =====");
            Serial.printf(" Relay 1 (Light 1   - GPIO 23): %s | Switch 1 (GPIO 32): %d\n", state_light1 ? "ON" : "OFF", digitalRead(SWITCH1_PIN));
            Serial.printf(" Relay 2 (Light 2   - GPIO 22): %s | Switch 2 (GPIO 33): %d\n", state_light2 ? "ON" : "OFF", digitalRead(SWITCH2_PIN));
            Serial.printf(" Relay 3 (Fan       - GPIO 21): %s | Switch 3 (GPIO 25): %d\n", state_fan ? "ON" : "OFF", digitalRead(SWITCH3_PIN));
            Serial.printf(" Relay 4 (Appliance - GPIO 19): %s | Switch 4 (GPIO 26): %d\n", state_appl ? "ON" : "OFF", digitalRead(SWITCH4_PIN));
            Serial.printf(" Climate: %.1f C | %.1f %% | Wi-Fi RSSI: %d dBm | Free Heap: %d bytes\n", current_temp, current_humidity, WiFi.RSSI(), ESP.getFreeHeap());
            Serial.println("================================\n");
        }
    }

    delay(2);
}
