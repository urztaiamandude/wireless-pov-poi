# HTTP API

The routes below are the ones `setupWebServer()` registers in `firmware/esp32_firmware/esp32_firmware.ino`. If this page and that function disagree, the sketch wins.

UART command bytes are in [PROTOCOLS.md](PROTOCOLS.md). This page does not redefine them.

## Base URL

On the access point `POV-POI-WiFi` (password `povpoi123`):

- `http://192.168.4.1`
- mDNS, when `MDNS.begin` succeeds: `http://povpoi-<id>.local`, where `<id>` is the last six characters of the device id with colons removed. There is no fixed `povpoi.local`.

The AP password is not an HTTP credential.

JSON bodies are read from the POST body (`server.arg("plain")`). No `/api/*` route reads query parameters.

## Authentication and CORS

There is no authentication. Any client that can reach the device can call every route.

`server.enableCORS(true)` is called before `server.begin()`. The sketch does not set an origin allow-list. On Arduino-ESP32 2.0.17 that helper adds `Access-Control-Allow-Origin: *`. CORS is open.

## Responses

Command handlers that accept a request answer **HTTP 200** with `"status": "ok"`. That means the ESP32 parsed the body and, where the route says so, wrote a UART or ESP-NOW frame. It does not mean the Teensy applied the command, that an SD file changed, or that a peer received it. Several routes return `ok` even when the operation was a no-op (legacy sync execute, sync push, SD delete and load with no card check).

Validation failures use **400**, **413**, **500**, or **503** and a body `{"error":"..."}`. They do not use `"status": "ok"`.

Two SD list routes use **HTTP 200** with an `error` string when the card is missing or the Teensy does not answer. See those routes below.

There is no rate limit and no WebSocket server. Nothing is registered at `/ws` or `/api/battery`.

## Display values the Teensy understands

`POST /api/mode` forwards `mode` and `index` with no range check. The Teensy display loop uses:

| `mode` | Meaning |
|---|---|
| 0 | Idle (LEDs off) |
| 1 | Image |
| 2 | Pattern |
| 3 | Sequence |
| 4 | Live frame |

Pattern ids are **0–18** (`MAX_PATTERNS` is 19):

| Id | Pattern |
|---|---|
| 0 | Rainbow |
| 1 | Wave |
| 2 | Gradient |
| 3 | Sparkle |
| 4 | Fire |
| 5 | Comet |
| 6 | Breathing |
| 7 | Strobe |
| 8 | Meteor |
| 9 | Wipe |
| 10 | Plasma |
| 11–15 | Audio (MAX9814): VU meter, pulse, rainbow, center burst, sparkle |
| 16 | Split spin |
| 17 | Theater chase |
| 18 | Retro strobe |

Image slots the ESP32 will assign, load, or put in a sequence are **0–127**. The Teensy PSRAM array is `MAX_IMAGES` 200, and a raw UART `0x0B` can name a higher slot, but sequence playback keeps only the low 7 bits of an image index. Slots 128–199 are not reachable from this API. Web uploads start at slot 5 and stop at 127.

## Routes

| Method | Path |
|---|---|
| GET | `/` |
| GET | `/api/status` |
| POST | `/api/mode` |
| POST | `/api/brightness` |
| POST | `/api/framerate` |
| POST | `/api/power/mode` |
| POST | `/api/pattern` |
| POST | `/api/sequence` |
| POST | `/api/image` |
| POST | `/api/live` |
| GET | `/api/sync/status` |
| POST | `/api/sync/discover` |
| POST | `/api/sync/execute` |
| GET | `/api/sync/data` |
| POST | `/api/sync/push` |
| GET | `/api/device/config` |
| POST | `/api/device/config` |
| GET | `/api/hardware/leds` |
| POST | `/api/hardware/leds` |
| GET | `/api/wifi/status` |
| GET | `/api/wifi/scan` |
| POST | `/api/wifi/connect` |
| POST | `/api/wifi/disconnect` |
| GET | `/api/multipoi/status` |
| POST | `/api/multipoi/pair` |
| POST | `/api/multipoi/unpair` |
| POST | `/api/multipoi/syncmode` |
| POST | `/api/multipoi/peercmd` |
| GET | `/api/sd/list` |
| GET | `/api/sd/info` |
| POST | `/api/sd/delete` |
| POST | `/api/sd/load` |
| GET | `/api/sd/pattern/list` |
| POST | `/api/sd/pattern/save` |
| POST | `/api/sd/pattern/load` |
| GET | `/manifest.json` |
| GET | `/sw.js` |

