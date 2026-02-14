#include "pov_engine.h"
#include "sd_storage.h"
#include <new>
#include <cstring>
#include <FastLED.h>
#include <math.h>

POVEngine::POVEngine(LEDDriver& ledDriver) 
    : leds(ledDriver), 
      currentAngle(0),
      rotationSpeed(0.0),
      displayMode(0),
      modeIndex(0),
      enabled(false),
      lastUpdateTime(0),
      lastFrameTime(0),
      frameDelay(16),  // Default ~60 FPS
      currentColumn(0),
      patternTime(0),
      currentSequenceItem(0),
      sequenceStartTime(0),
      sequencePlaying(false) {
    
    // Initialize image storage
    for (int i = 0; i < MAX_IMAGES; i++) {
        images[i].data = nullptr;
        images[i].width = 0;
        images[i].height = 0;
        images[i].filename[0] = '\0';
        images[i].active = false;
    }
    
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

    // Initialize sequence storage
    for (int i = 0; i < 5; i++) {
        sequences[i].active = false;
        sequences[i].count = 0;
        sequences[i].loop = false;
        for (int j = 0; j < 10; j++) {
            sequences[i].items[j] = 0;
            sequences[i].durations[j] = 0;
        }
    }
}

void POVEngine::begin() {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("POV Engine initialized");
    #endif

    // Create a default demo sequence in slot 0 that cycles through
    // the first few patterns. This mirrors the Arduino firmware's
    // "Demo Mix" sequence but uses patterns only (no built-in images).
    Sequence demo;
    demo.active = true;
    demo.loop = true;
    demo.count = 5;
    // Use high bit to mark items as patterns (compatible with Arduino design)
    demo.items[0] = 0x80 | 0;  // Pattern 0
    demo.items[1] = 0x80 | 1;  // Pattern 1
    demo.items[2] = 0x80 | 2;  // Pattern 2
    demo.items[3] = 0x80 | 3;  // Pattern 3
    demo.items[4] = 0x80 | 4;  // Pattern 4 (if configured)
    for (int i = 0; i < demo.count; i++) {
        demo.durations[i] = 2000;  // 2 seconds per item
    }
    loadSequence(0, demo);

    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("Default demo sequence (index 0) initialized");
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
            if (modeIndex < MAX_IMAGES && images[modeIndex].active && images[modeIndex].data) {
                uint16_t column = getColumnForAngle(currentAngle, modeIndex);
                renderColumn(column, modeIndex);
            } else {
                leds.clear();
                leds.show();
            }
            break;
            
        case 2:  // Pattern mode
            renderPattern();
            break;
            
        case 3:  // Sequence mode
            renderSequence();
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
    // Store uploaded images in slot 0 by default (most recent upload)
    uint8_t slot = 0;
    
    // Free existing data in slot 0 if any
    freeImageSlot(slot);
    
    // Allocate new buffer
    size_t bufferSize = width * height * 3; // RGB data
    images[slot].data = new (std::nothrow) uint8_t[bufferSize];
    
    if (images[slot].data && data) {
        memcpy(images[slot].data, data, bufferSize);
        images[slot].width = width;
        images[slot].height = height;
        images[slot].filename[0] = '\0';  // Uploaded images have no filename
        images[slot].active = true;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Image loaded to slot ");
        DEBUG_SERIAL.print(slot);
        DEBUG_SERIAL.print(": ");
        DEBUG_SERIAL.print(width);
        DEBUG_SERIAL.print("x");
        DEBUG_SERIAL.println(height);
        #endif
    } else if (!images[slot].data) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Failed to allocate image buffer");
        #endif
        images[slot].active = false;
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
    
    // Check if image is already loaded (find by filename)
    uint8_t slot = findImageByFilename(filename);
    if (slot < MAX_IMAGES) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Image already loaded in slot ");
        DEBUG_SERIAL.println(slot);
        #endif
        return SD_OK;  // Already loaded
    }
    
    // Find a free slot or reuse slot 0
    slot = findFreeImageSlot();
    if (slot >= MAX_IMAGES) {
        // No free slots - reuse slot 0 (most recent upload)
        slot = 0;
        freeImageSlot(slot);
    }
    
    // Get image info first to allocate proper buffer
    size_t width, height, fileSize;
    SDError error = sdStorage->getImageInfo(filename, width, height, fileSize);
    
    if (error != SD_OK) {
        return error;
    }
    
    // Allocate buffer for image data
    size_t bufferSize = width * height * 3;
    images[slot].data = new (std::nothrow) uint8_t[bufferSize];
    
    if (!images[slot].data) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Failed to allocate image buffer");
        #endif
        return SD_ERROR_OUT_OF_MEMORY;
    }
    
    // Load image data from SD card
    size_t loadedWidth, loadedHeight;
    error = sdStorage->loadImage(filename, images[slot].data, bufferSize, loadedWidth, loadedHeight);
    
    if (error == SD_OK) {
        images[slot].width = loadedWidth;
        images[slot].height = loadedHeight;
        strncpy(images[slot].filename, filename, MAX_FILENAME_LEN);
        images[slot].filename[MAX_FILENAME_LEN] = '\0';
        images[slot].active = true;
        
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Image loaded from SD to slot ");
        DEBUG_SERIAL.print(slot);
        DEBUG_SERIAL.print(": ");
        DEBUG_SERIAL.print(loadedWidth);
        DEBUG_SERIAL.print("x");
        DEBUG_SERIAL.println(loadedHeight);
        #endif
    } else {
        // Clean up on error
        freeImageSlot(slot);
        
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

void POVEngine::loadSequence(uint8_t index, const Sequence& sequence) {
    if (index < 5) {
        sequences[index] = sequence;

        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Sequence ");
        DEBUG_SERIAL.print(index);
        DEBUG_SERIAL.print(" loaded, items: ");
        DEBUG_SERIAL.println(sequences[index].count);
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

uint16_t POVEngine::getColumnForAngle(uint16_t angle, uint8_t imageSlot) {
    if (imageSlot >= MAX_IMAGES || !images[imageSlot].active || images[imageSlot].width == 0) {
        return 0;
    }
    
    // Map angle (0-359) to column (0 to imageWidth-1)
    return (angle * images[imageSlot].width) / 360;
}

void POVEngine::renderColumn(uint16_t column, uint8_t imageSlot) {
    if (imageSlot >= MAX_IMAGES || !images[imageSlot].active || !images[imageSlot].data) {
        return;
    }
    
    POVImage& img = images[imageSlot];
    if (column >= img.width) {
        return;
    }
    
    // Render one image column onto the active display LED range.
    for (uint16_t y = 0; y < img.height && y < (leds.getNumLEDs() - DISPLAY_LED_START); y++) {
        size_t pixelIndex = (y * img.width + column) * 3;
        uint8_t r = img.data[pixelIndex];
        uint8_t g = img.data[pixelIndex + 1];
        uint8_t b = img.data[pixelIndex + 2];
        leds.setPixel(y + DISPLAY_LED_START, r, g, b);
    }
    
    // Advance column for next frame (for image mode rotation)
    currentColumn = (currentColumn + 1) % img.width;
    
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

        // Extended pattern IDs from ESP32 UI - distinct implementations
        case 4:   // Fire
            renderFirePattern(pattern);
            break;

        case 5:   // Comet
            renderCometPattern(pattern);
            break;

        case 6:   // Breathing
            renderBreathingPattern(pattern);
            break;

        case 7:   // Strobe
            renderStrobePattern(pattern);
            break;

        case 8:   // Meteor
            renderMeteorPattern(pattern);
            break;

        case 9:   // Wipe
            renderWipePattern(pattern);
            break;

        case 10:  // Plasma
            renderPlasmaPattern(pattern);
            break;

        // Audio-reactive patterns (IDs 11-15), driven by microphone input on AUDIO_PIN.
        case 11:  // VU Meter
        {
            static uint16_t audioSamples[AUDIO_SAMPLES] = {0};
            static uint8_t sampleIndex = 0;
            static uint8_t peakLevel = 0;
            static uint8_t peakDecay = 0;
            static uint8_t beatHue = 0;

            const uint16_t rawSample = analogRead(AUDIO_PIN);
            audioSamples[sampleIndex] = rawSample;
            sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;

            uint32_t sum = 0;
            for (int i = 0; i < AUDIO_SAMPLES; i++) {
                sum += audioSamples[i];
            }
            const uint16_t avg = sum / AUDIO_SAMPLES;

            int16_t level = abs((int16_t)rawSample - (int16_t)avg);
            level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
            const uint8_t audioLevel = map(level, 0, 512, 0, 255);
            const uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;

            if (audioLevel > peakLevel + 30) {
                beatHue += 32;
            }

            if (audioLevel > peakLevel) {
                peakLevel = audioLevel;
                peakDecay = 0;
            } else if (++peakDecay > 5) {
                peakLevel = qsub8(peakLevel, 3);
            }

            const uint8_t ledsToLight = map(audioLevel, 0, 255, 0, displayLEDCount);

            CRGB* ledsArray = leds.getLEDs();
            for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
                const uint16_t ledIndex = i - DISPLAY_LED_START;
                if (ledIndex < ledsToLight) {
                    uint8_t hue = 0;
                    if (ledIndex < displayLEDCount / 3) {
                        hue = 96;   // Green
                    } else if (ledIndex < (2 * displayLEDCount) / 3) {
                        hue = 64;   // Yellow
                    }
                    hue = (hue + beatHue) % 256;
                    ledsArray[i] = CHSV(hue, 255, 255);
                } else {
                    ledsArray[i].fadeToBlackBy(50);
                }
            }

            const uint16_t peakPos = map(peakLevel, 0, 255, DISPLAY_LED_START, leds.getNumLEDs() - 1);
            if (peakPos >= DISPLAY_LED_START && peakPos < leds.getNumLEDs()) {
                ledsArray[peakPos] = CRGB::White;
            }
            break;
        }

        case 12:  // Pulse
        {
            static uint16_t audioSamples[AUDIO_SAMPLES] = {0};
            static uint8_t sampleIndex = 0;
            static uint8_t pulseVal = 0;
            static uint8_t lastLevel = 0;

            const uint16_t rawSample = analogRead(AUDIO_PIN);
            audioSamples[sampleIndex] = rawSample;
            sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;

            uint32_t sum = 0;
            for (int i = 0; i < AUDIO_SAMPLES; i++) {
                sum += audioSamples[i];
            }
            const uint16_t avg = sum / AUDIO_SAMPLES;

            int16_t level = abs((int16_t)rawSample - (int16_t)avg);
            level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
            const uint8_t audioLevel = map(level, 0, 512, 0, 255);

            if (audioLevel > lastLevel + 20 && audioLevel > 100) {
                pulseVal = 255;
            }
            lastLevel = audioLevel;

            CRGB baseColor(pattern.r1, pattern.g1, pattern.b1);
            for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
                CRGB c = baseColor;
                c.nscale8(pulseVal);
                leds.setPixel(i, c);
            }

            pulseVal = scale8(pulseVal, 220);
            break;
        }

        case 13:  // Audio Rainbow
        {
            static uint16_t audioSamples[AUDIO_SAMPLES] = {0};
            static uint8_t sampleIndex = 0;
            static uint16_t rainbowOffset = 0;

            const uint16_t rawSample = analogRead(AUDIO_PIN);
            audioSamples[sampleIndex] = rawSample;
            sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;

            uint32_t sum = 0;
            for (int i = 0; i < AUDIO_SAMPLES; i++) {
                sum += audioSamples[i];
            }
            const uint16_t avg = sum / AUDIO_SAMPLES;

            int16_t level = abs((int16_t)rawSample - (int16_t)avg);
            level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
            const uint8_t audioLevel = map(level, 0, 512, 0, 255);
            const uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;

            rainbowOffset += map(audioLevel, 0, 255, 1, 20);

            for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
                const uint16_t ledIndex = i - DISPLAY_LED_START;
                const uint8_t hue = (rainbowOffset / 4 + ledIndex * 255 / displayLEDCount) % 256;
                const uint8_t brightness = constrain(audioLevel + 50, 50, 255);
                leds.setPixel(i, CHSV(hue, 255, brightness));
            }
            break;
        }

        case 14:  // Center Burst
        {
            static uint16_t audioSamples[AUDIO_SAMPLES] = {0};
            static uint8_t sampleIndex = 0;

            const uint16_t rawSample = analogRead(AUDIO_PIN);
            audioSamples[sampleIndex] = rawSample;
            sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;

            uint32_t sum = 0;
            for (int i = 0; i < AUDIO_SAMPLES; i++) {
                sum += audioSamples[i];
            }
            const uint16_t avg = sum / AUDIO_SAMPLES;

            int16_t level = abs((int16_t)rawSample - (int16_t)avg);
            level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
            const uint8_t audioLevel = map(level, 0, 512, 0, 255);
            const uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;

            const uint8_t expansion = map(audioLevel, 0, 255, 0, displayLEDCount / 2);
            const int16_t center = DISPLAY_LED_START + (displayLEDCount / 2);

            CRGB* ledsArray = leds.getLEDs();
            for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
                ledsArray[i].fadeToBlackBy(80);
            }

            for (uint8_t i = 0; i <= expansion; i++) {
                const uint8_t hue = patternTime * pattern.speed / 20 + i * 10;
                const int16_t right = center + i;
                const int16_t left = center - i;
                if (right < (int16_t)leds.getNumLEDs()) {
                    ledsArray[right] = CHSV(hue, 255, 255);
                }
                if (left >= DISPLAY_LED_START) {
                    ledsArray[left] = CHSV(hue, 255, 255);
                }
            }
            break;
        }

        case 15:  // Audio Sparkle
        {
            static uint16_t audioSamples[AUDIO_SAMPLES] = {0};
            static uint8_t sampleIndex = 0;

            const uint16_t rawSample = analogRead(AUDIO_PIN);
            audioSamples[sampleIndex] = rawSample;
            sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;

            uint32_t sum = 0;
            for (int i = 0; i < AUDIO_SAMPLES; i++) {
                sum += audioSamples[i];
            }
            const uint16_t avg = sum / AUDIO_SAMPLES;

            int16_t level = abs((int16_t)rawSample - (int16_t)avg);
            level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
            const uint8_t audioLevel = map(level, 0, 512, 0, 255);

            CRGB* ledsArray = leds.getLEDs();
            for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
                ledsArray[i].fadeToBlackBy(40);
            }

            const uint8_t numSparkles = map(audioLevel, 0, 255, 0, 8);
            for (uint8_t s = 0; s < numSparkles; s++) {
                const uint16_t pos = random8(DISPLAY_LED_START, leds.getNumLEDs());
                const uint8_t hue = patternTime * 2 + random8(64);
                ledsArray[pos] = CHSV(hue, 255, 255);
            }
            break;
        }
            
        default:
            leds.clear();
            break;
    }
    
    leds.show();
}

