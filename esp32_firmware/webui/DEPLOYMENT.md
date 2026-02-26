# Deploying Web UI to ESP32 Filesystem

This guide explains how to build and deploy the React web UI to your ESP32's SPIFFS or LittleFS filesystem for standalone operation.

## Overview

The ESP32 serves the web UI as static files from its internal filesystem (SPIFFS or LittleFS). This allows the web interface to work without an external web server, making the POV system completely self-contained.

## Prerequisites

- Built web UI (see main README.md)
- ESP32 firmware with web server enabled
- Filesystem uploader tool for your development environment

## Step 1: Build Production Bundle

```bash
cd webui
npm run build
```

This creates an optimized bundle in `dist/` with:

- Single JavaScript chunk (no code splitting)
- Minified assets
- Compressed for embedded systems

**Expected output size:** ~300-500 KB (depends on features included)

## Step 2: Prepare ESP32 Filesystem

### Option A: Arduino IDE

1. **Install SPIFFS/LittleFS Plugin**
   - Download the [ESP32 Sketch Data Upload](https://github.com/me-no-dev/arduino-esp32fs-plugin) plugin
   - Extract to `<Arduino>/tools/ESP32FS/tool/`
   - Restart Arduino IDE

2. **Create data directory**

   ```bash
   cd ../esp32_firmware
   mkdir -p data
   ```

3. **Copy web UI files**

   ```bash
   cp -r ../webui/dist/* data/
   ```

4. **Upload filesystem**
   - In Arduino IDE: Tools > ESP32 Sketch Data Upload
   - Wait for upload to complete (30-60 seconds)

### Option B: PlatformIO

1. **Configure platformio.ini**

   Ensure your `esp32_firmware/platformio.ini` includes:

   ```ini
   [env:esp32]
   board_build.filesystem = littlefs
   platform = espressif32
   framework = arduino
   ```

2. **Copy web UI to data directory**

   ```bash
   cd ../esp32_firmware
   mkdir -p data
   cp -r ../webui/dist/* data/
   ```

3. **Upload filesystem**

   ```bash
   pio run --target uploadfs
   ```

### Option C: esptool.py (Advanced)

For direct filesystem image upload:

1. **Install esptool**

   ```bash
   pip install esptool
   ```

2. **Create filesystem image** (using mklittlefs or mkspiffs)

   ```bash
   # For LittleFS:
   mklittlefs -c data -s 0x100000 littlefs.bin

   # For SPIFFS:
   mkspiffs -c data -s 0x100000 spiffs.bin
   ```

3. **Flash to ESP32**

   ```bash
   esptool.py --port /dev/ttyUSB0 write_flash 0x310000 littlefs.bin
   ```

   (Adjust address based on your partition table)

## Step 3: Configure ESP32 Firmware

Ensure the ESP32 firmware serves static files from the filesystem:

```cpp
// In esp32_firmware.ino setup()
if (!SPIFFS.begin(true)) {
  Serial.println("SPIFFS Mount Failed");
  return;
}

// Serve static files
server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
```

Or for LittleFS:

```cpp
if (!LittleFS.begin(true)) {
  Serial.println("LittleFS Mount Failed");
  return;
}

server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
```

## Step 4: Verify Deployment

1. **Connect to POV-POI-WiFi**
   - SSID: `POV-POI-WiFi`
   - Password: `povpoi123`

2. **Access web UI**
   - Open browser to `http://192.168.4.1`
   - Or use mDNS: `http://povpoi.local`

3. **Check browser console**
   - Look for any 404 errors (missing files)
   - Verify API calls are reaching `/api/*` endpoints

## Filesystem Size Optimization

### Compression

Enable GZIP compression in ESP32 firmware:

```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/index.html", "text/html", false, processor);
}).setFilter(ON_STA_FILTER);

// Enable compression for static files
server.serveStatic("/", SPIFFS, "/")
  .setDefaultFile("index.html")
  .setCacheControl("max-age=600");
```

Pre-compress files before upload:

```bash
cd dist
gzip -k *.js *.css
# Upload both .gz and original files
```

### Asset Optimization

Reduce bundle size:

1. **Remove unused features**
   - Comment out unused components in App.tsx
   - Rebuild: `npm run build`

2. **Optimize images**

   ```bash
   # If you add images to the UI
   npm install -D imagemin imagemin-pngquant
   ```

3. **Tree-shaking**
   - Vite automatically removes unused code
   - Check bundle analysis: `npm run build -- --mode analyze`

## Partition Table Configuration

For larger web UIs, increase SPIFFS/LittleFS partition size:

### Option 1: Use Predefined Table

In `platformio.ini`:

```ini
board_build.partitions = min_spiffs.csv  # 1.9MB app, ~190KB SPIFFS
# or
board_build.partitions = default.csv     # 1.2MB app, ~1.5MB SPIFFS
```

### Option 2: Custom Partition Table

Create `partitions_custom.csv`:

```csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x140000,
app1,     app,  ota_1,   0x150000,0x140000,
spiffs,   data, spiffs,  0x290000,0x170000,
```

Reference in `platformio.ini`:

```ini
board_build.partitions = partitions_custom.csv
```

## Troubleshooting

### "SPIFFS Mount Failed"

- Filesystem not formatted: Flash with `-f` flag or use `SPIFFS.format()`
- Wrong partition table: Verify partition addresses
- Corrupted filesystem: Reflash filesystem image

### "404 Not Found" for static files

- Files not copied to data directory
- Filesystem not uploaded after copying files
- Incorrect SPIFFS/LittleFS path in firmware code

### Web UI loads but broken

- Missing assets (check browser network tab)
- API endpoints not matching (update DEVICE_IP in components)
- CORS issues (ensure firmware allows API requests)

### Out of memory during upload

- Reduce bundle size (remove unused components)
- Increase partition size (see Partition Table section)
- Use GZIP compression

## Automated Deployment Script

Create `deploy.sh` in webui directory:

```bash
#!/bin/bash
set -e

echo "Building web UI..."
npm run build

echo "Copying to ESP32 data directory..."
mkdir -p ../esp32_firmware/data
rm -rf ../esp32_firmware/data/*
cp -r dist/* ../esp32_firmware/data/

echo "Uploading filesystem..."
cd ../esp32_firmware

# For PlatformIO:
pio run --target uploadfs

# For Arduino IDE, prompt user:
# echo "Upload filesystem in Arduino IDE: Tools > ESP32 Sketch Data Upload"

echo "Deployment complete!"
```

Make executable:

```bash
chmod +x deploy.sh
./deploy.sh
```

## OTA Filesystem Updates (Future)

Once OTA endpoints are implemented, you'll be able to update the web UI wirelessly:

```bash
# Build and upload via HTTP
npm run build
curl -F "file=@dist/index.html" http://192.168.4.1/api/ota/filesystem
```

This feature requires backend implementation in ESP32 firmware.

## Best Practices

1. **Test locally first**: Always test with `npm run dev` before deploying
2. **Version control**: Tag web UI versions matching firmware versions
3. **Backup filesystem**: Save working filesystem images before updates
4. **Monitor size**: Keep total bundle under 1MB for standard partitions
5. **Cache control**: Set appropriate cache headers in firmware

## Resources

- [ESP32 SPIFFS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html)
- [LittleFS for ESP32](https://github.com/lorol/LITTLEFS)
- [Vite Build Optimization](https://vitejs.dev/guide/build.html)
- [Arduino ESP32 Filesystem](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPIFFS)
