# Second-Pass Deep Audit — Findings

**Scope:** Read-only audit of unreviewed subsystems. No code edits applied.
**Method:** Static review of cited files only. No builds, no hardware runs.
**Status:** Section 1 complete. Awaiting direction before proceeding to Section 2.

File paths in this report are relative to the repo root unless otherwise noted. All line numbers refer to the current working tree.

---

## Section 1 — `firmware/esp32_firmware/src/espnow_sync.h`

### 1.1 Payload math — comment is wrong, real value is 246, **but real code has worse bugs**

**Finding (confirmed):** `SYNC_MAX_PAYLOAD 244` is understated and the explanatory comment is wrong.

- File header (lines 11–12) documents the wire format as `[MAGIC:2][MSG_TYPE:1][SEQ:1][PAYLOAD:variable]`, i.e. **4 bytes of total overhead**.
- `sendMessage` at lines 391–406 literally writes 4 header bytes (`buf[0]=SYNC_MAGIC_0`, `buf[1]=SYNC_MAGIC_1`, `buf[2]=msgType`, `buf[3]=_seq++`) and then copies the payload starting at `buf + 4`.
- ESP-NOW v1 max frame is 250 bytes → true usable payload is `250 − 4 = 246`.
- The comment at line 24 (`// 250 - 4 byte header - 2 byte magic`) double-counts the magic bytes; the magic **is** the first two bytes of the 4-byte header.

**True value:** `SYNC_MAX_PAYLOAD` should be `246`, not `244`. The current value is merely conservative (harmless in itself), but see the next three items — the constant is not actually used to bound anything on the send path.

**Worse bug #1 — `SYNC_MAX_PAYLOAD` is never referenced.** A grep of the header shows the macro is defined once and never read. All payload-length bounds inside `sendMessage` are hand-coded literals (`min((int)payloadLen, 246)`, line 398). Changing the macro changes nothing at runtime.

**Worse bug #2 — buffer over-read on large `payloadLen`** (`espnow_sync.h:391–402`).
```cpp
void sendMessage(const uint8_t* mac, uint8_t msgType, const uint8_t* payload, uint8_t payloadLen) {
  uint8_t buf[250];
  ...
  if (payload && payloadLen > 0) {
    memcpy(buf + 4, payload, min((int)payloadLen, 246));   // memcpy is clamped
  }
  uint8_t totalLen = 4 + payloadLen;                        // totalLen is NOT clamped
  esp_err_t result = esp_now_send(mac, buf, totalLen);
```
- The `memcpy` is safely clamped to 246 bytes, so `buf[250]` itself is never overwritten.
- But `totalLen` is computed from the **raw, unclamped** `payloadLen`. If `payloadLen ∈ [247, 251]`, `esp_now_send` is told to transmit `totalLen` bytes from `buf`, but only the first `4 + 246 = 250` bytes are valid — the last `payloadLen − 246` bytes are whatever stack garbage lived past `buf[]`. Information leak to anyone listening on 2.4GHz in range.

**Worse bug #3 — `uint8_t` wraparound in `totalLen`** (same location).
- `payloadLen` is `uint8_t` (max 255). `totalLen` is also `uint8_t`. `totalLen = 4 + payloadLen` wraps modulo 256.
- For `payloadLen == 252`, `totalLen = 0` (ESP-NOW rejects zero-length — silently dropped).
- For `payloadLen == 253`, `totalLen = 1`; 254 → 2; 255 → 3. In all three, `esp_now_send` transmits a truncated 1–3 byte frame that doesn't even contain a full header, corrupting the protocol on the air. The receiver's `if (len < 4) return;` guard (line 500) will discard it, but any intermediate capture / sniffer sees a malformed frame.

In practice today, all callers pass `sizeof(struct)` for statically-sized packed structs (largest is `HeartbeatPayload` at ~36 bytes), so the bug is latent. But the signature accepts `uint8_t payloadLen` with no validation, and any future caller passing a larger buffer silently corrupts the wire format. Recommend: change parameter type to `size_t`, add an explicit `if (payloadLen > 246) return;` guard, make `totalLen` an `int`.

---

### 1.2 Threat model — ESP-NOW is broadcast-authenticated by magic bytes alone

**Confirmed:** No authentication, no integrity check, no replay protection.

- `broadcastPeer.encrypt = false` at line 174 (broadcast peer).
- `peerInfo.encrypt = false` at line 491 (every registered unicast peer).
- The only acceptance filter in `handleMessage` is `data[0]==SYNC_MAGIC_0 && data[1]==SYNC_MAGIC_1` (line 501) plus a "don't process messages from our own MAC" check (line 509). The latter is trivially bypassed by spoofing the source MAC in the ESP-NOW frame.
- For non-pairing messages (`MSG_SET_MODE`/`PATTERN`/`BRIGHTNESS`/`FRAMERATE`, plus `MSG_SYNC_TIME`) the handler does check `findPeer(mac) >= 0 && state == PEER_PAIRED` (e.g. line 623–624), so a random attacker can't drive those without first being paired.
- **However**, pairing is automatic and silent. `handlePairRequest` (lines 545–582) accepts **any** incoming `MSG_PAIR_REQUEST` whenever `_autoPairEnabled == true` (the default, set at line 137 and persisted). There is no user confirmation, no shared secret, no proximity check. Any 2.4GHz device within range can send a `MSG_PAIR_REQUEST` and immediately be elevated to `PEER_PAIRED` state, after which it can drive mode, pattern, brightness, and frame rate on every paired poi.
- `MSG_HEARTBEAT` has **no** peer-state check (lines 670–694). It updates the peer's reported `currentMode`, `currentIndex`, `brightness`, and **`name`** from any MAC that sends a valid-magic heartbeat, as long as that MAC is already in `_peers[]`. Heartbeats with garbage `hb->name` will silently rename paired peers shown in the UI.

