/*
 * ESP-NOW Multi-Poi Synchronization
 *
 * Provides low-latency peer-to-peer communication between poi using
 * ESP-NOW (connectionless, ~1ms latency, works alongside WiFi AP mode).
 *
 * Sync Modes:
 *   MIRROR      - Both poi display the same content in phase (default)
 *   INDEPENDENT - Each poi displays different content, controlled separately
 *
 * Protocol: [MAGIC:2][MSG_TYPE:1][SEQ:1][PAYLOAD:variable]
 *   Max ESP-NOW payload: 250 bytes
 */

#ifndef ESPNOW_SYNC_H
#define ESPNOW_SYNC_H

#include <esp_now.h>
#include <WiFi.h>

// Protocol constants
#define SYNC_MAGIC_0 0x4E  // 'N' for Nebula
#define SYNC_MAGIC_1 0x50  // 'P' for Poi
#define SYNC_MAX_PAYLOAD 244  // 250 - 4 byte header - 2 byte magic

// Message types
#define MSG_PAIR_REQUEST    0x01
#define MSG_PAIR_RESPONSE   0x02
#define MSG_UNPAIR          0x03
#define MSG_SET_MODE        0x10
#define MSG_SET_PATTERN     0x11
#define MSG_SET_BRIGHTNESS  0x12
#define MSG_SET_FRAMERATE   0x13
#define MSG_HEARTBEAT       0x20
#define MSG_SYNC_TIME       0x30
#define MSG_PEER_CMD        0x40  // Command targeting a specific peer in independent mode

// Sync modes
enum SyncMode {
  SYNC_MIRROR = 0,       // Both poi show same content
  SYNC_INDEPENDENT = 1   // Each poi shows different content
};

// Peer state
enum PeerState {
  PEER_NONE = 0,
  PEER_DISCOVERING = 1,
  PEER_PAIR_SENT = 2,
  PEER_PAIRED = 3
};

// Peer info structure
struct SyncPeer {
  uint8_t mac[6];
  char name[32];
  PeerState state;
  unsigned long lastSeen;
  uint8_t currentMode;
  uint8_t currentIndex;
  uint8_t brightness;
  bool online;
};

// Heartbeat payload (sent periodically to keep peers aware)
struct __attribute__((packed)) HeartbeatPayload {
  uint8_t mode;
  uint8_t index;
  uint8_t brightness;
  uint8_t frameDelay;
  uint32_t uptimeMs;
  uint8_t syncMode;  // Current SyncMode
  char name[24];
};

// Mode change payload
struct __attribute__((packed)) ModePayload {
  uint8_t mode;
  uint8_t index;
};

// Pattern payload
struct __attribute__((packed)) PatternPayload {
  uint8_t index;
  uint8_t type;
  uint8_t r1, g1, b1;
  uint8_t r2, g2, b2;
  uint8_t speed;
};

// Brightness payload
struct __attribute__((packed)) BrightnessPayload {
  uint8_t brightness;
};

// Frame rate payload
struct __attribute__((packed)) FrameRatePayload {
  uint8_t frameDelay;
};

// Sync time payload (for pattern phase alignment)
struct __attribute__((packed)) SyncTimePayload {
  uint32_t masterMillis;  // Sender's millis() value
};

// Pair request/response payload
struct __attribute__((packed)) PairPayload {
  uint8_t mac[6];
  char name[24];
  uint8_t accepted;  // Only used in response: 1=accepted, 0=rejected
};

// Peer command payload (for independent mode - command targeting a specific peer)
struct __attribute__((packed)) PeerCmdPayload {
  uint8_t cmdType;    // Which command (MSG_SET_MODE, MSG_SET_PATTERN, etc.)
  uint8_t data[32];   // Command-specific data
  uint8_t dataLen;
};

// Callback types
typedef void (*SyncModeChangeCallback)(uint8_t mode, uint8_t index);
typedef void (*SyncPatternCallback)(uint8_t index, uint8_t type, uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint8_t speed);
typedef void (*SyncBrightnessCallback)(uint8_t brightness);
typedef void (*SyncFrameRateCallback)(uint8_t frameDelay);
typedef void (*SyncTimeCallback)(int32_t offsetMs);
typedef void (*SyncPeerUpdateCallback)(const SyncPeer* peer);

