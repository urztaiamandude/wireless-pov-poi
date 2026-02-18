/*
 * Nebula Poi - ESP32 Firmware
 * 
 * This firmware creates a WiFi Access Point with a web portal for controlling
 * the Nebula Poi. It communicates with the Teensy 4.1 via Serial to send
 * images, patterns, and sequences.
 * 
 * Features:
 * - WiFi Access Point mode
 * - BLE support via Nordic UART Service
 * - Web server with file upload
 * - Image/pattern/sequence management
 * - Serial communication with Teensy
 * - REST API for web UI and integrations
 * - ESP-NOW multi-poi synchronization (mirror + independent modes)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// BLE support
#ifdef BLE_ENABLED
#include "src/ble_bridge.h"
#endif

// ESP-NOW multi-poi sync
#include "src/espnow_sync.h"

// Forward declarations for PlatformIO compilation
void setupWiFi();
void setupWebServer();
void checkTeensyConnection();
void handleRoot();
void handleStatus();
void handleSetMode();
void handleSetBrightness();
void handleSetFrameRate();
void handleUploadPattern();
void handleUploadImage();
void handleLiveFrame();
void handleSDList();
void handleSDDelete();
void handleSDInfo();
void handleSDLoad();
void handleManifest();
void handleServiceWorker();
void handleNotFound();
void sendFile(const char* path, const char* contentType);
void sendTeensyCommand(uint8_t cmd, uint8_t dataLen);
bool readTeensyResponse(uint8_t expectedMarker, uint8_t* buffer, size_t maxLen, size_t& bytesRead, unsigned long timeout = 500);

// Sync function declarations (legacy HTTP sync)
void handleSyncStatus();
void handleSyncDiscover();
void handleSyncExecute();
void handleSyncData();
void handleSyncPush();
void handleDeviceConfig();
void handleDeviceConfigUpdate();
void discoverPeers();
void performSync(String peerId);
String getDeviceId();
void loadDeviceConfig();
void saveDeviceConfig();

// ESP-NOW multi-poi sync declarations
void setupESPNowSync();
void handleMultiPoiStatus();
void handleMultiPoiPair();
void handleMultiPoiUnpair();
void handleMultiPoiSyncMode();
void handleMultiPoiPeerCmd();
void applyModeToTeensy(uint8_t mode, uint8_t index);
void applyPatternToTeensy(uint8_t idx, uint8_t type, uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint8_t speed);
void applyBrightnessToTeensy(uint8_t brightness);
void applyFrameRateToTeensy(uint8_t frameDelay);
void applySyncTimeToTeensy(int32_t offsetMs);

// WiFi Configuration
const char* ssid = "POV-POI-WiFi";
const char* password = "povpoi123";

// Serial Configuration
#define TEENSY_SERIAL Serial2
#define SERIAL_BAUD 115200
const uint8_t kMaxPatternIndex = 17;

// Image dimension limits
// Updated to match Teensy PSRAM capabilities: 400px width, 64px height (2x32 LEDs)
#define MAX_IMAGE_WIDTH 400
#define MAX_IMAGE_HEIGHT 64

// Sync Configuration
#define AUTO_SYNC_ENABLED false
#define AUTO_SYNC_INTERVAL 30000  // 30 seconds
#define PEER_DISCOVERY_INTERVAL 60000  // 60 seconds
#define PEER_TIMEOUT 120000  // 2 minutes

// Web Server
WebServer server(80);

// Preferences for persistent storage
Preferences preferences;

// BLE Bridge
#ifdef BLE_ENABLED
BLEBridge* bleBridge = nullptr;
#endif

// Device configuration
struct DeviceConfig {
  String deviceId;
  String deviceName;
  String syncGroup;
  bool autoSync;
  unsigned long syncInterval;
} deviceConfig;

// Peer device structure
struct PeerDevice {
  String deviceId;
  String deviceName;
  String ipAddress;
  unsigned long lastSeen;
  bool online;
};

// Maximum peers (legacy HTTP sync)
#define MAX_PEERS 5
PeerDevice peers[MAX_PEERS];
int peerCount = 0;

// ESP-NOW multi-poi sync
ESPNowSync espNowSync;
bool _syncCommandInProgress = false;  // Prevents echo loops when applying peer commands

// System state
struct SystemState {
  uint8_t currentMode;
  uint8_t currentIndex;
  uint8_t brightness;
  uint8_t frameRate;
  uint8_t cachedFrameDelay;  // Cached value: 1000 / frameRate
  bool connected;
  unsigned long lastSync;
  unsigned long lastDiscovery;
  bool sdCardPresent;
} state;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 Nebula Poi Controller Starting...");
  
  // Initialize Teensy Serial
  TEENSY_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, 44, 43);  // RX=GPIO44 (U0RXD), TX=GPIO43 (U0TXD)
  
  // Initialize SPIFFS for web files
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS Mounted");
  
  // Load device configuration
  loadDeviceConfig();
  Serial.print("Device ID: ");
  Serial.println(deviceConfig.deviceId);
  Serial.print("Device Name: ");
  Serial.println(deviceConfig.deviceName);
  
  // Initialize BLE Bridge (before WiFi to avoid conflicts)
  #ifdef BLE_ENABLED
  bleBridge = new BLEBridge(&TEENSY_SERIAL);
  bleBridge->setup();
  Serial.println("BLE Bridge initialized");
  #endif
  
  // Initialize WiFi Access Point
  setupWiFi();

  // Initialize ESP-NOW multi-poi sync (must be after WiFi init)
  setupESPNowSync();

  // Initialize web server
  setupWebServer();

  // Initialize mDNS
  String mdnsName = "povpoi-" + deviceConfig.deviceId.substring(deviceConfig.deviceId.length() - 6);
  mdnsName.replace(":", "");
  if (MDNS.begin(mdnsName.c_str())) {
    Serial.printf("mDNS responder started: http://%s.local\n", mdnsName.c_str());
    // Add service for discovery
    MDNS.addService("povpoi", "tcp", 80);
    MDNS.addServiceTxt("povpoi", "tcp", "deviceId", deviceConfig.deviceId.c_str());
    MDNS.addServiceTxt("povpoi", "tcp", "deviceName", deviceConfig.deviceName.c_str());
  }
  
  // Initialize system state
  state.currentMode = 0;
  state.currentIndex = 0;
  state.brightness = 128;
  state.frameRate = 50;
  state.cachedFrameDelay = 1000 / 50;  // Pre-calculate frame delay
  state.connected = false;
  state.lastSync = 0;
  state.lastDiscovery = 0;
  state.sdCardPresent = false;
  
  Serial.println("ESP32 Nebula Poi Controller Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Handle BLE communications
  #ifdef BLE_ENABLED
  if (bleBridge) {
    bleBridge->loop();
  }
  #endif
  
  server.handleClient();

  // ESP-NOW sync loop (heartbeats, time sync, peer timeouts)
  espNowSync.loop();

  // Check Teensy connection periodically
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    checkTeensyConnection();
    // Keep sync heartbeat state up to date
    espNowSync.setLocalState(state.currentMode, state.currentIndex,
                             state.brightness,
                             state.frameRate > 0 ? 1000 / state.frameRate : 20);
  }
  
  // Periodic peer discovery
  if (millis() - state.lastDiscovery > PEER_DISCOVERY_INTERVAL) {
    state.lastDiscovery = millis();
    discoverPeers();
  }
  
  // Auto-sync if enabled
  if (deviceConfig.autoSync && peerCount > 0 && 
      millis() - state.lastSync > deviceConfig.syncInterval) {
    state.lastSync = millis();
    // Auto-sync with first available peer
    for (int i = 0; i < peerCount; i++) {
      if (peers[i].online) {
        Serial.println("Auto-syncing with peer: " + peers[i].deviceName);
        performSync(peers[i].deviceId);
        break;
      }
    }
  }
}

void setupWiFi() {
  Serial.println("Starting Access Point...");
  
  // Configure AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  // Configure IP (192.168.4.1 is default)
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
}

void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, handleRoot);
  
  // API endpoints
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/mode", HTTP_POST, handleSetMode);
  server.on("/api/brightness", HTTP_POST, handleSetBrightness);
  server.on("/api/framerate", HTTP_POST, handleSetFrameRate);
  server.on("/api/pattern", HTTP_POST, handleUploadPattern);
  server.on("/api/image", HTTP_POST, 
    []() { 
      // Final response sent in handleUploadImage after upload completes
    },
    handleUploadImage);
  server.on("/api/live", HTTP_POST, handleLiveFrame);
  
  // Sync API endpoints
  server.on("/api/sync/status", HTTP_GET, handleSyncStatus);
  server.on("/api/sync/discover", HTTP_POST, handleSyncDiscover);
  server.on("/api/sync/execute", HTTP_POST, handleSyncExecute);
  server.on("/api/sync/data", HTTP_GET, handleSyncData);
  server.on("/api/sync/push", HTTP_POST, handleSyncPush);
  
  // Device configuration endpoints
  server.on("/api/device/config", HTTP_GET, handleDeviceConfig);
  server.on("/api/device/config", HTTP_POST, handleDeviceConfigUpdate);

  // ESP-NOW multi-poi sync endpoints
  server.on("/api/multipoi/status", HTTP_GET, handleMultiPoiStatus);
  server.on("/api/multipoi/pair", HTTP_POST, handleMultiPoiPair);
  server.on("/api/multipoi/unpair", HTTP_POST, handleMultiPoiUnpair);
  server.on("/api/multipoi/syncmode", HTTP_POST, handleMultiPoiSyncMode);
  server.on("/api/multipoi/peercmd", HTTP_POST, handleMultiPoiPeerCmd);
  
  // SD card management endpoints
  server.on("/api/sd/list", HTTP_GET, handleSDList);
  server.on("/api/sd/info", HTTP_GET, handleSDInfo);
  server.on("/api/sd/delete", HTTP_POST, handleSDDelete);
  server.on("/api/sd/load", HTTP_POST, handleSDLoad);
  
  // PWA support
  server.on("/manifest.json", HTTP_GET, handleManifest);
  server.on("/sw.js", HTTP_GET, handleServiceWorker);
  
  // Static files
  server.on("/style.css", HTTP_GET, []() {
    sendFile("/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, []() {
    sendFile("/script.js", "application/javascript");
  });
  
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="theme-color" content="#667eea">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="Nebula Poi">
    <link rel="manifest" href="/manifest.json">
    <title>Nebula Poi Controller</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        .status {
            background: rgba(255,255,255,0.2);
            padding: 10px;
            border-radius: 10px;
            margin-top: 15px;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
        }
        h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        .control-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #333;
        }
        select, input[type="range"], input[type="file"], input[type="number"], button {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            transition: all 0.3s;
        }
        select:focus, input[type="file"]:focus, input[type="number"]:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            cursor: pointer;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        button:active {
            transform: translateY(0);
        }
        .slider-value {
            text-align: center;
            font-size: 1.2em;
            font-weight: 600;
            color: #667eea;
            margin-top: 8px;
        }
        .pattern-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
            gap: 10px;
            margin-top: 15px;
        }
        .pattern-btn {
            padding: 20px;
            background: white;
            border: 2px solid #667eea;
            color: #667eea;
            cursor: pointer;
            border-radius: 8px;
            transition: all 0.3s;
        }
        .pattern-btn:hover {
            background: #667eea;
            color: white;
            transform: scale(1.05);
        }
        .color-picker-group {
            display: flex;
            gap: 10px;
            margin-top: 10px;
        }
        .color-picker {
            flex: 1;
        }
        input[type="color"] {
            width: 100%;
            height: 50px;
            border: 2px solid #ddd;
            border-radius: 8px;
            cursor: pointer;
        }
        .live-canvas {
            width: 100%;
            height: 200px;
            border: 2px solid #ddd;
            border-radius: 8px;
            background: #000;
            cursor: crosshair;
        }
        
        /* Mobile-first responsive design */
        @media (max-width: 768px) {
            body {
                padding: 10px;
            }
            
            .container {
                border-radius: 12px;
            }
            
            h1 {
                font-size: 1.8em;
            }
            
            h2 {
                font-size: 1.3em;
            }
            
            .header {
                padding: 20px;
            }
            
            .content {
                padding: 15px;
            }
            
            .section {
                padding: 15px;
                margin-bottom: 15px;
            }
            
            button {
                width: 100%;
                margin: 5px 0;
                padding: 15px;
                font-size: 16px;
                min-height: 44px;
                touch-action: manipulation;
            }
            
            input[type="range"] {
                width: 100%;
                height: 40px;
                -webkit-appearance: none;
                appearance: none;
            }
            
            input[type="range"]::-webkit-slider-thumb {
                -webkit-appearance: none;
                width: 32px;
                height: 32px;
                border-radius: 50%;
                background: #667eea;
                cursor: pointer;
            }
            
            input[type="range"]::-moz-range-thumb {
                width: 32px;
                height: 32px;
                border-radius: 50%;
                background: #667eea;
                cursor: pointer;
                border: none;
            }
            
            input[type="file"] {
                width: 100%;
                padding: 15px;
                font-size: 16px;
            }
            
            select {
                font-size: 16px;
                min-height: 44px;
            }
            
            .pattern-grid {
                grid-template-columns: repeat(2, 1fr);
                gap: 8px;
            }
            
            .pattern-btn {
                padding: 15px;
                font-size: 14px;
            }
            
            .color-picker-group {
                flex-direction: column;
            }
            
            input[type="color"] {
                height: 60px;
            }
            
            .live-canvas {
                height: 150px;
            }
            
            .status {
                font-size: 14px;
            }
        }
        
        /* Improve touch targets */
        button, input, select {
            min-height: 44px;
        }
        
        /* Better contrast for readability */
        body {
            font-size: 16px;
            line-height: 1.6;
        }
        
        /* LED Preview Strip */
        .led-preview {
            display: flex;
            gap: 2px;
            padding: 10px;
            background: #111;
            border-radius: 8px;
            margin-top: 5px;
            overflow-x: auto;
        }
        .led {
            width: 15px;
            height: 30px;
            border-radius: 3px;
            background: #222;
            transition: background 0.1s;
        }
        /* PWA Install Button */
        #installButton {
            display: none;
            position: fixed;
            bottom: 20px;
            right: 20px;
            z-index: 1000;
            background: #4CAF50;
            color: white;
            border: none;
            padding: 15px 25px;
            border-radius: 50px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
            cursor: pointer;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <!-- PWA Install Button -->
    <button id="installButton">📱 Install App</button>
    
    <div class="container">
        <div class="header">
            <h1>🎨 Nebula Poi Controller</h1>
            <div class="status">
                <div>Status: <span id="connection-status">Connecting...</span></div>
                <div>Mode: <span id="mode-status">Idle</span></div>
            </div>
        </div>
        
        <div class="content">
            <!-- LED Preview -->
            <div class="section">
                <h2>🔮 LED Preview</h2>
                <div class="led-preview" id="led-preview"></div>
            </div>
            
            <!-- Mode Selection -->
            <div class="section">
                <h2>Display Mode</h2>
                <div class="control-group">
                    <label for="mode-select">Select Mode:</label>
                    <select id="mode-select" onchange="changeMode()">
                        <option value="0">Idle (Off)</option>
                        <option value="1">Image Display</option>
                        <option value="2" selected>Pattern Display</option>
                        <option value="3">Sequence</option>
                        <option value="4">Live Mode</option>
                    </select>
                </div>
                <div style="margin-top: 10px; padding: 10px; background: rgba(102, 126, 234, 0.1); border-radius: 5px; font-size: 14px;">
                    <strong>Available Content:</strong><br>
                    📷 Images: 0=Smiley, 1=Rainbow, 2=Heart<br>
                    🎨 Pattern slots (0-17; preloaded 0-6): 0=Rainbow, 1=Fire, 2=Comet,<br>
                    3=Breathing, 4=Plasma, 5=Split Spin, 6=Theater Chase<br>
                    🎬 Sequences: 0=Demo Mix
                </div>
                <div class="control-group" style="margin-top: 15px;">
                    <label for="content-index">Content Index (0-17):</label>
                    <input type="number" id="content-index" min="0" max="17" value="0" onchange="changeContentIndex()">
                </div>
            </div>
            
            <!-- System Controls -->
            <div class="section">
                <h2>System Controls</h2>
                <div class="control-group">
                    <label for="brightness">Brightness: <span id="brightness-value">128</span></label>
                    <input type="range" id="brightness" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
                </div>
                <div class="control-group">
                    <label for="framerate">Frame Rate: <span id="framerate-value">50</span> FPS</label>
                    <input type="range" id="framerate" min="10" max="120" value="50" oninput="updateFrameRate(this.value)">
                </div>
            </div>
            
            <!-- Patterns -->
            <div class="section">
                <h2>Visual Patterns</h2>
                <div class="pattern-grid">
                    <button class="pattern-btn" data-pattern="0" onclick="setPattern(0)">🌈 Rainbow</button>
                    <button class="pattern-btn" data-pattern="1" onclick="setPattern(1)">🌊 Wave</button>
                    <button class="pattern-btn" data-pattern="2" onclick="setPattern(2)">🎨 Gradient</button>
                    <button class="pattern-btn" data-pattern="3" onclick="setPattern(3)">✨ Sparkle</button>
                    <button class="pattern-btn" data-pattern="4" onclick="setPattern(4)">🔥 Fire</button>
                    <button class="pattern-btn" data-pattern="5" onclick="setPattern(5)">☄️ Comet</button>
                    <button class="pattern-btn" data-pattern="6" onclick="setPattern(6)">💨 Breathing</button>
                    <button class="pattern-btn" data-pattern="7" onclick="setPattern(7)">⚡ Strobe</button>
                    <button class="pattern-btn" data-pattern="8" onclick="setPattern(8)">🌠 Meteor</button>
                    <button class="pattern-btn" data-pattern="9" onclick="setPattern(9)">🖌️ Wipe</button>
                    <button class="pattern-btn" data-pattern="10" onclick="setPattern(10)">🌀 Plasma</button>
                    <button class="pattern-btn" data-pattern="16" onclick="setPattern(16)">🌓 Split Spin</button>
                    <button class="pattern-btn" data-pattern="17" onclick="setPattern(17)">🎭 Theater Chase</button>
                </div>
            </div>
            
            <!-- Audio Reactive Patterns -->
            <div class="section">
                <h2>🎵 Audio Reactive</h2>
                <p style="font-size: 12px; color: #666; margin-bottom: 10px;">Requires MAX9814 microphone amplifier on Teensy pin A0</p>
                <div class="pattern-grid">
                    <button class="pattern-btn" data-pattern="11" onclick="setPattern(11)" style="background: linear-gradient(135deg, #00ff88 0%, #ff0088 100%); color: white; border: none;">📊 VU Meter</button>
                    <button class="pattern-btn" data-pattern="12" onclick="setPattern(12)" style="background: linear-gradient(135deg, #ff0088 0%, #00ff88 100%); color: white; border: none;">💓 Pulse</button>
                    <button class="pattern-btn" data-pattern="13" onclick="setPattern(13)" style="background: linear-gradient(135deg, #ff0000 0%, #00ff00 50%, #0000ff 100%); color: white; border: none;">🌈 Audio Rainbow</button>
                    <button class="pattern-btn" data-pattern="14" onclick="setPattern(14)" style="background: linear-gradient(135deg, #8800ff 0%, #ff8800 100%); color: white; border: none;">🎯 Center Burst</button>
                    <button class="pattern-btn" data-pattern="15" onclick="setPattern(15)" style="background: linear-gradient(135deg, #ffff00 0%, #ff00ff 100%); color: white; border: none;">✨ Audio Sparkle</button>
                </div>
            </div>
            
            <!-- Pattern Settings -->
            <div class="section">
                <h2>Pattern Settings</h2>
                <div class="color-picker-group">
                    <div class="color-picker">
                        <label>Color 1:</label>
                        <input type="color" id="color1" value="#ff0000">
                    </div>
                    <div class="color-picker">
                        <label>Color 2:</label>
                        <input type="color" id="color2" value="#0000ff">
                    </div>
                </div>
                <div class="control-group" style="margin-top: 10px;">
                    <label for="pattern-speed">Pattern Speed: <span id="speed-value">50</span></label>
                    <input type="range" id="pattern-speed" min="1" max="255" value="50" oninput="updateSpeed(this.value)">
                </div>
            </div>
            
            <!-- Image Upload -->
            <div class="section">
                <h2>Upload Image</h2>
                <div class="control-group">
                    <label for="image-upload">Select Image:</label>
                    <input type="file" id="image-upload" accept="image/*">
                </div>
                <div class="control-group">
                    <label style="display: flex; align-items: center; gap: 10px;">
                        <input type="checkbox" id="aspect-ratio-lock" checked style="width: auto; margin: 0;">
                        Lock Aspect Ratio
                    </label>
                </div>
                <div class="control-group" style="display: flex; gap: 10px;">
                    <div style="flex: 1;">
                        <label for="image-width">Width (px):</label>
                        <input type="number" id="image-width" min="1" max="100" value="32" style="width: 100%; padding: 8px;" oninput="updateImageDimensions('width')">
                    </div>
                    <div style="flex: 1;">
                        <label for="image-height">Height (px):</label>
                        <input type="number" id="image-height" min="1" max="200" value="64" style="width: 100%; padding: 8px;" oninput="updateImageDimensions('height')">
                    </div>
                </div>
                <div class="control-group" style="display: flex; gap: 15px;">
                    <label style="display: flex; align-items: center; gap: 5px;">
                        <input type="checkbox" id="flip-vertical" style="width: auto; margin: 0;">
                        Flip Vertical
                    </label>
                    <label style="display: flex; align-items: center; gap: 5px;">
                        <input type="checkbox" id="flip-horizontal" style="width: auto; margin: 0;">
                        Flip Horizontal
                    </label>
                </div>
                <div class="control-group">
                    <button onclick="uploadImage()" style="margin-top: 5px;">Upload & Display</button>
                </div>
            </div>
            
            <!-- Live Mode -->
            <div class="section">
                <h2>Live Draw Mode</h2>
                <canvas id="live-canvas" class="live-canvas" width="320" height="200"></canvas>
                <button onclick="clearCanvas()" style="margin-top: 10px;">Clear</button>
            </div>
            
            <!-- Multi-Poi Sync -->
            <div class="section" id="sync-section">
                <h2>Multi-Poi Sync</h2>
                <div id="sync-status-box" style="margin-bottom: 15px; padding: 10px; background: rgba(102, 126, 234, 0.1); border-radius: 5px;">
                    <div>This Poi: <span id="sync-local-name">--</span></div>
                    <div>Sync Mode: <strong><span id="sync-mode-display">Mirror</span></strong></div>
                    <div>Paired: <span id="sync-paired-status">No peers</span></div>
                </div>

                <!-- Sync mode toggle -->
                <div class="control-group">
                    <label>Sync Mode:</label>
                    <div style="display: flex; gap: 10px;">
                        <button id="btn-mirror" onclick="setSyncMode('mirror')" style="flex:1; background: #667eea; color: white;">Mirror</button>
                        <button id="btn-independent" onclick="setSyncMode('independent')" style="flex:1; background: #ddd; color: #333;">Independent</button>
                    </div>
                    <div style="margin-top: 8px; font-size: 13px; color: #666;">
                        <strong>Mirror:</strong> Both poi show the same thing.<br>
                        <strong>Independent:</strong> Control each poi separately.
                    </div>
                </div>

                <!-- Pairing controls -->
                <div class="control-group" style="margin-top: 15px;">
                    <label>Pairing:</label>
                    <div style="display: flex; gap: 10px;">
                        <button onclick="pairPoi()" style="flex:1; background: #4CAF50; color: white;">Pair</button>
                        <button onclick="unpairPoi()" style="flex:1; background: #f44336; color: white;">Unpair All</button>
                    </div>
                </div>

                <!-- Peer list -->
                <div id="sync-peer-list" style="margin-top: 10px;"></div>

                <!-- Independent mode: peer controls (hidden in mirror mode) -->
                <div id="independent-controls" style="display: none; margin-top: 15px; border-top: 2px solid #667eea; padding-top: 15px;">
                    <h3 style="color: #667eea; margin-bottom: 10px;">Paired Poi Controls</h3>
                    <div id="peer-control-panels"></div>
                </div>
            </div>

            <!-- SD Card Management -->
            <div class="section">
                <h2>💾 SD Card Storage</h2>
                <div id="sd-status" style="margin-bottom: 15px; padding: 10px; background: rgba(102, 126, 234, 0.1); border-radius: 5px;">
                    <div>Status: <span id="sd-status-text">Checking...</span></div>
                    <div id="sd-info" style="margin-top: 5px; font-size: 14px; color: #666;"></div>
                </div>
                <div class="control-group">
                    <button onclick="refreshSDList()" style="margin-bottom: 10px;">🔄 Refresh List</button>
                    <div id="sd-file-list" style="max-height: 200px; overflow-y: auto; border: 1px solid #ddd; border-radius: 5px; padding: 10px; background: white;">
                        <div style="text-align: center; color: #999;">No files loaded</div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        const NUM_LEDS = 32;
        let currentMode = 2;
        let currentPattern = 0;
        let brightness = 128;
        // aspect ratio is stored as height / width
        let originalImageAspectRatio = 1.0;
        
        // LED Preview
        function initLEDPreview() {
            const container = document.getElementById('led-preview');
            container.innerHTML = '';
            for (let i = 0; i < NUM_LEDS; i++) {
                const led = document.createElement('div');
                led.className = 'led';
                led.id = 'led-' + i;
                container.appendChild(led);
            }
        }
        function setLEDPreview(i, r, g, b) {
            const led = document.getElementById('led-' + i);
            if (led) {
                const s = brightness / 255;
                led.style.background = 'rgb(' + Math.round(r*s) + ',' + Math.round(g*s) + ',' + Math.round(b*s) + ')';
            }
        }
        function updateLEDPreviewFromStatus(mode, index) {
            for (let i = 0; i < NUM_LEDS; i++) setLEDPreview(i, 40, 40, 40);
            if (mode === 0) return;
            const hue = (index * 16) % 256;
            for (let i = 0; i < NUM_LEDS; i++) {
                const h = ((i * 8 + hue) % 256) / 256;
                const r = h < 0.333 ? 1 : (h < 0.666 ? 1 - (h - 0.333) * 3 : 0);
                const g = h < 0.333 ? h * 3 : (h < 0.666 ? 1 : 1 - (h - 0.666) * 3);
                const b = h < 0.333 ? 0 : (h < 0.666 ? (h - 0.333) * 3 : 1);
                setLEDPreview(i, r*255, g*255, b*255);
            }
        }
        
        // Initialize
        initLEDPreview();
        updateStatus();
        setInterval(updateStatus, 2000);
        
        // Image upload file listener to calculate aspect ratio
        document.getElementById('image-upload').addEventListener('change', function(e) {
            if (e.target.files.length > 0) {
                const file = e.target.files[0];
                const reader = new FileReader();
                reader.onload = function(event) {
                    const img = new Image();
                    img.onload = function() {
                        originalImageAspectRatio = img.height / img.width;
                        // When aspect lock is on, treat height as the LED count
                        // entered by the user and adjust width automatically to
                        // preserve the original image aspect ratio, so the
                        // physical LED count stays authoritative.
                        const heightInput = document.getElementById('image-height');
                        const widthInput = document.getElementById('image-width');
                        let targetHeight = parseInt(heightInput.value) || 32;
                        targetHeight = Math.min(200, Math.max(1, targetHeight));
                        heightInput.value = targetHeight;

                        if (document.getElementById('aspect-ratio-lock').checked) {
                            const newWidth = Math.round(targetHeight / originalImageAspectRatio);
                            widthInput.value = Math.min(100, Math.max(1, newWidth));
                        }
                    };
                    img.src = event.target.result;
                };
                reader.readAsDataURL(file);
            }
        });
        
        // Function to handle dimension changes with aspect ratio lock
        function updateImageDimensions(changedField) {
            const aspectLock = document.getElementById('aspect-ratio-lock').checked;
            const widthInput = document.getElementById('image-width');
            const heightInput = document.getElementById('image-height');

            // When aspect ratio is locked, we treat HEIGHT as the source of truth:
            // it represents the number of physical LEDs. Width is then derived
            // from that LED count and the image's aspect ratio to avoid warping.
            if (aspectLock) {
                // If the user changes height, recompute width from height.
                if (changedField === 'height') {
                    let newHeight = parseInt(heightInput.value) || 32;
                    newHeight = Math.min(200, Math.max(1, newHeight));
                    heightInput.value = newHeight;

                    const newWidth = Math.round(newHeight / originalImageAspectRatio);
                    widthInput.value = Math.min(100, Math.max(1, newWidth));
                } else if (changedField === 'width') {
                    // If the user changes width while locked, ignore that as a
                    // source of truth and instead snap width back to the value
                    // implied by the current LED count (height).
                    let ledCount = parseInt(heightInput.value) || 32;
                    ledCount = Math.min(200, Math.max(1, ledCount));
                    heightInput.value = ledCount;

                    const newWidth = Math.round(ledCount / originalImageAspectRatio);
                    widthInput.value = Math.min(100, Math.max(1, newWidth));
                }
            }
        }
        
        // Live canvas setup
        const canvas = document.getElementById('live-canvas');
        const ctx = canvas.getContext('2d');
        let isDrawing = false;
        
        canvas.addEventListener('mousedown', startDrawing);
        canvas.addEventListener('mousemove', draw);
        canvas.addEventListener('mouseup', stopDrawing);
        canvas.addEventListener('mouseleave', stopDrawing);
        
        canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            startDrawing(e.touches[0]);
        });
        canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
            draw(e.touches[0]);
        });
        canvas.addEventListener('touchend', stopDrawing);
        
        function startDrawing(e) {
            isDrawing = true;
            draw(e);
        }
        
        function stopDrawing() {
            isDrawing = false;
            sendLiveFrame();
        }
        
        function draw(e) {
            if (!isDrawing) return;
            
            const rect = canvas.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            
            ctx.fillStyle = document.getElementById('color1').value;
            ctx.beginPath();
            ctx.arc(x, y, 5, 0, Math.PI * 2);
            ctx.fill();
        }
        
        function clearCanvas() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            sendLiveFrame();
        }
        
        function sendLiveFrame() {
            const cw = canvas.width;
            const ch = canvas.height;
            const midY = Math.floor(ch / 2);
            const pixels = [];
            for (let i = 0; i < NUM_LEDS; i++) {
                const x = Math.min(cw - 1, Math.floor(i * (cw / NUM_LEDS) + (cw / NUM_LEDS) / 2));
                const p = ctx.getImageData(x, midY, 1, 1).data;
                pixels.push({ r: p[0], g: p[1], b: p[2] });
                setLEDPreview(i, p[0], p[1], p[2]);
            }
            fetch('/api/live', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({pixels: pixels})
            });
        }
        
        async function updateStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('connection-status').textContent = 
                    data.connected ? 'Connected ✓' : 'Disconnected ✗';
                document.getElementById('mode-status').textContent = 
                    ['Idle', 'Image', 'Pattern', 'Sequence', 'Live'][data.mode] || 'Unknown';
                
                currentMode = data.mode;
                brightness = data.brightness !== undefined ? data.brightness : brightness;
                
                document.getElementById('mode-select').value = String(data.mode);
                document.getElementById('content-index').value = data.index;
                document.getElementById('brightness').value = brightness;
                document.getElementById('brightness-value').textContent = brightness;
                document.getElementById('framerate').value = data.framerate;
                document.getElementById('framerate-value').textContent = data.framerate;
                
                if (data.mode === 2) {
                    currentPattern = data.index;
                    updatePatternActiveState();
                }
                updateLEDPreviewFromStatus(data.mode, data.index);
                
                if (data.sdCardPresent !== undefined) {
                    const sdStatusText = document.getElementById('sd-status-text');
                    if (data.sdCardPresent) {
                        sdStatusText.textContent = 'Present ✓';
                        sdStatusText.style.color = '#4CAF50';
                    } else {
                        sdStatusText.textContent = 'Not Present ✗';
                        sdStatusText.style.color = '#f44336';
                    }
                }
            } catch (e) {
                document.getElementById('connection-status').textContent = 'Error';
            }
        }
        function updatePatternActiveState() {
            document.querySelectorAll('.pattern-btn').forEach(function(btn) {
                const p = parseInt(btn.getAttribute('data-pattern'), 10);
                btn.classList.toggle('active', p === currentPattern);
            });
        }
        
        async function changeMode() {
            const mode = document.getElementById('mode-select').value;
            const index = document.getElementById('content-index').value;
            currentMode = parseInt(mode);
            
            await fetch('/api/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({mode: currentMode, index: parseInt(index)})
            });
            
            updateStatus();
        }
        
        async function changeContentIndex() {
            // When index changes, update the current mode with new index
            await changeMode();
        }
        
        function updateBrightness(value) {
            document.getElementById('brightness-value').textContent = value;
            
            fetch('/api/brightness', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({brightness: parseInt(value)})
            });
        }
        
        function updateFrameRate(value) {
            document.getElementById('framerate-value').textContent = value;
            
            fetch('/api/framerate', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({framerate: parseInt(value)})
            });
        }
        
        async function setPattern(type) {
            currentPattern = type;
            updatePatternActiveState();
            const color1 = document.getElementById('color1').value;
            const color2 = document.getElementById('color2').value;
            const speed = document.getElementById('pattern-speed').value;
            
            await fetch('/api/pattern', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    index: type,
                    type: type,
                    color1: hexToRgb(color1),
                    color2: hexToRgb(color2),
                    speed: parseInt(speed)
                })
            });
            
            document.getElementById('content-index').value = type;
            document.getElementById('mode-select').value = '2';
            await changeMode();
        }
        
        function updateSpeed(value) {
            document.getElementById('speed-value').textContent = value;
            // If currently in pattern mode, update the pattern with new speed
            if (currentMode === 2) {
                setPattern(currentPattern);
            }
        }
        
        function hexToRgb(hex) {
            const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
            return result ? {
                r: parseInt(result[1], 16),
                g: parseInt(result[2], 16),
                b: parseInt(result[3], 16)
            } : {r: 0, g: 0, b: 0};
        }
        
        async function uploadImage() {
            const fileInput = document.getElementById('image-upload');
            if (!fileInput.files.length) {
                alert('Please select an image first');
                return;
            }
            
            const file = fileInput.files[0];
            
            // Convert image to POV format using Canvas
            try {
                const povImageData = await convertImageToPOVFormat(file);
                
                // Create blob from raw RGB data and wrap in FormData
                // Encode dimensions in filename so the server can parse them
                const blob = new Blob([povImageData.data], { type: 'application/octet-stream' });
                const formData = new FormData();
                formData.append('file', blob, `image_${povImageData.width}x${povImageData.height}.rgb`);
                
                // Send as multipart form upload
                const response = await fetch('/api/image', {
                    method: 'POST',
                    body: formData
                });
                
                if (response.ok) {
                    alert(`Image uploaded successfully! (${povImageData.width}x${povImageData.height})`);
                    
                    // Switch to image mode
                    document.getElementById('mode-select').value = '1';
                    await changeMode();
                } else {
                    alert('Upload failed: ' + response.statusText);
                }
            } catch (e) {
                alert('Upload failed: ' + e.message);
            }
        }
        
        async function convertImageToPOVFormat(file) {
            return new Promise((resolve, reject) => {
                const img = new Image();
                const reader = new FileReader();
                
                reader.onload = (e) => {
                    img.onload = () => {
                        try {
                            // Create canvas for conversion
                            const canvas = document.createElement('canvas');
                            const ctx = canvas.getContext('2d');
                            
                            // Get user-specified dimensions
                            // Height is the physical LED count; width is derived
                            // from it (or directly used if aspect lock is off).
                            let targetHeight = parseInt(document.getElementById('image-height').value) || 32;
                            targetHeight = Math.min(200, Math.max(1, targetHeight));

                            let targetWidth = parseInt(document.getElementById('image-width').value) || 32;
                            const aspectLock = document.getElementById('aspect-ratio-lock').checked;
                            if (aspectLock) {
                                // Derive width from LED count and original aspect ratio.
                                targetWidth = Math.round(targetHeight / originalImageAspectRatio);
                            }
                            targetWidth = Math.min(100, Math.max(1, targetWidth));
                            
                            // Get flip options
                            const flipVertical = document.getElementById('flip-vertical').checked;
                            const flipHorizontal = document.getElementById('flip-horizontal').checked;
                            
                            // Validate dimensions
                            if (targetWidth < 1 || targetWidth > 100 || targetHeight < 1 || targetHeight > 200) {
                                reject(new Error('Invalid dimensions. Width: 1-100, Height: 1-200'));
                                return;
                            }
                            
                            canvas.width = targetWidth;
                            canvas.height = targetHeight;
                            
                            // Apply transformations
                            ctx.save();
                            
                            // Handle horizontal flip
                            if (flipHorizontal) {
                                ctx.translate(targetWidth, 0);
                                ctx.scale(-1, 1);
                            }
                            
                            // Draw and resize image
                            ctx.imageSmoothingEnabled = false; // Nearest neighbor
                            ctx.drawImage(img, 0, 0, targetWidth, targetHeight);
                            ctx.restore();
                            
                            // Get image data
                            let imageData = ctx.getImageData(0, 0, targetWidth, targetHeight);
                            
                            // Apply vertical flip if requested (in addition to the POV flip)
                            // Note: POV format always needs vertical flip for correct display
                            // If user wants "flip vertical", we skip the automatic flip
                            const applyPOVFlip = !flipVertical;
                            
                            if (applyPOVFlip) {
                                // Flip vertically so bottom of image is at LED 1 (closest to board)
                                // and top of image is at LED 31 (farthest from board)
                                const flippedData = ctx.createImageData(targetWidth, targetHeight);
                                
                                // Flip the image data vertically
                                for (let y = 0; y < targetHeight; y++) {
                                    for (let x = 0; x < targetWidth; x++) {
                                        const srcIndex = (y * targetWidth + x) * 4;
                                        const dstIndex = ((targetHeight - 1 - y) * targetWidth + x) * 4;
                                        flippedData.data[dstIndex] = imageData.data[srcIndex];
                                        flippedData.data[dstIndex + 1] = imageData.data[srcIndex + 1];
                                        flippedData.data[dstIndex + 2] = imageData.data[srcIndex + 2];
                                        flippedData.data[dstIndex + 3] = imageData.data[srcIndex + 3];
                                    }
                                }
                                imageData = flippedData;
                            }
                            
                            const pixels = imageData.data;
                            
                            // Convert to RGB array (remove alpha channel)
                            const rgbData = new Uint8Array(targetWidth * targetHeight * 3);
                            let rgbIndex = 0;
                            
                            for (let i = 0; i < pixels.length; i += 4) {
                                rgbData[rgbIndex++] = pixels[i];     // R
                                rgbData[rgbIndex++] = pixels[i + 1]; // G
                                rgbData[rgbIndex++] = pixels[i + 2]; // B
                            }
                            
                            resolve({
                                width: targetWidth,
                                height: targetHeight,
                                data: rgbData
                            });
                        } catch (err) {
                            reject(err);
                        }
                    };
                    img.onerror = reject;
                    img.src = e.target.result;
                };
                
                reader.onerror = reject;
                reader.readAsDataURL(file);
            });
        }
        
        // SD Card Management
        async function refreshSDList() {
            try {
                const response = await fetch('/api/sd/list');
                const data = await response.json();
                const listDiv = document.getElementById('sd-file-list');
                
                if (data.files && data.files.length > 0) {
                    listDiv.innerHTML = '';
                    data.files.forEach(filename => {
                        const fileItem = document.createElement('div');
                        fileItem.style.cssText = 'display: flex; justify-content: space-between; align-items: center; padding: 8px; margin: 5px 0; background: #f0f0f0; border-radius: 5px;';
                        fileItem.innerHTML = `
                            <span style="flex: 1; font-size: 14px;">${filename}</span>
                            <div style="display: flex; gap: 5px;">
                                <button onclick="loadSDImage('${filename}')" style="padding: 5px 10px; font-size: 12px; background: #4CAF50; color: white; border: none; border-radius: 3px; cursor: pointer;">Load</button>
                                <button onclick="deleteSDImage('${filename}')" style="padding: 5px 10px; font-size: 12px; background: #f44336; color: white; border: none; border-radius: 3px; cursor: pointer;">Delete</button>
                            </div>
                        `;
                        listDiv.appendChild(fileItem);
                    });
                } else {
                    listDiv.innerHTML = '<div style="text-align: center; color: #999;">No images found on SD card</div>';
                }
            } catch (e) {
                document.getElementById('sd-file-list').innerHTML = '<div style="text-align: center; color: #f44336;">Error loading file list</div>';
            }
        }
        
        async function loadSDImage(filename) {
            try {
                const response = await fetch('/api/sd/load', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({filename: filename})
                });
                if (response.ok) {
                    alert('Image loaded from SD: ' + filename);
                    // Switch to image mode
                    document.getElementById('mode-select').value = '1';
                    await changeMode();
                    updateStatus();
                } else {
                    alert('Failed to load image');
                }
            } catch (e) {
                alert('Error loading image: ' + e.message);
            }
        }
        
        async function deleteSDImage(filename) {
            if (!confirm('Delete ' + filename + ' from SD card?')) {
                return;
            }
            try {
                const response = await fetch('/api/sd/delete', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({filename: filename})
                });
                if (response.ok) {
                    alert('Image deleted from SD');
                    refreshSDList();
                } else {
                    alert('Failed to delete image');
                }
            } catch (e) {
                alert('Error deleting image: ' + e.message);
            }
        }
        
        async function updateSDStatus() {
            try {
                const response = await fetch('/api/sd/info');
                const data = await response.json();
                const statusText = document.getElementById('sd-status-text');
                const infoDiv = document.getElementById('sd-info');
                
                if (data.present) {
                    statusText.textContent = 'Present ✓';
                    statusText.style.color = '#4CAF50';
                    const totalMB = (data.totalSpace / (1024 * 1024)).toFixed(1);
                    const freeMB = (data.freeSpace / (1024 * 1024)).toFixed(1);
                    infoDiv.innerHTML = `Total: ${totalMB} MB | Free: ${freeMB} MB`;
                } else {
                    statusText.textContent = 'Not Present ✗';
                    statusText.style.color = '#f44336';
                    infoDiv.innerHTML = '';
                }
            } catch (e) {
                document.getElementById('sd-status-text').textContent = 'Error';
                document.getElementById('sd-status-text').style.color = '#f44336';
            }
        }
        
        // Update SD status on page load and periodically
        updateSDStatus();
        refreshSDList();
        setInterval(updateSDStatus, 5000);
        setInterval(refreshSDList, 10000);
        
        // ========== Multi-Poi Sync ==========
        let currentSyncMode = 'mirror';
        let syncPeers = [];

        async function updateSyncStatus() {
            try {
                const response = await fetch('/api/multipoi/status');
                const data = await response.json();

                document.getElementById('sync-local-name').textContent = data.localName || '--';
                currentSyncMode = data.syncMode || 'mirror';
                document.getElementById('sync-mode-display').textContent =
                    currentSyncMode === 'mirror' ? 'Mirror' : 'Independent';

                // Update mode buttons
                const btnMirror = document.getElementById('btn-mirror');
                const btnIndep = document.getElementById('btn-independent');
                if (currentSyncMode === 'mirror') {
                    btnMirror.style.background = '#667eea';
                    btnMirror.style.color = 'white';
                    btnIndep.style.background = '#ddd';
                    btnIndep.style.color = '#333';
                } else {
                    btnIndep.style.background = '#667eea';
                    btnIndep.style.color = 'white';
                    btnMirror.style.background = '#ddd';
                    btnMirror.style.color = '#333';
                }

                // Show/hide independent controls
                document.getElementById('independent-controls').style.display =
                    currentSyncMode === 'independent' ? 'block' : 'none';

                syncPeers = data.peers || [];
                const pairedCount = syncPeers.filter(p => p.state === 3 && p.online).length;
                document.getElementById('sync-paired-status').textContent =
                    pairedCount > 0 ? pairedCount + ' peer(s) online' : 'No peers';

                // Render peer list
                const peerList = document.getElementById('sync-peer-list');
                if (syncPeers.length === 0) {
                    peerList.innerHTML = '<div style="text-align:center;color:#999;font-size:13px;">No paired poi found. Tap Pair to discover.</div>';
                } else {
                    let html = '';
                    syncPeers.forEach((peer, i) => {
                        const stateNames = ['None','Discovering','Requesting','Paired'];
                        const statusColor = peer.online ? '#4CAF50' : '#f44336';
                        const statusText = peer.online ? 'Online' : 'Offline';
                        html += '<div style="display:flex;justify-content:space-between;align-items:center;padding:8px;margin:4px 0;background:white;border-radius:5px;border-left:4px solid ' + statusColor + ';">';
                        html += '<div><strong>' + (peer.name || 'Unknown') + '</strong>';
                        html += '<br><span style="font-size:12px;color:#666;">' + statusText + ' | ' + stateNames[peer.state] + '</span></div>';
                        html += '<button onclick="unpairSingle(' + i + ')" style="padding:5px 10px;font-size:12px;background:#f44336;color:white;border:none;border-radius:3px;cursor:pointer;min-height:30px;width:auto;">Unpair</button>';
                        html += '</div>';
                    });
                    peerList.innerHTML = html;
                }

                // Render per-peer control panels (independent mode)
                if (currentSyncMode === 'independent') {
                    renderPeerControls();
                }
            } catch (e) {
                console.error('Sync status error:', e);
            }
        }

        function renderPeerControls() {
            const container = document.getElementById('peer-control-panels');
            const onlinePeers = syncPeers.filter(p => p.state === 3 && p.online);
            if (onlinePeers.length === 0) {
                container.innerHTML = '<div style="text-align:center;color:#999;">No online peers to control.</div>';
                return;
            }
            let html = '';
            syncPeers.forEach((peer, i) => {
                if (peer.state !== 3 || !peer.online) return;
                html += '<div style="margin-bottom:15px;padding:12px;background:white;border-radius:8px;border:2px solid #667eea;">';
                html += '<div style="font-weight:bold;color:#667eea;margin-bottom:8px;">' + (peer.name || 'Peer ' + i) + '</div>';

                // Mode selector for this peer
                html += '<div style="margin-bottom:8px;">';
                html += '<label style="font-size:13px;">Mode:</label>';
                html += '<select id="peer-mode-' + i + '" style="width:100%;padding:8px;margin-top:4px;" onchange="sendPeerMode(' + i + ')">';
                html += '<option value="0">Idle</option><option value="1">Image</option><option value="2">Pattern</option><option value="3">Sequence</option>';
                html += '</select></div>';

                // Pattern buttons for this peer
                html += '<div style="margin-bottom:8px;">';
                html += '<label style="font-size:13px;">Quick Pattern:</label>';
                html += '<div style="display:grid;grid-template-columns:repeat(3,1fr);gap:4px;margin-top:4px;">';
                const patterns = [
                    {t:0,n:'Rainbow'},{t:4,n:'Fire'},{t:5,n:'Comet'},
                    {t:6,n:'Breathe'},{t:10,n:'Plasma'},{t:3,n:'Sparkle'},
                    {t:16,n:'Split'},{t:17,n:'Chase'},{t:7,n:'Strobe'}
                ];
                patterns.forEach(p => {
                    html += '<button onclick="sendPeerPattern(' + i + ',' + p.t + ')" style="padding:6px;font-size:11px;background:#764ba2;color:white;border:none;border-radius:4px;cursor:pointer;min-height:32px;width:auto;">' + p.n + '</button>';
                });
                html += '</div></div>';

                // Brightness for this peer
                html += '<div>';
                html += '<label style="font-size:13px;">Brightness: <span id="peer-bright-val-' + i + '">' + (peer.brightness || 128) + '</span></label>';
                html += '<input type="range" min="0" max="255" value="' + (peer.brightness || 128) + '" oninput="sendPeerBrightness(' + i + ',this.value);document.getElementById(\'peer-bright-val-' + i + '\').textContent=this.value" style="width:100%;">';
                html += '</div>';

                html += '</div>';
            });
            container.innerHTML = html;
        }

        async function setSyncMode(mode) {
            try {
                await fetch('/api/multipoi/syncmode', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({mode: mode})
                });
                currentSyncMode = mode;
                updateSyncStatus();
            } catch (e) {
                console.error('Set sync mode error:', e);
            }
        }

        async function pairPoi() {
            try {
                await fetch('/api/multipoi/pair', { method: 'POST' });
                // Poll for results after a brief delay
                setTimeout(updateSyncStatus, 2000);
            } catch (e) {
                console.error('Pair error:', e);
            }
        }

        async function unpairPoi() {
            try {
                await fetch('/api/multipoi/unpair', { method: 'POST' });
                updateSyncStatus();
            } catch (e) {
                console.error('Unpair error:', e);
            }
        }

        async function unpairSingle(index) {
            try {
                await fetch('/api/multipoi/unpair', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({index: index})
                });
                updateSyncStatus();
            } catch (e) {
                console.error('Unpair single error:', e);
            }
        }

        async function sendPeerMode(peerIndex) {
            const mode = parseInt(document.getElementById('peer-mode-' + peerIndex).value);
            try {
                await fetch('/api/multipoi/peercmd', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({peer: peerIndex, cmd: 'mode', mode: mode, index: 0})
                });
            } catch (e) { console.error(e); }
        }

        async function sendPeerPattern(peerIndex, type) {
            const color1 = hexToRgb(document.getElementById('color1').value);
            const color2 = hexToRgb(document.getElementById('color2').value);
            const speed = parseInt(document.getElementById('pattern-speed').value);
            try {
                await fetch('/api/multipoi/peercmd', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        peer: peerIndex, cmd: 'pattern', index: 0, type: type,
                        color1: color1, color2: color2, speed: speed
                    })
                });
            } catch (e) { console.error(e); }
        }

        async function sendPeerBrightness(peerIndex, brightness) {
            try {
                await fetch('/api/multipoi/peercmd', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({peer: peerIndex, cmd: 'brightness', brightness: parseInt(brightness)})
                });
            } catch (e) { console.error(e); }
        }

        // Initialize sync status polling
        updateSyncStatus();
        setInterval(updateSyncStatus, 3000);

        // PWA Install Prompt
        let deferredPrompt;
        
        window.addEventListener('beforeinstallprompt', (e) => {
            e.preventDefault();
            deferredPrompt = e;
            
            const installButton = document.getElementById('installButton');
            installButton.style.display = 'block';
            
            installButton.addEventListener('click', async () => {
                if (deferredPrompt) {
                    deferredPrompt.prompt();
                    const { outcome } = await deferredPrompt.userChoice;
                    console.log(`User response: ${outcome}`);
                    deferredPrompt = null;
                    installButton.style.display = 'none';
                }
            });
        });
        
        // Service Worker Registration
        if ('serviceWorker' in navigator) {
            navigator.serviceWorker.register('/sw.js')
                .then(reg => console.log('Service Worker registered'))
                .catch(err => console.log('Service Worker registration failed'));
        }
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  // Use ArduinoJson for efficient JSON serialization
  JsonDocument doc;
  doc["connected"] = state.connected;
  doc["mode"] = state.currentMode;
  doc["index"] = state.currentIndex;
  doc["brightness"] = state.brightness;
  doc["framerate"] = state.frameRate;
  doc["sdCardPresent"] = state.sdCardPresent;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetMode() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Simple JSON parsing
    int modeIdx = body.indexOf("\"mode\":");
    int indexIdx = body.indexOf("\"index\":");
    
    if (modeIdx != -1) {
      state.currentMode = body.substring(modeIdx + 7, body.indexOf(",", modeIdx)).toInt();
    }
    if (indexIdx != -1) {
      state.currentIndex = body.substring(indexIdx + 8, body.indexOf("}", indexIdx)).toInt();
    }
    
    // Send command to Teensy
    sendTeensyCommand(0x01, 2);
    TEENSY_SERIAL.write(state.currentMode);
    TEENSY_SERIAL.write(state.currentIndex);
    TEENSY_SERIAL.write(0xFE);

    // Broadcast to paired peers via ESP-NOW (mirror mode)
    if (!_syncCommandInProgress) {
      espNowSync.broadcastModeChange(state.currentMode, state.currentIndex);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleSetBrightness() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int idx = body.indexOf("\"brightness\":");
    
    if (idx != -1) {
      state.brightness = body.substring(idx + 13, body.indexOf("}", idx)).toInt();
      
      // Send command to Teensy
      sendTeensyCommand(0x06, 1);
      TEENSY_SERIAL.write(state.brightness);
      TEENSY_SERIAL.write(0xFE);

      // Broadcast to paired peers via ESP-NOW (mirror mode)
      if (!_syncCommandInProgress) {
        espNowSync.broadcastBrightness(state.brightness);
      }

      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
}

void handleSetFrameRate() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int idx = body.indexOf("\"framerate\":");
    
    if (idx != -1) {
      state.frameRate = body.substring(idx + 12, body.indexOf("}", idx)).toInt();
      state.cachedFrameDelay = 1000 / max((uint8_t)1, state.frameRate);  // Update cache

      // Send command to Teensy
      sendTeensyCommand(0x07, 1);
      TEENSY_SERIAL.write(state.cachedFrameDelay);
      TEENSY_SERIAL.write(0xFE);

      // Broadcast to paired peers via ESP-NOW (mirror mode)
      if (!_syncCommandInProgress) {
        espNowSync.broadcastFrameRate(state.cachedFrameDelay);
      }

      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
}

void handleUploadPattern() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse JSON manually (simple parsing, no external libs)
    // Expected format:
    // {
    //   "index": N,
    //   "type": N,
    //   "color1": {"r":R,"g":G,"b":B},
    //   "color2": {"r":R,"g":G,"b":B},
    //   "speed": N
    // }
    uint8_t index = 0;
    uint8_t type = 0;
    uint8_t r1 = 255, g1 = 0, b1 = 0;
    uint8_t r2 = 0, g2 = 0, b2 = 255;
    uint8_t speed = 50;

    auto parseUintField = [&](const char* key, uint8_t& outValue, uint8_t defaultValue) {
      int k = body.indexOf(key);
      if (k == -1) {
        outValue = defaultValue;
        return;
      }
      int start = body.indexOf(":", k);
      if (start == -1) {
        outValue = defaultValue;
        return;
      }
      // Read until comma or closing brace
      int end = body.indexOf(",", start);
      int endBrace = body.indexOf("}", start);
      if (end == -1 || (endBrace != -1 && endBrace < end)) {
        end = endBrace;
      }
      if (end == -1) {
        end = body.length();
      }
      String num = body.substring(start + 1, end);
      num.trim();
      long v = num.toInt();
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      outValue = (uint8_t)v;
    };

    // Top-level scalar fields
    parseUintField("\"index\"", index, 0);
    parseUintField("\"type\"", type, 0);
    parseUintField("\"speed\"", speed, 50);

    auto parseUintFieldIn = [&](const String& src, const char* key, uint8_t& outValue, uint8_t defaultValue) {
      int k = src.indexOf(key);
      if (k == -1) { outValue = defaultValue; return; }
      int start = src.indexOf(":", k);
      if (start == -1) { outValue = defaultValue; return; }
      int end = src.indexOf(",", start);
      int endBrace = src.indexOf("}", start);
      if (end == -1 || (endBrace != -1 && endBrace < end)) end = endBrace;
      if (end == -1) end = src.length();
      String num = src.substring(start + 1, end);
      num.trim();
      long v = num.toInt();
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      outValue = (uint8_t)v;
    };

    int c1 = body.indexOf("\"color1\"");
    if (c1 != -1) {
      String c1Sub = body.substring(c1);
      parseUintFieldIn(c1Sub, "\"r\"", r1, r1);
      parseUintFieldIn(c1Sub, "\"g\"", g1, g1);
      parseUintFieldIn(c1Sub, "\"b\"", b1, b1);
    }

    int c2 = body.indexOf("\"color2\"");
    if (c2 != -1) {
      String c2Sub = body.substring(c2);
      parseUintFieldIn(c2Sub, "\"r\"", r2, r2);
      parseUintFieldIn(c2Sub, "\"g\"", g2, g2);
      parseUintFieldIn(c2Sub, "\"b\"", b2, b2);
    }
    
    // Clamp the pattern index to the supported upper bound (0-17)
    if (index > kMaxPatternIndex) {
      index = kMaxPatternIndex;
    }
    
    // Send pattern to Teensy (simple protocol)
    // Data format expected by Teensy:
    // [index][type][r1][g1][b1][r2][g2][b2][speed]  (9 bytes)
    sendTeensyCommand(0x03, 9);
    TEENSY_SERIAL.write(index);
    TEENSY_SERIAL.write(type);
    TEENSY_SERIAL.write(r1);
    TEENSY_SERIAL.write(g1);
    TEENSY_SERIAL.write(b1);
    TEENSY_SERIAL.write(r2);
    TEENSY_SERIAL.write(g2);
    TEENSY_SERIAL.write(b2);
    TEENSY_SERIAL.write(speed);
    TEENSY_SERIAL.write(0xFE);

    // Broadcast to paired peers via ESP-NOW (mirror mode)
    if (!_syncCommandInProgress) {
      espNowSync.broadcastPattern(index, type, r1, g1, b1, r2, g2, b2, speed);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleUploadImage() {
  // Handle image upload from web interface
  // Images are pre-converted to RGB data by the web interface
  // Format: raw RGB bytes (width * height * 3)
  // Filename encodes dimensions: image_WxH.rgb
  
  HTTPUpload& upload = server.upload();
  static uint8_t imageBuffer[MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3];  // Max image buffer
  static uint16_t bufferIndex = 0;
  static uint16_t imageWidth = 32;
  static uint16_t imageHeight = 32;
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    bufferIndex = 0;
    
    // Parse dimensions from filename (format: image_WxH.rgb)
    String fname = upload.filename;
    int underscoreIdx = fname.indexOf('_');
    int xIdx = fname.indexOf('x', underscoreIdx);
    int dotIdx = fname.indexOf('.', xIdx);
    if (underscoreIdx != -1 && xIdx != -1) {
      imageWidth = fname.substring(underscoreIdx + 1, xIdx).toInt();
      if (dotIdx != -1) {
        imageHeight = fname.substring(xIdx + 1, dotIdx).toInt();
      } else {
        imageHeight = fname.substring(xIdx + 1).toInt();
      }
      // Clamp to valid range
      if (imageWidth < 1) imageWidth = 1;
      if (imageWidth > MAX_IMAGE_WIDTH) imageWidth = MAX_IMAGE_WIDTH;
      if (imageHeight < 1) imageHeight = 1;
      if (imageHeight > MAX_IMAGE_HEIGHT) imageHeight = MAX_IMAGE_HEIGHT;
    }
    Serial.printf("Parsed dimensions: %dx%d\n", imageWidth, imageHeight);
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // Accumulate image data
    size_t bytesToCopy = upload.currentSize;
    if (bufferIndex + bytesToCopy > sizeof(imageBuffer)) {
      bytesToCopy = sizeof(imageBuffer) - bufferIndex;
    }
    
    memcpy(imageBuffer + bufferIndex, upload.buf, bytesToCopy);
    bufferIndex += bytesToCopy;
    
    Serial.printf("Upload Write: %d bytes (total: %d)\n", bytesToCopy, bufferIndex);
    
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Upload End: %d bytes\n", bufferIndex);
    
    // Use dimensions parsed from filename
    // Verify against actual data size
    uint16_t expectedSize = imageWidth * imageHeight * 3;
    if (bufferIndex < expectedSize) {
      // Adjust height to match actual data if needed
      imageHeight = bufferIndex / (imageWidth * 3);
      if (imageHeight < 1) imageHeight = 1;
    }
    
    uint16_t actualSize = imageWidth * imageHeight * 3;
    if (actualSize > bufferIndex) actualSize = bufferIndex;
    
    Serial.printf("Detected image: %dx%d (%d bytes)\n", imageWidth, imageHeight, actualSize);
    
    // Send image data to Teensy for processing
    // Protocol: 0xFF 0x02 dataLen_high dataLen_low width_low width_high height_low height_high [RGB data...] 0xFE
    // Updated to support 16-bit dimensions for PSRAM support
    TEENSY_SERIAL.write(0xFF);  // Start marker
    TEENSY_SERIAL.write(0x02);  // Upload Image command
    TEENSY_SERIAL.write((actualSize >> 8) & 0xFF);  // Data length high byte
    TEENSY_SERIAL.write(actualSize & 0xFF);  // Data length low byte
    TEENSY_SERIAL.write(imageWidth & 0xFF);  // Image width low byte
    TEENSY_SERIAL.write((imageWidth >> 8) & 0xFF);  // Image width high byte
    TEENSY_SERIAL.write(imageHeight & 0xFF);  // Image height low byte
    TEENSY_SERIAL.write((imageHeight >> 8) & 0xFF);  // Image height high byte
    
    // Send pixel data
    for (uint16_t i = 0; i < actualSize && i < bufferIndex; i++) {
      TEENSY_SERIAL.write(imageBuffer[i]);
    }
    TEENSY_SERIAL.write(0xFE);  // End marker
    
    Serial.println("Image forwarded to Teensy");
    
    // Set mode to image display (remove unnecessary delay)
    state.currentMode = 1;
    state.currentIndex = 0;
    sendTeensyCommand(0x01, 2);
    TEENSY_SERIAL.write(state.currentMode);
    TEENSY_SERIAL.write(state.currentIndex);
    TEENSY_SERIAL.write(0xFE);
    
    // Auto-save to SD card if present
    // Check SD status first (from last status check)
    if (state.sdCardPresent) {
      // Reduced delay for serial processing
      delay(10);
      
      // Generate filename based on timestamp
      // Format: "upload_XXXXX.pov" where XXXXX is milliseconds modulo 100000
      uint32_t timestamp = millis() % 100000;
      char filename[32];
      snprintf(filename, sizeof(filename), "upload_%05lu.pov", timestamp);
      uint8_t filenameLen = strlen(filename);
      
      // Calculate total data length: filename_len + filename + width + height + image_data
      uint16_t totalDataLen = 1 + filenameLen + 2 + actualSize;
      
      // Send SD save command (simple protocol)
      // Format: 0xFF 0x20 dataLen [filename_len][filename][width][height][RGB_data...] 0xFE
      sendTeensyCommand(0x20, totalDataLen);
      TEENSY_SERIAL.write(filenameLen);
      TEENSY_SERIAL.write((const uint8_t*)filename, filenameLen);
      TEENSY_SERIAL.write(imageWidth);
      TEENSY_SERIAL.write(imageHeight);
      
      // Send pixel data
      for (uint16_t i = 0; i < actualSize && i < bufferIndex; i++) {
        TEENSY_SERIAL.write(imageBuffer[i]);
      }
      TEENSY_SERIAL.write(0xFE);
      
      Serial.print("Auto-saving image to SD: ");
      Serial.println(filename);
    } else {
      Serial.println("SD card not present - skipping auto-save");
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Serial.println("Upload aborted");
    server.send(500, "application/json", "{\"error\":\"Upload aborted\"}");
  }
}

void handleLiveFrame() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse JSON payload: {"pixels":[{"r":R,"g":G,"b":B}, ...]}
    const int ledCount = 32;  // Display LEDs (all 32 LEDs used for display)
    uint8_t rgb[ledCount * 3];
    for (int i = 0; i < ledCount * 3; i++) {
      rgb[i] = 0;
    }

    int pixelsIdx = body.indexOf("\"pixels\"");
    if (pixelsIdx != -1) {
      int pos = body.indexOf("[", pixelsIdx);
      int end = body.indexOf("]", pos);
      if (pos != -1 && end != -1) {
        String arr = body.substring(pos, end);
        int led = 0;
        int cursor = 0;
        while (led < ledCount) {
          int objStart = arr.indexOf("{", cursor);
          if (objStart == -1) break;
          int objEnd = arr.indexOf("}", objStart);
          if (objEnd == -1) break;
          String obj = arr.substring(objStart, objEnd + 1);

          auto parseComp = [&](const char* key, uint8_t& outValue) {
            int k = obj.indexOf(key);
            if (k == -1) return;
            int s = obj.indexOf(":", k);
            if (s == -1) return;
            int e = obj.indexOf(",", s);
            int eBrace = obj.indexOf("}", s);
            if (e == -1 || (eBrace != -1 && eBrace < e)) {
              e = eBrace;
            }
            if (e == -1) {
              e = obj.length();
            }
            String num = obj.substring(s + 1, e);
            num.trim();
            long v = num.toInt();
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            outValue = (uint8_t)v;
          };

          uint8_t r = 0, g = 0, b = 0;
          parseComp("\"r\"", r);
          parseComp("\"g\"", g);
          parseComp("\"b\"", b);

          rgb[led * 3 + 0] = r;
          rgb[led * 3 + 1] = g;
          rgb[led * 3 + 2] = b;

          led++;
          cursor = objEnd + 1;
        }
      }
    }

    // Send live frame command to Teensy
    sendTeensyCommand(0x05, ledCount * 3);  // 32 LEDs * 3 bytes
    for (int i = 0; i < ledCount * 3; i++) {
      TEENSY_SERIAL.write(rgb[i]);
    }
    TEENSY_SERIAL.write(0xFE);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

bool readTeensyResponse(uint8_t expectedMarker, uint8_t* buffer, size_t maxLen, size_t& bytesRead, unsigned long timeout) {
  bytesRead = 0;
  unsigned long start = millis();
  
  // Wait for start marker
  while (millis() - start < timeout) {
    if (TEENSY_SERIAL.available() > 0) {
      if (TEENSY_SERIAL.read() == 0xFF) {
        if (TEENSY_SERIAL.available() > 0) {
          uint8_t marker = TEENSY_SERIAL.read();
          if (marker == expectedMarker) {
            // Read data until 0xFE
            while (millis() - start < timeout && bytesRead < maxLen - 1) {
              if (TEENSY_SERIAL.available() > 0) {
                uint8_t byte = TEENSY_SERIAL.read();
                if (byte == 0xFE) {
                  return true;
                }
                buffer[bytesRead++] = byte;
              }
            }
            return bytesRead > 0;
          }
        }
      }
    }
  }
  return false;
}

void handleSDList() {
  if (!state.sdCardPresent) {
    server.send(200, "application/json", "{\"error\":\"SD card not present\",\"files\":[]}");
    return;
  }
  
  // Send list command
  sendTeensyCommand(0x21, 0);
  TEENSY_SERIAL.write(0xFE);
  
  // readTeensyResponse has its own timeout, no need for delay
  uint8_t buffer[2048];
  size_t bytesRead = 0;
  
  if (readTeensyResponse(0xCC, buffer, sizeof(buffer), bytesRead, 500)) {
    if (bytesRead > 0) {
      uint8_t count = buffer[0];
      
      // Use ArduinoJson for efficient array building
      JsonDocument doc;
      JsonArray files = doc["files"].to<JsonArray>();
      
      size_t pos = 1;
      for (uint8_t i = 0; i < count && pos < bytesRead; i++) {
        uint8_t nameLen = buffer[pos++];
        if (pos + nameLen <= bytesRead) {
          char filename[64];
          for (uint8_t j = 0; j < nameLen && j < 63; j++) {
            filename[j] = (char)buffer[pos++];
          }
          filename[nameLen < 63 ? nameLen : 63] = '\0';
          files.add(filename);
        }
      }
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
      return;
    }
  }
  
  server.send(200, "application/json", "{\"error\":\"Failed to read response\",\"files\":[]}");
}

void handleSDInfo() {
  // Send info command
  sendTeensyCommand(0x23, 0);
  TEENSY_SERIAL.write(0xFE);
  
  // readTeensyResponse has its own timeout
  uint8_t buffer[20];
  size_t bytesRead = 0;
  
  if (readTeensyResponse(0xDD, buffer, sizeof(buffer), bytesRead, 500)) {
    if (bytesRead >= 17) {
      bool present = buffer[0] != 0;
      uint64_t totalSpace = 0;
      uint64_t freeSpace = 0;
      
      for (int i = 0; i < 8; i++) {
        totalSpace = (totalSpace << 8) | buffer[1 + i];
        freeSpace = (freeSpace << 8) | buffer[9 + i];
      }
      
      // Use ArduinoJson for efficient serialization
      JsonDocument doc;
      doc["present"] = present;
      doc["totalSpace"] = totalSpace;
      doc["freeSpace"] = freeSpace;
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
      return;
    }
  }
  
  server.send(200, "application/json", "{\"error\":\"Failed to read response\"}");
}

void handleSDDelete() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse JSON: {"filename":"name.pov"}
    int filenameIdx = body.indexOf("\"filename\":");
    if (filenameIdx == -1) {
      server.send(400, "application/json", "{\"error\":\"Missing filename\"}");
      return;
    }
    
    int start = body.indexOf("\"", filenameIdx + 11);
    int end = body.indexOf("\"", start + 1);
    if (start == -1 || end == -1) {
      server.send(400, "application/json", "{\"error\":\"Invalid filename format\"}");
      return;
    }
    
    String filename = body.substring(start + 1, end);
    uint8_t filenameLen = filename.length();
    if (filenameLen > 63) filenameLen = 63;
    
    // Send delete command
    sendTeensyCommand(0x22, filenameLen);
    TEENSY_SERIAL.write((const uint8_t*)filename.c_str(), filenameLen);
    TEENSY_SERIAL.write(0xFE);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleSDLoad() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse JSON: {"filename":"name.pov"}
    int filenameIdx = body.indexOf("\"filename\":");
    if (filenameIdx == -1) {
      server.send(400, "application/json", "{\"error\":\"Missing filename\"}");
      return;
    }
    
    int start = body.indexOf("\"", filenameIdx + 11);
    int end = body.indexOf("\"", start + 1);
    if (start == -1 || end == -1) {
      server.send(400, "application/json", "{\"error\":\"Invalid filename format\"}");
      return;
    }
    
    String filename = body.substring(start + 1, end);
    uint8_t filenameLen = filename.length();
    if (filenameLen > 63) filenameLen = 63;
    
    // Send load command
    sendTeensyCommand(0x24, filenameLen);
    TEENSY_SERIAL.write((const uint8_t*)filename.c_str(), filenameLen);
    TEENSY_SERIAL.write(0xFE);
    
    // Switch to image mode after loading
    delay(100);
    state.currentMode = 1;
    state.currentIndex = 0;
    sendTeensyCommand(0x01, 2);
    TEENSY_SERIAL.write(state.currentMode);
    TEENSY_SERIAL.write(state.currentIndex);
    TEENSY_SERIAL.write(0xFE);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleManifest() {
  String manifest = R"rawliteral({
  "name": "Nebula Poi Control",
  "short_name": "Nebula Poi",
  "description": "Wireless Nebula Poi Control Interface",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#ffffff",
  "theme_color": "#667eea",
  "icons": [
    {
      "src": "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ccircle cx='50' cy='50' r='40' fill='%23667eea'/%3E%3Ctext x='50' y='65' font-size='50' text-anchor='middle' fill='white'%3E🎨%3C/text%3E%3C/svg%3E",
      "sizes": "192x192",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    },
    {
      "src": "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ccircle cx='50' cy='50' r='40' fill='%23667eea'/%3E%3Ctext x='50' y='65' font-size='50' text-anchor='middle' fill='white'%3E🎨%3C/text%3E%3C/svg%3E",
      "sizes": "512x512",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
})rawliteral";
  
  server.send(200, "application/json", manifest);
}

void handleServiceWorker() {
  String sw = R"rawliteral(
const CACHE_NAME = 'pov-poi-v1';
const urlsToCache = [
  '/',
  '/manifest.json'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => {
        return cache.addAll(urlsToCache);
      })
  );
});

