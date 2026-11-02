# ⚡ 220V AC to 5V (2A) & 3.3V (1A) Isolated Power Supply

Industrial-grade dual-rail power supply designed for powering ESP32 smart home systems and up to 10 relays simultaneously with zero brownout resets.

---

## 📐 Technical Specifications
* **Mains Input:** 100V – 240V AC (50/60 Hz).
* **Primary SMPS Module:** Hi-Link HLK-10M05 (5V DC @ 2.0A, 10 Watts Isolated).
* **Primary High-Current Rail:** **+5.0V @ 2.0A max** (Powers relay coils, 1000µF Low-ESR filter).
* **Secondary Logic Rail:** **+3.3V @ 1.0A max** (AMS1117-3.3 linear regulator for ESP32 & sensors).
* **Surge Protection:** 14D471K MOV (Metal Oxide Varistor, clamps surges at 710V).
* **Inrush Suppression:** 10D-9 NTC Thermistor.
* **Overcurrent Protection:** 2A 250V Slow-Blow Cartridge Fuse (5x20mm).
* **High-Voltage Creepage:** 2.0mm physical routed air gap slot (>6.3mm effective isolation).

---

## 📂 Folder Contents
* `5V_Power_Supply.kicad_pro` / `.kicad_sch` / `.kicad_pcb`: Complete KiCad project files.
* `5V_Power_Supply_Schematic.pdf`: High-resolution vector schematic for print & review.
* `BOM.csv`: Sourced bill of materials with LCSC part numbers.
* `production/`: Complete fabrication package (Gerber zip, drill files, CPL pick-and-place, and BOM).
* `renders/`: 3D PCB renders showing component placement.
