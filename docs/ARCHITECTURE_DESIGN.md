# Architecture & Design Patterns

**Design decisions, patterns, and architectural guidelines for Nebula POI**

---

## System Architecture Overview

### High-Level Design

```
┌─────────────────────────────────────────────────────────┐
│                    User Layer                            │
│  Web Browser / Mobile App / BLE Client                   │
└────────────────────┬─────────────────────────────────────┘
                     │ WiFi/BLE
┌────────────────────▼─────────────────────────────────────┐
│              Communication Layer                          │
│  ESP32-S3: WiFi AP + Web Server + BLE + Serial           │
└────────────────────┬─────────────────────────────────────┘
                     │ Serial (115200 baud)
┌────────────────────▼─────────────────────────────────────┐
│               Processing Layer                            │
│  Teensy 4.1: Command Processing + Pattern Generation     │
└────────────────────┬─────────────────────────────────────┘
                     │ SPI
┌────────────────────▼─────────────────────────────────────┐
│                Display Layer                              │
│  APA102 LED Strip: 32 LEDs (31 display + 1 shifter)      │
└──────────────────────────────────────────────────────────┘
```

### Design Philosophy

**Separation of Concerns:**
- ESP32: Communication & UI (WiFi, BLE, web serving)
- Teensy: Real-time processing (POV rendering, LED control)
- Reason: Teensy optimized for deterministic timing, ESP32 for connectivity

**Single Responsibility:**
- Each module has one clear purpose
- Minimal coupling between components
- Easy to test and modify independently

---

## Core Design Patterns

### 1. Command Pattern

**Purpose:** Decouple command senders (web UI) from command executors (Teensy)

**Implementation:**
```cpp
// Command structure (implicit)
struct Command {
    String type;         // "MODE", "PATTERN", "BRIGHTNESS", etc.
    String parameters;   // Command-specific data
};

// Command processor (Teensy)
void processCommand() {
    if (Serial1.available()) {
        String cmd = Serial1.readStringUntil('\n');
        
        // Dispatch to appropriate handler
        if (cmd.startsWith("MODE:")) {
            handleModeCommand(cmd);
        } else if (cmd.startsWith("PATTERN:")) {
            handlePatternCommand(cmd);
        }
        // ... etc
    }
}
```

**Benefits:**
- Easy to add new commands
- Commands can be queued/buffered
- Same commands work via WiFi, BLE, or Serial

---

### 2. State Machine Pattern

**Purpose:** Manage display modes and transitions

**States:**
```
IDLE ←→ PATTERN ←→ IMAGE ←→ SEQUENCE ←→ LIVE
  ↓        ↓         ↓         ↓         ↓
Each state has specific behavior and valid transitions
```

**Implementation:**
```cpp
enum DisplayMode {
    MODE_IDLE,
    MODE_PATTERN,
    MODE_IMAGE,
    MODE_SEQUENCE,
    MODE_LIVE
};

DisplayMode currentMode = MODE_IDLE;

void loop() {
    switch (currentMode) {
        case MODE_IDLE:
            displayIdle();
            break;
        case MODE_PATTERN:
            displayPattern();
            break;
        case MODE_IMAGE:
            displayImage();
            break;
        case MODE_SEQUENCE:
            displaySequence();
            break;
        case MODE_LIVE:
            displayLive();
            break;
    }
}
```

**Benefits:**
- Clear state transitions
- Each mode isolated
- Easy to debug (know current state)

---

### 3. Strategy Pattern

**Purpose:** Interchangeable pattern algorithms

**Implementation:**
```cpp
// Pattern interface (implicit)
void displayPattern() {
    switch (currentPattern) {
        case PATTERN_RAINBOW:
            rainbowStrategy();
            break;
        case PATTERN_FIRE:
            fireStrategy();
            break;
        // Each pattern is a different strategy
    }
}

// Individual strategies
void rainbowStrategy() {
    for (int i = 0; i < DISPLAY_LEDS; i++) {
        leds[i] = ColorFromPalette(
            RainbowColors_p,
            (patternCounter + i * 8) & 0xFF
        );
    }
    FastLED.show();
}

void fireStrategy() {
    // Different algorithm, same interface
    heat[random16(DISPLAY_LEDS)]= random8(160, 255);
    // ... fire algorithm
    FastLED.show();
}
```