self.addEventListener('fetch', (event) => {
  event.respondWith(
    caches.match(event.request)
      .then((response) => {
        if (response) {
          return response;
        }
        return fetch(event.request);
      })
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cacheName) => {
          if (cacheName !== CACHE_NAME) {
            return caches.delete(cacheName);
          }
        })
      );
    })
  );
});
)rawliteral";
  
  server.send(200, "application/javascript", sw);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void sendFile(const char* path, const char* contentType) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
  } else {
    server.send(404, "text/plain", "File Not Found");
  }
}

void sendTeensyCommand(uint8_t cmd, uint8_t dataLen) {
  TEENSY_SERIAL.write(0xFF);  // Start marker
  TEENSY_SERIAL.write(cmd);
  TEENSY_SERIAL.write(dataLen);
}

void checkTeensyConnection() {
  // Request status from Teensy
  // #region agent log
  Serial.println("[DBG][H2] checkTeensyConnection: sending 0x10 status request to Teensy");
  // #endregion
  sendTeensyCommand(0x10, 0);
  TEENSY_SERIAL.write(0xFE);
  
  // Wait for response (simplified)
  // Format: 0xFF 0xBB mode index sd_present 0xFE
  unsigned long start = millis();
  while (millis() - start < 100) {
    int av = TEENSY_SERIAL.available();
    if (av >= 5) {
      uint8_t b0 = TEENSY_SERIAL.read();
      uint8_t b1 = TEENSY_SERIAL.read();
      // #region agent log
      Serial.printf("[DBG][H2] checkTeensyConnection: got %d bytes, header=0x%02X 0x%02X (expect FF BB)\n", av, b0, b1);
      // #endregion
      if (b0 == 0xFF && b1 == 0xBB) {
        state.currentMode = TEENSY_SERIAL.read();
        state.currentIndex = TEENSY_SERIAL.read();
        state.sdCardPresent = (TEENSY_SERIAL.read() != 0);
        state.connected = true;
        // #region agent log
        Serial.println("[DBG][H2] checkTeensyConnection: SUCCESS connected=true");
        // #endregion
        return;
      }
    }
  }
  state.connected = false;
  state.sdCardPresent = false;
  // #region agent log
  static unsigned long _lastFailLog = 0;
  if (millis() - _lastFailLog > 5000) {
    Serial.println("[DBG][H2] checkTeensyConnection: FAILED no valid response (H1/H4: wiring or timeout)");
    _lastFailLog = millis();
  }
  // #endregion
}

