# Architecture Decisions

Key design choices for the Wireless POV Poi system and the reasoning behind them.

---

## Dual-Microcontroller Design

**Decision**: Use separate Teensy 4.1 (POV engine) and ESP32-S3 (WiFi/BLE) rather than a single MCU.

**Rationale**:
- Teensy 4.1's 600 MHz ARM Cortex-M7 provides deterministic, real-time LED control critical for POV timing
- ESP32-S3 handles WiFi AP, web server, and BLE without impacting display timing
- Serial UART (115200 baud) bridge keeps the two loosely coupled
- Either firmware can be updated independently

**Trade-offs**: Added wiring complexity and power draw vs. rock-solid POV rendering.

---

## APA102 over WS2812B

**Decision**: Use APA102 (SPI) LED strip instead of WS2812B (single-wire).

**Rationale**:
- SPI clock line allows much higher data rates than WS2812B's 800 kHz protocol
- No strict timing requirements — SPI is tolerant of interrupts
- Hardware clock means reliable communication even at high CPU loads
- Global brightness control per LED

**Trade-offs**: Extra wire (clock), slightly higher cost per LED.

---

## All 32 LEDs as Display Pixels

**Decision**: Use an external MOSFET level shifter so all 32 LEDs are available for display (DISPLAY_LED_START=0, DISPLAY_LEDS=32).

**Rationale**:
- Maximizes vertical resolution for POV images
- External level shifting (3.3 V → 5 V) is more reliable than sacrificing LED 0

**Note**: Earlier prototypes reserved LED 0 for level shifting; that approach was superseded by the hardware shifter.

---

## Two Firmware Build Systems

**Decision**: Maintain both Arduino IDE (`teensy_firmware/`) and PlatformIO (`firmware/teensy41/`) builds.

**Rationale**:
- Arduino IDE version is the production-ready, single-file firmware — easiest to modify and upload
- PlatformIO version offers modular architecture for advanced development and CI builds
- Two separate `platformio.ini` files (root for Teensy, `esp32_firmware/` for ESP32) to avoid cross-contamination

**Caveat**: Root `platformio.ini` has `src_dir = teensy_firmware` — always run ESP32 builds from inside `esp32_firmware/`.

---

## React Web UI on ESP32 SPIFFS

**Decision**: Ship a Vite-built React + TypeScript + Tailwind CSS bundle on ESP32's SPIFFS filesystem.

**Rationale**:
- Modern, responsive UI accessible from any phone/browser over WiFi
- Single-chunk build (~325 KB) fits comfortably in SPIFFS
- Falls back to embedded `rootPage` PROGMEM string if SPIFFS is unavailable

---

## Image Height Fixed at 32 px

**Decision**: All POV images are exactly 32 pixels tall (one pixel per display LED); width is variable.

**Rationale**:
- Simplifies storage format: `pixels[x][y]` where y maps directly to an LED index
- Upload endpoint auto-resizes to 32 px height, preserving aspect ratio
- Width is limited by available RAM (up to 400 px with PSRAM, 200 px without)
