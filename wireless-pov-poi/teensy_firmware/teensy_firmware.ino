/*
 * Nebula Poi - Teensy 4.1 Firmware
 * 
 * This firmware controls a 32 LED APA102 strip for POV (Persistence of Vision) display.
 * LED 0 is used for level shifting, LEDs 1-31 are used for display.
 * Communicates with ESP32 via Serial1 to receive images, patterns, and sequences.
 * 
 * Hardware:
 * - Teensy 4.1
 * - APA102 LED Strip (32 LEDs)
 * - ESP32 connected via Serial1 (RX1=0, TX1=1)
 * - Optional: microSD card in Teensy 4.1 built-in slot (for SD_SUPPORT)
 */

#include <FastLED.h>

// SD Card Support - Uncomment to enable SD card features
// Requires microSD card in Teensy 4.1's built-in SD card slot
//#define SD_SUPPORT

#ifdef SD_SUPPORT
  #include <SD.h>
  #include <SPI.h>
#endif

// LED Configuration
// ⚠️ CRITICAL: LED 0 is used for level shifting only - NEVER use for display!
// All display loops MUST start from index 1, not 0.
// Example: for (int i = 1; i < NUM_LEDS; i++) { leds[i] = color; }
#define NUM_LEDS 32
#define DATA_PIN 11
#define CLOCK_PIN 13
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define DISPLAY_LEDS 31       // LEDs 1-31 used for display (LED 0 for level shifting)
#define DISPLAY_LED_START 1   // First LED index used for display content

// Audio Input Configuration (for music reactive patterns)
#define AUDIO_PIN A0         // Analog microphone input
#define AUDIO_SAMPLES 64     // Samples for averaging
#define AUDIO_NOISE_FLOOR 50 // Minimum threshold to filter noise

// Communication
#define SERIAL_BAUD 115200
#define ESP32_SERIAL Serial1

// Display Configuration
// NOTE: IMAGE_HEIGHT = DISPLAY_LEDS = 31 (fixed, matches physical LEDs)
//       IMAGE_MAX_WIDTH = variable (calculated from aspect ratio)
#define MAX_IMAGES 10
#define IMAGE_HEIGHT 31         // Fixed: matches DISPLAY_LEDS (one pixel per LED)
#define IMAGE_MAX_WIDTH 200     // Maximum width for stored images
#define MAX_PATTERNS 16  // 0-15: rainbow, wave, gradient, sparkle, fire, comet, breathing, strobe, meteor, wipe, plasma, music VU, music pulse, music rainbow, music center, music sparkle
#define MAX_SEQUENCES 5

#ifdef SD_SUPPORT
  // SD Card Configuration
  #define SD_IMAGE_DIR "/poi_images"
  #define MAX_FILENAME_LEN 32
  #define MAX_SD_FILES 10
  #define MAX_FILEPATH_LEN 64
#endif

// LED Array
CRGB leds[NUM_LEDS];

// Image storage structure
struct POVImage {
  uint8_t width;
  uint8_t height;
  CRGB pixels[IMAGE_WIDTH][IMAGE_HEIGHT];  // Max 31x64 image
  bool active;
};

// Pattern structure
// Pattern types (0-15):
//   Basic:  0=rainbow, 1=wave, 2=gradient, 3=sparkle, 4=fire, 5=comet
//           6=breathing, 7=strobe, 8=meteor, 9=wipe, 10=plasma
//   Music:  11=VU meter, 12=pulse, 13=rainbow, 14=center, 15=sparkle
struct Pattern {
  uint8_t type;   // Pattern type (0-15), see types above
  CRGB color1;    // Primary color for pattern
  CRGB color2;    // Secondary color for pattern
  uint8_t speed;  // Animation speed (1-255): higher = faster animation
                  //   Typical: 20-40 slow, 50-80 medium, 100+ fast
                  //   For strobe: controls flash rate
                  //   For sparkle: controls sparkle density
  bool active;    // Whether this pattern slot is in use
};

// Sequence structure
struct Sequence {
  uint8_t items[10];  // Image/pattern indices
  uint16_t durations[10];  // Duration in ms
  uint8_t count;
  bool active;
  bool loop;
};

// Storage arrays
POVImage images[MAX_IMAGES];
Pattern patterns[MAX_PATTERNS];
Sequence sequences[MAX_SEQUENCES];

// Display state
uint8_t currentMode = 0;  // 0=idle, 1=image, 2=pattern, 3=sequence, 4=live
uint8_t currentIndex = 0;
uint32_t lastUpdate = 0;
uint32_t frameDelay = 20;  // 50 FPS default
uint8_t currentColumn = 0;
bool displaying = false;

// Sequence state tracking
uint8_t currentSequenceItem = 0;
uint32_t sequenceStartTime = 0;
bool sequencePlaying = false;

// Live mode buffer
CRGB liveBuffer[DISPLAY_LEDS];

