# Teensy CLI Compilation

This page covers CLI workflows for the Teensy 4.1 firmware.

## Arduino CLI

Install:

- Linux:
  `curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh`
- macOS: `brew install arduino-cli`
- Windows: `winget install ArduinoSA.CLI`

Setup:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
arduino-cli core update-index
arduino-cli core install teensy:avr
arduino-cli lib install FastLED
```

Compile:

```bash
arduino-cli compile \
  --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std \
  teensy_firmware/teensy_firmware.ino \
  --output-dir teensy_firmware/build
```

Output:
`teensy_firmware/build/teensy_firmware.ino.hex`

Upload:

```bash
arduino-cli upload \
  --fqbn teensy:avr:teensy41 \
  --port /dev/ttyACM0 \
  teensy_firmware/teensy_firmware.ino
```

## PlatformIO CLI

Install:

```bash
pip install platformio
```

Build:

```bash
pio run -e teensy41
```

Upload:

```bash
pio run -e teensy41 -t upload
```

Build scripts:

```bash
./scripts/build_arduino_cli.sh
./scripts/build_teensy_hex.sh
```

Windows scripts:

```cmd
scripts\build_arduino_cli.bat
scripts\build_teensy_hex.bat
```

## CI quick hint

The PlatformIO build can run headless in CI:

```bash
pip install platformio
pio run -e teensy41
```
