# Hardware Specifications

**Complete hardware reference for Nebula POI system**

---

## Component Overview

### Main Controller: Teensy 4.1

**Specifications:**
- **MCU:** NXP i.MX RT1062 (ARM Cortex-M7)
- **Clock Speed:** 600 MHz
- **Flash:** 8 MB (onboard)
- **RAM:** 1024 KB
- **PSRAM:** Optional 8MB (not required for this project)
- **USB:** Native USB 2.0 (480 Mbit/sec)
- **SD Card:** Built-in microSD card socket
- **GPIO:** 55 digital pins

**Why Teensy 4.1?**
- Fast enough for real-time POV rendering (600 MHz)
- Hardware SPI for APA102 control
- Sufficient RAM for frame buffers
- FastLED library support
- Built-in SD card for image storage

**Used Pins:**
- Pin 0 (RX1): Serial receive from ESP32
- Pin 1 (TX1): Serial transmit to ESP32  
- Pin 11: APA102 Data (MOSI)
- Pin 13: APA102 Clock (SCK)

---

## WiFi Module: ESP32-S3

### ESP32-S3 N16R8 (Recommended)

**Specifications:**
- **MCU:** Xtensa dual-core LX7
- **Clock Speed:** 240 MHz (per core)
- **Flash:** 16 MB
- **PSRAM:** 8 MB
- **WiFi:** 802.11 b/g/n (2.4 GHz)
- **Bluetooth:** BLE 5.0
- **USB:** Native USB (OTG)

**Why ESP32-S3?**
- Large flash (16MB) for web UI assets
- PSRAM for image buffering
- Dual-core for parallel WiFi + BLE
- Better than ESP32 classic for this use case

**Pin Configuration:**
- GPIO 16 (RX2): Serial receive from Teensy
- GPIO 17 (TX2): Serial transmit to Teensy
- GPIO 18: Optional status LED

### ESP32 Classic (Supported but not recommended)

**Specifications:**
- **Flash:** 4 MB (typical)
- **PSRAM:** Not standard
- **Still works** but less headroom for features

**See:** `ESP32_S3_PURCHASE_GUIDE.md` for buying recommendations

**Used Pins:**
- GPIO 16 (RX2): Serial from Teensy
- GPIO 17 (TX2): Serial to Teensy

---

## LED Strip: APA102

### Specification

**Type:** APA102-C (Dotstar compatible)
- **Count:** 32 LEDs
- **Display:** 32 LEDs (LED 0-31)
- **Level Shifter:** Hardware level shifter (3.3V -> 5V)

**Electrical:**
- **Voltage:** 5V DC
- **Current per LED:** ~60mA @ full white
- **Total max current:** 32 × 60mA = 1.92A
- **Typical current:** ~1A (mixed colors, <50% brightness)

**Protocol:**
- **Type:** SPI-like (2-wire)
- **Clock:** Independent of data
- **Speed:** Up to 20 MHz (we use ~1 MHz)
- **Timing:** Not critical (unlike WS2812)

**Advantages over WS2812:**
- Clock signal = precise timing
- Less sensitive to voltage drop
- Higher refresh rates possible
- Better for POV applications

**Physical:**
- **Pitch:** 16.7mm per LED (60 LEDs/meter)
- **Total length:** 32 × 16.7mm = 534mm (~21 inches)
- **Width:** 10-12mm typical
- **IP rating:** IP30 (non-weatherproof) to IP67 (silicone coating)

---

## Optional: MAX9814 Microphone

**Purpose:** Music-reactive patterns (11-15)

**Specifications:**
- **Type:** Electret microphone with AGC
- **Gain:** Auto-adjusting (40-60dB)
- **Output:** Analog (0-3.3V)
- **Supply:** 2.7V - 5.5V
- **Frequency Response:** 20Hz - 20kHz

**Module Pin Layout (5 pins):**

With the microphone capsule facing **up**, pins run **left to right**: AR → OUT → GAIN → VCC → GND.

| Pin # | Name | Connects To | Notes |
|-------|------|-------------|-------|
| 1 | **AR** | Leave floating (recommended) | Attack/Release for AGC timing. Unconnected = default (~10 ms attack, ~500 ms release) |
| 2 | **OUT** | Teensy **Pin A0** | Analog audio signal output |
| 3 | **GAIN** | Leave floating (60 dB default) | Float = 60 dB, GND = 50 dB, VCC = 40 dB |
| 4 | **VCC** | Teensy **3.3V** | ⚠️ 3.3V only — never connect to 5V |
| 5 | **GND** | Common Ground | Shared ground rail |

**Minimum required connections (3 wires):**
```
MAX9814 OUT  →  Teensy A0
MAX9814 VCC  →  Teensy 3.3V
MAX9814 GND  →  Teensy GND
```
AR and GAIN can both be left unconnected for a fully working default setup.

**Not Required If:**
- You only use basic patterns (0-10)
- Music-reactive patterns not needed

---

## Power Supply Requirements

### Minimum Configuration
- **Voltage:** 5V DC (±5%)
- **Current:** 2A minimum
- **Connector:** Barrel jack or USB-C (depending on build)

### Recommended Configuration
- **Voltage:** 5V DC
- **Current:** 3A (headroom for full brightness)
- **Type:** Regulated switching supply
- **Protection:** Over-current, short-circuit

### Current Budget

| Component | Idle | Typical | Max |
|-----------|------|---------|-----|
| Teensy 4.1 | 100mA | 150mA | 200mA |
| ESP32-S3 | 80mA | 150mA | 250mA |
| APA102 (32 LEDs) | 32mA | 500mA | 1920mA |
| **Total** | **212mA** | **800mA** | **2370mA** |