// Serial command buffer
// Buffer size calculation: 31 (width) × 64 (height) × 3 (RGB bytes) = 5,952 bytes
// Plus protocol overhead (~100 bytes): 0xFF start, cmd, len, 0xFE end markers
// Rounded up to 6144 for safety margin
#define CMD_BUFFER_SIZE 6144
uint8_t cmdBuffer[CMD_BUFFER_SIZE];
uint16_t cmdBufferIndex = 0;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("Teensy 4.1 Nebula Poi Initializing...");
  
  // Initialize ESP32 Serial
  ESP32_SERIAL.begin(SERIAL_BAUD);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();
  
  // Initialize storage
  initStorage();
  
  // Initialize SD card (if enabled)
  #ifdef SD_SUPPORT
    initSDCard();
  #endif
  
  // Startup animation
  startupAnimation();
  
  Serial.println("Teensy 4.1 Nebula Poi Ready!");
  Serial.println("Commands: IMAGE, PATTERN, SEQUENCE, LIVE, STATUS");
  #ifdef SD_SUPPORT
    Serial.println("SD Card support: ENABLED");
  #else
    Serial.println("SD Card support: DISABLED");
  #endif
}

void loop() {
  // Process serial commands from ESP32
  processSerialCommands();
  
  // Update display based on current mode
  if (millis() - lastUpdate >= frameDelay) {
    lastUpdate = millis();
    updateDisplay();
  }
}

void initStorage() {
  // Clear all storage
  for (int i = 0; i < MAX_IMAGES; i++) {
    images[i].active = false;
    images[i].width = 0;
    images[i].height = 0;
  }
  
  for (int i = 0; i < MAX_PATTERNS; i++) {
    patterns[i].active = false;
    patterns[i].type = 0;
    patterns[i].speed = 50;
  }
  
  for (int i = 0; i < MAX_SEQUENCES; i++) {
    sequences[i].active = false;
    sequences[i].count = 0;
    sequences[i].loop = false;
  }
  
  // Initialize with default patterns
  patterns[0].active = true;
  patterns[0].type = 0;  // Rainbow
  patterns[0].color1 = CRGB::Red;
  patterns[0].color2 = CRGB::Blue;
  patterns[0].speed = 50;
  
  patterns[1].active = true;
  patterns[1].type = 4;  // Fire
  patterns[1].color1 = CRGB::OrangeRed;
  patterns[1].color2 = CRGB::Yellow;
  patterns[1].speed = 120;
  
  patterns[2].active = true;
  patterns[2].type = 5;  // Comet
  patterns[2].color1 = CRGB::Cyan;
  patterns[2].color2 = CRGB::Blue;
  patterns[2].speed = 80;
  
  patterns[3].active = true;
  patterns[3].type = 6;  // Breathing
  patterns[3].color1 = CRGB::Purple;
  patterns[3].color2 = CRGB::Black;
  patterns[3].speed = 60;
  
  patterns[4].active = true;
  patterns[4].type = 10;  // Plasma
  patterns[4].color1 = CRGB::Green;
  patterns[4].color2 = CRGB::Magenta;
  patterns[4].speed = 40;
}

void startupAnimation() {
  // Rainbow sweep animation
  for (int hue = 0; hue < 256; hue += 4) {
    for (int i = 1; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue + (i * 8), 255, 255);
    }
    FastLED.show();
    delay(10);
  }
  FastLED.clear();
  FastLED.show();
}

void processSerialCommands() {
  while (ESP32_SERIAL.available()) {
    uint8_t byte = ESP32_SERIAL.read();
    
    // Start of command marker
    if (byte == 0xFF && cmdBufferIndex == 0) {
      cmdBuffer[cmdBufferIndex++] = byte;
    }
    // Continue building command
    else if (cmdBufferIndex > 0) {
      cmdBuffer[cmdBufferIndex++] = byte;
      
      // Prevent buffer overflow
      if (cmdBufferIndex >= CMD_BUFFER_SIZE) {
        Serial.println("WARNING: Command buffer overflow, resetting");
        cmdBufferIndex = 0;
        continue;
      }
      
      // Check if we have end marker (commands vary in length)
      if (byte == 0xFE && cmdBufferIndex >= 4) {
        parseCommand();
        cmdBufferIndex = 0;
      }
    }
  }
}

void parseCommand() {
  if (cmdBuffer[0] != 0xFF) return;
  
  uint8_t cmd = cmdBuffer[1];
  
  // Note: dataLen interpretation depends on command type
  // For image command (0x02), bytes 2-3 are 16-bit length
  // For other commands, byte 2 is 8-bit length
  uint16_t dataLen = cmdBuffer[2];  // Used for simple commands
  
  Serial.print("Command received: 0x");
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case 0x01:  // Set mode
      if (dataLen >= 2) {
        uint8_t newMode = cmdBuffer[3];
        uint8_t newIndex = cmdBuffer[4];
        
        // Reset sequence state when changing modes
        if (newMode != currentMode || newIndex != currentIndex) {
          sequencePlaying = false;
          currentSequenceItem = 0;
          sequenceStartTime = 0;
        }
        
        currentMode = newMode;
        currentIndex = newIndex;
        Serial.print("Mode set to: ");
        Serial.println(currentMode);
      }
      sendAck(cmd);
      break;
      
    case 0x02:  // Upload image (uses 16-bit length in bytes 2-3)
      receiveImage();
      sendAck(cmd);
      break;
      
    case 0x03:  // Upload pattern
      receivePattern();
      sendAck(cmd);
      break;
      
    case 0x04:  // Upload sequence
      receiveSequence();
      sendAck(cmd);
      break;
      
    case 0x05:  // Live frame data
      receiveLiveFrame();
      break;
      
    case 0x06:  // Set brightness
      if (dataLen >= 1) {
        FastLED.setBrightness(cmdBuffer[3]);
        Serial.print("Brightness set to: ");
        Serial.println(cmdBuffer[3]);
      }
      sendAck(cmd);
      break;
      
    case 0x07:  // Set frame rate
      if (dataLen >= 1) {
        frameDelay = cmdBuffer[3];
        Serial.print("Frame delay set to: ");
        Serial.println(frameDelay);
      }
      sendAck(cmd);
      break;
      
    case 0x10:  // Status request
      sendStatus();
      break;
      
    #ifdef SD_SUPPORT
    case 0x20:  // Save image to SD
      saveImageToSD();
      break;
      
    case 0x21:  // Load image from SD
      loadImageFromSD();
      break;
      
    case 0x22:  // List SD images
      listSDImages();
      break;
      
    case 0x23:  // Delete image from SD
      deleteSDImage();
      break;
      
    case 0x30:  // Pattern preset commands (save/load/list/delete)
      handlePatternSDCommand();
      break;
    #endif
      
    default:
      Serial.println("Unknown command");
      break;
  }
}

