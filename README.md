# Wireless POV Poi — Nebula Poi

Welcome to the wireless POV Poi project - a creative venture that combines art and technology! This project aims to build a wireless LED poi that displays patterns and animations when spun.

## Features

### Core
- **Wireless Control**: WiFi AP mode (SSID: `POV-POI-WiFi`) with optional connection to existing networks (STA mode), plus BLE via Nordic UART Service
- **Customizable Patterns**: 18 built-in animated patterns with configurable dual colors and speed
- **Rechargeable Battery**: Long-lasting 5V battery with power management profiles

### Display Modes
- **Image Display**: Upload and display POV images (up to 400×32 pixels with PSRAM)
- **Pattern Display**: 18 animated patterns including rainbow, fire, plasma, comet, meteor, and more
- **Audio-Reactive Patterns**: 5 music-reactive patterns (VU meter, pulse, audio rainbow, center burst, sparkle) driven by an optional MAX9814 microphone
- **Sequence/Playlist Mode**: Chain images and patterns with per-item durations and looping
- **Live Drawing Mode**: Real-time interactive canvas drawing streamed to the LEDs

### Web Interface
- **Responsive Web UI**: Mobile-optimized dark-theme interface with 7 tabs (Dashboard, Patterns, Image Lab, Live Draw, Multi-Poi, SD Card, Settings)
- **PWA Support**: Installable Progressive Web App with Service Worker for offline caching
- **Procedural Image Generator**: Create organic and geometric patterns in-browser and upload directly
- **REST API**: Full programmatic control via REST endpoints for mode, brightness, frame rate, patterns, images, live frames, SD card, and multi-poi sync
- **mDNS Discovery**: Access the web UI at `http://povpoi-XXXXX.local` when connected to the same network

### Hardware & Storage
- **PSRAM Support**: Optional 16 MB expanded memory on Teensy 4.1 for up to 200 stored images
- **SD Card Storage**: Save and load images and pattern presets to the Teensy 4.1 built-in microSD slot
- **5 Built-in Demo Images**: Smiley face, rainbow spectrum, heart, starburst, and nebula spiral ship pre-loaded

### Multi-Poi Synchronization
- **ESP-NOW Pairing**: Peer discovery, pairing, and heartbeat-based status tracking
- **Mirror & Independent Modes**: Synchronized playback across poi or independent per-device control
- **Sync Time Offset**: Phase alignment so paired poi animate in unison

### Controls & Configuration
- **Brightness Control**: Adjustable 0–255 via slider or API
- **Frame Rate Control**: Adjustable 10–250 FPS
- **Power Modes**: Four profiles (Performance, Balanced, Power Save, Ultra Save) that scale CPU frequency, max FPS, and brightness caps
- **Device Configuration**: Customizable device name, sync group, and auto-sync settings persisted across reboots
- **WiFi Network Manager**: Scan, connect, and disconnect from external WiFi networks with saved credentials

## Installation

1. Clone the repository:  
   `git clone https://github.com/urztaiamandude/wireless-pov-poi.git`
2. Navigate into the project directory:  
   `cd wireless-pov-poi`
3. Install the required packages:  
   `npm install`

## Usage

1. Connect the poi to your device.
2. Use the mobile application to select designs.
3. Spin your poi to see the magic!

## Start Here (First Release)

If you are setting up your first physical pair of poi, follow:

- `docs/FIRST_PAIR_QUICKSTART.md` (beginner-friendly setup and validation checklist)
- `docs/POI_PAIRING.md` (pairing and synchronization details)

## Contributing

We welcome contributions from the community. Please fork the repository and create a pull request.

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Acknowledgements

- Inspired by the beauty of motion and light.
- Special thanks to all contributors and supporters.

*Spin on.* 🎨✨