void POVEngine::renderRainbowPattern(const Pattern& pattern) {
    // Rainbow pattern - rotating hue across LEDs
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        // Calculate hue based on position and time
        uint16_t ledIndex = i - DISPLAY_LED_START;
        uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;
        uint8_t hue = (patternTime * pattern.speed / 10 + ledIndex * 255 / displayLEDCount) % 256;
        
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
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        // Calculate brightness using sine wave approximation
        uint16_t ledIndex = i - DISPLAY_LED_START;
        uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;
        float angle = (patternTime * pattern.speed / 10.0 + ledIndex * 255.0 / displayLEDCount) * 0.0245; // Convert to radians
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
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        // Calculate blend factor (0-255)
        uint16_t ledIndex = i - DISPLAY_LED_START;
        uint16_t displayLEDCount = leds.getNumLEDs() - DISPLAY_LED_START;
        uint8_t blend = (ledIndex * 255) / displayLEDCount;
        
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
    CRGB* ledsArray = leds.getLEDs();
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        ledsArray[i].fadeToBlackBy(25);  // Fade by ~10%
    }
    
    // Add random sparkles based on speed
    if (random8() < pattern.speed) {
        uint16_t led = random(DISPLAY_LED_START, leds.getNumLEDs());
        leds.setPixel(led, pattern.r1, pattern.g1, pattern.b1);
    }
}