void receiveImage() {
  // Parse image header from command buffer
  // Protocol: 0xFF 0x02 dataLen_high dataLen_low width height [RGB data...] 0xFE
  uint16_t dataLen = (cmdBuffer[2] << 8) | cmdBuffer[3];
  uint8_t srcWidth = cmdBuffer[4];
  uint8_t srcHeight = cmdBuffer[5];
  
  // Always store uploaded images in slot 0 (most recent upload)
  // This simplifies the web/app interface - they don't need to manage slots
  uint8_t imgIndex = 0;
  
  // Calculate expected data size
  uint16_t expectedBytes = 6 + srcWidth * srcHeight * 3 + 1; // header + pixels + end marker
  
  if (imgIndex >= MAX_IMAGES) {
    Serial.println("Error: Invalid image index");
    return;
  }
  
  if (cmdBufferIndex < expectedBytes) {
    Serial.print("Warning: Incomplete image data. Expected ");
    Serial.print(expectedBytes);
    Serial.print(", got ");
    Serial.println(cmdBufferIndex);
    // Continue anyway with what we have
  }
  
  Serial.print("Receiving image, source size: ");
  Serial.print(srcWidth);
  Serial.print("x");
  Serial.print(srcHeight);
  Serial.print(" (buffer has ");
  Serial.print(cmdBufferIndex);
  Serial.println(" bytes)");
  
  // If image is already 31 pixels wide, use it directly
  if (srcWidth == IMAGE_WIDTH && srcHeight <= 64) {
    Serial.println("Image is already POV-compatible size");
    images[imgIndex].width = srcWidth;
    images[imgIndex].height = srcHeight;
    images[imgIndex].active = true;
    
    // Read pixel data directly
    uint16_t pixelCount = srcWidth * srcHeight;
    for (uint16_t i = 0; i < pixelCount; i++) {
      uint16_t bufferPos = 6 + i * 3;
      // Ensure we have all 3 bytes for this pixel
      if (bufferPos + 2 < cmdBufferIndex - 1) { // -1 for end marker
        uint8_t x = i % srcWidth;
        uint8_t y = i / srcWidth;
        images[imgIndex].pixels[x][y] = CRGB(
          cmdBuffer[bufferPos],
          cmdBuffer[bufferPos + 1],
          cmdBuffer[bufferPos + 2]
        );
      } else {
        // Fill remaining with black if data is incomplete
        uint8_t x = i % srcWidth;
        uint8_t y = i / srcWidth;
        images[imgIndex].pixels[x][y] = CRGB::Black;
      }
    }
  } else {
    // Image needs conversion - resize to 31 pixels wide
    Serial.println("Converting image to POV format (31 pixels wide)");
    
    // Calculate target height maintaining aspect ratio
    uint8_t targetHeight = (uint16_t)srcHeight * IMAGE_WIDTH / srcWidth;
    if (targetHeight > 64) targetHeight = 64;
    if (targetHeight < 1) targetHeight = 1;
    
    Serial.print("Target size: ");
    Serial.print(IMAGE_WIDTH);
    Serial.print("x");
    Serial.println(targetHeight);
    
    images[imgIndex].width = IMAGE_WIDTH;
    images[imgIndex].height = targetHeight;
    images[imgIndex].active = true;
    
    // Perform nearest-neighbor resize
    for (uint8_t ty = 0; ty < targetHeight; ty++) {
      for (uint8_t tx = 0; tx < IMAGE_WIDTH; tx++) {
        // Map target pixel to source pixel
        uint8_t sx = (uint16_t)tx * srcWidth / IMAGE_WIDTH;
        uint8_t sy = (uint16_t)ty * srcHeight / targetHeight;
        
        // Get source pixel index
        uint16_t srcIndex = sy * srcWidth + sx;
        uint16_t bufferPos = 6 + srcIndex * 3;
        
        // Read and store pixel (with bounds checking)
        if (bufferPos + 2 < cmdBufferIndex - 1) { // -1 for end marker
          images[imgIndex].pixels[tx][ty] = CRGB(
            cmdBuffer[bufferPos],
            cmdBuffer[bufferPos + 1],
            cmdBuffer[bufferPos + 2]
          );
        } else {
          images[imgIndex].pixels[tx][ty] = CRGB::Black;
        }
      }
    }
  }
  
  Serial.println("Image received and processed successfully");
}

