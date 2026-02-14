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
    PATTERN_SPARKLE = 3,
    PATTERN_FIRE = 4,
    PATTERN_COMET = 5,
    PATTERN_BREATHING = 6,
    PATTERN_STROBE = 7,
    PATTERN_METEOR = 8,
    PATTERN_WIPE = 9,
    PATTERN_PLASMA = 10,
    // Audio-reactive patterns (require MAX9814 microphone on AUDIO_PIN)
    PATTERN_AUDIO_VU_METER = 11,
    PATTERN_AUDIO_PULSE = 12,
    PATTERN_AUDIO_RAINBOW = 13,
    PATTERN_AUDIO_CENTER_BURST = 14,
    PATTERN_AUDIO_SPARKLE = 15,
    // Extended patterns
    PATTERN_SPLIT_SPIN = 16,
    PATTERN_THEATER_CHASE = 17
};

// Pattern data structure
struct Pattern {
    uint8_t type;
    uint8_t r1, g1, b1;  // Color 1
    uint8_t r2, g2, b2;  // Color 2
    uint8_t speed;
    bool active;
};

// Image data structure (stored in RAM slots)
struct POVImage {
    uint8_t* data;           // RGB pixel data (width * height * 3 bytes)
    size_t width;
    size_t height;
    char filename[MAX_FILENAME_LEN + 1];  // SD filename (empty for uploaded images)
    bool active;
};

// Sequence data structure (pattern/image timeline)
struct Sequence {
    uint8_t items[10];       // Item indices (MSB=1 => pattern, MSB=0 => image slot)
    uint16_t durations[10];  // Duration per item in ms
    uint8_t count;           // Number of valid items
    bool active;
    bool loop;
};

class POVEngine {
public:
    POVEngine(LEDDriver& ledDriver);
    
    // Initialize POV engine
    void begin();
    
    // Update POV display based on current rotation
    void update();
    
    // Load image data for POV display (stores in slot 0 by default)
    void loadImageData(const uint8_t* data, size_t width, size_t height);
    
    // Load image from SD card (stores with filename, finds or creates slot)
    SDError loadImageFromSD(const char* filename, SDStorageManager* sdStorage);
    
    // Find image slot by filename (returns slot index or MAX_IMAGES if not found)
    uint8_t findImageByFilename(const char* filename) const;
    
    // Set rotation speed (RPM)
    void setRotationSpeed(float rpm);
    
    // Get current angle (0-359 degrees)
    uint16_t getCurrentAngle() const { return currentAngle; }
    
    // Set display mode (0=idle, 1=image, 2=pattern, 3=sequence, 4=live)
    void setMode(uint8_t mode);
    uint8_t getMode() const { return displayMode; }
    
    // Set mode index (for selecting which image/pattern/sequence)
    void setModeIndex(uint8_t index);
    uint8_t getModeIndex() const { return modeIndex; }
    
    // Enable/disable POV engine
    void setEnabled(bool enabled);
    
    // Pattern management
    void loadPattern(uint8_t index, const Pattern& pattern);
    void setPattern(uint8_t index);

    // Sequence management
    void loadSequence(uint8_t index, const Sequence& sequence);
    
    // Frame rate control
    void setFrameDelay(uint8_t delayMs);
    uint8_t getFrameDelay() const { return frameDelay; }

private:
    LEDDriver& leds;
    uint16_t currentAngle;
    float rotationSpeed;
    uint8_t displayMode;
    uint8_t modeIndex;
    bool enabled;
    unsigned long lastUpdateTime;
    unsigned long lastFrameTime;
    uint8_t frameDelay;  // Delay in milliseconds between frames
    
    // Image storage (keyed by SD filename)
    POVImage images[MAX_IMAGES];
    uint16_t currentColumn;  // Current column for image display
    
    // Pattern storage
    Pattern patterns[5];  // Support up to 5 patterns
    uint32_t patternTime;

    // Sequence storage and playback state
    Sequence sequences[5];       // Support up to 5 sequences
    uint8_t currentSequenceItem;
    unsigned long sequenceStartTime;
    bool sequencePlaying;
    
    // Helper methods
    uint8_t findFreeImageSlot();
    void freeImageSlot(uint8_t slot);
    
    // Calculate column to display based on rotation angle
    uint16_t getColumnForAngle(uint16_t angle, uint8_t imageSlot);
    
    // Render current column
    void renderColumn(uint16_t column, uint8_t imageSlot);
    
    // Pattern rendering
    void renderPattern();
    void renderRainbowPattern(const Pattern& pattern);
    void renderWavePattern(const Pattern& pattern);
    void renderGradientPattern(const Pattern& pattern);
    void renderSparklePattern(const Pattern& pattern);
    void renderFirePattern(const Pattern& pattern);
    void renderCometPattern(const Pattern& pattern);
    void renderBreathingPattern(const Pattern& pattern);
    void renderStrobePattern(const Pattern& pattern);
    void renderMeteorPattern(const Pattern& pattern);
    void renderWipePattern(const Pattern& pattern);
    void renderPlasmaPattern(const Pattern& pattern);

    // Audio-reactive pattern rendering (MAX9814 microphone input)
    void renderAudioVUMeter(const Pattern& pattern);
    void renderAudioPulse(const Pattern& pattern);
    void renderAudioRainbow(const Pattern& pattern);
    void renderAudioCenterBurst(const Pattern& pattern);
    void renderAudioSparkle(const Pattern& pattern);

    // Extended pattern rendering
    void renderSplitSpinPattern(const Pattern& pattern);
    void renderTheaterChasePattern(const Pattern& pattern);

    // Audio processing helpers
    uint8_t readAudioLevel();   // Read and process audio from MAX9814
    uint16_t audioSamples[AUDIO_SAMPLES]; // Running sample buffer
    uint8_t audioSampleIndex;   // Current index into sample buffer
    uint8_t audioPeakLevel;     // Peak level with decay
    uint8_t audioPeakDecay;     // Decay counter for peak
    uint8_t audioBeatHue;       // Hue shift on beat detection

    // Sequence rendering
    void renderSequence();
};

#endif // POV_ENGINE_H
