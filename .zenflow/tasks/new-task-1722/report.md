# Implementation Report: Nebula POI Firmware & Web UI Audit + Enhancement

**Date:** 2026-03-01  
**Task:** Audit and fix Teensy + ESP32 firmware and web UI; add missing controls; fix patterns.

---

## Summary of Work

Four implementation steps were completed across three codebases: the ESP32 embedded (PROGMEM) web UI, the Teensy 4.1 firmware, and the React companion web UI.

---

## Step 1: PROGMEM HTML Bug Fixes + Power Modes + Prev/Next Navigation

**File:** `esp32_firmware/esp32_firmware.ino`  
**Commit:** `e8a5914`

### Bugs Fixed

| Bug | Fix |
|-----|-----|
| `DISPLAY_LED_START` undefined in JS — LED preview `ReferenceError` | Added `const DISPLAY_LED_START=0;` to the JS constants block |
| Image upload width hard-capped at 100 px | Changed `Math.min(100,…)` → `Math.min(400,…)` — respects PSRAM 400 px max |
| `state.powerMode` boots as 0 (performance) instead of balanced | Added `state.powerMode = 1` in `setup()` |
| `nextImage()` had no upper bound — could request out-of-range image index | Added `Math.min(el.max, …)` clamp using the numeric input's `max` attribute |

### Features Added

- **Prev / Next image buttons** beside the Content Index input in the Dashboard tab; call `/api/mode` with decremented / incremented index.
- **Power Performance Mode card** in the Dashboard tab — 4 preset buttons:

  | Label | CPU MHz | Frame Rate | Max Brightness |
  |-------|---------|------------|----------------|
  | Performance | 240 | 120 | 255 |
  | Balanced *(default)* | 160 | 60 | 255 |
  | Power Save | 80 | 30 | 180 |
  | Ultra Save | 80 | 15 | 80 |

  Note: Ultra Save uses 80 MHz (not 40 MHz) because `setCpuFrequencyMhz(40)` crashes the WiFi stack on ESP32.

- **New REST endpoint** `POST /api/power/mode` — sets CPU frequency, updates `state.frameRate`, forwards the new frame rate to Teensy, broadcasts via ESP-NOW, and caps max brightness for low-power modes.
- `powerMode` added to `SystemState` struct and included in `/api/status` response so page refreshes restore the active button highlight.
- `setPowerMode()` JavaScript function updates the framerate and brightness UI sliders immediately on success so the UI stays in sync.
- `handlePowerMode()` includes guarded calls to `espNowSync.broadcastFrameRate()` and `espNowSync.broadcastBrightness()` so peer poi adjust as well.

---

## Step 2: Teensy Pattern Bug Fixes

**File:** `teensy_firmware/teensy_firmware.ino`  
**Commit:** `9214d65`

### Pattern 2 — Gradient (animated)

**Problem:** Pattern was a static color blend; no movement, unsuitable for a spinning POV display.

**Fix:** Added a `sin8()`-based sinusoidal phase offset driven by `millis()` and `pat.speed`:

```cpp
case 2:
  {
    uint32_t gradMillis = millis() / 500u;
    uint8_t phase = sin8((uint8_t)(gradMillis * pat.speed));
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      uint8_t pos = (uint8_t)((i - DISPLAY_LED_START) * 255 / DISPLAY_LEDS) + phase;
      leds[i] = blend(pat.color1, pat.color2, pos);
    }
  }
  break;
```

Using `sin8()` gives a smooth wave that returns to its start value — no visible seam when the animation cycles. Dividing `millis()` by 500 first prevents 32-bit overflow at long uptimes.

### Pattern 7 — Strobe (timing accuracy)

**Problem:** Flash timing compared `lastStrobe` against `patternTime` (a frame counter), giving arbitrary timing that varied with frame rate.

**Fix:** `lastStrobe` now holds a `millis()` value, and flash period is `map(pat.speed, 1, 255, 500, 10)` milliseconds — giving a true 2 Hz to 100 Hz strobe range independent of frame rate.