// ============================================================================
// ESP-NOW MULTI-POI SYNC IMPLEMENTATION
// ============================================================================

void setupESPNowSync() {
  String name = deviceConfig.deviceName;
  if (name.length() == 0) name = "Nebula Poi";

  if (!espNowSync.begin(name.c_str())) {
    Serial.println("[SYNC] ESP-NOW setup failed!");
    return;
  }

  // Register callbacks - these fire when a paired peer sends us a command
  espNowSync.onModeChange([](uint8_t mode, uint8_t index) {
    applyModeToTeensy(mode, index);
  });

  espNowSync.onPattern([](uint8_t idx, uint8_t type,
                          uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          uint8_t speed) {
    applyPatternToTeensy(idx, type, r1, g1, b1, r2, g2, b2, speed);
  });

  espNowSync.onBrightness([](uint8_t brightness) {
    applyBrightnessToTeensy(brightness);
  });

  espNowSync.onFrameRate([](uint8_t frameDelay) {
    applyFrameRateToTeensy(frameDelay);
  });

  espNowSync.onSyncTime([](int32_t offsetMs) {
    applySyncTimeToTeensy(offsetMs);
  });

  espNowSync.onPeerUpdate([](const SyncPeer* peer) {
    Serial.printf("[SYNC] Peer update: '%s' online=%d state=%d\n",
      peer->name, peer->online, peer->state);
  });

  Serial.println("[SYNC] ESP-NOW multi-poi sync ready");
}

