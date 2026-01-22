#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

class LEDDriver {
public:
    LEDDriver();
    
    // Initialize the LED strip
    void begin();
    
    // Set a single LED color
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    void setPixel(uint16_t index, CRGB color);
    
    // Display the current frame
    void show();
    
    // Clear all LEDs
    void clear();
    
    // Set brightness (0-255)
    void setBrightness(uint8_t brightness);
    
    // Get LED count
    uint16_t getNumLEDs() const { return NUM_LEDS; }
    
    // Get a pixel color
    void getPixel(uint16_t index, uint8_t& r, uint8_t& g, uint8_t& b);
    
    // Direct access to LED array
    CRGB* getLEDs() { return leds; }

private:
    CRGB leds[NUM_LEDS];
    uint8_t brightness;
};

#endif // LED_DRIVER_H
