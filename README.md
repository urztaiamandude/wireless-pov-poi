# Nebula Poi

For coding agents: see [AGENTS.md](AGENTS.md).

Wireless persistence-of-vision (POV) poi. A Teensy 4.1 drives a 32-LED APA102 strip. An ESP32-S3 hosts WiFi, BLE, and the control UIs, and talks to the Teensy over a binary UART protocol.

This README is the source of truth for the current rebuild. If another document disagrees with this file or with the firmware, trust this file and the code. The hardware is being rebuilt from scratch and has not been fully validated end to end.

Parts for one pair: [docs/HARDWARE_BOM.md](docs/HARDWARE_BOM.md).  
How to flash: [docs/FLASHING.md](docs/FLASHING.md).  
Doc map: [docs/README.md](docs/README.md).

## Current hardware

| Item | Decision |
|---|---|
| POV engine | Teensy 4.1. **2× 8 MB PSRAM must be soldered on.** The firmware halts if PSRAM is missing. |
| Co-processor | ESP32-S3 DevKitC-1 (N16R8: 16 MB flash, 8 MB PSRAM) |
| LEDs | APA102 strip, 60 LEDs/m, cut to 32. All 32 are display pixels. |
| Level shift | **2× SN74AHCT1G125** per poi (one gate each for data and clock). VCC = 5 V, OE tied to GND, Teensy 3.3 V in, 5 V out to the strip. |
| Sacrificial LEDs | **Default 0.** Still configurable at runtime (web UI → Advanced Settings → LED hardware, stored on the Teensy in EEPROM). A sacrificial LED plus a MOSFET shifter is only a fallback if the buffer is not fitted. |
| Color order | Firmware `COLOR_ORDER` is **BGR**. On first power-up, light a known red. If red and blue are swapped, change it to `BRG` in `firmware/teensy_firmware/teensy_firmware.ino` and reflash. Many APA102 strips are BRG. |
| Bring-up power | 5 V, 4 A bench supply. |
| Portable power | 3S 1300 mAh drone LiPo, XT60, quick-swap pod on the spin axis. **No BMS.** Balance-charge the pack off the poi. Pololu **D24V50F5** (5 V, 5 A) buck. INA219 on the **handle side** of the XT60. 10 A switch in the battery positive. |
| Microphone | Optional MAX9814, powered from 3.3 V, output to Teensy **A0**. |
| Storage | microSD in the Teensy 4.1 slot. `SD_SUPPORT` is compiled in. Images live in `/poi_images` (`.pov`), pattern presets in `/poi_patterns` (`.pat`). |
| Bulk capacitance | 1000 µF across 5 V at the regulator output and at the strip. |

Wiring: [docs/WIRING.md](docs/WIRING.md). Power: [docs/POWER_SUPPLY_DESIGN.md](docs/POWER_SUPPLY_DESIGN.md). PSRAM soldering: [docs/PSRAM_INSTALLATION.md](docs/PSRAM_INSTALLATION.md).

The INA219 is part of the hardware plan. `BatteryMonitor.h` is **not** included by the sketch, and there is no battery API in the running firmware. Until that is integrated, watch pack voltage with a meter. Do not rely on the UI for cutoff. There is no BMS to do it for you.

A Teensy that already has LED config in EEPROM (magic byte `0xA5` at address 0) keeps that saved sacrificial count instead of the compile-time default. Set it from the web UI, or clear that EEPROM byte, on a board that was programmed with the old default of 1.

## Architecture

```
Phone or browser
    │  WiFi  POV-POI-WiFi / povpoi123 / 192.168.4.1
    ▼
ESP32-S3  (web server, BLE, REST)
    │  UART 115200 8N1
    │  Teensy pin 1 TX1 → ESP32 GPIO 16 RX
    │  Teensy pin 0 RX1 ← ESP32 GPIO 17 TX
    ▼
Teensy 4.1  (POV, FastLED, SD, PSRAM)
    │  pin 11 data, pin 13 clock, through SN74AHCT1G125
    ▼
APA102 × 32
```

The Teensy is the only chip wired to the LEDs. `POST /api/brightness` forwards a raw byte and does not clamp it. `POST /api/power/mode` can lower that brightness (`powersave` 180, `ultrasave` 80). `POST /api/pattern` clamps pattern index and type to 0–18. `POST /api/hardware/leds` accepts `numLeds` from 2 to 32 only when the sacrificial count is lower than that.

The Teensy↔ESP32 link is a **binary** frame: `0xFF`, command, length, payload, `0xFE`. It is not newline-terminated text. Command bytes and the HTTP API are in [docs/PROTOCOLS.md](docs/PROTOCOLS.md) and [docs/API.md](docs/API.md).

## Two control UIs, one API