// Maximum paired peers (ESP-NOW supports up to 20 unencrypted)
#define MAX_SYNC_PEERS 6

class ESPNowSync {
public:
  ESPNowSync() : _peerCount(0), _syncMode(SYNC_MIRROR), _seq(0),
                 _lastHeartbeat(0), _lastTimeSync(0),
                 _onModeChange(nullptr), _onPattern(nullptr),
                 _onBrightness(nullptr), _onFrameRate(nullptr),
                 _onSyncTime(nullptr), _onPeerUpdate(nullptr),
                 _autoPairEnabled(true), _timeOffset(0) {
    memset(_peers, 0, sizeof(_peers));
    memset(_localMac, 0, sizeof(_localMac));
    _localName[0] = '\0';
  }

  // Initialize ESP-NOW (call after WiFi.mode() is set)
  bool begin(const char* deviceName) {
    strncpy(_localName, deviceName, sizeof(_localName) - 1);
    _localName[sizeof(_localName) - 1] = '\0';

    // Get own MAC address
    WiFi.macAddress(_localMac);

    Serial.printf("[SYNC] Local MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
      _localMac[0], _localMac[1], _localMac[2],
      _localMac[3], _localMac[4], _localMac[5]);
    Serial.printf("[SYNC] Device name: %s\n", _localName);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("[SYNC] ESP-NOW init failed");
      return false;
    }

    // Store singleton for static callbacks
    _instance = this;

    // Register callbacks
    esp_now_register_send_cb(onSendStatic);
    esp_now_register_recv_cb(onRecvStatic);

    // Register broadcast peer for discovery
    esp_now_peer_info_t broadcastPeer;
    memset(&broadcastPeer, 0, sizeof(broadcastPeer));
    memset(broadcastPeer.peer_addr, 0xFF, 6);  // Broadcast address
    broadcastPeer.channel = 0;  // Use current channel
    broadcastPeer.encrypt = false;

    if (esp_now_add_peer(&broadcastPeer) != ESP_OK) {
      Serial.println("[SYNC] Failed to add broadcast peer");
    }

