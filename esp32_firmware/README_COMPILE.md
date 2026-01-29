# Compiling and Uploading ESP32 Firmware

## Quick Start

### Option 1: Using the Batch Script (Easiest)
1. Double-click `build_and_upload.bat` in the `esp32_firmware` folder
2. Wait for compilation and upload to complete

### Option 2: Using PlatformIO Command Line

#### Build only:
```bash
cd esp32_firmware
pio run -e esp32
```

#### Build and upload to COM8:
```bash
cd esp32_firmware
pio run -e esp32 --target upload --upload-port COM8
```

#### Monitor serial output:
```bash
cd esp32_firmware
pio device monitor --port COM8 --baud 115200
```

### Option 3: Using Arduino IDE

1. Open `esp32_firmware.ino` in Arduino IDE
2. Select **Tools > Board > ESP32 Dev Module**
3. Select **Tools > Port > COM8**
4. Select **Tools > Partition Scheme > Default** (or "Minimal SPIFFS" if you have 4MB flash)
5. Select **Tools > Flash Size > 4MB (32Mb)**
6. Click **Upload** button

## Troubleshooting

### If compilation fails:
- Make sure ESP32 board support is installed in Arduino IDE
- For PlatformIO, the ESP32 platform will be downloaded automatically on first build

### If upload fails:
- Check that COM8 is the correct port (check Device Manager)
- Try pressing the BOOT button on ESP32 during upload
- Make sure no other program is using COM8

### If you get "Permission denied" errors:
- Close any serial monitor or other programs using COM8
- Try running as administrator
