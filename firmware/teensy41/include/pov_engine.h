#ifndef POV_ENGINE_H
#define POV_ENGINE_H

#include <Arduino.h>
#include "config.h"
#include "led_driver.h"
#include "sd_storage.h"

// Forward declaration
class SDStorageManager;

// Pattern types
enum PatternType {
    PATTERN_RAINBOW = 0,
    PATTERN_WAVE = 1,
    PATTERN_GRADIENT = 2,
    PATTERN_SPARKLE = 3
};

// Pattern data structure
struct Pattern {
    uint8_t type;
    uint8_t r1, g1, b1;  // Color 1
    uint8_t r2, g2, b2;  // Color 2
    uint8_t speed;
    bool active;
};

class POVEngine {
public:
    POVEngine(LEDDriver& ledDriver);
    
    // Initialize POV engine
    void begin();
    
    // Update POV display based on current rotation
    void update();
    
    // Load image data for POV display
    void loadImageData(const uint8_t* data, size_t width, size_t height);
    
    // Load image from SD card
    SDError loadImageFromSD(const char* filename, SDStorageManager* sdStorage);
    
    // Set rotation speed (RPM)
    void setRotationSpeed(float rpm);
    
    // Get current angle (0-359 degrees)
    uint16_t getCurrentAngle() const { return currentAngle; }
    
    // Set display mode (0=idle, 1=image, 2=pattern, 3=sequence, 4=live)
    void setMode(uint8_t mode);
    
    // Set mode index (for selecting which image/pattern/sequence)
    void setModeIndex(uint8_t index);
    
    // Enable/disable POV engine
    void setEnabled(bool enabled);
    
    // Pattern management
    void loadPattern(uint8_t index, const Pattern& pattern);
    void setPattern(uint8_t index);
    
    // Frame rate control
    void setFrameDelay(uint8_t delayMs);
    uint8_t getFrameDelay() const { return frameDelay; }

private:
    LEDDriver& leds;
    uint8_t* imageBuffer;
    size_t imageWidth;
    size_t imageHeight;
    uint16_t currentAngle;
    float rotationSpeed;
    uint8_t displayMode;
    uint8_t modeIndex;
    bool enabled;
    unsigned long lastUpdateTime;
    unsigned long lastFrameTime;
    uint8_t frameDelay;  // Delay in milliseconds between frames
    
    // Pattern storage
    Pattern patterns[5];  // Support up to 5 patterns
    uint32_t patternTime;
    
    // Calculate column to display based on rotation angle
    uint16_t getColumnForAngle(uint16_t angle);
    
    // Render current column
    void renderColumn(uint16_t column);
    
    // Pattern rendering
    void renderPattern();
    void renderRainbowPattern(const Pattern& pattern);
    void renderWavePattern(const Pattern& pattern);
    void renderGradientPattern(const Pattern& pattern);
    void renderSparklePattern(const Pattern& pattern);
};

#endif // POV_ENGINE_H
