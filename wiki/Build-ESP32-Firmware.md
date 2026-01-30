# ESP32 Firmware Build

The ESP32 firmware is located at `esp32_firmware/esp32_firmware.ino`.

## Arduino IDE

### ESP32 (WROOM-32 and similar)

1. Open `esp32_firmware/esp32_firmware.ino`.
2. Select **Tools > Board > ESP32 Dev Module**.
3. Select the correct serial port.
4. Set **Partition Scheme > Default** (or "Minimal SPIFFS" for 4MB flash).
5. Set **Flash Size > 4MB (32Mb)**.
6. Click **Upload**.

### ESP32-S3

1. Open `esp32_firmware/esp32_firmware.ino`.
2. Select **Tools > Board > ESP32S3 Dev Module**.
3. Select the correct serial port.
4. Set **USB CDC On Boot > Enabled**.
5. Set **Partition Scheme > 16MB Flash (3MB APP/9.9MB FATFS)**.
6. Click **Upload**.

## PlatformIO

### Root `platformio.ini`

```bash
# From repo root
pio run -e esp32
pio run -e esp32s3

# Upload
pio run -e esp32 -t upload
pio run -e esp32s3 -t upload
```

### Local `esp32_firmware/platformio.ini`

```bash
cd esp32_firmware
pio run -e esp32
pio run -e esp32 -t upload
```

## Windows batch script

For ESP32 (Windows only), you can use:

```cmd
esp32_firmware\build_and_upload.bat
```

## Serial monitor

```bash
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

## Troubleshooting

- If upload fails, hold the BOOT button during upload.
- If you see SPIFFS mount errors, try a different partition scheme.