Unmatched paths go to `handleNotFound`, which serves a SPIFFS file at that path when one exists, otherwise **404**.

## GET `/api/status`

No body.

```json
{
  "connected": true,
  "mode": 0,
  "index": 0,
  "brightness": 128,
  "framerate": 50,
  "sdCardPresent": false,
  "powerMode": 1,
  "count": 10
}
```

`connected` and `sdCardPresent` come from the Teensy status poll. `brightness` and `framerate` are the ESP32's copy (boot defaults 128 and 50). `powerMode` is 0–3 (boot default 1, balanced). `count` is the number of images this boot has uploaded, or **10** when that count is still 0. It is not the Teensy slot count and it is not 200.

## POST `/api/mode`

```json
{ "mode": 2, "index": 0 }
```

Both fields are optional. Omitted fields stay at the ESP32's current values, and those current values are what is sent. Invalid JSON is **400** `{"error":"Invalid JSON"}`. A missing body is **400** `{"error":"No data"}`. There is no range check.

Success: **200** `{"status":"ok"}`. UART `0x01` with `[mode][index]`. In mirror mode this is also broadcast over ESP-NOW.

## POST `/api/brightness`

```json
{ "brightness": 128 }
```

`brightness` is required. Missing or non-integer: **400** `{"error":"Invalid data"}`.

The value is cast to `uint8_t` and forwarded. The ESP32 does not clamp it to 0–255; a larger integer wraps. The Teensy passes that byte to `FastLED.setBrightness` with no further clamp. A power-mode change can lower the stored brightness later (see below).

Success: **200** `{"status":"ok"}`. UART `0x06`. Mirrored over ESP-NOW when mirror mode is on.

## POST `/api/framerate`

```json
{ "framerate": 60 }
```

`framerate` is required. Missing or non-integer: **400** `{"error":"Invalid data"}`.

The value is cast to `uint16_t` and sent as big-endian FPS (UART `0x07`, two bytes). There is no 10–1000 limit on this route. `0` is still sent; the Teensy ignores a 2-byte FPS of 0 and leaves its delay unchanged. The ESP32's local delay estimate becomes 20 ms in that case.

Success: **200** `{"status":"ok"}`. Mirrored over ESP-NOW when mirror mode is on.

## POST `/api/power/mode`

The handler searches the raw body for the text `"mode":` and then a quoted string. It is not a general JSON parse. These strings are accepted:

| `mode` | `powerMode` | ESP32 CPU | FPS sent | Brightness cap |
|---|---|---|---|---|
| `performance` | 0 | 240 MHz | 1000 | 255 |
| `balanced` | 1 | 160 MHz | 60 | 255 |
| `powersave` | 2 | 80 MHz | 30 | 180 |
| `ultrasave` | 3 | 80 MHz | 15 | 80 |

Unknown text: **400** `{"error":"Unknown mode"}`. No `"mode":` field: **400** `{"error":"No data"}`.

On success the ESP32 sets its CPU frequency, stores the FPS, and sends UART `0x07`. If the current brightness is above the cap, it lowers `state.brightness` and sends UART `0x06`. The cap is not applied to a later `POST /api/brightness`.

Success body:

```json
{"status":"ok","mode":"powersave","cpu_mhz":80,"framerate":30,"max_brightness":180}
```

The power-mode name itself is not broadcast. The FPS write, and a brightness write when the cap hits, are broadcast in mirror mode.

## POST `/api/pattern`

```json
{
  "index": 0,
  "type": 0,
  "color1": {"r": 255, "g": 0, "b": 0},
  "color2": {"r": 0, "g": 0, "b": 255},
  "speed": 50
}
```

Defaults when a field is missing: `index` 0, `type` 0, `speed` 50, color1 red, color2 blue. Each channel is a byte.

`index` and `type` above 18 are **clamped to 18**. They are not rejected. JSON that does not parse: **400** `{"error":"No data"}` (including a body that is present but invalid).

