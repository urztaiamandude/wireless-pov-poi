# ESP32-S3 on this poi

The co-processor in the current build is an **ESP32-S3 DevKitC-1 N16R8** (16 MB flash, 8 MB PSRAM). It runs WiFi, BLE, and the web UI. It does not drive the APA102 strip. The Teensy 4.1 does that.

This page used to treat the S3 as an optional upgrade from an original ESP32, and it offered GPIO 43/44 as an alternate Teensy UART. That wiring is not what the firmware uses. Do not move the link there.

## UART to the Teensy

The only link is `Serial2` at 115200 8N1. The pins are compile-time checked in `firmware/esp32_firmware/esp32_firmware.ino`:

| ESP32-S3 | Direction | Teensy 4.1 |
|---|---|---|
| GPIO 16 (`SERIAL_RX_PIN`) | RX ← Teensy TX | Pin 1 (TX1) |
| GPIO 17 (`SERIAL_TX_PIN`) | TX → Teensy RX | Pin 0 (RX1) |

```cpp
#define TEENSY_SERIAL Serial2
TEENSY_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
// SERIAL_RX_PIN is 16, SERIAL_TX_PIN is 17
```

Frames are binary: `0xFF CMD LEN DATA 0xFE`. See [PROTOCOLS.md](PROTOCOLS.md).

GPIO 43 and GPIO 44 are the ESP32-S3 UART0 pins (often tied to the USB-serial console). The sketch does not open them for the Teensy. GPIO 16 and 17 are not the boot strapping pins (GPIO 0 and GPIO 46).

## What the module is for

- WiFi access point `POV-POI-WiFi` at `http://192.168.4.1`, plus an optional STA client configured from the web UI.
- HTTP API in [API.md](API.md). CORS is enabled with no origin allow-list. There is no authentication.
- BLE Nordic UART Service. See [BLE_PROTOCOL.md](BLE_PROTOCOL.md).
- ESP-NOW pairing. Peers are not saved across reboot. See [POI_PAIRING.md](POI_PAIRING.md).

Images and patterns are forwarded to the Teensy. The ESP32 does not keep an image gallery in PSRAM. The upload handler uses a static RGB buffer of `MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3` (400 × 64 × 3). There is no `imageBuffer[31 * 64 * 3]` in the current sketch.

## Build

From `firmware/esp32_firmware`, not from the repository root:

```bash
pio run -e esp32s3
```

`platformio.ini` in that directory pins `platform = espressif32@6.9.0` and `board_build.arduino.memory_type = qio_opi`. Leave the pin. An unpinned `espressif32` resolves to 8.x and does not compile this sketch. PlatformIO may print the board name "DevKitC-1-N8, No PSRAM"; the image is still the 16 MB OPI build.

## Known gaps

- OTA updates are not implemented. Extra flash does not, by itself, provide an update path.
- Battery monitoring is not implemented. There is no `/api/battery`. The INA219 is on the Teensy I2C pins and is not read.
- Multi-poi sync is in the sketch and has not been proven on two spinning poi.
- The ESP32 PSRAM is not used for image storage. `ps_malloc()` is not called.

## Parts that are not this build

Adafruit Feather and SparkFun Thing Plus boards are different pinouts. This repository's firmware and wiring are the DevKitC-1 N16R8 with GPIO 16 and 17 free for the Teensy. An original ESP32, or an ESP32-S2, is not the board the sketch is built for.
