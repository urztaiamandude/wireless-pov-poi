# Battery Power System - Implementation Roadmap

**Status**: ✅ All components ready for implementation
**Last Updated**: 2026-02-19
**Estimated Build Time**: 8-12 hours (over 2-3 days)

---

## 📦 What You're Building

A **modular, battery-powered wireless POV poi system** with real-time battery monitoring via web interface.

```
Quick Sessions Module (1500mAh, 45-60 min)  ← Swap batteries in < 30 seconds
Extended Sessions Module (3000mAh, 2-3 hours)
                    ↓
         Modular Battery Pouch
  (BMS + Buck Regulator + INA219)
                    ↓
              XT60 Connector
                    ↓
        Teensy 4.1 + ESP32-S3
            + 32 LED Strip
                    ↓
        Web Dashboard (on phone)
        Shows real-time battery status
```

**Total system weight**: 250-350g (light, hand-spun)
**Total cost**: $110-150 per poi
**Runtime**: 45 min to 3 hours depending on battery module

---

## 🛒 Phase 1: Order Components (1-2 days)

### Battery Modules

| Item | Qty | Cost | Vendor | Notes |
|------|-----|------|--------|-------|
| 3S LiPo 1500mAh 50-100C | 2-4 | $20-30 | Amazon, HobbyKing | Quick sessions |
| 3S LiPo 3000mAh 50-100C | 2-4 | $35-45 | Amazon, HobbyKing | Extended sessions |

**Where to find:**
- Search: "3S LiPo" or "11.1V LiPo battery"
- Look for: 50-100C discharge rating, gold connectors, good reviews

### Power Management

| Item | Qty | Cost | Vendor |
|------|-----|------|--------|
| 3S BMS Module (50A) | 1-2 | $8-15 | Amazon |
| Buck Regulator (5V/3.3V) | 1-2 | $15-25 | Amazon |
| INA219 I2C Monitor | 1-2 | $5-10 | Amazon |

**Specific part links:**
- BMS: Search "3S LiPo BMS 50A module"
- Regulator: Pololu D24V25F5S or MP2307DN module
- Monitor: "INA219 voltage current sensor module"

### Connectors & Accessories

| Item | Qty | Cost | Purpose |
|------|-----|------|---------|
| XT60 gold connectors | 5 pairs | $3-5 | Battery disconnect |
| 10 AWG silicone wire | 1 meter | $3-5 | Power connections |
| Heat shrink tubing kit | 1 | $2-3 | Cable insulation |
| M3 nylon spacers | 10 | $1-2 | Mounting/insulation |
| Fireproof LiPo bag | 2 | $5-10 | Battery storage |

### Battery Charger

| Item | Qty | Cost | Purpose |
|------|-----|------|---------|
| Tenergy LiPo Charger | 1 | $20-40 | Safe charging |

**Recommendation**: Use 50W+ charger with balance connector support
**Alternative**: If already have RC charger, just use that

### Shopping Checklist

**Priority 1 (Essential):**
- ☐ 2x 3S LiPo 1500mAh battery
- ☐ 1x 3S LiPo 3000mAh battery
- ☐ 1x 3S BMS module
- ☐ 1x Buck regulator
- ☐ 1x INA219 monitor
- ☐ 2x XT60 connectors
- ☐ 10 AWG wire (red/black)
- ☐ Heat shrink tubing

**Priority 2 (Recommended):**
- ☐ LiPo charger
- ☐ Fireproof LiPo bags
- ☐ Multimeter (for testing)

**Priority 3 (Optional):**
- ☐ XT30 micro connectors (for modularity)
- ☐ 3D printed enclosure (see later steps)

**Estimated Total**: $100-200 for full setup

---

## ⚙️ Phase 2: Assemble Power Stage (3-4 hours)

### Tools Needed

```
✓ Soldering iron (350-400°C)
✓ Solder (60/40, lead-free preferred)
✓ Wet sponge or brass cleaner
✓ Multimeter (for testing)
✓ Wire strippers
✓ Heat gun or lighter (for heat shrink)
✓ Work surface (non-flammable)
```