Success: **200** `{"status":"ok"}`. UART `0x03`, nine bytes: `[index][type][r1][g1][b1][r2][g2][b2][speed]`. Mirrored over ESP-NOW in mirror mode. This route does not switch the display to pattern mode; send `POST /api/mode` with `"mode": 2` and the same index.

## POST `/api/sequence`

```json
{
  "index": 1,
  "loop": true,
  "items": [
    {"kind": "image", "index": 5, "duration": 2000},
    {"kind": "pattern", "index": 0, "duration": 1000}
  ]
}
```

| Field | Rule |
|---|---|
| `index` | Sequence slot. Default 1. **0 is rejected** (demo sequence). **5 or more is rejected.** Valid user slots are 1–4. |
| `loop` | Default true. |
| `items` | Required, at least one. Only the first **10** are sent; extras are dropped with no error. |
| `kind` | `"pattern"` or anything else (treated as an image). Default `"image"`. |
| image `index` | 0–127. Outside that range: **400**. |
| pattern `index` | 0–18. Outside that range: **400**. |
| `duration` | Milliseconds. Default 2000. Values under 100 are raised to 100. |

Success: **200** `{"status":"ok","seqIndex":1,"itemCount":2}`. UART `0x04`. This route does not start playback. Send `POST /api/mode` with `"mode": 3` and the sequence index.

## POST `/api/image`

Multipart upload, not JSON. The filename is parsed as `image_<width>x<height>.rgb`. If that parse fails, width and height stay **32×32**. Declared width must be 1–400 and height 1–64, and the declared byte count must fit in the 76,800-byte buffer. Otherwise the upload ends with **413** `{"error":"Image dimensions exceed firmware limits"}`. An aborted upload is **500** `{"error":"Upload aborted"}`.

The pixel bytes are raw RGB888, `width * height * 3`. If fewer bytes arrive, height is reduced to match the buffer.

Slot assignment: `nextImageSlot` starts at **5** each boot (0–4 are the demo slots). It increments through **127**, then wraps to 5. The response includes the slot that was used:

```json
{"status":"ok","slot":5}
```

The handler then tells the Teensy to show that image (UART `0x01`, mode 1). If `sdCardPresent` is true it also sends a save (`0x20`) under a generated `up#####` stem. The Teensy adds `.pov`. Image upload is not broadcast to paired poi.

The Teensy pixel array is 400 columns by **32** rows (`IMAGE_HEIGHT`). The ESP32's height check allows up to 64. A height above 32 passes this route and does not fit that array. Treat 32 as the display height.

## POST `/api/live`

```json
{"pixels": [{"r": 255, "g": 0, "b": 0}]}
```

Pixels are RGB bytes. Only the first `displayLeds()` pixels are used (`numLeds - sacrificialLeds`, at most 32). Missing channels default to 0. A body that is present but not valid JSON still returns **200** `{"status":"ok"}` and sends a black frame. A missing body is **400** `{"error":"No data"}`.

UART `0x05`. This route does not set mode 4. The Teensy shows `liveBuffer` only when the current mode is 4.

## Hardware LEDs

### GET `/api/hardware/leds`

```json
{"numLeds": 32, "sacrificialLeds": 0, "displayLeds": 32, "displayLedStart": 0}
```

Loaded from NVS keys `hw_numLeds` and `hw_sacLeds` (defaults 32 and 0). Out-of-range saved values are corrected at boot.

### POST `/api/hardware/leds`

```json
{"numLeds": 32, "sacrificialLeds": 0}
```

Either field may be omitted. `numLeds` must be **2–32** and `sacrificialLeds` must be **less than `numLeds`**. Otherwise **400** with `{"error":"numLeds must be 2-32 and sacrificialLeds must be less than numLeds"}`.

Success returns the same four fields as GET, plus `"status":"ok"`. Values are stored in NVS and sent as UART `0x09`.

## WiFi station

### GET `/api/wifi/status`

```json
{
  "apIp": "192.168.4.1",
  "apSsid": "POV-POI-WiFi",
  "staConnected": false,
  "staIp": "",
  "savedSsid": ""
}
```

`mdnsName` is included only when mDNS started.

### GET `/api/wifi/scan`

Asynchronous. The first call (and any call while a scan is running) returns **200** `{"scanning": true}`. When the scan finishes:

```json
{"scanning": false, "networks": [{"ssid": "example", "rssi": -40, "secure": true}]}
```