// Apply commands received from a paired peer to the local Teensy
// The _syncCommandInProgress flag prevents the web handler broadcasts
// from echoing the command back to the peer that sent it.

void applyModeToTeensy(uint8_t mode, uint8_t index) {
  _syncCommandInProgress = true;
  state.currentMode = mode;
  state.currentIndex = index;
  sendTeensyCommand(0x01, 2);
  TEENSY_SERIAL.write(mode);
  TEENSY_SERIAL.write(index);
  TEENSY_SERIAL.write(0xFE);
  Serial.printf("[SYNC] Applied mode=%d index=%d from peer\n", mode, index);
  _syncCommandInProgress = false;
}

void applyPatternToTeensy(uint8_t idx, uint8_t type,
                          uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          uint8_t speed) {
  _syncCommandInProgress = true;
  sendTeensyCommand(0x03, 9);
  TEENSY_SERIAL.write(idx);
  TEENSY_SERIAL.write(type);
  TEENSY_SERIAL.write(r1);
  TEENSY_SERIAL.write(g1);
  TEENSY_SERIAL.write(b1);
  TEENSY_SERIAL.write(r2);
  TEENSY_SERIAL.write(g2);
  TEENSY_SERIAL.write(b2);
  TEENSY_SERIAL.write(speed);
  TEENSY_SERIAL.write(0xFE);

  // Also set mode to pattern display
  state.currentMode = 2;
  state.currentIndex = idx;
  sendTeensyCommand(0x01, 2);
  TEENSY_SERIAL.write(state.currentMode);
  TEENSY_SERIAL.write(state.currentIndex);
  TEENSY_SERIAL.write(0xFE);

  Serial.printf("[SYNC] Applied pattern type=%d from peer\n", type);
  _syncCommandInProgress = false;
}

