# Wiring Diagram

## Complete System Wiring

```
┌─────────────────────────────────────────────────────────────────┐
│                        NEBULA POI                                │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────┐
│  POWER SUPPLY    │
│    5V 2-3A       │
└────┬────┬────────┘
     │    │
     │    └──────────────────────────┐
     │                               │
     │                               │
┌────▼──────────────┐         ┌─────▼──────────────┐
│   TEENSY 4.1      │         │  APA102 LED STRIP  │
│                   │         │     (32 LEDs)      │
│  Pin 11 (MOSI) ──┼────────►│ DATA (DI)          │
│  Pin 13 (SCK)  ──┼────────►│ CLOCK (CI)         │
│  GND ────────────┼────────►│ GND                │
│  VIN (5V) ───────┤         │ 5V ◄────[from PSU] │
│                   │         └────────────────────┘
│  Pin 1 (TX1) ────┼──┐
│  Pin 0 (RX1) ◄───┼──┼─┐
│                   │  │ │      ┌──────────────────┐
│  Pin A0 ◄────────┼──┼─┼─────►│  MAX9814 (Opt.)  │
│  3.3V ───────────┼──┼─┼─────►│  Microphone      │
│  GND ────────────┼──┼─┼─────►│  For Audio       │
└───────────────────┘  │ │      └──────────────────┘
                       │ │
                       │ │
┌──────────────────┐   │ │
│   ESP32/ESP32-S3 │   │ │
│   DEV MODULE     │   │ │.j
│                  │   │ │
│  GPIO17 (TX2) ───┼───┘ │
│  GPIO16 (RX2) ───┼─────┘
│  GND ────────────┤
│  5V (VIN/USB) ───┤
│                  │
└──────────────────┘
        │
        │ WiFi AP
        │ SSID: POV-POI-WiFi
        │ Pass: povpoi123
        │
        ▼
┌──────────────────┐
│  YOUR DEVICE     │
│  (Phone/Laptop)  │
│  Browser:        │
│  192.168.4.1     │
└──────────────────┘
```

**Note**: ESP32-S3 uses the same GPIO pins (16/17) as ESP32 - no wiring changes needed!  
See [ESP32-S3 Compatibility Guide](ESP32_S3_COMPATIBILITY.md) for details.

## Pin Connections Table

### Teensy 4.1 Pin Assignments

| Teensy Pin | Function | Connects To | Notes |
|------------|----------|-------------|-------|
| Pin 11 | SPI MOSI (Data) | APA102 DATA (DI) | LED data signal |
| Pin 13 | SPI SCK (Clock) | APA102 CLOCK (CI) | LED clock signal |
| Pin 0 | UART RX1 | ESP32 GPIO17 (TX2) | Serial receive from ESP32 |
| Pin 1 | UART TX1 | ESP32 GPIO16 (RX2) | Serial transmit to ESP32 |
| Pin A0 | Analog Input | MAX9814 OUT (optional) | Audio input for music-reactive patterns |
| 3.3V | Power Output | MAX9814 VCC (optional) | Power for microphone module |
| GND | Ground | Common Ground | Shared with ESP32 and LEDs |
| VIN | Power Input | 5V Power Supply | 5V input (or USB power) |

### ESP32 / ESP32-S3 Pin Assignments

Both ESP32 and ESP32-S3 use the same GPIO pins - **no wiring changes needed!**

| ESP32/S3 Pin | Function | Connects To | Notes |
|--------------|----------|-------------|-------|
| GPIO 16 | UART2 RX | Teensy Pin 1 (TX1) | Serial receive from Teensy |
| GPIO 17 | UART2 TX | Teensy Pin 0 (RX1) | Serial transmit to Teensy |
| GND | Ground | Common Ground | Shared with Teensy and power |
| VIN/5V | Power Input | 5V Power Supply | 5V input or USB power |

### APA102 LED Strip Connections

| APA102 Pin | Function | Connects To | Notes |
|------------|----------|-------------|-------|
| DI (Data In) | Data Signal | Teensy Pin 11 | LED data input |
| CI (Clock In) | Clock Signal | Teensy Pin 13 | LED clock input |
| 5V | Power | 5V Power Supply | Direct from power supply |
| GND | Ground | Common Ground | Shared ground |

## Detailed Connection Instructions

### Step 1: Power Supply Setup

1. **Select Appropriate Power Supply**
   - Voltage: 5V regulated
   - Current: Minimum 2A, recommended 3A
   - 32 LEDs at full brightness: ~1.9A
   - Add headroom for Teensy and ESP32: ~200-300mA
   
2. **Power Distribution**
   - Connect power supply GND to common ground rail
   - Connect power supply 5V to power rail
   - Add 1000µF capacitor across power lines near LED strip

