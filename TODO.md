# Open work

Checklist for the rebuild on `main` after PRs #206, #207, #208, and #209. Hardware has not been validated end to end. Firmware items are separate pull requests, one area at a time. This file is the working list. Bug detail stays in [docs/project_notes/issues.md](docs/project_notes/issues.md). The battery plan stays in [docs/BATTERY_IMPLEMENTATION_ROADMAP.md](docs/BATTERY_IMPLEMENTATION_ROADMAP.md).

## Recently completed

- [x] Docs match the sketches that actually build (#207): UART pins, routes, 32 LEDs, sacrificial default 0, image slots the ESP32 can reach, pairing and BLE behavior.
- [x] Capacitor app uses the saved poi address (`192.168.4.1` by default) instead of the phone's own host (#206). Display Config can edit it and **Test connection** hits that address.
- [x] Web UI placeholder controls removed. Live draw on the dashboard posts frames to `POST /api/live` (#208).
- [x] GitHub Actions **Android debug APK** workflow builds `pov-poi-debug-apk` (`app-debug.apk` inside the zip) on pushes to `main` (#209).

Checked and not left open: `docs/API.md` does not claim pattern speed is 1–100. It also does not state the range. Firmware stores a `uint8_t` (`patterns[].speed`, comment 1–255). The embedded page slider is 1–255. The React app clamps to 1–255, and to 1–127 for Retro Strobe (pattern 18), because bit 7 is the RGB-mode flag.

## Hardware bring-up and on-device testing

Next, once the parts are here. Bench supply first. The pack has no BMS.

- [ ] Bench bring-up on a 5 V 4 A supply. Flash the Teensy 4.1 (`pio run -e teensy41` from the repo root) and the ESP32-S3 DevKitC-1 N16R8 (`pio run -e esp32s3` from `firmware/esp32_firmware`). Both PSRAM chips have to be soldered on the Teensy; the sketch halts without them.
- [ ] On first light, confirm `COLOR_ORDER` `BGR` against a known red. Many APA102 strips are `BRG`.
- [ ] Download `pov-poi-debug-apk` from a green **Android debug APK** run on `main`, install it, join `POV-POI-WiFi` / `povpoi123`, confirm `192.168.4.1` in Display Config, tap **Test connection**, then try brightness, patterns, image upload, Image Lab, and Live draw.
- [ ] If the APK is not installed, the same address in the phone browser serves the React app when SPIFFS has `/index.html`, and the embedded page when it does not.
- [ ] On the 3S 1300 mAh LiPo, read pack voltage with a meter and keep it above about 10.5 V (3.5 V per cell). The INA219 is in the wiring plan and is not read by firmware. There is no `/api/battery` and no BMS.
- [ ] Teensy pin 13 is the APA102 clock (`CLOCK_PIN`) through one SN74AHCT1G125. The onboard LED is on that same pin, so it follows the LED clock. It is not a status heartbeat.

## Firmware

Each block is its own pull request. A protocol change updates `docs/PROTOCOLS.md`, both sides of the UART link, and the React app when it sends the command.

### Battery monitor (INA219)

`BatteryMonitor.h` is not `#include`d. Address in the header is `0x40`. Wiring is Teensy I2C0: SDA pin 18, SCL pin 19.

- [ ] Compile it into `teensy_firmware.ino` and read the INA219.
- [ ] Replace the hardcoded `batteryCapacity` of 1500 mAh with the 1300 mAh 3S pack.
- [ ] Fix the shunt calibration. The header sets `calibrationValue` to 4096 and comments that as 10 A on a 0.004 Ω shunt (`0.04096 / (Rshunt * Imax)` is not 4096). `getCurrent()` then comments both 2.56 mA/bit and 0.1 A/bit, and multiplies by 0.001.
- [ ] Add a binary UART battery command (`0xFF … 0xFE`), an ESP32 `GET /api/battery` route, a low-battery warning and cutoff, and a readout on both UIs.

### Memory safety and bounds

- [ ] `receiveImage()` accepts height up to `IMAGE_HEIGHT * 2` (64) and indexes `pixels[x][y]` with that same limit. The array is `CRGB pixels[IMAGE_MAX_WIDTH][IMAGE_HEIGHT]` (32 rows). Reject or clamp to 32 before the write.
- [ ] `handleSetFrameRate()` stores any `uint16_t` and sets `cachedFrameDelay` with `(uint8_t)(1000 / frameRate)`. Below 10 FPS that byte overflows. Clamp the route to 10–1000 FPS. The Teensy already treats 1000 as the fastest delay it will run (1 ms).

### UART framing and reliability

The Teensy sends an ack from `parseCommand()`. Most ESP32 routes still answer HTTP `{"status":"ok"}` as soon as the bytes are written. `readTeensyResponse()` is only used on a few SD and status paths. BLE translation can write `Serial2` from the BLE callback while the HTTP handler also writes it.

- [ ] One writer for the Teensy UART.
- [ ] Wait for the Teensy ack (or a real error) before returning success.
- [ ] Harden baud and framing so a dropped `0xFF` / `0xFE` cannot desync the parser.

### SD persistence and PSRAM slots

Images (`.pov` in `/poi_images`) and pattern presets (`.pat` in `/poi_patterns`) are saved. Sequences live in the RAM array `sequences[MAX_SEQUENCES]` only (`receiveSequence()`).

- [ ] Save sequences to the SD card and reload them on boot.
- [ ] PSRAM `MAX_IMAGES` is 200. Boot auto-load fills slots from 5 up toward 200. The ESP32 API and sequence items only use 0–127, because bit 7 of the sequence item byte is the image/pattern flag. Slots 128–199 can be filled and cannot be selected. Either expose them or stop allocating and auto-loading them.
- [ ] `POST /api/sd/load` sets `targetSlot` to 0 when `slot` is omitted, which is a demo image slot. Require a slot, or default to the next user slot (5–127).

### BLE

- [ ] `CC_SET_PATTERN` (`0x04`) only swaps the opcode to Teensy `0x03` and copies the payload. The Teensy reads `slot, type, r1, g1, b1, r2, g2, b2, speed`. A type-first body stores the type in `slot`. See [docs/BLE_PROTOCOL.md](docs/BLE_PROTOCOL.md).
- [ ] `CC_SET_PATTERN_ALL` (`0x06`) sends mode 2 with index `0xFF`. The Teensy stores that index and clears the LEDs. Auto-cycle is not implemented.

### OTA

No `/api/ota/esp32` or `/api/ota/teensy` handler. The React OTA screen was removed on purpose (`firmware/esp32_firmware/webui/README.md`).

- [ ] Add those two endpoints, then put an update screen back. Until then, flash over USB.

### Multi-poi sync

ESP-NOW pairing, sync mode, and auto-pair are RAM-only. Nothing is tried on two poi yet. Detail is in [docs/POI_PAIRING.md](docs/POI_PAIRING.md).

- [ ] Persist pairing across reboot.
- [ ] In mirror mode, copy images (and sequences), not only mode, pattern, brightness, frame rate, and the time sync.
- [ ] In independent mode, ignore incoming `MSG_SET_*` / time-sync from the other poi. Today a paired sender is applied either way.
- [ ] Replace or remove the legacy HTTP routes. `POST /api/sync/push` acks and does not apply the body. `POST /api/sync/execute` reports `settingsUpdated: true` even when nothing was sent.
- [ ] Run the ESP-NOW path on two real poi before treating sync as more than experimental.

### Security

The AP password is `povpoi123`. Routes have no auth.

- [ ] Optional WiFi or API authentication.
- [ ] Tighten `server.enableCORS(true)` in `esp32_firmware.ino` (open CORS on every route).

### Embedded PROGMEM page

`rootPage` in `esp32_firmware.ino` is what `http://192.168.4.1` serves when SPIFFS has no `/index.html`. It is a second UI on the same REST API (pattern speed slider already 1–255, image upload, basic control).

- [ ] Keep it self-contained (no CDN) and either trim it to a small fallback or bring it in line with the React app. Do not let the two drift on commands the sketch actually sends.

## Web app, tooling, and repo cleanup

Not firmware. These can land without a Teensy or ESP32 change. Firmware and docs-only edits stay in separate pull requests.

- [ ] `firmware/esp32_firmware/webui/components/WiringGuide.tsx` draws UART, power, and pins 11/13 straight to the strip. It does not show the two SN74AHCT1G125 gates (VCC 5 V, OE to GND) on those pins.
- [ ] `examples/image_converter.py` and `examples/image_converter_gui.py` still default to 31 rows and comments that say LED 0 is sacrificial. The build default is 32 display LEDs (`DEFAULT_SACRIFICIAL_LEDS` 0). The CLI `--height` help text already says 32 while the variable starts at 31.
- [ ] Decide whether to delete [docs/SD_IMPLEMENTATION_SUMMARY.md](docs/SD_IMPLEMENTATION_SUMMARY.md). It is marked obsolete and describes `firmware/teensy41/`, which is not in the tree. Running SD code is in `teensy_firmware.ino` (`/poi_images`, `/poi_patterns`).
- [ ] State pattern `speed` as 1–255 in `docs/API.md` (see the check in Recently completed). Retro Strobe's 1–127 slider limit belongs in that sentence.
- [ ] Multi-poi controls in the React app after the firmware sync items above work on hardware. The dashboard still seeds one local device card. It does not drive ESP-NOW.
- [ ] `.github/workflows/ai-agent-automation.yml` is `disabled_inactivity` on GitHub. The file still has the Teensy and ESP32 PlatformIO jobs and runs on every pull request when enabled. Re-enable it, or replace it, only if those builds should gate merges again. The dynamic CodeQL scan is a separate, active workflow.