void receivePattern() {
  uint8_t patIndex = cmdBuffer[3];
  
  if (patIndex >= MAX_PATTERNS) return;
  
  patterns[patIndex].active = true;
  patterns[patIndex].type = cmdBuffer[4];
  patterns[patIndex].color1 = CRGB(cmdBuffer[5], cmdBuffer[6], cmdBuffer[7]);
  patterns[patIndex].color2 = CRGB(cmdBuffer[8], cmdBuffer[9], cmdBuffer[10]);
  patterns[patIndex].speed = cmdBuffer[11];
  
  Serial.print("Pattern ");
  Serial.print(patIndex);
  Serial.println(" received");
}

void receiveSequence() {
  uint8_t seqIndex = cmdBuffer[3];
  
  if (seqIndex >= MAX_SEQUENCES) return;
  
  sequences[seqIndex].active = true;
  sequences[seqIndex].count = cmdBuffer[4];
  sequences[seqIndex].loop = cmdBuffer[5];
  
  // Receive sequence items and durations
  for (int i = 0; i < sequences[seqIndex].count && i < 10; i++) {
    sequences[seqIndex].items[i] = cmdBuffer[6 + i * 3];
    sequences[seqIndex].durations[i] = (cmdBuffer[7 + i * 3] << 8) | cmdBuffer[8 + i * 3];
  }
  
  Serial.print("Sequence ");
  Serial.print(seqIndex);
  Serial.println(" received");
}

void receiveLiveFrame() {
  // Receive live frame data for immediate display
  // Each LED needs 3 bytes (RGB), starting at cmdBuffer[3]
  for (int i = 0; i < DISPLAY_LEDS && (3 + (i + 1) * 3 - 1) < CMD_BUFFER_SIZE; i++) {
    liveBuffer[i] = CRGB(cmdBuffer[3 + i * 3], cmdBuffer[4 + i * 3], cmdBuffer[5 + i * 3]);
  }
}

void updateDisplay() {
  // Clear LED 0 (level shifter)
  leds[0] = CRGB::Black;
  
  switch (currentMode) {
    case 0:  // Idle - off
      FastLED.clear();
      break;
      
    case 1:  // Display image
      displayImage();
      break;
      
    case 2:  // Display pattern
      displayPattern();
      break;
      
    case 3:  // Display sequence
      displaySequence();
      break;
      
    case 4:  // Live mode
      displayLive();
      break;
  }
  
  FastLED.show();
}

