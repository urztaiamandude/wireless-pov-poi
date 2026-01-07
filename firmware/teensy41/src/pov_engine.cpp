#include "pov_engine.h"
#include <new>

POVEngine::POVEngine(LEDDriver& ledDriver) 
    : leds(ledDriver), 
      imageBuffer(nullptr), 
      imageWidth(0), 
      imageHeight(0),
      currentAngle(0),
      rotationSpeed(0.0),
      displayMode(0),
      enabled(false),
      lastUpdateTime(0) {
}

void POVEngine::begin() {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("POV Engine initialized");
    #endif
}

void POVEngine::update() {
    if (!enabled || !imageBuffer) {
        return;
    }
    
    // Update current angle based on rotation speed and time
    // This is a simulation for now - in real hardware, this would be
    // driven by accelerometer
    unsigned long currentTime = millis();
    if (lastUpdateTime > 0 && rotationSpeed > 0) {
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0; // seconds
        float degreesPerSecond = rotationSpeed * 6.0; // RPM to degrees/sec
        float angleDelta = degreesPerSecond * deltaTime;
        currentAngle = (currentAngle + (uint16_t)angleDelta) % 360;
    }
    lastUpdateTime = currentTime;
    
    // Render the current column based on angle
    uint16_t column = getColumnForAngle(currentAngle);
    renderColumn(column);
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

void POVEngine::setRotationSpeed(float rpm) {
    rotationSpeed = rpm;
}

void POVEngine::setMode(uint8_t mode) {
    displayMode = mode;
}

void POVEngine::setEnabled(bool en) {
    enabled = en;
    
    if (!enabled) {
        leds.clear();
        leds.show();
    }
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
    for (uint16_t y = 0; y < imageHeight && y < leds.getNumLEDs(); y++) {
        size_t pixelIndex = (y * imageWidth + column) * 3;
        uint8_t r = imageBuffer[pixelIndex];
        uint8_t g = imageBuffer[pixelIndex + 1];
        uint8_t b = imageBuffer[pixelIndex + 2];
        leds.setPixel(y, r, g, b);
    }
    
    leds.show();
}