At most 20 networks (`MAX_SCAN_RESULTS`). Hidden networks are included in the scan.

### POST `/api/wifi/connect`

```json
{"ssid": "example", "password": "secret"}
```

`ssid` is required and must be non-empty after trim. `password` is optional and defaults to `""`. Missing body: **400** `{"error":"No data"}`. Missing ssid: **400** `{"error":"Missing ssid"}`. Empty ssid: **400** `{"error":"SSID cannot be empty"}`.

Success: **200** `{"status":"ok","message":"Connecting to network. Check /api/wifi/status for result."}`. Credentials are stored (`sta_ssid`, `sta_password`) and `WiFi.begin` is called. The HTTP response does not wait for association.

### POST `/api/wifi/disconnect`

No body. Disconnects STA and deletes the saved SSID and password.

**200** `{"status":"ok","message":"Disconnected and credentials cleared"}`.

## Device config

### GET `/api/device/config`

```json
{
  "deviceId": "AA:BB:CC:DD:EE:FF",
  "deviceName": "POV Poi EE:FF",
  "syncGroup": "",
  "autoSync": false,
  "syncInterval": 30000
}
```

`deviceId` defaults to the WiFi MAC. `autoSync` defaults to false. `syncInterval` is milliseconds (default 30000). `syncGroup` is stored and is **not** read by the ESP-NOW pairer.

### POST `/api/device/config`

Optional fields: `deviceName` (string), `syncGroup` (string), `autoSync` (boolean). `deviceId` and `syncInterval` in the body are ignored. Missing body: **400** `{"error":"No data"}`. Invalid JSON: **400** `{"error":"Invalid JSON"}`.

Success: **200** `{"status":"ok","message":"Configuration updated"}`. Name, group, and auto-sync are written to NVS.

## SD card

The ESP32 does not mount the card. These routes send UART commands. `sdCardPresent` is whatever the last Teensy status poll said.

| Route | Card check | Success body | Failure |
|---|---|---|---|
| GET `/api/sd/list` | yes | `{"files":["name", ...]}` | **200** `{"error":"SD card not present","files":[]}` or `{"error":"Failed to read response","files":[]}` |
| GET `/api/sd/info` | no UART call | `present`, `totalSpace` 0, `freeSpace` 0, `spaceAvailable` false, `note` | always **200**. Space is not read. Teensy `0x23` is not sent. |
| POST `/api/sd/delete` | none | `{"status":"ok"}` | **400** if `filename` is missing |
| POST `/api/sd/load` | none | `{"status":"ok","slot":N}` | **400** if `filename` is missing |
| GET `/api/sd/pattern/list` | yes | `{"presets":["name", ...]}` | **200** with `error` and `"presets":[]` |
| POST `/api/sd/pattern/save` | **503** if absent | `{"status":"ok"}` | **400** if `name` is missing or not `[A-Za-z0-9_-]{1,32}` |
| POST `/api/sd/pattern/load` | **503** if absent | `{"status":"ok"}` | same name rules |

Delete and load accept `{"filename":"name.pov"}`. A trailing `.pov` and any directory prefix are stripped. The stem is truncated to 32 bytes. Delete and load return `ok` after the bytes are written to the UART. They do not wait for a Teensy result.

Load also accepts `"slot"`. The slot is used only when it is an integer from **0 to 127**. Any other value is ignored and slot **0** is sent. After the load command the ESP32 waits 100 ms and sends mode 1 for that slot.

Pattern save and load use `{"name":"preset"}`.

## Legacy HTTP sync

These routes are separate from `/api/multipoi/*`. They keep an mDNS peer table in RAM (at most 5). They do not copy images or patterns. `syncGroup` is not used to filter peers.

### GET `/api/sync/status`

`deviceId`, `deviceName`, `syncGroup`, `autoSync`, `syncInterval` (milliseconds), and `peers[]` of `deviceId`, `deviceName`, `ipAddress`, `lastSeen` (`millis()` of the last mDNS sighting, not a Unix time), `online`.

### POST `/api/sync/discover`

Runs mDNS discovery and returns **200** `{"status":"ok","peersFound":N,"peers":[...]}` with `deviceId`, `deviceName`, and `ipAddress`.

### POST `/api/sync/execute`

