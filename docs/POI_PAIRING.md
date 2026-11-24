# Pairing and sync

Two mechanisms exist in `firmware/esp32_firmware`. They do not do the same job. Neither has been proven on two physical poi. Treat both as experimental.

| Path | Code | What it actually does |
|---|---|---|
| ESP-NOW | `src/espnow_sync.h`, routes under `/api/multipoi/` | Pairing and live control. This is the path the firmware uses for mirror and per-peer commands. |
| Legacy HTTP | `/api/sync/*`, mDNS `_povpoi._tcp` | A peer list plus a settings POST that the receiver does not apply. |

## ESP-NOW pairing

Pairing state lives in RAM. It is cleared on reset. Nothing writes the peer list, the sync mode, or the auto-pair flag to NVS.

`syncGroup` from `POST /api/device/config` is stored and is not consulted here. Two poi do not auto-pair because they share a group name.

`autoPair` defaults to true. `setAutoPair()` is never called from the sketch, and no HTTP route changes it. A device that receives `MSG_PAIR_REQUEST` accepts it until six peers are stored (`MAX_SYNC_PEERS`).

### How a pair is made

1. `POST /api/multipoi/pair` broadcasts `MSG_PAIR_REQUEST` (`0x01`) to `FF:FF:FF:FF:FF:FF`. The payload is the sender MAC, a 24-character name, and `accepted = 0`.
2. A receiver with auto-pair on registers the ESP-NOW peer, marks the slot `PEER_PAIRED` (state 3), and replies `MSG_PAIR_RESPONSE` (`0x02`) with `accepted = 1`.
3. The initiator does the same bookkeeping when it sees `accepted = 1`.
4. `POST /api/multipoi/unpair` sends `MSG_UNPAIR` (`0x03`) and deletes the peer. A body `{"index": N}` drops one peer. No index drops all of them.

Heartbeats (`MSG_HEARTBEAT`, `0x20`) go out every 2 seconds. A paired peer is `online` when a packet from it arrived in the last 10 seconds. Heartbeats do not create a pair. `PEER_DISCOVERING` and `PEER_PAIR_SENT` are defined and never assigned.

Encryption is off. The wire header is `0x4E 0x50`, message type, sequence, payload. The largest send buffer is 250 bytes.

### Sync mode

`POST /api/multipoi/syncmode` with `"mirror"` (or `0`) or `"independent"` (or `1`). Default after boot is mirror. The choice is RAM-only.

**Mirror is partial.** While the mode is mirror, a paired online peer is told about:

- display mode and index
- pattern slot, type, two RGB colors, and speed (the nine-byte pattern body, not an image)
- brightness
- frame rate
- a time sync (`MSG_SYNC_TIME`, `0x30`) every 5 seconds, applied on the Teensy as command `0x08`

Mirror does **not** send image uploads, sequences, live frames, SD operations, or the LED-count configuration. A power-mode change is not a message of its own; the frame-rate write, and a brightness write if the cap lowered it, are the bytes that go out.

**Independent** turns those broadcasts off. `POST /api/multipoi/peercmd` can still unicast `mode`, `pattern`, `brightness`, or `framerate` to one peer index. `MSG_PEER_CMD` (`0x40`) is defined and not sent. The unicast uses the same `MSG_SET_*` types as mirror.

Incoming `MSG_SET_*` and `MSG_SYNC_TIME` are applied whenever the sender is paired. The receiver does not check mirror versus independent. Independent mode does not ignore commands the other poi sends.

### What you can try

1. Join each poi's `POV-POI-WiFi` in turn, or use each STA address.
2. Optional: `POST /api/device/config` with a `deviceName`. The name is in the pair payload. The sync group is not.
3. On one poi, `POST /api/multipoi/pair`.
4. `GET /api/multipoi/status` on both. `hasPairedPeer` becomes true only after a peer is paired and still inside the 10-second online window.
5. With `syncMode` `"mirror"`, change mode, pattern, brightness, or frame rate on one poi and see whether the other follows. Uploading an image will not copy it.
6. Reboot either board. The pair is gone. Pair again.

`{"status":"ok","message":"Pair request broadcast"}` means the broadcast was queued. It does not mean the other poi answered.

## Legacy HTTP sync

This is a stub. Do not use it to copy a show.

| Route | Behavior |
|---|---|
| `GET /api/sync/status` | This device's id, name, group, `autoSync`, `syncInterval` (milliseconds, default 30000), and the mDNS peer table (at most 5). |
| `POST /api/sync/discover` | Runs mDNS discovery and returns who answered. |
| `GET /api/sync/data` | Live brightness, frame rate, mode, and index, plus a hardcoded image list (ids 0–2) and a hardcoded pattern list (ids 0–17). Pattern 18 is missing. This is not the SD card. |
| `POST /api/sync/push` | Prints the body and returns `{"status":"ok","message":"Sync data received"}`. The settings are not applied. |
| `POST /api/sync/execute` | Requires `peerId`. Other JSON fields are ignored. It GETs the peer's `/api/sync/data` and POSTs local settings to `/api/sync/push`. The reply always says `itemsSynced: 1`, `imagesAdded: 0`, `patternsAdded: 0`, `settingsUpdated: true`, including when the peer was offline and nothing was sent. |

`autoSync` defaults to false. When it is true, `loop()` calls `performSync` on an online legacy peer. That call has the same limit: the receiver does not apply the POST.

There is no timestamp merge, no user-selected direction, and no image or pattern transfer on this path.

## Known gaps

- Pairing is not persisted. A reboot drops every peer and returns the mode to mirror.
- Mirror does not copy images, sequences, or live frames.
- Independent mode does not filter incoming ESP-NOW commands.
- `syncGroup` does not select who may pair.
- Auto-pair cannot be turned off from the API.
- Legacy HTTP sync does not apply received settings and does not sync content.
- None of this has been confirmed on two real poi.
