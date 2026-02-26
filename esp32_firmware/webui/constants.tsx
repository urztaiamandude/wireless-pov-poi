
import { ArduinoSketch } from './types';

// ⚠️ NOTE: These sketch templates are simplified examples for educational purposes.
// They demonstrate the basic architecture but DO NOT match the production firmware exactly.
// For actual deployment, use the firmware files from:
// - esp32_firmware/esp32_firmware.ino (ESP32 production code)
// - teensy_firmware/teensy_firmware.ino (Teensy production code)
// These templates are useful for understanding the communication flow and structure.

export const getESP32Sketch = (ledCount: number): ArduinoSketch => ({
  title: "ESP32-S3 Gateway (Simplified Example)",
  description: "Educational example showing basic WiFi AP and UART bridge architecture. See esp32_firmware/esp32_firmware.ino for production code.",
  libraries: ["WiFi", "WebServer", "SPIFFS", "ArduinoJson"],
  code: `
// ==========================================
// SIMPLIFIED EXAMPLE - NOT PRODUCTION CODE
// For production firmware, see: esp32_firmware/esp32_firmware.ino
// ==========================================

// This example demonstrates the basic architecture of the ESP32 gateway.
// The actual production firmware includes additional features:
// - BLE support via Nordic UART Service
// - Peer discovery and synchronization
// - SD card management endpoints
// - OTA update capability (pending implementation)
// - Advanced error handling and recovery

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "POV-POI-WiFi";
const char* password = "povpoi123";

// Serial to Teensy (TX=GPIO43, RX=GPIO44 for ESP32-S3)
#define TEENSY_SERIAL Serial2
#define SERIAL_BAUD 115200

WebServer server(80);

// Basic command structure for Teensy communication
void sendTeensyCommand(uint8_t cmd, const uint8_t* data, size_t len) {
  TEENSY_SERIAL.write(0xFF);  // Start marker
  TEENSY_SERIAL.write(cmd);   // Command byte
  TEENSY_SERIAL.write(len);   // Data length
  TEENSY_SERIAL.write(data, len);  // Data payload
  TEENSY_SERIAL.write(0xFE);  // End marker
}

void relayToTeensy(String json) {
  TEENSY_SERIAL.println(json);
}

void setup() {
  Serial.begin(115200);
  // ESP32-S3 uses GPIO 43 (TX) and GPIO 44 (RX) for Serial2
  TEENSY_SERIAL.begin(SERIAL_BAUD, SERIAL_8N1, 44, 43);

  // Initialize filesystem
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }

  // Create WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // API Endpoints (simplified examples)
  server.on("/api/status", HTTP_GET, [](){
    server.send(200, "application/json", 
      "{\\"mode\\":1,\\"brightness\\":128,\\"connected\\":true}");
  });

  server.on("/api/mode", HTTP_POST, [](){
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      // Parse JSON and send to Teensy
      relayToTeensy(body);
      server.send(200, "application/json", "{\\"status\\":\\"ok\\"}");
    } else {
      server.send(400, "application/json", "{\\"error\\":\\"No data\\"}");
    }
  });

  server.on("/api/brightness", HTTP_POST, [](){
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      relayToTeensy(body);
      server.send(200, "application/json", "{\\"status\\":\\"ok\\"}");
    }
  });

  // Serve static web UI files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
`
});

export const getTeensySketch = (ledCount: number): ArduinoSketch => ({
  title: "Teensy 4.1 POV Engine (Simplified Example)",
  description: "Educational example showing basic LED control and serial communication. See teensy_firmware/teensy_firmware.ino for production code.",
  libraries: ["FastLED", "SdFat", "ArduinoJson"],
  code: `
// ==========================================
// SIMPLIFIED EXAMPLE - NOT PRODUCTION CODE
// For production firmware, see: teensy_firmware/teensy_firmware.ino
// ==========================================

// This example demonstrates the basic architecture of the Teensy POV engine.
// The actual production firmware includes additional features:
// - PSRAM support for storing up to 50 images
// - 18 built-in animated patterns (including music-reactive)
// - Sequence playback with duration control
// - SD card image loading and management
// - Binary protocol for efficient data transfer
// - Hardware timers for precise POV timing

#include <Arduino.h>
#include <FastLED.h>
#include <SdFat.h>
#include <ArduinoJson.h>

// LED Configuration
#define NUM_LEDS ${ledCount}
#define DATA_PIN 11
#define CLOCK_PIN 13
#define SERIAL_BAUD 115200
#define DEFAULT_BRIGHTNESS 128

// Note: In production, DISPLAY_LED_START=0 and all 32 LEDs are display pixels
// with hardware level shifter. This example uses simplified indexing.

constexpr uint8_t kPixelBytes = 3;

CRGB leds[NUM_LEDS];
SdFs sd;

uint8_t currentBrightness = DEFAULT_BRIGHTNESS;
uint8_t currentMode = 0;  // 0=Idle, 1=Image, 2=Pattern

// Simple pattern example
void displayPattern(uint8_t patternIndex) {
  static uint8_t hue = 0;
  
  // Example: Rainbow pattern
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + (i * 10), 255, 255);
  }
  hue++;
  FastLED.show();
}

void clearLEDs() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void setup() {
  // Serial1 for communication with ESP32 (TX1=Pin 1, RX1=Pin 0)
  Serial1.begin(SERIAL_BAUD);
  
  // Debug serial
  Serial.begin(115200);

  // Initialize FastLED with APA102
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(currentBrightness);
  clearLEDs();

  // Initialize SD card (optional)
  if (!sd.begin(SdioConfig(FIFO_SDIO))) {
    Serial.println("SD card initialization failed");
  } else {
    Serial.println("SD card ready");
  }

  Serial.println("Teensy POV Engine Ready");
}

void loop() {
  // Check for commands from ESP32 via Serial1
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\\n');
    StaticJsonDocument<256> doc;
    
    if (deserializeJson(doc, line) == DeserializationError::Ok) {
      String cmd = doc["cmd"] | "";
      
      if (cmd == "mode") {
        currentMode = doc["val"] | 0;
        Serial.print("Mode changed to: ");
        Serial.println(currentMode);
      } 
      else if (cmd == "brightness") {
        currentBrightness = constrain(doc["val"] | 128, 0, 255);
        FastLED.setBrightness(currentBrightness);
        Serial.print("Brightness set to: ");
        Serial.println(currentBrightness);
      }
      else if (cmd == "pattern") {
        currentMode = 2;  // Pattern mode
        Serial.println("Pattern mode activated");
      }
    }
  }

  // Main display logic
  if (currentMode == 0) {
    // Idle - LEDs off
    clearLEDs();
    delay(100);
  } 
  else if (currentMode == 2) {
    // Pattern mode - display animated pattern
    displayPattern(0);
    delay(50);
  }
}

// For the complete production implementation with image storage,
// sequence playback, SD card support, and all 18 patterns,
// see: teensy_firmware/teensy_firmware.ino
`
});
