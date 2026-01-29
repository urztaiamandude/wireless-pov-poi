# Testing Teensy Without ESP32

This guide shows you how to test your Teensy 4.1 POV Poi firmware without the ESP32 connected.

## Quick Test Options

### Option 1: Use Built-in Demo Patterns (Easiest)

The firmware includes built-in demo patterns and images that are created on startup. You can activate them by sending commands via USB Serial.

**Steps:**
1. Upload the firmware to your Teensy
2. Open Serial Monitor (Arduino IDE or PlatformIO) at **115200 baud**
3. You should see: `"Teensy 4.1 Nebula Poi Ready!"`
4. The firmware listens for commands on `Serial1` (pins 0/1), but you can temporarily modify it to accept commands from USB `Serial`

### Option 2: Modify Code to Accept USB Serial Commands

Temporarily modify the firmware to accept commands from USB Serial instead of Serial1:

**In `teensy_firmware.ino`, change:**
```cpp
void processSerialCommands() {
  while (ESP32_SERIAL.available()) {  // Change this line
```

**To:**
```cpp
void processSerialCommands() {
  while (Serial.available()) {  // Use USB Serial instead
```

And change:
```cpp
  while (ESP32_SERIAL.available()) {
    uint8_t byte = ESP32_SERIAL.read();
```

**To:**
```cpp
  while (Serial.available()) {
    uint8_t byte = Serial.read();
```

Also update `sendAck()` and `sendStatus()` to use `Serial` instead of `ESP32_SERIAL`.

### Option 3: Use USB-to-Serial Adapter

Connect a USB-to-Serial adapter to Teensy pins 0 (RX) and 1 (TX):
- Adapter TX → Teensy Pin 0 (RX)
- Adapter RX → Teensy Pin 1 (TX)
- Common GND

Then send commands through the adapter at 115200 baud.

## Command Protocol

Commands use this format: `[0xFF][CMD][LEN][DATA...][0xFE]`

### Test Commands (Send as Hex)

#### 1. Set Mode to Pattern (Rainbow)
```
FF 01 02 02 00 FE
```
- Mode 2 = Pattern mode
- Index 0 = Pattern 0 (Rainbow)

#### 2. Set Mode to Pattern (Fire)
```
FF 01 02 02 01 FE
```
- Pattern 1 = Fire

#### 3. Set Mode to Image (Smiley Face)
```
FF 01 02 01 00 FE
```
- Mode 1 = Image mode
- Index 0 = Image 0 (Smiley Face)

#### 4. Set Mode to Sequence
```
FF 01 02 03 00 FE
```
- Mode 3 = Sequence mode
- Index 0 = Sequence 0 (cycles through demo images/patterns)

#### 5. Set Brightness
```
FF 06 01 80 FE
```
- Brightness = 128 (50%)

#### 6. Request Status
```
FF 10 00 FE
```
- Response: `FF BB [mode] [index] FE`

#### 7. Set Mode to Idle (Turn Off)
```
FF 01 02 00 00 FE
```
- Mode 0 = Idle (LEDs off)

## Built-in Demo Content

The firmware automatically creates these on startup:

### Demo Patterns (5 patterns):
- **Pattern 0**: Rainbow (Red/Blue)
- **Pattern 1**: Fire (Orange/Yellow)
- **Pattern 2**: Comet (Cyan/Blue)
- **Pattern 3**: Breathing (Purple)
- **Pattern 4**: Plasma (Green/Magenta)

### Demo Images (3 images):
- **Image 0**: Smiley Face (Yellow)
- **Image 1**: Rainbow Gradient
- **Image 2**: Heart (Red)

### Demo Sequence:
- **Sequence 0**: Cycles through images and patterns (2 seconds each)

## Using Serial Monitor (Arduino IDE)

1. **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Set line ending to **"No line ending"** or **"Newline"**
4. For hex commands, you'll need a tool like:
   - **Serial Monitor with hex input** (some IDEs support this)
   - **Python script** (see below)
   - **Terminal program** (PuTTY, Tera Term, etc.)

## Python Test Script

Create a file `test_teensy.py`:

```python
import serial
import time

# Change COM port to match your Teensy
ser = serial.Serial('COM3', 115200, timeout=1)  # Windows
# ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)  # Linux/Mac

time.sleep(2)  # Wait for connection

def send_command(cmd_bytes):
    """Send command bytes to Teensy"""
    ser.write(bytes(cmd_bytes))
    time.sleep(0.1)
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"Response: {response.hex()}")

# Test commands
print("Setting mode to Pattern 0 (Rainbow)...")
send_command([0xFF, 0x01, 0x02, 0x02, 0x00, 0xFE])

time.sleep(5)

print("Setting mode to Pattern 1 (Fire)...")
send_command([0xFF, 0x01, 0x02, 0x02, 0x01, 0xFE])

time.sleep(5)

print("Setting mode to Image 0 (Smiley)...")
send_command([0xFF, 0x01, 0x02, 0x01, 0x00, 0xFE])

time.sleep(5)

print("Setting brightness to 50%...")
send_command([0xFF, 0x06, 0x01, 0x80, 0xFE])

time.sleep(2)

print("Requesting status...")
send_command([0xFF, 0x10, 0x00, 0xFE])

ser.close()
```

Run with: `python test_teensy.py`

## Quick Visual Test

If you just want to see if LEDs work:

1. Upload firmware
2. Watch for startup animation (rainbow sweep)
3. After startup, LEDs should be off (idle mode)
4. Send a pattern command to see LEDs light up

## Troubleshooting

### No LEDs Light Up
- Check wiring: Pin 11 (DATA) and Pin 13 (CLOCK)
- Verify power to LED strip (5V, 2-3A)
- Check Serial Monitor for error messages
- Try setting brightness: `FF 06 01 FF FE` (full brightness)

### Commands Not Working
- Verify baud rate is 115200
- Check you're sending to correct serial port
- Ensure command format is correct: `FF [CMD] [LEN] [DATA] FE`
- Check Serial Monitor for "Command received" messages

### Serial Monitor Shows Garbage
- Wrong baud rate (should be 115200)
- Wrong serial port selected
- Try resetting Teensy (press button)

## Reverting Changes

After testing, remember to change the code back to use `ESP32_SERIAL` instead of `Serial` if you modified it, so it works with the ESP32 again.