**Threat model summary to document:**

| Vector | Requires | Effect |
|---|---|---|
| Inject pair request | On the same WiFi channel (default 1), within ~40m open air | Becomes a paired peer, gains full control of mode/pattern/brightness/framerate |
| Replay captured commands | Ditto | Works — no seq dedup (see §1.6) |
| Spoof heartbeat after pairing known MAC | MAC of paired peer | Rewrites paired peer's display name in UI |
| DoS via flood | In range | Receive callback runs heavy work in system task (see §1.4) — likely degrades web UI responsiveness |

Not necessarily a "bug" for a hobbyist LED toy, but should be explicitly called out in docs. If authentication is desired, ESP-NOW supports LMK/PMK encryption keys; swap `encrypt = false` for `encrypt = true` with a shared key derived from a user-configured passphrase.

---

### 1.3 Peer table management

**Duplicate peers:** `addPeerSlot` (lines 473–480) correctly calls `findPeer` first and returns the existing index. No duplicates.

**MAC comparisons:** Always `memcmp(..., 6)` (lines 467, 509). Correct length, no byte-order issue (MACs are byte arrays, not integers).

**Bounds on `_peerCount`:**
- Incremented at line 477 only after `_peerCount >= MAX_SYNC_PEERS` check. OK.
- Decremented at lines 252 and 618 after a shift-down. Decrement cannot underflow because both code paths are guarded by `findPeer(mac) >= 0` or `index < _peerCount`.
- `unpairAll` (line 236) resets `_peerCount = 0` and zeros `_peers`. OK.
- No negative or out-of-bounds access found.

**Stale entries / eviction — a real issue:**
`checkPeerTimeouts` (lines 440–462):
- For `PEER_PAIRED` peers: only toggles `_peers[i].online = false` after 10 s of silence (line 445). **Never evicts.** A peer that is powered off or out of range permanently occupies a slot in the 6-slot `_peers[]` table until reboot or explicit `unpairPeer()` from the UI.
- For `PEER_PAIR_SENT` peers: evicted after 30 s (lines 452–460). OK.
- `PEER_DISCOVERING` state is enum value 1 but **never assigned anywhere** in the file — grep-verified. Dead enum value.

**Answer to "does `PEER_TIMEOUT` actually evict peers":**
- There is no `PEER_TIMEOUT` constant in this file. The 10000 ms and 30000 ms values are hardcoded literals at lines 445 and 452. The `PEER_TIMEOUT` macro referenced at `esp32_firmware.ino:119` (value 120000 / 2 min) belongs to a **completely separate** mDNS/HTTP peer discovery table in the main `.ino` (`peers[]` / `peerCount` / `performSync`). The file has two different peer tables with similar names; don't conflate them.
- Net: paired ESP-NOW peers are marked offline but never evicted. With `MAX_SYNC_PEERS = 6`, a user who pairs → loses device → pairs again with a new MAC will silently fill the table after six such events and further pairing requests will be dropped at line 562 ("No peer slots available") with no UI feedback.

**`esp_now_del_peer` hygiene:**
- `unpairAll` (line 233), `unpairPeer` (line 246), `handleUnpair` (line 613), and the stale-peer cleanup (line 453) all call `esp_now_del_peer`. OK.
- `checkPeerTimeouts` offline path (line 446) does **not** call `esp_now_del_peer`. That's consistent with the "don't evict" behavior above but means offline peers still count against ESP-NOW's internal peer limit (20 for unencrypted).

---

### 1.4 Callback reentrancy — heavy work in ESP-NOW system-task context

**Confirmed risky.** The static receive callback (lines 714–722) calls `handleMessage`, which runs entirely on the ESP-NOW system task. Every handler does substantial work:

- **Serial.printf in every handler:** lines 540, 549, 612, 628, 642, 653, 665, 687. `Serial.printf` on ESP32 Arduino is blocking on the UART FIFO; during a flood (e.g. multiple peers heartbeating) this back-pressures the ESP-NOW RX pipeline.
- **User callbacks fired inline:** `_onModeChange(...)` at line 633, `_onPattern` at 644, `_onBrightness` at 656, `_onFrameRate` at 667, `_onSyncTime` at 706, `_onPeerUpdate` at 581 / 605 / 688. These are set by the main firmware (see §2.1) and typically:
  - write to `Preferences` (NVS flash write → tens of ms, blocks the task),
  - push bytes out Serial1 to the Teensy (UART blocking), and
  - mutate the HTTP handler's shared state flags.
- **No queueing** — there is no deferral layer. No `xQueueSendFromISR`, no task notify, no ring buffer. The recv callback is the executor.

**Consequences:**
- Heartbeat interval is 2 s and heartbeats are broadcast (line 431). With 6 paired peers all heartbeating, the RX task services 6 NVS-writing callbacks every 2 s.
- A malicious or buggy peer that floods `MSG_HEARTBEAT` with a renamed `name[]` will blast the UI via `_onPeerUpdate` every packet.
- If the Teensy UART is blocked (Teensy busy in FastLED.show()), the callback's UART write stalls, stalling subsequent ESP-NOW processing.

**Recommendation for the report:** post incoming messages to a `QueueHandle_t` from the recv callback and drain it from the main `loop()`. The receive path should do only magic-byte validation, length check, and enqueue.

---

### 1.5 Channel coordination

