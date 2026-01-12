# Copilot Instructions for Wireless POV POI System

## Project Overview

This is a wireless persistence of vision (POV) LED poi system featuring:
- **Teensy 4.1**: High-performance main controller for LED display and POV rendering
- **ESP32**: WiFi co-processor for wireless communication and web interface
- **APA102 LED Strip**: 32 addressable RGB LEDs (31 for display, 1 for level shifting)
- **Serial Communication**: 115200 baud between Teensy and ESP32

The system creates stunning POV light displays with wireless control through a web portal.

## Architecture

### System Components
- **Teensy Firmware** (`teensy_firmware/`): LED control, POV rendering, pattern generation using FastLED
- **ESP32 Firmware** (`esp32_firmware/`): WiFi AP, web server, REST API, serial communication
- **Documentation** (`docs/`): Setup guides, wiring diagrams, API reference
- **Examples** (`examples/`): Image converter tools, Android app
- **Scripts** (`scripts/`): Setup verification and utility scripts

### Data Flow
```
User → Web Interface → ESP32 (WiFi/HTTP) → Serial → Teensy → FastLED → APA102 LEDs
```

## Technology Stack

### Firmware
- **Platform**: Arduino framework via PlatformIO
- **Languages**: C++ for embedded systems
- **Libraries**:
  - FastLED 3.5.0+ for Teensy LED control
  - ESP32 built-in WiFi and web server libraries
- **Build Tool**: PlatformIO (preferred) or Arduino IDE with Teensyduino

### Web Interface
- **Frontend**: HTML5, CSS3, vanilla JavaScript (no frameworks)
- **Design**: Mobile-responsive, PWA-capable, touch-optimized
- **Protocol**: HTTP REST API with JSON

### Tools
- **Image Conversion**: Python with PIL/Pillow
- **Mobile App**: Android (Java/Kotlin)

## Code Standards

### General Principles
- Keep changes minimal and focused
- Follow existing code style
- Comment complex logic but avoid obvious comments
- Test thoroughly before submitting
- Update documentation when changing functionality

### C++ Firmware (Teensy & ESP32)

#### Style Guidelines
- Use descriptive variable names (e.g., `ledBrightness`, not `b`)
- Use `camelCase` for variables and functions
- Use `UPPER_CASE` for constants and defines
- Keep functions focused and short (ideally < 50 lines)
- Avoid blocking operations in main loop

#### Memory Management
- Be mindful of RAM constraints on microcontrollers
- Use `PROGMEM` for large constant data when appropriate
- Avoid dynamic memory allocation where possible
- Prefer static buffers with known sizes

#### Serial Communication
- Always validate serial data before processing
- Use binary protocol for efficiency (not text)
- Handle partial reads and buffer overflows gracefully
- Document protocol format in comments

#### FastLED Usage (Teensy)
```cpp
// Efficient pattern: minimize calls to show()
// Note: LED 0 is used for level shifting, display uses LEDs 1-31
for (int i = 1; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;  // Set display pixels
}
FastLED.show();  // Single update

// Avoid: calling show() in loop
```

### Web Interface (ESP32)

#### HTML/CSS/JavaScript Style
- Use modern JavaScript (ES6+)
- Keep UI responsive and mobile-first
- Test on mobile devices (phones and tablets)
- Minimize external dependencies
- Follow accessibility best practices (ARIA labels, semantic HTML)

#### API Design
- Validate all API inputs
- Return appropriate HTTP status codes
- Use JSON for request/response bodies
- Keep response times fast (< 100ms typical)
- Document all endpoints in `docs/API.md`

#### Error Handling
```cpp
// ESP32 firmware pattern
if (!validateInput(data)) {
    server.send(400, "application/json", "{\"error\":\"Invalid input\"}");
    return;
}
// Process valid data...
```

### Python Scripts

#### Style
- Follow PEP 8 style guide
- Use type hints for function signatures
- Include docstrings for modules, classes, and functions
- Handle exceptions gracefully with user-friendly messages

#### Image Processing
- Support common formats (PNG, JPG, GIF)
- Maintain aspect ratio when resizing
- Provide preview before conversion
- Output in POV-compatible format (31 pixels wide)

## Build and Test

### Building Firmware

#### PlatformIO (Preferred)
```bash
# Build Teensy firmware
pio run -e teensy41

# Build ESP32 firmware
pio run -e esp32

# Upload to device
pio run -e teensy41 --target upload
```

#### Arduino IDE
- Teensy: Select "Teensy 4.1" board, "Serial" USB type
- ESP32: Select "ESP32 Dev Module" board
- Verify code compiles without warnings

### Testing

#### Hardware Testing
- Test on actual hardware before submitting PRs
- Verify all display modes (Idle, Image, Pattern, Sequence, Live)
- Test WiFi connectivity and web interface
- Check serial communication reliability
- Verify LED patterns display correctly

#### Software Testing
- Code must compile without errors or warnings
- Test edge cases (invalid inputs, boundary conditions)
- Verify no memory leaks or buffer overflows
- Test with various image sizes and formats

### Code Checks
```bash
# PlatformIO static analysis
pio check

# Specific tools
pio check --tool cppcheck
pio check --tool clangtidy
```