void displayImage() {
  if (currentIndex >= MAX_IMAGES || !images[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  POVImage& img = images[currentIndex];
  
  // Display current column of the image
  for (int i = 0; i < DISPLAY_LEDS && i < img.height; i++) {
    leds[i + 1] = img.pixels[currentColumn][i];
  }
  
  currentColumn = (currentColumn + 1) % img.width;
}

void displayPattern() {
  if (currentIndex >= MAX_PATTERNS || !patterns[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  Pattern& pat = patterns[currentIndex];
  static uint32_t patternTime = 0;
  patternTime++;
  
  switch (pat.type) {
    case 0:  // Rainbow
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t hue = (patternTime * pat.speed / 10 + i * 255 / DISPLAY_LEDS) % 256;
        leds[i] = CHSV(hue, 255, 255);
      }
      break;
      
    case 1:  // Wave
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t brightness = (sin8(patternTime * pat.speed / 10 + i * 255 / DISPLAY_LEDS));
        leds[i] = pat.color1;
        leds[i].nscale8(brightness);
      }
      break;
      
    case 2:  // Gradient
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t blend = (i * 255) / DISPLAY_LEDS;
        leds[i] = blend8(pat.color1, pat.color2, blend);
      }
      break;
      
    case 3:  // Sparkle
      if (random8() < pat.speed) {
        leds[random8(1, NUM_LEDS)] = pat.color1;
      }
      fadeToBlackBy(leds, NUM_LEDS, 20);
      break;
      
    case 4:  // Fire - heat rises from LED 1 upward
      {
        static uint8_t heat[NUM_LEDS];
        // Cool down every cell
        for (int i = 1; i < NUM_LEDS; i++) {
          heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / DISPLAY_LEDS) + 2));
        }
        // Heat rises - drift heat upward
        for (int i = NUM_LEDS - 1; i >= 2; i--) {
          heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
        }
        // Random ignition at the bottom
        if (random8() < pat.speed) {
          int y = random8(1, 4);
          heat[y] = qadd8(heat[y], random8(160, 255));
        }
        // Map heat to colors
        for (int i = 1; i < NUM_LEDS; i++) {
          leds[i] = HeatColor(heat[i]);
        }
      }
      break;
      
    case 5:  // Comet - single bright head with fading tail
      {
        static uint8_t cometPos = 1;
        static uint8_t direction = 1;
        fadeToBlackBy(leds, NUM_LEDS, 60);  // Fade creates tail
        cometPos += direction;
        if (cometPos >= NUM_LEDS - 1 || cometPos <= 1) {
          direction = -direction;
        }
        leds[cometPos] = pat.color1;
        leds[cometPos - direction] = pat.color1;
        leds[cometPos - direction].nscale8(128);
      }
      break;
      
    case 6:  // Breathing - smooth pulse on/off
      {
        uint8_t breath = beatsin8(pat.speed / 4, 20, 255);
        for (int i = 1; i < NUM_LEDS; i++) {
          leds[i] = pat.color1;
          leds[i].nscale8(breath);
        }
      }
      break;
      
    case 7:  // Strobe - quick flashes
      {
        static bool strobeOn = false;
        static uint32_t lastStrobe = 0;
        uint32_t strobeDelay = map(pat.speed, 1, 255, 100, 10);
        if (patternTime - lastStrobe > strobeDelay) {
          strobeOn = !strobeOn;
          lastStrobe = patternTime;
        }
        for (int i = 1; i < NUM_LEDS; i++) {
          leds[i] = strobeOn ? pat.color1 : CRGB::Black;
        }
      }
      break;
      
    case 8:  // Meteor - falling with random decay
      {
        static uint8_t meteorPos = NUM_LEDS - 1;
        // Fade all LEDs randomly for sparkly tail
        for (int i = 1; i < NUM_LEDS; i++) {
          if (random8() < 80) {
            leds[i].fadeToBlackBy(64);
          }
        }
        // Draw meteor head
        for (int i = 0; i < 4; i++) {
          if (meteorPos - i >= 1 && meteorPos - i < NUM_LEDS) {
            leds[meteorPos - i] = pat.color1;
            leds[meteorPos - i].nscale8(255 - (i * 60));
          }
        }
        meteorPos--;
        if (meteorPos < 1) meteorPos = NUM_LEDS - 1;
      }
      break;
      
    case 9:  // Color Wipe - progressive fill then clear
      {
        static uint8_t wipePos = 1;
        static bool filling = true;
        leds[wipePos] = filling ? pat.color1 : CRGB::Black;
        wipePos++;
        if (wipePos >= NUM_LEDS) {
          wipePos = 1;
          filling = !filling;
        }
      }
      break;
      
    case 10:  // Plasma - organic color mixing
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t hue = sin8(i * 10 + patternTime * pat.speed / 20) + 
                      sin8(i * 15 - patternTime * pat.speed / 15) +
                      sin8(patternTime * pat.speed / 10);
        leds[i] = CHSV(hue, 255, 255);
      }
      break;
      
    case 11:  // Music Reactive - VU meter style with beat detection
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint8_t peakLevel = 0;
        static uint8_t peakDecay = 0;
        static uint8_t beatHue = 0;
        
        // Read audio sample
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average and peak
        uint32_t sum = 0;
        uint16_t maxVal = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) {
          sum += audioSamples[i];
          if (audioSamples[i] > maxVal) maxVal = audioSamples[i];
        }
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level (0-255)
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Beat detection - sudden increase in level
        if (audioLevel > peakLevel + 30) {
          beatHue += 32;  // Shift color on beat
        }
        
        // Update peak with decay
        if (audioLevel > peakLevel) {
          peakLevel = audioLevel;
          peakDecay = 0;
        } else {
          peakDecay++;
          if (peakDecay > 5) {
            peakLevel = qsub8(peakLevel, 3);
          }
        }
        
        // Map audio level to number of LEDs to light
        uint8_t ledsToLight = map(audioLevel, 0, 255, 0, DISPLAY_LEDS);
        
        // Draw VU meter with color gradient
        for (int i = 1; i < NUM_LEDS; i++) {
          uint8_t ledIndex = i - 1;  // 0-30 for display
          if (ledIndex < ledsToLight) {
            // Gradient from green to yellow to red based on position
            uint8_t hue;
            if (ledIndex < DISPLAY_LEDS / 3) {
              hue = 96;  // Green
            } else if (ledIndex < 2 * DISPLAY_LEDS / 3) {
              hue = 64;  // Yellow
            } else {
              hue = 0;   // Red
            }
            // Add beat color shift
            hue = (hue + beatHue) % 256;
            leds[i] = CHSV(hue, 255, 255);
          } else {
            leds[i].fadeToBlackBy(50);  // Smooth fade
          }
        }
        
        // Draw peak indicator
        uint8_t peakPos = map(peakLevel, 0, 255, 1, NUM_LEDS - 1);
        if (peakPos >= 1 && peakPos < NUM_LEDS) {
          leds[peakPos] = CRGB::White;
        }
      }
      break;
      
    case 12:  // Music Pulse - whole strip pulses with beat
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint8_t pulseVal = 0;
        static uint8_t lastLevel = 0;
        
        // Read audio sample
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Beat detection - pulse up on beat
        if (audioLevel > lastLevel + 20 && audioLevel > 100) {
          pulseVal = 255;
        }
        lastLevel = audioLevel;
        
        // Apply pulse to all LEDs
        for (int i = 1; i < NUM_LEDS; i++) {
          leds[i] = pat.color1;
          leds[i].nscale8(pulseVal);
        }
        
        // Decay pulse
        pulseVal = scale8(pulseVal, 220);
      }
      break;
      
    case 13:  // Music Rainbow - audio controls rainbow speed
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint16_t rainbowOffset = 0;
        
        // Read audio sample
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Audio level controls rainbow speed
        rainbowOffset += map(audioLevel, 0, 255, 1, 20);
        
        // Draw rainbow with audio-controlled speed
        for (int i = 1; i < NUM_LEDS; i++) {
          uint8_t hue = (rainbowOffset / 4 + i * 255 / DISPLAY_LEDS) % 256;
          uint8_t brightness = constrain(audioLevel + 50, 50, 255);
          leds[i] = CHSV(hue, 255, brightness);
        }
      }
      break;
      
    case 14:  // Music Center - expands from center based on audio
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        
        // Read audio sample
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Map level to expansion from center
        uint8_t expansion = map(audioLevel, 0, 255, 0, DISPLAY_LEDS / 2);
        uint8_t center = NUM_LEDS / 2;
        
        // Fade all first
        fadeToBlackBy(leds, NUM_LEDS, 80);
        
        // Draw expanding from center
        for (int i = 0; i <= expansion; i++) {
          uint8_t hue = patternTime * pat.speed / 20 + i * 10;
          if (center + i < NUM_LEDS) leds[center + i] = CHSV(hue, 255, 255);
          if (center - i >= 1) leds[center - i] = CHSV(hue, 255, 255);
        }
      }
      break;
      
    case 15:  // Music Sparkle - sparkles intensity based on audio
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        
        // Read audio sample
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Fade existing
        fadeToBlackBy(leds, NUM_LEDS, 40);
        
        // Add sparkles based on audio - more audio = more sparkles
        uint8_t numSparkles = map(audioLevel, 0, 255, 0, 8);
        for (int s = 0; s < numSparkles; s++) {
          uint8_t pos = random8(1, NUM_LEDS);
          uint8_t hue = patternTime * 2 + random8(64);  // Shifting colors
          leds[pos] = CHSV(hue, 255, 255);
        }
      }
      break;
      
    default:
      FastLED.clear();
      break;
  }
}

