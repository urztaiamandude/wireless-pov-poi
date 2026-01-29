/*
 * Nebula Poi - ESP32 Firmware
 * 
 * This firmware creates a WiFi Access Point with a web portal for controlling
 * the Nebula Poi. It communicates with the Teensy 4.1 via Serial to send
 * images, patterns, and sequences.
 * 
 * Features:
 * - WiFi Access Point mode
 * - Web server with file upload
 * - Image/pattern/sequence management
 * - Serial communication with Teensy
 * - REST API for mobile apps
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Preferences.h>

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

// Sync function declarations
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

// WiFi Configuration
const char* ssid = "POV-POI-WiFi";
const char* password = "povpoi123";

// Serial Configuration
#define TEENSY_SERIAL Serial2
#define SERIAL_BAUD 115200

// Sync Configuration
#define AUTO_SYNC_ENABLED false
#define AUTO_SYNC_INTERVAL 30000  // 30 seconds
#define PEER_DISCOVERY_INTERVAL 60000  // 60 seconds
#define PEER_TIMEOUT 120000  // 2 minutes

// Web Server
WebServer server(80);

// Preferences for persistent storage
Preferences preferences;

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

// Maximum peers
#define MAX_PEERS 5
PeerDevice peers[MAX_PEERS];
int peerCount = 0;

// System state
struct SystemState {
  uint8_t currentMode;
  uint8_t currentIndex;
  uint8_t brightness;
  uint8_t frameRate;
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
  TEENSY_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, 16, 17);  // RX=16, TX=17
  
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
  
  // Initialize WiFi Access Point
  setupWiFi();
  
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
  state.connected = false;
<<<<<<< HEAD
  state.lastSync = 0;
  state.lastDiscovery = 0;
=======
  state.sdCardPresent = false;
>>>>>>> session/agent_2e04f9ce-de7f-4893-bf0e-ebf078df6ecb
  
  Serial.println("ESP32 Nebula Poi Controller Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  
  // Check Teensy connection periodically
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    checkTeensyConnection();
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
  
<<<<<<< HEAD
  // Sync API endpoints
  server.on("/api/sync/status", HTTP_GET, handleSyncStatus);
  server.on("/api/sync/discover", HTTP_POST, handleSyncDiscover);
  server.on("/api/sync/execute", HTTP_POST, handleSyncExecute);
  server.on("/api/sync/data", HTTP_GET, handleSyncData);
  server.on("/api/sync/push", HTTP_POST, handleSyncPush);
  
  // Device configuration endpoints
  server.on("/api/device/config", HTTP_GET, handleDeviceConfig);
  server.on("/api/device/config", HTTP_POST, handleDeviceConfigUpdate);
=======
  // SD card management endpoints
  server.on("/api/sd/list", HTTP_GET, handleSDList);
  server.on("/api/sd/info", HTTP_GET, handleSDInfo);
  server.on("/api/sd/delete", HTTP_POST, handleSDDelete);
  server.on("/api/sd/load", HTTP_POST, handleSDLoad);
>>>>>>> session/agent_2e04f9ce-de7f-4893-bf0e-ebf078df6ecb
  
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
                    🎨 Patterns: 0=Rainbow, 1=Fire, 2=Comet, 3=Breathing, 4=Plasma<br>
                    🎬 Sequences: 0=Demo Mix
                </div>
                <div class="control-group" style="margin-top: 15px;">
                    <label for="content-index">Content Index (0-15):</label>
                    <input type="number" id="content-index" min="0" max="15" value="0" onchange="changeContentIndex()">
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
                    <button class="pattern-btn" onclick="setPattern(0)">🌈 Rainbow</button>
                    <button class="pattern-btn" onclick="setPattern(1)">🌊 Wave</button>
                    <button class="pattern-btn" onclick="setPattern(2)">🎨 Gradient</button>
                    <button class="pattern-btn" onclick="setPattern(3)">✨ Sparkle</button>
                    <button class="pattern-btn" onclick="setPattern(4)">🔥 Fire</button>
                    <button class="pattern-btn" onclick="setPattern(5)">☄️ Comet</button>
                    <button class="pattern-btn" onclick="setPattern(6)">💨 Breathing</button>
                    <button class="pattern-btn" onclick="setPattern(7)">⚡ Strobe</button>
                    <button class="pattern-btn" onclick="setPattern(8)">🌠 Meteor</button>
                    <button class="pattern-btn" onclick="setPattern(9)">🖌️ Wipe</button>
                    <button class="pattern-btn" onclick="setPattern(10)">🌀 Plasma</button>
                </div>
            </div>
            
            <!-- Audio Reactive Patterns -->
            <div class="section">
                <h2>🎵 Audio Reactive</h2>
                <p style="font-size: 12px; color: #666; margin-bottom: 10px;">Requires microphone on Teensy pin A0</p>
                <div class="pattern-grid">
                    <button class="pattern-btn" onclick="setPattern(11)" style="background: linear-gradient(135deg, #00ff88 0%, #ff0088 100%); color: white; border: none;">📊 VU Meter</button>
                    <button class="pattern-btn" onclick="setPattern(12)" style="background: linear-gradient(135deg, #ff0088 0%, #00ff88 100%); color: white; border: none;">💓 Pulse</button>
                    <button class="pattern-btn" onclick="setPattern(13)" style="background: linear-gradient(135deg, #ff0000 0%, #00ff00 50%, #0000ff 100%); color: white; border: none;">🌈 Audio Rainbow</button>
                    <button class="pattern-btn" onclick="setPattern(14)" style="background: linear-gradient(135deg, #8800ff 0%, #ff8800 100%); color: white; border: none;">🎯 Center Burst</button>
                    <button class="pattern-btn" onclick="setPattern(15)" style="background: linear-gradient(135deg, #ffff00 0%, #ff00ff 100%); color: white; border: none;">✨ Audio Sparkle</button>
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
                        <input type="number" id="image-width" min="1" max="100" value="31" style="width: 100%; padding: 8px;" oninput="updateImageDimensions('width')">
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
                <canvas id="live-canvas" class="live-canvas" width="310" height="200"></canvas>
                <button onclick="clearCanvas()" style="margin-top: 10px;">Clear</button>
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
        let currentMode = 2;
        let currentPattern = 0;
        // aspect ratio is stored as height / width
        let originalImageAspectRatio = 1.0;
        
        // Initialize
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
                        let targetHeight = parseInt(heightInput.value) || 31;
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
                    let newHeight = parseInt(heightInput.value) || 31;
                    newHeight = Math.min(200, Math.max(1, newHeight));
                    heightInput.value = newHeight;

                    const newWidth = Math.round(newHeight / originalImageAspectRatio);
                    widthInput.value = Math.min(100, Math.max(1, newWidth));
                } else if (changedField === 'width') {
                    // If the user changes width while locked, ignore that as a
                    // source of truth and instead snap width back to the value
                    // implied by the current LED count (height).
                    let ledCount = parseInt(heightInput.value) || 31;
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
            const imageData = ctx.getImageData(0, 0, 31, 1);
            const pixels = [];
            
            for (let i = 0; i < 31; i++) {
                const idx = i * 4;
                pixels.push({
                    r: imageData.data[idx],
                    g: imageData.data[idx + 1],
                    b: imageData.data[idx + 2]
                });
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
                
                // Update SD card status if available
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
            const color1 = document.getElementById('color1').value;
            const color2 = document.getElementById('color2').value;
            const speed = document.getElementById('pattern-speed').value;
            
            await fetch('/api/pattern', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    index: 0,
                    type: type,
                    color1: hexToRgb(color1),
                    color2: hexToRgb(color2),
                    speed: parseInt(speed)
                })
            });
            
            // Switch to pattern mode
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
                
                // Create blob from raw RGB data
                const blob = new Blob([povImageData.data], { type: 'application/octet-stream' });
                
                // Send raw RGB data to server
                const response = await fetch('/api/image', {
                    method: 'POST',
                    body: blob
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
                            let targetHeight = parseInt(document.getElementById('image-height').value) || 31;
                            targetHeight = Math.min(200, Math.max(1, targetHeight));

                            let targetWidth = parseInt(document.getElementById('image-width').value) || 31;
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
        setInterval(updateSDStatus, 5000);
        setInterval(refreshSDList, 10000);  // Refresh list every 10 seconds
        
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
  String json = "{";
  json += "\"connected\":" + String(state.connected ? "true" : "false") + ",";
  json += "\"mode\":" + String(state.currentMode) + ",";
  json += "\"index\":" + String(state.currentIndex) + ",";
  json += "\"brightness\":" + String(state.brightness) + ",";
  json += "\"framerate\":" + String(state.frameRate) + ",";
  json += "\"sdCardPresent\":" + String(state.sdCardPresent ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
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
      uint8_t frameDelay = 1000 / state.frameRate;
      
      // Send command to Teensy
      sendTeensyCommand(0x07, 1);
      TEENSY_SERIAL.write(frameDelay);
      TEENSY_SERIAL.write(0xFE);
      
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

    // color1 components
    int c1 = body.indexOf("\"color1\"");
    if (c1 != -1) {
      String c1Sub = body.substring(c1);
      parseUintField("\"r\"", r1, r1);
      parseUintField("\"g\"", g1, g1);
      parseUintField("\"b\"", b1, b1);
    }

    // color2 components
    int c2 = body.indexOf("\"color2\"");
    if (c2 != -1) {
      String c2Sub = body.substring(c2);
      // Temporarily operate on c2Sub for parsing
      auto parseUintFieldLocal = [&](String& src, const char* key, uint8_t& outValue, uint8_t defaultValue) {
        int k = src.indexOf(key);
        if (k == -1) {
          outValue = defaultValue;
          return;
        }
        int start = src.indexOf(":", k);
        if (start == -1) {
          outValue = defaultValue;
          return;
        }
        int end = src.indexOf(",", start);
        int endBrace = src.indexOf("}", start);
        if (end == -1 || (endBrace != -1 && endBrace < end)) {
          end = endBrace;
        }
        if (end == -1) {
          end = src.length();
        }
        String num = src.substring(start + 1, end);
        num.trim();
        long v = num.toInt();
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        outValue = (uint8_t)v;
      };

      parseUintFieldLocal(c2Sub, "\"r\"", r2, r2);
      parseUintFieldLocal(c2Sub, "\"g\"", g2, g2);
      parseUintFieldLocal(c2Sub, "\"b\"", b2, b2);
    }
    
    // Clamp the pattern index to the range supported by the Teensy engine (0-4)
    if (index > 4) {
      index = 4;
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
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleUploadImage() {
  // Handle image upload from web interface
  // Images are pre-converted to RGB data by the web interface
  // Format: raw RGB bytes (width * height * 3)
  
  HTTPUpload& upload = server.upload();
  static uint8_t imageBuffer[31 * 64 * 3];  // Max image: 31x64 RGB
  static uint16_t bufferIndex = 0;
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    bufferIndex = 0;
    
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
    
    // Detect image dimensions from data size
    // Expected: 31 * height * 3 bytes
    uint16_t imageWidth = 31;
    uint16_t imageHeight = bufferIndex / (imageWidth * 3);
    
    if (imageHeight > 64) imageHeight = 64;
    if (imageHeight < 1) imageHeight = 1;
    
    uint16_t actualSize = imageWidth * imageHeight * 3;
    
    Serial.printf("Detected image: %dx%d (%d bytes)\n", imageWidth, imageHeight, actualSize);
    
    // Send image data to Teensy for processing
    // Protocol: 0xFF 0x02 dataLen_high dataLen_low width height [RGB data...] 0xFE
    TEENSY_SERIAL.write(0xFF);  // Start marker
    TEENSY_SERIAL.write(0x02);  // Upload Image command
    TEENSY_SERIAL.write((actualSize >> 8) & 0xFF);  // Data length high byte
    TEENSY_SERIAL.write(actualSize & 0xFF);  // Data length low byte
    TEENSY_SERIAL.write(imageWidth);  // Image width
    TEENSY_SERIAL.write(imageHeight);  // Image height
    
    // Send pixel data
    for (uint16_t i = 0; i < actualSize && i < bufferIndex; i++) {
      TEENSY_SERIAL.write(imageBuffer[i]);
    }
    TEENSY_SERIAL.write(0xFE);  // End marker
    
    Serial.println("Image forwarded to Teensy");
    
    // Set mode to image display
    delay(100);
    state.currentMode = 1;
    state.currentIndex = 0;
    sendTeensyCommand(0x01, 2);
    TEENSY_SERIAL.write(state.currentMode);
    TEENSY_SERIAL.write(state.currentIndex);
    TEENSY_SERIAL.write(0xFE);
    
    // Auto-save to SD card if present
    // Check SD status first (from last status check)
    if (state.sdCardPresent) {
      delay(50);  // Small delay to ensure previous command processed
      
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
    const int ledCount = 31;  // Display LEDs (Teensy uses 31 display LEDs)
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
    sendTeensyCommand(0x05, ledCount * 3);  // 31 LEDs * 3 bytes
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
  
  // Wait for response: 0xFF 0xCC count [filename_len][filename]... 0xFE
  delay(100);
  uint8_t buffer[2048];
  size_t bytesRead = 0;
  
  if (readTeensyResponse(0xCC, buffer, sizeof(buffer), bytesRead, 500)) {
    if (bytesRead > 0) {
      uint8_t count = buffer[0];
      String json = "{\"files\":[";
      
      size_t pos = 1;
      for (uint8_t i = 0; i < count && pos < bytesRead; i++) {
        if (i > 0) json += ",";
        uint8_t nameLen = buffer[pos++];
        if (pos + nameLen <= bytesRead) {
          String filename = "";
          for (uint8_t j = 0; j < nameLen; j++) {
            filename += (char)buffer[pos++];
          }
          json += "\"" + filename + "\"";
        }
      }
      json += "]}";
      server.send(200, "application/json", json);
      return;
    }
  }
  
  server.send(200, "application/json", "{\"error\":\"Failed to read response\",\"files\":[]}");
}

void handleSDInfo() {
  // Send info command
  sendTeensyCommand(0x23, 0);
  TEENSY_SERIAL.write(0xFE);
  
  // Wait for response: 0xFF 0xDD present [total:8][free:8] 0xFE
  delay(100);
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
      
      String json = "{";
      json += "\"present\":" + String(present ? "true" : "false") + ",";
      json += "\"totalSpace\":" + String(totalSpace) + ",";
      json += "\"freeSpace\":" + String(freeSpace);
      json += "}";
      server.send(200, "application/json", json);
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
  sendTeensyCommand(0x10, 0);
  TEENSY_SERIAL.write(0xFE);
  
  // Wait for response (simplified)
  // Format: 0xFF 0xBB mode index sd_present 0xFE
  unsigned long start = millis();
  while (millis() - start < 100) {
    if (TEENSY_SERIAL.available() >= 5) {
      if (TEENSY_SERIAL.read() == 0xFF && TEENSY_SERIAL.read() == 0xBB) {
        state.currentMode = TEENSY_SERIAL.read();
        state.currentIndex = TEENSY_SERIAL.read();
        state.sdCardPresent = (TEENSY_SERIAL.read() != 0);
        state.connected = true;
        return;
      }
    }
  }
  state.connected = false;
  state.sdCardPresent = false;
}

// ============================================================================
// PEER-TO-PEER SYNC IMPLEMENTATION
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
  json += "\"images\":[],";  // TODO: Implement image listing
  json += "\"patterns\":[],";  // TODO: Implement pattern listing
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
