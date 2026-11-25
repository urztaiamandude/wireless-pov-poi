# BLE protocol

The ESP32 bridge is `firmware/esp32_firmware/src/ble_bridge.cpp`. It advertises one service, the Nordic UART Service, and translates `0xD0 … 0xD1` writes into the Teensy frame `0xFF CMD LEN DATA 0xFE`. It does not parse text commands.

BLE is compiled when `BLE_ENABLED` is set. The bridge is constructed from `esp32_firmware.ino` and polled from `loop()`.

## GATT

| | UUID | Properties |
|---|---|---|
| Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | Nordic UART Service |
| RX (phone → poi) | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | write, write without response |
| TX (poi → phone) | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | read, notify, plus a CCCD (`BLE2902`) |

The GAP name is `Wireless POV Poi`. Advertising includes the service UUID and scan response. Preferred connection intervals are `0x06` and `0x12`. The sketch does not call `setMTU`. Notify chunks are capped at `BLE_MAX_PACKET_SIZE` (509) with 10 ms between chunks.

The bridge keeps one `deviceConnected` flag. After a disconnect it waits 500 ms and calls `startAdvertising()` again. It does not configure a connection limit in the controller.

## Frame

App → device:

```
0xD0  opcode  payload…  0xD1
```

The bridge strips `0xD0` and `0xD1`. `opcode` is the first byte it keeps. The rest is the payload.

Device → app, built from a Teensy frame `0xFF cmd len data… 0xFE`:

```
0xD0  cmd  data…  0xD1
```

The length byte is dropped. A Teensy ack is `0xFF 0xAA <cmd> 0xFE` (four bytes, no extra data). The bridge treats `0xAA` as the command and the following command byte as the length, so the notify is:

```
0xD0 0xAA 0xD1
```

`CC_SUCCESS` (`0x00`) and `CC_ERROR` (`0x01`) are constants in the header. The bridge never sends them. A client that waits for `D0 00 D1` will not see a success.

## Opcodes the bridge handles

| Opcode | Name | What the bridge sends to the Teensy |
|---|---|---|
| `0x02` | `CC_SET_BRIGHTNESS` | `0xFF 0x06 0x01 <brightness> 0xFE` |
| `0x03` | `CC_SET_SPEED` | `0xFF 0x07 <len> <payload> 0xFE`. One payload byte is the legacy delay in milliseconds, not the 16-bit FPS the web API sends. |
| `0x04` | `CC_SET_PATTERN` | Opcode rewritten to `0x03`. Payload copied unchanged. See the known issue below. |
| `0x05` | `CC_SET_PATTERN_SLOT` | `0xFF 0x01 0x02 0x02 <slot> 0xFE` (mode 2, that index). The slot byte is not range-checked. |
| `0x06` | `CC_SET_PATTERN_ALL` | `0xFF 0x01 0x02 0x02 0xFF 0xFE`. **Not implemented** on the Teensy: index 255 is `>= MAX_PATTERNS`, so `displayPattern()` clears the LEDs. There is no auto-cycle. |
| `0x0E` | `CC_SET_SEQUENCER` | Opcode rewritten to `0x04`. Payload copied unchanged. |
| `0x0F` | `CC_START_SEQUENCER` | `0xFF 0x01 0x02 0x03 <index> 0xFE` (mode 3, that sequence index). |
| anything else | pass-through | `0xFF <same opcode> <len> <payload> 0xFE`. The Teensy has no handler for `0x00` or `0x01`. |

### `0x02` brightness

```
D0 02 80 D1
```

sets brightness 128. The byte is forwarded as-is.

### `0x03` speed

```
D0 03 14 D1
```

is a one-byte frame delay of 20 ms on the Teensy (`0x07` with length 1). It is not "50 FPS". The web route `POST /api/framerate` uses a different, two-byte FPS encoding. Sending two payload bytes here would forward both, and the Teensy would read them as big-endian FPS.

### `0x05` show a pattern slot

```
D0 05 03 D1
```

shows pattern slot 3, if that slot is active. Slots are 0–18. The old "1–15" range is wrong. This command does not upload pattern data.

### `0x0F` start a sequence

```
D0 0F 01 D1
```