void applyBrightnessToTeensy(uint8_t brightness) {
  _syncCommandInProgress = true;
  state.brightness = brightness;
  sendTeensyCommand(0x06, 1);
  TEENSY_SERIAL.write(brightness);
  TEENSY_SERIAL.write(0xFE);
  Serial.printf("[SYNC] Applied brightness=%d from peer\n", brightness);
  _syncCommandInProgress = false;
}

void applyFrameRateToTeensy(uint8_t frameDelay) {
  _syncCommandInProgress = true;
  // Update both frameRate and cached delay
  if (frameDelay > 0) {
    state.frameRate = 1000 / frameDelay;
    state.cachedFrameDelay = frameDelay;
  }
  sendTeensyCommand(0x07, 1);
  TEENSY_SERIAL.write(frameDelay);
  TEENSY_SERIAL.write(0xFE);
  Serial.printf("[SYNC] Applied frameDelay=%d from peer\n", frameDelay);
  _syncCommandInProgress = false;
}

void applySyncTimeToTeensy(int32_t offsetMs) {
  // Send sync time offset to Teensy so pattern animations stay in phase
  // Protocol: 0xFF 0x08 4 [offset_bytes:4] 0xFE
  sendTeensyCommand(0x08, 4);
  TEENSY_SERIAL.write((uint8_t)(offsetMs >> 24));
  TEENSY_SERIAL.write((uint8_t)(offsetMs >> 16));
  TEENSY_SERIAL.write((uint8_t)(offsetMs >> 8));
  TEENSY_SERIAL.write((uint8_t)(offsetMs & 0xFF));
  TEENSY_SERIAL.write(0xFE);
}

