# Firmware Development

Key knowledge for working on the Teensy 4.1 and ESP32-S3 firmware.

## LED Array — Critical Rule

All 32 LEDs are display pixels (hardware MOSFET level shifter handles 3.3 V → 5 V).

```cpp
// Constants
#define NUM_LEDS          32
#define DISPLAY_LEDS      32
#define DISPLAY_LED_START  0

// ✅ Correct — use all 32 LEDs
for (int i = 0; i < NUM_LEDS; i++) {
  leds[i] = color;
}
```

## Display Modes

| Mode | Value | Description |
|------|-------|-------------|
| Idle | 0 | Off / standby |
| Image | 1 | Stored POV image |
| Pattern | 2 | Animated pattern (types 0-17) |
| Sequence | 3 | Playlist of images/patterns |
| Live | 4 | Real-time drawing from web UI |

## Adding a New Pattern

1. Add a `case` to `displayPattern()` in `teensy_firmware.ino`
2. Update `MAX_PATTERNS` constant
3. Add a button in the ESP32 web interface
4. Test at various speeds (1-255)

```cpp
case 18:  // New pattern
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + i * 8, 255, 255);
  }
  break;
```

## Serial Protocol (Teensy ↔ ESP32)

- Baud: 115200, Teensy TX1/RX1 ↔ ESP32 RX2/TX2
- Simple frame: `[0xFF][CMD][LEN][DATA...][0xFE]`
- Command codes: 0x01=Mode, 0x02=Image, 0x03=Pattern, 0x05=LiveFrame, 0x06=Brightness, 0x07=FrameRate, 0x10=Status, 0x20-0x23=SD

## Image Format

- Height: 32 px (fixed, one per LED)
- Width: variable (max 400 px with PSRAM, 200 px without)
- Storage: `pixels[x][y]` — y maps directly to LED index
- No vertical flip needed

## Code Style

- Functions: `camelCase` (e.g., `displayPattern()`)
- Constants: `UPPER_CASE` (e.g., `NUM_LEDS`)
- Variables: `camelCase` (e.g., `currentPatternIndex`)
- No blocking calls in Teensy loop — timing is critical
