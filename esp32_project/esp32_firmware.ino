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

// WiFi Configuration
const char* ssid = "POV-POI-WiFi";
const char* password = "povpoi123";

// Serial Configuration
#define TEENSY_SERIAL Serial2
#define SERIAL_BAUD 115200

// Web Server
WebServer server(80);

// System state
struct SystemState {
    uint8_t currentMode;
    uint8_t currentIndex;
    uint8_t brightness;
    uint8_t frameRate;
};

SystemState state;

void setup() {
    Serial.begin(115200);
    TEENSY_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, 44, 43);  // RX=GPIO44 (U0RXD), TX=GPIO43 (U0TXD)
    setupWiFi();
    setupWebServer();
}

void loop() {
    server.handleClient();
    checkTeensyConnection();
}

// ...rest of the code from esp32_firmware.ino should be copied here as needed...