// REST API endpoints for multi-poi control

void handleMultiPoiStatus() {
  // Update local state in sync object (use cached frameDelay)
  espNowSync.setLocalState(state.currentMode, state.currentIndex,
                           state.brightness, state.cachedFrameDelay);

  // Use ArduinoJson for efficient JSON serialization
  JsonDocument doc;
  doc["syncMode"] = (espNowSync.getSyncMode() == SYNC_MIRROR ? "mirror" : "independent");
  doc["localName"] = espNowSync.getLocalName();

  // Local MAC
  const uint8_t* mac = espNowSync.getLocalMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  doc["localMac"] = macStr;
  doc["hasPairedPeer"] = espNowSync.hasPairedPeer();
  doc["autoPair"] = espNowSync.getAutoPair();

  JsonArray peers = doc["peers"].to<JsonArray>();
  for (int i = 0; i < espNowSync.getPeerCount(); i++) {
    const SyncPeer* peer = espNowSync.getPeer(i);
    if (!peer) continue;
    
    JsonObject peerObj = peers.add<JsonObject>();
    char peerMac[18];
    snprintf(peerMac, sizeof(peerMac), "%02X:%02X:%02X:%02X:%02X:%02X",
      peer->mac[0], peer->mac[1], peer->mac[2],
      peer->mac[3], peer->mac[4], peer->mac[5]);
    
    peerObj["name"] = peer->name;
    peerObj["mac"] = peerMac;
    peerObj["state"] = peer->state;
    peerObj["online"] = peer->online;
    peerObj["mode"] = peer->currentMode;
    peerObj["index"] = peer->currentIndex;
    peerObj["brightness"] = peer->brightness;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleMultiPoiPair() {
  espNowSync.startPairing();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Pair request broadcast\"}");
}

void handleMultiPoiUnpair() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int indexIdx = body.indexOf("\"index\":");
    if (indexIdx != -1) {
      int peerIdx = body.substring(indexIdx + 8).toInt();
      espNowSync.unpairPeer(peerIdx);
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  // No index specified - unpair all
  espNowSync.unpairAll();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"All peers unpaired\"}");
}

