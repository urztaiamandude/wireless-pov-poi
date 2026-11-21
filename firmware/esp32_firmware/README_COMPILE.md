# Compiling and uploading the ESP32-S3 firmware

Run PlatformIO from **this directory** (`firmware/esp32_firmware`). The root `platformio.ini` builds the Teensy only.

```bash
pio run -e esp32s3
pio run -e esp32s3 -t upload
```

`build_and_upload.bat` does the same upload and then forces `--upload-port COM8`. Change that port before using the script on another machine. If the port never appears, hold BOOT, tap RESET, and release BOOT.

The partition table is `default_16MB.csv` in this directory (4 MB factory app, SPIFFS for the rest of the 16 MB flash). `platformio.ini` points at it.

## Richer web UI (SPIFFS)

The firmware already contains a simple browser page. To serve the React app instead, build it and upload the filesystem:

```bash
cd webui
npm install
npm run build
cd ..
pio run -e esp32s3 -t uploadfs
```

`uploadfs` reads `webui/dist` (`data_dir` in `platformio.ini`).

## Arduino IDE

1. Open `esp32_firmware.ino` in this folder. Leave `src/` next to it.
2. Board: ESP32S3 Dev Module.
3. USB CDC On Boot: Enabled. Flash size: 16 MB. PSRAM: OPI PSRAM.
4. Upload.

## Serial monitor

After boot the USB CDC port is the console (`ARDUINO_USB_CDC_ON_BOOT=1`). 115200 baud. The mDNS name printed there is `povpoi-<id>.local`, not `povpoi.local`. On the access point, use `http://192.168.4.1`.
