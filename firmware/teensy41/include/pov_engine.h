#ifndef POV_ENGINE_H
#define POV_ENGINE_H

#include <Arduino.h>
#include "config.h"
#include "led_driver.h"

class POVEngine {
public:
    POVEngine(LEDDriver& ledDriver);
    
    // Initialize POV engine
    void begin();
    
    // Update POV display based on current rotation
    void update();
    
    // Load image data for POV display
    void loadImageData(const uint8_t* data, size_t width, size_t height);
    
    // Set rotation speed (RPM)
    void setRotationSpeed(float rpm);
    
    // Get current angle (0-359 degrees)
    uint16_t getCurrentAngle() const { return currentAngle; }
    
    // Set display mode
    void setMode(uint8_t mode);
    
    // Enable/disable POV engine
    void setEnabled(bool enabled);

private:
    LEDDriver& leds;
    uint8_t* imageBuffer;
    size_t imageWidth;
    size_t imageHeight;
    uint16_t currentAngle;
    float rotationSpeed;
    uint8_t displayMode;
    bool enabled;
    unsigned long lastUpdateTime;
    
    // Calculate column to display based on rotation angle
    uint16_t getColumnForAngle(uint16_t angle);
    
    // Render current column
    void renderColumn(uint16_t column);
};

#endif // POV_ENGINE_H