---

## Step 3: React Web UI — Full Controls

**Files:** `Dashboard.tsx`, `AdvancedSettings.tsx`, `ImageLab.tsx`, `types.ts`  
**Commits:** `c2cc3bf` (initial), `267331d` (review fixes)

### Dashboard.tsx — New Features

| Feature | Implementation |
|---------|---------------|
| **Mode Selector** | 5 buttons (Idle / Image / Pattern / Sequence / Live) — each calls `POST /api/mode` |
| **Content Navigation** | Prev (SkipBack) / Next (SkipForward) with index display; shown only in Image and Sequence modes; Next capped at `maxContentIndex` (seeded from `data.count` in status poll, defaults to 49 for PSRAM max) |
| **Patterns Panel** | 18 buttons grouped: Basic 0–10, Audio Reactive 11–15 (pink), Advanced 16–17 (violet); shown only in Pattern mode; each calls `POST /api/pattern` then `POST /api/mode` |
| **Frame Rate Slider** | 10–120 FPS, debounced `POST /api/framerate`; poll skip guard prevents overwriting mid-drag |
| **Power Mode Selector** | 4 preset buttons calling `POST /api/power/mode` |
| **Status polling** | Reads `mode`, `brightness`, `framerate`, `powerMode` from `/api/status` to seed initial UI state; poll skips overwriting values touched within the last 1 s to prevent race conditions |

### Bug Fixes

| Bug | Fix |
|-----|-----|
| Hardcoded `DEVICE_IP = '10.100.9.230'` in `AdvancedSettings.tsx` | Removed; `baseUrl` uses `''` (relative path works in both Vite dev and on-device) |
| Hardcoded `FLEET_IPS = ['10.100.9.230']` in `ImageLab.tsx` | Replaced with `getDeviceBase()` using `window.location.origin` detection |
| `showPatterns` dead state variable | Removed entirely |
| Unused icon imports (`Flame`, `Wind`, `ChevronLeft`, `ChevronRight`) | Removed from import list |
| `debouncedFrameRateUpdate` stale closure (captured `isSyncMode` / `devices` / `activeDevice`) | Replaced with `isSyncModeRef` / `devicesRef` / `activeDeviceRef` refs; `useCallback` deps array is `[]` |
| Ultra Save labeled "40 MHz" but firmware uses 80 MHz | Corrected label to "80 MHz*" |

### types.ts

```typescript
export type PowerMode = 'performance' | 'balanced' | 'powersave' | 'ultrasave';
```

---

## Verification

### TypeScript Typecheck

```
cd esp32_firmware/webui && npx tsc --noEmit
```
✅ Zero errors.

### React Build

```
cd esp32_firmware/webui && npm run build
```
✅ Clean build, ~288 KB bundle (optimized for ESP32 SPIFFS).

### Pattern Logic (code review)

All 18 patterns reviewed against their names:

| ID | Name | Status |
|----|------|--------|
| 0 | Rainbow | ✅ |
| 1 | Wave | ✅ |
| 2 | Gradient | ✅ Fixed — now animated |
| 3 | Sparkle | ✅ |
| 4 | Fire | ✅ |
| 5 | Comet | ✅ |
| 6 | Breathing | ✅ |
| 7 | Strobe | ✅ Fixed — millis() timing |
| 8 | Meteor | ✅ |
| 9 | Wipe | ✅ |
| 10 | Plasma | ✅ |
| 11 | VU Meter | ✅ Audio reactive via analogRead(A0) |
| 12 | Pulse | ✅ Audio reactive |
| 13 | Audio Rainbow | ✅ Audio reactive |
| 14 | Center Burst | ✅ Audio reactive |
| 15 | Audio Sparkle | ✅ Audio reactive |
| 16 | Split Spin | ✅ |
| 17 | Theater Chase | ✅ |

