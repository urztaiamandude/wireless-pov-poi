# System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         NEBULA POI                                        │
└─────────────────────────────────────────────────────────────────────────┘

                              ┌──────────────┐
                              │   USER       │
                              │ Phone/Laptop │
                              └──────┬───────┘
                                     │
                                WiFi │ 192.168.4.1
                                     │
                              ┌──────▼───────┐
                              │    ESP32     │
                              │  WiFi AP     │
                              │ Web Server   │
                              │  REST API    │
                              └──────┬───────┘
                                     │
                        Serial(TX/RX)│ 115200 baud
                                     │
                              ┌──────▼───────┐
                              │  Teensy 4.1  │
                              │ POV Engine   │
                              │   FastLED    │
                              └──────┬───────┘
                                     │
                            SPI(11,13)│ DATA/CLOCK
                                     │
                              ┌──────▼───────┐
                              │  APA102 LED  │
                              │  Strip (32)  │
                              │ LED 0: Level │
                              │ LED 1-31: ✨ │
                              └──────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                           DATA FLOW                                      │
└─────────────────────────────────────────────────────────────────────────┘

User Action → Web Interface → HTTP Request → ESP32 Web Server
    ↓
ESP32 Processes Request
    ↓
ESP32 → Serial Command → Teensy 4.1
    ↓
Teensy Processes Command
    ↓
Update LED State → FastLED → APA102 LEDs
    ↓
Visual Output (POV Display) ✨

┌─────────────────────────────────────────────────────────────────────────┐
│                        FEATURE OVERVIEW                                  │
└─────────────────────────────────────────────────────────────────────────┘

┌────────────────┐  ┌────────────────┐  ┌────────────────┐
│   PATTERNS     │  │    IMAGES      │  │   LIVE MODE    │
│                │  │                │  │                │
│  🌈 Rainbow    │  │  📷 Upload     │  │  🎨 Draw       │
│  🌊 Wave       │  │  📐 31x64 max  │  │  🖱️ Canvas     │
│  🎨 Gradient   │  │  🖼️ PNG/JPG    │  │  ⚡ Real-time  │
│  ✨ Sparkle    │  │  🔄 Convert    │  │  📱 Touch      │
└────────────────┘  └────────────────┘  └────────────────┘

┌────────────────┐  ┌────────────────┐  ┌────────────────┐
│   CONTROLS     │  │   WIRELESS     │  │    API         │
│                │  │                │  │                │
│  💡 Brightness │  │  📡 WiFi AP    │  │  🔌 REST       │
│  ⚡ Frame Rate │  │  🌐 Web Portal │  │  📲 JSON       │
│  🎮 Modes      │  │  🔐 Password   │  │  🌐 Web UI     │
│  🎬 Sequences  │  │  📶 30m range  │  │               │
└────────────────┘  └────────────────┘  └────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                      HARDWARE STACK                                      │
└─────────────────────────────────────────────────────────────────────────┘

 Layer 4: User Interface
 ┌─────────────────────────────────────────┐
│  Web Browser                            │
 │  HTML5 + CSS3 + JavaScript              │
 └─────────────────────────────────────────┘
                    ▲│▼
 Layer 3: Network
 ┌─────────────────────────────────────────┐
 │  WiFi 2.4GHz (802.11 b/g/n)            │
 │  HTTP Protocol                          │
 └─────────────────────────────────────────┘
                    ▲│▼
 Layer 2: Control
 ┌─────────────────────────────────────────┐
 │  ESP32 @ 240MHz (Dual Core)            │
 │  WiFi Stack + Web Server                │
 └─────────────────────────────────────────┘
                    ▲│▼
 Layer 1.5: Communication
 ┌─────────────────────────────────────────┐
 │  Serial UART (115200 baud)              │
 │  Binary Protocol                        │
 └─────────────────────────────────────────┘
                    ▲│▼
 Layer 1: Processing
 ┌─────────────────────────────────────────┐
 │  Teensy 4.1 @ 600MHz (ARM Cortex-M7)   │
 │  POV Engine + FastLED                   │
 └─────────────────────────────────────────┘
                    ▲│▼
 Layer 0: Display
 ┌─────────────────────────────────────────┐
 │  APA102 LED Strip (32 RGB LEDs)        │
 │  SPI Interface (20 MHz capable)         │
 └─────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                       POWER DISTRIBUTION                                 │
└─────────────────────────────────────────────────────────────────────────┘

        ┌─────────────────┐
        │  5V Power Supply│
        │     2-3A        │
        └────────┬────────┘
                 │
        ┌────────┴────────┐
        │                 │
        ▼                 ▼
 ┌──────────┐      ┌──────────┐
 │ Teensy   │      │  ESP32   │
 │ + APA102 │      │          │
 │  (~2A)   │      │ (~200mA) │
 └──────────┘      └──────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                      SIGNAL CONNECTIONS                                  │
└─────────────────────────────────────────────────────────────────────────┘

Teensy Pin 11 ════════════════► APA102 DATA
Teensy Pin 13 ════════════════► APA102 CLOCK

Teensy TX1 (Pin 1) ═══════════► ESP32 RX2 (GPIO 16)
Teensy RX1 (Pin 0) ◄═══════════ ESP32 TX2 (GPIO 17)

Common GND ═══════════════════► All Components

┌─────────────────────────────────────────────────────────────────────────┐
│                      PERFORMANCE METRICS                                 │
└─────────────────────────────────────────────────────────────────────────┘

  Metric              │ Value                │ Notes
──────────────────────┼──────────────────────┼───────────────────────
  Frame Rate          │ 10-120 FPS           │ Adjustable
  Response Time       │ < 50ms               │ Command to LED
  LED Count           │ 32 (31 display)      │ Expandable
  Image Resolution    │ 31 × 64 pixels       │ Maximum
  WiFi Range          │ ~30 meters           │ Typical
  Power Consumption   │ 2-3A @ 5V            │ Max brightness
  Serial Baudrate     │ 115200 bps           │ Reliable
  Web Clients         │ 1 active             │ HTTP limitation

┌─────────────────────────────────────────────────────────────────────────┐
│                          FILE STRUCTURE                                  │
└─────────────────────────────────────────────────────────────────────────┘

wireless-pov-poi/
├── 📱 teensy_firmware/        # Teensy 4.1 firmware (443 lines)
├── 📡 esp32_firmware/         # ESP32 firmware (744 lines)
├── 📚 docs/                   # Complete documentation
│   ├── README.md             # Setup guide
│   ├── WIRING.md             # Wiring diagrams
│   └── API.md                # API reference
├── 🎨 examples/               # Example code and tools
│   └── image_converter.py    # Python image tool
├── 🔧 scripts/                # Utility scripts
│   └── verify_setup.sh       # Setup verification
├── 📖 README.md               # Project overview
├── 🚀 QUICKSTART.md           # 30-minute setup
├── 🔍 TROUBLESHOOTING.md      # Problem solving
├── 🤝 CONTRIBUTING.md         # Contribution guide
├── 📝 CHANGELOG.md            # Version history
├── ⚖️ LICENSE                 # MIT License
└── ⚙️ platformio.ini          # PlatformIO config

Legend: 📱 Firmware | 📡 Network | 📚 Docs | 🎨 Examples | 🔧 Tools
```