- `broadcastPeer.channel = 0` at line 173 and every unicast `peerInfo.channel = 0` at line 490. `channel = 0` in `esp_now_peer_info_t` means "follow the current WiFi interface's channel."
- `begin()` is called after `WiFi.mode()` is configured (per the doc comment at line 143). If the AP is on channel 1 (per `esp32_firmware.ino` `WIFI_CHANNEL` — not audited yet, deferred to §2), then ESP-NOW also runs on channel 1 by construction. No explicit `esp_wifi_set_channel` call is made.
- **Gap:** if this device also joins STA, the STA's AP may force the shared radio to a different channel than the SoftAP's configured channel. ESP-NOW on `channel = 0` will follow the hardware's current channel, so a peer running AP-only on channel 1 and a peer running STA-joined on channel 6 **cannot hear each other**. Nothing in `espnow_sync.h` detects or surfaces this.
- No channel-hopping scan, no channel pinning, no error to the user when peers are unreachable for channel reasons.

---

### 1.6 Sequence numbers — dead weight, wraparound not handled

**Confirmed dead.**
- `_seq` is defined at line 368, initialized to 0 in the constructor (line 132).
- Incremented exactly once, at `sendMessage` line 396 (`buf[3] = _seq++`).
- On the receive side, line 504 is literally: `// uint8_t seq = data[3];  // Available for dedup if needed`. The byte is not even read into a local variable; it's consumed only by the pointer arithmetic at line 505 (`payload = data + 4`).
- No deduplication, no replay detection, no ordering logic anywhere in the file.
- `_seq` is a `uint8_t` (line 368) so it wraps at 256. Even if dedup were implemented later with a naive "accept if seq > last_seq", it would break every 256 messages.

**Recommendation:** either implement a per-peer `lastSeq` with wrap-aware comparison (standard `(int8_t)(new - last) > 0` trick), or remove the seq byte from the protocol and reclaim one byte. If kept for replay protection purposes, widen to 16 or 32 bits.

---

### 1.7 `MSG_PEER_CMD` (0x40) is entirely dead code

**Confirmed by grep (`MSG_PEER_CMD|PeerCmdPayload` across `firmware/esp32_firmware/`):**

- `MSG_PEER_CMD` is defined at line 36 and **never sent** — no `sendMessage(..., MSG_PEER_CMD, ...)` call exists in this header or in `esp32_firmware.ino`.
- `PeerCmdPayload` (lines 113–117) is defined and **never instantiated** anywhere.
- `handleMessage`'s switch (lines 511–542) has **no case for `MSG_PEER_CMD`** — a received 0x40 message would fall to the `default` branch and log `"Unknown message type"` (line 540).
- The independent-mode targeted-send functions (`sendPeerModeChange` at line 288, `sendPeerPattern` at 295, `sendPeerBrightness` at 305, `sendPeerFrameRate` at 312) all call `sendMessage` with the **same message types used in mirror mode** (`MSG_SET_MODE`, `MSG_SET_PATTERN`, etc.), just unicast instead of broadcast.
- `esp32_firmware.ino:3230` calls `espNowSync.sendPeerModeChange(peerIdx, mode, index);` — confirmed the only caller, and it flows through the plain `MSG_SET_MODE` path.

**Implications for the receiving device in independent mode:**
Because `handleSetMode` / `handleSetPattern` / etc. (lines 622–668) do not distinguish between "targeted unicast in independent mode" and "broadcast in mirror mode," a peer in independent mode that receives a `MSG_SET_MODE` from its paired peer will apply it anyway — it has no way to know the sender meant "you specifically" vs. "broadcast to all." In a mirror-mode network this is fine; in a mixed deployment (device A mirror, device B independent) the semantics are ambiguous. The stated design intent ("independent targets a specific peer") is not enforced on the receive side.

**Recommendation:** either delete `MSG_PEER_CMD` and `PeerCmdPayload` as dead code, or actually wrap targeted commands in `MSG_PEER_CMD` on the send side and dispatch on the receive side so the recipient can distinguish targeted vs. broadcast intent.

---

### 1.8 Additional minor findings

| # | Location | Finding |
|---|---|---|
| a | `espnow_sync.h:470` | `findPeer` returns `-1` for "not found"; callers correctly gate on `< 0`. OK. |
| b | `espnow_sync.h:694` | `handleHeartbeat` has a comment claiming unknown-device discovery is "noted" for the UI, but the code path for unknown MAC is a no-op — the comment is aspirational. |
| c | `espnow_sync.h:704` | `_timeOffset = (int32_t)p->masterMillis − (int32_t)millis();` — `millis()` returns `unsigned long` (`uint32_t` on ESP32). Signed subtraction after cast is standard wrap-around arithmetic, OK for small offsets but will produce garbage once uptime exceeds ~24.8 days on either side. |
| d | `espnow_sync.h:710–712` | `onSendStatic` is a no-op. Send failures visible only via `sendMessage`'s own `Serial.printf` at line 404. No retry logic anywhere — if delivery fails (peer transiently unreachable), the packet is lost. |
| e | `espnow_sync.h:726` | `ESPNowSync* ESPNowSync::_instance = nullptr;` defined in a header. This is legal in C++17 only if the header is included in exactly one translation unit. Arduino projects single-TU by concatenation usually get away with it, but including this header from a second `.cpp` will produce a linker multiple-definition error. Flag for portability. |
| f | `espnow_sync.h:552–555` | If `_autoPairEnabled == false`, `handlePairRequest` drops silently with no response. The sender has no "rejected" signal and will keep retrying on its own pairing timer. Consider sending a `MSG_PAIR_RESPONSE` with `accepted = 0`. |
| g | `espnow_sync.h:509` | Self-MAC filter happens *after* the length and magic checks but before dispatch. Adequate, but note that `_localMac` is populated from `WiFi.macAddress()` in `begin()`; ESP-NOW actually uses the **soft-AP** MAC when in AP+STA mode, which may differ from the STA MAC returned here. Worth verifying against hardware; could cause a device to process its own broadcasts. (Depends on `WiFi.mode()` at init — verified in §2.) |

