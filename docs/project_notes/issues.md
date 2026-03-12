# POV Poi — Work Log & Issue Tracker

Chronological log of completed work, active items, and known remaining tasks.

---

## Status Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Complete |
| 🔄 | In Progress |
| ⏳ | Planned / Backlog |
| ❌ | Blocked |
| 🐛 | Known Bug / Workaround Available |

---

## Completed Milestones

### Core Hardware & Firmware

- ✅ Teensy 4.1 + APA102 32-LED strip wiring with MOSFET level shifter
- ✅ FastLED integration for APA102 SPI control
- ✅ Arduino IDE single-file firmware (`teensy_firmware/`) — production-ready
- ✅ PlatformIO modular firmware (`firmware/teensy41/`) — advanced build (command processing partial)
- ✅ UART serial communication (115200 baud) between Teensy and ESP32
- ✅ Image buffer architecture: `CRGB[frame_count][NUM_LEDS]` with EXTMEM support

### Display Engine

- ✅ POV image display — fixed 31px HEIGHT, variable WIDTH from aspect ratio
- ✅ 16 animated patterns (0–10 basic, 11–15 music-reactive)
- ✅ Sequence mode — chain images and patterns
- ✅ Live mode — real-time web-based drawing
- ✅ Brightness control (0–255)
- ✅ Frame rate control (10–120 FPS)

### ESP32 / ESP32-S3 Firmware

- ✅ WiFi Access Point setup (SSID: POV-POI-WiFi, IP: 192.168.4.1)
- ✅ Web portal — mobile-responsive, PWA support
- ✅ REST API — complete endpoint set (status, mode, brightness, patterns, image upload)
- ✅ Automatic image conversion on upload (any size → 31px height, aspect-ratio width)
- ✅ BLE Nordic UART Service (NUS) — cross-platform BLE control
- ✅ mDNS support (`povpoi.local`)
- ✅ ESP32-S3 N16R8 support (16 MB Flash + 8 MB OPI PSRAM)
- ✅ Pre-loaded demo content (3 images, 5 patterns, 1 sequence)

### Multi-POI Synchronization

- ✅ Peer-to-peer sync protocol (no master/slave)
- ✅ mDNS-based device discovery
- ✅ Bidirectional sync — images, patterns, settings
- ✅ Independent standalone operation preserved

### Image Processing

- ✅ BMPImageReader class — BMP header parsing, line-by-line access
- ✅ BMPImageSequence class — playlist management with durations
- ✅ `imagelist.txt` SD card playlist support
- ✅ GUI image converter (`examples/image_converter_gui.py`)
- ✅ CLI image converter (`examples/image_converter.py`)

### Tooling & Documentation

- ✅ Arduino CLI / shell build scripts (`scripts/`)
- ✅ Comprehensive README with quickstart, API, wiring, troubleshooting
- ✅ ARCHITECTURE.md, FIRMWARE_ARCHITECTURE.md, BLE_IMPLEMENTATION_SUMMARY.md
- ✅ API.md, BLE_PROTOCOL.md, POI_PAIRING.md, BMP_IMAGE_PROCESSING.md, WIRING.md
- ✅ TROUBLESHOOTING.md, TESTING.md, CHANGELOG.md
- ✅ Project memory system (`docs/project_notes/`)

---

## Active / In Progress

- 🔄 Repository consolidation — merging AI Studio dashboard UI into main repo (four-agent integration strategy in progress)
- 🔄 CLAUDE.md / AI agent context documentation — ensuring consistent context across Claude Desktop, GitHub Copilot, and API sessions
- 🔄 Multi-POI synchronization refinement and testing with physical paired devices

---

## Backlog / Planned

- ⏳ IMU/gyroscope integration for automatic rotation detection (removes need for fixed FPS assumptions)
- ⏳ Battery power with charge management circuit
- ⏳ OTA (over-the-air) firmware updates via web portal
- ⏳ Music synchronization improvements (extend 11–15 pattern set)
- ⏳ PlatformIO firmware — complete command processing (bring to parity with Arduino IDE build)
- ⏳ SD card image storage for larger image libraries beyond PSRAM capacity
- ⏳ Image format: consider adding PNG support alongside BMP
- ⏳ Web UI: dark mode / theme options
- ⏳ Web UI: image library management (delete, reorder, rename images)
- ⏳ docs/project_notes/ automatic TOC generation when files exceed ~20 entries

---

## Known Open Issues (No Fix Yet)

- 🐛 PlatformIO Teensy firmware command processing incomplete — workaround: use Arduino IDE firmware
- 🐛 mDNS `povpoi.local` unreliable on Windows without Bonjour — workaround: use `192.168.4.1`
- 🐛 UART between Teensy/ESP32 lacks flow control — workaround: implement packet framing (see BUG-004)

---

## Notes for Future Sessions

- When resuming repo consolidation work, check `plans/` directory for the four-agent integration strategy document
- The `wireless-pov-poi/` subdirectory at repo root may be the legacy standalone version — verify before modifying
- `wireless_pov_poi_app/` contains the Flutter/Dart companion app (3.6% of codebase)
- `.cursor/` at repo root contains Cursor IDE rules — keep in sync with CLAUDE.md if making AI context changes

---
