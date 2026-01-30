# Teensy HEX Files

This page explains how to compile the Teensy 4.1 firmware into HEX format
for use with the Teensy Loader application.

## Prerequisites

- Arduino IDE 1.8.x or 2.x with Teensyduino and FastLED, or
- PlatformIO (install via `pip install platformio`)

## Arduino IDE (export HEX)

1. Open `teensy_firmware/teensy_firmware.ino`.
2. Select **Tools > Board > Teensy 4.1**.
3. Select **Tools > USB Type > Serial**.
4. Select **Tools > CPU Speed > 600 MHz**.
5. Go to **Sketch > Export Compiled Binary**.

Output:
`teensy_firmware/teensy_firmware.ino.teensy41.hex`

## PlatformIO (scripts)

Linux/macOS:

```bash
./scripts/build_teensy_hex.sh
```

Windows:

```cmd
scripts\build_teensy_hex.bat
```

Output:
`build_output/teensy41_firmware.hex`

## PlatformIO (manual)

```bash
pio run -e teensy41 --target clean
pio run -e teensy41
```

Output locations:
- `.pio/build/teensy41/firmware.hex`
- `build_output/teensy41_firmware.hex`

## Load HEX with Teensy Loader

1. Open Teensy Loader: https://www.pjrc.com/teensy/loader.html
2. **File > Open HEX File**, select your HEX file.
3. Press the white button on the Teensy 4.1 board.
4. Click **Program** (or auto-program) and then **Reboot** if needed.

## Troubleshooting

- "Teensy board not found": install Teensyduino and restart Arduino IDE.
- "FastLED library not found": install FastLED via Library Manager.
- "HEX file not compatible": ensure the target board is Teensy 4.1.
