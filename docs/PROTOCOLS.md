# Communication Protocols

**Complete reference for serial and network protocols in Nebula POI**

---

## Serial Protocol (Teensy ↔ ESP32)

### Connection Parameters

```
Baud Rate:    115200 bps
Data Bits:    8
Parity:       None
Stop Bits:    1
Flow Control: None
Voltage:      3.3V (both devices)
```

### Physical Connections

```
Teensy TX1 (Pin 1)  →  ESP32 RX2 (GPIO 16)
Teensy RX1 (Pin 0)  ←  ESP32 TX2 (GPIO 17)
Common Ground       ←→ Ground
```

### Message Format

**All commands are newline-terminated ASCII text:**

```
COMMAND:PARAMETER\n
```

**Rules:**
- Commands are case-sensitive
- Parameters separated by colons (:)
- Multiple parameters separated by commas (,)
- Newline (\n) terminates each command
- Maximum line length: 1024 characters

---

## Command Reference

### Display Mode Commands

#### Set Mode
```
MODE:<mode>
```

**Modes:**
- `IDLE` - LEDs off or solid color
- `IMAGE` - Display uploaded image
- `PATTERN` - Display animated pattern
- `SEQUENCE` - Play pattern/image sequence
- `LIVE` - Real-time drawing mode

**Example:**
```
MODE:PATTERN
```

---

### Brightness Control

```
BRIGHTNESS:<value>
```

**Parameters:**
- `value`: 0-255 (0=off, 255=maximum)

**Example:**
```
BRIGHTNESS:128
```

---

### Frame Rate Control

```
FPS:<value>
```

**Parameters:**
- `value`: 10-120 (frames per second)
- Lower = smoother POV but slower spin required
- Higher = faster refresh but may flicker

**Example:**
```
FPS:60
```

---

### Pattern Commands

#### Select Pattern
```
PATTERN:<pattern_id>
```

**Pattern IDs:**
```
0  = Rainbow
1  = Wave
2  = Gradient
3  = Sparkle
4  = Fire
5  = Comet
6  = Breathing
7  = Strobe
8  = Meteor
9  = Wipe
10 = Plasma
11 = VU Meter (requires microphone)
12 = Pulse (requires microphone)
13 = Audio Rainbow (requires microphone)
14 = Center Burst (requires microphone)
15 = Audio Sparkle (requires microphone)
```

**Example:**
```
PATTERN:0
```

#### Pattern with Color
```
PATTERN:<pattern_id>:<r>,<g>,<b>
```

**Parameters:**
- `pattern_id`: 0-15
- `r, g, b`: 0-255 (RGB color values)

**Example:**
```
PATTERN:2:255,0,128
```

---

### Image Commands

#### Upload Image
```
IMAGE:<width>,<height>,<hex_data>
```

**Parameters:**
- `width`: Image width in pixels
- `height`: Must be 31 (LED count)
- `hex_data`: RGB pixel data in hex format

**Format:**
- Each pixel: 6 hex chars (RRGGBB)
- Bottom row first, top row last
- Left to right within each row

**Example (2×31 image):**
```
IMAGE:2,31,FF0000FF0000[...RGB data for 62 pixels...]
```

**Constraints:**
- Height MUST be 31 pixels
- Width limited by available RAM (~100 pixels max)
- Total size: width × 31 × 3 bytes

---

### Live Mode Commands

#### Live Frame Update
```
LIVE:<frame_data>
```

**Parameters:**
- `frame_data`: 31 × 3 bytes hex-encoded (RRGGBB for each LED)

**Format:**
- LED 0 (bottom): First 6 hex chars
- LED 30 (top): Last 6 hex chars
- Total: 186 hex characters (93 bytes)

**Example:**
```
LIVE:FF0000FF0000...(186 hex chars total)
```

---

### Sequence Commands

#### Start Sequence Upload
```
SEQUENCE:START
```
Clears current sequence and prepares for new items.

#### Add Sequence Item
```
SEQUENCE:ADD:<type>,<value>,<duration>
```

**Types:**
- `IMAGE` - Display image
- `PATTERN` - Display pattern

**Parameters:**
- `value`: Image index or pattern ID
- `duration`: Milliseconds to display

**Examples:**
```
SEQUENCE:ADD:PATTERN,0,2000    # Rainbow for 2 seconds
SEQUENCE:ADD:IMAGE,0,3000      # First image for 3 seconds
```

#### Play Sequence
```
SEQUENCE:PLAY
```
Starts sequence playback (loops continuously).

#### Stop Sequence
```
SEQUENCE:STOP
```
Stops sequence playback.

---

### System Commands

#### Get Status
```
STATUS
```

**Response (from Teensy):**
```json
{
  "mode": "PATTERN",
  "brightness": 128,
  "fps": 60,
  "pattern": 0,
  "version": "1.0.0"
}
```

#### Reset System
```
RESET
```
Returns to IDLE mode, clears sequences.

---

## WiFi Network Protocol

### Access Point Configuration

```
SSID:     POV-POI-WiFi
Password: povpoi123
Security: WPA2-PSK
Channel:  1 (2.4 GHz)
IP:       192.168.4.1
Subnet:   255.255.255.0
Gateway:  192.168.4.1
```

### mDNS Configuration

```
Hostname: povpoi.local
Service:  _http._tcp
Port:     80
```

**Access via:**
- Direct IP: `http://192.168.4.1`
- mDNS: `http://povpoi.local` (if supported by device)

---

## HTTP REST API