## Documentation Standards

### Code Comments
```cpp
/**
 * Brief function description
 * 
 * @param brightness LED brightness level (0-255)
 * @param pattern Pattern type (0=rainbow, 1=wave, etc.)
 * @return true if successful, false on error
 */
bool setPattern(uint8_t brightness, uint8_t pattern);
```

### Markdown Files
- Use clear headings and structure
- Include code examples where helpful
- Keep formatting consistent with existing docs
- Update relevant docs when changing functionality

### API Documentation
- Document all REST endpoints in `docs/API.md`
- Include request/response examples
- Show curl command examples
- Note any breaking changes

## Common Patterns

### Serial Protocol (Teensy ↔ ESP32)
```cpp
// ESP32 sends commands as binary packets
// Format: [CMD][LEN][DATA...]
// CMD: 1 byte command code
// LEN: 1 byte data length
// DATA: variable length payload
```

### LED Pattern Implementation (Teensy)
```cpp
// Pattern functions follow this structure:
void displayPattern() {
    for (int i = 1; i < NUM_LEDS; i++) {  // Start at 1 (0 is level shifter)
        // Calculate color based on pattern type
        leds[i] = calculateColor(i);
    }
    FastLED.show();
}
```

### Web API Handler (ESP32)
```cpp
// REST endpoint handlers follow this pattern:
server.on("/api/endpoint", HTTP_POST, []() {
    // 1. Parse request
    String body = server.arg("plain");
    
    // 2. Validate input
    if (!isValid(body)) {
        server.send(400, "application/json", errorResponse("Invalid input"));
        return;
    }
    
    // 3. Process request
    bool success = processRequest(body);
    
    // 4. Send response
    server.send(success ? 200 : 500, "application/json", response);
});
```

## Safety and Security

### Power Safety
- Document power requirements (5V, 2-3A minimum)
- Warn about looking directly at LEDs at full brightness
- Ensure proper insulation of electrical connections

### Input Validation
- Always validate user inputs from web interface
- Sanitize file uploads (images)
- Check array bounds before access
- Validate numeric ranges (brightness 0-255, FPS 10-120)

### Code Security
- No hardcoded secrets or credentials (use examples/placeholders)
- Sanitize all user-provided content before display
- Validate image dimensions before processing

## Contribution Workflow

### Before Starting
1. Read `CONTRIBUTING.md` for detailed guidelines
2. Check existing issues to avoid duplicates
3. Discuss approach for significant changes

### Development Process
1. Fork repository and create feature branch
2. Make focused changes with clear commits
3. Test thoroughly on hardware if possible
4. Update documentation as needed
5. Submit pull request with clear description

### Commit Messages
```
Type: Brief description (50 chars max)

Detailed explanation if needed
- Reference issues: Fixes #123
```

**Types**: `Add:`, `Fix:`, `Update:`, `Docs:`, `Style:`, `Refactor:`, `Test:`, `Chore:`

## Project Files

### Key Files
- `README.md`: Project overview and quick start
- `QUICKSTART.md`: 30-minute setup guide
- `ARCHITECTURE.md`: System design and diagrams
- `CONTRIBUTING.md`: Contribution guidelines
- `TROUBLESHOOTING.md`: Common problems and solutions
- `platformio.ini`: Build configuration

### Firmware Entry Points
- `teensy_firmware/teensy_firmware.ino`: Teensy main code file
- `esp32_firmware/esp32_firmware.ino`: ESP32 main code file

### Important Directories
- `docs/`: All documentation files
- `examples/`: Example code and tools
- `scripts/`: Utility scripts

## Getting Help

1. Check documentation in `docs/` folder
2. Review `TROUBLESHOOTING.md` for common issues
3. Search existing GitHub issues
4. Create new issue with detailed information
5. Include serial monitor output when reporting bugs

## Tips for Working with This Codebase

- **Teensy is time-critical**: LED updates must be fast and non-blocking
- **ESP32 handles user interaction**: Web server and API should be responsive
- **Serial protocol is binary**: Not human-readable, but efficient
- **LED 0 is special**: Used for level shifting, not part of display
- **Images are 31px wide**: LED 1-31 used for POV display (0 is level shifter)
- **Test on hardware**: Simulator cannot replicate POV effects
- **Mobile-first design**: Most users will control via phone
- **Power matters**: Full brightness on all LEDs draws significant current

## Performance Considerations

- **Teensy Frame Rate**: Target 60+ FPS for smooth POV display
- **ESP32 Response Time**: API calls should complete in < 100ms
- **Serial Bandwidth**: 115200 baud sufficient for control, not real-time video
- **Memory Limits**: Teensy has more RAM (1MB) than ESP32 (520KB)
- **WiFi Range**: ~30m typical, design for occasional disconnects

## Future Enhancements

Consider these when making changes:
- IMU/gyroscope for rotation detection
- Battery power with charge management
- SD card storage for more images
- OTA firmware updates
- Multiple poi synchronization

---

**Remember**: This is a hardware project. Changes should be tested on actual hardware when possible. Safety first! 🎨✨
