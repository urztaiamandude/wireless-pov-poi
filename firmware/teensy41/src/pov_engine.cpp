#include "pov_engine.h"
#include "sd_storage.h"
#include <new>

POVEngine::POVEngine(LEDDriver& ledDriver) 
    : leds(ledDriver), 
      imageBuffer(nullptr), 
      imageWidth(0), 
      imageHeight(0),
      currentAngle(0),
      rotationSpeed(0.0),
      displayMode(0),
      modeIndex(0),
      enabled(false),
      lastUpdateTime(0),
      lastFrameTime(0),
      frameDelay(16),  // Default ~60 FPS
      patternTime(0) {
    
    // Initialize pattern storage
    for (int i = 0; i < 5; i++) {
        patterns[i].active = false;
        patterns[i].type = 0;
        patterns[i].speed = 50;
        patterns[i].r1 = 255;
        patterns[i].g1 = 0;
        patterns[i].b1 = 0;
        patterns[i].r2 = 0;
        patterns[i].g2 = 0;
        patterns[i].b2 = 255;
    }
}

void POVEngine::begin() {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("POV Engine initialized");
    #endif
}

void POVEngine::update() {
    // Check frame delay timing
    unsigned long currentTime = millis();
    if (currentTime - lastFrameTime < frameDelay) {
        return;  // Not time for next frame yet
    }
    lastFrameTime = currentTime;
    
    if (!enabled) {
        return;
    }
    
    // Update current angle based on rotation speed and time
    // This is a simulation for now - in real hardware, this would be
    // driven by accelerometer
    if (lastUpdateTime > 0 && rotationSpeed > 0) {
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0; // seconds
        float degreesPerSecond = rotationSpeed * 6.0; // RPM to degrees/sec
        float angleDelta = degreesPerSecond * deltaTime;
        currentAngle = (currentAngle + (uint16_t)angleDelta) % 360;
    }
    lastUpdateTime = currentTime;
    
    // Render based on display mode
    switch (displayMode) {
        case 0:  // Idle mode - clear display
            leds.clear();
            leds.show();
            break;
            
        case 1:  // Image mode
            if (imageBuffer) {
                uint16_t column = getColumnForAngle(currentAngle);
                renderColumn(column);
            }
            break;
            
        case 2:  // Pattern mode
            renderPattern();
            break;
            
        case 3:  // Sequence mode
            // NOTE: Sequence support is implemented in the Arduino IDE firmware 
            // (teensy_firmware.ino), but not yet ported to this PlatformIO version.
            // Implementation needed: port displaySequence() function and related
            // sequence state tracking code (currentSequenceItem, sequenceStartTime,
            // item/duration arrays, loop support). See teensy_firmware.ino for reference.
            break;
            
        case 4:  // Live mode - handled externally
            break;
            
        default:
            leds.clear();
            leds.show();
            break;
    }
}

void POVEngine::loadImageData(const uint8_t* data, size_t width, size_t height) {
    // Free existing buffer if any
    if (imageBuffer) {
        delete[] imageBuffer;
        imageBuffer = nullptr;
    }
    
    // Allocate new buffer
    size_t bufferSize = width * height * 3; // RGB data
    imageBuffer = new (std::nothrow) uint8_t[bufferSize];
    
    if (imageBuffer && data) {
        memcpy(imageBuffer, data, bufferSize);
        imageWidth = width;
        imageHeight = height;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Image loaded: ");
        DEBUG_SERIAL.print(width);
        DEBUG_SERIAL.print("x");
        DEBUG_SERIAL.println(height);
        #endif
    } else if (!imageBuffer) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Failed to allocate image buffer");
        #endif
        imageWidth = 0;
        imageHeight = 0;
    }
}