**Benefits:**
- Easy to add new patterns
- Patterns independent of each other
- Can test patterns in isolation

---

### 4. Observer Pattern (Implicit)

**Purpose:** Web UI updates when state changes

**Implementation:**
```javascript
// JavaScript polls for status
setInterval(async () => {
    const response = await fetch('/api/status');
    const status = await response.json();
    
    // Update UI based on status
    updateBrightnessSlider(status.brightness);
    updateModeDisplay(status.mode);
}, 1000);
```

**Future Enhancement:**
```javascript
// WebSocket for push updates (not yet implemented)
const ws = new WebSocket('ws://192.168.4.1/ws');
ws.onmessage = (event) => {
    const status = JSON.parse(event.data);
    updateUI(status);
};
```

---

### 5. Factory Pattern (for Image Processing)

**Purpose:** Create image objects from different sources

**Implementation:**
```cpp
class BMPImageReader {
public:
    // Factory method
    static BMPImageReader* create(File& file) {
        BMPImageReader* reader = new BMPImageReader();
        if (reader->begin(file)) {
            return reader;
        }
        delete reader;
        return nullptr;
    }
    
    // Common interface
    virtual uint16_t width() const = 0;
    virtual uint16_t height() const = 0;
    virtual uint8_t* getLine(uint8_t* buffer, int y) = 0;
};
```

**Benefits:**
- Abstraction over file formats
- Easy to add new image types (PNG, JPG)
- Consistent interface for image data

---

## Architectural Decisions

### Decision 1: Two-MCU Architecture

**Question:** Why separate ESP32 and Teensy?

**Decision:** Use ESP32 for connectivity, Teensy for real-time control

**Rationale:**
- **Teensy strengths:**
  - Deterministic timing (no WiFi interrupts)
  - Hardware SPI for LEDs
  - More predictable for POV rendering
  
- **ESP32 strengths:**
  - Built-in WiFi + BLE
  - Large flash for web assets
  - Dual-core for parallel tasks

**Alternative considered:**
- Single ESP32: WiFi interrupts cause LED glitches
- Single Teensy: No WiFi/BLE capability

**Conclusion:** Two MCUs = best of both worlds

---

### Decision 2: Serial vs WiFi for LED Commands

**Question:** Should ESP32 directly control LEDs via WiFi?

**Decision:** Serial commands to Teensy, which controls LEDs

**Rationale:**
- Serial more reliable than WiFi direct control
- Teensy can buffer commands
- Works even if WiFi fails
- Lower latency (no network stack overhead)

**Tradeoff:**
- Extra wire between MCUs
- But: simpler, more robust

---

### Decision 3: 31-Pixel Fixed Height

**Question:** Should images be any size and scaled at runtime?

**Decision:** Pre-scale to 31 pixels height, vary width

**Rationale:**
- **Memory constraint:** Teensy has 1MB RAM
- **Processing overhead:** Scaling in real-time = slower
- **Deterministic:** Fixed height = predictable memory usage

**Implementation:**
```cpp
#define DISPLAY_LEDS 31  // Fixed height
int imageWidth = variable;  // Calculated from aspect ratio
```

**Benefits:**
- Simple memory allocation: `width × 31 × 3 bytes`
- Fast image rendering (no scaling needed)
- Predictable performance

---

### Decision 4: Embedded Web UI

**Question:** Separate web server or embedded in firmware?

**Decision:** Embed HTML/CSS/JS directly in ESP32 firmware

**Rationale:**
- **Pro:** No external dependencies
- **Pro:** Works offline (no internet needed)
- **Pro:** Self-contained system
- **Con:** Harder to update UI
- **Con:** Larger firmware size

**Implementation:**
```cpp
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<!-- Entire web UI here -->
</html>
)rawliteral";
```

**Alternative considered:**
- Separate web server → Requires internet, more complex
- Files on SD card → ESP32 doesn't have SD slot

**Conclusion:** Embedded = simplest for standalone device

---

### Decision 5: ASCII vs Binary Protocol

**Question:** Should serial protocol be human-readable ASCII or compact binary?

**Decision:** ASCII text commands

**Rationale:**
- **Debugging:** Can monitor with terminal
- **Simplicity:** Easy to parse and understand
- **Flexibility:** Easy to extend with new commands
- **Tradeoff:** Less efficient than binary

