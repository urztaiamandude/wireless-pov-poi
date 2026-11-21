# POV Poi — Bug Log & Solutions

Entries are reverse-chronological. Search this file before debugging any recurring issue.

---

### 2025-xx-xx — BUG-001: EXTMEM / ps_malloc() allocation silently returns NULL

**Issue**: Large frame buffer (e.g. `CRGB imageBuffer[MAX_FRAMES][32]`) declared in normal RAM causes hard-fault or garbage display on Teensy 4.1 / ESP32-S3.

**Root Cause**: Large arrays exceed the on-chip SRAM limit. Teensy 4.1 has ~1 MB SRAM but PSRAM requires explicit allocation via `EXTMEM` keyword or `ps_malloc()`. ESP32-S3 has 8 MB PSRAM (N16R8 variant) but it is also opt-in.

**Solution**:
- Teensy 4.1: prefix large global arrays with `EXTMEM` → `EXTMEM CRGB imageBuffer[MAX_FRAMES][32];`
- ESP32-S3: use `ps_malloc()` at runtime; check return value before use.
- Always verify PSRAM is detected at boot: print `ESP.getPsramSize()` / check `external_psram_size`.

**Prevention**: Any buffer that could exceed ~200 KB must use EXTMEM/ps_malloc. Add a compile-time `static_assert` on buffer sizes if possible.

---

### 2025-xx-xx — BUG-002: Image displays with wrong aspect ratio / squashed

**Issue**: Uploaded image appears horizontally squashed or stretched when poi is spun.

**Root Cause**: Image width (number of frame slices) was calculated incorrectly. HEIGHT matches the display LED count (`g_displayLeds`, 32 by default). WIDTH is derived from the source aspect ratio: `frame_count = round(original_width * (display_leds / original_height))`.

**Solution**: Re-run image conversion with corrected aspect-ratio formula. Verify output dimensions match expected frame count before uploading.

**Prevention**: The web upload endpoint and `image_converter.py` both apply this formula automatically. If writing a custom pipeline, always derive width from aspect ratio — never hardcode a fixed frame count.

---

### 2025-xx-xx — BUG-003: FastLED APA102 output corrupted without a 5 V level shifter

**Issue**: LEDs display random colors or fail to respond despite correct wiring.

**Root Cause**: Teensy 4.1 GPIO outputs 3.3 V logic; APA102 DATA/CLOCK inputs expect 5 V logic. Without level shifting the signal is unreliable, especially at higher clock speeds or cable lengths over about 10 cm.

**Solution**: Use one SN74AHCT1G125 per signal (data on pin 11, clock on pin 13). VCC = 5 V, OE tied to GND. A MOSFET module plus a dark sacrificial LED is only a fallback: set the sacrificial count to 1. The default count is 0, so all 32 LEDs display.

**Prevention**: Fit both buffers before blaming the strip. Confirm `COLOR_ORDER` (firmware default BGR) with a known red before chasing wiring.

---

### 2025-xx-xx — BUG-004: Serial communication drops frames between Teensy and ESP32

**Issue**: Pattern changes or image data from ESP32 are occasionally missed by Teensy, causing display to freeze or revert.

**Root Cause**: 115200 baud UART between Teensy TX1/RX1 (pins 0-1) and ESP32 RX2/TX2 (GPIO 16/17) lacks flow control. If Teensy is busy in FastLED `show()`, it can miss incoming bytes.

**Solution**: Implement a simple packet framing protocol with start/end markers and ACK. Buffer incoming serial on the Teensy side using an ISR-driven ring buffer so bytes are never dropped during LED update.

**Prevention**: Never rely on raw byte streaming at 115200 without framing. Use length-prefixed packets or delimiter-based framing for all Teensy↔ESP32 messages.

---

### 2025-xx-xx — BUG-005: ESP32-S3 not detected / PSRAM shows 0 bytes

**Issue**: `ESP.getPsramSize()` returns 0 on ESP32-S3 N16R8 board despite PSRAM being physically present.

**Root Cause**: Arduino/PlatformIO board definition defaults to no-PSRAM variant. Must explicitly select the "ESP32S3 Dev Module" board with PSRAM mode set to "OPI PSRAM" (for N16R8).

**Solution**: In Arduino IDE → Tools → PSRAM → OPI PSRAM. In platformio.ini: `board_build.arduino.memory_type = qio_opi`.

**Prevention**: Always verify PSRAM at boot with a printed diagnostic. Add to ESP32 setup(): `Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());` and halt if 0 when PSRAM is required.

---

### 2025-xx-xx — BUG-006: mDNS name is not `povpoi.local`

**Issue**: Browser cannot open a fixed `http://povpoi.local`. The raw IP `192.168.4.1` works on the AP.

**Root Cause**: The firmware sets the hostname to `povpoi-` plus a short device id (`mdnsHostname` in `esp32_firmware.ino`) and prints `http://<that>.local` at boot. There is no `povpoi.local` registration. Windows mDNS is also inconsistent.

**Solution**: Use `http://192.168.4.1` on the AP. Use the hostname from the ESP32 serial log if you want mDNS.

**Prevention**: Do not document a fixed `povpoi.local`.

---

### 2026-11-21 — BUG-007: Closed. There is no `firmware/teensy41/` tree

**Issue**: Notes said a second PlatformIO Teensy firmware at `firmware/teensy41/` had incomplete command processing.

**Root Cause**: That directory is not in the repository. The only Teensy sketch is `firmware/teensy_firmware/teensy_firmware.ino`. PlatformIO builds it from the root `platformio.ini` (`pio run -e teensy41`). Arduino IDE opens the same file. Both paths compile one sketch.

**Solution**: Do not recreate `firmware/teensy41/` to "finish" command processing. Change `firmware/teensy_firmware/teensy_firmware.ino`.

**Prevention**: See ADR-001. PlatformIO is the preferred build. Arduino IDE is an alternate uploader for the same sketch.

---

### 2025-xx-xx — BUG-008: Image buffer 2D array indexing — frame vs. LED axis confusion

**Issue**: Display shows image rotated 90° or scrambled because frame slices and LED indices were swapped in the buffer access code.

**Root Cause**: The image buffer is `CRGB buffer[frame_count][NUM_LEDS]` — first index is the time/column (frame slice), second is the LED row (pixel). Inverting these gives incorrect output.

**Solution**: Always access as `buffer[frame_index][led_index]`. In POV display loop: outer loop over `frame_index` (advances with rotation), inner maps to LED strip positions.

**Prevention**: Add a comment block near the buffer declaration explaining axis convention. The HEIGHT axis (32 LEDs) is always the second index; the WIDTH/time axis is always the first.

---
