# Battery Power System Integration Guide

Complete guide for integrating battery power monitoring into your wireless POV poi system.

## Overview

This guide covers:
1. **Hardware selection** - LiPo batteries and power regulators
2. **Firmware integration** - Adding battery monitoring to Teensy
3. **Wiring and assembly** - Physical connections
4. **Charging and safety** - Proper handling procedures

---

## Part 1: Hardware Components

### Battery Modules

#### Module 1 - Quick Sessions (30-45 minutes)
- **Type**: 3S LiPo Battery (11.1V nominal)
- **Capacity**: 1500 mAh
- **Discharge Rating**: 50-100C
- **Weight**: ~160g
- **Vendors**: Turnigy, Tattu, Thunder Power, Gens ace
- **Expected Price**: $20-30
- **Runtime**: 45-60 min at medium brightness

#### Module 2 - Extended Sessions (2-3 hours)
- **Type**: 3S LiPo Battery (11.1V nominal)
- **Capacity**: 3000 mAh
- **Discharge Rating**: 50-100C
- **Weight**: ~230g
- **Runtime**: 2-2.5 hours at medium brightness
- **Expected Price**: $35-45

**Where to buy:**
- Amazon (search "3S LiPo 1500mAh" or "3S LiPo 3000mAh")
- HobbyKing (international)
- Local hobby/drone shops

### Power Management Circuits

#### 1. Buck Regulator (Required - convert 11.1V to 5V/3.3V)

**Recommended Option**: Pololu D24V25F5S dual-output regulator

```
┌─────────────────────────────────┐
│ Pololu D24V25F5S                │
│ (or equivalent dual-output)     │
├─────────────────────────────────┤
│ • 5V @ 2.5A output              │
│ • 3.3V @ 2.5A output            │
│ • Input: 6-40V (can handle 12V) │
│ • 92% efficiency                │
│ • Current limiting built-in      │
└─────────────────────────────────┘
```

**Alternatives:**
- MP2307DN module (slightly smaller, requires assembly)
- Meanwell RSP-75 (if you prefer redundancy)

**Cost**: $15-25

#### 2. Battery Management System (BMS) - Required

**Purpose**: Protects LiPo battery from:
- Over-charging
- Over-discharging
- Over-current
- Short circuit

**Specifications**:
- 3S LiPo BMS module
- 50-100A rated (more than sufficient)
- Integrated protection circuitry
- Pre-wired for battery connector

**Cost**: $8-15

**Where to buy**: Amazon (search "3S BMS 50A")

#### 3. Voltage/Current Monitor (INA219)

**Purpose**: Real-time battery monitoring via I2C

```
┌──────────────────────┐
│ INA219 Module        │
├──────────────────────┤
│ • I2C interface      │
│ • Measures voltage   │
│ • Measures current   │
│ • Status LEDs        │
└──────────────────────┘
```

**Pinout**:
- VCC → 5V
- GND → Ground
- SCL → Teensy pin 19
- SDA → Teensy pin 18

**Cost**: $5-10

**Where to buy**: Amazon (search "INA219 module")

### Connectors & Wiring

| Item | Purpose | Cost |
|------|---------|------|
| XT60 connectors (gold-plated) | Battery disconnect | $2-5 |
| 10 AWG silicone wire (red/black) | Power distribution | $3-5 |
| M3 nylon spacers | Insulation/mounting | $1-2 |
| Heat shrink tubing | Cable protection | $1-2 |
| XT30 micro connectors (optional) | BMS to regulator | $2-3 |

**Total component cost: $55-110**

---

## Part 2: Firmware Integration

### Step 1: Add Battery Monitor Header

The file `BatteryMonitor.h` is already included in your firmware folder. It provides:
- Voltage reading functions
- Current measurement
- Battery percentage estimation
- Low-battery alerts

### Step 2: Integrate into teensy_firmware.ino

**Add this include at the top** (after existing includes):

```cpp
#include "BatteryMonitor.h"
```

**Add this global variable** (in the "Display state" section, around line 137):

```cpp
// Battery monitoring
BatteryMonitor battery;
bool batteryWarningShown = false;
uint32_t lastBatteryCheck = 0;
```

**Add this to the setup() function** (after FastLED.begin(), around line 210):