### Step 2: Teensy 4.1 Connections

1. **LED Control Pins**
   ```
   Teensy Pin 11 → APA102 DI (DATA)
   Teensy Pin 13 → APA102 CI (CLOCK)
   ```
   - Use short, direct wires for data/clock
   - Keep wires away from power lines if possible
   - Maximum wire length: 6 inches recommended

2. **Serial Communication to ESP32**
   ```
   Teensy TX1 (Pin 1) → ESP32 RX2 (GPIO 16)
   Teensy RX1 (Pin 0) → ESP32 TX2 (GPIO 17)
   ```
   - Cross-connection: TX→RX, RX→TX
   - Both devices operate at 3.3V logic level (compatible)

3. **Power**
   ```
   Teensy VIN → 5V Power Supply (+)
   Teensy GND → Power Supply Ground (-)
   ```
   - Alternatively: Power via USB (testing only)

### Step 3: ESP32 or ESP32-S3 Connections

**Note**: ESP32-S3 uses the same pins as ESP32 (GPIO 16/17) - no wiring changes needed!

1. **Serial Communication to Teensy**
   ```
   ESP32/S3 RX2 (GPIO 16) → Teensy TX1 (Pin 1)
   ESP32/S3 TX2 (GPIO 17) → Teensy RX1 (Pin 0)
   ```

2. **Power**
   ```
   ESP32/S3 VIN → 5V Power Supply (+)
   ESP32/S3 GND → Power Supply Ground (-)
   ```
   - Alternatively: Power via USB (development/testing)

### Step 4: APA102 LED Strip

1. **Signal Connections**
   ```
   APA102 DI → Teensy Pin 11 (MOSI)
   APA102 CI → Teensy Pin 13 (SCK)
   ```
   - LED 0 can be used as level shifter if needed
   - LEDs 1-31 are used for display

2. **Power Connections**
   ```
   APA102 5V → Power Supply (+) [DIRECT CONNECTION]
   APA102 GND → Power Supply (-) [DIRECT CONNECTION]
   ```
   - **Important:** Connect LED power directly to supply
   - Do NOT power LEDs through Teensy
   - Use adequate wire gauge (22 AWG or thicker)

3. **Power Distribution**
   - For longer strips, consider power injection at both ends
   - Add capacitor (1000µF) near LED strip power input

## Ground Connection Diagram

**CRITICAL: All grounds must be connected together!**

```
Power Supply GND ──┬── Teensy GND
                   ├── ESP32/ESP32-S3 GND
                   └── LED Strip GND
```

## Optional Components

### Audio Input for Music Reactive Patterns

For music-reactive pattern modes (VU Meter, Pulse, Audio Rainbow, Center Burst, Audio Sparkle), you'll need to add a microphone module:

**Recommended Hardware: MAX9814 Microphone Module**

The MAX9814 is an ideal choice because it:
- Provides automatic gain control (AGC)
- Outputs 3.3V compatible analog signal
- Has built-in amplification (40-60dB gain)
- Low noise floor
- Frequency response: 20Hz - 20kHz

**Wiring:**
```
┌──────────────────┐
│   MAX9814        │
│   Microphone     │
│                  │
│  VCC ────────────┼──── 3.3V (Teensy 3.3V output)
│  GND ────────────┼──── GND (Common ground)
│  OUT ────────────┼──── Teensy Pin A0 (Analog input)
│  GAIN ───────────┼──── Leave floating or connect to GND/VCC
│  AR (Attack/Rel) │      (see module datasheet for gain selection)
└──────────────────┘
```

**Pin Connections:**

| MAX9814 Pin | Function | Connects To | Notes |
|-------------|----------|-------------|-------|
| VCC | Power | Teensy 3.3V pin | **Use 3.3V, NOT 5V** |
| GND | Ground | Common Ground | Shared ground rail |
| OUT | Audio Signal | Teensy Pin A0 | Analog audio output |
| GAIN | Gain Select | Float/GND/VCC | Optional: Controls mic sensitivity |
| AR | Attack/Release | Optional | Optional: Adjust AGC response |

**Alternative Microphone Options:**
- **Electret Microphone with Amplifier** - Cheaper but requires manual gain tuning
- **MEMS Microphone (I2S)** - Digital option, requires code changes
- **Sparkfun Sound Detector** - Similar to MAX9814, also works well

**Configuration Notes:**
1. Position microphone to face sound source
2. Avoid placing near spinning components (mechanical noise)
3. Calibrate sensitivity by testing patterns at different volumes
4. The analog input range is 0-3.3V (Teensy ADC reads 0-1023)
5. Audio patterns sample at high frequency (~1kHz) for responsiveness