SDError POVEngine::loadImageFromSD(const char* filename, SDStorageManager* sdStorage) {
    if (!sdStorage || !sdStorage->isInitialized()) {
        return SD_ERROR_NOT_INITIALIZED;
    }
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Loading image from SD: ");
    DEBUG_SERIAL.println(filename);
    #endif
    
    // Get image info first to allocate proper buffer
    size_t width, height, fileSize;
    SDError error = sdStorage->getImageInfo(filename, width, height, fileSize);
    
    if (error != SD_OK) {
        return error;
    }
    
    // Free existing buffer if any
    if (imageBuffer) {
        delete[] imageBuffer;
        imageBuffer = nullptr;
    }
    
    // Allocate buffer for image data
    size_t bufferSize = width * height * 3;
    imageBuffer = new (std::nothrow) uint8_t[bufferSize];
    
    if (!imageBuffer) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Failed to allocate image buffer");
        #endif
        imageWidth = 0;
        imageHeight = 0;
        return SD_ERROR_OUT_OF_MEMORY;
    }
    
    // Load image data from SD card
    size_t loadedWidth, loadedHeight;
    error = sdStorage->loadImage(filename, imageBuffer, bufferSize, loadedWidth, loadedHeight);
    
    if (error == SD_OK) {
        imageWidth = loadedWidth;
        imageHeight = loadedHeight;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Image loaded from SD successfully");
        #endif
    } else {
        // Clean up on error
        delete[] imageBuffer;
        imageBuffer = nullptr;
        imageWidth = 0;
        imageHeight = 0;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Failed to load image from SD");
        #endif
    }
    
    return error;
}

void POVEngine::setRotationSpeed(float rpm) {
    rotationSpeed = rpm;
}

void POVEngine::setMode(uint8_t mode) {
    displayMode = mode;
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Display mode set to: ");
    DEBUG_SERIAL.println(mode);
    #endif
}

void POVEngine::setModeIndex(uint8_t index) {
    modeIndex = index;
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Mode index set to: ");
    DEBUG_SERIAL.println(index);
    #endif
}

void POVEngine::setEnabled(bool en) {
    enabled = en;
    
    if (!enabled) {
        leds.clear();
        leds.show();
    }
}

void POVEngine::loadPattern(uint8_t index, const Pattern& pattern) {
    if (index < 5) {
        patterns[index] = pattern;
        patterns[index].active = true;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Pattern ");
        DEBUG_SERIAL.print(index);
        DEBUG_SERIAL.print(" loaded, type: ");
        DEBUG_SERIAL.println(pattern.type);
        #endif
    }
}

void POVEngine::setPattern(uint8_t index) {
    if (index < 5) {
        modeIndex = index;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Pattern index set to: ");
        DEBUG_SERIAL.println(index);
        #endif
    }
}

void POVEngine::setFrameDelay(uint8_t delayMs) {
    frameDelay = delayMs;
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Frame delay set to: ");
    DEBUG_SERIAL.print(delayMs);
    DEBUG_SERIAL.println(" ms");
    #endif
}

uint16_t POVEngine::getColumnForAngle(uint16_t angle) {
    if (imageWidth == 0) {
        return 0;
    }
    
    // Map angle (0-359) to column (0 to imageWidth-1)
    return (angle * imageWidth) / 360;
}

void POVEngine::renderColumn(uint16_t column) {
    if (!imageBuffer || column >= imageWidth) {
        return;
    }
    
    // Render column of pixels to LED strip
    // LED 0 is for level shifting, LEDs 1-31 are for display
    for (uint16_t y = 0; y < imageHeight && y < leds.getNumLEDs() - 1; y++) {
        size_t pixelIndex = (y * imageWidth + column) * 3;
        uint8_t r = imageBuffer[pixelIndex];
        uint8_t g = imageBuffer[pixelIndex + 1];
        uint8_t b = imageBuffer[pixelIndex + 2];
        leds.setPixel(y + 1, r, g, b);  // +1 to skip LED 0
    }
    
    leds.show();
}

