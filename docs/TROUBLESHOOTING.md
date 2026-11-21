# Troubleshooting

Short checks for the current hardware. The README is the source of truth for what the build is supposed to be.

## Teensy prints FATAL and the LEDs stay off

PSRAM is required. The sketch halts when `external_psram_size` is 0, because the image buffers are placed in external RAM. Solder both 8 MB chips. [PSRAM_INSTALLATION.md](PSRAM_INSTALLATION.md).

## LEDs are dark, random, or flicker

- Bench supply is 5 V and can deliver 4 A. The strip is powered from the supply or the buck, not from the Teensy pin.
- Common ground between supply, Teensy, ESP32-S3, both SN74AHCT1G125 devices, and the strip.
- Each buffer: VCC = 5 V, OE = GND, A from Teensy pin 11 (data) or pin 13 (clock), Y to the strip.
- 1000 µF across 5 V at the strip.
- `COLOR_ORDER` is BGR. A red test that comes out blue means change it to BRG and reflash. That is a channel-order issue, not a wiring fault.

Sacrificial LED 0 is not the default. If an older EEPROM config saved `sacrificial = 1`, LED 0 stays black until you set sacrificial LEDs to 0 in the web UI.

## Colors are wrong but the pattern is right

Change `COLOR_ORDER` in `firmware/teensy_firmware/teensy_firmware.ino` from `BGR` to `BRG` (or the reverse) and reflash the Teensy only.

## No WiFi network

- ESP32-S3 is flashed and powered from 5 V.
- AP name `POV-POI-WiFi`, password `povpoi123`, then `http://192.168.4.1`.
- The hostname is `povpoi-<id>.local`, printed at boot. There is no fixed `povpoi.local`.

## Web UI is the simple page, or the rich app, and you expected the other

The ESP32 serves SPIFFS `/index.html` when that file exists, and the embedded page otherwise. Upload the React build with `pio run -e esp32s3 -t uploadfs` from `firmware/esp32_firmware` to get the rich app. Erase SPIFFS, or never upload it, to get the embedded page. Both speak the same REST API.

## Teensy ignores the web UI

UART is crossed and only these pins:

- Teensy pin 1 (TX1) → ESP32-S3 GPIO 16 (RX)
- Teensy pin 0 (RX1) ← ESP32-S3 GPIO 17 (TX)
- GND joined

115200 8N1. Frames are `0xFF … 0xFE`, not text lines. GPIO 18 is not the link.

## Image looks upside down or mirrored

[POV_DISPLAY_ORIENTATION_GUIDE.md](POV_DISPLAY_ORIENTATION_GUIDE.md). LED 0 is the bottom of the image when sacrificial LEDs are 0.

## Battery reading never appears

Expected. `BatteryMonitor.h` is not part of the build, and there is no `/api/battery`. Use a meter. The pack has no BMS.

## More detail

- Wiring: [WIRING.md](WIRING.md)
- Power: [POWER_SUPPLY_DESIGN.md](POWER_SUPPLY_DESIGN.md)
- First pair bring-up: [FIRST_PAIR_QUICKSTART.md](FIRST_PAIR_QUICKSTART.md)
- Orientation: [ORIENTATION_TROUBLESHOOTING.md](ORIENTATION_TROUBLESHOOTING.md)
