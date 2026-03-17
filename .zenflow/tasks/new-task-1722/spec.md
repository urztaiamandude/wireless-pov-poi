# Technical Specification: Nebula POI Firmware & Web UI Audit + Enhancement

## Difficulty Assessment: **HARD**

Complex, multi-component task touching C++ firmware (Teensy + ESP32), embedded HTML/JS, and a React TypeScript web UI. Multiple systems interact via a binary serial protocol. Power management is a new feature requiring both firmware and UI changes.

---

## Technical Context

### Language & Runtimes
- **Teensy Firmware**: C++ (Arduino/Teensyduino), FastLED library
- **ESP32 Firmware**: C++ (Arduino), ArduinoJson, WebServer, SPIFFS
- **Embedded Web UI**: Vanilla HTML/CSS/JS (PROGMEM in esp32_firmware.ino ~3186 lines)
- **React Web UI**: TypeScript + React 19 + Vite + Tailwind CSS (`esp32_firmware/webui/`)

### Key Architecture Facts
- The **primary device UI** at `http://192.168.4.1` is the PROGMEM HTML embedded in `esp32_firmware.ino` (the `rootPage[]` constant, ~1200 lines of minified HTML/CSS/JS).
- The React `webui/` app is a companion fleet-management / code-generation tool intended to optionally replace the PROGMEM HTML when deployed to SPIFFS, but is NOT currently served by the ESP32 firmware's `handleRoot()`.
- Both UIs call the same REST API endpoints (`/api/*`).
- All 18 patterns (types 0–17) are fully implemented in `teensy_firmware.ino`.
- Audio patterns (11–15) read from `analogRead(AUDIO_PIN)` on Teensy pin A0 — they work correctly with a MAX9814 microphone; without hardware they show minimal activity (expected behavior, not a bug).

---

## Findings / Discrepancies

### A. PROGMEM HTML (Primary Web UI)
| Issue | Severity | Description |
|-------|----------|-------------|
| `DISPLAY_LED_START` undefined in JS | Bug | Used in LED preview loops but never declared in JS. Should be `0`. Causes `ReferenceError`; LED preview panel is silently broken. |
| No Next/Previous image buttons | Missing feature | Content index is a numeric input, no prev/next buttons for navigating stored images. |
| No Power Performance Modes | Missing feature | No CPU/FPS/brightness presets for power management. |
| Image upload width capped at 100px | Bug | `tW=Math.min(100,...)` in `convertImageToPOVFormat()` but firmware supports up to 400px with PSRAM. Should use 400. |
| Pattern 2 (Gradient) static | Behavior gap | Gradient is a static color blend; doesn't animate. Consistent with a fixed gradient display, but no movement. Should scroll/animate for better POV effect. |
| Strobe pattern uses `patternTime` for timing | Code issue | `lastStrobe` is compared against `patternTime` (a frame counter) not actual milliseconds. Timing inconsistency. Should use `millis()`. |

### B. React Web UI (`webui/`)
| Issue | Severity | Description |
|-------|----------|-------------|
| Dashboard missing full mode selector | Missing feature | Only PLAY/STOP buttons. No idle/image/pattern/sequence/live selector. |
| No pattern selection | Missing feature | No pattern buttons in Dashboard or any other view. |
| No Next/Previous image navigation | Missing feature | Not present anywhere in React UI. |
| No frame rate control | Missing feature | Dashboard only has brightness slider. |
| No power performance modes | Missing feature | Not implemented anywhere. |
| No SD card management | Missing feature | Not present in React UI (present in PROGMEM HTML). |
| Hardcoded `DEVICE_IP = '10.100.9.230'` | Bug | `AdvancedSettings.tsx` has a hardcoded developer IP that won't work for other users. Should auto-detect. |
| Hardcoded fleet IPs in `ImageLab.tsx` | Bug | `const FLEET_IPS = ['10.100.9.230']` — same issue. |
| No live draw canvas | Missing feature | Not in React UI (present in PROGMEM HTML). |

