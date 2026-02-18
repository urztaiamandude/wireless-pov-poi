
import { ArduinoSketch } from './types';

export const getESP32Sketch = (ledCount: number): ArduinoSketch => ({
  title: "ESP32-S3 Fleet Gateway",
  description: "Supports UDP-based synchronization for mirroring multiple POV devices.",
  libraries: ["WiFi", "ESP Async WebServer", "AsyncUDP", "ArduinoJson", "LittleFS"],
  code: `
// ==========================================
// FILE: platformio.ini
// ==========================================
/*
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
    ottowinter/ESPAsyncWebServer-esphome @ ^3.1.0
    bblanchon/ArduinoJson @ ^6.21.3
build_flags = -D CORE_DEBUG_LEVEL=5
board_build.filesystem = littlefs
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

AsyncWebServer server(80);
AsyncUDP udp;
const int SYNC_PORT = 4210;

bool isSyncLeader = false;

void broadcastSync(String cmd, String val = "") {
  StaticJsonDocument<256> doc;
  doc["cmd"] = cmd;
  doc["val"] = val;
  char buffer[256];
  serializeJson(doc, buffer);
  udp.broadcastTo(buffer, SYNC_PORT);
}

void relayToTeensy(String json) {
  Serial2.println(json);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 18, 17);

  if(!LittleFS.begin(true)) Serial.println("LittleFS Error");

  WiFi.softAP("POV-POI-WiFi", "povpoi123");

  // Listen for Sync Packets from other Leader
  if (udp.listen(SYNC_PORT)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      if (!isSyncLeader) { // Only followers relay broadcasted packets
        Serial2.write(packet.data(), packet.length());
        Serial2.println();
      }
    });
  }

  server.on("/api/mode", HTTP_GET, [](AsyncWebServerRequest *request){
    isSyncLeader = (request->arg("leader") == "true");
    request->send(200, "text/plain", isSyncLeader ? "Leader Mode" : "Follower Mode");
  });

  server.on("/api/brightness", HTTP_POST, [](AsyncWebServerRequest *request){
    String val = request->arg("val");
    StaticJsonDocument<256> doc;
    doc["cmd"] = "brightness";
    doc["val"] = val;
    String out;
    serializeJson(doc, out);
    relayToTeensy(out);
    if (isSyncLeader) broadcastSync("brightness", val);
    request->send(200, "text/plain", "OK");
  });

  server.on("/api/sd/load", HTTP_POST, [](AsyncWebServerRequest *request){
    String file = request->arg("file");
    StaticJsonDocument<256> doc;
    doc["cmd"] = "load";
    doc["val"] = file;
    String out;
    serializeJson(doc, out);
    relayToTeensy(out);
    if (isSyncLeader) broadcastSync("load", file);
    request->send(200, "text/plain", "OK");
  });

  // Standard static serving and upload handlers...
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();
}

void loop() {}
`
});

export const getTeensySketch = (ledCount: number): ArduinoSketch => ({
  title: "Teensy 4.1 Synchronized Engine",
  description: "MicroSD-backed POV renderer with APA102 output and serial command control.",
  libraries: ["FastLED", "SdFat", "ArduinoJson"],
  code: `
#include <Arduino.h>
#include <FastLED.h>
#include <SdFat.h>
#include <ArduinoJson.h>

#define NUM_LEDS ${ledCount}
#define DATA_PIN 11
#define CLOCK_PIN 13
#define SERIAL_BAUD 115200
#define DEFAULT_BRIGHTNESS 128
#define SLICE_DELAY_MS 8

constexpr uint8_t kPixelBytes = 3;
constexpr uint16_t kBmpSignature = 0x4D42;
constexpr uint8_t kSerialTerminator = 10;

CRGB leds[NUM_LEDS];
SdFs sd;
FsFile povFile;

enum class PlayMode : uint8_t { kIdle, kPlayback, kRandom };

PlayMode mode = PlayMode::kIdle;
uint16_t sliceCount = 0;
uint16_t currentSlice = 0;
uint32_t lastSliceMs = 0;
uint8_t sliceBuffer[NUM_LEDS * kPixelBytes];

struct PovHeader {
  char magic[4];
  uint16_t ledCount;
  uint16_t sliceCount;
};

void sendStatus(const char *status, const char *detail = nullptr) {
  StaticJsonDocument<128> doc;
  doc["status"] = status;
  if (detail) {
    doc["detail"] = detail;
  }
  serializeJson(doc, Serial1);
  Serial1.println();
}

void clearLEDs() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial1.begin(SERIAL_BAUD);

  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  clearLEDs();

  if (!sd.begin(SdioConfig(FIFO_SDIO))) {
    sendStatus("sd_error");
  }
}

void loop() {
  if (Serial1.available()) {
    String line = Serial1.readStringUntil(kSerialTerminator);
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, line) == DeserializationError::Ok) {
      String cmd = doc["cmd"] | "";
      String val = doc["val"] | "";

      if (cmd == "play") {
        mode = povFile ? PlayMode::kPlayback : PlayMode::kRandom;
        currentSlice = 0;
      } else if (cmd == "stop") {
        mode = PlayMode::kIdle;
        clearLEDs();
      } else if (cmd == "brightness") {
        int brightness = constrain(val.toInt(), 0, 255);
        FastLED.setBrightness(brightness);
        FastLED.show();
      } else if (cmd == "load") {
        sendStatus("loaded", val.c_str());
      } else if (cmd == "random") {
        mode = PlayMode::kRandom;
      }
    }
  }

  uint32_t now = millis();
  if ((now - lastSliceMs) < SLICE_DELAY_MS) return;
  lastSliceMs = now;

  if (mode == PlayMode::kPlayback && sliceCount > 0) {
    FastLED.show();
    currentSlice = (currentSlice + 1) % sliceCount;
  } else if (mode == PlayMode::kRandom) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(random8(), 255, random8(128, 255));
    }
    FastLED.show();
  }
}
`
});