    Serial.println("[SYNC] ESP-NOW initialized");
    return true;
  }

  // Main loop - call from Arduino loop()
  void loop() {
    unsigned long now = millis();

    // Send heartbeat every 2 seconds
    if (now - _lastHeartbeat > 2000) {
      _lastHeartbeat = now;
      sendHeartbeat();
      checkPeerTimeouts();
    }

    // Send time sync every 5 seconds (only when paired and in mirror mode)
    if (_syncMode == SYNC_MIRROR && hasPairedPeer() && now - _lastTimeSync > 5000) {
      _lastTimeSync = now;
      sendTimeSync();
    }
  }

  // Set sync mode
  void setSyncMode(SyncMode mode) {
    _syncMode = mode;
    Serial.printf("[SYNC] Sync mode: %s\n", mode == SYNC_MIRROR ? "MIRROR" : "INDEPENDENT");
  }

  SyncMode getSyncMode() const { return _syncMode; }

  // Start pairing discovery (broadcast pair request)
  void startPairing() {
    Serial.println("[SYNC] Broadcasting pair request...");

    PairPayload payload;
    memcpy(payload.mac, _localMac, 6);
    strncpy(payload.name, _localName, sizeof(payload.name) - 1);
    payload.name[sizeof(payload.name) - 1] = '\0';
    payload.accepted = 0;

    // Broadcast pair request
    uint8_t broadcastAddr[6];
    memset(broadcastAddr, 0xFF, 6);
    sendMessage(broadcastAddr, MSG_PAIR_REQUEST, (uint8_t*)&payload, sizeof(payload));
  }

  // Unpair all peers
  void unpairAll() {
    for (int i = 0; i < _peerCount; i++) {
      if (_peers[i].state == PEER_PAIRED) {
        // Send unpair notification
        sendMessage(_peers[i].mac, MSG_UNPAIR, nullptr, 0);
        // Remove from ESP-NOW peer list
        esp_now_del_peer(_peers[i].mac);
      }
    }
    _peerCount = 0;
    memset(_peers, 0, sizeof(_peers));
    Serial.println("[SYNC] All peers unpaired");
  }

  // Unpair a specific peer by index
  void unpairPeer(int index) {
    if (index < 0 || index >= _peerCount) return;
    if (_peers[index].state == PEER_PAIRED) {
      sendMessage(_peers[index].mac, MSG_UNPAIR, nullptr, 0);
      esp_now_del_peer(_peers[index].mac);
    }
    // Shift remaining peers
    for (int i = index; i < _peerCount - 1; i++) {
      _peers[i] = _peers[i + 1];
    }
    _peerCount--;
    memset(&_peers[_peerCount], 0, sizeof(SyncPeer));
    Serial.printf("[SYNC] Peer %d unpaired\n", index);
  }

  // Send commands to paired peers (called when local state changes)
  // These are the functions the main firmware calls when user changes settings

  void broadcastModeChange(uint8_t mode, uint8_t index) {
    if (_syncMode != SYNC_MIRROR || !hasPairedPeer()) return;
    ModePayload payload = { mode, index };
    broadcastToPeers(MSG_SET_MODE, (uint8_t*)&payload, sizeof(payload));
  }

  void broadcastPattern(uint8_t idx, uint8_t type,
                        uint8_t r1, uint8_t g1, uint8_t b1,
                        uint8_t r2, uint8_t g2, uint8_t b2,
                        uint8_t speed) {
    if (_syncMode != SYNC_MIRROR || !hasPairedPeer()) return;
    PatternPayload payload = { idx, type, r1, g1, b1, r2, g2, b2, speed };
    broadcastToPeers(MSG_SET_PATTERN, (uint8_t*)&payload, sizeof(payload));
  }

  void broadcastBrightness(uint8_t brightness) {
    if (_syncMode != SYNC_MIRROR || !hasPairedPeer()) return;
    BrightnessPayload payload = { brightness };
    broadcastToPeers(MSG_SET_BRIGHTNESS, (uint8_t*)&payload, sizeof(payload));
  }

  void broadcastFrameRate(uint8_t frameDelay) {
    if (_syncMode != SYNC_MIRROR || !hasPairedPeer()) return;
    FrameRatePayload payload = { frameDelay };
    broadcastToPeers(MSG_SET_FRAMERATE, (uint8_t*)&payload, sizeof(payload));
  }

  // Send command to a specific peer (for independent mode)
  void sendPeerModeChange(int peerIndex, uint8_t mode, uint8_t index) {
    if (peerIndex < 0 || peerIndex >= _peerCount) return;
    if (_peers[peerIndex].state != PEER_PAIRED) return;
    ModePayload payload = { mode, index };
    sendMessage(_peers[peerIndex].mac, MSG_SET_MODE, (uint8_t*)&payload, sizeof(payload));
  }

  void sendPeerPattern(int peerIndex, uint8_t idx, uint8_t type,
                       uint8_t r1, uint8_t g1, uint8_t b1,
                       uint8_t r2, uint8_t g2, uint8_t b2,
                       uint8_t speed) {
    if (peerIndex < 0 || peerIndex >= _peerCount) return;
    if (_peers[peerIndex].state != PEER_PAIRED) return;
    PatternPayload payload = { idx, type, r1, g1, b1, r2, g2, b2, speed };
    sendMessage(_peers[peerIndex].mac, MSG_SET_PATTERN, (uint8_t*)&payload, sizeof(payload));
  }

  void sendPeerBrightness(int peerIndex, uint8_t brightness) {
    if (peerIndex < 0 || peerIndex >= _peerCount) return;
    if (_peers[peerIndex].state != PEER_PAIRED) return;
    BrightnessPayload payload = { brightness };
    sendMessage(_peers[peerIndex].mac, MSG_SET_BRIGHTNESS, (uint8_t*)&payload, sizeof(payload));
  }

  void sendPeerFrameRate(int peerIndex, uint8_t frameDelay) {
    if (peerIndex < 0 || peerIndex >= _peerCount) return;
    if (_peers[peerIndex].state != PEER_PAIRED) return;
    FrameRatePayload payload = { frameDelay };
    sendMessage(_peers[peerIndex].mac, MSG_SET_FRAMERATE, (uint8_t*)&payload, sizeof(payload));
  }

  // Register callbacks for when this device receives commands from a peer
  void onModeChange(SyncModeChangeCallback cb) { _onModeChange = cb; }
  void onPattern(SyncPatternCallback cb) { _onPattern = cb; }
  void onBrightness(SyncBrightnessCallback cb) { _onBrightness = cb; }
  void onFrameRate(SyncFrameRateCallback cb) { _onFrameRate = cb; }
  void onSyncTime(SyncTimeCallback cb) { _onSyncTime = cb; }
  void onPeerUpdate(SyncPeerUpdateCallback cb) { _onPeerUpdate = cb; }

  // Getters
  int getPeerCount() const { return _peerCount; }
  bool hasPairedPeer() const {
    for (int i = 0; i < _peerCount; i++) {
      if (_peers[i].state == PEER_PAIRED && _peers[i].online) return true;
    }
    return false;
  }

  const SyncPeer* getPeer(int index) const {
    if (index < 0 || index >= _peerCount) return nullptr;
    return &_peers[index];
  }

  int32_t getTimeOffset() const { return _timeOffset; }

  void setAutoPair(bool enabled) { _autoPairEnabled = enabled; }
  bool getAutoPair() const { return _autoPairEnabled; }

  const uint8_t* getLocalMac() const { return _localMac; }
  const char* getLocalName() const { return _localName; }

  void setLocalName(const char* name) {
    strncpy(_localName, name, sizeof(_localName) - 1);
    _localName[sizeof(_localName) - 1] = '\0';
  }

  // Set local state for heartbeat reporting
  void setLocalState(uint8_t mode, uint8_t index, uint8_t brightness, uint8_t frameDelay) {
    _localMode = mode;
    _localIndex = index;
    _localBrightness = brightness;
    _localFrameDelay = frameDelay;
  }

