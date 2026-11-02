# ⚠️ 230V AC Mains Electrical Safety & Wiring Checklist

> [!CAUTION]
> **DANGER: 110V – 240V AC MAINS VOLTAGE CAN CAUSE SEVERE INJURY OR DEATH.**  
> Always turn off the main circuit breaker (MCB) in your electrical distribution panel before touching, measuring, or rewiring any high-voltage AC cables.

---

## ⚡ 1. Relay Output Terminal Wiring

On every channel of the 4-channel relay module:
* **`COM` (Common Terminal):** Connect to the **Phase / Live wire (Brown or Red)** incoming from the wall circuit breaker.
* **`NO` (Normally Open Terminal):** Connect to the load wire leading to the light bulb, fan, or socket.
* **`NC` (Normally Closed Terminal):** **LEAVE EMPTY.** (Using NC causes loads to stay ON whenever the ESP32 is powered off).
* **Neutral Wire (Blue or Black):** **NEVER connect Neutral to the relay terminals!** Neutral runs directly from the mains distribution board to the load.

```
  AC Mains Phase (Live) ─────► [ COM ] (Relay Switch) [ NO ] ─────► Appliance Load
                                                                           │
  AC Mains Neutral (Return) ───────────────────────────────────────────────┘
```

---

## ⚡ 2. Physical Wall Switch (Optocoupler) Wiring

Connecting physical household switches to the 4-CH PC814 optocoupler board:
* Each wall switch terminal sends a switched **230V Phase wire** into terminals `SW1`, `SW2`, `SW3`, or `SW4`.
* Terminal `AC_N` on the optocoupler board connects to the **Mains Neutral line**.
* **Optical Isolation Barrier:** The low-voltage side of the optocoupler board (`3V3`, `GND`, `OUT1–OUT4`) is completely isolated (5,000V RMS) from the mains inputs. Never connect AC Neutral or Live to the low-voltage header!

---

## ✅ Pre-Power-On Safety Checklist

Before flipping on the main AC breaker:
1. [ ] **Continuity Check:** Use a multimeter in resistance/continuity mode to ensure **zero continuity** between any AC screw terminal and the low-voltage DC rails (+5V, +3.3V, GND).
2. [ ] **Common Ground:** Confirm the ESP32 `GND`, relay board `GND`, and optocoupler board `GND` are tied together.
3. [ ] **Fuse Verified:** Ensure a `2A 250V Slow-Blow Fuse (5x20mm)` is seated firmly in the fuse holder.
4. [ ] **Creepage Clearance:** Ensure no loose stray wire strands bridge the 2.0mm isolation slot on the power board.
5. [ ] **Insulated Enclosure:** Mount all AC boards inside a fire-resistant ABS plastic junction box. Never operate bare boards on metallic or conductive surfaces!