**Example:**
```
ASCII:  "MODE:PATTERN\n"  (14 bytes)
Binary: 0x01 0x02        (2 bytes)
```

**Conclusion:** Debugging benefit > size overhead

**Future:** Could add optional binary mode for large image transfers

---

## Data Flow Patterns

### Pattern 1: Web UI → LED Display

```
User clicks pattern button
    ↓
JavaScript POST to /api/pattern
    ↓
ESP32 receives HTTP request
    ↓
ESP32 sends "PATTERN:0\n" to Teensy via Serial
    ↓
Teensy receives command
    ↓
Teensy updates currentPattern variable
    ↓
Main loop calls displayPattern()
    ↓
Pattern algorithm calculates LED colors
    ↓
FastLED.show() updates APA102 LEDs
    ↓
User sees pattern on POI
```

**Total latency:** ~50-100ms (network + serial + rendering)

---

### Pattern 2: Image Upload

```
User selects image in web UI
    ↓
JavaScript reads file as binary
    ↓
POST to /api/upload with image data
    ↓
ESP32 receives image
    ↓
ESP32 converts to 31px height (auto-scale)
    ↓
ESP32 converts to hex RGB string
    ↓
ESP32 sends "IMAGE:width,31,<hex_data>\n" to Teensy
    ↓
Teensy parses hex data into RGB array
    ↓
Teensy stores in imageBuffer
    ↓
Teensy switches to MODE_IMAGE
    ↓
displayImage() renders from buffer
```

**Bottleneck:** Serial transfer of large images (1-3 seconds for 64×31)

**Optimization:** Could compress or use chunked transfer

---

### Pattern 3: Live Drawing

```
User draws on canvas (touch/mouse)
    ↓
JavaScript captures touch coordinates
    ↓
Convert coordinates to LED column (0-30)
    ↓
Map to RGB color based on color picker
    ↓
Build frame array (31 LEDs × RGB)
    ↓
POST to /api/live with frame data
    ↓
ESP32 receives frame
    ↓
ESP32 sends "LIVE:<hex_data>\n" to Teensy
    ↓
Teensy updates leds[] array
    ↓
FastLED.show() displays immediately
```

**Latency:** ~20-50ms (network optimized)

**Optimization:** WebSocket for lower latency (future)

---

## Memory Management Patterns

### Pattern 1: Static Allocation

**Preferred for fixed-size buffers:**
```cpp
// LED array: known size at compile time
CRGB leds[NUM_LEDS];

// Image buffer: max width × 31 × 3
uint8_t imageBuffer[MAX_IMAGE_WIDTH * 31 * 3];
```

**Benefits:**
- No fragmentation
- Predictable memory usage
- Faster than dynamic allocation

---

### Pattern 2: Dynamic Allocation (Careful)

**Use only when size truly varies:**
```cpp
// For variable-width images
uint8_t* loadImage(int width, int height) {
    int size = width * height * 3;
    uint8_t* buffer = (uint8_t*)malloc(size);
    
    if (buffer == NULL) {
        Serial.println("ERROR: malloc failed");
        return NULL;
    }
    
    // Use buffer...
    
    // MUST free when done
    free(buffer);
    return buffer;
}
```

**Caution:**
- Always check for NULL
- Free when done (avoid leaks)
- Teensy has 1MB - don't allocate too much

---

### Pattern 3: PROGMEM for Constants

**Store constant data in flash, not RAM:**
```cpp
// BAD: Uses 256 bytes of RAM
const uint8_t gamma8[256] = {
    0, 0, 0, 0, ...
};

// GOOD: Uses flash, saves RAM
const uint8_t gamma8[256] PROGMEM = {
    0, 0, 0, 0, ...
};

// Access with pgm_read_byte
uint8_t value = pgm_read_byte(&gamma8[index]);
```

**Use for:**
- Lookup tables
- Fixed patterns
- Large constants

---

## Error Handling Patterns

### Pattern 1: Serial Timeout

```cpp
void processCommand() {
    // Set timeout for serial read
    Serial1.setTimeout(1000);  // 1 second
    
    if (Serial1.available()) {
        String cmd = Serial1.readStringUntil('\n');
        
        if (cmd.length() == 0) {
            // Timeout occurred
            Serial.println("ERROR: Serial timeout");
            return;
        }
        
        // Process command
    }
}
```