**See `API.md` for complete REST API documentation.**

**Quick Reference:**

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Web UI |
| `/api/status` | GET | Get system status |
| `/api/mode` | POST | Set display mode |
| `/api/brightness` | POST | Set brightness |
| `/api/fps` | POST | Set frame rate |
| `/api/pattern` | POST | Set pattern |
| `/api/upload` | POST | Upload image |
| `/api/live` | POST | Live frame update |

---

## BLE Protocol

**See `BLE_PROTOCOL.md` for complete BLE documentation.**

### Service Configuration

```
Service UUID:     6E400001-B5A3-F393-E0A9-E50E24DCCA9E
TX Characteristic: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
RX Characteristic: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
```

**Protocol:** Nordic UART Service (NUS)

**Commands:** Same as serial protocol, sent as UTF-8 strings

---

## Peer-to-Peer Sync Protocol

**See `POI_PAIRING.md` for complete pairing documentation.**

### Discovery

**mDNS Service:**
```
Service Type: _poi._tcp
Instance:     poi-<device_id>
Port:         80
TXT Records:
  - name=<device_name>
  - version=<firmware_version>
```

### Sync Commands (HTTP)

**Endpoints:**

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/sync/discover` | GET | List nearby devices |
| `/api/sync/pair` | POST | Pair with device |
| `/api/sync/unpair` | POST | Unpair device |
| `/api/sync/send` | POST | Send image/pattern to peer |

---

## Error Handling

### Serial Communication Errors

**Timeout:**
- Wait up to 1 second for response
- Retry up to 3 times
- Log error if all retries fail

**Checksum (future):**
- Optional CRC-16 at end of long commands
- Format: `COMMAND:DATA:CRC\n`

**Buffer Overflow:**
- Maximum command length: 1024 bytes
- Truncate and warn if exceeded

### Network Errors

**WiFi Connection Loss:**
- ESP32 maintains access point even if no clients
- Auto-reconnect not needed (always AP mode)

**HTTP Timeouts:**
- Client timeout: 30 seconds
- Server timeout: 10 seconds per request

---

## Protocol Examples

### Example 1: Display Rainbow Pattern

**HTTP (from web UI):**
```http
POST /api/mode HTTP/1.1
Content-Type: application/json

{"mode": "PATTERN"}
```

**ESP32 → Teensy:**
```
MODE:PATTERN
```

**HTTP (set pattern):**
```http
POST /api/pattern HTTP/1.1
Content-Type: application/json

{"pattern": 0}
```

**ESP32 → Teensy:**
```
PATTERN:0
```

---

### Example 2: Upload Small Image

**Web UI → ESP32 (HTTP):**
```http
POST /api/upload HTTP/1.1
Content-Type: multipart/form-data
Content-Length: [size]

[Binary BMP data]
```

**ESP32 processing:**
1. Receive image data
2. Convert to 31px height
3. Encode as hex RGB

**ESP32 → Teensy:**
```
IMAGE:2,31,FF0000FF0000...[full hex data]
```

---

### Example 3: Live Drawing

**Web canvas → ESP32 (HTTP):**
```http
POST /api/live HTTP/1.1
Content-Type: application/json

{
  "frame": [
    [255, 0, 0],    // LED 0: Red
    [0, 255, 0],    // LED 1: Green
    ...             // 31 total LEDs
  ]
}
```

**ESP32 → Teensy:**
```
LIVE:FF000000FF00...[93 bytes hex]
```

---

## Debugging Protocol

### Monitor Serial Traffic

**From computer:**

```bash
# Monitor Teensy
pio device monitor -b 115200 -p /dev/ttyACM0

# Monitor ESP32
pio device monitor -b 115200 -p /dev/ttyUSB0
```

**From Python:**

```python
import serial
import time

# Connect to ESP32
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# Send command
ser.write(b'MODE:PATTERN\n')
time.sleep(0.1)

# Read response (if any)
response = ser.readline().decode('utf-8')
print(f"Response: {response}")
```

### Test Commands

```bash
# Use screen or minicom
screen /dev/ttyACM0 115200

# Type commands directly:
MODE:PATTERN
PATTERN:0
BRIGHTNESS:128
```

---

## Performance Characteristics

### Serial Throughput

**Baud Rate:** 115200 bps
- Theoretical max: 14.4 KB/s
- Practical max: ~10 KB/s (overhead)

**Image Transfer Time:**
- Small (2×31): ~0.05 seconds
- Medium (32×31): ~0.5 seconds  
- Large (64×31): ~1.0 seconds

### HTTP Response Times

| Request | Typical | Max |
|---------|---------|-----|
| Status | 10ms | 50ms |
| Mode change | 20ms | 100ms |
| Pattern set | 20ms | 100ms |
| Image upload | 500ms | 3s |

### BLE Latency

- Connection: 100-500ms
- Command: 20-50ms
- Image transfer: Similar to serial (10 KB/s)

---

## Future Protocol Enhancements

### Planned
- [ ] Binary protocol option (faster than ASCII)
- [ ] CRC checksums for data integrity
- [ ] Chunked image transfers (for large images)
- [ ] OTA firmware update protocol

### Under Consideration
- [ ] WebSocket support (for lower latency)
- [ ] Mesh networking (multi-poi sync)
- [ ] Bluetooth Classic (audio streaming)

---

**For more details:**
- REST API: See `API.md`
- BLE Commands: See `BLE_PROTOCOL.md`
- Sync Protocol: See `POI_PAIRING.md`
