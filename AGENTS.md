# AGENTS.md

Rules for every coding agent on this repo (Cursor, Claude, Codex, Copilot, Gemini). If `CLAUDE.md`, `.github/copilot-instructions.md`, `.claude/skillz/SKILL.md`, or a project-notes file disagrees with this file, follow this file and fix the other one in the same change.

The root [README.md](README.md) is the description of the hardware. When this file and the README disagree about a part, pin, or power path, the README wins and this file gets updated in that same change.

## Source of truth

- Read [README.md](README.md) before changing hardware, firmware, or docs. Agents have been reading it and leaving it stale. If your change makes a sentence in the README wrong, edit the README in the same PR.
- [docs/README.md](docs/README.md) is a map of the docs. It is not a second manual. Do not add a parallel "current hardware" section there.
- Detail lives in these pages. Edit the existing page. Do not add `QUICKSTART.md`, a second architecture doc, or a copy of the README under another name.
  - [docs/HARDWARE_BOM.md](docs/HARDWARE_BOM.md) — parts for one pair
  - [docs/FLASHING.md](docs/FLASHING.md) — PlatformIO, Arduino IDE, Teensy Loader, browser flashing
  - [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
  - [docs/PROTOCOLS.md](docs/PROTOCOLS.md) — Teensy ↔ ESP32 serial frame `0xFF CMD LEN DATA 0xFE`
  - [docs/WIRING.md](docs/WIRING.md)
  - [docs/POWER_SUPPLY_DESIGN.md](docs/POWER_SUPPLY_DESIGN.md)
  - [docs/BATTERY_POWER_INTEGRATION.md](docs/BATTERY_POWER_INTEGRATION.md) and [docs/BATTERY_WIRING_QUICK_REFERENCE.md](docs/BATTERY_WIRING_QUICK_REFERENCE.md)

## Hardware baseline

This is the rebuild Jordan is building. Treat it as current even where an older sentence in the tree still says otherwise.

- Teensy 4.1 with two 8 MB PSRAM chips soldered on. The sketch halts if PSRAM is missing.
- ESP32-S3 DevKitC-1 N16R8 (16 MB flash, 8 MB PSRAM). WiFi/BLE and the web UI only. It does not drive the LEDs.
- UART, 115200: Teensy RX1 pin 0 and TX1 pin 1 ↔ ESP32 GPIO16 (RX) and GPIO17 (TX). This is the only link between the two chips, including SD access.
- 32 APA102 LEDs. `NUM_LEDS` stays 32.
- Level shift: two SN74AHCT1G125 single-gate buffers per poi (SOT-23-5, VCC = 5 V, OE to GND), one for data (Teensy pin 11) and one for clock (pin 13).
- Sacrificial LED count default is 0, and it stays configurable. The MOSFET-plus-sacrificial-LED chain is an alternative, not the default build.
- Portable power: 3S 1300 mAh drone LiPo on XT60 in a quick-swap pod on the spin axis, 10 A switch, INA219 on the handle side of the XT60, Pololu D24V50F5 (5 V, 5 A). No BMS. Bring-up power is a 5 V 4 A bench supply. The INA219 is on Teensy I2C pins 18 (SDA) and 19 (SCL).
- Optional MAX9814 on Teensy A0, powered from 3.3 V.

A hardware change (part, pin, power path, LED count, level shifter) updates all of these in the same PR:

- [README.md](README.md)
- [docs/HARDWARE_BOM.md](docs/HARDWARE_BOM.md)
- [docs/WIRING.md](docs/WIRING.md) and the power docs listed above
- The pin defines in both `firmware/teensy_firmware/` and `firmware/esp32_firmware/`

The Teensy is the only device on the APA102 strip. LED rendering and which LEDs are lit belong in `firmware/teensy_firmware/`. `POST /api/brightness` forwards the value as a raw byte: the ESP32 does not clamp it to 0–255 (a larger integer wraps in the `uint8_t` cast), and the Teensy passes that byte to `FastLED.setBrightness`. `POST /api/power/mode` can lower the stored brightness before forwarding it (`powersave` caps at 180, `ultrasave` at 80). `POST /api/pattern` clamps pattern `index` and `type` to 0–18. `POST /api/hardware/leds` rejects `numLeds` outside 2–32 and a sacrificial count that is not less than `numLeds`. `POST /api/mode` does not range-check mode or index.

## Firmware facts

Checked against the sketches.

- One Teensy sketch: `firmware/teensy_firmware/teensy_firmware.ino`. PlatformIO builds that sketch (`pio run -e teensy41` from the repo root). Arduino IDE can open the same file. There is no second Teensy tree.
- UART is Teensy `Serial1` (pins 0 and 1) ↔ ESP32 `Serial2` (GPIO16 RX, GPIO17 TX) at 115200.
- LED library is FastLED. Do not add a second LED library.
- Large Teensy buffers in the sketch (`images[]`, `cmdBuffer`) are `EXTMEM`. The sketch halts if PSRAM is absent. A 10-image build without PSRAM is not a supported target.
- BLE is the Nordic UART Service, implemented in `firmware/esp32_firmware/src/ble_bridge.cpp`.
- Multi-poi sync is peer-to-peer. The README still treats synchronized behavior as experimental until it is tried on hardware.
- WiFi AP address is `192.168.4.1`. The mDNS hostname is `povpoi-` plus the last six characters of the device id, with colons removed. It is not a fixed `povpoi.local`. Windows often fails to resolve it (BUG-006).
- `SD_SUPPORT` is already defined. Image files are `/poi_images`, patterns are `/poi_patterns`.
- `IMAGE_HEIGHT` is 32 (the array size). How many of those rows are shown is `g_displayLeds`. `DEFAULT_SACRIFICIAL_LEDS` is 0. `MAX_PATTERNS` is 19 (pattern ids 0–18).
- The serial frame is `0xFF CMD LEN DATA 0xFE` only. There is no second checksum framing.
- Microphone defines in the sketch: `AUDIO_PIN A0`, `AUDIO_SAMPLES 64`, `AUDIO_NOISE_FLOOR 50`. Power the MAX9814 from 3.3 V.
- The ESP32 sources do not call `ps_malloc()` or `malloc()`. Do not rewrite allocations unless the change is about memory.

## Project memory

`docs/project_notes/` is the log (`bugs.md`, `decisions.md`, `key_facts.md`, `issues.md`). Procedure: [.claude/skillz/SKILL.md](.claude/skillz/SKILL.md).

Read this file and the README before the notes. If a note disagrees, fix the note. Do not put passwords or tokens in it.

After a bug fix or a hardware decision, add the bug or ADR entry and a line in `issues.md`. A hardware decision still updates the README, the BOM, and the wiring and power docs in the same PR.

## Definition of done

- Docs land in the same PR as the code or hardware change. The README is part of that, not a follow-up.
- Both PlatformIO builds compile when firmware or build config changes:
  - Teensy, from the repo root: `pio run -e teensy41`. The root `platformio.ini` is the Teensy project (`src_dir = firmware/teensy_firmware`).
  - ESP32-S3, only from `firmware/esp32_firmware`: `pio run -e esp32s3`. That `platformio.ini` pins `platform = espressif32@6.9.0` (Arduino-ESP32 2.0.17) and `memory_type = qio_opi`. Do not bump the pin without a successful compile of this sketch. An unpinned install resolves to 8.x and does not compile. The board JSON name still prints "DevKitC-1-N8, No PSRAM"; the image is the 16 MB OPI build.
- A protocol change updates `docs/PROTOCOLS.md` and both sides of the link: the Teensy parser and the ESP32 sender, plus `firmware/esp32_firmware/webui/` when the React app sends that command. The on-device page in the ESP32 sketch uses the same REST API.

## Dead files

Delete an unused file. Do not move it to `archive/`, `old/`, `legacy/`, or a new folder to get it out of the way. Git history is the archive.

Do not add a second document for a topic that already has a page. Edit that page.

`BatteryMonitor.h`, `BMPImageReader.h`, and `BMPImageSequence.h` are unused today. Leave them until a change actually wires them in. Do not relocate them.

## Branches and pull requests

- One branch per task. Small PRs.
- Firmware and firmware build config (anything under `firmware/`, any `platformio.ini`, any `.ino` / `.h` / `.cpp`) always go through a PR. Do not commit that onto `main` directly.
- Firmware changes and docs-only cleanup go in separate PRs.
- Before starting, list open PRs and remote branches. If an open PR already edits the files you need, wait or ask. Do not open a second branch on the same files.
- Rebase on `main` before finishing.
- Delete the branch after the PR merges.

## Working with other agents

- The PR description names the files and areas you touched.
- Leave a short handoff: what changed, what you did not finish, and which open PR this will conflict with.
- Do not edit a file another open PR is already changing unless the owner asked for that overlap.

## Known gotchas

- The Teensy sketch halts when the PSRAM chips are not soldered on. There is no supported 10-image fallback on this board.
- `COLOR_ORDER` is `BGR`. On first power-up, check it against a known red. Many APA102 strips are `BRG`.
- A Teensy with EEPROM magic `0xA5` at address 0, or an ESP32 that already saved `hw_sacLeds`, keeps that saved sacrificial count. The compile-time default does not erase that store.
- The INA219 is in the wiring plan and is not read by the firmware. There is no `/api/battery`.
- `BMPImageReader.h`, `BMPImageSequence.h`, and `BatteryMonitor.h` in `firmware/teensy_firmware/` are not `#include`d.
- Shipped UI is the React app in `firmware/esp32_firmware/webui/` (built to `dist/`) and the PROGMEM page in `esp32_firmware.ino` when SPIFFS has no `index.html`. `web_preview.html` and `test_webui_server.js` are local tools. Fixes to on-device behavior go in the React app or the sketch, not those two files.
- The PROGMEM page stays self-contained (no CDN, relative API paths). The React app is a normal Vite build and is allowed to use npm.

## Cursor Cloud

Build and test commands that work in the cloud environment, with no hardware attached:

| What | Command |
|---|---|
| Teensy firmware | `pio run -e teensy41` from the repo root |
| ESP32-S3 firmware | `cd firmware/esp32_firmware && pio run -e esp32s3` |
| Web UI typecheck | `cd firmware/esp32_firmware/webui && npx tsc --noEmit` |
| Web UI build | `cd firmware/esp32_firmware/webui && npm run build` |
| Web UI dev server | `cd firmware/esp32_firmware/webui && npm run dev` (port 3000, host `0.0.0.0`) |
| Python tests | `cd examples && python3 -m pytest test_*.py -v` |
| Mock API | `node firmware/esp32_firmware/test_webui_server.js` (port 8765) |

`pio` is on `~/.local/bin`. Use `python3`. There is no ESLint config and no pre-commit hook; `tsc --noEmit` is the web UI check. The Vite dev server proxies `/api` to `VITE_API_PROXY_TARGET`, default `http://192.168.4.1`. Appending `-t upload` needs a connected board.