---

### Pattern 2: Bounds Checking

```cpp
void setPixel(int x, int y, CRGB color) {
    // Validate bounds
    if (x < 0 || x >= imageWidth) {
        Serial.print("ERROR: x out of bounds: ");
        Serial.println(x);
        return;
    }
    
    if (y < 0 || y >= 31) {
        Serial.print("ERROR: y out of bounds: ");
        Serial.println(y);
        return;
    }
    
    // Safe to access
    imageBuffer[(y * imageWidth + x) * 3] = color.r;
    imageBuffer[(y * imageWidth + x) * 3 + 1] = color.g;
    imageBuffer[(y * imageWidth + x) * 3 + 2] = color.b;
}
```

---

### Pattern 3: Graceful Degradation

```cpp
void displayImage() {
    if (imageBuffer == NULL) {
        // No image loaded - fall back to solid color
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        Serial.println("WARNING: No image, displaying black");
        return;
    }
    
    // Display image normally
    renderImage();
}
```

---

## Performance Patterns

### Pattern 1: Frame Timing

```cpp
void loop() {
    unsigned long frameStart = millis();
    
    // Render frame
    switch (currentMode) {
        case MODE_PATTERN:
            displayPattern();
            break;
        // ... other modes
    }
    
    // Enforce frame rate
    unsigned long elapsed = millis() - frameStart;
    int frameTime = 1000 / targetFPS;
    
    if (elapsed < frameTime) {
        delay(frameTime - elapsed);
    } else {
        // Frame took too long!
        Serial.print("WARNING: Frame overrun by ");
        Serial.print(elapsed - frameTime);
        Serial.println(" ms");
    }
}
```

---

### Pattern 2: Lookup Tables

```cpp
// Pre-calculate expensive operations
uint8_t sinTable[256];

void setup() {
    // Build lookup table once
    for (int i = 0; i < 256; i++) {
        sinTable[i] = (sin(i * 2 * PI / 256) + 1) * 127.5;
    }
}

void loop() {
    // Fast lookup instead of sin() calculation
    for (int i = 0; i < DISPLAY_LEDS; i++) {
        uint8_t brightness = sinTable[(counter + i) & 0xFF];
        leds[i] = CRGB(brightness, 0, 0);
    }
}
```

---

## Testing Patterns

### Pattern 1: Mock Serial (Python)

```python
# Simulate Teensy responses for testing ESP32
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 115200)

while True:
    if ser.in_waiting:
        cmd = ser.readline().decode('utf-8').strip()
        print(f"Received: {cmd}")
        
        # Mock response
        if cmd == "STATUS":
            response = '{"mode":"PATTERN","brightness":128}\n'
            ser.write(response.encode('utf-8'))
```

---

### Pattern 2: Test Mode Flag

```cpp
// Compile-time test mode
#define TEST_MODE 0

#if TEST_MODE
void loop() {
    // Run test sequence
    testAllPatterns();
    testSerialCommands();
    testImageRendering();
}
#else
void loop() {
    // Normal operation
    processCommands();
    updateDisplay();
}
#endif
```

---

## Future Architecture Considerations

### Potential Enhancements

**1. WebSocket Support**
- Lower latency for live mode
- Push updates to UI
- Reduces polling overhead

**2. Multi-POI Sync**
- Mesh networking between devices
- Synchronized patterns
- Leader election algorithm

**3. OTA Updates**
- Update firmware over WiFi
- Requires partition scheme
- Adds ~500KB to firmware size

**4. Advanced Image Compression**
- Run-length encoding
- Reduces serial transfer time
- Adds CPU overhead

---

## Summary: Design Principles

✅ **Key Principles:**
1. **Separation of Concerns** - Each component has one job
2. **Fail Gracefully** - Degrade functionality, don't crash
3. **Optimize Carefully** - Profile before optimizing
4. **Document Decisions** - Future-you will thank you
5. **Test Early** - Catch bugs before hardware integration

---

**Related Documentation:**
- `CLAUDE.md` - AI coding context
- `docs/PROTOCOLS.md` - Communication details
- `docs/DEVELOPMENT.md` - Coding guidelines
- `FIRMWARE_ARCHITECTURE.md` - Firmware comparison