### C. Firmware Completeness
| Component | Status | Notes |
|-----------|--------|-------|
| Teensy: All 18 patterns | ✅ Complete | Each pattern implements its named behavior correctly. |
| Teensy: Audio patterns (11–15) | ✅ Complete | React to audio via `analogRead(A0)`. Without mic: shows low/zero levels (expected). |
| Teensy: Image display | ✅ Complete | Supports up to 200 images w/ PSRAM. |
| Teensy: Sequence mode | ✅ Complete | Looping, timed, mix of images and patterns. |
| Teensy: Live mode | ✅ Complete | Accepts RGB frames from ESP32. |
| Teensy: SD card | ✅ Complete | Save/load/list/delete when `SD_SUPPORT` defined. |
| ESP32: REST API | ✅ Complete | All documented endpoints present. |
| ESP32: Multi-poi sync | ✅ Complete | ESP-NOW based, plus legacy HTTP sync. |
| ESP32: BLE | ✅ Conditional | Needs `BLE_ENABLED` define; bridge file present at `src/ble_bridge.h`. |
| Power modes | ❌ Missing | Not implemented in firmware or any UI. |

---

## Implementation Approach

### Phase 1: PROGMEM HTML Bug Fixes
Fix the broken LED preview and image width cap. These are self-contained JS changes inside the giant PROGMEM string.

### Phase 2: PROGMEM HTML — New Controls
Add to the **Dashboard tab** in the PROGMEM HTML:
1. **Next / Previous image buttons** (beside Content Index input)  
2. **Power Performance Mode selector** (new card in Dashboard tab)

Add to the **ESP32 firmware**:
1. `/api/power/mode` POST endpoint accepting `{ "mode": "performance"|"balanced"|"powersave"|"ultrasave" }` that:
   - Sets ESP32 CPU frequency via `setCpuFrequencyMhz()`
   - Adjusts `state.frameRate` (and forwards new framerate to Teensy)
   - Caps max brightness allowed in low-power modes

### Phase 3: Animate Gradient Pattern (Teensy firmware)
Fix pattern 2 (Gradient) to scroll the gradient across LEDs over time, making it visually distinct from a static color blend.

### Phase 4: Fix Strobe Timing (Teensy firmware)
Fix the strobe pattern to use `millis()` instead of `patternTime` for accurate flash timing.

### Phase 5: React Web UI — Full Controls
Enhance `Dashboard.tsx` with:
1. Full **mode selector** (Idle/Image/Pattern/Sequence/Live) via `/api/mode`
2. **Next/Previous image** buttons (mode 1 = image, increment/decrement index)
3. All **18 pattern buttons** (organized: Basic 0–10, Advanced 16–17, Audio 11–15)
4. **Frame rate slider** (10–120 FPS) via `/api/framerate`
5. **Power Performance Mode** buttons/selector (4 presets) via `/api/power/mode`

Fix hardcoded IPs:
- `AdvancedSettings.tsx`: `DEVICE_IP` → use same `getDeviceBase()` pattern from `Dashboard.tsx`
- `ImageLab.tsx`: `FLEET_IPS` → use `window.location.origin` for non-localhost

---

## Source Code Changes

### Files Modified

| File | Change |
|------|--------|
| `esp32_firmware/esp32_firmware.ino` | Fix `DISPLAY_LED_START` in JS, fix image width cap (100→400), add prev/next image JS functions, add power mode card in HTML, add `/api/power/mode` handler + `handlePowerMode()` C++ function, register route in `setupWebServer()` |
| `teensy_firmware/teensy_firmware.ino` | Fix Gradient pattern to animate (scrolling), fix Strobe timing to use `millis()` |
| `esp32_firmware/webui/components/Dashboard.tsx` | Add mode selector, prev/next buttons, pattern buttons (all 18), frame rate slider, power mode selector |
| `esp32_firmware/webui/components/AdvancedSettings.tsx` | Remove hardcoded `DEVICE_IP`, use `getDeviceBase()` helper |
| `esp32_firmware/webui/components/ImageLab.tsx` | Fix hardcoded `FLEET_IPS` |
| `esp32_firmware/webui/types.ts` | Add `PowerMode` type |

### New API Endpoint

```
POST /api/power/mode
Content-Type: application/json
Body: { "mode": "performance" | "balanced" | "powersave" | "ultrasave" }

Response: { "status": "ok", "mode": "...", "cpu_mhz": N, "framerate": N, "max_brightness": N }
```

**Power mode presets:**

