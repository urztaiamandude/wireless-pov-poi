# Copilot instructions

Follow [AGENTS.md](../AGENTS.md). It is the authoritative ruleset. The root [README.md](../README.md) is the hardware description. When this file disagrees with either, AGENTS.md and the README win.

Build from the commands in AGENTS.md. The ESP32-S3 build is `cd firmware/esp32_firmware && pio run -e esp32s3`. Do not run `pio run -e esp32s3` from the repo root. That project's `platformio.ini` pins `espressif32@6.9.0`.

## Style

C++ in the firmware: `camelCase` functions and locals, `PascalCase` types, `UPPER_CASE` constants. LED loops start at `g_displayLedStart` and run for `g_displayLeds`. No blocking calls in the Teensy display loop.

Python in `examples/`: PEP 8, type hints, Pillow for images.

TypeScript in `firmware/esp32_firmware/webui/`: PascalCase components, `camelCase` functions, hooks prefixed with `use`, Tailwind utility classes (PostCSS, not a CDN stylesheet). Debounce slider and text changes before calling the ESP32 API.

## Still easy to get wrong

These match [AGENTS.md](../AGENTS.md).

- One Teensy sketch, `firmware/teensy_firmware/teensy_firmware.ino`. PlatformIO builds it. There is no second Teensy tree.
- UART is Teensy `Serial1` pins 0/1 ↔ ESP32 `Serial2` GPIO16/17 at 115200.
- 32 APA102s. Sacrificial count default is 0 and stays configurable.
- PSRAM is required. Large Teensy buffers are `EXTMEM`. The sketch halts without the chips.
- LED library is FastLED. Serial frames are `0xFF … 0xFE` only. `SD_SUPPORT` is already on (`/poi_images`, `/poi_patterns`).
- BLE is the Nordic UART Service. Multi-poi sync is peer-to-peer.
- AP address `192.168.4.1`. mDNS name is `povpoi-` plus a short id, not `povpoi.local`.
- The Teensy firmware owns the LEDs. The ESP32 forwards values and does not enforce LED limits.
- On-device UI fixes go in `firmware/esp32_firmware/webui/` or the PROGMEM page in `firmware/esp32_firmware/esp32_firmware.ino`. `web_preview.html` and `test_webui_server.js` are not flashed.
- REST shape and the serial frame are documented in [docs/API.md](../docs/API.md) and [docs/PROTOCOLS.md](../docs/PROTOCOLS.md). Change the doc in the same PR as the code.
- WiFi credentials stay in the ESP32 sketch. Do not copy them into this file, the web UI, or `docs/project_notes/`.
