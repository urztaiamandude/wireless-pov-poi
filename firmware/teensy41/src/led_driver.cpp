#include "led_driver.h"

LEDDriver::LEDDriver() : brightness(LED_BRIGHTNESS) {
}

void LEDDriver::begin() {
    // Initialize FastLED for APA102 LEDs
    FastLED.addLeds<APA102, LED_DATA_PIN, LED_CLOCK_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    clear();
    show();
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("LED Driver initialized");
    DEBUG_SERIAL.print("Number of LEDs: ");
    DEBUG_SERIAL.println(NUM_LEDS);
    #endif
}

void LEDDriver::setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < NUM_LEDS) {
        leds[index] = CRGB(r, g, b);
    }
}

void LEDDriver::setPixel(uint16_t index, CRGB color) {
    if (index < NUM_LEDS) {
        leds[index] = color;
    }
}

void LEDDriver::show() {
    FastLED.show();
}

void LEDDriver::clear() {
    FastLED.clear();
}

void LEDDriver::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    FastLED.setBrightness(brightness);
}

void LEDDriver::getPixel(uint16_t index, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (index < NUM_LEDS) {
        r = leds[index].r;
        g = leds[index].g;
        b = leds[index].b;
    } else {
        r = g = b = 0;
    }
}
