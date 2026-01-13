# API Documentation

This document describes the REST API provided by the ESP32 web server for controlling the wireless POV POI system.

## Base URL

When connected to the POV-POI-WiFi network:
- IP Address: `http://192.168.4.1`
- mDNS: `http://povpoi.local` (if supported)

## Authentication

Currently, the API does not require authentication. All endpoints are publicly accessible on the local WiFi network.

## Response Format

All API responses are in JSON format with appropriate HTTP status codes.

Success Response:
```json
{
  "status": "ok"
}
```

Error Response:
```json
{
  "error": "Error description"
}
```

## Endpoints

### System Status

#### Get System Status
Get current system state and connection status.

**Endpoint:** `GET /api/status`

**Response:**
```json
{
  "connected": true,
  "mode": 2,
  "index": 0,
  "brightness": 128,
  "framerate": 50
}
```

**Response Fields:**
- `connected` (boolean): Teensy connection status
- `mode` (integer): Current display mode (0-4)
  - 0: Idle (off)
  - 1: Image display
  - 2: Pattern display
  - 3: Sequence playback
  - 4: Live mode
- `index` (integer): Current image/pattern/sequence index
- `brightness` (integer): LED brightness (0-255)
- `framerate` (integer): Display frame rate (10-120 FPS)

**Example:**
```bash
curl http://192.168.4.1/api/status
```

---

### Display Mode Control

#### Set Display Mode
Change the current display mode and select which content to display.

**Endpoint:** `POST /api/mode`

**Request Body:**
```json
{
  "mode": 2,
  "index": 0
}
```

**Request Fields:**
- `mode` (integer, required): Display mode (0-4)
- `index` (integer, required): Content index to display

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/mode \
  -H "Content-Type: application/json" \
  -d '{"mode": 2, "index": 0}'
```

---

### System Settings

#### Set Brightness
Adjust LED brightness.

**Endpoint:** `POST /api/brightness`

**Request Body:**
```json
{
  "brightness": 200
}
```

**Request Fields:**
- `brightness` (integer, required): Brightness level (0-255)
  - 0: LEDs off
  - 128: Half brightness
  - 255: Full brightness

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 150}'
```

**Note:** Lower brightness saves power and reduces heat.

---

#### Set Frame Rate
Change the display refresh rate.

**Endpoint:** `POST /api/framerate`

**Request Body:**
```json
{
  "framerate": 60
}
```

**Request Fields:**
- `framerate` (integer, required): Frame rate in FPS (10-120)
  - 30: Smooth, lower CPU usage
  - 50: Default, good balance
  - 60+: Very smooth, higher CPU usage

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/framerate \
  -H "Content-Type: application/json" \
  -d '{"framerate": 60}'
```

---

### Pattern Control

#### Upload Pattern Configuration
Create or update a pattern with specific colors and animation type.

**Endpoint:** `POST /api/pattern`

**Request Body:**
```json
{
  "index": 0,
  "type": 1,
  "color1": {
    "r": 255,
    "g": 0,
    "b": 0
  },
  "color2": {
    "r": 0,
    "g": 0,
    "b": 255
  },
  "speed": 50
}
```

**Request Fields:**
- `index` (integer, required): Pattern slot (0-4)
- `type` (integer, required): Pattern type
  - 0: Rainbow - Rotating rainbow effect
  - 1: Wave - Color wave animation
  - 2: Gradient - Smooth gradient between colors
  - 3: Sparkle - Random sparkles
  - 4+: Custom patterns (if implemented)
- `color1` (object, required): Primary color RGB values (0-255)
- `color2` (object, required): Secondary color RGB values (0-255)
- `speed` (integer, required): Animation speed (1-100)

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/pattern \
  -H "Content-Type: application/json" \
  -d '{
    "index": 0,
    "type": 1,
    "color1": {"r": 255, "g": 0, "b": 0},
    "color2": {"r": 0, "g": 255, "b": 0},
    "speed": 75
  }'
```

---

### Image Management

#### Upload Image
Upload an image for POV display.

**Endpoint:** `POST /api/image`

**Request:** Multipart form data

**Form Fields:**
- `image` (file, required): Image file (JPG, PNG, etc.)

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/image \
  -F "image=@myimage.png"
```

**Image Guidelines:**
- Width: 31 pixels (matching LED count)
- Height: Up to 64 pixels recommended
- Format: Any standard image format
- High contrast works best for POV
- Simple graphics better than photos

**Note:** Image processing converts the uploaded image to LED-compatible format. Complex images may not translate well to 31-pixel width.

---

### Live Mode

#### Send Live Frame
Send real-time LED data for immediate display.

**Endpoint:** `POST /api/live`

**Request Body:**
```json
{
  "pixels": [
    {"r": 255, "g": 0, "b": 0},
    {"r": 0, "g": 255, "b": 0},
    {"r": 0, "g": 0, "b": 255}
  ]
}
```

**Request Fields:**
- `pixels` (array, required): Array of 31 RGB color objects
  - Each object has `r`, `g`, `b` fields (0-255)
  - Array length must be exactly 31 (matching display LEDs)

**Response:**
```json
{
  "status": "ok"
}
```

**Example:**
```bash
curl -X POST http://192.168.4.1/api/live \
  -H "Content-Type: application/json" \
  -d '{
    "pixels": [
      {"r": 255, "g": 0, "b": 0},
      {"r": 255, "g": 127, "b": 0},
      {"r": 255, "g": 255, "b": 0}
    ]
  }'
