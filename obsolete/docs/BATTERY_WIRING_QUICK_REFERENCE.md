# Battery Power System - Quick Wiring Reference

## System Block Diagram

```
                         ┌─────────────────┐
                         │  3S LiPo        │
                         │  Battery        │
                         │  11.1V / 12.6V  │
                         │  1500-3000mAh   │
                         └────┬────────┬───┘
                              │        │
                         (Red)+│        │-(Black)
                              │        │
                         ┌────▼───────▼───┐
                         │  XT60 Connector │
                         │ (Disconnect pt) │
                         └────┬───────┬───┘
                              │       │
                    ┌──────────▼─────┐│
                    │  Optional:     ││
                    │  INA219 Module ││
                    │  (Monitor)     ││
                    │  5V, GND,      ││
                    │  SCL, SDA      ││
                    │  (I2C)         ││
                    └──┬──────────┬──┘│
                       │          │   │
                   (11.1V)    (GND)   │
                       │          │   │
                    ┌──▼──────────▼──┐│
                    │ 3S BMS Module  ││
                    │ (Protection)   ││
                    └──┬──────────┬──┘│
                       │          │   │
                       │    (GND) │   │
                    ┌──▼──────────▼──┐│
                    │ Buck Regulator ││
                    │ (Pololu        ││
                    │ D24V25F5S or   ││
                    │ equivalent)    ││
                    │                ││
                    │ Input: 11.1V   ││
                    │ Output 1: 5V   ││
                    │ Output 2: 3.3V ││
                    └──┬────┬────┬──┘│
                       │    │    │   │
                   (5V)│ (GND) (3.3V)
                       │    │    │   │
         ┌─────────────┼────┼────┼───┼──────────┐
         │             │    │    │   │          │
      ┌──▼───────┐   ┌─┴────▼────┐ │        ┌──▼────────┐
      │ Teensy   │   │ APA102 LED │ │        │ ESP32-S3  │
      │ 4.1      │   │ Strip (32) │ │        │           │
      │          │   │            │ │        │           │
      │VIN-5V◄───┴───┤VCC         │ │        │VIN-5V◄────┘
      │GND◄────┐ ┌───┤GND         │ │        │GND◄─────┐
      │        │ │   │            │ │        │         │
      │        │ │   │ Data-11◄───┤ │        │         │
      │        │ │   │ Clock-13◄──┤ │        │         │
      │        │ │   └────────────┘ │        │         │
      │        │ │     Level        │        │         │
      │        │ │    Shifter       │        │         │
      │        │ │   (3.3V→5V)      │        │         │
      │        │ │                  │        │         │
      │        └─┴──────────────────┴────────┴────────┘
      │           All grounds tied together
      │
      │ I2C Optional Monitor (if using INA219):
      │  SCL (pin 19) ──────────┐
      │  SDA (pin 18) ──────────┼─→ INA219 (SCL/SDA)
      │                         │
      └─────────────────────────┘
```

---

## Pin Connections Summary

### Teensy 4.1 Power Pins
```
┌─────────────────────────────┐
│      TEENSY 4.1 POWER       │
├─────────────────────────────┤
│ VIN  ← 5V from Buck Reg     │
│ GND  ← Common Ground Rail   │
│      (multiple GND pins)    │
│                             │
│ I2C (optional INA219):      │
│ Pin 19 (SCL) → INA219 SCL   │
│ Pin 18 (SDA) → INA219 SDA   │
│                             │
│ LED Control:                │
│ Pin 11 → APA102 Data        │
│ Pin 13 → APA102 Clock       │
└─────────────────────────────┘
```

### Buck Regulator Connections
```
Pololu D24V25F5S (or equivalent dual-output)

                  ┌──────────────────┐
                  │  BUCK REGULATOR  │
                  ├──────────────────┤
        11.1V ───→│ IN+              │
        GND  ───→│ IN-/GND          │
                  │                  │
                  │ OUT1 (5V @ 2.5A) │←─ To Teensy VIN
                  │ OUT2 (3.3V)      │←─ To ESP32 VIN
                  │ GND              │←─ To common GND
                  └──────────────────┘

Alternative pinouts may vary - always verify with datasheet
```

### INA219 Module Connections (Optional)
```
                  ┌──────────────────┐
                  │    INA219        │
                  ├──────────────────┤
   From Battery ─→│ + (positive)     │
                  │                  │
                  │ - (negative)     │←─ To Buck Reg Input
                  │                  │
                  │ VCC              │←─ 5V from Buck Reg
                  │ GND              │←─ Common Ground
                  │ SCL              │←─ Teensy Pin 19
                  │ SDA              │←─ Teensy Pin 18
                  └──────────────────┘

Current Sense: INA219 measures voltage drop across
               internal resistor to calculate current
```

---

## Wire Gauge Reference

| Current | Wire Size | Max Length |
|---------|-----------|-----------|
| 1-2A | 14 AWG | 3-4 feet |
| 2-3A | 12 AWG | 4-6 feet |
| 3-5A | 10 AWG | 6-8 feet |
| 5-10A | 8 AWG | 8-10 feet |

**Recommendation for POI System:**
- Main battery to buck reg: **10 AWG** (can handle full 2-3A continuous)
- 5V output to Teensy: **10 AWG** (short distance OK)
- 3.3V to ESP32: **14 AWG** (lower current draw)
- I2C wires: **22 AWG** (signal, not power)

