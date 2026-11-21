# Flashing

Jordan's gaming handhelds are broken and are not a flashing path. Use a normal computer, or a Raspberry Pi, with a data-capable USB cable.

Preferred tool is PlatformIO. Arduino IDE is the alternative. Browser flashing is a third path for the ESP32-S3 only.

Build commands and what they mean are in the [repository README](../README.md#build). This page is only about getting a binary onto the board.

## Teensy 4.1

The bootloader is a separate chip. You do not hold a BOOT button the way you do on an ESP32. If the uploader is waiting, press the button on the Teensy once.

### PlatformIO (preferred)

From the repository root, on Windows, macOS, or Linux:

```bash
pio run -e teensy41 -t upload
```

Root `platformio.ini` sets `upload_protocol = teensy-gui`, which opens PJRC's Teensy Loader.

On a Raspberry Pi, or any machine where the GUI loader is a poor fit, upload with the command-line loader instead. Either export a `.hex` (below) and run [teensy_loader_cli](https://www.pjrc.com/teensy/loader_cli.html), or override the protocol for one upload:

```bash
pio run -e teensy41 -t upload --upload-protocol teensy-cli
```

### Arduino IDE

1. Install [Teensyduino](https://www.pjrc.com/teensy/td_download.html) (or the Teensy board package, depending on IDE version) and the FastLED library.
2. Open `firmware/teensy_firmware/teensy_firmware.ino`.
3. Board: Teensy 4.1. USB Type: Serial. CPU: 600 MHz.
4. Upload.

To hand the file to Teensy Loader yourself: Sketch → Export Compiled Binary, then open the `.hex` in Teensy Loader and press the Teensy button. The same `.hex` is what `teensy_loader_cli` takes on a Pi.

Hex-export notes that used to point at a wiki are in [BUILDING_HEX.md](BUILDING_HEX.md). Command-line notes are in [CLI_COMPILATION.md](CLI_COMPILATION.md). Prefer the commands in the README if those pages disagree.

## ESP32-S3 DevKitC-1

Use the USB-C port that enumerates as the native USB (CDC) device. `platformio.ini` sets `ARDUINO_USB_CDC_ON_BOOT=1`, so the serial console is that same USB port after boot.

If upload cannot find a port: hold BOOT, tap RESET, release BOOT, and retry.

### PlatformIO (preferred)

```bash
cd firmware/esp32_firmware
pio run -e esp32s3 -t upload
```

This environment exists only in `firmware/esp32_firmware/platformio.ini`. Running `-e esp32s3` from the repository root is the wrong project. That file pins `platform = espressif32@6.9.0` and `memory_type = qio_opi`. Do not bump the platform without a successful compile of this sketch.

### Arduino IDE

1. Install the Espressif ESP32 boards package.
2. Open `firmware/esp32_firmware/esp32_firmware.ino` (the `src/` folder next to it must stay put; the sketch includes those headers).
3. Board: ESP32S3 Dev Module.
4. USB CDC On Boot: Enabled. Flash size: 16 MB. PSRAM: OPI PSRAM.
5. Upload. Hold BOOT if the port never appears.

### Browser (ESP Web Tools / esptool-js)

Practical when you already have a PlatformIO build and only a Chromium browser (Chrome, Edge, or Chromium on a Pi) with Web Serial. No handheld required.

1. Build on any machine: `cd firmware/esp32_firmware && pio run -e esp32s3`.
2. Open the esptool-js page, for example <https://espressif.github.io/esptool-js/>.
3. Connect the DevKitC-1 and flash the three files PlatformIO produced under `firmware/esp32_firmware/.pio/build/esp32s3/`.

For this board and the Arduino-ESP32 default 16 MB partition table, the offsets PlatformIO's own esptool step uses are:

| File | Offset |
|---|---|
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `firmware.bin` | `0x10000` |

Confirm those three offsets against the esptool command printed at the end of `pio run -e esp32s3` before you flash. If that log disagrees with this table, trust the log. ESP32-S3 bootloaders sit at `0x0`, not at the ESP32-classic `0x1000`.

The richer React UI is a separate SPIFFS image (`pio run -e esp32s3 -t uploadfs`). The browser steps above flash the firmware only. Without SPIFFS content, the ESP32 still serves the embedded simple page.

## After flashing the Teensy

Open the Teensy USB serial monitor at 115200. You want a PSRAM detection line, not a `FATAL` halt. No PSRAM means the board will sit there forever; solder both chips before debugging LEDs.

Then put a solid red on the strip. If you see blue, `COLOR_ORDER` is wrong for that reel. See the README.