void POVEngine::renderFirePattern(const Pattern& pattern) {
    // Fire pattern - heat simulation rising upward
    static uint8_t heat[NUM_LEDS];
    CRGB* ledsArray = leds.getLEDs();
    uint16_t displayLEDs = leds.getNumLEDs() - DISPLAY_LED_START;
    
    // Cool down every cell
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / displayLEDs) + 2));
    }
    
    // Heat rises - drift heat upward
    for (uint16_t i = leds.getNumLEDs() - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
    }
    
    // Random ignition at the bottom
    if (random8() < pattern.speed) {
        uint16_t y = random8(DISPLAY_LED_START, DISPLAY_LED_START + 3);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }
    
    // Map heat to colors using FastLED HeatColor
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        ledsArray[i] = HeatColor(heat[i]);
    }
}

void POVEngine::renderCometPattern(const Pattern& pattern) {
    // Comet pattern - single bright head with fading tail
    static int16_t cometPos = DISPLAY_LED_START;
    static int8_t direction = 1;
    CRGB* ledsArray = leds.getLEDs();
    
    // Fade creates tail (fade all LEDs)
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        ledsArray[i].fadeToBlackBy(60);
    }
    
    // Move comet
    cometPos += direction;
    if (cometPos >= (int16_t)leds.getNumLEDs() - 1 || cometPos <= DISPLAY_LED_START) {
        direction = -direction;
    }
    
    // Draw comet head and tail
    CRGB color(pattern.r1, pattern.g1, pattern.b1);
    if (cometPos >= DISPLAY_LED_START && cometPos < (int16_t)leds.getNumLEDs()) {
        ledsArray[cometPos] = color;
    }
    int16_t tailPos = cometPos - direction;
    if (tailPos >= DISPLAY_LED_START && tailPos < (int16_t)leds.getNumLEDs()) {
        ledsArray[tailPos] = color;
        ledsArray[tailPos].nscale8(128);
    }
}