```cpp
  // Initialize battery monitor
  if (!battery.begin()) {
    Serial.println("WARNING: Battery monitor not detected. Continuing without monitoring.");
    Serial.println("Check INA219 connections on I2C (pins 18/19)");
  } else {
    Serial.println("Battery monitor initialized");
    Serial.print("Voltage: ");
    Serial.print(battery.getVoltage());
    Serial.println("V");
  }
```

**Add this to the loop() function** (after processSerialCommands(), around line 231):

```cpp
  // Check battery every 5 seconds
  if (millis() - lastBatteryCheck >= 5000) {
    lastBatteryCheck = millis();
    checkBattery();
  }
```

### Step 3: Add Battery Check Function

**Add this function** at the end of the file (before the closing brace):

```cpp
void checkBattery() {
  float voltage = battery.getVoltage();
  float percentage = battery.getPercentage();
  float current = battery.getCurrent();

  // Serial monitor output (for debugging)
  Serial.print("Battery: ");
  Serial.print(voltage);
  Serial.print("V (");
  Serial.print(percentage);
  Serial.print("%) | Current: ");
  Serial.print(current);
  Serial.print("A | Runtime: ");
  Serial.print(battery.getRuntimeMinutes());
  Serial.println(" min");

  // Critical battery warning
  if (battery.isCritical()) {
    Serial.println("🚨 CRITICAL BATTERY - SHUTTING DOWN");
    // Flash red pattern as visual warning
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Red;
    }
    FastLED.show();
    delay(100);
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    // Stop displaying
    currentMode = 0;
    displaying = false;
  }
  // Low battery warning
  else if (battery.isLowBattery() && !batteryWarningShown) {
    Serial.println("⚠️ LOW BATTERY - Please recharge soon");
    batteryWarningShown = true;

    // Flash yellow pattern as visual warning
    for (int x = 0; x < 3; x++) {
      for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Yellow;
      }
      FastLED.show();
      delay(200);
      for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      delay(200);
    }
  }
  // Reset warning flag when battery recovers
  else if (!battery.isLowBattery() && batteryWarningShown) {
    batteryWarningShown = false;
  }
}
```

### Step 4: Verify Compilation

1. Open `teensy_firmware.ino` in Arduino IDE
2. Select Board: **Teensy 4.1**
3. Select USB Type: **Serial**
4. Click **Verify** (compile without uploading)
5. Check for errors - if successful, proceed to wiring

---

## Part 3: Wiring Diagram

### Full System Wiring

```
┌─────────────────────────────────────────────────────────────┐
│ BATTERY POWER SYSTEM WIRING                                 │
└─────────────────────────────────────────────────────────────┘

3S LiPo Battery (11.1V nominal)
├─ Positive (Red) ──────┐
│                       │
│                    ┌──▼──────────┐
│                    │ 3S BMS      │
│                    │ Module      │
│                    │ (Protection)│
│                    └──┬──────────┘
│                       │
│        ┌──────────────┘
│        │
│   [XT60 Connector - Male on battery]
│        │
│   [XT60 Connector - Female on harness]
│        │
│    ┌───▼────────────────────────────┐
│    │ INA219 Voltage Monitor          │ ◄── (Optional but recommended)
│    │ (Measures battery health)       │     SCL → Teensy pin 19
│    │                                 │     SDA → Teensy pin 18
│    └───┬────────────────────────────┘
│        │
│        ├─ 11.1V from INA219
│        │   (or direct from XT60)
│        │
│    ┌───▼──────────────────┐
│    │ BUCK REGULATOR       │
│    │ Dual-output          │
│    │                      │
│    │ Input: 11.1V         │
│    ├─ 5V @ 2.5A output ──┐
│    │                      │
│    ├─ 3.3V @ 2.5A ────┐  │
│    │                  │  │
│    └────────┬─────────┘  │
│             │            │
│        ┌────▼────┐   ┌───▼────┐
│        │ Teensy  │   │ APA102  │
│        │ 4.1     │   │ LEDs    │
│        │         │   │         │
│        ├─ GND ──────────────────────┬─ GND
│        │         │   │         │    │
│        └─────────┴───┴─────────┘    │
│                                      │
└──────────────────────────────────────┘

GND Connection (Critical):
- Battery GND (Black)
- Teensy GND
- APA102 GND
- Level shifter GND
- Buck regulator GND
- INA219 GND
→ All connected to common ground rail
```

### Detailed Connection Points

#### From Battery to Buck Regulator:

