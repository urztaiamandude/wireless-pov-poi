# Production Readiness Review (Teensy + ESP32-S3 + Web UI)

This review was performed after reading the project architecture in `README.md` and focusing on concrete failure modes that can block production progress.

## Scope
- Teensy firmware (`teensy_firmware/teensy_firmware.ino`)
- ESP32-S3 firmware (`esp32_firmware/esp32_firmware.ino`)
- Self-hosted React Web UI (`esp32_firmware/webui/`)

## Critical findings

### 1) Teensy image buffer write can go out-of-bounds for 64px uploads
**Where:** `teensy_firmware/teensy_firmware.ino` (`POVImage::pixels` and `receiveImage()`)

**Issue:**
- `POVImage::pixels` is declared as `pixels[IMAGE_MAX_WIDTH][IMAGE_HEIGHT]` where `IMAGE_HEIGHT` is 32.
- `receiveImage()` accepts and processes heights up to `IMAGE_HEIGHT * 2` (64), then writes `pixels[x][y]` when `y < IMAGE_HEIGHT * 2`.
- This allows writes past the second dimension (32), causing memory corruption.

**Production risk:** intermittent crashes, corrupted image data, undefined behavior under larger uploads.

**Suggested fix (no feature removal):**
- Make dimensions consistent across firmware boundary.
- Either:
  1. Expand backing storage to a true 64-row structure (if intentionally supporting dual-side/stacked uploads), or
  2. Strictly clamp write bounds to `y < IMAGE_HEIGHT` and reject/resize uploads that exceed 32 rows before write.
- Add an explicit guard before any write: `if (y >= IMAGE_HEIGHT) continue;`.

---

### 2) Teensy live-frame parser reads stale/unvalidated command bytes
**Where:** `teensy_firmware/teensy_firmware.ino` (`receiveLiveFrame()`)

**Issue:**
- Bounds check compares indices to `CMD_BUFFER_SIZE` rather than current packet length (`cmdBufferIndex`).
- If a live frame packet is short/truncated, parser may read old bytes left in command buffer.

**Production risk:** random pixel artifacts, hard-to-reproduce rendering glitches, protocol desync symptoms.

**Suggested fix:**
- Validate the exact payload length before reading:
  - expected payload = `g_displayLeds * 3`
  - required packet length = `4 + expected payload` (`0xFF cmd len ... 0xFE`)
- Use `cmdBufferIndex` for per-byte safety checks, not `CMD_BUFFER_SIZE`.
- Reject malformed frames with a debug counter/log and keep the previous frame.

---

### 3) Teensy command handlers lack per-command payload validation
**Where:** `teensy_firmware/teensy_firmware.ino` (`receivePattern()`, `receiveSequence()`, command switch in `parseCommand()`)

**Issue:**
- Several handlers index directly into `cmdBuffer` without confirming full packet payload length for that command.
- Example: `receivePattern()` reads up to `cmdBuffer[11]` without checking `dataLen`/packet size first.

**Production risk:** malformed or partial serial packets can corrupt pattern/sequence state.

**Suggested fix:**
- Add a central `validatePacket(minDataLen)` helper and apply before each command handler.
- For variable-size commands (sequence), validate against computed expected length.
- NACK malformed packets so ESP32 can retry.

---

### 4) ESP32 default STA credentials are hardcoded to a real-looking SSID/password
**Where:** `esp32_firmware/esp32_firmware.ino` (`loadDeviceConfig()`)

**Issue:**
- Defaults are set to `"Office"` and `"6195717200"` when no credentials are saved.

**Production risk:**
- Security/privacy concern (shipping with guessed credentials pattern).
- Unexpected connection attempts at boot.
- Undesired behavior in deployments where AP mode is expected unless explicitly configured.

**Suggested fix:**
- Default to empty STA credentials (`""`, `""`).
- Attempt STA connect only when SSID is non-empty and explicitly configured.
- Keep AP mode as deterministic fallback.

---

### 5) ESP32 ↔ Teensy link check does not verify full framed response
**Where:** `esp32_firmware/esp32_firmware.ino` (`checkTeensyConnection()`)

**Issue:**
- Reads fixed bytes after `0xFF 0xBB` marker but does not verify end marker (`0xFE`).
- Can leave stream misaligned if any stray bytes arrive.

**Production risk:** false disconnects, stale status, occasional protocol desync under noise/high traffic.

**Suggested fix:**
- Reuse `readTeensyResponse()` for status checks as well.
- Enforce complete frame validation (`start`, `marker`, expected payload bytes, `end`).
- Flush/realign on malformed response.

## Medium-priority findings

### 6) Web UI LED count defaults to 31 until Advanced Settings is opened
**Where:** `esp32_firmware/webui/App.tsx`

**Issue:**
- `ledCount` starts at 31 in top-level app state.
- It is only updated after `AdvancedSettings` fetches `/api/hardware/leds`.
- Other tabs can operate with stale assumptions if user never opens that page.

**Production risk:** inconsistent previews/upload assumptions vs real hardware config.

**Suggested fix:**
- Fetch `/api/hardware/leds` once at app bootstrap and hydrate global `ledCount`.
- Keep Advanced Settings as editor, not the first source of truth fetch.

---

### 7) Web UI depends on `AbortSignal.timeout()` (compatibility risk)
**Where:** multiple fetch calls in `esp32_firmware/webui/components/AdvancedSettings.tsx`

**Issue:**
- `AbortSignal.timeout()` is not universal across older mobile webviews/browsers.

**Production risk:** API calls can throw before request starts on some clients, especially embedded/legacy webviews.

**Suggested fix:**
- Add a small compatibility wrapper for fetch timeout (`AbortController` + `setTimeout`) and use it across the UI.

## Hardening recommendations (cross-cutting)

1. **Protocol contract doc + tests:** define each serial command payload schema and length invariants; add negative tests (truncated packet, oversized packet, malformed marker).
2. **Counters for malformed packets:** expose on `/api/status` so field debugging is possible without serial console.
3. **Fuzz-lite harness:** random packet mutation tests against Teensy parser in test mode.
4. **Secure defaults:** no default external WiFi credentials; explicit opt-in only.
5. **Pre-release gate:** enforce `webui tsc`, `webui build`, and both firmware compile checks in CI.

## What to fix first (recommended order)
1. Teensy out-of-bounds image write (Critical)
2. Teensy packet validation + live-frame length guard (Critical)
3. ESP32 secure STA defaults (Critical)
4. ESP32 framed status parsing hardening (Critical)
5. Web UI global LED config bootstrap (Medium)
6. Web UI timeout compatibility wrapper (Medium)
