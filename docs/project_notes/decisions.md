# POV Poi — Architectural Decision Records (ADRs)

Check this file before proposing any architectural change. Entries are numbered; superseded decisions are marked.

---

### ADR-001: One Teensy sketch, PlatformIO preferred

**Date**: Project inception, revised 2026-11-21  
**Status**: Active. Supersedes the claim that `firmware/teensy41/` is a second firmware.

**Context**: Notes described an Arduino sketch and a separate PlatformIO tree at `firmware/teensy41/`. That tree is not in the repository.

**Decision**: `firmware/teensy_firmware/teensy_firmware.ino` is the only Teensy firmware. Build it with PlatformIO from the repository root (`pio run -e teensy41`). Arduino IDE + Teensy Loader is an alternate way to compile and flash the same file. ESP32-S3 builds only from `firmware/esp32_firmware`.

**Alternatives**: Maintaining two Teensy codebases. Rejected. The second tree was never present and the notes about its missing commands were about a path that does not exist.

**Consequences**: Features land in the one sketch. Do not recreate `firmware/teensy41/`.

---

### ADR-002: All 32 LEDs display; sacrificial count defaults to 0

**Date**: Early hardware design, revised 2026-11-21  
**Status**: Active. Supersedes the fixed 31-pixel display and the "LED 32 is the shifter" wording.

**Context**: The strip has 32 APA102s. Teensy GPIO is 3.3 V and the strip wants 5 V clocks and data. An earlier build darkened LED 0 and hoped the first pixel would level-shift the rest. MOSFET shifter modules were a poor fit.

**Decision**: Fit two SN74AHCT1G125 buffers (data and clock). `DEFAULT_SACRIFICIAL_LEDS` is 0, so LEDs 0–31 all display. The count stays runtime-configurable (command `0x09`, web UI, EEPROM) for a fallback build that still uses a sacrificial pixel. `NUM_LEDS` stays 32.

**Alternatives**: Sacrificial LED 0 as the default. Kept only as the documented fallback.

**Consequences**: Image height follows `g_displayLeds` (32 with a fresh EEPROM). Do not hard-code 31.

---

### ADR-003: Image HEIGHT follows the display LED count

**Date**: Early firmware design, revised 2026-11-21  
**Status**: Active

**Context**: POV images must map to the LEDs that are actually lit. Rotation sets how wide the image can look, not how many LEDs exist.

**Decision**: HEIGHT is `g_displayLeds` (32 when the sacrificial count is 0). WIDTH (column count) comes from the source aspect ratio: `frame_count = round(source_width × (display_leds / source_height))`, capped by `IMAGE_MAX_WIDTH` (400 with the PSRAM build).

**Alternatives**: A fixed 31-pixel height. That matched the old sacrificial default and is withdrawn.

**Consequences**: Converters and the upload path must use the active display height. "Width" means time slices along the spin, not a physical pixel pitch.

---

### ADR-004: ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM) is the recommended co-processor variant

**Date**: Mid-project hardware revision  
**Status**: Active

**Context**: Original builds used generic ESP32 WROOM-32 (4 MB Flash, no PSRAM). Expanding image storage and web UI size strained the 4 MB limit.

**Decision**: New builds should use **ESP32-S3 N16R8** (16 MB Flash + 8 MB integrated PSRAM). Existing ESP32 builds are still supported but not recommended for new work.

**Alternatives**: ESP32 with external SPI flash — more complex, higher cost. Staying on ESP32 — limits future image storage growth.

**Consequences**: PlatformIO env `esp32s3` is the default for new builds. PSRAM must be explicitly enabled (OPI PSRAM mode). All new ESP32-S3 code should use `ps_malloc()` for large buffers.

---

### ADR-005: FastLED library for APA102 LED control (Teensy side)

**Date**: Project inception  
**Status**: Active

**Context**: Multiple LED libraries exist for Arduino-compatible platforms.

**Decision**: Use **FastLED** for all LED control on Teensy 4.1. It has mature APA102 SPI support, extensive effects primitives (HSV, palettes), and the team is familiar with its API.

**Alternatives**: Adafruit NeoPixel/DotStar — less performant for APA102 at high frame rates. Raw SPI — more control but more code.

**Consequences**: All pattern code, color manipulation, and LED array operations use FastLED types (`CRGB`, `CRGBPalette16`, `CHSV`, etc.). Do not introduce a competing LED library.

---

