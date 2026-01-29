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
#include <FS.h>
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
void handleManifest();
void handleServiceWorker();
void handleNotFound();
void sendFile(const char* path, const char* contentType);
void sendTeensyCommand(uint8_t cmd, uint8_t dataLen);

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
  state.lastSync = 0;
  state.lastDiscovery = 0;
  
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
  
  // Sync API endpoints
  server.on("/api/sync/status", HTTP_GET, handleSyncStatus);
  server.on("/api/sync/discover", HTTP_POST, handleSyncDiscover);
  server.on("/api/sync/execute", HTTP_POST, handleSyncExecute);
  server.on("/api/sync/data", HTTP_GET, handleSyncData);
  server.on("/api/sync/push", HTTP_POST, handleSyncPush);
  
  // Device configuration endpoints
  server.on("/api/device/config", HTTP_GET, handleDeviceConfig);
  server.on("/api/device/config", HTTP_POST, handleDeviceConfigUpdate);
  
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
        </div>
    </div>
    
    <script>
        let currentMode = 2;
        let currentPattern = 0;
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
                        // Update height based on current width and new aspect ratio if lock is on
                        if (document.getElementById('aspect-ratio-lock').checked) {
                            const width = parseInt(document.getElementById('image-width').value);
                            const newHeight = Math.round(width * originalImageAspectRatio);
                            document.getElementById('image-height').value = Math.min(200, Math.max(1, newHeight));
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
            if (!aspectLock) return;
            
            const widthInput = document.getElementById('image-width');
            const heightInput = document.getElementById('image-height');
            
            if (changedField === 'width') {
                const newWidth = parseInt(widthInput.value) || 31;
                const newHeight = Math.round(newWidth * originalImageAspectRatio);
                heightInput.value = Math.min(200, Math.max(1, newHeight));
            } else if (changedField === 'height') {
                const newHeight = parseInt(heightInput.value) || 64;
                const newWidth = Math.round(newHeight / originalImageAspectRatio);
                widthInput.value = Math.min(100, Math.max(1, newWidth));
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
            } catch (e) {
                document.getElementById('connection-status').textContent = 'Error';
            }
        }
        
        async function changeMode() {
            const mode = document.getElementById('mode-select').value;
            currentMode = parseInt(mode);
            
            await fetch('/api/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({mode: currentMode, index: 0})
            });
            
            updateStatus();
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
                            const targetWidth = parseInt(document.getElementById('image-width').value) || 31;
                            const targetHeight = parseInt(document.getElementById('image-height').value) || 64;
                            
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
  json += "\"framerate\":" + String(state.frameRate);
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
    
    // Parse JSON manually (simple parsing)
    uint8_t index = 0;
    uint8_t type = 0;
    uint8_t r1 = 255, g1 = 0, b1 = 0;
    uint8_t r2 = 0, g2 = 0, b2 = 255;
    uint8_t speed = 50;
    
    int idx = body.indexOf("\"type\":");
    if (idx != -1) {
      type = body.substring(idx + 7, body.indexOf(",", idx)).toInt();
    }
    
    // Send pattern to Teensy
    sendTeensyCommand(0x03, 8);
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
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Serial.println("Upload aborted");
    server.send(500, "application/json", "{\"error\":\"Upload aborted\"}");
  }
}

void handleLiveFrame() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Send live frame command to Teensy
    sendTeensyCommand(0x05, 93);  // 31 LEDs * 3 bytes
    
    // Parse and send pixel data (simplified)
    // Real implementation would parse JSON array
    for (int i = 0; i < 31; i++) {
      TEENSY_SERIAL.write(0);  // R
      TEENSY_SERIAL.write(0);  // G
      TEENSY_SERIAL.write(0);  // B
    }
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
  unsigned long start = millis();
  while (millis() - start < 100) {
    if (TEENSY_SERIAL.available() >= 4) {
      if (TEENSY_SERIAL.read() == 0xFF && TEENSY_SERIAL.read() == 0xBB) {
        state.currentMode = TEENSY_SERIAL.read();
        state.currentIndex = TEENSY_SERIAL.read();
        state.connected = true;
        return;
      }
    }
  }
  state.connected = false;
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