### Assembly Steps

**Step 1: Prepare Battery Modules (30 min)**

1. Inspect batteries for physical damage
2. Check that balance connectors are intact
3. Measure voltage with multimeter (should be 11.0-12.6V)
4. Place each in protective pouch

**Step 2: Wire BMS Module (45 min)**

```
Battery (+) Red   ──→ BMS Input +
Battery (-) Black ──→ BMS Input -
         ↓
    [BMS Output]
         ↓
   Continue to next step
```

**Soldering checklist:**
- ☐ Strip 1/4" insulation from each wire
- ☐ Tin both battery lead and BMS pad
- ☐ Solder with 3-5 second continuous heat
- ☐ Apply heat shrink tubing
- ☐ Verify with multimeter (same voltage at output)

**Step 3: Install INA219 Monitor (30 min, optional)**

```
BMS Output (+) ──→ INA219 +
BMS Output (-) ──→ INA219 -
    ↓
[INA219 Output continues to buck regulator]
```

**Connections:**
- INA219 VCC → 5V from buck regulator (install regulator first, then add)
- INA219 GND → Common ground
- INA219 SCL → Teensy pin 19
- INA219 SDA → Teensy pin 18

**Step 4: Install Buck Regulator (45 min)**

```
Battery/BMS Output (11.1V) ──→ Regulator Input
                    ↓
            ┌──────────────┬──────────────┐
            ↓              ↓              ↓
        GND Rail      5V Output      3.3V Output
            ↓              ↓              ↓
    Common Ground    Teensy VIN   ESP32 VIN
```

**Key points:**
- Input: 11.1V nominal
- Output 1: 5V @ 2.5A (for Teensy)
- Output 2: 3.3V @ 2.5A (for ESP32)
- All GND pins tied to common rail

**Step 5: Test Power Delivery (30 min)**

```
Measure at buck regulator output:
✓ 5V output: exactly 5.0V ± 0.2V (no load)
✓ 3.3V output: exactly 3.3V ± 0.1V (no load)
✓ All GND connections: 0Ω (continuity)
✓ No voltage between 5V and 3.3V outputs
```

**Troubleshooting if voltages wrong:**
- Check input polarity (+ should be positive, - should be negative)
- Verify connections with multimeter
- Check for cold solder joints (dull appearance)
- Reflow solder on suspect connections

**Step 6: XT60 Connector Installation (30 min)**

Connect XT60 on harness (female) to battery side (male)

```
XT60 Female ──┐
              ├─ Soldered to power output harness
              │
              ↓
          Harness:
         ┌─────────┐
         │ To Poi: │
         │ 5V,3.3V │
         │ GND     │
         └─────────┘
```

**Soldering XT60:**
1. Tin both connector and wire
2. Solder with continuous heat
3. Verify with multimeter before connecting
4. Test connection by pressing together firmly

### Assembly Verification Checklist

After assembly:
- ☐ All solder joints are shiny, not dull
- ☐ No visible cracks or cold joints
- ☐ All connections covered with heat shrink
- ☐ No exposed bare wire
- ☐ Voltages correct at output
- ☐ All ground connections verified (0Ω)
- ☐ Regulator is cool to touch
- ☐ XT60 connector is secure

---

## 💾 Phase 3: Firmware Integration (2-3 hours)

### Teensy 4.1 Firmware

**File**: `teensy_firmware/teensy_firmware.ino`

**Step 1: Add BatteryMonitor Library**

Copy `BatteryMonitor.h` to teensy_firmware folder
(Already created in this implementation)

**Step 2: Integrate Battery Code**

Follow `BATTERY_CODE_INTEGRATION_SNIPPETS.md`:

1. Add include: `#include "BatteryMonitor.h"`
2. Add global variables: battery, batteryWarningShown, lastBatteryCheck
3. Add to setup(): battery.begin() initialization
4. Add to loop(): battery check call
5. Add function: checkBattery()
6. Verify compilation (Verify button)