private:
  static ESPNowSync* _instance;

  SyncPeer _peers[MAX_SYNC_PEERS];
  int _peerCount;
  SyncMode _syncMode;
  uint8_t _seq;
  unsigned long _lastHeartbeat;
  unsigned long _lastTimeSync;
  uint8_t _localMac[6];
  char _localName[32];
  int32_t _timeOffset;  // Offset to align with peer's millis()
  bool _autoPairEnabled;

  // Local state for heartbeat
  uint8_t _localMode = 0;
  uint8_t _localIndex = 0;
  uint8_t _localBrightness = 128;
  uint8_t _localFrameDelay = 20;

  // Callbacks
  SyncModeChangeCallback _onModeChange;
  SyncPatternCallback _onPattern;
  SyncBrightnessCallback _onBrightness;
  SyncFrameRateCallback _onFrameRate;
  SyncTimeCallback _onSyncTime;
  SyncPeerUpdateCallback _onPeerUpdate;

  // Send a message to a specific MAC
  void sendMessage(const uint8_t* mac, uint8_t msgType, const uint8_t* payload, uint8_t payloadLen) {
    uint8_t buf[250];
    buf[0] = SYNC_MAGIC_0;
    buf[1] = SYNC_MAGIC_1;
    buf[2] = msgType;
    buf[3] = _seq++;
    if (payload && payloadLen > 0) {
      memcpy(buf + 4, payload, min((int)payloadLen, 246));
    }
    uint8_t totalLen = 4 + payloadLen;

    esp_err_t result = esp_now_send(mac, buf, totalLen);
    if (result != ESP_OK) {
      Serial.printf("[SYNC] Send failed (type 0x%02X): %d\n", msgType, result);
    }
  }

  // Broadcast to all paired peers
  void broadcastToPeers(uint8_t msgType, const uint8_t* payload, uint8_t payloadLen) {
    for (int i = 0; i < _peerCount; i++) {
      if (_peers[i].state == PEER_PAIRED && _peers[i].online) {
        sendMessage(_peers[i].mac, msgType, payload, payloadLen);
      }
    }
  }

  void sendHeartbeat() {
    HeartbeatPayload payload;
    payload.mode = _localMode;
    payload.index = _localIndex;
    payload.brightness = _localBrightness;
    payload.frameDelay = _localFrameDelay;
    payload.uptimeMs = millis();
    payload.syncMode = (uint8_t)_syncMode;
    strncpy(payload.name, _localName, sizeof(payload.name) - 1);
    payload.name[sizeof(payload.name) - 1] = '\0';

    // Broadcast heartbeat (so unpaired devices can also discover us)
    uint8_t broadcastAddr[6];
    memset(broadcastAddr, 0xFF, 6);
    sendMessage(broadcastAddr, MSG_HEARTBEAT, (uint8_t*)&payload, sizeof(payload));
  }

  void sendTimeSync() {
    SyncTimePayload payload;
    payload.masterMillis = millis();
    broadcastToPeers(MSG_SYNC_TIME, (uint8_t*)&payload, sizeof(payload));
  }

  void checkPeerTimeouts() {
    unsigned long now = millis();
    for (int i = 0; i < _peerCount; i++) {
      if (_peers[i].state == PEER_PAIRED) {
        bool wasOnline = _peers[i].online;
        _peers[i].online = (now - _peers[i].lastSeen < 10000);  // 10s timeout
        if (wasOnline && !_peers[i].online) {
          Serial.printf("[SYNC] Peer '%s' went offline\n", _peers[i].name);
          if (_onPeerUpdate) _onPeerUpdate(&_peers[i]);
        }
      }
      // Clean up stale discovery peers (not yet paired, older than 30s)
      if (_peers[i].state == PEER_PAIR_SENT && now - _peers[i].lastSeen > 30000) {
        esp_now_del_peer(_peers[i].mac);
        for (int j = i; j < _peerCount - 1; j++) {
          _peers[j] = _peers[j + 1];
        }
        _peerCount--;
        memset(&_peers[_peerCount], 0, sizeof(SyncPeer));
        i--;  // Recheck this index
      }
    }
  }

  // Find peer by MAC
  int findPeer(const uint8_t* mac) {
    for (int i = 0; i < _peerCount; i++) {
      if (memcmp(_peers[i].mac, mac, 6) == 0) return i;
    }
    return -1;
  }

  // Add or get a peer slot
  int addPeerSlot(const uint8_t* mac) {
    int idx = findPeer(mac);
    if (idx >= 0) return idx;
    if (_peerCount >= MAX_SYNC_PEERS) return -1;
    idx = _peerCount++;
    memcpy(_peers[idx].mac, mac, 6);
    return idx;
  }

  // Register peer with ESP-NOW
  void registerESPNowPeer(const uint8_t* mac) {
    // Check if already registered
    if (esp_now_is_peer_exist(mac)) return;

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("[SYNC] Failed to register ESP-NOW peer");
    }
  }

  // Handle incoming message
  void handleMessage(const uint8_t* mac, const uint8_t* data, int len) {
    if (len < 4) return;
    if (data[0] != SYNC_MAGIC_0 || data[1] != SYNC_MAGIC_1) return;

    uint8_t msgType = data[2];
    // uint8_t seq = data[3];  // Available for dedup if needed
    const uint8_t* payload = data + 4;
    int payloadLen = len - 4;

    // Skip messages from ourselves
    if (memcmp(mac, _localMac, 6) == 0) return;

    switch (msgType) {
      case MSG_PAIR_REQUEST:
        handlePairRequest(mac, payload, payloadLen);
        break;
      case MSG_PAIR_RESPONSE:
        handlePairResponse(mac, payload, payloadLen);
        break;
      case MSG_UNPAIR:
        handleUnpair(mac);
        break;
      case MSG_SET_MODE:
        handleSetMode(mac, payload, payloadLen);
        break;
      case MSG_SET_PATTERN:
        handleSetPattern(mac, payload, payloadLen);
        break;
      case MSG_SET_BRIGHTNESS:
        handleSetBrightness(mac, payload, payloadLen);
        break;
      case MSG_SET_FRAMERATE:
        handleSetFrameRate(mac, payload, payloadLen);
        break;
      case MSG_HEARTBEAT:
        handleHeartbeat(mac, payload, payloadLen);
        break;
      case MSG_SYNC_TIME:
        handleSyncTime(mac, payload, payloadLen);
        break;
      default:
        Serial.printf("[SYNC] Unknown message type: 0x%02X\n", msgType);
        break;
    }
  }

  void handlePairRequest(const uint8_t* mac, const uint8_t* payload, int len) {
    if (len < (int)sizeof(PairPayload)) return;
    PairPayload* req = (PairPayload*)payload;

    Serial.printf("[SYNC] Pair request from '%s' (%02X:%02X:%02X:%02X:%02X:%02X)\n",
      req->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (!_autoPairEnabled) {
      Serial.println("[SYNC] Auto-pair disabled, ignoring");
      return;
    }

    // Auto-accept: register peer and send response
    registerESPNowPeer(mac);

    int idx = addPeerSlot(mac);
    if (idx < 0) {
      Serial.println("[SYNC] No peer slots available");
      return;
    }

    strncpy(_peers[idx].name, req->name, sizeof(_peers[idx].name) - 1);
    _peers[idx].name[sizeof(_peers[idx].name) - 1] = '\0';
    _peers[idx].state = PEER_PAIRED;
    _peers[idx].lastSeen = millis();
    _peers[idx].online = true;

    // Send pair response
    PairPayload resp;
    memcpy(resp.mac, _localMac, 6);
    strncpy(resp.name, _localName, sizeof(resp.name) - 1);
    resp.name[sizeof(resp.name) - 1] = '\0';
    resp.accepted = 1;
    sendMessage(mac, MSG_PAIR_RESPONSE, (uint8_t*)&resp, sizeof(resp));

    Serial.printf("[SYNC] Paired with '%s'\n", _peers[idx].name);
    if (_onPeerUpdate) _onPeerUpdate(&_peers[idx]);
  }

  void handlePairResponse(const uint8_t* mac, const uint8_t* payload, int len) {
    if (len < (int)sizeof(PairPayload)) return;
    PairPayload* resp = (PairPayload*)payload;

    if (!resp->accepted) {
      Serial.printf("[SYNC] Pair rejected by '%s'\n", resp->name);
      return;
    }

    registerESPNowPeer(mac);

    int idx = addPeerSlot(mac);
    if (idx < 0) return;

    strncpy(_peers[idx].name, resp->name, sizeof(_peers[idx].name) - 1);
    _peers[idx].name[sizeof(_peers[idx].name) - 1] = '\0';
    _peers[idx].state = PEER_PAIRED;
    _peers[idx].lastSeen = millis();
    _peers[idx].online = true;

    Serial.printf("[SYNC] Pair accepted by '%s'\n", _peers[idx].name);
    if (_onPeerUpdate) _onPeerUpdate(&_peers[idx]);
  }

  void handleUnpair(const uint8_t* mac) {
    int idx = findPeer(mac);
    if (idx < 0) return;

    Serial.printf("[SYNC] Unpair from '%s'\n", _peers[idx].name);
    esp_now_del_peer(mac);

    for (int i = idx; i < _peerCount - 1; i++) {
      _peers[i] = _peers[i + 1];
    }
    _peerCount--;
    memset(&_peers[_peerCount], 0, sizeof(SyncPeer));
  }

  void handleSetMode(const uint8_t* mac, const uint8_t* payload, int len) {
    int idx = findPeer(mac);
    if (idx < 0 || _peers[idx].state != PEER_PAIRED) return;
    if (len < (int)sizeof(ModePayload)) return;

    ModePayload* p = (ModePayload*)payload;
    Serial.printf("[SYNC] Mode from '%s': mode=%d index=%d\n", _peers[idx].name, p->mode, p->index);

    _peers[idx].currentMode = p->mode;
    _peers[idx].currentIndex = p->index;

    if (_onModeChange) _onModeChange(p->mode, p->index);
  }

  void handleSetPattern(const uint8_t* mac, const uint8_t* payload, int len) {
    int idx = findPeer(mac);
    if (idx < 0 || _peers[idx].state != PEER_PAIRED) return;
    if (len < (int)sizeof(PatternPayload)) return;

    PatternPayload* p = (PatternPayload*)payload;
    Serial.printf("[SYNC] Pattern from '%s': type=%d\n", _peers[idx].name, p->type);

    if (_onPattern) _onPattern(p->index, p->type, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2, p->speed);
  }

  void handleSetBrightness(const uint8_t* mac, const uint8_t* payload, int len) {
    int idx = findPeer(mac);
    if (idx < 0 || _peers[idx].state != PEER_PAIRED) return;
    if (len < (int)sizeof(BrightnessPayload)) return;

    BrightnessPayload* p = (BrightnessPayload*)payload;
    Serial.printf("[SYNC] Brightness from '%s': %d\n", _peers[idx].name, p->brightness);

    _peers[idx].brightness = p->brightness;
    if (_onBrightness) _onBrightness(p->brightness);
  }

  void handleSetFrameRate(const uint8_t* mac, const uint8_t* payload, int len) {
    int idx = findPeer(mac);
    if (idx < 0 || _peers[idx].state != PEER_PAIRED) return;
    if (len < (int)sizeof(FrameRatePayload)) return;

    FrameRatePayload* p = (FrameRatePayload*)payload;
    Serial.printf("[SYNC] FrameRate from '%s': %d\n", _peers[idx].name, p->frameDelay);

    if (_onFrameRate) _onFrameRate(p->frameDelay);
  }

  void handleHeartbeat(const uint8_t* mac, const uint8_t* payload, int len) {
    if (len < (int)sizeof(HeartbeatPayload)) return;
    HeartbeatPayload* hb = (HeartbeatPayload*)payload;

    int idx = findPeer(mac);
    if (idx >= 0) {
      // Update existing peer
      _peers[idx].lastSeen = millis();
      bool wasOffline = !_peers[idx].online;
      _peers[idx].online = true;
      _peers[idx].currentMode = hb->mode;
      _peers[idx].currentIndex = hb->index;
      _peers[idx].brightness = hb->brightness;
      strncpy(_peers[idx].name, hb->name, sizeof(_peers[idx].name) - 1);
      _peers[idx].name[sizeof(_peers[idx].name) - 1] = '\0';

      if (wasOffline) {
        Serial.printf("[SYNC] Peer '%s' back online\n", _peers[idx].name);
        if (_onPeerUpdate) _onPeerUpdate(&_peers[idx]);
      }
    }
    // If we get a heartbeat from an unknown device and auto-pair is on,
    // we don't auto-pair from heartbeats - only from explicit pair requests.
    // But we note the device exists for the web UI to show as "discoverable".
  }

  void handleSyncTime(const uint8_t* mac, const uint8_t* payload, int len) {
    int idx = findPeer(mac);
    if (idx < 0 || _peers[idx].state != PEER_PAIRED) return;
    if (len < (int)sizeof(SyncTimePayload)) return;

    SyncTimePayload* p = (SyncTimePayload*)payload;

    // Calculate time offset: positive means peer is ahead of us
    _timeOffset = (int32_t)p->masterMillis - (int32_t)millis();

    if (_onSyncTime) _onSyncTime(_timeOffset);
  }

  // Static callbacks (ESP-NOW requires C-style function pointers)
  static void onSendStatic(const uint8_t* mac_addr, esp_now_send_status_t status) {
    // Could log delivery failures here if needed
  }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  static void onRecvStatic(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (_instance) _instance->handleMessage(info->src_addr, data, len);
  }
#else
  static void onRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (_instance) _instance->handleMessage(mac_addr, data, len);
  }
#endif
};

// Singleton instance pointer (defined in .cpp or at bottom of include)
ESPNowSync* ESPNowSync::_instance = nullptr;

#endif // ESPNOW_SYNC_H
