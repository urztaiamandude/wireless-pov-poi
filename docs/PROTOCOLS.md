# Protocols

How the pieces talk. The bytes below are what `firmware/teensy_firmware/teensy_firmware.ino` parses. Older copies of this file described newline-terminated text such as `MODE:PATTERN`. The firmware does not parse that.

## UART

| | |
|---|---|
| Baud | 115200 |
| Format | 8N1, no flow control |
| Levels | 3.3 V both sides, direct wires |
| Teensy TX1 pin 1 | ESP32-S3 GPIO 16 RX |
| Teensy RX1 pin 0 | ESP32-S3 GPIO 17 TX |

### Frame

```
0xFF  CMD  LEN  DATA…  0xFE
```

- `0xFF` starts a frame. `0xFE` ends it. The parser accepts the end marker once at least 4 bytes are in the buffer.
- For every command except image upload (`0x02`), `LEN` is one byte and is the payload length.
- For `0x02`, the two bytes after the command are a 16-bit length (high byte first). The image header then carries width and height as little-endian uint16.

Image upload layout, from `receiveImage()`:

```
0xFF  0x02  len_hi  len_lo  width_lo  width_hi  height_lo  height_hi  RGB…  0xFE
```

### Commands the Teensy handles

| CMD | Meaning | Payload |
|---|---|---|
| `0x01` | Set mode | mode, index |
| `0x02` | Upload image | 16-bit length, then width, height, RGB |
| `0x03` | Upload pattern | pattern body |
| `0x04` | Upload sequence | sequence body |
| `0x05` | Live frame | RGB for the current display height |
| `0x06` | Brightness | 0–255 |
| `0x07` | Frame rate | uint16 FPS, big-endian. A 1-byte payload is a legacy delay in milliseconds. |
| `0x08` | Sync time offset | int32 milliseconds, big-endian |
| `0x09` | LED config | physical LED count, sacrificial count |
| `0x0A` | Read LED config | none. Reply is `0xFF 0xBD num sacrificial display 0xFE` |
| `0x0B` | Next image slot | slot index. Resets to 0 after the next `0x02`. |
| `0x10` | Status | none |
| `0x20` | Save image to SD | filename length, filename, image slot |
| `0x21` | List SD images | none |
| `0x22` | Delete SD image | filename |
| `0x23` | SD card info | none |
| `0x24` | Load SD image into a slot | filename, slot |
| `0x30` | Pattern preset | subcommand, then a name for save, load, and delete |

`0x20`–`0x30` are compiled in because `SD_SUPPORT` is defined. SD paths are `/poi_images/*.pov` and `/poi_patterns/*.pat`.

Most commands get a short ack from the Teensy. Status and SD list replies use their own framed packets (`0xBB`, `0xCC`, and others). When you add a command, follow this framing. Do not add a parallel text protocol.

## WiFi and HTTP

The ESP32-S3 is AP+STA.

| | |
|---|---|
| AP SSID | `POV-POI-WiFi` |
| AP password | `povpoi123` |
| AP address | `http://192.168.4.1` |
| mDNS | `http://povpoi-<id>.local` printed at boot. Not `povpoi.local`. |

HTTP is the API both UIs use. The endpoint list in the firmware is the authority; [API.md](API.md) is the written version and should be checked against `esp32_firmware.ino` when they drift. Common routes:

| Method | Path | Role |
|---|---|---|
| GET | `/` | Embedded page, or SPIFFS `index.html` if present |
| GET | `/api/status` | Status |
| POST | `/api/mode` | Display mode |
| POST | `/api/brightness` | Brightness |
| POST | `/api/framerate` | Frame rate |
| POST | `/api/pattern` | Pattern |
| POST | `/api/image` | Image upload |
| POST | `/api/live` | Live frame |
| GET/POST | `/api/hardware/leds` | Physical count and sacrificial count |

There is no `/api/battery`.

## BLE

The ESP32 BLE bridge speaks Nordic UART Service and translates those packets into the `0xFF … 0xFE` frames above. It does not send UTF-8 command lines. Details: [BLE_PROTOCOL.md](BLE_PROTOCOL.md).

## Multi-poi

Discovery uses mDNS service `_povpoi._tcp` (the firmware calls `MDNS.addService("povpoi", "tcp", 80)`). Behavior on real hardware is unproven. See [POI_PAIRING.md](POI_PAIRING.md).