**Step 3: Upload & Test**

1. Select Board: Teensy 4.1
2. Select USB Type: Serial
3. Click Upload
4. Open Serial Monitor (115200 baud)
5. Should see: "Battery monitor initialized" + voltage reading

### ESP32 Firmware

**File**: `esp32_firmware/esp32_firmware.ino`

**Step 1: Add Battery Endpoint**

In `setupWebServer()` function, add:
```cpp
server.on("/api/battery", HTTP_GET, handleBatteryStatus);
```

**Step 2: Add Battery Handler Function**

At end of file, add complete `handleBatteryStatus()` function
(See `BATTERY_WEB_INTERFACE.md`)

**Step 3: Compile & Upload**

1. Use Arduino IDE or PlatformIO
2. Select Board: ESP32-S3 (or standard ESP32)
3. Compile: should show no errors
4. Upload to board

**Step 4: Test API**

1. Connect to WiFi: POV-POI-WiFi
2. Open browser: http://192.168.4.1/api/battery
3. Should see JSON response with voltage, percentage, etc.

### Firmware Testing Checklist

- ☐ Teensy firmware compiles without errors
- ☐ Serial monitor shows battery readings
- ☐ ESP32 firmware compiles without errors
- ☐ WiFi AP appears (POV-POI-WiFi)
- ☐ Can connect to WiFi
- ☐ API endpoint returns JSON
- ☐ Voltage reading is realistic (11-12V)
- ☐ Current increases when LEDs turn on

---

## 🌐 Phase 4: Web Interface Setup (30 min - 1 hour)

### Option A: Standalone Dashboard (Recommended for Quick Start)

1. Copy `web/battery_dashboard.html` to your poi system
2. Access at: `http://192.168.4.1/battery_dashboard.html`
3. Should show gauge and real-time readings

**Pros:**
- Works immediately
- No code modifications needed
- Easy to customize

**Cons:**
- Separate from main interface (one more click)

### Option B: Integrate into Main Dashboard

1. Copy battery panel HTML/CSS/JS from `BATTERY_WEB_INTERFACE.md`
2. Paste into your existing main interface HTML
3. Updates real-time on main page

**Pros:**
- All info in one place
- Professional appearance

**Cons:**
- Requires modifying existing HTML

### Option C: Use Web File Server

If your ESP32 has SPIFFS enabled:

1. Upload `battery_dashboard.html` to /web folder on ESP32 SPIFFS
2. Serve automatically
3. Access at: `http://192.168.4.1/`

---

## ✅ Phase 5: Testing & Validation (2-3 hours)

### Pre-flight Checklist

Use the interactive HTML checklist at: `tools/poi_preflight_checklist.html`

Or use the text version: `docs/PREFLIGHT_CHECKLIST.txt`

**Before every spin session:**
- ☐ Battery voltage 11.0V - 12.6V
- ☐ No battery damage
- ☐ XT60 connector secure
- ☐ Teensy boots and shows readings
- ☐ WiFi accessible
- ☐ API responds with JSON
- ☐ Web dashboard shows readings
- ☐ Area clear for spinning
- ☐ Know how to shut down

### Field Testing

1. **Battery voltage test**
   - Disconnect battery
   - Measure with multimeter
   - Should be 11-12.6V
   - If < 10.5V: do not spin

2. **Power delivery test**
   - Plug in battery
   - Measure 5V and 3.3V outputs
   - Should be within 0.2V

3. **Firmware test**
   - Open Serial Monitor
   - Should see battery readings every 5 seconds
   - Voltage should be stable

4. **Web dashboard test**
   - Open browser to dashboard
   - Gauge should update every 5 seconds
   - Stats should match serial monitor

5. **LED brightness test**
   - Start with pattern at brightness 50
   - Gradually increase brightness
   - Current draw should increase
   - Runtime estimate should decrease