```

**Use Cases:**
- Real-time drawing applications
- Music visualization
- External control from another system
- Custom animations

**Performance Note:** Live mode updates should not exceed ~50 FPS to avoid overwhelming the serial communication.

---

## Serial Protocol (Teensy ↔ ESP32)

For developers extending the system, here's the internal serial protocol.

### Message Format

All messages follow this structure:
```
[0xFF] [CMD] [LEN] [DATA...] [0xFE]
```

- `0xFF`: Start marker
- `CMD`: Command byte
- `LEN`: Data length
- `DATA`: Command-specific data
- `0xFE`: End marker

### Command Codes

| Code | Command | Direction | Description |
|------|---------|-----------|-------------|
| 0x01 | Set Mode | ESP32→Teensy | Change display mode |
| 0x02 | Upload Image | ESP32→Teensy | Transfer image data |
| 0x03 | Upload Pattern | ESP32→Teensy | Send pattern config |
| 0x04 | Upload Sequence | ESP32→Teensy | Send sequence data |
| 0x05 | Live Frame | ESP32→Teensy | Real-time LED data |
| 0x06 | Set Brightness | ESP32→Teensy | Adjust brightness |
| 0x07 | Set Frame Rate | ESP32→Teensy | Adjust frame rate |
| 0x10 | Status Request | ESP32→Teensy | Request status |
| 0x20 | Save to SD | ESP32→Teensy | Save image to SD card (v2.0+) |
| 0x21 | Load from SD | ESP32→Teensy | Load image from SD card (v2.0+) |
| 0x22 | List SD Images | ESP32→Teensy | List stored images (v2.0+) |
| 0x23 | Delete from SD | ESP32→Teensy | Delete image from SD (v2.0+) |
| 0xAA | Acknowledge | Teensy→ESP32 | Command received |
| 0xBB | Status Response | Teensy→ESP32 | Status data |
| 0xCC | List Response | Teensy→ESP32 | SD image list (v2.0+) |

**Note:** Commands 0x20-0x23 require SD card support to be enabled in Teensy firmware (uncomment `#define SD_SUPPORT`).

### SD Card Commands (v2.0+)

#### Save Image to SD (0x20)

Saves an image from RAM to SD card for persistent storage.

**Format:**
```
0xFF 0x20 [LEN] [FILENAME_LEN] [FILENAME...] [IMG_INDEX] 0xFE
```

**Fields:**
- `FILENAME_LEN`: Length of filename (1 byte)
- `FILENAME`: Filename without extension (max 32 chars)
- `IMG_INDEX`: Image slot to save (0-9)

**Response:** ACK (0xAA)

**Example:** Save image slot 0 as "heart"
```
0xFF 0x20 0x07 0x05 'h' 'e' 'a' 'r' 't' 0x00 0xFE
```

#### Load Image from SD (0x21)

Loads a stored image from SD card into RAM.

**Format:**
```
0xFF 0x21 [LEN] [FILENAME_LEN] [FILENAME...] [IMG_INDEX] 0xFE
```

**Fields:**
- `FILENAME_LEN`: Length of filename (1 byte)
- `FILENAME`: Filename without extension (max 32 chars)
- `IMG_INDEX`: Target image slot (0-9)

**Response:** ACK (0xAA)

**Example:** Load "heart" into slot 0
```
0xFF 0x21 0x07 0x05 'h' 'e' 'a' 'r' 't' 0x00 0xFE
```

#### List SD Images (0x22)

Lists all stored images on SD card.

**Format:**
```
0xFF 0x22 0x00 0xFE
```

**Response:** List response (0xCC)
```
0xFF 0xCC [COUNT] [NAME1_LEN] [NAME1...] [NAME2_LEN] [NAME2...] ... 0xFE
```

**Fields:**
- `COUNT`: Number of images found
- For each image:
  - `NAMEX_LEN`: Length of filename
  - `NAMEX`: Filename without extension

**Example Response:** Two images "heart" and "star"
```
0xFF 0xCC 0x02 0x05 'h' 'e' 'a' 'r' 't' 0x04 's' 't' 'a' 'r' 0xFE
```

#### Delete Image from SD (0x23)

Deletes an image file from SD card.

**Format:**
```
0xFF 0x23 [LEN] [FILENAME_LEN] [FILENAME...] 0xFE
```

**Fields:**
- `FILENAME_LEN`: Length of filename (1 byte)
- `FILENAME`: Filename without extension (max 32 chars)

**Response:** ACK (0xAA)

**Example:** Delete "heart"
```
0xFF 0x23 0x06 0x05 'h' 'e' 'a' 'r' 't' 0xFE
```

### SD Card File Format

Images are stored as `.pov` files with this structure:

```
[WIDTH:1] [HEIGHT:1] [RGB_DATA...]
```

- Width: 1 byte (max 31)
- Height: 1 byte (max 64)
- RGB Data: width × height × 3 bytes (R, G, B values)

### Example: Set Mode Command

```
0xFF 0x01 0x02 0x02 0x00 0xFE
│    │    │    │    │    │
│    │    │    │    │    └─ End marker
│    │    │    │    └────── Index: 0
│    │    │    └─────────── Mode: 2 (pattern)
│    │    └──────────────── Length: 2 bytes
│    └───────────────────── Command: Set Mode
└────────────────────────── Start marker
```

---

## Error Codes

| HTTP Code | Description |
|-----------|-------------|
| 200 | Success |
| 400 | Bad Request - Invalid or missing data |
| 404 | Not Found - Invalid endpoint |
| 500 | Internal Server Error |

---

## Rate Limiting

No explicit rate limiting is implemented, but practical limits exist:

- Serial baudrate: 115200 bps
- Web server handles one request at a time
- Live mode updates: ~50 FPS maximum recommended
- File uploads: Limited by SPIFFS storage and upload speed

---

## CORS

The API does not currently implement CORS headers. Access is limited to devices on the same WiFi network.

---

## WebSocket Support

WebSocket support is not currently implemented but could be added for:
- Real-time status updates
- Bidirectional live mode
- Lower latency control
- Streaming animations

---

## Mobile App Integration Guide

### Android App Example (Kotlin)

```kotlin
import okhttp3.*
import org.json.JSONObject

class POVPoiAPI {
    private val client = OkHttpClient()
    private val baseUrl = "http://192.168.4.1"
    
    fun setMode(mode: Int, index: Int) {
        val json = JSONObject()
        json.put("mode", mode)
        json.put("index", index)
        
        val body = RequestBody.create(
            MediaType.parse("application/json"), 
            json.toString()
        )
        
        val request = Request.Builder()
            .url("$baseUrl/api/mode")
            .post(body)
            .build()
            
        client.newCall(request).execute()
    }
    
    fun setBrightness(brightness: Int) {
        val json = JSONObject()
        json.put("brightness", brightness)
        
        val body = RequestBody.create(
            MediaType.parse("application/json"),
            json.toString()
        )
        
        val request = Request.Builder()
            .url("$baseUrl/api/brightness")
            .post(body)
            .build()
            
        client.newCall(request).execute()
    }
}
```

### iOS App Example (Swift)

```swift
import Foundation

class POVPoiAPI {
    let baseURL = "http://192.168.4.1"
    
    func setMode(mode: Int, index: Int) {
        guard let url = URL(string: "\(baseURL)/api/mode") else { return }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        let json: [String: Any] = ["mode": mode, "index": index]
        request.httpBody = try? JSONSerialization.data(withJSONObject: json)
        
        URLSession.shared.dataTask(with: request).resume()
    }
    
    func setBrightness(brightness: Int) {
        guard let url = URL(string: "\(baseURL)/api/brightness") else { return }
        
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        let json: [String: Any] = ["brightness": brightness]
        request.httpBody = try? JSONSerialization.data(withJSONObject: json)
        
        URLSession.shared.dataTask(with: request).resume()
    }
}
```

---

## Testing the API

### Using curl

```bash
# Get status
curl http://192.168.4.1/api/status

# Set pattern mode
curl -X POST http://192.168.4.1/api/mode \
  -H "Content-Type: application/json" \
  -d '{"mode": 2, "index": 0}'

# Set brightness to 50%
curl -X POST http://192.168.4.1/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 128}'

# Set rainbow pattern
curl -X POST http://192.168.4.1/api/pattern \
  -H "Content-Type: application/json" \
  -d '{
    "index": 0,
    "type": 0,
    "color1": {"r": 255, "g": 0, "b": 0},
    "color2": {"r": 0, "g": 0, "b": 255},
    "speed": 50
  }'
```

### Using Postman

1. Create new request
2. Set method to POST
3. Enter URL: `http://192.168.4.1/api/mode`
4. Set Headers: `Content-Type: application/json`
5. Set Body (raw JSON): `{"mode": 2, "index": 0}`
6. Send request

### Using Python

```python
import requests

base_url = "http://192.168.4.1"

# Get status
response = requests.get(f"{base_url}/api/status")
print(response.json())

# Set pattern
data = {
    "index": 0,
    "type": 1,
    "color1": {"r": 255, "g": 0, "b": 0},
    "color2": {"r": 0, "g": 255, "b": 0},
    "speed": 50
}
response = requests.post(f"{base_url}/api/pattern", json=data)
print(response.json())
```

---

## Future API Enhancements

Planned additions:
- WebSocket support for real-time updates
- Authentication for security
- Sequence management endpoints
- Preset library management
- Configuration backup/restore
- OTA update endpoints
- Network configuration API
- Statistics and diagnostics

---

## Support

For API questions or issues:
- Check serial monitor output for debugging
- Verify WiFi connection to POV-POI-WiFi
- Test with curl before mobile app integration
- Review example code in documentation
