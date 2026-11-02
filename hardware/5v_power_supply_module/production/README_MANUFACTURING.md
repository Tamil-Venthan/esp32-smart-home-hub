# Manufacturing & Assembly Guide
## 220V AC to 5V (2A) & 3.3V (1A) Dual Power Supply Board

---

### 1. Recommended PCB Fabrication Specs (JLCPCB / PCBWay / Any Fab)

When uploading Gerber_5V_Power_Supply.zip to the fabricator website, select:
- **Dimensions**: 100 mm x 60 mm (Auto-detected from Edge.Cuts)
- **Layers**: 2 Layers
- **PCB Thickness**: 1.6 mm
- **Base Material**: FR-4 (Standard TG 130-140 or TG 150)
- **Copper Weight**: 1 oz (Outer layers)
- **Solder Mask Color**: Green (Default / Fastest) or any color of your choice
- **Silkscreen Color**: White
- **Surface Finish**: HASL with lead or Lead-Free HASL (RoHS)
- **Castellated Holes**: No
- **Edge Connector**: No
- **Milled Slots / Cutouts**: Yes (2.0mm isolation slot under U1, automatically included in Edge.Cuts)

---

### 2. Files in this Production Package

| File | Purpose |
| :--- | :--- |
| **Gerber_5V_Power_Supply.zip** | **Main file to upload** to JLCPCB / PCBWay / fab house. |
| **BOM_5V_Power_Supply.csv** | Full Bill of Materials with quantities, ratings, and footprints. |
| **CPL_5V_Power_Supply.csv** | Component Pick-and-Place position file (XY coordinates). |
| **gerber/** | Individual unzipped Gerber RS-274X and Excellon drill files. |

---

### 3. Recommended Hand Soldering Order (Lowest to Tallest)

Because every single component on this board is **100% Through-Hole**, you can solder the entire board using only a standard 30W–60W hand soldering iron and 60/40 or 63/37 rosin core solder:

1. **Step 1: Low-profile passives**
   - Resistors: R1 (1kΩ), R2 (1kΩ)
   - Ceramic Disc Capacitors: C2 (100nF), C5 (100nF)
2. **Step 2: Protection components & small electrolytics**
   - 5x20mm Fuse clips: F1
   - NTC Thermistor: RT1 (10D-9)
   - Varistor: MOV1 (14D471K)
   - Small Radial Electrolytic Caps: C3 (10µF), C4 (100µF)
3. **Step 3: Semiconductor ICs & LEDs**
   - Voltage Regulator: U2 (LM1117T-3.3 TO-220, metal tab faces north)
   - LEDs: LED1 (5mm Green), LED2 (5mm Blue) — Note: Long leg = Anode (+)
4. **Step 4: Connectors & Bulk Capacitor**
   - Dupont Pin Headers: J4 (5V), J5 (3.3V)
   - Screw Terminals: J1 (220V AC), J3 (3.3V Out), J2 (5V Relays Out) — Ensure wire entry holes face outward
   - Bulk Electrolytic Capacitor: C1 (1000µF 16V/25V) — Note polarity: White stripe = Negative (-)
5. **Step 5: Main AC-DC Converter Module**
   - Hi-Link U1 (HLK-10M05) — Solder last to keep the board flat while populating other components.
   - Insert 2A 250V 5x20mm slow-blow glass fuse into F1 clips.

---

### 4. Safety & First Power-Up Verification

> [!CAUTION]
> **MAINS VOLTAGE WARNING:** 220V AC is lethal. Never touch any part of the PCB on the left side (J1, F1, RT1, MOV1, U1 input pins) while mains power is connected.

1. **Cold Multimeter Check (Before plugging in):**
   - Measure resistance between J1 Pin 1 and Pin 2 (AC Live & Neutral): It should read open circuit / infinite resistance.
   - Measure resistance between +5V and GND at J2 / J4: It should show initial capacitor charging, settling to high resistance (> 1kΩ).
   - Measure resistance between +3.3V and GND at J3 / J5: High resistance (> 500Ω).
   - **Ensure there are NO dead shorts before applying AC power.**
2. **Series Bulb Test (Recommended for first power-on):**
   - Wire a 60W or 100W incandescent light bulb in series with the AC Live wire.
   - If there is an accidental short, the light bulb will glow brightly, safely protecting the board and circuit breaker.
   - If everything is normal, the bulb will flash briefly for a split second (charging C1) and then stay completely dark.
3. **Voltage Verification:**
   - Both LED1 (Green) and LED2 (Blue) should illuminate.
   - Measure DC voltage across J2: Should read **.0	ext{V} \pm 0.1	ext{V}$**.
   - Measure DC voltage across J3: Should read **.3	ext{V} \pm 0.05	ext{V}$**.