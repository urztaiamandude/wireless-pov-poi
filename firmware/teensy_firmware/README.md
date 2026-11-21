# Teensy 4.1 firmware

This directory is the only Teensy firmware. `teensy_firmware.ino` is the sketch. PlatformIO builds it from the repository root:

```bash
pio run -e teensy41
```

Arduino IDE opens the same `.ino` (board Teensy 4.1, USB type Serial, 600 MHz). There is no `firmware/teensy41/` tree.

## What the sketch actually does

- Requires 2× 8 MB PSRAM. `setup()` halts if none is detected.
- 32 APA102 LEDs. `DEFAULT_SACRIFICIAL_LEDS` is 0, so all 32 display. The count is still configurable (command `0x09`, stored in EEPROM). A saved EEPROM record overrides the default.
- `COLOR_ORDER` is `BGR`. On first power-up, show a known red. If it looks blue, change the define to `BRG`.
- `SD_SUPPORT` is defined. Images are `/poi_images/*.pov`. Pattern presets are `/poi_patterns/*.pat`.
- Up to 200 images, each up to 32×400, in external PSRAM.
- Patterns 0–18, sequences, live frames.
- UART to the ESP32-S3 is binary: `0xFF`, command, length, payload, `0xFE`. Pins: RX1 = 0, TX1 = 1, 115200 8N1.

## Not part of the build

These headers are not `#include`d. Each file says so at the top.

- `BatteryMonitor.h` — future INA219 reader
- `BMPImageReader.h` — future BMP loader
- `BMPImageSequence.h` — future BMP playlist

## License

MIT. See [LICENSE](../../LICENSE).
