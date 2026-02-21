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
void handleWifiStatus();
void handleWifiConnect();
void handleWifiDisconnect();
void saveWifiStaConfig(const String& ssid, const String& pass);
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

// WiFi STA (client) credentials - saved to Preferences, used to connect to existing network
String staSsid = "";
String staPassword = "";

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
  Serial.println("Starting Access Point (AP+STA)...");
  
  // AP+STA: AP for direct connection (POV-POI-WiFi), STA to connect to existing network
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  
  // Configure AP IP (192.168.4.1 is default)
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP SSID: ");
  Serial.println(ssid);
  
  // If we have saved STA credentials, try to connect to that network (non-blocking)
  if (staSsid.length() > 0) {
    Serial.printf("Connecting to saved network: %s\n", staSsid.c_str());
    WiFi.begin(staSsid.c_str(), staPassword.c_str());
  }
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
  
  // WiFi network (STA) configuration - connect to existing network for web UI access
  server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);
  server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);
  server.on("/api/wifi/disconnect", HTTP_POST, handleWifiDisconnect);

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
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="theme-color" content="#0f172a">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="Nebula Poi">
    <link rel="manifest" href="/manifest.json">
    <title>Nebula Poi Controller</title>
    <style>
        *{margin:0;padding:0;box-sizing:border-box}
        body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#020617;color:#e2e8f0;min-height:100vh;font-size:14px;line-height:1.5;overflow-x:hidden}
        .app{display:flex;flex-direction:column;min-height:100vh}

        /* Top Navigation */
        .topnav{background:#0f172a;border-bottom:1px solid #1e293b;padding:0;position:sticky;top:0;z-index:50;overflow-x:auto;-webkit-overflow-scrolling:touch}
        .topnav-inner{display:flex;align-items:center;min-width:max-content}
        .brand{padding:12px 20px;display:flex;align-items:center;gap:10px;border-right:1px solid #1e293b;flex-shrink:0}
        .brand-icon{width:28px;height:28px;background:linear-gradient(135deg,#06b6d4,#a855f7);border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:14px;font-weight:900;color:#fff}
        .brand h1{font-size:16px;font-weight:800;background:linear-gradient(90deg,#06b6d4,#a855f7);-webkit-background-clip:text;-webkit-text-fill-color:transparent;white-space:nowrap}
        .brand sub{font-size:9px;color:#475569;font-family:monospace;display:block}
        .nav-tabs{display:flex;gap:0;flex:1}
        .nav-tab{padding:14px 18px;color:#64748b;font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.08em;cursor:pointer;border:none;background:none;white-space:nowrap;transition:all .2s;border-bottom:2px solid transparent;position:relative}
        .nav-tab:hover{color:#cbd5e1;background:rgba(30,41,59,.5)}
        .nav-tab.active{color:#06b6d4;border-bottom-color:#06b6d4;background:rgba(6,182,212,.05)}
        .status-dot{position:absolute;top:10px;right:10px;width:6px;height:6px;border-radius:50%;background:#ef4444}
        .status-dot.on{background:#22c55e;animation:pulse 2s infinite}
        @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

        /* Main Content */
        .main{flex:1;padding:16px;max-width:900px;margin:0 auto;width:100%}
        .tab-content{display:none;animation:fadeIn .3s ease}
        .tab-content.active{display:block}
        @keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:none}}

        /* Cards */
        .card{background:#0f172a;border:1px solid #1e293b;border-radius:16px;padding:20px;margin-bottom:16px}
        .card-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
        .card-title{font-size:11px;font-weight:800;text-transform:uppercase;letter-spacing:.1em;color:#64748b;display:flex;align-items:center;gap:8px}
        .card-title .dot{width:8px;height:8px;border-radius:50%}
        h2{font-size:13px;font-weight:800;text-transform:uppercase;letter-spacing:.1em;color:#94a3b8;margin-bottom:14px}

        /* Status Banner */
        .status-banner{display:flex;gap:12px;flex-wrap:wrap;margin-bottom:16px}
        .stat-chip{background:#1e293b;border:1px solid #334155;border-radius:10px;padding:10px 16px;flex:1;min-width:120px;display:flex;flex-direction:column;gap:2px}
        .stat-label{font-size:9px;font-weight:800;text-transform:uppercase;letter-spacing:.12em;color:#64748b}
        .stat-value{font-size:16px;font-weight:800;color:#e2e8f0;font-family:monospace}
        .stat-value.ok{color:#22c55e}
        .stat-value.err{color:#ef4444}
        .stat-value.cyan{color:#06b6d4}

        /* LED Preview */
        .led-strip{display:flex;gap:3px;padding:12px;background:#000;border-radius:12px;border:1px solid #1e293b;overflow-x:auto}
        .led{width:18px;height:36px;border-radius:4px;background:#111827;transition:background .15s;flex-shrink:0}

        /* Controls */
        .ctrl{margin-bottom:16px}
        .ctrl label{display:flex;justify-content:space-between;align-items:center;font-size:11px;font-weight:700;color:#94a3b8;text-transform:uppercase;letter-spacing:.08em;margin-bottom:8px}
        .ctrl label span{color:#06b6d4;font-family:monospace;font-size:13px}
        input[type=range]{-webkit-appearance:none;width:100%;height:6px;background:#1e293b;border-radius:3px;outline:none;cursor:pointer}
        input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:#06b6d4;cursor:pointer;border:2px solid #0f172a;box-shadow:0 0 8px rgba(6,182,212,.4)}
        input[type=range]::-moz-range-thumb{width:22px;height:22px;border-radius:50%;background:#06b6d4;cursor:pointer;border:2px solid #0f172a}
        select,input[type=number],input[type=file]{width:100%;padding:10px 14px;background:#0f172a;border:1px solid #334155;border-radius:10px;color:#e2e8f0;font-size:14px;outline:none;transition:border .2s}
        select:focus,input[type=number]:focus{border-color:#06b6d4}
        input[type=color]{width:100%;height:48px;border:1px solid #334155;border-radius:10px;background:#0f172a;cursor:pointer;padding:4px}

        /* Buttons */
        .btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;padding:12px 20px;border:none;border-radius:12px;font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:.06em;cursor:pointer;transition:all .2s;min-height:44px;width:100%;color:#fff}
        .btn:active{transform:scale(.97)}
        .btn-primary{background:linear-gradient(135deg,#06b6d4,#a855f7)}
        .btn-primary:hover{box-shadow:0 4px 20px rgba(6,182,212,.3)}
        .btn-green{background:#16a34a}
        .btn-green:hover{background:#22c55e}
        .btn-red{background:#dc2626}
        .btn-red:hover{background:#ef4444}
        .btn-slate{background:#334155;color:#e2e8f0}
        .btn-slate:hover{background:#475569}
        .btn-sm{padding:8px 14px;font-size:11px;min-height:36px;border-radius:8px}
        .btn-ghost{background:transparent;border:1px solid #334155;color:#94a3b8}
        .btn-ghost:hover{border-color:#64748b;color:#e2e8f0}
        .btn-icon{width:auto;padding:8px 12px}

        /* Pattern Grid */
        .pgrid{display:grid;grid-template-columns:repeat(auto-fill,minmax(100px,1fr));gap:8px}
        .pbtn{padding:14px 8px;background:#1e293b;border:1px solid #334155;color:#cbd5e1;cursor:pointer;border-radius:10px;transition:all .2s;font-size:12px;font-weight:600;text-align:center;min-height:44px}
        .pbtn:hover{border-color:#06b6d4;background:rgba(6,182,212,.1);color:#06b6d4;transform:translateY(-1px)}
        .pbtn.active{border-color:#06b6d4;background:rgba(6,182,212,.15);color:#06b6d4;box-shadow:0 0 12px rgba(6,182,212,.2)}
        .pbtn-audio{background:linear-gradient(135deg,var(--g1),var(--g2));border:none;color:#fff;font-weight:700}
        .pbtn-audio:hover{transform:translateY(-2px);box-shadow:0 4px 16px rgba(0,0,0,.3)}

        /* Two-column layout */
        .grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
        .grid3{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
        .flex-row{display:flex;gap:10px}
        .flex-1{flex:1}

        /* Canvas */
        .draw-canvas{width:100%;height:200px;border:1px solid #334155;border-radius:12px;background:#000;cursor:crosshair;touch-action:none}

        /* Checkbox */
        .chk{display:flex;align-items:center;gap:8px;font-size:13px;color:#94a3b8;cursor:pointer}
        .chk input{width:18px;height:18px;accent-color:#06b6d4;cursor:pointer}

        /* Sync Section */
        .sync-info{background:#1e293b;border:1px solid #334155;border-radius:10px;padding:14px;margin-bottom:14px;font-size:13px;color:#94a3b8}
        .sync-info strong{color:#e2e8f0}
        .peer-item{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;margin:6px 0;background:#1e293b;border-radius:10px;border-left:3px solid var(--pc,#64748b)}
        .peer-item .name{font-weight:700;color:#e2e8f0;font-size:13px}
        .peer-item .meta{font-size:11px;color:#64748b}
        .peer-panel{background:#1e293b;border:1px solid #334155;border-radius:12px;padding:16px;margin-bottom:12px}
        .peer-panel .peer-title{font-weight:700;color:#a855f7;margin-bottom:10px;font-size:13px}

        /* SD Card */
        .sd-file{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;margin:4px 0;background:#1e293b;border-radius:8px}
        .sd-file .fname{flex:1;font-size:13px;color:#cbd5e1;font-family:monospace}
        .sd-file .actions{display:flex;gap:6px}

        /* Toast / Notification */
        .toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#1e293b;border:1px solid #334155;border-radius:12px;padding:12px 24px;font-size:13px;color:#e2e8f0;box-shadow:0 8px 32px rgba(0,0,0,.5);z-index:100;display:none;animation:slideUp .3s ease}
        @keyframes slideUp{from{opacity:0;transform:translate(-50%,20px)}to{opacity:1;transform:translate(-50%,0)}}

        /* PWA Install */
        #installButton{display:none;position:fixed;bottom:20px;right:20px;z-index:60;background:#16a34a;color:#fff;border:none;padding:12px 20px;border-radius:50px;box-shadow:0 4px 16px rgba(0,0,0,.4);cursor:pointer;font-weight:700;font-size:13px;min-height:auto;width:auto}

        /* Scrollbar */
        ::-webkit-scrollbar{width:6px;height:6px}
        ::-webkit-scrollbar-track{background:#0f172a}
        ::-webkit-scrollbar-thumb{background:#334155;border-radius:3px}

        /* Responsive */
        @media(max-width:640px){
            .topnav-inner{padding-right:8px}
            .brand{padding:10px 14px}
            .brand h1{font-size:14px}
            .nav-tab{padding:12px 12px;font-size:10px}
            .main{padding:10px}
            .card{padding:14px;border-radius:12px}
            .pgrid{grid-template-columns:repeat(2,1fr)}
            .grid2{grid-template-columns:1fr}
            .status-banner{flex-direction:column}
            .stat-chip{min-width:auto}
        }
    </style>
</head>
<body>
<div class="app">
    <!-- PWA Install -->
    <button id="installButton">Install App</button>

    <!-- Top Navigation -->
    <nav class="topnav">
        <div class="topnav-inner">
            <div class="brand">
                <div class="brand-icon">N</div>
                <div><h1>Nebula Poi</h1><sub>ESP32-S3 | POV-POI-WiFi</sub></div>
            </div>
            <div class="nav-tabs" id="nav-tabs">
                <button class="nav-tab active" data-tab="dashboard">Dashboard<span class="status-dot" id="conn-dot"></span></button>
                <button class="nav-tab" data-tab="patterns">Patterns</button>
                <button class="nav-tab" data-tab="imagelab">Image Lab</button>
                <button class="nav-tab" data-tab="livedraw">Live Draw</button>
                <button class="nav-tab" data-tab="sync">Multi-Poi</button>
                <button class="nav-tab" data-tab="sdcard">SD Card</button>
                <button class="nav-tab" data-tab="settings">Settings</button>
            </div>
        </div>
    </nav>

    <div class="main">
        <!-- ===== DASHBOARD TAB ===== -->
        <div class="tab-content active" id="tab-dashboard">
            <!-- Status Banner -->
            <div class="status-banner">
                <div class="stat-chip">
                    <div class="stat-label">Connection</div>
                    <div class="stat-value" id="conn-status">--</div>
                </div>
                <div class="stat-chip">
                    <div class="stat-label">Mode</div>
                    <div class="stat-value cyan" id="mode-display">Idle</div>
                </div>
                <div class="stat-chip">
                    <div class="stat-label">Brightness</div>
                    <div class="stat-value" id="bright-display">128</div>
                </div>
                <div class="stat-chip">
                    <div class="stat-label">FPS</div>
                    <div class="stat-value" id="fps-display">50</div>
                </div>
            </div>

            <!-- LED Preview -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#a855f7"></span> LED Preview</div>
                <div class="led-strip" id="led-preview"></div>
            </div>

            <!-- Quick Actions -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#06b6d4"></span> Quick Actions</div>
                <div class="grid2" style="margin-bottom:14px">
                    <button class="btn btn-green" onclick="quickAction('play')">PLAY</button>
                    <button class="btn btn-red" onclick="quickAction('stop')">STOP</button>
                </div>
                <button class="btn btn-slate" onclick="quickAction('random')">Synced Pattern Cycle</button>
            </div>

            <!-- Mode & Index -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#f59e0b"></span> Display Mode</div>
                <div class="ctrl">
                    <label>Mode</label>
                    <select id="mode-select" onchange="changeMode()">
                        <option value="0">Idle (Off)</option>
                        <option value="1">Image Display</option>
                        <option value="2" selected>Pattern Display</option>
                        <option value="3">Sequence</option>
                        <option value="4">Live Mode</option>
                    </select>
                </div>
                <div class="ctrl">
                    <label>Content Index <span id="idx-display">0</span></label>
                    <input type="number" id="content-index" min="0" max="17" value="0" onchange="changeContentIndex()">
                </div>
                <div style="padding:10px;background:#1e293b;border-radius:8px;font-size:12px;color:#64748b;margin-top:4px">
                    Images: 0=Smiley, 1=Rainbow, 2=Heart | Patterns: 0-17 | Sequences: 0=Demo Mix
                </div>
            </div>

            <!-- Brightness & Frame Rate -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#eab308"></span> System Controls</div>
                <div class="ctrl">
                    <label>Brightness <span id="brightness-value">128</span></label>
                    <input type="range" id="brightness" min="0" max="255" value="128" oninput="updateBrightness(this.value)">
                </div>
                <div class="ctrl">
                    <label>Frame Rate <span id="framerate-value">50</span> FPS</label>
                    <input type="range" id="framerate" min="10" max="120" value="50" oninput="updateFrameRate(this.value)">
                </div>
            </div>
        </div>

        <!-- ===== PATTERNS TAB ===== -->
        <div class="tab-content" id="tab-patterns">
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#06b6d4"></span> Visual Patterns</div>
                <div class="pgrid" id="pattern-grid">
                    <button class="pbtn" data-pattern="0" onclick="setPattern(0)">Rainbow</button>
                    <button class="pbtn" data-pattern="1" onclick="setPattern(1)">Wave</button>
                    <button class="pbtn" data-pattern="2" onclick="setPattern(2)">Gradient</button>
                    <button class="pbtn" data-pattern="3" onclick="setPattern(3)">Sparkle</button>
                    <button class="pbtn" data-pattern="4" onclick="setPattern(4)">Fire</button>
                    <button class="pbtn" data-pattern="5" onclick="setPattern(5)">Comet</button>
                    <button class="pbtn" data-pattern="6" onclick="setPattern(6)">Breathing</button>
                    <button class="pbtn" data-pattern="7" onclick="setPattern(7)">Strobe</button>
                    <button class="pbtn" data-pattern="8" onclick="setPattern(8)">Meteor</button>
                    <button class="pbtn" data-pattern="9" onclick="setPattern(9)">Wipe</button>
                    <button class="pbtn" data-pattern="10" onclick="setPattern(10)">Plasma</button>
                    <button class="pbtn" data-pattern="16" onclick="setPattern(16)">Split Spin</button>
                    <button class="pbtn" data-pattern="17" onclick="setPattern(17)">Theater Chase</button>
                </div>
            </div>

            <div class="card">
                <div class="card-title"><span class="dot" style="background:#ec4899"></span> Audio Reactive</div>
                <div style="font-size:11px;color:#64748b;margin-bottom:10px">Requires MAX9814 microphone on Teensy pin A0</div>
                <div class="pgrid">
                    <button class="pbtn pbtn-audio" data-pattern="11" onclick="setPattern(11)" style="--g1:#00ff88;--g2:#ff0088">VU Meter</button>
                    <button class="pbtn pbtn-audio" data-pattern="12" onclick="setPattern(12)" style="--g1:#ff0088;--g2:#00ff88">Pulse</button>
                    <button class="pbtn pbtn-audio" data-pattern="13" onclick="setPattern(13)" style="--g1:#ff0000;--g2:#0000ff">Audio Rainbow</button>
                    <button class="pbtn pbtn-audio" data-pattern="14" onclick="setPattern(14)" style="--g1:#8800ff;--g2:#ff8800">Center Burst</button>
                    <button class="pbtn pbtn-audio" data-pattern="15" onclick="setPattern(15)" style="--g1:#ffff00;--g2:#ff00ff">Audio Sparkle</button>
                </div>
            </div>

            <div class="card">
                <div class="card-title"><span class="dot" style="background:#f59e0b"></span> Pattern Settings</div>
                <div class="grid2">
                    <div class="ctrl">
                        <label>Color 1</label>
                        <input type="color" id="color1" value="#ff0000">
                    </div>
                    <div class="ctrl">
                        <label>Color 2</label>
                        <input type="color" id="color2" value="#0000ff">
                    </div>
                </div>
                <div class="ctrl">
                    <label>Speed <span id="speed-value">50</span></label>
                    <input type="range" id="pattern-speed" min="1" max="255" value="50" oninput="updateSpeed(this.value)">
                </div>
            </div>
        </div>

        <!-- ===== IMAGE LAB TAB ===== -->
        <div class="tab-content" id="tab-imagelab">
            <!-- Upload Section -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#ec4899"></span> Image Upload</div>
                <div class="ctrl">
                    <label>Select Image</label>
                    <input type="file" id="image-upload" accept="image/*">
                </div>
                <div style="margin-bottom:12px">
                    <label class="chk"><input type="checkbox" id="aspect-ratio-lock" checked> Lock Aspect Ratio</label>
                </div>
                <div class="grid2">
                    <div class="ctrl">
                        <label>Width (px)</label>
                        <input type="number" id="image-width" min="1" max="100" value="32" oninput="updateImageDimensions('width')">
                    </div>
                    <div class="ctrl">
                        <label>Height (px)</label>
                        <input type="number" id="image-height" min="32" max="32" value="32" readonly>
                    </div>
                </div>
                <div class="flex-row" style="margin-bottom:14px">
                    <label class="chk"><input type="checkbox" id="flip-vertical"> Flip Vertical</label>
                    <label class="chk"><input type="checkbox" id="flip-horizontal"> Flip Horizontal</label>
                </div>
                <button class="btn btn-primary" onclick="uploadImage()">Upload & Display</button>
            </div>

            <!-- Procedural Generator -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#a855f7"></span> Procedural Generator</div>
                <div class="grid2" style="margin-bottom:14px">
                    <button class="btn btn-slate" id="btn-organic" onclick="setGenType('organic')" style="border:1px solid #06b6d4">Organic</button>
                    <button class="btn btn-slate" id="btn-geometric" onclick="setGenType('geometric')">Geometric</button>
                </div>
                <div class="ctrl">
                    <label>Complexity <span id="complexity-val">8</span></label>
                    <input type="range" id="gen-complexity" min="1" max="25" value="8" oninput="document.getElementById('complexity-val').textContent=this.value">
                </div>
                <div class="grid2">
                    <button class="btn btn-slate" onclick="generateArt()">Generate</button>
                    <button class="btn btn-slate" onclick="rollColors()">Roll Colors</button>
                </div>
                <canvas id="gen-canvas" style="width:100%;margin-top:14px;border-radius:8px;border:1px solid #1e293b;display:none"></canvas>
                <button class="btn btn-primary" onclick="uploadGenerated()" style="margin-top:12px;display:none" id="btn-upload-gen">Upload Generated Image</button>
            </div>

            <!-- Image Preview -->
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#22c55e"></span> Preview</div>
                <div id="img-preview-box" style="background:#000;border-radius:12px;border:1px solid #1e293b;min-height:80px;display:flex;align-items:center;justify-content:center;overflow:hidden">
                    <span style="color:#334155;font-size:12px">No image loaded</span>
                </div>
                <div id="img-dimensions" style="margin-top:8px;font-size:11px;color:#64748b;font-family:monospace;text-align:center"></div>
            </div>
        </div>

        <!-- ===== LIVE DRAW TAB ===== -->
        <div class="tab-content" id="tab-livedraw">
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#f97316"></span> Live Draw Mode</div>
                <div style="font-size:12px;color:#64748b;margin-bottom:12px">Draw on the canvas below. Colors are sampled and sent to the LEDs in real-time.</div>
                <canvas id="live-canvas" class="draw-canvas" width="320" height="200"></canvas>
                <div class="grid2" style="margin-top:12px">
                    <div class="ctrl">
                        <label>Brush Color</label>
                        <input type="color" id="draw-color" value="#ff0000">
                    </div>
                    <div class="ctrl">
                        <label>Brush Size <span id="brush-size-val">5</span></label>
                        <input type="range" id="brush-size" min="1" max="30" value="5" oninput="document.getElementById('brush-size-val').textContent=this.value">
                    </div>
                </div>
                <button class="btn btn-red" onclick="clearCanvas()">Clear Canvas</button>
            </div>
        </div>

        <!-- ===== MULTI-POI SYNC TAB ===== -->
        <div class="tab-content" id="tab-sync">
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#a855f7"></span> Multi-Poi Sync</div>
                <div class="sync-info">
                    <div>This Poi: <strong><span id="sync-local-name">--</span></strong></div>
                    <div>Sync Mode: <strong><span id="sync-mode-display">Mirror</span></strong></div>
                    <div>Paired: <strong><span id="sync-paired-status">No peers</span></strong></div>
                </div>

                <h2>Sync Mode</h2>
                <div class="grid2" style="margin-bottom:12px">
                    <button class="btn" id="btn-mirror" onclick="setSyncMode('mirror')" style="background:#06b6d4">Mirror</button>
                    <button class="btn btn-slate" id="btn-independent" onclick="setSyncMode('independent')">Independent</button>
                </div>
                <div style="font-size:12px;color:#64748b;margin-bottom:16px">
                    <strong>Mirror:</strong> Both poi show the same thing.
                    <strong>Independent:</strong> Control each poi separately.
                </div>

                <h2>Pairing</h2>
                <div class="grid2" style="margin-bottom:16px">
                    <button class="btn btn-green" onclick="pairPoi()">Pair</button>
                    <button class="btn btn-red" onclick="unpairPoi()">Unpair All</button>
                </div>

                <h2>Peers</h2>
                <div id="sync-peer-list">
                    <div style="text-align:center;color:#475569;font-size:12px;padding:12px">No paired poi found. Tap Pair to discover.</div>
                </div>

                <div id="independent-controls" style="display:none;margin-top:16px;border-top:1px solid #334155;padding-top:16px">
                    <h2>Peer Controls</h2>
                    <div id="peer-control-panels"></div>
                </div>
            </div>
        </div>

        <!-- ===== SD CARD TAB ===== -->
        <div class="tab-content" id="tab-sdcard">
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#f59e0b"></span> SD Card Storage</div>
                <div class="sync-info" style="margin-bottom:14px">
                    <div>Status: <strong><span id="sd-status-text">Checking...</span></strong></div>
                    <div id="sd-info" style="margin-top:4px;font-size:12px;color:#64748b"></div>
                </div>
                <button class="btn btn-slate" onclick="refreshSDList()" style="margin-bottom:14px">Refresh File List</button>
                <div id="sd-file-list" style="max-height:300px;overflow-y:auto">
                    <div style="text-align:center;color:#475569;font-size:12px;padding:12px">No files loaded</div>
                </div>
            </div>
        </div>

        <!-- ===== SETTINGS TAB ===== -->
        <div class="tab-content" id="tab-settings">
            <div class="card">
                <div class="card-title"><span class="dot" style="background:#06b6d4"></span> Device Configuration</div>
                <div class="ctrl">
                    <label>Device Name</label>
                    <input type="text" id="cfg-device-name" style="width:100%;padding:10px 14px;background:#0f172a;border:1px solid #334155;border-radius:10px;color:#e2e8f0;font-size:14px;outline:none" placeholder="Nebula Poi">
                </div>
                <div class="ctrl">
                    <label>Sync Group</label>
                    <input type="text" id="cfg-sync-group" style="width:100%;padding:10px 14px;background:#0f172a;border:1px solid #334155;border-radius:10px;color:#e2e8f0;font-size:14px;outline:none" placeholder="default">
                </div>
                <div style="margin-bottom:14px">
                    <label class="chk"><input type="checkbox" id="cfg-auto-sync"> Enable Auto Sync</label>
                </div>
                <button class="btn btn-primary" onclick="saveConfig()">Save Configuration</button>
                <div id="cfg-status" style="margin-top:10px;font-size:12px;color:#64748b;text-align:center"></div>
            </div>

            <div class="card">
                <div class="card-title"><span class="dot" style="background:#a855f7"></span> Hardware Info</div>
                <div style="font-family:monospace;font-size:12px;color:#94a3b8;line-height:2">
                    <div>Platform: <span style="color:#06b6d4">ESP32-S3 N16R8</span></div>
                    <div>Flash: <span style="color:#06b6d4">16MB</span></div>
                    <div>PSRAM: <span style="color:#06b6d4">8MB</span></div>
                    <div>WiFi SSID: <span style="color:#06b6d4">POV-POI-WiFi</span></div>
                    <div>IP Address: <span style="color:#06b6d4">192.168.4.1</span></div>
                    <div>Serial Baud: <span style="color:#06b6d4">115200</span></div>
                    <div>LEDs: <span style="color:#06b6d4">32x APA102 (SPI)</span></div>
                    <div>UART TX: <span style="color:#06b6d4">GPIO43</span> | RX: <span style="color:#06b6d4">GPIO44</span></div>
                    <div id="cfg-device-id" style="color:#64748b">Device ID: --</div>
                </div>
            </div>

            <div class="card">
                <div class="card-title"><span class="dot" style="background:#ef4444"></span> Display Engine</div>
                <div style="background:#000;border:1px solid #1e293b;border-radius:10px;padding:14px;font-family:monospace;font-size:11px;color:#94a3b8;line-height:1.8">
                    <div style="color:#475569">// Teensy 4.1 Hardware Allocation</div>
                    <div style="color:#06b6d4">#define NUM_LEDS 32</div>
                    <div style="color:#06b6d4">CRGB leds[NUM_LEDS];</div>
                    <div style="color:#a855f7">FastLED.addLeds&lt;APA102, 11, 13&gt;(leds, NUM_LEDS);</div>
                    <div style="color:#475569">// Modes: 0=Idle 1=Image 2=Pattern 3=Sequence 4=Live</div>
                    <div style="color:#475569">// Patterns: 0-10 Basic | 11-15 Audio | 16-17 Advanced</div>
                </div>
            </div>
        </div>
    </div>

    <!-- Toast notification -->
    <div class="toast" id="toast"></div>
</div>
    
    <script>
    const NUM_LEDS=32;
    const DISPLAY_LEDS=32;
    let currentMode=2,currentPattern=0,brightness=128,originalImageAspectRatio=1.0;
    let currentSyncMode='mirror',syncPeers=[];
    let genType='organic',colorSeed=Math.random();
    const MODES=['Idle','Image','Pattern','Sequence','Live'];

    // ===== Tab Navigation =====
    document.querySelectorAll('.nav-tab').forEach(tab=>{
        tab.addEventListener('click',()=>{
            document.querySelectorAll('.nav-tab').forEach(t=>t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c=>c.classList.remove('active'));
            tab.classList.add('active');
            document.getElementById('tab-'+tab.dataset.tab).classList.add('active');
        });
    });

    // ===== Toast =====
    function showToast(msg,duration){
        const t=document.getElementById('toast');
        t.textContent=msg;t.style.display='block';
        clearTimeout(t._tid);
        t._tid=setTimeout(()=>{t.style.display='none'},duration||2500);
    }

    // ===== LED Preview =====
    function initLEDPreview(){
        const c=document.getElementById('led-preview');c.innerHTML='';
        for(let i=DISPLAY_LED_START;i<NUM_LEDS;i++){const d=document.createElement('div');d.className='led';d.id='led-'+i;c.appendChild(d)}
    }
    function setLEDPreview(i,r,g,b){
        const d=document.getElementById('led-'+i);
        if(d){const s=brightness/255;d.style.background='rgb('+Math.round(r*s)+','+Math.round(g*s)+','+Math.round(b*s)+')'}
    }
    function updateLEDPreviewFromStatus(mode,index){
        for(let i=DISPLAY_LED_START;i<NUM_LEDS;i++)setLEDPreview(i,40,40,40);
        if(mode===0)return;
        const hue=(index*16)%256;
        for(let i=DISPLAY_LED_START;i<NUM_LEDS;i++){
            const h=((i*8+hue)%256)/256;
            const r=h<.333?1:(h<.666?1-(h-.333)*3:0);
            const g=h<.333?h*3:(h<.666?1:1-(h-.666)*3);
            const b=h<.333?0:(h<.666?(h-.333)*3:1);
            setLEDPreview(i,r*255,g*255,b*255);
        }
    }

    // ===== Status Polling =====
    async function updateStatus(){
        try{
            const res=await fetch('/api/status');
            const d=await res.json();
            const connected=d.connected;
            document.getElementById('conn-status').textContent=connected?'Connected':'Disconnected';
            document.getElementById('conn-status').className='stat-value '+(connected?'ok':'err');
            document.getElementById('conn-dot').className='status-dot'+(connected?' on':'');
            document.getElementById('mode-display').textContent=MODES[d.mode]||'Unknown';
            currentMode=d.mode;
            brightness=d.brightness!==undefined?d.brightness:brightness;
            document.getElementById('bright-display').textContent=brightness;
            document.getElementById('fps-display').textContent=d.framerate;
            document.getElementById('mode-select').value=String(d.mode);
            document.getElementById('content-index').value=d.index;
            document.getElementById('idx-display').textContent=d.index;
            document.getElementById('brightness').value=brightness;
            document.getElementById('brightness-value').textContent=brightness;
            document.getElementById('framerate').value=d.framerate;
            document.getElementById('framerate-value').textContent=d.framerate;
            if(d.mode===2){currentPattern=d.index;updatePatternActiveState()}
            updateLEDPreviewFromStatus(d.mode,d.index);
            if(d.sdCardPresent!==undefined){
                const st=document.getElementById('sd-status-text');
                st.textContent=d.sdCardPresent?'Present':'Not Present';
                st.style.color=d.sdCardPresent?'#22c55e':'#ef4444';
            }
        }catch(e){
            document.getElementById('conn-status').textContent='Error';
            document.getElementById('conn-status').className='stat-value err';
            document.getElementById('conn-dot').className='status-dot';
        }
    }

    function updatePatternActiveState(){
        document.querySelectorAll('.pbtn[data-pattern]').forEach(btn=>{
            const p=parseInt(btn.getAttribute('data-pattern'),10);
            btn.classList.toggle('active',p===currentPattern);
        });
    }

    // ===== Quick Actions =====
    async function quickAction(action){
        let mode=0,index=0;
        if(action==='play'){mode=3;index=0}
        else if(action==='stop'){mode=0;index=0}
        else if(action==='random'){mode=2;index=Math.floor(Math.random()*11)}
        await fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode,index})});
        showToast(action.toUpperCase()+' sent');
        updateStatus();
    }

    // ===== Mode & Controls =====
    async function changeMode(){
        const mode=document.getElementById('mode-select').value;
        const index=document.getElementById('content-index').value;
        currentMode=parseInt(mode);
        await fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:currentMode,index:parseInt(index)})});
        updateStatus();
    }
    async function changeContentIndex(){await changeMode()}

    function updateBrightness(value){
        document.getElementById('brightness-value').textContent=value;
        document.getElementById('bright-display').textContent=value;
        brightness=parseInt(value);
        fetch('/api/brightness',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({brightness:parseInt(value)})});
    }
    function updateFrameRate(value){
        document.getElementById('framerate-value').textContent=value;
        document.getElementById('fps-display').textContent=value;
        fetch('/api/framerate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({framerate:parseInt(value)})});
    }

    // ===== Patterns =====
    async function setPattern(type){
        currentPattern=type;updatePatternActiveState();
        const c1=document.getElementById('color1').value;
        const c2=document.getElementById('color2').value;
        const speed=document.getElementById('pattern-speed').value;
        await fetch('/api/pattern',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({index:type,type:type,color1:hexToRgb(c1),color2:hexToRgb(c2),speed:parseInt(speed)})});
        document.getElementById('content-index').value=type;
        document.getElementById('mode-select').value='2';
        await changeMode();
    }
    function updateSpeed(value){
        document.getElementById('speed-value').textContent=value;
        if(currentMode===2)setPattern(currentPattern);
    }
    function hexToRgb(hex){
        const r=/^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
        return r?{r:parseInt(r[1],16),g:parseInt(r[2],16),b:parseInt(r[3],16)}:{r:0,g:0,b:0};
    }

    // ===== Image Upload =====
    document.getElementById('image-upload').addEventListener('change',function(e){
        if(e.target.files.length>0){
            const file=e.target.files[0];
            const reader=new FileReader();
            reader.onload=function(event){
                const img=new Image();
                img.onload=function(){
                    originalImageAspectRatio=img.height/img.width;
                    const hI=document.getElementById('image-height');
                    const wI=document.getElementById('image-width');
                    const tH=DISPLAY_LEDS;
                    hI.value=tH;
                    if(document.getElementById('aspect-ratio-lock').checked){
                        wI.value=Math.min(100,Math.max(1,Math.round(tH/originalImageAspectRatio)));
                    }
                    // Show preview
                    const box=document.getElementById('img-preview-box');
                    box.innerHTML='<img src="'+event.target.result+'" style="max-width:100%;max-height:200px;image-rendering:pixelated">';
                    document.getElementById('img-dimensions').textContent=img.width+'x'+img.height+' (original)';
                };
                img.src=event.target.result;
            };
            reader.readAsDataURL(file);
        }
    });

    function updateImageDimensions(changedField){
        const lock=document.getElementById('aspect-ratio-lock').checked;
        const wI=document.getElementById('image-width');
        const hI=document.getElementById('image-height');
        if(lock){
            const h=DISPLAY_LEDS;hI.value=h;
            wI.value=Math.min(100,Math.max(1,Math.round(h/originalImageAspectRatio)));
        }
    }

    async function uploadImage(){
        const fI=document.getElementById('image-upload');
        if(!fI.files.length){showToast('Select an image first');return}
        try{
            const d=await convertImageToPOVFormat(fI.files[0]);
            const blob=new Blob([d.data],{type:'application/octet-stream'});
            const fd=new FormData();
            fd.append('file',blob,'image_'+d.width+'x'+d.height+'.rgb');
            const res=await fetch('/api/image',{method:'POST',body:fd});
            if(res.ok){
                showToast('Image uploaded! ('+d.width+'x'+d.height+')');
                document.getElementById('mode-select').value='1';
                await changeMode();
            }else{showToast('Upload failed: '+res.statusText)}
        }catch(e){showToast('Upload failed: '+e.message)}
    }

    async function convertImageToPOVFormat(file){
        return new Promise((resolve,reject)=>{
            const img=new Image();const reader=new FileReader();
            reader.onload=(e)=>{
                img.onload=()=>{
                    try{
                        const cv=document.createElement('canvas');const cx=cv.getContext('2d');
                        const originalImageAspectRatio=img.width/img.height;
                        const tH=DISPLAY_LEDS;
                        let tW=parseInt(document.getElementById('image-width').value)||32;
                        if (document.getElementById('aspect-ratio-lock').checked) {
                            const ar = (typeof originalImageAspectRatio === 'number' && originalImageAspectRatio > 0) ? originalImageAspectRatio : 1.0; // width/height
                            tW = Math.round(tH * ar);
                        }
                        tW=Math.min(100,Math.max(1,tW));
                        const fV=document.getElementById('flip-vertical').checked;
                        const fH=document.getElementById('flip-horizontal').checked;
                        if(tW<1||tW>100){reject(new Error('Invalid dimensions'));return}
                        cv.width=tW;cv.height=tH;cx.save();
                        if(fH){cx.translate(tW,0);cx.scale(-1,1)}
                        cx.imageSmoothingEnabled=false;cx.drawImage(img,0,0,tW,tH);cx.restore();
                        let iD=cx.getImageData(0,0,tW,tH);
                        if(fV){
                            const fl=cx.createImageData(tW,tH);
                            for(let y=0;y<tH;y++)for(let x=0;x<tW;x++){
                                const s=(y*tW+x)*4,d=((tH-1-y)*tW+x)*4;
                                fl.data[d]=iD.data[s];fl.data[d+1]=iD.data[s+1];
                                fl.data[d+2]=iD.data[s+2];fl.data[d+3]=iD.data[s+3];
                            }
                            iD=fl;
                        }
                        const px=iD.data;const rgb=new Uint8Array(tW*tH*3);let ri=0;
                        for(let i=0;i<px.length;i+=4){rgb[ri++]=px[i];rgb[ri++]=px[i+1];rgb[ri++]=px[i+2]}
                        resolve({width:tW,height:tH,data:rgb});
                    }catch(err){reject(err)}
                };
                img.onerror=reject;img.src=e.target.result;
            };
            reader.onerror=reject;reader.readAsDataURL(file);
        });
    }

    // ===== Procedural Art Generator =====
    function setGenType(type){
        genType=type;
        document.getElementById('btn-organic').style.border=type==='organic'?'1px solid #06b6d4':'1px solid #334155';
        document.getElementById('btn-geometric').style.border=type==='geometric'?'1px solid #a855f7':'1px solid #334155';
    }
    function rollColors(){colorSeed=Math.random();generateArt()}
    function generateArt(){
        const cv=document.getElementById('gen-canvas');const cx=cv.getContext('2d');
        const ledCount=DISPLAY_LEDS;
        const complexity=parseInt(document.getElementById('gen-complexity').value)||8;
        const tH=Math.min(Math.max(ledCount,1),64);
        let tW=Math.min(tH*4,400);
        // Ensure payload does not exceed firmware buffer (400*64*3 = 76800 bytes)
        while(tW>1&&tW*tH*3>76800)tW--;
        cv.width=tW;cv.height=tH;cv.style.display='block';
        cx.fillStyle='#000';cx.fillRect(0,0,tW,tH);
        const hueStart=colorSeed*360;
        if(genType==='organic'){
            for(let i=0;i<complexity;i++){
                const xOff=Math.random()*tW,freq=.01+Math.random()*.04;
                const amp=tH/4+Math.random()*tH/2;
                const hue=(hueStart+i*(360/complexity))%360;
                cx.beginPath();cx.strokeStyle='hsla('+hue+',90%,60%,0.7)';
                cx.lineWidth=2+Math.random()*8;
                for(let x=0;x<=tW;x++){
                    const y=tH/2+Math.sin(x*freq+xOff)*amp+Math.cos(x*freq*.5)*(amp*.3);
                    x===0?cx.moveTo(x,y):cx.lineTo(x,y);
                }
                cx.stroke();
            }
        }else{
            const cols=Math.max(4,Math.floor(complexity*2));
            const cellSize=tW/cols;const rows=Math.floor(tH/cellSize)||1;
            for(let x=0;x<cols;x++)for(let y=0;y<rows;y++){
                if(Math.random()>.4){
                    const hue=(hueStart+Math.random()*60)%360;
                    cx.fillStyle='hsla('+hue+',90%,50%,0.9)';
                    const px=x*cellSize,py=y*cellSize,size=cellSize*(.5+Math.random()*.4);
                    const shape=Math.floor(Math.random()*3);
                    if(shape===0)cx.fillRect(px+(cellSize-size)/2,py+(cellSize-size)/2,size,size);
                    else if(shape===1){cx.beginPath();cx.arc(px+cellSize/2,py+cellSize/2,size/2,0,Math.PI*2);cx.fill()}
                    else{cx.strokeStyle=cx.fillStyle;cx.lineWidth=3;cx.beginPath();cx.moveTo(px,py);cx.lineTo(px+cellSize,py+cellSize);cx.stroke()}
                }
            }
        }
        document.getElementById('btn-upload-gen').style.display='block';
        const box=document.getElementById('img-preview-box');
        box.innerHTML='<img src="'+cv.toDataURL()+'" style="max-width:100%;image-rendering:pixelated">';
        document.getElementById('img-dimensions').textContent=tW+'x'+tH+' (generated)';
        showToast('Pattern generated: '+genType);
    }

    async function uploadGenerated(){
        const cv=document.getElementById('gen-canvas');
        if(!cv.width)return;
        const tW=cv.width,tH=cv.height;
        const payloadSize=tW*tH*3;
        // Firmware max buffer: MAX_IMAGE_WIDTH(400) * MAX_IMAGE_HEIGHT(64) * 3 = 76800 bytes
        if(tW<1||tH<1||tW>400||tH>64||payloadSize>76800){
            showToast('Image too large to upload (max 400x64)');return;
        }
        const cx=cv.getContext('2d');
        const iD=cx.getImageData(0,0,tW,tH);const px=iD.data;
        const rgb=new Uint8Array(payloadSize);let ri=0;
        for(let i=0;i<px.length;i+=4){rgb[ri++]=px[i];rgb[ri++]=px[i+1];rgb[ri++]=px[i+2]}
        const blob=new Blob([rgb],{type:'application/octet-stream'});
        const fd=new FormData();fd.append('file',blob,'image_'+tW+'x'+tH+'.rgb');
        try{
            const res=await fetch('/api/image',{method:'POST',body:fd});
            if(res.ok){
                showToast('Generated image uploaded! ('+tW+'x'+tH+')');
                document.getElementById('mode-select').value='1';await changeMode();
            }else showToast('Upload failed');
        }catch(e){showToast('Upload error: '+e.message)}
    }

    // ===== Live Draw =====
    const canvas=document.getElementById('live-canvas');
    const ctx=canvas.getContext('2d');
    let isDrawing=false;

    canvas.addEventListener('mousedown',startDrawing);
    canvas.addEventListener('mousemove',draw);
    canvas.addEventListener('mouseup',stopDrawing);
    canvas.addEventListener('mouseleave',stopDrawing);
    canvas.addEventListener('touchstart',(e)=>{e.preventDefault();startDrawing(e.touches[0])});
    canvas.addEventListener('touchmove',(e)=>{e.preventDefault();draw(e.touches[0])});
    canvas.addEventListener('touchend',stopDrawing);

    function startDrawing(e){isDrawing=true;draw(e)}
    function stopDrawing(){isDrawing=false;sendLiveFrame()}
    function draw(e){
        if(!isDrawing)return;
        const rect=canvas.getBoundingClientRect();
        const scaleX=canvas.width/rect.width;
        const scaleY=canvas.height/rect.height;
        const x=(e.clientX-rect.left)*scaleX;
        const y=(e.clientY-rect.top)*scaleY;
        const brushSize=parseInt(document.getElementById('brush-size').value)||5;
        ctx.fillStyle=document.getElementById('draw-color').value;
        ctx.beginPath();ctx.arc(x,y,brushSize,0,Math.PI*2);ctx.fill();
    }
    function clearCanvas(){ctx.clearRect(0,0,canvas.width,canvas.height);sendLiveFrame()}
    function sendLiveFrame(){
        const cw=canvas.width,ch=canvas.height,midY=Math.floor(ch/2);
        const pixels=[];
        for(let i=DISPLAY_LED_START;i<NUM_LEDS;i++){
            const col=i-DISPLAY_LED_START;
            const x=Math.min(cw-1,Math.floor(col*(cw/NUM_LEDS)+(cw/NUM_LEDS)/2));
            const p=ctx.getImageData(x,midY,1,1).data;
            pixels.push({r:p[0],g:p[1],b:p[2]});
            setLEDPreview(i,p[0],p[1],p[2]);
        }
        fetch('/api/live',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pixels:pixels})});
    }

    // ===== SD Card =====
    async function refreshSDList(){
        try{
            const res=await fetch('/api/sd/list');const d=await res.json();
            const list=document.getElementById('sd-file-list');
            if(d.files&&d.files.length>0){
                list.innerHTML='';
                d.files.forEach(f=>{
                    const el=document.createElement('div');el.className='sd-file';
                    const nameSpan=document.createElement('span');nameSpan.className='fname';nameSpan.textContent=f;
                    const actions=document.createElement('div');actions.className='actions';
                    const loadBtn=document.createElement('button');loadBtn.className='btn btn-green btn-sm btn-icon';loadBtn.textContent='Load';loadBtn.addEventListener('click',()=>loadSDImage(f));
                    const delBtn=document.createElement('button');delBtn.className='btn btn-red btn-sm btn-icon';delBtn.textContent='Delete';delBtn.addEventListener('click',()=>deleteSDImage(f));
                    actions.appendChild(loadBtn);actions.appendChild(delBtn);
                    el.appendChild(nameSpan);el.appendChild(actions);
                    list.appendChild(el);
                });
            }else{list.innerHTML='<div style="text-align:center;color:#475569;font-size:12px;padding:12px">No images on SD card</div>'}
        }catch(e){document.getElementById('sd-file-list').innerHTML='<div style="text-align:center;color:#ef4444;font-size:12px;padding:12px">Error loading list</div>'}
    }
    async function loadSDImage(f){
        try{
            const res=await fetch('/api/sd/load',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({filename:f})});
            if(res.ok){showToast('Loaded: '+f);document.getElementById('mode-select').value='1';await changeMode();updateStatus()}
            else showToast('Load failed');
        }catch(e){showToast('Error: '+e.message)}
    }
    async function deleteSDImage(f){
        if(!confirm('Delete '+f+'?'))return;
        try{
            const res=await fetch('/api/sd/delete',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({filename:f})});
            if(res.ok){showToast('Deleted: '+f);refreshSDList()}else showToast('Delete failed');
        }catch(e){showToast('Error: '+e.message)}
    }
    async function updateSDStatus(){
        try{
            const res=await fetch('/api/sd/info');const d=await res.json();
            const st=document.getElementById('sd-status-text');
            const info=document.getElementById('sd-info');
            if(d.present){
                st.textContent='Present';st.style.color='#22c55e';
                info.innerHTML='Total: '+(d.totalSpace/(1024*1024)).toFixed(1)+' MB | Free: '+(d.freeSpace/(1024*1024)).toFixed(1)+' MB';
            }else{st.textContent='Not Present';st.style.color='#ef4444';info.innerHTML=''}
        }catch(e){document.getElementById('sd-status-text').textContent='Error';document.getElementById('sd-status-text').style.color='#ef4444'}
    }

    // ===== Multi-Poi Sync =====
    async function updateSyncStatus(){
        try{
            const res=await fetch('/api/multipoi/status');const d=await res.json();
            document.getElementById('sync-local-name').textContent=d.localName||'--';
            currentSyncMode=d.syncMode||'mirror';
            document.getElementById('sync-mode-display').textContent=currentSyncMode==='mirror'?'Mirror':'Independent';
            const bM=document.getElementById('btn-mirror'),bI=document.getElementById('btn-independent');
            if(currentSyncMode==='mirror'){bM.style.background='#06b6d4';bM.className='btn';bI.style.background='';bI.className='btn btn-slate'}
            else{bI.style.background='#a855f7';bI.className='btn';bM.style.background='';bM.className='btn btn-slate'}
            document.getElementById('independent-controls').style.display=currentSyncMode==='independent'?'block':'none';
            syncPeers=d.peers||[];
            const pairedCount=syncPeers.filter(p=>p.state===3&&p.online).length;
            document.getElementById('sync-paired-status').textContent=pairedCount>0?pairedCount+' peer(s) online':'No peers';
            const peerList=document.getElementById('sync-peer-list');
            if(syncPeers.length===0){peerList.innerHTML='<div style="text-align:center;color:#475569;font-size:12px;padding:12px">No paired poi found. Tap Pair to discover.</div>'}
            else{
                let html='';
                syncPeers.forEach((peer,i)=>{
                    const stN=['None','Discovering','Requesting','Paired'];
                    const pc=peer.online?'#22c55e':'#ef4444';
                    const st=peer.online?'Online':'Offline';
                    html+='<div class="peer-item" style="--pc:'+pc+'"><div><span class="name">'+(peer.name||'Unknown')+'</span><br><span class="meta">'+st+' | '+stN[peer.state]+'</span></div>';
                    html+='<button class="btn btn-red btn-sm btn-icon" onclick="unpairSingle('+i+')" style="width:auto">Unpair</button></div>';
                });
                peerList.innerHTML=html;
            }
            if(currentSyncMode==='independent')renderPeerControls();
        }catch(e){console.error('Sync error:',e)}
    }

    function renderPeerControls(){
        const c=document.getElementById('peer-control-panels');
        const online=syncPeers.filter(p=>p.state===3&&p.online);
        if(online.length===0){c.innerHTML='<div style="text-align:center;color:#475569;font-size:12px">No online peers</div>';return}
        let html='';
        syncPeers.forEach((peer,i)=>{
            if(peer.state!==3||!peer.online)return;
            html+='<div class="peer-panel"><div class="peer-title">'+(peer.name||'Peer '+i)+'</div>';
            html+='<div class="ctrl"><label style="font-size:12px">Mode</label><select id="peer-mode-'+i+'" style="width:100%;padding:8px;background:#0f172a;border:1px solid #334155;border-radius:8px;color:#e2e8f0" onchange="sendPeerMode('+i+')">';
            html+='<option value="0">Idle</option><option value="1">Image</option><option value="2">Pattern</option><option value="3">Sequence</option></select></div>';
            html+='<div class="ctrl"><label style="font-size:12px">Quick Pattern</label><div class="grid3">';
            [{t:0,n:'Rainbow'},{t:4,n:'Fire'},{t:5,n:'Comet'},{t:6,n:'Breathe'},{t:10,n:'Plasma'},{t:3,n:'Sparkle'},{t:16,n:'Split'},{t:17,n:'Chase'},{t:7,n:'Strobe'}].forEach(p=>{
                html+='<button class="btn btn-sm" style="background:#a855f7;width:auto" onclick="sendPeerPattern('+i+','+p.t+')">'+p.n+'</button>';
            });
            html+='</div></div>';
            html+='<div class="ctrl"><label style="font-size:12px">Brightness <span id="peer-bv-'+i+'">'+(peer.brightness||128)+'</span></label>';
            html+='<input type="range" min="0" max="255" value="'+(peer.brightness||128)+'" oninput="sendPeerBrightness('+i+',this.value);document.getElementById(\'peer-bv-'+i+'\').textContent=this.value"></div>';
            html+='</div>';
        });
        c.innerHTML=html;
    }

    async function setSyncMode(mode){
        try{await fetch('/api/multipoi/syncmode',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:mode})});currentSyncMode=mode;updateSyncStatus()}catch(e){console.error(e)}
    }
    async function pairPoi(){try{await fetch('/api/multipoi/pair',{method:'POST'});showToast('Pairing...');setTimeout(updateSyncStatus,2000)}catch(e){console.error(e)}}
    async function unpairPoi(){try{await fetch('/api/multipoi/unpair',{method:'POST'});updateSyncStatus()}catch(e){console.error(e)}}
    async function unpairSingle(i){try{await fetch('/api/multipoi/unpair',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({index:i})});updateSyncStatus()}catch(e){console.error(e)}}
    async function sendPeerMode(i){
        const mode=parseInt(document.getElementById('peer-mode-'+i).value);
        try{await fetch('/api/multipoi/peercmd',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({peer:i,cmd:'mode',mode:mode,index:0})})}catch(e){console.error(e)}
    }
    async function sendPeerPattern(i,type){
        const c1=hexToRgb(document.getElementById('color1').value),c2=hexToRgb(document.getElementById('color2').value);
        const speed=parseInt(document.getElementById('pattern-speed').value);
        try{await fetch('/api/multipoi/peercmd',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({peer:i,cmd:'pattern',index:0,type:type,color1:c1,color2:c2,speed:speed})})}catch(e){console.error(e)}
    }
    async function sendPeerBrightness(i,b){
        try{await fetch('/api/multipoi/peercmd',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({peer:i,cmd:'brightness',brightness:parseInt(b)})})}catch(e){console.error(e)}
    }

    // ===== Settings =====
    async function loadConfig(){
        try{
            const res=await fetch('/api/device/config');const d=await res.json();
            document.getElementById('cfg-device-name').value=d.deviceName||'';
            document.getElementById('cfg-sync-group').value=d.syncGroup||'';
            document.getElementById('cfg-auto-sync').checked=d.autoSync||false;
            document.getElementById('cfg-device-id').textContent='Device ID: '+(d.deviceId||'--');
        }catch(e){}
    }
    async function saveConfig(){
        const body={
            deviceName:document.getElementById('cfg-device-name').value,
            syncGroup:document.getElementById('cfg-sync-group').value,
            autoSync:document.getElementById('cfg-auto-sync').checked
        };
        try{
            const res=await fetch('/api/device/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
            document.getElementById('cfg-status').textContent=res.ok?'Configuration saved!':'Save failed';
            document.getElementById('cfg-status').style.color=res.ok?'#22c55e':'#ef4444';
            showToast(res.ok?'Config saved':'Save failed');
        }catch(e){document.getElementById('cfg-status').textContent='Error: '+e.message;document.getElementById('cfg-status').style.color='#ef4444'}
    }

    // ===== PWA =====
    let deferredPrompt;
    window.addEventListener('beforeinstallprompt',(e)=>{
        e.preventDefault();deferredPrompt=e;
        const btn=document.getElementById('installButton');btn.style.display='block';
        btn.addEventListener('click',async()=>{
            if(deferredPrompt){deferredPrompt.prompt();const{outcome}=await deferredPrompt.userChoice;deferredPrompt=null;btn.style.display='none'}
        });
    });
    if('serviceWorker' in navigator){navigator.serviceWorker.register('/sw.js').catch(()=>{})}

    // ===== Initialize =====
    initLEDPreview();
    updateStatus();
    setInterval(updateStatus,2000);
    updateSDStatus();refreshSDList();
    setInterval(updateSDStatus,5000);
    setInterval(refreshSDList,10000);
    updateSyncStatus();
    setInterval(updateSyncStatus,3000);
    loadConfig();
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
  static size_t bufferIndex = 0;
  static bool uploadRejected = false;
  static uint16_t imageWidth = 32;
  static uint16_t imageHeight = 32;
  static const size_t MAX_UPLOAD_BYTES = (size_t)MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3;
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    bufferIndex = 0;
    uploadRejected = false;
    
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
      // Reject uploads that declare dimensions exceeding firmware limits
      if (imageWidth < 1 || imageWidth > MAX_IMAGE_WIDTH ||
          imageHeight < 1 || imageHeight > MAX_IMAGE_HEIGHT) {
        Serial.printf("Upload rejected: declared dimensions %dx%d exceed limits %dx%d\n",
                      imageWidth, imageHeight, (int)MAX_IMAGE_WIDTH, (int)MAX_IMAGE_HEIGHT);
        uploadRejected = true;
        return;
      }
    }
    // Reject if declared payload would overflow buffer
    uint32_t declaredBytes = (uint32_t)imageWidth * imageHeight * 3;
    if (declaredBytes > MAX_UPLOAD_BYTES) {
      Serial.printf("Upload rejected: declared payload %u bytes exceeds buffer %u bytes\n",
                    declaredBytes, (unsigned)MAX_UPLOAD_BYTES);
      uploadRejected = true;
      return;
    }
    Serial.printf("Parsed dimensions: %dx%d\n", imageWidth, imageHeight);
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadRejected) return;
    // Reject if received data would overflow buffer
    if (bufferIndex + upload.currentSize > MAX_UPLOAD_BYTES) {
      Serial.printf("Upload rejected: received data exceeds buffer (%u bytes)\n",
                    (unsigned)(bufferIndex + upload.currentSize));
      uploadRejected = true;
      return;
    }
    
    memcpy(imageBuffer + bufferIndex, upload.buf, upload.currentSize);
    bufferIndex += upload.currentSize;
    
    Serial.printf("Upload Write: %d bytes (total: %u)\n", upload.currentSize, (unsigned)bufferIndex);
    
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadRejected) {
      server.send(413, "application/json", "{\"error\":\"Image dimensions exceed firmware limits\"}");
      return;
    }
    Serial.printf("Upload End: %u bytes\n", (unsigned)bufferIndex);
    
    // Use dimensions parsed from filename
    // Verify against actual data size
    uint32_t expectedSize = (uint32_t)imageWidth * imageHeight * 3;
    if (bufferIndex < expectedSize) {
      // Adjust height to match actual data if needed
      imageHeight = bufferIndex / (imageWidth * 3);
      if (imageHeight < 1) imageHeight = 1;
    }
    
    uint32_t actualSize = (uint32_t)imageWidth * imageHeight * 3;
    if (actualSize > bufferIndex) actualSize = bufferIndex;
    
    Serial.printf("Detected image: %dx%d (%u bytes)\n", imageWidth, imageHeight, (unsigned)actualSize);
    
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
    for (size_t i = 0; i < actualSize && i < bufferIndex; i++) {
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
  "background_color": "#020617",
  "theme_color": "#0f172a",
  "icons": [
    {
      "src": "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Crect width='100' height='100' rx='20' fill='%230f172a'/%3E%3Ccircle cx='50' cy='50' r='30' fill='url(%23g)'/%3E%3Cdefs%3E%3ClinearGradient id='g' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop offset='0' stop-color='%2306b6d4'/%3E%3Cstop offset='1' stop-color='%23a855f7'/%3E%3C/linearGradient%3E%3C/defs%3E%3Ctext x='50' y='58' font-size='28' font-weight='bold' text-anchor='middle' fill='white'%3EN%3C/text%3E%3C/svg%3E",
      "sizes": "192x192",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    },
    {
      "src": "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Crect width='100' height='100' rx='20' fill='%230f172a'/%3E%3Ccircle cx='50' cy='50' r='30' fill='url(%23g)'/%3E%3Cdefs%3E%3ClinearGradient id='g' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop offset='0' stop-color='%2306b6d4'/%3E%3Cstop offset='1' stop-color='%23a855f7'/%3E%3C/linearGradient%3E%3C/defs%3E%3Ctext x='50' y='58' font-size='28' font-weight='bold' text-anchor='middle' fill='white'%3EN%3C/text%3E%3C/svg%3E",
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
  
  // Load saved WiFi STA (client) credentials for connecting to existing network
  staSsid = preferences.getString("sta_ssid", "");
  staPassword = preferences.getString("sta_password", "");
  
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

// Save WiFi STA credentials to Preferences and optionally start connection
void saveWifiStaConfig(const String& ssid, const String& pass) {
  staSsid = ssid;
  staPassword = pass;
  preferences.begin("povpoi", false);
  preferences.putString("sta_ssid", staSsid);
  preferences.putString("sta_password", staPassword);
  preferences.end();
}

// GET /api/wifi/status - WiFi AP + STA status for settings UI
void handleWifiStatus() {
  bool staConnected = (WiFi.status() == WL_CONNECTED);
  String staIp = staConnected ? WiFi.localIP().toString() : "";
  String apIp = WiFi.softAPIP().toString();
  
  String json = "{";
  json += "\"apIp\":\"" + apIp + "\",";
  json += "\"apSsid\":\"" + String(ssid) + "\",";
  json += "\"staConnected\":" + String(staConnected ? "true" : "false") + ",";
  json += "\"staIp\":\"" + staIp + "\",";
  json += "\"savedSsid\":\"" + staSsid + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// POST /api/wifi/connect - Save SSID/password and connect to network (body: {"ssid":"...", "password":"..."})
void handleWifiConnect() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  int ssidIdx = body.indexOf("\"ssid\":");
  if (ssidIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Missing ssid\"}");
    return;
  }
  
  int ssidStart = body.indexOf("\"", ssidIdx + 7) + 1;
  int ssidEnd = body.indexOf("\"", ssidStart);
  String newSsid = body.substring(ssidStart, ssidEnd);
  newSsid.trim();
  
  if (newSsid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID cannot be empty\"}");
    return;
  }
  
  String newPass = "";
  int passIdx = body.indexOf("\"password\":");
  if (passIdx != -1) {
    int passStart = body.indexOf("\"", passIdx + 11) + 1;
    int passEnd = body.indexOf("\"", passStart);
    newPass = body.substring(passStart, passEnd);
  }
  
  saveWifiStaConfig(newSsid, newPass);
  WiFi.begin(newSsid.c_str(), newPass.c_str());
  
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"message\":\"Connecting to network. Check /api/wifi/status for result.\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// POST /api/wifi/disconnect - Disconnect from STA and clear saved credentials
void handleWifiDisconnect() {
  WiFi.disconnect(true);
  staSsid = "";
  staPassword = "";
  preferences.begin("povpoi", false);
  preferences.remove("sta_ssid");
  preferences.remove("sta_password");
  preferences.end();
  
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"message\":\"Disconnected and credentials cleared\"";
  json += "}";
  
  server.send(200, "application/json", json);
}