void displaySequence() {
  // Get current sequence
  if (currentIndex >= MAX_SEQUENCES || !sequences[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  Sequence& seq = sequences[currentIndex];
  
  // Check if sequence has items
  if (seq.count == 0) {
    FastLED.clear();
    return;
  }
  
  // Initialize sequence playback on first call or mode switch
  if (!sequencePlaying) {
    currentSequenceItem = 0;
    sequenceStartTime = millis();
    sequencePlaying = true;
    Serial.print("Starting sequence ");
    Serial.print(currentIndex);
    Serial.print(", items: ");
    Serial.println(seq.count);
  }
  
  // Check if current item duration has elapsed
  uint32_t elapsedTime = millis() - sequenceStartTime;
  if (elapsedTime >= seq.durations[currentSequenceItem]) {
    // Move to next item
    currentSequenceItem++;
    sequenceStartTime = millis();
    
    Serial.print("Sequence item ");
    Serial.print(currentSequenceItem);
    Serial.print(" of ");
    Serial.println(seq.count);
    
    // Check if sequence is complete
    if (currentSequenceItem >= seq.count) {
      if (seq.loop) {
        // Loop back to start
        currentSequenceItem = 0;
        Serial.println("Sequence looping...");
      } else {
        // Sequence complete, stop playing
        sequencePlaying = false;
        currentSequenceItem = seq.count - 1;  // Stay on last item
        Serial.println("Sequence complete");
      }
    }
  }
  
  // Get the current item index
  uint8_t itemIndex = seq.items[currentSequenceItem];
  
  // Determine if item is an image or pattern (MSB indicates type)
  // Bit 7: 0 = image, 1 = pattern
  bool isPattern = (itemIndex & 0x80) != 0;
  uint8_t actualIndex = itemIndex & 0x7F;  // Remove type bit
  
  // Display current item
  if (isPattern) {
    // Display pattern
    if (actualIndex < MAX_PATTERNS && patterns[actualIndex].active) {
      // Temporarily set currentIndex for pattern display
      uint8_t savedIndex = currentIndex;
      currentIndex = actualIndex;
      displayPattern();
      currentIndex = savedIndex;
    } else {
      FastLED.clear();
    }
  } else {
    // Display image
    if (actualIndex < MAX_IMAGES && images[actualIndex].active) {
      // Temporarily set currentIndex for image display
      uint8_t savedIndex = currentIndex;
      currentIndex = actualIndex;
      displayImage();
      currentIndex = savedIndex;
    } else {
      FastLED.clear();
    }
  }
}

void displayLive() {
  // Display the live buffer
  for (int i = 0; i < DISPLAY_LEDS; i++) {
    leds[i + 1] = liveBuffer[i];
  }
}

void sendAck(uint8_t cmd) {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xAA);  // ACK
  ESP32_SERIAL.write(cmd);
  ESP32_SERIAL.write(0xFE);
}

void sendStatus() {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xBB);  // Status response
  ESP32_SERIAL.write(currentMode);
  ESP32_SERIAL.write(currentIndex);
  ESP32_SERIAL.write(0xFE);
}

// ==================== SD CARD FUNCTIONS ====================
#ifdef SD_SUPPORT