void POVEngine::renderBreathingPattern(const Pattern& pattern) {
    // Breathing pattern - smooth pulse on/off
    // Use beatsin8 for smooth sine wave pulsing
    uint8_t breath = beatsin8(pattern.speed / 4, 20, 255);
    CRGB color(pattern.r1, pattern.g1, pattern.b1);
    
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        CRGB ledColor = color;
        ledColor.nscale8(breath);
        leds.setPixel(i, ledColor);
    }
}

void POVEngine::renderStrobePattern(const Pattern& pattern) {
    // Strobe pattern - quick flashes
    static bool strobeOn = false;
    static unsigned long lastStrobe = 0;
    
    // Map speed (1-255) to delay (100ms to 10ms)
    uint32_t strobeDelay = map(pattern.speed, 1, 255, 100, 10);
    
    if (millis() - lastStrobe > strobeDelay) {
        strobeOn = !strobeOn;
        lastStrobe = millis();
    }
    
    CRGB color = strobeOn ? CRGB(pattern.r1, pattern.g1, pattern.b1) : CRGB::Black;
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        leds.setPixel(i, color);
    }
}

void POVEngine::renderMeteorPattern(const Pattern& pattern) {
    // Meteor pattern - falling with random decay
    static int16_t meteorPos = NUM_LEDS - 1;
    CRGB* ledsArray = leds.getLEDs();
    CRGB color(pattern.r1, pattern.g1, pattern.b1);
    
    // Fade all LEDs randomly for sparkly tail
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        if (random8() < 80) {
            ledsArray[i].fadeToBlackBy(64);
        }
    }
    
    // Draw meteor head with fading tail
    for (uint8_t i = 0; i < 4; i++) {
        int16_t headPos = meteorPos - i;
        if (headPos >= DISPLAY_LED_START && headPos < (int16_t)leds.getNumLEDs()) {
            CRGB meteorColor = color;
            meteorColor.nscale8(255 - (i * 60));
            ledsArray[headPos] = meteorColor;
        }
    }
    
    // Move meteor down
    meteorPos--;
    if (meteorPos < DISPLAY_LED_START) {
        meteorPos = leds.getNumLEDs() - 1;
    }
}

