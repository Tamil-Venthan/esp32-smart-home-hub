# 🛠️ Complete Breadboard to Custom PCB Transition Guide

This guide outlines the hardware migration process from breadboard prototyping to custom fabricated PCB modules.

---

## 🏗️ The 4 Modular Subsystems

```
[ 220V AC Mains ]
       │
       ▼
┌───────────────────────────────┐
│ Module 1: Dual-Rail Power Bd  │ ──► +5.0V High-Current Rail ──► [ 4-CH 5V Relay Board ]
│ (HLK-10M05 + AMS1117-3.3)     │ ──► +3.3V Clean Logic Rail  ──► [ ESP32 Microcontroller ]
└───────────────────────────────┘                                         ▲
       ▲                                                                  │ (3.3V Logic)
       │ (230V Switched AC)                                               │
┌───────────────────────────────┐                                         │
│ Module 2: 4-CH Optocoupler Bd │ ────────────────────────────────────────┘
│ (PC814 + 10µF Filter)         │
└───────────────────────────────┘
       ▲
       │
[ 4x Domestic Wall Switches ]
```

---

## 📦 Manufacturing Custom Boards (JLCPCB / PCBWay)

### 1. Dual-Rail Power Supply Board
- Navigate to `hardware/5v_power_supply_module/production/`.
- Upload `Gerber_5V_Power_Supply.zip` directly to [JLCPCB](https://jlcpcb.com) or [PCBWay](https://pcbway.com).
- **PCB Specifications:**
  - 2-Layer FR-4, 1.6mm thickness.
  - 1oz or 2oz copper (2oz recommended for high-current AC tracks).
  - Solder Mask: Blue or Black with white silkscreen.
  - Ensure **Milling/Routing Slots** are enabled (to route the 2.0mm creepage slot).

### 2. 4-Channel AC Optocoupler Board
- Navigate to `hardware/optocoupler_4ch_module/production/`.
- Upload `Optocoupler_4CH_Gerbers.zip` to JLCPCB.
- For SMT Assembly, upload `BOM` and `CPL_Optocoupler_4CH_JLCPCB.csv`.

---

## 🧰 Enclosure & Installation Recommendations

1. **Junction Box Enclosure:** Mount all modules inside a flame-retardant ABS electrical enclosure (minimum dimensions: 150mm x 100mm x 55mm).
2. **Mounting Standoffs:** Use nylon M3 standoffs (never brass or conductive metal) to secure the PCBs to the enclosure base.
3. **Mains Cable Glands:** Secure incoming AC cables using nylon PG7 or PG9 cable glands with strain relief.
4. **Relay Snubbers (For Inductive Loads):** When switching high-inductance loads like ceiling fans, old fluorescent ballasts, or water pumps, connect an external **RC Snubber (100Ω 2W resistor in series with a 0.1µF 275V X2 capacitor)** across the relay `COM` and `NO` screw terminals. This eliminates contact arcing and back-EMF spikes.