void initSDCard() {
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Failed!");
    Serial.println("Check that SD card is inserted");
    return;
  }
  
  Serial.println("OK");
  
  // Create image directory if it doesn't exist
  if (!SD.exists(SD_IMAGE_DIR)) {
    Serial.print("Creating directory: ");
    Serial.println(SD_IMAGE_DIR);
    SD.mkdir(SD_IMAGE_DIR);
  }
  
  // Print card info (with error handling)
  Serial.print("SD Card Type: ");
  if (SD.card) {
    switch (SD.card.type()) {
      case 0: Serial.println("UNKNOWN"); break;
      case 1: Serial.println("SD1"); break;
      case 2: Serial.println("SD2"); break;
      case 3: Serial.println("SDHC/SDXC"); break;
      default: Serial.println("ERROR"); break;
    }
  } else {
    Serial.println("ERROR: Cannot read card type");
  }
}

void saveImageToSD() {
  // Protocol: 0xFF 0x20 len filename_len [filename] img_index 0xFE
  // Save the specified image slot to SD card with given filename
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x20);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Get image index
  uint8_t imgIndex = cmdBuffer[4 + filenameLen];
  
  if (imgIndex >= MAX_IMAGES || !images[imgIndex].active) {
    Serial.println("Invalid image index");
    sendAck(0x20);
    return;
  }
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Saving image to: ");
  Serial.println(filepath);
  
  // Open file for writing
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    sendAck(0x20);
    return;
  }
  
  POVImage& img = images[imgIndex];
  
  // Write header: width (1 byte), height (1 byte)
  file.write(img.width);
  file.write(img.height);
  
  // Write pixel data (RGB, row by row)
  for (int x = 0; x < img.width; x++) {
    for (int y = 0; y < img.height; y++) {
      file.write(img.pixels[x][y].r);
      file.write(img.pixels[x][y].g);
      file.write(img.pixels[x][y].b);
    }
  }
  
  file.close();
  Serial.println("Image saved successfully");
  sendAck(0x20);
}

void loadImageFromSD() {
  // Protocol: 0xFF 0x21 len filename_len [filename] img_index 0xFE
  // Load image from SD card into specified image slot
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x21);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Get image index
  uint8_t imgIndex = cmdBuffer[4 + filenameLen];
  
  if (imgIndex >= MAX_IMAGES) {
    Serial.println("Invalid image index");
    sendAck(0x21);
    return;
  }
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Loading image from: ");
  Serial.println(filepath);
  
  // Open file for reading
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file");
    sendAck(0x21);
    return;
  }
  
  // Read header
  uint8_t width = file.read();
  uint8_t height = file.read();
  
  if (width > IMAGE_WIDTH || height > IMAGE_HEIGHT) {
    Serial.println("Image dimensions too large");
    file.close();
    sendAck(0x21);
    return;
  }
  
  // Set image properties
  images[imgIndex].width = width;
  images[imgIndex].height = height;
  images[imgIndex].active = true;
  
  // Read pixel data
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      images[imgIndex].pixels[x][y].r = file.read();
      images[imgIndex].pixels[x][y].g = file.read();
      images[imgIndex].pixels[x][y].b = file.read();
    }
  }
  
  file.close();
  Serial.print("Image loaded successfully (");
  Serial.print(width);
  Serial.print("x");
  Serial.print(height);
  Serial.println(")");
  sendAck(0x21);
}

void listSDImages() {
  // Protocol: 0xFF 0x22 len 0xFE
  // Response: 0xFF 0xCC count [name1_len name1 ...] 0xFE
  
  Serial.println("Listing SD images...");
  
  File dir = SD.open(SD_IMAGE_DIR);
  if (!dir) {
    Serial.println("Failed to open directory");
    // Send empty list
    ESP32_SERIAL.write(0xFF);
    ESP32_SERIAL.write(0xCC);  // List response
    ESP32_SERIAL.write(0);     // Count = 0
    ESP32_SERIAL.write(0xFE);
    return;
  }
  
  // Count .pov files
  uint8_t count = 0;
  char filenames[MAX_SD_FILES][MAX_FILENAME_LEN];  // Store up to MAX_SD_FILES filenames
  
  File entry;
  while ((entry = dir.openNextFile()) && count < MAX_SD_FILES) {
    if (!entry.isDirectory()) {
      const char* name = entry.name();
      // Check if file ends with .pov
      int len = strlen(name);
      if (len > 4 && strcmp(name + len - 4, ".pov") == 0) {
        // Copy filename without extension
        int nameLen = len - 4;
        if (nameLen > MAX_FILENAME_LEN - 1) nameLen = MAX_FILENAME_LEN - 1;
        strncpy(filenames[count], name, nameLen);
        filenames[count][nameLen] = '\0';
        count++;
      }
    }
    entry.close();
  }
  dir.close();
  
  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" images");
  
  // Send response
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xCC);  // List response
  ESP32_SERIAL.write(count);
  
  for (int i = 0; i < count; i++) {
    uint8_t nameLen = strlen(filenames[i]);
    ESP32_SERIAL.write(nameLen);
    ESP32_SERIAL.write(filenames[i], nameLen);
  }
  
  ESP32_SERIAL.write(0xFE);
}

void deleteSDImage() {
  // Protocol: 0xFF 0x23 len filename_len [filename] 0xFE
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x23);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Deleting image: ");
  Serial.println(filepath);
  
  if (SD.remove(filepath)) {
    Serial.println("Image deleted successfully");
    sendAck(0x23);
  } else {
    Serial.println("Failed to delete image");
    sendAck(0x23);
  }
}

