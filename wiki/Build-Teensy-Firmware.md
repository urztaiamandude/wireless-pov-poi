# Teensy 4.1 Firmware Build

The recommended Teensy firmware lives at
`teensy_firmware/teensy_firmware.ino`.

## Arduino IDE (recommended)

Prerequisites:
- Arduino IDE 1.8.x or 2.x
- Teensyduino: https://www.pjrc.com/teensy/td_download.html
- FastLED library (install via Library Manager)

Steps:
1. Open `teensy_firmware/teensy_firmware.ino`.
2. Select **Tools > Board > Teensy 4.1**.
3. Select **Tools > USB Type > Serial**.
4. Select **Tools > CPU Speed > 600 MHz**.
5. Click **Upload**.

## PlatformIO

The root `platformio.ini` includes a `teensy41` environment:

```bash
# From repo root
pio run -e teensy41
pio run -e teensy41 -t upload
```

Serial monitor:

```bash
pio device monitor --port /dev/ttyACM0 --baud 115200
```

## HEX file output

If you prefer using Teensy Loader, build a HEX file:

- Arduino IDE: **Sketch > Export Compiled Binary**
- PlatformIO scripts: `./scripts/build_teensy_hex.sh` or
  `scripts\build_teensy_hex.bat`

See [Build-Teensy-HEX](Build-Teensy-HEX.md) for full details.

## Notes

- The Teensy serial baud rate is 115200.
- PlatformIO copies HEX output to `build_output/teensy41_firmware.hex`.