---

## XT60 Connector Wiring

```
Connector View (looking at female connector):

    ┌──────────┐
    │    ●  ●  │  ← Two contacts
    │ (Positive)(Negative)
    └──────────┘

Battery side: Solder directly to battery balance lead
Harness side: XT60 male connector on battery harness

Current path: Battery (+) ─→ XT60 (F) ─→ XT60 (M) ─→ Buck Reg (+)
              Battery (-) ─→ XT60 (F) ─→ XT60 (M) ─→ Buck Reg (-)
```

---

## Power Flow Chart

```
Battery (11.1V max)
    ↓
[Fuse 15A - recommended]
    ↓
[XT60 Disconnect]
    ↓
[INA219 Voltage Monitor - optional]
    ↓
[3S BMS Module - Protection]
    ↓
[Buck Regulator - 11.1V → 5V + 3.3V]
    ├─→ 5V @ 2.5A ──→ Teensy VIN
    │                 (600mA typical)
    │
    │              ┌─ Level Shifter (minimal)
    │              │
    │              └─ APA102 LEDs
    │                 (1500mA at full brightness)
    │
    └─→ 3.3V @ 2.5A ──→ ESP32-S3
                       (150mA typical)
                       WiFi/BLE radio

All GND pins must connect to common ground rail
```

---

## Soldering Checklist

Before soldering any connections:

- [ ] Wire gauge appropriate for current (see table above)
- [ ] Heat shrink tubing cut and ready
- [ ] Soldering iron at proper temperature (350-400°C)
- [ ] Solder (lead-free, 60/40 ratio recommended)
- [ ] Wet sponge or brass cleaner ready
- [ ] Work in well-ventilated area
- [ ] No flammable materials nearby

### Soldering Procedure

1. Strip 1/4" (6mm) of wire insulation
2. Tin both wire end and component pad
3. Solder with continuous heat for 3-5 seconds
4. Remove iron and let cool (don't blow on it)
5. Check for cold solder joints (dull, blobby appearance)
6. Slide heat shrink over joint while cool
7. Heat shrink with heat gun (not torch!)
8. Verify connection with multimeter

---

## Testing Procedure

**Equipment needed:**
- Multimeter (or XT60 battery voltage checker)
- Small load (LED + resistor, ~100mA)

### Step 1: Battery Voltage Test
```
1. Connect multimeter to XT60 connector
2. Positive probe → Red wire
3. Negative probe → Black wire
4. Should read 11.0V - 12.6V
```

### Step 2: Buck Regulator Output Test
```
1. Power on system with battery only (no load)
2. Measure voltage at 5V output:
   - Should be exactly 5.0V ± 0.2V
3. Measure voltage at 3.3V output:
   - Should be exactly 3.3V ± 0.1V
```

### Step 3: Under Load Test
```
1. Power system with 500mA load (LEDs)
2. 5V output: Should stay 4.9V - 5.1V
3. 3.3V output: Should stay 3.2V - 3.4V
4. No heat generation on regulator
```

### Step 4: GND Continuity Test
```
1. Set multimeter to resistance (ohms)
2. Test between any two GND points
3. Should read 0Ω (no resistance)
4. Repeat for all GND connections
```

---

## Quick Troubleshooting

| Problem | Test | Solution |
|---------|------|----------|
| No power | Battery voltage | Battery dead? Use LiPo checker |
| Teens won't boot | Measure Teensy VIN | Check regulator output |
| LEDs dim | Measure 5V under load | Check wire gauge/BMS |
| INA219 not found | I2C scanner | Check SCL/SDA connections |
| Regulator hot | Current draw | Check for short circuit |
| Battery dies fast | Runtime vs capacity | Normal (WiFi draws power) |

---

## Component Substitutions

If exact components unavailable:

| Original | Substitute | Notes |
|----------|-----------|-------|
| Pololu D24V25F5S | Meanwell RSP-75 | Overkill but works |
| Pololu D24V25F5S | Custom MP2307 | More complex |
| XT60 | XT30 | Smaller, use for low-power |
| XT60 | Anderson | Different connector type |
| 10 AWG silicone | 12 AWG copper | Thinner, less flexible |

---

## Reference Voltages

**Battery Monitoring Thresholds (3S LiPo):**

| Voltage | Status | Action |
|---------|--------|--------|
| 12.6V | Fully charged | Ready to use |
| 12.0V | 90% charged | Fine for use |
| 11.1V | ~50% charged | Nominal |
| 10.5V | 20% charged | Low battery warning |
| 10.0V | ~10% charged | Recommend charging |
| 9.6V | Critical | Stop immediately |
| < 9.0V | Damaged | Do not use |

---

## Assembly Order

1. Solder battery to BMS input
2. Solder BMS output to INA219 input (if used)
3. Solder INA219 output to buck regulator input
4. Solder buck regulator outputs to Teensy/ESP32
5. Solder XT60 connector on harness side
6. Connect I2C wires from INA219 to Teensy pins 18/19
7. Verify all connections with multimeter
8. Upload firmware with battery monitoring
9. Test power delivery before hot-swapping battery

---

**Last Updated**: 2026-02-19
**Version**: Quick Reference v1.0
