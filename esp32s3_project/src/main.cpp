/**
 * ESP32-S3 Project Template
 * Board: ESP32-S3-DevKitC-1 (8MB Flash, 8MB PSRAM)
 */

#include <Arduino.h>
#include <FastLED.h>

// LED Configuration
#define LED_PIN     48      // Built-in RGB LED on most ESP32-S3 DevKits
#define NUM_LEDS    1
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32-S3 Starting...");
    Serial.printf("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM Size: %d bytes\n", ESP.getPsramSize());
    
    // Initialize built-in LED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    
    Serial.println("Setup complete!");
}

void loop() {
    // Cycle through colors on built-in LED
    static uint8_t hue = 0;
    
    leds[0] = CHSV(hue++, 255, 255);
    FastLED.show();
    
    delay(20);
}
