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

## Sections 2–5

**Not yet started.** Pausing here per the "pause briefly and ask" ground rule.