---

## Open questions deferred to later sections

- `WIFI_CHANNEL` value and whether `esp32_firmware.ino` pins ESP-NOW to the AP channel explicitly (Section 2, ties into §1.5 above).
- What the six receive-callback user callbacks actually do in the main firmware (Section 2, ties into §1.4 reentrancy risk).
- Whether self-MAC reported by `WiFi.macAddress()` matches the MAC ESP-NOW actually uses for the current `WiFi.mode()` (Section 2, ties into §1.8.g).

---

## Section 2 — `firmware/esp32_firmware/esp32_firmware.ino`

### 2.1 Echo-loop / `_syncCommandInProgress` audit

**Confirmed: no recursive echo loop today, but the flag is not a race-safe guard.**

Writers (all at file scope in `applyModeToTeensy` / `applyPatternToTeensy` / `applyBrightnessToTeensy` / `applyFrameRateToTeensy`):
- `_syncCommandInProgress = true;` at lines 3013, 3028, 3054, 3064
- `_syncCommandInProgress = false;` at lines 3021, 3050, 3060, 3076
- Set/clear are **paired in all code paths** — no early return, no exception paths, no conditional branches between set and clear. OK.

Readers (in the HTTP handlers before calling `espNowSync.broadcast*`):
- Lines 1979, 2005, 2034, 2085, 2094, 2159 — all of the form `if (!_syncCommandInProgress) { espNowSync.broadcastX(...); }`.

