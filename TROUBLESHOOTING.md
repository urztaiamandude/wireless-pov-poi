# Troubleshooting Guide

Comprehensive solutions for common issues with the Nebula Poi.

## Table of Contents

- [LED Issues](#led-issues)
- [WiFi and Network Issues](#wifi-and-network-issues)
- [Serial Communication Issues](#serial-communication-issues)
- [Web Interface Issues](#web-interface-issues)
- [Pattern and Display Issues](#pattern-and-display-issues)
- [Power Issues](#power-issues)
- [Upload and Programming Issues](#upload-and-programming-issues)
- [Performance Issues](#performance-issues)

---

## LED Issues

### LEDs Don't Light Up At All

**Possible Causes:**
- No power to LED strip
- Wrong data/clock connections
- Incorrect LED strip type

**Solutions:**
1. Check 5V connection to LED strip
2. Verify power supply is working (test with multimeter)
3. Check LED strip polarity (5V, GND, DATA, CLOCK)
4. Verify connections:
   - Teensy Pin 11 → LED DATA
   - Teensy Pin 13 → LED CLOCK
5. Check LED_TYPE in code matches your strip (APA102)
6. Test with simple FastLED example

### Only Some LEDs Light Up

**Possible Causes:**
- Power supply insufficient
- Voltage drop along strip
- Damaged LEDs

**Solutions:**
1. Measure voltage at LED strip (should be ~5V)
2. Upgrade to higher current power supply
3. Add power injection at end of strip
4. Check for physically damaged LEDs
5. Test with lower brightness setting

### LEDs Show Wrong Colors

**Possible Causes:**
- Incorrect color order setting
- Data corruption
- Level shifting issue

**Solutions:**
1. Try different COLOR_ORDER settings:
   ```cpp
   #define COLOR_ORDER BGR  // Try RGB, GRB, etc.
   ```
2. Add level shifter between Teensy and LEDs
3. Shorten data wire
4. Add 330Ω resistor on data line
5. Check ground connections

### LEDs Flicker or Glitch

**Possible Causes:**
- Poor power supply
- Signal interference
- Loose connections

**Solutions:**
1. Add 1000µF capacitor across LED power
2. Use shielded wire for data/clock
3. Shorten wire runs
4. Check all connections are secure
5. Separate power and signal wires
6. Reduce brightness

### First LED Always On/Wrong

**Expected Behavior:**
- LED 0 is used for level shifting
- Should be off or dim
- LEDs 1-31 used for display

**If Different:**
- Check DISPLAY_LEDS setting in code
- Verify LED indexing (0-based)

---

## WiFi and Network Issues

### Can't See POV-POI-WiFi Network

**Possible Causes:**
- ESP32 not powered
- ESP32 not programmed
- WiFi disabled in code
- Hardware failure

**Solutions:**
1. Check ESP32 power LED is on
2. Check ESP32 serial monitor output:
   ```
   Starting Access Point...
   AP IP address: 192.168.4.1
   ```
3. Wait 20-30 seconds after power on
4. Try different WiFi channel (edit code)
5. Reprogram ESP32
6. Try different ESP32 board

### Can Connect to WiFi But No Internet

**Expected Behavior:**
- This is normal!
- System is Access Point (not router)
- No internet by design
- Only for local control

**Note:** Some devices show "No Internet" warning - this is OK.

### WiFi Keeps Disconnecting

**Possible Causes:**
- Weak signal
- Too far from ESP32
- Power issues
- ESP32 restarting

**Solutions:**
1. Move closer to ESP32 (< 10 meters)
2. Check ESP32 power supply
3. Monitor ESP32 serial for restarts
4. Add capacitor near ESP32 power
5. Check for overheating

### Wrong IP Address

**Expected IP:** 192.168.4.1

**If Different:**
1. Check serial monitor for actual IP
2. Verify not connected to different network
3. Use mDNS: http://povpoi.local
4. Reboot ESP32

### Can't Access http://povpoi.local

**Possible Causes:**
- mDNS not supported
- Network configuration

**Solutions:**
1. Use IP address directly: http://192.168.4.1
2. Check device supports mDNS
3. iOS/Mac: Usually works
4. Windows: Install Bonjour service

---

## Serial Communication Issues

### Teensy and ESP32 Not Communicating

**Symptoms:**
- Patterns don't change from web interface
- Status shows "Disconnected"
- Commands have no effect

**Solutions:**
1. Verify serial connections:
   - Teensy TX1 (Pin 1) → ESP32 RX2 (GPIO 16)
   - Teensy RX1 (Pin 0) → ESP32 TX2 (GPIO 17)
   - **Note:** TX goes to RX (crossover)
2. Check common ground connection
3. Verify baud rate (115200) in both codes
4. Check serial monitor outputs:
   - Teensy: Should show "Ready"
   - ESP32: Should show "Controller Ready"
5. Test with simple serial echo program

### Serial Monitor Shows Garbage

**Possible Causes:**
- Wrong baud rate
- Serial port conflict
- USB issue

**Solutions:**
1. Set Serial Monitor to 115200 baud
2. Select correct COM port
3. Close other serial programs
4. Try different USB cable
5. Restart Arduino IDE

### Commands Sent But Not Received

**Debug Steps:**
1. Check Teensy serial monitor for incoming data
2. Check ESP32 serial monitor for outgoing data
3. Verify command format (see protocol documentation)
4. Test with simple commands first
5. Check for data corruption (add checksums)

---

## Web Interface Issues

### Web Page Won't Load

**Solutions:**
1. Verify connected to POV-POI-WiFi
2. Try different browser
3. Clear browser cache
4. Disable VPN
5. Try incognito/private mode
6. Check ESP32 serial for errors
7. Reflash ESP32 firmware

### Web Page Loads But Buttons Don't Work

**Possible Causes:**
- JavaScript errors
- Network latency
- ESP32 overloaded

**Solutions:**
1. Check browser console for errors (F12)
2. Refresh page (Ctrl+F5)
3. Try different browser
4. Reduce command frequency
5. Check ESP32 serial for errors

### Status Shows "Disconnected"

**Causes:**
- Teensy-ESP32 serial issue
- Teensy not running
- Protocol error

**Solutions:**
1. Reboot both Teensy and ESP32
2. Check serial connections
3. Monitor both serial outputs
4. Verify firmware versions match

### Slider/Controls Lag

**Possible Causes:**
- Network latency
- Serial bottleneck
- Too many updates

**Solutions:**
1. Move closer to ESP32
2. Reduce update frequency in code
3. Optimize serial protocol
4. Lower frame rate

---

## Pattern and Display Issues

### Patterns Don't Display

**Solutions:**
1. Set mode to "Pattern" (mode 2)
2. Check pattern index (0-4)
3. Verify brightness > 0
4. Check Teensy serial monitor
5. Test with simple pattern first

### Images Don't Upload

**Solutions:**
1. Check file size (< 1MB)
2. Use supported format (JPG, PNG)
3. Check SPIFFS space
4. Monitor ESP32 serial
5. Simplify image (reduce size)

### POV Effect Doesn't Work

**POV Technique:**
- Spin poi at constant speed
- 2-3 rotations per second works best
- Practice in dark room
- Use high contrast images

**Tips:**
1. Test patterns first (easier than images)
2. Increase frame rate (60+ FPS)
3. Adjust brightness
4. Use simple designs
5. Check LED alignment

### Colors Don't Match Expected

**Solutions:**
1. Check COLOR_ORDER setting
2. Verify RGB values in pattern
3. Test with pure red (255,0,0)
4. Check gamma correction
5. Adjust brightness

---

## Power Issues

### System Keeps Rebooting

**Possible Causes:**
- Insufficient power
- Voltage drops
- Short circuit

**Solutions:**
1. Upgrade power supply (3A minimum)
2. Add bulk capacitors
3. Check for shorts
4. Reduce LED brightness
5. Power LEDs separately from controllers

### Low Voltage Warning

**Solutions:**
1. Use adequate power supply
2. Reduce LED count or brightness
3. Add power injection
4. Use thicker wires
5. Check connections

### Battery Drains Quickly

**Power Optimization:**
1. Reduce brightness (biggest factor)
2. Lower frame rate
3. Use simpler patterns
4. Turn off when not in use
5. Consider larger battery

### Overheating

**Solutions:**
1. Reduce brightness
2. Improve ventilation
3. Add heatsinks
4. Check for shorts
5. Reduce duty cycle

---

## Upload and Programming Issues

### Can't Upload to Teensy

**Solutions:**
1. Install/update Teensyduino
2. Select correct board (Teensy 4.1)
3. Press Teensy button during upload
4. Try different USB port
5. Try different USB cable
6. Check device manager (Windows)

### Can't Upload to ESP32

**Solutions:**
1. Hold BOOT button during upload
2. Select correct board type
3. Reduce upload speed
4. Try different USB cable
5. Install/update USB drivers
6. Check COM port selection

### Compilation Errors

**Common Fixes:**
1. Install required libraries:
   - FastLED (for Teensy)
2. Check Arduino IDE version
3. Update board packages
4. Verify code syntax
5. Check #define statements

### Out of Memory Errors

**Solutions:**
1. Reduce image storage
2. Optimize code
3. Use PROGMEM for constants
4. Reduce buffer sizes
5. Check partition scheme (ESP32)

---

## Performance Issues

### Slow Frame Rate

**Solutions:**
1. Lower frame rate setting
2. Simplify patterns
3. Reduce serial communication
4. Optimize display code
5. Check CPU usage

### Delayed Response

**Solutions:**
1. Reduce network latency
2. Optimize web interface
3. Buffer commands
4. Reduce update frequency

### Inconsistent Patterns

**Solutions:**
1. Check power stability
2. Verify timing
3. Test at lower speed
4. Check for interference
5. Optimize code

---

## Diagnostic Tools

### Serial Monitor Checklist

**Teensy Monitor Should Show:**
```
Teensy 4.1 Nebula Poi Initializing...
Teensy 4.1 Nebula Poi Ready!
Command received: 0x01
Mode set to: 2
```

**ESP32 Monitor Should Show:**
```
ESP32 Nebula Poi Controller Starting...
SPIFFS Mounted
AP IP address: 192.168.4.1
Web server started
```

### LED Test Sequence

1. Power on → Startup animation
2. Set mode 2 → Pattern displays
3. Change pattern → New pattern shows
4. Adjust brightness → LEDs dim/brighten
5. Set mode 0 → LEDs turn off

### Network Test

```bash
# Test connectivity
ping 192.168.4.1

# Test API
curl http://192.168.4.1/api/status
```

---

## Getting Help

If problems persist:

1. **Check Serial Monitors**
   - Both Teensy and ESP32
   - Look for error messages
   - Note where it fails

2. **Test Components Individually**
   - Test LEDs with simple code
   - Test WiFi separately
   - Test serial connection

3. **Document Your Setup**
   - Take photos of wiring
   - Note error messages
   - List what you've tried

4. **Review Documentation**
   - [Main README](README.md)
   - [Setup Guide](docs/README.md)
   - [Wiring Guide](docs/WIRING.md)

---

## Preventive Maintenance

- Check connections regularly
- Keep firmware updated
- Test before important use
- Have backup components
- Document custom changes
- Keep power supply adequate
- Store safely when not in use

---

**Still having issues?** Double-check all connections and power supply first - these solve 90% of problems!