Audio patterns (11–15) react to live analog input on Teensy pin A0 from a MAX9814 microphone. Without a mic they display low/zero activity — this is expected hardware behavior, not a bug.

### Firmware Manual Verification (required hardware)

The following should be verified on physical hardware:

- Flash `esp32_firmware.ino` → connect to POV-POI-WiFi → confirm PROGMEM HTML shows Prev/Next image buttons and Power Mode card.
- Verify LED preview in PROGMEM HTML renders without `ReferenceError` (DISPLAY_LED_START fix).
- `curl -X POST http://192.168.4.1/api/power/mode -H 'Content-Type: application/json' -d '{"mode":"powersave"}'` → confirm 200 response with `cpu_mhz: 80`.
- Flash `teensy_firmware.ino` → select Pattern 2 → confirm gradient scrolls. Select Pattern 7 → confirm strobe flashes at hardware-accurate rate.

---

## Challenges Encountered

1. **PROGMEM string editing** is fragile — minified HTML/JS is a single C string constant spanning ~1200 lines. Each change requires careful brace/quote escaping.
2. **ESP32 WiFi / CPU frequency floor**: `setCpuFrequencyMhz(40)` is below the minimum WiFi radio clock and crashes the stack. Ultra Save mode uses 80 MHz as the safe floor.
3. **React stale closure in debounced callbacks**: The frame rate debounce used captured state variables that went stale in long-running intervals; replaced with refs to avoid triggering unnecessary re-renders while keeping values current.
4. **Status poll / user input race**: Polling `/api/status` and user slider interaction can race, causing the UI to snap back. Fixed by tracking a `lastInteraction` timestamp per control and skipping the poll overwrite for 1 s after user input.

---

## Enhancement Suggestions

The following improvements were identified during the audit. None are blocking, but each would meaningfully improve the product:

### 1. Battery / Power Monitoring (High Priority)
`teensy_firmware/BatteryMonitor.h` exists and is documented but is **not included** in `teensy_firmware.ino`. Integrating it would:
- Enable `/api/status` to return `batteryPercent` and `batteryVoltage`.
- Allow the UI to show a battery gauge.
- Allow automatic power mode downgrade as battery drains (e.g., auto-switch to Power Save below 30%).

### 2. Audio Level Indicator (Medium Priority)
The PROGMEM HTML and React UI have no feedback on whether the MAX9814 microphone is detecting audio. A simple `/api/audio/level` poll endpoint + a small level meter in the Dashboard would let users confirm the mic is working before selecting audio patterns — currently there is no way to know without watching the LEDs.

### 3. Image Gallery (Medium Priority)
After images are uploaded they occupy anonymous PSRAM slots. There is no UI to see which slots are in use, preview stored images, or delete individual images. A `/api/image/list` endpoint (returning slot index + dimensions) combined with a thumbnail gallery view in the React UI would significantly improve usability.

### 4. OTA Firmware Updates (Medium Priority)
`FirmwareManager.tsx` in the React UI references OTA update functionality. The `docs/DEPLOYMENT.md` notes OTA as pending. The ESP32 Arduino framework supports `Update.h` / `ArduinoOTA` natively. Implementing OTA for the ESP32 side (and optionally Teensy via HalfKay / TeensyLoader) would eliminate the need for USB access during maintenance.

### 5. PWA / Service Worker Offline Caching (Low Priority)
The ESP32 serves a stub `/sw.js` (returns a minimal script that installs but caches nothing). A real service worker that caches the PROGMEM HTML shell, CSS, and JS on first visit would allow the control UI to load even when the ESP32 is temporarily unreachable — useful when the poi battery is low and the AP is intermittently available.

### 6. Pattern Preview in UI (Low Priority)
Pattern buttons in both UIs are text-only labels. Small animated CSS mini-previews (e.g., a looping color sweep for Rainbow, a blinking square for Strobe) would help users understand what each pattern looks like before selecting it, reducing trial-and-error on-device.
