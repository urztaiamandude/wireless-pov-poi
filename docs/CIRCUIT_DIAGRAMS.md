# Circuit Diagrams and Wiring Guide

A comprehensive visual guide for wiring the Nebula Poi with Teensy 4.1, ESP32, and APA102 LED strip.

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Component Pinout Diagrams](#component-pinout-diagrams)
3. [Complete Wiring Diagrams](#complete-wiring-diagrams)
4. [Connection Matrix](#connection-matrix)
5. [Step-by-Step Wiring Guide](#step-by-step-wiring-guide)
6. [Breadboard Layouts](#breadboard-layouts)
7. [Testing Procedures](#testing-procedures)
8. [Common Wiring Mistakes](#common-wiring-mistakes)
9. [3D Assembly Views](#3d-assembly-views)
10. [Tools and Equipment](#tools-and-equipment)
11. [Bill of Materials](#bill-of-materials)

---

## System Architecture

```
┌────────────────────────────────────────────────────────────────────┐
│                     NEBULA POI OVERVIEW                          │
└────────────────────────────────────────────────────────────────────┘

                        USER DEVICE
                    ┌─────────────────┐
                    │  Phone/Tablet   │
                    │    Browser      │
                    └────────┬────────┘
                             │
                        WiFi │ 2.4GHz
                             │ 192.168.4.1
                             │
                    ┌────────▼────────┐
                    │  ESP32/ESP32-S3 │
                    │   (All Variants)│
                    │   WiFi Module   │
                    │   Web Server    │
                    │   REST API      │
                    └────────┬────────┘
                             │
               Serial UART   │ 115200 baud
                   (TX/RX)   │ GPIO 16/17
                             │ (All Variants)
                             │
                    ┌────────▼────────┐
                    │   Teensy 4.1    │
                    │  POV Controller │
                    │  600 MHz ARM    │
                    │   FastLED       │
                    └────────┬────────┘
                             │
                  SPI Bus    │ Pin 11/13
               (Data/Clock)  │ 20 MHz capable
                             │
                    ┌────────▼────────┐
                    │  APA102 LEDs    │
                    │   32 RGB LEDs   │
                    │  (all 32 for    │
                    │   display)      │
                    └─────────────────┘
```

### Signal Flow

```
User Input → Web Interface → HTTP Request → ESP32 Web Server
    ↓
ESP32 parses command → Serial Protocol → Teensy 4.1
    ↓
Teensy processes command → Updates LED buffer
    ↓
FastLED library → SPI signals → APA102 LED strip
    ↓
Visual POV Display ✨
```

---

## Component Pinout Diagrams

### Teensy 4.1 Pinout (Top View)

```
                        ┌──────────────────┐
                        │   Teensy 4.1     │
                        │                  │
    GND ──────┤ GND              VIN ├────── 5V IN
      0 ──────┤ RX1    (USB)     GND ├────── GND
      1 ──────┤ TX1             3.3V ├────── 3.3V OUT
      2 ──────┤ 2                 23 ├────── 23
      3 ──────┤ 3                 22 ├────── 22
      4 ──────┤ 4                 21 ├────── 21
      5 ──────┤ 5                 20 ├────── 20
      6 ──────┤ 6                 19 ├────── 19
      7 ──────┤ 7                 18 ├────── 18
      8 ──────┤ 8                 17 ├────── 17
      9 ──────┤ 9                 16 ├────── 16
     10 ──────┤ 10                15 ├────── 15
     11 ──────┤ 11/MOSI          14 ├────── 14
     12 ──────┤ 12/MISO          13 ├────── 13/SCK
    GND ──────┤ GND              GND ├────── GND
                        │                  │
                        └──────────────────┘

    ⭐ Key Pins Used in This Project:
    • Pin 0 (RX1)  - Serial receive from ESP32
    • Pin 1 (TX1)  - Serial transmit to ESP32
    • Pin 11 (MOSI) - APA102 Data signal
    • Pin 13 (SCK)  - APA102 Clock signal
    • VIN          - 5V power input
    • GND          - Ground (connect to all)
```

### ESP32 Development Board Pinout (Top View)

**Note**: This pinout applies to all ESP32 variants (WROOM-32, DevKitC, ESP32-S3).
GPIO 16/17 are available on all variants with the same wiring.

```
                    ┌────────────────────────┐
                    │  ESP32 DEV MODULE      │
                    │  (WROOM/DevKit/S3)     │
                    │                        │
    EN ─────┤ EN           (USB)       D23 ├───── 23
   VP/36 ───┤ VP                       D22 ├───── 22
   VN/39 ───┤ VN                       TX0 ├───── 1
   D34 ─────┤ 34                       RX0 ├───── 3
   D35 ─────┤ 35                       D21 ├───── 21
   D32 ─────┤ 32                       GND ├───── GND
   D33 ─────┤ 33                       D19 ├───── 19
   D25 ─────┤ 25                       D18 ├───── 18
   D26 ─────┤ 26                       D5  ├───── 5
   D27 ─────┤ 27                       D17 ├───── 17
   D14 ─────┤ 14                       D16 ├───── 16
   D12 ─────┤ 12                       D4  ├───── 4
   GND ─────┤ GND                      D0  ├───── 0
   D13 ─────┤ 13                       D2  ├───── 2
    D9 ─────┤ 9/SD2                    D15 ├───── 15
   D10 ─────┤ 10/SD3                   D8  ├───── 8/SD1
   D11 ─────┤ 11/CMD                   D7  ├───── 7/SD0
   VIN ─────┤ VIN                      D6  ├───── 6/CLK
   GND ─────┤ GND                      GND ├───── GND
                    │                        │
                    └────────────────────────┘

    ⭐ Key Pins Used in This Project (All ESP32 Variants):
    • GPIO 16 (RX2) - Serial receive from Teensy
    • GPIO 17 (TX2) - Serial transmit to Teensy
    • VIN           - 5V power input
    • GND           - Ground (connect to all)
```

### APA102 LED Strip Connection Points

```
    ┌──────────────────────────────────────────────┐
    │         APA102 RGB LED STRIP (32 LEDs)       │
    └──────────────────────────────────────────────┘

    INPUT END (Connect to Controller):
    ┌────┬────┬────┬────┐
    │ 5V │ CI │ DI │GND │
    └─┬──┴─┬──┴─┬──┴─┬──┘
      │    │    │    │
      │    │    │    └── Ground (Black)
      │    │    └─────── Data In (Green/Yellow)
      │    └──────────── Clock In (Blue/White)
      └───────────────── +5V Power (Red)

    OUTPUT END (Can chain to more LEDs):
    ┌────┬────┬────┬────┐
    │ 5V │ CO │ DO │GND │
    └────┴────┴────┴────┘

    LED Layout (hardware level shifter used):
    [LED 0] → [LED 1] → [LED 2] → ... → [LED 31]
       ↑─────────────────────────────────────↑
              All 32 LEDs for Display
```

---

## Complete Wiring Diagrams

### Master Wiring Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                  COMPLETE SYSTEM WIRING DIAGRAM                      │
└─────────────────────────────────────────────────────────────────────┘

    ┌────────────────────┐
    │  5V POWER SUPPLY   │
    │     2-3 Amp        │
    │  (Wall Adapter or  │
    │   Battery Pack)    │
    └──────┬─────┬───────┘
           │     │
       +5V │     │ GND
           │     │
    ┏━━━━━┷━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
    ┃        POWER & GROUND DISTRIBUTION       ┃
    ┃        (Common Bus/Breadboard Rails)     ┃
    ┗━━━━━┯━━━━━┯━━━━━┯━━━━━━━━━━━━━━━━━━━━━━┛
          │     │     │
          │     │     └────────────┐
          │     │                  │
          │     └──────────┐       │
          │                │       │
    ┌─────▼──────┐  ┌──────▼────┐ │  ┌────────▼─────────┐
    │  Teensy    │  │   ESP32   │ │  │  APA102 Strip    │
    │   4.1      │  │  DevKit   │ │  │   (32 LEDs)      │
    └────────────┘  └───────────┘ │  └──────────────────┘
         │  │             │  │    │         │  │
         │  │             │  │    │         │  │
    Pin  │  │ Pin    GPIO │  │    │     DI  │  │ CI
    11   │  │ 13      17  │  │ 16 │   (Data)│  │(Clock)
    MOSI │  │ SCK     TX2 │  │ RX2│         │  │
         │  │             │  │    │         │  │
         │  └─────────────┼──┼────┼─────────┘  │
         │                │  │    │            │
         └────────────────┼──┼────┼────────────┘
         TX1          RX1 │  │    │
         Pin 1        Pin 0  │    │
                            │    │
    ┌───────────────────────┴────┴────────────────────────┐
    │  Serial Communication (Cross-connected)              │
    │  Teensy TX1 (Pin 1)  →  ESP32 RX2 (GPIO 16)         │
    │  Teensy RX1 (Pin 0)  ←  ESP32 TX2 (GPIO 17)         │
    └──────────────────────────────────────────────────────┘

    Legend:
    ━━━  Power/Ground rails
    ───  Signal wires
    →    Data flow direction
```

### Detailed Connection Diagram with Wire Colors

```
POWER SUPPLY (5V 3A)
    │
    ├─── RED ──────┬───► Teensy VIN
    │              ├───► ESP32 VIN
    │              └───► LED Strip 5V
    │
    └─── BLACK ────┬───► Teensy GND
                   ├───► ESP32 GND
                   └───► LED Strip GND

TEENSY 4.1
    Pin 11 ─── GREEN ────► APA102 DI (Data In)
    Pin 13 ─── YELLOW ───► APA102 CI (Clock In)
    Pin 1  ─── BLUE ─────► ESP32 GPIO 16 (RX2)
    Pin 0  ◄─── ORANGE ── ESP32 GPIO 17 (TX2)

ESP32
    GPIO 16 ◄─── BLUE ─── Teensy Pin 1 (TX1)
    GPIO 17 ─── ORANGE ──► Teensy Pin 0 (RX1)
```

---

## Connection Matrix

### Pin-to-Pin Connection Table

| Source Device | Source Pin | Wire Color | Destination Device | Destination Pin | Signal Type |
|--------------|------------|------------|-------------------|----------------|-------------|
| Power Supply | +5V | Red | Teensy 4.1 | VIN | Power |
| Power Supply | +5V | Red | ESP32 | VIN | Power |
| Power Supply | +5V | Red | LED Strip | 5V | Power |
| Power Supply | GND | Black | Teensy 4.1 | GND | Ground |
| Power Supply | GND | Black | ESP32 | GND | Ground |
| Power Supply | GND | Black | LED Strip | GND | Ground |
| Teensy 4.1 | Pin 11 (MOSI) | Green | LED Strip | DI | SPI Data |
| Teensy 4.1 | Pin 13 (SCK) | Yellow | LED Strip | CI | SPI Clock |
| Teensy 4.1 | Pin 1 (TX1) | Blue | ESP32 | GPIO 16 (RX2) | UART TX |
| ESP32 | GPIO 17 (TX2) | Orange | Teensy 4.1 | Pin 0 (RX1) | UART RX |

### Connection Summary by Component

#### Teensy 4.1 Connections
- **Power**: VIN (5V), GND
- **LED Control**: Pin 11 → LED DI, Pin 13 → LED CI
- **Serial**: Pin 1 → ESP32 RX, Pin 0 ← ESP32 TX
- **Total wires**: 6 (2 power + 2 LED + 2 serial)

#### ESP32 Connections
- **Power**: VIN (5V), GND
- **Serial**: GPIO 16 ← Teensy TX, GPIO 17 → Teensy RX
- **Total wires**: 4 (2 power + 2 serial)

#### APA102 LED Strip Connections
- **Power**: 5V, GND (direct from supply)
- **Control**: DI ← Teensy Pin 11, CI ← Teensy Pin 13
- **Total wires**: 4 (2 power + 2 control)

#### MAX9814 Microphone (Optional)

The MAX9814 module has **5 pins**. Only 3 wires are needed for basic use; GAIN and AR can be left unconnected.

```
MAX9814 pin layout (typical module):
  AR   ─── leave floating (default AGC timing)
  OUT  ─── Teensy Pin A0  (WHITE wire)
  GND  ─── Common Ground  (BLACK wire)  ← may be labelled "0V" or unlabelled
  GAIN ─── leave floating (60 dB default gain)
  VCC  ─── Teensy 3.3V    (RED wire)   ← ⚠️ 3.3V only, never 5V
```

| MAX9814 Pin | Connects To | Wire Color | Notes |
|-------------|-------------|------------|-------|
| VCC | Teensy 3.3V | Red | ⚠️ 3.3V only |
| GND | Common Ground | Black | May read "0V" on some modules |
| OUT | Teensy A0 | White | Audio signal |
| GAIN | (unconnected) | — | Float = 60 dB default |
| AR | (unconnected) | — | Float = default AGC timing |

---

## Step-by-Step Wiring Guide

### Phase 1: Power Distribution Setup
1. Connect power supply +5V to breadboard RED rail
2. Connect power supply GND to breadboard BLACK rail
3. Place 1000µF capacitor across power rails
4. Verify voltage: 4.8V - 5.2V using multimeter

### Phase 2: Teensy 4.1 Installation
1. Mount Teensy on breadboard
2. Connect VIN to +5V rail (Red wire)
3. Connect GND to GND rail (Black wire)
4. Verify power (LED lights)

### Phase 3: ESP32 Installation
1. Mount ESP32 on breadboard
2. Connect VIN to +5V rail (Red wire)
3. Connect GND to GND rail (Black wire)
4. Verify power (LED lights)

### Phase 4: Serial Communication Wiring
1. Connect Teensy Pin 1 (TX1) → ESP32 GPIO 16 (RX2) with Blue wire
2. Connect ESP32 GPIO 17 (TX2) → Teensy Pin 0 (RX1) with Orange wire
3. Verify crossover: TX→RX, RX→TX

### Phase 5: LED Strip Connection
1. Identify LED strip INPUT end
2. Connect LED 5V directly to power supply +5V (Red wire, 20-22 AWG)
3. Connect LED GND directly to power supply GND (Black wire, 20-22 AWG)
4. Connect Teensy Pin 11 → LED DI (Green wire)
5. Connect Teensy Pin 13 → LED CI (Yellow wire)
6. Add 1000µF capacitor at LED power input

### Phase 6: Final Inspection Checklist
- [ ] All power connections correct polarity
- [ ] No shorts between power and ground
- [ ] Serial crossover correct (TX→RX)
- [ ] LED strip powered directly from supply
- [ ] All grounds connected together
- [ ] Capacitors installed
- [ ] All connections secure
- [ ] No exposed conductors

---

## Breadboard Layouts

Refer to diagrams above for breadboard arrangement showing Teensy 4.1, ESP32, and LED connections on a single 830-point breadboard.

---

## Testing Procedures

### Pre-Power Testing (CRITICAL)
1. **Ground Continuity**: All GNDs must beep on multimeter
2. **No Power Shorts**: +5V to GND must NOT beep
3. **Voltage Check**: Verify 4.8-5.2V at power supply
4. **Signal Isolation**: Data/clock wires should NOT connect to power

### Initial Power-On (Controllers Only)
1. Disconnect LED strip first
2. Apply power - check for LEDs lighting on Teensy/ESP32
3. Measure voltages at each component (4.8-5.2V)
4. Current draw should be 180-300mA idle

### Serial Communication Test
1. Program both devices with firmware
2. Monitor Teensy serial: "Nebula Poi Initialized"
3. Monitor ESP32 serial: "WiFi AP started"
4. Verify communication working

### LED Strip Test (Low Power)
1. Set firmware brightness to 10
2. Connect LED strip
3. Power on - LEDs should light (dim)
4. Test patterns: Rainbow, Wave, etc.

### Full System Test
1. Connect to WiFi: "POV-POI-WiFi"
2. Open browser: http://192.168.4.1
3. Test brightness control
4. Test pattern changes
5. Gradually increase brightness to full

---

## Common Wiring Mistakes

1. **TX-to-TX Connection** - Must be TX→RX crossover
2. **Powering LEDs Through Teensy** - Connect LEDs directly to power supply
3. **Missing Common Ground** - All GNDs must be connected
4. **Reversed LED Polarity** - Check 5V and GND carefully
5. **Wrong Teensy Pins** - Use Pin 11 (data), Pin 13 (clock)
6. **Insufficient Power** - Need 2-3A minimum supply
7. **No Capacitors** - Add 1000µF at power inputs
8. **Swapped Data/Clock** - Pin 11→DI, Pin 13→CI
9. **Wrong LED End** - Use INPUT end, not OUTPUT
10. **Long Signal Wires** - Keep data/clock under 12 inches

---

## 3D Assembly Views

Spinning POI assemblies typically have:
- Control module (Teensy + ESP32 + Battery) at center
- LED strip extending outward
- Counterweight for balance
- Handle/grip for spinning

---

## Tools and Equipment

### Essential Tools
- [ ] Breadboard (830 tie-points)
- [ ] Wire strippers (22-28 AWG)
- [ ] Wire cutters
- [ ] Digital multimeter (DMM) **ESSENTIAL**
- [ ] Soldering iron (for permanent builds)
- [ ] USB cables (Micro USB for ESP32)

### Recommended Wire
- Power: 22 AWG Red/Black
- Data/Clock: 22-24 AWG Green/Yellow
- Serial: 24-26 AWG Blue/Orange

### Virtual Breadboard Tools
- **Fritzing** - fritzing.org
- **TinkerCAD Circuits** - tinkercad.com/circuits  
- **Wokwi** - wokwi.com
- **KiCad** - kicad.org (for PCB design)

---

## Bill of Materials

### Core Components
| Item | Qty | Cost | Source |
|------|-----|------|--------|
| Teensy 4.1 | 1 | $26.85 | PJRC.com |
| ESP32 Dev Board | 1 | $8-12 | Amazon |
| APA102 LED Strip (32) | 1 | $12-20 | Adafruit |
| 5V 3A Power Supply | 1 | $8-15 | Amazon |
| Breadboard | 1 | $6-10 | Amazon |
| USB Cables | 2 | $6-10 | Amazon |

**Core Total: ~$70-95**

### Wiring & Components
| Item | Qty | Cost | Source |
|------|-----|------|--------|
| Jumper Wire Kit | 1 | $6-10 | Amazon |
| 22 AWG Wire (Red/Black) | 6 ft | $4-8 | Amazon |
| 1000µF Capacitors | 2 | $1-2 | Adafruit |
| Heat Shrink Tubing | 1 kit | $8-12 | Amazon |

**Wiring Total: ~$20-35**

### Tools (if needed)
| Item | Cost | Source |
|------|------|--------|
| Digital Multimeter | $15-30 | Amazon |
| Wire Strippers | $8-15 | Amazon |
| Soldering Iron | $20-40 | Amazon |

**Tools Total: ~$45-85**

### Project Cost Summary
- **Minimum** (have tools): $90-130
- **Complete** (need tools): $135-215

---

## Next Steps

After completing wiring:
1. ✓ **Power supply design**: See [POWER_SUPPLY_DESIGN.md](POWER_SUPPLY_DESIGN.md)
2. ✓ **Upload firmware**: Follow [README.md](../README.md)
3. ✓ **Test web interface**: http://192.168.4.1
4. ✓ **Optimize setup**: Portable vs stationary

## Additional Resources
- [Main Documentation](README.md)
- [Wiring Quick Reference](WIRING.md)
- [API Reference](API.md)
- [Troubleshooting](../TROUBLESHOOTING.md)

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Applies to**: Nebula Poi v1.0+

---

**Happy Building! 🔧✨**