void handleMultiPoiSyncMode() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }

  String body = server.arg("plain");
  int modeIdx = body.indexOf("\"mode\":");
  if (modeIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Missing mode\"}");
    return;
  }

  String modeStr = body.substring(modeIdx + 7);
  modeStr.trim();

  if (modeStr.startsWith("\"mirror\"") || modeStr.startsWith("0")) {
    espNowSync.setSyncMode(SYNC_MIRROR);
  } else if (modeStr.startsWith("\"independent\"") || modeStr.startsWith("1")) {
    espNowSync.setSyncMode(SYNC_INDEPENDENT);
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid mode (use mirror or independent)\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"status\":\"ok\",\"syncMode\":\"" +
    String(espNowSync.getSyncMode() == SYNC_MIRROR ? "mirror" : "independent") + "\"}");
}

void handleMultiPoiPeerCmd() {
  // Send a command to a specific peer (used in independent mode)
  // JSON: {"peer": 0, "cmd": "mode", "mode": 2, "index": 0}
  //       {"peer": 0, "cmd": "pattern", "type": 0, "color1":{"r":255,"g":0,"b":0}, "color2":{"r":0,"g":0,"b":255}, "speed": 50, "index": 0}
  //       {"peer": 0, "cmd": "brightness", "brightness": 200}
  //       {"peer": 0, "cmd": "framerate", "framerate": 60}
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }

  String body = server.arg("plain");

  // Parse peer index
  int peerFieldIdx = body.indexOf("\"peer\":");
  if (peerFieldIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Missing peer index\"}");
    return;
  }
  int peerIdx = body.substring(peerFieldIdx + 7).toInt();

  // Parse command type
  int cmdFieldIdx = body.indexOf("\"cmd\":");
  if (cmdFieldIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Missing cmd\"}");
    return;
  }
  int cmdStart = body.indexOf("\"", cmdFieldIdx + 6) + 1;
  int cmdEnd = body.indexOf("\"", cmdStart);
  String cmd = body.substring(cmdStart, cmdEnd);

  if (cmd == "mode") {
    int mIdx = body.indexOf("\"mode\":", cmdEnd);
    int iIdx = body.indexOf("\"index\":");
    uint8_t mode = (mIdx != -1) ? body.substring(mIdx + 7).toInt() : 0;
    uint8_t index = (iIdx != -1) ? body.substring(iIdx + 8).toInt() : 0;
    espNowSync.sendPeerModeChange(peerIdx, mode, index);
  }
  else if (cmd == "pattern") {
    uint8_t type = 0, r1 = 255, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 255, speed = 50, idx = 0;
    int tIdx = body.indexOf("\"type\":");
    if (tIdx != -1) type = body.substring(tIdx + 7).toInt();
    int sIdx = body.indexOf("\"speed\":");
    if (sIdx != -1) speed = body.substring(sIdx + 8).toInt();
    int iIdx = body.indexOf("\"index\":");
    if (iIdx != -1) idx = body.substring(iIdx + 8).toInt();

    int c1 = body.indexOf("\"color1\"");
    if (c1 != -1) {
      String c1s = body.substring(c1, body.indexOf("}", c1) + 1);
      int ri = c1s.indexOf("\"r\":");
      int gi = c1s.indexOf("\"g\":");
      int bi = c1s.indexOf("\"b\":");
      if (ri != -1) r1 = c1s.substring(ri + 4).toInt();
      if (gi != -1) g1 = c1s.substring(gi + 4).toInt();
      if (bi != -1) b1 = c1s.substring(bi + 4).toInt();
    }
    int c2 = body.indexOf("\"color2\"");
    if (c2 != -1) {
      String c2s = body.substring(c2, body.indexOf("}", c2) + 1);
      int ri = c2s.indexOf("\"r\":");
      int gi = c2s.indexOf("\"g\":");
      int bi = c2s.indexOf("\"b\":");
      if (ri != -1) r2 = c2s.substring(ri + 4).toInt();
      if (gi != -1) g2 = c2s.substring(gi + 4).toInt();
      if (bi != -1) b2 = c2s.substring(bi + 4).toInt();
    }
    espNowSync.sendPeerPattern(peerIdx, idx, type, r1, g1, b1, r2, g2, b2, speed);
  }
  else if (cmd == "brightness") {
    int bIdx = body.indexOf("\"brightness\":");
    uint8_t brightness = (bIdx != -1) ? body.substring(bIdx + 13).toInt() : 128;
    espNowSync.sendPeerBrightness(peerIdx, brightness);
  }
  else if (cmd == "framerate") {
    int fIdx = body.indexOf("\"framerate\":");
    uint8_t fps = (fIdx != -1) ? body.substring(fIdx + 12).toInt() : 50;
    uint8_t frameDelay = 1000 / max((uint8_t)1, fps);
    espNowSync.sendPeerFrameRate(peerIdx, frameDelay);
  }
  else {
    server.send(400, "application/json", "{\"error\":\"Unknown cmd\"}");
    return;
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ============================================================================
// LEGACY PEER-TO-PEER SYNC IMPLEMENTATION (HTTP/mDNS)
// ============================================================================

// Load device configuration from preferences
void loadDeviceConfig() {
  preferences.begin("povpoi", false);
  
  // Generate device ID from MAC address if not set
  String macAddr = WiFi.macAddress();
  deviceConfig.deviceId = preferences.getString("deviceId", macAddr);
  if (deviceConfig.deviceId.length() == 0) {
    deviceConfig.deviceId = macAddr;
    preferences.putString("deviceId", deviceConfig.deviceId);
  }
  
  // Load device name (default: "POV Poi" + last 4 chars of MAC)
  String defaultName = "POV Poi " + deviceConfig.deviceId.substring(deviceConfig.deviceId.length() - 5);
  deviceConfig.deviceName = preferences.getString("deviceName", defaultName);
  
  // Load sync group
  deviceConfig.syncGroup = preferences.getString("syncGroup", "");
  
  // Load auto-sync settings
  deviceConfig.autoSync = preferences.getBool("autoSync", AUTO_SYNC_ENABLED);
  deviceConfig.syncInterval = preferences.getULong("syncInterval", AUTO_SYNC_INTERVAL);
  
  preferences.end();
}

// Save device configuration
void saveDeviceConfig() {
  preferences.begin("povpoi", false);
  preferences.putString("deviceId", deviceConfig.deviceId);
  preferences.putString("deviceName", deviceConfig.deviceName);
  preferences.putString("syncGroup", deviceConfig.syncGroup);
  preferences.putBool("autoSync", deviceConfig.autoSync);
  preferences.putULong("syncInterval", deviceConfig.syncInterval);
  preferences.end();
}

// Get device ID
String getDeviceId() {
  return deviceConfig.deviceId;
}

// Discover peer devices using mDNS
void discoverPeers() {
  Serial.println("Discovering peers...");
  
  int n = MDNS.queryService("povpoi", "tcp");
  Serial.printf("Found %d POV Poi devices\n", n);
  
  for (int i = 0; i < n && peerCount < MAX_PEERS; i++) {
    String peerId = MDNS.txt(i, "deviceId");
    
    // Skip self
    if (peerId == deviceConfig.deviceId) {
      continue;
    }
    
    // Check if peer already exists
    bool exists = false;
    int peerIndex = -1;
    for (int j = 0; j < peerCount; j++) {
      if (peers[j].deviceId == peerId) {
        exists = true;
        peerIndex = j;
        break;
      }
    }
    
    if (exists) {
      // Update existing peer
      peers[peerIndex].deviceName = MDNS.txt(i, "deviceName");
      peers[peerIndex].ipAddress = MDNS.IP(i).toString();
      peers[peerIndex].lastSeen = millis();
      peers[peerIndex].online = true;
      Serial.printf("Updated peer: %s (%s)\n", peers[peerIndex].deviceName.c_str(), peers[peerIndex].ipAddress.c_str());
    } else {
      // Add new peer
      peers[peerCount].deviceId = peerId;
      peers[peerCount].deviceName = MDNS.txt(i, "deviceName");
      peers[peerCount].ipAddress = MDNS.IP(i).toString();
      peers[peerCount].lastSeen = millis();
      peers[peerCount].online = true;
      peerCount++;
      Serial.printf("Added peer: %s (%s)\n", peers[peerCount-1].deviceName.c_str(), peers[peerCount-1].ipAddress.c_str());
    }
  }
  
  // Mark peers as offline if not seen recently
  for (int i = 0; i < peerCount; i++) {
    if (millis() - peers[i].lastSeen > PEER_TIMEOUT) {
      peers[i].online = false;
    }
  }
}

// Perform sync with peer device
void performSync(String peerId) {
  // Find peer
  PeerDevice* peer = nullptr;
  for (int i = 0; i < peerCount; i++) {
    if (peers[i].deviceId == peerId) {
      peer = &peers[i];
      break;
    }
  }
  
  if (peer == nullptr || !peer->online) {
    Serial.println("Peer not found or offline");
    return;
  }
  
  Serial.printf("Syncing with %s (%s)...\n", peer->deviceName.c_str(), peer->ipAddress.c_str());
  
  // Create HTTP client
  HTTPClient http;
  WiFiClient client;
  
  // Get peer's data
  String url = "http://" + peer->ipAddress + "/api/sync/data";
  http.begin(client, url);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Received sync data from peer");
    
    // Push our data to peer
    http.end();
    url = "http://" + peer->ipAddress + "/api/sync/push";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    
    // Build sync data JSON (simplified - just settings for now)
    String syncData = "{";
    syncData += "\"deviceId\":\"" + deviceConfig.deviceId + "\",";
    syncData += "\"deviceName\":\"" + deviceConfig.deviceName + "\",";
    syncData += "\"settings\":{";
    syncData += "\"brightness\":" + String(state.brightness) + ",";
    syncData += "\"framerate\":" + String(state.frameRate) + ",";
    syncData += "\"mode\":" + String(state.currentMode) + ",";
    syncData += "\"index\":" + String(state.currentIndex) + ",";
    syncData += "\"timestamp\":" + String(millis());
    syncData += "}}";
    
    httpCode = http.POST(syncData);
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Successfully synced with peer");
    } else {
      Serial.printf("Sync push failed: %d\n", httpCode);
    }
  } else {
    Serial.printf("Sync failed: %d\n", httpCode);
  }
  
  http.end();
}