**Recommendations:**
- **Development:** USB power (500mA) OK for low brightness
- **Testing:** 2A supply minimum
- **Production:** 3A supply for full brightness headroom

---

## Wiring Specifications

### Serial Communication (Teensy ↔ ESP32)

**Protocol:** UART
- **Baud Rate:** 115200 bps
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None

**Connections:**
```
Teensy TX1 (Pin 1)  →  ESP32 RX2 (GPIO 16)
Teensy RX1 (Pin 0)  ←  ESP32 TX2 (GPIO 17)
Common Ground       ←→ Common Ground
```

**Voltage Levels:**
- Teensy: 3.3V logic (5V tolerant on most pins)
- ESP32: 3.3V logic (NOT 5V tolerant)
- **Direct connection OK** (both 3.3V)

### LED Strip (Teensy → APA102)

**Protocol:** SPI (software or hardware)
- **Clock Frequency:** ~1 MHz (adjustable)
- **Data Format:** 32-bit per LED (start frame + RGB + brightness)

**Connections:**
```
Teensy Pin 11 (MOSI) → Level Shifter IN → Level Shifter OUT → APA102 DI (Data In)
Teensy Pin 13 (SCK)  → Level Shifter IN → Level Shifter OUT → APA102 CI (Clock In)
5V Power             → APA102 5V
Common Ground        → APA102 GND
```

**Level Shifting:**
- Hardware level shifter used (e.g., 74AHCT125)
- Teensy 3.3V -> Level Shifter -> 5V output to LED strip
- All 32 LEDs used for display

---

## Physical Build Considerations

### POI Handle Design

**Grip Area:**
- Length: 10-15cm minimum
- Diameter: 25-30mm comfortable
- Material: PVC pipe, 3D printed, turned wood

**Component Housing:**
- Teensy + ESP32: Near handle (balanced)
- Battery: Handle or counterweight
- LED strip: Extended along poi length

**Weight Distribution:**
- Target: 150-250g total
- Balance point: At grip for easy spinning
- Counterweight if needed

### Enclosure Requirements

**Protection Level:**
- IP40 minimum (dust protection)
- IP65 recommended (water resistant)
- Consider: Dropping, spinning forces, weather

**Material Options:**
- PLA/PETG: 3D printed, lightweight
- Polycarbonate: Impact resistant, clear
- Aluminum: Heat dissipation, durable

**Ventilation:**
- Teensy needs minimal cooling
- ESP32 can get warm (WiFi active)
- Small vent holes if enclosed

---

## Assembly Checklist

### Before Powering On

- [ ] All ground connections common
- [ ] 5V power NOT connected to 3.3V pins
- [ ] Serial TX → RX, RX → TX (crossover)
- [ ] APA102 data/clock correct polarity
- [ ] No short circuits between power/ground
- [ ] Firmware uploaded to both MCUs

### First Power-On Test

1. Connect power (2A minimum supply)
2. Check Teensy LED (should blink if programmed)
3. Check ESP32 WiFi (SSID "POV-POI-WiFi" appears)
4. Connect to WiFi, navigate to 192.168.4.1
5. Test basic pattern (Rainbow = pattern 0)
6. Verify LEDs light up

---

## Bill of Materials (BOM)

### Essential Components

| Part | Spec | Qty | Est. Cost |
|------|------|-----|-----------|
| Teensy 4.1 | With headers | 1 | $30 |
| ESP32-S3 N16R8 | DevKit | 1 | $10-15 |
| APA102 Strip | 32 LEDs, 60/m | 1 | $15-20 |
| Power Supply | 5V 3A | 1 | $8-12 |
| Wires | 22-24 AWG | Set | $5 |

**Total Essential:** ~$70-85

### Optional Components

| Part | Purpose | Est. Cost |
|------|---------|-----------|
| MAX9814 Mic | Music-reactive | $8 |
| MicroSD Card | Image storage | $5 |
| 3D Printed Case | Enclosure | $10 (filament) |
| Battery Pack | Portable power | $20-30 |

---

## Troubleshooting Hardware

### LEDs Not Lighting
- Check 5V power to strip
- Verify data/clock pins (11, 13)
- Test with low brightness first
- Measure voltage at LED strip (should be ~5V)

### Serial Communication Failure
- Verify baud rate (115200)
- Check TX/RX crossover
- Confirm common ground
- Monitor with serial terminal on both ends

### WiFi Not Appearing
- ESP32 powered? (check 3.3V regulator if separate)
- Firmware uploaded correctly?
- Wait 10-15 seconds after power-on
- Check antenna connection (if external)

---

## Safety Notes

⚠️ **Electrical Safety**
- Use properly rated power supply
- Avoid exposing electronics to water
- Check for shorts before applying power
- Do not exceed LED current ratings

⚠️ **Mechanical Safety**
- Secure all components (centrifugal force when spinning)
- Balance poi to avoid wrist strain
- Use wrist straps or lanyards
- Be aware of surroundings when spinning

⚠️ **Optical Safety**
- Avoid looking directly at LEDs at full brightness
- Use lower brightness indoors
- Consider diffuser for strip

---

## References

- [Teensy 4.1 Datasheet](https://www.pjrc.com/store/teensy41.html)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [APA102 Protocol](https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/)
- [FastLED Library](https://github.com/FastLED/FastLED)

---

**For wiring diagrams, see:** `WIRING.md`
**For troubleshooting, see:** `TROUBLESHOOTING.md`