```
Battery (+) → XT60 (M) → XT60 (F) → 10 AWG Red Wire → Buck Reg Input+
Battery (-) → XT60 (M) → XT60 (F) → 10 AWG Blk Wire → Buck Reg Input-
                                   └─ All grounds tied to common rail
```

#### Buck Regulator Outputs:

**5V Output (for Teensy + level shifter):**
```
Buck Reg 5V Out → 10 AWG Red → XT60 or JST connector → Teensy VIN
Buck Reg GND    → 10 AWG Blk → Teensy GND
```

**3.3V Output (for ESP32):**
```
Buck Reg 3.3V Out → Teensy pin with 3.3V rail (or direct to ESP32)
```

#### APA102 LED Strip:

```
Teensy Pin 11 → Level Shifter (5V side) → APA102 Data
Teensy Pin 13 → Level Shifter (5V side) → APA102 Clock
Teensy GND    → Level Shifter GND        → APA102 GND
5V from Buck  → Level Shifter VCC        → APA102 VCC
```

**Note**: Use existing level shifter - no changes needed!

#### INA219 Battery Monitor (Optional):

```
INA219 Positive → Battery positive rail (after XT60)
INA219 Negative → Buck regulator input negative
INA219 SCL      → Teensy pin 19 (I2C0_SCL)
INA219 SDA      → Teensy pin 18 (I2C0_SDA)
INA219 GND      → Common ground
INA219 VCC      → 5V from buck regulator
```

---

## Part 4: Physical Assembly

### Recommended Layout

```
Poi Handle/Enclosure:
┌─────────────────────────────────────────┐
│                                         │
│  ┌─────────────────────────────────┐   │
│  │ Battery Module (in pouch)       │   │
│  │ ┌─────────────────────────────┐ │   │
│  │ │ 3S LiPo Battery (160-230g) │ │   │
│  │ └─────────────────────────────┘ │   │
│  │                                 │   │
│  │ ┌─────────────────────────────┐ │   │
│  │ │ BMS + Buck Regulator        │ │   │
│  │ │ + INA219 (30-40g)           │ │   │
│  │ └─────────────────────────────┘ │   │
│  └─────────────┬───────────────────┘   │
│                │ XT60 Connector        │
│  ┌─────────────▼───────────────────┐   │
│  │ Teensy 4.1 + ESP32-S3           │   │
│  │ + APA102 LED Strip (32 LEDs)    │   │
│  └─────────────────────────────────┘   │
│                                         │
└─────────────────────────────────────────┘
```

### Assembly Steps

1. **Prepare Battery Pack**
   - Check LiPo for physical damage
   - Ensure balance connector is accessible
   - Insert into protective pouch

2. **Install BMS Module**
   - Wire battery to BMS input
   - Wire BMS output to buck regulator input
   - Use heat shrink on all solder joints

3. **Mount Buck Regulator**
   - Secure in protective case (prevents shorts)
   - Ensure good ventilation (can get warm at full load)
   - Solder output wires with proper gauge

4. **Install INA219 Monitor** (Optional)
   - Mount near battery pack
   - Solder between BMS output and buck regulator input
   - Run I2C wires to Teensy

5. **Connect Main Harness**
   - XT60 connector between battery pouch and main circuit
   - Allows quick battery swaps without soldering

6. **Test Before Use**
   - Measure voltages at each stage
   - Upload firmware with battery monitoring
   - Check serial output for battery readings

---

## Part 5: Charging & Safety

### LiPo Battery Safety

⚠️ **CRITICAL SAFETY RULES:**

1. **Never leave batteries unattended while charging**
2. **Use only LiPo-compatible chargers** (not standard USB chargers)
3. **Charge on non-flammable surface** (ceramic tile, concrete)
4. **Store in fireproof container** when not in use
5. **Never discharge below 3.0V per cell** (9.0V for 3S)
6. **Never exceed 4.2V per cell** (12.6V for 3S)

### Charging Procedure

#### Recommended Charger: Tenergy Balance Charger

**Specifications:**
- Supports 3S LiPo batteries
- Balance charging (equalizes cell voltage)
- Auto-detection of battery configuration
- Cost: $20-40

**Charging Steps:**

```
1. Connect battery balance connector to charger
2. Connect main battery connector to charger
3. Set charger to:
   - Chemistry: LiPo
   - Cells: 3S
   - Current: 1C (1.5A for 1500mAh, 3A for 3000mAh)
4. Press "Start" and wait for completion
5. Green LED indicates fully charged
6. Unplug battery immediately after charging complete
```

