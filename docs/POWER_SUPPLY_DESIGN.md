# Power Supply Design Guide

Comprehensive power supply design guide for the Wireless POV POI system with Teensy 4.1, ESP32, and APA102 LEDs.

## Table of Contents

1. [Power Requirements Analysis](#power-requirements-analysis)
2. [Design Option 1: Single Supply with Separate Rails](#design-option-1-single-supply-with-separate-rails)
3. [Design Option 2: Three Independent Regulators](#design-option-2-three-independent-regulators)
4. [Design Option 3: Portable Battery System](#design-option-3-portable-battery-system)
5. [Design Option 4: Wall Power with Distribution Board](#design-option-4-wall-power-with-distribution-board)
6. [Battery Runtime Calculations](#battery-runtime-calculations)
7. [PCB Layouts](#pcb-layouts)
8. [Testing and Validation](#testing-and-validation)
9. [Troubleshooting Power Issues](#troubleshooting-power-issues)
10. [Safety Considerations](#safety-considerations)
11. [Component BOMs](#component-boms)

---

## Power Requirements Analysis

### Component Power Consumption

| Component | Voltage | Current (Typical) | Current (Max) | Power (Max) |
|-----------|---------|-------------------|---------------|-------------|
| Teensy 4.1 | 5V | 100-150mA | 250mA | 1.25W |
| ESP32 | 5V (via regulator) | 80-150mA | 240mA | 1.2W |
| APA102 LEDs (32) | 5V | 60mA/LED | 1.92A | 9.6W |
| **Total** | **5V** | **~300mA + LED load** | **~2.4A** | **~12W** |

### Detailed LED Power Calculations

```
Single LED (full brightness):
- Red: 20mA @ 5V
- Green: 20mA @ 5V
- Blue: 20mA @ 5V
- Total: 60mA @ 5V (all colors on = white)

32 LEDs (full brightness, all white):
- 32 × 60mA = 1,920mA = 1.92A
- Power: 1.92A × 5V = 9.6W

Typical usage (50% brightness):
- ~960mA = 0.96A
- Power: ~4.8W

With controllers:
- Total @ full brightness: 2.4A
- Total @ 50% brightness: 1.4A
```

### Operating Modes Power Profile

| Mode | Brightness | LED Current | Total Current | Power |
|------|------------|-------------|---------------|-------|
| Idle (off) | 0% | 0mA | 300mA | 1.5W |
| Low | 10% | 192mA | 492mA | 2.5W |
| Medium | 50% | 960mA | 1.26A | 6.3W |
| High | 80% | 1.54A | 1.84A | 9.2W |
| Maximum | 100% | 1.92A | 2.22A | 11.1W |

### Power Supply Sizing Guidelines

**Minimum Requirements:**
- Voltage: 5V ± 5% (4.75V - 5.25V)
- Current: 2.5A minimum
- Power: 12.5W minimum

**Recommended Specifications:**
- Voltage: 5V ± 2% (4.9V - 5.1V)
- Current: 3A (25% safety margin)
- Power: 15W
- Efficiency: >80%
- Ripple: <100mV peak-to-peak

**Safety Margin Calculations:**
```
Maximum load: 2.22A
Safety margin: 25-30%
Required capacity: 2.22A × 1.25 = 2.78A
Rounded up: 3A power supply
```

---

## Design Option 1: Single Supply with Separate Rails

**Best for**: Tabletop/stationary setup, testing, development

### Schematic

```
     AC Wall Adapter (5V 3A)
            │
            ├───[Fuse 3A]───┐
            │               │
        [+5V Rail]      [GND Rail]
            │               │
            ├───[1000µF]────┤ Power filtering
            │               │
            ├───[Teensy 4.1]┤
            │               │
            ├───[ESP32]─────┤
            │               │
            └───[APA102]────┘
                 (32 LEDs)

Legend:
├── Parallel connection
──  Wire/trace
[ ] Component
```

### Component Values

- **Input**: 5V 3A wall adapter
- **Fuse**: 3A fast-blow (protection)
- **Bulk Capacitor**: 1000µF 16V electrolytic (main power filtering)
- **LED Capacitor**: 1000µF 16V electrolytic (at LED strip)
- **Bypass Caps**: 100nF ceramic (optional, near each IC)
- **Wire Gauge**: 22 AWG for power distribution

### Advantages
✓ Simple and inexpensive  
✓ Minimal components  
✓ Easy to troubleshoot  
✓ Direct 5V supply (no voltage drop)  
✓ Good for development/testing  

### Disadvantages
✗ Not portable  
✗ Requires wall outlet  
✗ Single point of failure  
✗ Limited current protection  

### Implementation Steps

1. **Power Input**
   - Use quality 5V 3A wall adapter
   - Add barrel jack or screw terminal
   - Install 3A fuse for protection

2. **Power Distribution**
   - Create power rails on breadboard or PCB
   - Use thick traces or bus bars for 5V and GND
   - Minimize resistance in power path

3. **Filtering**
   - Place 1000µF capacitor at power input
   - Place second 1000µF capacitor at LED strip
   - Add 100nF ceramic caps near microcontrollers

4. **Protection**
   - Include fuse or PTC resettable fuse
   - Add reverse polarity protection diode (optional)
   - Consider TVS diode for surge protection

### Circuit Diagram

```
Input (Barrel Jack)
    │
    ├── [D1: 1N5819]─┐ (Reverse protection)
    │                │
    ├── [F1: 3A]─────┤ (Overcurrent)
    │                │
    ├── [C1: 1000µF]─┤ (Bulk filtering)
    │                │
    ├── [Power Rail] ┤
    │   +5V          │
    │                │
    ├── Teensy VIN ──┤
    │                │
    ├── ESP32 VIN ───┤
    │                │
    ├──[C2: 1000µF]──┤ (LED filtering)
    │                │
    └── LED Strip 5V─┤
        LED Strip GND┘

Component List:
- D1: 1N5819 Schottky diode (40V 1A) - reverse protection
- F1: 3A fast-blow fuse or PTC resettable fuse
- C1, C2: 1000µF 16V electrolytic capacitors
```

---

## Design Option 2: Three Independent Regulators

**Best for**: Systems with multiple voltage requirements, isolated power domains

### Schematic

```
    7.5-12V Input (Battery or Adapter)
            │
            ├─────[Buck Converter 1]──[5V @ 2A]──► LED Strip
            │                                       + C1 (1000µF)
            │
            ├─────[Buck Converter 2]──[5V @ 500mA]─► Teensy 4.1
            │                                        + C2 (100µF)
            │
            └─────[Buck Converter 3]──[5V @ 500mA]─► ESP32
                                                     + C3 (100µF)

Each regulator isolated, preventing noise coupling
```

### Recommended Regulators

**For LED Strip (High Current):**
- LM2596 Buck Converter (adjustable, 3A)
- MP1584 Module (3A, compact)
- XL4015 Module (5A capability)

**For Controllers (Low Noise):**
- LM7805 Linear (1A, low noise but inefficient)
- LM2940 LDO (1A, low dropout)
- AMS1117-5.0 (1A, low cost)

### Component Selection

| Regulator | Type | Output | Efficiency | Ripple | Cost |
|-----------|------|--------|------------|--------|------|
| LM2596 | Buck | 5V 3A | 85% | ~50mV | $2-3 |
| MP1584 | Buck | 5V 3A | 90% | ~30mV | $1-2 |
| LM7805 | Linear | 5V 1A | 40-60% | <10mV | $0.50 |
| AMS1117 | LDO | 5V 1A | 75% | <20mV | $0.30 |

### Advantages
✓ Isolated power domains  
✓ Reduces noise coupling  
✓ Independent current limiting  
✓ Flexible input voltage (7-24V)  
✓ Can use battery or wall adapter  

### Disadvantages
✗ More complex  
✗ Higher cost  
✗ Requires more board space  
✗ More components to fail  

### Complete Circuit

```
Input: 9-12V @ 3A
    │
    ├── [Buck 1: LM2596]──► 5V @ 2A ──┬── [1000µF] ── LED Strip
    │   Feedback: 5.0V                │
    │   Current Limit: 2.5A           └── [100nF]
    │
    ├── [Buck 2: MP1584]──► 5V @ 500mA ─┬── [100µF] ── Teensy
    │   Feedback: 5.0V                  │
    │   Current Limit: 600mA            └── [100nF]
    │
    └── [LDO 3: AMS1117]──► 5V @ 500mA ─┬── [100µF] ── ESP32
        Input: 7-12V                     │
        Dropout: 1V                      └── [100nF]

Heat Dissipation:
- Buck converters: Minimal (use heatsink if >80°C)
- Linear regulator: Calculate (Vin-Vout) × Iout
  Example: (12V - 5V) × 0.25A = 1.75W (needs heatsink)
```

### PCB Layout Considerations

1. **Separate Power Planes**
   - Independent ground planes for each regulator
   - Star ground connection at input
   - Minimize ground loops

2. **Component Placement**
   - Keep input caps close to regulator input
   - Keep output caps close to regulator output
   - Short, wide traces for high current paths

3. **Thermal Management**
   - Add copper pour under switching regulators
   - Include thermal vias for heat dissipation
   - Mount heatsinks on linear regulators if needed

---

## Design Option 3: Portable Battery System

**Best for**: Spinning POI, portable operation, wireless freedom

### Battery Chemistry Comparison

| Chemistry | Voltage | Capacity | Weight | Cost | Cycles | Notes |
|-----------|---------|----------|--------|------|--------|-------|
| Li-Ion 18650 | 3.7V | 2500-3500mAh | 45g/cell | Low | 500+ | Best choice |
| LiPo | 3.7V | High | Light | Medium | 300+ | Good for custom |
| Li-Ion Polymer | 3.7V | Medium | Light | High | 500+ | Slim form factor |
| NiMH AA | 1.2V | 2000mAh | 25g/cell | Low | 500+ | Easy to replace |

### Recommended: 3S Li-Ion Configuration

```
Battery Pack: 3S (11.1V nominal)
    │
    ├── [Cell 1: 3.7V]─┬
    ├── [Cell 2: 3.7V]─┼── Series = 11.1V
    └── [Cell 3: 3.7V]─┘
         │
         ├── [BMS 3S]──────► Protection (overcharge, discharge, short)
         │
         ├── [Buck Converter]─► 5V @ 3A
         │   (LM2596 or similar)
         │
         └── [Power Distribution]
              │
              ├── LED Strip
              ├── Teensy
              └── ESP32

Battery Stats:
- Voltage: 9.0V (discharged) to 12.6V (full)
- Capacity: 2500-3500mAh (per cell)
- Runtime: 1-3 hours (see calculations below)
```

### Complete Battery System Schematic

```
┌─────────────────────────────────────────────┐
│           3S Li-Ion Battery Pack            │
│  [Cell 1]──[Cell 2]──[Cell 3]              │
│   3.7V      3.7V      3.7V                  │
│  = 11.1V nominal, 12.6V full, 9.0V empty   │
└──────────────┬──────────────────────────────┘
               │
        ┌──────▼───────┐
        │   BMS 3S     │ 10A continuous, 15A peak
        │  Protection  │ Overcharge: 4.2V/cell
        └──────┬───────┘ Overdischarge: 2.5V/cell
               │         Short circuit protection
               │
        ┌──────▼──────────┐
        │  Power Switch   │ 10A rated
        │  (ON/OFF)       │
        └──────┬──────────┘
               │
        ┌──────▼──────────────┐
        │  Buck Converter     │ LM2596 or XL4015
        │  11.1V → 5V @ 3A    │ Efficiency: ~85%
        │  [Pot for trim]     │ Adjustable output
        └──────┬──────────────┘
               │
        ┌──────▼──────┐
        │  1000µF 16V │ Output filtering
        │  Capacitor  │
        └──────┬──────┘
               │
        ┌──────▼──────────────────────┐
        │   5V Distribution            │
        │   ├─► LED Strip (1.92A max) │
        │   ├─► Teensy (0.15A typ)    │
        │   └─► ESP32 (0.15A typ)     │
        └──────────────────────────────┘

Additional Components:
- XT60 connector for charging
- LED indicator (battery level)
- Voltage monitor (optional)
```

### Battery Pack Assembly

**Components Needed:**
- 3× 18650 Li-Ion cells (2500-3500mAh each)
- 3S BMS board (10A continuous rating)
- 18650 battery holder (3S configuration)
- LM2596 buck converter module
- XT60 connector (charging port)
- Power switch (10A rating)
- 1000µF capacitor (output)
- Wire (18 AWG for battery, 22 AWG for output)

**Assembly Steps:**

1. **Cell Selection**
   - Use matched cells (same capacity, same brand)
   - Test voltage of each cell (should be 3.7-4.0V)
   - Never mix old and new cells

2. **BMS Connection**
   ```
   BMS Connections:
   B- ──── Cell 1 negative (battery pack negative)
   B1 ──── Between Cell 1 and Cell 2
   B2 ──── Between Cell 2 and Cell 3
   B+ ──── Cell 3 positive (battery pack positive)
   P+ ──── Output positive (to load)
   P- ──── Output negative (to load)
   C+ ──── Charge positive (from charger)
   C- ──── Charge negative (from charger)
   ```

3. **Buck Converter Setup**
   - Input: BMS P+/P- output
   - Adjust pot to set 5.0V output
   - Test with multimeter before connecting load
   - Add heatsink if module gets hot (>60°C)

4. **Charging**
   - Use 12.6V 1-2A Li-Ion charger
   - Connect to XT60 charging port
   - BMS handles cell balancing
   - Charge time: 2-4 hours (depends on capacity)

### Advantages
✓ Portable and wireless  
✓ 1-3 hour runtime  
✓ Rechargeable  
✓ Perfect for spinning POI  
✓ No cables during operation  

### Disadvantages
✗ Requires charging  
✗ Limited runtime  
✗ Battery weight (135g for 3×18650)  
✗ Requires BMS and charger  
✗ Safety considerations  

### Safety Features

**Must-Have:**
- BMS with overcharge protection
- Overdischarge protection (2.5V per cell cutoff)
- Short circuit protection
- Temperature monitoring
- Proper cell isolation/insulation

**Recommended:**
- Fuse between battery and load (3-5A)
- Low voltage alarm/indicator
- Charge port separate from discharge port
- Flame-retardant enclosure

---

## Design Option 4: Wall Power with Distribution Board

**Best for**: Permanent installations, workshops, testing stations

### System Architecture

```
┌──────────────────────────────────────────────────────┐
│          AC Wall Power (120V/240V AC)                │
└────────────────────┬─────────────────────────────────┘
                     │
          ┌──────────▼──────────┐
          │  5V 10A Power Supply│ Mean Well LRS-50-5
          │  (50W capacity)     │ or similar
          └──────────┬──────────┘
                     │
          ┌──────────▼───────────────┐
          │  Power Distribution PCB  │
          │  ┌────────────────────┐  │
          │  │ [Fuse 1: 3A]       │  │ Output 1 → Main POI
          │  │ [LED 1: Power]     │  │
          │  │ [Switch 1]         │  │
          │  ├────────────────────┤  │
          │  │ [Fuse 2: 3A]       │  │ Output 2 → POI 2 (optional)
          │  │ [LED 2: Power]     │  │
          │  │ [Switch 2]         │  │
          │  ├────────────────────┤  │
          │  │ [Fuse 3: 3A]       │  │ Output 3 → POI 3 (optional)
          │  │ [LED 3: Power]     │  │
          │  │ [Switch 3]         │  │
          │  └────────────────────┘  │
          └──────────┬───────────────┘
                     │
            Multiple outputs to POI units
```

### Distribution Board Schematic

```
Input: 5V @ 10A from power supply
    │
    ├── [TVS Diode]──[GND] (Surge protection)
    │
    ├── [10,000µF]──[GND] (Bulk capacitance)
    │
    ├── Output Channel 1:
    │    ├── [Switch 1: SPST]
    │    ├── [Fuse 1: 3A]
    │    ├── [LED 1 + 1kΩ resistor]
    │    ├── [1000µF cap]
    │    └── [Terminal Block] → POI Unit 1
    │
    ├── Output Channel 2:
    │    ├── [Switch 2: SPST]
    │    ├── [Fuse 2: 3A]
    │    ├── [LED 2 + 1kΩ resistor]
    │    ├── [1000µF cap]
    │    └── [Terminal Block] → POI Unit 2
    │
    └── Output Channel 3:
         ├── [Switch 3: SPST]
         ├── [Fuse 3: 3A]
         ├── [LED 3 + 1kΩ resistor]
         ├── [1000µF cap]
         └── [Terminal Block] → POI Unit 3

Features:
- Independent on/off switches per channel
- Per-channel fuse protection
- Power indicator LEDs
- Large input capacitor bank
- Screw terminals for easy connection
```

### Recommended Power Supply

**Mean Well LRS-50-5:**
- Output: 5V @ 10A (50W)
- Input: 85-264VAC
- Efficiency: 88%
- Protection: Overload, overvoltage, short circuit
- Cost: ~$15-20
- Reliability: Industrial grade

**Alternative: Bench Power Supply**
- Adjustable voltage 0-30V
- Current limiting 0-10A
- LCD display
- Perfect for development/testing

### Distribution Board Components

| Component | Specification | Quantity | Cost |
|-----------|--------------|----------|------|
| Power Supply | Mean Well LRS-50-5 | 1 | $18 |
| PCB | Custom or protoboard | 1 | $5-10 |
| Fuse Holder | 5×20mm | 3 | $3 |
| Fuses | 3A fast-blow | 3 | $2 |
| Switches | SPST 10A toggle | 3 | $6 |
| LEDs | 5mm red | 3 | $1 |
| Resistors | 1kΩ 1/4W | 3 | $0.30 |
| Capacitors | 1000µF 16V | 3 | $3 |
| Bulk Cap | 10,000µF 16V | 1 | $5 |
| Terminal Blocks | 2-position screw | 6 | $6 |
| TVS Diode | P6KE6.8CA | 1 | $1 |

**Total Cost: ~$50-60**

### Advantages
✓ Powers multiple POI units  
✓ Individual channel control  
✓ Fuse protection per channel  
✓ High capacity (10A total)  
✓ Industrial reliability  

### Disadvantages
✗ Not portable  
✗ Requires AC power  
✗ Larger/heavier  
✗ Higher initial cost  

---

## Battery Runtime Calculations

### Formula

```
Runtime (hours) = (Battery Capacity (mAh) × Battery Voltage × Efficiency) / (Load Power (W))

Or simplified for same voltage:
Runtime (hours) = (Battery Capacity (mAh) × Efficiency) / (Load Current (mA))
```

### Example: 3S Li-Ion Pack (2500mAh per cell)

**Scenario 1: Full Brightness (worst case)**
```
Battery: 3× 2500mAh @ 3.7V = 11.1V, 2500mAh
Load: 2.22A @ 5V = 11.1W

Energy available: 2.5Ah × 11.1V = 27.75 Wh
Energy needed: 11.1W

Runtime = 27.75 Wh / 11.1W = 2.5 hours

With 85% buck converter efficiency:
Runtime = 2.5h × 0.85 = 2.1 hours
```

**Scenario 2: Medium Brightness (50%)**
```
Load: 1.26A @ 5V = 6.3W

Runtime = 27.75 Wh / 6.3W = 4.4 hours
With efficiency: 4.4h × 0.85 = 3.7 hours
```

**Scenario 3: Low Brightness (10%)**
```
Load: 0.49A @ 5V = 2.5W

Runtime = 27.75 Wh / 2.5W = 11.1 hours
With efficiency: 11.1h × 0.85 = 9.4 hours
```

### Runtime Table (2500mAh cells)

| Brightness | LED Current | Total Current | Power | Runtime |
|------------|-------------|---------------|-------|---------|
| 10% | 192mA | 492mA | 2.5W | ~9 hours |
| 25% | 480mA | 780mA | 3.9W | ~6 hours |
| 50% | 960mA | 1.26A | 6.3W | ~3.7 hours |
| 75% | 1.44A | 1.74A | 8.7W | ~2.6 hours |
| 100% | 1.92A | 2.22A | 11.1W | ~2.1 hours |

### Runtime Table (3500mAh cells)

| Brightness | LED Current | Total Current | Power | Runtime |
|------------|-------------|---------------|-------|---------|
| 10% | 192mA | 492mA | 2.5W | ~12.5 hours |
| 25% | 480mA | 780mA | 3.9W | ~8.5 hours |
| 50% | 960mA | 1.26A | 6.3W | ~5.2 hours |
| 75% | 1.44A | 1.74A | 8.7W | ~3.6 hours |
| 100% | 1.92A | 2.22A | 11.1W | ~3.0 hours |

### Capacity vs Runtime Graph (Conceptual)

```
Runtime at 50% Brightness vs Battery Capacity

Runtime (hours)
    6 │                              ●  (4000mAh)
    5 │                        ●  (3500mAh)
    4 │                  ●  (3000mAh)
    3 │            ●  (2500mAh)
    2 │      ●  (2000mAh)
    1 │ ●  (1500mAh)
    0 └─────┴─────┴─────┴─────┴─────┴─────
      1500  2000  2500  3000  3500  4000
              Battery Capacity (mAh)
```

### Recommendations

**For Long Runtime (3+ hours @ 50%):**
- Use 3500mAh 18650 cells
- Consider 4S configuration for better buck efficiency
- Add capacity indicator (voltage monitor)

**For Light Weight (spinning POI):**
- Use 2500mAh cells (lighter)
- Limit brightness to 50% max
- Still get ~3.7 hours runtime

**For Professional Use:**
- Carry spare battery pack
- 2× battery packs = all-day use
- Quick-swap design with XT60 connectors

---

## PCB Layouts

### Simple Power Distribution PCB

```
┌─────────────────────────────────────────┐
│  POV POI Power Distribution Board v1.0  │
│                                         │
│  [Input Terminal]                       │
│   +5V ○     ○ GND                       │
│                                         │
│  ┌────────────┐                         │
│  │[10,000µF]  │  Bulk Capacitor        │
│  └────────────┘                         │
│                                         │
│  Output 1:         Output 2:            │
│  [Fuse]            [Fuse]               │
│  [1000µF]          [1000µF]             │
│  +5V ○   ○ GND     +5V ○   ○ GND       │
│                                         │
│  Board Size: 50mm × 70mm                │
│  Copper: 2oz (70µm) for high current    │
│  Layers: 2-layer (top copper + bottom)  │
└─────────────────────────────────────────┘
```

### Battery Charger / Buck Converter PCB

```
┌──────────────────────────────────────────────┐
│  3S Li-Ion Battery Manager + 5V Converter    │
│                                              │
│  [XT60 Charge Port]  [XT60 Battery Port]    │
│                                              │
│  ┌─────────────────┐                        │
│  │  BMS 3S (10A)   │  Balancing circuits    │
│  │  Balance leads  │  B- B1 B2 B+           │
│  └─────────────────┘                        │
│          │                                   │
│          ▼                                   │
│  ┌─────────────────┐                        │
│  │ LM2596 Module   │  11.1V → 5V @ 3A       │
│  │ Buck Converter  │  [Trim Pot]            │
│  └─────────────────┘                        │
│          │                                   │
│          ▼                                   │
│  [Output Terminal]                          │
│   +5V ○     ○ GND                           │
│                                              │
│  [LED Indicators]                           │
│  ● Charging  ● Full  ● Low Battery          │
│                                              │
│  Board Size: 60mm × 80mm                    │
└──────────────────────────────────────────────┘
```

### PCB Design Guidelines

**Trace Width for Current Carrying:**
```
Current   Trace Width (1oz copper)  Trace Width (2oz copper)
500mA     10 mils (0.25mm)         6 mils (0.15mm)
1A        20 mils (0.5mm)          12 mils (0.3mm)
2A        40 mils (1.0mm)          25 mils (0.65mm)
3A        60 mils (1.5mm)          35 mils (0.9mm)
```

**Recommended:**
- Use 2oz copper for power PCBs
- 5V power traces: 60-80 mils (1.5-2.0mm)
- GND plane: full pour on bottom layer
- Via stitching: 20-30 vias per square inch on ground

**Layer Stack:**
```
Top Layer:
- Component placement
- Signal traces
- Power distribution traces (wide)

Bottom Layer:
- Ground plane (full pour)
- Return currents
- Additional power routing if needed
```

---

## Testing and Validation

### Pre-Power Testing Checklist

- [ ] Visual inspection of all solder joints
- [ ] Continuity test: GND connections
- [ ] Short circuit test: Power to Ground (should be open)
- [ ] Measure power supply voltage (no load): 4.9-5.1V
- [ ] Check polarity at all output terminals
- [ ] Verify fuse ratings and orientation
- [ ] Check capacitor polarity (electrolytics)

### Power-On Testing Procedure

**Step 1: No Load Test**
```
1. Connect power supply (no POI connected)
2. Measure output voltage: 4.9-5.1V expected
3. Check ripple with oscilloscope: <100mV p-p
4. Measure quiescent current: <50mA expected
5. Monitor temperature: should stay cool (<40°C)
```

**Step 2: Dummy Load Test**
```
1. Use 10Ω 10W resistor as load (500mA draw)
2. Measure voltage under load: >4.85V expected
3. Calculate voltage drop: <0.15V acceptable
4. Monitor temperature for 10 minutes
5. Check for stable operation
```

**Step 3: Half-Load Test**
```
1. Connect POI with LEDs at 50% brightness (~1.3A)
2. Measure voltage at POI: >4.8V expected
3. Measure current draw: verify matches calculation
4. Monitor for 30 minutes continuous
5. Check temperatures of all components
```

**Step 4: Full Load Test**
```
1. Set POI to full brightness (2.2A draw)
2. Measure voltage: >4.75V minimum
3. Check current limit/fuse doesn't trip
4. Monitor temperature: components should be <70°C
5. Run for 1 hour to verify thermal stability
```

### Test Equipment Needed

- Digital multimeter (voltage, current, resistance)
- Oscilloscope (for ripple measurement - optional)
- Dummy load resistors (10Ω 10W, 2.5Ω 25W)
- Infrared thermometer or temperature sensor
- Timer/stopwatch
- Test leads and clips

### Acceptance Criteria

| Parameter | Specification | Test Method | Pass/Fail |
|-----------|--------------|-------------|-----------|
| Output Voltage (No Load) | 4.9-5.1V | DMM | ☐ |
| Output Voltage (Full Load) | >4.75V | DMM @ 2.2A | ☐ |
| Voltage Ripple | <100mV p-p | Oscilloscope | ☐ |
| Current Capacity | ≥2.5A | Load test | ☐ |
| Temperature (Full Load) | <70°C | Thermometer | ☐ |
| Efficiency | >80% | Power calc | ☐ |
| Protection | Fuse blows @ >3.5A | Overcurrent test | ☐ |

### Battery System Testing

**Charge/Discharge Test:**
```
1. Fully charge battery pack (12.6V)
2. Measure voltage at full charge
3. Run POI at 50% brightness
4. Record voltage every 30 minutes
5. Note cutoff voltage (9.0V or BMS cutoff)
6. Calculate actual runtime vs theoretical
```

**Expected Voltage Curve (3S Li-Ion):**
```
12.6V ─┐  (Full charge)
12.0V  │
11.4V ─┤  (75%)
11.1V ─┤  (50% - Nominal)
10.8V ─┤  (25%)
10.2V  │
 9.0V ─┘  (Empty - cutoff)
```

---

## Troubleshooting Power Issues

### Voltage Too Low

**Symptom:** Output voltage <4.7V under load

**Possible Causes:**
1. Insufficient power supply capacity
2. Voltage drop in wiring/connectors
3. Buck converter not properly adjusted
4. Overloaded output

**Solutions:**
- Upgrade to higher current power supply
- Use thicker wire (lower gauge number)
- Check all connections for resistance
- Adjust buck converter trim pot
- Reduce LED brightness

### Random Resets / Crashes

**Symptom:** System restarts unexpectedly

**Possible Causes:**
1. Voltage droops under load
2. Insufficient bulk capacitance
3. Ground loops or poor grounding
4. Noise from buck converter

**Solutions:**
- Add more bulk capacitance (1000-10,000µF)
- Ensure proper star ground configuration
- Add 100nF ceramic caps near ICs
- Check for loose connections
- Shield/filter buck converter output

### Excessive Heat

**Symptom:** Components hot to touch (>70°C)

**Possible Causes:**
1. Linear regulator with high voltage drop
2. Insufficient heatsinking
3. Overcurrent condition
4. Poor PCB thermal design

**Solutions:**
- Add heatsink to regulator
- Increase PCB copper thickness
- Add thermal vias
- Switch to switching regulator (more efficient)
- Verify load current is within spec

### LEDs Flickering

**Symptom:** LEDs flicker or show wrong colors

**Possible Causes:**
1. Insufficient power supply capacity
2. Voltage drop in LED power wiring
3. Poor ground connection
4. Buck converter ripple too high

**Solutions:**
- Add 1000µF cap directly at LED strip
- Use thicker wire for LED power
- Verify common ground connection
- Add LC filter on buck output

### Battery Won't Charge

**Symptom:** Battery voltage doesn't increase during charging

**Possible Causes:**
1. BMS in protection mode
2. Charger voltage too low
3. Reversed polarity
4. Dead cell in pack

**Solutions:**
- Check BMS status LEDs
- Verify charger output: 12.6V for 3S
- Check polarity with multimeter
- Test individual cell voltages
- Reset BMS (disconnect battery briefly)

### Short Battery Runtime

**Symptom:** Battery depletes faster than calculated

**Possible Causes:**
1. Old/worn batteries (reduced capacity)
2. Higher than expected load current
3. Buck converter inefficiency
4. One weak cell in series pack

**Solutions:**
- Replace old batteries
- Measure actual current draw
- Check buck efficiency (could be <80%)
- Test individual cell capacity
- Use matched cells only

---

## Safety Considerations

### Electrical Safety

**Overcurrent Protection:**
- Always use fuses (3-5A rated)
- Never bypass protection devices
- Use proper wire gauge for current
- Include thermal fuses if high power

**Short Circuit Protection:**
- Install fuses close to power source
- Use PTC resettable fuses for development
- Include reverse polarity protection
- Add TVS diodes for transient protection

**Grounding:**
- All grounds must be connected (common ground)
- Use star ground topology
- Avoid ground loops
- Ensure low-resistance ground paths

### Battery Safety

**Li-Ion Battery Warnings:**
```
⚠️ DANGER:
- Never charge above 4.2V per cell
- Never discharge below 2.5V per cell
- Never short circuit terminals
- Do not puncture, crush, or incinerate
- Keep away from heat sources (>60°C)
- Do not mix old and new cells
```

**Required Protection:**
- BMS (Battery Management System) - MANDATORY
- Proper cell holder/isolation
- Fire-resistant enclosure
- Temperature monitoring
- Charge/discharge current limiting

**Emergency Procedures:**
- If battery swells: stop use immediately
- If battery gets hot (>60°C): disconnect and cool
- If fire occurs: use Class D extinguisher or sand
- Never use water on lithium battery fires
- Dispose of damaged batteries properly

### Thermal Safety

**Component Temperature Limits:**
```
Component         Max Safe Temp    Action if Exceeded
────────────────  ──────────────  ───────────────────────
Li-Ion Cells      60°C            Disconnect immediately
Electrolytic Cap  85-105°C        Add cooling/reduce load
Buck Converter    85°C            Add heatsink
Teensy 4.1        85°C            Improve airflow
ESP32             85°C            Reduce WiFi TX power
```

**Thermal Management:**
- Add heatsinks to hot components
- Ensure airflow in enclosure
- Use thermal compound
- Monitor temps during operation
- Add ventilation holes if needed

### Mechanical Safety (Spinning POI)

**For Rotating Applications:**
- Secure all components firmly
- No loose wires
- Balance weight distribution
- Use thread-locker on screws
- Strain relief on all connections
- Test at low speed first

**Enclosure Requirements:**
- Impact resistant
- Secure lid/cover
- No sharp edges
- Proper wire management
- Consider clear cover for troubleshooting

---

## Component BOMs

### BOM: Option 1 - Single Supply

| Item | Description | Qty | Unit Cost | Total | Source |
|------|-------------|-----|-----------|-------|--------|
| PS1 | 5V 3A Wall Adapter | 1 | $10 | $10 | Amazon |
| F1 | 3A Fast-Blow Fuse | 1 | $0.50 | $0.50 | DigiKey |
| C1 | 1000µF 16V Electrolytic | 2 | $0.50 | $1 | DigiKey |
| J1 | Barrel Jack | 1 | $1 | $1 | Amazon |
| J2 | Screw Terminal 2-pos | 2 | $0.50 | $1 | DigiKey |
| D1 | 1N5819 Schottky Diode | 1 | $0.30 | $0.30 | DigiKey |
| | **TOTAL** | | | **$13.80** | |

### BOM: Option 2 - Three Regulators

| Item | Description | Qty | Unit Cost | Total | Source |
|------|-------------|-----|-----------|-------|--------|
| U1 | LM2596 Buck Module | 1 | $2 | $2 | Amazon |
| U2 | MP1584 Buck Module | 1 | $1.50 | $1.50 | Amazon |
| U3 | AMS1117-5.0 LDO | 1 | $0.30 | $0.30 | DigiKey |
| C1-C3 | 100µF 16V Electrolytic | 3 | $0.30 | $0.90 | DigiKey |
| C4 | 1000µF 16V Electrolytic | 1 | $0.50 | $0.50 | DigiKey |
| PS1 | 12V 3A Wall Adapter | 1 | $12 | $12 | Amazon |
| | Heatsink for U3 | 1 | $2 | $2 | Amazon |
| | **TOTAL** | | | **$19.20** | |

### BOM: Option 3 - Battery System

| Item | Description | Qty | Unit Cost | Total | Source |
|------|-------------|-----|-----------|-------|--------|
| BAT1 | 18650 Li-Ion 3500mAh | 3 | $6 | $18 | 18650BatteryStore |
| BMS1 | 3S 10A BMS Board | 1 | $5 | $5 | Amazon |
| U1 | LM2596 Buck Module | 1 | $2 | $2 | Amazon |
| HOLD1 | 3S 18650 Holder | 1 | $3 | $3 | Amazon |
| J1 | XT60 Connector Pair | 1 | $2 | $2 | Amazon |
| SW1 | Power Switch 10A | 1 | $2 | $2 | Amazon |
| CHG1 | 12.6V 2A Li-Ion Charger | 1 | $10 | $10 | Amazon |
| C1 | 1000µF 16V Capacitor | 1 | $0.50 | $0.50 | DigiKey |
| WIRE | 18 AWG Silicone Wire | 3ft | $5 | $5 | Amazon |
| | **TOTAL** | | | **$47.50** | |

### BOM: Option 4 - Distribution Board

| Item | Description | Qty | Unit Cost | Total | Source |
|------|-------------|-----|-----------|-------|--------|
| PS1 | Mean Well LRS-50-5 | 1 | $18 | $18 | DigiKey |
| PCB | Custom PCB 50×70mm | 1 | $10 | $10 | PCBWay |
| F1-F3 | 3A Fast-Blow Fuse | 3 | $0.50 | $1.50 | DigiKey |
| FH1-FH3 | Fuse Holder | 3 | $1 | $3 | DigiKey |
| SW1-SW3 | SPST Toggle Switch | 3 | $2 | $6 | Amazon |
| LED1-LED3 | 5mm Red LED | 3 | $0.20 | $0.60 | DigiKey |
| R1-R3 | 1kΩ Resistor 1/4W | 3 | $0.10 | $0.30 | DigiKey |
| C1-C3 | 1000µF 16V Cap | 3 | $0.50 | $1.50 | DigiKey |
| C4 | 10,000µF 25V Cap | 1 | $5 | $5 | DigiKey |
| TB1-TB6 | Screw Terminal 2-pos | 6 | $1 | $6 | DigiKey |
| TVS1 | P6KE6.8CA TVS Diode | 1 | $1 | $1 | DigiKey |
| | **TOTAL** | | | **$52.90** | |

---

## Conclusion

This guide provides four complete power supply design options for the Wireless POV POI system:

1. **Single Supply** - Simple, inexpensive, good for testing
2. **Three Regulators** - Isolated domains, low noise
3. **Battery System** - Portable, 2-4 hours runtime
4. **Distribution Board** - Multiple units, workshop use

Choose based on your application:
- **Testing/Development**: Option 1
- **Low-Noise Critical**: Option 2
- **Spinning POI**: Option 3
- **Workshop/Permanent**: Option 4

### Next Steps

1. ✓ Complete wiring per [CIRCUIT_DIAGRAMS.md](CIRCUIT_DIAGRAMS.md)
2. ✓ Build power supply using chosen option
3. ✓ Test thoroughly before full operation
4. ✓ Upload firmware and test system

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Applies to**: Wireless POV POI System v1.0+

---

**Stay Powered! ⚡🔋**