selects sequence mode and sequence index 1. It does not upload the sequence.

### `0x0E` sequence upload

The bridge does not reshape the payload. The Teensy `receiveSequence()` reads:

```
seqIndex  count  loop  (item  duration_hi  duration_lo) × count
```

`seqIndex` must be 0–4 or the Teensy returns without storing. `count` is stored as sent; playback walks at most 10 items. `loop` is 0 or 1. Each `item` byte uses bit 7 as the kind flag: 0 = image index 0–127, 1 = pattern index 0–18.

A payload that starts at `count` and omits `seqIndex` and `loop` is stored shifted. That layout is not implemented as a translator.

## Known issue: `CC_SET_PATTERN` is off by one

`CC_SET_PATTERN` (`0x04`) only changes the opcode to Teensy `0x03` and copies the BLE payload into the UART frame (`ble_bridge.cpp`, `translateBLEtoInternalProtocol`).

The Teensy stores nine data bytes (`receivePattern()`):

```
slot  type  r1  g1  b1  r2  g2  b2  speed
```

`slot` must be 0–18. `type` is the pattern id, also 0–18.

The bytes that match that layout are:

```
D0 04  00 00  FF 00 00  00 00 FF  32  D1
```

slot 0, type 0 (rainbow), color1 red, color2 blue, speed 50.

An eight-byte body that starts at `type` (the layout this document used to specify, and the layout an Open-Pixel-Poi-style client sends) is one field early:

| Byte the client meant | Byte the Teensy stores |
|---|---|
| type | slot |
| color1.r | type |
| color1.g | color1.r |
| … | … |
| speed | color2.b |
| (none) | speed is whatever byte follows, or 0 |

Example: `D0 04 00 FF 00 00 00 00 FF 32 D1` was documented as "rainbow, speed 50". The Teensy reads slot 0, **type 0xFF**, and color1 starting at `00 00 00`. The bridge does not insert a slot byte and does not reject the short body. Until that is fixed in firmware, clients have to send the nine-byte form above.

`POST /api/pattern` already sends the nine-byte form. The bug is the BLE path only.

## What is not on BLE

- Image upload. There is no image opcode.
- LED count. Use `POST /api/hardware/leds`.
- A generated success or error frame. Watch for `D0 AA D1` if you need the Teensy ack, and treat it as "an ack was forwarded," not as a decoded command number.

## Notifying the phone

TX supports read and notify. The bridge writes a value and calls `notify()` from `sendResponse()`. Enable notifications on the TX characteristic after connect.

## Sketch of a client

UUIDs and the brightness write match the bridge. The notify check has to look for `0xAA`, not `0x00`.

```dart
final serviceUuid = Guid("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
final rxUuid = Guid("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
final txUuid = Guid("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

Future<void> setBrightness(BluetoothCharacteristic rx, int brightness) async {
  await rx.write([0xD0, 0x02, brightness & 0xFF, 0xD1]);
}

void onNotify(List<int> value) {
  // Teensy ack arrives as D0 AA D1. 0x00 is not produced by this firmware.
  if (value.length >= 3 && value[0] == 0xD0 && value[1] == 0xAA) {
    // ack forwarded; the original command byte is not in this packet
  }
}
```

A pattern write that lines up with `receivePattern()`:

```dart
await rx.write([
  0xD0, 0x04,
  0x00,       // slot
  0x00,       // type (rainbow)
  0xFF, 0x00, 0x00,
  0x00, 0x00, 0xFF,
  0x32,       // speed
  0xD1,
]);
```

## Known gaps

- `CC_SET_PATTERN` does not add the slot byte. An 8-byte type-first payload is stored one field early.
- `CC_SET_PATTERN_ALL` does not cycle patterns. Index `0xFF` clears the strip.
- Success and error opcodes `0x00` and `0x01` are not emitted. Acks are `0xAA` with the command byte dropped.
- `CC_SET_SPEED` is a millisecond delay when the payload is one byte. It is not the web API's frames-per-second field.
- `CC_SET_SEQUENCER` is a raw copy. The Teensy layout is `seqIndex, count, loop, items…`.
- No image transfer, no MTU request, and no second GATT service.