**Purchasing:**
- Adafruit: MAX9814 Microphone Amplifier Module
- Amazon/AliExpress: "MAX9814 microphone module"
- SparkFun: Sound Detector module

### Logic Level Shifter
If you experience signal issues:
```
Teensy 3.3V ──┬── Shifter LV
              └── (3.3V side)

Power 5V ─────┬── Shifter HV
              └── (5V side)

Teensy Pin 11 ─── Shifter LV1 ~~> HV1 ─── LED DI
Teensy Pin 13 ─── Shifter LV2 ~~> HV2 ─── LED CI
```

### Capacitor Placement
```
LED Strip Power Input:
   5V ──┬─── [1000µF] ───┬─── GND
        │                 │
        └────── LED ──────┘
```

### Current Measurement Points
For monitoring power consumption:
```
Power Supply (+) ───[Ammeter]─── 5V Rail
```

## Physical Assembly Considerations

### For Spinning POI

1. **Weight Distribution**
   - Mount Teensy and ESP32 together
   - Balance weight around rotation axis
   - Use counterweight if necessary

2. **Secure Mounting**
   - Use hot glue or mounting brackets
   - Ensure no loose wires
   - Protect components from impacts

3. **Wire Management**
   - Use flexible wires for spinning applications
   - Secure wires to prevent tangling
   - Consider strain relief at connection points

4. **Power Options for Portable**
   - USB power bank (5V output)
   - LiPo battery with 5V boost converter
   - 18650 cells (3.7V) with boost to 5V

## Testing Procedure

### Step-by-Step Testing

1. **Visual Inspection**
   - Check all connections
   - Verify no shorts between power and ground
   - Ensure proper polarity

2. **Power Test (No LEDs)**
   - Connect only Teensy and ESP32
   - Power on and check for:
     - Teensy LED indicators
     - ESP32 power LED
     - No smoke or overheating

3. **Serial Communication Test**
   - Upload firmware to both devices
   - Monitor serial outputs
   - Verify communication between devices

4. **LED Test (Low Brightness)**
   - Connect LED strip
   - Set brightness to 25% (64/255)
   - Test basic patterns
   - Monitor current draw

5. **Full System Test**
   - Gradually increase brightness
   - Test all patterns and modes
   - Connect via WiFi
   - Test web interface

## Troubleshooting Wiring Issues

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| No LEDs light up | No power to LEDs | Check 5V connection to strip |
| | Wrong data/clock pins | Verify Pin 11 and 13 connections |
| | Level shifting issue | Try adding level shifter |
| Erratic LED behavior | Poor ground connection | Ensure common ground |
| | Signal interference | Shorten data wires, add capacitor |
| | Insufficient power | Upgrade power supply |
| ESP32 won't boot | Low voltage | Check power supply voltage |
| | Brown-out | Add bulk capacitor near ESP32 |
| No serial communication | Wrong RX/TX connections | Verify crossover (TX→RX) |
| | Baud rate mismatch | Confirm 115200 baud both sides |
| | Ground not shared | Connect grounds together |
| High current draw | LEDs at full brightness | Reduce brightness setting |
| | Short circuit | Check for crossed wires |
| Voltage drop | Wire too thin | Use thicker gauge wire |
| | Wire too long | Shorten power wire runs |

## Safety Checklist

- [ ] All connections verified before power on
- [ ] No exposed conductors (solder/cover all connections)
- [ ] Adequate power supply capacity
- [ ] Proper wire gauges used
- [ ] Capacitors installed at LED power input
- [ ] All grounds connected together
- [ ] No shorts between power and ground
- [ ] Components securely mounted
- [ ] Spinning assembly balanced
- [ ] Safe operating environment

## Wire Gauge Recommendations

| Connection | Distance | Wire Gauge |
|------------|----------|------------|
| Power Supply to LEDs | < 1 foot | 22 AWG |
| Power Supply to LEDs | 1-3 feet | 20 AWG |
| LED Data/Clock | < 6 inches | 24-26 AWG |
| Serial (TX/RX) | < 1 foot | 26-28 AWG |
| Ground connections | Any | Match power wire |

## Component Suppliers

Recommended sources for components:
- **Teensy 4.1**: PJRC.com
- **ESP32**: Adafruit, SparkFun, AliExpress
- **ESP32-S3 N16R8** ⭐ (Recommended): Adafruit, SparkFun, AliExpress, Amazon
- **APA102 LEDs**: Adafruit, BTF-Lighting, AliExpress
- **Power Supplies**: Mean Well, Amazon
- **Level Shifters**: Adafruit, SparkFun

---

**Note**: Always double-check connections before applying power. Test incrementally to identify issues early. When in doubt, measure voltages with a multimeter.