6. **Low battery warning test**
   - Discharge battery to ~10.5V
   - Serial monitor should show yellow alert
   - Dashboard should show warning (amber color)
   - LED strip should flash yellow

### Field Testing Checklist

- ☐ Battery reads correct voltage
- ☐ Power outputs are stable under load
- ☐ Serial monitor shows consistent readings
- ☐ Web dashboard updates smoothly
- ☐ Current draw increases with brightness
- ☐ Runtime calculation seems reasonable
- ☐ Low battery alert triggers at 10.5V
- ☐ Can safely shut down system

---

## 🔄 Phase 6: Charging & Maintenance (ongoing)

### Charging Procedure

1. **Before charging:**
   - Disconnect battery from poi
   - Inspect for damage

2. **Charging settings:**
   - Chemistry: LiPo
   - Cells: 3S
   - Current: 1.5A (for 1500mAh) or 3A (for 3000mAh)
   - Mode: Balance charging

3. **During charging:**
   - Never leave unattended
   - Monitor charger for errors
   - Charge on fireproof surface

4. **After charging:**
   - Green LED indicates complete
   - Unplug immediately
   - Store in cool location

**Typical charge times:**
- 1500mAh @ 1.5A: ~1 hour
- 3000mAh @ 3A: ~1 hour

### Storage

Between uses:

- Store at room temperature
- Keep in fireproof LiPo bag
- Ideal storage voltage: 3.8V per cell (11.4V for 3S)
- Check monthly for puffing/damage
- Never expose to heat or moisture

---

## 📊 Cost Summary

| Phase | Item | Cost |
|-------|------|------|
| **Hardware** | Batteries (2x 1500mAh + 1x 3000mAh) | $75-100 |
|  | BMS, Regulator, INA219 | $30-50 |
|  | Connectors, wire, shrink | $8-15 |
|  | Charger | $20-40 |
| **Total Hardware** | Per poi system | **$110-205** |
| **Time** | Assembly, firmware, testing | **8-12 hours** |

---

## 📋 Complete File Reference

All files are in your repository under the `claude/poi-battery-power-V6Cju` branch:

```
📄 Hardware & Power Design
  ├─ docs/BATTERY_POWER_INTEGRATION.md
  ├─ docs/BATTERY_WIRING_QUICK_REFERENCE.md
  └─ teensy_firmware/BatteryMonitor.h

📄 Firmware Integration
  ├─ docs/BATTERY_CODE_INTEGRATION_SNIPPETS.md
  └─ docs/BATTERY_WEB_INTERFACE.md

📄 Web Interface
  └─ web/battery_dashboard.html

📄 Safety & Procedures
  ├─ docs/PREFLIGHT_CHECKLIST.txt
  └─ tools/poi_preflight_checklist.html

📄 This Guide
  └─ docs/BATTERY_IMPLEMENTATION_ROADMAP.md (you are here)
```

---

## 🚀 Quick Start Summary

**Day 1:** Order components
**Day 2-3:** Assemble power stage, test voltages
**Day 4:** Integrate firmware code, test API
**Day 5:** Set up web dashboard
**Day 6:** Full system testing before first spin

---

## ❓ Questions?

1. **"Where should I buy the battery?"**
   → Amazon or HobbyKing, search "3S LiPo 1500mAh"

2. **"How do I know if my battery is good?"**
   → Should read 11.0-12.6V with multimeter

3. **"What if the API doesn't respond?"**
   → Check Serial1 wiring and baud rate (115200)

4. **"Can I use a different charger?"**
   → Yes, if it supports LiPo and has balance connector

5. **"What's the weight with battery?"**
   → ~250g (quick) or ~320g (extended)

6. **"How long does it really last?"**
   → 45-60 min (quick module) or 2-2.5 hours (extended)

---

**Ready to build? Start with Phase 1: Order Components!** 🛒
**Questions? Check the detailed guides in the docs folder.**

**Last Updated**: 2026-02-19
**Status**: ✅ All components ready
**Next**: Begin Phase 1 ordering