void POVEngine::renderPattern() {
    if (modeIndex >= 5 || !patterns[modeIndex].active) {
        leds.clear();
        leds.show();
        return;
    }
    
    const Pattern& pattern = patterns[modeIndex];
    patternTime++;
    
    switch (pattern.type) {
        case PATTERN_RAINBOW:
            renderRainbowPattern(pattern);
            break;
            
        case PATTERN_WAVE:
            renderWavePattern(pattern);
            break;
            
        case PATTERN_GRADIENT:
            renderGradientPattern(pattern);
            break;
            
        case PATTERN_SPARKLE:
            renderSparklePattern(pattern);
            break;
            
        default:
            leds.clear();
            break;
    }
    
    leds.show();
}

void POVEngine::renderRainbowPattern(const Pattern& pattern) {
    // Rainbow pattern - rotating hue across LEDs
    for (uint16_t i = 1; i < leds.getNumLEDs(); i++) {
        // Calculate hue based on position and time
        uint8_t hue = (patternTime * pattern.speed / 10 + i * 255 / (leds.getNumLEDs() - 1)) % 256;
        
        // Convert HSV to RGB (simplified)
        uint8_t sector = hue / 43;  // 0-5
        uint8_t offset = (hue % 43) * 6;  // 0-255
        
        uint8_t r, g, b;
        switch (sector) {
            case 0:  // Red to yellow
                r = 255; g = offset; b = 0;
                break;
            case 1:  // Yellow to green
                r = 255 - offset; g = 255; b = 0;
                break;
            case 2:  // Green to cyan
                r = 0; g = 255; b = offset;
                break;
            case 3:  // Cyan to blue
                r = 0; g = 255 - offset; b = 255;
                break;
            case 4:  // Blue to magenta
                r = offset; g = 0; b = 255;
                break;
            default:  // Magenta to red
                r = 255; g = 0; b = 255 - offset;
                break;
        }
        
        leds.setPixel(i, r, g, b);
    }
}

void POVEngine::renderWavePattern(const Pattern& pattern) {
    // Wave pattern - animated sine wave
    for (uint16_t i = 1; i < leds.getNumLEDs(); i++) {
        // Calculate brightness using sine wave approximation
        float angle = (patternTime * pattern.speed / 10.0 + i * 255.0 / (leds.getNumLEDs() - 1)) * 0.0245; // Convert to radians
        float sinValue = sin(angle);
        uint8_t brightness = (uint8_t)((sinValue + 1.0) * 127.5);  // Convert -1..1 to 0..255
        
        // Apply brightness to color1
        uint8_t r = (pattern.r1 * brightness) / 255;
        uint8_t g = (pattern.g1 * brightness) / 255;
        uint8_t b = (pattern.b1 * brightness) / 255;
        
        leds.setPixel(i, r, g, b);
    }
}

void POVEngine::renderGradientPattern(const Pattern& pattern) {
    // Gradient pattern - smooth transition between two colors
    for (uint16_t i = 1; i < leds.getNumLEDs(); i++) {
        // Calculate blend factor (0-255)
        uint8_t blend = (i * 255) / (leds.getNumLEDs() - 1);
        
        // Blend between color1 and color2
        uint8_t r = pattern.r1 + ((pattern.r2 - pattern.r1) * blend) / 255;
        uint8_t g = pattern.g1 + ((pattern.g2 - pattern.g1) * blend) / 255;
        uint8_t b = pattern.b1 + ((pattern.b2 - pattern.b1) * blend) / 255;
        
        leds.setPixel(i, r, g, b);
    }
}

void POVEngine::renderSparklePattern(const Pattern& pattern) {
    // Sparkle pattern - random sparkles that fade
    // Fade all LEDs
    for (uint16_t i = 1; i < leds.getNumLEDs(); i++) {
        uint8_t r, g, b;
        leds.getPixel(i, r, g, b);
        r = (r * 230) / 255;  // Fade to ~90%
        g = (g * 230) / 255;
        b = (b * 230) / 255;
        leds.setPixel(i, r, g, b);
    }
    
    // Add random sparkles based on speed
    if (random(256) < pattern.speed) {
        uint16_t led = random(1, leds.getNumLEDs());
        leds.setPixel(led, pattern.r1, pattern.g1, pattern.b1);
    }
}
