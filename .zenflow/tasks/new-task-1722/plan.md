# Spec and build

## Configuration
- **Artifacts Path**: `.zenflow/tasks/new-task-1722`

---

## Agent Instructions

Ask the user questions when anything is unclear or needs their input. This includes:
- Ambiguous or incomplete requirements
- Technical decisions that affect architecture or user experience
- Trade-offs that require business context

Do not make assumptions on important decisions — get clarification first.

If you are blocked and need user clarification, mark the current step with `[!]` in plan.md before stopping.

---

## Workflow Steps

### [x] Step: Technical Specification
<!-- chat-id: 577f0d4c-1216-4d0a-8967-dcfa1e21f70b -->

See `.zenflow/tasks/new-task-1722/spec.md` for full findings.

Key findings:
- PROGMEM HTML has `DISPLAY_LED_START` undefined bug (LED preview broken)
- Image upload width capped at 100px (should be 400)
- No next/previous image navigation in either UI
- No power performance modes anywhere
- Pattern 2 (Gradient) static - needs scrolling animation
- Pattern 7 (Strobe) timing uses frame counter instead of millis()
- React UI missing: full mode selector, pattern buttons, frame rate control, power modes, SD card UI
- React UI hardcoded IPs in AdvancedSettings.tsx and ImageLab.tsx

---

### [x] Step: Fix PROGMEM HTML bugs and add next/prev image + power modes
<!-- chat-id: 9c57eb7c-8a50-4466-8150-5e5d7f120743 -->

Modify `esp32_firmware/esp32_firmware.ino`:

1. In the JavaScript section of `rootPage[]`, add `const DISPLAY_LED_START=0;` to the constants block alongside `NUM_LEDS` and `DISPLAY_LEDS`.
2. In `convertImageToPOVFormat()`, change `tW=Math.min(100,...)` to `tW=Math.min(400,...)` so image upload respects the PSRAM-capable 400px max width.
3. In the Dashboard tab HTML, add Prev/Next image buttons beside the Content Index input.
4. Add corresponding `prevImage()` and `nextImage()` JavaScript functions that decrement/increment the content index and call `/api/mode`.
5. Add a new "Power Mode" card to the Dashboard tab HTML with 4 preset buttons: Performance, Balanced, Power Save, Ultra Save.
6. Add a `powerMode` JavaScript variable and `setPowerMode(mode)` function calling new endpoint `/api/power/mode`.
7. In the C++ section, add `handlePowerMode()` function implementing CPU frequency setting (`setCpuFrequencyMhz()`), frame rate update, and optional brightness cap.
8. Register `server.on("/api/power/mode", HTTP_POST, handlePowerMode)` in `setupWebServer()`.
9. Add `uint8_t powerMode = 1;` to `SystemState` struct.
10. Run `npx tsc --noEmit` and `npm run build` in `esp32_firmware/webui` to ensure no regressions.

---

### [ ] Step: Fix Teensy pattern bugs (Gradient animation, Strobe timing)
<!-- chat-id: 49bbd39b-4730-459c-a858-cdfcaa38c4ae -->

Modify `teensy_firmware/teensy_firmware.ino`:

1. **Pattern 2 (Gradient)**: Change the static gradient to a scrolling version by adding a time-based offset to the blend position. The gradient will now sweep across LEDs over time, producing a proper POV animation.
   ```cpp
   case 2:
     for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
       uint8_t pos = ((i - DISPLAY_LED_START) * 255 / DISPLAY_LEDS + patternTime * pat.speed / 20) & 0xFF;
       leds[i] = blend(pat.color1, pat.color2, pos);
     }
     break;
   ```

2. **Pattern 7 (Strobe)**: Replace `patternTime`-based timing with `millis()` for accurate wall-clock flash rate. Also fix the `strobeDelay` map range to use milliseconds (500ms–10ms instead of arbitrary frame units).

---

### [ ] Step: Enhance React Web UI Dashboard with full controls
<!-- chat-id: ce8079f9-7bb7-4081-81e0-05669f1458c6 -->

Modify `esp32_firmware/webui/components/Dashboard.tsx`:

1. Add a **Mode Selector** section: 5 buttons (Idle, Image, Pattern, Sequence, Live) that call `/api/mode` with the correct numeric mode value.
2. Add **Prev/Next image** navigation buttons (active only when mode=1/Image). Call `/api/mode` with incremented/decremented index.
3. Add a **Patterns panel** (collapsible or tabbed) with buttons for all 18 patterns (Basic 0–10, Advanced 16–17, Audio 11–15). Each button calls `/api/pattern` then `/api/mode`.
4. Add a **Frame Rate slider** (10–120 FPS) with debounced API call to `/api/framerate`.
5. Add a **Power Mode selector**: 4 preset buttons (Performance / Balanced / Power Save / Ultra Save) calling `/api/power/mode`.

Fix hardcoded IPs:
- `esp32_firmware/webui/components/AdvancedSettings.tsx`: Remove `const DEVICE_IP = '10.100.9.230'` and use the same `getDeviceBase()` pattern from Dashboard.tsx (detect localhost vs. device).
- `esp32_firmware/webui/components/ImageLab.tsx`: Replace `const FLEET_IPS = ['10.100.9.230']` with dynamic origin detection.

Add to `esp32_firmware/webui/types.ts`:
```typescript
export type PowerMode = 'performance' | 'balanced' | 'powersave' | 'ultrasave';
```

After all changes:
- Run `cd esp32_firmware/webui && npx tsc --noEmit`
- Run `cd esp32_firmware/webui && npm run build`
- Fix any type errors found.

---

### [ ] Step: Write report
<!-- chat-id: 4fa1a60a-8d73-46e6-a5e5-6899b880716a -->

After all implementation steps complete, write `.zenflow/tasks/new-task-1722/report.md` describing:
- What was implemented
- How each fix was verified
- Challenges encountered
- Enhancement suggestions (battery monitor, audio level indicator, image gallery, PWA offline caching, OTA)