**Charging Times:**
- 1500mAh at 1.5A: ~1 hour
- 3000mAh at 3A: ~1 hour
- Using lower current (0.5C) extends battery life

### Storage

- Store at **3.8V per cell** (11.4V for 3S)
- Cool, dry location away from flammable materials
- In fireproof box or LiPo-safe bag
- Check monthly (should lose < 0.1V per month)

### Pre-flight Checklist

Before spinning the poi:

```
□ Battery voltage between 11.0V and 12.6V
□ INA219 shows voltage on serial monitor
□ Current draw reads correctly (should be ~0A at idle)
□ No visible damage to battery
□ All connectors are tight
□ No heat from regulator
□ Both poi have similar battery levels
```

---

## Part 6: Troubleshooting

### No Power

**Symptoms**: Teensy doesn't boot, no LED activity

**Solutions:**
1. Check XT60 connectors - ensure fully seated
2. Measure voltage at buck regulator input (should be 11-12.6V)
3. Check battery voltage with multimeter (should be > 9V)
4. Verify BMS is not in protection mode (too low voltage)

### INA219 Not Detected

**Symptoms**: Serial output shows "INA219 not found at 0x40"

**Solutions:**
1. Check I2C wiring (pins 18/19 on Teensy)
2. Verify INA219 is powered (5V)
3. Check for I2C address conflicts (use I2C scanner to verify)
4. System will work without INA219 (just no battery monitoring)

### Voltage Drop When LEDs Turn On

**Symptoms**: Voltage drops significantly when displaying

**Solutions:**
1. Check wire gauge (should be 10 AWG minimum for power)
2. Verify all ground connections
3. Check for loose XT60 connectors
4. Ensure buck regulator output can handle current (should be 2.5A+ rated)

### Battery Dies Quickly

**Potential causes:**
1. WiFi radio drawing constant 200-300mA (expected)
2. Full brightness constantly (draws 2-3A)
3. Battery capacity lower than labeled (common in cheap batteries)
4. Check runtime calculation against observed runtime

---

## Part 7: Firmware Serial Monitor Output

After integration, your serial output should look like:

```
Battery monitor initialized
Voltage: 11.52V

Battery: 11.52V (100%) | Current: 0.45A | Runtime: 210 min
Battery: 11.51V (100%) | Current: 1.23A | Runtime: 128 min
Battery: 11.48V (99%) | Current: 1.18A | Runtime: 124 min
...
Battery: 10.80V (21%) | Current: 1.15A | Runtime: 18 min
⚠️ LOW BATTERY - Please recharge soon
...
Battery: 9.65V (2%) | Current: 0.98A | Runtime: 2 min
🚨 CRITICAL BATTERY - SHUTTING DOWN
```

---

## Modular Battery Swap Procedure

### Swapping Batteries in the Field

**Time**: < 30 seconds

1. Stop current display (send idle command or press button)
2. Disconnect XT60 connector on poi
3. Swap battery module from pouch
4. Reconnect XT60 (should click in place)
5. Wait 2-3 seconds for regulator to stabilize
6. Resume display

**Battery pouch should include:**
- Pre-charged spare battery
- Extra XT60 connectors (2x)
- Battery voltage indicator chart
- Charging cable

---

## Cost Summary

| Component | Cost | Weight |
|-----------|------|--------|
| 1500mAh 3S LiPo | $25 | 160g |
| 3000mAh 3S LiPo | $40 | 230g |
| 3S BMS Module | $12 | 15g |
| Buck Regulator | $20 | 12g |
| INA219 Monitor | $8 | 8g |
| Connectors/wiring | $8 | 8g |
| Protective pouch | $5 | 30g |
| **Total (quick module)** | **$78** | **~245g** |
| **Total (extended module)** | **$113** | **~320g** |

---

## Next Steps

1. **Order components** - Buy batteries and power management modules
2. **Prepare firmware** - Add BatteryMonitor.h code to teensy_firmware.ino
3. **Assemble power stage** - Wire BMS, regulator, INA219
4. **Test power delivery** - Verify voltages at each stage
5. **Load firmware** - Upload modified firmware with battery monitoring
6. **Field test** - Spin poi and monitor battery readings

---

**Last Updated**: 2026-02-19
**Status**: Ready for implementation
**Questions?** Check `docs/README.md` for general setup guide