Keep both. They call the same ESP32 REST API.

1. **Simple browser app**, embedded in `firmware/esp32_firmware/esp32_firmware.ino` as the PROGMEM page `rootPage`. The ESP32 serves it at `http://192.168.4.1` when SPIFFS has no `/index.html`. Use it for pairing, image upload, and basic control. It has to stay self-contained: no CDN, no build step.
2. **Richer app**, `firmware/esp32_firmware/webui/` (React, Vite, Capacitor for Android/iOS). Build it and upload the `dist/` files to SPIFFS (`pio run -e esp32s3 -t uploadfs` from `firmware/esp32_firmware`). When `/index.html` is on SPIFFS, that app replaces the embedded page.

`firmware/esp32_firmware/web_preview.html` and `firmware/esp32_firmware/test_webui_server.js` are local dev tools. They are not what the ESP32 serves.

WiFi AP: `POV-POI-WiFi` / `povpoi123` / `http://192.168.4.1`.  
mDNS is **not** a fixed `povpoi.local`. The firmware starts `http://povpoi-<id>.local`, where `<id>` comes from the device id, and prints the name on the ESP32 serial console at boot. Use the IP if mDNS does not resolve.

## Repository layout

| Path | What it is |
|---|---|
| `firmware/teensy_firmware/teensy_firmware.ino` | The only Teensy firmware. |
| `firmware/teensy_firmware/BatteryMonitor.h` | Not included. Future INA219 monitor. |
| `firmware/teensy_firmware/BMPImageReader.h` | Not included. Future BMP loader. |
| `firmware/teensy_firmware/BMPImageSequence.h` | Not included. Future BMP playlist. |
| `firmware/esp32_firmware/` | ESP32-S3 firmware, embedded page, and `webui/`. |
| `docs/` | Documentation. Start here only after this README. |
| `examples/` | Python image tools and tests. |
| `platformio.ini` | **Teensy build only.** |

There is no `firmware/teensy41/` tree. Do not document one.

## Build

PlatformIO is the preferred build. Arduino IDE still works; see [docs/FLASHING.md](docs/FLASHING.md).

Teensy 4.1, from the repository root (this `platformio.ini` has `src_dir = firmware/teensy_firmware`):

```bash
pio run -e teensy41
```

ESP32-S3, from its own project. Do not build it from the root `platformio.ini`. That file's source directory is the Teensy tree.

```bash
cd firmware/esp32_firmware
pio run -e esp32s3
```

That project's `platformio.ini` pins `platform = espressif32@6.9.0` (Arduino-ESP32 2.0.17) and sets `board_build.arduino.memory_type = qio_opi` for the N16R8. Leave the pin. Platform 8.x does not compile this sketch. PlatformIO still prints the board name "DevKitC-1-N8, No PSRAM"; the image is the 16 MB OPI build.

Richer web UI:

```bash
cd firmware/esp32_firmware/webui
npm install
npm run build
cd ..
pio run -e esp32s3 -t uploadfs
```

### Android debug APK

GitHub Actions builds an installable debug APK of the Capacitor app. Android Studio is not required.

The workflow runs on every push to `main`, on pull requests that change `firmware/esp32_firmware/webui/` or the workflow file, and when someone starts it with **Run workflow**.

1. Open the **Actions** tab and choose **Android debug APK**.
2. Open a green run. The newest run on `main` is the current app.
3. Download the **pov-poi-debug-apk** artifact. GitHub wraps it in a zip; the file inside is `app-debug.apk`.
4. Copy that APK to the phone and open it. Android asks you to allow installs from this source (the files app or browser you used). Allow it, then install.

It is a debug build. The debug keystore is created by the Android Gradle plugin, so the workflow has no signing secrets. Join `POV-POI-WiFi` (`povpoi123`) before using the app with a poi.

The ESP32 also serves this app at `http://192.168.4.1` when the built web UI is on SPIFFS. Open that address in the phone browser if you would rather not install the APK. When SPIFFS has no `/index.html`, that same address serves the smaller page embedded in the ESP32 firmware.

Image tools:

```bash
cd examples
python3 -m pytest test_*.py -v
```

## What is already in the firmware

Images, patterns 0–18 (including music-reactive 11–15 when a mic is fitted), sequences, brightness, frame rate, SD image and pattern files, BLE, a REST API, live frames, and multi-poi sync code. The Teensy PSRAM array is 200 images. The ESP32 API and sequence playback only reach slots 0–127. Sync is experimental until it is tried on two real poi: ESP-NOW pairing is not saved across reboot, mirror mode does not copy images, and the older HTTP sync route does not apply what it receives. OTA updates are not implemented. Battery monitoring is not implemented.

## License

MIT. See [LICENSE](LICENSE).