// ==================== PATTERN PRESET FUNCTIONS ====================
#define SD_PATTERN_DIR "/poi_patterns"
#define PATTERN_FILE_MAGIC 0x50415431  // "PAT1" in hex

void savePatternPreset(const char* presetName) {
  // Save all active patterns to a preset file
  
  // Create pattern directory if it doesn't exist
  if (!SD.exists(SD_PATTERN_DIR)) {
    SD.mkdir(SD_PATTERN_DIR);
  }
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, presetName);
  
  Serial.print("Saving pattern preset to: ");
  Serial.println(filepath);
  
  // Open file for writing
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create pattern file");
    return;
  }
  
  // Write magic number
  uint32_t magic = PATTERN_FILE_MAGIC;
  file.write((uint8_t*)&magic, sizeof(magic));
  
  // Write number of patterns
  file.write(MAX_PATTERNS);
  
  // Write each pattern
  for (int i = 0; i < MAX_PATTERNS; i++) {
    Pattern& pat = patterns[i];
    file.write(pat.active ? 1 : 0);
    file.write(pat.type);
    file.write(pat.color1.r);
    file.write(pat.color1.g);
    file.write(pat.color1.b);
    file.write(pat.color2.r);
    file.write(pat.color2.g);
    file.write(pat.color2.b);
    file.write(pat.speed);
  }
  
  file.close();
  Serial.println("Pattern preset saved successfully");
}

bool loadPatternPreset(const char* presetName) {
  // Load patterns from a preset file
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, presetName);
  
  Serial.print("Loading pattern preset from: ");
  Serial.println(filepath);
  
  // Open file for reading
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open pattern file");
    return false;
  }
  
  // Read and verify magic number
  uint32_t magic = 0;
  file.read((uint8_t*)&magic, sizeof(magic));
  if (magic != PATTERN_FILE_MAGIC) {
    Serial.println("Invalid pattern file format");
    file.close();
    return false;
  }
  
  // Read number of patterns
  uint8_t patCount = file.read();
  
  // Read each pattern (up to our MAX_PATTERNS)
  for (int i = 0; i < patCount && i < MAX_PATTERNS; i++) {
    Pattern& pat = patterns[i];
    pat.active = file.read() != 0;
    pat.type = file.read();
    pat.color1.r = file.read();
    pat.color1.g = file.read();
    pat.color1.b = file.read();
    pat.color2.r = file.read();
    pat.color2.g = file.read();
    pat.color2.b = file.read();
    pat.speed = file.read();
  }
  
  file.close();
  Serial.println("Pattern preset loaded successfully");
  return true;
}

void listPatternPresets() {
  // List all pattern preset files on SD card
  
  if (!SD.exists(SD_PATTERN_DIR)) {
    Serial.println("No pattern presets directory");
    ESP32_SERIAL.write(0xCC);
    ESP32_SERIAL.write((uint8_t)0);
    ESP32_SERIAL.write(0xFE);
    return;
  }
  
  File root = SD.open(SD_PATTERN_DIR);
  if (!root) {
    Serial.println("Failed to open pattern directory");
    return;
  }
  
  char filenames[MAX_SD_FILES][MAX_FILENAME_LEN];
  int count = 0;
  
  while (count < MAX_SD_FILES) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    String name = entry.name();
    if (name.endsWith(".pat")) {
      // Remove extension and store
      name = name.substring(0, name.length() - 4);
      strncpy(filenames[count], name.c_str(), MAX_FILENAME_LEN - 1);
      filenames[count][MAX_FILENAME_LEN - 1] = '\0';
      count++;
      Serial.print("Found preset: ");
      Serial.println(filenames[count - 1]);
    }
    entry.close();
  }
  root.close();
  
  Serial.print("Total pattern presets: ");
  Serial.println(count);
  
  // Send list to ESP32
  ESP32_SERIAL.write(0xCD);  // Pattern list response
  ESP32_SERIAL.write(count);
  
  for (int i = 0; i < count; i++) {
    uint8_t nameLen = strlen(filenames[i]);
    ESP32_SERIAL.write(nameLen);
    ESP32_SERIAL.write(filenames[i], nameLen);
  }
  
  ESP32_SERIAL.write(0xFE);
}

void handlePatternSDCommand() {
  // Protocol for pattern commands:
  // Save: 0xFF 0x30 len 0x01 name_len [name] 0xFE
  // Load: 0xFF 0x30 len 0x02 name_len [name] 0xFE
  // List: 0xFF 0x30 len 0x03 0xFE
  // Delete: 0xFF 0x30 len 0x04 name_len [name] 0xFE
  
  uint8_t subCmd = cmdBuffer[3];
  
  switch (subCmd) {
    case 0x01: {  // Save
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        savePatternPreset(name);
      }
      sendAck(0x30);
      break;
    }
    case 0x02: {  // Load
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        loadPatternPreset(name);
      }
      sendAck(0x30);
      break;
    }
    case 0x03:  // List
      listPatternPresets();
      break;
    case 0x04: {  // Delete
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        char filepath[MAX_FILEPATH_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, name);
        SD.remove(filepath);
        Serial.print("Deleted pattern preset: ");
        Serial.println(name);
      }
      sendAck(0x30);
      break;
    }
    default:
      Serial.println("Unknown pattern SD command");
      sendAck(0x30);
  }
}

#endif  // SD_SUPPORT