void POVEngine::renderWipePattern(const Pattern& pattern) {
    // Color Wipe pattern - progressive fill then clear
    static uint8_t wipePos = DISPLAY_LED_START;
    static bool filling = true;
    CRGB* ledsArray = leds.getLEDs();
    CRGB color(pattern.r1, pattern.g1, pattern.b1);
    
    ledsArray[wipePos] = filling ? color : CRGB::Black;
    
    wipePos++;
    if (wipePos >= leds.getNumLEDs()) {
        wipePos = DISPLAY_LED_START;
        filling = !filling;
    }
}

void POVEngine::renderPlasmaPattern(const Pattern& pattern) {
    // Plasma pattern - organic color mixing
    CRGB* ledsArray = leds.getLEDs();
    
    for (uint16_t i = DISPLAY_LED_START; i < leds.getNumLEDs(); i++) {
        uint8_t hue = sin8(i * 10 + patternTime * pattern.speed / 20) + 
                      sin8(i * 15 - patternTime * pattern.speed / 15) +
                      sin8(patternTime * pattern.speed / 10);
        ledsArray[i] = CHSV(hue, 255, 255);
    }
}

void POVEngine::renderSequence() {
    // Ensure selected sequence is valid
    if (modeIndex >= 5 || !sequences[modeIndex].active) {
        leds.clear();
        leds.show();
        sequencePlaying = false;
        currentSequenceItem = 0;
        return;
    }

    Sequence& seq = sequences[modeIndex];

    // No items -> nothing to display
    if (seq.count == 0) {
        leds.clear();
        leds.show();
        sequencePlaying = false;
        currentSequenceItem = 0;
        return;
    }

    unsigned long now = millis();

    // Initialize playback on first entry or after mode/index changes
    if (!sequencePlaying) {
        currentSequenceItem = 0;
        sequenceStartTime = now;
        sequencePlaying = true;

        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Starting sequence ");
        DEBUG_SERIAL.print(modeIndex);
        DEBUG_SERIAL.print(", items: ");
        DEBUG_SERIAL.println(seq.count);
        #endif
    }

    // Check if current item duration has elapsed
    unsigned long elapsed = now - sequenceStartTime;
    if (elapsed >= seq.durations[currentSequenceItem]) {
        currentSequenceItem++;
        sequenceStartTime = now;

        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Sequence item ");
        DEBUG_SERIAL.print(currentSequenceItem);
        DEBUG_SERIAL.print(" of ");
        DEBUG_SERIAL.println(seq.count);
        #endif

        // End-of-sequence handling
        if (currentSequenceItem >= seq.count) {
            if (seq.loop) {
                currentSequenceItem = 0;
                #if DEBUG_ENABLED
                DEBUG_SERIAL.println("Sequence looping...");
                #endif
            } else {
                // Stop on last item
                sequencePlaying = false;
                currentSequenceItem = seq.count - 1;
            }
        }
    }

    // Display the current item
    uint8_t itemIndex = seq.items[currentSequenceItem];

    // MSB indicates whether this is a pattern (1) or image (0),
    // mirroring the Arduino firmware convention.
    // Images are stored in slots (0 to MAX_IMAGES-1) and can be referenced by sequence items.
    bool isPattern = (itemIndex & 0x80) != 0;
    uint8_t actualIndex = itemIndex & 0x7F;

    if (isPattern) {
        // Render one of the stored patterns
        if (actualIndex < 5 && patterns[actualIndex].active) {
            // Temporarily use this pattern index
            uint8_t savedIndex = modeIndex;
            modeIndex = actualIndex;
            renderPattern();
            modeIndex = savedIndex;
            return;
        }
    } else {
        // Image-type item: display image from slot
        if (actualIndex < MAX_IMAGES && images[actualIndex].active && images[actualIndex].data) {
            // Temporarily use this image index for rendering
            uint8_t savedIndex = modeIndex;
            modeIndex = actualIndex;
            uint16_t column = getColumnForAngle(currentAngle, actualIndex);
            renderColumn(column, actualIndex);
            modeIndex = savedIndex;
            return;
        } else {
            #if DEBUG_ENABLED
            DEBUG_SERIAL.print("Sequence item refers to image slot ");
            DEBUG_SERIAL.print(actualIndex);
            DEBUG_SERIAL.println(" which is not loaded");
            #endif
        }
    }

    // Fallback: clear if item is invalid/unimplemented
    leds.clear();
    leds.show();
}

uint8_t POVEngine::findImageByFilename(const char* filename) const {
    if (!filename || filename[0] == '\0') {
        return MAX_IMAGES;  // Not found
    }
    
    for (uint8_t i = 0; i < MAX_IMAGES; i++) {
        if (images[i].active && strcmp(images[i].filename, filename) == 0) {
            return i;
        }
    }
    
    return MAX_IMAGES;  // Not found
}

uint8_t POVEngine::findFreeImageSlot() {
    for (uint8_t i = 0; i < MAX_IMAGES; i++) {
        if (!images[i].active) {
            return i;
        }
    }
    return MAX_IMAGES;  // No free slots
}

void POVEngine::freeImageSlot(uint8_t slot) {
    if (slot >= MAX_IMAGES) {
        return;
    }
    
    if (images[slot].data) {
        delete[] images[slot].data;
        images[slot].data = nullptr;
    }
    
    images[slot].width = 0;
    images[slot].height = 0;
    images[slot].filename[0] = '\0';
    images[slot].active = false;
}