// Handle sync status request
void handleSyncStatus() {
  String json = "{";
  json += "\"deviceId\":\"" + deviceConfig.deviceId + "\",";
  json += "\"deviceName\":\"" + deviceConfig.deviceName + "\",";
  json += "\"syncGroup\":\"" + deviceConfig.syncGroup + "\",";
  json += "\"autoSync\":" + String(deviceConfig.autoSync ? "true" : "false") + ",";
  json += "\"syncInterval\":" + String(deviceConfig.syncInterval) + ",";
  json += "\"peers\":[";
  
  for (int i = 0; i < peerCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"deviceId\":\"" + peers[i].deviceId + "\",";
    json += "\"deviceName\":\"" + peers[i].deviceName + "\",";
    json += "\"ipAddress\":\"" + peers[i].ipAddress + "\",";
    json += "\"lastSeen\":" + String(peers[i].lastSeen) + ",";
    json += "\"online\":" + String(peers[i].online ? "true" : "false");
    json += "}";
  }
  
  json += "]}";
  server.send(200, "application/json", json);
}

// Handle peer discovery request
void handleSyncDiscover() {
  discoverPeers();
  
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"peersFound\":" + String(peerCount) + ",";
  json += "\"peers\":[";
  
  for (int i = 0; i < peerCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"deviceId\":\"" + peers[i].deviceId + "\",";
    json += "\"deviceName\":\"" + peers[i].deviceName + "\",";
    json += "\"ipAddress\":\"" + peers[i].ipAddress + "\"";
    json += "}";
  }
  
  json += "]}";
  server.send(200, "application/json", json);
}

// Handle sync execution request
void handleSyncExecute() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Parse peer ID (simplified - should use proper JSON parser)
  int peerIdIdx = body.indexOf("\"peerId\":");
  if (peerIdIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Missing peerId\"}");
    return;
  }
  
  // Extract peer ID (simplified parsing)
  int startIdx = body.indexOf("\"", peerIdIdx + 9) + 1;
  int endIdx = body.indexOf("\"", startIdx);
  String peerId = body.substring(startIdx, endIdx);
  
  // Perform sync
  performSync(peerId);
  state.lastSync = millis();
  
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"itemsSynced\":1,";
  json += "\"imagesAdded\":0,";
  json += "\"patternsAdded\":0,";
  json += "\"settingsUpdated\":true";
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handle sync data request (GET)
void handleSyncData() {
  String json = "{";
  json += "\"deviceId\":\"" + deviceConfig.deviceId + "\",";
  json += "\"deviceName\":\"" + deviceConfig.deviceName + "\",";
  // Image listing (built-in demo images)
  json += "\"images\":[";
  json += "{\"id\":0,\"name\":\"Smiley Face\"},";
  json += "{\"id\":1,\"name\":\"Rainbow Gradient\"},";
  json += "{\"id\":2,\"name\":\"Heart\"}";
  json += "],";
  // Pattern listing (built-in patterns)
  json += "\"patterns\":[";
  json += "{\"id\":0,\"name\":\"Rainbow\"},";
  json += "{\"id\":1,\"name\":\"Wave\"},";
  json += "{\"id\":2,\"name\":\"Gradient\"},";
  json += "{\"id\":3,\"name\":\"Sparkle\"},";
  json += "{\"id\":4,\"name\":\"Fire\"},";
  json += "{\"id\":5,\"name\":\"Comet\"},";
  json += "{\"id\":6,\"name\":\"Breathing\"},";
  json += "{\"id\":7,\"name\":\"Strobe\"},";
  json += "{\"id\":8,\"name\":\"Meteor\"},";
  json += "{\"id\":9,\"name\":\"Wipe\"},";
  json += "{\"id\":10,\"name\":\"Plasma\"},";
  json += "{\"id\":11,\"name\":\"VU Meter\"},";
  json += "{\"id\":12,\"name\":\"Pulse\"},";
  json += "{\"id\":13,\"name\":\"Audio Rainbow\"},";
  json += "{\"id\":14,\"name\":\"Center Burst\"},";
  json += "{\"id\":15,\"name\":\"Audio Sparkle\"},";
  json += "{\"id\":16,\"name\":\"Split Spin\"},";
  json += "{\"id\":17,\"name\":\"Theater Chase\"}";
  json += "],";

  json += "\"settings\":{";
  json += "\"brightness\":" + String(state.brightness) + ",";
  json += "\"framerate\":" + String(state.frameRate) + ",";
  json += "\"mode\":" + String(state.currentMode) + ",";
  json += "\"index\":" + String(state.currentIndex) + ",";
  json += "\"timestamp\":" + String(millis());
  json += "}}";
  
  server.send(200, "application/json", json);
}

// Handle sync push request (POST)
void handleSyncPush() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  Serial.println("Received sync data:");
  Serial.println(body);
  
  // Parse and apply sync data (simplified)
  // In full implementation, would parse JSON and update settings/images/patterns
  
  // For now, just acknowledge receipt
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"message\":\"Sync data received\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handle device configuration request (GET)
void handleDeviceConfig() {
  String json = "{";
  json += "\"deviceId\":\"" + deviceConfig.deviceId + "\",";
  json += "\"deviceName\":\"" + deviceConfig.deviceName + "\",";
  json += "\"syncGroup\":\"" + deviceConfig.syncGroup + "\",";
  json += "\"autoSync\":" + String(deviceConfig.autoSync ? "true" : "false") + ",";
  json += "\"syncInterval\":" + String(deviceConfig.syncInterval);
  json += "}";
  
  server.send(200, "application/json", json);
}

// Handle device configuration update (POST)
void handleDeviceConfigUpdate() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Parse and update configuration (simplified)
  // In full implementation, would use proper JSON parser
  
  int nameIdx = body.indexOf("\"deviceName\":");
  if (nameIdx != -1) {
    int startIdx = body.indexOf("\"", nameIdx + 13) + 1;
    int endIdx = body.indexOf("\"", startIdx);
    deviceConfig.deviceName = body.substring(startIdx, endIdx);
  }
  
  int groupIdx = body.indexOf("\"syncGroup\":");
  if (groupIdx != -1) {
    int startIdx = body.indexOf("\"", groupIdx + 12) + 1;
    int endIdx = body.indexOf("\"", startIdx);
    deviceConfig.syncGroup = body.substring(startIdx, endIdx);
  }
  
  int autoSyncIdx = body.indexOf("\"autoSync\":");
  if (autoSyncIdx != -1) {
    deviceConfig.autoSync = body.indexOf("true", autoSyncIdx) != -1;
  }
  
  // Save configuration
  saveDeviceConfig();
  
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"message\":\"Configuration updated\"";
  json += "}";
  
  server.send(200, "application/json", json);
}
