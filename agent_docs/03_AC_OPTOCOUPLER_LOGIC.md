# ⚡ 230V AC Optocoupler Interface & Anti-Bounce Logic

## 🛡️ The AC Mains Interfacing Challenge

Domestic wall switches carry **230V AC Mains (50Hz / 60Hz)**. Connecting domestic switches directly to a 3.3V microcontroller presents two critical hazards:
1. **Lethal Voltage / Ground Faults:** AC mains directly destroys low-voltage silicon and poses an electrocution risk to users.
2. **50Hz Zero-Crossing Ripple (The 100Hz Problem):** AC voltage alternates polarity and crosses zero volts **100 times per second** (every 10 milliseconds). A standard optocoupler turns off near each zero crossing, generating a continuous 100Hz square wave instead of a steady HIGH or LOW logic level.

---

## 🔬 Hardware Circuit Architecture (PC814)

```
230V AC Live (via Wall Switch) ───[ 100kΩ 0.5W ]───┐
                                                   ├──► [ PC814 Bidirectional ]
230V AC Neutral ──────────────────[ 100kΩ 0.5W ]───┘        Optical Barrier (5kV)
                                                                 │
                                                       +3.3V ────┴───[ 10kΩ Pull-Up ]──┐
                                                                                       ├──► ESP32 GPIO Input
                                                                   [ 10µF Capacitor ]──┤
                                                                                       │
                                                                   ESP32 GND ──────────┴─── GND
```

### 1. Galvanic Optical Isolation
- The **Sharp PC814** IC contains two anti-parallel infrared emitting diodes optically coupled to a phototransistor.
- Provides **5,000V RMS galvanic isolation**, completely separating mains transients from the ESP32.

### 2. High-Voltage Mains Dropper Resistors
- Two **100kΩ 0.5W metal film resistors** in series drop the 230V AC RMS down to ~1.1mA forward current through the infrared diodes:
  $$I_{RMS} \approx \frac{230\text{V}}{200\text{k}\Omega} = 1.15\text{mA}$$
- Power dissipated across each resistor:
  $$P = I^2 \times R = (1.15\text{mA})^2 \times 100\text{k}\Omega \approx 0.132\text{W}$$
  *(Well within the 0.500W resistor rating, ensuring long-term cool operation).*

### 3. Hardware Zero-Crossing Smoothing
- A **10µF 16V ceramic capacitor (`C1–C4`)** is connected in parallel across the collector and emitter of each channel.
- With the 10kΩ pull-up resistor, this forms an **RC low-pass filter** with a time constant:
  $$\tau = R \times C = 10\text{k}\Omega \times 10\mu\text{F} = 100\text{ms}$$
- This $100\text{ms}$ time constant bridges the $10\text{ms}$ zero-crossing dips, turning the pulsing signal into a **smooth, continuous DC level**:
  - **Wall Switch ON (AC Present):** Phototransistor stays saturated -> **GPIO is clean LOW (0V)**.
  - **Wall Switch OFF (No AC):** Phototransistor is open -> **GPIO is pulled clean HIGH (3.3V)**.

---

## 💻 Firmware Anti-Bounce & 2-Way Synchronization Engine

### State Detection Algorithm
In domestic automation, a wall switch is **not a slave control**; it acts like a 2-way staircase switch. Regardless of whether the switch is flipped UP or DOWN, the relay must **toggle** its state:

```cpp
// In main.cpp
void checkWallSwitches() {
    unsigned long currentMillis = millis();
    
    for (int i = 0; i < NUM_RELAYS; i++) {
        int reading = digitalRead(switchPins[i]);
        
        // Check if physical switch state altered
        if (reading != lastSwitchState[i]) {
            lastDebounceTime[i] = currentMillis; // Reset anti-bounce timer
        }
        
        // Confirm stable state after debounce window
        if ((currentMillis - lastDebounceTime[i]) > DEBOUNCE_DELAY_MS) {
            if (reading != stableSwitchState[i]) {
                stableSwitchState[i] = reading;
                
                // Toggle relay state
                bool newState = !relayStates[i];
                setRelay(i, newState, true); // Push update to Cloud & NVS
            }
        }
        
        lastSwitchState[i] = reading;
    }
}
```

### Critical Timing Constraints for AI Agents:
- **`DEBOUNCE_DELAY_MS`:** Set to **`120ms`** (or minimum `80ms`).
- **Why NOT lower?** If set below 40ms, mains voltage fluctuations or inductive motor sparks when switching off a fan can produce voltage glitches that falsely toggle the relay multiple times in a fraction of a second.