### ADR-006: No master/slave hierarchy for multi-POI sync — peer-to-peer only

**Date**: Sync feature implementation  
**Status**: Active

**Context**: When synchronizing multiple poi devices for paired performance use, a master/slave model is the simplest to implement but creates a single point of failure.

**Decision**: All poi devices are **equal peers**. Discovery via mDNS, bidirectional sync of images/patterns/settings. No device is designated as master.

**Alternatives**: Designated master device — simpler protocol but if master fails, all devices lose sync reference.

**Consequences**: Sync protocol must handle conflict resolution without a single authority. Each device operates standalone; sync is an optional enhancement, not a dependency.

---

### ADR-007: WiFi Access Point mode (not STA/router mode) as the primary wireless interface

**Date**: Project inception  
**Status**: Active

**Context**: The poi is a portable device used in performance environments where a known WiFi router may not be available.

**Decision**: ESP32 operates as a **WiFi Access Point** (SSID: `POV-POI-WiFi`, password: `povpoi123`, IP: `192.168.4.1`). Clients connect directly to the poi. No internet or external router required.

**Alternatives**: Station mode (connect to existing WiFi) — requires configuration per environment, impractical at performances. Both modes simultaneously — possible but adds complexity.

**Consequences**: No internet access from the connected device while using poi WiFi. BLE (Nordic UART Service) available simultaneously as a lower-power control channel.

---

### ADR-008: Serial UART at 115200 baud for Teensy↔ESP32 inter-processor communication

**Date**: Project inception  
**Status**: Active

**Context**: Teensy 4.1 and ESP32 are separate processors that must exchange display commands and image data.

**Decision**: Use hardware UART at **115200 bps** (Teensy TX1/pin1 → ESP32 RX2/GPIO16; ESP32 TX2/GPIO17 → Teensy RX1/pin0). Common ground required.

**Alternatives**: SPI between processors — faster but requires more pins and more complex protocol. I2C — too slow for image data.

**Consequences**: Must implement packet framing to avoid data loss (see BUG-004). Frame rate for large image transfers is limited by UART bandwidth; optimize by caching on Teensy side.

---

### ADR-009: EXTMEM (Teensy) and ps_malloc (ESP32-S3) required for all large buffers

**Date**: During PSRAM integration  
**Status**: Active

**Context**: Image frame buffers and other large data structures exceed on-chip SRAM on both processors.

**Decision**: On **Teensy 4.1**, large global arrays must be declared with `EXTMEM`. On **ESP32-S3**, large dynamic allocations must use `ps_malloc()`, never `malloc()` or `new`.

**Alternatives**: Limit buffer sizes to fit in SRAM — restricts image count and resolution. External SPI RAM chip — already provided by PSRAM on both boards.

**Consequences**: All future features requiring significant memory must plan for EXTMEM/ps_malloc from the start. Add boot-time PSRAM size checks.

---

### ADR-010: BLE uses Nordic UART Service (NUS) for cross-platform compatibility

**Date**: BLE implementation  
**Status**: Active

**Context**: BLE control was added as a lower-latency, lower-power alternative to WiFi. Multiple client platforms (Android, iOS, Windows, Web via Chrome/Edge) must be supported.

**Decision**: Use **Nordic UART Service (NUS)** UUIDs (service `6E400001-...`, TX `6E400003-...`, RX `6E400002-...`). This is a well-known pseudo-standard supported by nRF Connect, Flutter BLE libs, and Web Bluetooth.

**Alternatives**: Custom GATT profile — more control but requires custom client code on every platform. HID profile — only suitable for simple input, not bidirectional data.

**Consequences**: All BLE commands follow the NUS framing. Device advertises as `Wireless POV Poi`. BLE and WiFi operate simultaneously.

---

### ADR-011: docs/project_notes/ is the canonical location for project memory

**Date**: 2025 (project memory setup)  
**Status**: Active

**Context**: AI coding assistants (Claude Code, etc.) lose context between sessions. Critical decisions, bugs, and facts need a persistent, version-controlled home.

**Decision**: All project memory lives in `docs/project_notes/` (bugs.md, decisions.md, key_facts.md, issues.md). This directory is part of the repo and looks like standard engineering documentation.

**Alternatives**: A separate `ai-memory/` folder — stigmatized as AI tooling, unlikely to be maintained by non-AI-assisted contributors.

**Consequences**: All team members (human and AI) should consult and update these files. CLAUDE.md links to them so Claude Code auto-loads them at session start.

---
