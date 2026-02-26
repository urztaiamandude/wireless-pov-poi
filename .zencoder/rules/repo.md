---
description: Repository Information Overview
alwaysApply: true
---

# Wireless POV Poi Repository Information

## Repository Summary

The **Wireless POV Poi** (also known as **Nebula Poi**) is a high-performance persistence of vision (POV) LED display system. It uses a dual-microcontroller architecture with a **Teensy 4.1** acting as the main POV engine and an **ESP32-S3** serving as a WiFi/BLE co-processor for wireless control and web-based management.

## Repository Structure

- **teensy_firmware/**: Core POV engine and LED controller firmware for Teensy 4.1.
- **esp32_firmware/**: WiFi/BLE co-processor firmware for ESP32-S3, including an embedded web server.
- **esp32_firmware/webui/**: React-based web interface for controlling the poi system (deployed to ESP32 filesystem).
- **examples/**: Python-based tools for image conversion, testing, and protocol validation.
- **docs/**: Comprehensive documentation covering hardware wiring, API, and protocols.
- **scripts/**: Utility scripts for serial debugging and firmware management.

### Main Repository Components

- **POV Engine (Teensy 4.1)**: Handles time-critical LED refreshing, image rendering from PSRAM/SD, and pattern generation.
- **Wireless Controller (ESP32-S3)**: Manages WiFi AP, BLE connections, and provides a REST API for the Web UI.
- **Web UI**: A modern dashboard for real-time control, image uploads, and system configuration.

## Projects

### Teensy POV Engine

**Configuration File**: [./platformio.ini](./platformio.ini), [./teensy_firmware/teensy_firmware.ino](./teensy_firmware/teensy_firmware.ino)

#### Teensy Language & Runtime

**Language**: C++ (Arduino)  
**Version**: Teensyduino / PlatformIO  
**Build System**: PlatformIO / Arduino IDE  
**Package Manager**: PlatformIO Library Manager

#### Teensy Dependencies

**Main Dependencies**:

- **FastLED@^3.6.0**: High-performance LED animation library.

#### Teensy Build & Installation

```bash
# Build using PlatformIO
pio run -e teensy41

# Upload to Teensy 4.1
pio run -e teensy41 -t upload
```

#### Teensy Testing

**Framework**: Custom Python test scripts
**Test Location**: [./examples/](./examples/)
**Naming Convention**: `test_*.py`
**Run Command**:

```bash
# Example test run
python examples/test_teensy_standalone.py
```

### ESP32-S3 Wireless Co-processor

**Configuration File**: [./esp32_firmware/platformio.ini](./esp32_firmware/platformio.ini), [./esp32_firmware/esp32_firmware.ino](./esp32_firmware/esp32_firmware.ino)

#### ESP32-S3 Language & Runtime

**Language**: C++ (Arduino)  
**Version**: ESP32 Arduino Core  
**Build System**: PlatformIO / Arduino IDE  
**Package Manager**: PlatformIO Library Manager

#### ESP32-S3 Dependencies

**Main Dependencies**:

- **ArduinoJson@^7.2.0**: JSON serialization for API communication.

#### ESP32-S3 Build & Installation

```bash
# Build using PlatformIO
pio run -e esp32s3

# Upload to ESP32-S3
pio run -e esp32s3 -t upload
```

### Web UI

**Configuration File**: [./esp32_firmware/webui/package.json](./esp32_firmware/webui/package.json)

#### Web UI Language & Runtime

**Language**: TypeScript / JavaScript  
**Version**: Node.js 18+  
**Build System**: Vite  
**Package Manager**: npm

#### Web UI Dependencies

**Main Dependencies**:

- **React 19**: UI framework.
- **Lucide React**: Icon library.
- **Tailwind CSS**: Styling framework.

#### Web UI Build & Installation

```bash
cd esp32_firmware/webui
npm install
npm run build
```

#### Web UI Usage & Operations

```bash
# Start development server
npm run dev
```

### Python Tools & Utilities

**Configuration File**: [./examples/requirements.txt](./examples/requirements.txt)

#### Python Tools Language & Runtime

**Language**: Python 3  
**Required Tools**: Python 3.x, pip

#### Key Resources

**Main Files**:

- `image_converter.py`: CLI tool for converting images to POV-compatible format (31px height).
- `image_converter_gui.py`: GUI version of the image converter.

#### Python Tools Usage & Operations

```bash
pip install -r examples/requirements.txt
python examples/image_converter.py input_image.jpg
```

#### Python Tools Validation

**Testing Approach**: Standalone test scripts for validating image conversion, aspect ratios, and protocol logic.
**Run Command**:

```bash
# Run all Python tests
pytest examples/test_*.py -v
```