| Mode | CPU MHz | Frame Rate | Max Brightness | Notes |
|------|---------|-----------|----------------|-------|
| performance | 240 | 120 | 255 | Full power |
| balanced | 160 | 60 | 255 | Default |
| powersave | 80 | 30 | 180 | ~40% power reduction |
| ultrasave | 80 | 15 | 80 | Max battery life |

The `/api/power/mode` handler also forwards the frame rate to Teensy via the existing `/api/framerate` logic.

---

## Data Model / API Changes

No new data models needed. New field added to `SystemState`:
```cpp
struct SystemState {
  // ... existing fields ...
  uint8_t powerMode;  // 0=performance, 1=balanced, 2=powersave, 3=ultrasave
};
```

New React type:
```typescript
type PowerMode = 'performance' | 'balanced' | 'powersave' | 'ultrasave';
```

---

## Pattern Fixes Detail

### Pattern 2 – Gradient (Teensy firmware, `displayPattern()` case 2)
**Current:** Static blend from `color1` to `color2` along strip. No animation.  
**Fix:** Add a time-based offset so the gradient scrolls, making it visually interesting as a POV pattern.

```cpp
case 2:  // Gradient - scrolling color blend
  for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
    uint8_t pos = ((i - DISPLAY_LED_START) * 255 / DISPLAY_LEDS + patternTime * pat.speed / 20) & 0xFF;
    leds[i] = blend(pat.color1, pat.color2, pos);
  }
  break;
```

### Pattern 7 – Strobe (Teensy firmware, `displayPattern()` case 7)
**Current:** Uses `patternTime` (frame-counter-based time) for lastStrobe comparison. Produces correct relative behavior but not accurate wall-clock timing.  
**Fix:** Use `millis()` directly for strobe timing.

```cpp
case 7:  // Strobe - quick flashes
  {
    static bool strobeOn = false;
    static uint32_t lastStrobe = 0;
    uint32_t strobeDelay = map(pat.speed, 1, 255, 500, 10);  // ms
    uint32_t now = millis();
    if (now - lastStrobe > strobeDelay) {
      strobeOn = !strobeOn;
      lastStrobe = now;
    }
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      leds[i] = strobeOn ? pat.color1 : CRGB::Black;
    }
  }
  break;
```

---

## Suggestions / Enhancements Discovered

1. **Battery / Power monitoring**: The docs reference battery power extensively (`BATTERY_POWER_INTEGRATION.md`, `BatteryMonitor.h` exists in `teensy_firmware/`) but the `BatteryMonitor.h` is not included in `teensy_firmware.ino`. Integrating it would enable the UI to show battery percentage and automatically select power modes.

2. **Pattern preview thumbnails in UI**: The PROGMEM HTML pattern buttons are text-only. Adding small animated CSS previews or emoji indicators would help users understand patterns before selecting.

3. **Audio level indicator**: The embedded web UI has no feedback on whether the microphone is detecting audio. A simple audio level meter in the Dashboard (polling `/api/status` for an audio level field) would help users confirm the mic is working before selecting audio patterns.

4. **OTA Updates**: `FirmwareManager.tsx` in the React UI references OTA but it's not implemented in firmware. The `docs/DEPLOYMENT.md` notes this as pending. Given the project claims production-ready status, this is a notable gap.

5. **Image gallery**: Once images are uploaded to Teensy RAM (or SD card), there's no way to see which slots are used or preview stored images. A `/api/image/list` endpoint + gallery view would improve usability significantly.

6. **PWA offline caching**: The PROGMEM HTML has a service worker stub (`/sw.js`) that's sent as a stub, not a real caching SW. A real service worker would allow the UI to work offline after first visit.

---

## Verification Approach

### Type Check (React UI)
```bash
cd esp32_firmware/webui
npx tsc --noEmit
```

### Build Verification (React UI)
```bash
cd esp32_firmware/webui
npm run build
```

### Python Tests
```bash
cd examples
python3 -m pytest test_*.py -v
```

### Manual Firmware Verification
- Flash `teensy_firmware.ino` and verify each pattern type via USB serial monitor
- Flash `esp32_firmware.ino` and verify `/api/power/mode` endpoint with curl
- Connect to POV-POI-WiFi and verify PROGMEM HTML prev/next buttons work
- Verify LED preview renders (DISPLAY_LED_START fix)