**How the loop is actually broken:**
The four `apply*ToTeensy` functions are fired from the ESP-NOW receive callback (registered at lines 2977–2994). They talk **only to the Teensy over UART** — they do not call any `espNowSync.broadcast*`. The flag therefore only matters if a received peer command somehow caused an HTTP handler to run (which can't happen: the HTTP handlers are driven by `server.handleClient()` from the main loop, not from the ESP-NOW RX context). In practice the recursion doesn't exist because the two code paths are disjoint; `_syncCommandInProgress` is a belt-and-suspenders defense.

**Real concern:** the flag is a plain `bool`, not `volatile`, not atomic. The ESP-NOW receive callback runs on the WiFi system task (see §1.4); the HTTP handlers run on the Arduino `loop()` task. If an HTTP `POST /api/brightness` is mid-flight on the loop task while an ESP-NOW `MSG_SET_BRIGHTNESS` from a peer arrives:

1. HTTP handler reads `_syncCommandInProgress == false` at line 2005 → proceeds to broadcast.
2. Simultaneously, the ESP-NOW task runs `applyBrightnessToTeensy`, which writes to Teensy UART.
3. Both tasks now write to `TEENSY_SERIAL` interleaved — frame corruption.

`TEENSY_SERIAL.write()` is not synchronized between the two tasks. There is no mutex. The sequences of bytes pushed by `sendTeensyCommand(0x06, 1); write(brightness); write(0xFE)` (3 bytes from HTTP handler) and the 3-byte sequence from `applyBrightnessToTeensy` can be reordered byte-wise into `[0xFF, 0xFF, 0x06, 0x06, 1, 1, val1, val2, 0xFE, 0xFE]`, and the Teensy protocol parser will desync.

This is not the echo-loop you asked about, but it's a real data race in the same subsystem. Recommend: either route all Teensy-UART writes through a queue drained by the main loop, or wrap `sendTeensyCommand` + the follow-up `write()`s with a FreeRTOS mutex. (Shares root cause with §1.4.)

**`state.*` is also touched from both tasks:** `applyModeToTeensy` writes `state.currentMode` / `state.currentIndex` (lines 3014–3015), `applyBrightnessToTeensy` writes `state.brightness` (line 3055), `applyFrameRateToTeensy` writes `state.frameRate` / `state.cachedFrameDelay` (lines 3067–3068). These same fields are read and written by HTTP handlers. Torn reads of `state.frameRate` (a `uint16_t`) are theoretically possible on ESP32 but practically benign (word-aligned). Bigger issue is logical consistency: a status poll (`handleStatus`, line 1939) can observe an intermediate state where `currentMode` was updated but `currentIndex` wasn't. See §2.7.

---

### 2.2 HTTP handler robustness — generally good, several notable issues

Handlers audited: `handleSetMode` (1956), `handleSetBrightness` (1989), `handleSetFrameRate` (2015), `handlePowerMode` (2044), `handleUploadPattern` (2106), `handleUploadSequence` (2171), `handleUploadImage` (2285), `handleLiveFrame` (2459), `handleSDList` (2525), `handleSDInfo` (2570), `handleSDDelete` (2585), `handleSDLoad` (2621), `handleSDPatternList` (2694), `handleSDPatternSave` (2737), `handleSDPatternLoad` (2772), `handleMultiPoiStatus` (3092), `handleMultiPoiPair` (3136), `handleMultiPoiUnpair` (3141), `handleMultiPoiSyncMode` (3157), `handleMultiPoiPeerCmd` (3197), `handleDeviceConfigUpdate` (not viewed in detail), `handleGetLEDConfig` (3756), `handleSetLEDConfig` (3768), `handleWifiStatus` / `handleWifiConnect` / `handleWifiDisconnect` (3703+).

**Good patterns seen:**
- All JSON-taking handlers call `deserializeJson(doc, body)` and check its return. If it fails, most return 400. ✅
- `handleUploadSequence` (lines 2171–2283) does thorough per-item validation: range-checks pattern index 0–18 (line 2239), image index 0–127 (line 2247), clamps `itemCount` at 10 (line 2228), enforces `dur >= 100` (line 2234). ✅
- `handleSDPatternSave` / `handleSDPatternLoad` use `isValidPresetName` (lines 2685–2692) to reject path-traversal characters — only `[A-Za-z0-9_-]` allowed, max length `MAX_SD_FILENAME_LEN = 32`. ✅
- `handleSetLEDConfig` bounds-checks `numLeds ∈ [2,32]` and `sacrificialLeds < numLeds` (line 3783). ✅
- `handleMultiPoiSyncMode` tolerates both numeric and string "mode" values and rejects invalid ones with 400 (lines 3171–3190). ✅
- `handleUploadPattern` clamps `index` and `type` to `kMaxPatternIndex = 18` (lines 2136–2141) rather than rejecting — forgiving but safe. ✅

**Gaps / bugs:**

**a. `handlePowerMode` (lines 2044–2104) — hand-rolled JSON parser, not ArduinoJson.**
```cpp
int idx = body.indexOf("\"mode\":");   // line 2047
int start = idx + 7;                    // line 2049
while (... && (body[start] == ' ' || body[start] == '"')) start++;
String modeStr = body.substring(start);
modeStr = modeStr.substring(0, modeStr.indexOf('"'));
```
This is the only handler not using ArduinoJson. It will silently accept malformed payloads where `"mode":` appears inside another string (e.g. a value containing the literal `"mode":"fake"`). Low severity because the resulting `modeStr` still has to match one of the five string literals, but it's inconsistent with the rest of the file and a future reviewer will read it wrong.

**b. `handleSetMode` (lines 1956–1987) — no bounds check on `mode` or `index`.**
```cpp
if (doc["mode"].is<int>()) state.currentMode = doc["mode"].as<uint8_t>();
if (doc["index"].is<int>()) state.currentIndex = doc["index"].as<uint8_t>();
```
A POST with `{"mode": 255, "index": 255}` is accepted and forwarded to the Teensy. Per `CLAUDE.md`, valid modes are 0–4 (idle/image/pattern/sequence/live). `index` bounds depend on the mode (0–127 for images, 0–18 for patterns, 0–4 for sequences). Nothing here enforces any of that. The Teensy should defensively clamp, but per the "ESP32 basic input sanitization" rule in the project instructions, this handler should also reject out-of-range input. Recommend: add `if (mode > 4)` check, return 400.

**c. `handleSetBrightness` (lines 1989–2013) — no range validation.**
`state.brightness = doc["brightness"].as<uint8_t>();` silently wraps / truncates to 0–255 (fine as `uint8_t`), but there's no check for missing-value vs. negative. A POST with `{"brightness": -1}` becomes `255` due to the integer-to-`uint8_t` cast. Low severity.

**d. `handleSetFrameRate` (lines 2015–2042) — range issue.**
Accepts any `uint16_t`. Per the problem statement README says 10–120, `key_facts.md` says 10–1000. This handler accepts 0 and does protect against div-by-zero at line 2024 (`frameRate > 0 ? 1000/frameRate : 20`). But it forwards `fps=0` to the Teensy as two literal zero bytes, which the Teensy may interpret differently (see §4.7). Also no upper bound: `fps=65535` is accepted and forwarded. Recommend: clamp to device's actual supported range.

**e. `handleLiveFrame` (lines 2459–2493) — silent success on malformed body.**
```cpp
if (deserializeJson(doc, body) == DeserializationError::Ok) {
  JsonArray pixels = doc["pixels"].as<JsonArray>();
  // loop copying r/g/b
}
// Falls through to send Teensy command with zeroed rgb[] buffer
```
If JSON parse fails, `memset(rgb, 0, sizeof(rgb))` (line 2467) leaves the array black and the handler proceeds to send a 0x05 command with `ledCount*3` zero bytes to the Teensy, then responds `200 OK`. A malformed `/api/live` request blanks the LEDs silently. At ~60 Hz this is almost a "kill switch." Recommend: on parse failure, return 400 without touching the Teensy.

**f. `handleMultiPoiPair` (lines 3136–3139) — no auth, no rate limiting.**
Any HTTP client on the AP can issue unlimited pair requests. Each one broadcasts an ESP-NOW frame (§1.2 threat model already covered the consuming side). Also no body validation — `POST /api/multipoi/pair` with any or no body works.

**g. `handleMultiPoiPeerCmd` (lines 3197–3258) — `peerIdx` not bounds-checked in this handler.**
```cpp
int peerIdx = doc["peer"] | -1;
if (peerIdx == -1) { /* 400 */ return; }   // only catches missing/literal -1
// ... then passed directly to espNowSync.sendPeerX(peerIdx, ...)
```
Values like `peerIdx = 99` or `-2` are passed through. The `espNowSync` layer (§1 lines 289, 299, 306, 313) does check `< 0 || >= _peerCount`, so this is defense-in-depth only, but also means a bad request silently returns `200 OK` with nothing happening. Minor.

**h. `handleSDList` (line 2536) — static 2 KB stack buffer.**
`uint8_t buffer[2048];` on a task stack; `handleSDPatternList` (line 2708) allocates `uint8_t buffer[4096]`. Combined with other stack usage in the same handler chain, be aware that the WebServer task's default stack is 8 KB on Arduino-ESP32. These are legal but leave little headroom.

**i. `handleNotFound` (lines 2892–2903) — light path-traversal risk.**
```cpp
String path = server.uri();
if (SPIFFS.exists(path)) { /* streamFile(path) */ }
```
Passes the raw URI through to SPIFFS. ESP32 SPIFFS is flat (no real directories) and rejects paths with weird characters, so this is essentially safe, but a defensive check (reject `..`, reject paths not starting with `/`, reject non-alnum-slash-dot-dash-underscore) would be cheap insurance. Not a real exploit today.

**j. None of the handlers that write `preferences` (lines 3273, 3305–3309, 3635–3636, 3743–3744, 3793–3794) rate-limit writes.** A client that POSTs `/api/wifi/connect` in a tight loop would burn NVS flash (~100k cycles rated). Realistic attack surface small (needs to be on the AP already), but worth noting. Same applies to `/api/device/config` and `/api/hardware/leds`. No debounce, no check-before-write (the handler at line 3792 unconditionally writes even if the values are unchanged).

**k. Content-length is not validated before `deserializeJson`.** Every handler calls `server.arg("plain")` and passes the resulting `String` to `deserializeJson`. The underlying `WebServer` library does bound the request body by its own `HTTP_MAX_POST_ARGS`/`HTTP_UPLOAD_BUFLEN` settings, but the individual handlers don't inspect `Content-Length` header. An attacker sending a 10 KB JSON body to `/api/brightness` forces a 10 KB `String` allocation on the heap and then a JSON parse. ArduinoJson v7's `JsonDocument` is dynamic, so it'll allocate further to parse. Multiple concurrent connections × repeated large bodies = heap exhaustion. Recommend: add an early `if (body.length() > N)` guard per-handler based on expected payload size.

---

### 2.3 Image upload path — static 76.8 KB buffer, filename-driven dimensions

**`handleUploadImage` (lines 2285–2457) — the buffer is a fixed static allocation, not `new`.**
```cpp
static uint8_t imageBuffer[MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3];   // line 2292
// = 400 * 64 * 3 = 76800 bytes in .bss
```
No unbounded allocations; no `new uint8_t[huge]`. The 76,800-byte static is a permanent RAM cost (ESP32 has 320 KB DRAM, so ~24% of it is gone to this one buffer). PSRAM is available on S3 variants but the buffer is declared without `EXT_RAM_BSS_ATTR` so it lives in internal RAM regardless. Recommend: if PSRAM is present on target hardware, move to PSRAM.

**Bounds enforcement (lines 2317–2332):**
- Declared dimensions from filename: rejected if `width < 1 || width > MAX_IMAGE_WIDTH || height < 1 || height > MAX_IMAGE_HEIGHT`. ✅
- Declared payload: rejected if `declaredBytes > MAX_UPLOAD_BYTES`. ✅
- Received data: rejected if `bufferIndex + upload.currentSize > MAX_UPLOAD_BYTES` (line 2338). ✅

**But: the filename parser silently defaults on parse failure (lines 2304–2324).**
```cpp
static uint16_t imageWidth = 32;    // STATIC — retains last upload's value
static uint16_t imageHeight = 32;
...
int underscoreIdx = fname.indexOf('_');
int xIdx = fname.indexOf('x', underscoreIdx);
if (underscoreIdx != -1 && xIdx != -1) {
  imageWidth = fname.substring(underscoreIdx + 1, xIdx).toInt();
  ...
}
```
If the filename doesn't match `*_WxH.rgb`, the dimensions retain the previous upload's values (due to `static`). The subsequent data copy then proceeds with the *wrong* dimensions but the bytes-received clamp will still prevent buffer overflow. The Teensy, however, will be told the wrong `imageWidth`/`imageHeight` and will misrender the new bytes as if they were the previous image's shape. Recommend: reset `imageWidth = imageHeight = 0;` on each `UPLOAD_FILE_START`, and reject the upload explicitly if dimensions can't be parsed.

**Truncated upload handling (lines 2350–2370):**
- On `UPLOAD_FILE_END`, if `bufferIndex < expectedSize`, the code recomputes `imageHeight = bufferIndex / (imageWidth * 3)` and proceeds (line 2362). It trusts the `imageWidth` and truncates the image vertically to match the received bytes.
- This is forgiving but can be gamed: upload a filename claiming `400x64` (67,200 bytes needed after width/height/3 math) but send only 300 bytes. The handler will compute `imageHeight = 300 / (400*3) = 0`, clamp to 1, and forward to the Teensy as a `400x1` image. The Teensy receives one row of 300 bytes. Not a crash, but confusing behavior with no user feedback. Recommend: fail closed — if `bufferIndex < expectedSize`, return 413 or 400.

**Framing:** the Teensy send at lines 2387–2400 pushes `0xFF 0x02 [len_hi:1][len_lo:1][w_lo:1][w_hi:1][h_lo:1][h_hi:1] [rgb...] 0xFE`. If the upload is aborted mid-stream (`UPLOAD_FILE_ABORTED`, line 2453), the code sends 500 but **has not yet sent anything to the Teensy**, so the Teensy UART state is unaffected. ✅ However, if the upload fails silently (WiFi drop between UPLOAD_FILE_WRITE chunks), neither `UPLOAD_FILE_END` nor `UPLOAD_FILE_ABORTED` may ever fire, leaving `bufferIndex` populated and `uploadRejected == false` — the next call sequence will re-enter `UPLOAD_FILE_START` which resets both, so no persistent corruption. ✅

**Image slot assignment (lines 2375–2377):**
```cpp
uint8_t assignedSlot = state.nextImageSlot;
state.nextImageSlot = (state.nextImageSlot < 127) ? (state.nextImageSlot + 1) : 5;
```
Starts at slot 5, wraps back to 5 at 127. Slots 0–4 are preserved as "preloaded demos." Wraps silently — the 128th uploaded image overwrites slot 5, evicting the user's earliest upload. Not a bug, but not surfaced to the UI either. The `/api/status` response returns `imageCount` (line 1949) but never exposes `nextImageSlot`.

**`state.imageCount` is saturation-incremented at `255`** (line 2405) but never decremented, even on delete. So the advertised count diverges from reality after deletes.

---

### 2.4 Inline HTML embedded UI (lines 419–1913) — passes all constraints

I grepped for every forbidden construct. Verdict: the embedded UI follows the constraints.

| Constraint | Result |
|---|---|
| No `<script src="https://...">` | ✅ No external script tags found |
| No ES module syntax (`import` / `export`) | ✅ Script is a plain inline `<script>` block; grep for `^import `/`^export ` in the HTML range returned no hits inside `rawliteral` |
| No hardcoded `192.168.x.x` URLs in `fetch()` | ✅ All 25 `fetch()` calls (lines 1139–1835) use relative paths (`/api/status`, `/api/multipoi/pair`, etc.). The `192.168.4.1` occurrences at lines 954, 956, 1005 are **user-visible display text** telling the user what IP to point their phone at — they're not in any code path. |
| No `localStorage`/`sessionStorage` | ✅ grep returned no hits |
| No reference to a build step / bundled asset | ✅ All JS/CSS is inlined in the `rootPage` PROGMEM literal |
| No `http://` URLs in JS | ✅ The single http ref at line 1781 is `'http://'+d.mdnsName+'.local'` — it's constructed from the server-provided hostname for display purposes, not for fetch |

**Only notable finding:** `handleRoot` at line 1915 prefers `/index.html` from SPIFFS (the bundled Vite/React webui output) and falls back to the embedded `rootPage`. So in production, the inline HTML is the **fallback** (if `uploadfs` wasn't run), not the primary UI. Both paths coexist. Good design.

`handleManifest` and `handleServiceWorker` are referenced but the PWA manifest JSON (lines 2806+) uses inline `data:image/svg+xml,...` icons — no external asset references. ✅

---

### 2.5 WiFi STA fallback logic

**Confirmed clean** (lines 3287–3290, 327–352, 308–325).

`loadDeviceConfig` (line 3289): `staSsid = preferences.getString("sta_ssid", "")` — default is empty string, consistent with the project's claim that cleanup removed default STA creds. ✅

`setupWiFi` (lines 327–352):
- Mode is `WIFI_AP_STA` unconditionally (line 333) — so the SoftAP always comes up regardless of STA state. ✅
- STA connect only attempted if `staSsid.length() > 0` (line 348). ✅
- Non-blocking: `WiFi.begin(...)` returns immediately; no spin-wait for connection.

`onWiFiEvent` (lines 308–325):
- On `STA_DISCONNECTED`, only reconnects if `staSsid.length() > 0` (line 315). ✅
- If creds empty, logs once and skips (`"reconnect skipped"`). No reconnect loop. ✅

**One small concern:** the event handler calls `WiFi.begin(staSsid.c_str(), staPassword.c_str())` from the WiFi event callback (line 317). Depending on Arduino-ESP32 version, `WiFi.begin` from inside an event callback can deadlock if it tries to acquire the same internal mutex. Most recent versions handle this, but worth a test on the actual target version. Not flagged as a bug without repro.

---

### 2.6 Preferences keys — catalog

All under namespace `"povpoi"`, opened at lines 3266, 3304, 3634, 3742, 3792.

| Key | Type | Default | Writer | Notes |
|---|---|---|---|---|
| `deviceId` | String | `WiFi.macAddress()` | 3273, 3305 | Read at 3270; auto-initialized on first read if missing. ✅ |
| `deviceName` | String | `"POV Poi " + macSuffix` | 3306 | Read at 3278. ✅ |
| `syncGroup` | String | `""` | 3307 | Read at 3281. ✅ |
| `autoSync` | bool | `AUTO_SYNC_ENABLED` (false) | 3308 | Read at 3284. ✅ |
| `syncInterval` | ulong | `AUTO_SYNC_INTERVAL` (30000) | 3309 | Read at 3285. ✅ |
| `sta_ssid` | String | `""` | 3635, removed at 3743 | Read at 3289. ✅ (see §2.5) |
| `sta_password` | String | `""` | 3636, removed at 3744 | Read at 3290. ⚠️ Stored **plaintext** in NVS. No encryption. An attacker with physical flash access can dump it. Also `/api/wifi/status` may return the SSID (not audited in detail) — make sure the password is never returned. |
| `hw_numLeds` | UInt | 32 | 3793 | Read at 3293. ✅ |
| `hw_sacLeds` | UInt | 1 | 3794 | Read at 3294. ✅ |

**No conflicts, no keys read without an initial-write path.** `saveDeviceConfig` at line 3303 writes `deviceId` + `deviceName` + `syncGroup` + `autoSync` + `syncInterval` together but `loadDeviceConfig` also auto-persists `deviceId` on first boot if the key is empty (line 3273). OK.

**One concern — `preferences.begin(...)` without matching `preferences.end()`:** the Preferences object is a module-scoped global (line 125), and `loadDeviceConfig` calls `preferences.end()` at line 3299 but `saveDeviceConfig` at line 3310 does too. The later handlers (`saveWifiStaConfig` line 3637, `handleWifiDisconnect` line 3745, and `handleSetLEDConfig` line 3795) all pair begin/end. ✅ No orphaned `begin()` found.

**Default values don't leak secrets** (defaults are empty strings or sensible constants). ✅

**Heads-up for future refactor:** Preferences keys are limited to **15 characters** on ESP32. All keys above are ≤ 14. `syncInterval` is 12 chars, OK. Keep this in mind before renaming.

---

### 2.7 State machine hazards

`SystemState state` is declared at line 178 with 11 fields. Access pattern:

| Accessor | Context | Fields touched |
|---|---|---|
| `setup()` (line 246) | main task, once | init all |
| `loop()` (line 264) | main task | reads `lastDiscovery`, `lastSync` |
| `checkTeensyConnection()` (line 2921) | main task (loop) | writes `currentMode`, `currentIndex`, `sdCardPresent`, `connected` |
| HTTP handlers | WebServer task (same as main task in Arduino-ESP32) | read/write most fields |
| `apply*ToTeensy()` (lines 3012–3077) | **ESP-NOW system task** | writes `currentMode`, `currentIndex`, `brightness`, `frameRate`, `cachedFrameDelay` |

**The hazard:** in Arduino-ESP32, `server.handleClient()` runs cooperatively on the main Arduino loop task. The ESP-NOW receive callback runs on a different task. So `apply*ToTeensy` mutating `state.brightness` races against `handleSetBrightness` reading `doc["brightness"]`, writing `state.brightness`, and calling `broadcastBrightness`. §2.1 already covered the UART-byte interleave; here I note the same race on `state.*`.

**Specific TOCTOU in `handleStatus` (line 1939):** builds a JSON response from `state.currentMode`, `state.currentIndex`, `state.brightness`, `state.frameRate` individually. If a concurrent peer-command arrives between reads, the response will mix old/new values. Visible as UI flicker; harmless.

**Specific TOCTOU in `handlePowerMode` (lines 2044–2104):** modifies `state.frameRate` at line 2077, then `state.brightness` at line 2090 conditionally. Each section sends a Teensy command in between. If a peer command arrives between them, the Teensy will see an intermediate state.

**Static local state in `handleUploadImage`** (lines 2292–2297): `static uint8_t imageBuffer[...]`, `static size_t bufferIndex`, `static bool uploadRejected`, `static uint16_t imageWidth/imageHeight`. If two overlapping uploads were possible the statics would collide, but the single-threaded `WebServer` serializes requests. ✅ (Single-server assumption — if this ever moves to a multi-client async server, rewrite needed.)

**No explicit synchronization primitives** (no FreeRTOS mutex, no semaphore) anywhere in the file. Entire thread-safety story rests on "handlers are on the main task and ESP-NOW callback writes are quick." See §1.4 and §2.1 for the mitigation recommendation (queue-and-defer).

---

### 2.8 Additional findings in `esp32_firmware.ino`

| # | Location | Finding |
|---|---|---|
| a | 175 | `_syncCommandInProgress` declared as global, not wrapped in a namespace or `static`. Leading underscore on a file-scope name is an unusual C++ convention but not reserved. Cosmetic. |
| b | 257 | `state.nextImageSlot = 5;` — starts at slot 5 to preserve slots 0–4 as demo images. Not a bug, but worth documenting as a protocol constant somewhere shared with the Teensy. |
| c | 283–284 | `espNowSync.setLocalState(...)` called every 5 s from `loop`. Good. But heartbeat-sent state (line 420 in `espnow_sync.h`) is only as fresh as this 5 s tick — for a control target that changes at 60+ FPS, heartbeats will usually lag. Minor. |
| d | 308–325 | `onWiFiEvent` — no handler for `ARDUINO_EVENT_WIFI_AP_STACONNECTED` or `...STADISCONNECTED`. Means the firmware has no way to see/log when clients join the AP. Not required, but a common instrumentation gap. |
| e | 1929–1935 | `handleRoot` fallback streams the embedded UI in 1 KB chunks using a VLA-adjacent `char buf[chunkSize + 1]` on stack (not a true VLA since `chunkSize` is a compile-time `const size_t`). 1025-byte stack allocation on the handler's task — fine. |
| f | 2402 | `Serial.printf("Image forwarded to Teensy (slot %u)\n", assignedSlot);` — uses `%u` with a `uint8_t`. Works on this compiler but technically `%u` expects `unsigned int`. Default argument promotion makes it correct; no bug. |
| g | 2455 | `UPLOAD_FILE_ABORTED` returns 500; should probably be 400 or 499. Purely informational. |
| h | 3083 | `applySyncTimeToTeensy` sends MSB-first on offset but `sendTeensyCommand` uses `dataLen = 4` — the protocol framing looks right, Teensy side deferred to §4. |
| i | 3228–3250 | `handleMultiPoiPeerCmd` uses `doc["field"] \| default` pattern for every field; no type-check for mistyped fields. `{"type": "red"}` → `type` gets 0 (the default), silently accepted. Low severity. |
| j | 414 | `server.enableCORS(true);` — permissive CORS (`Access-Control-Allow-Origin: *`). For a device on a local AP this is fine, but if the device joins a hostile STA network, any website the user visits could `fetch('http://poi.local/api/brightness')` from the user's browser via a DNS rebinding / local-network attack. Worth documenting. |
| k | ~114–119 | Constants like `MAX_IMAGE_WIDTH 400` and `MAX_IMAGE_HEIGHT 64` must stay in sync with Teensy's `IMAGE_MAX_WIDTH` / `IMAGE_HEIGHT`. Not enforced by any shared header. Drift risk. |
| l | 3379 (spotted earlier) | `performSync`/`discoverPeers` path is a completely separate legacy HTTP/mDNS sync system (the `PeerDevice peers[MAX_PEERS]` table), unrelated to ESP-NOW. Two sync systems coexist; Section 1 already flagged the `PEER_TIMEOUT` naming confusion. |

---

## Section 2 open questions / cross-refs for later

- BLE bridge: does `applyX` get called from the BLE callback too? If so, same race as §2.1 on `TEENSY_SERIAL`. → Section 3.
- Teensy side of commands 0x01/0x03/0x05/0x06/0x07/0x08/0x09/0x0B/0x20/0x21/0x22/0x24/0x30: framing, bounds, error handling. → Section 4.
- Is the inline HTML actually reachable in production, or is it truly fallback-only? It's dead code if every deployment uploads SPIFFS. Usefulness question, not a bug.

---

## Sections 3–5

**Not yet started.** Pausing per ground rules.