Body must include `peerId`. Other fields (`direction`, `syncImages`, `syncPatterns`, `syncSettings`) are ignored.

The handler calls `performSync`, which GETs the peer's `/api/sync/data` and POSTs this device's brightness, framerate, mode, and index to the peer's `/api/sync/push`. The peer payload is logged and **not applied**. If the peer is missing or offline, `performSync` returns without doing that, and this route still responds:

```json
{"status":"ok","itemsSynced":1,"imagesAdded":0,"patternsAdded":0,"settingsUpdated":true}
```

Those counts are constants. They are not a report of what changed.

### GET `/api/sync/data`

`deviceId`, `deviceName`, a **hardcoded** `images` list (ids 0–2), a **hardcoded** `patterns` list (ids 0–17, pattern 18 omitted), and `settings` from the live ESP32 state (`brightness`, `framerate`, `mode`, `index`, `timestamp` = `millis()`).

### POST `/api/sync/push`

Requires a body. The body is printed to serial and not parsed. **200** `{"status":"ok","message":"Sync data received"}`.

## ESP-NOW multi-poi

Behavior and its limits are in [POI_PAIRING.md](POI_PAIRING.md). The HTTP surface:

### GET `/api/multipoi/status`

```json
{
  "syncMode": "mirror",
  "localName": "POV Poi EE:FF",
  "localMac": "AA:BB:CC:DD:EE:FF",
  "hasPairedPeer": false,
  "autoPair": true,
  "peers": []
}
```

`syncMode` is `"mirror"` or `"independent"`. `hasPairedPeer` is true only when a peer is both paired and online (seen within 10 s). `autoPair` is the in-RAM flag; nothing in the sketch turns it off, and this API cannot set it. Each peer has `name`, `mac`, `state` (0 none, 1 discovering, 2 pair-sent, 3 paired — only 3 is assigned by the pairer), `online`, `mode`, `index`, `brightness`.

### POST `/api/multipoi/pair`

No body. Broadcasts a pair request. **200** `{"status":"ok","message":"Pair request broadcast"}`. The response does not mean a peer answered.

### POST `/api/multipoi/unpair`

Optional `{"index": 0}` unpairs that peer and returns `{"status":"ok"}`. No body, or a body without an integer `index`, unpairs everyone: `{"status":"ok","message":"All peers unpaired"}`.

### POST `/api/multipoi/syncmode`

`{"mode": "mirror"}` or `{"mode": "independent"}`. Numeric `0` and `1` are also accepted. Missing body: **400** `{"error":"No data"}`. Missing mode: **400** `{"error":"Missing mode"}`. Anything else: **400** `{"error":"Invalid mode (use mirror or independent)"}`.

Success: `{"status":"ok","syncMode":"mirror"}`. The mode is RAM-only.

### POST `/api/multipoi/peercmd`

```json
{"peer": 0, "cmd": "brightness", "brightness": 128}
```

`peer` is required (index into the peer array). `cmd` is `mode`, `pattern`, `brightness`, or `framerate`.

| `cmd` | Fields | Defaults |
|---|---|---|
| `mode` | `mode`, `index` | 0, 0 |
| `pattern` | `index`, `type`, `color1`, `color2`, `speed` | 0, 0, red, blue, 50 |
| `brightness` | `brightness` | 128 |
| `framerate` | `framerate` | 50 |

Pattern index and type are **not** clamped on this route. Unknown `cmd`: **400** `{"error":"Unknown cmd"}`. Success: **200** `{"status":"ok"}` even if `peer` is out of range (the send is then a no-op). The command goes to that peer over ESP-NOW. It is not applied to the local Teensy.

## Static files

GET `/` serves SPIFFS `/index.html` when that file exists, otherwise the PROGMEM page in the sketch.

GET `/manifest.json` and GET `/sw.js` return strings embedded in the sketch.

## Known gaps

- `"status": "ok"` is not a confirmed result from the Teensy or from a peer.
- No authentication. CORS is open.
- No `/api/battery`, no WebSocket, no rate limit.
- `GET /api/status` `count` is not image capacity.
- `GET /api/sd/info` does not report card capacity.
- `POST /api/sync/push` does not apply the body. `POST /api/sync/execute` reports success anyway.
- Image slots above 127 are not reachable from these routes.
- `POST /api/power/mode` depends on the substring `"mode":` in the body.
