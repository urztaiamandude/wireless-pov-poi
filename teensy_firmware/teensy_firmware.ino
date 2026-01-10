/*
 * Wireless POV POI - Teensy 4.1 Firmware
 * 
 * This firmware controls a 32 LED APA102 strip for POV (Persistence of Vision) display.
 * LED 0 is used for level shifting, LEDs 1-31 are used for display.
 * Communicates with ESP32 via Serial1 to receive images, patterns, and sequences.
 * 
 * Hardware:
 * - Teensy 4.1
 * - APA102 LED Strip (32 LEDs)
 * - ESP32 connected via Serial1 (RX1=0, TX1=1)
 */

#include <FastLED.h>

// LED Configuration
#define NUM_LEDS 32
#define DATA_PIN 11
#define CLOCK_PIN 13
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define DISPLAY_LEDS 31  // LEDs 1-31 used for display (LED 0 for level shifting)

// Communication
#define SERIAL_BAUD 115200
#define ESP32_SERIAL Serial1

// Display Configuration
#define MAX_IMAGES 10
#define IMAGE_WIDTH 31
#define MAX_PATTERNS 5
#define MAX_SEQUENCES 5

// LED Array
CRGB leds[NUM_LEDS];

// Image storage structure
struct POVImage {
  uint8_t width;
  uint8_t height;
  CRGB pixels[IMAGE_WIDTH][64];  // Max 31x64 image
  bool active;
};

// Pattern structure
struct Pattern {
  uint8_t type;  // 0=rainbow, 1=wave, 2=gradient, 3=sparkle, 4=custom
  CRGB color1;
  CRGB color2;
  uint8_t speed;
  bool active;
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

// Live mode buffer
CRGB liveBuffer[DISPLAY_LEDS];

// Serial command buffer
#define CMD_BUFFER_SIZE 256
uint8_t cmdBuffer[CMD_BUFFER_SIZE];
uint16_t cmdBufferIndex = 0;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("Teensy 4.1 POV POI Initializing...");
  
  // Initialize ESP32 Serial
  ESP32_SERIAL.begin(SERIAL_BAUD);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();
  
  // Initialize storage
  initStorage();
  
  // Startup animation
  startupAnimation();
  
  Serial.println("Teensy 4.1 POV POI Ready!");
  Serial.println("Commands: IMAGE, PATTERN, SEQUENCE, LIVE, STATUS");
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
  
  // Initialize with default pattern
  patterns[0].active = true;
  patterns[0].type = 0;  // Rainbow
  patterns[0].color1 = CRGB::Red;
  patterns[0].color2 = CRGB::Blue;
  patterns[0].speed = 50;
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
  uint16_t dataLen = cmdBuffer[2];
  
  Serial.print("Command received: 0x");
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case 0x01:  // Set mode
      if (dataLen >= 2) {
        currentMode = cmdBuffer[3];
        currentIndex = cmdBuffer[4];
        Serial.print("Mode set to: ");
        Serial.println(currentMode);
      }
      sendAck(cmd);
      break;
      
    case 0x02:  // Upload image
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
  uint8_t imgIndex = 0;  // Always use slot 0 for uploaded images
  
  // Calculate expected data size
  uint16_t expectedBytes = 6 + srcWidth * srcHeight * 3 + 1; // header + pixels + end marker
  
  if (imgIndex >= MAX_IMAGES) {
    Serial.println("Error: Invalid image index");
    return;
  }
  
  if (cmdBufferIndex < expectedBytes) {
    Serial.printf("Warning: Incomplete image data. Expected %d, got %d\n", 
                  expectedBytes, cmdBufferIndex);
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
  for (int i = 0; i < DISPLAY_LEDS && (6 + i * 3) < CMD_BUFFER_SIZE; i++) {
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
      
    default:
      FastLED.clear();
      break;
  }
}

void displaySequence() {
  // Sequence handling would cycle through multiple images/patterns
  // Simplified for now
  displayPattern();
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
