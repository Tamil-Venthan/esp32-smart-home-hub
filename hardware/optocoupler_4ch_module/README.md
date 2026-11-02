# ⚡ 4-Channel 230V AC Optocoupler Wall Switch Interface

Dedicated sensor interface board for connecting 4 domestic 230V AC wall switches to low-voltage 3.3V microcontrollers (ESP32-WROOM / ESP32-S3) with complete 5,000V optical galvanic isolation.

---

## 📐 Technical Specifications
* **Optocoupler IC:** 4x Sharp **PC814** (Bidirectional AC Input, DIP-4).
* **Isolation Voltage:** 5,000V RMS Galvanic Barrier.
* **Input Voltage:** 100V – 240V AC (50Hz / 60Hz) from domestic toggle or rocker switches.
* **Mains Current Draw:** ~1.15mA per channel across two 100kΩ 0.5W dropping resistors.
* **Zero-Crossing Filter:** Onboard 10µF 16V ceramic smoothing capacitors bridge the 100Hz AC zero-crossing dips, delivering continuous, jitter-free 3.3V DC logic to the ESP32.
* **Output Logic:**
  - Wall Switch **OFF** -> Output is **HIGH (3.3V)**.
  - Wall Switch **ON** -> Output is **LOW (0V / GND)**.

---

## 📂 Folder Contents
* `Optocoupler_4CH.kicad_pro` / `.kicad_sch` / `.kicad_pcb`: Complete KiCad project files.
* `production/`: JLCPCB fabrication zip (`Optocoupler_4CH_Gerbers.zip`), CPL, and BOM.
* `renders/`: High-resolution top and bottom board renders.
